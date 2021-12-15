//
// Created by henry on 06/12/2021.
//

#ifndef CPPBAZAARBOT_AI_TRADER_H
#define CPPBAZAARBOT_AI_TRADER_H

#include <utility>

#include "inventory.h"
#include "../common/messages.h"

#include "../auction/auction_house.h"
#include "../metrics/logger.h"

class AITrader;

namespace {
    double PositionInRange(double value, double min, double max) {
        value -= min;
        max -= min;
        min = 0;
        value = (value / (max - min));

        if (value < 0) { value = 0; }
        if (value > 1) { value = 1; }

        return value;
    }
}

class Role {
private:
    // rng_gen
    std::mersenne_twister_engine<uint_fast32_t, 32, 624, 397, 31, 0x9908b0dfUL, 11, 0xffffffffUL, 7, 0x9d2c5680UL, 15, 0xefc60000UL, 18, 1812433253UL> rng_gen = std::mt19937(std::random_device()());

public:
    std::string required_good;
    Role(std::string required = "none") : required_good(required){};
    bool Random(double chance);
    virtual void TickRole(AITrader & trader) = 0;
    void Produce(AITrader & trader, const std::string& commodity, int amount, double chance = 1);
    void Consume(AITrader & trader, const std::string& commodity, int amount, double chance = 1);
    void LoseMoney(AITrader & trader, double amount);
    double track_costs = 0;
};


class AITrader : public Trader {
private:
    std::mt19937 rng_gen = std::mt19937(std::random_device()());
    double MIN_PRICE = 0.01;
    bool initialised = false;

    std::optional<std::shared_ptr<Role>> logic;
    Inventory _inventory;

    std::weak_ptr<AuctionHouse> auction_house;

    std::map<std::string, std::vector<double>> _observedTradingRange;

    int  external_lookback = 5; //history range (num ticks)
    int internal_lookback = 10; //history range (num trades)

public:
    double curr_profit = 0;
    double total_profit = 0;
    double IDLE_TAX = 2;
    std::string class_name; // eg "Farmer", "Woodcutter" etc. Auction House will verify this on registration. (TODO)
    double money;

    bool destroyed = false;
    double money_last_round = 0;
    double profit = 0;

    ConsoleLogger logger;

public:
    AITrader(int id, std::weak_ptr<AuctionHouse> auction_house_ptr, std::optional<std::shared_ptr<Role>> AI_logic, const std::string& class_name, double starting_money, double inv_capacity, const std::vector<InventoryItem> &starting_inv, Log::LogLevel log_level = Log::WARN)
    : Trader(id)
    , auction_house(std::move(auction_house_ptr))
    , logic(std::move(AI_logic))
    , class_name(class_name)
    , money(starting_money)
    , logger(ConsoleLogger(class_name+std::to_string(id), log_level)){
        //construct inv
        _inventory = Inventory(inv_capacity, starting_inv);
        for (const auto &item : starting_inv) {
            double base_price = 1;
            if (item.name == "metals") {
                base_price = 2;
            } else if (item.name == "tools") {
                base_price = 3;
            }
            _observedTradingRange[item.name] = {base_price, base_price*3};
            _inventory.SetCost(item.name, base_price);
        }
    }

    void PrintState();
    void FlushOutbox();
    void FlushInbox();

    void ProcessBidResult(Message& message);
    void ProcessAskResult(Message& message);
    void ProcessRegistrationResponse(Message& message);

    // Inventory functions
    bool HasMoney(double quantity) override;
    double TryTakeMoney(double quantity, bool atomic) override;
    void ForceTakeMoney(double quantity);
    void AddMoney(double quantity) override;

    bool HasCommodity(const std::string& commodity, int quantity) override;
    int TryTakeCommodity(const std::string& commodity, int quantity, std::optional<double> unit_price, bool atomic) override;
    int TryAddCommodity(const std::string& commodity, int quantity, std::optional<double> unit_price, bool atomic) override;

    int GetIdeal(const std::string& name);
    int Query(const std::string& name);
    double QueryCost(const std::string& name);

    // Trading functions
    void UpdatePriceModelFromBid(BidResult& result);
    void UpdatePriceModelFromAsk(const AskResult& result);

    void GenerateOffers(const std::string& commodity);
    BidOffer CreateBid(const std::string& commodity, int min_limit, int max_limit, double desperation = 0);
    AskOffer CreateAsk(const std::string& commodity, int min_limit);

    int DetermineBuyQuantity(const std::string& commodity, double bid_price);
    int DetermineSaleQuantity(const std::string& commodity);

    std::pair<double, double> ObserveTradingRange(const std::string& commodity, int window);

    double GetProfit();
    // Misc
    void Destroy();
    void Tick();
};

void AITrader::FlushOutbox() {
        logger.Log(Log::DEBUG, "Flushing outbox");
        while (!outbox.empty()) {
            auto& outgoing = outbox.back();
            // Trader can currently only talk to auction houses (not other traders)
            if (outgoing.first != auction_house.lock()->id) {
                logger.Log(Log::ERROR, "Failed to send message, unknown recipient " + std::to_string(outgoing.first));
                continue;
            }
            logger.LogSent(outgoing.first, Log::DEBUG, outgoing.second.ToString());
            auction_house.lock()->ReceiveMessage(std::move(outgoing.second));
            outbox.pop_back();
        }
        logger.Log(Log::DEBUG, "Flush finished");
}
void AITrader::FlushInbox() {
    logger.Log(Log::DEBUG, "Flushing inbox");
    while (!inbox.empty()) {
        auto& incoming_message = inbox.back();
        logger.LogReceived(incoming_message.sender_id, Log::INFO, incoming_message.ToString());
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
void AITrader::ProcessAskResult(Message& message) {
    UpdatePriceModelFromAsk(*message.ask_result);
}
void AITrader::ProcessBidResult(Message& message) {
    UpdatePriceModelFromBid(*message.bid_result);
}
void AITrader::ProcessRegistrationResponse(Message& message) {
    if (message.register_response->accepted) {
        initialised = true;
        logger.Log(Log::INFO, "Successfully registered with auction house");
    } else {
        logger.Log(Log::ERROR, "Failed to register with auction house");
        Destroy();
    }
}

bool AITrader::HasMoney(double quantity) {
    return (money >= quantity);
}
double AITrader::TryTakeMoney(double quantity, bool atomic) {
    double amount_transferred;
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
void AITrader::ForceTakeMoney(double quantity) {
    logger.Log(Log::DEBUG, "Lost money: $" + std::to_string(quantity));
    money -= quantity;
}
void AITrader::AddMoney(double quantity) {
    logger.Log(Log::DEBUG, "Gained money: $" + std::to_string(quantity));
    money += quantity;
}

bool AITrader::HasCommodity(const std::string& commodity, int quantity) {
    auto stored = _inventory.Query(commodity);
    return (stored >= quantity);
}
int AITrader::TryTakeCommodity(const std::string& commodity, int quantity, std::optional<double> unit_price, bool atomic) {
    auto comm = _inventory.GetItem(commodity);
    if (!comm) {
        //item unknown, fail
        logger.Log(Log::ERROR, "Tried to take unknown item "+commodity);
        return 0;
    }
    int actual_transferred ;
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
    _inventory.TakeItem(commodity, actual_transferred, unit_price);
    return actual_transferred;
}
int AITrader::TryAddCommodity(const std::string& commodity, int quantity, std::optional<double> unit_price, bool atomic) {
    auto comm = _inventory.GetItem(commodity);
    if (!comm) {
        //item unknown, fail
        logger.Log(Log::ERROR, "Tried to add unknown item "+commodity);
        return 0;
    }
    int actual_transferred;
    if (_inventory.GetEmptySpace() >= quantity*comm->size) {
        actual_transferred = quantity;
    } else {
        if (atomic) {
            actual_transferred = 0;
            logger.Log(Log::DEBUG, "Failed to add "+commodity+std::string(" x") + std::to_string(quantity));
        } else {
            actual_transferred = std::floor(_inventory.GetEmptySpace()/comm->size);
            //overproduced! Drop value of goods accordingly
            int overproduction = quantity - actual_transferred;
            _inventory.inventory[commodity].original_cost *= std::pow(1.3, -1*overproduction);
        }
    }
    _inventory.AddItem(commodity, actual_transferred, unit_price);
    return actual_transferred;
}
int AITrader::GetIdeal(const std::string& name) {
    auto res = _inventory.GetItem(name);
    if (!res) {
        return 0;
    }
    return res->ideal_quantity;
}
int AITrader::Query(const std::string& name) { return _inventory.Query(name); }
double AITrader::QueryCost(const std::string& name) { return _inventory.QueryCost(name); }

// Trading functions
void AITrader::UpdatePriceModelFromBid(BidResult& result) {
    for (int i = 0; i < result.quantity_traded; i++) {
        _observedTradingRange[result.commodity].push_back(result.avg_price);
    }

    while (_observedTradingRange[result.commodity].size() > internal_lookback) {
        _observedTradingRange[result.commodity].erase(_observedTradingRange[result.commodity].begin());
    }
}
void AITrader::UpdatePriceModelFromAsk(const AskResult& result) {
    for (int i = 0; i < result.quantity_traded; i++) {
        _observedTradingRange[result.commodity].push_back(result.avg_price);
    }

    while (_observedTradingRange[result.commodity].size() > internal_lookback) {
        _observedTradingRange[result.commodity].erase(_observedTradingRange[result.commodity].begin());
    }
}

void AITrader::GenerateOffers(const std::string& commodity) {
    int surplus = _inventory.Surplus(commodity);
    if (surplus >= 1) {
        logger.Log(Log::DEBUG, "Considering ask for "+commodity + std::string(" - Current surplus = ") + std::to_string(surplus));
        auto offer = CreateAsk(commodity, 1);
        if (offer.quantity > 0) {
            SendMessage(*Message(id).AddAskOffer(offer), auction_house.lock()->id);
        }
    }

    int shortage = _inventory.Shortage(commodity);
    double space = _inventory.GetEmptySpace();
    double unit_size = _inventory.GetSize(commodity);


    double fulfillment;
    if (class_name == "refiner" || class_name == "blacksmith") {
        fulfillment = _inventory.Query(commodity) / (0.001 + _inventory.GetItem(commodity)->ideal_quantity);
        fulfillment = std::max(0.5, fulfillment);
    } else {
        fulfillment = _inventory.Query(commodity) / (0.001 + _inventory.GetItem(commodity)->ideal_quantity);
    }

    if (fulfillment < 1 && space >= unit_size) {
        int max_limit = (shortage*unit_size <= space) ? shortage : (int) space/shortage;
        if (max_limit > 0)
        {
            int min_limit = (_inventory.Query(commodity) == 0) ? 1 : 0;
            logger.Log(Log::DEBUG, "Considering bid for "+commodity + std::string(" - Current shortage = ") + std::to_string(shortage));

            //double desperation = 2 - std::pow((0.2 * money / IDLE_TAX), 0.2*shortage);
            double days_savings = money / IDLE_TAX;
            double desperation = ( 1 /(days_savings*days_savings)) + 1;
            desperation *= 1 - (0.4*(fulfillment - 0.5))/(1 + 0.4*std::abs(fulfillment-0.5));
            auto offer = CreateBid(commodity, min_limit, max_limit, desperation);
            if (offer.quantity > 0) {
                SendMessage(*Message(id).AddBidOffer(offer), auction_house.lock()->id);
            }
        }
    }
}
BidOffer AITrader::CreateBid(const std::string& commodity, int min_limit, int max_limit, double desperation) {
    double fair_bid_price = (auction_house.lock()->AverageHistoricalMidPrice(commodity, external_lookback));
    //scale between price based on need
    double max_price = money;
    double min_price = MIN_PRICE;
    double bid_price = fair_bid_price *desperation;
    bid_price = std::min(std::max(min_price, bid_price), max_price);

    int ideal = DetermineBuyQuantity(commodity, bid_price);
    int quantity = std::max(std::min(ideal, max_limit), min_limit);
    return BidOffer(id, commodity, quantity, bid_price);
}
AskOffer AITrader::CreateAsk(const std::string& commodity, int min_limit) {
    //AI agents offer a fair ask price - costs + 2% profit
    double fair_price = QueryCost(commodity) * 1.02;
    double market_price = auction_house.lock()->AverageHistoricalMidPrice(commodity, external_lookback);
    double ask_price;
    if (fair_price > market_price) {
        ask_price = fair_price;
    } else {
        std::uniform_real_distribution<> random_price(fair_price, market_price);
        ask_price = random_price(rng_gen);
    }
    ask_price = std::max(MIN_PRICE, ask_price);
    int quantity = DetermineSaleQuantity(commodity);
    //can't sell less than limit
    quantity = quantity < min_limit ? min_limit : quantity;
    return AskOffer(id, commodity, quantity, ask_price);
}

int AITrader::DetermineBuyQuantity(const std::string& commodity, double avg_price) {
    std::pair<double, double> range = ObserveTradingRange(commodity, internal_lookback);
    if (range.first == 0 && range.second == 0) {
        //uninitialised range
        logger.Log(Log::WARN, "Tried to make bid with unitialised trading range");
        return 0;
    }
    double favorability = PositionInRange(avg_price, range.first, range.second);
    favorability = 1 - favorability; //do 1 - favorability to see how close we are to the low end
    double amount_to_buy = favorability * _inventory.Shortage(commodity);//double

    return std::ceil(amount_to_buy);
}
int AITrader::DetermineSaleQuantity(const std::string& commodity) {
    return _inventory.Surplus(commodity); //Sell all surplus
}

std::pair<double, double> AITrader::ObserveTradingRange(const std::string& commodity, int window) {
    if (_observedTradingRange.count(commodity) < 1 || _observedTradingRange[commodity].empty()) {
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
}

double AITrader::GetProfit() {return money - money_last_round;}
// Misc
void AITrader::Destroy() {
    auto res = auction_house.lock();
    if (res) {
        res->ReceiveMessage(*Message(id).AddShutdownNotify({id}));
    }
    destroyed = true;
    logger.Log(Log::INFO, class_name+std::to_string(id)+std::string(" destroyed."));
    _inventory.inventory.clear();
    auction_house.reset();
}
void AITrader::Tick() {
    if (destroyed) {
        return;
    }
    curr_profit = GetProfit();
    total_profit += curr_profit;
    money_last_round = money;
    FlushInbox();
    if (initialised) {
        if (logic) {
            logger.Log(Log::DEBUG, "Ticking internal logic");
            (*logic)->TickRole(*this);
        }
        for (const auto& commodity : _inventory.inventory) {
            GenerateOffers(commodity.first);
        }
    }
    if (money <= 0) {
        Destroy();
        return;
    }
    FlushOutbox();
    if (initialised){
        ticks++;
    }
}

void AITrader::PrintState() {
    std::cout <<"--------------------\nSTATUS: " << class_name << "-" << id << " age " << ticks << std::endl;
    std::cout << "\tInventory:" << std::endl;
    std::cout << "\t\tMoney: $" << money << std::endl;
    for (auto& item : _inventory.inventory) {
        std::cout << "\t\t" << item.first << ": " <<item.second.stored << "\t(ideal: " << item.second.ideal_quantity << ")" << std::endl;
    }
    std::cout << "Profit (total): " << curr_profit << " \t(" << total_profit << ")\n";
}


bool Role::Random(double chance) {
    if (chance >= 1) return true;
    return (rng_gen() < chance*rng_gen.max());
}
void Role::Produce(AITrader& trader, const std::string& commodity, int amount, double chance) {
    if (amount > 0 && Random(chance)) {
        trader.logger.Log(Log::DEBUG, "Produced " + std::string(commodity) + std::string(" x") + std::to_string(amount));

        if (track_costs < 1) track_costs = 1;
        trader.TryAddCommodity(commodity, amount, track_costs /  amount, false);
        track_costs = 0;
    }
}
void Role::Consume(AITrader& trader, const std::string& commodity, int amount, double chance) {
    if (Random(chance)) {
        trader.logger.Log(Log::DEBUG, "Consumed " + std::string(commodity) + std::string(" x") + std::to_string(amount));
        int actual_quantity = trader.TryTakeCommodity(commodity, amount, 0, false);
        if (actual_quantity > 0) {
            track_costs += actual_quantity*trader.QueryCost(commodity);
        }
    }
}
void Role::LoseMoney(AITrader& trader, double amount) {
    trader.ForceTakeMoney(amount);
    track_costs += amount;
}

#endif//CPPBAZAARBOT_AI_TRADER_H
