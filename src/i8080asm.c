/**
 * TODO(driverfury):
 * [ ] srcl is not working properly, we need to increment srcl when
 *     we parse not when we get the token.
 */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "i8080.h"

#define arrsize(a) (sizeof(a)/sizeof(a[0]))

#define MAX_ID_LEN 33

enum
{
    LBL_UNSOLVED,
    LBL_RESOLVED,

    LBL_STATUS_COUNT
};

struct Label
{
    int status;
    char name[MAX_ID_LEN];
    unsigned int addr;
    int isconst;
};

struct Label lbls[200]; /* TODO: More labels? */
int lblscount;

struct Label *
getlbl(char *name)
{
    int i;

    for(i = 0;
        i < lblscount;
        ++i)
    {
        if(!strcmp(name, lbls[i].name))
        {
            return(&lbls[i]);
        }
    }

    return(0);
}

struct Label *
addlbl(char *name, unsigned int addr, int status)
{
    struct Label *l;

    assert(lblscount < arrsize(lbls));

    l = &lbls[lblscount++];

    strncpy(l->name, name, MAX_ID_LEN);
    l->name[MAX_ID_LEN-1] = 0;
    l->addr = addr;
    l->status = status;
    l->isconst = 0;

    return(l);
}

enum
{
    OP_NONE,

    OP_REG_A,
    OP_REG_B,
    OP_REG_C,
    OP_REG_D,
    OP_REG_E,
    OP_REG_H,
    OP_REG_L,
    OP_REG_M,
    OP_REG_SP,
    OP_REG_PSW,

    OP_IMM,

    OP_COUNT
};

struct Op
{
    int type;
    struct Expr *expr;
};

struct Op *
mkoperand(int type, struct Expr *expr)
{
    struct Op *op;

    op = (struct Op *)malloc(sizeof(struct Op));
    if(op)
    {
        op->type = type;
        op->expr = expr;
    }

    return(op);
}

enum
{
    ENC_SIMPLE,
    ENC_IMM8,
    ENC_IMM16,

    ENC_COUNT
};

struct Ins
{
    char *mnem;
    int size;
    u8 opc;
    int numop;
    int op1;
    int op2;
    int enc;
};

struct Ins isa[] = 
{
/* nop         */ { "nop",  1, 0x00, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "NOP",  1, 0x00, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* lxi b,d16   */ { "lxi",  3, 0x01, 2, OP_REG_B,   OP_IMM,     ENC_IMM16  },
                  { "LXI",  3, 0x01, 2, OP_REG_B,   OP_IMM,     ENC_IMM16  },
/* stax b      */ { "stax", 1, 0x02, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "STAX", 1, 0x02, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* inx b       */ { "inx",  1, 0x03, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "INX",  1, 0x03, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* inr b       */ { "inr",  1, 0x04, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "INR",  1, 0x04, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* dcr b       */ { "dcr",  1, 0x05, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "DCR",  1, 0x05, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* mvi b,d8    */ { "mvi",  2, 0x06, 2, OP_REG_B,   OP_IMM,     ENC_IMM8   },
                  { "MVI",  2, 0x06, 2, OP_REG_B,   OP_IMM,     ENC_IMM8   },
/* rlc         */ { "rlc",  1, 0x07, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RLC",  1, 0x07, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* dad b       */ { "dad",  1, 0x09, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "DAD",  1, 0x09, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* ldax b      */ { "ldax", 1, 0x0a, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "LDAX", 1, 0x0a, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* dcx b       */ { "dcx",  1, 0x0b, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "DCX",  1, 0x0b, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* inr c       */ { "inr",  1, 0x0c, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
                  { "INR",  1, 0x0c, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
/* dcr c       */ { "dcr",  1, 0x0d, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
                  { "DCR",  1, 0x0d, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
/* mvi c,d8    */ { "mvi",  2, 0x0e, 2, OP_REG_C,   OP_IMM,     ENC_IMM8   },
                  { "MVI",  2, 0x0e, 2, OP_REG_C,   OP_IMM,     ENC_IMM8   },
/* rrc         */ { "rrc",  1, 0x0f, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RRC",  1, 0x0f, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* lxi d,d16   */ { "lxi",  3, 0x11, 2, OP_REG_D,   OP_IMM,     ENC_IMM16  },
                  { "LXI",  3, 0x11, 2, OP_REG_D,   OP_IMM,     ENC_IMM16  },
/* stax d      */ { "stax", 1, 0x12, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "STAX", 1, 0x12, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* inx d       */ { "inx",  1, 0x13, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "INX",  1, 0x13, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* inr d       */ { "inr",  1, 0x14, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "INR",  1, 0x14, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* dcr d       */ { "dcr",  1, 0x15, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "DCR",  1, 0x15, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* mvi d,d8    */ { "mvi",  2, 0x16, 2, OP_REG_D,   OP_IMM,     ENC_IMM8   },
                  { "MVI",  2, 0x16, 2, OP_REG_D,   OP_IMM,     ENC_IMM8   },
/* ral         */ { "ral",  1, 0x17, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RAL",  1, 0x17, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* dad d       */ { "dad",  1, 0x19, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "DAD",  1, 0x19, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* ldax d      */ { "ldax", 1, 0x1a, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "LDAX", 1, 0x1a, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* dcx d       */ { "dcx",  1, 0x1b, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "DCX",  1, 0x1b, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* inr e       */ { "inr",  1, 0x1c, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
                  { "INR",  1, 0x1c, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
/* dcr e       */ { "dcr",  1, 0x1d, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
                  { "DCR",  1, 0x1d, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
/* mvi e,d8    */ { "mvi",  2, 0x1e, 2, OP_REG_E,   OP_IMM,     ENC_IMM8   },
                  { "MVI",  2, 0x1e, 2, OP_REG_E,   OP_IMM,     ENC_IMM8   },
/* rar         */ { "rar",  1, 0x1f, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RAR",  1, 0x1f, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* lxi h,d16   */ { "lxi",  3, 0x21, 2, OP_REG_H,   OP_IMM,     ENC_IMM16  },
                  { "LXI",  3, 0x21, 2, OP_REG_H,   OP_IMM,     ENC_IMM16  },
/* shld adr    */ { "shld", 3, 0x22, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "SHLD", 3, 0x22, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* inx h       */ { "inx",  1, 0x23, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "INX",  1, 0x23, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* inr h       */ { "inr",  1, 0x24, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "INR",  1, 0x24, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* dcr h       */ { "dcr",  1, 0x25, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "DCR",  1, 0x25, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* mvi h,d8    */ { "mvi",  2, 0x26, 2, OP_REG_H,   OP_IMM,     ENC_IMM8   },
                  { "MVI",  2, 0x26, 2, OP_REG_H,   OP_IMM,     ENC_IMM8   },
/* daa         */ { "daa",  1, 0x27, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "DAA",  1, 0x27, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* dad h       */ { "dad",  1, 0x29, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "DAD",  1, 0x29, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* lhld adr    */ { "lhld", 3, 0x2a, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "LHLD", 3, 0x2a, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* dcx h       */ { "dcx",  1, 0x2b, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "DCX",  1, 0x2b, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* inr l       */ { "inr",  1, 0x2c, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
                  { "INR",  1, 0x2c, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
/* dcr l       */ { "dcr",  1, 0x2d, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
                  { "DCR",  1, 0x2d, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
/* mvi l,d8    */ { "mvi",  2, 0x2e, 2, OP_REG_L,   OP_IMM,     ENC_IMM8   },
                  { "MVI",  2, 0x2e, 2, OP_REG_L,   OP_IMM,     ENC_IMM8   },
/* cma         */ { "cma",  1, 0x2f, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "CMA",  1, 0x2f, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* lxi sp,d16  */ { "lxi",  3, 0x31, 2, OP_REG_SP,  OP_IMM,     ENC_IMM16  },
                  { "LXI",  3, 0x31, 2, OP_REG_SP,  OP_IMM,     ENC_IMM16  },
/* sta adr     */ { "sta",  3, 0x32, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "STA",  3, 0x32, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* inx sp      */ { "inx",  1, 0x33, 1, OP_REG_SP,  OP_NONE,    ENC_SIMPLE },
                  { "INX",  1, 0x33, 1, OP_REG_SP,  OP_NONE,    ENC_SIMPLE },
/* inr m       */ { "inr",  1, 0x34, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
                  { "INR",  1, 0x34, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
/* dcr m       */ { "dcr",  1, 0x35, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
                  { "DCR",  1, 0x35, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
/* mvi m,d8    */ { "mvi",  2, 0x36, 2, OP_REG_M,   OP_IMM,     ENC_IMM8   },
                  { "MVI",  2, 0x36, 2, OP_REG_M,   OP_IMM,     ENC_IMM8   },
/* stc         */ { "stc",  1, 0x37, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "STC",  1, 0x37, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* dad sp      */ { "dad",  1, 0x39, 1, OP_REG_SP,  OP_NONE,    ENC_SIMPLE },
                  { "DAD",  1, 0x39, 1, OP_REG_SP,  OP_NONE,    ENC_SIMPLE },
/* lda adr     */ { "lda",  3, 0x3a, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "LDA",  3, 0x3a, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* dcx sp      */ { "dcx",  1, 0x3b, 1, OP_REG_SP,  OP_NONE,    ENC_SIMPLE },
                  { "DCX",  1, 0x3b, 1, OP_REG_SP,  OP_NONE,    ENC_SIMPLE },
/* inr a       */ { "inr",  1, 0x3c, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
                  { "INR",  1, 0x3c, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
/* dcr a       */ { "dcr",  1, 0x3d, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
                  { "DCR",  1, 0x3d, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
/* mvi a,d8    */ { "mvi",  2, 0x3e, 2, OP_REG_A,   OP_IMM,     ENC_IMM8   },
                  { "MVI",  2, 0x3e, 2, OP_REG_A,   OP_IMM,     ENC_IMM8   },
/* cmc         */ { "cmc",  1, 0x3f, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "CMC",  1, 0x3f, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* mov b,b     */ { "mov",  1, 0x40, 2, OP_REG_B,   OP_REG_B,   ENC_SIMPLE },
                  { "MOV",  1, 0x40, 2, OP_REG_B,   OP_REG_B,   ENC_SIMPLE },
/* mov b,c     */ { "mov",  1, 0x41, 2, OP_REG_B,   OP_REG_C,   ENC_SIMPLE },
                  { "MOV",  1, 0x41, 2, OP_REG_B,   OP_REG_C,   ENC_SIMPLE },
/* mov b,d     */ { "mov",  1, 0x42, 2, OP_REG_B,   OP_REG_D,   ENC_SIMPLE },
                  { "MOV",  1, 0x42, 2, OP_REG_B,   OP_REG_D,   ENC_SIMPLE },
/* mov b,e     */ { "mov",  1, 0x43, 2, OP_REG_B,   OP_REG_E,   ENC_SIMPLE },
                  { "MOV",  1, 0x43, 2, OP_REG_B,   OP_REG_E,   ENC_SIMPLE },
/* mov b,h     */ { "mov",  1, 0x44, 2, OP_REG_B,   OP_REG_H,   ENC_SIMPLE },
                  { "MOV",  1, 0x44, 2, OP_REG_B,   OP_REG_H,   ENC_SIMPLE },
/* mov b,l     */ { "mov",  1, 0x45, 2, OP_REG_B,   OP_REG_L,   ENC_SIMPLE },
                  { "MOV",  1, 0x45, 2, OP_REG_B,   OP_REG_L,   ENC_SIMPLE },
/* mov b,m     */ { "mov",  1, 0x46, 2, OP_REG_B,   OP_REG_M,   ENC_SIMPLE },
                  { "MOV",  1, 0x46, 2, OP_REG_B,   OP_REG_M,   ENC_SIMPLE },
/* mov b,a     */ { "mov",  1, 0x47, 2, OP_REG_B,   OP_REG_A,   ENC_SIMPLE },
                  { "MOV",  1, 0x47, 2, OP_REG_B,   OP_REG_A,   ENC_SIMPLE },
/* mov c,b     */ { "mov",  1, 0x48, 2, OP_REG_C,   OP_REG_B,   ENC_SIMPLE },
                  { "MOV",  1, 0x48, 2, OP_REG_C,   OP_REG_B,   ENC_SIMPLE },
/* mov c,c     */ { "mov",  1, 0x49, 2, OP_REG_C,   OP_REG_C,   ENC_SIMPLE },
                  { "MOV",  1, 0x49, 2, OP_REG_C,   OP_REG_C,   ENC_SIMPLE },
/* mov c,d     */ { "mov",  1, 0x4a, 2, OP_REG_C,   OP_REG_D,   ENC_SIMPLE },
                  { "MOV",  1, 0x4a, 2, OP_REG_C,   OP_REG_D,   ENC_SIMPLE },
/* mov c,e     */ { "mov",  1, 0x4b, 2, OP_REG_C,   OP_REG_E,   ENC_SIMPLE },
                  { "MOV",  1, 0x4b, 2, OP_REG_C,   OP_REG_E,   ENC_SIMPLE },
/* mov c,h     */ { "mov",  1, 0x4c, 2, OP_REG_C,   OP_REG_H,   ENC_SIMPLE },
                  { "MOV",  1, 0x4c, 2, OP_REG_C,   OP_REG_H,   ENC_SIMPLE },
/* mov c,l     */ { "mov",  1, 0x4d, 2, OP_REG_C,   OP_REG_L,   ENC_SIMPLE },
                  { "MOV",  1, 0x4d, 2, OP_REG_C,   OP_REG_L,   ENC_SIMPLE },
/* mov c,m     */ { "mov",  1, 0x4e, 2, OP_REG_C,   OP_REG_M,   ENC_SIMPLE },
                  { "MOV",  1, 0x4e, 2, OP_REG_C,   OP_REG_M,   ENC_SIMPLE },
/* mov c,a     */ { "mov",  1, 0x4f, 2, OP_REG_C,   OP_REG_A,   ENC_SIMPLE },
                  { "MOV",  1, 0x4f, 2, OP_REG_C,   OP_REG_A,   ENC_SIMPLE },
/* mov d,b     */ { "mov",  1, 0x50, 2, OP_REG_D,   OP_REG_B,   ENC_SIMPLE },
                  { "MOV",  1, 0x50, 2, OP_REG_D,   OP_REG_B,   ENC_SIMPLE },
/* mov d,c     */ { "mov",  1, 0x51, 2, OP_REG_D,   OP_REG_C,   ENC_SIMPLE },
                  { "MOV",  1, 0x51, 2, OP_REG_D,   OP_REG_C,   ENC_SIMPLE },
/* mov d,d     */ { "mov",  1, 0x52, 2, OP_REG_D,   OP_REG_D,   ENC_SIMPLE },
                  { "MOV",  1, 0x52, 2, OP_REG_D,   OP_REG_D,   ENC_SIMPLE },
/* mov d,e     */ { "mov",  1, 0x53, 2, OP_REG_D,   OP_REG_E,   ENC_SIMPLE },
                  { "MOV",  1, 0x53, 2, OP_REG_D,   OP_REG_E,   ENC_SIMPLE },
/* mov d,h     */ { "mov",  1, 0x54, 2, OP_REG_D,   OP_REG_H,   ENC_SIMPLE },
                  { "MOV",  1, 0x54, 2, OP_REG_D,   OP_REG_H,   ENC_SIMPLE },
/* mov d,l     */ { "mov",  1, 0x55, 2, OP_REG_D,   OP_REG_L,   ENC_SIMPLE },
                  { "MOV",  1, 0x55, 2, OP_REG_D,   OP_REG_L,   ENC_SIMPLE },
/* mov d,m     */ { "mov",  1, 0x56, 2, OP_REG_D,   OP_REG_M,   ENC_SIMPLE },
                  { "MOV",  1, 0x56, 2, OP_REG_D,   OP_REG_M,   ENC_SIMPLE },
/* mov d,a     */ { "mov",  1, 0x57, 2, OP_REG_D,   OP_REG_A,   ENC_SIMPLE },
                  { "MOV",  1, 0x57, 2, OP_REG_D,   OP_REG_A,   ENC_SIMPLE },
/* mov e,b     */ { "mov",  1, 0x58, 2, OP_REG_E,   OP_REG_B,   ENC_SIMPLE },
                  { "MOV",  1, 0x58, 2, OP_REG_E,   OP_REG_B,   ENC_SIMPLE },
/* mov e,c     */ { "mov",  1, 0x59, 2, OP_REG_E,   OP_REG_C,   ENC_SIMPLE },
                  { "MOV",  1, 0x59, 2, OP_REG_E,   OP_REG_C,   ENC_SIMPLE },
/* mov e,d     */ { "mov",  1, 0x5a, 2, OP_REG_E,   OP_REG_D,   ENC_SIMPLE },
                  { "MOV",  1, 0x5a, 2, OP_REG_E,   OP_REG_D,   ENC_SIMPLE },
/* mov e,e     */ { "mov",  1, 0x5b, 2, OP_REG_E,   OP_REG_E,   ENC_SIMPLE },
                  { "MOV",  1, 0x5b, 2, OP_REG_E,   OP_REG_E,   ENC_SIMPLE },
/* mov e,h     */ { "mov",  1, 0x5c, 2, OP_REG_E,   OP_REG_H,   ENC_SIMPLE },
                  { "MOV",  1, 0x5c, 2, OP_REG_E,   OP_REG_H,   ENC_SIMPLE },
/* mov e,l     */ { "mov",  1, 0x5d, 2, OP_REG_E,   OP_REG_L,   ENC_SIMPLE },
                  { "MOV",  1, 0x5d, 2, OP_REG_E,   OP_REG_L,   ENC_SIMPLE },
/* mov e,m     */ { "mov",  1, 0x5e, 2, OP_REG_E,   OP_REG_M,   ENC_SIMPLE },
                  { "MOV",  1, 0x5e, 2, OP_REG_E,   OP_REG_M,   ENC_SIMPLE },
/* mov e,a     */ { "mov",  1, 0x5f, 2, OP_REG_E,   OP_REG_A,   ENC_SIMPLE },
                  { "MOV",  1, 0x5f, 2, OP_REG_E,   OP_REG_A,   ENC_SIMPLE },
/* mov h,b     */ { "mov",  1, 0x60, 2, OP_REG_H,   OP_REG_B,   ENC_SIMPLE },
                  { "MOV",  1, 0x60, 2, OP_REG_H,   OP_REG_B,   ENC_SIMPLE },
/* mov h,c     */ { "mov",  1, 0x61, 2, OP_REG_H,   OP_REG_C,   ENC_SIMPLE },
                  { "MOV",  1, 0x61, 2, OP_REG_H,   OP_REG_C,   ENC_SIMPLE },
/* mov h,d     */ { "mov",  1, 0x62, 2, OP_REG_H,   OP_REG_D,   ENC_SIMPLE },
                  { "MOV",  1, 0x62, 2, OP_REG_H,   OP_REG_D,   ENC_SIMPLE },
/* mov h,e     */ { "mov",  1, 0x63, 2, OP_REG_H,   OP_REG_E,   ENC_SIMPLE },
                  { "MOV",  1, 0x63, 2, OP_REG_H,   OP_REG_E,   ENC_SIMPLE },
/* mov h,h     */ { "mov",  1, 0x64, 2, OP_REG_H,   OP_REG_H,   ENC_SIMPLE },
                  { "MOV",  1, 0x64, 2, OP_REG_H,   OP_REG_H,   ENC_SIMPLE },
/* mov h,l     */ { "mov",  1, 0x65, 2, OP_REG_H,   OP_REG_L,   ENC_SIMPLE },
                  { "MOV",  1, 0x65, 2, OP_REG_H,   OP_REG_L,   ENC_SIMPLE },
/* mov h,m     */ { "mov",  1, 0x66, 2, OP_REG_H,   OP_REG_M,   ENC_SIMPLE },
                  { "MOV",  1, 0x66, 2, OP_REG_H,   OP_REG_M,   ENC_SIMPLE },
/* mov h,a     */ { "mov",  1, 0x67, 2, OP_REG_H,   OP_REG_A,   ENC_SIMPLE },
                  { "MOV",  1, 0x67, 2, OP_REG_H,   OP_REG_A,   ENC_SIMPLE },
/* mov l,b     */ { "mov",  1, 0x68, 2, OP_REG_L,   OP_REG_B,   ENC_SIMPLE },
                  { "MOV",  1, 0x68, 2, OP_REG_L,   OP_REG_B,   ENC_SIMPLE },
/* mov l,c     */ { "mov",  1, 0x69, 2, OP_REG_L,   OP_REG_C,   ENC_SIMPLE },
                  { "MOV",  1, 0x69, 2, OP_REG_L,   OP_REG_C,   ENC_SIMPLE },
/* mov l,d     */ { "mov",  1, 0x6a, 2, OP_REG_L,   OP_REG_D,   ENC_SIMPLE },
                  { "MOV",  1, 0x6a, 2, OP_REG_L,   OP_REG_D,   ENC_SIMPLE },
/* mov l,e     */ { "mov",  1, 0x6b, 2, OP_REG_L,   OP_REG_E,   ENC_SIMPLE },
                  { "MOV",  1, 0x6b, 2, OP_REG_L,   OP_REG_E,   ENC_SIMPLE },
/* mov l,h     */ { "mov",  1, 0x6c, 2, OP_REG_L,   OP_REG_H,   ENC_SIMPLE },
                  { "MOV",  1, 0x6c, 2, OP_REG_L,   OP_REG_H,   ENC_SIMPLE },
/* mov l,l     */ { "mov",  1, 0x6d, 2, OP_REG_L,   OP_REG_L,   ENC_SIMPLE },
                  { "MOV",  1, 0x6d, 2, OP_REG_L,   OP_REG_L,   ENC_SIMPLE },
/* mov l,m     */ { "mov",  1, 0x6e, 2, OP_REG_L,   OP_REG_M,   ENC_SIMPLE },
                  { "MOV",  1, 0x6e, 2, OP_REG_L,   OP_REG_M,   ENC_SIMPLE },
/* mov l,a     */ { "mov",  1, 0x6f, 2, OP_REG_L,   OP_REG_A,   ENC_SIMPLE },
                  { "MOV",  1, 0x6f, 2, OP_REG_L,   OP_REG_A,   ENC_SIMPLE },
/* mov m,b     */ { "mov",  1, 0x70, 2, OP_REG_M,   OP_REG_B,   ENC_SIMPLE },
                  { "MOV",  1, 0x70, 2, OP_REG_M,   OP_REG_B,   ENC_SIMPLE },
/* mov m,c     */ { "mov",  1, 0x71, 2, OP_REG_M,   OP_REG_C,   ENC_SIMPLE },
                  { "MOV",  1, 0x71, 2, OP_REG_M,   OP_REG_C,   ENC_SIMPLE },
/* mov m,d     */ { "mov",  1, 0x72, 2, OP_REG_M,   OP_REG_D,   ENC_SIMPLE },
                  { "MOV",  1, 0x72, 2, OP_REG_M,   OP_REG_D,   ENC_SIMPLE },
/* mov m,e     */ { "mov",  1, 0x73, 2, OP_REG_M,   OP_REG_E,   ENC_SIMPLE },
                  { "MOV",  1, 0x73, 2, OP_REG_M,   OP_REG_E,   ENC_SIMPLE },
/* mov m,h     */ { "mov",  1, 0x74, 2, OP_REG_M,   OP_REG_H,   ENC_SIMPLE },
                  { "MOV",  1, 0x74, 2, OP_REG_M,   OP_REG_H,   ENC_SIMPLE },
/* mov m,l     */ { "mov",  1, 0x75, 2, OP_REG_M,   OP_REG_L,   ENC_SIMPLE },
                  { "MOV",  1, 0x75, 2, OP_REG_M,   OP_REG_L,   ENC_SIMPLE },
/* hlt         */ { "hlt",  1, 0x76, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "HLT",  1, 0x76, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* mov m,a     */ { "mov",  1, 0x77, 2, OP_REG_M,   OP_REG_A,   ENC_SIMPLE },
                  { "MOV",  1, 0x77, 2, OP_REG_M,   OP_REG_A,   ENC_SIMPLE },
/* mov a,b     */ { "mov",  1, 0x78, 2, OP_REG_A,   OP_REG_B,   ENC_SIMPLE },
                  { "MOV",  1, 0x78, 2, OP_REG_A,   OP_REG_B,   ENC_SIMPLE },
/* mov a,c     */ { "mov",  1, 0x79, 2, OP_REG_A,   OP_REG_C,   ENC_SIMPLE },
                  { "MOV",  1, 0x79, 2, OP_REG_A,   OP_REG_C,   ENC_SIMPLE },
/* mov a,d     */ { "mov",  1, 0x7a, 2, OP_REG_A,   OP_REG_D,   ENC_SIMPLE },
                  { "MOV",  1, 0x7a, 2, OP_REG_A,   OP_REG_D,   ENC_SIMPLE },
/* mov a,e     */ { "mov",  1, 0x7b, 2, OP_REG_A,   OP_REG_E,   ENC_SIMPLE },
                  { "MOV",  1, 0x7b, 2, OP_REG_A,   OP_REG_E,   ENC_SIMPLE },
/* mov a,h     */ { "mov",  1, 0x7c, 2, OP_REG_A,   OP_REG_H,   ENC_SIMPLE },
                  { "MOV",  1, 0x7c, 2, OP_REG_A,   OP_REG_H,   ENC_SIMPLE },
/* mov a,l     */ { "mov",  1, 0x7d, 2, OP_REG_A,   OP_REG_L,   ENC_SIMPLE },
                  { "MOV",  1, 0x7d, 2, OP_REG_A,   OP_REG_L,   ENC_SIMPLE },
/* mov a,m     */ { "mov",  1, 0x7e, 2, OP_REG_A,   OP_REG_M,   ENC_SIMPLE },
                  { "MOV",  1, 0x7e, 2, OP_REG_A,   OP_REG_M,   ENC_SIMPLE },
/* mov a,a     */ { "mov",  1, 0x7f, 2, OP_REG_A,   OP_REG_A,   ENC_SIMPLE },
                  { "MOV",  1, 0x7f, 2, OP_REG_A,   OP_REG_A,   ENC_SIMPLE },
/* add b       */ { "add",  1, 0x80, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "ADD",  1, 0x80, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* add c       */ { "add",  1, 0x81, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
                  { "ADD",  1, 0x81, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
/* add d       */ { "add",  1, 0x82, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "ADD",  1, 0x82, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* add e       */ { "add",  1, 0x83, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
                  { "ADD",  1, 0x83, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
/* add h       */ { "add",  1, 0x84, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "ADD",  1, 0x84, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* add l       */ { "add",  1, 0x85, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
                  { "ADD",  1, 0x85, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
/* add m       */ { "add",  1, 0x86, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
                  { "ADD",  1, 0x86, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
/* add a       */ { "add",  1, 0x87, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
                  { "ADD",  1, 0x87, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
/* adc b       */ { "adc",  1, 0x88, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "ADC",  1, 0x88, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* adc c       */ { "adc",  1, 0x89, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
                  { "ADC",  1, 0x89, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
/* adc d       */ { "adc",  1, 0x8a, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "ADC",  1, 0x8a, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* adc e       */ { "adc",  1, 0x8b, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
                  { "ADC",  1, 0x8b, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
/* adc h       */ { "adc",  1, 0x8c, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "ADC",  1, 0x8c, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* adc l       */ { "adc",  1, 0x8d, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
                  { "ADC",  1, 0x8d, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
/* adc m       */ { "adc",  1, 0x8e, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
                  { "ADC",  1, 0x8e, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
/* adc a       */ { "adc",  1, 0x8f, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
                  { "ADC",  1, 0x8f, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
/* sub b       */ { "sub",  1, 0x90, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "SUB",  1, 0x90, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* sub c       */ { "sub",  1, 0x91, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
                  { "SUB",  1, 0x91, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
/* sub d       */ { "sub",  1, 0x92, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "SUB",  1, 0x92, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* sub e       */ { "sub",  1, 0x93, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
                  { "SUB",  1, 0x93, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
/* sub h       */ { "sub",  1, 0x94, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "SUB",  1, 0x94, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* sub l       */ { "sub",  1, 0x95, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
                  { "SUB",  1, 0x95, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
/* sub m       */ { "sub",  1, 0x96, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
                  { "SUB",  1, 0x96, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
/* sub a       */ { "sub",  1, 0x97, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
                  { "SUB",  1, 0x97, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
/* sbb b       */ { "sbb",  1, 0x98, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "SBB",  1, 0x98, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* sbb c       */ { "sbb",  1, 0x99, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
                  { "SBB",  1, 0x99, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
/* sbb d       */ { "sbb",  1, 0x9a, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "SBB",  1, 0x9a, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* sbb e       */ { "sbb",  1, 0x9b, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
                  { "SBB",  1, 0x9b, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
/* sbb h       */ { "sbb",  1, 0x9c, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "SBB",  1, 0x9c, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* sbb l       */ { "sbb",  1, 0x9d, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
                  { "SBB",  1, 0x9d, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
/* sbb m       */ { "sbb",  1, 0x9e, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
                  { "SBB",  1, 0x9e, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
/* sbb a       */ { "sbb",  1, 0x9f, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
                  { "SBB",  1, 0x9f, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
/* ana b       */ { "ana",  1, 0xa0, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "ANA",  1, 0xa0, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* ana c       */ { "ana",  1, 0xa1, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
                  { "ANA",  1, 0xa1, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
/* ana d       */ { "ana",  1, 0xa2, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "ANA",  1, 0xa2, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* ana e       */ { "ana",  1, 0xa3, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
                  { "ANA",  1, 0xa3, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
/* ana h       */ { "ana",  1, 0xa4, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "ANA",  1, 0xa4, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* ana l       */ { "ana",  1, 0xa5, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
                  { "ANA",  1, 0xa5, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
/* ana m       */ { "ana",  1, 0xa6, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
                  { "ANA",  1, 0xa6, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
/* ana a       */ { "ana",  1, 0xa7, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
                  { "ANA",  1, 0xa7, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
/* xra b       */ { "xra",  1, 0xa8, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "XRA",  1, 0xa8, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* xra c       */ { "xra",  1, 0xa9, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
                  { "XRA",  1, 0xa9, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
/* xra d       */ { "xra",  1, 0xaa, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "XRA",  1, 0xaa, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* xra e       */ { "xra",  1, 0xab, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
                  { "XRA",  1, 0xab, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
/* xra h       */ { "xra",  1, 0xac, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "XRA",  1, 0xac, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* xra l       */ { "xra",  1, 0xad, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
                  { "XRA",  1, 0xad, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
/* xra m       */ { "xra",  1, 0xae, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
                  { "XRA",  1, 0xae, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
/* xra a       */ { "xra",  1, 0xaf, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
                  { "XRA",  1, 0xaf, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
/* ora b       */ { "ora",  1, 0xb0, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "ORA",  1, 0xb0, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* ora c       */ { "ora",  1, 0xb1, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
                  { "ORA",  1, 0xb1, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
/* ora d       */ { "ora",  1, 0xb2, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "ORA",  1, 0xb2, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* ora e       */ { "ora",  1, 0xb3, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
                  { "ORA",  1, 0xb3, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
/* ora h       */ { "ora",  1, 0xb4, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "ORA",  1, 0xb4, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* ora l       */ { "ora",  1, 0xb5, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
                  { "ORA",  1, 0xb5, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
/* ora m       */ { "ora",  1, 0xb6, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
                  { "ORA",  1, 0xb6, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
/* ora a       */ { "ora",  1, 0xb7, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
                  { "ORA",  1, 0xb7, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
/* cmp b       */ { "cmp",  1, 0xb8, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "CMP",  1, 0xb8, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* cmp c       */ { "cmp",  1, 0xb9, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
                  { "CMP",  1, 0xb9, 1, OP_REG_C,   OP_NONE,    ENC_SIMPLE },
/* cmp d       */ { "cmp",  1, 0xba, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "CMP",  1, 0xba, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* cmp e       */ { "cmp",  1, 0xbb, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
                  { "CMP",  1, 0xbb, 1, OP_REG_E,   OP_NONE,    ENC_SIMPLE },
/* cmp h       */ { "cmp",  1, 0xbc, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "CMP",  1, 0xbc, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* cmp l       */ { "cmp",  1, 0xbd, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
                  { "CMP",  1, 0xbd, 1, OP_REG_L,   OP_NONE,    ENC_SIMPLE },
/* cmp m       */ { "cmp",  1, 0xbe, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
                  { "CMP",  1, 0xbe, 1, OP_REG_M,   OP_NONE,    ENC_SIMPLE },
/* cmp a       */ { "cmp",  1, 0xbf, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
                  { "CMP",  1, 0xbf, 1, OP_REG_A,   OP_NONE,    ENC_SIMPLE },
/* rnz         */ { "rnz",  1, 0xc0, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RNZ",  1, 0xc0, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* pop b       */ { "pop",  1, 0xc1, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "POP",  1, 0xc1, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* jnz adr     */ { "jnz",  3, 0xc2, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "JNZ",  3, 0xc2, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* jmp adr     */ { "jmp",  3, 0xc3, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "JMP",  3, 0xc3, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* cnz adr     */ { "cnz",  3, 0xc4, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "CNZ",  3, 0xc4, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* push b      */ { "push", 1, 0xc5, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
                  { "PUSH", 1, 0xc5, 1, OP_REG_B,   OP_NONE,    ENC_SIMPLE },
/* adi d8      */ { "adi",  2, 0xc6, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
                  { "ADI",  2, 0xc6, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
/* rst0        */ { "rst0", 1, 0xc7, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RST0", 1, 0xc7, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* rz          */ { "rz",   1, 0xc8, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RZ",   1, 0xc8, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* ret         */ { "ret",  1, 0xc9, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RET",  1, 0xc9, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* jz adr      */ { "jz",   3, 0xca, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "JZ",   3, 0xca, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* cz adr      */ { "cz",   3, 0xcc, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "CZ",   3, 0xcc, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* call adr    */ { "call", 3, 0xcd, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "CALL", 3, 0xcd, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* aci d8      */ { "aci",  2, 0xce, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
                  { "ACI",  2, 0xce, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
/* rst1        */ { "rst1", 1, 0xcf, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RST1", 1, 0xcf, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* rnc         */ { "rnc",  1, 0xd0, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RNC",  1, 0xd0, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* pop d       */ { "pop",  1, 0xd1, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "POP",  1, 0xd1, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* jnc adr     */ { "jnc",  3, 0xd2, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "JNC",  3, 0xd2, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* out d8      */ { "out",  2, 0xd3, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
                  { "OUT",  2, 0xd3, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
/* cnc adr     */ { "cnc",  3, 0xd4, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "CNC",  3, 0xd4, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* push d      */ { "push", 1, 0xd5, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
                  { "PUSH", 1, 0xd5, 1, OP_REG_D,   OP_NONE,    ENC_SIMPLE },
/* sui d8      */ { "sui",  2, 0xd6, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
                  { "SUI",  2, 0xd6, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
/* rst2        */ { "rst2", 1, 0xd7, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RST2", 1, 0xd7, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* rc          */ { "rc",   1, 0xd8, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RC",   1, 0xd8, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* jc adr      */ { "jc",   3, 0xda, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "JC",   3, 0xda, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* in d8       */ { "in",   2, 0xdb, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
                  { "IN",   2, 0xdb, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
/* cc adr      */ { "cc",   3, 0xdc, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "CC",   3, 0xdc, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* sbi d8      */ { "sbi",  2, 0xde, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
                  { "SBI",  2, 0xde, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
/* rst3        */ { "rst3", 1, 0xdf, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RST3", 1, 0xdf, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* rpo         */ { "rpo",  1, 0xe0, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RPO",  1, 0xe0, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* pop h       */ { "pop",  1, 0xe1, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "POP",  1, 0xe1, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* jpo adr     */ { "jpo",  3, 0xe2, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "JPO",  3, 0xe2, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* xthl        */ { "xthl", 1, 0xe3, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "XTHL", 1, 0xe3, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* cpo adr     */ { "cpo",  3, 0xe4, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "CPO",  3, 0xe4, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* push h      */ { "push", 1, 0xe5, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
                  { "PUSH", 1, 0xe5, 1, OP_REG_H,   OP_NONE,    ENC_SIMPLE },
/* ani d8      */ { "ani",  2, 0xe6, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
                  { "ANI",  2, 0xe6, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
/* rst4        */ { "rst4", 1, 0xe7, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RST4", 1, 0xe7, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* rpe         */ { "rpe",  1, 0xe8, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RPE",  1, 0xe8, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* pchl        */ { "pchl", 1, 0xe9, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "PCHL", 1, 0xe9, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* jpe adr     */ { "jpe",  3, 0xea, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "JPE",  3, 0xea, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* xchg        */ { "xchg", 1, 0xeb, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "XCHG", 1, 0xeb, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* cpe adr     */ { "cpe",  3, 0xec, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "CPE",  3, 0xec, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* xri d8      */ { "xri",  2, 0xee, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
                  { "XRI",  2, 0xee, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
/* rst5        */ { "rst5", 1, 0xef, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RST5", 1, 0xef, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* rp          */ { "rp",   1, 0xf0, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RP",   1, 0xf0, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* pop psw     */ { "pop",  1, 0xf1, 1, OP_REG_PSW, OP_NONE,    ENC_SIMPLE },
                  { "POP",  1, 0xf1, 1, OP_REG_PSW, OP_NONE,    ENC_SIMPLE },
/* jp adr      */ { "jp",   3, 0xf2, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "JP",   3, 0xf2, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* di          */ { "di",   1, 0xf3, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "DI",   1, 0xf3, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* cp adr      */ { "cp",   3, 0xf4, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "CP",   3, 0xf4, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* push psw    */ { "push", 1, 0xf5, 1, OP_REG_PSW, OP_NONE,    ENC_SIMPLE },
                  { "PUSH", 1, 0xf5, 1, OP_REG_PSW, OP_NONE,    ENC_SIMPLE },
/* ori d8      */ { "ori",  2, 0xf6, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
                  { "ORI",  2, 0xf6, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
/* rst6        */ { "rst6", 1, 0xf7, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RST6", 1, 0xf7, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* rm          */ { "rm",   1, 0xf8, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RM",   1, 0xf8, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* sphl        */ { "sphl", 1, 0xf9, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "SPHL", 1, 0xf9, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* jm adr      */ { "jm",   3, 0xfa, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "JM",   3, 0xfa, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* ei          */ { "ei",   1, 0xfb, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "EI",   1, 0xfb, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
/* cm adr      */ { "cm",   3, 0xfc, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
                  { "CM",   3, 0xfc, 1, OP_IMM,     OP_NONE,    ENC_IMM16  },
/* cpi d8      */ { "cpi",  2, 0xfe, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
                  { "CPI",  2, 0xfe, 1, OP_IMM,     OP_NONE,    ENC_IMM8   },
/* rst7        */ { "rst7", 1, 0xff, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
                  { "RST7", 1, 0xff, 0, OP_NONE,    OP_NONE,    ENC_SIMPLE },
};

struct Ins *
getins(char *mnem, int numop, int op1, int op2)
{
    int i;

    for(i = 0;
        i < arrsize(isa);
        ++i)
    {
        if(numop == isa[i].numop &&
           !strncmp(mnem, isa[i].mnem, 8))
       {
            if(numop == 0)
            {
                return(&isa[i]);
            }
            else if(numop == 1 && op1 == isa[i].op1)
            {
                return(&isa[i]);
            }
            else if(numop == 2 && op1 == isa[i].op1 && op2 == isa[i].op2)
            {
                return(&isa[i]);
            }
       }
    }

    return(0);
}

struct Ins *
getinsbyopc(u8 opc)
{
    int i;

    for(i = 0;
        i < arrsize(isa);
        ++i)
    {
        if(isa[i].opc == opc)
        {
            return(&isa[i]);
        }
    }

    return(0);
}

char *src;
int srcl;

void
fatal(char *fmt, ...)
{
    va_list ap;

    printf("[!] ERROR on line %d ", srcl);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");

    exit(1);
}

enum
{
    T_EOF,
    T_EOL,

    T_LABEL,
    T_INTLIT,

    T_LPAREN,
    T_RPAREN,

    T_COMMA,
    T_COLON,
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,

    T_COUNT
};

char *tokstr[T_COUNT] = {
    "end of file",
    "end of line",

    "label",
    "integer literal",

    "(",
    ")",

    ",",
    ":",
    "+",
    "-",
    "*",
    "/",
};

int token;
int tokenval;
char tokenlbl[MAX_ID_LEN];

int isputback;
int putb;
int putbval;

void
putback(void)
{
    putb = token;
    putbval = tokenval;
    isputback = 1;
}

#define hexval(c) (((c) >= '0' && (c) <= '9') ? ((c) - '0') : (tolower(c) - 'a' + 10))

int
next(void)
{
    if(isputback)
    {
        token = putb;
        tokenval = putbval;
        isputback = 0;
        return(token);
    }

    while(*src && isspace(*src) && *src != '\n')
    {
        ++src;
    }

    switch(*src)
    {
        case 0:
        {
            token = T_EOF;
        } break;

        case '\n':
        {
            ++srcl;
            ++src;
            token = T_EOL;
        } break;

        case ';':
        {
            while(*src && *src != '\n')
            {
                ++src;
            }
            return(next());
        } break;

        case '.': case '_':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W': case 'X': case 'Y': case 'Z':
        {
            int i;

            i = 0;
            tokenlbl[i++] = *src++;
            while(isalnum(*src) || *src == '_')
            {
                if(i < arrsize(tokenlbl) - 1)
                {
                    tokenlbl[i++] = *src;
                }
                ++src;
            }
            tokenlbl[i] = 0;

            token = T_LABEL;
        } break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        {
            int base;
            int val;

            val = 0;
            base = 10;
            if(*src == '0' && tolower(*(src+1)) == 'x')
            {
                base = 16;
                src += 2;
            }

            if(base == 10)
            {
                while(isdigit(*src))
                {
                    val = val*base + (*src - '0');
                    ++src;
                }
            }
            else if(base == 16)
            {
                if(!isxdigit(*src))
                {
                    fatal("Invalid hexadecimal number");
                }

                while(isxdigit(*src))
                {
                    val = val*base + hexval(*src);
                    ++src;
                }
            }

            token = T_INTLIT;
            tokenval = val;
        } break;

        case '\'':
        {
            /* TODO: Escapes (\n, \t, etc...) */
            ++src;

            token = T_INTLIT;
            tokenval = (int)*src;

            ++src;
            if(*src != '\'')
            {
                fatal("Invalid byte");
            }
            ++src;
        } break;

        case '(': { token = T_LPAREN; ++src; } break;
        case ')': { token = T_RPAREN; ++src; } break;

        case ',': { token = T_COMMA; ++src; } break;
        case ':': { token = T_COLON; ++src; } break;
        case '+': { token = T_PLUS;  ++src; } break;
        case '-': { token = T_MINUS; ++src; } break;
        case '*': { token = T_STAR;  ++src; } break;
        case '/': { token = T_SLASH; ++src; } break;

        default:
        {
            fatal("Invalid token '%c'", *src);
        } break;
    }

    return(token);
}

int
peek(void)
{
    int t;

    t = next();
    putback();

    return(t);
}

void
expect(int type)
{
    int tfound;

    tfound = next();
    if(tfound != type)
    {
        fatal("Unexpected token '%s' (#%d), expected '%s' (#%d)",
              tokstr[tfound], tfound,
              tokstr[type], type);
    }
}

enum
{
    EXPR_CONST,
    EXPR_LBL,

    EXPR_MUL,
    EXPR_DIV,
    EXPR_ADD,
    EXPR_SUB,

    EXPR_COUNT
};

struct Expr
{
    int type;
    int val;
    struct Label *lbl;

    struct Expr *l;
    struct Expr *r;
};

struct Expr *
mkexpr(int type, int val, struct Expr *l, struct Expr *r)
{
    struct Expr *e;

    e = (struct Expr *)malloc(sizeof(struct Expr));
    if(e)
    {
        e->type = type;
        e->val = val;
        e->l = l;
        e->r = r;
    }

    return(e);
}

struct Expr *pexpr(void);

struct Expr *
pexprbase(void)
{
    struct Expr *e;
    int t;

    e = 0;
    t = next();
    switch(t)
    {
        case T_LPAREN:
        {
            e = pexpr();
            expect(T_RPAREN);
        } break;

        case T_PLUS:
        {
            expect(T_INTLIT);
            e = mkexpr(EXPR_CONST, tokenval, 0, 0);
        } break;

        case T_MINUS:
        {
            expect(T_INTLIT);
            e = mkexpr(EXPR_CONST, -tokenval, 0, 0);
        } break;

        case T_INTLIT:
        {
            e = mkexpr(EXPR_CONST, tokenval, 0, 0);
        } break;

        case T_LABEL:
        {
            struct Label *lbl;

            lbl = getlbl(tokenlbl);
            if(!lbl)
            {
                lbl = addlbl(tokenlbl, 0, LBL_UNSOLVED);
            }

            e = mkexpr(EXPR_LBL, 0, 0, 0);
            e->lbl = lbl;
        } break;

        default:
        {
            fatal("Invalid primary expression, invalid token '%s' (#%d)",
                tokstr[t], t);
        } break;
    }

    return(e);
}

struct Expr *
pexprfact(void)
{
    struct Expr *l;
    struct Expr *r;
    int t;

    l = pexprbase();
    t = next();
    while(t == T_STAR || t == T_SLASH)
    {
        if(t == T_STAR)
        {
            r = pexprbase();
            l = mkexpr(EXPR_MUL, 0, l, r);
        }
        else if(t == T_SLASH)
        {
            r = pexprbase();
            l = mkexpr(EXPR_DIV, 0, l, r);
        }
        t = next();
    }
    putback();

    return(l);
}

struct Expr *
pexpr(void)
{
    struct Expr *l;
    struct Expr *r;
    int t;

    l = pexprfact();
    t = next();
    while(t == T_PLUS || t == T_MINUS)
    {
        if(t == T_PLUS)
        {
            r = pexprfact();
            l = mkexpr(EXPR_ADD, 0, l, r);
        }
        else if(t == T_MINUS)
        {
            r = pexprfact();
            l = mkexpr(EXPR_SUB, 0, l, r);
        }
        t = next();
    }
    putback();

    return(l);
}

int
exprisconst(struct Expr *e)
{
    switch(e->type)
    {
        case EXPR_CONST:
        {
            return(1);
        } break;

        case EXPR_LBL:
        {
            return(e->lbl->isconst);
        } break;

        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_ADD:
        case EXPR_SUB:
        {
            return(exprisconst(e->l) && exprisconst(e->r));
        } break;

        default:
        {
            return(0);
        } break;
    }

    return(0);
}

int
eval(struct Expr *e)
{
    switch(e->type)
    {
        case EXPR_CONST:
        {
            return(e->val);
        } break;

        case EXPR_LBL:
        {
            if(e->lbl->status != LBL_RESOLVED)
            {
                fatal("Unsolved label '%s'", e->lbl->name);
            }
            return(e->lbl->addr);
        } break;

        case EXPR_MUL:
        {
            return(eval(e->l) * eval(e->r));
        } break;

        case EXPR_DIV:
        {
            return(eval(e->l) / eval(e->r));
        } break;

        case EXPR_ADD:
        {
            return(eval(e->l) + eval(e->r));
        } break;

        case EXPR_SUB:
        {
            return(eval(e->l) - eval(e->r));
        } break;

        default:
        {
            fatal("Invalid expression type");
        } break;
    }

    return(0);
}

enum
{
    LINE_NRML,
    LINE_DB,
    LINE_DW,

    LINE_COUNT
};

struct Line
{
    int type;
    struct Ins *ins;
    struct Op *op1;
    struct Op *op2;
    int line;
    unsigned int addr;
    struct Expr *expr;

    struct Line *next;
};

struct Line *lines;
struct Line *lastline;

void
addline(struct Line *l)
{
    if(l)
    {
        if(!lines)
        {
            lines = l;
        }
        if(lastline)
        {
            lastline->next = l;
        }
        lastline = l;
    }
}

struct Line *
mkline(
    int type,
    struct Ins *ins, struct Op *op1, struct Op *op2,
    int line, unsigned int addr,
    struct Expr *expr)
{
    struct Line *l;

    l = (struct Line *)malloc(sizeof(struct Line));
    if(l)
    {
        l->type = type;
        l->ins = ins;
        l->op1 = op1;
        l->op2 = op2;
        l->line = line;
        l->addr = addr;
        l->next = 0;
        l->expr = expr;
    }

    return(l);
}

struct Op *
poperand(void)
{
    struct Op *op;
    int t;

    op = 0;
    t = peek();
    switch(t)
    {
        case T_LPAREN:
        case T_INTLIT:
        case T_PLUS:
        case T_MINUS:
        {
            op = mkoperand(OP_IMM, pexpr());
        } break;

        case T_LABEL:
        {
            if(!strcmp(tokenlbl, "a") || !strcmp(tokenlbl, "A"))
            {
                op = mkoperand(OP_REG_A, 0);
                next();
            }
            else if(!strcmp(tokenlbl, "b") || !strcmp(tokenlbl, "B"))
            {
                op = mkoperand(OP_REG_B, 0);
                next();
            }
            else if(!strcmp(tokenlbl, "c") || !strcmp(tokenlbl, "C"))
            {
                op = mkoperand(OP_REG_C, 0);
                next();
            }
            else if(!strcmp(tokenlbl, "d") || !strcmp(tokenlbl, "D"))
            {
                op = mkoperand(OP_REG_D, 0);
                next();
            }
            else if(!strcmp(tokenlbl, "e") || !strcmp(tokenlbl, "E"))
            {
                op = mkoperand(OP_REG_E, 0);
                next();
            }
            else if(!strcmp(tokenlbl, "h") || !strcmp(tokenlbl, "H"))
            {
                op = mkoperand(OP_REG_H, 0);
                next();
            }
            else if(!strcmp(tokenlbl, "l") || !strcmp(tokenlbl, "L"))
            {
                op = mkoperand(OP_REG_L, 0);
                next();
            }
            else if(!strcmp(tokenlbl, "m") || !strcmp(tokenlbl, "M"))
            {
                op = mkoperand(OP_REG_M, 0);
                next();
            }
            else if(!strcmp(tokenlbl, "sp") || !strcmp(tokenlbl, "SP"))
            {
                op = mkoperand(OP_REG_SP, 0);
                next();
            }
            else if(!strcmp(tokenlbl, "psw") || !strcmp(tokenlbl, "PSW"))
            {
                op = mkoperand(OP_REG_PSW, 0);
                next();
            }
            else
            {
                op = mkoperand(OP_IMM, pexpr());
            }
        } break;
    }

    return(op);
}

unsigned int addr;

struct Line *
pline(void)
{
    struct Line *line;
    struct Ins *ins;
    struct Op *op1;
    struct Op *op2;
    int numop;
    int t;

    ins = 0;
    line = 0;
    op1 = 0;
    op2 = 0;
    numop = 0;
    t = peek();
    switch(t)
    {
        case T_EOF:
        {
            line = 0;
        } break;

        case T_EOL:
        {
            line = 0;
            next();
        } break;

        case T_LABEL:
        {
            char name[MAX_ID_LEN];

            next();
            strncpy(name, tokenlbl, MAX_ID_LEN);

            if(!strcmp(name, "db") || !strcmp(name, "DB"))
            {
                struct Expr *expr;

                do
                {
                    expr = pexpr();
                    if(!exprisconst(expr))
                    {
                        fatal("Invalid constant expression for directive 'DB'");
                    }
                    line = mkline(LINE_DB, 0, 0, 0, srcl, addr, expr);
                    addline(line);
                    addr += 1;
                    t = peek();
                    if(t == T_COMMA)
                    {
                        next();
                    }
                }
                while(t == T_COMMA);
            }
            else if(!strcmp(name, "dw") || !strcmp(name, "DW"))
            {
                struct Expr *expr;

                do
                {
                    expr = pexpr();
                    if(!exprisconst(expr))
                    {
                        fatal("Invalid constant expression for directive 'DW'");
                    }
                    line = mkline(LINE_DW, 0, 0, 0, srcl, addr, expr);
                    addline(line);
                    addr += 2;
                    t = peek();
                    if(t == T_COMMA)
                    {
                        next();
                    }
                }
                while(t == T_COMMA);
            }
            else
            {
                t = peek();
                if(t == T_COLON)
                {
                    struct Label *lbl;

                    next();
                    lbl = getlbl(name);
                    if(lbl)
                    {
                        lbl->addr = addr;
                        lbl->status = LBL_RESOLVED;
                    }
                    else
                    {
                        addlbl(name, addr, LBL_RESOLVED);
                    }

                    t = peek();
                    if(t == T_EOL)
                    {
                        expect(T_EOL);
                    }
                    line = pline();
                }
                else if(t == T_LABEL && (!strcmp(tokenlbl, "equ") || !strcmp(tokenlbl, "EQU")))
                {
                    struct Label *lbl;
                    struct Expr *expr;

                    next();
                    lbl = getlbl(name);
                    if(lbl && lbl->status == LBL_RESOLVED)
                    {
                        fatal("Label '%s' already defined", name);
                    }
                    else if(!lbl)
                    {
                        lbl = addlbl(name, 0, LBL_UNSOLVED);
                    }

                    expr = pexpr();
                    if(!exprisconst(expr))
                    {
                        fatal("Invalid constant expression for directive 'EQU'");
                    }

                    lbl->addr = eval(expr);
                    lbl->status = LBL_RESOLVED;
                    lbl->isconst = 1;

                    expect(T_EOL);
                }
                else
                {
                    op1 = poperand();
                    if(op1)
                    {
                        ++numop;
                        if(peek() == T_COMMA)
                        {
                            next();
                            op2 = poperand();
                            if(op2)
                            {
                                ++numop;
                            }
                        }
                    }

                    if(!strcmp(name, "org") || !strcmp(name, "ORG"))
                    {
                        if(numop != 1)
                        {
                            fatal("Pseudo-instruction 'org' accepts only one argument");
                        }

                        if(op1->type != OP_IMM || !exprisconst(op1->expr))
                        {
                            fatal("Pseudo-instruction 'org' must have a constant as argument");
                        }

                        addr = eval(op1->expr);
                    }
                    else
                    {
                        if(numop == 0)
                        {
                            ins = getins(name, numop, OP_NONE, OP_NONE);
                        }
                        else if(numop == 1)
                        {
                            ins = getins(name, numop, op1->type, OP_NONE);
                        }
                        else
                        {
                            ins = getins(name, numop, op1->type, op2->type);
                        }

                        if(!ins)
                        {
                            fatal("Invalid instruction '%s'", name);
                        }

                        line = mkline(LINE_NRML, ins, op1, op2, srcl, addr, 0);
                        addline(line);
                        addr += ins->size;
                    }

                    expect(T_EOL);
                }
            }
        } break;

        default:
        {
            fatal("Invalid token '%s' (#%d)", tokstr[t], t);
        } break;
    }

    return(line);
}

void
pprogram(void)
{
    int t;

    t = peek();
    while(t != T_EOF)
    {
        pline();
        t = peek();
    }
    expect(T_EOF);
}

u8 code[0x1000];
int codesz;

void
emit(FILE *f, u8 b)
{
    fputc((int)b, f);
    ++codesz;
}

void
emitw(FILE *f, u16 w)
{
    emit(f, (u8)((w & 0x00ff) >> 0));
    emit(f, (u8)((w & 0xff00) >> 8));
}

#define i8tou8(i)   ((u8)((((i)&0x80))?((int)(i&0xff)):(i)))
#define i16tou16(i) ((u16)((((i)&0x8000))?((int)(i&0xffff)):(i)))

void
emitcode(FILE *fout)
{
    struct Line *line;
    struct Ins *ins;

    line = lines;
    while(line)
    {
        ins = line->ins;
        srcl = line->line;

        if(line->type == LINE_DB)
        {
            i8 val = (i8)eval(line->expr);
            emit(fout, i8tou8(val));
        }
        else if(line->type == LINE_DW)
        {
            i16 val = (i16)eval(line->expr);
            emitw(fout, i16tou16(val));
        }
        else if(line->type == LINE_NRML)
        {
            assert(ins);
            switch(ins->enc)
            {
                case ENC_SIMPLE:
                {
                    emit(fout, ins->opc);
                } break;

                case ENC_IMM8:
                {
                    int val;

                    emit(fout, ins->opc);
                    if(line->op1->type == OP_IMM)
                    {
                        val = eval(line->op1->expr);
                    }
                    else
                    {
                        val = eval(line->op2->expr);
                    }
                    if(val > 255 || val < -128)
                    {
                        fatal("Value %d cannot fit 8 bit", val);
                    }
                    val = val & 0xff;
                    emit(fout, i8tou8(val));
                } break;

                case ENC_IMM16:
                {
                    int val;

                    emit(fout, ins->opc);
                    if(line->op1->type == OP_IMM)
                    {
                        val = eval(line->op1->expr);
                    }
                    else
                    {
                        val = eval(line->op2->expr);
                    }
                    if(val > 65536 || val < -32768)
                    {
                        fatal("Value %d cannot fit 16 bit", val);
                    }
                    val = val & 0xffff;

                    printf("DEBUG val: %d %d\n", (int)val, (unsigned int)i16tou16(val));

                    emitw(fout, i16tou16(val));
                } break;

                default:
                {
                    fatal("Invalid encryption");
                } break;
            }
        }

        line = line->next;
    }
}

#undef i8tou8
#undef i16tou16

int
printop(int op, char **out)
{
    int printed;

    printed = 0;
    switch(op)
    {
        case OP_REG_A:   { *out += sprintf(*out, "a");   printed = 1; } break;
        case OP_REG_B:   { *out += sprintf(*out, "b");   printed = 1; } break;
        case OP_REG_C:   { *out += sprintf(*out, "c");   printed = 1; } break;
        case OP_REG_D:   { *out += sprintf(*out, "d");   printed = 1; } break;
        case OP_REG_E:   { *out += sprintf(*out, "e");   printed = 1; } break;
        case OP_REG_H:   { *out += sprintf(*out, "h");   printed = 1; } break;
        case OP_REG_L:   { *out += sprintf(*out, "l");   printed = 1; } break;
        case OP_REG_M:   { *out += sprintf(*out, "m");   printed = 1; } break;
        case OP_REG_SP:  { *out += sprintf(*out, "sp");  printed = 1; } break;
        case OP_REG_PSW: { *out += sprintf(*out, "psw"); printed = 1; } break;

        default:
        {
            printed = 0;
        } break;
    }

    return(printed);
}

int
disassemble(u8 **in, unsigned int insz, char *out)
{
    struct Ins *ins;
    int size;

    size = 1;
    if(insz == 0)
    {
        *out = 0;
    }
    else
    {
        ins = getinsbyopc(**in);
        ++(*in);
        if(ins)
        {
            out += sprintf(out, "%s", ins->mnem);
            if(ins->numop > 0)
            {
                out += sprintf(out, " ");
                if(!printop(ins->op1, &out))
                {
                    if(ins->enc == ENC_IMM8)
                    {
                        size = 2;
                        if(insz > 1)
                        {
                            out += sprintf(out, "0x%.2x", (unsigned int)**in);
                            ++(*in);
                        }
                    }
                    else if(ins->enc == ENC_IMM16)
                    {
                        size = 3;
                        if(insz > 2)
                        {
                            out += sprintf(out, "0x%.4x", *((u16 *)(*in)));
                            *in += 2;
                        }
                    }
                }
            }
            if(ins->numop > 1)
            {
                out += sprintf(out, ",");
                if(!printop(ins->op2, &out))
                {
                    if(ins->enc == ENC_IMM8)
                    {
                        size = 2;
                        if(insz > 1)
                        {
                            out += sprintf(out, "0x%.2x", (unsigned int)**in);
                            ++(*in);
                        }
                    }
                    else if(ins->enc == ENC_IMM16)
                    {
                        size = 3;
                        if(insz > 2)
                        {
                            out += sprintf(out, "0x%.4x", *((u16 *)(*in)));
                            *in += 2;
                        }
                    }
                }
            }
        }
        else
        {
            out += sprintf(out, "-");
        }
    }
    *out = 0;

    return(size);
}

int
assemble(char *fnamein, char *fnameout)
{
    FILE *fin;
    FILE *fout;
    int i;
    char *_src;

    if(!fnamein || !fnameout)
    {
        return(1);
    }

    fin = fopen(fnamein, "r");
    if(!fin)
    {
        return(2);
    }
    fseek(fin, 0, SEEK_END);
    i = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    _src = (char*)malloc(i+1);
    if(!_src)
    {
        return(3);
    }
    i = fread(_src, 1, i, fin);
    _src[i] = 0;
    fclose(fin);

    fout = fopen(fnameout, "wb");
    if(!fout)
    {
        return(4);
    }
    src = _src;
    srcl = 1;
    addr = 0;
    /* Parsing + first pass */
    pprogram();
    for(i = 0;
        i < lblscount;
        ++i)
    {
        printf("%s: %.4x\n", lbls[i].name, lbls[i].addr);
    }

    /* Second pass (emitting code) */
    emitcode(fout);
    fclose(fout);

    return(0);
}

#ifdef I8080ASM_STANDALONE

int
main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("Usage: %s <source> <output>\n", argv[0]);
    }

    return(assemble(argv[1], argv[2]));
}

#endif
