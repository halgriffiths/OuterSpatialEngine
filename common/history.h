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

    void add(std::string name, double amount) {
        if (log.count(name) != 1) {
            return;// no entry found
        }
        log[name].push_back(amount);
    }

    double average(std::string name, int range) {
        if (log.count(name) != 1) {
            return 0;// no entry found
        }
        int log_length = log[name].size();
        if (log_length < range) {
            range = log_length;
        }

        double total = 0;
        for (int i = 0; i < range; i++) {
            total += log[name][i];
        }
        return total/range;
    }

};

class History{
public:
    HistoryLog prices;
    HistoryLog asks;
    HistoryLog bids;
    HistoryLog trades;

    History()
        : prices(HistoryLog(PRICE))
        , asks(HistoryLog(ASK))
        , bids(HistoryLog(BID))
        , trades(HistoryLog(TRADE)) {};

    void initialise(const std::string& name) {
        prices.initialise(name);
        asks.initialise(name);
        bids.initialise(name);
        trades.initialise(name);
    }
};

#endif//CPPBAZAARBOT_HISTORY_H
