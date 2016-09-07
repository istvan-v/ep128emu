
        org   0c00ah

loaderCodeBegin:

extMain:
        di
        push  af
        push  bc
        push  de
        ld    hl, (compressedSize)
        ld    bc, codeEnd - 1
        add   hl, bc
        ld    de, decompressCodeEnd - 1
        ld    bc, decompressCodeEnd - decompressCodeBegin
        lddr                            ; copy decompressor code
        ld    hl, (loaderCodeEnd)
        ld    c, l                      ; BC: compressed data size
        ld    b, h
        ld    de, extMain               ; initialize decompress write address
        push  de
        add   hl, de
        push  hl
        exx
        ex    (sp), hl                  ; HL': compressed data read address + 1
        push  bc
        exx
        ld    hl, loaderCodeEnd + 2
        jp    decompressData

compressedSize:
        defw  0

loaderCodeEnd:

        assert  loaderCodeEnd == 0c034h

; -----------------------------------------------------------------------------

        phase 0ff84h

decompressCodeBegin:

; HL: compressed data start address
; HL': compressed data read address + 1
; DE: decompressed data start address

decompressData:
        ldir
        ld    c, (hl)                   ; read uncompressed data size increase
        inc   hl
        ld    b, (hl)
        ex    de, hl
        add   hl, bc
        ex    de, hl
        ld    a, c
        or    b
        jr    z, decompressDone         ; no compression ?
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
.l1:    ld    hl, 010000h - extMain     ; 2's complement of decompressed data
        add   hl, de                    ; start address
        jr    c, .l4                    ; end of data not reached yet ?
        pop   de
.l2:    exx
        pop   bc
        pop   hl
        exx
        pop   hl                        ; HL = extMain (C00Ah)
        pop   de
        pop   bc
        pop   af
        ei
        jp    (hl)
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

codeEnd:

