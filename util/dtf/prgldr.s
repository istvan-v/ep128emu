
NO_STATUS_LINE  equ     1

    macro exos n
        rst   30h
        defb  n
    endm

        org   00f0h
        defw  0500h, codeEnd - 0100h, 0, 0, 0, 0, 0, 0

        di
        ld    sp, 0100h
        ld    c, 40h
        exos  0
        ld    a, 0c3h
        ld    (0028h), a
        ld    hl, 0069h
        ld    (0029h), hl
        ld    hl, loaderCodeBegin
        ld    de, 0060h
        push  de
        ld    bc, loaderCodeEnd - loaderCodeBegin
        ldir
        ei
        ld    a, 1
        ld    de, fileName
        exos  1
        or    a
        jr    nz, fileError + (loaderCodeBegin - 0060h)
    if NO_STATUS_LINE == 0
        dec   a
        out   (0b2h), a
        ld    a, 20h
        ld    (fileName), a
        ld    hl, fileName - 12
        ld    de, (0bff6h)
        ld    bc, 40
        ldir
    endif
        ld    a, 0fdh
        out   (0b1h), a
        add   a, a
        out   (0b2h), a
        inc   a
        out   (0b3h), a
        ret

loaderCodeBegin:

        phase 0060h

        call  0066h
        jp    0100h
        ld    de, 0100h                 ; 0066h
        push  de                        ; 0069h
        ld    de, .l1 + 1
        ld    bc, 2
        call  .l2
.l1:    ld    bc, 0                     ; *
        pop   de
.l2:    ld    a, 1
        exos  6
        ld    l, e
        ld    h, d
        or    a
        in    a, (0b2h)
        ret   z

fileError:
        di
        ld    sp, 0100h
        ld    c, 60h
.l1:    exos  0
        ld    c, 80h
        jr    .l1

        dephase

loaderCodeEnd:

    if NO_STATUS_LINE == 0
        defm  "     LOADING"
    endif
fileName:
        defb  00h
        block 27, 20h
codeEnd:

