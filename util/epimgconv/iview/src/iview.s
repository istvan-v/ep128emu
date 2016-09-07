
;               OUTPUT  "iview.ext"

;               ORG   0BFFAH
;               DEFB  000H, 006H
;               DEFW  codeEnd - codeBegin
;               BLOCK 12, 000H

        MACRO EXOS N
                RST   030H
                DEFB  N
        ENDM

codeBegin:

extMain:
                LD    A, C
                CP    2
                JR    Z, COM1 ;PARANCSFUZER
                CP    6
                JP    Z, LOADM ;MODUL BETOLTES
                CP    3
                RET   NZ    ;VISSZATERES HA NEM HELP
                LD    A, B
                OR    A
                JR    Z, ALTAL
                LD    HL, IVIEW
                CALL  KERD
                RET   NZ
                LD    BC, HOSSZ2
                LD    DE, HH2
                LD    A, 255
                EXOS  8
                LD    C, 0
                RET
ALTAL           PUSH  DE
                PUSH  BC
                LD    DE, HH2
                LD    BC, HOSSZ1
                LD    A, 255
                EXOS  8
                POP   BC
                POP   DE
                RET
COM1            LD    HL, IVIEW
                CALL  KERD
                RET   NZ

                LD    HL, FOPROGRAM
                LD    DE, 100H
                LD    BC, FPH
                LDIR
                POP   AF
                LD    (CSSZ), A
                JP    FILELOAD
                XOR   A
                LD    C, A
                RET
KERD            LD    A, B
                CP    (HL)
                RET   NZ
                INC   HL
                PUSH  BC
                PUSH  DE
                INC   DE

AZONOS          LD    A, (DE)
                CP    (HL)
                JR    NZ, NEMA
                INC   HL
                INC   DE
                DJNZ  AZONOS
NEMA            POP   DE
                POP   BC
                RET

LOADM:          LD    IXH, D
                LD    IXL, E
                LD    A, (IX+1)
                CP    "I"
                RET   NZ
                PUSH  BC
                LD    HL, FOPROGRAM
                LD    DE, 100H
                LD    BC, FPH
                LDIR
                POP   AF
                LD    (CSSZ), A
                JP    SIMPLELOAD
                XOR   A
                LD    C, A
                RET

;FOPROGRAM:
;       PHASE 00100H

                LD    SP, 100H
FILELOAD:       LD    C, 60H
                EXOS  0
                LD    SP, 100H
                LD    HL, KILEPES
                LD    (0BFF8H), HL
                LD    DE, EXDFD
                EXOS  26
                LD    (NINCSEXD), A
                JR    NZ, NOEXD
                LD    DE, INIFILE
ININYIT:        LD    A, 1
                EXOS  1
NOEXD:          LD    (NINCSINI), A
FILECIKLUS:     LD    A, (NINCSINI)
                OR    A
                CALL  Z, INIREAD
                JR    Z, INIREADED
INIHALT:
                LD    A, (NINCSEXD)
                OR    A
                JR    NZ, TAPELOAD
                LD    DE, FILE
                EXOS  26
                PUSH  AF
                LD    A, 1
                EXOS  3
                POP   AF
                JP    NZ, KILEPES
                LD    DE, FILEPUF
                LD    A, 1
                EXOS  1
                LD    A, 1
                EXOS  5
                LD    A, B
                OR    A
                PUSH  AF
                LD    A, 1
                EXOS  3
                POP   AF
                JR    Z, INIREADED
                LD    DE, FILEPUF
                JR    ININYIT
TAPELOAD:       XOR   A
                LD    (FILEPUF), A
INIREADED:      LD    A, 104
                LD    (TOPM), A
                LD    A, 105
                LD    (BOTTM), A
                XOR   A
                LD    (CSSZ), A
                LD    DE, FILEPUF
                EXOS  1
                JR    NZ, NYITHIB
                LD    DE, FEJLEC
                LD    BC, 10H
                EXOS  6
                JP    NZ, KILEPES
                EX    AF, AF'
                LD    IX, FEJLEC
                LD    A, (IX+1)
                CP    11
                JP    Z, VLOAD
                CP    "I"
                JR    NZ, NEMISMERT
                CALL  STARTLOAD
NYITHIB:        EX    AF, AF'
                LD    A, 255
                OUT   (0B2H), A
                LD    HL, (0BFF4H)
                SET   6, H
                LD    B, 4
LFORGAT:        SRL   H
                RR    L
                DJNZ  LFORGAT
                LD    A, L
                OUT   (82H), A
                LD    A, H
                OR    0C0H
                OUT   (83H), A
                CALL  MEMORYBACK
NEMISMERT:      XOR   A
                EXOS  3
                EX    AF, AF'
                CP    229
                JP    NZ, FILECIKLUS
                LD    A, 1
                EXOS  3
                JP    INIHALT
INIREAD:        LD    IY, FILEPUF
                LD    (IY), 0
                LD    HL, FILEPUF+1
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
EXDFD           DB    6, "EXDOS", 0FDH
INIFILE         DB    9, "IVIEW.INI"
FILE            DB    7, "FILE "
                DW    FILEPUF
SIMPLELOAD:   ; LD    SP, 100H
                LD    HL, KILEPES
                LD    (0BFF8H), HL
                CALL  STARTLOAD
                DI
                LD    A, 255
                OUT   (0B2H), A
                LD    HL, KILEPES
                LD    (0BFF8H), HL
                CALL  MEMORYBACK
                JP    EXIT
INVALID:        LD    A, 220
                OR    A
                RET
VLOAD:          LD    E, (IX+5) ;MAGASSAG KARAKTERSOROKBAN
                LD    D, 0
                LD    H, D
                LD    L, D
                LD    B, 9
SOR9X:          ADD   HL, DE
                DJNZ  SOR9X
                LD    (IX+6), L ;MAGASSAG PIXEL SOROKBAN
                LD    (IX+7), H
                LD    A, (IX+2) ;VIDEO MODE
                CP    1
                JP    Z, HIRESG
                CP    5
                JP    NZ, NEMISMERT
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
                SET   4, A    ;VRES BIT
                ADD   A, 2
                BIT   2, (IX+2)
                JR    Z, HIRESG2
                ADD   A, 12
HIRESG2:        LD    (IX+0), A
                XOR   A
                LD    (IX+2), A
                LD    (IX+3), A
                LD    (IX+4), A
                LD    (IX+5), A
                LD    (IX+9), A
                LD    (IX+10), A
                LD    (IX+11), A
                LD    (IX+12), A
                LD    (IX+13), A
                LD    (IX+14), A
                LD    (IX+15), A
                LD    B, 8
                LD    HL, PALETTE
PALIR:          LD    (HL), A
                DEC   A
                INC   HL
                DJNZ  PALIR
                LD    HL, NYITHIB
                PUSH  HL
STARTLOAD:      LD    L, (IX+6)  ;MAGASSAG
                LD    H, (IX+7)
                LD    A, H
                OR    L
                JP    Z, INVALID
                LD    BC, 301
                SBC   HL, BC
                JP    NC, INVALID
                LD    A, (IX+8)   ;SZELESSEG
                OR    A
                JP    Z, INVALID
                CP    47
                JP    NC, INVALID
                LD    A, (IX+10)  ;TOMORITES TIPUSA
                OR    A
                JP    NZ, INVALID
                LD    A, (IX+11)  ;KEPKOCKAK SZAMA
                OR    A
                JR    NZ, BEALLITVA
                LD    (IX+11), 1
BEALLITVA:      LD    A, (IX+2)
                OR    A
                JP    NZ, INVALID
                LD    A, (IX+3)
                OR    A
                JP    NZ, INVALID
                LD    L, (IX+6)
                LD    H, (IX+7)
                LD    A, (IX+4)
                OR    A
                JR    Z, ALLANDO
                CALL  W_MODULO
                LD    A, L
                OR    H
                JP    NZ, INVALID
                EX    DE, HL
                JR    KEVESM256  ; 100H 101H 102H
ALLANDO:        DEC   HL        ; 0FFH 100H 101H
                LD    A, H        ; 0    1    1
                OR    A          ; Z    NZ   NZ
                LD    HL, 1       ;      1    1
                JR    Z, KEVESM256 ;Z    NZ   NZ
                INC   HL        ; 1    2    2
KEVESM256       LD    (LPBN), HL   ;SZUKSEGES LPTSOROK SZAMA
                EX    DE, HL
                LD    A, 16
                LD    HL, SYNCH
                CALL  SZORZAS
                LD    (LPTS), HL ;EGY LPT TABLA MERETE
                LD    A, (IX+5)
                OR    A
                JR    Z, NOLACE
                EX    DE, HL
                LD    A, (IX+12) ;HANYSZOR SZEREPEL EGY FAZIS
                INC   A
                CALL  SZORZAS0
                EX    DE, HL
                LD    A, (IX+11) ;INTERLACE FAZISOK SZAMA
                CALL  SZORZAS0
NOLACE:         LD    (LPTTS), HL ;A TELJES LPT MERETE
                LD    A, (IX+1)
                CP    "I"
                JR    Z, VMODEL
                LD    A, (IX)
                JR    VMODELOADED
VMODEL:         LD    A, (CSSZ)
                EXOS  5
                LD    A, B
            ;   SET   4, A   ;VRES BIT
VMODELOADED:    LD    (MODEB), A
                AND   1110B
                LD    H, (IX+8) ;VIDEOADAT
                LD    L, 0      ;ATTRADAT
                CP    1110B    ;LPIXEL MOD?
                JR    Z, ADATOK
                LD    L, (IX+8)
                CP    0100B    ;ATTRIBUTE MOD?
                JR    Z, ADATOK
                CP    0010B
                JP    NZ, INVALID
                LD    A, H
                ADD   A, H
                LD    H, A
                LD    L, 0
ADATOK          PUSH  HL  ;L, EGY SOR ATTRIBUTUM TERULETENEK MERETE
                         ;H, EGY SOR VIDEO TERULETENEK MERETE
                LD    E, H
                LD    A, (IX+4)
                OR    A
                JR    NZ, NEMFIX1
                LD    L, H
                LD    H, 0
                JR    FIX1
NEMFIX1         CALL  SZORZAS00
FIX1            LD    (VIDS), HL ;EGY LPT SORNYI VIDEO TERULET MERETE
                             ;VAGY EGY PIXEL SORNYI
                POP   DE
                PUSH  DE
                LD    A, (IX+4)
                OR    A
                JR    NZ, NEMFIX2
                LD    L, E
                LD    H, 0
                JR    FIX2
NEMFIX2         CALL  SZORZAS00
FIX2            LD    (ATTRS), HL ;EGY LPT SORNYI ATTRIB TERULET MERETE
                              ;VAGY EGY PIXEL SORNYI
                POP   DE
                PUSH  DE
                LD    A, E
                LD    E, (IX+6)
                LD    D, (IX+7)
                CALL  SZORZAS0
                LD    (ATTRS1), HL ;EGY LPTNYI ATTRIB TERULET MERETE
                EX    DE, HL
                LD    A, (IX+11)
                CALL  SZORZAS0
                LD    (ATTRTS), HL ;TELJES ATTRIBUTUM TERULET MERETE
                POP   AF
                LD    E, (IX+6)
                LD    D, (IX+7)
                CALL  SZORZAS0
                LD    (VIDS1), HL ;EGY LPTNYI VIDEO TERULET MERETE
                EX    DE, HL
                LD    A, (IX+11)
                CALL  SZORZAS0
                LD    (VIDTS), HL ;TELJES VIDEO TERULET MERETE
                LD    BC, (ATTRTS)
                ADD   HL, BC
                LD    (LOADS), HL ;BETOLTENDO ADATMENNYISEG
                LD    HL, (LS)
                LD    BC, (LPTTS)
                ADD   HL, BC
                LD    (AS), HL   ;ATTRIBUTUM TERULET KEZDETE
                LD    BC, (ATTRTS)
                ADD   HL, BC
                LD    (VS), HL   ;VIDEO TERULET KEZDETE

                CALL  MEMORY

  ;A KEP MAGASSAGANAK A FELET LEVONJUK AZ ALSO ES A FELSO MARGOBOL
                LD    E, (IX+6)
                LD    D, (IX+7)
                SRL   D
                RR    E
             ;  LD    HL, (BOTTM)
             ;  ADC   HL, DE
             ;  LD    (BOTTM), HL
             ;  LD    HL, (TOPM)
             ;  ADD   HL, DE
             ;  LD    (TOPM), HL
                EX    AF, AF'
                LD    A, (TOPM)
                ADD   A, E
                LD    D, A
                EX    AF, AF'
                LD    A, (BOTTM)
                ADC   A, E
                LD    B, 4
ELTOL:          CP    255
                JR    Z, ELTOLVA
                INC   A
                DEC   D
                DJNZ  ELTOL
ELTOLVA:        LD    (BOTTM), A
                LD    A, D
                LD    (TOPM), A
  ;JOBB ES BAL MARGO BEALLITASA
                LD    E, (IX+8)
                SRL   E
                LD    A, 31
                SBC   A, E
                LD    (LEFTM), A
                LD    A, 31
                ADD   A, E
                LD    (RIGHTM), A

                LD    A, (MODEB)
                AND   1110B
                CP    0100B
                JR    NZ, NEMATTRIB
                LD    A, 8
                LD    (COLORS), A
                LD    (ATTRIB), A
                JR    BIASLOAD
NEMATTRIB:      LD    A, (MODEB)
                AND   1100000B
                JR    NZ, NEM2COL
                LD    (ATTRIB), A
                LD    A, 2
                LD    (COLORS), A
                JR    PALLOAD
NEM2COL:        CP    0100000B
                JR    NZ, NEM4COL
                LD    A, 4
                LD    (COLORS), A
                XOR   A
                LD    (ATTRIB), A
                JR    PALLOAD
NEM4COL:        CP    1000000B
                JR    NZ, NEM16COL
                LD    A, 8
                LD    (COLORS), A
                XOR   A
                LD    (ATTRIB), A
                JR    BIASLOAD
NEM16COL:       XOR   A
                LD    (COLORS), A
                LD    (ATTRIB), A
                JR    PALLOAD
BIASLOAD:       LD    A, (IX+1)
                CP    "I"
                JR    NZ, PALLOAD
                LD    A, (CSSZ)
                EXOS  5
                LD    A, B
                LD    (BIAS), A
PALLOAD:        XOR   A
                LD    (LCIK), A
                LD    A, (V0)
                OUT   (0B1H), A
                LD    DE, (LS)
                RES   7, D
                SET   6, D
                LD    (LADDR), DE
LPTCREAT:       LD    DE, (LADDR)
                LD    HL, 3F06H
                LD    (ISYNC1), HL
                LD    HL, 203FH
                LD    (ISYNC2), HL
                LD    A, 0FCH
                LD    (ISYNC3), A
                LD    A, (LCIK)
                BIT   0, A
                JR    Z, ELSOKEP
                BIT   7, (IX+5)
                JR    Z, ELSOKEP
                LD    HL, 3F20H
                LD    (ISYNC1), HL
                LD    HL, 383FH
                LD    (ISYNC2), HL
                LD    A, 0FBH
                LD    (ISYNC3), A
ELSOKEP         LD    HL, SYNC
                LD    BC, SYNCH
                LDIR
                LD    (LADDR), DE
                LD    HL, (VS)
                LD    A, (LCIK)
                LD    DE, (VIDS1)
                CALL  SZORZAS
                LD    A, (ATTRIB)
                OR    A
                JR    Z, NEMA0
                LD    (LD2), HL
                LD    HL, (AS)
                LD    A, (LCIK)
                LD    DE, (ATTRS1)
                CALL  SZORZAS
NEMA0:          LD    (LD1), HL
                LD    A, (IX+4)
                OR    A
                JP    NZ, VALTOZO
                LD    A, (IX+1)
                CP    "I"
                JR    NZ, NINCSSZIN
                LD    A, (COLORS)
                OR    A
                JR    Z, NINCSSZIN
                LD    C, A
                LD    B, 0
                LD    DE, PALETTE
                LD    A, (CSSZ)
                EXOS  6
NINCSSZIN:
                LD    DE, (LADDR)
                LD    HL, SCANL
                LD    BC, 16
                LD    A, (IX+7)
                OR    A
                JR    Z, KEVESEBB
                LD    A, (IX+6)
                OR    A
                JR    Z, KEVESEBB
                XOR   A
                PUSH  HL
                PUSH  BC
                LD    (HL), A
                LDIR
                LD    A, (ATTRIB)
                OR    A
                LD    BC, (VIDS-1)
                JR    Z, NEMATT
                LD    HL, (LD2)
                LD    C, 0
                ADD   HL, BC
                LD    (LD2), HL
                LD    BC, (ATTRS-1)
NEMATT          LD    HL, (LD1)
                LD    C, 0
                ADD   HL, BC
                LD    (LD1), HL
                POP   BC
                POP   HL
                XOR   A
KEVESEBB:       SUB   (IX+6)
                LD    (HL), A
                PUSH  DE
                LDIR
                LD    (LADDR), DE
                POP   HL
                JP    LPTKESZ
VALTOZO:
                XOR   A
                SUB   (IX+4)
                LD    (SCANL), A
                LD    HL, (LPBN)
LPTCIKLUS:      PUSH  HL
                LD    BC, (COLORS)
                LD    B, 0
                LD    DE, PALETTE
                LD    A, (CSSZ)
                EXOS  6
                LD    HL, SCANL
                LD    DE, (LADDR)
                LD    BC, 16
                PUSH  DE
                LDIR
                LD    (LADDR), DE
                LD    A, (ATTRIB)
                OR    A
                JR    Z, NEMA2
                LD    HL, (LD2)
                LD    BC, (VIDS)
                ADD   HL, BC
                LD    (LD2), HL
                LD    BC, (ATTRS)
                JR    IGENA2
NEMA2:          LD    BC, (VIDS)
IGENA2:         LD    HL, (LD1)
                ADD   HL, BC
                LD    (LD1), HL
                POP   DE
                POP   HL
                DEC   HL
                LD    A, H
                OR    L
                JR    NZ, LPTCIKLUS
                EX    DE, HL
LPTKESZ         INC   HL
                LD    A, (LCIK)
                INC   A
                LD    (LCIK), A
                CP    (IX+11)
                JP    NZ, LPTCREAT
                SET   0, (HL)  ;RELOAD BIT
                LD    A, (BIAS)
                OUT   (80H), A
                SLA   A
                SLA   A
                SLA   A
                LD    D, A
                LD    BC, 256+28
                EXOS  16   ;BIAS
                LD    A, (IX+9)
                OUT   (81H), A
                LD    D, A
                LD    BC, 256+27
                EXOS  16   ;BORDER
                DI
                XOR   A
                LD    (FFCSERE), A

                LD    HL, (LS)
                LD    B, 4
LFORGAT2        SRL   H
                RR    L
                DJNZ  LFORGAT2
                LD    A, L
                OUT   (82H), A
                LD    A, H
                OR    0C0H
                OUT   (83H), A
             ;  OUT   (82H), A
             ;  LD    A, 192
             ;  OUT   (83H), A
                LD    BC, (LADDR)
                LD    HL, 8000H
                OR    A
                SBC   HL, BC
                PUSH  HL
                LD    BC, (LOADS)
                SBC   HL, BC
                JR    C, TULSOK
                POP   HL
                PUSH  BC
TULSOK          POP   BC
                LD    A, (CSSZ)
                OR    A
                LD    HL, (LOADS)
                SBC   HL, BC
                LD    (LOADS), HL
                LD    DE, (LADDR)
                EXOS  6
                LD    A, H
                OR    L
                JP    Z, BETOLTVE
                LD    A, (V1)
                OUT   (0B1H), A
                LD    HL, 4000H
                PUSH  HL
                LD    BC, (LOADS)
                SBC   HL, BC
                JR    C, TULSOK1
                POP   HL
                PUSH  BC
TULSOK1         POP   BC
                LD    A, (CSSZ)
                OR    A
                LD    HL, (LOADS)
                SBC   HL, BC
                LD    (LOADS), HL
                LD    DE, 4000H
                EXOS  6
                LD    A, H
                OR    L
                JP    Z, BETOLTVE
                LD    A, (V2)
                OUT   (0B1H), A
                LD    HL, 4000H
                PUSH  HL
                LD    BC, (LOADS)
                SBC   HL, BC
                JR    C, TULSOK2
                POP   HL
                PUSH  BC
TULSOK2         POP   BC
                LD    A, (CSSZ)
                OR    A
                LD    HL, (LOADS)
                SBC   HL, BC
                LD    (LOADS), HL
                LD    DE, 4000H
                EXOS  6
                LD    A, H
                OR    L
                JR    Z, BETOLTVE
                LD    A, (V3)
                OUT   (0B1H), A
                LD    HL, (VE)
                PUSH  HL
                LD    BC, (LOADS)
                SBC   HL, BC
                JR    C, TULSOK3
                POP   HL
                PUSH  BC
TULSOK3         POP   BC
                LD    A, (CSSZ)
                OR    A
                LD    HL, (LOADS)
                SBC   HL, BC
                LD    (LOADS), HL
                LD    DE, 4000H
                EXOS  6
                LD    (CSEREC), DE
                LD    A, H
                OR    L
                JR    Z, BETOLTVE
                IN    A, (0B0H)
                CP    0FCH
                JR    Z, BETOLTVE
                LD    BC, VEGE
                LD    HL, 4000H
                OR    A
                SBC   HL, BC
                PUSH  HL
                LD    BC, (LOADS)
                SBC   HL, BC
                JR    C, TULSOK4
                POP   HL
                PUSH  BC
TULSOK4         POP   BC
                LD    A, (CSSZ)
                OR    A
                LD    HL, (LOADS)
                SBC   HL, BC
                LD    (LOADS), HL
                LD    DE, VEGE
                LD    (CSEREH), BC
                EXOS  6
                LD    A, 1
                LD    (FFCSERE), A
BETOLTVE        LD    A, 255
                OUT   (0B2H), A
                LD    A, (CSSZ)
                EXOS  3
                DI
                LD    A, (FFCSERE)
                OR    A
                JR    Z, KEYWAIT
                LD    HL, VEGE
                LD    IY, (CSEREC)
                LD    BC, (CSEREH)
CSEREL1         LD    D, (HL)
                LD    A, (IY)
                LD    (HL), A
                LD    (IY), D
                INC   HL
                INC   IY
                DEC   BC
                LD    A, B
                OR    C
                JR    NZ, CSEREL1
KEYWAIT:       ;LD HL, 0600H
                LD    DE, (WAIT)
KEYWAIT0:       LD    HL, 8000H
KEYWAIT1:       LD    B, 9
                LD    A, B
KEYASK:         CP    4
                OUT   (0B5H), A
                IN    A, (0B5H)
                JR    NZ, NEMFK
                LD    E, 1
                BIT   7, A
                JR    Z, FK
                INC   E
                BIT   6, A
                JR    Z, FK
                INC   E
                BIT   2, A
                JR    Z, FK
                INC   E
                BIT   0, A
                JR    Z, FK
                INC   E
                BIT   4, A
                JR    Z, FK
                INC   E
                BIT   3, A
                JR    Z, FK
                INC   E
                BIT   5, A
                JR    Z, FK
                INC   E
                BIT   1, A
                JR    NZ, FK2
FK:             LD    A, (WAIT+1)
                SUB   D
                LD    C, A
                LD    A, E
                LD    (WAIT+1), A
                SUB   C
                JR    C, FK3
                JR    Z, FK3
                LD    A, 1
FK3:            LD    D, A
                JR    FK2
NEMFK:          INC   A
                JR    NZ, KEYEND
FK2:            DEC   B
                LD    A, B
                CP    255
                JR    NZ, KEYASK
                DEC   HL
                LD    A, H
                OR    L
                JR    NZ, KEYWAIT1
                DEC   D
                JR    NZ, KEYWAIT0
                LD    A, (NINCSINI)
                OR    A
                JR    NZ, KEYWAIT0
KEYEND:         XOR   A
                LD    (STOPGOMB), A
                LD    A, 7
                OUT   (0B5H), A
                IN    A, (0B5H)
                BIT   0, A
                JR    Z, STOP
                LD    A, 3
                OUT   (0B5H), A
                IN    A, (0B5H)
                BIT   7, A
                JR    NZ, NEMSTOP
STOP            LD    A, 229
                LD    (STOPGOMB), A
NEMSTOP         LD    A, (FFCSERE)
                OR    A
                JR    Z, KEYRET
                LD    HL, VEGE
                LD    IY, (CSEREC)
                LD    BC, (CSEREH)
CSEREL2         LD    D, (HL)
                LD    A, (IY)
                LD    (HL), A
                LD    (IY), D
                INC   HL
                INC   IY
                DEC   BC
                LD    A, B
                OR    C
                JR    NZ, CSEREL2
KEYRET          LD    A, (STOPGOMB)
                OR    A
                RET
MEMORY:         LD    A, 254
                LD    HL, CSSZ
CSATZAR:        CP    (HL)
                JR    Z, KIHAGY
                CP    1
                JR    NZ, ZAR
                LD    A, (NINCSINI)
                OR    A
                LD    A, 1
                JR    Z, KIHAGY
ZAR             PUSH  AF
                EXOS  3
                POP   AF
KIHAGY          SUB   1
                JR    NC, CSATZAR
                LD    HL, FILEPUF
                LD    (HL), 0
                IN    A, (0B0H)
                CP    0FCH
                JR    NZ, NEM64K
                LD    (V0), A
                LD    HL, ((VEGE/16)+1)*16
                LD    (LS), HL
                EX    DE, HL
                LD    HL, (VS)
                ADD   HL, DE
                LD    (VS), HL
                LD    HL, (AS)
                ADD   HL, DE
                LD    (AS), HL

NEM64K:         EXOS  24
                JR    NZ, ELFOGYOTT
                LD    A, C
                CP    0FCH
                JR    C, NEMFE
                JR    NZ, NEMFC
                LD    (V0), A
                JR    NEM64K
NEMFC           CP    0FDH
                JR    NZ, NEMFD
                LD    (V1), A
                JR    NEM64K
NEMFD           CP    0FEH
                JR    NZ, NEMFE
                LD    (V2), A
                JR    NEM64K
NEMFE           LD    (HL), A
                INC   HL
                LD    (HL), 0
                JR    NEM64K
ELFOGYOTT       CP    7FH
                RET   NZ
                DEC   DE
                LD    (VE), DE
                EXOS  23
                LD    A, 255
                LD    (V3), A
                LD    HL, FILEPUF
VISSZAAD        LD    A, (HL)
                OR    A
                RET   Z
                LD    C, A
                EXOS  25
                INC   HL
                JR    VISSZAAD
MEMORYBACK:     LD    BC, (V0)
                EXOS  25
                LD    BC, (V1)
                EXOS  25
                LD    BC, (V2)
                EXOS  25
                LD    BC, (V3)
                EXOS  25
                RET

;MUL_DIV.ASM          /SZORZAS, OSZTAS, MODULO       1991. HSOFT:

WSZORZAS:       PUSH  HL         ;HLBC=HL*DE
                PUSH  DE
                LD    A, H
                PUSH  AF
                LD    A, L
                CALL  SZORZAS00  ;L*E
                PUSH  HL
                EXX
                POP   BC          ;BC'
                EXX
                POP   AF
                POP   DE
                PUSH  DE
                CALL  SZORZAS00  ;H*E
                PUSH  HL
                EXX
                EX    AF, AF'
                XOR   A
                POP   DE
                LD    H, 0
                LD    L, B
                ADD   HL, DE
                ADC   A, 0
                EX    AF, AF'
                EXX   ;AHLC'
                POP   DE
                POP   HL
                LD    E, D
                LD    A, H
                PUSH  AF
                PUSH  DE
                LD    A, L
                CALL  SZORZAS00  ;L*D
                PUSH  HL
                EXX
                EX    AF, AF'
                POP   DE
                ADD   HL, DE
                ADC   A, 0
                EX    AF, AF'
                EXX   ;AHLC'
                POP   DE
                POP   AF
                CALL  SZORZAS00  ;H*D
                PUSH  HL
                EXX
                EX    AF, AF'
                POP   DE
                LD    B, L
                LD    L, H
                LD    H, A
                ADD   HL, DE       ;HLBC
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
W_MODULO:       LD    E, L          ;DE=INT(HL/A)  HL=MOD(HL, A)
                LD    L, H
                LD    B, A
                CALL  B_MODULO0
                LD    D, A
                LD    H, L
                LD    L, E
                LD    B, C
                CALL  B_MODULO
                LD    E, A
                RET
;----------------------------
B_MODULO0:      LD    H, 0
B_MODULO:       LD    C, 0     ;A=INT(L/B)  L=MOD(L, B)
                LD    A, 1
MODULO10:       SRL   B
                RR    C
                SBC   HL, BC
                JR    NC, MODULO20
                ADD   HL, BC
MODULO20:       RLA
                JR    NC, MODULO10
                CPL
                RET

; -----------------------------------------------------------------------------

FOPROGRAM:

        PHASE 00100H

EXIT:           XOR   A
                OUT   (0B3H), A
                JP    0C000H
KILEPES:        LD    SP, 100H
                LD    A, 255
                OUT   (0B2H), A
                CALL  MEMORYBACK
                LD    A, 6
                EX    AF, AF'
                LD    A, (0B21CH)
                CP    0D3H
                LD    A, 1
                LD    HL, 0C00DH
                JP    Z, 0B21CH
                JP    0B217H
;----------------------------
WAIT            DW    4
STOPGOMB        DB    0
LPBN            DW    0
LPTS            DW    0
LPTTS           DW    0
ATTRS           DW    0
ATTRS1          DW    0
ATTRTS          DW    0
VIDS            DW    0
VIDS1           DW    0
VIDTS           DW    0
LOADS           DW    0
COLORS          DB    0
ATTRIB          DB    0
BIAS            DB    0
LADDR           DW    0
SCANL           DB    255
MODEB           DB    12H
LEFTM           DB    63
RIGHTM          DB    0
LD1             DW    0
LD2             DW    0
PALETTE         DB    0, 255, 254, 253, 252, 251, 250, 249
SYNC
BOTTM       DEFB 105, 12H, 63, 0, 0, 0, 0, 0
            DEFB 0, 0, 0, 0, 0, 0, 0, 0
                               ;151 sor, ez hatarozza meg a also
                               ;margo nagysagat,

        ;   DEFB 253, 16, 63, 0, 0, 0, 0, 0
        ;   DEFB 0, 0, 0, 0, 0, 0, 0, 0
                               ;3 sor, a szinkronizacio
                               ;kikapcsolva
            DEFB 252, 16
ISYNC1      DEFB 6, 63, 0, 0, 0, 0
            DEFB 0, 0, 0, 0, 0, 0, 0, 0
                               ;4 sor, a szinkronizacio
                               ;bekapcsolva
            DEFB 255, 90H
ISYNC2      DEFB 63, 32, 0, 0, 0, 0
            DEFB 0, 0, 0, 0, 0, 0, 0, 0
                               ;1 sor, a szinkronizacio a sor
                               ;felenel kikapcsol, a NICK chip
                               ;ennel a sornal ad megszakitast
ISYNC3      DEFB 252, 12H, 6, 63, 0, 0, 0, 0
            DEFB 0, 0, 0, 0, 0, 0, 0, 0
                               ;4 ures sor

TOPM        DEFB 104, 12H, 63, 0, 0, 0, 0, 0
            DEFB 0, 0, 0, 0, 0, 0, 0, 0
                               ;152 sor, ez hatarozza meg a felso
                               ;margo nagysagat
SYNCH           EQU   $-SYNC

NINCSEXD        DB    0
NINCSINI        DB    255
V0              DB    0;FCH
V1              DB    0;FDH
V2              DB    0;FEH
V3              DB    0;FFH
LS              DW    0
AS              DW    0
VS              DW    0
VE              DW    0BFFFH
LCIK            DB    0
FFCSERE         DB    0
CSEREH          DW    0
CSEREC          DW    0
CSSZ            DB    0
FEJLEC          DS    16
FILEPUF         DS    256
FPH             EQU   $-100H
VEGE            EQU   $

        DEPHASE

IVIEW           DB    5, "IVIEW"
HH2             EQU   IVIEW+1
                DB    " version 0.42", 13, 10
HOSSZ1          EQU   $-HH2
                DB    "Image viewer for converted PC images.", 13, 10
                DB    "Written by Zozosoft, 2008-2009", 13, 10
                DB    "Converter program by IstvanV", 13, 10
HOSSZ2          EQU   $-HH2

codeEnd:

