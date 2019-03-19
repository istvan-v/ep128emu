
; disable support for -m6 compression (143 bytes)
NO_M6_FORMAT            equ     0
; disable EXOS 11 support (8 bytes)
NO_SPECIAL_FUNC         equ     0
; disable ROMDIR command and help string (70 bytes)
NO_ROMDIR_CMD           equ     0
; do not include support for printing version and usage (174 or 143 bytes)
NO_HELP_STRING          equ     0
; disable support for loading the first file on startup (39 bytes)
NO_START_FILE           equ     0
; disable border effects (6 bytes, or 4 if NO_M6_FORMAT)
NO_BORDER_FX            equ     0

    macro exos n
        rst     030h
        defb    n
    endm

        org     0c000h

        defm    "EXOS_ROM"
        defw    (deviceChain & 3fffh) + 4000h

main:
        ld      a, c
    if NO_ROMDIR_CMD == 0
        cp      2
        jr      z, parseCommand
      if NO_HELP_STRING == 0
        cp      3
        jr      z, helpString
      endif
    endif
        cp      5
        jr      z, errorString
    if NO_START_FILE == 0
        dec     a
        jr      z, coldReset
    endif

irqRoutine:
closeChannel:
destroyChannel:
initDevice:
bufferMoved:
        xor     a
        ret

createChannel:
writeCharacter:
writeBlock:
channelReadStatus:
setChannelStatus:
    if NO_SPECIAL_FUNC != 0
specialFunction:
    endif
invalidFunction:
        ld      a, 0e7h                 ; .NOFN
        ret

errorString:
        ld      a, b
        xor     0b8h                    ; data error
        jr      z, .l1
        add     a, 256 - 77h            ; file not found (CFh XOR B8h)
        ret     nz
        ld      de, errMsgCF - 8000h
.l1:    ld      c, a                    ; A = 0
        in      a, (0b3h)
        ld      b, a
        ret     c
        ld      de, errMsgB8 - 8000h
        ret

    if NO_ROMDIR_CMD == 0
parseCommand:
        ld      a, b
        cp      6                       ; length of "ROMDIR"
        ret     nz
        ld      hl, romdirCommand
.l1:    push    bc
        push    de
.l2:    inc     de
        ld      a, (de)
        xor     (hl)
        inc     hl
        jr      nz, .l3
        djnz    .l2
.l3:    pop     de
        pop     bc
        ret     nz
        ld      c, a                    ; A = 0
      if NO_HELP_STRING == 0
        bit     1, b
        jr      z, helpString.l1        ; checking for "ROMFS"?
      endif

printFileList:
        ld      hl, fileSystemData
.l1:    ld      b, (hl)
        inc     hl
        cp      b                       ; A = 0
        jr      z, .l3
        ret     p
.l2:    ld      a, (hl)
        call    .l4
        djnz    .l2
.l3:    ld      a, 0dh                  ; CR
        call    .l4
        ld      a, 0ah                  ; LF
        call    .l4
        inc     hl
        jr      .l1
.l4:    inc     hl
        push    bc
        ld      b, a
        ld      a, 0ffh
        exos    7
        pop     bc
        ret     z
        pop     hl
        ret

      if NO_HELP_STRING == 0
helpString:
        ld      hl, helpCmdName
        ld      a, b
        cp      5                       ; length of "ROMFS"
        jr      z, parseCommand.l1
        or      a
        ret     nz
        ld      l, longHelp - shortHelp
        defb    0dah                    ; = JP C, nn
.l1:    ld      l, helpStrEnd - shortHelp
        push    bc
        push    de
        ld      de, shortHelp
        ld      c, l
        ld      b, a                    ; A = 0
        dec     a
        exos    8
        pop     de
        pop     bc
        ret
      endif
    endif

    if NO_START_FILE == 0
coldReset:
        ld      a, 3
        di
        out     (0b5h), a
        in      a, (0b5h)
        ei
        and     02h                     ; 1
        ret     nz
        ld      de, startFileName
        exos    1
        jr      nz, .l1
        ld      b, a
        ld      de, 3000h
        exos    29
.l1:    xor     a
        exos    3
        ld      c, 1
        ret
    endif

        defw    0                       ; XX_NEXT_LOW, XX_NEXT_HI
        defw    0fffdh                  ; XX_RAM_LOW, XX_RAM_HI
lDDTyp: defb    00h                     ; DD_TYPE
        defb    00h                     ; DD_IRQFLAG
        defb    00h                     ; DD_FLAGS
        defw    (entryPointTable & 3fffh) + 4000h   ; DD_TAB_LOW, DD_TAB_HI
        defb    30h                     ; DD_TAB_SEG
        defb    00h                     ; DD_UNIT_COUNT
fileDeviceName:
        defb    3                       ; DD_NAME
        defm    "ROM"
deviceChain:
        defb    low (deviceChain - lDDTyp)  ; XX_SIZE

entryPointTable:
        defw    irqRoutine
        defw    openChannel
        defw    createChannel
        defw    closeChannel
        defw    destroyChannel
        defw    readCharacter
        defw    readBlock
        defw    writeCharacter
        defw    writeBlock
        defw    channelReadStatus
        defw    setChannelStatus
        defw    specialFunction
        defw    initDevice
        defw    bufferMoved
        defw    invalidFunction
        defw    invalidFunction

openChannel:
        ld      hl, fileSystemData
        push    de
        ld      a, (de)
        dec     a
        jr      nz, .l1
        inc     de
        ld      a, (de)
        xor     2ah                     ; '*'
        jr      nz, .l4
        ld      e, l                    ; open first file
        ld      d, h
.l1:    ld      b, (hl)
        inc     b
        jr      z, .l5
.l2:    ld      a, (de)
        cp      (hl)
        jr      nz, .l3
        inc     de
        inc     hl
        djnz    .l2
        ld      de, 3
        ld      c, b
        exos    27
        pop     de
        ret     nz
        in      a, (0b3h)
        add     a, (hl)
        inc     hl
        ld      b, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, b
        jr      saveFilePtr.l2          ; Carry = 0: returns A = 0
.l3:    ld      a, b
        add     a, 3
        ld      e, a
        ld      d, 0
        add     hl, de
.l4:    pop     de
        push    de
        jr      .l1
.l5:    pop     de
        ld      a, 0cfh                 ; "file not found"
        ret

readCharacter:
        call    getFilePtr
        or      a
        jr      nz, fileError.l1
        ld      b, (hl)
        call    incFilePtr
        exx
        jr      saveFilePtr.l1

readBlock:
        call    getFilePtr
    if NO_M6_FORMAT == 0
        ld      iyl, a
        and     0fdh                    ; 6 -> 4, does not change .EOF (E4h)
    endif
        cp      4
        jr      nz, fileError
        ld      a, c
        sub     (hl)
        exx
        ld      e, a
        exx
        call    incFilePtr
        ld      a, b
        sbc     a, (hl)
        jr      c, dataError
        exx
        push    de
        ld      d, a
        push    de
        exx
        call    incFilePtr
        ld      a, d
        rlca
        rlca
        exx
        ld      hl, 0bffch              ; USR_P0
        ld      d, h
        or      l
        ld      l, a                    ; (HL') = current output segment
        ld      a, (hl)
        di
        out     (0b0h), a               ; write decompressed data to page 0
        exx
        ld      a, d
        and     high 3fffh
        ld      d, a
    if NO_M6_FORMAT == 0
        ld      a, iyl
        or      a
        call    pe, decompressM6        ; returns parity even
        call    po, decompressM4
    else
        call    decompressM4
    endif
        exx
        ld      a, l
        exx
        rrca
        rrca
        and     high 0c000h
        or      d
        ld      d, a
        pop     bc
        ld      a, c
        or      b
        add     a, 0ffh                 ; Carry = 1 if BC > 0

saveFilePtr:
        exx
        pop     de
.l1:    out     (c), d
        ld      a, b
        exx
.l2:    ld      (ix - 3), a             ; Carry must be 0 if not from readBlock
        ld      (ix - 2), l
        ld      (ix - 1), h
        sbc     a, a
        ret     z                       ; no error?
        exx
        out     (c), b
        exx
        ld      a, (hl)                 ; check if there are more blocks

fileError:
        or      a
.l1:    ret     m                       ; EOF?

dataError:
        ld      a, 0b8h                 ; "data error"
        ret

    if NO_SPECIAL_FUNC == 0
specialFunction:
        in      a, (0b3h)
        ld      c, a
        ld      de, fileSystemData - 8000h
        xor     a
        ret
    endif

errMsgCF:
        defb    14
        defm    "File not found"
errMsgB8:
        defb    10
        defm    "Data error"
  if NO_ROMDIR_CMD == 0
romdirCommand:
        defm    "ROMDIR"
    if NO_HELP_STRING == 0
helpCmdName:
shortHelp:
        defm    "ROMFS version 1.0\r\n"
longHelp:
        defm    "\r\n:ROMDIR  list files\r\n"
        defm    "LOAD \"ROM:filename\"  start file\r\n"
      if NO_START_FILE == 0
        defm    "Press 1 at EP logo screen, or\r\n"
      endif
        defm    "LOAD \"ROM:*\"  start first file\r\n"
helpStrEnd:
    endif
  endif
  if NO_START_FILE == 0
startFileName:
        defb    5
        defm    "ROM:*"
  endif

; =============================================================================

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

; HL = source address on page 1
; DE = destination address on page 0
; B' = page 1 segment
; C' = B1h
; HL' = output segment table pointer (BFFCh..BFFFh)
; D' = BFh
;
; returns B'HL, L'DE = updated source, destination address

decompressM4:
        push    hl
        ld      hl, 0ff00h              ; allocate memory for tables
        add     hl, sp
        ld      a, l
        ld      l, low decodeTablesBegin
        ex      (sp), hl
        ex      (sp), ix
        ld      sp, ix
        ld      iyl, a                  ; save original stack pointer
        ld      a, 80h                  ; initialize shift register
.l1:    ld      bc, 4002h               ; decompress data block
        call    readBitsB               ; get prefix size for >= 3 byte matches
        inc     b
        ex      af, af'
        ld      a, low ((offs3DecodeTable >> 1) | 80h)  ; prefix size codes:
.l2:    sla     c                       ; 40h, 20h, 10h, 08h with table offset
        rra
        djnz    .l2
        ld      iyh, a
        ld      ixl, low decodeTablesBegin      ; initialize decode tables
        push    de
.l3:    ld      de, 1
.l4:    ex      af, af'
        call    read4Bits
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
        ex      af, af'
        ld      a, ixl
        sub     low (offs3DecodeTable - 104)
        jr      nc, .l6
        and     07h
        jr      nz, .l4
.l6:    jr      z, .l3                  ; at the beginning of the next table?
        dec     c
        jr      nz, .l4                 ; continue until all tables are read
        pop     de                      ; DE = decompressed data write address
        jr      .l11                    ; jump to main decompress loop
.l7:    ld      c, 15
        ccf
.l8:    ldi                             ; copy literal sequence
        ex      af, af'
        ld      a, d
        rla
        or      h
        call    m, updatePaging
        ex      af, af'
        jp      pe, .l8
        jr      c, .l7
.l9:    ldi                             ; copy literal byte
        ex      af, af'
        ld      a, d
        rla
        or      h
.l10:   call    m, updatePaging
.l11:   ex      af, af'
        add     a, a                    ; read flag bit
.l12:   jr      nc, .l9                 ; literal byte?
        jr      z, .l15
        ld      bc, 0800h | (lengthDecodeTable & 00ffh)
.l13:   add     a, a                    ; read length prefix bits
        call    z, readCompressedByte
        jr      nc, copyLZMatch
        inc     c
        djnz    .l13
        ld      c, (hl)                 ; literal sequence, read length - 16
        call    incFilePtr
        dec     c                       ; Carry = 1
        jr      z, .l1                  ; next block?
        inc     c
        jr      nz, .l8                 ; not end of block?
        ld      a, ixh
        inc     a
        ld      iyh, a
        ld      sp, iy                  ; restore stack pointer
        pop     ix
.l14:   ld      a, (0bffch)
        out     (0b0h), a
        xor     a
    if NO_BORDER_FX == 0
        out     (81h), a                ; reset border color
    endif
        ret
.l15:   ld      a, (hl)
        rla
        inc     l
        jr      nz, .l12
        inc     h
        call    m, incFilePtr.l1
        jr      .l12

copyLZMatch:
        push    de
        ld      ixl, c
        call    decodeLZMatchParam      ; decode length
    if NO_BORDER_FX == 0
        out     (81h), a
    endif
        push    de
        inc     d
        dec     d
        jr      nz, .l1                 ; length >= 256 bytes?
        dec     e
        jr      z, .l2                  ; length == 1?
        ld      b, low (((offs2DecodeTable & 00ffh) | 0100h) >> 3)
        dec     e
        jr      z, .l3                  ; length == 2?
.l1:    ld      b, iyh                  ; length >= 3 bytes,
                                        ; variable prefix size
        defb    0cah                    ; = JP Z, nn
.l2:    ld      b, low (((offs1DecodeTable & 00ffh) | 0100h) >> 2)
.l3:    call    readBitsB               ; read offset prefix and decode offset
        pop     bc                      ; BC = length
        ex      (sp), hl
        ex      af, af'
        ex      de, hl
        ld      a, e                    ; calculate source address
        sub     l
        ld      l, a
        ld      a, d
        sbc     a, h
        ld      h, a
        jr      c, .l7                  ; source on different page?
        ld      a, d
        sub     high 4000h
        adc     a, b
        jr      c, .l5                  ; sequence crosses page boundary?
.l4:    ldir                            ; expand match
        pop     hl
        jp      decompressM4.l11        ; return to main decompress loop
.l5:    jr      nz, .l6
        ld      a, e
        add     a, c
        jr      nc, .l4
.l6:    ld      a, h
.l7:    call    .l8
        exx
        out     (c), b
        exx
        pop     hl
        add     a, a
        jp      decompressM4.l10
.l8:    or      high 3fffh
        sub     d
        rlca
        rlca
        exx
        add     a, l
        or      low 0bffch
        ld      e, a
.l9:    ld      a, (de)
        out     (0b1h), a
        exx
        res     7, h
        set     6, h
.l10:   ldi
        ld      a, d
        ret     po
        rla
        or      h
        jp      p, .l10
        call    updatePaging.l1
        bit     7, h
        jr      z, .l10
        exx
        inc     e
        jr      .l9

getFilePtr:
        ld      l, (ix - 2)
        ld      h, (ix - 1)
        exx
        ld      c, 0b1h
        in      d, (c)                  ; cannot be segment 0, clears Z
                                        ; D' = saved channel RAM segment
        ld      b, (ix - 3)             ; B' = current input segment
        out     (c), b
        exx
        ld      a, (hl)
        defb    0cah                    ; = JP Z, nn

readCompressedByte:
        ld      a, (hl)
        rla

incFilePtr:
        inc     l
        ret     nz
        inc     h
        ret     p
.l1:    ld      h, high 4000h
        exx
        inc     b
        out     (c), b
        exx
        ret

updatePaging:
        bit     7, h
        call    nz, incFilePtr.l1
.l1:    bit     6, d
        ret     z
.l2:    ld      d, e                    ; E = 0
        exx
        inc     l
        jr      nz, .l3
        ld      l, low 0bffch
.l3:    ld      a, (hl)
        out     (0b0h), a
        exx
        ret

read4Bits:
        ld      b, 10h

readBitsB:
        add     a, a
        call    z, readCompressedByte
        rl      b
.l1:    add     a, a
        call    z, readCompressedByte
        rl      b
        jr      nc, .l1
        ret     p
        ld      ixl, b

; IX = table address

decodeLZMatchParam:
        ld      b, (ix)                 ; B = number of extra bits + 1
        djnz    .l1
        ld      e, (ix - 104)
        ld      d, (ix - 52)
        ret
.l1:    ex      de, hl
        ld      hl, 0
.l2:    add     a, a
        jr      z, .l4
.l3:    adc     hl, hl
        djnz    .l2
        ld      c, (ix - 104)
        ld      b, (ix - 52)            ; BC = base value
        add     hl, bc
        ex      de, hl
        ret
.l4:    ld      a, (de)
        rla
        inc     e
        jr      nz, .l3
        ex      de, hl
        inc     h
        call    m, incFilePtr.l1
        ex      de, hl
        jr      .l3

; =============================================================================

; default -m6 encoding: FG0L,G,,12345678,0123456789ABCDEF

; HL = source address on page 1
; DE = destination address on page 0
; B' = page 1 segment
; C' = B1h
; HL' = output segment table pointer (BFFCh..BFFFh)
; D' = BFh
;
; returns B'HL, L'DE = updated source, destination address

    if NO_M6_FORMAT == 0

decompressM6:
        ld      a, 80h                  ; initialize shift register
        ld      b, 0
.l1:    push    de
        call    readLength              ; get literal sequence length
        ld      c, e
        ld      b, d
        pop     de
.l2:    ldi                             ; copy literal sequence
        ex      af, af'
        ld      a, d
        rla
        or      h
        call    m, updatePaging
        ex      af, af'
        jp      pe, .l2
.l3:    push    de
        call    readLength              ; get LZ77 match length - 1
    if NO_BORDER_FX == 0
        out     (81h), a
    endif
        push    de
        ld      de, 1000h               ; Carry = 1 if length is 2 bytes
        ld      b, 3
        jr      c, .l4                  ; length == 2?
        inc     b                       ; length > 2: 4 offset prefix bits
.l4:    call    readBits.l2
        ld      b, e
        call    nc, readBits.l1         ; length == 2?
        dec     de
        call    c, readBits
        pop     bc
        inc     bc                      ; BC = length
        ex      (sp), hl
        ex      af, af'
        ex      de, hl
        ld      a, e                    ; calculate source address
        sub     l
        ld      l, a
        ld      a, d
        sbc     a, h
        ld      h, a
        jr      c, .l8                  ; source on different page?
        ld      a, d
        sub     high 4000h
        adc     a, b
        jr      c, .l7                  ; sequence crosses page boundary?
        ldir                            ; expand match
.l5:    pop     hl
        ex      af, af'
        add     a, a                    ; check flag bit:
.l6:    jr      nc, .l3                 ; LZ77 match?
        jr      nz, .l1                 ; no, continue with literal sequence
        call    readCompressedByte      ; returns Z=0
        jr      .l6
.l7:    ld      a, h
.l8:    call    copyLZMatch.l8
        exx
        out     (c), b
        exx
        add     a, a
        jp      p, .l5
        call    updatePaging.l2
        jr      .l5

; read gamma encoded sequence length (B must be 0)

readLength:
.l1:    add     a, a
        call    z, readCompressedByte
        jr      c, readBits
        inc     b
        bit     4, b
        jr      z, .l1                  ; end of data not reached yet?
        pop     bc                      ; pop return address
        pop     de
        jp      decompressM4.l14

readBits:
        dec     b
.l1:    ld      de, 1
        inc     b
        ret     z
.l2:    add     a, a
        call    z, readCompressedByte
        rl      e
        rl      d
        djnz    .l2                     ; Carry = 0 on return if bits are read,
        ret                             ; not changed otherwise

    endif

; =============================================================================

;       align   16
fileSystemData:

