# Compile the assembler
#gcc -o i8080asm src/i8080asm.c -ansi -pedantic -Wall -Werror -DI8080ASM_STANDALONE -DVERBOSE
gcc -o i8080asm src/i8080asm.c -ansi -pedantic -Wall -Werror -DI8080ASM_STANDALONE

# Compile the emulator
gcc -o cpm/cpm22d src/i8080emu.c -ansi -pedantic -Wall -Werror -DI8080_DEBUG_MODE -DCPM
gcc -o cpm/cpm22 src/i8080emu.c -ansi -pedantic -Wall -Werror -DCPM
gcc -o i8080emu src/i8080emu.c -ansi -pedantic -Wall -Werror

# Assemble BIOS and CP/M OS
./i8080asm cpm/bios.asm cpm/bios.bin
./i8080asm cpm/cpm22.asm cpm/cpm22.bin
./i8080asm cpm/hello.asm cpm/hello.com
