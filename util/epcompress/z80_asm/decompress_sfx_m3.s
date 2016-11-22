
; border effects are disabled if this is set to any non-zero value
NO_BORDER_FX            equ     0
; do not reset memory paging and stack pointer after decompression
NO_CLEANUP              equ     0

    macro exos n
        rst   30h
        defb  n
    endm

        org   00feh

        defw  endMain - main

main:
        di
.l1:    ld    sp, 0100h
        ld    a, 0ffh
        out   (0b2h), a
        ld    hl, (uncompressedEndAddr)
        dec   hl
        push  hl
        ld    a, h
        rlca
        rlca
        ld    de, 00fch
        ld    hl, (compressedSize)
        add   hl, de
        push  hl
        exx
        pop   hl                        ; HL': compressed data read address + 1
        exx
        ld    bc, endMain + (decompressCodeEnd - decompressCodeBegin) - 00fdh
        add   hl, bc
        or    e
        ld    b, a                      ; B: last 16K page / uncompressed
        push  hl
        ld    a, h
        rlca
        rlca
        or    e
        ld    l, a                      ; L: last 16K page / compressed
        ld    h, 0bfh
        sub   b
        jr    z, .l3
.l2:    ex    af, af'                   ; allocate segments if needed
        exos  24
        di
        add   a, a
        jr    c, .l4                    ; .NOSEG ?
        inc   l
        ld    (hl), c                   ; store segment numbers at BFFC-BFFF
        ex    af, af'
        inc   a
        jr    nz, .l2
.l3:    ld    a, (hl)                   ; initialize memory paging
        out   (0b3h), a                 ; page 3 is always set to last segment
        ld    l, 0fdh
        ld    a, (hl)
        out   (0b1h), a
        inc   l
        ld    a, (hl)
        out   (0b2h), a
        pop   hl
        ld    de, decompressCodeEnd - 1
        ld    bc, decompressCodeEnd - decompressCodeBegin
        lddr                            ; copy decompressor code
        ld    hl, 1001h
        push  hl
        ld    hl, endMain + 2
        ld    de, main
        ld    bc, (endMain)
        jp    decompressData
.l4:    ld    c, 40h                    ; insufficient memory: reset machine
.l5:    exos  0
        ld    c, 80h
        jr    .l5

uncompressedEndAddr:
        defw  0000h                     ; address of last decompressed byte + 1
compressedSize:
        defw  0000h                     ; compressed data size

endMain:

; -----------------------------------------------------------------------------

        phase 0060h

decompressCodeBegin:

; HL: compressed data start address
; HL': compressed data read address + 1
; DE: decompressed data start address

decompressData:
        ldir
        pop   bc                        ; BC = 1001h
        pop   de
        ld    a, (hl)
        inc   hl
        or    (hl)
        jr    z, decompressDone         ; no compression ?
                                        ; initialize shift register,
        call  readBits.l2               ; fill 2-byte input buffer (BC')
.l1:    call  readLength                ; get literal sequence length
.l2:    exx
        ld    a, b
        ld    b, c
        dec   hl
        ld    c, (hl)                   ; read compressed byte
        exx
    if NO_BORDER_FX == 0
        out   (81h), a
    endif
        jr    nc, .l3
        rra
        ld    c, a
        ret
.l3:    ld    (de), a                   ; copy literal bytes
        dec   de
        dec   hl
        ld    a, l
        or    h
        jr    nz, .l2
.l4:    call  readLength                ; get LZ77 match length
        push  hl
        ld    l, b
        ld    b, 4
        jr    z, .l6                    ; length = 2 bytes ?
        call  readBits.l2               ; no, read 4 offset prefix bits
        ld    b, l
        call  readBits                  ; read and decode offset
.l5:    add   hl, de                    ; calculate source address,
        ld    a, c
        ldd                             ; and expand match
        pop   bc
        lddr
        ld    c, a                      ; restore shift register
        srl   c                         ; check flag bit:
        call  z, readCompressedByte
        jr    nc, .l1                   ; literal sequence ?
        jr    .l4                       ; no, continue with another LZ77 match
.l6:    call  readBits.l3               ; length = 2: read 3 offset prefix bits
        ld    b, l
        call  readBits.l1               ; read and decode offset
        dec   hl
        jr    .l5

readCompressedByte      equ     decompressData.l2

readLength:
.l1:    ld    a, d
        or    a
        jr    nz, .l4                   ; end of data not reached yet ?
        pop   de
.l2:
    if NO_BORDER_FX == 0
        out   (81h), a                  ; reset border color (A = 0)
    endif
    if NO_CLEANUP == 0
        dec   a                         ; reset memory paging and stack pointer
        out   (0b2h), a
        in    a, (0b3h)
        out   (0b1h), a
        ld    sp, 0b217h
    endif
        jp    main                      ; start decompressed program
.l3:    inc   b
.l4:    srl   c
        call  z, readCompressedByte
        jr    c, .l3

decompressDone          equ     readLength.l2

readBits:
        dec   b
.l1:    ld    hl, 1
        inc   b
        ret   z                         ; return with Z=1 if no bits are read
.l2:    srl   c
        call  z, readCompressedByte
        adc   hl, hl
.l3:    djnz  .l2
        ret

decompressCodeEnd:

        dephase

