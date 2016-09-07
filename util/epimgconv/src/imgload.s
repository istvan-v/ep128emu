
        org   000f0h

        defb  000h, 005h
        defw  codeEnd - codeBegin
        block 12, 000h

segmentsAllocated       equ     00100h
segmentsUsed            equ     segmentsAllocated + 8
backupSegments          equ     segmentsUsed + 4
forceColdReset          equ     backupSegments + 4
variablesEnd            equ     forceColdReset + 1

    macro exos n
        rst   030h
        defb  n
    endm

codeBegin:

main:
        di
        ld    sp, 00100h
        ld    a, 0ffh
        out   (0b2h), a
        ld    hl, resetRoutine
        ld    (0bff8h), hl
        ld    l, low segmentsAllocated
        ld    b, low (variablesEnd - segmentsAllocated)
        xor   a
.l1:    ld    (hl), a
        inc   l
        djnz  .l1
        ld    de, segmentsUsed          ; create table of segments used
        push  de
        in    a, (0b0h)
        ld    (de), a
        ld    hl, (videoDataSizeMSB)
        ld    a, h
        add   a, low (videoDataBegin - 1)
        ld    a, l
        adc   a, high (videoDataBegin - 1)
        rlca
        rlca
        and   3
        jr    z, .l2
        ld    hl, 0bffdh
        inc   e
        ld    c, a                      ; NOTE: B is set to 0 by the LDIR above
        ldir
.l2:    pop   hl                        ; HL = segmentsUsed
        ld    b, 4
.l3:    ld    a, (hl)
        inc   a
        jr    nz, .l4
        call  setForceColdReset         ; system segment is overwritten: EXOS
        jr    .l6                       ; calls and warm reset are not possible
.l4:    inc   l
        djnz  .l3
        ld    l, low segmentsAllocated  ; allocate all available segments
        defb  001h                      ; = LD BC, nnnn
.l5:    ld    (hl), c
        inc   l
        res   3, l
        exos  24
        di
        or    a
        jr    z, .l5
        cp    07fh                      ; .SHARE ?
        jr    nz, .l6
        exos  25
        di
.l6:    ld    hl, (borderColor)         ; set border color
        ld    a, l
        out   (081h), a
        ld    a, h                      ; set FIXBIAS
        and   01fh
        out   (080h), a
        xor   a
.l7:    push  af
        call  copyVideoSegment
        pop   af
        ld    c, a
        ld    hl, videoDataBegin
        call  z, setLPTAddress
        ld    a, c
        add   a, 041h                   ; NOTE: copyVideoSegment ignores b2..b7
        jr    nc, .l7

waitForSpaceKey:
        xor   a
.l1:    ld    e, a
        ld    a, 8
        out   (0b5h), a
        in    a, (0b5h)
        add   a, a
        add   a, a
        ld    a, e
        adc   a, a
        cp    0feh
        jr    nz, .l1

exitProgram:
        ld    a, (forceColdReset)
        or    a
        jr    nz, coldReset
        ld    hl, backupSegments
        ld    b, 4
.l1:    ld    a, (hl)                   ; restore saved segments
        or    a
        jr    z, .l2
        ld    a, l
        add   a, low (0fffch - backupSegments)
        ld    c, a
        ld    a, (hl)
        call  copySegment
.l2:    inc   l
        djnz  .l1

resetRoutine:
        di
        ld    sp, 00100h
        ld    a, 0ffh
        out   (0b2h), a
        ld    hl, (0bff4h)
        set   7, h
        set   6, h
        call  setLPTAddress
        ld    c, 060h
        exos  0
        ld    a, 001h
        out   (0b3h), a
        ld    a, 6
        jp    0c00dh

coldReset:
        di
        xor   a
        out   (0b3h), a
        dec   a
        out   (0b2h), a
        ld    hl, 08000h
        ld    de, 08001h
        ld    bc, 03fffh
        ld    (hl), l
        ldir
        inc   hl
        jp    (hl)                      ; HL = 0C000h

setLPTAddress:
        ld    a, l
        ld    b, 4
.l1:    srl   h
        rra
        djnz  .l1
        out   (082h), a
        ld    a, h
        out   (083h), a
        set   6, a
        out   (083h), a
        set   7, a
        out   (083h), a
        ret

setForceColdReset:
        ld    a, 0ffh
        ld    (forceColdReset), a
disableWarmReset:
        push  hl
        ld    hl, coldReset
        ld    (0bff8h), hl
        pop   hl
        ret

copyVideoSegment:
        or    0fch
        ld    c, a
        add   a, low (segmentsUsed - 0fch)
        ld    l, a
        ld    h, high segmentsUsed
        ld    a, (hl)
        or    a
        ret   z                         ; if this page is unused: nothing to do
        cp    c
        ret   z                         ; already using the correct segment ?
        ld    de, segmentsAllocated
        ld    b, 16
.l1:    ld    a, (de)
        cp    c
        jr    nz, .l2
        ld    a, e
        cp    low segmentsUsed
        jr    c, .l9                    ; found in segments allocated table,
        jr    .l3                       ; or segments used or backup table ?
.l2:    inc   e
        djnz  .l1
        call  disableWarmReset          ; not found: need to disable warm
        ld    a, l                      ; reset, and save the video segment
        add   a, low (backupSegments - segmentsUsed)
        ld    e, a
.l3:    in    a, (0b0h)
        cp    (hl)
        jr    nz, .l10                  ; is this not the page 0 segment ?
        push  hl                        ; page 0 segment:
        ld    l, low segmentsAllocated  ; an allocated segment is needed
        ld    b, 8
.l4:    ld    a, (hl)
        or    a
        jr    nz, .l8
        inc   l
        djnz  .l4
        call  setForceColdReset
        xor   a
.l5:    dec   a                         ; find an unused video segment
        ld    l, low segmentsAllocated
        ld    b, 16
.l6:    cp    (hl)
        jr    nz, .l7
        cp    0fch
        jr    nz, .l5
        jp    exitProgram               ; there is not enough memory
.l7:    inc   l
        djnz  .l6
        defb  021h                      ; = LD HL, nnnn
.l8:    ld    (hl), 0
        pop   hl
        ld    (de), a
        ld    (hl), c
        ld    c, a
        ld    a, (hl)
        call  copySegment               ; video segment -> backup segment
        ld    c, a
        in    a, (0b0h)
        jr    copySegment               ; video data -> video segment
.l9:    ld    a, (hl)
        ld    (de), a
        ld    (hl), c
        jr    copySegment
.l10:   ld    a, (hl)
        ld    (de), a
        ld    (hl), c

; swap segment A and segment C

swapSegments:
        call  copySegment.l2
        di
        ld    (.l2 + 1), sp
        ld    sp, 04000h
        ld    hl, 0c000h
.l1:    pop   bc
        ld    e, (hl)
        ld    (hl), c
        inc   l
        ld    d, (hl)
        ld    (hl), b
        push  de
        pop   de
        inc   l
        jp    nz, .l1
        inc   h
        jr    nz, .l1
.l2:    ld    sp, 00000h
        jr    copySegment.l1

; copy segment A to segment C

copySegment:
        call  .l2
        ld    hl, 04000h
        ld    de, 0c000h
        ld    b, h
        ld    c, l
        ldir
.l1:    pop   af
        pop   bc
        pop   de
        pop   hl
        ret
.l2:    ex    (sp), hl
        push  de
        push  bc
        push  af
        out   (0b1h), a
        ld    a, c
        out   (0b3h), a
        jp    (hl)

; the LPT must be aligned to 16 bytes
    if ($ & 15) != 12
        block 16 - (($ + 4) & 15), 000h
    endif

codeEnd:

        assert  (segmentsAllocated & 0ffh) == 0
        assert  (resetRoutine & 0ff00h) == (segmentsAllocated & 0ff00h)

borderColor             equ     codeEnd
fixBias                 equ     codeEnd + 1
videoDataSizeMSB        equ     codeEnd + 2
videoDataSizeLSB        equ     codeEnd + 3
videoDataBegin          equ     codeEnd + 4

