
; if non-zero, there are no border effects, and code size is smaller by 5 bytes
NO_BORDER_FX            equ     0
; disable the use of self-modifying code (2 bytes smaller, but uses IYH)
READ_ONLY_CODE          equ     0
; smaller code by 7 bytes, but ~3.5% slower
SIZE_OPTIMIZED          equ     0

; total table size is 156 bytes, and should not cross a 256-byte page boundary
decodeTablesBegin       equ     0ff64h
        assert  (decodeTablesBegin & 001fh) == 0004h
        assert  (decodeTablesBegin & 00ffh) >= 0018h
        assert  (decodeTablesBegin & 00ffh) <= 0064h

offs1DecodeTable        equ     decodeTablesBegin + 104
lengthDecodeTable       equ     offs1DecodeTable + 4
offs2DecodeTable        equ     lengthDecodeTable + 8
offs3DecodeTable        equ     offs2DecodeTable + 8
decodeTablesEnd         equ     offs3DecodeTable + 32

; input/output parameters (updated on return):
;   HL:               source address (forward decompression)
;   DE:               destination address
; registers used:
;   BC:               0 on return
;   A:                undefined if NO_BORDER_FX, 0 otherwise
;   IX:               undefined
;   IYH:              undefined if READ_ONLY_CODE, otherwise not changed

decompressData:
        ld      a, 80h                  ; initialize shift register
.l1:    push    de                      ; decompress data block
        ld      bc, 4002h
        add     a, a
        call    readBitsB.l2            ; get prefix size for >= 3 byte matches
        inc     b
    if READ_ONLY_CODE == 0
        ld      d, a
        ld      a, low ((offs3DecodeTable >> 1) | 80h)
.l2:    sla     c                       ; len >= 3 offset slots: 4, 8, 16, 32
        rra
        djnz    .l2
        ld      (copyLZMatch.l1 + 1), a
        ld      a, d
    else
        ld      d, low ((offs3DecodeTable >> 1) | 80h)  ; prefix size codes:
.l2:    sla     c                       ; 40h, 20h, 10h, 08h with table offset
        srl     d
        djnz    .l2
        ld      iyh, d
    endif
        ld      ix, decodeTablesBegin   ; initialize decode tables
.l3:    ld      de, 1
.l4:    ld      b, 10h                  ; Carry must be 0 here
        call    readBitsB
        inc     b
        ld      (ix + 104), b           ; store the number of bits to read + 1
        ld      (ix), e                 ; store base value LSB
        ld      (ix + 52), d            ; store base value MSB
        inc     ixl
        push    hl
        ld      hl, 1                   ; calculate 2 ^ nBits
        defb    0feh                    ; = CP n
.l5:    add     hl, hl
        djnz    .l5
        add     hl, de                  ; calculate new base value
        ex      de, hl
        pop     hl
        ld      b, a
        ld      a, ixl
        sub     low (offs3DecodeTable - 104)
        jr      nc, .l6
        and     07h
        ld      a, b
        jr      nz, .l4
.l6:    ld      a, b
        jr      z, .l3                  ; at the beginning of the next table?
        dec     c
        jr      nz, .l4                 ; continue until all tables are read
        pop     de                      ; DE = decompressed data write address
        jr      .l10                    ; jump to main decompress loop
.l8:    ldir                            ; copy literal sequence
        ld      c, 15
        ldir
.l9:    ldi                             ; copy literal byte
.l10:   add     a, a                    ; read flag bit
        jr      nz, .l11
        ld      a, (hl)
        inc     hl
        rla
.l11:   jr      nc, .l9                 ; literal byte?
        ld      bc, 0800h | (lengthDecodeTable & 00ffh)
.l12:   add     a, a                    ; read length prefix bits
        jr      nz, .l13
        ld      a, (hl)
        inc     hl
        rla
.l13:   jr      nc, copyLZMatch
        inc     c
        djnz    .l12
        ld      c, (hl)                 ; literal sequence, read length - 16
        inc     hl
        dec     c
        jr      z, .l1                  ; next block?
        inc     c
        jr      nz, .l8                 ; not end of block?
    if NO_BORDER_FX == 0
        xor     a                       ; reset border color
        out     (81h), a
    endif
        ret

copyLZMatch:
        push    de
    if SIZE_OPTIMIZED == 0
        ld      ixl, c
    else
        ld      b, c
    endif
        call    decodeLZMatchParam      ; decode length
    if NO_BORDER_FX == 0
        out     (81h), a
    endif
        push    de                      ; Carry = 0 from decodeLZMatchParam
        inc     d
        dec     d
        jr      nz, .l1                 ; length >= 256 bytes?
        ld      b, low (((offs1DecodeTable & 00ffh) | 0100h) >> 2)
        dec     e
        jr      z, .l2                  ; length == 1?
        ld      b, low (((offs2DecodeTable & 00ffh) | 0100h) >> 3)
        dec     e
        jr      z, .l2                  ; length == 2?
.l1:                                    ; length >= 3 bytes,
                                        ; variable prefix size
    if READ_ONLY_CODE == 0
        ld      b, low (((offs3DecodeTable & 00ffh) | 0100h) >> 4)      ; *
    else
        ld      b, iyh
    endif
.l2:    call    readBitsB               ; read offset prefix and decode offset
        pop     bc                      ; BC = length
        ex      (sp), hl
        push    hl
        sbc     hl, de                  ; Carry = 0
        pop     de
        ldir                            ; expand match
        pop     hl
    if SIZE_OPTIMIZED == 0
        jp      decompressData.l10      ; return to main decompress loop
    else
        jr      decompressData.l10
    endif
.l3:    ld      a, (hl)
        inc     hl

readBitsB:
.l1:    adc     a, a
.l2:    jr      z, copyLZMatch.l3
        rl      b
        jr      nc, .l1
        ret     p
    if SIZE_OPTIMIZED == 0
        ld      ixl, b
    endif

decodeLZMatchParam:
    if SIZE_OPTIMIZED != 0
        ld      ixl, b                  ; IX = table address
    endif
        ld      b, (ix)                 ; B = number of extra bits + 1
        ld      c, (ix - 104)
    if SIZE_OPTIMIZED == 0
        djnz    .l1
        ld      e, c
        ld      d, (ix - 52)
        or      a                       ; Carry = 0 on return
        ret
    endif
.l1:    ex      de, hl
        ld      hl, 0
    if SIZE_OPTIMIZED != 0
        dec     b
        jr      z, .l4
    endif
.l2:    add     a, a
        jp      nz, .l3
        ld      a, (de)
        inc     de
        rla
.l3:    adc     hl, hl
        djnz    .l2
.l4:    ld      b, (ix - 52)            ; BC = base value
        add     hl, bc
        ex      de, hl
        ret

