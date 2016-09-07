
BUILD_EXTENSION         equ     1
BUILD_EXTENSION_ROM     equ     0
NO_BORDER_FX            equ     0

; -----------------------------------------------------------------------------

moduleType              equ     'd'

  if BUILD_EXTENSION_ROM == 0
    if BUILD_EXTENSION == 0
        output  "dl2.com"
    else
        output  "dl2.ext"
    endif
  else
        output  "dl2.rom"
  endif

  macro exos n
        rst   30h
        defb  n
  endm

; -----------------------------------------------------------------------------

  if BUILD_EXTENSION_ROM == 0
    if BUILD_EXTENSION == 0
        org   00f0h
        defw  0500h
    else
        org   0bffah
        defw  0600h
    endif
        defw  codeEnd - codeBegin, 0, 0, 0, 0, 0, 0
  else
        org   0c000h
        defm  "EXOS_ROM"
        defw  0000h
  endif

codeBegin:

  if (BUILD_EXTENSION != 0) || (BUILD_EXTENSION_ROM != 0)

extMain:
        ld    a, c
        sub   2
        jr    z, parseCommand
        dec   a
        jr    z, helpString
        sub   3
        jr    z, loadModule
        xor   a
        ret

parseCommand:
        call  checkCommandName
        jp    z, main
        xor   a
        ret

helpString:
        ld    a, b
        or    a
        jr    nz, .l3
        push  hl
        ld    hl, commandName
.l1:    call  printString
        pop   hl
.l2:    xor   a
        ret
.l3:    call  checkCommandName
        jr    nz, .l2
        push  hl
        ld    hl, commandName
        call  printString
        ld    hl, descriptionString
        ld    c, a
        jr    .l1

loadModule:
        inc   de
        ld    a, (de)
        dec   de
        cp    moduleType
        jr    z, .l1
        xor   a
        ret
.l1:    ld    a, b                      ; channel number must be 1
        cp    1
        ld    a, 0fbh                   ; .ICHAN
        ret   nz
        ld    l, 1                      ; if loading a module:
        xor   a
.l2:    exos  3                         ; close all channels,
        inc   l                         ; but keep the input file open
        ld    a, l
        jr    nz, .l2
        ld    c, 40h                    ; free all allocated memory
        call  exosReset_
        scf                             ; Carry = 1 for LZ formats
        jp    loadCompressedFile_

checkCommandName:
        ld    a, (commandNameLength)
        cp    b
        ret   nz
        push  bc
        push  de
        push  hl
        ld    hl, commandName
.l1:    inc   de
        ld    a, (de)
        sub   (hl)
        jr    nz, .l2
        inc   hl
        djnz  .l1
.l2:    pop   hl
        pop   de
        pop   bc
        or    a
        ret

printString:
        xor   a
        cp    (hl)
        ret   z
        push  bc
        push  de
        push  hl
        ld    d, h
        ld    e, l
        ld    b, a
        ld    c, a
.l1:    inc   bc
        inc   hl
        cp    (hl)
        jr    nz, .l1
        dec   a
        exos  8
        pop   hl
        pop   de
        pop   bc
        xor   a
        ret

commandNameLength:
        defb  versionString - commandName
commandName:
        defm  "DL2"
versionString:
        defm  "   version 1.03\r\n"
        defb  00h
descriptionString:
        defb  00h

  endif

; =============================================================================

main:
        di
        ld    sp, 0100h
  if (BUILD_EXTENSION != 0) || (BUILD_EXTENSION_ROM != 0)
        ex    de, hl
        ld    de, fileNameBuffer
        ld    a, (hl)
        sub   b
        jr    z, .l5
        jr    c, .l5
        ld    c, b
        ld    b, 0
        add   hl, bc
        inc   hl
        ld    b, a
.l1:    ld    a, (hl)
        cp    ' ' + 1
        jr    nc, .l3
        inc   hl
        djnz  .l1
        jr    .l5
.l2:    ld    a, (hl)
        cp    ' ' + 1
        jr    c, .l4
.l3:    inc   de
        ld    (de), a
        inc   hl
        djnz  .l2
.l4:    ld    a, e
        sub   low fileNameBuffer
        ld    (fileNameBuffer), a
        jr    nz, loadCompressedFile
  endif
.l5:    call  exosReset
  if (BUILD_EXTENSION == 0) && (BUILD_EXTENSION_ROM == 0)
        ld    hl, resetRoutine
        ld    (0bff8h), hl
  endif
        ld    de, exdosCommandString    ; check if EXDOS is available
        exos  26
        or    a
        jr    z, .l6
        xor   a                         ; if not, then default to empty name
        ld    (fileNameBuffer), a
        jr    loadCompressedFile
.l6:    ld    de, fileCommandString     ; try to select a file
        exos  26                        ; with the 'FILE' extension
        or    a
        jr    z, loadCompressedFile
        cp    0e5h
        jp    z, resetRoutine           ; .STOP ?
        ld    hl, defaultFileName       ; use default file name (DL2_FILE.BIN)
        ld    de, fileNameBuffer
        ld    c, (hl)
        ld    b, 0
        inc   bc
        ldir

loadCompressedFile:
        call  exosReset
        ld    a, 1                      ; open input file
        ld    de, fileNameBuffer
        exos  1
        or    a
        jr    nz, .l1
        inc   a
        ld    bc, 2
        ld    de, 0060h
        exos  6
        or    a
        jr    nz, .l1
        ld    bc, (0060h)
        ld    a, b                      ; check module header:
        xor   moduleType
        or    c
        jr    z, .l2                    ; LZ format ?
        ld    a, b                      ; no, check TOM block size (BC)
        cp    40h
        jr    nc, resetRoutine          ; larger than one segment is error
        or    c
        jr    nz, loadCompressedFile_   ; DTF/TOM format: clear carry
.l1:    jr    resetRoutine              ; zero size is error
.l2:    inc   a
        ld    bc, 14
        exos  6
        or    a
        jr    nz, resetRoutine
.l3:    ld    hl, (006eh)               ; check header version:
        xor   a
        or    h
        jr    nz, resetRoutine
        or    l
        jr    z, .l4
        dec   a
        jr    nz, resetRoutine
        ld    de, LZM2Loader.decompressData
        ld    hl, lzm2CodeTable
        jr    loadCompressedFile_.l1
.l4:    scf                             ; set carry for simple LZ format

loadCompressedFile_:
        ld    de, decompressData
        ld    hl, lzCodeTable
        jr    c, .l1
        ld    de, dtfDecompressInit
        ld    hl, dtfCodeTable
        push  bc
.l1:    push  de
        in    a, (0b0h)
        out   (0b1h), a
.l2:    ld    c, (hl)                   ; BC = code block size
        inc   hl
        ld    b, (hl)
        inc   hl
        ld    a, c
        or    b
        jr    z, .l3                    ; all loader code has been copied ?
        ld    e, (hl)
        inc   hl
        ld    d, (hl)
        inc   hl
        push  de
        ld    e, (hl)
        inc   hl
        ld    d, (hl)
        inc   hl
        ex    (sp), hl
        ldir
        pop   hl
        jr    .l2
.l3:    pop   hl
        bit   3, l
        jr    nz, .l4                   ; LZ format (started with RST 28H) ?
        pop   bc
        ld    (dtfDecompressMain.l1 + 1), bc    ; store TOM block size
        ld    a, 0fdh
        out   (0b1h), a
        add   a, a
        out   (0b2h), a
        inc   a
.l4:    ld    de, 0100h                 ; load address of first (.com) block
        push  de
        jp    (hl)

resetRoutine:
        di
        ld    sp, 0100h
        call  exosReset
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
        ld    hl, 0b3d3h                ; = OUT (0B3H), A
        ld    (0100h), hl
        ld    hl, 0e979h                ; = LD A, C : JP (HL)
        ld    (0102h), hl
        ld    hl, 0c00dh
        ld    c, 6
        ld    a, 01h
        jp    0100h

exosReset:
        ld    c, 60h

exosReset_:
        di
        pop   de
        ld    sp, 0100h
        push  de
        ld    hl, (004fh)
        ld    de, 13
        add   hl, de
        res   6, h
        xor   a
        out   (0b2h), a
        ld    e, (hl)
        inc   hl
        ld    d, (hl)
        dec   a
        out   (0b2h), a
        xor   a
        ld    (de), a
        ld    hl, (0038h)
        ld    a, 0c9h                   ; = RET
        ld    (0038h), a                ; work around EXOS 0 crash
        exos  0
        ld    (0038h), hl
        ei
        ret

lzCodeTable:
        defw  decompCode1End - decompCode1Begin
        defw  decompCode1Begin, 0008h
        defw  decompCode2End - decompCode2Begin
        defw  decompCode2Begin, 0060h
        defw  0

dtfCodeTable:
        defw  dtfDecompCode1End - dtfDecompCode1Begin
        defw  dtfDecompCode1Begin, 0020h
        defw  dtfDecompCode2End - dtfDecompCode2Begin
        defw  dtfDecompCode2Begin, 0060h
        defw  dtfDecompCode3End - dtfDecompCode3Begin
        defw  dtfDecompCode3Begin, 0af00h
        defw  0

lzm2CodeTable:
        defw  LZM2Loader.decompCode1End - LZM2Loader.decompCode1Begin
        defw  LZM2Loader.decompCode1Begin, 0020h
        defw  LZM2Loader.decompCode2End - LZM2Loader.decompCode2Begin
        defw  LZM2Loader.decompCode2Begin, 0060h
        defw  LZM2Loader.decompCode3End - LZM2Loader.decompCode3Begin
        defw  LZM2Loader.decompCode3Begin, 0ac80h
        defw  0

; =============================================================================

decompCode1Begin:

        phase 0008h

readLength:                             ; RST 08H: read and decode length value
                                        ; NOTE: B should be set to zero
.l1:    ld    hl, 0000h                 ; * 2's complement of start address
        add   hl, de                    ; check write address:
        jr    c, .l3
        jr    decompressDone            ; all data has been decompressed
.l2:    inc   b                         ; read prefix
.l3:    srl   c
        call  z, decompressData_.l2
        jr    c, .l2

; -----------------------------------------------------------------------------

    if $ < 0018h
        block 0018h - $, 00h
    endif

readBits_:                              ; RST 18H: read B (can be 0) bits
        inc   b                         ; to HL, and add a leading '1' bit
        ld    hl, 1                     ; (e.g. B=7 would read 1xxxxxxxb)
        dec   b
        ret   z                         ; return Z=1 if no bits are read

readBits:
.l1:    srl   c                         ; shift B (must be > 0) bits to HL
        call  z, decompressData_.l2
        adc   hl, hl                    ; HL is expected to be initialized
.l2:    djnz  .l1                       ; to 0 or 1
        ret

; -----------------------------------------------------------------------------

    if $ < 0028h
        block 0028h - $, 00h
    endif

decompressData:                         ; RST 28H: load compressed data to DE
        call  readWordFromFile          ; get compressed data size
        xor   a
        ld    l, a
        ld    h, a
        jr    decompressData_

        dephase

decompCode1End:

; -----------------------------------------------------------------------------

decompCode2Begin:

        phase 0060h

decompressDone:
        pop   de                        ; pop return address of readLength
        pop   de
.l1:    ld    l, e                      ; DE = HL = last write address + 1
        ld    h, d
        xor   a                         ; A = 0 (no error)
        ld    c, a                      ; BC = 0 (all data has been loaded)
    if NO_BORDER_FX == 0
        out   (81h), a                  ; reset border color
    endif
        ei
        ret

decompressData_:
        sbc   hl, de                    ; calculate and store
        ld    (readLength.l1 + 1), hl   ; 2's complement of start address
        inc   a                         ; A = 1 (input file channel number)
        exos  6                         ; read compressed data
        call  readWordFromFile          ; get uncompressedSize - compressedSize
        ld    a, c
        or    b
        jr    z, decompressDone.l1      ; uncompressed data ?
        di
        push  de
        exx
        pop   hl                        ; HL' = compressed data read addr. + 1
        exx
        ex    de, hl
        add   hl, bc
        push  hl
        dec   hl
        ex    de, hl                    ; DE = decompressed data write address
        ld    bc, 1001h                 ; initialize shift register
        rst   18h                       ; fill input buffer (2 bytes)
.l1:    rst   08h                       ; read literal sequence length
.l2:    exx                             ; copy literal data
        ld    a, b                      ; read a compressed byte to A
        ld    b, c                      ; use a 2-byte buffer
        dec   hl
        ld    c, (hl)
        exx
    if NO_BORDER_FX == 0
        out   (81h), a
    endif
        jr    nc, .l3
        rra
        ld    c, a
        ret
.l3:    ld    (de), a
        dec   de
        dec   hl
        ld    a, l
        or    h
        jr    nz, .l2
.l4:    rst   08h                       ; get match length - 1
        push  hl
        ld    l, b
        ld    b, 4
        jr    z, .l6                    ; match length = 2 bytes ?
        call  readBits                  ; read offset prefix (4 bits)
        ld    b, l
        rst   18h                       ; read offset bits
.l5:    add   hl, de                    ; calculate source address
        ld    a, c                      ; save shift register
        ldd
        pop   bc                        ; BC = match length - 1
        lddr                            ; expand match
        ld    c, a
        srl   c                         ; read flag bit:
        call  z, .l2
        jr    nc, .l1                   ; 0: literal sequence
        jr    .l4                       ; 1: LZ77 match
.l6:    call  readBits.l2               ; special case for 2-byte matches:
        inc   l                         ; prefix length = 3 bits,
        ld    b, l                      ; encoding is 12345678,
        rst   18h                       ; max offset: 510
        dec   hl
        jr    .l5

readWordFromFile:                       ; read a 16-bit LSB first word
        call  .l1                       ; from the input file to BC
        ld    l, b
.l1:    push  de
        ld    a, 1
        exos  5
        pop   de
        ld    c, l
        ret

        dephase

decompCode2End:

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

        module  LZM2Loader

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

; -----------------------------------------------------------------------------

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

        endmod

; =============================================================================

exdosCommandString:
        defb  6
        defm  "EXDOS"
        defb  0fdh

fileCommandString:
        defb  7
        defm  "FILE "
        defw  fileNameBuffer

defaultFileName:
        defb  12
        defm  "DL2_FILE.BIN"

codeEnd:

  if (BUILD_EXTENSION != 0) || (BUILD_EXTENSION_ROM != 0)
fileNameBuffer          equ     0100h
  else
fileNameBuffer          equ     (codeEnd + 0ffh) & 0ff00h
  endif

  if BUILD_EXTENSION_ROM != 0
        size  16384
  endif

