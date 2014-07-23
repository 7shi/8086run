// This file is in the public domain.

#ifdef _WIN32
#include <windows.h>
#endif
#include <time.h>
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

int mclock() {
#ifdef _WIN32
    clock_t clk = clock();
    static clock_t clk0 = clk;
    clk -= clk0;
    return clk * 1000 / CLOCKS_PER_SEC;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    static time_t sec0 = ts.tv_sec;
    return (ts.tv_sec - sec0) * 1000 + ts.tv_nsec / 1000000;
#endif
}
