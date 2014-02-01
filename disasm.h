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

struct OpCode {
    const char *prefix;
    size_t len;
    Operand opr1, opr2;
};

OpCode disasm1(uint8_t *text, uint16_t addr);
OpCode disasm1(uint8_t *text, uint16_t addr, size_t size);
