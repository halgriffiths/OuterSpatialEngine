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
#endif // Windows/Linux

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


void display_plot(GlobalMetrics& metrics, int window = 0) {
    int x = 100;
    int y = 100;
    get_terminal_size(x, y);
    y -= 7;//leave space for legend at bottom
    auto raw = metrics.plot_terminal(window, x, y);
    std::cout << raw << std::endl;
}

#endif//CPPBAZAARBOT_DISPLAY_H
