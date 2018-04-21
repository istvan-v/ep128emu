
; if non-zero, there are no border effects, and code size is smaller by 5 bytes
NO_BORDER_FX            equ     0
; disable the use of self-modifying code (1 byte smaller, but uses IX)
READ_ONLY_CODE          equ     0

; total table size is 156 bytes, and should not cross a 256-byte page boundary
decodeTablesBegin       equ     0ff00h

offs1DecodeTable        equ     decodeTablesBegin
lengthDecodeTable       equ     offs1DecodeTable + (4 * 3)
offs2DecodeTable        equ     lengthDecodeTable + (8 * 3)
offs3DecodeTable        equ     offs2DecodeTable + (8 * 3)
decodeTablesEnd         equ     offs3DecodeTable + (32 * 3)

; input/output parameters (updated on return):
;   HL:               source address (forward decompression)
;   DE:               destination address
; registers used:
;   BC:               0 on return
;   A:                undefined if NO_BORDER_FX, 0 otherwise
;   IX:               undefined if READ_ONLY_CODE, otherwise not changed

decompressData:
        ld      a, 80h                  ; initialize shift register
.l1:    push    de                      ; decompress data block
        ld      bc, 4002h
        or      a
        call    readBitsB               ; get prefix size for >= 3 byte matches
        inc     b
        push    hl
    if READ_ONLY_CODE == 0
        ld      hl, 0f000h + ((10000h - decodeTablesEnd) & 00ffh)
.l2:    sla     c                       ; len >= 3 offset slots: 4, 8, 16, 32
        srl     h
        djnz    .l2
        ld      (copyLZMatch.l1 + 1), hl
    else
        ld      d, 0f0h                 ; prefix size codes: 40h, 20h, 10h, 08h
.l2:    sla     c                       ; with -20h offset
        srl     d
        djnz    .l2
        ld      ixh, d
    endif
        ld      hl, decodeTablesBegin   ; initialize decode tables
.l3:    ld      de, 1
.l4:    ld      b, 10h
        ex      (sp), hl                ; Carry must be 0 here
        call    readBitsB
        inc     b
        ex      (sp), hl
        ld      (hl), b                 ; store the number of bits to read + 1
        inc     l
        ld      (hl), e                 ; store base value LSB
        inc     l
        ld      (hl), d                 ; store base value MSB
        inc     l
        push    hl
        ld      hl, 1                   ; calculate 2 ^ nBits
        defb    0feh                    ; = CP n
.l5:    add     hl, hl
        djnz    .l5
        add     hl, de                  ; calculate new base value
        ex      de, hl
        pop     hl
        ld      b, a
        ld      a, l
        sub     low offs3DecodeTable
        jr      nc, .l6
        and     07h
        ld      a, b
        jr      nz, .l4
.l6:    ld      a, b
        jr      z, .l3                  ; at the beginning of the next table?
        dec     c
        jr      nz, .l4                 ; continue until all tables are read
        pop     hl
        pop     de                      ; DE = decompressed data write address
        jr      .l10                    ; jump to main decompress loop
.l7:    jr      c, .l1                  ; more blocks remaining?
    if NO_BORDER_FX == 0
        xor     a                       ; reset border color
        out     (81h), a
    endif
        ret
.l8:    ld      c, (hl)                 ; read length - 16
        srl     c
        ld      c, (hl)
        inc     hl
        jr      z, .l7                  ; end of block?
        ldir
        ld      c, 15
        ldir
.l9:    ldi                             ; copy literal byte
.l10:   add     a, a                    ; read flag bit
        jr      nz, .l11
        ld      a, (hl)
        inc     hl
        rla
.l11:   jr      nc, .l9                 ; literal byte?
        ld      bc, 0f700h + ((10000h - offs2DecodeTable) & 00ffh)
.l12:   inc     b
        jr      z, .l8                  ; literal sequence?
        add     a, a                    ; read length prefix bits
        jr      nz, .l13
        ld      a, (hl)
        inc     hl
        rla
.l13:   jr      c, .l12

copyLZMatch:
        push    de
        call    decodeLZMatchParam      ; decode length
    if NO_BORDER_FX == 0
        out     (81h), a
    endif
        push    de                      ; Carry = 0 from decodeLZMatchParam
        inc     d
        dec     d
        jr      nz, .l1                 ; length >= 256 bytes?
        dec     e
        jr      z, .l2                  ; length == 1?
        ld      bc, 3f00h + ((10000h - offs3DecodeTable) & 00ffh)
        dec     e
        jr      z, .l3                  ; length == 2?
.l1:                                    ; length >= 3 bytes,
                                        ; variable prefix size
    if READ_ONLY_CODE == 0
        ld      bc, 1e00h + ((10000h - decodeTablesEnd) & 00ffh)        ; *
    else
        ld      c, low (10000h - decodeTablesEnd)
        ld      b, ixh
    endif
        jr      .l3
.l2:    ld      bc, 7f00h + ((10000h - lengthDecodeTable) & 00ffh)
.l3:    call    .l5                     ; read offset prefix and decode offset
        pop     bc                      ; BC = length
        ex      (sp), hl
        push    hl
        sbc     hl, de                  ; Carry = 0
        pop     de
        ldir                            ; expand match
        pop     hl
        jr      decompressData.l10      ; return to main decompress loop
.l4:    ld      a, (hl)
        inc     hl
.l5:    adc     a, a                    ; readBitsB
        jr      z, .l4
        rl      b
        jr      nc, .l5
        ret     p

readBitsB       equ     copyLZMatch.l5

decodeLZMatchParam:
        ex      de, hl
        ld      h, a
        ld      a, b                    ; calculate table address L (3 * B - C)
        add     a, a
        add     a, b
        sub     c
        ld      l, a                    ; Carry = 0
        ld      a, h
        ld      h, high decodeTablesBegin
        ld      b, (hl)                 ; B = number of extra bits + 1
        inc     l
        ld      c, (hl)
        inc     l
        ld      h, (hl)
        ld      l, c                    ; HL = base value
        dec     b
        jr      z, .l3
        push    hl
        ld      hl, 0
.l1:    add     a, a
        jp      nz, .l2
        ld      a, (de)
        inc     de
        rla
.l2:    adc     hl, hl
        djnz    .l1
        pop     bc
        add     hl, bc
.l3:    ex      de, hl
        ret

