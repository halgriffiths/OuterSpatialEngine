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

    std::vector<std::pair<double, double>> avg_food_price, avg_food_trades, avg_food_asks, avg_food_bids;
    std::vector<std::pair<double, double>> avg_wood_price, avg_wood_trades, avg_wood_asks, avg_wood_bids;

    std::vector<std::pair<double, double>> num_alive_farmers;
    std::vector<std::pair<double, double>> num_alive_woodcutters;

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

    auto FarmerWithWoodAndTools = CreateAndRegisterFarmer(1,  WoodAndToolsInv, auction_house, 20.0);
    auto FarmerWithWoodNoTools = CreateAndRegisterFarmer(2, WoodNoToolsInv, auction_house, 20.0);
    auto FarmerNoWood = CreateAndRegisterFarmer(3, NoWoodInv, auction_house, 20.0);

    std::vector<InventoryItem> LoadsOfWoodInv {{wood, 10, 0}, {food, 1, 5}, {tools,1 , 1}};
    auto WoodcutterNoFood = CreateAndRegister(4, auction_house, std::make_shared<RoleWoodcutter>(), "woodcutter", 20.0, 50, LoadsOfWoodInv, Log::WARN);

    std::vector<InventoryItem> LoadsOfWoodAndFoodInv {{wood, 10, 0}, {food, 10, 5}, {tools,1 , 1}};
    auto RichWoodcutter = CreateAndRegister(4, auction_house, std::make_shared<RoleWoodcutter>(), "woodcutter", 20.0, 50, LoadsOfWoodAndFoodInv, Log::WARN);

    std::vector<std::shared_ptr<BasicTrader>> all_traders = {FarmerWithWoodAndTools, FarmerWithWoodNoTools, FarmerNoWood, WoodcutterNoFood, RichWoodcutter};
    for (int curr_tick = 0; curr_tick < 100; curr_tick++) {
        int num_farmers = 0;
        int num_woodcutters = 0;

        for (const auto& t : all_traders) {
            t->Tick();
            if (!t->destroyed) {
                if (t->class_name == "farmer") {
                    num_farmers++;
                } else if (t->class_name == "woodcutter") {
                    num_woodcutters++;
                }
            }
        }

        auction_house->Tick();
        std::cout << "\n ------ END OF TICK " << curr_tick << " -----\n";


        num_alive_farmers.emplace_back(curr_tick, num_farmers);
        num_alive_woodcutters.emplace_back(curr_tick, num_woodcutters);
        // collect metrics
        avg_food_price.emplace_back(curr_tick, auction_house->AverageHistoricalPrice("food", 1));
        avg_food_trades.emplace_back(curr_tick, auction_house->AverageHistoricalTrades("food", 1));
        avg_food_asks.emplace_back(curr_tick, auction_house->AverageHistoricalAsks("food", 1));
        avg_food_bids.emplace_back(curr_tick, auction_house->AverageHistoricalBids("food", 1));

        avg_wood_price.emplace_back(curr_tick, auction_house->AverageHistoricalPrice("wood", 1));
        avg_wood_trades.emplace_back(curr_tick, auction_house->AverageHistoricalTrades("wood", 1));
        avg_wood_asks.emplace_back(curr_tick, auction_house->AverageHistoricalAsks("wood", 1));
        avg_wood_bids.emplace_back(curr_tick, auction_house->AverageHistoricalBids("wood", 1));
    }




    // Plot results
    Gnuplot gp;
    gp << "set multiplot layout 2,2\n";
    gp << "set offsets 0, 0, 1, 0\n";
    gp << "set title 'Prices'\n";
    gp << "set yrange [-0.1:]\n";
    auto plots = gp.plotGroup();
    plots.add_plot1d(avg_food_price, "with lines title 'food'");
    plots.add_plot1d(avg_wood_price, "with lines title 'wood'");
    gp << plots;

    gp << "set title 'Num successful trades'\n";
    plots = gp.plotGroup();
    plots.add_plot1d(avg_food_trades, "with lines title 'food'");
    plots.add_plot1d(avg_wood_trades, "with lines title 'wood'");
    gp << plots;

    gp << "set title 'Demographics'\n";
    plots = gp.plotGroup();
    plots.add_plot1d(num_alive_farmers, "with lines title 'Farmers'");
    plots.add_plot1d(num_alive_woodcutters, "with lines title 'Woodcutters'");
    gp << plots;
}