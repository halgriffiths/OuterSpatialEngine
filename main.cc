//
// Created by henry on 06/12/2021.
//
#include <iostream>
#include "BazaarBot.h"

void scrap() {
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
    auto WoodcutterLoadsWood = CreateAndRegister(4, auction_house, std::make_shared<RoleWoodcutter>(), "woodcutter", 100.0, 50, LoadsOfWoodInv, Log::DEBUG);

    auction_house->Tick();
}
// ---------------- MAIN ----------
int main() {
    scrap();
}