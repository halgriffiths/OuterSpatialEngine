//
// Created by henry on 06/12/2021.
//

#ifndef CPPBAZAARBOT_MESSAGES_H
#define CPPBAZAARBOT_MESSAGES_H

// IWY
#include "../traders/inventory.h"
#include "../common/commodity.h"
#include <memory>

class Agent;
class BasicTrader;

enum MessageType {
    BASE,
    REGISTER_REQUEST,
    REGISTER_RESPONSE,
    BID_OFFER,
    ASK_OFFER,
    BID_RESULT,
    ASK_RESULT
};

// base class for inheritance only
class Message {
public:
    int sender_id; //originator of message
    Message(int sender_id, MessageType type) : sender_id(sender_id), type(type) {};
    virtual std::string ToString() {return std::string("");};
    MessageType GetType() {
        return type;
    }
private:
    MessageType type;
};

// Not currently transmitted over FakeNetwork
class RegisterRequest : public Message{
public:
    std::shared_ptr<BasicTrader> trader_pointer;
    RegisterRequest(int sender_id, std::shared_ptr<BasicTrader> new_trader)
    : Message(sender_id, REGISTER_REQUEST)
    , trader_pointer(new_trader) {};

    std::string ToString() override {
        std::string output("RegistrationRequest from ");
        output.append(std::to_string(sender_id))
                .append(" ");
        return output;
    }

};

class RegisterResponse : public Message{
public:
    bool accepted;
    std::optional<std::string> rejection_reason;

    RegisterResponse(int sender_id, bool accepted, std::optional<std::string> reason = std::nullopt)
    : Message(sender_id, REGISTER_RESPONSE)
    , accepted(accepted)
    , rejection_reason(std::move(reason)) {};

    std::string ToString() override {
        std::string output("RegistrationRequest from ");
        output.append(std::to_string(sender_id))
                .append(": ");
        if (accepted) {
            output.append("OK");
        } else {
            output.append("FAILED - ");
            if (rejection_reason) {
                output.append(*rejection_reason);
            }
        }
        return output;
    }

};

class TransactionResult : public Message {
public:
    std::string commodity;
    int quantity_untraded = 0;
    int quantity_traded = 0;
    double avg_price = 0;

    TransactionResult(int sender_id, std::string commodity, MessageType type)
        : Message(sender_id, type)
        , commodity(std::move(commodity)) {};

    void UpdateWithTrade(int trade_quantity, double unit_price) {
        avg_price = (avg_price*quantity_traded + unit_price*trade_quantity)/(trade_quantity + quantity_traded);
        quantity_traded += trade_quantity;
    }

    void UpdateWithNoTrade(int remainder) {
        quantity_untraded += remainder;
    }

};

class BidResult : public TransactionResult {
public:
    BidResult(int sender_id, std::string commodity)
            : TransactionResult(sender_id, std::move(commodity), BID_RESULT) {};

    std::string ToString() override {
        std::string output("BID RESULT from ");
        output.append(std::to_string(sender_id))
                .append(": Bought ")
                .append(commodity)
                .append(" x")
                .append(std::to_string(quantity_traded))
                .append(" @ avg price $")
                .append(std::to_string(avg_price))
                .append(" (")
                .append(std::to_string(quantity_untraded))
                .append(" unbought)");
        return output;
    }
};

class AskResult : public TransactionResult {
public:
    AskResult(int sender_id, std::string commodity)
            : TransactionResult(sender_id, std::move(commodity), ASK_RESULT) {};

    std::string ToString() override {
        std::string output("ASK RESULT from ");
        output.append(std::to_string(sender_id))
                .append(": Sold   ")
                .append(commodity)
                .append(" x")
                .append(std::to_string(quantity_traded))
                .append(" @ avg price $")
                .append(std::to_string(avg_price))
                .append(" (")
                .append(std::to_string(quantity_untraded))
                .append(" unsold)");
        return output;
    }
};

// base class for inheritance only
class Offer : public Message{
public:
    std::string commodity;
    int quantity;
    double unit_price;

    Offer(int agent_id, const std::string& commodity_name, int quantity, double unit_price, MessageType type)
        : Message(agent_id, type)
        , commodity(commodity_name)
        , quantity(quantity)
        , unit_price(unit_price) {};
};

bool operator< (const Offer& a, const Offer& b) {
    return a.unit_price < b.unit_price;
};

class BidOffer : public Offer {
public:
    BidOffer(int agent_id, const std::string& commodity_name, int quantity, double unit_price)
        : Offer(agent_id, commodity_name, quantity, unit_price, BID_OFFER) {};

    std::string ToString() override {
        std::string output("BID from ");
        output.append(std::to_string(sender_id))
                .append(": ")
                .append(commodity)
                .append(" x")
                .append(std::to_string(quantity))
                .append(" @ $")
                .append(std::to_string(unit_price));
        return output;
    }


};

class AskOffer : public Offer {
public:
    AskOffer(int agent_id, const std::string& commodity_name, int quantity, double unit_price)
            : Offer(agent_id, commodity_name, quantity, unit_price, ASK_OFFER) {};
    std::string ToString() override {
        std::string output("ASK from ");
        output.append(std::to_string(sender_id))
                .append(": ")
                .append(commodity)
                .append(" x")
                .append(std::to_string(quantity))
                .append(" @ $")
                .append(std::to_string(unit_price));
        return output;
    }
};
#endif//CPPBAZAARBOT_MESSAGES_H
