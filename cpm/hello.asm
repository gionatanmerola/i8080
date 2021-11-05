BDOS    EQU     0x0005

        ORG     0x0100

        LXI     D,MSG
        MVI     C,0x09
        CALL    BDOS

        RET

MSG:    DB      "Hello World!!!!$",0
