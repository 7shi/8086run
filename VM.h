#pragma once
#include "OpCode.h"

extern int trace;
extern const char *header;

struct VM {
    uint8_t *mem;
    bool hasExited;
    
    uint16_t ip, r[8];
    uint8_t * r8[8];
    bool OF, DF, SF, ZF, PF, CF;
    uint16_t start_sp;

    static bool ptable[256];
    void init();

    VM();
    virtual ~VM();

    inline uint8_t read8(uint16_t addr) {
        return mem[addr];
    }

    inline uint16_t read16(uint16_t addr) {
        return ::read16(mem + addr);
    }

    inline uint32_t read32(uint16_t addr) {
        return ::read32(mem + addr);
    }

    inline void write8(uint16_t addr, uint8_t value) {
        mem[addr] = value;
    }

    inline void write16(uint16_t addr, uint16_t value) {
        ::write16(mem + addr, value);
    }

    inline void write32(uint16_t addr, uint32_t value) {
        ::write32(mem + addr, value);
    }

    void showHeader();
    void run2();

    void run1(uint8_t prefix = 0);
    void debug(uint16_t ip, const OpCode &op);
    int addr(const Operand &opr);

    inline int setf8(int value, bool cf) {
        int8_t v = value;
        OF = value != v;
        SF = v < 0;
        ZF = v == 0;
        PF = ptable[uint8_t(value)];
        CF = cf;
        return value;
    }

    inline int setf16(int value, bool cf) {
        int16_t v = value;
        OF = value != v;
        SF = v < 0;
        ZF = v == 0;
        PF = ptable[uint8_t(value)];
        CF = cf;
        return value;
    }

    inline uint8_t get8(const Operand &opr) {
        switch (opr.type) {
            case Reg: return *r8[opr.value];
            case Imm: return opr.value;
        }
        int ad = addr(opr);
        return ad < 0 ? 0 : read8(ad);
    }

    inline uint16_t get16(const Operand &opr) {
        switch (opr.type) {
            case Reg: return r[opr.value];
            case Imm: return opr.value;
        }
        int ad = addr(opr);
        return ad < 0 ? 0 : read16(ad);
    }

    inline void set8(const Operand &opr, uint8_t value) {
        if (opr.type == Reg) {
            *r8[opr.value] = value;
        } else {
            int ad = addr(opr);
            if (ad >= 0) write8(ad, value);
        }
    }

    inline void set16(const Operand &opr, uint16_t value) {
        if (opr.type == Reg) {
            r[opr.value] = value;
        } else {
            int ad = addr(opr);
            if (ad >= 0) write16(ad, value);
        }
    }
};
