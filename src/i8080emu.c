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
#ifdef CPM
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
#else
    raw.c_lflag &= ~(ECHO | ICANON);
#endif
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

#include "i8080.c"

#define MEM_SIZE 0x10000
u8 mem[MEM_SIZE];

u8
memread(u16 addr)
{
    return(mem[(unsigned int)addr % MEM_SIZE]);
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
    mem[(unsigned int)addr % MEM_SIZE] = val;
}

int diskno;
int trackno;
int sectorno;
u8  dmahi;
u8  dmalo;

#define TRACKS_PER_DSK 77
#define SECT_PER_TRACK 26
#define BYTES_PER_SECT 128

void
portout(u8 port, u8 b)
{
    switch(port)
    {
        /* TTY output */
        case 1:
        {
            write(STDOUT_FILENO, (void *)&b, 1);
        } break;

        /* DISK selection */
        case 2: { diskno = (int)b; } break;
        /* TRACK selection */
        case 3: { trackno = (int)b; } break;
        /* SECTOR selection */
        case 4: { sectorno = (int)b; } break;

        /* DMA set low order addres */
        case 5: { dmalo = b; } break;
        /* DMA set high order addres */
        case 6: { dmahi = b; } break;

        /* Write a sector from DMA to disk */
        case 7:
        {
            FILE *fdisk;
            unsigned int dma;
            u8 towrite;
            long int offset;
            int i;

            fdisk = fopen("a.hdd", "r+");

            dma = (unsigned int)
                  (((((unsigned int)dmahi&0xff)<<8) |
                    ((unsigned int)dmalo&0xff))
                   & 0xffff);
            offset = ((sectorno-1)*BYTES_PER_SECT) +
                     (trackno*SECT_PER_TRACK*BYTES_PER_SECT);
            fseek(fdisk, offset, SEEK_SET);

            for(i = 0;
                i < BYTES_PER_SECT;
                ++i)
            {
                towrite = memread(dma+i);
                fwrite((void *)&towrite, 1, 1, fdisk);
            }
            fclose(fdisk);
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
        /* TTY status */
        case 0:
        {
            out = 0;
            fseek(stdin, 0, SEEK_END);
            if(ftell(stdin) > 0)
            {
                out = 0xff;
            }
            rewind(stdin);
        } break;

        /* TTY input */
        case 1:
        {
            char c;

            if(fread(&c, 1, 1, stdin) != 1)
            {
                c = 0;
            }
            out = (u8)c;
        } break;

        /* Read a sector from disk to DMA */
        case 7:
        {
            FILE *fdisk;
            unsigned int dma;
            u8 towrite;
            long int offset;
            int i;

            fdisk = fopen("a.hdd", "r+");

            dma = (unsigned int)
                  (((((unsigned int)dmahi&0xff)<<8) |
                    ((unsigned int)dmalo&0xff))
                   & 0xffff);
            offset = ((sectorno-1)*BYTES_PER_SECT) +
                     (trackno*SECT_PER_TRACK*BYTES_PER_SECT);
            fseek(fdisk, offset, SEEK_SET);

            for(i = 0;
                i < BYTES_PER_SECT;
                ++i)
            {
                fread((void *)&towrite, 1, 1, fdisk);
                memwrite(dma+i, towrite);
            }
            fclose(fdisk);
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

#ifdef CPM
    fname = "cpm22.bin";
    f = fopen(fname, "rb");
    i = 0x3400;
    while(fread((void *)&b, 1, 1, f))
    {
        mem[i++] = b;
    }
    fclose(f);

    fname = "bios.bin";
    f = fopen(fname, "rb");
    i = 0x4a00;
    while(fread((void *)&b, 1, 1, f))
    {
        mem[i++] = b;
    }
    fclose(f);

    cpu.pc = 0x4a00;
#else
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
#endif


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
        fread(&c, 1, 1, stdin);
        if(c == 0 || c == 'q')
        {
            break;
        }
#endif
    }
    while(!i8080exec(&cpu));

    return(0);
}
