#pragma once
#include "Operand.h"

struct OpCode {
    const char *prefix;
    size_t len;
    Operand opr1, opr2;
};

OpCode disasm1(uint8_t *text, uint16_t addr);
OpCode disasm1(uint8_t *text, uint16_t addr, size_t size);
