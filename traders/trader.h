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
    friend AuctionHouse;
    Inventory _inventory;
    std::shared_ptr<Agent> auction_house;

public:
    std::string class_name; // eg "Farmer", "Woodcutter" etc. Auction House will verify this on registration. (TODO)
    double money;

    bool destroyed = false;
    double profit_last_round = 0;
    double profit = 0;

    double track_costs;
private:
    std::map<std::string, std::vector<double>> _observedTradingRange;
    double _profit = 0;
    int _lookback = 15; //history range (default 15 ticks)

public:
    ConsoleLogger logger;
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

    double GetEmptySpace() { return _inventory.GetEmptySpace(); };

    void ReceiveMessage(Message& incoming_message) override {
        logger.LogReceived(incoming_message.sender_id, Log::INFO, incoming_message.ToString());
    }

    void SendMessage(Message& outgoing_message, std::shared_ptr<Agent> recipient) override {
        logger.LogSent(recipient->id, Log::INFO, outgoing_message.ToString());
        recipient->ReceiveMessage(outgoing_message);
    }
    // TODO: continue from line 108 in BasicAgent.cs
};
#endif//CPPBAZAARBOT_TRADER_H
