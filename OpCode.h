#pragma once
#include "Operand.h"
#include <stdint.h>

namespace i8086 {

    struct OpCode {
        const char *prefix;
        size_t len;
        const char *mne;
        Operand opr1, opr2;

        inline bool empty() const {
            return len == 0;
        }

        bool undef() const;
        std::string str() const;
        void swap();
    };

    extern OpCode undefop;

    inline OpCode getop(
            size_t len, const char *mne,
            const Operand &opr1 = noopr,
            const Operand &opr2 = noopr) {
        OpCode ret = {NULL, len, mne, opr1, opr2};
        return ret;
    }

    inline OpCode getop(
            const char *mne, const Operand &opr = noopr) {
        OpCode ret = {mne, 1, mne, noopr, opr};
        return ret;
    }
}
