;*******************************************************************
;
;                   OS for i8080 emulator
;               https://github.com/driverfury/i8080
;
;*******************************************************************

TAB     EQU     0x09
LF      EQU     0x0a
VTAB    EQU     0x0b
FFEED   EQU     0x0c
CR      EQU     0x0d
SPACE   EQU     0x20

;
; BIOS/BDOS
;

BOOT:   LXI     SP,0xfff0
        LXI     B,WLCMSG
        CALL    PUTS
        CALL    NEWLN
        CALL    CMD
        CALL    EXIT
        HLT
        RET

CMD:    MVI     A,'>'
        CALL    PUTCH
        CALL    GETCMD
        CALL    NEWLN
        ;CALL    PUTS
        ;CALL    NEWLN
        CALL    EXEC
        JMP     CMD
        RET

EXIT:   LXI     B,EXITMSG
        CALL    PUTS
        HLT
        RET

; Put the string (BC) to the terminal
PUTS:   PUSH    B
.PSLP:  LDAX    B
        CPI     0
        JZ      .PSEND
        CALL    PUTCH
        INX     B
        JMP     .PSLP
.PSEND: POP     B
        RET

; Put the char A to ther terminal
PUTCH:  OUT     1
        RET

; Put the line feed character (0x13) to the terminal
NEWLN:  MVI     A,LF
        CALL    PUTCH
        RET

; Get a character A from the terminal input
GETCH:  IN      1
        RET

; Get a string from the therminal input (max length: 32)
; Return: BC -> pointer to string (command)
;         A  -> length of the command
GETCMD: MVI     D,0             ; D  = String counter
        LXI     B,CMDBUF        ; BC = String pointer
.GCLP:  MOV     A,D
        CPI     CMDBUFN-1       ; if A == 31 max length reached
        JZ      .GCEND
        CALL    GETCH           ; get char from stdin
        CPI     LF              ; if input char is line feed or carriage ret
        JZ      .GCEND          ; end the loop
        CPI     CR
        JZ      .GCEND
        CALL    PUTCH           ; echo the char
        STAX    B               ; *BC = A
        INX     B               ; ++BC
        INR     D               ; ++D
        JMP     .GCLP           ; loop
.GCEND: MVI     A,0             ; insert null character at the end
        STAX    B
        LXI     B,CMDBUF        ; BC = CMDBUF
        MOV     A,D             ; A  = D
        RET

;; Get a string (BC) and replace every whitespace with 0
;STRTOK: PUSH    B
;.STLP:  LDAX    B
;        CPI     TAB
;        JZ      .STSP
;        CPI     LF
;        JZ      .STSP
;        CPI     VTAB
;        JZ      .STSP
;        CPI     FFEED
;        JZ      .STSP
;        CPI     CR
;        JZ      .STSP
;        CPI     SPACE
;        JZ      .STSP
;        CPI     0
;        JZ      .STEND
;        INX     B
;        JMP     .STLP
;.STSP:  MVI     A,0
;        STAX    B
;        INX     B
;        JMP     .STLP
;.STEND: POP     B
;        RET

; Return 0 in A if strings (BC) and (DE) are equals,
; otherwise return 0xff in A
STREQ:  PUSH    B
        PUSH    D
        PUSH    H
.SELP:  LDAX    B           ; A = *BC
        MOV     H,A         ; H = A

        ; NOTE: Why when I have this debug code,
        ; this function will work properly, but
        ; when I don't have it, it won't work???
        ; DEBUG
        ;CALL    PUTCH
        ;CALL    NEWLN

        ; DEBUG
        ;LDAX    D           ; A = *DE
        ;CALL    PUTCH
        ;CALL    NEWLN

        LDAX    D           ; A = *DE
        SUB     H           ; A = A - H
        JNZ     .SEQF       ; if(A != 0) return(0);
        MOV     A,H         ; A = H
        CPI     0           ; if(A == 0) return(1);
        JZ      .SEQT
        LDAX    D           ; if(*DE == 0) return(1);
        CPI     0
        JZ      .SEQT
        INX     B           ; ++BC;
        INX     D           ; ++DE;
        JMP     .SELP       ; loop
.SEQT:  MVI     A,0xff      ; return(1);
        JMP     .SEQE
.SEQF:  MVI     A,0x00      ; return(0);
.SEQE:  POP     H
        POP     D
        POP     B
        RET

; Execute the command string (BC)
EXEC:   ;CALL    STRTOK

        ; cmd: exit
        LXI     D,CMDEXIT
        CALL    STREQ
        CPI     0
        CNZ     EXIT

        ; cmd: help
DBGLBL: LXI     D,CMDHELP
        CALL    STREQ
        CPI     0
        JNZ     HELP

        ; cmd unknown
        CALL    PUTS
        MVI     A,'?'
        CALL    PUTCH
        CALL    NEWLN
        JMP     .EXEND

.EXEND: RET

HELP:   LXI     B,HELPMSG
        CALL    PUTS
        RET

.DBG:   MVI     A,'!'
        CALL    PUTCH
        CALL    NEWLN
        RET

WLCMSG: DB      'O','S','8','0','8','0',0,0
EXITMSG:DB      'E','X','I','T','I','N','G','.','.','.',0
HELPMSG:DB      'H','E','L','P',LF,LF,' ','h','e','l','p',LF,' ','e','x','i','t',LF,0
CMDBUF: DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
CMDBUFN EQU     32

CMDEXIT:DB      'e','x','i','t',0,0,0,0
CMDHELP:DB      'h','e','l','p',0,0,0,0
