#include "VM.h"
#include "disasm.h"
#include "regs.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

bool VM::ptable[256];

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
