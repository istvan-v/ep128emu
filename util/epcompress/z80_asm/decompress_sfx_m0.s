
; zero: decompressed data start address is passed to decompressData in DE
; non-zero: decompressed data start address is read from the compressed data
USE_BLOCK_START_ADDR    equ     1
; border effects are disabled if this is set to any non-zero value
NO_BORDER_FX            equ     0
; do not reset memory paging and stack pointer after decompression
NO_CLEANUP              equ     0
; do not verify checksum
NO_CRC_CHECK            equ     0

        org   00feh

        defw  endMain - main

main:
        di
.l1:    ld    sp, 0100h
        ld    a, 0ffh
        out   (0b2h), a
        ld    bc, endMain + (decompressCodeEnd - decompressCodeBegin) - 1
        ld    hl, (compressedSize)
        add   hl, bc
        ld    a, h
        rlca
        rlca
        ex    de, hl                    ; DE: address of last byte to be copied
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
        ld    b, a
.l2:    exx                             ; allocate segments if needed
        rst   30h
        defb  24                        ; EXOS 24
        di
        add   a, a
        jr    c, .l4                    ; .NOSEG ?
        ld    a, c
        exx
        inc   l
        ld    (hl), a                   ; store segment numbers at BFFC-BFFF
        djnz  .l2
.l3:    ld    a, (hl)                   ; initialize memory paging
        out   (0b3h), a                 ; page 3 is always set to last segment
        ld    a, l                      ; A: last 16K page / uncompressed
        ld    l, 0fdh
        ld    c, 0b1h
        outi
        inc   c
        outi
        ld    sp, 0fffch
        ld    hl, 10001h - endMain
        add   hl, de
        push  de
        ex    (sp), hl                  ; HL: address of last byte to be copied
        pop   bc                        ; BC: number of bytes to be copied
        ld    de, decompressCodeEnd - 1 ; DE: address of last byte written
        rrca                            ; by copying compressed data and
        rrca                            ; decompressor code
        and   d
        ld    d, a
        lddr
        ex    de, hl                    ; initialize read position
        inc   hl
        jp    decompressMain
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

; =============================================================================

; total data size is 864 bytes; the lower 7 bits of the start address must be 0
decompressDataBegin     equ     0fc80h

huffmanSymbolTable1     equ     decompressDataBegin
huffmanSymbolTable2     equ     huffmanSymbolTable1 + 324
prvDistanceTable        equ     huffmanSymbolTable2 + 28
huffmanDecodeTable      equ     decompressDataBegin + 384
huffmanDecodeTableL     equ     huffmanDecodeTable + 64
decompressDataEnd       equ     decompressDataBegin + 864

; NOTE: the upper byte of the address of huffmanDecode1, huffmanDecode2,
; read9Bits, read8Bits and read5Bits must be the same (see assert directive
; below)

        phase 0fa9ch

decompressCodeBegin:

; input/output registers:
;  DE:  decompressed data write address
;  HL:  compressed data read address

decompressData:
        push  ix
        inc   hl                        ; ignore checksum byte
        push  hl
        ex    (sp), iy                  ; IY = compressed data read address
    if USE_BLOCK_START_ADDR == 0
        push  de
        exx
        pop   de                        ; DE' = decompressed data write address
        exx
    endif
        ld    e, 80h                    ; initialize shift register
        jp    decompressAllDataBlocks   ; decompress all blocks

; reserved registers:
;  D:   previous offsets table write position
;  E:   shift register
;  DE': decompressed data write address
;  IX:  2's complement of bytes remaining
;  IY:  compressed data read address

huffmanDecode1:
        ld    hl, huffmanDecodeTable

huffmanDecode2:
        xor   a
        sla   e
        jr    z, .l3
.l1:    rla
        sbc   a, (hl)
        jr    c, .l4
.l2:    inc   l
        sla   e
        jp    nz, .l1
.l3:    ld    e, (iy)
        inc   iy
        rl    e
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

read5Bits:
        ld    b, 5
        defb  21h                       ; = LD HL, nnnn

read8Bits:
        ld    b, 8
        defb  21h                       ; = LD HL, nnnn

read9Bits:
        ld    b, 9

        assert  (huffmanDecode1 & 0ff00h) == (read9Bits & 0ff00h)

readBBits:
        xor   a
.l1:    sla   e
        call  z, readCompressedByte
        adc   a, a
        djnz  .l1
        ret

readCompressedByte:
        ld    e, (iy)
        inc   iy
        rl    e                         ; assume carry is set
        ret

gammaDecode:
        xor   a
        ld    b, a
        inc   a
.l1:    sla   e
        call  z, readCompressedByte
        ret   nc
        sla   e
        call  z, readCompressedByte
        rla
        rl    b
        jp    .l1

read16Bits:
        ld    a, (16 + 1) * 4

readLZMatchParameterBits:
        ld    b, a
        srl   b
        srl   b
        dec   b
        and   3
        or    4
        ld    c, 0
.l1:    sla   e
        jr    z, .l3
.l2:    rla
        rl    c
        djnz  .l1
        ld    b, c
        ret
.l3:    ld    e, (iy)
        inc   iy
        rl    e
        jp    .l2

huffmanInit:
        ld    hl, addrTable1            ; read main Huffman table
        ld    bc, readCharAddrLow
        call  .l1
        ld    hl, addrTable2            ; read match length Huffman table
        ld    bc, readLengthAddrLow
.l1:    sla   e                         ; is Huffman encoding enabled?
        call  z, readCompressedByte
        ld    a, (hl)                   ; if not, use 9 or 5 bit literals,
        ld    (bc), a
        ret   nc                        ; and return
        inc   hl
        ld    a, (hl)                   ; set decoder routine address
        ld    (bc), a
        inc   hl
        ld    c, (hl)                   ; table write address in BC
        inc   hl
        ld    b, (hl)
        inc   hl
        ld    l, (hl)                   ; symbol cnt/offset table address in HL
        ld    h, high huffmanDecodeTable
.l2:    push  bc
        call  gammaDecode               ; get number of symbols with this size
        ld    c, a
        dec   bc
        ld    d, c                      ; D = number of symbols remaining & FFh
        dec   b
        pop   bc
.l3:    jr    z, .l3                    ; halt on unsupported symbol cnt >= 256
        ld    (hl), d
        set   5, l
        dec   a
        jr    z, .l5                    ; unused code length?
        push  hl
        ld    hl, 0ffffh                ; HL = decoded value (from -1)
.l4:    push  bc
        call  gammaDecode               ; get difference from previous symbol
        ld    c, a
        add   hl, bc                    ; calculate decoded value,
        pop   bc
        xor   a                         ; and store it in the symbol table
        sub   h                         ; MSB = FFh if value >= 256
        ld    (bc), a
        inc   b
        inc   b
        ld    a, l
        ld    (bc), a
        dec   b
        dec   b
        inc   bc
        dec   d                         ; check the number of symbols remaining
        jr    nz, .l4                   ; not done with this code size yet?
        pop   hl
.l5:    ld    (hl), c                   ; store table address offset - 256
        set   4, l
        ld    (hl), b
        dec   (hl)
        ld    a, l
        sub   30h - 1
        ld    l, a
        and   0fh
        jr    nz, .l2                   ; continue with the next code size
        ret

decompressAllDataBlocks:
.l1:
    if USE_BLOCK_START_ADDR != 0
        call  read16Bits                ; read block start address
        ld    c, a
        push  bc
        exx
        pop   de
        exx
    endif
        call  read16Bits                ; read 2's complement of block length
        ld    ixl, a
        ld    ixh, b                    ; IX = -(block length)
        ld    b, 2                      ; read 'last block' and
        call  readBBits                 ; 'compression enabled' flags
        srl   a
        push  af                        ; save last block flag
        ld    a, low read8Bits
        ld    (readCharAddrLow), a
        call  c, huffmanInit            ; initialize Huffman decoding
.l2:    call  huffmanDecode1            ; * read next character
        jr    c, copyLZMatch            ; match code?
        exx
        ld    (de), a                   ; no, store literal byte
        inc   de                        ; increment write address
.l3:    exx
        inc   ixl                       ; check bytes remaining
        jp    nz, .l2
    if NO_BORDER_FX == 0
        ld    a, e
        out   (81h), a
    endif
        inc   ixh
        jr    nz, .l2
        pop   af                        ; check last block flag:
        jr    z, .l1                    ; more blocks remaining?
        exx
        push  de
        exx
        pop   de                        ; DE = address of last byte written + 1
        ex    (sp), iy
        pop   hl                        ; HL = address of last byte read + 1
        pop   ix
    if NO_BORDER_FX == 0
        xor   a                         ; reset border color
        out   (81h), a
    endif
        ret

readCharAddrLow         equ     decompressAllDataBlocks.l2 + 1

copyLZMatch:
        cp    8
        jr    c, .l2                    ; short offset?
        cp    3ch
        jr    nc, .l3                   ; special match code?
        call  readLZMatchParameterBits  ; no, read extra bits
        ld    c, a                      ; BC = match offset - 1
        ld    a, d
        dec   a
        and   6
        ld    d, a
        add   a, low prvDistanceTable
        ld    l, a
        ld    h, high prvDistanceTable
        ld    (hl), c                   ; store in recent offsets table
        inc   l
        ld    (hl), b
        push  bc
        exx
        pop   bc
.l1:    call  getLZMatchParameters
        ldi
        ldir                            ; expand LZ match
        jp    decompressAllDataBlocks.l3
.l2:    exx                             ; short offset
        ld    b, 0
        ld    c, a
        jp    .l1
.l3:    add   a, a
        jp    p, .l4                    ; LZ match with delta value?
        add   a, d                      ; no, use recent offset from table
        and   6
        add   a, low prvDistanceTable
        exx
        ld    l, a
        ld    h, high prvDistanceTable
        ld    c, (hl)
        inc   l
        ld    b, (hl)
        jp    .l1
.l4:    ld    b, 7                      ; read and decode delta value (7 bits)
        and   b
        rra
        ld    c, a                      ; save offset
        call  readBBits                 ; NOTE: readBBits clears B to zero
        push  bc
        add   a, 0c0h
        adc   a, b
        ld    (.l6 + 1), a
        exx
        pop   bc                        ; BC' = match offset - 1
        call  getLZMatchParameters
.l5:    ld    a, (hl)                   ; expand match
.l6:    add   a, 0                      ; * add delta value
        ld    (de), a
        inc   de
        cpi
        jp    pe, .l5
        jp    decompressAllDataBlocks.l3

getLZMatchParameters:
        ld    h, d                      ; calculate match start address (HL')
        ld    l, e
        scf
        sbc   hl, bc
        exx
    if NO_BORDER_FX == 0
        ld    a, e
        out   (81h), a
    endif
        ld    hl, huffmanDecodeTableL
.l1:    call  huffmanDecode2            ; * get match length
        cp    8
        call  nc, readLZMatchParameterBits  ; long match?
        exx
        ld    b, 0
        ld    c, a
        inc   bc                        ; BC = match length - 1
        add   ix, bc                    ; update bytes remaining
        inc   bc
        ret

readLengthAddrLow       equ     getLZMatchParameters.l1 + 1

addrTable1:
        defb  low read9Bits
        defb  low huffmanDecode1
        defw  huffmanSymbolTable1
        defb  low huffmanDecodeTable

addrTable2:
        defb  low read5Bits
        defb  low huffmanDecode2
        defw  huffmanSymbolTable2
        defb  low huffmanDecodeTableL

; -----------------------------------------------------------------------------

decompressMain:
    if NO_CRC_CHECK == 0
        push  hl                        ; verify checksum
        ld    bc, (compressedSize)
        dec   bc
        inc   c
        inc   b
        ld    a, 80h
        ld    e, 0c4h
.l1:    sub   e
        rrca
        xor   (hl)
        inc   hl
        djnz  .l1
        dec   c
        jr    nz, .l1
        pop   hl
        or    a
        jr    nz, .l2                   ; checksum error ?
    endif
        call  decompressData            ; decompress all blocks
    if NO_BORDER_FX == 0
        xor   a                         ; reset border color
        out   (81h), a
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
.l2:
    if NO_CRC_CHECK == 0
        ld    c, 40h                    ; checksum error: reset machine
.l3:    rst   30h
        defb  0
        ld    c, 80h
        jr    .l3
    endif

    if decompressData < decompressDataBegin
        assert  $ <= decompressDataBegin
    else
        assert  decompressData >= decompressDataEnd
    endif

decompressCodeEnd:

        dephase

