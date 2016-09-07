
; border effects are disabled if this is set to any non-zero value
NO_BORDER_FX            equ     0

decompressCodeBegin     equ     0fe24h
; total table size is 156 bytes
; NOTE: the upper byte of the address of all table entries must be the same
decodeTablesBegin       equ     0ff4ch
; stack: 0ffe8h-

;       var 045h,file:decompress.com

; -----------------------------------------------------------------------------

; A':  temp. register
; BC': symbols (literal byte or match code) remaining
; D':  temp. register
; E':  shift register
; HL': compressed data read address
; A:   temp. register
; BC:  temp. register (number of literal/LZ77 bytes to copy)
; DE:  decompressed data write address
; HL:  temp. register (literal/LZ77 data source address)
; IXH: decode table upper byte

nLengthSlots            equ 8
nOffs1Slots             equ 4
nOffs2Slots             equ 8
maxOffs3Slots           equ 32
totalSlots              equ nLengthSlots+nOffs1Slots+nOffs2Slots+maxOffs3Slots
; NOTE: the upper byte of the address of all table entries must be the same
slotBitsTable           equ decodeTablesBegin
slotBaseLowTable        equ slotBitsTable+totalSlots
slotBaseHighTable       equ slotBaseLowTable+totalSlots
slotBitsTableL          equ slotBitsTable
slotBaseLowTableL       equ slotBaseLowTable
slotBaseHighTableL      equ slotBaseHighTable
slotBitsTableO1         equ slotBitsTableL+nLengthSlots
slotBaseLowTableO1      equ slotBaseLowTableL+nLengthSlots
slotBaseHighTableO1     equ slotBaseHighTableL+nLengthSlots
slotBitsTableO2         equ slotBitsTableO1+nOffs1Slots
slotBaseLowTableO2      equ slotBaseLowTableO1+nOffs1Slots
slotBaseHighTableO2     equ slotBaseHighTableO1+nOffs1Slots
slotBitsTableO3         equ slotBitsTableO2+nOffs2Slots
slotBaseLowTableO3      equ slotBaseLowTableO2+nOffs2Slots
slotBaseHighTableO3     equ slotBaseHighTableO2+nOffs2Slots

        org     decompressCodeBegin

decompressData:
        push hl
        exx
        pop hl
        inc hl                          ; skip checksum byte
        ld e, 080h                      ; initialize shift register
        exx
ldd_01: call decompressDataBlock        ; decompress all blocks
        jr z, ldd_01
        if NO_BORDER_FX=0
        xor a                           ; reset border color
        out (081h), a
        endif
        ret

decompressDataBlock:
        ld xh, high slotBitsTable       ; store decode table address MSB in IXH
        call read8Bits                  ; read number of symbols - 1 (BC)
        ld b, a
        call read8Bits
        ld c, a
        ld a, 040h
        call readBits                   ; read flag bits
        srl a
        push af                         ; save last block flag (A=1,Z=0)
        jr c, lddb02                    ; is compression enabled ?
        inc bc                          ; no, copy uncompressed literal data
        exx
        ld bc, 00101h
        jr lddb11
lddb02: push bc                         ; compression enabled:
        exx
        pop bc                          ; store the number of symbols in BC'
        inc c
        inc b
        ld a, 040h
        call readBits_                  ; get prefix size for length >= 3 bytes
        ld b, a
        inc b
        ld a, 002h
        ld hl, offs3PrefixSize
        ld (hl), 080h
lddb03: add a, a
        srl (hl)
        djnz lddb03
        add a, nLengthSlots+nOffs1Slots+nOffs2Slots-3
        push de                         ; save decompressed data write address
        ld c, a                         ; store total table size - 3 in C
        ld xl, low slotBitsTable        ; initialize decode tables
lddb04: ld hl, 00001h
lddb05: ld (ix+totalSlots), l           ; store base value LSB
        ld (ix+totalSlots*2), h         ; store base value MSB
        ld a, 010h
        call readBits
        ld (ix), a                      ; store the number of bits to read
        ex de, hl
        ld hl, 00001h
        jr z, lddb07
        ld b, a                         ; calculate 1 << nBits
lddb06: add hl, hl
        djnz lddb06
lddb07: add hl, de                      ; calculate new base value
        inc xl
        ld a, xl
        cp low slotBitsTableO1
        jr z, lddb04                    ; end of length decode table ?
        cp low slotBitsTableO2
        jr z, lddb04                    ; end of offset table for length=1 ?
        cp low slotBitsTableO3
        jr z, lddb04                    ; end of offset table for length=2 ?
        dec c
        jr nz, lddb05                   ; continue until all tables are read
        pop de                          ; DE = decompressed data write address
        exx
lddb08: xor a
lddb09: sla e
        jp nz, lddb10
        ld e, (hl)
        inc hl
        rl e
lddb10: jr nc, lddb12
        inc a
        cp 9
        jp c, lddb09
        exx
        call read8Bits                  ; get literal sequence length - 17
        sub 256-17
        ld c, a
        sbc a, a
        inc a
        ld b, a
        exx
lddb11: push hl
        exx
        pop hl
        ldir                            ; copy literal sequence
        push hl
        exx
        pop hl
        jr lddb14
lddb12: dec a
        jp p, copyLZMatch               ; LZ77 match ?
        ld a, (hl)                      ; copy literal byte
        inc hl
        exx
        ld (de), a
        inc de
lddb13: exx
lddb14: dec c
        jr nz, lddb08
        djnz lddb08
        exx
        pop af                          ; return with last block flag
        ret                             ; (A=1,Z=0 if last block)

copyLZMatch:
        exx
        add a, low slotBitsTableL
        call readEncodedValue           ; decode match length
        ld c, l
        jr nz, lclm04                   ; length >= 256 bytes ?
        ld b, l
        djnz lclm03                     ; length > 1 byte ?
        ld a, 040h                      ; no, read 2 prefix bits
        ld l, low slotBitsTableO1
lclm01: ld b, h
        call readBits
        add a, l
        call readEncodedValue           ; decode match offset
lclm02: ld a, e                         ; calculate LZ77 match read address
        if NO_BORDER_FX=0
        out (081h), a
        endif
        sub l
        ld l, a
        ld a, d
        sbc a, h
        ld h, a
        ldir                            ; copy match data
        jp lddb13                       ; return to main decompress loop
lclm03: djnz lclm04                     ; length > 2 bytes ?
        ld a, 020h                      ; no, read 3 prefix bits
        ld l, low slotBitsTableO2
        jr lclm01
lclm04: ld a, 010h                      ; length >= 3 bytes,
        ld l, low slotBitsTableO3       ; variable prefix size
        jr lclm01

offs3PrefixSize         equ lclm04+1

read8Bits:
        ld a, 001h

readBits:
        exx

readBits_:
lrb_01: sla e
        jp nz, lrb_02
        ld e, (hl)
        inc hl
        rl e
lrb_02: adc a, a
        jp nc, lrb_01
        exx
        ret

readEncodedValue:
        ld xl, a
        xor a
        ld h, a
        ld l, (ix)
        cp l
        jr z, lrev03
lrev01: exx
        sla e
        jp nz, lrev02
        ld e, (hl)
        inc hl
        rl e
lrev02: exx
        adc a, a
        rl h
        dec l
        jp nz, lrev01
lrev03: add a, (ix+totalSlots)
        ld l, a
        ld a, h
        adc a, (ix+totalSlots*2)
        ld h, a
        ret

decompressCodeEnd:

