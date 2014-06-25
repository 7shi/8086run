// This file is in the public domain.

#include "8086run.h"
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
    inittty();

    char *appname = argv[0];
    if (argc > 1 && !strcmp(argv[1], "-hlt")) {
        hltend = true;
        --argc;
        ++argv;
    }
    if (argc < 2 || 3 < argc) {
        fprintf(stderr, "usage: %s [-hlt] fd1image [fd2image]\n", appname);
        fprintf(stderr, "  -hlt: stop on HLT (for demo)\n");
        return 1;
    }
    for (int i = 1; i < argc; ++i) {
        if (!disks[i - 1].open(argv[i])) {
            fprintf(stderr, "can not open: %s\n", argv[i]);
            return 1;
        }
    }

    for (int i = 0; i < 256; ++i) {
        int n = 0;
        for (int j = 1; j < 256; j += j) {
            if (i & j) n++;
        }
        ptable[i] = (n & 1) == 0;
    }

    uint16_t tmp = 0x1234;
    uint8_t *p = (uint8_t *) r;
    if (*(uint8_t *) & tmp == 0x34) {
        for (int i = 0; i < 4; ++i) {
            r8[i] = p + i * 2;
            r8[i + 4] = r8[i] + 1;
        }
    } else {
        for (int i = 0; i < 4; ++i) {
            r8[i] = p + i * 2 + 1;
            r8[i + 4] = r8[i] - 1;
        }
    }

    for (int i = 0; i < 0x20; ++i) {
        write16(&mem[i << 2], i);
    }

    ES = CS = SS = DS = 0;
    IP = 0x7c00;
    fread(&mem[IP], 1, 512, disks[0].f); // read MBR
    write16(&mem[0x41a], 0x1e); // keyboard queue head
    write16(&mem[0x41c], 0x1e); // keyboard queue tail
    int interval = 1 << 16, counter = interval, pitc = 0, trial = 4;
    clock_t next = clock();
    while (IP || *CS) {
        if (!--counter) {
            clock_t c = clock();
            if (c <= next && !--trial) {
                int old = interval;
                if ((interval <<= 1) < old) interval = old;
                trial = 4;
            }
            while (c > next) {
                intr(8);
                // 3600/65536 = 225/4096
                int t1 = pitc * 225000 / 4096;
                int t2 = (++pitc) * 225000 / 4096;
                if (pitc == 4096) pitc = 0;
                if (kbhit()) intr(9);
                next += CLOCKS_PER_SEC * (t2 - t1) / 1000;
                trial = 4;
            }
            counter = interval;
        }
        oldip = IP;
        step(0, NULL);
    }
}
