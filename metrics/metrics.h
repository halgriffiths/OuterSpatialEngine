//
// Created by henry on 15/12/2021.
//

#ifndef CPPBAZAARBOT_METRICS_H
#define CPPBAZAARBOT_METRICS_H

#include "../traders/AI_trader.h"

class GlobalMetrics {
public:
    std::vector<std::string> tracked_goods;
    std::vector<std::string> tracked_roles;
    int total_deaths = 0;
    double avg_overall_age = 0;
    std::map<std::string, int> deaths_per_class;
    std::map<std::string, double> age_per_class;
private:

    int curr_tick = 0;
    int SAMPLE_ID = 0;
    int SAMPLE_ID2 = 1;
    std::map<std::string, std::vector<std::pair<double, double>>> net_supply_metrics;
    std::map<std::string, std::vector<std::pair<double, double>>> avg_price_metrics;
    std::map<std::string, std::vector<std::pair<double, double>>> avg_trades_metrics;
    std::map<std::string, std::vector<std::pair<double, double>>> avg_asks_metrics;
    std::map<std::string, std::vector<std::pair<double, double>>> avg_bids_metrics;
    std::map<std::string, std::vector<std::pair<double, double>>> num_alive_metrics;
    std::map<std::string, std::vector<std::pair<double, double>>> sample1_metrics;
    std::map<std::string, std::vector<std::pair<double, double>>> sample2_metrics;

public:
    GlobalMetrics(std::vector<std::string> tracked_goods, std::vector<std::string> tracked_roles)
            : tracked_goods(tracked_goods)
            , tracked_roles(tracked_roles) {
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
            age_per_class[role] = 0;
            deaths_per_class[role] = 0;
        }
    }

    void CollectMetrics(std::shared_ptr<AuctionHouse> auction_house, std::vector<std::shared_ptr<AITrader>> all_traders, std::map<std::string, int> num_alive) {
        for (auto& good : tracked_goods) {
            double asks = auction_house->AverageHistoricalAsks(good, 10);
            double bids = auction_house->AverageHistoricalBids(good, 10);

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

        curr_tick++;
    }
    void plot_verbose() {
        // Plot results
        Gnuplot gp;
        gp << "set multiplot layout 2,2\n";
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

//    gp << "set title 'Sample Trader Detail - 1'\n";
//    plots = gp.plotGroup();
//    for (auto& good : tracked_goods) {
//        plots.add_plot1d(sample1_metrics[good], "with lines title '"+good+std::string("'"));
//    }
//    plots.add_plot1d(sample1_metrics["money"], "with lines title 'money'");
//    gp << plots;
//
//    gp << "set title 'Sample Trader Detail - 2'\n";
//    plots = gp.plotGroup();
//    for (auto& good : tracked_goods) {
//        plots.add_plot1d(sample2_metrics[good], "with lines title '"+good+std::string("'"));
//    }
//    plots.add_plot1d(sample2_metrics["money"], "with lines title 'money'");
//    gp << plots;
    }

    void plot_terse(int window = 0) {
        // Plot results
        Gnuplot gp;
        gp << "set term dumb 180 65\n";
        if (window > 0 && window < curr_tick) {
            gp << "set xrange [" + std::to_string(curr_tick - window) + ":"+ std::to_string(curr_tick) +"]\n";
        }
        gp << "set offsets 0, 0, 1, 0\n";
        gp << "set title 'Prices'\n";
        auto plots = gp.plotGroup();
        for (auto& good : tracked_goods) {
            plots.add_plot1d(avg_price_metrics[good], "with lines title '"+good+std::string("'"));
        }
        gp << plots;
    }

    void plot_terse_tofile() {
        Gnuplot gp(std::fopen("plot.gnu", "w"));
        gp << "set term dumb 180 65\n";
        gp << "set offsets 0, 0, 1, 0\n";
        gp << "set title 'Prices'\n";
        auto plots = gp.plotGroup();
        gp << "plot";
        for (auto& good : tracked_goods) {
            gp << gp.file1d(avg_price_metrics[good], good+".dat") << "with lines title '"+good+std::string("',");
        }
        gp << std::endl; //flush result
    }

    void TrackDeath(std::string& class_name, int age) {
        avg_overall_age = (avg_overall_age*total_deaths + age)/(total_deaths+1);
        total_deaths++;

        age_per_class[class_name] = (age_per_class[class_name]*deaths_per_class[class_name] + age)/(deaths_per_class[class_name]+1);
        deaths_per_class[class_name]++;
        std::map<std::string, double> age_per_class;
    }
};
#endif//CPPBAZAARBOT_METRICS_H
