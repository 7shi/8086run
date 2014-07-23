// This file is in the public domain.

#ifdef _WIN32
#  include <windows.h>
#else
#  include <time.h>
#endif
#include <stdio.h>

void clocksleep(int clk) {
    if (clk < 0) return;
    int msec = clk * 1000 / CLOCKS_PER_SEC;
#ifdef _WIN32
    Sleep(msec);
#else
    struct timespec ts = { msec / 1000, (msec % 1000) * 1000000 };
    nanosleep(&ts, NULL);
#endif
}
