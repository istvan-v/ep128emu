
; border effects are disabled if this is set to any non-zero value
NO_BORDER_FX            equ     0

; total data size is 864 bytes; the lower byte of the start address must be 80h
decompressDataBegin     equ     0fc80h

huffmanDecodeTable1     equ     decompressDataBegin
huffmanDecodeTable2     equ     huffmanDecodeTable1 + 324
prvDistanceTable        equ     huffmanDecodeTable2 + 28
huffmanLimitTable       equ     decompressDataBegin + 384
huffmanLimitTableL      equ     huffmanLimitTable + 64
decompressDataEnd       equ     decompressDataBegin + 864

; NOTE: the upper byte of the address of huffmanDecode1, huffmanDecode2,
; read9Bits, read8Bits, and read5Bits must be the same (see assert directive
; below)

        org   0fa98h

; input/output registers:
;  DE:  decompressed data write address
;  HL:  compressed data read address

decompressData:
        push  ix
        push  iy
        push  de
        exx
        pop   de                        ; DE' = decompressed data write address
        exx
        inc   hl                        ; ignore checksum byte
        push  hl
        pop   iy                        ; IY = compressed data read address
        ld    e, 080h                   ; initialize shift register
        jp    decompressAllDataBlocks   ; decompress all blocks

; reserved registers:
;  D:   previous offsets table write position
;  E:   shift register
;  DE': decompressed data write address
;  IX:  2's complement of bytes remaining
;  IY:  compressed data read address

huffmanDecode1:
        ld    hl, huffmanLimitTable

huffmanDecode2:
        xor   a
        ld    b, a
        sla   e
        call  z, readCompressedByte
        adc   a, 2
.l1:    cp    (hl)
        jr    c, .l4
        inc   l
        sla   e
        jr    z, .l5
.l2:    rla
        jp    nc, .l1
.l3:    cp    (hl)
        ld    c, a
        ld    a, b
        set   4, l
        sbc   a, (hl)
        res   4, l
        ld    a, c
        jr    c, .l4
        inc   l
        sla   e
        call  z, readCompressedByte
        rla
        rl    b
        jp    p, .l3
.l4:    set   5, l
        add   a, (hl)
        ld    c, a
        ld    a, b
        set   4, l
        adc   a, (hl)
        ld    b, a
        ld    a, (bc)
        inc   b
        inc   b
        rrca
        ld    a, (bc)
        ret
.l5:    ld    e, (iy)
        inc   iy
        rl    e
        jp    .l2

read9Bits:
        ld    b, 9
        defb  021h                      ; = LD HL, nnnn

read8Bits:
        ld    b, 8
        defb  021h                      ; = LD HL, nnnn

read5Bits:
        ld    b, 5

        assert  (huffmanDecode1 & 0ff00h) == (read5Bits & 0ff00h)

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
.l1:    sla   e                         ; is Huffman encoding enabled ?
        call  z, readCompressedByte
        ld    a, (hl)                   ; if not, use 9 or 5 bit literals,
        ld    (bc), a
        ret   nc                        ; and return
        inc   hl
        ld    a, (hl)                   ; set decoder routine address
        ld    (bc), a
        ld    d, 16                     ; number of code sizes remaining (D)
        inc   hl
        ld    c, (hl)                   ; table write address in BC
        inc   hl
        ld    b, (hl)
        inc   hl
        ld    a, (hl)
        ld    ixl, a                    ; limit/offset table base address in IX
        ld    ixh, high huffmanLimitTable
        ld    hl, 1                     ; initialize limit counter (HL)
.l2:    add   hl, hl                    ; = (last_prv_size_value + 1) * 2
        ld    a, d
        xor   9                         ; special case for 8 bit codes:
        jr    nz, .l3
        ld    h, a                      ; clear limit counter MSB to zero
.l3:    ld    a, c
        sub   l
        ld    (ix + 32), a              ; store decode table address offset
        ld    a, b
        sbc   a, h
        ld    (ix + 48), a
        push  bc
        call  gammaDecode               ; get number of symbols with this size
        ld    c, a
        add   hl, bc                    ; update and store limit counter
        dec   hl
        ld    (ix), l
        ld    (ix + 16), h
        push  bc
        exx
        pop   bc                        ; BC' = number of symbols remaining + 1
        exx
        pop   bc
        ld    hl, 0ffffh                ; HL = decoded value (from -1)
        jr    .l5
.l4:    push  bc
        call  gammaDecode               ; get difference from previous symbol
        ld    c, a
        add   hl, bc                    ; calculate decoded value,
        pop   bc
        ld    a, l
        inc   b
        inc   b
        ld    (bc), a                   ; and store it in the decode table
        ld    a, h
        dec   b
        dec   b
        ld    (bc), a
        inc   bc
.l5:    exx                             ; check the number of symbols remaining
        dec   bc
        ld    a, b
        or    c
        exx
        jr    nz, .l4                   ; not done with this code size yet ?
        ld    l, (ix)                   ; get limit value
        ld    h, (ix + 16)
        inc   ix
        dec   d
        jr    nz, .l2                   ; continue with the next code size
        ret

decompressAllDataBlocks:
.l1:    call  read16Bits                ; read 2's complement of block length
        ld    l, a
        ld    h, b
        ld    b, 2                      ; read 'last block' and
        call  readBBits                 ; 'compression enabled' flags
        srl   a
        push  af                        ; save last block flag
        push  hl                        ; and block length
        ld    a, low read8Bits
        ld    (readCharAddrLow), a
        call  c, huffmanInit            ; initialize Huffman decoding
        pop   ix
.l2:    call  huffmanDecode1            ; read next character
        jr    c, copyLZMatch            ; match code ?
        exx
        ld    (de), a                   ; no, store literal byte
        inc   de                        ; increment write address
.l3:    exx
        inc   ixl                       ; check bytes remaining
        jp    nz, .l2
    if NO_BORDER_FX == 0
        ld    a, e
        out   (081h), a
    endif
        inc   ixh
        jr    nz, .l2
        pop   af                        ; check last block flag:
        jr    z, .l1                    ; more blocks remaining ?
        exx
        push  de
        exx
        pop   de                        ; DE = address of last byte written + 1
        push  iy
        pop   hl                        ; HL = address of last byte read + 1
        pop   iy
        pop   ix
    if NO_BORDER_FX == 0
        xor   a                         ; reset border color
        out   (081h), a
    endif
        ret

readCharAddrLow         equ     decompressAllDataBlocks.l2 + 1

copyLZMatch:
        cp    8
        jr    c, .l2                    ; short offset ?
        cp    03ch
        jr    nc, .l3                   ; special match code ?
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
        jp    p, .l4                    ; LZ match with delta value ?
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
        out   (081h), a
    endif
        ld    hl, huffmanLimitTableL
.l1:    call  huffmanDecode2            ; get match length
        cp    8
        call  nc, readLZMatchParameterBits  ; long match ?
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
        defw  huffmanDecodeTable1
        defb  low huffmanLimitTable

addrTable2:
        defb  low read5Bits
        defb  low huffmanDecode2
        defw  huffmanDecodeTable2
        defb  low huffmanLimitTableL

    if decompressData < decompressDataBegin
        assert  $ <= decompressDataBegin
    else
        assert  decompressData >= decompressDataEnd
    endif

