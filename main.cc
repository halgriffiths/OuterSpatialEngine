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

    auto auction_house = std::make_shared<AuctionHouse>(0, Log::DEBUG);
    auction_house->RegisterCommodity(food);
    auction_house->RegisterCommodity(wood);
    auction_house->RegisterCommodity(tools);

    std::vector<std::shared_ptr<BasicTrader>> all_traders;
    std::vector<InventoryItem> DefaultFarmerInv{{food, 0, 0}, {wood, 0, 3}, {tools, 1, 1}};
    std::vector<InventoryItem> DefaultWoodcutterInv{{food, 1, 3}, {wood, 0, 0}, {tools, 1, 1}};

    int max_id = 1;
    std::shared_ptr<BasicTrader> new_trader;
    for (int i = 0; i < 3; i++) {
        new_trader = CreateAndRegister(max_id, auction_house, std::make_shared<RoleFarmer>(), "farmer", 20.0, 20, DefaultFarmerInv, Log::WARN);
        all_traders.push_back(new_trader);
        max_id++;
        new_trader = CreateAndRegister(max_id, auction_house, std::make_shared<RoleWoodcutter>(), "woodcutter", 20.0, 20, DefaultWoodcutterInv, Log::WARN);
        all_traders.push_back(new_trader);
        max_id++;
    }

    for (int curr_tick = 0; curr_tick < 30; curr_tick++) {
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
            } else {
                //trader died, add new trader?

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