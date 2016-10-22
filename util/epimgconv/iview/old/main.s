
BUILD_EXTENSION_ROM     equ     0

    if BUILD_EXTENSION_ROM == 0
        output  "cview.ext"
        org     0bffah
        defb    000h, 006h
        defw    codeEnd - Main.main
        block   12, 000h
    else
        output  "cview.rom"
        org     0c000h
        defm    "EXOS_ROM"
        defw    00000h
    endif

        module  Main

main:
        ld    a, c
        cp    6
        jr    z, .l1
        call  IView.extMain
        dec   c
        inc   c
        call  nz, CView.extMain
        dec   c
        inc   c
        call  nz, IPlay.extMain
        dec   c
        inc   c
        call  nz, Uncompress.extMain
        dec   c
        inc   c
        jp    nz, Uncompres2.extMain
        or    a
        ret
.l1:    inc   de
        ld    a, (de)
        dec   de
        cp    049h
        jr    z, .l2
        cp    076h
        jp    z, IPlay.extMain
        xor   a
        ret
.l2:    ld    ixh, d
        ld    ixl, e
        ld    a, (ix + 10)
        or    a
        jp    z, IView.extMain
        jp    CView.extMain

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

        module  Uncompress
        include "uncompress.s"
        endmod

        module  Uncompres2
        include "uncompres2.s"
        endmod

codeEnd:

    if BUILD_EXTENSION_ROM != 0
        size  16384
    endif

