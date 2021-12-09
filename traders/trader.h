//
// Created by henry on 06/12/2021.
//

#ifndef CPPBAZAARBOT_TRADER_H
#define CPPBAZAARBOT_TRADER_H

#include <utility>

#include "inventory.h"
#include "../common/messages.h"

#include "../metrics/metrics.h"

class AuctionHouse;

class BasicTrader : public Agent {
private:
    int ticks = 0;
    friend AuctionHouse;
    Inventory _inventory;
    // TODO Change this to an array to allow multiple auction houses to be traded with at once
    std::shared_ptr<Agent> auction_house;

    std::vector<Message> inbox;
    std::vector<std::pair<int, Message>> outbox;

    std::map<std::string, std::vector<double>> _observedTradingRange;
    double _profit = 0;
    int _lookback = 15; //history range (default 15 ticks)
    ConsoleLogger logger;

public:
    std::string class_name; // eg "Farmer", "Woodcutter" etc. Auction House will verify this on registration. (TODO)
    double money;

    bool destroyed = false;
    double profit_last_round = 0;
    double profit = 0;

    double track_costs;

public:
    BasicTrader(int id, std::shared_ptr<Agent> auction_house_ptr, std::string  class_name, double starting_money, double inv_capacity, const std::vector<std::pair<Commodity, int>> &starting_inv, Log::LogLevel log_level = Log::WARN)
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

    void ProcessBidResult(Message& message) {};
    void ProcessAskResult(Message& message) {};
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
    void AddMoney(double quantity) override {
        money += quantity;
    }

    bool HasCommodity(std::string commodity, int quantity) override {
        auto stored = _inventory.Query(commodity);
        if ( !stored) {
            return false;
        }
        return (*stored >= quantity);
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
        if ( *stored>= quantity) {
            actual_transferred = quantity;
        } else {
            if (atomic) {
                actual_transferred = 0;
                logger.Log(Log::DEBUG, "Failed to take "+commodity+std::string(" x") + std::to_string(quantity));
            } else {
                actual_transferred = *stored;
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

    std::optional<int> Query(const std::string& name) override { return _inventory.Query(name); }
    double GetEmptySpace() override { return _inventory.GetEmptySpace(); }

    // TODO: continue from line 108 in BasicAgent.cs
    void Tick() {
        FlushInbox();
        FlushOutbox();
        ticks++;
    }
};
#endif//CPPBAZAARBOT_TRADER_H
