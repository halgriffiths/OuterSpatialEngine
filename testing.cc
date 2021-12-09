//
// Created by henry on 09/12/2021.
//
#include <gtest/gtest.h>
#include "BazaarBot.h"
// Demonstrate some basic assertions.

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// test driver for BazaarBot library
std::shared_ptr<BasicTrader> CreateAndRegisterBasic(int id,
                                               const std::vector<std::pair<Commodity, int>>& inv,
                                               const std::shared_ptr<AuctionHouse>& auction_house) {

    std::vector<InventoryItem> inv_vector;
    for (const auto &item : inv) {
        inv_vector.emplace_back(item.first.name, item.second);
    }
    auto trader = std::make_shared<BasicTrader>(id, auction_house, std::nullopt, "test_class", 100.0, 50, inv_vector, Log::WARN);

    trader->SendMessage(*Message(id).AddRegisterRequest(std::move(RegisterRequest(trader->id, trader))), auction_house->id);
    trader->Tick();
    return trader;
}
std::shared_ptr<BasicTrader> CreateAndRegister(int id,
                                               const std::shared_ptr<AuctionHouse>& auction_house,
                                               std::shared_ptr<Role> AI_logic,
                                               const std::string& name,
                                               double starting_money,
                                               double inv_capacity,
                                               const std::vector<InventoryItem>& inv,
                                               Log::LogLevel log_level
                                               ) {

    auto trader = std::make_shared<BasicTrader>(id, auction_house, std::move(AI_logic), name, starting_money, inv_capacity, inv, log_level);
    trader->SendMessage(*Message(id).AddRegisterRequest(std::move(RegisterRequest(trader->id, trader))), auction_house->id);
    trader->Tick();
    return trader;
}
std::shared_ptr<BasicTrader> CreateAndRegisterFarmer(int id,
                                                 const std::vector<InventoryItem>& inv,
                                                 const std::shared_ptr<AuctionHouse>& auction_house) {
    std::shared_ptr<Role> AI_logic;
    AI_logic = std::make_shared<RoleFarmer>();
    return CreateAndRegister(id, auction_house, AI_logic, "farmer", 100.0, 50, inv, Log::WARN);
}

// ------------- TESTS ------------
TEST(RoleTests, ProduceFarmer) {
    auto auction_house = std::make_shared<AuctionHouse>(0, Log::ERROR);
    
    auto food = Commodity("food");
    auto wood = Commodity("wood");
    auto tools = Commodity("tools");
    std::vector<InventoryItem> WoodAndToolsInv {{food, 0, 10}, {wood, 5, 5}, {tools,1 , 1}};
    std::vector<InventoryItem> WoodNoToolsInv {{food, 0, 10}, {wood, 5, 5}, {tools, 0, 1}};
    std::vector<InventoryItem> NoWoodInv {{food, 0, 10}, {wood, 0, 5}, {tools, 1, 1}};

    auto FarmerWithWoodAndTools = CreateAndRegisterFarmer(1,  WoodAndToolsInv, auction_house);
    auto FarmerWithWoodNoTools = CreateAndRegisterFarmer(2, WoodNoToolsInv, auction_house);
    auto FarmerNoWood = CreateAndRegisterFarmer(3, NoWoodInv, auction_house);

    auction_house->Tick();
    ASSERT_EQ(auction_house->NumKnownTraders(), 3);

    // Guy with wood and tools makes 6 food
    ASSERT_EQ(FarmerWithWoodAndTools->Query("food"), 6);
    // Guy with wood but no tools makes 3 food
    ASSERT_EQ(FarmerWithWoodNoTools->Query("food"), 3);
    // Guy with no wood makes no food
    ASSERT_EQ(FarmerNoWood->Query("food"), 0);
}

TEST(BasicTests, SimpleTradeTest) {
    
    auto comm = Commodity("comm");
    auto comm1 = Commodity("comm1");
    auto auction_house = std::make_shared<AuctionHouse>(0, Log::ERROR);
    auction_house->RegisterCommodity(comm);
    auction_house->RegisterCommodity(comm1);

    std::vector<std::pair<Commodity, int>> c_v = {{comm, 5}, {comm1,9}};
    auto Alice = CreateAndRegisterBasic(1, c_v, auction_house);
    auto Bob = CreateAndRegisterBasic(2, c_v, auction_house);
    auto Charlie = CreateAndRegisterBasic(3, c_v, auction_house);
    auto Dan =  CreateAndRegisterBasic(4, c_v, auction_house);

    auction_house->Tick();
    ASSERT_EQ(auction_house->NumKnownTraders(), 4);

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
    ASSERT_EQ(Alice->Query("comm"), 2);

    ASSERT_EQ(Bob->Query("comm"), 4);

    ASSERT_EQ(Charlie->Query("comm"), 9);

    ASSERT_EQ(Dan->Query("comm"), 5);
}

TEST(BasicTests, InvalidRegistrationTest) {
    auto comm = Commodity("comm");
    auto comm1 = Commodity("comm1");
    auto auction_house = std::make_shared<AuctionHouse>(0, Log::ERROR);
    auction_house->RegisterCommodity(comm);
    auction_house->RegisterCommodity(comm1);

    std::vector<std::pair<Commodity, int>> c_v = {{comm, 5}, {comm1,9}};
    auto Alice = CreateAndRegisterBasic(1, c_v, auction_house);
    auto Bob = CreateAndRegisterBasic(1, c_v, auction_house);

    Alice->Tick();
    Bob->Tick();

    auction_house->Tick();

    ASSERT_EQ(auction_house->NumKnownTraders(), 1);
}