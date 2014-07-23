// This file is in the public domain.

#ifdef _WIN32
#  include <windows.h>
#else
#  include <time.h>
#endif

void msleep(int msec) {
    if (msec < 0) return;
#ifdef _WIN32
    Sleep(msec);
#else
    struct timespec ts = { 0, msec * 1000000 };
    nanosleep(&ts, NULL);
#endif
}
