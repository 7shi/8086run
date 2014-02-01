#include "Operand.h"
#include "disasm.h"

Operand noopr = getopr(-1, false, 0, 0);
Operand dx = getopr(0, true, Reg, 2);
Operand cl = getopr(0, false, Reg, 1);
Operand es = getopr(0, true, SReg, 0);
Operand cs = getopr(0, true, SReg, 1);
Operand ss = getopr(0, true, SReg, 2);
Operand ds = getopr(0, true, SReg, 3);
