#pragma once
#include "utils.h"
#include <stdio.h>
#include <vector>
#include <list>
#include <map>
#ifdef WIN32
#define NO_FORK
#endif
#ifdef unix
#undef unix
#endif

extern int trace;

struct Symbol {
    std::string name;
    int type, addr;
};

struct VMBase {
    uint8_t *text, *data;
    size_t tsize, dsize;
    uint16_t brksize;
    bool hasExited;
    std::map<int, Symbol> syms[2];

    VMBase();
    VMBase(const VMBase &vm);
    virtual ~VMBase();

    void release();
    void showsym(uint16_t addr);
    void debugsym(uint16_t pc);

    virtual bool load(const std::string &fn, FILE *f, size_t size);
    virtual void disasm() = 0;
    virtual void showHeader() = 0;
    virtual void run2() = 0;

    inline uint8_t read8(uint16_t addr) {
        return data[addr];
    }

    inline uint16_t read16(uint16_t addr) {
        return ::read16(data + addr);
    }

    inline uint32_t read32(uint16_t addr) {
        return ::read32(data + addr);
    }

    inline void write8(uint16_t addr, uint8_t value) {
        data[addr] = value;
    }

    inline void write16(uint16_t addr, uint16_t value) {
        ::write16(data + addr, value);
    }

    inline void write32(uint16_t addr, uint32_t value) {
        ::write32(data + addr, value);
    }

    inline const char *str(uint16_t addr) {
        return (const char *) (data + addr);
    }

    inline const char *str16(uint8_t *addr) {
        return (const char *) (data + ::read16(addr));
    }
};
