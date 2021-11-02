# Intel 8080 emulator

This is an intel 8080 emulator written in ANSI C89.

We'll probably also make a toolchain for this emulator (OS, assembler, compilers and so on...).

- [x] Assembler
- [x] Emulator
- [ ] BIOS
- [ ] OS (CP/M, ...)

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

## BIOS

We need to make our own BIOS so we can run an OS.
