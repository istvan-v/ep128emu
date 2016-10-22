
codeBegin:

extMain:
main:
        ld a, 2
        cp c
        jr z, lm__02
        inc a
        cp c
        jr z, helpString
        ld a, 5
        cp c
        jp z, errorString
lm__01: xor a
        ret
lm__02: call checkCommandName
        jr nz, lm__01
        jp parseCommand

printString:
        push bc
        push de
        ld d, h
        ld e, l
        jr lhs_01

helpString:
        xor a
        cp b
        jr nz, lhs_03
        push bc
        push de
        ld de, versionMsg
lhs_01: ld a, (de)
        or a
        jr z, lhs_02
        ld b, a
        ld a, 0ffh
        push de
        rst 030h
        defb 7
        pop de
        inc de
        jr lhs_01
lhs_02: pop de
        pop bc
        xor a
        ret
lhs_03: call checkCommandName
        jr z, lhs_04
        xor a
        ret
lhs_04: ld c, a
        push bc
        push de
        ld de, usageMsg
        jr lhs_01

checkCommandName:
        push bc
        push de
        push hl
        ld a, cmdNameLength
        cp b
        jr nz, lccn03
        ld hl, cmdName
lccn01: inc de
        ld a, (de)
        cp (hl)
        jr nz, lccn02
        inc hl
        djnz lccn01
lccn02: ld a, b
lccn03: or a
        pop hl
        pop de
        pop bc
        ret

errorString:
        ld hl, errorTable
les_01: ld a, (hl)
        or a
        ret z
        inc hl
        inc hl
        inc hl
        cp b
        jr nz, les_01
        inc hl
        ld e, (hl)
        inc hl
        ld d, (hl)
        in a, (0b3h)
        ld b, a
        xor a
        ld c, a
        ret

; -----------------------------------------------------------------------------

; IX+0:   file name / I/O buffer address
; IX+2:   input file bytes remaining
; IX+4:   input file channel
; IX+5:   compressed data checksum
; IX+6:   output file channel
; IX+7:   non-zero if the compressed data includes start addresses
; IX+8:   non-zero: load flag (/L)
; IX+9:   non-zero: test flag (/T)
; IX+10:  IVIEW data block uncompressed size
; IX+12:  decompressor input buffer position (HL)
; IX+14:  decompressor output buffer position (DE)
; IX+16:  decompressor segment table pointer (IY)

; IX+24:  saved IY register
; IX+26:  saved stack pointer
; IX+28:  segment table

parseCommand:
        xor a
        ld h, a
        ld l, a
        add hl, sp
        push hl
        pop ix
        defb 0ddh
        ld l, a
        defb 0ddh
        dec h
        defb 0ddh
        dec h
        ld sp, ix
        defb 0ddh
        ld l, 0a4h
        defb 0ddh
        inc h
        ld (ix+26), l                   ; save stack pointer
        ld (ix+27), h
        push iy
        pop hl
        ld (ix+24), l                   ; save IY register
        ld (ix+25), h
        push ix
        pop hl
        dec h
        ld (ix+0), a                    ; store buffer start address
        ld (ix+1), h
        ld (ix+4), a                    ; input file channel
        ld (ix+6), a                    ; output file channel
        ld (ix+8), a                    ; load flag
        ld (ix+9), a                    ; test flag
        inc h
        ld l, 0c0h                      ; create empty segment table
lpc_01: ld (hl), a
        inc l
        jr nz, lpc_01
        dec l
        ld (hl), 1
        push bc
        push de
        rst 030h                        ; allocate first segment
        defb 24
        or a
        ld a, 0f7h
        jp nz, endCmd
        ld (ix+28), c                   ; store segment number
        pop de
        pop bc
        ld a, (de)
        ld b, a
        inc de
        call getNextFileName            ; skip command name
        push bc
        push de
        call getNextFileName            ; check for /L or /T flag
        cp 2
        jr nz, lpc_04
        inc hl
        ld a, (hl)
        cp 02fh
        jr nz, lpc_04
        inc hl
        ld a, (hl)
        cp 054h
        jr z, lpc_03
        cp 074h
        jr z, lpc_03
        cp 04ch
        jr z, lpc_02
        cp 06ch
        jp nz, cmdError
lpc_02: inc (ix+8)                      ; set load flag
        jr lpc_05
lpc_03: inc (ix+9)                      ; set test flag
        jr lpc_05
lpc_04: pop de
        pop bc
        push bc
        push de
lpc_05: call getNextFileName            ; get first file name (required)
        bit 0, (ix+9)
        jr z, lpc_08
        pop hl
        pop hl
lpc_06: push bc                         ; test mode:
        push de
        ld l, (ix+0)
        ld h, (ix+1)
        ld b, (hl)
lpc_07: inc hl
        ld a, (hl)
        push bc
        ld b, a
        ld a, 0ffh
        rst 030h
        defb 7
        pop bc
        djnz lpc_07
        ld hl, testMsg1
        call printString
        call decompressFile
        call closeFiles
        call freeMemory
        ld hl, testMsg2
        call printString
        pop de
        pop bc
        call checkCmdArgs
        jp z, endCmd
        call getNextFileName
        jr lpc_06
lpc_08: push bc                         ; extract mode:
        push de
        call decompressFile
        call closeFiles
        pop de
        pop bc
        call checkCmdArgs
        jr nz, lpc_10                   ; is the output file name specified ?
        bit 0, (ix+8)                   ; load the extracted file ?
        jr z, lpc_09
        pop hl                          ; yes, use temporary file name
        pop hl
        ld hl, tmpFileName
        ld e, (ix+0)                    ; copy the file name to the buffer
        ld d, (ix+1)
        ld c, (hl)
        ld b, 0
        inc bc
        ldir
        jr lpc_11
lpc_09: pop de                          ; overwrite input file
        pop bc
        call getNextFileName
        jr lpc_11
lpc_10: pop hl                          ; use the specified output file name
        pop hl
        call getNextFileName
lpc_11: call writeDecompressedData      ; write output file
        call closeFiles
        call freeAllMemory
        xor a
        bit 0, (ix+8)                   ; if load not requested, then done
        jr z, endCmd
        call openFileR                  ; load module:
        ld a, (ix+4)
        ld l, (ix+24)                   ; clean up first
        ld h, (ix+25)                   ; restore IY
        push hl
        pop iy
        ld l, (ix+26)                   ; restore stack pointer
        ld h, (ix+27)
        ld bc, 16                       ; allocate 16 bytes for the header
        or a
        sbc hl, bc
        ld sp, hl
        ld b, a
lpc_12: ld d, h                         ; read next module
        ld e, l
        push bc
        rst 030h
        defb 29
        pop bc
        or a
        jr z, lpc_12
        ld hl, 16                       ; done loading module, clean up
        add hl, sp
        ld sp, hl
        push af
        ld a, b
        rst 030h
        defb 3
        pop af
        ld c, 0
        cp 0ech                         ; check status: .NOMOD ?
        jr nz, lpc_13
        xor a
        ret
lpc_13: or a
        ret

endCmd:
        defb 0ddh
        ld l, 0a4h
        ld l, (ix+24)                   ; restore IY register
        ld h, (ix+25)
        push hl
        pop iy
        ld (ix+23), a
        call freeAllMemory              ; free all allocated segments
        call closeFiles                 ; close all files
        ld l, (ix+26)                   ; restore stack pointer
        ld h, (ix+27)
        ld sp, hl
        ld c, 0
        ld a, (ix+23)
        or a
        ret

checkCmdArgs:
        ld a, b
        or a
        ret z
lcca01: ld a, (de)
        or a
        ret z
        cp 021h
        jr nc, lcca02
        inc de
        djnz lcca01
        xor a
        ret
lcca02: xor a
        inc a
        ret

getNextFileName:
lgnfn1: ld a, (de)                      ; skip leading white space
        or a
        jr z, cmdError
        cp 021h
        jr nc, lgnfn2
        inc de
        djnz lgnfn1
        jr cmdError
lgnfn2: ld l, (ix+0)
        ld h, (ix+1)
lgnfn3: ld a, (de)
        cp 021h
        jr c, lgnfn4
        inc hl
        ld (hl), a
        inc de
        djnz lgnfn3
lgnfn4: ld a, l
        inc hl
        ld (hl), 0
        ld l, 0
        ld (hl), a
        ret

cmdError:
        ld a, 0f0h
        jp endCmd

decompressFile:
        call openFileR
        call checkFileHeader
        cp 5
        jr z, ldf_02
        cp 049h
        jr z, ldf_03
        call initializeDecompressor     ; raw compressed data:
        ld (ix+2), 0ffh                 ; "infinite" input data size
        ld (ix+3), 0ffh
        ld (ix+7), 0                    ; no start addresses
        inc (ix+12)
        call decompressFileData_
ldf_01: jp closeFiles
ldf_02: call decompressProgram
        jr ldf_01
ldf_03: call decompressImageFile
        jr ldf_01

writeDecompressedData:
        call openFileW
        push ix
        pop hl
        ld l, 0c0h
lwdd01: ld a, (hl)
        cp 2
        jp c, closeFiles
        ld bc, 04000h
        ld d, b
        ld e, c
        inc hl
        ld a, (hl)
        dec hl
        cp 2
        jr nc, lwdd02
        ld c, (ix+14)
        ld b, (ix+15)
        dec bc
        res 7, b
        res 6, b
        inc bc
lwdd02: in a, (0b1h)
        push af
        ld a, (hl)
        inc hl
        out (0b1h), a
        ld a, (ix+6)
        rst 030h
        defb 8
        ld c, a
        pop af
        out (0b1h), a
        ld a, c
        or a
        jr z, lwdd01
        jp endCmd

checkFileHeader:
        ld a, (ix+4)                    ; read header
        ld bc, 16
        ld l, (ix+0)
        ld h, (ix+1)
        ld e, l
        ld d, h
        rst 030h
        defb 6
        cp 0e4h
        jr z, lcfh01                    ; EOF ?
        or a
        jp nz, endCmd
        ld a, e
        cp 16
        jr c, lcfh01
        ld a, (hl)                      ; first byte:
        or a
        jr nz, lcfh01                   ; if non-zero, then raw file
        inc hl
        ld a, (hl)                      ; second byte:
        cp 5
        jr z, lcfh06                    ; program file ?
        cp 049h
        jr z, lcfh05                    ; IVIEW image ?
lcfh01: ld a, e                         ; raw compressed data:
        cp 5
        ld a, 0e4h
        jp c, endCmd                    ; length must be at least 5 bytes
        ld a, e
        cp 16
        jr c, lcfh03
lcfh02: ld a, (ix+4)                    ; fill input buffer
        ld bc, 239
        rst 030h
        defb 6
        cp 0e4h
        jr z, lcfh03
        or a
        jp nz, endCmd
lcfh03: ld b, e                         ; initialize checksum
        ld l, (ix+0)
        ld h, (ix+1)
        ld a, 080h
lcfh04: sub 0ach
        rrca
        xor (hl)
        inc l
        djnz lcfh04
        ld (ix+5), a
        ld l, 0feh                      ; move buffer data
        ld e, 0ffh
        ld bc, 255
        lddr
        xor a                           ; return file type 0 (raw data)
        ret
lcfh05: ld l, 10                        ; IVIEW image (header type 049h)
        ld a, (hl)
        cp 1
        jr nz, lcfh02                   ; uncompressed or unknown compression
lcfh06: ld l, 15                        ; program file (header type 5)
        ld a, (hl)
        or a
        jr nz, lcfh02                   ; last byte of header must be zero
        ld l, 1
        ld a, (hl)                      ; return header type (5 or 049h)
        or a
        ret

getDataSize:
lgds01: ld a, (ix+4)
        push bc
        rst 030h
        defb 5
        or a
        jp nz, endCmd
        ld a, b
        pop bc
        ld e, d
        ld d, l
        ld l, h
        ld h, a
        djnz lgds01
        ret

writeByte:
        push bc
        push de
        ld e, (ix+14)
        ld d, (ix+15)
        res 7, d
        set 6, d
        push af
        in a, (0b1h)
        ld b, a
        ld a, (ix+28)
        out (0b1h), a
        pop af
        ld (de), a
        inc de
        push af
        ld a, b
        out (0b1h), a
        pop af
        ld (ix+14), e
        ld (ix+15), d
        pop de
        pop bc
        ret

decompressProgram:
        ld b, 110
        call getDataSize
        dec d
        ld (ix+7), 1                    ; have start addresses
        ld (ix+2), l                    ; compressed size LSB
        ld (ix+3), h                    ; compressed size MSB
        ld (ix+10), e                   ; uncompressed size LSB
        ld (ix+11), d                   ; uncompressed size MSB
        or l
        jp z, fileCRCError              ; zero size is invalid
        ld a, d
        or e
        jp z, fileCRCError
        push de
        call initializeDecompressor
        pop de
        xor a                           ; write file header
        call writeByte
        ld a, 5
        call writeByte
        ld a, e
        call writeByte
        ld a, d
        call writeByte
        xor a
        ld b, 12
ldp_01: call writeByte
        djnz ldp_01
        call decompressFileData
        jr verifyDataSize

decompressImageFile:
        call initializeDecompressor
        ld l, 10                        ; write file header
        ld h, (ix+1)
        ld (hl), 0
        ld l, 0
        ld b, 16
ldif01: ld a, (hl)
        inc hl
        call writeByte
        djnz ldif01
        call decompressImageData        ; decompress first block (palette data)
        ld (ix+12), 0                   ; reset read pointer

decompressImageData:
        ld b, 4
        call getDataSize
        ld (ix+7), 0                    ; no start addresses
        ld (ix+2), l                    ; compressed size LSB
        ld (ix+3), h                    ; compressed size MSB
        ld (ix+10), e                   ; uncompressed size LSB
        ld (ix+11), d                   ; uncompressed size MSB
        or l
        jp z, fileCRCError              ; zero size is invalid
        ld a, d
        or e
        jp z, fileCRCError
        call decompressFileData

verifyDataSize:
  if 0 == 1
        push de
        pop hl
        ld a, (ix+10)
        cp l
        jp nz, fileCRCError
        ld a, (ix+11)
        xor h
        and 03fh
        jp nz, fileCRCError
        ld a, (ix+10)
        add a, 0ffh
        ld a, (ix+11)
        adc a, 03fh
        rla
        rla
        rla
        and 7
        ld b, a
        push ix
        pop hl
        ld l, 0c0h
lvds01: ld a, (hl)
        cp 2
        jp c, fileCRCError
        inc l
        djnz lvds01
        ld a, (hl)
        cp 2
        jp nc, fileCRCError
  endif
        ret

initializeDecompressor:
        call freeMemory                 ; free all previously allocated memory
        xor a
        ld (ix+14), a                   ; output buffer position
        ld (ix+15), a
        ld a, (ix+0)                    ; input buffer position
        ld (ix+12), a
        ld a, (ix+1)
        ld (ix+13), a
        push ix
        pop bc
        ld (ix+16), 0c0h                ; segment table pointer
        ld (ix+17), b
        ret

decompressFileData:
        ld (ix+5), 080h                 ; initialize checksum
decompressFileData_:
        ld l, (ix+16)
        ld h, (ix+17)
        push hl
        pop iy                          ; IY = segment table pointer
        ld l, (ix+12)
        ld h, (ix+13)                   ; HL = read buffer position
        ld e, (ix+14)
        ld d, (ix+15)                   ; DE = write buffer position
        call decompressData
        defb 0ddh
        ld l, 0a4h
        jr nc, ldfd01
        ld a, 0f7h
        jp endCmd                       ; memory allocation error
ldfd01: ld (ix+12), l                   ; save read buffer position
        ld (ix+13), h
        ld (ix+14), e                   ; save write buffer position
        ld (ix+15), d
        ld a, d
        and 03fh
        or e
        jr nz, ldfd02
        inc iy
ldfd02: push iy
        pop hl
        ld (ix+16), l                   ; save segment table pointer
        ld (ix+17), h
        ld l, (ix+24)
        ld h, (ix+25)
        push hl
        pop iy
        ld a, (ix+5)                    ; verify checksum: must be 0FFh
        sub 0ffh
        jp nz, fileCRCError             ; checksum error
        ret

freeAllMemory:
        ld l, 0c0h
        defb 03ah                       ; = LD a, (nnnn)

freeMemory:
        ld l, 0c1h
        defb 0ddh
        ld a, h
        ld h, a
lfm_01: ld a, (hl)
        cp 2
        jr c, lfm_02
        ld c, a
        rst 030h
        defb 25
        xor a
        ld (hl), a
lfm_02: inc l
        jr nz, lfm_01
        dec l
        ld (hl), 1
        ret

closeFiles:
        ld a, (ix+4)
        or a
        jr z, lcf_01
        rst 030h
        defb 3
        ld (ix+4), 0
lcf_01: ld a, (ix+6)
        or a
        ret z
        rst 030h
        defb 4
        xor a
        ld (ix+6), a
        ret

openFileR:
        ld a, 1
lofr01: push af
        ld e, (ix+0)
        ld d, (ix+1)
        rst 030h
        defb 1
        or a
        jr z, lofr02
        cp 0f9h
        jp nz, endCmd                   ; error opening file ?
        pop af
        inc a
        cp 0ffh
        jr c, lofr01
        ld a, 0f9h                      ; could not find a free channel
        jp endCmd
lofr02: pop af                          ; file is successfully opened
        ld (ix+4), a
        ret

openFileW:
        ld a, 1
lofw01: push af
        ld e, (ix+0)
        ld d, (ix+1)
        rst 030h
        defb 2
        or a
        jr z, lofw02
        cp 0f9h
        jp nz, endCmd                   ; error opening file ?
        pop af
        inc a
        cp 0ffh
        jr c, lofw01
        ld a, 0f9h                      ; could not find a free channel
        jp endCmd
lofw02: pop af                          ; file is successfully opened
        ld (ix+6), a
        ret

readBlock:
        call setEXOSPaging
        push af
        push bc
        push de
        push hl
        ld d, h
        ld e, l
        push ix
        pop hl
        ld l, 0a4h+2                    ; check the number of bytes remaining:
        ld c, (hl)
        inc l
        ld b, (hl)
        inc l
        ld a, b
        or a
        jr nz, lrbl01                   ; >= 256 bytes ?
        or c
        jr z, fileReadError             ; no data is left to read: error
        jr lrbl02
lrbl01: and c
        inc a
        jr z, lrbl05                    ; if length = 0FFFFh: read all data
        ld bc, 00100h                   ; read at most 256 bytes
lrbl02: ld a, (hl)
        push bc
        rst 030h
        defb 6
        cp 0fbh
        jr z, fileReadError             ; channel does not exist: error
        ld a, b
        or c
        jr nz, fileReadError            ; not enough data was read: error
        pop bc
        ld l, 0a4h+2                    ; update bytes remaining
        ld a, (hl)
        sub c
        ld (hl), a
        inc l
        ld a, (hl)
        sbc a, b
        ld (hl), a
lrbl03: ld l, 0a4h+5                    ; update checksum
        ld a, (hl)
        pop hl
        push hl
        ld b, c
        ld c, 0ach
lrbl04: sub c
        rrca
        xor (hl)
        inc l
        djnz lrbl04
        push ix
        pop hl
        ld l, 0a4h+5
        ld (hl), a
        pop hl
        pop de
        pop bc
        pop af
        call setDecompressPaging
        ret
lrbl05: ld a, (hl)                      ; "infinite" input data length:
        ld bc, 00100h
        rst 030h
        defb 6
        cp 0fbh
        jr z, fileReadError             ; channel does not exist: error
        xor a                           ; calculate the number of bytes read
        sub c
        ld c, a
        ld a, 1
        sbc a, b
        ld b, a                         ; non-zero: update checksum and return,
        or c
        jr nz, lrbl03                   ; otherwise read error

fileReadError:
        ld a, 0e4h
        jp endCmd

fileCRCError:
        ld a, 0d0h
        jp endCmd

; -----------------------------------------------------------------------------

cmdNameLength   equ     10              ; length of "UNCOMPRESS"
cmdName:
versionMsg:
        defm "UNCOMPRESS version 1.0"
        defb 00dh, 00ah
        defb 000h
usageMsg:
        defm "UNCOMPRESS version 1.0"
        defb 00dh, 00ah
        defm "Usage:"
        defb 00dh, 00ah
        defm "  UNCOMPRESS <infile>"
        defb 00dh, 00ah
        defm "    uncompress and replace 'infile'"
        defb 00dh, 00ah
        defm "  UNCOMPRESS <infile> <outfile>"
        defb 00dh, 00ah
        defm "    uncompress 'infile' to 'outfile'"
        defb 00dh, 00ah
        defm "  UNCOMPRESS /L <infile>"
        defb 00dh, 00ah
        defm "    uncompress to 'UNCOMP__.TMP', and"
        defb 00dh, 00ah
        defm "    load the resulting temporary file"
        defb 00dh, 00ah
        defm "  UNCOMPRESS /L <infile> <outfile>"
        defb 00dh, 00ah
        defm "    uncompress 'infile' to 'outfile',"
        defb 00dh, 00ah
        defm "    and then load 'outfile'"
        defb 00dh, 00ah
        defm "  UNCOMPRESS /T <infile> [infile2...]"
        defb 00dh, 00ah
        defm "    test all input files"
        defb 00dh, 00ah
        defb 000h

testMsg1:
        defm ": "
        defb 000h

testMsg2:
        defm "OK"
        defb 00dh, 00ah, 000h

tmpFileName:
        defb 12
        defm "UNCOMP__.TMP"

errorTable:
        defb 000h

; =============================================================================

; border effects are disabled if this is set to any non-zero value
NO_BORDER_FX            equ     0

decompressData:
        push hl
        exx
        pop hl                          ; HL' = compressed data read address
        dec l
        exx
        ld hl, 00000h
        add hl, sp
        defb 0ddh
        ld l, decodeTableEnd+6
        ld sp, ix
        in a, (0b2h)                    ; save memory paging,
        push af
        in a, (0b1h)
        push af
        push hl                         ; and stack pointer
        ld sp, hl
        call setDecompressPaging
        call allocateSegment            ; get first output segment
        dec de
        exx
        ld e, 080h                      ; initialize shift register
        exx
        call read8Bits                  ; skip checksum byte
ldd_01: call decompressDataBlock        ; decompress all blocks
        jr z, ldd_01
        call setEXOSPaging
        inc de
        defb 0feh                       ; = CP nn

memoryError:
        xor a

decompressDone:
        ld c, a                         ; save error flag: 0: error, 1: success
        defb 0ddh
        ld l, decodeTableEnd
        ld sp, ix
        pop hl                          ; restore stack pointer,
        pop af                          ; memory paging,
        out (0b1h), a
        pop af
        out (0b2h), a
  if NO_BORDER_FX == 0
        xor a                           ; and border color
        out (081h), a
  endif
        ld sp, hl
        exx
        inc l
        push hl
        exx
        pop hl
        ld a, c                         ; on success: return A=0, Z=1, C=0
        sub 1                           ; on error: return A=0FFh, Z=0, C=1
        ret

writeBlock:
        inc d
        bit 6, d
        ret z
        inc iy

allocateSegment:
        set 7, d                        ; write decompressed data to page 2
        res 6, d
        push af
        push bc
        push de
        ld a, (iy+0)
        cp 1
        jr z, memoryError
        jr nc, las_01                   ; use pre-allocated segment ?
        call setEXOSPaging
        rst 030h                        ; no, allocate new segment
        defb 24
        or a
        jr nz, las_02
        call setDecompressPaging
        ld a, c
        ld (iy+0), a                    ; save segment number
las_01: out (0b2h), a
        pop de
        pop bc
        pop af
        ret
las_02: cp 0f5h                         ; memory allocation error:
        jr z, memoryError               ; no segment ?
        rst 030h                        ; shared segment, free it first
        defb 25
        jr memoryError

setEXOSPaging:
        di
        push af
        set 7, h
        exx
        set 7, h
        exx
        push hl
        ld a, 0ffh
        out (0b2h), a
        ld hl, 00000h
        add hl, sp
        set 7, h
        ld sp, hl
        ld a, (0bffch)
        out (0b0h), a
        defb 0ddh
        ld a, h
        or 080h
        defb 0ddh
        ld h, a
        defb 0fdh
        ld a, h
        or 080h
        defb 0fdh
        ld h, a
        pop hl
        pop af
        ei
        ret

setDecompressPaging:
        di
        push af
        push hl
        ld a, 0ffh
        out (0b0h), a
        ld hl, 00000h
        add hl, sp
        res 7, h
        ld sp, hl
        defb 0ddh
        ld a, h
        and 07fh
        defb 0ddh
        ld h, a
        defb 0fdh
        ld a, h
        and 07fh
        defb 0fdh
        ld h, a
        ld a, (iy+0)
        out (0b2h), a
        pop hl
        res 7, h
        exx
        res 7, h
        exx
        pop af
        ret

; -----------------------------------------------------------------------------

; A':  temp. register
; BC': symbols (literal byte or match code) remaining
; D':  prefix size for LZ77 matches with length >= 3 bytes
; E':  shift register
; HL': compressed data read address
; A:   temp. register
; BC:  temp. register (number of literal/LZ77 bytes to copy)
; DE:  decompressed data write address
; HL:  temp. register (literal/LZ77 data source address)
; IXH: decode table upper byte
; IY:  segment table pointer

nLengthSlots            equ 8
nOffs1Slots             equ 4
nOffs2Slots             equ 8
maxOffs3Slots           equ 32
totalSlots              equ nLengthSlots+nOffs1Slots+nOffs2Slots+maxOffs3Slots
; NOTE: the upper byte of the address of all table entries must be the same
slotBitsTable           equ 00000h
;slotBaseLowTable       equ slotBitsTable+totalSlots
;slotBaseHighTable      equ slotBaseLowTable+totalSlots
slotBitsTableL          equ slotBitsTable
;slotBaseLowTableL      equ slotBaseLowTable
;slotBaseHighTableL     equ slotBaseHighTable
slotBitsTableO1         equ slotBitsTableL+nLengthSlots
;slotBaseLowTableO1     equ slotBaseLowTableL+nLengthSlots
;slotBaseHighTableO1    equ slotBaseHighTableL+nLengthSlots
slotBitsTableO2         equ slotBitsTableO1+nOffs1Slots
;slotBaseLowTableO2     equ slotBaseLowTableO1+nOffs1Slots
;slotBaseHighTableO2    equ slotBaseHighTableO1+nOffs1Slots
slotBitsTableO3         equ slotBitsTableO2+nOffs2Slots
;slotBaseLowTableO3     equ slotBaseLowTableO2+nOffs2Slots
;slotBaseHighTableO3    equ slotBaseHighTableO2+nOffs2Slots
decodeTableEnd          equ slotBitsTable+(totalSlots*3)

decompressDataBlock:
        defb 0ddh
        ld l, 0a4h
        ld a, (ix+7)
        or a
        jr z, lddb15
        call read8Bits                  ; ignore start address
        call read8Bits
lddb15: call read8Bits                  ; read number of symbols - 1 (BC)
        ld c, a                         ; NOTE: MSB is in C, and LSB is in B
        call read8Bits
        ld b, a
        inc b
        inc c
        ld a, 040h
        call readBits                   ; read flag bits
        srl a
        push af                         ; save last block flag (A=1,Z=0)
        jr c, lddb01                    ; is compression enabled ?
        exx                             ; no, copy uncompressed literal data
        ld bc, 00101h
        jr lddb12
lddb01: push bc                         ; compression enabled:
        ld a, 040h
        call readBits                   ; get prefix size for length >= 3 bytes
        exx
        ld b, a
        inc b
        ld a, 002h
        ld d, 080h
lddb02: add a, a
        srl d                           ; D' = prefix size code for readBits
        djnz lddb02
        pop bc                          ; store the number of symbols in BC'
        exx
        add a, nLengthSlots+nOffs1Slots+nOffs2Slots-3
        ld c, a                         ; store total table size - 3 in C
        push de                         ; save decompressed data write address
        defb 0ddh                       ; initialize decode tables
        ld l, slotBitsTable&0ffh
lddb03: sbc hl, hl                      ; set initial base value (carry is 0)
lddb04: ld (ix+totalSlots), l           ; store base value LSB
        ld (ix+(totalSlots*2)), h       ; store base value MSB
        ld a, 010h
        call readBits
        ld (ix+0), a                    ; store the number of bits to read
        ex de, hl
        ld hl, 00001h                   ; calculate 1 << nBits
        jr z, lddb06
        ld b, a
lddb05: add hl, hl
        djnz lddb05
lddb06: add hl, de                      ; calculate new base value
        defb 0ddh
        inc l
        defb 0ddh
        ld a, l
        cp slotBitsTableO1&0ffh
        jr z, lddb03                    ; end of length decode table ?
        cp slotBitsTableO2&0ffh
        jr z, lddb03                    ; end of offset table for length=1 ?
        cp slotBitsTableO3&0ffh
        jr z, lddb03                    ; end of offset table for length=2 ?
        dec c
        jr nz, lddb04                   ; continue until all tables are read
        pop de                          ; DE = decompressed data write address
        exx
lddb07: sla e
        jp nz, lddb08
        inc l
        call z, readBlock
        ld e, (hl)
        rl e
lddb08: jr nc, lddb13                   ; literal byte ?
        ld a, 0f8h
lddb09: sla e
        jp nz, lddb10
        inc l
        call z, readBlock
        ld e, (hl)
        rl e
lddb10: jr nc, copyLZMatch              ; LZ77 match ?
        inc a
        jr nz, lddb09
        exx
        ld c, a
        call read8Bits                  ; get literal sequence length - 17
        add a, 16
        ld b, a
        rl c
        inc b                           ; B: (length - 1) LSB + 1
        inc c                           ; C: (length - 1) MSB + 1
lddb11: exx                             ; copy literal sequence
lddb12: inc l
        call z, readBlock
        ld a, (hl)
        exx
        inc e
        call z, writeBlock
        ld (de), a
        djnz lddb11
        dec c
        jr nz, lddb11
        jr lddb14
lddb13: inc l                           ; copy literal byte
        call z, readBlock
        ld a, (hl)
        exx
        inc e
        call z, writeBlock
        ld (de), a
lddb14: exx
        djnz lddb07
        dec c
        jr nz, lddb07
        exx
        pop af                          ; return with last block flag
        ret                             ; (A=1,Z=0 if last block)

copyLZMatch:
        exx
        add a, (slotBitsTableL+8)&0ffh
        defb 0ddh                       ; decode match length - 1
        ld l, a
        xor a
        ld h, a
        ld b, (ix+0)
        cp b
        jr z, lrev03
lrev01: exx
        sla e
        jp nz, lrev02
        inc l
        call z, readBlock
        ld e, (hl)
        rl e
lrev02: exx
        adc a, a
        rl h
        djnz lrev01
lrev03: add a, (ix+totalSlots)
        ld l, a                         ; L = (length - 1) LSB
        ld a, h
        adc a, (ix+(totalSlots*2))
        ld c, a                         ; C = (length - 1) MSB
        or l
        jr z, lclm02                    ; length == 1 byte ?
        dec a
        or c
        jr nz, lclm01                   ; length >= 3 bytes ?
        ld a, 020h                      ; length == 2 bytes, read 3 prefix bits
        ld b, slotBitsTableO2&0ffh
        jr lclm03
lclm01: exx                             ; length >= 3 bytes,
        ld a, d                         ; variable prefix size
        exx
        ld b, slotBitsTableO3&0ffh
        jr lclm03
lclm02: ld a, 040h                      ; length == 1 byte, read 2 prefix bits
        ld b, slotBitsTableO1&0ffh
lclm03: call readBits                   ; read offset prefix bits
        add a, b
        defb 0ddh                       ; decode match offset
        ld l, a
        xor a
        ld h, a
        ld b, (ix+0)
        cp b
        jr z, lrev13
lrev11: exx
        sla e
        jr z, lrev14
lrev12: exx
        adc a, a
        rl h
        djnz lrev11
lrev13: add a, (ix+totalSlots)
        ld b, a
        ld a, h
        adc a, (ix+(totalSlots*2))
        ld h, a
        ld a, e                         ; calculate LZ77 match read address
  if NO_BORDER_FX == 0
        out (081h), a
  endif
        sub b
        ld b, l                         ; length LSB
        ld l, a
        ld a, d
        sbc a, h
        ld h, a
        jr c, lclm15                    ; set up memory paging
        jp p, lclm13
        in a, (0b2h)
lclm04: res 7, h                        ; read from page 1
lclm05: set 6, h
lclm06: out (0b1h), a
        inc b                           ; B: (length - 1) LSB + 1
        inc c                           ; C: (length - 1) MSB + 1
lclm07: inc e                           ; copy match data
        jr z, lclm10
lclm08: ld a, (hl)
        ld (de), a
        inc l
        jr z, lclm11
lclm09: djnz lclm07
        dec c
        jp z, lddb14                    ; return to main decompress loop
        jr lclm07
lclm10: call writeBlock
        jr lclm08
lclm11: inc h
        jp p, lclm09
        ld h, 040h
        push iy
        in a, (0b1h)
lclm12: cp (iy+0)
        dec iy
        jr nz, lclm12
        ld a, (iy+2)
        out (0b1h), a                   ; read next segment
        pop iy
        jr lclm09
lclm13: add a, a
        jp p, lclm14
        ld a, (iy-1)
        jr lclm06
lclm14: ld a, (iy-2)
        jr lclm05
lclm15: add a, a
        jp p, lclm16
        ld a, (iy-3)
        jr lclm04
lclm16: ld a, (iy-4)
        jr lclm04
lrev14: inc l
        call z, readBlock
        ld e, (hl)
        rl e
        jp lrev12

read8Bits:
        ld a, 001h

readBits:
        exx
lrb_01: sla e
        jr z, lrb_03
lrb_02: adc a, a
        jp nc, lrb_01
        exx
        ret
lrb_03: inc l
        call z, readBlock
        ld e, (hl)
        rl e
        jp lrb_02

codeEnd:

