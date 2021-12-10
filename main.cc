//
// Created by henry on 06/12/2021.
//
#include <iostream>
#include "gnuplot-iostream.h"
#include "BazaarBot.h"

//plotting stuff
#include <vector>
#include <boost/bind.hpp>



// ---------------- MAIN ----------
int main() {

    std::vector<std::pair<double, double>> avg_food_price, avg_wood_price;

    // Setup scenario
    auto food = Commodity("food");
    auto wood = Commodity("wood");
    auto tools = Commodity("tools");

    auto auction_house = std::make_shared<AuctionHouse>(0, Log::INFO);
    auction_house->RegisterCommodity(food);
    auction_house->RegisterCommodity(wood);
    auction_house->RegisterCommodity(tools);

    std::vector<InventoryItem> WoodAndToolsInv {{food, 0, 10}, {wood, 5, 5}, {tools,1 , 1}};
    std::vector<InventoryItem> WoodNoToolsInv {{food, 0, 10}, {wood, 5, 5}, {tools, 0, 1}};
    std::vector<InventoryItem> NoWoodInv {{food, 0, 10}, {wood, 0, 5}, {tools, 1, 1}};

    auto FarmerWithWoodAndTools = CreateAndRegisterFarmer(1,  WoodAndToolsInv, auction_house, 10.0);
    auto FarmerWithWoodNoTools = CreateAndRegisterFarmer(2, WoodNoToolsInv, auction_house, 10.0);
    auto FarmerNoWood = CreateAndRegisterFarmer(3, NoWoodInv, auction_house, 10.0);

    std::vector<InventoryItem> LoadsOfWoodInv {{wood, 25, 5}};
    auto WoodcutterLoadsWood = CreateAndRegister(4, auction_house, std::make_shared<RoleWoodcutter>(), "woodcutter", 10.0, 50, LoadsOfWoodInv, Log::WARN);
    auto WoodcutterLoadsWood2 = CreateAndRegister(5, auction_house, std::make_shared<RoleWoodcutter>(), "woodcutter", 10.0, 50, LoadsOfWoodInv, Log::WARN);

    for (int curr_tick = 0; curr_tick < 10; curr_tick++) {
        FarmerWithWoodAndTools->Tick();
        FarmerWithWoodNoTools->Tick();
        FarmerNoWood->Tick();
        WoodcutterLoadsWood->Tick();
        WoodcutterLoadsWood2->Tick();
        auction_house->Tick();
        std::cout << "\n ------ END OF TICK " << curr_tick << " -----\n";

        // collect metrics
        avg_food_price.emplace_back(curr_tick, auction_house->AverageHistoricalPrice("food", 2));
        avg_wood_price.emplace_back(curr_tick, auction_house->AverageHistoricalPrice("wood", 2));
    }




    // Plot results
    Gnuplot gp;
    gp << "set yrange [-0.1:2]\n";
    auto plots = gp.plotGroup();
    plots.add_plot1d(avg_food_price, "with lines title 'food'");
    plots.add_plot1d(avg_wood_price, "with lines title 'wood'");
    gp << plots;
}