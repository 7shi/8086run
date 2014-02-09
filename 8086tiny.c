// 8086tiny: a tiny, highly functional, highly portable PC emulator/VM
// Copyright 2013, Adrian Cable (adrian.cable@gmail.com) - http://www.megalith.co.uk/8086tiny
//
// derived from Revision 1.03
//
// This work is licensed under the MIT License. See included LICENSE-8086TINY.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <conio.h>
#endif

// Emulator system constants

#define REGS_BASE 0xF0000

// Global variable definitions

extern struct SReg {
    unsigned char *p;
    unsigned short v;
} sr[];

#define ES sr[0]
#define CS sr[1]

extern unsigned char mem[], *r8[], IF, TF;
extern unsigned short ip, r[];

#define AX r[0]
#define BX r[3]
#define BP r[5]
#define SI r[6]

#define AL *r8[0]
#define DL *r8[2]

unsigned char trap_flag;
unsigned short inst_counter;
unsigned int int8_asap;
time_t clock_buf;
FILE *fdimg;

// Execute INT #interrupt_num on the emulated machine

extern void intr(int);

// Emulator entry point

void init_8t(char *bios, char *fd) {
    // CS is initialised to F000
    CS.v = REGS_BASE >> 4;

    // Open BIOS
    FILE *f;
    struct stat st;
    if (stat(bios, &st) || !(f = fopen(bios, "rb"))) {
        fprintf(stderr, "can not open bios: %s\n", bios);
        exit(1);
    }

    // Load BIOS image into F000:0100, and set IP to 0100
    fread(mem + REGS_BASE + (ip = 0x100), 1, st.st_size, f);
    fclose(f);

    // Open floppy disk image
    if (!(fdimg = fopen(fd, "r+b"))) {
        fprintf(stderr, "can not open fd image: %s\n", fd);
        exit(1);
    }
}

int compat_8t() {
    // Update the video graphics display when inst_counter wraps around, i.e. every 64K instructions
    if (!++inst_counter) {
        // Set flag to execute an INT 8 as soon as appropriate (see below)
        int8_asap = 1;
    }

    // Application has set trap flag, so fire INT 1
    if (trap_flag)
        intr(1);

    trap_flag = TF;

    // If an INT 8 is pending, and no segment overrides or REP prefixes are active, and IF is enabled, and TF is disabled,
    // then process the INT 8 and check for new keystrokes from the terminal

    if (int8_asap && IF && !TF) {
        // Keyboard and timer driver. This may need changing for UNIX/non-UNIX platforms
#ifdef _WIN32
        intr(8);
        if ((int8_asap = kbhit())) {
            mem[0x4A6] = getch();
            intr(7);
        }
#else
        intr(8);
        if (int8_asap = read(0, mem + 0x4A6, 1)) intr(7);
#endif
    }

    // Instruction execution loop. Terminates if CS:IP = 0:0
    return ip || CS.v;
}

// Emulator-specific 0F xx opcodes

void hypcall_8t(int n) {
    switch (n) {
        case 0: // PUTCHAR_AL
            ip += 2;
            write(1, &AL, 1);
            return;
        case 1: // GET_RTC
            ip += 2;
            time(&clock_buf);
            memcpy(ES.p + BX, localtime(&clock_buf), sizeof (struct tm));
            return;
        case 2: // READ_DISK
            if (DL == 1) {
                ip += 2;
                if (fseek(fdimg, ((SI << 4) | BP) << 9, SEEK_SET) >= 0) {
                    AL = fread(ES.p + BX, 1, AX, fdimg);
                } else {
                    AL = 0;
                }
                return;
            }
            break;
        case 3: // WRITE_DISK
            if (DL == 1) {
                ip += 2;
                if (fseek(fdimg, ((SI << 4) | BP) << 9, SEEK_SET) >= 0) {
                    AL = fwrite(ES.p + BX, 1, AX, fdimg);
                } else {
                    AL = 0;
                }
                return;
            }
            break;
    }
    fprintf(stderr, "%04x:%04x 0f%02x invalid hyper call\n", CS.v, ip, n);
    exit(1);
}
