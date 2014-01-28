#include "Operand.h"
#include "disasm.h"

Operand noopr = getopr(-1, false, 0, 0);
Operand dx = getopr(0, true, Reg, 2);
Operand cl = getopr(0, false, Reg, 1);
Operand es = getopr(0, true, SReg, 0);
Operand cs = getopr(0, true, SReg, 1);
Operand ss = getopr(0, true, SReg, 2);
Operand ds = getopr(0, true, SReg, 3);

std::string Operand::str() const {
    switch (type) {
        case Reg: return (w ? regs16: regs8)[value];
        case SReg: return sregs[value];
        case Imm: return hex(value, len == 2 ? 4: 0);
        case Addr: return hex(value, 4);
        case Far: return hex((value >> 16) & 0xffff, 4) + ":" + hex(value & 0xffff, 4);
    }
    std::string ret = "[";
    if (seg >= 0) ret += sregs[seg] + ":";
    if (type == Ptr) return ret + hex(value, 4) + "]";
    const char *ms[] = {"bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx"};
    ret += ms[type - ModRM];
    if (value > 0) {
        ret += "+" + hex(value);
    } else if (value < 0) {
        ret += hex(value);
    }
    return ret + "]";
}
