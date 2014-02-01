#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define AX r[0]
#define CX r[1]
#define DX r[2]
#define BX r[3]
#define SP r[4]
#define BP r[5]
#define SI r[6]
#define DI r[7]
#define AL *r8[0]
#define CL *r8[1]
#define DL *r8[2]
#define BL *r8[3]
#define AH *r8[4]
#define CH *r8[5]
#define DH *r8[6]
#define BH *r8[7]

uint8_t mem[0x11000];
uint16_t ip, r[8];
uint8_t *r8[8];
bool OF, DF, SF, ZF, PF, CF;
bool ptable[256];

inline uint16_t read16(uint8_t *mem) {
    return mem[0] | (mem[1] << 8);
}

inline uint32_t read32(uint8_t *mem) {
    return mem[0] | (mem[1] << 8) | (mem[2] << 16) | (mem[3] << 24);
}

// Operand

enum OperandType {
    Reg, SReg, Imm, Addr, Far, Ptr, ModRM
};

struct Operand {
    int len;
    bool w;
    int type, value, seg;

    inline bool empty() const {
        return len < 0;
    }
};

inline Operand getopr(int len, bool w, int type, int value, int seg = -1) {
    Operand ret = {len, w, type, value, seg};
    return ret;
}

Operand noopr = getopr(-1, false, 0, 0);
Operand dx = getopr(0, true, Reg, 2);
Operand cl = getopr(0, false, Reg, 1);
Operand es = getopr(0, true, SReg, 0);
Operand cs = getopr(0, true, SReg, 1);
Operand ss = getopr(0, true, SReg, 2);
Operand ds = getopr(0, true, SReg, 3);

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

// disasm

inline size_t getop(
        Operand *r1, Operand *r2,
        size_t len, const char *mne,
        const Operand &opr1 = noopr,
        const Operand &opr2 = noopr) {
    *r1 = opr1;
    *r2 = opr2;
    return len;
}

inline size_t modrm(Operand *opr1, Operand *opr2, uint8_t *mem, const char *mne, bool w) {
    Operand opr;
    uint8_t b = mem[1], mod = b >> 6, rm = b & 7;
    switch (mod) {
        case 0:
            if (rm == 6) {
                opr = getopr(3, w, Ptr, read16(mem + 2));
            } else {
                opr = getopr(1, w, ModRM + rm, 0);
            }
            break;
        case 1:
            opr = getopr(2, w, ModRM + rm, (int8_t) mem[2]);
            break;
        case 2:
            opr = getopr(3, w, ModRM + rm, (int16_t) read16(mem + 2));
            break;
        default:
            opr = getopr(1, w, Reg, rm);
            break;
    }
    return getop(opr1, opr2, 1 + opr.len, mne, opr);
}

inline size_t regrm(Operand *opr1, Operand *opr2, uint8_t *mem, const char *mne, bool d, int w) {
    size_t len = modrm(opr1, opr2, mem, mne, w);
    if (w == 2) {
        *opr2 = getopr(0, w, SReg, (mem[1] >> 3) & 3);
    } else {
        *opr2 = getopr(0, w, Reg, (mem[1] >> 3) & 7);
    }
    if (d) {
        Operand tmp = *opr1;
        *opr1 = *opr2;
        *opr2 = tmp;
    }
    return len;
}

inline size_t aimm(Operand *opr1, Operand *opr2, uint8_t *mem, const char *mne) {
    return *mem & 1 ?
            getop(opr1, opr2, 3, mne, reg(0, true), imm16(read16(mem + 1))) :
            getop(opr1, opr2, 2, mne, reg(0, false), imm8(mem[1]));
}

size_t disasm(Operand *opr1, Operand *opr2, uint8_t *p) {
    uint8_t b = *p;
    switch (b) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03: return regrm(opr1, opr2, p, "add", b & 2, b & 1);
        case 0x04:
        case 0x05: return aimm(opr1, opr2, p, "add");
        case 0x06: return getop(opr1, opr2, 1, "push", es);
        case 0x07: return getop(opr1, opr2, 1, "pop", es);
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b: return regrm(opr1, opr2, p, "or", b & 2, b & 1);
        case 0x0c:
        case 0x0d: return aimm(opr1, opr2, p, "or");
        case 0x0e: return getop(opr1, opr2, 1, "push", cs);
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13: return regrm(opr1, opr2, p, "adc", b & 2, b & 1);
        case 0x14:
        case 0x15: return aimm(opr1, opr2, p, "adc");
        case 0x16: return getop(opr1, opr2, 1, "push", ss);
        case 0x17: return getop(opr1, opr2, 1, "pop", ss);
        case 0x18:
        case 0x19:
        case 0x1a:
        case 0x1b: return regrm(opr1, opr2, p, "sbb", b & 2, b & 1);
        case 0x1c:
        case 0x1d: return aimm(opr1, opr2, p, "sbb");
        case 0x1e: return getop(opr1, opr2, 1, "push", ds);
        case 0x1f: return getop(opr1, opr2, 1, "pop", ds);
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23: return regrm(opr1, opr2, p, "and", b & 2, b & 1);
        case 0x24:
        case 0x25: return aimm(opr1, opr2, p, "and");
        case 0x27: return getop(opr1, opr2, 1, "daa");
        case 0x28:
        case 0x29:
        case 0x2a:
        case 0x2b: return regrm(opr1, opr2, p, "sub", b & 2, b & 1);
        case 0x2c:
        case 0x2d: return aimm(opr1, opr2, p, "sub");
        case 0x2f: return getop(opr1, opr2, 1, "das");
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33: return regrm(opr1, opr2, p, "xor", b & 2, b & 1);
        case 0x34:
        case 0x35: return aimm(opr1, opr2, p, "xor");
        case 0x37: return getop(opr1, opr2, 1, "aaa");
        case 0x38:
        case 0x39:
        case 0x3a:
        case 0x3b: return regrm(opr1, opr2, p, "cmp", b & 2, b & 1);
        case 0x3c:
        case 0x3d: return aimm(opr1, opr2, p, "cmp");
        case 0x3f: return getop(opr1, opr2, 1, "aas");
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47: return getop(opr1, opr2, 1, "inc", reg(b & 7, true));
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f: return getop(opr1, opr2, 1, "dec", reg(b & 7, true));
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57: return getop(opr1, opr2, 1, "push", reg(b & 7, true));
        case 0x58:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5e:
        case 0x5f: return getop(opr1, opr2, 1, "pop", reg(b & 7, true));
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
            return getop(opr1, opr2, 2, mnes[b & 15], disp8(p + 1, ip + 1));
        }
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        {
            const char *mnes[] = {"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"};
            const char *mne = mnes[(p[1] >> 3) & 7];
            size_t oplen = modrm(opr1, opr2, p, mne, b & 1);
            off_t iimm = oplen;
            if (b & 2) {
                oplen++;
                *opr2 = getopr(1, false, Imm, (int8_t) p[iimm]);
            } else if (b & 1) {
                oplen += 2;
                *opr2 = imm16(::read16(p + iimm));
            } else {
                oplen++;
                *opr2 = imm8(p[iimm]);
            }
            return oplen;
        }
        case 0x84:
        case 0x85: return regrm(opr1, opr2, p, "test", false, b & 1);
        case 0x86:
        case 0x87: return regrm(opr1, opr2, p, "xchg", false, b & 1);
        case 0x88:
        case 0x89:
        case 0x8a:
        case 0x8b: return regrm(opr1, opr2, p, "mov", b & 2, b & 1);
        case 0x8c: return regrm(opr1, opr2, p, "mov", false, 2);
        case 0x8d: return regrm(opr1, opr2, p, "lea", true, 1);
        case 0x8e: return regrm(opr1, opr2, p, "mov", true, 2);
        case 0x8f: return modrm(opr1, opr2, p, "pop", true);
        case 0x90: return getop(opr1, opr2, 1, "nop");
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x96:
        case 0x97: return getop(opr1, opr2, 1, "xchg", reg(b & 7, true), reg(0, true));
        case 0x98: return getop(opr1, opr2, 1, "cbw");
        case 0x99: return getop(opr1, opr2, 1, "cwd");
        case 0x9a: return getop(opr1, opr2, 5, "callf", far(::read32(p + 1)));
        case 0x9b: return getop(opr1, opr2, 1, "wait");
        case 0x9c: return getop(opr1, opr2, 1, "pushf");
        case 0x9d: return getop(opr1, opr2, 1, "popf");
        case 0x9e: return getop(opr1, opr2, 1, "sahf");
        case 0x9f: return getop(opr1, opr2, 1, "lahf");
        case 0xa0:
        case 0xa1: return getop(opr1, opr2, 3, "mov", reg(0, b & 1), ptr(::read16(p + 1), b & 1));
        case 0xa2:
        case 0xa3: return getop(opr1, opr2, 3, "mov", ptr(::read16(p + 1), b & 1), reg(0, b & 1));
        case 0xa4: return getop(opr1, opr2, 1, "movsb");
        case 0xa5: return getop(opr1, opr2, 1, "movsw");
        case 0xa6: return getop(opr1, opr2, 1, "cmpsb");
        case 0xa7: return getop(opr1, opr2, 1, "cmpsw");
        case 0xa8: return getop(opr1, opr2, 2, "test", reg(0, false), imm8(p[1]));
        case 0xa9: return getop(opr1, opr2, 3, "test", reg(0, true), imm16(::read16(p + 1)));
        case 0xaa: return getop(opr1, opr2, 1, "stosb");
        case 0xab: return getop(opr1, opr2, 1, "stosw");
        case 0xac: return getop(opr1, opr2, 1, "lodsb");
        case 0xad: return getop(opr1, opr2, 1, "lodsw");
        case 0xae: return getop(opr1, opr2, 1, "scasb");
        case 0xaf: return getop(opr1, opr2, 1, "scasw");
        case 0xb0:
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xb7: return getop(opr1, opr2, 2, "mov", reg(b & 7, false), imm8(p[1]));
        case 0xb8:
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xbe:
        case 0xbf: return getop(opr1, opr2, 3, "mov", reg(b & 7, true), imm16(::read16(p + 1)));
        case 0xc2: return getop(opr1, opr2, 3, "ret", imm16(::read16(p + 1)));
        case 0xc3: return getop(opr1, opr2, 1, "ret");
        case 0xc4: return regrm(opr1, opr2, p, "les", true, 1);
        case 0xc5: return regrm(opr1, opr2, p, "lds", true, 1);
        case 0xc6:
        case 0xc7:
        {
            size_t oplen = modrm(opr1, opr2, p, "mov", b & 1);
            *opr2 = b & 1 ? imm16(::read16(p + oplen)) : imm8(p[oplen]);
            oplen += opr2->len;
            return oplen;
        }
        case 0xca: return getop(opr1, opr2, 3, "retf", imm16(::read16(p + 1)));
        case 0xcb: return getop(opr1, opr2, 1, "retf");
        case 0xcc: return getop(opr1, opr2, 1, "int3");
        case 0xcd: return getop(opr1, opr2, 2, "int", imm8(p[1]));
        case 0xce: return getop(opr1, opr2, 1, "into");
        case 0xcf: return getop(opr1, opr2, 1, "iret");
        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xd3:
        {
            const char *mnes[] = {"rol", "ror", "rcl", "rcr", "shl", "shr", NULL, "sar"};
            const char *mne = mnes[(p[1] >> 3) & 7];
            if (!mne) break;
            size_t oplen = modrm(opr1, opr2, p, mne, b & 1);
            *opr2 = b & 2 ? cl : getopr(0, false, Imm, 1);
            return oplen;
        }
        case 0xd4: if (p[1] == 0x0a) return getop(opr1, opr2, 2, "aam");
            else break;
        case 0xd5: if (p[1] == 0x0a) return getop(opr1, opr2, 2, "aad");
            else break;
        case 0xd7: return getop(opr1, opr2, 1, "xlat");
        case 0xe0: return getop(opr1, opr2, 2, "loopnz", disp8(p + 1, ip + 1));
        case 0xe1: return getop(opr1, opr2, 2, "loopz", disp8(p + 1, ip + 1));
        case 0xe2: return getop(opr1, opr2, 2, "loop", disp8(p + 1, ip + 1));
        case 0xe3: return getop(opr1, opr2, 2, "jcxz", disp8(p + 1, ip + 1));
        case 0xe4:
        case 0xe5: return getop(opr1, opr2, 2, "in", reg(0, b & 1), imm8(p[1]));
        case 0xe6:
        case 0xe7: return getop(opr1, opr2, 2, "out", imm8(p[1]), reg(0, b & 1));
        case 0xe8: return getop(opr1, opr2, 3, "call", disp16(p + 1, ip + 1));
        case 0xe9: return getop(opr1, opr2, 3, "jmp", disp16(p + 1, ip + 1));
        case 0xea: return getop(opr1, opr2, 5, "jmpf", far(::read32(p + 1)));
        case 0xeb: return getop(opr1, opr2, 2, "jmp short", disp8(p + 1, ip + 1));
        case 0xec:
        case 0xed: return getop(opr1, opr2, 1, "in", reg(0, b & 1), dx);
        case 0xee:
        case 0xef: return getop(opr1, opr2, 1, "out", dx, reg(0, b & 1));
        case 0xf4: return getop(opr1, opr2, 1, "hlt");
        case 0xf5: return getop(opr1, opr2, 1, "cmc");
        case 0xf6:
        case 0xf7:
        {
            const char *mnes[] = {"test", NULL, "not", "neg", "mul", "imul", "div", "idiv"};
            int t = (p[1] >> 3) & 7;
            const char *mne = mnes[t];
            if (!mne) break;
            size_t oplen = modrm(opr1, opr2, p, mne, b & 1);
            if (t > 0) return oplen;
            *opr2 = b & 1 ? imm16(::read16(p + oplen)) : imm8(p[oplen]);
            oplen += opr2->len;
            return oplen;
        }
        case 0xf8: return getop(opr1, opr2, 1, "clc");
        case 0xf9: return getop(opr1, opr2, 1, "stc");
        case 0xfa: return getop(opr1, opr2, 1, "cli");
        case 0xfb: return getop(opr1, opr2, 1, "sti");
        case 0xfc: return getop(opr1, opr2, 1, "cld");
        case 0xfd: return getop(opr1, opr2, 1, "std");
        case 0xfe:
        case 0xff:
        {
            const char *mnes[] = {"inc", "dec", "call", "callf", "jmp", "jmpf", "push", NULL};
            const char *mne = mnes[(p[1] >> 3) & 7];
            if (!mne) break;
            return modrm(opr1, opr2, p, mne, b & 1);
        }
    }
    *opr1 = noopr;
    *opr2 = noopr;
    return 1;
}

// run

int addr(const Operand &opr) {
    switch (opr.type) {
        case Ptr: return uint16_t(opr.value);
        case ModRM + 0: return uint16_t(BX + SI + opr.value);
        case ModRM + 1: return uint16_t(BX + DI + opr.value);
        case ModRM + 2: return uint16_t(BP + SI + opr.value);
        case ModRM + 3: return uint16_t(BP + DI + opr.value);
        case ModRM + 4: return uint16_t(SI + opr.value);
        case ModRM + 5: return uint16_t(DI + opr.value);
        case ModRM + 6: return uint16_t(BP + opr.value);
        case ModRM + 7: return uint16_t(BX + opr.value);
    }
    return -1;
}

inline uint8_t read8(uint16_t addr) {
    return mem[addr];
}

inline uint16_t read16(uint16_t addr) {
    return ::read16(mem + addr);
}

inline uint32_t read32(uint16_t addr) {
    return ::read32(mem + addr);
}

inline void write8(uint16_t addr, uint8_t value) {
    mem[addr] = value;
}

inline void write16(uint16_t addr, uint16_t value) {
    mem[addr] = value;
    mem[addr + 1] = value >> 8;
}

inline void write32(uint16_t addr, uint32_t value) {
    mem[addr] = value;
    mem[addr + 1] = value >> 8;
    mem[addr + 2] = value >> 16;
    mem[addr + 3] = value >> 24;
}

inline int setf8(int value, bool cf) {
    int8_t v = value;
    OF = value != v;
    SF = v < 0;
    ZF = v == 0;
    PF = ptable[uint8_t(value)];
    CF = cf;
    return value;
}

inline int setf16(int value, bool cf) {
    int16_t v = value;
    OF = value != v;
    SF = v < 0;
    ZF = v == 0;
    PF = ptable[uint8_t(value)];
    CF = cf;
    return value;
}

inline uint8_t get8(const Operand &opr) {
    switch (opr.type) {
        case Reg: return *r8[opr.value];
        case Imm: return opr.value;
    }
    int ad = addr(opr);
    return ad < 0 ? 0 : read8(ad);
}

inline uint16_t get16(const Operand &opr) {
    switch (opr.type) {
        case Reg: return r[opr.value];
        case Imm: return opr.value;
    }
    int ad = addr(opr);
    return ad < 0 ? 0 : read16(ad);
}

inline void set8(const Operand &opr, uint8_t value) {
    if (opr.type == Reg) {
        *r8[opr.value] = value;
    } else {
        int ad = addr(opr);
        if (ad >= 0) write8(ad, value);
    }
}

inline void set16(const Operand &opr, uint16_t value) {
    if (opr.type == Reg) {
        r[opr.value] = value;
    } else {
        int ad = addr(opr);
        if (ad >= 0) write16(ad, value);
    }
}

void run(uint8_t rep, uint8_t seg) {
    Operand opr1, opr2;
    uint8_t *p = mem + ip;
    size_t oplen = disasm(&opr1, &opr2, p);
    int opr1v = opr1.value, opr2v = opr2.value;
    uint8_t b = mem[ip];
    uint16_t oldip = ip;
    int dst, src, val;
    ip += oplen;
    switch (b) {
        case 0x00: // add r/m, reg8
            val = int8_t(dst = get8(opr1)) + int8_t(*r8[opr2v]);
            set8(opr1, setf8(val, dst > uint8_t(val)));
            return;
        case 0x01: // add r/m, reg16
            val = int16_t(dst = get16(opr1)) + int16_t(r[opr2v]);
            set16(opr1, setf16(val, dst > uint16_t(val)));
            return;
        case 0x02: // add reg8, r/m
            val = int8_t(dst = *r8[opr1v]) + int8_t(get8(opr2));
            *r8[opr1v] = setf8(val, dst > uint8_t(val));
            return;
        case 0x03: // add reg16, r/m
            val = int16_t(dst = r[opr1v]) + int16_t(get16(opr2));
            r[opr1v] = setf16(val, dst > uint16_t(val));
            return;
        case 0x04: // add al, imm8
            val = int8_t(dst = AL) + int8_t(opr2v);
            AL = setf8(val, dst > uint8_t(val));
            return;
        case 0x05: // add ax, imm16
            val = int16_t(dst = AX) + int16_t(opr2v);
            AX = setf16(val, dst > uint16_t(val));
            return;
        case 0x08: // or r/m, reg8
            set8(opr1, setf8(int8_t(get8(opr1) | *r8[opr2v]), false));
            return;
        case 0x09: // or r/m, reg16
            set16(opr1, setf16(int16_t(get16(opr1) | r[opr2v]), false));
            return;
        case 0x0a: // or reg8, r/m
            *r8[opr1v] = setf8(int8_t(*r8[opr1v] | get8(opr2)), false);
            return;
        case 0x0b: // or reg16, r/m
            r[opr1v] = setf16(int16_t(r[opr1v] | get16(opr2)), false);
            return;
        case 0x0c: // or al, imm8
            AL = setf8(int8_t(AL | opr2v), false);
            return;
        case 0x0d: // or ax, imm16
            AX = setf16(int16_t(AX | opr2v), false);
            return;
        case 0x10: // adc r/m, reg8
            val = int8_t(dst = get8(opr1)) + int8_t(*r8[opr2v]) + int(CF);
            set8(opr1, setf8(val, dst > uint8_t(val)));
            return;
        case 0x11: // adc r/m, reg16
            val = int16_t(dst = get16(opr1)) + int16_t(r[opr2v]) + int(CF);
            set16(opr1, setf16(val, dst > uint16_t(val)));
            return;
        case 0x12: // adc reg8, r/m
            val = int8_t(dst = *r8[opr1v]) + int8_t(get8(opr2)) + int(CF);
            *r8[opr1v] = setf8(val, dst > uint8_t(val));
            return;
        case 0x13: // adc reg16, r/m
            val = int16_t(dst = r[opr1v]) + int16_t(get16(opr2)) + int(CF);
            r[opr1v] = setf16(val, dst > uint16_t(val));
            return;
        case 0x14: // adc al, imm8
            val = int8_t(dst = AL) + int8_t(opr2v) + int(CF);
            AL = setf8(val, dst > uint8_t(val));
            return;
        case 0x15: // adc ax, imm16
            val = int16_t(dst = AX) + int16_t(opr2v) + int(CF);
            AX = setf16(val, dst > uint16_t(val));
            return;
        case 0x18: // sbb r/m, reg8
            val = int8_t(dst = get8(opr1)) - int8_t(src = *r8[opr2v] + int(CF));
            set8(opr1, setf8(val, dst < src));
            return;
        case 0x19: // sbb r/m, reg16
            val = int16_t(dst = get16(opr1)) - int16_t(src = r[opr2v] + int(CF));
            set16(opr1, setf16(val, dst < src));
            return;
        case 0x1a: // sbb reg8, r/m
            val = int8_t(dst = *r8[opr1v]) - int8_t(src = get8(opr2) + int(CF));
            *r8[opr1v] = setf8(val, dst < src);
            return;
        case 0x1b: // sbb reg16, r/m
            val = int16_t(dst = r[opr1v]) - int16_t(src = get16(opr2) + int(CF));
            r[opr1v] = setf16(val, dst < src);
            return;
        case 0x1c: // sbb al, imm8
            val = int8_t(dst = AL) - int8_t(src = opr2v + int(CF));
            AL = setf8(val, dst < src);
            return;
        case 0x1d: // sbb ax, imm16
            val = int16_t(dst = AX) - int16_t(src = opr2v + int(CF));
            AX = setf16(val, dst < src);
            return;
        case 0x20: // and r/m, reg8
            set8(opr1, setf8(int8_t(get8(opr1) & *r8[opr2v]), false));
            return;
        case 0x21: // and r/m, reg16
            set16(opr1, setf16(int16_t(get16(opr1) & r[opr2v]), false));
            return;
        case 0x22: // and reg8, r/m
            *r8[opr1v] = setf8(int8_t(*r8[opr1v] & get8(opr2)), false);
            return;
        case 0x23: // and reg16, r/m
            r[opr1v] = setf16(int16_t(r[opr1v] & get16(opr2)), false);
            return;
        case 0x24: // and al, imm8
            AL = setf8(int8_t(AL & opr2v), false);
            return;
        case 0x25: // and ax, imm16
            AX = setf16(int16_t(AX & opr2v), false);
            return;
        case 0x26: // es:
            return run(0, b);
        case 0x28: // sub r/m, reg8
            val = int8_t(dst = get8(opr1)) - int8_t(src = *r8[opr2v]);
            set8(opr1, setf8(val, dst < src));
            return;
        case 0x29: // sub r/m, reg16
            val = int16_t(dst = get16(opr1)) - int16_t(src = r[opr2v]);
            set16(opr1, setf16(val, dst < src));
            return;
        case 0x2a: // sub reg8, r/m
            val = int8_t(dst = *r8[opr1v]) - int8_t(src = get8(opr2));
            *r8[opr1v] = setf8(val, dst < src);
            return;
        case 0x2b: // sub reg16, r/m
            val = int16_t(dst = r[opr1v]) - int16_t(src = get16(opr2));
            r[opr1v] = setf16(val, dst < src);
            return;
        case 0x2c: // sub al, imm8
            val = int8_t(dst = AL) - int8_t(src = opr2v);
            AL = setf8(val, dst < src);
            return;
        case 0x2d: // sub ax, imm16
            val = int16_t(dst = AX) - int16_t(src = opr2v);
            AX = setf16(val, dst < src);
            return;
        case 0x2e: // cs:
            return run(0, b);
        case 0x30: // xor r/m, reg8
            set8(opr1, setf8(int8_t(get8(opr1) ^ *r8[opr2v]), false));
            return;
        case 0x31: // xor r/m, reg16
            set16(opr1, setf16(int16_t(get16(opr1) ^ r[opr2v]), false));
            return;
        case 0x32: // xor reg8, r/m
            *r8[opr1v] = setf8(int8_t(*r8[opr1v] ^ get8(opr2)), false);
            return;
        case 0x33: // xor reg16, r/m
            r[opr1v] = setf16(int16_t(r[opr1v] ^ get16(opr2)), false);
            return;
        case 0x34: // xor al, imm8
            AL = setf8(int8_t(AL ^ opr2v), false);
            return;
        case 0x35: // xor ax, imm16
            AX = setf16(int16_t(AX ^ opr2v), false);
            return;
        case 0x36: // ss:
            return run(0, b);
        case 0x38: // cmp r/m, reg8
            val = int8_t(dst = get8(opr1)) - int8_t(src = *r8[opr2v]);
            setf8(val, dst < src);
            return;
        case 0x39: // cmp r/m, reg16
            val = int16_t(dst = get16(opr1)) - int16_t(src = r[opr2v]);
            setf16(val, dst < src);
            return;
        case 0x3a: // cmp reg8, r/m
            val = int8_t(dst = *r8[opr1v]) - int8_t(src = get8(opr2));
            setf8(val, dst < src);
            return;
        case 0x3b: // cmp reg16, r/m
            val = int16_t(dst = r[opr1v]) - int16_t(src = get16(opr2));
            setf16(val, dst < src);
            return;
        case 0x3c: // cmp al, imm8
            val = int8_t(dst = AL) - int8_t(src = opr2v);
            setf8(val, dst < src);
            return;
        case 0x3d: // cmp ax, imm16
            val = int16_t(dst = AX) - int16_t(src = opr2v);
            setf16(val, dst < src);
            return;
        case 0x3e: // ds:
            return run(0, b);
        case 0x40: // inc reg16
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
            r[opr1v] = setf16(int16_t(r[opr1v]) + 1, CF);
            return;
        case 0x48: // dec reg16
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f:
            r[opr1v] = setf16(int16_t(r[opr1v]) - 1, CF);
            return;
        case 0x50: // push reg16
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
            val = r[opr1v];
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
            r[opr1v] = read16(SP);
            SP += 2;
            return;
        case 0x70: // jo
            if (OF) ip = opr1v;
            return;
        case 0x71: // jno
            if (!OF) ip = opr1v;
            return;
        case 0x72: // jb/jnae
            if (CF) ip = opr1v;
            return;
        case 0x73: // jnb/jae
            if (!CF) ip = opr1v;
            return;
        case 0x74: // je/jz
            if (ZF) ip = opr1v;
            return;
        case 0x75: // jne/jnz
            if (!ZF) ip = opr1v;
            return;
        case 0x76: // jbe/jna
            if (CF || ZF) ip = opr1v;
            return;
        case 0x77: // jnbe/ja
            if (!CF && !ZF) ip = opr1v;
            return;
        case 0x78: // js
            if (SF) ip = opr1v;
            return;
        case 0x79: // jns
            if (!SF) ip = opr1v;
            return;
        case 0x7a: // jp
            if (PF) ip = opr1v;
            return;
        case 0x7b: // jnp
            if (!PF) ip = opr1v;
            return;
        case 0x7c: // jl/jnge
            if (SF != OF) ip = opr1v;
            return;
        case 0x7d: // jnl/jge
            if (SF == OF) ip = opr1v;
            return;
        case 0x7e: // jle/jng
            if (ZF || SF != OF) ip = opr1v;
            return;
        case 0x7f: // jnle/jg
            if (!ZF && SF == OF) ip = opr1v;
            return;
        case 0x80: // r/m, imm8
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // add
                    val = int8_t(dst = get8(opr1)) + int8_t(opr2v);
                    set8(opr1, setf8(val, dst > uint8_t(val)));
                    return;
                case 1: // or
                    set8(opr1, setf8(int8_t(get8(opr1) | opr2v), false));
                    return;
                case 2: // adc
                    val = int8_t(dst = get8(opr1)) + int8_t(opr2v) + int(CF);
                    set8(opr1, setf8(val, dst > uint8_t(val)));
                    return;
                case 3: // sbb
                    val = int8_t(dst = get8(opr1)) - int8_t(src = opr2v + int(CF));
                    set8(opr1, setf8(val, dst < src));
                    return;
                case 4: // and
                    set8(opr1, setf8(int8_t(get8(opr1) & opr2v), false));
                    return;
                case 5: // sub
                    val = int8_t(dst = get8(opr1)) - int8_t(src = opr2v);
                    set8(opr1, setf8(val, dst < src));
                    return;
                case 6: // xor
                    set8(opr1, setf8(int8_t(get8(opr1) ^ opr2v), false));
                    return;
                case 7: // cmp
                    val = int8_t(dst = get8(opr1)) - int8_t(src = opr2v);
                    setf8(val, dst < src);
                    return;
            }
            break;
        case 0x81: // r/m, imm16
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // add
                    val = int16_t(dst = get16(opr1)) + int16_t(opr2v);
                    set16(opr1, setf16(val, dst > uint16_t(val)));
                    return;
                case 1: // or
                    set16(opr1, setf16(int16_t(get16(opr1) | opr2v), false));
                    return;
                case 2: // adc
                    val = int16_t(dst = get16(opr1)) + int16_t(opr2v) + int(CF);
                    set16(opr1, setf16(val, dst > uint16_t(val)));
                    return;
                case 3: // sbb
                    val = int16_t(dst = get16(opr1)) - int16_t(src = opr2v + int(CF));
                    set16(opr1, setf16(val, dst < src));
                    return;
                case 4: // and
                    set16(opr1, setf16(int16_t(get16(opr1) & opr2v), false));
                    return;
                case 5: // sub
                    val = int16_t(dst = get16(opr1)) - int16_t(src = opr2v);
                    set16(opr1, setf16(val, dst < src));
                    return;
                case 6: // xor
                    set16(opr1, setf16(int16_t(get16(opr1) ^ opr2v), false));
                    return;
                case 7: // cmp
                    val = int16_t(dst = get16(opr1)) - int16_t(src = opr2v);
                    setf16(val, dst < src);
                    return;
            }
            break;
        case 0x82: // r/m, imm8(signed)
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // add
                    val = int8_t(dst = get8(opr1)) + int8_t(opr2v);
                    set8(opr1, setf8(val, dst > uint8_t(val)));
                    return;
                case 2: // adc
                    val = int8_t(dst = get8(opr1)) + int8_t(opr2v) + int(CF);
                    set8(opr1, setf8(val, dst > uint8_t(val)));
                    return;
                case 3: // sbb
                    val = int8_t(dst = get8(opr1)) - int8_t(src = opr2v + int(CF));
                    set8(opr1, setf8(val, dst < uint8_t(src)));
                    return;
                case 5: // sub
                    val = int8_t(dst = get8(opr1)) - int8_t(src = opr2v);
                    set8(opr1, setf8(val, dst < uint8_t(src)));
                    return;
                case 7: // cmp
                    val = int8_t(dst = get8(opr1)) - int8_t(src = opr2v);
                    setf8(val, dst < uint8_t(src));
                    return;
            }
            break;
        case 0x83: // r/m, imm16(signed)
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // add
                    val = int16_t(dst = get16(opr1)) + int8_t(opr2v);
                    set16(opr1, setf16(val, dst > uint16_t(val)));
                    return;
                case 2: // adc
                    val = int16_t(dst = get16(opr1)) + int8_t(opr2v) + int(CF);
                    set16(opr1, setf16(val, dst > uint16_t(val)));
                    return;
                case 3: // sbb
                    val = int16_t(dst = get16(opr1)) - int8_t(src = opr2v + int(CF));
                    set16(opr1, setf16(val, dst < uint16_t(src)));
                    return;
                case 5: // sub
                    val = int16_t(dst = get16(opr1)) - int8_t(src = opr2v);
                    set16(opr1, setf16(val, dst < uint16_t(src)));
                    return;
                case 7: // cmp
                    val = int16_t(dst = get16(opr1)) - int8_t(src = opr2v);
                    setf16(val, dst < uint16_t(src));
                    return;
            }
            break;
        case 0x84: // test r/m, reg8
            setf8(int8_t(get8(opr1) & *r8[opr2v]), false);
            return;
        case 0x85: // test r/m, reg16
            setf16(int16_t(get16(opr1) & r[opr2v]), false);
            return;
        case 0x86: // xchg r/m, reg8
            val = *r8[opr2v];
            *r8[opr2v] = get8(opr1);
            set8(opr1, val);
            return;
        case 0x87: // xchg r/m, reg16
            val = r[opr2v];
            r[opr2v] = get16(opr1);
            set16(opr1, val);
            return;
        case 0x88: // mov r/m, reg8
            set8(opr1, *r8[opr2v]);
            return;
        case 0x89: // mov r/m, reg16
            set16(opr1, r[opr2v]);
            return;
        case 0x8a: // mov reg8, r/m
            *r8[opr1v] = get8(opr2);
            return;
        case 0x8b: // mov reg16, r/m
            r[opr1v] = get16(opr2);
            return;
        case 0x8d: // lea reg16, r/m
            r[opr1v] = addr(opr2);
            return;
        case 0x8f: // pop r/m
            set16(opr1, read16(SP));
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
            AX = r[opr1v];
            r[opr1v] = val;
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
            AL = get8(opr2);
            return;
        case 0xa1: // mov ax, [addr]
            AX = get16(opr2);
            return;
        case 0xa2: // mov [addr], al
            set8(opr1, AL);
            return;
        case 0xa3: // mov [addr], ax
            set16(opr1, AX);
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
                if (rep) CX--;
            } while ((rep == 0xf2 || rep == 0xf3) && CX);
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
                if (rep) CX--;
            } while ((rep == 0xf2 || rep == 0xf3) && CX);
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
                if (rep) CX--;
            } while (((rep == 0xf2 && !ZF) || (rep == 0xf3 && ZF)) && CX);
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
                if (rep) CX--;
            } while (((rep == 0xf2 && !ZF) || (rep == 0xf3 && ZF)) && CX);
            return;
        case 0xa8: // test al, imm8
            setf8(int8_t(AL & opr2v), false);
            return;
        case 0xa9: // test ax, imm16
            setf16(int16_t(AX & opr2v), false);
            return;
        case 0xaa: // stosb
            do {
                write8(DI, AL);
                if (DF) DI--;
                else DI++;
                if (rep) CX--;
            } while ((rep == 0xf2 || rep == 0xf3) && CX);
            return;
        case 0xab: // stosw
            do {
                write16(DI, AX);
                if (DF) DI -= 2;
                else DI += 2;
                if (rep) CX--;
            } while ((rep == 0xf2 || rep == 0xf3) && CX);
            return;
        case 0xac: // lodsb
            do {
                AL = read8(SI);
                if (DF) SI--;
                else SI++;
                if (rep) CX--;
            } while ((rep == 0xf2 || rep == 0xf3) && CX);
            return;
        case 0xad: // lodsw
            do {
                AX = read16(SI);
                if (DF) SI -= 2;
                else SI += 2;
                if (rep) CX--;
            } while ((rep == 0xf2 || rep == 0xf3) && CX);
            return;
        case 0xae: // scasb
            do {
                val = int8_t(AL) - int8_t(src = read8(DI));
                setf8(val, AL < src);
                if (DF) DI--;
                else DI++;
                if (rep) CX--;
            } while (((rep == 0xf2 && !ZF) || (rep == 0xf3 && ZF)) && CX);
            return;
        case 0xaf: // scasw
            do {
                val = int16_t(AX) - int16_t(src = read16(DI));
                setf16(val, AX < src);
                if (DF) DI -= 2;
                else DI += 2;
                if (rep) CX--;
            } while (((rep == 0xf2 && !ZF) || (rep == 0xf3 && ZF)) && CX);
            return;
        case 0xb0: // mov reg8, imm8
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xb7:
            *r8[opr1v] = opr2v;
            return;
        case 0xb8: // mov reg16, imm16
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xbe:
        case 0xbf:
            r[opr1v] = opr2v;
            return;
        case 0xc2: // ret imm16
            ip = read16(SP);
            SP += 2 + opr1v;
            return;
        case 0xc3: // ret
            ip = read16(SP);
            SP += 2;
            return;
        case 0xc6: // mov r/m, imm8
            set8(opr1, opr2v);
            return;
        case 0xc7: // mov r/m, imm16
            set16(opr1, opr2v);
            return;
        case 0xcd: // int imm8
            break;
        case 0xd0: // byte r/m, 1
            src = get8(opr1);
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // rol
                    CF = src & 0x80;
                    set8(opr1, (src << 1) | CF);
                    OF = CF ^ bool(src & 0x40);
                    return;
                case 1: // ror
                    CF = src & 1;
                    set8(opr1, (src >> 1) | (CF ? 0x80 : 0));
                    OF = CF ^ bool(src & 0x80);
                    return;
                case 2: // rcl
                    set8(opr1, (src << 1) | CF);
                    CF = src & 0x80;
                    OF = CF ^ bool(src & 0x40);
                    return;
                case 3: // rcr
                    set8(opr1, (src >> 1) | (CF ? 0x80 : 0));
                    OF = CF ^ bool(src & 0x80);
                    CF = src & 1;
                    return;
                case 4: // shl/sal
                    set8(opr1, setf8(int8_t(src) << 1, src & 0x80));
                    return;
                case 5: // shr
                    set8(opr1, setf8(int8_t(src >> 1), src & 1));
                    OF = src & 0x80;
                    return;
                case 7: // sar
                    set8(opr1, setf8(int8_t(src) >> 1, src & 1));
                    OF = false;
                    return;
            }
            break;
        case 0xd1: // r/m, 1
            src = get16(opr1);
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // rol
                    CF = src & 0x8000;
                    set16(opr1, (src << 1) | CF);
                    OF = CF ^ bool(src & 0x4000);
                    return;
                case 1: // ror
                    CF = src & 1;
                    set16(opr1, (src >> 1) | (CF ? 0x8000 : 0));
                    OF = CF ^ bool(src & 0x8000);
                    return;
                case 2: // rcl
                    set16(opr1, (src << 1) | CF);
                    CF = src & 0x8000;
                    OF = CF ^ bool(src & 0x4000);
                    return;
                case 3: // rcr
                    set16(opr1, (src >> 1) | (CF ? 0x8000 : 0));
                    OF = CF ^ bool(src & 0x8000);
                    CF = src & 1;
                    return;
                case 4: // shl/sal
                    set16(opr1, setf16(int16_t(src) << 1, src & 0x8000));
                    return;
                case 5: // shr
                    set16(opr1, setf16(int16_t(src >> 1), src & 1));
                    OF = src & 0x8000;
                    return;
                case 7: // sar
                    set16(opr1, setf16(int16_t(src) >> 1, src & 1));
                    OF = false;
                    return;
            }
            break;
        case 0xd2: // byte r/m, cl
            val = get8(opr1);
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // rol
                    for (int i = 0; i < CL; i++)
                        val = (val << 1) | (CF = val & 0x80);
                    set8(opr1, val);
                    return;
                case 1: // ror
                    for (int i = 0; i < CL; i++)
                        val = (val >> 1) | ((CF = val & 1) ? 0x80 : 0);
                    set8(opr1, val);
                    return;
                case 2: // rcl
                    for (int i = 0; i < CL; i++) {
                        val = (val << 1) | CF;
                        CF = val & 0x100;
                    }
                    set8(opr1, val);
                    return;
                case 3: // rcr
                    for (int i = 0; i < CL; i++) {
                        bool f = val & 1;
                        val = (val >> 1) | (CF ? 0x80 : 0);
                        CF = f;
                    }
                    set8(opr1, val);
                    return;
                case 4: // shl/sal
                    if (CL > 0) {
                        val <<= CL;
                        set8(opr1, setf8(int8_t(val), val & 0x100));
                    }
                    return;
                case 5: // shr
                    if (CL > 0) {
                        val >>= CL - 1;
                        set8(opr1, setf8(int8_t(val >> 1), val & 1));
                    }
                    return;
                case 7: // sar
                    if (CL > 0) {
                        val = int8_t(val) >> (CL - 1);
                        set8(opr1, setf8(val >> 1, val & 1));
                    }
                    return;
            }
            break;
        case 0xd3: // r/m, cl
            val = get16(opr1);
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // rol
                    for (int i = 0; i < CL; i++) {
                        CF = val & 0x8000;
                        set16(opr1, val = (val << 1) | CF);
                    }
                    return;
                case 1: // ror
                    for (int i = 0; i < CL; i++) {
                        CF = val & 1;
                        set16(opr1, val = (val >> 1) | (CF ? 0x8000 : 0));
                    }
                    return;
                case 2: // rcl
                    for (int i = 0; i < CL; i++) {
                        set16(opr1, val = (val << 1) | CF);
                        CF = val & 0x8000;
                    }
                    return;
                case 3: // rcr
                    for (int i = 0; i < CL; i++) {
                        set16(opr1, val = (val >> 1) | (CF ? 0x8000 : 0));
                        CF = val & 1;
                    }
                    return;
                case 4: // shl/sal
                    if (CL > 0) {
                        val <<= CL;
                        set16(opr1, setf16(int16_t(val), val & 0x10000));
                    }
                    return;
                case 5: // shr
                    if (CL > 0) {
                        val >>= CL - 1;
                        set16(opr1, setf16(int16_t(val >> 1), val & 1));
                    }
                    return;
                case 7: // sar
                    if (CL > 0) {
                        val = int16_t(val) >> (CL - 1);
                        set16(opr1, setf16(val >> 1, val & 1));
                    }
                    return;
            }
            break;
        case 0xd7: // xlat
            AL = read8(BX + AL);
            return;
        case 0xe0: // loopnz/loopne
            if (--CX > 0 && !ZF) ip = opr1v;
            return;
        case 0xe1: // loopz/loope
            if (--CX > 0 && ZF) ip = opr1v;
            return;
        case 0xe2: // loop
            if (--CX > 0) ip = opr1v;
            return;
        case 0xe3: // jcxz
            if (CX == 0) ip = opr1v;
            return;
        case 0xe8: // call disp
            SP -= 2;
            write16(SP, ip);
            ip = opr1v;
            return;
        case 0xe9: // jmp disp
            ip = opr1v;
            return;
        case 0xeb: // jmp short
            ip = opr1v;
            return;
        case 0xf2: // repnz/repne
        case 0xf3: // rep/repz/repe
            if (CX) {
                ip = oldip + 1;
                run(b, 0);
            }
            return;
        case 0xf5: // cmc
            CF = !CF;
            return;
        case 0xf6:
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // test r/m, imm8
                    setf8(int8_t(get8(opr1) & opr2v), false);
                    return;
                case 2: // not byte r/m
                    set8(opr1, ~get8(opr1));
                    return;
                case 3: // neg byte r/m
                    src = get8(opr1);
                    set8(opr1, setf8(-int8_t(src), src));
                    return;
                case 4: // mul byte r/m
                    AX = AL * get8(opr1);
                    OF = CF = AH;
                    return;
                case 5: // imul byte r/m
                    AX = int8_t(AL) * int8_t(get8(opr1));
                    OF = CF = AH;
                    return;
                case 6: // div byte r/m
                    dst = AX;
                    src = get8(opr1);
                    AL = dst / src;
                    AH = dst % src;
                    return;
                case 7:
                { // idiv byte r/m
                    val = int16_t(AX);
                    int16_t y = int8_t(get8(opr1));
                    AL = val / y;
                    AH = val % y;
                    return;
                }
            }
            break;
        case 0xf7:
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // test r/m, imm16
                    setf16(int16_t(get16(opr1) & opr2v), false);
                    return;
                case 2: // not r/m
                    set16(opr1, ~get16(opr1));
                    return;
                case 3: // neg r/m
                    src = get16(opr1);
                    set16(opr1, setf16(-int16_t(src), src));
                    return;
                case 4:
                { // mul r/m
                    uint32_t v = AX * get16(opr1);
                    DX = v >> 16;
                    AX = v;
                    OF = CF = DX;
                    return;
                }
                case 5: // imul r/m
                    val = int16_t(AX) * int16_t(get16(opr1));
                    DX = val >> 16;
                    AX = val;
                    OF = CF = DX;
                    return;
                case 6:
                { // div r/m
                    uint32_t x = (DX << 16) | AX;
                    src = get16(opr1);
                    AX = x / src;
                    DX = x % src;
                    return;
                }
                case 7:
                { // idiv r/m
                    int32_t x = (DX << 16) | AX;
                    int32_t y = int16_t(get16(opr1));
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
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // inc
                    set8(opr1, setf8(int8_t(get8(opr1)) + 1, CF));
                    return;
                case 1: // dec
                    set8(opr1, setf8(int8_t(get8(opr1)) - 1, CF));
                    return;
            }
            break;
        case 0xff: // r/m
            switch ((mem[oldip + 1] >> 3) & 7) {
                case 0: // inc
                    set16(opr1, setf16(int16_t(get16(opr1)) + 1, CF));
                    return;
                case 1: // dec
                    set16(opr1, setf16(int16_t(get16(opr1)) - 1, CF));
                    return;
                case 2: // call
                    SP -= 2;
                    write16(SP, ip);
                    ip = get16(opr1);
                    return;
                case 3: // callf
                    break;
                case 4: // jmp
                    ip = get16(opr1);
                    return;
                case 5: // jmpf
                    break;
                case 6: // push
                    SP -= 2;
                    write16(SP, get16(opr1));
                    return;
            }
            break;
    }
    fprintf(stderr, "not implemented\n");
}

int main() {
    for (int i = 0; i < 256; i++) {
        int n = 0;
        for (int j = 1; j < 256; j += j) {
            if (i & j) n++;
        }
        ptable[i] = (n & 1) == 0;
    }
    uint16_t tmp = 0x1234;
    uint8_t *p = (uint8_t *) r;
    if (*(uint8_t *) & tmp == 0x34) {
        for (int i = 0; i < 4; i++) {
            r8[i] = p + i * 2;
            r8[i + 4] = r8[i] + 1;
        }
    } else {
        for (int i = 0; i < 4; i++) {
            r8[i] = p + i * 2 + 1;
            r8[i + 4] = r8[i] - 1;
        }
    }
}
