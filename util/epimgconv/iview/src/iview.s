
;               OUTPUT  "iview.ext"

;               ORG   0BFFAH
;               DEFB  000H, 006H
;               DEFW  codeEnd - codeBegin
;               BLOCK 12, 000H

codeBegin:

parseCommand:   CALL  ZPMASOL
                XOR   A
                LD    (CSSZ), A
                JP    FILELOAD

loadModule:     LD    IXH, D
                LD    IXL, E
                PUSH  BC
                CALL  ZPMASOL
                POP   AF
                LD    (CSSZ), A
                JP    SIMPLELOAD

ZPMASOL:        LD    HL, FOPROGRAM
                LD    DE, 100H
                LD    BC, NINCSEXD - 100H
                LDIR
                LD    L, E
                LD    H, D
                INC   DE
                LD    (HL), C
                LD    BC, VEGE - (NINCSEXD + 1)
                LDIR
                LD    A, 255
                LD    (NINCSINI), A
                LD    HL, 400H
                LD    (WAIT), HL
                LD    HL, 0BFFFH
                LD    (VE), HL
                RET

;FOPROGRAM:
;       PHASE 00100H

FILELOAD:       LD    C, 60H
                CALL  exosReset_SP0100
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
                JR    NZ, KILEPESNZ1
                INC   A
                EXOS  3
                LD    DE, FILEPUF
                LD    A, 1
                EXOS  1
                LD    A, 1
                EXOS  5
                OR    B
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
                INC   A
                LD    (BOTTM), A
                XOR   A
                LD    (CSSZ), A
                LD    DE, FILEPUF
                EXOS  1
                CP    0A6H
                JP    Z, KILEPES
                OR    A
                JR    NZ, NYITHIB
                LD    DE, FEJLEC
                LD    BC, 10H
                EXOS  6
KILEPESNZ1:     JP    NZ, KILEPES
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
                LD    A, 1CH
LFORGAT:        ADD   HL, HL
                RLA
                JR    NC, LFORGAT
                LD    C, 82H
                OUT   (C), H
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
INIREAD:        LD    HL, FILEPUF
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
VLOAD:          LD    HL, PALETTE
                CALL  VLOADINIT
                JP    NZ, NEMISMERT
                CALL  STARTLOAD
                JP    NYITHIB
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
STARTLOAD:      LD    L, (IX+6)   ;MAGASSAG
                LD    H, (IX+7)
                DEC   HL
                LD    BC, 10000H - 300
                ADD   HL, BC
                JP    C, INVALID
                LD    A, (IX+8)   ;SZELESSEG
                DEC   A
                CP    46
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
                LD    HL, SYNCH * 4
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
                LD    H, A
NEMFIX1         CALL  NZ, SZORZAS00
                LD    (VIDS), HL ;EGY LPT SORNYI VIDEO TERULET MERETE
                             ;VAGY EGY PIXEL SORNYI
                POP   DE
                PUSH  DE
                LD    A, (IX+4)
                OR    A
                JR    NZ, NEMFIX2
                LD    L, E
                LD    H, A
NEMFIX2         CALL  NZ, SZORZAS00
                LD    (ATTRS), HL ;EGY LPT SORNYI ATTRIB TERULET MERETE
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
                LD    HL, 0000H
                LD    (LS), HL
                LD    HL, (LPTTS)
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
                ADC   A, E
                LD    (RIGHTM), A
                LD    A, 31
                SUB   E
                LD    (LEFTM), A

                LD    HL, COLORS
                LD    A, (MODEB)
                AND   0EH
                SUB   04H
                CP    1
                SBC   A, A              ; A = 0: (L)PIXEL, A = FFH: ATTRIBUTE
                LD    (ATTRIB), A
                JR    NZ, BIASLOAD
                LD    A, (MODEB)        ; PIXEL VAGY LPIXEL MOD:
                ADD   A, 20H            ; 2, 4, 16, 256 SZIN:
                AND   60H               ; A = 20H, 40H, 60H, 00H
                RRA
                RRA
                RRA
                RRA                     ; A = 02H, 04H, 06H, 00H
                LD    (HL), A           ; PALETTA MERET TAROLASA
                CP    6
                JR    NZ, PALLOAD
BIASLOAD:       LD    (HL), 8           ; 16 SZIN VAGY ATTRIBUTE
                LD    A, (IX+1)
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
                LD    HL, (LS)
                RES   7, H
                SET   6, H
                LD    (LADDR), HL
LPTCREAT:       LD    DE, (LADDR)
                LD    HL, 3F06H
                LD    BC, 203FH
                LD    A, (LCIK)
                RRCA                    ; MASODIK FELKEP
                AND   (IX+5)            ; ES INTERLACE?
                LD    A, 0FCH
                JP    P, ELSOKEP
                LD    L, B
                LD    B, 38H
                DEC   A
ELSOKEP:        LD    (ISYNC1), HL
                LD    (ISYNC2), BC
                LD    (ISYNC3), A
                LD    HL, SYNC
                LD    BC, SYNCH
                XOR   A
SYNCMASOL1:     LDI
                LDI
                LDI
                LDI
                LD    B, 6
SYNCMASOL2:     LD    (DE), A
                INC   E
                LD    (DE), A
                INC   DE
                DJNZ  SYNCMASOL2
                CP    C
                JR    NZ, SYNCMASOL1
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
                LD    A, (LCIK)
                CP    1
                SBC   A, A              ; ELSO FELKEP: A = FFH
                OR    (IX+5)            ; INTERLACE
                AND   04H               ; A = 04H HA ELSO FELKEP
                JR    Z, NINCSSZIN      ; VAGY PALETTA INTERLACE
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
                LD    A, (VIDS)
                JR    Z, NEMATT
                LD    HL, (LD2)
                LD    B, A              ; C = 0
                ADD   HL, BC
                LD    (LD2), HL
                LD    A, (ATTRS)
NEMATT          LD    HL, (LD1)
                LD    B, A
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
                ADD   A, A
                ADD   A, A
                ADD   A, A
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
                LD    A, 1CH
LFORGAT2:       ADD   HL, HL
                RLA
                JR    NC, LFORGAT2
                LD    C, 82H
                OUT   (C), H
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
KEYWAIT:        LD    DE, (WAIT)
KEYWAIT0:       LD    HL, 450           ;9 s
KEYWAIT1:       EXX
                CALL  @File.MOUSEINPUT
                EXX
                OR    A
                RRA
                JR    NZ, KEYEND        ;bal vagy jobb eger gomb?
                LD    B, 10
KEYASK:         LD    A, B
                DEC   A
                CP    4
                OUT   (0B5H), A
                IN    A, (0B5H)
                CPL
                JR    NZ, NEMFK         ;nem a 4. sor?
                OR    A
                JR    Z, FK3
                LD    A, (WAIT+1)
                SUB   D
                LD    C, A
                LD    DE, TIMERTABLE+8
FK1:            DEC   DE
                RLA
                JR    NC, FK1
                LD    A, (DE)
                LD    (WAIT+1), A
                SUB   C
                DEC   A
                JP    P, FK2
                XOR   A
FK2:            INC   A
                LD    D, A
                XOR   A
NEMFK:          OR    A
                RLA                     ;0. bit torlese
                JR    NZ, KEYEND
FK3:            DJNZ  KEYASK
                DEC   HL
                LD    A, L
                OR    H
                JR    NZ, KEYWAIT1
                DEC   D
                JR    NZ, KEYWAIT0
                LD    A, (NINCSINI)
                OR    A
                JR    NZ, KEYWAIT0
KEYEND:         LD    BC, 07B5H
                OUT   (C), B
                IN    B, (C)            ;B & 01H = STOP
                CPL                     ;A & 01H = jobb eger gomb
                AND   B
                RRCA
                LD    B, 3
                OUT   (C), B
                IN    B, (C)            ;B & 80H = ESC
                AND   B
                CP    80H               ;CARRY = 1 ha STOP, ESC, vagy jobb gomb
                SBC   A, A
                PUSH  AF
                LD    A, (FFCSERE)
                OR    A
                JR    Z, KEYRET
                LD    HL, VEGE
                LD    IY, (CSEREC)
                LD    BC, (CSEREH)
CSEREL2:        LD    D, (HL)
                LD    A, (IY)
                LD    (HL), A
                LD    (IY), D
                INC   HL
                INC   IY
                DEC   BC
                LD    A, B
                OR    C
                JR    NZ, CSEREL2
KEYRET:         POP   AF
                AND   0E5H              ;.STOP
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
                SUB   0FCH
                JR    C, NEMVRAM
                ADD   A, LOW V0
                LD    E, A
                ADC   A, HIGH V0
                SUB   E
                LD    D, A
                LD    A, C
                LD    (DE), A
                JR    NEM64K
NEMVRAM:        LD    (HL), C
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
MEMORYBACK:     LD    HL, V0
.L1:            LD    C, (HL)
                EXOS  25
                INC   HL
                LD    A, L
                CP    LOW (V0 + 4)
                JR    NZ, .L1
                RET

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

FOPROGRAM:

        PHASE 00100H

EXIT:           XOR   A
                OUT   (0B3H), A
                JP    0C000H
KILEPES:        DI
                LD    SP, 100H
                LD    A, 255
                OUT   (0B2H), A
                LD    C, 40H
                EXOS  0
                LD    A, 1
                OUT   (0B3H), A
                LD    A, 6
                JP    0C00DH
;----------------------------
                ;     F4  F8  F3  F6  F5  F7  F2  F1
TIMERTABLE:     DB     4,  8,  3,  6,  5,  7,  2,  1

SCANL           DB    255
MODEB           DB    12H
LEFTM           DB    63
RIGHTM          DB    0
LD1             DW    0
LD2             DW    0
PALETTE         DB    0, 255, 254, 253, 252, 251, 250, 249
SYNC
BOTTM       DEFB 105, 12H, 63, 0
                               ;151 sor, ez hatarozza meg a also
                               ;margo nagysagat,

        ;   DEFB 253, 16, 63, 0
                               ;3 sor, a szinkronizacio
                               ;kikapcsolva
            DEFB 252, 16
ISYNC1      DEFB 6, 63
                               ;4 sor, a szinkronizacio
                               ;bekapcsolva
            DEFB 255, 90H
ISYNC2      DEFB 63, 32
                               ;1 sor, a szinkronizacio a sor
                               ;felenel kikapcsol, a NICK chip
                               ;ennel a sornal ad megszakitast
ISYNC3      DEFB 252, 12H, 6, 63
                               ;4 ures sor

TOPM        DEFB 104, 12H, 63, 0
                               ;152 sor, ez hatarozza meg a felso
                               ;margo nagysagat
SYNCH           EQU   $-SYNC

NINCSEXD:
NINCSINI        EQU     NINCSEXD + 1    ; = 255
WAIT            EQU     NINCSINI + 1    ; = 400H
LPBN            EQU     WAIT + 2
LPTS            EQU     LPBN + 2
LPTTS           EQU     LPTS + 2
ATTRS           EQU     LPTTS + 2
ATTRS1          EQU     ATTRS + 2
ATTRTS          EQU     ATTRS1 + 2
VIDS            EQU     ATTRTS + 2
VIDS1           EQU     VIDS + 2
VIDTS           EQU     VIDS1 + 2
LOADS           EQU     VIDTS + 2
COLORS          EQU     LOADS + 2
ATTRIB          EQU     COLORS + 1
BIAS            EQU     ATTRIB + 1
LADDR           EQU     BIAS + 1
V0              EQU     LADDR + 2
V1              EQU     V0 + 1
V2              EQU     V1 + 1
V3              EQU     V2 + 1
LS              EQU     V3 + 1
AS              EQU     LS + 2
VS              EQU     AS + 2
VE              EQU     VS + 2          ; = 0BFFFH
LCIK            EQU     VE + 2
FFCSERE         EQU     LCIK + 1
CSEREH          EQU     FFCSERE + 1
CSEREC          EQU     CSEREH + 2
CSSZ            EQU     CSEREC + 2
FEJLEC          EQU     CSSZ + 1
FILEPUF         EQU     FEJLEC + 16
VEGE            EQU     FILEPUF + 256

        DEPHASE

shortHelpString:
                DB    "IVIEW version 0.42", 13, 10, 0
longHelpString:
                DB    "Image viewer for converted PC images.", 13, 10
                DB    "Written by Zozosoft, 2008-2009", 13, 10
                DB    "Converter program by IstvanV", 13, 10, 0

codeEnd:

