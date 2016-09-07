
NO_BORDER_FX    equ     1

        org   0ff60h

; HL: compressed data start address
; DE: decompressed data start address (updated on return)

decompressData:
        di
        push  hl
        xor   a                         ; calculate and store 2's complement of
        ld    l, a                      ; decompressed data start address
        ld    h, a
        sbc   hl, de
        ld    (readLength.l1 + 1), hl
        pop   hl
        ld    c, (hl)                   ; read compressed data size
        inc   hl
        ld    b, (hl)
        adc   hl, bc
        push  hl
        exx
        pop   hl                        ; HL': compressed data read address + 1
        exx
        ex    de, hl
        add   hl, bc
        ld    a, (de)                   ; read uncompressed data size increase
        inc   de
        ld    c, a
        ld    a, (de)
        ld    b, a
        add   hl, bc
        ex    de, hl
        or    c
        jr    z, decompressDone         ; no compression ?
        push  de
        dec   de                        ; DE: decompressed data write address
        ld    bc, 1001h                 ; initialize shift register,
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
.l1:    ld    hl, 0                     ; * 2's complement of decompressed data
        add   hl, de                    ; start address
        jr    c, .l4                    ; end of data not reached yet ?
        pop   de
        pop   de
.l2:
    if NO_BORDER_FX == 0
        xor   a
        out   (81h), a
    endif
        ei
        ret
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

