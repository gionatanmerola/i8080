# Intel 8080 emulator

This is an intel 8080 emulator written in ANSI C89.

We'll probably also make a toolchain for this emulator (OS, assembler, compilers and so on...).

- [x] Assembler
- [x] Emulator
- [x] Bootloader
- [x] BIOS
- [x] OS (CP/M)

## Assembler

To create the assembler standalone executable file you need to compile ```src/i8080asm.c``` source file:

```bash
gcc -o i8080asm src/i8080asm.c -ansi -pedantic -DI8080_ASM_STANDALONE
```

To assemble a file you need to specify input file (source) and output (binary).
```bash
./i8080asm hello.asm hello.bin
```

## Emulator

How to compile the emulator:

```bash
gcc -o i8080emu src/i8080emu.c -ansi -pedantic
```

The you can run a binary file.

```bash
./i8080emu hello.bin
```
### CP/M

[CP/M](https://en.wikipedia.org/wiki/CP/M) is a minimalistic disk operating system born
in the 70s.

You can compile the CP/M emulator with the following command:

```bash
gcc -o cpm/cpm22 src/i8080emu.c -ansi -pedantic -Wall -Werror -DCPM
```
And run it:

```bash
./cpm22
```

You can even compile in debug mode:
```bash
gcc -o cpm/cpm22d src/i8080emu.c -ansi -pedantic -Wall -Werror -DI8080_DEBUG_MODE -DCPM
```

For now, our emulator only operates on 1 disk which is the file ```a.hdd```.

So you need to create a CP/M raw image on a.hdd (mind the sector skew table, you can
enable or disable it in the file ```bios.asm```).

The first sector (128 bytes) of the disk is the bootloader, it is automatically loaded
into memory and it is executed.

You can find a default bootloader (```cpm/bootloader.asm```) for CP/M which loads the first
2 tracks into memory and jumps to CP/M entry point.

The BIOS and the bootloader are platform dependent (hardware-dependent), while CP/M is
not because it has the BDOS which is an interface between the BIOS and CP/M.

## BIOS

We need a BIOS to run an OS.

Our BIOS is pretty simple, it has the following table at the start (it is compatible with CP/M):

```asm
    JMP     BOOT            ; cold start
    JMP     WBOOT           ; warm start
    JMP     CONST           ; console status
    JMP     CONIN           ; console char in
    JMP     CONOUT          ; console char out
    JMP     LIST            ; list char out
    JMP     PUNCH           ; punch char our
    JMP     READER          ; reader char out
    JMP     HOME            ; move head to home pos
    JMP     SELDSK          ; select disk
    JMP     SETTRK          ; select track
    JMP     SETSEC          ; select sector
    JMP     SETDMA          ; set DMA address
    JMP     READ            ; read fromdisk
    JMP     WRITE           ; write to disk
    JMP     LISTST          ; return list status
    JMP     SECTRAN         ; sector translate
```

So you can call address ```BIOS+9``` for CONIN, ```BIOS+12``` for CONOUT and so on...

