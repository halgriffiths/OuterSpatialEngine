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

    int NUM_TRADERS_EACH_TYPE = 10;
    int NUM_TICKS = 50;

    double STARTING_MONEY = 20.0;
    int SAMPLE_ID = 0;
    int SAMPLE_ID2 = 1;

    std::vector<std::string> tracked_goods = {"food", "wood"};
    std::vector<std::string> tracked_roles = {"farmer", "woodcutter"};
    std::map<std::string, std::vector<std::pair<double, double>>> net_supply_metrics;
    std::map<std::string, std::vector<std::pair<double, double>>> avg_price_metrics, avg_trades_metrics, avg_asks_metrics, avg_bids_metrics;

    std::map<std::string, std::vector<std::pair<double, double>>> num_alive_metrics;

    std::map<std::string, std::vector<std::pair<double, double>>> sample1_metrics;
    std::map<std::string, std::vector<std::pair<double, double>>> sample2_metrics;

    sample1_metrics["money"] = {};
    sample2_metrics["money"] = {};
    for (auto& good : tracked_goods) {
        net_supply_metrics[good] = {};
        avg_price_metrics[good] = {};
        avg_trades_metrics[good] = {};
        avg_asks_metrics[good] = {};
        avg_bids_metrics[good] = {};

        sample1_metrics[good] = {};
        sample2_metrics[good] = {};
    }
    for (auto& role : tracked_roles) {
        num_alive_metrics[role] = {};
    }

    // Setup scenario
    auto food = Commodity("food");
    auto wood = Commodity("wood");
    auto tools = Commodity("tools");

    std::vector<std::vector<std::pair<double, double>>> supply_metrics;
    auto auction_house = std::make_shared<AuctionHouse>(0, Log::DEBUG);
    auction_house->RegisterCommodity(food);
    auction_house->RegisterCommodity(wood);
    auction_house->RegisterCommodity(tools);

    std::vector<std::shared_ptr<AITrader>> all_traders;
    std::vector<InventoryItem> DefaultFarmerInv{{food, 0, 0}, {wood, 2, 3}, {tools, 1, 1}};
    std::vector<InventoryItem> DefaultWoodcutterInv{{food, 2, 3}, {wood, 0, 0}, {tools, 1, 1}};

    int max_id = 1;
    std::shared_ptr<AITrader> new_trader;
    for (int i = 0; i < NUM_TRADERS_EACH_TYPE; i++) {
        new_trader = CreateAndRegister(max_id, auction_house, std::make_shared<RoleFarmer>(), "farmer", STARTING_MONEY, 20, DefaultFarmerInv, Log::WARN);
        all_traders.push_back(new_trader);
        max_id++;
        new_trader = CreateAndRegister(max_id, auction_house, std::make_shared<RoleWoodcutter>(), "woodcutter", STARTING_MONEY, 20, DefaultWoodcutterInv, Log::WARN);
        all_traders.push_back(new_trader);
        max_id++;
    }

    all_traders[SAMPLE_ID]->logger.verbosity = Log::DEBUG;
    all_traders[SAMPLE_ID2]->logger.verbosity = Log::DEBUG;

    for (int curr_tick = 0; curr_tick < NUM_TICKS; curr_tick++) {
        std::map<std::string, int> num_alive;
        for (auto& role : tracked_roles) {
            num_alive[role] = 0;
        }

        for (int i = 0; i < all_traders.size(); i++) {
            if (!all_traders[i]->destroyed) {
                all_traders[i]->Tick();

                num_alive[all_traders[i]->class_name] += 1;
            } else {
                //trader died, add new trader?
                if (rand() % 2) {
                    // WOODCUTTER
                    all_traders[i] = CreateAndRegister(max_id, auction_house, std::make_shared<RoleWoodcutter>(), "woodcutter", STARTING_MONEY, 20, DefaultWoodcutterInv, Log::WARN);
                } else {
                    //FARMER
                    all_traders[i] = CreateAndRegister(max_id, auction_house, std::make_shared<RoleFarmer>(), "farmer", STARTING_MONEY, 20, DefaultFarmerInv, Log::WARN);
                }

                max_id++;
            }
        }

        auction_house->Tick();
        std::cout << "\n ------ END OF TICK " << curr_tick << " -----\n";

        // collect metrics
        for (auto& good : tracked_goods) {
            double asks = auction_house->AverageHistoricalAsks(good, 1);
            double bids = auction_house->AverageHistoricalBids(good, 1);

            avg_price_metrics[good].emplace_back(curr_tick, auction_house->AverageHistoricalPrice(good, 1));
            avg_trades_metrics[good].emplace_back(curr_tick, auction_house->AverageHistoricalTrades(good, 1));
            avg_asks_metrics[good].emplace_back(curr_tick, asks);
            avg_bids_metrics[good].emplace_back(curr_tick, bids);

            net_supply_metrics[good].emplace_back(curr_tick, asks-bids);

            sample1_metrics[good].emplace_back(curr_tick, all_traders[SAMPLE_ID]->Query(good));
            sample2_metrics[good].emplace_back(curr_tick, all_traders[SAMPLE_ID2]->Query(good));
        }
        for (auto& role : tracked_roles) {
            num_alive_metrics[role].emplace_back(curr_tick, num_alive[role]);
        }

        sample1_metrics["money"].emplace_back(curr_tick, all_traders[SAMPLE_ID]->money);
        sample2_metrics["money"].emplace_back(curr_tick, all_traders[SAMPLE_ID2]->money);
    }



    // Plot results
    Gnuplot gp;
    gp << "set multiplot layout 3,2\n";
    gp << "set offsets 0, 0, 1, 0\n";
    gp << "set title 'Prices'\n";
    auto plots = gp.plotGroup();
    for (auto& good : tracked_goods) {
        plots.add_plot1d(avg_price_metrics[good], "with lines title '"+good+std::string("'"));
    }
    gp << plots;

    gp << "set title 'Num successful trades'\n";
    plots = gp.plotGroup();
    for (auto& good : tracked_goods) {
        plots.add_plot1d(avg_trades_metrics[good], "with lines title '"+good+std::string("'"));
    }
    gp << plots;

    gp << "set title 'Demographics'\n";
    plots = gp.plotGroup();
    for (auto& role : tracked_roles) {
        plots.add_plot1d(num_alive_metrics[role], "with lines title '"+role+std::string("'"));
    }
    gp << plots;

    gp << "set title 'Net supply'\n";
    plots = gp.plotGroup();
    for (auto& good : tracked_goods) {
        plots.add_plot1d(net_supply_metrics[good], "with lines title '"+good+std::string("'"));
    }
    gp << plots;

    gp << "set title 'Sample Trader Detail - 1'\n";
    plots = gp.plotGroup();
    for (auto& good : tracked_goods) {
        plots.add_plot1d(sample1_metrics[good], "with lines title '"+good+std::string("'"));
    }
    plots.add_plot1d(sample1_metrics["money"], "with lines title 'money'");
    gp << plots;

    gp << "set title 'Sample Trader Detail - 2'\n";
    plots = gp.plotGroup();
    for (auto& good : tracked_goods) {
        plots.add_plot1d(sample2_metrics[good], "with lines title '"+good+std::string("'"));
    }
    plots.add_plot1d(sample2_metrics["money"], "with lines title 'money'");
    gp << plots;
}