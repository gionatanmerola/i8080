;**********************************************************
;
;                   BIOS for CP/M OS
;
;**********************************************************

MSIZE   EQU     20          ; CP/M version mem size in KB

; BIAS is address offset from 0x3400 for memory system
; with more than 16k
BIAS    EQU     (MSIZE-20)*1024
CCP     EQU     0x3400+BIAS     ; base of CCP
BDOS    EQU     CCP+0x806       ; base of BDOS
BIOS    EQU     CCP+0x1600      ; base of BIOS
CDISK   EQU     0x0004          ; current disk no 0=A, ...
IOBYTE  EQU     0x0003          ; intel I/O byte

        ORG     BIOS            ; Origin of this program

NSECTS  EQU     ($-CCP)/128     ; Warm start sector count

        JMP     BOOT            ; cold start
WBOOTE: JMP     WBOOT           ; warm start
        JMP     CONST           ; console status
        JMP     CONIN           ; console char in
        JMP     CONOUT          ; console char out
        JMP     LIST            ; list char out
        JMP     PUNCH           ; punch char out
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

;
; Fixed data tables for four-drive standard
; IBM-compatible 8'' disks
;

; Disk params header for disk 00
DPBASE: DW      TRANS,  0x0000
        DW      0x0000, 0x0000
        DW      DIRBF,  DPBLK
        DW      CHK00,  ALL00

; Disk params header for disk 01
        DW      TRANS,  0x0000
        DW      0x0000, 0x0000
        DW      DIRBF,  DPBLK
        DW      CHK01,  ALL01

; Disk params header for disk 02
        DW      TRANS,  0x0000
        DW      0x0000, 0x0000
        DW      DIRBF,  DPBLK
        DW      CHK02,  ALL02

; Disk params header for disk 03
        DW      TRANS,  0x0000
        DW      0x0000, 0x0000
        DW      DIRBF,  DPBLK
        DW      CHK03,  ALL03

; Sector translate vector
; NOTE(driverfury): We don't need a sector skew table since
; we are just emulating
;TRANS:  DB       1,  7, 13, 19  ; sectors  1,  2,  3,  4
;        DB      25,  5, 11, 17  ; sectors  5,  6,  7,  8
;        DB      23,  3,  9, 15  ; sectors  9, 10, 11, 12
;        DB      21,  2,  8, 14  ; sectors 13, 14, 15, 16
;        DB      20, 26,  6, 12  ; sectors 17, 18, 19, 20
;        DB      18, 24,  4, 10  ; sectors 21, 22, 23, 24
;        DB      16, 22          ; sectors 25, 26
TRANS:  DB       1,  2,  3,  4
        DB       5,  6,  7,  8
        DB       9, 10, 11, 12
        DB      13, 14, 15, 16
        DB      17, 18, 19, 20
        DB      21, 22, 23, 24
        DB      25, 26

; Disk parameter block (common to all disks)
DPBLK:  DW      26              ; sectors per track
        DB      3               ; bock shift factor
        DB      7               ; block mask
        DB      0               ; null mask
        DW      242             ; disk size-1
        DW      63              ; directory max
        DB      192             ; alloc 0
        DB      0               ; alloc 1
        DW      16              ; check size
        DW      2               ; track offset


;
; Individual subroutines to perform each function
;

; BOOT just perform params initialization
BOOT:   XRA     A               ; zero the acc
        STA     IOBYTE          ; clear the IOBYTE
        STA     CDISK           ; select disk zero
        JMP     GOCPM           ; go to CP/M

; WBOOT read the disk until all sectors are loaded
WBOOT:  LXI     SP,0x80         ; use stack below buffer
        MVI     C,0             ; select disk 0
        CALL    SELDSK
        CALL    HOME            ; go to track 00

        MVI     B,NSECTS        ; B=#sects to load
        MVI     C,0             ; C=current track #
        MVI     D,2             ; D=next sector to read
; NOTE: we begin by reading track 0, sector 2 since sector 1
; contains the cold start loader, which is skipped in a warm
; start
        LXI     H,CCP           ; base of CP/M (current DMA)
LOAD1:  PUSH    B
        PUSH    D
        PUSH    H
        MOV     C,D
        CALL    SETSEC          ; set sector
        POP     B               ; recall DMA addr to BC
        PUSH    B
        CALL    SETDMA

        CALL    READ
        CPI     0               ; any errors?
        JNZ     WBOOT           ; retry the entire WBOOT
        POP     H               ; recall DMA addr
        LXI     D,128
        DAD     D               ; DMA = DMA + 128
        POP     D               ; recall sector #
        POP     B               ; recall num of sects rem
        DCR     B               ; sectors = sectors - 1
        JZ      GOCPM

        ; More sectors remain to load, check for track change
        INR     D
        MOV     A,D
        CPI     27
        JC      LOAD1           ; sector==27 => change track

        ; End of current track, go to next track
        MVI     D,1             ; begin with the first sector
        INR     C               ; track = track + 1

        ; Save register state and change tracks
        PUSH    B
        PUSH    D
        PUSH    H
        CALL    SETTRK
        POP     H
        POP     D
        POP     B
        JMP     LOAD1

GOCPM:  MVI     A,0xc3          ; c3 is a JMP instruction
        STA     0
        LXI     H,WBOOTE
        SHLD    1               ; set addr field for jmp

        STA     5               ; for jump to BDOS
        LXI     H,BDOS
        SHLD    6               ; set addr field for jmp

        LXI     B,0x80          ; default DMA addr is 0x80
        CALL    SETDMA

        EI                      ; enable interrupt system
        LDA     CDISK           ; get current disk number
        MOV     C,A
        JMP     CCP             ; go to CCP

;
; Simple I/O handlers
;

; Return console status in register A
;   A = 0x00  => no character ready for reading
;   A = 0xff  => character is ready for reading
CONST:  IN      0
        RET

; Get console input character in register A
CONIN:  IN      1
        CPI     27
        JZ      EXIT
        RET

; Output the character in C to the console
CONOUT: MOV     A,C
        OUT     1
        RET

; TODO: Understand why this function
; List character from register C
LIST:   MOV     A,C
        RET

; TODO: Understand why this function
; Return list  status (0 if not ready, 1 if ready)
LISTST: XRA     A           ; 0 is always ok to return
        RET

; TODO: Understand why this function
; Punch character from register C
PUNCH:  MOV     A,C
        RET

; Reader character into register A from reader device
READER: MVI     A,0x1a      ; enter end of file for now (replace later)
        ANI     0x7f        ; strip parity bit
        RET

;
; I/O drivers for the disk
;

; Move to track 0
HOME:   MVI     C,0
        CALL    SETTRK
        RET

; Select disk given by register C
SELDSK: LXI     H,0x0000    ; error return code
        MOV     A,C
        STA     DISKNO
        CPI     4           ; disk number must be between 0 and 3
        RNC                 ; no carry if 4, 5, ...
        OUT     2           ; Select disk A
        LDA     DISKNO
        MOV     L,A         ; L=disk number (0, 1, 2, 3)
        MVI     H,0         ; high order zero
        DAD     H           ; HL*2
        DAD     H           ; HL*4
        DAD     H           ; HL*8
        DAD     H           ; HL*16
        LXI     D,DPBASE
        DAD     D           ; HL=(DISKNO*16)+DPBASE
        RET

; Select track given by register C
SETTRK: MOV     A,C
        STA     TRACK
        OUT     3           ; Set track A
        RET

; Select sector given by register C
SETSEC: MOV     A,C
        STA     SECTOR      ; Set sector A
        OUT     4
        RET

; Translate the sector given by BC using the
; translate table given by DE. Return in HL
SECTRAN:XCHG                ; HL <-> DE
        DAD     B           ; HL = HL + BC
        MOV     L,M         ; L=(HL)
        MVI     H,0         ; H=0
        RET

; Set DM address given by BC
SETDMA: MOV     L,C
        MOV     H,B
        SHLD    DMAAD       ; save the DMA addr
        MOV     A,C
        OUT     5
        MOV     A,B
        OUT     6
        RET

; Perform read operation
READ:   IN      7
        JMP     WAITIO

; Perform write operation
WRITE:  OUT     7
        JMP     WAITIO

WAITIO: MVI     A,0         ; 0 = no error
        RET

TRACK:  DW      0
SECTOR: DW      0
DMAAD:  DW      0
DISKNO: DB      0

;
; Scratch RAM area for BDOS use
;

BEGDAT  EQU     $           ; Beginning of data area

DIRBF:  DS      128         ; Scratch directory area
ALL00:  DS      31          ; Allocation vector 0
ALL01:  DS      31          ; Allocation vector 1
ALL02:  DS      31          ; Allocation vector 2
ALL03:  DS      31          ; Allocation vector 3
CHK00:  DS      16          ; Check vector 0
CHK01:  DS      16          ; Check vector 1
CHK02:  DS      16          ; Check vector 2
CHK03:  DS      16          ; Check vector 3

ENDDAT  EQU     $           ; End of data area
DASIZ   EQU     $-BEGDAT    ; Size of data area

EXIT:   HLT
