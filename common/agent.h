//
// Created by henry on 07/12/2021.
//

#ifndef CPPBAZAARBOT_AGENT_H
#define CPPBAZAARBOT_AGENT_H

#include "messages.h"
#include <memory>

// All an agent is is an entity with an id and bridge, capable of sending and receiving messages
class Agent {
public:
    int id;
    Agent(int agent_id) : id(agent_id) {};

    virtual void ReceiveMessage(Message incoming_message) = 0;
    virtual void SendMessage(Message& outgoing_message,  int recipient) = 0;

    virtual bool HasMoney(double quantity) {return false;};
    virtual bool HasCommodity(std::string commodity, int quantity) {return false;};
    virtual double TryTakeMoney(double quantity, bool atomic) {};
    virtual void AddMoney(double quantity) {};
    virtual int TryAddCommodity(std::string commodity, int quantity, std::optional<double> unit_price, bool atomic) {return 0;};
    virtual int TryTakeCommodity(std::string commodity, int quantity, std::optional<double> unit_price, bool atomic) {return 0;};

    virtual int Query(const std::string& name) { return 0; }
    virtual double QueryCost(const std::string& name) { return 0;}
    virtual double GetEmptySpace() { return 0; }
};
#endif//CPPBAZAARBOT_AGENT_H
