
NO_BORDER_FX    equ     1

  macro exos n
        rst   30h
        defb  n
  endm

        org   0100h

main:
        ld    a, 0fdh
        out   (0b1h), a
        add   a, a
        out   (0b2h), a
        inc   a
        out   (0b3h), a
        ld    hl, (0060h)
        ld    bc, 66cch
        sbc   hl, bc
        jr    z, .l1
        ld    hl, decompCode1Begin
        ld    de, 0020h
        ld    bc, decompCode1End - decompCode1Begin
        ldir
;       ld    hl, decompCode2Begin
        ld    e, low 0060h
        ld    c, low (decompCode2End - decompCode2Begin)
        ldir
.l1:    ld    de, 0100h
        push  de
        jp    decompressData

; =============================================================================

decompCode1Begin:

        phase 0020h

readBit:                                ; RST 20H:
        srl   c                         ; read a compressed bit to carry
        ret   nz
        jr    decompressData_.l3
.l1:    rra
        ld    c, a
        ret

; -----------------------------------------------------------------------------

    if $ < 0028h
        block 0028h - $, 00h
    endif

decompressData:                         ; RST 28H: load compressed data to DE
        ld    h, 1                      ; input file channel number
        call  readWordFromFile          ; get compressed data size
        ld    a, h
        jr    decompressData_

        dephase

decompCode1End:

; -----------------------------------------------------------------------------

decompCode2Begin:

        phase 0060h

decompressData_:
        ld    (readLength.l1 + 1), de   ; store start address
        exos  6                         ; read compressed data
        push  de
        exx
        defb  21h                       ; = LD HL, nnnn
        jr    decompressData            ; for CALL 0069H compatibility
        pop   hl                        ; HL' = compressed data read addr. + 1
        exx
        call  readWordFromFile          ; get uncompressedSize - compressedSize
        ld    a, c
        or    b
        jr    z, decompressDone.l1      ; uncompressed data ?
        di
        ex    de, hl
        add   hl, bc
        push  hl
        dec   hl
        ex    de, hl                    ; DE = decompressed data write address
        ld    bc, 1001h                 ; initialize shift register
.l1:    rst   20h                       ; fill input buffer (2 bytes), clear B
        djnz  .l1
.l2:    call  readLength                ; read literal sequence length
.l3:    exx                             ; copy literal data
        ld    a, b                      ; read a compressed byte to A
        ld    b, c                      ; use a 2-byte buffer
        dec   hl
        ld    c, (hl)
        exx
    if NO_BORDER_FX == 0
        out   (81h), a
    endif
        jr    c, readBit.l1
        ld    (de), a
        dec   de
        dec   hl
        ld    a, l
        or    h
        jr    nz, .l3
.l4:    call  readLength                ; get match length - 1
        push  hl
        ld    l, b
        ld    b, 4
        jr    z, .l6                    ; match length = 2 bytes ?
        call  readBits                  ; read offset prefix (4 bits)
        ld    b, l
        call  readBits_                 ; read offset bits
.l5:    add   hl, de                    ; calculate source address
        ld    a, c                      ; save shift register
        pop   bc                        ; BC = match length - 1
        inc   bc
        lddr                            ; expand match
        ld    c, a
        rst   20h                       ; read flag bit:
        jr    nc, .l2                   ; 0: literal sequence
        jr    .l4                       ; 1: LZ77 match
.l6:    call  readBits.l2               ; special case for 2-byte matches:
        ld    b, l                      ; prefix length = 3 bits,
        call  readBits_.l1              ; encoding is 12345678, max offset: 510
        dec   hl
        jr    .l5

readLength:                             ; read and decode length value
                                        ; NOTE: B should be set to zero
.l1:    ld    hl, 0000h                 ; * start address
        scf
        sbc   hl, de                    ; check write address
        jr    c, readLength_.l2

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

readLength_:
.l1:    inc   b                         ; read prefix
.l2:    rst   20h
        jr    c, .l1

readBits_:                              ; read B (can be 0) bits to HL,
        dec   b                         ; and add a leading '1' bit
.l1:    ld    hl, 1                     ; (e.g. B=7 would read 1xxxxxxxb)
        inc   b
        ret   z                         ; return Z=1 if no bits are read

readBits:
.l1:    rst   20h                       ; shift B (must be > 0) bits to HL
        adc   hl, hl                    ; HL is expected to be initialized
.l2:    djnz  .l1                       ; to 0 or 1
        ret

readWordFromFile:                       ; read a 16-bit LSB first word
        call  .l1                       ; from the input file to BC
        ld    l, b
.l1:    push  de
        ld    a, h
        exos  5
        pop   de
        ld    c, l
        ret

        dephase

decompCode2End:

