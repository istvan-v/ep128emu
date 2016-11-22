
; border effects are disabled if this is set to any non-zero value
NO_BORDER_FX            equ     0
; do not reset memory paging and stack pointer after decompression
NO_CLEANUP              equ     0
; if non-zero, code size decreases by 9 bytes at the expense of ~2% slowdown
SIZE_OPTIMIZED          equ     0

        org   00feh

        defw  endMain - main

main:
        di
.l1:    ld    sp, 0100h
        ld    a, 0ffh
        out   (0b2h), a
        ld    bc, endMain + (decompressCodeEnd - decompressCodeBegin) - 1
        ld    hl, (compressedSize)
        add   hl, bc                    ; address of last byte to be copied
        ld    a, h
        rlca
        rlca
        push  hl
        ld    bc, 10000h - decompressCodeBegin
        ld    hl, (uncompressedEndAddr)
        add   hl, bc
        or    0fch
        ld    l, a                      ; L: last 16K page / compressed
        ld    a, h
        rlca
        rlca
        or    0fch                      ; last 16K page / uncompressed
        ld    h, 0bfh
        sub   l                         ; number of segments to allocate
        jr    z, .l3
.l2:    ex    af, af'                   ; allocate segments if needed
        rst   30h
        defb  24                        ; EXOS 24
        di
        add   a, a
        jr    c, .l4                    ; .NOSEG ?
        inc   l
        ld    (hl), c                   ; store segment numbers at BFFC-BFFF
        ex    af, af'
        dec   a
        jr    nz, .l2
.l3:    ld    a, (hl)                   ; initialize memory paging
        out   (0b3h), a                 ; page 3 is always set to last segment
        ld    a, l                      ; A: last 16K page / uncompressed
        ld    l, 0fdh
        ld    c, 0b1h
        outi
        inc   c
        outi
        exx
        pop   de
        ld    hl, 10000h - endMain
        add   hl, de
        ld    c, l                      ; BC': number of bytes to be copied
        ld    b, h                      ; (checksum byte is not copied)
        ex    de, hl                    ; HL': address of last byte to be copied
        rrca                            ; DE': address of last byte written
        rrca                            ; by copying compressed data and
        ld    d, a                      ; decompressor code
        ld    e, low (decompressCodeEnd - 1)
        lddr
        ex    de, hl                    ; initialize read position (HL'),
        inc   hl
        ld    e, 80h                    ; and shift register (E')
        exx
        ld    sp, 0fffch
        xor   a
        ld    e, a                      ; decompressed program start address L
        jp    decompressData
.l4:    ld    c, 40h                    ; insufficient memory: reset machine
.l5:    rst   30h
        defb  0                         ; EXOS 0
        ld    c, 80h
        jr    .l5

uncompressedEndAddr:
        defw  0000h                     ; address of last decompressed byte + 1
compressedSize:
        defw  0000h                     ; compressed data size

endMain:

; -----------------------------------------------------------------------------

; total table size is 156 bytes, and should not cross a 256-byte page boundary
decodeTablesBegin       equ     0ff4ch

nLengthSlots            equ     8
nOffs1Slots             equ     4
nOffs2Slots             equ     8
maxOffs3Slots           equ     32

lengthDecodeTable       equ     decodeTablesBegin
offs1DecodeTable        equ     lengthDecodeTable + (nLengthSlots * 3)
offs2DecodeTable        equ     offs1DecodeTable + (nOffs1Slots * 3)
offs3DecodeTable        equ     offs2DecodeTable + (nOffs2Slots * 3)
decodeTablesEnd         equ     offs3DecodeTable + (maxOffs3Slots * 3)

        assert  decodeTablesBegin >= ((decodeTablesEnd - 1) & 0ff00h)

        phase 0fe40h

decompressCodeBegin:

; BC': symbols (literal byte or match code) remaining
; D':  prefix size code for length >= 3 matches
; E':  shift register
; HL': compressed data read address
; A:   temp. register
; BC:  temp. register (number of literal/LZ77 bytes to copy)
; DE:  decompressed data write address
; HL:  temp. register (literal/LZ77 data source address)

; NOTE: A, E, and BC' must be zero

decompressData:
        ld    d, 01h                    ; decompressed program start address H
.l1:    ld    bc, 2001h                 ; FIXME: start address is ignored
        ld    h, a
        call  readBits16                ; read the number of symbols (HL)
        ld    c, 0                      ; set C = 0 for read2Bits/readBits
        call  read2Bits                 ; read flag bits
        srl   a
        push  af                        ; save last block flag (A=1, Z=0: yes)
    if NO_CLEANUP == 0
        jp    nc, .l14                  ; uncompressed data ?
    else
        jr    nc, .l14
    endif
        call  read2Bits                 ; get prefix size for >= 3 byte matches
        push  de                        ; save decompressed data write address
        push  hl
        exx
        ld    b, a
        ld    a, 02h                    ; len >= 3 offset slots: 4, 8, 16, 32
        ld    d, 80h                    ; prefix size codes: 40h, 20h, 10h, 08h
        inc   b
.l2:    rlca
        srl   d                         ; D' = prefix size code for length >= 3
        djnz  .l2
        pop   bc                        ; store the number of symbols in BC'
        exx
        add   a, nLengthSlots + nOffs1Slots + nOffs2Slots - 3
        ld    b, a                      ; store total table size - 3 in B
        ld    hl, decodeTablesBegin     ; initialize decode tables
.l3:    ld    de, 1
.l4:    ld    a, 10h                    ; NOTE: C is 0 here, as set above
        call  readBits
        ld    (hl), a                   ; store the number of bits to read
        inc   hl
        ld    (hl), e                   ; store base value LSB
        inc   hl
        ld    (hl), d                   ; store base value MSB
        inc   hl
        push  hl
        ld    hl, 1                     ; calculate 2 ^ nBits
        jr    z, .l6                    ; readBits sets Z = 1 if A = 0
.l5:    add   hl, hl
        dec   a
        jr    nz, .l5
.l6:    add   hl, de                    ; calculate new base value
        ex    de, hl
        pop   hl
        ld    a, l
        cp    low offs1DecodeTable
        jr    z, .l3                    ; end of length decode table ?
        cp    low offs2DecodeTable
        jr    z, .l3                    ; end of offset table for length = 1 ?
        cp    low offs3DecodeTable
        jr    z, .l3                    ; end of offset table for length = 2 ?
        djnz  .l4                       ; continue until all tables are read
        pop   de                        ; DE = decompressed data write address
        jr    .l9                       ; jump to main decompress loop
.l7:    exx
        pop   af                        ; check last block flag:
        jr    z, .l1                    ; more blocks remaining ?
    if NO_BORDER_FX == 0
        xor   a                         ; reset border color
        out   (081h), a
      if NO_CLEANUP == 0
        dec   a                         ; reset memory paging and stack pointer
      endif
    else
      if NO_CLEANUP == 0
        ld    a, 0ffh
      endif
    endif
    if NO_CLEANUP == 0
        out   (0b2h), a
        in    a, (0b3h)
        out   (0b1h), a
        ld    sp, 0b217h
    endif
        jp    main                      ; start decompressed program
.l8:    ld    a, (hl)                   ; copy literal byte
        inc   hl
        exx
        ld    (de), a
        inc   de
.l9:    exx
.l10:   ld    a, c                      ; check the data size remaining:
        or    b
        jr    z, .l7                    ; end of block ?
        dec   bc
        sla   e                         ; read flag bit
    if SIZE_OPTIMIZED == 0
        jr    nz, .l11
        ld    e, (hl)
        inc   hl
        rl    e
    else
        call  z, readCompressedByte
    endif
.l11:   jr    nc, .l8                   ; literal byte ?
        ld    a, 0f8h
.l12:   sla   e                         ; read length prefix bits
    if SIZE_OPTIMIZED == 0
        jr    nz, .l13
        ld    e, (hl)
        inc   hl
        rl    e
    else
        call  z, readCompressedByte
    endif
.l13:   jr    nc, copyLZMatch           ; LZ77 match ?
        inc   a
        jr    nz, .l12
        exx                             ; literal sequence:
        ld    bc, 0811h                 ; 0b1, 0b11111111, 0bxxxxxxxx
        ld    h, a
        call  readBits16                ; length is 8-bit value + 17
.l14:   ld    c, l                      ; copy literal sequence,
        ld    b, h                      ; or uncompressed block
        exx
        push  hl
        exx
        pop   hl
        ldir
        push  hl
        exx
        pop   hl
        jr    .l10                      ; return to main decompress loop

copyLZMatch:
        exx
        ld    b, low (lengthDecodeTable + 24)
        call  readEncodedValue          ; decode match length
        ld    c, 20h                    ; C = 20h: not readBits routine
        or    h                         ; if length <= 255, then A and H are 0
        jr    nz, .l6                   ; length >= 256 bytes ?
        ld    b, l
        djnz  .l5                       ; length > 1 byte ?
        ld    b, low offs1DecodeTable   ; no, read 2 prefix bits
.l1:    ld    a, 40h                    ; read2Bits routine if C is 0
.l2:    exx                             ; readBits routine if C is 0
.l3:    sla   e                         ; if C is FFh, read offset prefix bits
    if SIZE_OPTIMIZED == 0
        jp    nz, .l4
        ld    e, (hl)
        inc   hl
        rl    e
    else
        call  z, readCompressedByte
    endif
.l4:    rla
        jr    nc, .l3
        exx
        cp    c
        ret   nc
        push  hl
        call  readEncodedValue          ; decode match offset
        ld    a, e                      ; calculate LZ77 match read address
    if NO_BORDER_FX == 0
        out   (81h), a
    endif
        sub   l
        ld    l, a
        ld    a, d
        sbc   a, h
        ld    h, a
        pop   bc
        ldir                            ; copy match data
        jr    decompressData.l9         ; return to main decompress loop
.l5:    djnz  .l6                       ; length > 2 bytes ?
        ld    a, c                      ; no, read 3 prefix bits (C = 20h)
        ld    b, low offs2DecodeTable
        jr    .l2
.l6:    exx                             ; length >= 3 bytes,
        ld    a, d                      ; variable prefix size
        exx
        ld    b, low offs3DecodeTable
        jr    .l2

; NOTE: C must be 0 when calling these
read2Bits       equ     copyLZMatch.l1
; read 1 to 8 bits to A for A = 80h, 40h, 20h, 10h, 08h, 04h, 02h, 01h
readBits        equ     copyLZMatch.l2

readEncodedValue:
        ld    l, a                      ; calculate table address L (3 * A + B)
        add   a, a
        add   a, l
        add   a, b
        ld    l, a
        ld    h, high decodeTablesBegin
        ld    b, (hl)                   ; B = number of prefix bits
        inc   l
        ld    c, (hl)                   ; AC = base value
        inc   l
        ld    h, (hl)
        xor   a

; read B bits to HL, and add HC to the result; A must be zero

readBits16:
        ld    l, c
        cp    b
        ret   z
        ld    c, a
.l1:    exx
        sla   e
    if SIZE_OPTIMIZED == 0
        jp    nz, .l2
        ld    e, (hl)
        inc   hl
        rl    e
    else
        call  z, readCompressedByte
    endif
.l2:    exx
        rl    c
        rla
        djnz  .l1
        ld    b, a
        add   hl, bc
        ret

    if SIZE_OPTIMIZED != 0
readCompressedByte:
        ld    e, (hl)
        inc   hl
        rl    e
        ret
    endif

        assert  ($ <= decodeTablesBegin) || (decompressData >= decodeTablesEnd)

decompressCodeEnd:

        dephase

