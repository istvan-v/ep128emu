
; assume raw Deflate input (no 2 bytes header and Adler-32 checksum)
NO_ZLIB_HEADER          equ     0
; enable support for epcompress extensions
DEFLATE_EXTENSIONS      equ     1
; border effects are disabled if this is set to any non-zero value
NO_BORDER_FX            equ     0
; decrease code size at the expense of slightly slower decompression
SIZE_OPTIMIZED          equ     0

; total data size is 864 bytes; the lower 7 bits of the start address must be 0
decompressDataBegin     equ     0fc80h

huffmanSymbolTableL     equ     decompressDataBegin
huffmanSymbolTableD     equ     huffmanSymbolTableL + 288
huffmanSymbolTableC     equ     huffmanSymbolTableD + 32
huffmanDecodeTableL     equ     decompressDataBegin + 384
huffmanDecodeTableD     equ     huffmanDecodeTableL + 64
huffmanDecodeTableC     equ     huffmanDecodeTableL + 64
decompressDataEnd       equ     decompressDataBegin + 864

;       org   0fa00h

; input/output registers:
;  DE:  decompressed data write address
;  HL:  compressed data read address
; reserved registers:
;  E:   shift register
;  DE': decompressed data write address
;  BC, BC', HL':  temporary variables

decompressData:
    if NO_ZLIB_HEADER == 0
        ld    bc, 6                     ; ignore ZLib header and Adler-32 checksum
        add   hl, bc
    endif
    if DEFLATE_EXTENSIONS != 0
        push  ix                        ; IX = last offset
    endif
        push  hl
        ex    (sp), iy                  ; IY = compressed data read address
        push  de                        ;      (+ 4 if ZLib format)
        exx
        pop   de                        ; DE' = decompressed data write address
        exx
        ld    e, 01h                    ; initialize shift register
.l1:    ld    b, 3
        call  readBits8                 ; read block header
        srl   a
        push  af                        ; save last block flag
        call  nz, huffmanInit           ; compressed block?
        jr    nz, .l5                   ; huffmanInit always returns with Z = 0
        ld    e, 01h                    ; uncompressed blocks are byte aligned
        ld    b, 32
    if NO_ZLIB_HEADER == 0
        push  iy
    endif
        call  readBits16                ; read block size, ignore 1's complement
        push  hl
        exx
        pop   bc
    if NO_ZLIB_HEADER != 0
        push  iy
    endif
        pop   hl
        add   iy, bc
        ldir                            ; copy uncompressed data
        exx
.l2:
    if NO_BORDER_FX == 0
        out   (81h), a                  ; reset border color (A = 0)
    endif
        pop   af                        ; end of block, check last block flag
        jr    nc, .l1                   ; more blocks remaining?
        exx
        push  de
        exx
        pop   de                        ; DE = address of last byte written + 1
        ex    (sp), iy
        pop   hl                        ; HL = address of last byte read + 1
    if DEFLATE_EXTENSIONS != 0
        pop   ix
    endif
        ret
.l3:    exx
        ld    (de), a                   ; store literal byte
        inc   de
.l4:    exx
.l5:    call  huffmanDecodeL            ; read next character
        jr    nc, .l3                   ; literal byte?
        or    a
        jr    z, .l2                    ; end of block?

copyLZMatch:
    if DEFLATE_EXTENSIONS == 0
        cp    1dh
        jr    nc, .l5                   ; length == 258?
        ld    h, 0
    else
        ld    hl, 2
        cp    1dh
        jr    nc, .l5                   ; special length code?
    endif
        ld    l, a                      ; decode length
        cp    9
        jr    c, .l1                    ; no extra bits?
        dec   a
        ld    b, a
        srl   b                         ; B = (number of extra bits + 1) * 2
        and   3
        or    4
        call  readLZMatchParamBits
        inc   hl
.l1:    inc   hl
        inc   hl
.l2:    push  hl                        ; save length
        ld    hl, huffmanDecodeTableD + 1
        call  huffmanDecode             ; read distance code
        ld    l, a
        ld    h, 0
        cp    4
        jr    c, .l3                    ; no extra bits?
        ld    b, a                      ; B = (number of extra bits + 1) * 2
        and   1
        or    2
        call  readLZMatchParamBits
.l3:    push  hl                        ; HL = offset - 1
    if DEFLATE_EXTENSIONS != 0
        pop   ix                        ; save last offset (IX)
    endif
.l4:    exx
    if DEFLATE_EXTENSIONS == 0
        pop   hl
    endif
        pop   bc                        ; BC' = length
        ld    a, e                      ; calculate source address
    if NO_BORDER_FX == 0
        out   (81h), a
    endif
        scf
    if DEFLATE_EXTENSIONS == 0
        sbc   a, l
        ld    l, a
        ld    a, d
        sbc   a, h
    else
        sbc   a, ixl
        ld    l, a
        ld    a, d
        sbc   a, ixh
    endif
        ld    h, a                      ; HL' = source address
        ldir
        jp    decompressData.l4
.l5:
    if DEFLATE_EXTENSIONS == 0
        ld    hl, 258
        jr    .l2
    else
        cp    1eh
        jr    z, .l2                    ; length == 2?
        push  hl
        jr    nc, .l4                   ; length == 2, repeat last offset?
        pop   hl
        inc   h
        jr    .l2                       ; length == 258
    endif

huffmanDecodeL:
        ld    hl, huffmanDecodeTableL + 1

huffmanDecode:
        xor   a
        srl   e
        jr    z, .l3
.l1:    rla
        sbc   a, (hl)
        jr    c, .l4
.l2:    inc   l
        srl   e
    if SIZE_OPTIMIZED == 0
        jr    z, .l3
        rla
        sbc   a, (hl)
        jr    c, .l4
        inc   l
        srl   e
    endif
        jp    nz, .l1
.l3:
    if NO_ZLIB_HEADER == 0
        ld    e, (iy - 4)
    else
        ld    e, (iy)
    endif
        inc   iy
        rr    e
        rla
        sbc   a, (hl)
        jp    nc, .l2
.l4:    set   5, l
        add   a, (hl)
        set   4, l
        ld    h, (hl)
        ld    l, a
        adc   a, h
        sub   l
        ld    h, a
        cp    (hl)
        inc   h
        inc   h
        ld    a, (hl)
        ret

setFixedHuffmanCode:
        push  de                        ; B = 0 from readBits8 to read header
        ld    hl, fixedHuffmanEncoding
        ld    de, huffmanSymbolTableL
        xor   a
.l1:    ld    c, (hl)
        inc   hl
        push  de
        ldi
        ex    (sp), hl
        ldir
        pop   hl
        cp    (hl)
        jr    nz, .l1
        pop   de
        ld    c, 32                     ; HDIST
        push  bc
        inc   b                         ; HLIT (288 = 256 + 32)
        jp    huffmanInit.l11

huffmanInit:
        dec   a
        jr    z, setFixedHuffmanCode
        ld    h, 2
.l1:    ld    b, 5
        call  readBits8
        inc   a
        dec   h
        ld    l, a
        push  hl                        ; HLIT, HDIST
        jr    nz, .l1
        ld    a, 4
        ld    b, a
        call  readBits8.l1
        ld    d, a                      ; HCLEN
        ld    hl, huffmanSymbolTableC + 1
        ld    b, 15
        xor   a
.l2:    ld    (hl), a
        inc   hl
        djnz  .l2
.l3:    ld    b, 3                      ; read code length encoding
        call  readBits8                 ; 16,17,18,0,
        ld    (hl), a                   ; 8,7,9,6,10,5,11,4,12,3,13,2,14,1,15
        ld    a, l
        sub   low huffmanSymbolTableC
        jr    z, .l5
        cp    16
        jr    c, .l4
        cp    18
        jr    c, .l6
        ld    a, 15
.l4:    cpl                             ; -9,-8,-10,-7,-11,...
        add   a, 8                      ; -1,0,-2,1,-3,...
.l5:    adc   a, low (huffmanSymbolTableC + 7)  ; 6,8,5,9,4,...
        ld    l, a
.l6:    inc   l
        dec   d
        jr    nz, .l3
        ld    l, low huffmanSymbolTableC
        ld    a, low huffmanDecodeTableC
        ld    c, 19                     ; B = 0 from readBits8
        call  buildDecodeTable
        pop   hl                        ; HDIST
        ld    a, l
        ex    (sp), hl                  ; HLIT
        add   a, l
        push  hl
        ld    hl, huffmanSymbolTableL
        add   a, l                      ; C = previous value for RLE (BC = 0)
        ld    d, a                      ; D = symbol table end address LSB
.l7:    push  hl
        ld    hl, huffmanDecodeTableC + 1
        call  huffmanDecode
        pop   hl
        cp    16
        jr    c, .l10                   ; not an RLE code?
        jr    z, .l8
        ld    c, b                      ; B = 0
        rla
        rla
        sub   51
.l8:    sub   14
        ld    b, a                      ; B = number of bits to read (2, 3, 7)
        and   4
        inc   a
        rla                             ; A = length - 1 (2, 2, 10)
        ld    (hl), c
        call  readBits8.l1
        ld    b, a
        ld    a, (hl)
.l9:    ld    (hl), a                   ; write RLE sequence
        inc   hl
        djnz  .l9
.l10:   ld    c, a
        ld    (hl), a
        inc   hl
        ld    a, l
        cp    d
        jr    nz, .l7
        bit   0, h
    if (huffmanSymbolTableL & 0100h) == 0
        jr    z, .l7
    else
        jr    nz, .l7
    endif
        pop   bc                        ; HLIT
.l11:   ld    hl, huffmanSymbolTableL
        ld    a, low huffmanDecodeTableL
        call  buildDecodeTable
        pop   bc                        ; HDIST
        ld    a, low huffmanDecodeTableD

; HL = source / symbol table address
; A = decode table address LSB
; BC = number of symbols

buildDecodeTable:
        push  de
        ld    d, high huffmanDecodeTableL
        ld    e, a
        push  de
        ex    (sp), ix
        xor   a
.l1:    ld    (de), a                   ; clear symbol counts
        inc   e
        bit   4, e
        jr    z, .l1
        ld    e, l
        ld    d, h
        push  bc
.l2:    ld    a, (hl)                   ; code length
        or    a
        jr    z, .l5
        ld    (.l3 + 2), a
.l3:    inc   (ix + 0)                  ; * update symbol count for this length
.l4:    jr    z, .l4                    ; halt on unsupported symbol cnt (256)
.l5:    cpi
        jp    pe, .l2
        ld    l, e                      ; calculate initial symbol table offsets
        ld    h, d
        dec   h
        ld    a, 16
.l6:    ld    (ix + 20h), l             ; store table offset - 256
        ld    (ix + 30h), h
        ld    c, (ix)
        add   hl, bc
        inc   ixl
        dec   a
        jr    nz, .l6
        pop   bc
        push  bc
        ld    l, e
        ld    h, d
.l7:    ld    a, (hl)                   ; build symbol table
        and   0fh
        jr    z, .l8                    ; unused value?
        or    ixl
        add   a, 10h
        exx
        ld    l, a
        ld    h, high huffmanDecodeTableL
        ld    c, (hl)
        set   4, l
        ld    b, (hl)
        inc   bc                        ; update and store table offset
        ld    (hl), b
        ld    l, a
        ld    (hl), c
        ld    hl, 0300h - 1
        add   hl, bc
        exx
        ld    a, l
        sub   e
        exx
        ld    (hl), a                   ; store decoded value LSB
        exx
        ld    a, h
        sbc   a, d
        exx
        dec   h
        dec   h
        rrca
        or    (hl)
        ld    (hl), a                   ; store bit 8 of decoded value
        exx
.l8:    cpi
        jp    pe, .l7
        ld    l, e
        ld    h, d
        pop   bc
        pop   ix
        pop   de
.l9:    ld    a, (hl)
        rla
        sbc   a, a                      ; MSB = FFh if decoded value >= 256
        ld    (hl), a
        cpi
        jp    pe, .l9
        dec   a                         ; return with BC = 0, Z = 0,
        ret                             ; HL = symbol table end

readBits8:
        xor   a
.l1:    ld    c, 01h
.l2:    srl   e
        jr    nc, .l4
        jr    nz, .l3
    if NO_ZLIB_HEADER == 0
        ld    e, (iy - 4)
    else
        ld    e, (iy)
    endif
        inc   iy
        rr    e
        jr    nc, .l4
.l3:    add   a, c
.l4:    sla   c
        djnz  .l2
        ret

readLZMatchParamBits:
        srl   b
        dec   b                         ; B = number of extra bits
        ld    l, a

readBits16:
        ld    a, b
.l1:    add   hl, hl
        djnz  .l1
        ld    c, 1
        srl   e
        jr    nc, .l3
        jr    z, .l5
.l2:    add   hl, bc
.l3:    dec   a
        ret   z
.l4:    sla   c
        rl    b
        srl   e
        jr    nc, .l3
        jp    nz, .l2
.l5:
    if NO_ZLIB_HEADER == 0
        ld    e, (iy - 4)
    else
        ld    e, (iy)
    endif
        inc   iy
        rr    e
        jr    c, .l2
        dec   a
        jr    nz, .l4
        ret

fixedHuffmanEncoding:
        defw  0890h, 0970h, 0718h, 0808h, 0520h
        defb  00h

    if decompressData < decompressDataBegin
        assert  $ <= decompressDataBegin
    else
        assert  decompressData >= decompressDataEnd
    endif

