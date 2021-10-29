/***************************************************************/
/**                         i8080 CPU                         **/
/***************************************************************/

typedef struct i8080 i8080;

typedef uint8_t  (*mread)(uint16_t addr);
typedef uint16_t (*mread16)(uint16_t addr);
typedef void     (*mwrite)(uint16_t addr, uint8_t val);
typedef void     (*out)(uint8_t port, uint8_t b);
typedef uint8_t  (*in)(uint8_t port);

struct
i8080
{
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t sp;
    uint16_t pc;

    struct {
        uint8_t z:1;
        uint8_t s:1;
        uint8_t p:1;
        uint8_t cy:1;
        uint8_t ac:1;
        uint8_t pad:3;
    } cc;

    uint8_t int_enabled;

    mread   MEM_READ;
    mread16 MEM_READ16;
    mwrite  MEM_WRITE;

    out OUT_PORT;
    in  IN_PORT;
};

void
unimplemented(i8080 *)
{
#if 0
    printf("Unimplemented");
    exit(1);
#endif
}

int
parity(int x, int size)
{
    int i;
    int p;

    p = 0;
    x = (x & ((1 << size) - 1));
    for(i = 0;
        i < size;
        ++i)
    {
        if(x & 0x1) ++p;
        x = x >> 1;
    }
    return((p & 0x1) == 0);
}

void
setflagsarith(i8080 *cpu, uint16_t ans)
{
    cpu->cc.z  = ((ans & 0xff) == 0) ? 1 : 0;
    cpu->cc.s  = (ans & 0x80) ? 1 : 0;
    cpu->cc.cy = (ans > 0xff) ? 1 : 0;
    /* TODO cpu->cc.ac = ... Auxilliary carry */
    cpu->cc.p = (parity((ans & 0xff), 8)) ? 1 : 0;
}

void
setflags(i8080 *cpu, uint16_t ans)
{
    cpu->cc.z  = ((ans & 0xff) == 0) ? 1 : 0;
    cpu->cc.s  = (ans & 0x80) ? 1 : 0;
    cpu->cc.cy = (ans > 0xff) ? 1 : 0;
    cpu->cc.p = (parity((ans & 0xff), 8)) ? 1 : 0;
}


int
i8080exec(i8080 *cpu)
{
    int halt;
    uint8_t  opcode;
    uint16_t addr;
    uint8_t  tmp;
    uint8_t  tmp2;
    uint16_t tmp16;
    uint32_t tmp32;

    opcode = cpu->MEM_READ(cpu->pc);
    cpu->pc += 1;

    halt = 0;
    switch(opcode)
    {
#define arithop(ans)\
        tmp16 = (ans);\
        setflagsarith(cpu, tmp16);\
        cpu->a = (uint8_t)(tmp16 & 0xff)
#define normlop(ans, dest)\
        tmp16 = (ans);\
        setflags(cpu, tmp16);\
        dest = (uint8_t)(tmp16 & 0xff)

#define hl (uint16_t)(((uint16_t)cpu->h << 8) | ((uint16_t)cpu->l))
#define bc (uint16_t)(((uint16_t)cpu->b << 8) | ((uint16_t)cpu->c))
#define de (uint16_t)(((uint16_t)cpu->d << 8) | ((uint16_t)cpu->e))

#define call()\
        addr = cpu->MEM_READ16(cpu->pc); cpu->pc += 2;\
        cpu->MEM_WRITE(cpu->sp-1, (uint8_t)((cpu->pc >> 8) & 0xff));\
        cpu->MEM_WRITE(cpu->sp-2, (uint8_t)((cpu->pc >> 0) & 0xff));\
        cpu->sp -= 2; cpu->pc = addr
#define ret() cpu->pc = cpu->MEM_READ16(cpu->sp); cpu->sp += 2

        /* NOP */
        case 0x00: break;

        /* LXI B, d16 */
        case 0x01: { cpu->c = cpu->MEM_READ(cpu->pc++); cpu->b = cpu->MEM_READ(cpu->pc++); } break;

        /* STAX B */
        case 0x02: { cpu->MEM_WRITE(bc, cpu->a); } break;

        /* INX B */
        case 0x03: { tmp16 = bc + 1; cpu->b = (uint8_t)((tmp16 >> 8) & 0xff); cpu->c = (uint8_t)((tmp16 >> 0) & 0xff); } break;

        /* INR/DCR B */
        case 0x04: { normlop((uint16_t)cpu->b + 1, cpu->b); } break;
        case 0x05: { normlop((uint16_t)cpu->b - 1, cpu->b); } break;

        /* MVI B,d8 */
        case 0x06: { cpu->b = cpu->MEM_READ(cpu->pc++); } break;

        /* RLC */
        case 0x07:
        {
            tmp = cpu->a;
            cpu->a = ((tmp&0x80)>>7)|(tmp<<1);
            cpu->cc.cy = (tmp&0x80) ? 1 : 0;
        } break;

        case 0x08: { /* Nothing */ } break;

        /* DAD B */
        case 0x09:
        {
            tmp32 = (uint32_t)hl + (uint32_t)bc;
            cpu->h = (uint8_t)((tmp32&0xff00)>>8);
            cpu->l = (uint8_t)((tmp32&0x00ff)>>0);
            cpu->cc.cy = (tmp32 > 0xffff) ? 1 : 0;
        } break;

        /* LDAX B */
        case 0x0a: { cpu->a = cpu->MEM_READ(bc); } break;

        /* DCX B */
        case 0x0b: { tmp16 = bc - 1; cpu->b = (uint8_t)((tmp16 >> 8) & 0xff); cpu->c = (uint8_t)((tmp16 >> 0) & 0xff); } break;

        /* INR/DCR C */
        case 0x0c: { normlop((uint16_t)cpu->c + 1, cpu->c); } break;
        case 0x0d: { normlop((uint16_t)cpu->c - 1, cpu->c); } break;

        /* MVI C,d8 */
        case 0x0e: { cpu->c = cpu->MEM_READ(cpu->pc++); } break;

        /* RRC */
        case 0x0f:
        {
            tmp = cpu->a;
            cpu->a = ((tmp&1)<<7)|(tmp>>1);
            cpu->cc.cy = (tmp&1) ? 1 : 0;
        } break;

        case 0x10: { /* Nothing */ } break;

        /* LXI D,d16 */
        case 0x11: { cpu->e = cpu->MEM_READ(cpu->pc++); cpu->d = cpu->MEM_READ(cpu->pc++); } break;

        /* STAX D */
        case 0x12: { cpu->MEM_WRITE(de, cpu->a); } break;

        /* INX D */
        case 0x13: { tmp16 = de + 1; cpu->d = (uint8_t)((tmp16 >> 8) & 0xff); cpu->e = (uint8_t)((tmp16 >> 0) & 0xff); } break;

        /* INR/DCR D */
        case 0x14: { normlop((uint16_t)cpu->d + 1, cpu->d); } break;
        case 0x15: { normlop((uint16_t)cpu->d - 1, cpu->d); } break;

        /* MVI D,d8 */
        case 0x16: { cpu->d = cpu->MEM_READ(cpu->pc++); } break;

        /* RAL */
        case 0x17:
        {
            tmp = cpu->a;
            cpu->a = ((cpu->cc.cy)>>7)|(tmp<<1);
            cpu->cc.cy = (tmp&0x80) ? 1 : 0;
        } break;

        case 0x18: { /* Nothing */ } break;

        /* DAD D */
        case 0x19: {
            tmp32 = (uint32_t)hl + (uint32_t)de;
            cpu->h = (uint8_t)((tmp32&0xff00)>>8);
            cpu->l = (uint8_t)((tmp32&0x00ff)>>0);
            cpu->cc.cy = (tmp32 > 0xffff) ? 1 : 0;
        } break;

        /* LDAX D */
        case 0x1a: { cpu->a = cpu->MEM_READ(de); } break;

        /* DCX D */
        case 0x1b: { tmp16 = de - 1; cpu->d = (uint8_t)((tmp16 >> 8) & 0xff); cpu->e = (uint8_t)((tmp16 >> 0) & 0xff); } break;

        /* INR/DCR E */
        case 0x1c: { normlop((uint16_t)cpu->e + 1, cpu->e); } break;
        case 0x1d: { normlop((uint16_t)cpu->e - 1, cpu->e); } break;

        /* MVI E,d8 */
        case 0x1e: { cpu->e = cpu->MEM_READ(cpu->pc++); } break;

        /* RAR */
        case 0x1f:
        {
            tmp = cpu->a;
            cpu->a = (cpu->cc.cy<<7)|(tmp>>1);
            cpu->cc.cy = (tmp&1) ? 1 : 0;
        } break;

        case 0x20: { /* Nothing */ } break;

        /* LXI H,d16 */
        case 0x21: { cpu->l = cpu->MEM_READ(cpu->pc++); cpu->h = cpu->MEM_READ(cpu->pc++); } break;

        /* SHLD adr */
        case 0x22: { addr = cpu->MEM_READ16(cpu->pc); cpu->pc += 2; cpu->MEM_WRITE(addr, cpu->l); cpu->MEM_WRITE(addr+1, cpu->h); } break;

        /* INX H */
        case 0x23: { tmp16 = hl + 1; cpu->h = (uint8_t)((tmp16 >> 8) & 0xff); cpu->l = (uint8_t)((tmp16 >> 0) & 0xff); } break;

        /* INR/DCR H */
        case 0x24: { normlop((uint16_t)cpu->h + 1, cpu->h); } break;
        case 0x25: { normlop((uint16_t)cpu->h - 1, cpu->h); } break;

        /* MVI H,d8 */
        case 0x26: { cpu->h = cpu->MEM_READ(cpu->pc++); } break;

        case 0x27: { unimplemented(cpu); } break;

        case 0x28: { /* Nothing */ } break;

        /* DAD H */
        case 0x29: {
            tmp32 = (uint32_t)hl + (uint32_t)hl;
            cpu->h = (uint8_t)((tmp32&0xff00)>>8);
            cpu->l = (uint8_t)((tmp32&0x00ff)>>0);
            cpu->cc.cy = (tmp32 > 0xffff) ? 1 : 0;
        } break;

        /* LHLD adr */
        case 0x2a: { addr = cpu->MEM_READ16(cpu->pc); cpu->pc += 2; cpu->l = cpu->MEM_READ(addr); cpu->h = cpu->MEM_READ(addr+1); } break;

        /* DCX H */
        case 0x2b: { tmp16 = hl - 1; cpu->h = (uint8_t)((tmp16 >> 8) & 0xff); cpu->l = (uint8_t)((tmp16 >> 0) & 0xff); } break;

        /* INR/DCR L */
        case 0x2c: { normlop((uint16_t)cpu->l + 1, cpu->l); } break;
        case 0x2d: { normlop((uint16_t)cpu->l - 1, cpu->l); } break;

        /* MVI L,d8 */
        case 0x2e: { cpu->l = cpu->MEM_READ(cpu->pc++); } break;

        /* CMA */
        case 0x2f: { cpu->a = ~(cpu->a); } break;

        case 0x30: { /* Nothing */ } break;

        /* LXI SP,d16 */
        case 0x31: { cpu->sp = (uint16_t)cpu->MEM_READ(cpu->pc++) | ((uint16_t)cpu->MEM_READ(cpu->pc++) << 8); } break;

        /* STA adr */
        case 0x32: { addr = cpu->MEM_READ16(cpu->pc); cpu->pc += 2; cpu->MEM_WRITE(addr, cpu->a); } break;

        /* INX SP */
        case 0x33: { cpu->sp += 1; } break;

        /* INR/DCR M */
        case 0x34: { tmp16 = (uint16_t)cpu->MEM_READ(hl) + 1; setflags(cpu, tmp16); cpu->MEM_WRITE(hl, (uint8_t)(tmp16 & 0xff)); } break;
        case 0x35: { tmp16 = (uint16_t)cpu->MEM_READ(hl) - 1; setflags(cpu, tmp16); cpu->MEM_WRITE(hl, (uint8_t)(tmp16 & 0xff)); } break;

        /* MVI M,d8 */
        case 0x36: { cpu->MEM_WRITE(hl, cpu->MEM_READ(cpu->pc++)); } break;

        /* STC */
        case 0x37: { cpu->cc.cy = 1; } break;

        case 0x38: { /* Nothing */ } break;

        /* DAD SP */
        case 0x39: {
            tmp32 = (uint32_t)hl + (uint32_t)cpu->sp;
            cpu->h = (uint8_t)((tmp32&0xff00)>>8);
            cpu->l = (uint8_t)((tmp32&0x00ff)>>0);
            cpu->cc.cy = (tmp32 > 0xffff) ? 1 : 0;
        } break;

        /* LDA adr */
        case 0x3a: { addr = cpu->MEM_READ16(cpu->pc); cpu->pc += 2; cpu->a = cpu->MEM_READ(addr); } break;

        /* DCX SP */
        case 0x3b: { cpu->sp -= 1; } break;

        /* INR/DCR A */
        case 0x3c: { normlop((uint16_t)cpu->a + 1, cpu->a); } break;
        case 0x3d: { normlop((uint16_t)cpu->a - 1, cpu->a); } break;

        /* MVI A,d8 */
        case 0x3e: { cpu->a = cpu->MEM_READ(cpu->pc++); } break;

        /* CMC */
        case 0x3f: { cpu->cc.cy = (cpu->cc.cy) ? 0 : 1; } break;

        /* MOV B, ...*/
        case 0x40: { cpu->b = cpu->b;    } break;
        case 0x41: { cpu->b = cpu->c;    } break;
        case 0x42: { cpu->b = cpu->d;    } break;
        case 0x43: { cpu->b = cpu->e;    } break;
        case 0x44: { cpu->b = cpu->h;    } break;
        case 0x45: { cpu->b = cpu->l;    } break;
        case 0x46: { cpu->b = cpu->MEM_READ(hl); } break;
        case 0x47: { cpu->b = cpu->a;     } break;

        /* MOV C, ...*/
        case 0x48: { cpu->c = cpu->b;    } break;
        case 0x49: { cpu->c = cpu->c;    } break;
        case 0x4a: { cpu->c = cpu->d;    } break;
        case 0x4b: { cpu->c = cpu->e;    } break;
        case 0x4c: { cpu->c = cpu->h;    } break;
        case 0x4d: { cpu->c = cpu->l;    } break;
        case 0x4e: { cpu->c = cpu->MEM_READ(hl); } break;
        case 0x4f: { cpu->c = cpu->a;    } break;

        /* MOV D, ...*/
        case 0x50: { cpu->d = cpu->b;    } break;
        case 0x51: { cpu->d = cpu->c;    } break;
        case 0x52: { cpu->d = cpu->d;    } break;
        case 0x53: { cpu->d = cpu->e;    } break;
        case 0x54: { cpu->d = cpu->h;    } break;
        case 0x55: { cpu->d = cpu->l;    } break;
        case 0x56: { cpu->d = cpu->MEM_READ(hl); } break;
        case 0x57: { cpu->d = cpu->a;    } break;

        /* MOV E, ...*/
        case 0x58: { cpu->e = cpu->b;    } break;
        case 0x59: { cpu->e = cpu->c;    } break;
        case 0x5a: { cpu->e = cpu->d;    } break;
        case 0x5b: { cpu->e = cpu->e;    } break;
        case 0x5c: { cpu->e = cpu->h;    } break;
        case 0x5d: { cpu->e = cpu->l;    } break;
        case 0x5e: { cpu->e = cpu->MEM_READ(hl); } break;
        case 0x5f: { cpu->e = cpu->a;    } break;

        /* MOV H, ...*/
        case 0x60: { cpu->h = cpu->b;    } break;
        case 0x61: { cpu->h = cpu->c;    } break;
        case 0x62: { cpu->h = cpu->d;    } break;
        case 0x63: { cpu->h = cpu->e;    } break;
        case 0x64: { cpu->h = cpu->h;    } break;
        case 0x65: { cpu->h = cpu->l;    } break;
        case 0x66: { cpu->h = cpu->MEM_READ(hl); } break;
        case 0x67: { cpu->h = cpu->a;    } break;

        /* MOV L, ...*/
        case 0x68: { cpu->l = cpu->b;    } break;
        case 0x69: { cpu->l = cpu->c;    } break;
        case 0x6a: { cpu->l = cpu->d;    } break;
        case 0x6b: { cpu->l = cpu->e;    } break;
        case 0x6c: { cpu->l = cpu->h;    } break;
        case 0x6d: { cpu->l = cpu->l;    } break;
        case 0x6e: { cpu->l = cpu->MEM_READ(hl); } break;
        case 0x6f: { cpu->l = cpu->a;    } break;

        /* MOVE M, ... */
        case 0x70: { cpu->MEM_WRITE(hl, cpu->b); } break;
        case 0x71: { cpu->MEM_WRITE(hl, cpu->c); } break;
        case 0x72: { cpu->MEM_WRITE(hl, cpu->d); } break;
        case 0x73: { cpu->MEM_WRITE(hl, cpu->e); } break;
        case 0x74: { cpu->MEM_WRITE(hl, cpu->h); } break;
        case 0x75: { cpu->MEM_WRITE(hl, cpu->l); } break;

        /* HLT */
        case 0x76: { halt = 1; } break;

        /* MOVE M, A */
        case 0x77: { cpu->MEM_WRITE(hl, cpu->a); } break;

        /* MOV A, ... */
        case 0x78: { cpu->a = cpu->b;       } break;
        case 0x79: { cpu->a = cpu->c;       } break;
        case 0x7a: { cpu->a = cpu->d;       } break;
        case 0x7b: { cpu->a = cpu->e;       } break;
        case 0x7c: { cpu->a = cpu->h;       } break;
        case 0x7d: { cpu->a = cpu->l;       } break;
        case 0x7e: { cpu->a = cpu->MEM_READ(hl); } break;
        case 0x7f: { cpu->a = cpu->a;       } break;

        /* ADD ... */
        case 0x80: { arithop((uint16_t)cpu->a + cpu->b);       } break;
        case 0x81: { arithop((uint16_t)cpu->a + cpu->c);       } break;
        case 0x82: { arithop((uint16_t)cpu->a + cpu->d);       } break;
        case 0x83: { arithop((uint16_t)cpu->a + cpu->e);       } break;
        case 0x84: { arithop((uint16_t)cpu->a + cpu->h);       } break;
        case 0x85: { arithop((uint16_t)cpu->a + cpu->l);       } break;
        case 0x86: { arithop((uint16_t)cpu->a + cpu->MEM_READ(hl)); } break;
        case 0x87: { arithop((uint16_t)cpu->a + cpu->a);       } break;

        /* ADC ... */
        case 0x88: { arithop((uint16_t)cpu->a + cpu->b + cpu->cc.cy);       } break;
        case 0x89: { arithop((uint16_t)cpu->a + cpu->c + cpu->cc.cy);       } break;
        case 0x8a: { arithop((uint16_t)cpu->a + cpu->d + cpu->cc.cy);       } break;
        case 0x8b: { arithop((uint16_t)cpu->a + cpu->e + cpu->cc.cy);       } break;
        case 0x8c: { arithop((uint16_t)cpu->a + cpu->h + cpu->cc.cy);       } break;
        case 0x8d: { arithop((uint16_t)cpu->a + cpu->l + cpu->cc.cy);       } break;
        case 0x8e: { arithop((uint16_t)cpu->a + cpu->MEM_READ(hl) + cpu->cc.cy); } break;
        case 0x8f: { arithop((uint16_t)cpu->a + cpu->a + cpu->cc.cy);       } break;

        /* SUB ... */
        case 0x90: { arithop((uint16_t)cpu->a - cpu->b);       } break;
        case 0x91: { arithop((uint16_t)cpu->a - cpu->c);       } break;
        case 0x92: { arithop((uint16_t)cpu->a - cpu->d);       } break;
        case 0x93: { arithop((uint16_t)cpu->a - cpu->e);       } break;
        case 0x94: { arithop((uint16_t)cpu->a - cpu->h);       } break;
        case 0x95: { arithop((uint16_t)cpu->a - cpu->l);       } break;
        case 0x96: { arithop((uint16_t)cpu->a - cpu->MEM_READ(hl)); } break;
        case 0x97: { arithop((uint16_t)cpu->a - cpu->a);       } break;

        /* SBB ... */
        case 0x98: { arithop((uint16_t)cpu->a - cpu->b - cpu->cc.cy);       } break;
        case 0x99: { arithop((uint16_t)cpu->a - cpu->c - cpu->cc.cy);       } break;
        case 0x9a: { arithop((uint16_t)cpu->a - cpu->d - cpu->cc.cy);       } break;
        case 0x9b: { arithop((uint16_t)cpu->a - cpu->e - cpu->cc.cy);       } break;
        case 0x9c: { arithop((uint16_t)cpu->a - cpu->h - cpu->cc.cy);       } break;
        case 0x9d: { arithop((uint16_t)cpu->a - cpu->l - cpu->cc.cy);       } break;
        case 0x9e: { arithop((uint16_t)cpu->a - cpu->MEM_READ(hl) - cpu->cc.cy); } break;
        case 0x9f: { arithop((uint16_t)cpu->a - cpu->a - cpu->cc.cy);       } break;

        /* ANA ... */
        case 0xa0: { arithop((uint16_t)cpu->a & cpu->b);       } break;
        case 0xa1: { arithop((uint16_t)cpu->a & cpu->c);       } break;
        case 0xa2: { arithop((uint16_t)cpu->a & cpu->d);       } break;
        case 0xa3: { arithop((uint16_t)cpu->a & cpu->e);       } break;
        case 0xa4: { arithop((uint16_t)cpu->a & cpu->h);       } break;
        case 0xa5: { arithop((uint16_t)cpu->a & cpu->l);       } break;
        case 0xa6: { arithop((uint16_t)cpu->a & cpu->MEM_READ(hl)); } break;
        case 0xa7: { arithop((uint16_t)cpu->a & cpu->a);       } break;

        /* XRA ... */
        case 0xa8: { arithop((uint16_t)cpu->a ^ cpu->b);       } break;
        case 0xa9: { arithop((uint16_t)cpu->a ^ cpu->c);       } break;
        case 0xaa: { arithop((uint16_t)cpu->a ^ cpu->d);       } break;
        case 0xab: { arithop((uint16_t)cpu->a ^ cpu->e);       } break;
        case 0xac: { arithop((uint16_t)cpu->a ^ cpu->h);       } break;
        case 0xad: { arithop((uint16_t)cpu->a ^ cpu->l);       } break;
        case 0xae: { arithop((uint16_t)cpu->a ^ cpu->MEM_READ(hl)); } break;
        case 0xaf: { arithop((uint16_t)cpu->a ^ cpu->a);       } break;

        /* ORA ... */
        case 0xb0: { arithop((uint16_t)cpu->a | cpu->b);       } break;
        case 0xb1: { arithop((uint16_t)cpu->a | cpu->c);       } break;
        case 0xb2: { arithop((uint16_t)cpu->a | cpu->d);       } break;
        case 0xb3: { arithop((uint16_t)cpu->a | cpu->e);       } break;
        case 0xb4: { arithop((uint16_t)cpu->a | cpu->h);       } break;
        case 0xb5: { arithop((uint16_t)cpu->a | cpu->l);       } break;
        case 0xb6: { arithop((uint16_t)cpu->a | cpu->MEM_READ(hl)); } break;
        case 0xb7: { arithop((uint16_t)cpu->a | cpu->a);       } break;

        /* CMP ... */
        case 0xb8: { tmp = cpu->a; arithop((uint16_t)cpu->a - cpu->b); cpu->a = tmp;       } break;
        case 0xb9: { tmp = cpu->a; arithop((uint16_t)cpu->a - cpu->c); cpu->a = tmp;       } break;
        case 0xba: { tmp = cpu->a; arithop((uint16_t)cpu->a - cpu->d); cpu->a = tmp;       } break;
        case 0xbb: { tmp = cpu->a; arithop((uint16_t)cpu->a - cpu->e); cpu->a = tmp;       } break;
        case 0xbc: { tmp = cpu->a; arithop((uint16_t)cpu->a - cpu->h); cpu->a = tmp;       } break;
        case 0xbd: { tmp = cpu->a; arithop((uint16_t)cpu->a - cpu->l); cpu->a = tmp;       } break;
        case 0xbe: { tmp = cpu->a; arithop((uint16_t)cpu->a - cpu->MEM_READ(hl)); cpu->a = tmp; } break;
        case 0xbf: { tmp = cpu->a; arithop((uint16_t)cpu->a - cpu->a); cpu->a = tmp;       } break;

        /* RNZ */
        case 0xc0: { if(cpu->cc.z == 0) { ret(); } } break;

        /* POP B */
        case 0xc1: { cpu->c = cpu->MEM_READ(cpu->sp++); cpu->b = cpu->MEM_READ(cpu->sp++); } break;

        /* JNZ adr */
        case 0xc2: { addr = cpu->MEM_READ16(cpu->pc); if(cpu->cc.z == 0) { cpu->pc = addr; } else { cpu->pc += 2; } } break;

        /* JMP adr */
#ifdef CPUDIAG
        case 0xc3:
        {
            addr = cpu->MEM_READ16(cpu->pc);
            if(addr == 0)
            {
                halt = 1;
            }
            else
            {
                cpu->pc = addr;
            }
        } break;
#else
        case 0xc3: { addr = cpu->MEM_READ16(cpu->pc); cpu->pc = addr; } break;
#endif

        /* CNZ adr */
        case 0xc4: { if(cpu->cc.z == 0) { call(); } else { cpu->pc += 2; } } break;

        /* PUSH B */
        case 0xc5: { cpu->MEM_WRITE(--cpu->sp, cpu->b); cpu->MEM_WRITE(--cpu->sp, cpu->c); } break;

        /* ADI d8 */
        case 0xc6: { tmp = cpu->MEM_READ(cpu->pc++); arithop((uint16_t)cpu->a + tmp); } break;

        case 0xc7: { unimplemented(cpu); } break;

        /* RZ */
        case 0xc8: { if(cpu->cc.z) { ret(); } } break;

        /* RET */
        case 0xc9: { ret(); } break;

        /* JZ adr */
        case 0xca: { addr = cpu->MEM_READ16(cpu->pc); if(cpu->cc.z) { cpu->pc = addr; } else { cpu->pc += 2; } } break;

        case 0xcb: { /* Nothing */ } break;

        /* CZ adr */
        case 0xcc: { if(cpu->cc.z) { call(); } else { cpu->pc += 2; } } break;

        /* CALL adr */
#ifdef CPUDIAG
        case 0xcd:
        {
            addr = cpu->MEM_READ16(cpu->pc);
            if(addr == 5)
            {
                cpu->pc += 2;
                if(cpu->c == 9)
                {
                    tmp16 = de;
                    char *str = (char *)(&memory[tmp16 + 3]);
                    while(*str != '$') /* i8080 strings generally terminate with '$' character */
                    {
                        printf("%c", *str);
                        ++str;
                    }
                }
                else if(cpu->c == 2)
                {
                    printf("%c", (char)cpu->e);
                }
            }
            else if(addr == 0)
            {
                halt = 1;
            }
            else
            {
                call();
            }
        } break;
#else
        case 0xcd: { call(); } break;
#endif

        /* ACI d8 */
        case 0xce:
        {
            tmp = cpu->MEM_READ(cpu->pc++);
            arithop((uint16_t)cpu->a + tmp + cpu->cc.cy);
        } break;

        case 0xcf: { unimplemented(cpu); } break;

        /* RNC */
        case 0xd0: { if(cpu->cc.cy == 0) { ret(); } } break;

        /* POP D */
        case 0xd1: { cpu->e = cpu->MEM_READ(cpu->sp++); cpu->d = cpu->MEM_READ(cpu->sp++); } break;

        /* JNC adr */
        case 0xd2: { addr = cpu->MEM_READ16(cpu->pc); if(cpu->cc.cy == 0) { cpu->pc = addr; } else { cpu->pc += 2; } } break;

        /* OUT d8 */
        case 0xd3: { cpu->OUT_PORT(cpu->MEM_READ(cpu->pc++), cpu->a); } break;

        /* CNC adr */
        case 0xd4: { if(cpu->cc.cy == 0) { call(); } else { cpu->pc += 2; } } break;

        /* PUSH D */
        case 0xd5: { cpu->MEM_WRITE(--cpu->sp, cpu->d); cpu->MEM_WRITE(--cpu->sp, cpu->e); } break;

        /* SUI d8 */
        case 0xd6: { tmp = cpu->MEM_READ(cpu->pc++); arithop((uint16_t)cpu->a - tmp); } break;

        case 0xd7: { unimplemented(cpu); } break;

        /* RC */
        case 0xd8: { if(cpu->cc.cy) { ret(); } } break;

        case 0xd9: { /* Nothing */ } break;

        /* JC adr */
        case 0xda: { addr = cpu->MEM_READ16(cpu->pc); if(cpu->cc.cy) { cpu->pc = addr; } else { cpu->pc += 2; } } break;

        /* IN d8 */
        case 0xdb: { cpu->a = cpu->IN_PORT(cpu->MEM_READ(cpu->pc++)); } break;

        /* CC adr */
        case 0xdc: { if(cpu->cc.cy) { call(); } else { cpu->pc += 2; } } break;

        case 0xdd: { /* Nothing */ } break;

        /* SBI d8 */
        case 0xde: { tmp = cpu->MEM_READ(cpu->pc++); arithop((uint16_t)cpu->a - tmp - cpu->cc.cy); } break;

        case 0xdf: { unimplemented(cpu); } break;

        /* RPO */
        case 0xe0: { if(cpu->cc.p == 0) { ret(); } } break;

        /* POP H */
        case 0xe1: { cpu->l = cpu->MEM_READ(cpu->sp++); cpu->h = cpu->MEM_READ(cpu->sp++); } break;

        /* JPO adr */
        case 0xe2: { addr = cpu->MEM_READ16(cpu->pc); if(cpu->cc.p == 0) { cpu->pc = addr; } else { cpu->pc += 2; } } break;

        /* XTHL */
        case 0xe3:
        {
            tmp  = cpu->MEM_READ(cpu->sp);
            tmp2 = cpu->MEM_READ(cpu->sp + 1);
            cpu->MEM_WRITE(cpu->sp, cpu->l);
            cpu->MEM_WRITE(cpu->sp + 1, cpu->h);
            cpu->l = tmp;
            cpu->h = tmp2;
        } break;

        /* CPO adr */
        case 0xe4: { if(cpu->cc.p == 0) { call(); } else { cpu->pc += 2; } } break;

        /* PUSH H */
        case 0xe5: { cpu->MEM_WRITE(--cpu->sp, cpu->h); cpu->MEM_WRITE(--cpu->sp, cpu->l); } break;

        /* ANI d8 */
        case 0xe6: { tmp = cpu->MEM_READ(cpu->pc++); arithop((uint16_t)cpu->a & tmp); } break;

        case 0xe7: { unimplemented(cpu); } break;

        /* RPE */
        case 0xe8: { if(cpu->cc.p) { ret(); } } break;

        /* PCHL */
        case 0xe9: { cpu->pc = hl; } break;

        /* JPE adr */
        case 0xea: { addr = cpu->MEM_READ16(cpu->pc); if(cpu->cc.p) { cpu->pc = addr; } else { cpu->pc += 2; } } break;

        /* XCHG */
        case 0xeb:
        {
            tmp = cpu->h;
            tmp2 = cpu->l;
            cpu->h = cpu->d;
            cpu->d = tmp;
            cpu->l = cpu->e;
            cpu->e = tmp2;
        } break;

        /* CPE adr */
        case 0xec: { if(cpu->cc.p) { call(); } else { cpu->pc += 2; } } break;

        case 0xed: { /* Nothing */ } break;

        /* XRI d8 */
        case 0xee: { tmp = cpu->MEM_READ(cpu->pc++); arithop((uint16_t)cpu->a ^ tmp); } break;

        case 0xef: { unimplemented(cpu); } break;

        /* RP */
        case 0xf0: { if(cpu->cc.s == 0) { ret(); } } break;

        /* POP PSW */
        case 0xf1:
        {
            tmp = cpu->MEM_READ(cpu->sp++);
            cpu->a = cpu->MEM_READ(cpu->sp++);
            cpu->cc.z  = (0x01 == (tmp & 0x01));
            cpu->cc.s  = (0x02 == (tmp & 0x02));
            cpu->cc.p  = (0x04 == (tmp & 0x04));
            cpu->cc.cy = (0x08 == (tmp & 0x08));
            cpu->cc.ac = (0x10 == (tmp & 0x10));
        } break;

        /* JP adr */
        case 0xf2: { addr = cpu->MEM_READ16(cpu->pc); if(cpu->cc.s == 0) { cpu->pc = addr; } else { cpu->pc += 2; } } break;

        /* DI */
        case 0xf3: { cpu->int_enabled = 0; } break;

        /* CP adr */
        case 0xf4: { if(cpu->cc.s == 0) { call(); } else { cpu->pc += 2; } } break;

        /* PUSH PSW */
        case 0xf5:
        {
            tmp = ((cpu->cc.z << 0) | (cpu->cc.s << 1) | (cpu->cc.p << 2) | (cpu->cc.cy << 3) | (cpu->cc.ac << 4));
            cpu->MEM_WRITE(--cpu->sp, cpu->a);
            cpu->MEM_WRITE(--cpu->sp, tmp);
        } break;

        /* ORI d8 */
        case 0xf6: { tmp = cpu->MEM_READ(cpu->pc++); arithop((uint16_t)cpu->a | tmp); } break;

        case 0xf7: { unimplemented(cpu); } break;

        /* RM */
        case 0xf8: { if(cpu->cc.s) { ret(); } } break;

        /* SPHL */
        case 0xf9: { cpu->sp = hl; } break;

        /* JM adr */
        case 0xfa: { addr = cpu->MEM_READ16(cpu->pc); if(cpu->cc.s) { cpu->pc = addr; } else { cpu->pc += 2; } } break;

        /* EI */
        case 0xfb: { cpu->int_enabled = 1; } break;

        /* CM adr */
        case 0xfc: { if(cpu->cc.s) { call(); } else { cpu->pc += 2; } } break;

        case 0xfd: { /* Nothing */ } break;

        /* CPI d8 */
        case 0xfe: { tmp2 = cpu->a; tmp = cpu->MEM_READ(cpu->pc++); arithop((uint16_t)cpu->a - tmp); cpu->a = tmp2; } break;

        case 0xff: { unimplemented(cpu); } break;

#undef ret
#undef call

#undef de
#undef bc
#undef hl

#undef normlop
#undef arithop
    }

    return(halt);
}
