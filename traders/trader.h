//
// Created by henry on 06/12/2021.
//

#ifndef CPPBAZAARBOT_TRADER_H
#define CPPBAZAARBOT_TRADER_H

#include <utility>

#include "inventory.h"
#include "../common/messages.h"

#include "../auction/auction_house.h"
#include "../metrics/metrics.h"

class Role;

namespace {
    double PositionInRange(double value, double min, double max) {
        value -= min;
        max -= min;
        min = 0;
        value = (value / (max - min));

        if (value < 0) { value = 0; }
        if (value > 1) { value = 1; }

        return value;
    };
}
class BasicTrader : public Agent {
private:
    std::shared_ptr<Role> logic;
    double MIN_PRICE = 0.01;
    int ticks = 0;
    friend AuctionHouse;
    Inventory _inventory;
    // TODO Change this to an array to allow multiple auction houses to be traded with at once
    std::shared_ptr<AuctionHouse> auction_house;

    std::vector<Message> inbox;
    std::vector<std::pair<int, Message>> outbox;

    std::map<std::string, std::vector<double>> _observedTradingRange;
    double profit_last_round = 0;
    int  external_lookback = 15; //history range (num ticks)
    int internal_lookback = 50; //history range (num trades)
    ConsoleLogger logger;

public:
    std::string class_name; // eg "Farmer", "Woodcutter" etc. Auction House will verify this on registration. (TODO)
    double money;

    bool destroyed = false;
    double money_last_round = 0;
    double profit = 0;

    double track_costs;

public:
    BasicTrader(int id, std::shared_ptr<AuctionHouse> auction_house_ptr, std::string  class_name, double starting_money, double inv_capacity, const std::vector<std::pair<Commodity, int>> &starting_inv, Log::LogLevel log_level = Log::WARN)
    : Agent(id)
    , auction_house(std::move(auction_house_ptr))
    , class_name(class_name)
    , money(starting_money)
    , logger(ConsoleLogger(class_name+std::to_string(id), log_level)){
        //construct inv
        std::vector<InventoryItem> inv_vector;
        for (const auto &item : starting_inv) {
            inv_vector.emplace_back(item.first.name, item.second);
            _observedTradingRange[item.first.name] = {};
        }
        _inventory = Inventory(inv_capacity, inv_vector);

        track_costs = 0;

        //assign logic

    }

    // Messaging functions
    void ReceiveMessage(Message incoming_message) override {
        logger.LogReceived(incoming_message.sender_id, Log::INFO, incoming_message.ToString());
        inbox.push_back(incoming_message);
    }
    void SendMessage(Message& outgoing_message, int recipient) override {
        outbox.emplace_back(recipient, std::move(outgoing_message));
    }

    void FlushOutbox() {
        logger.Log(Log::DEBUG, "Flushing outbox");
        while (!outbox.empty()) {
            auto& outgoing = outbox.back();
            outbox.pop_back();
            // Trader can currently only talk to auction houses (not other traders)
            if (outgoing.first != auction_house->id) {
                logger.Log(Log::ERROR, "Failed to send message, unknown recipient " + std::to_string(outgoing.first));
                continue;
            }
            logger.LogSent(outgoing.first, Log::DEBUG, outgoing.second.ToString());
            auction_house->ReceiveMessage(std::move(outgoing.second));
        }
        logger.Log(Log::DEBUG, "Flush finished");
    }
    void FlushInbox() {
        logger.Log(Log::DEBUG, "Flushing inbox");
        while (!inbox.empty()) {
            auto& incoming_message = inbox.back();
            if (incoming_message.GetType() == Msg::EMPTY) {
                //no-op
            } else if (incoming_message.GetType() == Msg::BID_RESULT) {
                ProcessBidResult(incoming_message);
            } else if (incoming_message.GetType() == Msg::ASK_RESULT) {
                ProcessAskResult(incoming_message);
            } else if (incoming_message.GetType() == Msg::REGISTER_RESPONSE) {
                ProcessRegistrationResponse(incoming_message);
            } else {
                std::cout << "Unknown/unsupported message type " << incoming_message.GetType() << std::endl;
            }
            inbox.pop_back();
        }
        logger.Log(Log::DEBUG, "Flush finished");
    }

    void ProcessBidResult(Message& message) {
            UpdatePriceModelFromBid(*message.bid_result);
    };
    void ProcessAskResult(Message& message) {
        UpdatePriceModelFromAsk(*message.ask_result);
    };
    void ProcessRegistrationResponse(Message& message) {};

    // Inventory functions
    bool HasMoney(double quantity) override {
        return (money >= quantity);
    }
    double TryTakeMoney(double quantity, bool atomic) override {
        double amount_transferred = 0;
        if (!atomic) {
            // Take what you can
            amount_transferred = std::min(money, quantity);
        } else {
            if (money < quantity) {
                logger.Log(Log::DEBUG, "Failed to take $"+std::to_string(quantity));
                amount_transferred = 0;
            } else {
                amount_transferred = quantity;
            }
        }
        money -= amount_transferred;
        return amount_transferred;
    }
    void ForceTakeMoney(double quantity) {
        money -= quantity;
    }
    void AddMoney(double quantity) override {
        money += quantity;
    }

    bool HasCommodity(std::string commodity, int quantity) override {
        auto stored = _inventory.Query(commodity);
        return (stored >= quantity);
    }
    int TryTakeCommodity(std::string commodity, int quantity, bool atomic) override {
        int actual_transferred = 0;
        auto comm = _inventory.GetItem(commodity);
        if (!comm) {
            //item unknown, fail
            logger.Log(Log::ERROR, "Tried to take unknown item "+commodity);
            return 0;
        }

        auto stored = _inventory.Query(commodity);
        if ( stored>= quantity) {
            actual_transferred = quantity;
        } else {
            if (atomic) {
                actual_transferred = 0;
                logger.Log(Log::DEBUG, "Failed to take "+commodity+std::string(" x") + std::to_string(quantity));
            } else {
                actual_transferred = stored;
            }
        }
        _inventory.TakeItem(commodity, actual_transferred);
        return actual_transferred;
    }
    int TryAddCommodity(std::string commodity, int quantity, bool atomic) override {
        int actual_transferred = 0;
        auto comm = _inventory.GetItem(commodity);
        if (!comm) {
            //item unknown, fail
            logger.Log(Log::ERROR, "Tried to add unknown item "+commodity);
            return 0;
        }

        if (_inventory.GetEmptySpace() >= quantity*comm->size) {
            actual_transferred = quantity;
        } else {
            if (atomic) {
                actual_transferred = 0;
                logger.Log(Log::DEBUG, "Failed to add "+commodity+std::string(" x") + std::to_string(quantity));
            } else {
                actual_transferred = (int) (_inventory.GetEmptySpace()/comm->size);
            }
        }
        _inventory.AddItem(commodity, actual_transferred);
        return actual_transferred;
    }

    int Query(const std::string& name) override { return _inventory.Query(name); }
    double GetEmptySpace() override { return _inventory.GetEmptySpace(); }

    // Trading functions
    void UpdatePriceModelFromBid(BidResult& result) {
            for (int i = 0; i < result.quantity_traded; i++) {
                _observedTradingRange[result.commodity].push_back(result.avg_price);
            }

            while (_observedTradingRange[result.commodity].size() > internal_lookback) {
                _observedTradingRange[result.commodity].erase(_observedTradingRange[result.commodity].begin());
            }
    };
    void UpdatePriceModelFromAsk(AskResult result){
        for (int i = 0; i < result.quantity_traded; i++) {
            _observedTradingRange[result.commodity].push_back(result.avg_price);
        }

        while (_observedTradingRange[result.commodity].size() > internal_lookback) {
            _observedTradingRange[result.commodity].erase(_observedTradingRange[result.commodity].begin());
        }
    };

    void GenerateOffers(std::string commodity) {
        int surplus = _inventory.Surplus(commodity);
        if (surplus >= 1) {
            auto offer = CreateAsk(commodity, 1);
            if (offer.quantity > 0) {
                SendMessage(*Message(id).AddAskOffer(offer), auction_house->id);
            }
        }

        int shortage = _inventory.Shortage(commodity);
        double space = _inventory.GetEmptySpace();
        double unit_size = _inventory.GetSize(commodity);

        if (shortage > 0 && space >= unit_size) {
            int limit = (shortage*unit_size <= space) ? shortage : (int) space/shortage;
            if (limit > 0)
            {
                auto offer = CreateBid(commodity, limit);
                if (offer.quantity > 0) {
                    SendMessage(*Message(id).AddBidOffer(offer), auction_house->id);
                }
            }
        }
    };
    BidOffer CreateBid(std::string commodity, int max_limit) {
        //AI agents offer a fair bid price - 5% above recent average market value
        double bid_price = 1.05*auction_house->AverageHistoricalPrice(commodity, external_lookback);
        int ideal = DetermineBuyQuantity(commodity);

        //can't buy more than limit
        int quantity = ideal > max_limit ? max_limit : ideal;
        //note that this could be a noop (quantity=0) at this point
        return BidOffer(id, commodity, quantity, bid_price);
    }
    AskOffer CreateAsk(std::string commodity, int min_limit) {
        //AI agents offer a fair ask price - costs + 2% profit
        double ask_price = _inventory.QueryCost(commodity) * 1.02;

        int quantity = DetermineSaleQuantity(commodity);
        //can't sell less than limit
        quantity = quantity < min_limit ? min_limit : quantity;
        return AskOffer(id, commodity, quantity, ask_price);
    };

    int DetermineBuyQuantity(std::string commodity) {
        double avg_price = auction_house->AverageHistoricalPrice(commodity, external_lookback);
        std::pair<double, double> range = ObserveTradingRange(commodity, internal_lookback);
        if (range.first == 0 && range.second == 0) {
            //uninitialised range
            logger.Log(Log::WARN, "Tried to make bid with unitialised trading range");
            return 0;
        }
        double favorability = PositionInRange(avg_price, range.first, range.second);
        favorability = 1 - favorability; //do 1 - favorability to see how close we are to the low end
        double amount_to_buy = favorability * _inventory.Shortage(commodity);//double

        return (int) amount_to_buy;
    }
    int DetermineSaleQuantity(std::string commodity) {
        return _inventory.Surplus(commodity); //Sell all surplus
    };

    std::pair<double, double> ObserveTradingRange(std::string commodity, int window) {
        if (_observedTradingRange.count(commodity) < 1 || _observedTradingRange[commodity].size() < 1) {
            return {0,0};
        }
        double min_observed = _observedTradingRange[commodity][0];
        double max_observed = _observedTradingRange[commodity][0];
        window = std::min(window, (int) _observedTradingRange[commodity].size());

        for (int i = 0; i < window; i++) {
            min_observed = std::min(min_observed,_observedTradingRange[commodity][i]);
            max_observed = std::max(max_observed,_observedTradingRange[commodity][i]);
        }
        return {min_observed, max_observed};
    };

    double GetProfit() {return money - money_last_round;}
    // Misc
    void Destroy() {
        logger.Log(Log::INFO, class_name+std::to_string(id)+std::string(" destroyed."));
        destroyed = true;
        _inventory.inventory.clear();
        auction_house = nullptr;
    }
    void Tick() {
        FlushInbox();
        FlushOutbox();
        ticks++;
    }
};

class Role {
private:
    // rng_gen
    std::mersenne_twister_engine<uint_fast32_t, 32, 624, 397, 31, 0x9908b0dfUL, 11, 0xffffffffUL, 7, 0x9d2c5680UL, 15, 0xefc60000UL, 18, 1812433253UL> rng_gen = std::mt19937(std::random_device()());

public:
    virtual void TickRole(BasicTrader& trader) = 0;

    bool Random(double chance) {
        if (chance >= 1) return true;
        return (rng_gen() < chance*rng_gen.max());
    }
    void Produce(BasicTrader& trader, std::string commodity, int amount, double chance = 1) {
        if (Random(chance)) {
            trader.TryAddCommodity(commodity, amount, false);
        }
    }
    void Consume(BasicTrader& trader, std::string commodity, int amount, double chance = 1) {
        if (Random(chance)) {
            trader.TryTakeCommodity(commodity, amount, false);
        }
    }
    void LoseMoney(BasicTrader& trader, double amount) {
        trader.ForceTakeMoney(amount);
    }

};


class RoleFarmer : public Role {
public:
    void TickRole(BasicTrader& trader) override {
        bool has_wood = (0 < trader.Query("wood"));
        bool has_tools = (0 < trader.Query("tools"));

        if (!has_wood) {
            LoseMoney(trader, 2);//$2 idleness fine
            return;
        }

        if (has_tools) {
            // 10% chance tools break
            Consume(trader, "tools", 1, 0.1);
            Consume(trader, "wood", 1);
            Produce(trader, "food", 6);
        } else {
            Consume(trader, "wood", 1);
            Produce(trader, "food", 3);
        }
    }
};

class RoleWoodcutter : public Role {
public:
    void TickRole(BasicTrader& trader) override {
        bool has_food = (0 < trader.Query("food"));
        bool has_tools = (0 < trader.Query("tools"));

        if (!has_food) {
            LoseMoney(trader, 2);//$2 idleness fine
            return;
        }

        if (has_tools) {
            // 10% chance tools break
            Consume(trader, "tools", 1, 0.1);
            Consume(trader, "food", 1);
            Produce(trader, "wood", 2);
        } else {
            Consume(trader, "food", 1);
            Produce(trader, "wood", 1);
        }
    }
};
#endif//CPPBAZAARBOT_TRADER_H
