#pragma once
#include <sys/types.h>
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

size_t disasm1(Operand *opr1, Operand *opr2, uint8_t *text, uint16_t addr);
