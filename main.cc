//
// Created by henry on 06/12/2021.
//
#include <iostream>
#include "gnuplot-iostream.h"
#include "BazaarBot.h"

//plotting stuff
#include <vector>
#include <boost/bind.hpp>

#include <iomanip>

void plot_verbose(std::vector<std::string>& tracked_goods,
                  std::vector<std::string>& tracked_roles,
                  std::map<std::string, std::vector<std::pair<double, double>>>& avg_price_metrics,
                  std::map<std::string, std::vector<std::pair<double, double>>>& avg_trades_metrics,
                  std::map<std::string, std::vector<std::pair<double, double>>>& num_alive_metrics,
                  std::map<std::string, std::vector<std::pair<double, double>>>& net_supply_metrics,
                  std::map<std::string, std::vector<std::pair<double, double>>>& sample1_metrics,
                  std::map<std::string, std::vector<std::pair<double, double>>>& sample2_metrics) {
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

void plot_terse(std::vector<std::string>& tracked_goods,
                  std::map<std::string, std::vector<std::pair<double, double>>>& avg_price_metrics) {
    // Plot results
    Gnuplot gp;
    gp << "set term dumb 180 65\n";
    gp << "set offsets 0, 0, 1, 0\n";
    gp << "set title 'Prices'\n";
    auto plots = gp.plotGroup();
    for (auto& good : tracked_goods) {
        plots.add_plot1d(avg_price_metrics[good], "with lines title '"+good+std::string("'"));
    }
    gp << plots;
}

void plot_terse_tofile(std::vector<std::string>& tracked_goods,
                std::map<std::string, std::vector<std::pair<double, double>>>& avg_price_metrics) {
    Gnuplot gp(std::fopen("plot.gnu", "w"));
    gp << "set term dumb 180 65\n";
    gp << "set offsets 0, 0, 1, 0\n";
    gp << "set title 'Prices'\n";
    auto plots = gp.plotGroup();
    gp << "plot";
    for (auto& good : tracked_goods) {
        gp << gp.file1d(avg_price_metrics["food"], good+".dat") << "with lines title '"+good+std::string("',");
    }
    gp << std::endl; //flush result
}
// ---------------- MAIN ----------
int main() {

    int NUM_TRADERS_EACH_TYPE = 10;
    int NUM_TICKS = 500;
//    int NUM_TICKS_PER_STEP = 10;
    double STARTING_MONEY = 20.0;
    int SAMPLE_ID = 1;
    int SAMPLE_ID2 = 3;


    std::vector<std::string> tracked_goods = {"food", "wood", "fertilizer", "ore", "metal", "tools"};
    std::vector<std::string> tracked_roles = {"farmer", "woodcutter", "composter", "miner", "refiner", "blacksmith"};

    std::map<std::string, Commodity> comm;
    comm.emplace("food", Commodity("food", 0.5));
    comm.emplace("wood", Commodity("wood", 1));
    comm.emplace("ore", Commodity("ore", 1));
    comm.emplace("metal", Commodity("metal", 1));
    comm.emplace("tools", Commodity("tools", 1));
    comm.emplace("fertilizer", Commodity("fertilizer", 0.1));

    std::map<std::string, std::vector<InventoryItem>> inv;
    inv.emplace("farmer", std::vector<InventoryItem>{{comm["food"], 1, 0},
                                                         {comm["tools"], 1, 1},
                                                         {comm["wood"], 0, 3},
                                                         {comm["fertilizer"], 0, 3}});

    inv.emplace("miner", std::vector<InventoryItem>{{comm["food"], 1, 3},
                                                         {comm["tools"], 1, 1},
                                                         {comm["ore"], 0, 0}});

    inv.emplace("refiner", std::vector<InventoryItem>{{comm["food"], 1, 3},
                                                        {comm["tools"], 1, 1},
                                                        {comm["ore"], 0, 5},
                                                      {comm["metal"], 0, 0}});

    inv.emplace("woodcutter", std::vector<InventoryItem>{{comm["food"], 1, 3},
                                                        {comm["tools"], 1, 1},
                                                        {comm["wood"], 0, 0}});

    inv.emplace("blacksmith", std::vector<InventoryItem>{{comm["food"], 1, 3},
                                                    {comm["tools"], 0, 1},
                                                    {comm["metal"], 0, 5}});

    inv.emplace("composter", std::vector<InventoryItem>{{comm["food"], 1, 3},
                                                        {comm["fertilizer"], 0, 0}});



    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_real_distribution<> random_money(0.9*STARTING_MONEY, 1.1*STARTING_MONEY); // define the range
    std::uniform_int_distribution<> random_job(0, (int) tracked_roles.size() - 1); // define the range

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


    auto auction_house = std::make_shared<AuctionHouse>(0, Log::WARN);
    for (auto& item : comm) {
        auction_house->RegisterCommodity(item.second);
    }

    auto make_agent = [&] (const std::string& class_name, int curr_id) {
      if (class_name == "farmer") {
          return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleFarmer>(), class_name, random_money(gen), 20, inv[class_name], Log::WARN);
      } else if (class_name == "woodcutter") {
          return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleWoodcutter>(), class_name, random_money(gen), 20, inv[class_name], Log::WARN);
      } else if (class_name == "miner") {
          return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleMiner>(), class_name, random_money(gen), 20, inv[class_name], Log::WARN);
      } else if (class_name == "refiner") {
          return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleRefiner>(), class_name, random_money(gen), 20, inv[class_name], Log::WARN);
      } else if (class_name == "blacksmith") {
          return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleBlacksmith>(), class_name, random_money(gen), 20, inv[class_name], Log::WARN);
      } else if (class_name == "composter") {
          return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleComposter>(), class_name, random_money(gen), 20, inv[class_name], Log::WARN);
      } else {
          std::cout << "Error: Invalid class type passed to make_agent lambda" << std::endl;
      }
      return std::shared_ptr<AITrader>();
    };

    std::vector<std::shared_ptr<AITrader>> all_traders;
    int max_id = 1;

    std::shared_ptr<AITrader> new_trader;
    for (int i = 0; i < NUM_TRADERS_EACH_TYPE; i++) {
        for (auto& role : tracked_roles) {
            all_traders.push_back(make_agent(role, max_id));
            max_id++;
        }
    }

//    all_traders[SAMPLE_ID]->logger.verbosity = Log::DEBUG;
//    all_traders[SAMPLE_ID2]->logger.verbosity = Log::DEBUG;
    auto fake_trader = std::make_shared<FakeTrader>(max_id, auction_house);
    fake_trader->SendMessage(*Message(max_id).AddRegisterRequest(std::move(RegisterRequest(max_id, fake_trader))), auction_house->id);
    fake_trader->Tick();

    fake_trader->RegisterShortage("ore", 3, 120, 20);
    fake_trader->RegisterSurplus("wood", -0.9, 320, 20);
    max_id++;

    for (int curr_tick = 0; curr_tick < NUM_TICKS; curr_tick++) {
        std::map<std::string, int> num_alive;
        for (auto& role : tracked_roles) {
            num_alive[role] = 0;
        }
        fake_trader->Tick();
        for (int i = 0; i < all_traders.size(); i++) {
            if (!all_traders[i]->destroyed) {
                all_traders[i]->Tick();

                num_alive[all_traders[i]->class_name] += 1;
            } else {
                //trader died, add new trader?
                int new_job = random_job(gen);
                all_traders[i] = make_agent(tracked_roles[new_job], max_id);
                max_id++;
            }
        }

        auction_house->Tick();
//        std::cout << "\n ------ END OF TICK " << curr_tick << " -----\n";

        // collect metrics
        for (auto& good : tracked_goods) {
            double asks = auction_house->AverageHistoricalAsks(good, 1);
            double bids = auction_house->AverageHistoricalBids(good, 1);

            avg_price_metrics[good].emplace_back(curr_tick, auction_house->AverageHistoricalMidPrice(good, 5));
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

    std::cout << "Simulated " << NUM_TICKS << " with " << all_traders.size() << " agents." << std::endl;
    std::cout << "Overall statistics" << std::endl;


    std::cout << std::fixed;
    std::cout << std::setprecision(2);
    for (auto& good : tracked_goods) {
        std::cout << "\t" << good << ": " << std::endl;
        std::cout << "\t\tMean price: $" << auction_house->AverageHistoricalBuyPrice(good, NUM_TICKS) << std::endl;
        std::cout << "\t\tVariance: $" << auction_house->StdDev(good) << std::endl;
    }
    int richest_index = 0;
    int max_cash = 0;
    for (int i = 0; i < all_traders.size(); i++) {
        if (all_traders[i]->money > max_cash) {
            max_cash = all_traders[i]->money;
            richest_index = i;
        }
    }

    plot_terse(tracked_goods,avg_price_metrics);
//    plot_verbose(tracked_goods,
//                      tracked_roles,
//                      avg_price_metrics,
//                      avg_trades_metrics,
//                      num_alive_metrics,
//                      net_supply_metrics,
//                      sample1_metrics,
//                      sample2_metrics);
}