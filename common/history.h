//
// Created by henry on 07/12/2021.
//

#ifndef CPPBAZAARBOT_HISTORY_H
#define CPPBAZAARBOT_HISTORY_H
#include <map>
#include <vector>

enum LogType {
    PRICE,
    ASK,
    BID,
    TRADE
};

class HistoryLog {
public:
    LogType type;
    std::map<std::string, std::vector<double>> log;

    HistoryLog(LogType log_type)
    : type(log_type) {
        log = {};
    }

    void initialise(const std::string& name) {
        if (log.count(name) > 0) {
            return;// already registered
        }
        double starting_value = 1;
        if (name == "metals") {
            starting_value = 2;
        } else if (name == "tools") {
            starting_value = 3;
        }
        log[name] = {starting_value};
    }

    void add(const std::string& name, double amount) {
        if (log.count(name) != 1) {
            return;// no entry found
        }
        log[name].push_back(amount);
    }

    double average(const std::string& name, int range) {
        if (log.count(name) != 1) {
            return 0;// no entry found
        }
        int log_length = log[name].size();
        if (log_length < range) {
            range = log_length;
        }

        double total = 0;
        for (int i = log_length - range; i < log_length; i++) {
            total += log[name][i];
        }
        return total/range;
    }

    double variance(const std::string& name, int window) {
        double sum = 0;
        for (int i = log[name].size() - window; i < log[name].size(); i++) {
            sum += log[name][i];
        }
        double m =  sum / log[name].size();

        double accum = 0.0;
        for (int i = log[name].size() - window; i < log[name].size(); i++) {
            accum += (log[name][i] - m) * (log[name][i] - m);
        }

        return sqrt(accum / (log[name].size()-1));
    }

    double percentage_change(const std::string& name, int window) {
        double prev_value;
        if (window <= log[name].size()) {
            prev_value = log[name][log[name].size() - window];
        } else {
            prev_value = log[name][0];
        }

        double curr_value = log[name].back();
        return 100*(curr_value- prev_value)/prev_value;
    }

};

class History{
public:
    HistoryLog buy_prices;
    HistoryLog sell_prices;
    HistoryLog asks;
    HistoryLog bids;
    HistoryLog trades;

    History()
        : buy_prices(HistoryLog(PRICE))
        , sell_prices(HistoryLog(PRICE))
        , asks(HistoryLog(ASK))
        , bids(HistoryLog(BID))
        , trades(HistoryLog(TRADE)) {};

    void initialise(const std::string& name) {
        buy_prices.initialise(name);
        sell_prices.initialise(name);
        asks.initialise(name);
        bids.initialise(name);
        trades.initialise(name);
    }
};

#endif//CPPBAZAARBOT_HISTORY_H
