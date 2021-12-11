//
// Created by henry on 07/12/2021.
//

#ifndef CPPBAZAARBOT_AGENT_H
#define CPPBAZAARBOT_AGENT_H

#include "messages.h"
#include <memory>

class AuctionHouse;
// All an agent is is an entity with an id, capable of sending and receiving messages
class Agent {
public:
    int id;
    int ticks = 0;
    Agent(int agent_id) : id(agent_id) {};
    virtual void ReceiveMessage(Message incoming_message) = 0;
    virtual void SendMessage(Message& outgoing_message,  int recipient) = 0;
};

// A Trader is an Agent capable of interacting with an AuctionHouse
class Trader : public Agent {
public:
    Trader(int id) : Agent(id) {};

    virtual bool HasMoney(double quantity) {return false;};
    virtual bool HasCommodity(const std::string& commodity, int quantity) {return false;};

    virtual int GetIdeal(const std::string& name) { return 0; };
    virtual int Query(const std::string& name) { return 0; }
    virtual double QueryCost(const std::string& name) { return 0;}
    virtual double GetEmptySpace() { return 0; }
private:
    friend AuctionHouse;
    virtual double TryTakeMoney(double quantity, bool atomic) {};
    virtual void AddMoney(double quantity) {};
    virtual int TryAddCommodity(const std::string& commodity, int quantity, std::optional<double> unit_price, bool atomic) {return 0;};
    virtual int TryTakeCommodity(const std::string& commodity, int quantity, std::optional<double> unit_price, bool atomic) {return 0;};
};
#endif//CPPBAZAARBOT_AGENT_H
