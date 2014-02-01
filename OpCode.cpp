#include "OpCode.h"

OpCode undefop = {NULL, 1, "(undefined)", noopr, noopr};

void OpCode::swap() {
    Operand tmp = opr1;
    opr1 = opr2;
    opr2 = tmp;
}

bool OpCode::undef() const {
    return mne == undefop.mne;
}
