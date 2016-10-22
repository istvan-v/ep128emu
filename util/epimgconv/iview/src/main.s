
BUILD_EXTENSION_ROM     equ     0
CVIEW_BUFFER_SIZE       equ     1000h

    if BUILD_EXTENSION_ROM == 0
        output  "iview.ext"
        org     0bffah
        defw    0600h, mainCodeEnd - mainCodeBegin, 0, 0, 0, 0, 0, 0
    else
        output  "iview.rom"
        org     0c000h
        defm    "EXOS_ROM"
        defw    0000h
    endif

    macro exos n
        rst   30h
        defb  n
    endm

    macro EXOS N
        rst   30h
        defb  N
    endm

mainCodeBegin:

        module  Main

main:
        ld    a, c
        sub   2
        jr    z, .parseCommand
        dec   a
        jr    z, .helpString
        sub   2
        jr    z, .errorString
        dec   a
        jr    z, .loadModule
.nextExtension:
        xor   a
        ret
.parseCommand:
        call  checkCommandName
        jr    nz, .nextExtension
        ld    l, (ix + 1)               ; get jump address from table
        ld    h, (ix + 2)
        jp    (hl)
.helpString:
        cp    b
        jr    nz, .l2                   ; long help ?
        ld    hl, extensionTable
.l1:    ld    a, (hl)
        or    a
        ret   z                         ; end of table ?
        add   a, c                      ; BC = 3
        ld    c, a
        add   hl, bc
        ld    c, 3
        ld    a, (hl)
        inc   hl
        push  hl
        ld    h, (hl)
        ld    l, a
        call  printString
        pop   hl
        add   hl, bc
        jr    .l1
.l2:    call  checkCommandName
        jr    nz, .nextExtension
        ld    c, a
        ld    l, (ix + 3)
        ld    h, (ix + 4)
        call  printString
        ld    l, (ix + 5)
        ld    h, (ix + 6)
        jp    printString
.errorString:
        ld    de, errorMessages
        ld    h, a                      ; H = 0
.l3:    ld    a, (de)
        or    a
        ret   z
        inc   de
        cp    b
        jr    z, .l4
        ex    de, hl
        ld    e, (hl)
        add   hl, de
        inc   hl
        ex    de, hl
        jr    .l3
.l4:    in    a, (0b3h)
        ld    b, a
        xor   a
        ld    c, a
        ret
.loadModule:
        inc   de
        ld    a, (de)
        dec   de
        cp    'I'
        jr    z, .l5                    ; IVIEW format ?
        cp    'v'
        jp    z, IPlay.loadModule
        cp    's'
        jp    z, SndPlay.loadModule
        cp    'd'
        jr    nz, .nextExtension
        jp    DL2.loadModule
.l5:    xor   a                         ; call IVIEW or CVIEW
        ld    ixl, e                    ; depending on compression type
        ld    ixh, d
        cp    (ix + 10)
        jp    z, IView.loadModule
        jp    CView.loadModule

checkCommandName:
        ld    hl, extensionTable
.l1:    ld    a, (hl)
        dec   a
        ret   m                         ; end of table?
        inc   a
        cp    b
        jr    nz, .l4                   ; length does not match?
        push  bc
        push  de
.l2:    inc   de                        ; compare command name
        inc   hl
        ld    a, (de)
        sub   (hl)
        jr    nz, .l3                   ; does not match?
        djnz  .l2
        push  hl
        pop   ix
        pop   de                        ; return 00h if command is found,
        pop   bc                        ; FFh otherwise; IX+1 points to the
        ret                             ; address table
.l3:    pop   de
        ld    a, b
        pop   bc
        dec   a
.l4:    add   a, 7                      ; continue with next command
        add   a, l
        ld    l, a
        jr    nc, .l1
        inc   h
        jr    .l1

extensionTable:
        defb  5
        defm  "IVIEW"
        defw  IView.parseCommand, IView.shortHelpString
        defw  IView.longHelpString
        defb  5
        defm  "CVIEW"
        defw  CView.parseCommand, CView.shortHelpString
        defw  CView.longHelpString
        defb  5
        defm  "IPLAY"
        defw  IPlay.parseCommand, IPlay.shortHelpString
        defw  IPlay.longHelpString
        defb  7
        defm  "SNDPLAY"
        defw  SndPlay.parseCommand, SndPlay.shortHelpString
        defw  SndPlay.longHelpString
        defb  10
        defm  "UNCOMPRESS"
        defw  Uncompress.parseCommand, Uncompress.shortHelpString
        defw  Uncompress.longHelpString
        defb  3
        defm  "DL2"
        defw  DL2.parseCommand, DL2.shortHelpString
        defw  DL2.longHelpString
        defb  0

; error message table format:
;   defb  ERRORCODE1, MSGLEN1
;   defm  MSG1
;   defb  ERRORCODE2, MSGLEN2
;   defm  MSG2
;   ...
;   defb  0

errorMessages:
        defb  00h

        endmod

; -----------------------------------------------------------------------------

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

exosReset_SP0100:
        ld    hl, 0100h

exosReset:
        di
        pop   de
        ld    sp, hl
        push  de
        ld    hl, (004fh)
        ld    de, 13
        add   hl, de
        res   6, h
        xor   a
        out   (0b2h), a
        ld    e, (hl)
        inc   hl
        ld    d, (hl)
        dec   a
        out   (0b2h), a
        xor   a
        ld    (de), a
        ld    hl, (0038h)
        ld    a, 0c9h                   ; = RET
        ld    (0038h), a                ; work around EXOS 0 crash
        exos  0
        di
        ld    (0038h), hl
        ret

printString:
        xor   a
        cp    (hl)
        ret   z
        push  bc
        push  de
        push  hl
        ld    d, h
        ld    e, l
        ld    b, a
        ld    c, a
.l1:    inc   bc
        inc   hl
        cp    (hl)
        jr    nz, .l1
        dec   a
        exos  8
        pop   hl
        pop   de
        pop   bc
        xor   a
        ret

; -----------------------------------------------------------------------------

        module  Uncompress
        include "uncomprs.s"
        endmod

        module  IView
        include "iview.s"
        endmod

        module  CView
        include "cview.s"
        endmod

        module  IPlay
        include "iplay.s"
        endmod

        module  SndPlay
        include "sndplay.s"
        endmod

        module  DL2
        include "dl2.s"
        endmod

        module  File
        include "mouse.s"
        endmod

mainCodeEnd:

    if BUILD_EXTENSION_ROM != 0
        block 10000h - $, 00h
    endif

