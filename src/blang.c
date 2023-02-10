/**

SYNTAX:

<prog> := <gdecl>+

<gdecl> := 'var' <ident>
         | <ident> '(' ')' <sblock>

<sblock> := '{' <stmt>* '}'

<stmt> := <decl>
        | <sif>
        | <swhile>
        | <sblock>
        | <ident> '=' <expr>
        | <ident> '(' ')'

<expr> := <exprbin>

<exprbin> := <exprbase>
           | <expr> <op> <exprbase>

<op> := '+' | '-'
<ident> := [_A-z]+[_A-z0-9]*

*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void
error(char *s)
{
    fprintf(stderr, "%s", s);
    exit(1);
}

#define MAXIDLEN 32
#define MAXSYMS 100
#define GSYMOFF -100000
#define FSYMOFF -100001
char symids[MAXSYMS][MAXIDLEN+1];
int symoff[MAXSYMS];
int symnum;

#define SYMISFUNC(i) (symoff[(i)] == FSYMOFF)
#define SYMISGLOB(i) (symoff[(i)] == GSYMOFF)
#define SYMISLOC(i) (!SYMISFUNC(i) && !SYMISGLOB(i))

int curroff;

int
addsym(char *id)
{
    int i;

    if(symnum == MAXSYMS)
    {
        error("Too many symbols");
    }

    i = symnum;
    strncpy(symids[symnum], id, MAXIDLEN);
    symids[symnum][MAXIDLEN] = 0;
    ++symnum;

    return(i);
}

int
getsym(char *id)
{
    int i;

    for(i = symnum - 1;
        i >= 0;
        --i)
    {
        if(!strcmp(id, symids[i]))
        {
            return(i);
        }
    }

    return(-1);
}

enum
{
    T_ID = 128,
    T_INT,

    T_KVAR,
    T_KIF,
    T_KWHILE,
    T_KRETURN
};

FILE *fin;
int tk;
int intval;
char buf[MAXIDLEN + 1];
int putbkc;

void
putback(int c)
{
    if(isspace(c))
    {
        putbkc = 0;
    }
    else
    {
        putbkc = c;
    }
}

void
next()
{
    int c, i = 0;

    if(putbkc)
    {
        tk = putbkc;
        putbkc = 0;
        return;
    }

    c = fgetc(fin);
    while(isspace(c))
    {
        c = fgetc(fin);
    }

    if(c == EOF)
    {
        tk = 0;
        return;
    }

    if(isalpha(c) || c == '_')
    {
        tk = T_ID;
        while((isalnum(c) || c == '_') && i < MAXIDLEN)
        {
            buf[i++] = c;
            c = fgetc(fin);
        }
        putback(c);
        buf[i] = 0;

        if(!strcmp(buf, "var"))    tk = T_KVAR;
        if(!strcmp(buf, "if"))     tk = T_KIF;
        if(!strcmp(buf, "while"))  tk = T_KWHILE;
        if(!strcmp(buf, "return")) tk = T_KRETURN;
    }
    else if(isdigit(c))
    {
        tk = T_INT;
        intval = 0;
        while(isdigit(c))
        {
            intval = intval*10 + (c - '0');
            c = fgetc(fin);
        }
        putback(c);
    }
    else
    {
        tk = c;
    }
}

void
expect(int expted)
{
    if(tk != expted)
    {
        error("Unexpected token");
    }
    next();
}

void
emitl(char *l)
{
    printf("%s:\n", l);
}

void
emit(char *s, ...)
{
    va_list ap;

    printf("\t");
    va_start(ap, s);
    vprintf(s, ap);
    va_end(ap);

    printf("\n");
}

void expr();

void
exprbase()
{
    if(tk == T_INT)
    {
        emit("PUSH %d", intval);
        next();
    }
    else if(tk == '(')
    {
        expect('(');
        expr();
        expect(')');
    }
    else
    {
        error("Unimplemented");
    }
}

void
exprbin()
{
    exprbase();

    if(tk == '+')
    {
        emit("PUSH B");
        exprbase();
        emit("PUSH B");
        next();
        emit("XCHG");
        emit("POP B");
        emit("POP H");
        emit("DAD B");
        emit("XCHG");
        emit("MOV B,D");
        emit("MOV C,E");
        emit("POP B");
        emit("POP B");
    }
    else if(tk == '-')
    {
        emit("PUSH B");
        exprbase();
        emit("PUSH B");
        next();
        emit("XCHG");
        emit("POP B");
        emit("POP H");
        emit("MOV A,L");
        emit("SUB C");
        emit("MOV L,A");
        emit("MOV A,H");
        emit("SBB B");
        emit("MOV H,A");
        emit("XCHG");
        emit("MOV B,D");
        emit("MOV C,E");
        emit("POP B");
        emit("POP B");
    }
}

void
expr()
{
    exprbin();
}

void
loadlval(int si)
{
    if(SYMISGLOB(si))
    {
        emit("LXI B,%s", symids[si]);
    }
    else if(SYMISLOC(si))
    {
        emit("MOV D,H");
        emit("MOV E,L");
        emit("LXI B,%d", symoff[si]);
        emit("DAD B", symoff[si]);
        emit("MOV B,H");
        emit("MOV C,L");
        emit("XCHG");
    }
    else
    {
        error("Cannot use function as lvalue... for now");
    }
}

void sblock();

void
stmt()
{
    int si;

    if(tk == T_KVAR)
    {
        next();

        if(tk != T_ID)
        {
            expect(T_ID);
        }

        si = addsym(buf);
        symoff[si] = curroff;
        curroff += 2;
        next();

        emit("INX SP");
        emit("INX SP");

        expect(';');
    }
    else if(tk == T_KIF)
    {
        /* TODO */
    }
    else if(tk == T_KWHILE)
    {
        /* TODO */
    }
    else if(tk == '{')
    {
        sblock();
    }
    else if(tk == T_ID)
    {
        si = getsym(buf);
        if(si < 0)
        {
            error("Invalid symbol");
        }

        expect(T_ID);

        if(tk == '=')
        {
            expect('=');

            loadlval(si);
            emit("PUSH B");
            expr();

            emit("XCHG");

            emit("POP H");
            emit("MOV M,C");
            emit("INR M");
            emit("MOV M,B");

            emit("XCHG");
        }
        else if(tk == '(')
        {
            expect('(');
            expect(')');

            if(!SYMISFUNC(si))
            {
                error("Cannot call a non function");
            }

            emit("CALL %s", symids[si]);
        }
        else
        {
            error("Invalid statement");
        }

        expect(';');
    }
    else
    {
        error("Invalid statement");
    }
}

void
sblock()
{
    expect('{');

    /* TODO: Parse stmt block */
    while(tk != '}')
    {
        stmt();
    }

    expect('}');
}

void
prog()
{
    int si;

    emit("ORG 0x100");
    emit("CALL main");

    while(tk)
    {
        if(tk == T_KVAR)
        {
            /* Global var */
            next();
            if(tk != T_ID)
            {
                expect(T_ID);
            }

            si = addsym(buf);
            symoff[si] = GSYMOFF;
            emitl(buf);
            next();

            expect(';');

            emit("DW 0");
        }
        else if(tk == T_ID)
        {
            /* Function */
            curroff = 2;
            si = addsym(buf);
            symoff[si] = FSYMOFF;
            emitl(buf);
            next();

            expect('(');
            expect(')');
            sblock();
            emit("RET");
        }
        else
        {
            error("Invalid global decl");
        }
    }

    emit("RET");
    emit("HLT");
}

int
main()
{
    fin = fopen("ex.b", "r");
    next();
    prog();
    return(0);
}
