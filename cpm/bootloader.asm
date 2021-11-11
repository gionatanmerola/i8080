;***********************************************************************
;
;                           BOOT LOADER
;       This program is in the first sector of the first track
;       of the disk and is loaded at startup at address 0x0000
;       This bootloader loads the first two tracks of the disk
;       (except track 0,sector 1) to the address 0x3400.
;
;       Author: driverfury  https://github.com/driverfury
;
;***********************************************************************

CPMADR  EQU     0x3400
BIOSADR EQU     0x4a00

        ORG     0x0000


        MVI     A,0
        CALL    SETDSK

        LXI     H,CPMADR    ; HL = DMA
        CALL    SETDMA
        MVI     E,0         ; E = track
        MVI     D,2         ; D = sector
LOOP:   MOV     A,E
        CALL    SETTRK
        MOV     A,D
        CALL    SETSEC
        CALL    RDSEC
        INR     D
        MOV     A,D
        CPI     27
        CZ      INCTRK
        MOV     A,E
        CPI     2           ; if 2 tracks has been loaded
        JZ      BIOSADR     ; jump to BIOS addr
        LXI     B,128
        DAD     B           ; HL = HL + 128
        CALL    SETDMA
        JMP     LOOP
        HLT

INCTRK: MVI     D,1
        INR     E
        RET

SETDSK: OUT     2
        RET

SETTRK: OUT     3
        RET

SETSEC: OUT     4
        RET

SETDMA: MOV     A,L
        OUT     5
        MOV     A,H
        OUT     6

RDSEC:  IN      7
        RET
