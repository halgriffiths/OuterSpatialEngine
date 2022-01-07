//
// Created by henry on 17/12/2021.
//

#ifndef CPPBAZAARBOT_DISPLAY_H
#define CPPBAZAARBOT_DISPLAY_H
#include "metrics.h"
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#elif defined(__linux__)
#include <sys/ioctl.h>

#include <utility>
#endif // Windows/Linux

#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <notcurses/notcurses.h>
#include <chrono>
#include <thread>
#include <iostream>

void get_terminal_size(int& width, int& height) {
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    width = (int)(csbi.srWindow.Right-csbi.srWindow.Left+1);
    height = (int)(csbi.srWindow.Bottom-csbi.srWindow.Top+1);
#elif defined(__linux__)
    struct winsize w;
    ioctl(fileno(stdout), TIOCGWINSZ, &w);
    width = (int)(w.ws_col);
    height = (int)(w.ws_row);
#endif // Windows/Linux
}


class UserDisplay {
private:
    int window_ms = 60000;
    std::uint64_t start_time;
    std::uint64_t offset;
    int chart_update_ms;
    std::shared_ptr<std::mutex> file_mutex;
    std::vector<std::string> tracked_goods;
    std::map<std::string, bool> visible;
    std::map<std::string, std::tuple<std::string, std::string>> hardcoded_legend;

    std::thread chart_thread;
public:
    std::atomic_bool destroyed = false;
    std::atomic_bool active = true;

    UserDisplay(std::uint64_t start_time, double chart_update_ms, std::shared_ptr<std::mutex> mutex, const std::vector<std::string>& tracked_goods)
            : start_time(start_time)
            , chart_update_ms(chart_update_ms)
            , file_mutex(std::move(mutex))
            , tracked_goods(tracked_goods)
            , chart_thread([this] { Tick(); }) {
        offset = to_unix_timestamp_ms(std::chrono::system_clock::now()) - start_time;
        for (auto& good : tracked_goods) {
            visible[good] = true;
        }
        hardcoded_legend = {};
        hardcoded_legend["food"] = {R"(\*)", "\x1b[1;32m*\x1b[0m"};
        hardcoded_legend["wood"] = {"#", "\x1b[1;33m#\x1b[0m"};
        hardcoded_legend["fertilizer"] = {R"(\$)", "\x1b[1;35m$\x1b[0m"};
        hardcoded_legend["ore"] = {"%", "\x1b[1;31m%\x1b[0m"};
        hardcoded_legend["metal"] = {"@", "\x1b[1;37m@\x1b[0m"};
        hardcoded_legend["tools"] = {"&", "\x1b[1;34m&\x1b[0m"};
    }

    void Shutdown() {
        destroyed = true;
        if (chart_thread.joinable()) {
            chart_thread.join();
        }
        file_mutex.reset();
    }
    void Tick() {
        display_main();
        return;
        int working_frametime_ms;
        std::chrono::duration<double, std::milli> ms_double;
        auto t1 = std::chrono::high_resolution_clock::now();
        while (!destroyed) {
            t1 = std::chrono::high_resolution_clock::now();

            if (active) {
                std::cout << DrawChart() << std::endl;
            }

            ms_double = std::chrono::high_resolution_clock::now() - t1;
            working_frametime_ms = (int) ms_double.count();
            if (working_frametime_ms < chart_update_ms) {
                std::this_thread::sleep_for(std::chrono::milliseconds{chart_update_ms - working_frametime_ms});
            } else {
                std::cout << "[ERROR][DISPLAY] User display thread overran: took " << working_frametime_ms << " ms (target: " << chart_update_ms << ")" << std::endl;
            }
        }
        std::cout << "Closing display" << std::endl;
    }

    std::string DrawChart() {
        int x = 100;
        int y = 100;
        get_terminal_size(x, y);
        y -= 7;//leave space for legend at bottom


        std::string args = "gnuplot -e \"set term dumb " + std::to_string(x)+ " " + std::to_string(y);
        args += ";set offsets 0, 0, 0, 0";
        args += ";set title 'Prices'";
        //args += ";set xrange [" + std::to_string(curr_tick - window) + ":" + std::to_string(curr_tick) + "]";
        auto local_curr_time = to_unix_timestamp_ms(std::chrono::system_clock::now());
        double time_passed_s = (double)(local_curr_time - offset - start_time) / 1000;
        args += ";set xrange ["+ std::to_string(time_passed_s - (window_ms/1000)) + ":" + std::to_string(time_passed_s) + "]";

        args += ";plot ";
        for (auto& good : tracked_goods) {
            if (visible[good]) {
                args += "'tmp/"+good+".dat' with lines title '" + good + "',";
            }
        }
        args += "\"";
        // GENERATE ASCII PLOT
        file_mutex->lock();
        auto out = GetStdoutFromCommand(args);
        file_mutex->unlock();
        // Set colors using ANSI codes
        // Could do this all in 1 pass, but O(n) + O(n) is still O(n)
        for (auto& leg : hardcoded_legend) {
            out = std::regex_replace(out, std::regex(std::get<0>(leg.second)), std::get<1>(leg.second));
        }
        return out;
    }


    static void MarketTab(struct nctab* t, struct ncplane* ncp, void* curry){
        auto display = (UserDisplay*) curry;
        ncplane_erase(ncp);
        if (display->active) {
            auto chart = display->DrawChart().c_str();
            ncplane_puttext(ncp, 1,  NCALIGN_CENTER,chart, NULL);
            ncplane_puttext(ncp, 1, NCALIGN_CENTER,
                            "User display active.",
                            NULL);
        } else {
            ncplane_puttext(ncp, 1, NCALIGN_CENTER,
                            "User display not active.",
                            NULL);
        }
        ncplane_erase(ncp);
    }

    static void PlanetTab(struct nctab* t, struct ncplane* ncp, void* curry){
        (void) t;
        ncplane_erase(ncp);
        ncplane_puttext(ncp, 1, NCALIGN_CENTER,
                        "This is placeholder text for the PLANET TAB.",
                        NULL);
    }

    int display_main() {
        //hackily wait for things to kick off
        std::this_thread::sleep_for(std::chrono::milliseconds{1000});

        if(!setlocale(LC_ALL, "")){
            fprintf(stderr, "Couldn't set locale\n");
            return EXIT_FAILURE;
        }
        notcurses_options opts{};
        struct notcurses* nc = notcurses_core_init(&opts, nullptr);
        if(nc == nullptr){
            return EXIT_FAILURE;
        }
        unsigned dimy, dimx;
        struct ncplane* n = notcurses_stdplane(nc);
        ncplane_dim_yx(n, &dimy, &dimx);

        struct ncplane_options popts = {
                .y = 3,
                .x = 5,
                .rows = dimy - 10,
                .cols = dimx - 10
        };
        struct ncplane* ncp = ncplane_create(n, &popts);
        struct nctabbed_options topts = {};
        topts.hdrchan = NCCHANNELS_INITIALIZER(255, 0, 0, 60, 60, 60);
        topts.selchan = NCCHANNELS_INITIALIZER(0, 255, 0, 0, 0, 0);
        topts.sepchan = NCCHANNELS_INITIALIZER(255, 255, 255, 100, 100, 100);
        topts.separator = " || ";
        topts.flags = false ? NCTABBED_OPTION_BOTTOM : 0;

        struct nctabbed* nct = nctabbed_create(ncp, &topts);
        nctabbed_add(nct, NULL, NULL, MarketTab, "Galactic Market", this);
        nctabbed_add(nct, NULL, NULL, PlanetTab, "Planet", this);
        nctabbed_redraw(nct);
        notcurses_render(nc);


        ncplane_puttext(n, 1, NCALIGN_CENTER,
                        "Use left/right arrow keys for navigation, "
                        "'[' and ']' to rotate tabs, "
                        "'a' to add a tab, 'r' to remove a tab, "
                        "',' and '.' to move the selected tab, "
                        "and 'q' to quit",
                        NULL);

        uint32_t key;
        ncinput ni;
        int working_frametime_ms;
        std::chrono::duration<double, std::milli> ms_double;
        auto t1 = std::chrono::high_resolution_clock::now();
        while (!destroyed) {
            t1 = std::chrono::high_resolution_clock::now();
            key = notcurses_get_blocking(nc, &ni);
            switch(key){
                case 'q':
                    destroyed = true;
                    break;
                case NCKEY_RIGHT:
                    nctabbed_next(nct);
                    break;
                case NCKEY_LEFT:
                    nctabbed_prev(nct);
                    break;
            }
            t1 = std::chrono::high_resolution_clock::now();



            ms_double = std::chrono::high_resolution_clock::now() - t1;
            working_frametime_ms = (int) ms_double.count();
//            if (working_frametime_ms < chart_update_ms) {
//                std::this_thread::sleep_for(std::chrono::milliseconds{chart_update_ms - working_frametime_ms});
//            }

            nctabbed_ensure_selected_header_visible(nct);
            nctabbed_redraw(nct);
            if(notcurses_render(nc)){
                notcurses_stop(nc);
                return EXIT_FAILURE;
            }
        }

        if(notcurses_stop(nc) < 0){
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
};

void display_plot(GlobalMetrics& metrics, int window = 0) {
    int x = 100;
    int y = 100;
    get_terminal_size(x, y);
    y -= 7;//leave space for legend at bottom
    auto raw = metrics.plot_terminal(window, x, y);
    std::cout << raw << std::endl;
}

#endif//CPPBAZAARBOT_DISPLAY_H
