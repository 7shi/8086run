#pragma once
#include "VMBase.h"
#include "OpCode.h"

extern const char *header;

struct VM : public VMBase {
    uint16_t ip, r[8];
    uint8_t * r8[8];
    bool OF, DF, SF, ZF, PF, CF;
    uint16_t start_sp;
    std::vector<OpCode> cache;

    static bool ptable[256];
    void init();

    VM();
    VM(const VM &vm);
    virtual ~VM();

    virtual bool load(const std::string &fn, FILE *f, size_t size);
    virtual void disasm();
    virtual void showHeader();
    virtual void run2();

    std::string disstr(const OpCode &op);
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
