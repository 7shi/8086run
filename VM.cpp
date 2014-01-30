#include "VM.h"
#include "disasm.h"
#include "regs.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int trace;

const char *header = " AX   BX   CX   DX   SP   BP   SI   DI  FLAGS IP\n";

bool VM::ptable[256];

void VM::showHeader() {
    fprintf(stderr, header);
}

void VM::debug(uint16_t ip, const OpCode &op) {
    fprintf(stderr,
            "%04x %04x %04x %04x %04x %04x %04x %04x %c%c%c%c %04x:%-12s %s",
            r[0], r[3], r[1], r[2], r[4], r[5], r[6], r[7],
            "-O"[OF], "-S"[SF], "-Z"[ZF], "-C"[CF],
            ip, hexdump(mem + ip, op.len).c_str(), op.str().c_str());
    if (trace >= 3) {
        int ad1 = addr(op.opr1);
        int ad2 = addr(op.opr2);
        if (ad1 >= 0) {
            if (op.opr1.w)
                fprintf(stderr, " ;[%04x]%04x", ad1, read16(ad1));
            else
                fprintf(stderr, " ;[%04x]%02x", ad1, read8(ad1));
        }
        if (ad2 >= 0) {
            if (op.opr2.w)
                fprintf(stderr, " ;[%04x]%04x", ad2, read16(ad2));
            else
                fprintf(stderr, " ;[%04x]%02x", ad2, read8(ad2));
        }
    }
    fprintf(stderr, "\n");
}

static bool initialized;

void VM::init() {
    if (!initialized) {
        for (int i = 0; i < 256; i++) {
            int n = 0;
            for (int j = 1; j < 256; j += j) {
                if (i & j) n++;
            }
            ptable[i] = (n & 1) == 0;
        }
        initialized = true;
    }
    uint16_t tmp = 0x1234;
    uint8_t *p = (uint8_t *) r;
    if (*(uint8_t *) & tmp == 0x34) {
        for (int i = 0; i < 4; i++) {
            r8[i] = p + i * 2;
            r8[i + 4] = r8[i] + 1;
        }
    } else {
        for (int i = 0; i < 4; i++) {
            r8[i] = p + i * 2 + 1;
            r8[i + 4] = r8[i] - 1;
        }
    }
}

VM::VM() : mem(NULL), hasExited(false), ip(0), start_sp(0) {
    init();
    memset(r, 0, sizeof (r));
    OF = DF = SF = ZF = PF = CF = false;
}

VM::~VM() {
    if (mem) delete[] mem;
}

int VM::addr(const Operand &opr) {
    switch (opr.type) {
        case Ptr: return uint16_t(opr.value);
        case ModRM + 0: return uint16_t(BX + SI + opr.value);
        case ModRM + 1: return uint16_t(BX + DI + opr.value);
        case ModRM + 2: return uint16_t(BP + SI + opr.value);
        case ModRM + 3: return uint16_t(BP + DI + opr.value);
        case ModRM + 4: return uint16_t(SI + opr.value);
        case ModRM + 5: return uint16_t(DI + opr.value);
        case ModRM + 6: return uint16_t(BP + opr.value);
        case ModRM + 7: return uint16_t(BX + opr.value);
    }
    return -1;
}

void VM::run2() {
    while (!hasExited) run1();
}
