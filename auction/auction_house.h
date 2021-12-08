//
// Created by henry on 06/12/2021.
//

#ifndef CPPBAZAARBOT_AUCTION_HOUSE_H
#define CPPBAZAARBOT_AUCTION_HOUSE_H

#include <random>
#include <algorithm>
#include <utility>

#include "../common/history.h"

#include "../common/agent.h"
#include "../common/messages.h"
#include "../traders/inventory.h"
#include "../common/commodity.h"

#include "../traders/trader.h"

#include "../metrics/metrics.h"

class AuctionHouse : public Agent {
public:
    History history;
private:
    int ticks = 0;
    std::mersenne_twister_engine<uint_fast32_t, 32, 624, 397, 31, 0x9908b0dfUL, 11, 0xffffffffUL, 7, 0x9d2c5680UL, 15, 0xefc60000UL, 18, 1812433253UL> rng_gen = std::mt19937(std::random_device()());
    std::map<std::string, Commodity> known_commodities;
    std::map<int, std::shared_ptr<Agent>> known_traders;  //key = trader-id

    std::map<std::string, std::vector<BidOffer>> bid_book;
    std::map<std::string, std::vector<AskOffer>> ask_book;

    std::vector<std::shared_ptr<Message>> inbox;
    std::vector<std::shared_ptr<Message>> outbox;
    ConsoleLogger logger;
public:
    AuctionHouse(int auction_house_id, Log::LogLevel verbosity)
        : Agent(auction_house_id)
        , logger(ConsoleLogger(std::string("AH")+std::to_string(id), verbosity)) {

        history = {};

        known_commodities = {};
        known_traders = {};

        bid_book = {};
        ask_book = {};
    }

    void RegisterCommodity(const Commodity& new_commodity) {
        if (known_commodities.find(new_commodity.name) != known_commodities.end()) {
            //already exists
            return;
        }
        history.initialise(new_commodity.name);
        known_commodities[new_commodity.name] = new_commodity;

    }

    void ReceiveMessage(Message& incoming_message) override {
        logger.LogReceived(incoming_message.sender_id, Log::INFO, incoming_message.ToString());
        std::shared_ptr<Message> message;
        if (incoming_message.GetType() == BID_OFFER) {
            message = std::make_shared<BidOffer>(static_cast<BidOffer&>(incoming_message));
            inbox.push_back(message);
        } else if (incoming_message.GetType() == ASK_OFFER) {
            message = std::make_shared<AskOffer>(dynamic_cast<AskOffer&>(incoming_message));
        } else if (incoming_message.GetType() == REGISTER_REQUEST) {
            message = std::make_shared<RegisterRequest>(dynamic_cast<RegisterRequest&>(incoming_message));
        } else {
            logger.Log(Log::ERROR, "Unknown/unsupported message type ");
        }
        inbox.push_back(message);
    }

    void SendMessage(Message& outgoing_message, int recipient) {
        if (known_traders.find(recipient) != known_traders.end()) {
            SendMessage(outgoing_message, known_traders[recipient]);
        } else {
            logger.Log(Log::ERROR, "Can't send message - unknown recipient ID "+ std::to_string(recipient));
        }
    }
    void SendMessage(Message& outgoing_message, std::shared_ptr<Agent> recipient) override {
        logger.LogSent(recipient->id, Log::INFO, outgoing_message.ToString());
        recipient->ReceiveMessage(outgoing_message);
    }
    void ProcessMessages() {
        while (!inbox.empty()) {
            auto& incoming_message = inbox.back();
            if (incoming_message->GetType() == BID_OFFER) {
                BidOffer offer = *std::dynamic_pointer_cast<BidOffer>(incoming_message);
                ProcessBid(offer);
            } else if (incoming_message->GetType() == ASK_OFFER) {
                AskOffer offer = *std::dynamic_pointer_cast<AskOffer>(incoming_message);
                ProcessAsk(offer);
            } else if (incoming_message->GetType() == REGISTER_REQUEST) {
                RegisterRequest request = *std::dynamic_pointer_cast<RegisterRequest>(incoming_message);
                ProcessRegistrationRequest(request);
            } else {
                std::cout << "Unknown/unsupported message type " << incoming_message->GetType() << std::endl;
            }
            inbox.pop_back();
        }
    }
    void ProcessBid(BidOffer& bid) {
        bid_book[bid.commodity].push_back(bid);
    }
    void ProcessAsk(AskOffer& ask) {
        ask_book[ask.commodity].push_back(ask);
    }
    void ProcessRegistrationRequest(RegisterRequest& request) {
        // check no id clash
        auto requested_id = request.sender_id;
        if (requested_id == id) {
            auto response = RegisterResponse(id, false, "ID clash with auction house");
            SendMessage(response, request.trader_pointer);
            return;
        }

        if (known_traders.find(requested_id) != known_traders.end()) {
            auto response = RegisterResponse(id, false, "ID clash with existing trader");
            SendMessage(response, request.trader_pointer);
            return;
        }

        // check all requested commodities are known to auction house
        for (const auto& requested_item : request.trader_pointer->_inventory.inventory) {
            if (known_commodities.find(requested_item.first) == known_commodities.end()) {
                //if unknown commodity
                auto response = RegisterResponse(id, false, "Requested unknown commodity: " + requested_item.first);
                SendMessage(response, request.trader_pointer);
                return;
            }
        }

        // Otherwise, OK the request and register
        known_traders[requested_id] = request.trader_pointer;
        auto response = RegisterResponse(id, true);
        SendMessage(response, requested_id);
    }

    void Tick() {
        ProcessMessages();
        for (const auto& item : known_commodities) {
            ResolveOffers(item.first);
        }
        ticks++;
    }

private:
    void MakeTransaction(const std::string& commodity, int buyer, int seller, int quantity, double unit_price) {
        // TODO: Update inventory/cash values
    }

    void ResolveOffers(const std::string& commodity) {
        std::vector<BidOffer> bids = bid_book[commodity];
        std::vector<AskOffer> asks = ask_book[commodity];

        std::shuffle(bids.begin(), bids.end(), rng_gen);
        std::shuffle(bids.begin(), bids.end(), rng_gen);

        std::sort(bids.rbegin(), bids.rend()); // NOTE: Reversed order
        std::sort(asks.begin(), asks.end());   // lowest selling price first

        int num_trades_this_tick = 0;
        double money_traded_this_tick = 0;
        double units_traded_this_tick = 0;
        double avg_price_this_tick = 0;
        double supply = 0;
        double demand = 0;

        for (const auto& bid : bids) {
            demand += bid.quantity;
        }
        for (const auto& ask : asks) {
            supply += ask.quantity;
        }

        auto bid_result = BidResult(id, commodity);
        auto ask_result = AskResult(id, commodity);

        while (!bids.empty() && !asks.empty()) {
            BidOffer& curr_bid = bids[0];
            AskOffer& curr_ask = asks[0];

            if (curr_ask.unit_price > curr_bid.unit_price) {
                break;
            }

            int quantity_traded = std::min(curr_bid.quantity, curr_ask.quantity);
            double clearing_price = curr_ask.unit_price;

            if (quantity_traded > 0) {
                // MAKE TRANSACTION
                int buyer = curr_bid.sender_id;
                int seller = curr_ask.sender_id;
                MakeTransaction(commodity, buyer, seller, quantity_traded, clearing_price);
                // update the offers and results
                curr_bid.quantity -= quantity_traded;
                curr_ask.quantity -= quantity_traded;

                bid_result.UpdateWithTrade(quantity_traded, clearing_price);
                ask_result.UpdateWithTrade(quantity_traded, clearing_price);

                // update per-tick metrics
                avg_price_this_tick = (avg_price_this_tick*units_traded_this_tick + clearing_price*quantity_traded)/(units_traded_this_tick + quantity_traded);
                units_traded_this_tick += quantity_traded;
                money_traded_this_tick += quantity_traded*clearing_price;
                num_trades_this_tick += 1;
            }

            if (curr_bid.quantity <= 0) {
                // Fulfilled buy order
                SendMessage(bid_result, curr_bid.sender_id);
                bids.erase(bids.begin());
                // Reset bid result
                bid_result = BidResult(id, commodity);
            }
            if (curr_ask.quantity <= 0) {
                // Fulfilled sell order
                SendMessage(ask_result, curr_ask.sender_id);
                asks.erase(asks.begin());
                // Reset result
                ask_result = AskResult(id, commodity);
            }
        }

        while (!bids.empty()) {
            BidOffer& curr_bid = bids[0];
            bid_result.UpdateWithNoTrade(curr_bid.quantity);
            SendMessage(bid_result, curr_bid.sender_id);
            bids.erase(bids.begin());
            // Reset result
            bid_result = BidResult(id, commodity);
        }
        if (!asks.empty()) {
            AskOffer& curr_ask = asks[0];
            // ditto for last ask
            ask_result.UpdateWithNoTrade(curr_ask.quantity);
            SendMessage(ask_result, curr_ask.sender_id);
            asks.erase(asks.begin());
            // Reset result
            ask_result = AskResult(id, commodity);
        }

        // update history
        history.asks.add(commodity, supply);
        history.bids.add(commodity, demand);
        history.trades.add(commodity, num_trades_this_tick);
        if (units_traded_this_tick > 0) {
            history.prices.add(commodity, avg_price_this_tick);
        } else {
            // Set to same as last-tick's average if no trades occurred
            history.prices.add(commodity, history.prices.average(commodity, 1));
        }
    }



};

#endif//CPPBAZAARBOT_AUCTION_HOUSE_H
