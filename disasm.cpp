#include "disasm.h"
#include <stdio.h>

// Operand

inline Operand getopr(int len, bool w, int type, int value, int seg = -1) {
    Operand ret = {len, w, type, value, seg};
    return ret;
}

static Operand noopr = getopr(-1, false, 0, 0);
static Operand dx = getopr(0, true, Reg, 2);
static Operand cl = getopr(0, false, Reg, 1);
static Operand es = getopr(0, true, SReg, 0);
static Operand cs = getopr(0, true, SReg, 1);
static Operand ss = getopr(0, true, SReg, 2);
static Operand ds = getopr(0, true, SReg, 3);

inline Operand reg(int r, bool w) {
    return getopr(0, w, Reg, r);
}

inline Operand imm16(uint16_t v) {
    return getopr(2, true, Imm, v);
}

inline Operand imm8(uint8_t v) {
    return getopr(1, false, Imm, v);
}

inline Operand far(uint32_t a) {
    return getopr(4, false, Far, (int) a);
}

inline Operand ptr(uint16_t a, bool w) {
    return getopr(2, w, Ptr, a);
}

inline Operand disp8(uint8_t *mem, uint16_t addr) {
    return getopr(1, false, Addr, (uint16_t) (addr + 1 + (int8_t) mem[0]));
}

inline Operand disp16(uint8_t *mem, uint16_t addr) {
    return getopr(2, true, Addr, (uint16_t) (addr + 2 + (int16_t) read16(mem)));
}

// OpCode

struct OpCode {
    const char *prefix;
    size_t len;
    Operand opr1, opr2;
};

OpCode undefop = {NULL, 1, noopr, noopr};

static inline void swap(OpCode *op) {
    Operand tmp = op->opr1;
    op->opr1 = op->opr2;
    op->opr2 = tmp;
}

static inline OpCode getop(
        size_t len, const char *mne,
        const Operand &opr1 = noopr,
        const Operand &opr2 = noopr) {
    OpCode ret = {NULL, len, opr1, opr2};
    return ret;
}

static inline OpCode getop(
        const char *mne, const Operand &opr = noopr) {
    OpCode ret = {mne, 1, noopr, opr};
    return ret;
}

// disasm

static inline Operand modrm(uint8_t *mem, bool w) {
    uint8_t b = mem[1], mod = b >> 6, rm = b & 7;
    switch (mod) {
        case 0:
            if (rm == 6) return getopr(3, w, Ptr, read16(mem + 2));
            return getopr(1, w, ModRM + rm, 0);
        case 1:
            return getopr(2, w, ModRM + rm, (int8_t) mem[2]);
        case 2:
            return getopr(3, w, ModRM + rm, (int16_t) read16(mem + 2));
        default:
            return getopr(1, w, Reg, rm);
    }
}

static inline OpCode modrm(uint8_t *mem, const char *mne, bool w) {
    Operand opr = modrm(mem, w);
    return getop(1 + opr.len, mne, opr);
}

static inline OpCode regrm(uint8_t *mem, const char *mne, bool d, int w) {
    OpCode op = modrm(mem, mne, w);
    if (w == 2) {
        op.opr2 = getopr(0, w, SReg, (mem[1] >> 3) & 3);
    } else {
        op.opr2 = getopr(0, w, Reg, (mem[1] >> 3) & 7);
    }
    if (d) swap(&op);
    return op;
}

static inline OpCode aimm(uint8_t *mem, const char *mne) {
    return *mem & 1 ?
            getop(3, mne, reg(0, true), imm16(read16(mem + 1))) :
            getop(2, mne, reg(0, false), imm8(mem[1]));
}

OpCode disasm1(uint8_t *text, uint16_t addr) {
    uint8_t *mem = text + addr, b = mem[0];
    switch (b) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03: return regrm(mem, "add", b & 2, b & 1);
        case 0x04:
        case 0x05: return aimm(mem, "add");
        case 0x06: return getop(1, "push", es);
        case 0x07: return getop(1, "pop", es);
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b: return regrm(mem, "or", b & 2, b & 1);
        case 0x0c:
        case 0x0d: return aimm(mem, "or");
        case 0x0e: return getop(1, "push", cs);
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13: return regrm(mem, "adc", b & 2, b & 1);
        case 0x14:
        case 0x15: return aimm(mem, "adc");
        case 0x16: return getop(1, "push", ss);
        case 0x17: return getop(1, "pop", ss);
        case 0x18:
        case 0x19:
        case 0x1a:
        case 0x1b: return regrm(mem, "sbb", b & 2, b & 1);
        case 0x1c:
        case 0x1d: return aimm(mem, "sbb");
        case 0x1e: return getop(1, "push", ds);
        case 0x1f: return getop(1, "pop", ds);
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23: return regrm(mem, "and", b & 2, b & 1);
        case 0x24:
        case 0x25: return aimm(mem, "and");
        case 0x26: return getop("es:", es);
        case 0x27: return getop(1, "daa");
        case 0x28:
        case 0x29:
        case 0x2a:
        case 0x2b: return regrm(mem, "sub", b & 2, b & 1);
        case 0x2c:
        case 0x2d: return aimm(mem, "sub");
        case 0x2e: return getop("cs:", cs);
        case 0x2f: return getop(1, "das");
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33: return regrm(mem, "xor", b & 2, b & 1);
        case 0x34:
        case 0x35: return aimm(mem, "xor");
        case 0x36: return getop("ss:", ss);
        case 0x37: return getop(1, "aaa");
        case 0x38:
        case 0x39:
        case 0x3a:
        case 0x3b: return regrm(mem, "cmp", b & 2, b & 1);
        case 0x3c:
        case 0x3d: return aimm(mem, "cmp");
        case 0x3e: return getop("ds:", ds);
        case 0x3f: return getop(1, "aas");
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47: return getop(1, "inc", reg(b & 7, true));
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f: return getop(1, "dec", reg(b & 7, true));
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57: return getop(1, "push", reg(b & 7, true));
        case 0x58:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5e:
        case 0x5f: return getop(1, "pop", reg(b & 7, true));
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7c:
        case 0x7d:
        case 0x7e:
        case 0x7f:
        {
            const char *mnes[] = {
                "jo", "jno", "jb", "jnb", "je", "jne", "jbe", "jnbe",
                "js", "jns", "jp", "jnp", "jl", "jnl", "jle", "jnle"
            };
            return getop(2, mnes[b & 15], disp8(mem + 1, addr + 1));
        }
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        {
            const char *mnes[] = {"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"};
            const char *mne = mnes[(mem[1] >> 3) & 7];
            OpCode op = modrm(mem, mne, b & 1);
            off_t iimm = op.len;
            if (b & 2) {
                op.len++;
                op.opr2 = getopr(1, false, Imm, (int8_t) mem[iimm]);
            } else if (b & 1) {
                op.len += 2;
                op.opr2 = imm16(::read16(mem + iimm));
            } else {
                op.len++;
                op.opr2 = imm8(mem[iimm]);
            }
            return op;
        }
        case 0x84:
        case 0x85: return regrm(mem, "test", false, b & 1);
        case 0x86:
        case 0x87: return regrm(mem, "xchg", false, b & 1);
        case 0x88:
        case 0x89:
        case 0x8a:
        case 0x8b: return regrm(mem, "mov", b & 2, b & 1);
        case 0x8c: return regrm(mem, "mov", false, 2);
        case 0x8d: return regrm(mem, "lea", true, 1);
        case 0x8e: return regrm(mem, "mov", true, 2);
        case 0x8f: return modrm(mem, "pop", true);
        case 0x90: return getop(1, "nop");
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x96:
        case 0x97: return getop(1, "xchg", reg(b & 7, true), reg(0, true));
        case 0x98: return getop(1, "cbw");
        case 0x99: return getop(1, "cwd");
        case 0x9a: return getop(5, "callf", far(::read32(mem + 1)));
        case 0x9b: return getop(1, "wait");
        case 0x9c: return getop(1, "pushf");
        case 0x9d: return getop(1, "popf");
        case 0x9e: return getop(1, "sahf");
        case 0x9f: return getop(1, "lahf");
        case 0xa0:
        case 0xa1: return getop(3, "mov", reg(0, b & 1), ptr(::read16(mem + 1), b & 1));
        case 0xa2:
        case 0xa3: return getop(3, "mov", ptr(::read16(mem + 1), b & 1), reg(0, b & 1));
        case 0xa4: return getop(1, "movsb");
        case 0xa5: return getop(1, "movsw");
        case 0xa6: return getop(1, "cmpsb");
        case 0xa7: return getop(1, "cmpsw");
        case 0xa8: return getop(2, "test", reg(0, false), imm8(mem[1]));
        case 0xa9: return getop(3, "test", reg(0, true), imm16(::read16(mem + 1)));
        case 0xaa: return getop(1, "stosb");
        case 0xab: return getop(1, "stosw");
        case 0xac: return getop(1, "lodsb");
        case 0xad: return getop(1, "lodsw");
        case 0xae: return getop(1, "scasb");
        case 0xaf: return getop(1, "scasw");
        case 0xb0:
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xb7: return getop(2, "mov", reg(b & 7, false), imm8(mem[1]));
        case 0xb8:
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xbe:
        case 0xbf: return getop(3, "mov", reg(b & 7, true), imm16(::read16(mem + 1)));
        case 0xc2: return getop(3, "ret", imm16(::read16(mem + 1)));
        case 0xc3: return getop(1, "ret");
        case 0xc4: return regrm(mem, "les", true, 1);
        case 0xc5: return regrm(mem, "lds", true, 1);
        case 0xc6:
        case 0xc7:
        {
            OpCode op = modrm(mem, "mov", b & 1);
            op.opr2 = b & 1 ? imm16(::read16(mem + op.len)) : imm8(mem[op.len]);
            op.len += op.opr2.len;
            return op;
        }
        case 0xca: return getop(3, "retf", imm16(::read16(mem + 1)));
        case 0xcb: return getop(1, "retf");
        case 0xcc: return getop(1, "int3");
        case 0xcd: return getop(2, "int", imm8(mem[1]));
        case 0xce: return getop(1, "into");
        case 0xcf: return getop(1, "iret");
        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xd3:
        {
            const char *mnes[] = {"rol", "ror", "rcl", "rcr", "shl", "shr", NULL, "sar"};
            const char *mne = mnes[(mem[1] >> 3) & 7];
            if (!mne) break;
            OpCode op = modrm(mem, mne, b & 1);
            op.opr2 = b & 2 ? cl : getopr(0, false, Imm, 1);
            return op;
        }
        case 0xd4: if (mem[1] == 0x0a) return getop(2, "aam");
            else break;
        case 0xd5: if (mem[1] == 0x0a) return getop(2, "aad");
            else break;
        case 0xd7: return getop(1, "xlat");
#if 0
        case 0xd8:
        case 0xd9:
        case 0xda:
        case 0xdb:
        case 0xdc:
        case 0xdd:
        case 0xde:
        case 0xdf:
        {
            OpCode op = modrm(mem, "esc", false);
            op.opr2 = Operand(0, true, Addr, (b << 16) | ((mem[1] >> 3) & 7));
            swap(&op);
            return op;
        }
#endif
        case 0xe0: return getop(2, "loopnz", disp8(mem + 1, addr + 1));
        case 0xe1: return getop(2, "loopz", disp8(mem + 1, addr + 1));
        case 0xe2: return getop(2, "loop", disp8(mem + 1, addr + 1));
        case 0xe3: return getop(2, "jcxz", disp8(mem + 1, addr + 1));
        case 0xe4:
        case 0xe5: return getop(2, "in", reg(0, b & 1), imm8(mem[1]));
        case 0xe6:
        case 0xe7: return getop(2, "out", imm8(mem[1]), reg(0, b & 1));
        case 0xe8: return getop(3, "call", disp16(mem + 1, addr + 1));
        case 0xe9: return getop(3, "jmp", disp16(mem + 1, addr + 1));
        case 0xea: return getop(5, "jmpf", far(::read32(mem + 1)));
        case 0xeb: return getop(2, "jmp short", disp8(mem + 1, addr + 1));
        case 0xec:
        case 0xed: return getop(1, "in", reg(0, b & 1), dx);
        case 0xee:
        case 0xef: return getop(1, "out", dx, reg(0, b & 1));
        case 0xf0: return getop("lock");
        case 0xf2: return getop("repnz");
        case 0xf3: return getop("repz");
        case 0xf4: return getop(1, "hlt");
        case 0xf5: return getop(1, "cmc");
        case 0xf6:
        case 0xf7:
        {
            const char *mnes[] = {"test", NULL, "not", "neg", "mul", "imul", "div", "idiv"};
            int t = (mem[1] >> 3) & 7;
            const char *mne = mnes[t];
            if (!mne) break;
            OpCode op = modrm(mem, mne, b & 1);
            if (t > 0) return op;
            op.opr2 = b & 1 ? imm16(::read16(mem + op.len)) : imm8(mem[op.len]);
            op.len += op.opr2.len;
            return op;
        }
        case 0xf8: return getop(1, "clc");
        case 0xf9: return getop(1, "stc");
        case 0xfa: return getop(1, "cli");
        case 0xfb: return getop(1, "sti");
        case 0xfc: return getop(1, "cld");
        case 0xfd: return getop(1, "std");
        case 0xfe:
        case 0xff:
        {
            const char *mnes[] = {"inc", "dec", "call", "callf", "jmp", "jmpf", "push", NULL};
            const char *mne = mnes[(mem[1] >> 3) & 7];
            if (!mne) break;
            return modrm(mem, mne, b & 1);
        }
    }
    return undefop;
}

size_t disasm1(Operand *opr1, Operand *opr2, uint8_t *text, uint16_t addr) {
    OpCode op1 = disasm1(text, addr), *ret = &op1;
    uint16_t addr2 = addr + op1.len;
    if (op1.prefix) {
        OpCode op2 = disasm1(text, addr2);
        if (!op2.prefix) {
            op2.len += op1.len;
            if (!op1.opr2.empty()) {
                if (op2.opr1.type >= Ptr) {
                    op2.opr1.seg = op1.opr2.value;
                    ret = &op2;
                } else if (op2.opr2.type >= Ptr) {
                    op2.opr2.seg = op1.opr2.value;
                    ret = &op2;
                }
            }
        }
    }
    *opr1 = ret->opr1;
    *opr2 = ret->opr2;
    return ret->len;
}
