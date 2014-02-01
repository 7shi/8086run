#pragma once
#include <string>
#include <stdint.h>

inline uint16_t read16(uint8_t *mem) {
    return mem[0] | (mem[1] << 8);
}

inline uint32_t read32(uint8_t *mem) {
    return mem[0] | (mem[1] << 8) | (mem[2] << 16) | (mem[3] << 24);
}

enum OperandType {
    Reg, SReg, Imm, Addr, Far, Ptr, ModRM
};

struct Operand {
    int len;
    bool w;
    int type, value, seg;

    inline bool empty() const {
        return len < 0;
    }
};

inline Operand getopr(int len, bool w, int type, int value, int seg = -1) {
    Operand ret = {len, w, type, value, seg};
    return ret;
}

extern Operand noopr, dx, cl, es, cs, ss, ds;

inline Operand reg(int r, bool w) {
    return getopr(0, w, Reg, r);
}

inline Operand imm16(uint16_t v) {
    return getopr(2, true, Imm, v);
}

inline Operand imm8(uint8_t v) {
    return getopr(1, false, Imm, v);
}

inline Operand far(uint32_t a) {
    return getopr(4, false, Far, (int) a);
}

inline Operand ptr(uint16_t a, bool w) {
    return getopr(2, w, Ptr, a);
}

inline Operand disp8(uint8_t *mem, uint16_t addr) {
    return getopr(1, false, Addr, (uint16_t) (addr + 1 + (int8_t) mem[0]));
}

inline Operand disp16(uint8_t *mem, uint16_t addr) {
    return getopr(2, true, Addr, (uint16_t) (addr + 2 + (int16_t) read16(mem)));
}
