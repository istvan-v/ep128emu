
; default -m6 encoding: FG0L,G,,12345678,0123456789ABCDEF

NO_BORDER_FX    equ     0

;       org   0ff60h

; HL: compressed data start address
; DE: decompressed data start address (updated on return, must not overflow)

decompressData:
        xor   a
        ld    b, a                      ; initialize shift register
.l1:    push  de
        call  readLength                ; get literal sequence length
        ld    c, e
        ld    b, d
        pop   de
        ldir                            ; copy literal sequence
.l2:    push  de
        call  readLength                ; get LZ77 match length - 1
    if NO_BORDER_FX == 0
        out   (81h), a
    endif
        push  de
        ld    de, 1000h                 ; Carry = 1 if length is 2 bytes
        ld    b, 3
        jr    c, .lX                    ; length == 2?
        inc   b                         ; length > 2: 4 offset prefix bits
.lX:    call  readBits.l2
        ld    b, e
        call  nc, readBits.l1           ; length == 2?
        dec   de
        call  c, readBits
        pop   bc
        inc   bc                        ; BC = length
        ex    (sp), hl                  ; calculate source address,
        push  hl
        or    a
        sbc   hl, de
        pop   de
        ldir                            ; and expand match
        pop   hl
.l8:    adc   a, a                      ; check flag bit:
        jr    nc, .l2                   ; LZ77 match?
        jr    nz, .l1                   ; no, continue with literal sequence
        ld    a, (hl)
        inc   hl
        jr    .l8

; read gamma encoded sequence length (B must be 0)

readLength:
.l1:    add   a, a
        jr    nz, .l2
        ld    a, (hl)
        inc   hl
        scf
        rla
.l2:    jr    c, readBits
        inc   b
        bit   4, b
        jr    z, .l1                    ; end of data not reached yet?
    if NO_BORDER_FX == 0
        xor   a
        out   (81h), a
    endif
        pop   bc                        ; pop return address
        pop   de
        ret

readBits:
        dec   b
.l1:    ld    de, 1
        inc   b
        ret   z
.l2:    add   a, a
        jr    nz, .l3
        ld    a, (hl)
        inc   hl
        rla
.l3:    rl    e
        rl    d
        djnz  .l2                       ; Carry = 0 on return if bits are read,
        ret                             ; not changed otherwise

