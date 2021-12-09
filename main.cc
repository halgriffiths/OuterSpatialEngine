//
// Created by henry on 06/12/2021.
//
#include <iostream>
#include <cassert>
#include "BazaarBot.h"
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


// ---------------- MAIN ----------
int main() {
    auto food = Commodity("food");
    auto wood = Commodity("wood");
    auto tools = Commodity("tools");

    auto auction_house = std::make_shared<AuctionHouse>(0, Log::DEBUG);
    auction_house->RegisterCommodity(food);
    auction_house->RegisterCommodity(wood);
    auction_house->RegisterCommodity(tools);

    std::vector<InventoryItem> WoodAndToolsInv {{food, 0, 10}, {wood, 5, 5}, {tools,1 , 1}};
    std::vector<InventoryItem> WoodNoToolsInv {{food, 0, 10}, {wood, 5, 5}, {tools, 0, 1}};
    std::vector<InventoryItem> NoWoodInv {{food, 0, 10}, {wood, 0, 5}, {tools, 1, 1}};

    auto FarmerWithWoodAndTools = CreateAndRegisterFarmer(1,  WoodAndToolsInv, auction_house);
    auto FarmerWithWoodNoTools = CreateAndRegisterFarmer(2, WoodNoToolsInv, auction_house);
    auto FarmerNoWood = CreateAndRegisterFarmer(3, NoWoodInv, auction_house);

    std::vector<InventoryItem> LoadsOfWoodInv {{wood, 25, 5}};
    auto FarmerLoadsWood = CreateAndRegisterFarmer(4, LoadsOfWoodInv, auction_house);

    auction_house->Tick();

}