// This file is in the public domain.
// derived from 7run: https://bitbucket.org/7shi/i8086tools

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

uint8_t mem[0x110000], io[0x10000];
uint16_t IP, r[8];
uint8_t *r8[8];
bool OF, DF, IF, TF, SF, ZF, AF, PF, CF;
bool ptable[256];

clock_t start;
uint32_t ticks;
FILE *fdimg;

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

struct SReg {
    uint8_t *p;
    uint16_t v;

    inline uint16_t operator *() {
        return v;
    }

    inline uint16_t operator =(uint16_t v) {
        p = &mem[v << 4];
        return this->v = v;
    }

    inline uint8_t &operator[](int addr) {
        return p[addr];
    }
} sr[4];

#define ES sr[0]
#define CS sr[1]
#define SS sr[2]
#define DS sr[3]

void
debug(FILE *f) {
    fprintf(f,
            "%04x %04x %04x %04x-%04x %04x %04x %04x %c%c%c%c%c%c%c%c%c %04x %04x %04x %04x:%04x %02x%02x%02x\n",
            AX, BX, CX, DX, SP, BP, SI, DI,
            "-O"[OF], "-D"[DF], "-I"[IF], "-T"[TF], "-S"[SF], "-Z"[ZF], "-A"[AF], "-P"[PF], "-C"[CF],
            *ES, *SS, *DS, *CS, IP, CS[IP], CS[IP + 1], CS[IP + 2]);
}

inline uint16_t read16(uint8_t *p) {
    return p[0] | (p[1] << 8);
}

inline void write16(uint8_t *p, uint16_t value) {
    p[0] = value;
    p[1] = value >> 8;
}

inline int setf8(int value) {
    int8_t v = value;
    OF = value != v;
    SF = v < 0;
    ZF = v == 0;
    PF = ptable[uint8_t(value)];
    return value;
}

inline int setf16(int value) {
    int16_t v = value;
    OF = value != v;
    SF = v < 0;
    ZF = v == 0;
    PF = ptable[uint8_t(value)];
    return value;
}

enum OperandType {
    Reg, Imm, Ptr, ModRM
};

enum Direction {
    RmReg, RegRm
};

struct Operand {
    int type;
    bool w;
    int v;
    SReg *seg;

    Operand() {
    }

    Operand(int type, bool w, int v, SReg *seg) {
        set(type, w, v, seg);
    }

    inline void set(int type, bool w, int v, SReg *seg) {
        this->type = type;
        this->w = w;
        switch (type) {
            case Ptr:
                this->seg = seg ? seg : &DS;
                this->v = uint16_t(v);
                break;
            case ModRM + 0:
                this->seg = seg ? seg : &DS;
                this->v = uint16_t(BX + SI + v);
                break;
            case ModRM + 1:
                this->seg = seg ? seg : &DS;
                this->v = uint16_t(BX + DI + v);
                break;
            case ModRM + 2:
                this->seg = seg ? seg : &SS;
                this->v = uint16_t(BP + SI + v);
                break;
            case ModRM + 3:
                this->seg = seg ? seg : &SS;
                this->v = uint16_t(BP + DI + v);
                break;
            case ModRM + 4:
                this->seg = seg ? seg : &DS;
                this->v = uint16_t(SI + v);
                break;
            case ModRM + 5:
                this->seg = seg ? seg : &DS;
                this->v = uint16_t(DI + v);
                break;
            case ModRM + 6:
                this->seg = seg ? seg : &SS;
                this->v = uint16_t(BP + v);
                break;
            case ModRM + 7:
                this->seg = seg ? seg : &DS;
                this->v = uint16_t(BX + v);
                break;
            default:
                this->seg = NULL;
                this->v = v;
                break;
        }
    }

    inline size_t modrm(uint8_t *p, bool w, SReg *seg) {
        uint8_t b = p[1], mod = b >> 6, rm = b & 7;
        switch (mod) {
            case 0:
                if (rm == 6) {
                    set(Ptr, w, read16(p + 2), seg);
                    return 4;
                }
                set(ModRM + rm, w, 0, seg);
                return 2;
            case 1:
                set(ModRM + rm, w, (int8_t) p[2], seg);
                return 3;
            case 2:
                set(ModRM + rm, w, (int16_t) read16(p + 2), seg);
                return 4;
        }
        set(Reg, w, rm, NULL);
        return 2;
    }

    inline size_t regrm(Operand *opr, uint8_t *p, bool dir, bool w, SReg *seg) {
        if (dir) {
            set(Reg, w, (p[1] >> 3) & 7, NULL);
            return opr->modrm(p, w, seg);
        }
        opr->set(Reg, w, (p[1] >> 3) & 7, NULL);
        return modrm(p, w, seg);
    }

    inline size_t aimm(Operand *opr, bool w, uint8_t *p) {
        set(Reg, w, 0, NULL);
        opr->set(Imm, w, w ? read16(p) : *p, NULL);
        return 2 + w;
    }

    inline size_t getopr(Operand *opr, uint8_t b, uint8_t *p, SReg *seg) {
        if (b & 4) {
            return aimm(opr, b & 1, p + 1);
        }
        return regrm(opr, p, b & 2, b & 1, seg);
    }

    inline uint8_t * ptr() const {
        return seg ? &(*seg)[v] : NULL;
    }

    inline int u() const {
        if (type == Reg) return w ? r[v] : *r8[v];
        uint8_t *p = ptr();
        if (!p) return v;
        return w ? read16(p) : *p;
    }

    inline int operator *() const {
        int ret = u();
        return w ? int16_t(ret) : int8_t(ret);
    }

    inline void operator =(int value) {
        if (type == Reg) {
            if (w) {
                r[v] = value;
            } else {
                *r8[v] = value;
            }
            return;
        }
        uint8_t *p = ptr();
        if (p) {
            if (w) {
                write16(p, value);
            } else {
                *p = value;
            }
        }
    }

    inline uint16_t loadf(SReg *seg) {
        uint8_t *p = ptr();
        if (!p) return u();
        *seg = read16(p + 2);
        return read16(p);
    }

    inline bool operator>(int val) {
        return u() > (w ? uint16_t(val) : uint8_t(val));
    }

    inline bool operator<(int val) {
        return u() < (w ? uint16_t(val) : uint8_t(val));
    }

    inline int setf(int value) {
        return w ? setf16(value) : setf8(value);
    }
};

inline Operand ptr(uint16_t a, bool w, SReg *seg) {
    return Operand(Ptr, w, a, seg);
}

inline uint16_t getf() {
    return 0xf002 | (OF << 11) | (DF << 10) | (IF << 9) | (TF << 8)
            | (SF << 7) | (ZF << 6) | (AF << 4) | (PF << 2) | CF;
}

inline void setf(uint16_t flags) {
    OF = flags & 2048;
    DF = flags & 1024;
    IF = flags & 512;
    TF = flags & 256;
    SF = flags & 128;
    ZF = flags & 64;
    AF = flags & 16;
    PF = flags & 4;
    CF = flags & 1;
}

inline void jumpif(int8_t offset, bool c) {
    if (c) {
        IP += 2 + offset;
    } else {
        IP += 2;
    }
}

inline void push(uint16_t val) {
    write16(&SS[SP -= 2], val);
}

inline uint16_t pop() {
    uint16_t val = read16(&SS[SP]);
    SP += 2;
    return val;
}

inline int bcd(int v) {
    return ((v / 10) << 4) | (v % 10);
}

void bios(int n) {
    switch (n) {
        case 0x08: // timer
            // TODO
            break;
        case 0x10: // video
            if (AH == 0x0e) {
                write(1, &AL, 1);
                return;
            }
            break;
        case 0x11: // get equipment list
            AX = 0;
            return;
        case 0x12: // get memory size
            AX = 640;
            return;
        case 0x13: // disk
            if (DL != 0) { // not first fdd
                CF = 1; // error
                AH = 1; // invalid
                return;
            }
            switch (AH) {
                case 0x00: // reset disk system
                    CF = 0;
                    return;
                case 0x02: // read sectors
                case 0x03: // write sectors
                {
                    if (DH > 1 || CH > 79 || CL < 1 || CL > 18) {
                        CF = 1; // error
                        AH = 4; // sector
                        return;
                    }
                    int sect = 36 * CH + 18 * DH + CL - 1;
                    if (fseek(fdimg, sect << 9, SEEK_SET) < 0) {
                        CF = 1; // error
                        AH = 4; // sector
                        return;
                    }
                    if (AH == 2) {
                        fread(&ES[BX], 512, AL, fdimg);
                    } else {
                        fwrite(&ES[BX], 512, AL, fdimg);
                    }
                    CF = 0;
                    return;
                }
                case 0x08: // get drive params
                    AX = 0;
                    BL = 4; // 1440KB
                    CH = 79;
                    CL = 18;
                    DH = 1;
                    DL = 1;
                    ES = DI = 0;
                    CF = 0;
                    return;
                case 0x15: // get disk type
                    AH = 1;
                    CF = 0;
                    return;
            }
            CF = 1;
            return;
        case 0x16: // keyboard
            // TODO
            switch (AH) {
                case 0x00: // get keystroke
                case 0x01: // check for keystroke
                    break;
            }
            break;
        case 0x1a: // time
            switch (AH) {
                case 0x00: // get system time
                {
                    clock_t d = clock() - start;
                    uint32_t t = ticks + (d << 12) / (CLOCKS_PER_SEC * 225);
                    if ((AL = t > 0x240000)) t %= 0x240000;
                    CX = t >> 16;
                    DX = t;
                    return;
                }
                case 0x01: // set system time
                    start = clock();
                    ticks = (CX << 16) | DX;
                    return;
                case 0x02: // get RTC time
                case 0x04: // get RTC date
                {
                    time_t t = time(NULL);
                    tm *lt = localtime(&t);
                    if (AH == 2) {
                        CH = bcd(lt->tm_hour);
                        CL = bcd(lt->tm_min);
                        DH = bcd(lt->tm_sec);
                        DL = lt->tm_isdst == 1 ? 1 : 0;
                    } else {
                        CH = bcd(lt->tm_year / 100 + 19);
                        CL = bcd(lt->tm_year % 100);
                        DH = bcd(lt->tm_mon + 1);
                        DL = bcd(lt->tm_mday);
                    }
                    CF = 0;
                    return;
                }
            }
            break;
    }
    fprintf(stderr, "%04x:%04x int %02x,%02x not implemented\n", *CS, IP, n, AH);
    exit(1);
}

void intr(int n) {
    uint8_t *v = &mem[n << 2];
    uint16_t cs = read16(v + 2), ip = read16(v);
    if (!cs && !ip) return bios(n);
    push(getf());
    IF = TF = 0;
    push(*CS);
    push(IP);
    CS = cs;
    IP = ip;
}

inline void shift(Operand *opr, int c, uint8_t *p) {
    int val = opr->u(), m = opr->w ? 0x8000 : 0x80;
    switch ((p[1] >> 3) & 7) {
        case 0: // rol
            for (int i = 0; i < c; ++i)
                val = (val << 1) | (CF = val & m);
            OF = CF ^ bool(val & m);
            *opr = val;
            break;
        case 1: // ror
            for (int i = 0; i < c; ++i)
                val = (val >> 1) | ((CF = val & 1) ? m : 0);
            OF = CF ^ bool(val & (m >> 1));
            *opr = val;
            break;
        case 2: // rcl
            for (int i = 0; i < c; ++i) {
                val = (val << 1) | CF;
                CF = val & (m << 1);
            }
            OF = CF ^ bool(val & m);
            *opr = val;
            break;
        case 3: // rcr
            for (int i = 0; i < c; ++i) {
                bool f1 = val & 1, f2 = val & m;
                val = (val >> 1) | (CF ? m : 0);
                OF = CF ^ f2;
                CF = f1;
            }
            *opr = val;
            break;
        case 4: // shl/sal
            if (c > 0) {
                val <<= c;
                CF = val & (m << 1);
                OF = CF != bool(val & m);
                *opr = opr->setf(val);
            }
            break;
        case 5: // shr
            if (c > 0) {
                val >>= c - 1;
                CF = val & 1;
                OF = val & m;
                *opr = opr->setf(val >> 1);
            }
            break;
        case 7: // sar
            if (c > 0) {
                val = int8_t(val) >> (c - 1);
                CF = val & 1;
                OF = false;
                *opr = opr->setf(val >> 1);
            }
            break;
    }
}

void step(uint8_t rep, SReg *seg) {
    Operand opr1, opr2;
    uint8_t *p = &CS[IP], b = *p;
    int dst, src, val;
    switch (b) {
        case 0x00: // add r/m, reg8
        case 0x01: // add r/m, reg16
        case 0x02: // add reg8, r/m
        case 0x03: // add reg16, r/m
        case 0x04: // add al, imm8
        case 0x05: // add ax, imm16
            IP += opr1.getopr(&opr2, b, p, seg);
            val = *opr1 + *opr2;
            CF = opr1 > val;
            opr1 = opr1.setf(val);
            return;
        case 0x06: // push es
            ++IP;
            return push(*ES);
        case 0x07: // pop es
            ++IP;
            ES = pop();
            return;
        case 0x08: // or r/m, reg8
        case 0x09: // or r/m, reg16
        case 0x0a: // or reg8, r/m
        case 0x0b: // or reg16, r/m
        case 0x0c: // or al, imm8
        case 0x0d: // or ax, imm16
            IP += opr1.getopr(&opr2, b, p, seg);
            CF = false;
            opr1 = opr1.setf(*opr1 | *opr2);
            return;
        case 0x0e: // push cs
            ++IP;
            return push(*CS);
        case 0x10: // adc r/m, reg8
        case 0x11: // adc r/m, reg16
        case 0x12: // adc reg8, r/m
        case 0x13: // adc reg16, r/m
        case 0x14: // adc al, imm8
        case 0x15: // adc ax, imm16
            IP += opr1.getopr(&opr2, b, p, seg);
            val = *opr1 + (src = *opr2 + CF);
            CF = opr1 > val || (CF && !src);
            opr1 = opr1.setf(val);
            return;
        case 0x16: // push ss
            ++IP;
            return push(*SS);
        case 0x17: // pop ss
            ++IP;
            SS = pop();
            return;
        case 0x18: // sbb r/m, reg8
        case 0x19: // sbb r/m, reg16
        case 0x1a: // sbb reg8, r/m
        case 0x1b: // sbb reg16, r/m
        case 0x1c: // sbb al, imm8
        case 0x1d: // sbb ax, imm16
            IP += opr1.getopr(&opr2, b, p, seg);
            val = *opr1 - (src = *opr2 + CF);
            CF = opr1 < src || (CF && !src);
            opr1 = opr1.setf(val);
            return;
        case 0x1e: // push ds
            ++IP;
            return push(*DS);
        case 0x1f: // pop ds
            ++IP;
            DS = pop();
            return;
        case 0x20: // and r/m, reg8
        case 0x21: // and r/m, reg16
        case 0x22: // and reg8, r/m
        case 0x23: // and reg16, r/m
        case 0x24: // and al, imm8
        case 0x25: // and ax, imm16
            IP += opr1.getopr(&opr2, b, p, seg);
            CF = false;
            opr1 = opr1.setf(*opr1 & *opr2);
            return;
        case 0x26: // es:
            ++IP;
            return step(rep, &ES);
        case 0x28: // sub r/m, reg8
        case 0x29: // sub r/m, reg16
        case 0x2a: // sub reg8, r/m
        case 0x2b: // sub reg16, r/m
        case 0x2c: // sub al, imm8
        case 0x2d: // sub ax, imm16
            IP += opr1.getopr(&opr2, b, p, seg);
            val = *opr1 - (src = *opr2);
            CF = opr1 < src;
            opr1 = opr1.setf(val);
            return;
        case 0x2e: // cs:
            ++IP;
            return step(rep, &CS);
        case 0x30: // xor r/m, reg8
        case 0x31: // xor r/m, reg16
        case 0x32: // xor reg8, r/m
        case 0x33: // xor reg16, r/m
        case 0x34: // xor al, imm8
        case 0x35: // xor ax, imm16
            IP += opr1.getopr(&opr2, b, p, seg);
            CF = false;
            opr1 = opr1.setf(*opr1 ^ *opr2);
            return;
        case 0x36: // ss:
            ++IP;
            return step(rep, &SS);
        case 0x38: // cmp r/m, reg8
        case 0x39: // cmp r/m, reg16
        case 0x3a: // cmp reg8, r/m
        case 0x3b: // cmp reg16, r/m
        case 0x3c: // cmp al, imm8
        case 0x3d: // cmp ax, imm16
            IP += opr1.getopr(&opr2, b, p, seg);
            val = *opr1 - (src = *opr2);
            CF = opr1 < src;
            opr1.setf(val);
            return;
        case 0x3e: // ds:
            ++IP;
            return step(rep, &DS);
        case 0x40: // inc reg16
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
            ++IP;
            r[b & 7] = setf16(int16_t(r[b & 7]) + 1);
            return;
        case 0x48: // dec reg16
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f:
            ++IP;
            r[b & 7] = setf16(int16_t(r[b & 7]) - 1);
            return;
        case 0x50: // push reg16
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
            ++IP;
            push(r[b & 7]);
            return;
        case 0x58: // pop reg16
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5e:
        case 0x5f:
            ++IP;
            r[b & 7] = pop();
            return;
        case 0x70: // jo
            return jumpif(p[1], OF);
        case 0x71: // jno
            return jumpif(p[1], !OF);
        case 0x72: // jb/jnae
            return jumpif(p[1], CF);
        case 0x73: // jnb/jae
            return jumpif(p[1], !CF);
        case 0x74: // je/jz
            return jumpif(p[1], ZF);
        case 0x75: // jne/jnz
            return jumpif(p[1], !ZF);
        case 0x76: // jbe/jna
            return jumpif(p[1], CF || ZF);
        case 0x77: // jnbe/ja
            return jumpif(p[1], !(CF || ZF));
        case 0x78: // js
            return jumpif(p[1], SF);
        case 0x79: // jns
            return jumpif(p[1], !SF);
        case 0x7a: // jp
            return jumpif(p[1], PF);
        case 0x7b: // jnp
            return jumpif(p[1], !PF);
        case 0x7c: // jl/jnge
            return jumpif(p[1], SF != OF);
        case 0x7d: // jnl/jge
            return jumpif(p[1], SF == OF);
        case 0x7e: // jle/jng
            return jumpif(p[1], ZF || SF != OF);
        case 0x7f: // jnle/jg
            return jumpif(p[1], !(ZF || SF != OF));
        case 0x80: // r/m, imm8
        case 0x81: // r/m, imm16
        case 0x83: // r/m, imm8 (signed extend to 16bit)
            IP += opr1.modrm(p, b & 1, seg);
            if (b == 0x83) {
                opr2.set(Ptr, 0, IP, &CS);
            } else {
                opr2.set(Ptr, opr1.w, IP, &CS);
            }
            IP += 1 + opr2.w;
            switch ((p[1] >> 3) & 7) {
                case 0: // add
                    val = *opr1 + *opr2;
                    CF = opr1 > val;
                    opr1 = opr1.setf(val);
                    return;
                case 1: // or
                    CF = false;
                    opr1 = opr1.setf(*opr1 | *opr2);
                    return;
                case 2: // adc
                    val = *opr1 + (src = *opr2 + CF);
                    CF = opr1 > val || (CF && !src);
                    opr1 = opr1.setf(val);
                    return;
                case 3: // sbb
                    val = *opr1 - (src = *opr2 + CF);
                    CF = opr1 < src || (CF && !src);
                    opr1 = opr1.setf(val);
                    return;
                case 4: // and
                    CF = false;
                    opr1 = opr1.setf(*opr1 & *opr2);
                    return;
                case 5: // sub
                    val = *opr1 - (src = *opr2);
                    CF = opr1 < src;
                    opr1 = opr1.setf(val);
                    return;
                case 6: // xor
                    CF = false;
                    opr1 = opr1.setf(*opr1 ^ *opr2);
                    return;
                case 7: // cmp
                    val = *opr1 - (src = *opr2);
                    CF = opr1 < src;
                    opr1.setf(val);
                    return;
            }
            break;
        case 0x84: // test r/m, reg8
        case 0x85: // test r/m, reg16
            IP += opr1.regrm(&opr2, p, RmReg, b & 1, seg);
            CF = false;
            opr1.setf(*opr1 & *opr2);
            return;
        case 0x86: // xchg r/m, reg8
        case 0x87: // xchg r/m, reg16
            IP += opr1.regrm(&opr2, p, RmReg, b & 1, seg);
            val = *opr2;
            opr2 = *opr1;
            opr1 = val;
            return;
        case 0x88: // mov r/m, reg8
        case 0x89: // mov r/m, reg16
        case 0x8a: // mov reg8, r/m
        case 0x8b: // mov reg16, r/m
            IP += opr1.regrm(&opr2, p, b & 2, b & 1, seg);
            opr1 = *opr2;
            return;
        case 0x8c: // mov r/m, sreg
            IP += opr1.modrm(p, 1, seg);
            opr1 = *sr[(p[1] >> 3) & 3];
            return;
        case 0x8d: // lea reg16, r/m
            IP += opr1.regrm(&opr2, p, RegRm, 1, seg);
            opr1 = opr2.v;
            return;
        case 0x8e: // mov sreg, r/m
            IP += opr2.modrm(p, 1, seg);
            sr[(p[1] >> 3) & 3] = *opr2;
            return;
        case 0x8f: // pop r/m
            IP += opr1.modrm(p, 1, seg);
            opr1 = pop();
            return;
        case 0x90: // nop
            ++IP;
            return;
        case 0x91: // xchg reg, ax
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x96:
        case 0x97:
            ++IP;
            val = AX;
            AX = r[b & 7];
            r[b & 7] = val;
            return;
        case 0x98: // cbw
            ++IP;
            AX = (int16_t) (int8_t) AL;
            return;
        case 0x99: // cwd
            ++IP;
            DX = int16_t(AX) < 0 ? 0xffff : 0;
            return;
        case 0x9a: // callf
            IP += 5;
            push(*CS);
            push(IP);
            CS = read16(p + 3);
            IP = read16(p + 1);
            return;
        case 0x9c: // pushf
            ++IP;
            return push(getf());
        case 0x9d: // popf
            ++IP;
            return setf(pop());
        case 0x9e: // sahf
            ++IP;
            return setf((getf() & 0xff00) | AH);
        case 0x9f: // lahf
            ++IP;
            AH = getf();
            return;
        case 0xa0: // mov al, [addr]
            IP += 3;
            AL = *ptr(read16(p + 1), 0, seg);
            return;
        case 0xa1: // mov ax, [addr]
            IP += 3;
            AX = *ptr(read16(p + 1), 1, seg);
            return;
        case 0xa2: // mov [addr], al
            IP += 3;
            ptr(read16(p + 1), 0, seg) = AL;
            return;
        case 0xa3: // mov [addr], ax
            IP += 3;
            ptr(read16(p + 1), 1, seg) = AX;
            return;
        case 0xa4: // movsb
            ++IP;
            if (!seg) seg = &DS;
            do {
                ES[DI] = (*seg)[SI];
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
            ++IP;
            if (!seg) seg = &DS;
            do {
                write16(&ES[DI], read16(&(*seg)[SI]));
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
            ++IP;
            if (!seg) seg = &DS;
            do {
                val = int8_t(dst = (*seg)[SI]) - int8_t(src = ES[DI]);
                CF = dst < src;
                setf8(val);
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
            ++IP;
            if (!seg) seg = &DS;
            do {
                val = int16_t(dst = read16(&(*seg)[SI])) - int16_t(src = read16(&ES[DI]));
                CF = dst < src;
                setf16(val);
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
        case 0xa9: // test ax, imm16
            IP += opr1.aimm(&opr2, b & 1, p + 1);
            CF = false;
            opr1.setf(*opr1 & *opr2);
            return;
        case 0xaa: // stosb
            ++IP;
            do {
                ES[DI] = AL;
                if (DF) DI--;
                else DI++;
                if (rep) CX--;
            } while ((rep == 0xf2 || rep == 0xf3) && CX);
            return;
        case 0xab: // stosw
            ++IP;
            do {
                write16(&ES[DI], AX);
                if (DF) DI -= 2;
                else DI += 2;
                if (rep) CX--;
            } while ((rep == 0xf2 || rep == 0xf3) && CX);
            return;
        case 0xac: // lodsb
            ++IP;
            if (!seg) seg = &DS;
            do {
                AL = (*seg)[SI];
                if (DF) SI--;
                else SI++;
                if (rep) CX--;
            } while ((rep == 0xf2 || rep == 0xf3) && CX);
            return;
        case 0xad: // lodsw
            ++IP;
            if (!seg) seg = &DS;
            do {
                AX = read16(&(*seg)[SI]);
                if (DF) SI -= 2;
                else SI += 2;
                if (rep) CX--;
            } while ((rep == 0xf2 || rep == 0xf3) && CX);
            return;
        case 0xae: // scasb
            ++IP;
            do {
                val = int8_t(AL) - int8_t(src = ES[DI]);
                CF = AL < src;
                setf8(val);
                if (DF) DI--;
                else DI++;
                if (rep) CX--;
            } while (((rep == 0xf2 && !ZF) || (rep == 0xf3 && ZF)) && CX);
            return;
        case 0xaf: // scasw
            ++IP;
            do {
                val = int16_t(AX) - int16_t(src = read16(&ES[DI]));
                CF = AX < src;
                setf16(val);
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
            IP += 2;
            *r8[b & 7] = p[1];
            return;
        case 0xb8: // mov reg16, imm16
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xbe:
        case 0xbf:
            IP += 3;
            r[b & 7] = read16(p + 1);
            return;
        case 0xc0: // byte r/m, imm8 (80186)
        case 0xc1: // r/m, imm8 (80186)
            IP += opr1.modrm(p, b & 1, seg) + 1;
            return shift(&opr1, CS[IP - 1], p);
        case 0xc2: // ret imm16
            IP = pop();
            SP += read16(p + 1);
            return;
        case 0xc3: // ret
            IP = pop();
            return;
        case 0xc4: // les reg16, r/m
            IP += opr1.regrm(&opr2, p, RegRm, 1, seg);
            opr1 = opr2.loadf(&ES);
            return;
        case 0xc5: // lds reg16, r/m
            IP += opr1.regrm(&opr2, p, RegRm, 1, seg);
            opr1 = opr2.loadf(&DS);
            return;
        case 0xc6: // mov r/m, imm8
            IP += opr1.modrm(p, 0, seg) + 1;
            opr1 = CS[IP - 1];
            return;
        case 0xc7: // mov r/m, imm16
            IP += opr1.modrm(p, 1, seg) + 2;
            opr1 = read16(&CS[IP] - 2);
            return;
        case 0xca: // retf imm16
            IP = pop();
            CS = pop();
            SP += read16(p + 1);
            return;
        case 0xcb: // retf
            IP = pop();
            CS = pop();
            return;
        case 0xcc: // int3
            ++IP;
            return intr(3);
        case 0xcd: // int imm8
            IP += 2;
            return intr(p[1]);
        case 0xce: // into
            ++IP;
            if (OF) intr(4);
            return;
        case 0xcf: // iret
            IP = pop();
            CS = pop();
            setf(pop());
            return;
        case 0xd0: // byte r/m, 1
        case 0xd1: // r/m, 1
            IP += opr1.modrm(p, b & 1, seg);
            return shift(&opr1, 1, p);
        case 0xd2: // byte r/m, cl
        case 0xd3: // r/m, cl
            IP += opr1.modrm(p, b & 1, seg);
            return shift(&opr1, CL, p);
        case 0xd7: // xlat
            ++IP;
            AL = (seg ? *seg : DS)[BX + AL];
            return;
        case 0xd8: // esc (8087 FPU)
        case 0xd9:
        case 0xda:
        case 0xdb:
        case 0xdc:
        case 0xdd:
        case 0xde:
        case 0xdf:
            IP += 2;
            return;
        case 0xe0: // loopnz/loopne
            return jumpif(p[1], --CX > 0 && !ZF);
        case 0xe1: // loopz/loope
            return jumpif(p[1], --CX > 0 && ZF);
        case 0xe2: // loop
            return jumpif(p[1], --CX > 0);
        case 0xe3: // jcxz
            return jumpif(p[1], CX == 0);
        case 0xe4: // in al, imm8
            IP += 2;
            AL = io[p[1]];
            return;
        case 0xe5: // in ax, imm8
            IP += 2;
            AX = read16(io + p[1]);
            return;
        case 0xe6: // out imm8, al
            IP += 2;
            io[p[1]] = AL;
            return;
        case 0xe7: // out imm8, ax
            IP += 2;
            write16(io + p[1], AX);
            return;
        case 0xe8: // call disp
            push(IP + 3);
            IP += 3 + read16(p + 1);
            return;
        case 0xe9: // jmp disp
            IP += 3 + read16(p + 1);
            return;
        case 0xea: // jmpf
            CS = read16(p + 3);
            IP = read16(p + 1);
            return;
        case 0xeb: // jmp short
            IP += 2 + (int8_t) p[1];
            return;
        case 0xec: // in al, dx
            ++IP;
            AL = io[DX];
            return;
        case 0xed: // in ax, dx
            ++IP;
            AX = read16(io + DX);
            return;
        case 0xee: // out dx, al
            ++IP;
            io[DX] = AL;
            return;
        case 0xef: // out dx, ax
            ++IP;
            write16(io + DX, AX);
            return;
        case 0xf2: // repnz/repne
        case 0xf3: // rep/repz/repe
            ++IP;
            if (CX) step(b, seg);
            return;
        case 0xf4: // hlt
            ++IP;
            return;
        case 0xf5: // cmc
            ++IP;
            CF = !CF;
            return;
        case 0xf6:
            IP += opr1.modrm(p, 0, seg);
            switch ((p[1] >> 3) & 7) {
                case 0: // test r/m, imm8
                    CF = false;
                    setf8(*opr1 & int8_t(CS[IP++]));
                    return;
                case 2: // not byte r/m
                    opr1 = ~*opr1;
                    return;
                case 3: // neg byte r/m
                    src = *opr1;
                    CF = src;
                    opr1 = setf8(-src);
                    return;
                case 4: // mul byte r/m
                    AX = AL * uint8_t(*opr1);
                    OF = CF = AH;
                    return;
                case 5: // imul byte r/m
                    AX = int8_t(AL) * *opr1;
                    OF = CF = AH;
                    return;
                case 6: // div byte r/m
                    dst = AX;
                    src = uint8_t(*opr1);
                    AL = dst / src;
                    AH = dst % src;
                    return;
                case 7:
                { // idiv byte r/m
                    val = int16_t(AX);
                    int16_t y = *opr1;
                    AL = val / y;
                    AH = val % y;
                    return;
                }
            }
            break;
        case 0xf7:
            IP += opr1.modrm(p, 1, seg);
            switch ((p[1] >> 3) & 7) {
                case 0: // test r/m, imm16
                    CF = false;
                    setf16(*opr1 & int16_t(read16(&CS[IP += 2] - 2)));
                    return;
                case 2: // not r/m
                    opr1 = ~*opr1;
                    return;
                case 3: // neg r/m
                    src = *opr1;
                    CF = src;
                    opr1 = setf16(-int16_t(src));
                    return;
                case 4:
                { // mul r/m
                    uint32_t v = AX * uint16_t(*opr1);
                    DX = v >> 16;
                    AX = v;
                    OF = CF = DX;
                    return;
                }
                case 5: // imul r/m
                    val = int16_t(AX) * *opr1;
                    DX = val >> 16;
                    AX = val;
                    OF = CF = DX;
                    return;
                case 6:
                { // div r/m
                    uint32_t x = (DX << 16) | AX;
                    src = uint16_t(*opr1);
                    AX = x / src;
                    DX = x % src;
                    return;
                }
                case 7:
                { // idiv r/m
                    int32_t x = (DX << 16) | AX;
                    int32_t y = *opr1;
                    AX = x / y;
                    DX = x % y;
                    return;
                }
            }
            break;
        case 0xf8: // clc
            ++IP;
            CF = false;
            return;
        case 0xf9: // stc
            ++IP;
            CF = true;
            return;
        case 0xfa: // cli
            ++IP;
            IF = false;
            return;
        case 0xfb: // sti
            ++IP;
            IF = true;
            return;
        case 0xfc: // cld
            ++IP;
            DF = false;
            return;
        case 0xfd: // std
            ++IP;
            DF = true;
            return;
        case 0xfe: // byte r/m
            IP += opr1.modrm(p, 0, seg);
            switch ((p[1] >> 3) & 7) {
                case 0: // inc
                    opr1 = setf8(*opr1 + 1);
                    return;
                case 1: // dec
                    opr1 = setf8(*opr1 - 1);
                    return;
            }
            break;
        case 0xff: // r/m
            IP += opr1.modrm(p, 1, seg);
            switch ((p[1] >> 3) & 7) {
                case 0: // inc
                    opr1 = setf16(*opr1 + 1);
                    return;
                case 1: // dec
                    opr1 = setf16(*opr1 - 1);
                    return;
                case 2: // call
                    push(IP);
                    IP = *opr1;
                    return;
                case 3: // callf
                    push(*CS);
                    push(IP);
                    IP = opr1.loadf(&CS);
                    return;
                case 4: // jmp
                    IP = *opr1;
                    return;
                case 5: // jmpf
                    IP = opr1.loadf(&CS);
                    return;
                case 6: // push
                    push(*opr1);
                    return;
            }
            break;
#if 0
        case 0x27: // daa
        case 0x2f: // das
        case 0x37: // aaa
        case 0x3f: // aas
        case 0x9b: // wait
        case 0xd4: // aam
        case 0xd5: // aad
#endif
    }
    fprintf(stderr, "%04x:%04x %02x not implemented\n", *CS, p - &CS[0], b);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s fdimage\n", argv[0]);
        return 1;
    }
    if (!(fdimg = fopen(argv[1], "rb"))) {
        fprintf(stderr, "can not open: %s\n", argv[1]);
        return 1;
    }
    
    for (int i = 0; i < 256; ++i) {
        int n = 0;
        for (int j = 1; j < 256; j += j) {
            if (i & j) n++;
        }
        ptable[i] = (n & 1) == 0;
    }
    
    uint16_t tmp = 0x1234;
    uint8_t *p = (uint8_t *) r;
    if (*(uint8_t *) & tmp == 0x34) {
        for (int i = 0; i < 4; ++i) {
            r8[i] = p + i * 2;
            r8[i + 4] = r8[i] + 1;
        }
    } else {
        for (int i = 0; i < 4; ++i) {
            r8[i] = p + i * 2 + 1;
            r8[i + 4] = r8[i] - 1;
        }
    }
    
    ES = CS = SS = DS = 0;
    IP = 0x7c00;
    fread(&mem[IP], 1, 512, fdimg); // read MBR
    start = clock();
    while (IP || *CS) {
        step(0, NULL);
    }
}
