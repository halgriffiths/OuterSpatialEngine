//
// Created by henry on 06/12/2021.
//
#include <iostream>
#include "BazaarBot.h"
#include <memory>
// test driver for BazaarBot library

std::shared_ptr<BasicTrader> CreateAndRegister(int id,
                                               const std::vector<std::pair<Commodity, int>>& inv,
                                               const std::shared_ptr<AuctionHouse>& auction_house) {
    auto trader = std::make_shared<BasicTrader>(id, auction_house, "test_class", 100.0, 50, inv, Log::INFO);

    trader->SendMessage(*Message(id).AddRegisterRequest(std::move(RegisterRequest(trader->id, trader))), auction_house);
    return trader;
}

void SimpleTradeTest() {
    // We expect as an outcome:
//        1. Alice sells 3 to Charlie for $10 (0 unsold)
//        2. Bob sells 1 to Charlie for $12 (4 unsold)
//        3. Charlie gets 4 for $10.5 avg (0 unbought)
//        4. Dan gets nothing (1 unbought)

    auto comm = Commodity("comm");
    auto comm1 = Commodity("comm1");

    auto auction_house = std::make_shared<AuctionHouse>(0, Log::INFO);
    auction_house->RegisterCommodity(comm);
    auction_house->RegisterCommodity(comm1);


    std::vector<std::pair<Commodity, int>> c_v = {{comm, 3}, {comm1,9}};
    auto Alice = CreateAndRegister(1, c_v, auction_house);
    auto Bob = CreateAndRegister(2, c_v, auction_house);
    auto Charlie = CreateAndRegister(3, c_v, auction_house);
    auto Dan =  CreateAndRegister(4, c_v, auction_house);

    std::cout << "\n -- Registered traders --\n" <<std::endl;

    // Alice sells 3 for $10
    AskOffer ask = {1, "comm", 3, 10};
    Alice->SendMessage(*Message(1).AddAskOffer(std::move(ask)), auction_house);
    // Bob sells 5 for $12
    ask = {2, "comm", 5, 12};
    Bob->SendMessage(*Message(2).AddAskOffer(std::move(ask)), auction_house);

    // Charlie buys 4 for $15
    BidOffer bid = {3, "comm", 4, 15};
    Charlie->SendMessage(*Message(3).AddBidOffer(std::move(bid)), auction_house);
    // Dan buys 1 for $11
    bid = {4, "comm", 1, 11};
    Dan->SendMessage(*Message(4).AddBidOffer(std::move(bid)), auction_house);


    std::cout << "\n -- Received bids from traders --\n" <<std::endl;
    auction_house->Tick();
    std::cout << "Done." << std::endl;
}

int main() {
    SimpleTradeTest();

}