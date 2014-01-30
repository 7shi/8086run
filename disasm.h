#pragma once
#include "OpCode.h"

extern std::string regs8[];
extern std::string regs16[];
extern std::string sregs[];

OpCode disasm1(uint8_t *text, uint16_t addr);
OpCode disasm1(uint8_t *text, uint16_t addr, size_t size);
