
; ------------------------------- from iview.s: -------------------------------

INIREADHL:      PUSH  HL
                POP   IY
                LD    (HL), 0
                INC   HL
INICIKLUS:      LD    A, 1
                EXOS  5
                JR    Z, INI01
                LD    A, (IY)
                OR    A
                JR    Z, INIHIBA
                XOR   A
                RET
INIHIBA:        INC   A
                RET
INI01:          LD    A, B
                CP    33
                JR    NC, INI02
                LD    A, (IY)
                OR    A
                JR    Z, INICIKLUS
                XOR   A
                RET
INI02:          LD    (HL), A
                INC   HL
                INC   (IY)
                JR    NZ, INICIKLUS
                OR    A
                RET

; -----------------------------------------------------------------------------

EXDFD           DB    6, "EXDOS", 0FDH
INIFILE         DB    9, "IVIEW.INI"

; -----------------------------------------------------------------------------

VLOADINIT:      XOR   A
                LD    D, A
                LD    B, 8
PALIR:          LD    (HL), A   ;PALETTA: 00 FF FE FD FC FB FA F9
                DEC   A
                INC   HL
                DJNZ  PALIR
                LD    L, (IX+5) ;MAGASSAG KARAKTERSOROKBAN
                LD    H, D
                LD    E, L
                ADD   HL, HL
                ADD   HL, HL
                ADD   HL, HL
                ADD   HL, DE
                LD    (IX+6), L ;MAGASSAG PIXEL SOROKBAN
                LD    (IX+7), H
                LD    A, (IX+2) ;VIDEO MODE
                CP    1
                JP    Z, HIRESG
                CP    5
                RET   NZ
                LD    A, (IX+4)
                JR    LORESG
HIRESG:         LD    A, (IX+4) ;SZELESSEG BAJTOKBAN
                RRCA
LORESG:         LD    (IX+8), A
                LD    A, (IX+3) ;COLOR MODE
                AND   3
                RRCA
                RRCA
                RRCA
                SET   4, A      ;VRES BIT
                ADD   A, 2
                BIT   2, (IX+2)
                JR    Z, HIRESG2
                ADD   A, 12
HIRESG2:        PUSH  IX
                POP   HL
                LD    (HL), A
                LD    A, 87H    ;IX+2..5 ES IX+9..15 TORLESE
                LD    BC, 15 * 256
VLTOROL1:       INC   HL
                ADD   A, A
                JR    C, VLTOROL2
                LD    (HL), C
VLTOROL2:       DJNZ  VLTOROL1
                XOR   A
                RET

; ------------------------------- from main.s: --------------------------------

mulDEByBCToHLIX:
        ld    h, b
        ld    l, c
        ld    ix, 0
        ld    a, 16
.l1:    add   ix, ix
        adc   hl, hl
        jr    nc, .l2
        add   ix, de
        jr    nc, .l2
        inc   hl
.l2:    dec   a
        jr    nz, .l1
        ret

divHLIXByBCToDE:
        ld    de, 1                     ; NOTE: HL:IX must be < 80000000h,
.l1:    add   ix, ix                    ; and BC must be <= 8000h
        adc   hl, hl
        or    a
        sbc   hl, bc
        jr    nc, .l2
        add   hl, bc
.l2:    ccf
        rl    e
        rl    d
        jr    nc, .l1
        ret                             ; the remainder is returned in HL

; ------------------------------- from iview.s: -------------------------------

;MUL_DIV.ASM          /SZORZAS, OSZTAS, MODULO       1991. HSOFT:

WSZORZAS:       PUSH  IX            ;HLBC=HL*DE
                LD    B, H
                LD    C, L
                CALL  mulDEByBCToHLIX
                EX    (SP), IX
                POP   BC
                RET
;----------------------------
SZORZAS00:      LD    D, 0
SZORZAS0:       LD    HL, 0         ;HL=DE*A
SZORZAS:        LD    B, 8          ;HL=DE*A+HL
SZORZAS10:      RRCA
                JR    NC, SZORZAS20
                ADD   HL, DE
SZORZAS20:      SLA   E
                RL    D
                DJNZ  SZORZAS10
                RET
;-----------------------------
W_MODULO:       PUSH  HL            ;DE=INT(HL/A)  HL=MOD(HL, A)
                EX    (SP), IX
                LD    HL, 0
                LD    C, A
                LD    B, H
                CALL  divHLIXByBCToDE
                POP   IX
                LD    A, E
                RET

; -----------------------------------------------------------------------------

