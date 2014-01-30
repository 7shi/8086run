#pragma once
#include <stdint.h>
#include <string>

extern std::string rootpath;

inline uint16_t read16(uint8_t *mem) {
    return mem[0] | (mem[1] << 8);
}

inline uint32_t read32(uint8_t *mem) {
    return mem[0] | (mem[1] << 8) | (mem[2] << 16) | (mem[3] << 24);
}

inline void write16(uint8_t *mem, uint16_t v) {
    mem[0] = v;
    mem[1] = v >> 8;
}

inline void write32(uint8_t *mem, uint32_t v) {
    mem[0] = v;
    mem[1] = v >> 8;
    mem[2] = v >> 16;
    mem[3] = v >> 24;
}

std::string readstr(uint8_t *mem, int max);

std::string hex(int v, int len = 0);
std::string hexdump(uint8_t *mem, int len);
std::string hexdump2(uint8_t *mem, int len);

bool startsWith(const std::string &s, const std::string &prefix);
bool endsWith(const std::string &s, const std::string &suffix);
std::string replace(const std::string &src, const std::string &s1, const std::string &s2);
