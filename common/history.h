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
    int max_size = 6000; //10 min worth of data @ 100ms frametime
public:
    LogType type;
    std::map<std::string, std::vector<std::pair<double, std::int64_t>>> log;

    HistoryLog(LogType log_type)
    : type(log_type) {
        log = {};
    }

    void initialise(const std::string& name) {
        if (log.count(name) > 0) {
            return;// already registered
        }
        double starting_value = 10;
        log[name].emplace_back(starting_value, to_unix_timestamp_ns(std::chrono::system_clock::now()));
    }

    void add(const std::string& name, double amount) {
        if (log.count(name) != 1) {
            return;// no entry found
        }
        if (log[name].size() == max_size) {
            log[name].erase(log[name].begin());
        }
        log[name].emplace_back(amount, to_unix_timestamp_ns(std::chrono::system_clock::now()));
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
            total += log[name][i].first;
        }
        return total/range;
    }
    // time-based average
    double t_average(const std::string& name, std::int64_t start_time) {
        if (log.count(name) != 1) {
            return 0;// no entry found
        }
        double total = 0;
        int range = 0;
        auto it = log[name].rbegin();
        while (it != log[name].rend() && it->second >= start_time) {
            total += it->first;
            range++;
            it++;
        }
        return total/range;
    }

    double percentage_change(const std::string& name, int window) {
        double prev_value;
        if (window <= log[name].size()) {
            prev_value = log[name][log[name].size() - window].first;
        } else {
            prev_value = log[name][0].first;
        }

        double curr_value = log[name].back().first;
        return 100*(curr_value- prev_value)/prev_value;
    }

    double t_percentage_change(const std::string& name, std::int64_t time) {
        double prev_value;
        auto it = log[name].rbegin();
        while (it != log[name].rend() && it->second > time) {
            it++;
        }
        if (it == log[name].rend()) {
            prev_value = log[name].back().first;
        } else {
            prev_value = it->first;
        }

        double curr_value = log[name].back().first;
        return 100*(curr_value- prev_value)/prev_value;
    }
};

class History{
public:
    HistoryLog prices;
    HistoryLog buy_prices;
    HistoryLog asks;
    HistoryLog bids;
    HistoryLog trades;

    History()
        : prices(HistoryLog(PRICE))
        , buy_prices(HistoryLog(PRICE))
        , asks(HistoryLog(ASK))
        , bids(HistoryLog(BID))
        , trades(HistoryLog(TRADE)) {};

    void initialise(const std::string& name) {
        prices.initialise(name);
        buy_prices.initialise(name);
        asks.initialise(name);
        bids.initialise(name);
        trades.initialise(name);
    }
};

#endif//CPPBAZAARBOT_HISTORY_H
