//
// Created by henry on 06/12/2021.
//
#include <iostream>
#include "BazaarBot.h"
#include <assert.h>
// test driver for BazaarBot library
std::shared_ptr<BasicTrader> CreateAndRegister(int id,
                                               const std::vector<std::pair<Commodity, int>>& inv,
                                               const std::shared_ptr<AuctionHouse>& auction_house) {
    auto trader = std::make_shared<BasicTrader>(id, auction_house, "test_class", 100.0, 50, inv, Log::WARN);

    trader->SendMessage(*Message(id).AddRegisterRequest(std::move(RegisterRequest(trader->id, trader))), auction_house->id);
    return trader;
}

// ------------- TESTS ------------

void SimpleTradeTest() {
    auto comm = Commodity("comm");
    auto comm1 = Commodity("comm1");
    auto auction_house = std::make_shared<AuctionHouse>(0, Log::DEBUG);
    auction_house->RegisterCommodity(comm);
    auction_house->RegisterCommodity(comm1);

    std::vector<std::pair<Commodity, int>> c_v = {{comm, 5}, {comm1,9}};
    auto Alice = CreateAndRegister(1, c_v, auction_house);
    auto Bob = CreateAndRegister(2, c_v, auction_house);
    auto Charlie = CreateAndRegister(3, c_v, auction_house);
    auto Dan =  CreateAndRegister(4, c_v, auction_house);

    // Alice sells 3 for $10
    AskOffer ask = {1, "comm", 3, 10};
    Alice->SendMessage(*Message(1).AddAskOffer(std::move(ask)), auction_house->id);
    // Bob sells 5 for $12
    ask = {2, "comm", 5, 12};
    Bob->SendMessage(*Message(2).AddAskOffer(std::move(ask)), auction_house->id);

    // Charlie buys 4 for $15
    BidOffer bid = {3, "comm", 4, 15};
    Charlie->SendMessage(*Message(3).AddBidOffer(std::move(bid)), auction_house->id);
    // Dan buys 1 for $11
    bid = {4, "comm", 1, 11};
    Dan->SendMessage(*Message(4).AddBidOffer(std::move(bid)), auction_house->id);

    Alice->Tick();
    Bob->Tick();
    Charlie->Tick();
    Dan->Tick();

    auction_house->Tick();

    // We expect as an outcome:
    //        1. Alice sells 3 to Charlie for $10 (0 unsold)
    //        2. Bob sells 1 to Charlie for $12 (4 unsold)
    //        3. Charlie gets 4 for $10.5 avg (0 unbought)
    //        4. Dan gets nothing (1 unbought)

    // Since they all start with $100 and 5 "comm", we can make assertions here:
    std::optional<int> stored;
    stored = Alice->Query("comm");
    assert(stored);
    assert(*stored == 2);

    stored = Bob->Query("comm");
    assert(stored);
    assert(*stored == 4);

    stored = Charlie->Query("comm");
    assert(stored);
    assert(*stored == 9);

    stored = Dan->Query("comm");
    assert(stored);
    assert(*stored == 5);
    std::cout << "SimpleTradeTest passed." << std::endl;
}

void InvalidRegistrationTest() {
    auto comm = Commodity("comm");
    auto comm1 = Commodity("comm1");
    auto auction_house = std::make_shared<AuctionHouse>(0, Log::DEBUG);
    auction_house->RegisterCommodity(comm);
    auction_house->RegisterCommodity(comm1);

    std::vector<std::pair<Commodity, int>> c_v = {{comm, 5}, {comm1,9}};
    auto Alice = CreateAndRegister(1, c_v, auction_house);
    auto Bob = CreateAndRegister(1, c_v, auction_house);

    Alice->Tick();
    Bob->Tick();

    auction_house->Tick();

    assert(auction_house->NumKnownTraders() == 1);
    std::cout << "InvalidRegistrationTest passed." << std::endl;
}
void RunAllTests() {
    SimpleTradeTest();
    InvalidRegistrationTest();
}


int main() {
    RunAllTests();

}