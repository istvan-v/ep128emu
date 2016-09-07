
NO_BORDER_FX    equ     0

  macro exos n
        rst   30h
        defb  n
  endm

; -----------------------------------------------------------------------------

        org   00f0h
        defw  0500h, codeEnd - codeBegin, 0, 0, 0, 0, 0, 0

codeBegin:

main:
        di
        ld    sp, 3800h
        ld    c, 40h
        exos  0
        ld    a, 1                      ; open input file
        ld    de, defaultFileName
        exos  1
        or    a
        jr    nz, resetRoutine
        inc   a
        ld    bc, 2
        ld    de, 0060h
        exos  6
        or    a
        jr    nz, resetRoutine
        ld    bc, (0060h)
        ld    a, b                      ; check TOM block size (BC)
        cp    40h
        jr    nc, resetRoutine          ; larger than one segment is error
        or    c
        jr    z, resetRoutine           ; zero size is error
        push  bc
        ld    hl, dtfDecompCode1Begin
        ld    de, 0020h
        ld    bc, dtfDecompCode1End - dtfDecompCode1Begin
        ldir
;       ld    hl, dtfDecompCode2Begin
        ld    e, low 0060h
        ld    c, low (dtfDecompCode2End - dtfDecompCode2Begin)
        ldir
;       ld    hl, dtfDecompCode3Begin
        ld    de, 0af00h
        ld    bc, dtfDecompCode3End - dtfDecompCode3Begin
        ldir
        pop   bc
        ld    (dtfDecompressMain.l1 + 1), bc    ; store TOM block size
        ld    a, 0fdh
        out   (0b1h), a
        add   a, a
        out   (0b2h), a
        inc   a
        ld    sp, 0100h
        ld    de, 0100h                 ; load address of first (.com) block
        push  de
        jp    dtfDecompressInit

resetRoutine:
        ld    a, 1
        exos  3
        di
        ld    hl, 0008h
        ld    a, low 0030h
.l1:    ld    (hl), h
        inc   l
        jr    z, .l2
        cp    l
        jr    nz, .l1
        add   hl, hl
        jr    .l1
.l2:    ei
        ld    a, 0ffh
        out   (0b2h), a
        ld    a, 01h
        out   (0b3h), a
        ld    a, 6
        jp    0c00dh

; =============================================================================

dtfSlotBitsTable        equ     0acc0h
dtfDecodeTable          equ     0ad00h
dtfFileReadBuffer       equ     0ae00h

dtfDecompCode1Begin:

        phase 0020h

dtfSegmentTable:
dtfDecompressInit:
        out   (0b3h), a
        jr    0066h
        defb  0ffh

dtfReadBlock_:
        jp    dtfReadBlock

    if $ < 0028h
        block 0028h - $, 00h
    endif

dtfDecompressData:                      ; RST 28H: decompress DTF data block
        jr    dtfDecompressData_
.l1:    ld    sp, 0000h                 ; *
        ld    h, d
        ei
        ret

        dephase

dtfDecompCode1End:

; -----------------------------------------------------------------------------

dtfDecompCode2Begin:

        phase 0060h

        call  0066h
        jp    0100h
        ld    a, 0ffh
        defb  0feh                      ; = CP nn

dtfDecompressData_:
        xor   a                         ; A = 00h: DTF, A = FFh: TOM
        di
        ld    hl, dtfSegmentTable
        ld    bc, 04afh
.l1:    inc   c
        ini
        jr    nz, .l1
        dec   c
        outi
        ld    (dtfDecompressData.l1 + 1), sp
        ld    sp, 00dch
        call  dtfDecompressMain
        out   (0b2h), a
        jr    dtfDecompressData.l1

dtfReadByte:
        xor   a
        jr    dtfReadByte_

dtfReadBits_:
        adc   a, a
        ret   c

dtfReadBits:
        sla   c
        jp    nz, dtfReadBits_
        ld    h, a

dtfReadByte_:
        exx
.l1:    inc   l
        jr    z, dtfReadBlock_
.l2:    ld    a, (hl)
        exx
    if NO_BORDER_FX == 0
        out   (81h), a
    endif
        ret   nc
        adc   a, a
        ld    c, a
        ld    a, h
        jr    dtfReadBits_

dtfReadEncodedByte:
.l1:    ld    a, 40h                    ; * 100H >> nPrefixBits, or JR to
        call  dtfReadBits               ; dtfReadByte if no statistical comp.
        ld    hl, dtfSlotBitsTable
        add   a, a
        add   a, l
        ld    l, a
        ld    a, (hl)
        inc   l
        ld    l, (hl)
        or    a
        call  nz, dtfReadBits
        add   a, l
        ld    l, a
        ld    h, high dtfDecodeTable
        ld    a, (hl)
        ret

        dephase

dtfDecompCode2End:

; -----------------------------------------------------------------------------

dtfDecompCode3Begin:

        phase 0af00h

dtfDecompressMain:                      ; A = 00h: decompress DTF data
        ld    h, a                      ; A = FFh: decompress TOM data
        ld    a, d
        call  dtfGetSegment
.l1:    ld    bc, 0                     ; * the first two bytes of the file are
                                        ; already read when checking the header
.l2:    ld    a, (dtfReadWordFromFile)  ; * get compressed data size
        ld    a, 0cdh                   ; = CALL nnnn
        ld    (.l2), a
        add   hl, hl
        jr    nc, .l3
        dec   bc
.l3:    ld    (dtfInDataRemaining), bc
        call  nc, dtfReadWordFromFile   ; if DTF: get uncompressed data size
        push  bc
        exx
        pop   bc
        ld    hl, dtfFileReadBuffer + 0ffh
        exx
        call  dtfReadByteFromFile       ; get RLE flag byte
        ld    (tomDecompressBlock.l2 + 1), a
        ld    (dtfDecompressBlock.l5 + 1), a
        add   hl, hl                    ; carry = 0: DTF, carry = 1: TOM
        call  dtfDecompressBlock
        ld    a, (dtfSegmentTable + 1)
        out   (0b1h), a
        ld    a, (dtfPageNumber)
        add   a, d
        sub   40h
        ld    d, a
        xor   a
        ld    c, a
        ld    b, a
    if NO_BORDER_FX == 0
        out   (81h), a
    endif
        ld    l, e
        ld    a, (dtfSegmentTable + 2)
        ret

tomDecompressBlock:
.l1:    call  .l4
        ld    b, 1
.l2:    cp    00h                       ; * RLE flag byte
        jr    nz, .l3
        call  .l4
        ld    b, a
        call  .l4
.l3:    bit   7, d
        jp    nz, dtfResetRoutine
        ld    (de), a
        inc   de
        djnz  .l3
        jr    .l1
.l4:    exx
        ld    a, c
        or    b
        dec   bc
        jp    nz, dtfReadByte_.l1
        exx
        pop   hl
        ret

dtfDecompressBlock:
        jr    c, tomDecompressBlock     ; TOM format ?
        call  dtfReadByteFromFile
        ld    hl, dtfReadEncodedByte.l1 + 1
        ld    (hl), low ((dtfReadByte - (dtfReadEncodedByte.l1 + 2)) & 0ffh)
        dec   hl
        ld    (hl), 18h                 ; = JR +nn
        cp    88h
        jr    z, .l3                    ; statistical compression is disabled ?
        ld    c, 2                      ; C = number of prefix bits
        cp    10h
        jr    c, .l1
        cp    90h
        jr    c, dtfResetRoutine
        ld    b, a
        rrca
        rrca
        rrca
        rrca
        and   7
        ld    c, a
        cp    6
        jr    nc, dtfResetRoutine       ; prefix size >= 6 bits is unsupported
        ld    a, b
.l1:    and   0fh                       ; A = number of bits for first slot
        push  af
        ld    a, c
        call  dtfConvertBitCnt
        ld    (hl), 3eh                 ; = LD A, nn
        inc   hl
        ld    (hl), a
        pop   af
        push  de
        ld    hl, dtfSlotBitsTable
        ld    e, b                      ; NOTE: dtfConvertBitCnt returns B = 0
        ld    d, c
.l2:    cp    9
        jr    nc, dtfResetRoutine
        call  dtfConvertBitCnt
        ld    (hl), a
        inc   l
        ld    (hl), e
        ld    a, e
        add   a, c
        ld    e, a
        inc   hl
        call  dtfReadByteFromFile
        dec   d
        jr    nz, .l2
        call  dtfReadWordFromFile.l1
        xor   a
        ld    b, a
        dec   c
        inc   a
        inc   bc
        ld    de, dtfDecodeTable
        exos  6
        pop   de
        or    a
        jr    nz, dtfResetRoutine
.l3:    ld    bc, 0080h                 ; initialize shift register
.l4:    call  dtfReadEncodedByte
        inc   b
.l5:    cp    00h                       ; * RLE flag byte
        jr    nz, .l7
        call  dtfReadEncodedByte
        ld    b, a
        call  dtfReadEncodedByte
        ld    l, a
.l6:    ld    a, l
.l7:    ld    (de), a
        inc   e
        jr    nz, .l8
        inc   d
        call  m, dtfGetNextSegment
.l8:    exx
        dec   bc
        ld    a, c
        or    b
        exx
        ret   z
        djnz  .l6
        jr    .l4

dtfConvertBitCnt:
        ld    c, 1
        ld    b, a
        or    a
        ret   z
        xor   a
        scf
.l1:    rra
        sla   c
        djnz  .l1
        ret

dtfResetRoutine:
        di
        ld    sp, 0100h
        ld    c, 40h
        exos  0
        ld    a, 01h
        out   (0b3h), a
        ld    a, 6
        jp    0c00dh

dtfGetNextSegment:
.l1:    ld    a, 0                      ; * dtfPageNumber
        add   a, 40h

dtfPageNumber           equ     dtfGetNextSegment.l1 + 1

dtfGetSegment:
        res   7, d                      ; write decompressed data to page 1
        set   6, d
        and   0c0h
        ld    (dtfPageNumber), a
        rlca
        rlca
        add   a, low dtfSegmentTable
        ld    (.l1 + 1), a
.l1:    ld    a, (dtfSegmentTable)      ; *
        out   (0b1h), a
        ret

dtfReadWordFromFile:                    ; read a 16-bit LSB first word
        call  dtfReadByteFromFile       ; from the input file to BC
.l1:    ld    c, a

dtfReadByteFromFile:                    ; read a byte from the input file
        push  bc                        ; to A and B
        push  de
        ld    a, 1
        exos  5
        pop   de
        or    a
        jr    nz, dtfResetRoutine
        ld    a, b
        pop   bc
        ld    b, a
        ret

dtfReadBlock:
        exx
        push  af
        push  bc
        push  de
.l1:    ld    bc, 0                     ; * dtfInDataRemaining
        ld    a, b
        or    a
        jr    z, .l2
        dec   a
        ld    (dtfInDataRemaining + 1), a
        ld    a, c
        ld    bc, 0100h
.l2:    ld    (dtfInDataRemaining), a
        ld    a, c
        or    b
        jr    z, dtfResetRoutine
        ld    a, 1
        ld    de, dtfFileReadBuffer
        exos  6
        di
        pop   de
        pop   bc
        or    a
        jr    nz, dtfResetRoutine
        pop   af
        exx
        jp    dtfReadByte_.l2

dtfInDataRemaining      equ     dtfReadBlock.l1 + 1

        dephase

dtfDecompCode3End:

; =============================================================================

defaultFileName:
        defb  low (codeEnd - (defaultFileName + 1))

codeEnd:

