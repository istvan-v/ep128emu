
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
;   A, IX, IYH:       undefined

decompressData:
        ld      a, 80h                  ; initialize shift register
.l1:    push    de                      ; decompress data block
        ld      bc, 4002h
        call    readBitsB               ; get prefix size for >= 3 byte matches
        inc     b
        ld      d, low ((offs3DecodeTable >> 1) | 80h)  ; prefix size codes:
.l2:    sla     c                       ; 40h, 20h, 10h, 08h with table offset
        srl     d
        djnz    .l2
        ld      iyh, d
        ld      ix, decodeTablesBegin   ; initialize decode tables
.l3:    ld      de, 1
.l4:    call    read4Bits
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
.l7:    push    de
        call    decodeLZMatchParam      ; decode length
        push    de                      ; Carry = 0 from decodeLZMatchParam
        inc     d
        dec     d
        jr      nz, .l8                 ; length >= 256 bytes?
        ld      b, low (((offs1DecodeTable & 00ffh) | 0100h) >> 2)
        dec     e
        jr      z, .l9                  ; length == 1?
        ld      b, low (((offs2DecodeTable & 00ffh) | 0100h) >> 3)
        dec     e
        jr      z, .l9                  ; length == 2?
.l8:    ld      b, iyh                  ; length >= 3 bytes,
                                        ; variable prefix size
.l9:    call    readBitsB               ; read offset prefix and decode offset
        pop     bc                      ; BC = length
        ex      (sp), hl
        push    hl
        sbc     hl, de                  ; Carry = 0
        pop     de
        ldir                            ; expand match
        pop     hl
.l10:   add     a, a                    ; read flag bit
        call    z, readCompressedByte
        jr      nc, .l12                ; literal byte?
        ld      bc, 0800h | (lengthDecodeTable & 00ffh)
.l11:   add     a, a                    ; read length prefix bits
        call    z, readCompressedByte
        jr      nc, .l7                 ; LZ77 match?
        inc     c
        djnz    .l11
        ld      c, (hl)                 ; literal sequence, read length - 16
        inc     hl
        dec     c
        jr      z, .l1                  ; next block?
        inc     c
        ret     z                       ; end of last block?
        ldir                            ; copy literal sequence
        ld      c, 15
        ldir
.l12:   ldi                             ; copy literal byte
        jr      .l10                    ; return to main decompress loop

read4Bits:
        ld      b, 10h

readBitsB:
.l1:    add     a, a
        call    z, readCompressedByte
        rl      b
        jr      nc, .l1
        ret     p
        ld      c, b

decodeLZMatchParam:
        ld      ixl, c                  ; IX = table address
        ld      b, (ix)                 ; B = number of extra bits + 1
        ld      de, 0
        jr      .l2
.l1:    add     a, a
        call    z, readCompressedByte
        rl      e
        rl      d
.l2:    djnz    .l1
        ld      c, (ix - 104)
        ld      b, (ix - 52)            ; BC = base value
        ex      de, hl
        add     hl, bc
        ex      de, hl                  ; Carry = 0 on return
        ret

readCompressedByte:
        ld      a, (hl)
        inc     hl
        rla
        ret

