#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>

struct termios origtermios;

void
exitrawmode(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &origtermios);
}

void
rawmode(void)
{
    struct termios raw;

    tcgetattr(STDIN_FILENO, &origtermios);
    atexit(exitrawmode);
    raw = origtermios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

#include "i8080.c"

#define MEM_SIZE 0x1000
u8 mem[MEM_SIZE];

u8
memread(u16 addr)
{
    return(mem[addr % MEM_SIZE]);
}

u16
memread16(u16 addr)
{
    u8 b1, b2;
    u16 val;

    b1 = memread(addr);
    b2 = memread(addr+1);
    val = (u16)((b1 << 0) & 0x00ff) |
          (u16)((b2 << 8) & 0xff00);

    return(val);
}

void
memwrite(u16 addr, u8 val)
{
    mem[addr % MEM_SIZE] = val;
}

void
portout(u8 port, u8 b)
{
    switch(port)
    {
        case 1:
        {
            putchar(b);
        } break;
    }
}

u8
portin(u8 port)
{
    u8 out;

    out = 0;
    switch(port)
    {
        case 0:
        {
            out = 0;
        } break;

        case 1:
        {
            out = 0;
        } break;
    }

    return(out);
}

i8080 cpu;

void
usage(char *prog)
{
    printf("Usage: %s <bootloader.bin>\n", prog);
}

#include "i8080asm.c"

int
main(int argc, char *argv[])
{
    char *fname;
    FILE *f;
    u8 b;
    int i;


    if(argc < 2)
    {
        usage(argv[0]);
        exit(0);
    }

    fname = argv[1];
    f = fopen(fname, "rb");
    i = 0;
    while(fread(&b, sizeof(u8), 1, f))
    {
        mem[i++] = b;
    }
    fclose(f);

    cpu.MEM_READ   = &memread;
    cpu.MEM_READ16 = &memread16;
    cpu.MEM_WRITE  = &memwrite;
    cpu.OUT_PORT   = &portout;
    cpu.IN_PORT    = &portin;

    rawmode();
    do
    {
#ifdef I8080_DEBUG_MODE
        char c;
        char buff[48];

        int i;
        int size;
        u8 *cptr;

        cptr = &mem[cpu.pc];
        size = disassemble(&cptr, 0x1000, buff);

        printf("%.4x: ", cpu.pc);
        for(i = 0;
            i < size;
            ++i)
        {
            printf("%.2x ", (unsigned int)*(cptr - (size - i)));
        }
        for(i = 0;
            i < (9-size*3);
            ++i)
        {
            printf(" ");
        }
        printf(" | %s\n", buff);

        c = 0;
        read(STDIN_FILENO, &c, 1);
        if(c == 0 || c == 'q')
        {
            break;
        }
#endif
    }
    while(!i8080exec(&cpu));

    return(0);
}
