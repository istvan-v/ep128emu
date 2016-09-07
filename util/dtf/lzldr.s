
NO_BORDER_FX    equ     1
moduleType      equ     'd'

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
        ld    a, b                      ; check module header:
        xor   moduleType
        or    c
        jr    nz, resetRoutine          ; not LZ format ?
        inc   a
        ld    bc, 14
        exos  6
        or    a
        jr    nz, resetRoutine
        ld    hl, (006eh)               ; check header version
        ld    a, l
        or    h
        jr    nz, resetRoutine
        in    a, (0b0h)
        out   (0b1h), a
        ld    a, 0ffh
        out   (0b2h), a
        ld    hl, decompCode1Begin
        ld    de, 0008h
        ld    bc, decompCode1End - decompCode1Begin
        ldir
;       ld    hl, decompCode2Begin
        ld    e, low 0060h
        ld    c, low (decompCode2End - decompCode2Begin)
        ldir
        ld    sp, 0100h
        ld    de, 0100h                 ; load address of first (.com) block
        push  de
        jp    decompressData

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

defaultFileName:
        defb  low (codeEnd - (defaultFileName + 1))

codeEnd:

