#include "VM.h"
#include "disasm.h"
#include "regs.h"
#include <stdio.h>

using namespace i8086;

void VM::run1(uint8_t prefix) {
    OpCode *op, op1;
    if (cache.empty()) {
        op = &(op1 = disasm1(text, ip, tsize));
    } else {
        if (cache[ip].len > 0) {
            op = &cache[ip];
        } else {
            op = &(cache[ip] = disasm1(text, ip, tsize));
        }
    }
    if (ip + op->len > tsize) {
        fprintf(stderr, "overrun: %04x\n", ip);
        hasExited = true;
        return;
    }
    if (SP < brksize) {
        fprintf(stderr, "stack overflow: %04x\n", SP);
        hasExited = true;
        return;
    }
    int opr1 = op->opr1.value, opr2 = op->opr2.value;
    if (trace >= 2 && !prefix) debug(ip, *op);
    uint8_t b = text[ip];
    uint16_t oldip = ip;
    int dst, src, val;
    ip += op->len;
    switch (b) {
        case 0x00: // add r/m, reg8
            val = int8_t(dst = get8(op->opr1)) + int8_t(*r8[opr2]);
            set8(op->opr1, setf8(val, dst > uint8_t(val)));
            return;
        case 0x01: // add r/m, reg16
            val = int16_t(dst = get16(op->opr1)) + int16_t(r[opr2]);
            set16(op->opr1, setf16(val, dst > uint16_t(val)));
            return;
        case 0x02: // add reg8, r/m
            val = int8_t(dst = *r8[opr1]) + int8_t(get8(op->opr2));
            *r8[opr1] = setf8(val, dst > uint8_t(val));
            return;
        case 0x03: // add reg16, r/m
            val = int16_t(dst = r[opr1]) + int16_t(get16(op->opr2));
            r[opr1] = setf16(val, dst > uint16_t(val));
            return;
        case 0x04: // add al, imm8
            val = int8_t(dst = AL) + int8_t(opr2);
            AL = setf8(val, dst > uint8_t(val));
            return;
        case 0x05: // add ax, imm16
            val = int16_t(dst = AX) + int16_t(opr2);
            AX = setf16(val, dst > uint16_t(val));
            return;
        case 0x08: // or r/m, reg8
            set8(op->opr1, setf8(int8_t(get8(op->opr1) | *r8[opr2]), false));
            return;
        case 0x09: // or r/m, reg16
            set16(op->opr1, setf16(int16_t(get16(op->opr1) | r[opr2]), false));
            return;
        case 0x0a: // or reg8, r/m
            *r8[opr1] = setf8(int8_t(*r8[opr1] | get8(op->opr2)), false);
            return;
        case 0x0b: // or reg16, r/m
            r[opr1] = setf16(int16_t(r[opr1] | get16(op->opr2)), false);
            return;
        case 0x0c: // or al, imm8
            AL = setf8(int8_t(AL | opr2), false);
            return;
        case 0x0d: // or ax, imm16
            AX = setf16(int16_t(AX | opr2), false);
            return;
        case 0x10: // adc r/m, reg8
            val = int8_t(dst = get8(op->opr1)) + int8_t(*r8[opr2]) + int(CF);
            set8(op->opr1, setf8(val, dst > uint8_t(val)));
            return;
        case 0x11: // adc r/m, reg16
            val = int16_t(dst = get16(op->opr1)) + int16_t(r[opr2]) + int(CF);
            set16(op->opr1, setf16(val, dst > uint16_t(val)));
            return;
        case 0x12: // adc reg8, r/m
            val = int8_t(dst = *r8[opr1]) + int8_t(get8(op->opr2)) + int(CF);
            *r8[opr1] = setf8(val, dst > uint8_t(val));
            return;
        case 0x13: // adc reg16, r/m
            val = int16_t(dst = r[opr1]) + int16_t(get16(op->opr2)) + int(CF);
            r[opr1] = setf16(val, dst > uint16_t(val));
            return;
        case 0x14: // adc al, imm8
            val = int8_t(dst = AL) + int8_t(opr2) + int(CF);
            AL = setf8(val, dst > uint8_t(val));
            return;
        case 0x15: // adc ax, imm16
            val = int16_t(dst = AX) + int16_t(opr2) + int(CF);
            AX = setf16(val, dst > uint16_t(val));
            return;
        case 0x18: // sbb r/m, reg8
            val = int8_t(dst = get8(op->opr1)) - int8_t(src = *r8[opr2] + int(CF));
            set8(op->opr1, setf8(val, dst < src));
            return;
        case 0x19: // sbb r/m, reg16
            val = int16_t(dst = get16(op->opr1)) - int16_t(src = r[opr2] + int(CF));
            set16(op->opr1, setf16(val, dst < src));
            return;
        case 0x1a: // sbb reg8, r/m
            val = int8_t(dst = *r8[opr1]) - int8_t(src = get8(op->opr2) + int(CF));
            *r8[opr1] = setf8(val, dst < src);
            return;
        case 0x1b: // sbb reg16, r/m
            val = int16_t(dst = r[opr1]) - int16_t(src = get16(op->opr2) + int(CF));
            r[opr1] = setf16(val, dst < src);
            return;
        case 0x1c: // sbb al, imm8
            val = int8_t(dst = AL) - int8_t(src = opr2 + int(CF));
            AL = setf8(val, dst < src);
            return;
        case 0x1d: // sbb ax, imm16
            val = int16_t(dst = AX) - int16_t(src = opr2 + int(CF));
            AX = setf16(val, dst < src);
            return;
        case 0x20: // and r/m, reg8
            set8(op->opr1, setf8(int8_t(get8(op->opr1) & *r8[opr2]), false));
            return;
        case 0x21: // and r/m, reg16
            set16(op->opr1, setf16(int16_t(get16(op->opr1) & r[opr2]), false));
            return;
        case 0x22: // and reg8, r/m
            *r8[opr1] = setf8(int8_t(*r8[opr1] & get8(op->opr2)), false);
            return;
        case 0x23: // and reg16, r/m
            r[opr1] = setf16(int16_t(r[opr1] & get16(op->opr2)), false);
            return;
        case 0x24: // and al, imm8
            AL = setf8(int8_t(AL & opr2), false);
            return;
        case 0x25: // and ax, imm16
            AX = setf16(int16_t(AX & opr2), false);
            return;
        case 0x28: // sub r/m, reg8
            val = int8_t(dst = get8(op->opr1)) - int8_t(src = *r8[opr2]);
            set8(op->opr1, setf8(val, dst < src));
            return;
        case 0x29: // sub r/m, reg16
            val = int16_t(dst = get16(op->opr1)) - int16_t(src = r[opr2]);
            set16(op->opr1, setf16(val, dst < src));
            return;
        case 0x2a: // sub reg8, r/m
            val = int8_t(dst = *r8[opr1]) - int8_t(src = get8(op->opr2));
            *r8[opr1] = setf8(val, dst < src);
            return;
        case 0x2b: // sub reg16, r/m
            val = int16_t(dst = r[opr1]) - int16_t(src = get16(op->opr2));
            r[opr1] = setf16(val, dst < src);
            return;
        case 0x2c: // sub al, imm8
            val = int8_t(dst = AL) - int8_t(src = opr2);
            AL = setf8(val, dst < src);
            return;
        case 0x2d: // sub ax, imm16
            val = int16_t(dst = AX) - int16_t(src = opr2);
            AX = setf16(val, dst < src);
            return;
        case 0x30: // xor r/m, reg8
            set8(op->opr1, setf8(int8_t(get8(op->opr1) ^ *r8[opr2]), false));
            return;
        case 0x31: // xor r/m, reg16
            set16(op->opr1, setf16(int16_t(get16(op->opr1) ^ r[opr2]), false));
            return;
        case 0x32: // xor reg8, r/m
            *r8[opr1] = setf8(int8_t(*r8[opr1] ^ get8(op->opr2)), false);
            return;
        case 0x33: // xor reg16, r/m
            r[opr1] = setf16(int16_t(r[opr1] ^ get16(op->opr2)), false);
            return;
        case 0x34: // xor al, imm8
            AL = setf8(int8_t(AL ^ opr2), false);
            return;
        case 0x35: // xor ax, imm16
            AX = setf16(int16_t(AX ^ opr2), false);
            return;
        case 0x38: // cmp r/m, reg8
            val = int8_t(dst = get8(op->opr1)) - int8_t(src = *r8[opr2]);
            setf8(val, dst < src);
            return;
        case 0x39: // cmp r/m, reg16
            val = int16_t(dst = get16(op->opr1)) - int16_t(src = r[opr2]);
            setf16(val, dst < src);
            return;
        case 0x3a: // cmp reg8, r/m
            val = int8_t(dst = *r8[opr1]) - int8_t(src = get8(op->opr2));
            setf8(val, dst < src);
            return;
        case 0x3b: // cmp reg16, r/m
            val = int16_t(dst = r[opr1]) - int16_t(src = get16(op->opr2));
            setf16(val, dst < src);
            return;
        case 0x3c: // cmp al, imm8
            val = int8_t(dst = AL) - int8_t(src = opr2);
            setf8(val, dst < src);
            return;
        case 0x3d: // cmp ax, imm16
            val = int16_t(dst = AX) - int16_t(src = opr2);
            setf16(val, dst < src);
            return;
        case 0x40: // inc reg16
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
            r[opr1] = setf16(int16_t(r[opr1]) + 1, CF);
            return;
        case 0x48: // dec reg16
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f:
            r[opr1] = setf16(int16_t(r[opr1]) - 1, CF);
            return;
        case 0x50: // push reg16
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
            val = r[opr1];
            SP -= 2;
            write16(SP, val);
            return;
        case 0x58: // pop reg16
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5e:
        case 0x5f:
            r[opr1] = read16(SP);
            SP += 2;
            return;
        case 0x70: // jo
            if (OF) ip = opr1;
            return;
        case 0x71: // jno
            if (!OF) ip = opr1;
            return;
        case 0x72: // jb/jnae
            if (CF) ip = opr1;
            return;
        case 0x73: // jnb/jae
            if (!CF) ip = opr1;
            return;
        case 0x74: // je/jz
            if (ZF) ip = opr1;
            return;
        case 0x75: // jne/jnz
            if (!ZF) ip = opr1;
            return;
        case 0x76: // jbe/jna
            if (CF || ZF) ip = opr1;
            return;
        case 0x77: // jnbe/ja
            if (!CF && !ZF) ip = opr1;
            return;
        case 0x78: // js
            if (SF) ip = opr1;
            return;
        case 0x79: // jns
            if (!SF) ip = opr1;
            return;
        case 0x7a: // jp
            if (PF) ip = opr1;
            return;
        case 0x7b: // jnp
            if (!PF) ip = opr1;
            return;
        case 0x7c: // jl/jnge
            if (SF != OF) ip = opr1;
            return;
        case 0x7d: // jnl/jge
            if (SF == OF) ip = opr1;
            return;
        case 0x7e: // jle/jng
            if (ZF || SF != OF) ip = opr1;
            return;
        case 0x7f: // jnle/jg
            if (!ZF && SF == OF) ip = opr1;
            return;
        case 0x80: // r/m, imm8
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // add
                    val = int8_t(dst = get8(op->opr1)) + int8_t(opr2);
                    set8(op->opr1, setf8(val, dst > uint8_t(val)));
                    return;
                case 1: // or
                    set8(op->opr1, setf8(int8_t(get8(op->opr1) | opr2), false));
                    return;
                case 2: // adc
                    val = int8_t(dst = get8(op->opr1)) + int8_t(opr2) + int(CF);
                    set8(op->opr1, setf8(val, dst > uint8_t(val)));
                    return;
                case 3: // sbb
                    val = int8_t(dst = get8(op->opr1)) - int8_t(src = opr2 + int(CF));
                    set8(op->opr1, setf8(val, dst < src));
                    return;
                case 4: // and
                    set8(op->opr1, setf8(int8_t(get8(op->opr1) & opr2), false));
                    return;
                case 5: // sub
                    val = int8_t(dst = get8(op->opr1)) - int8_t(src = opr2);
                    set8(op->opr1, setf8(val, dst < src));
                    return;
                case 6: // xor
                    set8(op->opr1, setf8(int8_t(get8(op->opr1) ^ opr2), false));
                    return;
                case 7: // cmp
                    val = int8_t(dst = get8(op->opr1)) - int8_t(src = opr2);
                    setf8(val, dst < src);
                    return;
            }
            break;
        case 0x81: // r/m, imm16
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // add
                    val = int16_t(dst = get16(op->opr1)) + int16_t(opr2);
                    set16(op->opr1, setf16(val, dst > uint16_t(val)));
                    return;
                case 1: // or
                    set16(op->opr1, setf16(int16_t(get16(op->opr1) | opr2), false));
                    return;
                case 2: // adc
                    val = int16_t(dst = get16(op->opr1)) + int16_t(opr2) + int(CF);
                    set16(op->opr1, setf16(val, dst > uint16_t(val)));
                    return;
                case 3: // sbb
                    val = int16_t(dst = get16(op->opr1)) - int16_t(src = opr2 + int(CF));
                    set16(op->opr1, setf16(val, dst < src));
                    return;
                case 4: // and
                    set16(op->opr1, setf16(int16_t(get16(op->opr1) & opr2), false));
                    return;
                case 5: // sub
                    val = int16_t(dst = get16(op->opr1)) - int16_t(src = opr2);
                    set16(op->opr1, setf16(val, dst < src));
                    return;
                case 6: // xor
                    set16(op->opr1, setf16(int16_t(get16(op->opr1) ^ opr2), false));
                    return;
                case 7: // cmp
                    val = int16_t(dst = get16(op->opr1)) - int16_t(src = opr2);
                    setf16(val, dst < src);
                    return;
            }
            break;
        case 0x82: // r/m, imm8(signed)
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // add
                    val = int8_t(dst = get8(op->opr1)) + int8_t(opr2);
                    set8(op->opr1, setf8(val, dst > uint8_t(val)));
                    return;
                case 2: // adc
                    val = int8_t(dst = get8(op->opr1)) + int8_t(opr2) + int(CF);
                    set8(op->opr1, setf8(val, dst > uint8_t(val)));
                    return;
                case 3: // sbb
                    val = int8_t(dst = get8(op->opr1)) - int8_t(src = opr2 + int(CF));
                    set8(op->opr1, setf8(val, dst < uint8_t(src)));
                    return;
                case 5: // sub
                    val = int8_t(dst = get8(op->opr1)) - int8_t(src = opr2);
                    set8(op->opr1, setf8(val, dst < uint8_t(src)));
                    return;
                case 7: // cmp
                    val = int8_t(dst = get8(op->opr1)) - int8_t(src = opr2);
                    setf8(val, dst < uint8_t(src));
                    return;
            }
            break;
        case 0x83: // r/m, imm16(signed)
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // add
                    val = int16_t(dst = get16(op->opr1)) + int8_t(opr2);
                    set16(op->opr1, setf16(val, dst > uint16_t(val)));
                    return;
                case 2: // adc
                    val = int16_t(dst = get16(op->opr1)) + int8_t(opr2) + int(CF);
                    set16(op->opr1, setf16(val, dst > uint16_t(val)));
                    return;
                case 3: // sbb
                    val = int16_t(dst = get16(op->opr1)) - int8_t(src = opr2 + int(CF));
                    set16(op->opr1, setf16(val, dst < uint16_t(src)));
                    return;
                case 5: // sub
                    val = int16_t(dst = get16(op->opr1)) - int8_t(src = opr2);
                    set16(op->opr1, setf16(val, dst < uint16_t(src)));
                    return;
                case 7: // cmp
                    val = int16_t(dst = get16(op->opr1)) - int8_t(src = opr2);
                    setf16(val, dst < uint16_t(src));
                    return;
            }
            break;
        case 0x84: // test r/m, reg8
            setf8(int8_t(get8(op->opr1) & *r8[opr2]), false);
            return;
        case 0x85: // test r/m, reg16
            setf16(int16_t(get16(op->opr1) & r[opr2]), false);
            return;
        case 0x86: // xchg r/m, reg8
            val = *r8[opr2];
            *r8[opr2] = get8(op->opr1);
            set8(op->opr1, val);
            return;
        case 0x87: // xchg r/m, reg16
            val = r[opr2];
            r[opr2] = get16(op->opr1);
            set16(op->opr1, val);
            return;
        case 0x88: // mov r/m, reg8
            set8(op->opr1, *r8[opr2]);
            return;
        case 0x89: // mov r/m, reg16
            set16(op->opr1, r[opr2]);
            return;
        case 0x8a: // mov reg8, r/m
            *r8[opr1] = get8(op->opr2);
            return;
        case 0x8b: // mov reg16, r/m
            r[opr1] = get16(op->opr2);
            return;
        case 0x8d: // lea reg16, r/m
            r[opr1] = addr(op->opr2);
            return;
        case 0x8f: // pop r/m
            set16(op->opr1, read16(SP));
            SP += 2;
            return;
        case 0x90: // nop
            return;
        case 0x91: // xchg reg, ax
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x96:
        case 0x97:
            val = AX;
            AX = r[opr1];
            r[opr1] = val;
            return;
        case 0x98: // cbw
            AX = (int16_t) (int8_t) AL;
            return;
        case 0x99: // cwd
            DX = int16_t(AX) < 0 ? 0xffff : 0;
            return;
        case 0x9c: // pushf
            SP -= 2;
            write16(SP, (OF << 11) | (DF << 10) | (SF << 7) | (ZF << 6) | (PF << 2) | 2 | CF);
            return;
        case 0x9d: // popf
            val = read16(SP);
            SP += 2;
            OF = val & 2048;
            DF = val & 1024;
            SF = val & 128;
            ZF = val & 64;
            PF = val & 4;
            CF = val & 1;
            return;
        case 0x9e: // sahf
            SF = AH & 128;
            ZF = AH & 64;
            PF = AH & 4;
            CF = AH & 1;
            return;
        case 0x9f: // lahf
            AH = (SF << 7) | (ZF << 6) | (PF << 2) | 2 | CF;
            return;
        case 0xa0: // mov al, [addr]
            AL = get8(op->opr2);
            return;
        case 0xa1: // mov ax, [addr]
            AX = get16(op->opr2);
            return;
        case 0xa2: // mov [addr], al
            set8(op->opr1, AL);
            return;
        case 0xa3: // mov [addr], ax
            set16(op->opr1, AX);
            return;
        case 0xa4: // movsb
            do {
                write8(DI, read8(SI));
                if (DF) {
                    SI--;
                    DI--;
                } else {
                    SI++;
                    DI++;
                }
                if (prefix) CX--;
            } while ((prefix == 0xf2 || prefix == 0xf3) && CX);
            return;
        case 0xa5: // movsw
            do {
                write16(DI, read16(SI));
                if (DF) {
                    SI -= 2;
                    DI -= 2;
                } else {
                    SI += 2;
                    DI += 2;
                }
                if (prefix) CX--;
            } while ((prefix == 0xf2 || prefix == 0xf3) && CX);
            return;
        case 0xa6: // cmpsb
            do {
                val = int8_t(dst = read8(SI)) - int8_t(src = read8(DI));
                setf8(val, dst < src);
                if (DF) {
                    SI--;
                    DI--;
                } else {
                    SI++;
                    DI++;
                }
                if (prefix) CX--;
            } while (((prefix == 0xf2 && !ZF) || (prefix == 0xf3 && ZF)) && CX);
            return;
        case 0xa7: // cmpsw
            do {
                val = int16_t(dst = read16(SI)) - int16_t(src = read16(DI));
                setf16(val, dst < src);
                if (DF) {
                    SI -= 2;
                    DI -= 2;
                } else {
                    SI += 2;
                    DI += 2;
                }
                if (prefix) CX--;
            } while (((prefix == 0xf2 && !ZF) || (prefix == 0xf3 && ZF)) && CX);
            return;
        case 0xa8: // test al, imm8
            setf8(int8_t(AL & opr2), false);
            return;
        case 0xa9: // test ax, imm16
            setf16(int16_t(AX & opr2), false);
            return;
        case 0xaa: // stosb
            do {
                write8(DI, AL);
                if (DF) DI--;
                else DI++;
                if (prefix) CX--;
            } while ((prefix == 0xf2 || prefix == 0xf3) && CX);
            return;
        case 0xab: // stosw
            do {
                write16(DI, AX);
                if (DF) DI -= 2;
                else DI += 2;
                if (prefix) CX--;
            } while ((prefix == 0xf2 || prefix == 0xf3) && CX);
            return;
        case 0xac: // lodsb
            do {
                AL = read8(SI);
                if (DF) SI--;
                else SI++;
                if (prefix) CX--;
            } while ((prefix == 0xf2 || prefix == 0xf3) && CX);
            return;
        case 0xad: // lodsw
            do {
                AX = read16(SI);
                if (DF) SI -= 2;
                else SI += 2;
                if (prefix) CX--;
            } while ((prefix == 0xf2 || prefix == 0xf3) && CX);
            return;
        case 0xae: // scasb
            do {
                val = int8_t(AL) - int8_t(src = read8(DI));
                setf8(val, AL < src);
                if (DF) DI--;
                else DI++;
                if (prefix) CX--;
            } while (((prefix == 0xf2 && !ZF) || (prefix == 0xf3 && ZF)) && CX);
            return;
        case 0xaf: // scasw
            do {
                val = int16_t(AX) - int16_t(src = read16(DI));
                setf16(val, AX < src);
                if (DF) DI -= 2;
                else DI += 2;
                if (prefix) CX--;
            } while (((prefix == 0xf2 && !ZF) || (prefix == 0xf3 && ZF)) && CX);
            return;
        case 0xb0: // mov reg8, imm8
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xb7:
            *r8[opr1] = opr2;
            return;
        case 0xb8: // mov reg16, imm16
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xbe:
        case 0xbf:
            r[opr1] = opr2;
            return;
        case 0xc2: // ret imm16
            ip = read16(SP);
            SP += 2 + opr1;
            return;
        case 0xc3: // ret
            if (SP == start_sp) {
                hasExited = true;
                return;
            }
            ip = read16(SP);
            SP += 2;
            return;
        case 0xc6: // mov r/m, imm8
            set8(op->opr1, opr2);
            return;
        case 0xc7: // mov r/m, imm16
            set16(op->opr1, opr2);
            return;
        case 0xcd: // int imm8
            break;
        case 0xd0: // byte r/m, 1
            src = get8(op->opr1);
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // rol
                    CF = src & 0x80;
                    set8(op->opr1, (src << 1) | CF);
                    OF = CF ^ bool(src & 0x40);
                    return;
                case 1: // ror
                    CF = src & 1;
                    set8(op->opr1, (src >> 1) | (CF ? 0x80 : 0));
                    OF = CF ^ bool(src & 0x80);
                    return;
                case 2: // rcl
                    set8(op->opr1, (src << 1) | CF);
                    CF = src & 0x80;
                    OF = CF ^ bool(src & 0x40);
                    return;
                case 3: // rcr
                    set8(op->opr1, (src >> 1) | (CF ? 0x80 : 0));
                    OF = CF ^ bool(src & 0x80);
                    CF = src & 1;
                    return;
                case 4: // shl/sal
                    set8(op->opr1, setf8(int8_t(src) << 1, src & 0x80));
                    return;
                case 5: // shr
                    set8(op->opr1, setf8(int8_t(src >> 1), src & 1));
                    OF = src & 0x80;
                    return;
                case 7: // sar
                    set8(op->opr1, setf8(int8_t(src) >> 1, src & 1));
                    OF = false;
                    return;
            }
            break;
        case 0xd1: // r/m, 1
            src = get16(op->opr1);
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // rol
                    CF = src & 0x8000;
                    set16(op->opr1, (src << 1) | CF);
                    OF = CF ^ bool(src & 0x4000);
                    return;
                case 1: // ror
                    CF = src & 1;
                    set16(op->opr1, (src >> 1) | (CF ? 0x8000 : 0));
                    OF = CF ^ bool(src & 0x8000);
                    return;
                case 2: // rcl
                    set16(op->opr1, (src << 1) | CF);
                    CF = src & 0x8000;
                    OF = CF ^ bool(src & 0x4000);
                    return;
                case 3: // rcr
                    set16(op->opr1, (src >> 1) | (CF ? 0x8000 : 0));
                    OF = CF ^ bool(src & 0x8000);
                    CF = src & 1;
                    return;
                case 4: // shl/sal
                    set16(op->opr1, setf16(int16_t(src) << 1, src & 0x8000));
                    return;
                case 5: // shr
                    set16(op->opr1, setf16(int16_t(src >> 1), src & 1));
                    OF = src & 0x8000;
                    return;
                case 7: // sar
                    set16(op->opr1, setf16(int16_t(src) >> 1, src & 1));
                    OF = false;
                    return;
            }
            break;
        case 0xd2: // byte r/m, cl
            val = get8(op->opr1);
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // rol
                    for (int i = 0; i < CL; i++)
                        val = (val << 1) | (CF = val & 0x80);
                    set8(op->opr1, val);
                    return;
                case 1: // ror
                    for (int i = 0; i < CL; i++)
                        val = (val >> 1) | ((CF = val & 1) ? 0x80 : 0);
                    set8(op->opr1, val);
                    return;
                case 2: // rcl
                    for (int i = 0; i < CL; i++) {
                        val = (val << 1) | CF;
                        CF = val & 0x100;
                    }
                    set8(op->opr1, val);
                    return;
                case 3: // rcr
                    for (int i = 0; i < CL; i++) {
                        bool f = val & 1;
                        val = (val >> 1) | (CF ? 0x80 : 0);
                        CF = f;
                    }
                    set8(op->opr1, val);
                    return;
                case 4: // shl/sal
                    if (CL > 0) {
                        val <<= CL;
                        set8(op->opr1, setf8(int8_t(val), val & 0x100));
                    }
                    return;
                case 5: // shr
                    if (CL > 0) {
                        val >>= CL - 1;
                        set8(op->opr1, setf8(int8_t(val >> 1), val & 1));
                    }
                    return;
                case 7: // sar
                    if (CL > 0) {
                        val = int8_t(val) >> (CL - 1);
                        set8(op->opr1, setf8(val >> 1, val & 1));
                    }
                    return;
            }
            break;
        case 0xd3: // r/m, cl
            val = get16(op->opr1);
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // rol
                    for (int i = 0; i < CL; i++) {
                        CF = val & 0x8000;
                        set16(op->opr1, val = (val << 1) | CF);
                    }
                    return;
                case 1: // ror
                    for (int i = 0; i < CL; i++) {
                        CF = val & 1;
                        set16(op->opr1, val = (val >> 1) | (CF ? 0x8000 : 0));
                    }
                    return;
                case 2: // rcl
                    for (int i = 0; i < CL; i++) {
                        set16(op->opr1, val = (val << 1) | CF);
                        CF = val & 0x8000;
                    }
                    return;
                case 3: // rcr
                    for (int i = 0; i < CL; i++) {
                        set16(op->opr1, val = (val >> 1) | (CF ? 0x8000 : 0));
                        CF = val & 1;
                    }
                    return;
                case 4: // shl/sal
                    if (CL > 0) {
                        val <<= CL;
                        set16(op->opr1, setf16(int16_t(val), val & 0x10000));
                    }
                    return;
                case 5: // shr
                    if (CL > 0) {
                        val >>= CL - 1;
                        set16(op->opr1, setf16(int16_t(val >> 1), val & 1));
                    }
                    return;
                case 7: // sar
                    if (CL > 0) {
                        val = int16_t(val) >> (CL - 1);
                        set16(op->opr1, setf16(val >> 1, val & 1));
                    }
                    return;
            }
            break;
        case 0xd7: // xlat
            AL = read8(BX + AL);
            return;
        case 0xe0: // loopnz/loopne
            if (--CX > 0 && !ZF) ip = opr1;
            return;
        case 0xe1: // loopz/loope
            if (--CX > 0 && ZF) ip = opr1;
            return;
        case 0xe2: // loop
            if (--CX > 0) ip = opr1;
            return;
        case 0xe3: // jcxz
            if (CX == 0) ip = opr1;
            return;
        case 0xe8: // call disp
            SP -= 2;
            write16(SP, ip);
            ip = opr1;
            return;
        case 0xe9: // jmp disp
            ip = opr1;
            return;
        case 0xeb: // jmp short
            ip = opr1;
            return;
        case 0xf2: // repnz/repne
        case 0xf3: // rep/repz/repe
            if (CX) {
                ip = oldip + 1;
                run1(b);
            }
            return;
        case 0xf5: // cmc
            CF = !CF;
            return;
        case 0xf6:
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // test r/m, imm8
                    setf8(int8_t(get8(op->opr1) & opr2), false);
                    return;
                case 2: // not byte r/m
                    set8(op->opr1, ~get8(op->opr1));
                    return;
                case 3: // neg byte r/m
                    src = get8(op->opr1);
                    set8(op->opr1, setf8(-int8_t(src), src));
                    return;
                case 4: // mul byte r/m
                    AX = AL * get8(op->opr1);
                    OF = CF = AH;
                    return;
                case 5: // imul byte r/m
                    AX = int8_t(AL) * int8_t(get8(op->opr1));
                    OF = CF = AH;
                    return;
                case 6: // div byte r/m
                    dst = AX;
                    src = get8(op->opr1);
                    AL = dst / src;
                    AH = dst % src;
                    return;
                case 7:
                { // idiv byte r/m
                    val = int16_t(AX);
                    int16_t y = int8_t(get8(op->opr1));
                    AL = val / y;
                    AH = val % y;
                    return;
                }
            }
            break;
        case 0xf7:
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // test r/m, imm16
                    setf16(int16_t(get16(op->opr1) & opr2), false);
                    return;
                case 2: // not r/m
                    set16(op->opr1, ~get16(op->opr1));
                    return;
                case 3: // neg r/m
                    src = get16(op->opr1);
                    set16(op->opr1, setf16(-int16_t(src), src));
                    return;
                case 4:
                { // mul r/m
                    uint32_t v = AX * get16(op->opr1);
                    DX = v >> 16;
                    AX = v;
                    OF = CF = DX;
                    return;
                }
                case 5: // imul r/m
                    val = int16_t(AX) * int16_t(get16(op->opr1));
                    DX = val >> 16;
                    AX = val;
                    OF = CF = DX;
                    return;
                case 6:
                { // div r/m
                    uint32_t x = (DX << 16) | AX;
                    src = get16(op->opr1);
                    AX = x / src;
                    DX = x % src;
                    return;
                }
                case 7:
                { // idiv r/m
                    int32_t x = (DX << 16) | AX;
                    int32_t y = int16_t(get16(op->opr1));
                    AX = x / y;
                    DX = x % y;
                    return;
                }
            }
            break;
        case 0xf8: // clc
            CF = false;
            return;
        case 0xf9: // stc
            CF = true;
            return;
        case 0xfc: // cld
            DF = false;
            return;
        case 0xfd: // std
            DF = true;
            return;
        case 0xfe: // byte r/m
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // inc
                    set8(op->opr1, setf8(int8_t(get8(op->opr1)) + 1, CF));
                    return;
                case 1: // dec
                    set8(op->opr1, setf8(int8_t(get8(op->opr1)) - 1, CF));
                    return;
            }
            break;
        case 0xff: // r/m
            switch ((text[oldip + 1] >> 3) & 7) {
                case 0: // inc
                    set16(op->opr1, setf16(int16_t(get16(op->opr1)) + 1, CF));
                    return;
                case 1: // dec
                    set16(op->opr1, setf16(int16_t(get16(op->opr1)) - 1, CF));
                    return;
                case 2: // call
                    SP -= 2;
                    write16(SP, ip);
                    ip = get16(op->opr1);
                    return;
                case 3: // callf
                    break;
                case 4: // jmp
                    ip = get16(op->opr1);
                    return;
                case 5: // jmpf
                    break;
                case 6: // push
                    SP -= 2;
                    write16(SP, get16(op->opr1));
                    return;
            }
            break;
    }
    if (trace < 2 && !prefix) {
        fprintf(stderr, header);
        debug(oldip, *op);
    }
    fprintf(stderr, "not implemented\n");
}
