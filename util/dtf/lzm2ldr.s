
NO_BORDER_FX            equ     0

nLengthSlots            equ     8
nOffs1Slots             equ     4
nOffs2Slots             equ     8

decodeTablesBegin       equ     0ee30h
; aligned to 4 bytes (EE30h)
decodeTableL            equ     decodeTablesBegin
; aligned to 16 bytes (EE50h)
decodeTableO1           equ     decodeTableL + (nLengthSlots * 4)
; aligned to 32 bytes (EE60h)
decodeTableO2           equ     decodeTableO1 + (nOffs1Slots * 4)
; aligned to 128 bytes (EE80h)
decodeTableO3           equ     decodeTableO2 + (nOffs2Slots * 4)

fileReadBuffer          equ     0ef00h

    macro exos n
        rst   30h
        defb  n
    endm

; =============================================================================

        org   00f0h
        defw  0500h, codeEnd - codeBegin, 0, 0, 0, 0, 0, 0

codeBegin:
        di
        ld    sp, 0100h
        ld    c, 40h
        exos  0
        ld    a, 0ffh
        out   (0b2h), a
        ld    hl, decompCode1Begin
        ld    de, 0020h
        ld    bc, decompCode1End - decompCode1Begin
        ldir
;       ld    hl, decompCode2Begin
        ld    e, 60h
        ld    c, low (decompCode2End - decompCode2Begin)
        ldir
;       ld    hl, decompCode3Begin
        ld    de, 0ac80h
        ld    bc, decompCode3End - decompCode3Begin
        ldir
        ld    a, 1
        ld    de, fileName
        exos  1
        or    a
.l1:    jp    nz, resetRoutine - (0ec80h - decompCode3Begin)
        inc   a
        ld    de, 0010h
        ld    c, e
        ld    b, d
        exos  6
        or    a
        jr    nz, .l1
        ld    hl, (0010h)
        ld    a, h
        xor   'd'
        or    l
        jr    nz, .l1
        ld    hl, (001eh)
        ld    a, l
        dec   a
        or    h
        jr    nz, .l1
        ld    de, 0100h
        push  de
        jp    0028h

; =============================================================================

decompCode1Begin:

        phase 0020h

decompressData_:
        out   (0b3h), a
        jp    c, decompressMain
        xor   a
        ei
        ret

    if $ < 0028h
        block 0028h - $, 00h
    endif

decompressData:                         ; RST 28H: decompress data block
        di
        in    a, (0b3h)
        ld    b, a
        scf
        sbc   a, a
        jr    decompressData_

        dephase

decompCode1End:

; -----------------------------------------------------------------------------

decompCode2Begin:

        phase 0060h

        call  0066h
        jp    0100h
        jp    0069h
        jr    decompressData

copyLZMatch_:
    if NO_BORDER_FX != 0
        ld    l, a
        ld    a, d
    endif
        sbc   a, h
        ld    h, a
.l1:    ld    a, 00h                    ; * savedPage3Segment2
        out   (0b3h), a
        ld    a, c                      ; save shift register
    if NO_BORDER_FX == 0
        out   (81h), a
    endif
        pop   bc                        ; BC = match length
        ldir                            ; copy match data
        ld    c, a
        ld    a, 0ffh
        out   (0b3h), a

savedPage3Segment2      equ     copyLZMatch_.l1 + 1

decompressLoop_:
        dec   iyh
        ret   z

decompressLoop:
        ld    a, 80h
        rst   30h
        jr    z, .l2                    ; literal byte ?
        ld    b, 8
.l1:    rra                             ; A = 00000001B -> 10000000B
        rst   30h
        jp    z, copyLZMatch            ; LZ77 match ?
        djnz  .l1
        ld    l, b                      ; A = 1, B = 0
        rst   30h                       ; get literal sequence length - 17
        add   a, 16
        ld    b, a
        rl    l
        inc   b                         ; B: (length - 1) LSB + 1
        inc   l                         ; L: (length - 1) MSB + 1
.l2:    inc   ixl                       ; copy literal byte (Carry = 1)
        call  z, readBlock              ; or sequence (Carry = 0)
        ld    h, (ix)
.l3:    ld    a, 00h                    ; * savedPage3Segment
        out   (0b3h), a
        ld    a, h
        ld    (de), a
        inc   de
        ld    a, 0ffh
        out   (0b3h), a
        jr    c, decompressLoop_
        djnz  .l2
        dec   l
        jr    nz, .l2
        jr    decompressLoop_

savedPage3Segment       equ     decompressLoop.l3 + 1

        dephase

decompCode2End:

; -----------------------------------------------------------------------------

decompCode3Begin:

        phase 0ec80h

decompressMain:
        ld    a, b
        ld    (savedPage3Segment), a
        ld    (savedPage3Segment2), a
        ld    (decompressDone.l1 + 1), sp
        ld    sp, 00dch
        ld    hl, 0030h
        push  de
        ld    de, decompCode4End
        ld    bc, decompCode4End - decompCode4Begin
        ldir                            ; save EXOS page 0 code
        pop   de
        ld    l, 2                      ; H = 0
        ld    (inputDataRemaining), hl
        call  readBlock                 ; get compressed data size
        ld    hl, (fileReadBuffer)
        ld    (inputDataRemaining), hl
        push  ix
        push  iy
        ld    ix, fileReadBuffer + 0ffh

decompressDataBlock:
        ld    bc, 0380h                 ; skip checksum, init. shift register
.l1:    ld    a, 01h
        rst   30h                       ; read number of symbols - 1 (IY)
        inc   a
        ld    l, h
        ld    h, a                      ; H = LSB + 1, L = MSB + 1
        djnz  .l1
        ld    a, 61h                    ; read flag bits
        rst   30h                       ; (last block, compression enabled)
        rra
        inc   a                         ; C3h (JP nnnn), or C4h (CALL NZ, nnnn)
        ld    (.l10), a
        jr    c, .l2                    ; compressed data ?
        ld    iyh, 01h                  ; no, copy uncompressed literal data
        ld    b, h
        call  decompressLoop.l2
        jr    .l9
.l2:    push  hl                        ; compression enabled:
        pop   iy
        ld    a, 40h
        rst   30h                       ; get prefix size for length >= 3 bytes
        ld    b, a                      ; (2 to 5 bits)
        inc   b
        ld    l, 02h
        ld    a, 80h + ((decodeTableO3 >> 3) & 10h)
.l3:    add   hl, hl
        rrca
        djnz  .l3
        ld    (offs3PrefixSize), a
        ld    a, l                      ; 1 << nBits
        add   a, nLengthSlots + nOffs1Slots + nOffs2Slots - 3
        ld    b, a                      ; store total table size - 3 in B
        push  de                        ; save decompressed data write address
        ld    de, decodeTableL
.l4:    ld    hl, 1                     ; set initial base value
.l5:    ld    a, 10h
        rst   30h
        push  bc
        push  hl
        ld    b, a
        ld    hl, 1
        ld    c, h
        ld    a, h                      ; RST 20H returns carry = 1,
        jr    z, .l7                    ; S,Z set according to the value read
.l6:    rra
        rr    c
        add   hl, hl                    ; calculate 1 << nBits
        djnz  .l6
        cp    c
        adc   a, b
.l7:    ex    de, hl
        ld    (hl), c                   ; store the number of MSB bits to read
        inc   l
        ld    (hl), a                   ; store the number of LSB bits to read
        inc   l
        ld    c, e
        ld    b, d
        pop   de
        ld    (hl), e                   ; store base value LSB
        inc   l
        ld    (hl), d                   ; store base value MSB
        inc   l
        ex    de, hl
        add   hl, bc                    ; calculate new base value
        pop   bc
        ld    a, e
        cp    low decodeTableO1
        jr    z, .l4                    ; end of length decode table ?
        cp    low decodeTableO2
        jr    z, .l4                    ; end of offset table for length=1 ?
        cp    low decodeTableO3
        jr    z, .l4                    ; end of offset table for length=2 ?
        djnz  .l5                       ; continue until all tables are read
        pop   de                        ; DE = decompressed data write address
.l8:    call  decompressLoop
        dec   iyl
        jr    nz, .l8
.l9:    ld    b, 2
.l10:   jp    .l1                       ; * more blocks are remaining ?

decompressDone:
        pop   iy
        pop   ix
        call  restoreEXOSZPCode
        xor   a
        ld    c, a
        ld    b, a
    if NO_BORDER_FX == 0
        out   (81h), a
    endif
        ld    a, (savedPage3Segment)
.l1:    ld    sp, 0000h                 ; *
        ld    l, e
        ld    h, d
        jp    decompressData_

copyLZMatch:
        ld    a, low (((decodeTableL + (8 * 4)) >> 2) & 3fh)
        sub   b
        rst   38h                       ; get match length to HL
        push  hl                        ; NOTE: flags are set according to H
        jr    nz, .l1                   ; match length >= 256 bytes ?
        dec   l
        jr    z, .l2                    ; match length = 1 byte ?
        dec   l
        jr    nz, .l1                   ; match length >= 3 bytes ?
                                        ; length = 2 bytes, read 3 prefix bits
        ld    a, 20h + ((decodeTableO2 >> 5) & 07h)
        jr    .l3
.l1:                                    ; length >= 3, variable prefix size
        ld    a, 10h + ((decodeTableO3 >> 6) & 03h)     ; * offs3PrefixSize
        defb  21h                       ; = LD HL, nnnn
.l2:                                    ; length = 1 byte, read 2 prefix bits
        ld    a, 40h + ((decodeTableO1 >> 4) & 0fh)
.l3:    rst   30h                       ; get offset prefix
        rst   38h                       ; decode offset
        ld    a, e                      ; calculate source address
        sub   l
    if NO_BORDER_FX == 0
        ld    l, a
        ld    a, d
    endif
        jp    copyLZMatch_

offs3PrefixSize         equ     copyLZMatch.l1 + 1

readBlock:
        push  af
        push  bc
        push  de
        call  restoreEXOSZPCode
.l1:    ld    bc, 0                     ; * inputDataRemaining
        ld    a, b
        or    a
        jr    z, .l2
        dec   a
        ld    (inputDataRemaining + 1), a
        ld    a, c
        ld    bc, 0100h
.l2:    ld    (inputDataRemaining), a
        ld    a, c
        or    b
        jr    z, resetRoutine
        ld    a, 1
        ld    de, fileReadBuffer
        exos  6
        or    a
        jr    nz, resetRoutine
        call  copyZPCode
        pop   de
        pop   bc
        pop   af
        ret

inputDataRemaining      equ     readBlock.l1 + 1

resetRoutine:
        di
        ld    sp, 0100h
        ld    a, 0ffh
        out   (0b2h), a
        jp    (.l1 & 3fffh) | 8000h
.l1:    ld    c, 40h
        exos  0
        ld    a, 01h
        out   (0b3h), a
        ld    a, 6
        jp    0c00dh

restoreEXOSZPCode:
        push  hl
        ld    hl, decompCode4End
        jr    copyZPCode.l1

copyZPCode:
        di
        push  hl
        ld    hl, decompCode4Begin
.l1:    ld    de, 0030h
        ld    bc, decompCode4End - decompCode4Begin
        ldir
        pop   hl
        ret

; -----------------------------------------------------------------------------

decompCode4Begin:

        phase 0030h

readBits:                               ; RST 30H: read log2(100H / A) bits
.l1:    sla   c
        jr    z, readByte
        adc   a, a
        jr    nc, .l1
        ret

decodeLZMatchParam:                     ; RST 38H: decode length or offset
        add   a, a
        add   a, a
        ld    l, a
        ld    h, high decodeTablesBegin
        ld    a, (hl)
        or    a
        call  nz, readBits
        ld    b, a
        inc   l
        ld    a, (hl)
        or    a
        call  nz, readBits
        inc   l
        add   a, (hl)
        inc   l
        ld    h, (hl)
        ld    l, a
        ld    a, b
        adc   a, h                      ; return decoded value in HL, A = H,
        ld    h, a                      ; flags are set according to H
        ret

readByte:                               ; read compressed byte to shift
        inc   ixl                       ; register (assumes carry = 1)
        call  z, readBlock
        ld    c, (ix)
.l1:    rl    c
        adc   a, a
        jr    nc, .l1
        ret

        assert  $ <= 0060h

        dephase

decompCode4End:

        assert  ($ + (decompCode4End - decompCode4Begin)) <= decodeTablesBegin

        dephase

decompCode3End:

; =============================================================================

fileName:
        defb  low (codeEnd - (fileName + 1))

codeEnd:

