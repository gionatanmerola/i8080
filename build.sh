# Compile the assembler
#gcc -o i8080asm src/i8080asm.c -ansi -pedantic -Wall -Werror -DI8080ASM_STANDALONE -DVERBOSE
gcc -o i8080asm src/i8080asm.c -ansi -pedantic -Wall -Werror -DI8080ASM_STANDALONE

# Compile the emulator
#gcc -o i8080emu src/i8080emu.c -ansi -pedantic -Wall -Werror -DI8080_DEBUG_MODE -DCPM
gcc -o i8080emu src/i8080emu.c -ansi -pedantic -Wall -Werror -DCPM

# Assemble BIOS and CP/M OS
./i8080asm bios.asm bios.bin
./i8080asm cpm22.asm cpm22.bin
