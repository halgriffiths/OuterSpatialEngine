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

class BasicTrader;



#include "../metrics/metrics.h"

class AuctionHouse : public Agent {
public:
    History history;

private:
    int ticks = 0;
    std::mersenne_twister_engine<uint_fast32_t, 32, 624, 397, 31, 0x9908b0dfUL, 11, 0xffffffffUL, 7, 0x9d2c5680UL, 15, 0xefc60000UL, 18, 1812433253UL> rng_gen = std::mt19937(std::random_device()());
    std::map<std::string, Commodity> known_commodities;
    std::map<int, std::shared_ptr<Agent>> known_traders;  //key = trader-id

    std::map<std::string, std::vector<BidOffer>> bid_book = {};
    std::map<std::string, std::vector<AskOffer>> ask_book = {};

    std::vector<Message> inbox = {};
    std::vector<std::pair<int, Message>> outbox = {}; //Message, recipient_id
    ConsoleLogger logger;

public:
    AuctionHouse(int auction_house_id, Log::LogLevel verbosity)
        : Agent(auction_house_id)
        , logger(ConsoleLogger(std::string("AH")+std::to_string(id), verbosity)) {
    }

    // Messaging functions
    void ReceiveMessage(Message incoming_message) override {
        inbox.push_back(incoming_message);
    }
    void SendMessage(Message& outgoing_message, int recipient) override {
        outbox.emplace_back(recipient,std::move(outgoing_message));
    }
    void SendDirect(Message outgoing_message, std::shared_ptr<Agent>& recipient) {
        logger.Log(Log::WARN, "Using SendDirect method to reach unregistered trader");
        logger.LogSent(recipient->id, Log::DEBUG, outgoing_message.ToString());
        recipient->ReceiveMessage(std::move(outgoing_message));
    }
    void FlushOutbox() {
        logger.Log(Log::DEBUG, "Flushing outbox");
        while (!outbox.empty()) {
            auto& outgoing = outbox.back();
            outbox.pop_back();
            if (known_traders.find(outgoing.first) == known_traders.end()) {
                logger.Log(Log::ERROR, "Failed to send message, unknown recipient " + std::to_string(outgoing.first));
                continue;
            }
            logger.LogSent(outgoing.first, Log::DEBUG, outgoing.second.ToString());
            known_traders[outgoing.first]->ReceiveMessage(std::move(outgoing.second));
        }
        logger.Log(Log::DEBUG, "Flush finished");
    }
    void FlushInbox() {
        logger.Log(Log::DEBUG, "Flushing inbox");
        while (!inbox.empty()) {
            auto& incoming_message = inbox.back();
            logger.LogReceived(incoming_message.sender_id, Log::DEBUG, incoming_message.ToString());
            if (incoming_message.GetType() == Msg::EMPTY) {
                //no-op
            } else if (incoming_message.GetType() == Msg::BID_OFFER) {
                ProcessBid(incoming_message);
            } else if (incoming_message.GetType() == Msg::ASK_OFFER) {
                ProcessAsk(incoming_message);
            } else if (incoming_message.GetType() == Msg::REGISTER_REQUEST) {
                ProcessRegistrationRequest(incoming_message);
            } else {
                std::cout << "Unknown/unsupported message type " << incoming_message.GetType() << std::endl;
            }
            inbox.pop_back();
        }
        logger.Log(Log::DEBUG, "Flush finished");
    }

    // Message processing
    void ProcessBid(Message& message) {
        auto bid = message.bid_offer;
        if (!bid) {
            logger.Log(Log::ERROR, "Malformed bid_offer message");
            return; //drop
        }
        bid_book[bid->commodity].push_back(*bid);
    }
    void ProcessAsk(Message& message) {
        auto ask = message.ask_offer;
        if (!ask) {
            logger.Log(Log::ERROR, "Malformed ask_offer message");
            return; //drop
        }
        ask_book[ask->commodity].push_back(*ask);
    }
    void ProcessRegistrationRequest(Message& message) {
        auto request = message.register_request;
        if (!request) {
            logger.Log(Log::ERROR, "Malformed register_request message");
            return; //drop
        }

        // check no id clash
        auto requested_id = message.sender_id;
        if (requested_id == id) {
            auto msg = Message(id);
            msg.AddRegisterResponse(RegisterResponse(id, false, "ID clash with auction house"));
            std::shared_ptr<Agent> ptr = request->trader_pointer.lock();
            SendDirect(msg, ptr);
            return;
        }

        if (known_traders.find(requested_id) != known_traders.end()) {
            auto msg = Message(id);
            msg.AddRegisterResponse(RegisterResponse(id, false, "ID clash with existing trader"));
            std::shared_ptr<Agent> ptr = request->trader_pointer.lock();
            SendDirect(msg, ptr);
            return;
        }

        // Otherwise, OK the request and register
        known_traders[requested_id] = request->trader_pointer.lock();
        auto msg = Message(id).AddRegisterResponse(RegisterResponse(id, true));
        SendMessage(*msg, requested_id);
    }
    double AverageHistoricalPrice(std::string commodity, int window) {
        return history.prices.average(commodity, window);
    }
    int NumKnownTraders() {
        return known_traders.size();
    }
    void RegisterCommodity(const Commodity& new_commodity) {
        if (known_commodities.find(new_commodity.name) != known_commodities.end()) {
            //already exists
            return;
        }
        history.initialise(new_commodity.name);
        known_commodities[new_commodity.name] = new_commodity;

    }

    void Tick() {
        FlushInbox();
        for (const auto& item : known_commodities) {
            ResolveOffers(item.first);
        }
        FlushOutbox();
        ticks++;
    }
private:
    // Transaction functions
    bool CheckBidStake(BidOffer& offer) {
        if (offer.quantity < 0 || offer.unit_price <= 0) {
            logger.Log(Log::WARN, "Rejected nonsensical bid: " + offer.ToString());
            return false;
        }

        //we refund the agent (if applicable) upon transaction resolution
        auto res = known_traders[offer.sender_id]->HasMoney(offer.quantity*offer.unit_price);
        if (!res) {
            logger.Log(Log::DEBUG, "Failed to take Bid stake: " + offer.ToString());
            return false;
        }
        return true;
    }
    bool CheckAskStake(AskOffer& offer) {
        if (offer.quantity < 0 || offer.unit_price <= 0) {
            logger.Log(Log::WARN, "Rejected nonsensical ask: " + offer.ToString());
            return false;
        }
        //we refund the agent (if applicable) upon transaction resolution
        auto res = known_traders[offer.sender_id]->HasCommodity(offer.commodity, offer.quantity);
        if (!res) {
            logger.Log(Log::DEBUG, "Failed to take Ask stake: " + offer.ToString());
            return false;
        }
        return true;
    }
    void CloseBid(BidOffer bid, BidResult bid_result) {
        if (bid.quantity > 0) {
            // partially unfilled
            bid_result.UpdateWithNoTrade(bid.quantity);
        }
        SendMessage(*Message(id).AddBidResult(std::move(bid_result)), bid.sender_id);
    }
    void CloseAsk(AskOffer ask, AskResult ask_result) {
        if (ask.quantity > 0) {
            // partially unfilled
            ask_result.UpdateWithNoTrade(ask.quantity);
        }
        SendMessage(*Message(id).AddAskResult(std::move(ask_result)), ask.sender_id);
    }

    // 0 - success
    // 1 - seller failed
    // 2 - buyer failed
    int MakeTransaction(const std::string& commodity, int buyer, int seller, int quantity, double unit_price) {
        // take from seller
        auto actual_quantity = known_traders[seller]->TryTakeCommodity(commodity, quantity, 0, true);
        if (actual_quantity == 0) {
            // this may be unrecoverable, not sure
            logger.Log(Log::ERROR, "Seller lacks good! Aborting trade");
            return 1;
        }
        auto actual_money = known_traders[buyer]->TryTakeMoney(actual_quantity*unit_price, true);
        if (actual_money == 0) {
            // this may be unrecoverable, not sure
            logger.Log(Log::ERROR, "Buyer lacks money! Aborting trade");
            return 2;
        }

        known_traders[buyer]->TryAddCommodity(commodity, actual_quantity, unit_price, false);
        known_traders[buyer]->AddMoney(actual_quantity*unit_price);
        auto info_msg = std::string("Made trade: ") + std::to_string(seller) + std::string(" >>> ") + std::to_string(buyer) + std::string(" : ") + commodity + std::string(" x") + std::to_string(quantity) + std::string(" @ $") + std::to_string(unit_price);
        logger.Log(Log::INFO, info_msg);
        return 0;
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

            if (!CheckBidStake(curr_bid)) {
                CloseBid(std::move(curr_bid), std::move(bid_result));
                bids.erase(bids.begin());
                bid_result = BidResult(id, commodity);
                continue;
            }
            if (!CheckAskStake(curr_ask)) {
                CloseAsk(std::move(curr_ask), std::move(ask_result));
                asks.erase(asks.begin());
                ask_result = AskResult(id, commodity);
                continue;
            }

            if (curr_ask.unit_price > curr_bid.unit_price) {
                break;
            }

            int quantity_traded = std::min(curr_bid.quantity, curr_ask.quantity);
            double clearing_price = curr_ask.unit_price;

            if (quantity_traded > 0) {
                // MAKE TRANSACTION
                int buyer = curr_bid.sender_id;
                int seller = curr_ask.sender_id;
                auto res = MakeTransaction(commodity, buyer, seller, quantity_traded, clearing_price);
                if (res == 1) {
                    //seller failed
                    CloseAsk(std::move(curr_ask), std::move(ask_result));
                    asks.erase(asks.begin());
                    // Reset result
                    ask_result = AskResult(id, commodity);
                    break;
                }
                if (res == 2) {
                    //buyer failed
                    CloseBid(std::move(curr_bid), std::move(bid_result));
                    bids.erase(bids.begin());
                    bid_result = BidResult(id, commodity);
                    break;
                }
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
                CloseBid(std::move(curr_bid), std::move(bid_result));
                bids.erase(bids.begin());
                bid_result = BidResult(id, commodity);
            }
            if (curr_ask.quantity <= 0) {
                // Fulfilled sell order
                CloseAsk(std::move(curr_ask), std::move(ask_result));
                asks.erase(asks.begin());
                ask_result = AskResult(id, commodity);
            }
        }

        while (!bids.empty()) {
            BidOffer& curr_bid = bids[0];
            CloseBid(std::move(curr_bid), std::move(bid_result));
            bids.erase(bids.begin());
            // Reset result
            bid_result = BidResult(id, commodity);
        }
        if (!asks.empty()) {
            AskOffer& curr_ask = asks[0];
            CloseAsk(std::move(curr_ask), std::move(ask_result));
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
