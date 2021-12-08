#include <utility>

//
// Created by henry on 06/12/2021.
//

#ifndef CPPBAZAARBOT_METRICS_H
#define CPPBAZAARBOT_METRICS_H
namespace Log{
    enum LogLevel {
        ERROR,
        WARN,
        INFO, //Received messages (but not sent message) are reported
        DEBUG //Everything is logged
    };
}

namespace {
    const char* LogLevelToCString(Log::LogLevel level) {
        const char* level_name;
        if (level == Log::ERROR) {
            level_name = "ERROR";
        } else if (level == Log::WARN) {
            level_name = "WARN";
        } else if (level == Log::INFO) {
            level_name = "INFO";
        } else if (level == Log::DEBUG) {
            level_name = "DEBUG";
        } else {
            level_name = "UNKNOWN";
        }
        return level_name;
    }
}
// Base class makes no logs
class Logger {
public:
    std::string name;

    Logger(std::string owner_name = "none",Log::LogLevel verbosity = Log::ERROR)
    : name(std::move(owner_name))
    , verbosity(verbosity) {};
    virtual void LogInternal(std::string raw_message) {return;};
    virtual void LogSent(int to,Log::LogLevel level, std::string message) {return;};
    virtual void LogReceived(int from,Log::LogLevel level, std::string message) {return;};
    virtual void Log(Log::LogLevel level, std::string message) {return;};
    Log::LogLevel verbosity;
};

class ConsoleLogger : public Logger {
public:
    ConsoleLogger(std::string owner_name,Log::LogLevel verbosity)
        : Logger(std::move(owner_name), verbosity) {};

    void LogInternal(std::string raw_message) override {
        std::cout << raw_message << std::endl;
    }

    void LogSent(int to, Log::LogLevel level, std::string message) override {
        if (level > verbosity) {
            return;
        }
        char header_string[100]; //yolo lmao
        snprintf(header_string, 100, "[%s][Sent    ] %s >>> %d - ",LogLevelToCString(level), name.c_str(), to);
        LogInternal(std::string(header_string) + message);
    }

    void LogReceived(int from,Log::LogLevel level, std::string message) override {
        if (level > verbosity) {
            return;
        }
        char header_string[100]; //yolo lmao
        snprintf(header_string, 100, "[%s][Received] %s <<< %d - ",LogLevelToCString(level), name.c_str(), from);
        LogInternal(std::string(header_string) + message);
    }

    void Log(Log::LogLevel level, std::string message) override {
        if (level > verbosity) {
            return;
        }
        char header_string[30]; //yolo lmao
        snprintf(header_string, 30, "[%s] %s: ",LogLevelToCString(level), name.c_str());
        LogInternal(std::string(header_string) + message);
    }
};
#endif//CPPBAZAARBOT_METRICS_H
