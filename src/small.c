/**
 * This is a compiler for a subset of C targeting i8080 CPU.
 * The goal is to make a small compiler (small in size and
 * that uses less memory) which can compile itself.
 *
 * Types:
 * [ ] char (1 byte)
 * [ ] int  (2 bytes)
 * [ ] pointer (only one level)
 *
 * No struct, no unions. Just char, ints, pointers, enums.
 * [ ] enum
 *
 * Control structures:
 * [x] if-else
 * [x] while
 *
 * Calling convention:
 * When a function is called, first of all arguments are passed on
 * the stack in their respective order and then the function is called.
 *   - push(arg1)
 *   - push(arg2)
 *   - ...
 *   - call func  <- HL points here
 * The HL register will point to the address where return address is stored
 * throughout the whole function. It's like an BP register.
 *
 * Local variables are
 *
 */

/**
 * TODO(driverfury):
 *
 * [ ] We must subtract from HL (which is our BP) not add.
 * [ ] What about an identifier 'A' or 'B', it can be confused
 *     with registers of i8080. Put a '$' or '#' at the start
 *     of an immediate value??
 */

#include <stdio.h>
#include <assert.h>

FILE *fin;              /* input file handle */
FILE *fout;             /* output file handle */

int tk;                 /* current token */
int tkval;              /* token value */
char id[32];            /* max id len: 31 (last char in null) */

int pbtk;               /* token putback */
int pbtkval;
char pbid[32];
int pb;

int lblcount;           /* counter for unique lbl generation */

/**
 * NOTE(driverfury): Symbol table
 * Every record is organized like so:
 * 1. a char which indicates if it's a function or a variable
 * 2. a char which indicates the offset from EBP where the local var is
 * 3. a char which indicates the type
 * 4. the number of params
 * 5. a null-terminated string (identifier).
 *
 * The 1st char can assume the following values:
 *    0  => invalid
 *   'g' => global variable
 *   'f' => function (indeed global)
 *   'l' => local variable
 *
 * The 3rd char can assume the following value:
 *   0   => invalid
 *  'c'  => char
 *  'i'  => int
 *
 * symp is offset from symtbl to the next empty table entry
 */
char symtbl[0x1000];
int symp;

int foff;               /* offset for the next local variable */

void
fatal(char *s)
{
    printf(s);
    printf("\n");
    assert(0);
}

int
strl(char *s)
{
    int i;

    i = 0;
    while(*s)
    {
        ++s;
        ++i;
    }

    return(i);
}

int
streq(char *s1, char *s2)
{
    int i;

    i = 0;
    while(*s1 && *s2)
    {
        if(*s1 != *s2)
        {
            return(0);
        }
        ++s1;
        ++s2;
        ++i;
    }

    if(*s1 == *s2)
    {
        return(1);
    }

    return(0);
}

void
mcpy(char *src, char *dst, int len)
{
    int i;

    i = 0;
    while(i < len)
    {
        *dst = *src;
        ++src;
        ++dst;
        ++i;
    }
}

char *
addsym(
    char *id, char forv, char offset,
    char type, char numparams)
{
    char *res;

    res = symtbl + symp;
    symtbl[symp++] = forv;
    symtbl[symp++] = offset;
    symtbl[symp++] = type;
    symtbl[symp++] = numparams;
    while(*id)
    {
        symtbl[symp++] = *id;
        ++id;
    }
    symtbl[symp++] = 0;

    return(res);
}

char *
getsym(char *symid)
{
    char *sym;
    int i;

    sym = 0;
    i = 0;
    while(i < symp)
    {
        i += 4;
        if(streq(symid, &symtbl[i]))
        {
            /**
             * NOTE(driverfury): We don't break the loop since
             * we need to get the last occurrence of that symbol
             */
            sym = &symtbl[i - 4];
        }
        i += strl(&symtbl[i]) + 1;
    }

    return(sym);
}

enum
{
    T_EOF,        /*  0 */
    T_INTLIT,     /*  1 */
    T_ID,         /*  2 */

    T_LPAREN,     /*  3 */
    T_RPAREN,     /*  4 */
    T_LBRACE,     /*  5 */
    T_RBRACE,     /*  6 */
    T_LBRACK,     /*  7 */
    T_RBRACK,     /*  8 */

    T_SEMI,       /*  9 */
    T_COLON,      /* 10 */

    T_PLUS,       /* 11 */
    T_MINUS,      /* 12 */
    T_STAR,       /* 13 */
    T_SLASH,      /* 14 */

    T_EQ,         /* 15 */

    T_INT,        /* 16 */
    T_CHAR,       /* 17 */
    T_IF,         /* 18 */
    T_ELSE,       /* 19 */
    T_WHILE,      /* 20 */

    T_COUNT
};

void
emit(char *s)
{
    fprintf(fout, s);
}

void
emitint(int n)
{
    char buff[8];
    int i;
    int div;
    int tmp;

    n = n & 0xffff;
    if(n == 0)
    {
        emit("0");
    }
    else
    {
        div = 0x10000;
        while(n / div == 0)
        {
            div = div / 0x10;
        }
        i = 0;
        buff[i++] = '0';
        buff[i++] = 'x';
        while(div > 0)
        {
            tmp = n/div;
            if(tmp < 10) {
                buff[i++] = '0' + tmp;
            } else {
                buff[i++] = 'A' + (tmp - 10);
            }
            n -= tmp*div;
            div = div / 0x10;
        }
        buff[i++] = 0;
        emit(buff);
    }
}

void
genlbl(char *lbl)
{
    int n;
    int i;
    int div;
    int tmp;

    n = lblcount;

    i = 0;
    lbl[i++] = '.';
    lbl[i++] = 'L';

    if(n == 0)
    {
        lbl[i++] = '0';
    }
    else
    {
        div = 0x10000000;
        while(n / div == 0)
        {
            div = div / 0x10;
        }
        while(div > 0)
        {
            tmp = n/div;
            if(tmp < 10) {
                lbl[i++] = '0' + tmp;
            } else {
                lbl[i++] = 'A' + (tmp - 10);
            }
            n -= tmp*div;
            div = div / 0x10;
        }
    }
    lbl[i++] = 0;

    ++lblcount;
}

void
putback()
{
    pb = 1;
    pbtk = tk;
    pbtkval = tkval;
    mcpy(id, pbid, 32);
}

int
next()
{
    char c;
    int i;

    if(pb)
    {
        tk = pbtk;
        tkval = pbtkval;
        mcpy(pbid, id, 32);
        pb = 0;
        return(tk);
    }

    if(feof(fin))
    {
        tk = T_EOF;
    }
    else
    {
        c = fgetc(fin);
        while(c ==  ' ' || c == '\t' || c == '\v' ||
              c == '\r' || c == '\f' || c == '\n')
        {
            c = fgetc(fin);
        }

        if(c == EOF)
        {
            tk = T_EOF;
        }
        else if((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c == '_'))
        {
            tk = T_ID;
            i = 0;
            while((c >= 'a' && c <= 'z') ||
                  (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') ||
                  (c == '_'))
            {
                if(i < 31)
                {
                    id[i++] = c;
                }
                id[i] = 0;
                c = fgetc(fin);
            }
            ungetc(c, fin);

                 if(streq(id, "int"))   { tk = T_INT; }
            else if(streq(id, "char"))  { tk = T_CHAR; }
            else if(streq(id, "if"))    { tk = T_IF; }
            else if(streq(id, "else"))  { tk = T_ELSE; }
            else if(streq(id, "while")) { tk = T_WHILE; }
        }
        else if(c >= '0' && c <= '9')
        {
            tk = T_INTLIT;
            tkval = 0;

            while(c >= '0' && c <= '9')
            {
                tkval = tkval*10 + (c - '0');
                c = fgetc(fin);
            }
            ungetc(c, fin);
        }
        else if(c == ';') { tk = T_SEMI; }
        else if(c == ':') { tk = T_COLON; }
        else if(c == '(') { tk = T_LPAREN; }
        else if(c == ')') { tk = T_RPAREN; }
        else if(c == '{') { tk = T_LBRACE; }
        else if(c == '}') { tk = T_RBRACE; }
        else if(c == '[') { tk = T_LBRACK; }
        else if(c == ']') { tk = T_RBRACK; }
        else if(c == '+') { tk = T_PLUS; }
        else if(c == '-') { tk = T_MINUS; }
        else if(c == '*') { tk = T_STAR; }
        else if(c == '/') { tk = T_SLASH; }
        else if(c == '=') { tk = T_EQ; }
        else
        {
            fatal("Invalid token");
        }
    }

    return(tk);
}

int
peek()
{
    int t;
    t = next();
    putback();
    return(t);
}

void
expect(int t)
{
    int found;

    found = next();
    if(found != t)
    {
        printf("Expected %d, found %d\n", t, found);
        fatal("Unexpected token");
    }
}

/**
 * NOTE(driverfury): This function returns the precedence of an operator.
 * The lower the value returned, the lower the precedence.
 */

int
prec(int t)
{
         if(t == T_STAR || t == T_SLASH) { return(20); }
    else if(t == T_PLUS || t == T_MINUS) { return(10); }

    return(-1);
}

/**
 * NOTE(driverfury): Expressions syntax
 *
 * exprbase ::= T_INTLIT
 *            | T_ID
 *            | '(' expr ')'
 *
 * exprun ::= exprbase
 *          | '+' exprbase
 *          | '-' exprbase
 *
 * exprbin ::= exprun
 *           | exprun '*' exprun
 *           | exprun '/' exprun
 *           | exprun '+' exprun
 *           | exprun '-' exprun
 *
 * exprassign ::= T_ID '=' expr
 *
 * expr ::= exprassign
 */

void expr();

void
exprbase()
{
    if(tk == T_INTLIT)
    {
        emit("\tLXI B,");
        emitint(tkval);
        emit("\n");
        emit("\tPUSH B\n");
        next();
    }
    else if(tk == T_ID)
    {
        char *sym;

        sym = getsym(id);
        if(!sym)
        {
            fatal("Undefined symbol exprbase");
        }
        if(sym[0] != 'g' && sym[0] != 'l')
        {
            fatal("Symbol is not a variable");
        }

        emit("\tPUSH H\n");
        if(sym[0] == 'g')
        {
            emit("\tLHLD ");
            emit(id);
            emit("\n");
        }
        else
        {
            /* TODO: We must subtract the offset, not add */
            emit("\tLXI D,");
            emitint(sym[1]);
            emit("\n");
            emit("\tDAD D\n");
            emit("\tMOV E,M\n");
            emit("\tINX H\n");
            emit("\tMOV D,M\n");
            emit("\tXCHG\n");
        }
        emit("\tMOV B,H\n");
        emit("\tMOV C,L\n");
        emit("\tPOP H\n");
        emit("\tPUSH B\n");

        next();
    }
    else if(tk == T_LPAREN)
    {
        next();
        expr();
        if(tk != T_RPAREN)
        {
            fatal("Matching ')' not found");
        }
        next();
    }
    else
    {
        fatal("Invalid base expression");
    }
}

void
exprun()
{
    if(tk == T_MINUS)
    {
        next();
        exprbase();
        emit("\tPOP B\n");
        emit("\tXRA A\n");
        emit("\tSUB C\n");
        emit("\tMOV C,A\n");
        emit("\tXRA A\n");
        emit("\tSBB B\n");
        emit("\tMOV B,A\n");
        emit("\tPUSH B\n");
    }
    else if(tk == T_PLUS)
    {
        next();
        exprbase();
    }
    else
    {
        exprbase();
    }
}

void
exprbin(int p)
{
    int t;

    /* left operand: store the result on the stack */
    exprun();
    
    /*
     * parse the (optional) right operand, compute the operation and put it
     * on top of the stack
     */
    while(prec(tk) >= p)
    {
        t = tk;

        next();
        exprbin(prec(t));
        emit("\tPOP B\n"); /* right op */
        emit("\tPOP D\n"); /* left op */

        if(t == T_PLUS)
        {
            emit("\tMOV A,E\n");
            emit("\tADD C\n"); 
            emit("\tMOV C,A\n"); 
            emit("\tMOV A,D\n");
            emit("\tADC B\n"); 
            emit("\tMOV B,A\n"); 
            emit("\tPUSH B\n"); 
        }
        else if(t == T_MINUS)
        {
            /* TODO */
            /* This is only a placeholder */
            emit("\tSUB B,D\n");
            emit("\tPUSH B\n"); 
        }
        else if(t == T_STAR)
        {
            /* TODO */
            /* This is only a placeholder */
            emit("\tMUL B,D\n");
            emit("\tPUSH B\n"); 
        }
        else if(t == T_SLASH)
        {
            /* TODO */
            /* This is only a placeholder */
            emit("\tDIV B,D\n");
            emit("\tPUSH B\n"); 
        }
        else
        {
            fatal("Invalid expression");
        }
    }
}

void
exprassign()
{
    int oldtk;
    int oldtkval;
    char oldid[32];

    if(tk == T_ID)
    {
        char *sym;

        sym = getsym(id);
        if(!sym)
        {
            fatal("Undefined symbol in exprassign");
        }
        if(sym[0] != 'g' && sym[0] != 'l')
        {
            fatal("Symbol is not a variable");
        }

        oldtk = tk;
        oldtkval = tkval;
        mcpy(id, oldid, 32);

        if(next() == T_EQ)
        {
            next();
            expr();
            emit("\tPOP B\n");
            emit("\tPUSH H\n");
            if(sym[0] == 'g')
            {
                emit("\tLXI H,");
                emit(oldid);
                emit("\n");
            }
            else
            {
                /* TODO: We must subtract the offset, not add */
                emit("\tLXI D,");
                emitint(sym[1]);
                emit("\n");
                emit("\tDAD D\n");
            }
            emit("\tMOV A,C\n");
            emit("\tSTAX H\n");
            emit("\tINX H\n");
            emit("\tMOV A,B\n");
            emit("\tSTAX H\n");
            emit("\tPOP H\n");
            emit("\tPUSH B\n");
        }
        else
        {
            putback();
            tk = oldtk;
            tkval = oldtkval;
            mcpy(oldid, id, 32);
            exprbin(0);
        }
    }
    else
    {
        exprbin(0);
    }
}

void
expr()
{
    exprassign();
}

/**
 * NOTE(driverfury): Statements syntax
 *
 * stmt ::= stmtblk
 *        | (T_CHAR | T_INT) T_ID ';'
 *        | T_IF '(' expr ')' stmt [T_ELSE stmt]?
 *        | T_WHILE '(' expr ')' stmt
 *        | expr ';'
 *
 * stmtblk ::= '{' [stmt]* '}
 */

void stmtblk();

void
stmt()
{
    if(tk == T_LBRACE)
    {
        stmtblk();
    }
    else if(tk == T_INT || tk == T_CHAR)
    {
        char type;
        char size;

             if(tk == T_INT)  { type = 'i'; size = 2; }
        else if(tk == T_CHAR) { type = 'c'; size = 2; }

        expect(T_ID);

        addsym(id, 'l', foff, type, 0);
        foff += size;

        expect(T_SEMI);
        next();
    }
    else if(tk == T_IF)
    {
        char ifblk[32];
        char elseblk[32];
        char endif[32];

        genlbl(ifblk);
        genlbl(elseblk);
        genlbl(endif);

        expect(T_LPAREN);
        next();
        expr();
        if(tk != T_RPAREN)
        {
            fatal("Invalid if-condition");
        }
        next();

        emit("\tPOP B\n");
        emit("\tMOV A,B\n");
        emit("\tORA A\n");
        emit("\tJNZ ");
        emit(ifblk);
        emit("\n");
        emit("\tMOV A,C\n");
        emit("\tORA A\n");
        emit("\tJNZ ");
        emit(ifblk);
        emit("\n");
        emit("\tJMP ");
        emit(elseblk);
        emit("\n");

        /* ifblk: */
        emit(ifblk);
        emit(":\n");
        stmt();

        /* elseblk: */
        emit(elseblk);
        emit(":\n");
        if(tk == T_ELSE)
        {
            next();
            stmt();
        }

        /* endif: */
        emit(endif);
        emit(":\n");
    }
    else if(tk == T_WHILE)
    {
        char cond[32];
        char blk[32];
        char endwhl[32];

        genlbl(cond);
        genlbl(blk);
        genlbl(endwhl);

        /* cond: */
        emit(cond);
        emit(":\n");
        expect(T_LPAREN);
        next();
        expr();
        if(tk != T_RPAREN)
        {
            fatal("Invalid while-condition");
        }
        next();
        emit("\tPOP B\n");
        emit("\tMOV A,B\n");
        emit("\tORA A\n");
        emit("\tJNZ ");
        emit(blk);
        emit("\n");
        emit("\tMOV A,C\n");
        emit("\tORA A\n");
        emit("\tJNZ ");
        emit(blk);
        emit("\n");
        emit("\tJMP ");
        emit(endwhl);
        emit("\n");

        /* blk: */
        emit(blk);
        emit(":\n");
        stmt();
        emit("\tJMP ");
        emit(cond);
        emit("\n");

        /* endwhl: */
        emit(endwhl);
        emit(":\n");
    }
    else
    {
        expr();
        emit("\tPOP B\n");

        if(tk != T_SEMI)
        {
            fatal("Expected semi-colon ';' at the end of the statement");
        }
        next();
    }
}

void
stmtblk()
{
    if(tk != T_LBRACE)
    {
        fatal("Invalid block statement (missing '{')");
    }

    next();
    while(tk != T_RBRACE)
    {
        if(tk == T_EOF)
        {
            fatal("Invalid block statement (missing '}')");
        }
        stmt();
    }
    next();
}

void
prog()
{
    char forv;
    char type;
    char numparams;

    forv = 0;
    type = 0;
    numparams = 0;

    emit("\tORG 0x100\n\n");
    emit("\tJMP main\n\n");
    next();
    while(tk != T_EOF)
    {
        if(tk == T_INT || tk == T_CHAR)
        {
            char symid[32];

                 if(tk == T_INT)  { type = 'i'; }
            else if(tk == T_CHAR) { type = 'c'; }

            expect(T_ID);
            mcpy(id, symid, 32);
            emit(id);
            emit(":\n");
            next();
            if(tk == T_LPAREN)
            {
                forv = 'f';
                /* TODO(driverfury): parse params */
                expect(T_RPAREN);
                next();
                foff = 2;
                stmtblk();
            }
            else
            {
                forv = 'g';
                emit("\tDW 0\n");
                if(tk != T_SEMI)
                {
                    fatal("Invalid variable declaration");
                }
                next();
            }

            addsym(symid, forv, 0, type, numparams);
        }
        else
        {
            fatal("Invalid token");
        }
    }
}

int
main(int argc, char *argv[])
{
    fin = fopen("test.c", "r");
    fout = fopen("test.out", "w");

    prog();

    fclose(fout);
    fclose(fin);
#if 0
    emit("CIAO");
    emit(" AMICO ");
    emitint(0x800);
    emit(" ");
    emitint(0x4122);
#endif
    return(0);
}
