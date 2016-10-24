
BUILD_EXTENSION_ROM     equ     1

    if BUILD_EXTENSION_ROM == 0
        output  "uncompress.ext"
        org     0bffah
        defw    0600h, codeEnd - codeBegin, 0, 0, 0, 0, 0, 0
    else
        output  "uncompress.rom"
        org     0c000h
        defm    "EXOS_ROM"
        defw    0000h
    endif

    macro exos n
        rst   30h
        defb  n
    endm

codeBegin:
main:
        ld    a, 2
        cp    c
        jr    z, .l2
        inc   a
        cp    c
        jr    z, helpString
        ld    a, 5
        cp    c
        jr    z, errorString
.l1:    xor   a
        ret
.l2:    call  checkCommandName
        jr    nz, .l1
        jr    parseCommand

helpString:
        xor   a
        cp    b
        jr    z, .l1
        call  checkCommandName
        jr    z, .l2
        xor   a
        ret
.l1:    ld    hl, versionMsg
        jr    printString
.l2:    ld    c, a
        ld    hl, usageMsg

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

checkCommandName:
        ld    a, cmdNameLength
        cp    b
        ret   nz
        push  bc
        push  de
        push  hl
        ld    hl, cmdName
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

errorString:
        ld    hl, errorTable
.l1:    ld    a, (hl)
        or    a
        ret   z
        inc   hl
        inc   hl
        inc   hl
        cp    b
        jr    nz, .l1
        dec   hl
        ld    d, (hl)
        dec   hl
        ld    e, (hl)
        in    a, (0b3h)
        ld    b, a
        xor   a
        ld    c, a
        ret

; -----------------------------------------------------------------------------

inFileReadBuffer        equ     4000h
inFileReadBufferP0      equ     inFileReadBuffer - 4000h
inFileReadBufSize       equ     1000h
archiveIndexBuf         equ     5000h
archiveIndexBufP0       equ     archiveIndexBuf - 4000h
archiveMaxFiles         equ     (6000h - archiveIndexBuf) / 32
decompReadBuffer        equ     6000h
decompReadBufferP0      equ     decompReadBuffer - 4000h
inputFileName           equ     6400h
outputFileName          equ     6500h
decompTablesBase        equ     6600h
decompTablesBaseP0      equ     decompTablesBase - 4000h
variablesBase           equ     66d4h
variablesBaseP0         equ     variablesBase - 4000h
decompStack             equ     6700h
decompStackTop          equ     decompStack + 0100h
decompStackTopP0        equ     decompStackTop - 4000h

; IX +  0:  input file bytes remaining
; IX +  2:  input file channel
; IX +  3:  compressed data checksum
; IX +  4:  output file channel
; IX +  5:  non-zero: archive flag (/A)
; IX +  6:  non-zero: load flag (/L)
; IX +  7:  non-zero: test flag (/T)
; IX +  8:  data block uncompressed size (for IVIEW and EXOS 5 format)
; IX + 10:  decompressor output buffer position (DE)
; IX + 12:  decompressor segment table pointer (IY)
; IX + 14:  saved IY register
; IX + 16:  saved stack pointer
; IX + 18:  saved page 1 segment
; IX + 19:  allocated page 1 segment
; IX + 20:  saved decompressor page 1 segment
; IX + 21:  non-zero if the compressed data includes start addresses
; IX + 22:  input buffer position
; IX + 24:  input buffer bytes remaining
; IX + 26:  current archive file index
; IX + 27:  number of files in archive
; IX + 28:  current archive file bytes remaining (32 bit value)
; IX + 32:  segment table

    macro defineVariable n, m
n       equ     variablesBase + m
@.p0    equ     variablesBaseP0 + m
@.offs  equ     m
    endm

        defineVariable  inFileDataRemaining, 0
        defineVariable  inFileChannel, 2
        defineVariable  crcValue, 3
        defineVariable  outFileChannel, 4
        defineVariable  archiveFlag, 5
        defineVariable  loadFlag, 6
        defineVariable  testFlag, 7
        defineVariable  uncompDataSize, 8
        defineVariable  decompOutBufPos, 10
        defineVariable  decompSegTablePos, 12
        defineVariable  savedIYRegister, 14
        defineVariable  savedStackPointer, 16
        defineVariable  savedPage1Segment, 18
        defineVariable  page1Segment, 19
        defineVariable  decompPage1Segment, 20
        defineVariable  haveStartAddress, 21
        defineVariable  inFileBufPos, 22
        defineVariable  inFileBufBytesLeft, 24
        defineVariable  archiveFileIndex, 26
        defineVariable  archiveFileCnt, 27
        defineVariable  outFileBytesLeft, 28
        defineVariable  segmentTable, 32

parseCommand:
        push  de
        call  allocateMemory
        pop   de
        ld    a, 0f7h                   ; .NORAM
        ld    c, 0
        ret   nz
        ld    (savedStackPointer), sp   ; save stack pointer
        ld    (savedIYRegister), iy     ; save IY register
        ld    a, (de)
        ld    b, a
        inc   de
        ld    hl, decompReadBuffer
        call  getNextFileName           ; skip command name
        push  bc
        push  de
        ld    hl, decompReadBuffer
        call  getNextFileName           ; check flags
        ld    c, a
        inc   hl
        ld    a, (hl)
        cp    '/'
        jr    nz, .l4
        dec   c
        jp    z, invalidParamError
        inc   c
        push  de
.l1:    dec   c
        jr    z, .l3
        inc   hl
        ld    a, (hl)
        and   0dfh
        ld    de, archiveFlag
        cp    'A'
        jr    z, .l2
        ld    e, low loadFlag
        cp    'L'
        jr    z, .l2
        ld    e, low testFlag
        cp    'T'
        jp    nz, invalidParamError
.l2:    ld    a, 1
        ld    (de), a
        jr    .l1
.l3:    pop   de
        pop   hl
        pop   hl
        ld    a, (ix + archiveFlag.offs)
        or    (ix + testFlag.offs)
        and   (ix + loadFlag.offs)
        jp    nz, invalidParamError     ; /L cannot be combined with /A or /T
        defb  021h                      ; = LD HL, nnnn
.l4:    pop   de
        pop   bc
        call  checkCmdArgs              ; get first file name (required)
        jp    z, wrongNumParamsError
.l5:    ld    hl, inputFileName
        call  getNextFileName
        bit   0, (ix + testFlag.offs)
        jr    z, .l6
        push  bc                        ; test mode:
        push  de
        inc   hl
        call  printString
        ld    hl, testMsg1
        call  printString
        call  openInputFile
        call  decompressFile
        jr    nz, .l9
        call  closeFiles
        ld    hl, testMsg2
        call  printString
        pop   de
        pop   bc
        call  checkCmdArgs
        jr    nz, .l5
        jr    .l10
.l6:    call  checkCmdArgs              ; extract mode:
        ld    a, (ix + archiveFlag.offs)
        rrca
        push  af
        jr    nz, .l7                   ; is the output file name specified ?
        jr    c, .l8                    ; input file is an archive ?
        bit   0, (ix + loadFlag.offs)   ; load the extracted file ?
        jr    z, wrongNumParamsError    ; no, error: output name is required
        ld    hl, tmpFileName           ; yes, use temporary file name
        ld    de, outputFileName        ; copy the file name to the buffer
        call  copyString
        jr    .l8
.l7:    jr    c, wrongNumParamsError    ; archive: output name is not allowed
        ld    hl, outputFileName        ; use the specified output file name
        call  getNextFileName
        call  checkCmdArgs
        jr    nz, wrongNumParamsError   ; too many file names: error
.l8:    call  openInputFile
        pop   af                        ; carry is set if extracting archive
        call  nc, openOutputFile
        call  decompressFile
.l9:    jr    nz, endCmd
        call  writeDecompressedData     ; write output file
        call  closeFiles
        xor   a
        bit   0, (ix + loadFlag.offs)   ; if load not requested, then done
.l10:   jr    z, endCmd
        ld    hl, outputFileName
        ld    de, inputFileName
        call  copyString
        call  openInputFile             ; load module:
        ld    a, (inFileChannel)
        push  af
        call  freeAllMemory             ; clean up first
        pop   af
        ld    bc, 010000h - 16          ; allocate 16 bytes for the header
        add   hl, bc
        ld    sp, hl                    ; restore stack pointer
        ld    b, a
.l11:   ld    d, h                      ; read next module
        ld    e, l
        push  bc
        exos  29
        pop   bc
        or    a
        jr    z, .l11
        ld    hl, 16                    ; done loading module, clean up
        add   hl, sp
        ld    sp, hl
        push  af
        ld    a, b
        exos  3
        pop   af
        ld    c, 0
        cp    0ech                      ; check status: .NOMOD ?
        jr    nz, .l12
        xor   a
.l12:   or    a
        ret

wrongNumParamsError:
        ld    a, 09fh                   ; "wrong number of parameters"
        defb  021h                      ; = LD HL, nnnn
nameTooLongError:
        ld    a, 0a8h                   ; "invalid pathname string"
        defb  021h                      ; = LD HL, nnnn
invalidParamError:
        ld    a, 0aeh                   ; "invalid parameter"
        defb  021h                      ; = LD HL, nnnn
fileDataError:
        ld    a, 0b8h                   ; "data error"
        defb  021h                      ; = LD HL, nnnn
fileReadError:
        ld    a, 0e4h                   ; .EOF
        defb  021h                      ; = LD HL, nnnn
noChannelError:
        ld    a, 0f9h                   ; .CHANX

endCmd:
        ld    ix, variablesBase
        push  af
        call  closeFiles
        call  freeAllMemory
        pop   af
        ld    sp, hl                    ; restore stack pointer
        ld    c, 0
        or    a
        ret

checkCmdArgs:
        ld    a, b
        or    a
        ret   z
.l1:    ld    a, (de)
        or    a
        ret   z
        cp    021h
        jr    nc, .l2
        inc   de
        djnz  .l1
        xor   a
.l2:    or    a
        ret

; B:  command line bytes remaining
; DE: command line pointer
; HL: address of buffer to store file name (L should be zero)

getNextFileName:
        inc   b
.l1:    dec   b
        jr    z, wrongNumParamsError
        ld    a, (de)                   ; skip leading white space
        or    a
        jr    z, wrongNumParamsError
        cp    021h
        jr    nc, .l3
        inc   de
        jr    .l1
.l2:    ld    a, (de)
        cp    021h
        jr    c, .l4
.l3:    inc   hl
        ld    (hl), a
        inc   de
        djnz  .l2
.l4:    ld    a, l
        inc   hl
        ld    (hl), 0
        ld    l, 0
        ld    (hl), a
        ret

copyString:
        xor   a
        ld    c, (hl)
        ld    b, a
        inc   bc
        ldir
        ld    (de), a
        ret

writeDecompressedData:
        push  ix
        pop   hl
        ld    l, (ix + decompSegTablePos.offs)
        ld    bc, (decompOutBufPos)
        dec   bc
        res   7, b
        res   6, b
        inc   bc
        ld    a, (hl)
        call  writeSegment
        ret   z
        jr    endCmd

; input arguments:
;   A:  segment number
;   BC: number of bytes to write
;   DE: pointer to buffer on page 1
; output arguments:
;   A:  segment number on success (Z = 1), or error code (Z = 0)
;   BC: zero on success
;   DE: updated buffer pointer

writeOutputFile:
        res   7, d
        set   6, d
        bit   0, (ix + testFlag.offs)
        jr    nz, .l2
        push  hl                        ; if not test mode, write output file
        ld    l, a
        in    a, (0b1h)
        ld    h, a
        ld    a, l
        ld    l, (ix + outFileChannel.offs)
        out   (0b1h), a
        ld    a, l
        exos  8
        or    a
        jr    nz, .l1
        ld    a, b
        or    c
        ld    a, 0ach                   ; "disk full"
        jr    nz, .l1
        in    a, (0b1h)
.l1:    ld    l, a
        ld    a, h
        out   (0b1h), a
        ld    a, l
        pop   hl
        ret
.l2:    ex    de, hl                    ; test mode: update buffer pointer,
        add   hl, bc                    ; but do not write output file
        ex    de, hl
        ld    c, a
        xor   a
        ld    b, a
        ld    a, c
        ld    c, b
        ret

writeSegment:
        bit   0, (ix + archiveFlag.offs)
        jr    nz, extractArchive
        ld    de, 04000h
        call  writeOutputFile
        ret   nz
        xor   a
        ret

extractArchive:
        push  de
        push  hl
        ld    de, 04000h
        ld    l, (ix + archiveFileCnt.offs)
        dec   l
        inc   l
        jr    nz, .l1
        call  parseArchiveIndex         ; load archive index if not done yet
        jr    nz, .l3
        ld    l, (ix + archiveFileCnt.offs)
        ld    h, 0
        add   hl, hl
        add   hl, hl
        add   hl, hl
        inc   hl
        add   hl, hl
        add   hl, hl                    ; HL = archive index size
        ld    d, h                      ; update buffer pointer (DE),
        ld    e, l
        ld    h, b
        ld    l, c
        sbc   hl, de
        ld    b, h                      ; and buffer bytes remaining (BC)
        ld    c, l
        set   6, d
.l1:    push  af                        ; save segment number
        ld    a, b
        or    c
        jr    nz, .l4                   ; more data remaining ?
        defb  021h                      ; = LD HL, nnnn
.l2:    pop   hl
        pop   hl
        pop   hl
.l3:    pop   hl
        pop   de
        or    a
        ret
.l4:    ld    a, (outFileBytesLeft + 2)
        ld    hl, (outFileBytesLeft)
        or    h
        or    l
        jr    nz, .l8
        push  bc                        ; finished writing output file,
        push  de
        call  closeOutputFile           ; close it,
        ld    a, (ix + archiveFileIndex.offs)
        ld    l, a                      ; and open next one
        cp    (ix + archiveFileCnt.offs)
        ld    a, 0b8h                   ; "data error"
        jr    nc, .l2
        inc   (ix + archiveFileIndex.offs)
        ld    h, 0
        add   hl, hl
        add   hl, hl
        add   hl, hl
        add   hl, hl
        add   hl, hl
        ld    a, h
        add   a, high archiveIndexBuf
        ld    h, a
        ld    de, outFileBytesLeft + 3  ; copy file size
        ld    b, 3
.l5:    inc   hl
        dec   de
        ld    a, (hl)
        ld    (de), a
        djnz  .l5
        ld    de, outputFileName        ; copy file name
        push  de
        ld    b, 28
.l6:    inc   hl
        inc   de
        ld    a, (hl)
        ld    (de), a
        or    a
        jr    z, .l7
        djnz  .l6
        inc   de
        xor   a
        ld    (de), a
.l7:    pop   hl
        ld    (hl), 020h
        call  printString
        dec   e
        ld    (hl), e
        ld    hl, newLineMsg
        call  printString
        call  openOutputFile
        pop   de
        pop   bc
.l8:    ld    hl, (outFileBytesLeft)
        or    a
        sbc   hl, bc
        ld    a, (outFileBytesLeft + 2)
        sbc   a, 0
        jr    c, .l10
        ld    (outFileBytesLeft), hl
        ld    (outFileBytesLeft + 2), a
        pop   af
        call  writeOutputFile
.l9:    jr    z, .l1
        jr    .l3
.l10:   add   hl, bc
        ld    a, c
        sub   l
        ld    c, a
        ld    a, b
        sbc   a, h
        ld    b, a
        pop   af
        push  bc
        ld    b, h
        ld    c, l
        ld    hl, 0
        ld    (outFileBytesLeft), hl
        call  writeOutputFile
        pop   bc
        jr    .l9

parseArchiveIndex:
        ld    hl, 0
        ld    (ix + archiveFileIndex.offs), l
        ld    (ix + archiveFileCnt.offs), l
        ld    (outFileBytesLeft), hl
        ld    (outFileBytesLeft + 2), hl
        ld    e, a
        ld    d, h
        ld    a, c                      ; data size must be >= 36 bytes
        cp    36
        ld    a, b
        sbc   a, d
        push  bc
        push  de
        jp    c, .l6
        di
        ld    a, e
        out   (0b2h), a
        ld    h, 080h                   ; HL = 8000h
        ld    a, (hl)                   ; check the number of files:
        inc   hl
        or    (hl)
        inc   hl
        or    (hl)
        jr    nz, .l5
        inc   hl
        or    (hl)
        jr    z, .l5                    ; must be in the range 1 to 128
        cp    archiveMaxFiles + 1
        jr    nc, .l5
        ld    (ix + archiveFileCnt.offs), a
        ld    h, d                      ; check data size (D = 0)
        ld    l, a
        add   hl, hl
        add   hl, hl
        add   hl, hl
        add   hl, hl
        add   hl, hl
        ld    a, l
        or    3
        cp    c
        ld    a, h
        sbc   a, b
        jr    nc, .l5
        ld    b, h                      ; copy archive index
        ld    c, l
        ld    hl, 08004h
        ld    de, archiveIndexBuf
        ldir
        ld    a, 0ffh
        out   (0b2h), a
        ei
        ld    hl, archiveIndexBuf
        ld    de, 01f00h + '.'
        ld    b, (ix + archiveFileCnt.offs)
.l1:    ld    a, (hl)                   ; check file sizes:
        or    a
        jr    nz, .l6                   ; must be < 16M
        set   2, l
        ld    a, (hl)                   ; check file names:
        or    a
        jr    z, .l6                    ; empty name is invalid
        cp    e
        jr    z, .l6
.l2:    ld    a, (hl)
        or    a
        jr    z, .l4
        cp    e
        jr    z, .l3
        cp    '+'
        jr    z, .l3
        cp    '-'
        jr    z, .l3
        cp    '_'
        jr    z, .l3
        cp    '0'
        jr    c, .l6
        cp    '9' + 1
        jr    c, .l3
        xor   080h
        cp    0c0h
        jr    c, .l6
        and   05fh                      ; convert to upper case
        cp    'A'
        jr    c, .l6
        cp    'Z' + 1
        jr    nc, .l6
        ld    (hl), a
.l3:    inc   hl
        ld    a, l
        and   d
        jr    nz, .l2
        dec   hl
.l4:    ld    a, l
        or    d
        ld    l, a
        inc   hl
        djnz  .l1
        pop   de
        pop   bc
        xor   a                         ; on success, return with Z=1,
        ld    a, e                      ; but the segment number in A
        ret
.l5:    ld    a, 0ffh
        out   (0b2h), a
        ei
.l6:    pop   de
        pop   bc
        ld    a, 0b8h                   ; "data error"
        or    a
        ret

; BC: number of bytes (<= 1024)
; HL: buffer address

updateChecksum:
        di
        ld    e, c
        ld    d, b
        ld    a, c
        srl   b
        rra
        srl   b
        rra
        ld    b, a
        ld    c, 0ach
        ld    a, (crcValue)
        bit   1, e
        jr    z, .l1
        sub   c
        rrca
        xor   (hl)
        inc   hl
        sub   c
        rrca
        xor   (hl)
        inc   hl
.l1:    bit   0, e
        jr    z, .l2
        sub   c
        rrca
        xor   (hl)
        inc   hl
.l2:    ld    e, a
        ld    a, b
        or    d
        ld    a, e
        jr    z, .l4
        ex    de, hl
        ld    hl, 0
        add   hl, sp
        ex    de, hl                    ; DE = saved stack pointer
        ld    sp, hl
.l3:    pop   hl
        sub   c
        rrca
        xor   l
        sub   c
        rrca
        xor   h
        pop   hl
        sub   c
        rrca
        xor   l
        sub   c
        rrca
        xor   h
        djnz  .l3
        ld    l, b
        ld    h, b
        add   hl, sp
        ex    de, hl
        ld    sp, hl
        ex    de, hl
.l4:    ld    (crcValue), a
        ei
        ret

readInFileNoSplit:
        defb  0f6h                      ; = OR nn

readInputFile:
        xor   a
        push  af
        push  bc
        push  hl
        ld    hl, inFileReadBuffer
        push  hl
        ld    (inFileBufPos), hl
        ld    bc, inFileReadBufSize
        ld    a, (inFileChannel)
        ex    de, hl
        exos  6
        ex    de, hl
        cp    0e4h
        jr    z, .l1                    ; .EOF ?
        or    a
        jp    nz, endCmd                ; error reading file
.l1:    pop   bc                        ; BC = inFileReadBuffer
        sbc   hl, bc                    ; carry is always 0 here
        ld    (inFileBufBytesLeft), hl
        pop   hl
        pop   bc
        jr    z, .l2
        pop   af
        xor   a
        ret
.l2:    pop   af
        jp    nz, fileReadError         ; splitting input file is not allowed ?
        call  getNextVolume
        jr    readInFileNoSplit

; A = byte read (S, Z flags are set according to this byte)

readInputFileByte:
        push  hl
        ld    hl, (inFileBufPos)
        ld    a, l
        cp    low (inFileReadBuffer + inFileReadBufSize)
        ld    a, h
        sbc   a, high (inFileReadBuffer + inFileReadBufSize)
        call  nc, readInputFile
        ld    hl, (inFileBufBytesLeft)
        ld    a, h
        or    l
        jp    z, fileReadError
        dec   hl
        ld    (inFileBufBytesLeft), hl
        ld    hl, (inFileBufPos)
        ld    a, (hl)
        inc   hl
        ld    (inFileBufPos), hl
        pop   hl
        or    a
        ret

; BC: bytes remaining
; DE: buffer pointer
; A = status (0 for success, or E4h for EOF; S, Z flags are updated)

readInputFileBlock:
        push  hl
.l1:    ld    hl, (inFileBufBytesLeft)
        or    a
        sbc   hl, bc
        jr    c, .l2                    ; there is not enough data in buffer ?
        ld    (inFileBufBytesLeft), hl
        ld    hl, (inFileBufPos)
        call  copyBlock
        ld    (inFileBufPos), hl
        pop   hl
        xor   a
        ret
.l2:    push  bc
        or    a
        adc   hl, bc
        jr    z, .l3                    ; buffer is empty ?
        ld    b, h                      ; copy remaining data from the buffer
        ld    c, l
        pop   hl                        ; HL = number of bytes requested
        or    a                         ; BC = number of bytes in the buffer
        sbc   hl, bc
        push  hl
        ld    hl, (inFileBufPos)
        call  copyBlock
        ld    (inFileBufPos), hl
        ld    (inFileBufBytesLeft), bc  ; BC = 0
.l3:    ld    hl, (inFileBufPos)
        ld    bc, inFileReadBuffer + inFileReadBufSize
        or    a
        sbc   hl, bc
        pop   bc
        jr    c, .l4                    ; end of file ?
        call  readInputFile             ; no, fill buffer and continue
        jr    .l1
.l4:    pop   hl
        ld    a, 0e4h                   ; .EOF
        or    a
        ret

copyBlock:
.l1:    ld    a, c
        and   00fh
        jr    z, .l2
        ldi
        jp    pe, .l1
        ret
.l2:    ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        jp    pe, .l2
        ret

getNextVolume:
        push  bc
        push  de
        push  hl
        ld    hl, inputFileName
        ld    c, (hl)
        ld    b, 0
        add   hl, bc                    ; HL = address of last character
        ld    d, h
        ld    e, l
        inc   de                        ; DE = address of last character + 1
        ld    b, 4
        inc   c
.l1:    dec   c                         ; replace original extension,
        jr    z, .l2
        ld    a, (hl)
        cp    ':'
        jr    z, .l2
        cp    '\\'
        jr    z, .l2
        cp    '.'
        jr    z, .l3
        dec   hl
        djnz  .l1
.l2:    ld    h, d                      ; or append new one
        ld    l, e
.l3:    ld    bc, inputFileName + 252   ; check name length:
        or    a
        sbc   hl, bc
        add   hl, bc
        jp    nc, nameTooLongError
        ld    a, '.'
        ld    b, 4
        cp    (hl)
        ld    (hl), a
        jr    nz, .l12                  ; if appending extension
        dec   b
.l4:    inc   hl                        ; check if the existing extension
        ld    a, (hl)                   ; is already in 3-digit numeric format
        cp    '0'
        jr    c, .l12
        cp    '9' + 1
        jr    nc, .l12
        djnz  .l4
.l5:    ld    a, l
        sub   low inputFileName
        ld    (inputFileName), a        ; store name length
        inc   hl
        ld    (hl), 0
        ld    b, 3
.l6:    dec   hl                        ; increment suffix
        ld    a, (hl)
        cp    '9'
        jr    c, .l7
        ld    (hl), '0'
        djnz  .l6
        defb  0feh                      ; = CP nn
.l7:    inc   (hl)
        ld    hl, inputFileName + 1
        call  printString
        ld    hl, nextVolumeMsg
        call  printString
        ld    hl, 00000h
        ld    d, h
        ld    e, l
        ld    bc, 0bff3h                ; PORTB5
        di
.l8:    ld    a, (bc)
        and   0f0h
        or    007h
        out   (0b5h), a
        in    a, (0b5h)
        rrca
        rl    h                         ; H: STOP key state
        rlca
        rlca
        rlca
        rl    l                         ; L: ENTER key state
        ld    a, (bc)
        out   (0b5h), a
        ld    a, 0feh
        cp    h
        jr    nz, .l9
        ld    d, a                      ; STOP key is pressed
.l9:    cp    l
        jr    nz, .l10
        ld    e, a                      ; ENTER key is pressed
.l10:   ld    a, d
        or    e                         ; wait until one key is pressed,
        inc   a
        and   h
        and   l                         ; and then both are released
        inc   a
        jr    nz, .l8
        ei
        cp    d
        ld    a, 0e5h                   ; .STOP
        jp    nz, endCmd
        call  closeInputFile
        call  openInputFile
        pop   hl
        pop   de
        pop   bc
        xor   a
        ret
.l11:   inc   hl
.l12:   djnz  .l11
        ld    a, '0'
        ld    (hl), a
        dec   hl
        ld    (hl), a
        dec   hl
        ld    (hl), a
        inc   hl
        inc   hl
        jr    .l5

getDataSize:
.l1:    call  readInputFileByte
        ld    e, d
        ld    d, l
        ld    l, h
        ld    h, a
        djnz  .l1
        ret

writeByte:
        push  af
        push  bc
        push  hl
        ld    hl, (decompOutBufPos)
        res   7, h
        set   6, h
        ld    c, a
        in    a, (0b1h)
        ld    b, a
        ld    a, (segmentTable)
        out   (0b1h), a
        ld    (hl), c
        inc   hl
        ld    a, b
        out   (0b1h), a
        ld    (decompOutBufPos), hl
        pop   hl
        pop   bc
        pop   af
        ret

checkFileHeader:
        call  readInFileNoSplit         ; fill input buffer
        ld    hl, decompReadBuffer      ; read header
        ld    d, h
        ld    e, l
        ld    bc, 16
        call  readInputFileBlock
        jr    z, .l1
        ld    a, e                      ; EOF: assume raw format
        cp    5
        jr    nc, .l2                   ; length must be at least 5 bytes
        jp    fileReadError
.l1:    bit   0, (ix + archiveFlag.offs)
        jr    nz, .l2                   ; /A flag implies raw compressed format
        ld    a, (hl)                   ; first byte:
        or    a
        jr    nz, .l2                   ; if non-zero, then raw file
        inc   hl
        ld    a, (hl)                   ; second byte:
        cp    5
        jr    z, .l5                    ; program file ?
        cp    6
        jr    z, .l5                    ; extension module ?
        cp    049h
        jr    z, .l4                    ; IVIEW image ?
.l2:    ld    hl, decompReadBuffer      ; raw compressed data:
        ld    a, e
        cp    16
        jr    c, .l3
        ld    bc, 1024 - 16             ; fill decompressor input buffer
        call  readInputFileBlock
.l3:    ld    (ix + crcValue.offs), 080h  ; initialize checksum
        ex    de, hl
        or    a
        sbc   hl, de
        ld    b, h
        ld    c, l
        ex    de, hl
        call  updateChecksum
        xor   a                         ; file type = 0: raw compressed data
        ret
.l4:    ld    l, 10                     ; IVIEW image (header type 049h)
        ld    a, (hl)
        cp    1
        jr    nz, .l2                   ; uncompressed or unknown compression
.l5:    ld    l, 15                     ; program file (header type 5 or 6)
        ld    a, (hl)
        or    a
        jr    nz, .l2                   ; last byte of header must be zero
        ld    l, 1
        ld    a, (hl)                   ; A = header type (5, 6, or 049h)
        or    a
        ret

decompressFile:
        call  checkFileHeader
        cp    5
        jr    z, decompressProgram
        cp    6
        jr    z, decompressExtension
        cp    049h
        jp    z, decompressImageFile
        call  initializeDecompressor            ; raw compressed data:
        ld    hl, 0ffffh
        ld    (inFileDataRemaining), hl         ; "infinite" input data size
        ld    (ix + haveStartAddress.offs), 0   ; no start addresses
        jp    decompressFileData_

decompressProgram:
        ld    b, 110
        call  getDataSize
        dec   d
        ld    (ix + haveStartAddress.offs), 1   ; have start addresses
        ld    (inFileDataRemaining), hl ; compressed size
        ld    (uncompDataSize), de      ; uncompressed size
        ld    a, h
        or    l
        jp    z, fileDataError          ; zero size is invalid
        ld    a, d
        or    e
        jp    z, fileDataError
        call  initializeDecompressor
        xor   a                         ; write file header
        call  writeByte
        ld    a, 5
        call  writeByte
        ld    a, e
        call  writeByte
        ld    a, d
        call  writeByte
        xor   a
        ld    b, 12
.l1:    call  writeByte
        djnz  .l1
        jp    decompressFileData

decompressExtension:
        ld    b, 42
        call  getDataSize
        ld    (ix + haveStartAddress.offs), 0   ; no start addresses
        ld    (inFileDataRemaining), hl ; compressed size
        ld    a, h
        or    l
        jp    z, fileDataError          ; zero size is invalid
        call  initializeDecompressor
        xor   a                         ; write file header
        call  writeByte
        ld    a, 6
        call  writeByte
        ld    a, 0f6h                   ; uncompressed size is not known yet
        call  writeByte
        ld    a, 03fh
        call  writeByte
        xor   a
        ld    b, 12
.l1:    call  writeByte
        djnz  .l1
        call  decompressFileData
        ret   nz
        ld    hl, (decompOutBufPos)     ; store uncompressed size
        res   7, h
        res   6, h
        ld    bc, 16
        or    a
        sbc   hl, bc
        in    a, (0b1h)
        ld    b, a
        ld    a, (segmentTable)
        out   (0b1h), a
        ld    (04002h), hl
        ld    a, b
        out   (0b1h), a
        xor   a
        ret

decompressImageFile:
        call  initializeDecompressor
        ld    hl, decompReadBuffer + 10 ; write file header
        ld    (hl), 0
        ld    l, low decompReadBuffer
        ld    b, 16
.l1:    ld    a, (hl)
        inc   hl
        call  writeByte
        djnz  .l1
        call  decompressImageData       ; decompress first block (palette data)

decompressImageData:
        ld    b, 4
        call  getDataSize
        ld    (ix + haveStartAddress.offs), 0   ; no start addresses
        ld    (inFileDataRemaining), hl ; compressed size
        ld    (uncompDataSize), de      ; uncompressed size
        ld    a, h
        or    l
        jp    z, fileDataError          ; zero size is invalid
        ld    a, d
        or    e
        jp    z, fileDataError
        jp    decompressFileData

initializeDecompressor:
        push  hl
        ld    hl, 0
        ld    (decompOutBufPos), hl     ; output buffer position
        ld    (ix + archiveFileIndex.offs), l
        ld    (ix + archiveFileCnt.offs), l
        ld    (outFileBytesLeft), hl
        ld    (outFileBytesLeft + 2), hl
        ld    hl, segmentTable
        ld    (decompSegTablePos), hl   ; segment table pointer
        pop   hl
        ret

decompressFileData:
        ld    (ix + crcValue.offs), 080h  ; initialize checksum
        call  readCompressedData        ; fill decompressor input buffer

decompressFileData_:
        ld    iy, (decompSegTablePos)   ; IY = segment table pointer
        ld    hl, decompReadBuffer      ; HL = read buffer position
        ld    de, (decompOutBufPos)     ; DE = write buffer position
        call  decompressData
        ld    ixl, low variablesBase
        jp    c, endCmd                 ; if error decompressing data
        ld    (decompOutBufPos), de     ; save write buffer position
        ld    a, d
        and   03fh
        or    e
        jr    nz, .l1
        inc   iy
.l1:    ld    (decompSegTablePos), iy   ; save segment table pointer
        ld    iy, (savedIYRegister)
        ld    a, (crcValue)             ; verify checksum: must be 0FFh
        sub   0ffh
        jp    nz, fileDataError         ; checksum error
        ret

allocateMemory:
        ld    h, 5
        defb  0feh                      ; = CP nn
.l1:    push  hl
        exos  24
        ld    l, c
        or    a
        jr    nz, .l4                   ; error allocating segment ?
        dec   h
        jr    nz, .l1
.l2:    in    a, (0b1h)
        ld    c, a                      ; C = original page 1 segment
        ld    a, l
        out   (0b1h), a
        ld    b, a                      ; B = allocated page 1 segment
        push  bc
        ld    hl, 0                     ; clear all variables to zero
        ld    (inputFileName), hl
        ld    (outputFileName), hl
        ld    h, high variablesBase
        ld    (hl), l
        ld    de, (variablesBase & 0ff00h) + 1
        ld    bc, 255
        ldir
        pop   bc
        ld    ix, segmentTable + 3
        ld    a, 4
.l3:    pop   de
        ld    (ix), e                   ; store segment numbers in table
        ld    (ix + 4), e
        ld    (ix + 8), e
        dec   ixl
        dec   a
        jr    nz, .l3
        ld    ixl, low variablesBase
        ld    (ix + savedPage1Segment.offs), c  ; store original page 1 segment
        ld    (ix + page1Segment.offs), b   ; store allocated page 1 segment
        ret                             ; return success (A = 0, Z = 1)
.l4:    cp    07fh
        jr    nz, .l7                   ; not .SHARE ?
        dec   h
        jr    nz, .l5                   ; not enough segments allocated ?
        ld    a, d
        ld    de, 02800h
        cp    d
        jr    c, .l5                    ; not enough space in shared segment ?
        exos  23                        ; set user boundary (10K required)
        or    a
        jr    z, .l2
.l5:    ld    c, l                      ; on error: free all allocated segments
.l6:    exos  25
        inc   h
.l7:    ld    a, 4
        cp    h
        ret   c
        pop   bc
        jr    .l6

freeAllMemory:
        ld    hl, segmentTable
.l1:    ld    c, (hl)
        exos  25
        inc   l
        ld    a, l
        cp    low (segmentTable + 4)
        jr    c, .l1
        ld    iy, (savedIYRegister)     ; restore IY register
        ld    hl, (savedStackPointer)   ; HL = original stack pointer
        ld    a, (ix + savedPage1Segment.offs)
        ld    c, (ix + page1Segment.offs)
        out   (0b1h), a
        exos  25
        ret

closeFiles:
        call  closeInputFile
        jr    closeOutputFile

closeInputFile:
        ld    a, (inFileChannel)
        or    a
        ret   z
        ld    (ix + inFileChannel.offs), 0
        exos  3
        xor   a
        ret

closeOutputFile:
        ld    a, (outFileChannel)
        or    a
        ret   z
        ld    (ix + outFileChannel.offs), 0
        exos  4
        xor   a
        ret

openInputFile:
        ld    a, 1
.l1:    push  af
        ld    de, inputFileName
        exos  1
        or    a
        jr    z, .l2
        cp    0f9h                      ; .CHANX
        jr    nz, .l3                   ; error opening file ?
        pop   af
        inc   a
        cp    0ffh
        jr    c, .l1
        jp    noChannelError            ; could not find a free channel
.l2:    pop   af                        ; file is successfully opened
        ld    (inFileChannel), a
        ret
.l3:    ld    l, a
        pop   af
        exos  3
        ld    a, l
        jp    endCmd

openOutputFile:
        xor   a
        cp    (ix + testFlag.offs)      ; if test mode: nothing to do
        ret   nz
        ld    hl, inputFileName         ; input and output file name
        ld    de, outputFileName        ; must not be the same
        ld    a, (de)
        cp    (hl)
        jr    nz, .l2
        ld    b, a
.l1:    inc   de
        inc   hl
        ld    a, (de)
        xor   (hl)
        and   0dfh                      ; ignore case
        jr    nz, .l2
        djnz  .l1
        ld    a, 095h                   ; "file cannot be copied onto itself"
        jr    .l6
.l2:    ld    a, 1
.l3:    push  af
        ld    de, outputFileName
        exos  2
        or    a
        jr    z, .l4
        cp    0f9h                      ; .CHANX
        jr    nz, .l5                   ; error opening file ?
        pop   af
        inc   a
        cp    0ffh
        jr    c, .l3
        jp    noChannelError            ; could not find a free channel
.l4:    pop   af                        ; file is successfully opened
        ld    (outFileChannel), a
        ret
.l5:    ld    l, a
        pop   af
        exos  4
        ld    a, l
.l6:    jp    endCmd

readBlock:
        push  af
        inc   h
        bit   2, h
        jr    nz, .l1
        pop   af
        ret
.l1:    ld    h, high decompReadBufferP0
        pop   af
        call  setEXOSPaging
        push  ix
        push  iy
        ld    ix, variablesBase
        call  readCompressedData
        pop   iy
        pop   ix
        jp    setDecompressPaging

readCompressedData:
        push  af
        push  bc
        push  de
        push  hl
        ld    bc, 00400h                ; read at most 1024 bytes
        ld    de, decompReadBuffer
        ld    hl, (inFileDataRemaining) ; check the number of bytes remaining:
        ld    a, h
        or    l
        jp    z, fileDataError          ; no data is left to read: error
        ld    a, h
        and   l
        inc   a
        jr    z, .l3                    ; if length = 0FFFFh: read all data
        sbc   hl, bc
        jr    nc, .l1
        add   hl, bc
        ld    b, h
        ld    h, c
        ld    c, l                      ; BC = number of bytes to read
        ld    l, h                      ; HL = 0
.l1:    ld    (inFileDataRemaining), hl
        push  bc
        call  readInputFileBlock
        pop   bc
        jr    nz, .l4                   ; not enough data was read: error
.l2:    ld    hl, decompReadBuffer
        call  updateChecksum
        pop   hl
        pop   de
        pop   bc
        pop   af
        ret
.l3:    call  readInputFileBlock        ; "infinite" input data length:
        ld    c, e                      ; calculate the number of bytes read
        ld    a, d
        sub   high decompReadBuffer
        ld    b, a
        or    c
        jr    nz, .l2
.l4:    jp    fileReadError             ; if zero, then read error

; -----------------------------------------------------------------------------

cmdNameLength   equ     10              ; length of "UNCOMPRESS"
cmdName:
versionMsg:
        defm  "UNCOMPRESS version 1.04\r\n"
        defb  00h
usageMsg:
        defm  "UNCOMPRESS version 1.04\r\n"
        defm  "Usage:\r\n"
        defm  "  UNCOMPRESS <infile> <outfile>\r\n"
        defm  "    uncompress 'infile' to 'outfile'\r\n"
        defm  "  UNCOMPRESS /A <infile>\r\n"
        defm  "    extract all files from archive\r\n"
        defm  "  UNCOMPRESS /L <infile>\r\n"
        defm  "    uncompress to 'UNCOMP__.TMP', and\r\n"
        defm  "    load the resulting temporary file\r\n"
        defm  "  UNCOMPRESS /L <infile> <outfile>\r\n"
        defm  "    uncompress 'infile' to 'outfile',\r\n"
        defm  "    and then load 'outfile'\r\n"
        defm  "  UNCOMPRESS /T <infile> [infile2...]\r\n"
        defm  "    test all input files\r\n"
        defm  "  UNCOMPRESS /TA <infile>\r\n"
        defm  "    test compressed archive\r\n"
        defb  00h

testMsg1:
        defm  ": "
        defb  00h

testMsg2:
        defm  "OK"

newLineMsg:
        defm  "\r\n"
        defb  00h

nextVolumeMsg:
        defm  ": ENTER:OK, STOP:abort\r\n"
        defb  00h

tmpFileName:
        defb  12
        defm  "UNCOMP__.TMP"

errorTable:
        defb  00h

; =============================================================================

; border effects are disabled if this is set to any non-zero value
NO_BORDER_FX            equ     0

decompressData:
        di
        push  hl
        exx
        pop   hl                        ; HL' = compressed data read address
        res   7, h                      ; (on page 0)
        res   6, h
        ld    e, 80h                    ; initialize shift register
        exx
        ld    ((variablesBase & 0ff00h) + decodeTableEnd), sp
        call  setDecompressPaging
        call  getSegment                ; get first output segment
        dec   de
.l1:    call  decompressDataBlock       ; decompress all blocks
        jr    z, .l1
        call  setEXOSPaging
        inc   de
        xor   a

decompressDone:
        ld    c, a                      ; save error code
        call  setEXOSPaging             ; restore memory paging,
        di
        ld    sp, ((variablesBase & 0ff00h) + decodeTableEnd) ; stack pointer,
        ld    ix, variablesBase
    if NO_BORDER_FX == 0
        xor   a                         ; and border color
        out   (81h), a
    endif
        exx
        push  hl
        exx
        pop   hl                        ; HL = address of last compressed
        res   7, h                      ; byte read (on page 1)
        set   6, h
        ld    a, c                      ; on success: return A=0, Z=1, C=0
        add   a, 0ffh                   ; on error: return A=error, Z=0, C=1
        inc   a
        ei
        ret

writeBlock:
        inc   d
        bit   6, d
        ret   z                         ; return Z=1 if no new segment
        push  af
        ld    a, iyl
        inc   a
        cp    low (segmentTable + 8)
        jr    c, .l1
        sub   4
.l1:    ld    iyl, a
        pop   af

getSegment:
        set   7, d                      ; write decompressed data to page 2
        res   6, d
        push  af
        ld    a, iyl
        cp    low segmentTable
        jr    z, .l1                    ; no data in ring buffer yet ?
        call  setEXOSPaging
        push  bc
        push  ix
        push  iy
        ld    ix, variablesBase
        ld    a, (iy - 1)
        ld    bc, 4000h
        call  writeSegment
        pop   iy
        pop   ix
        pop   bc
        call  setDecompressPaging
        jr    nz, decompressDone
.l1:    ld    a, (iy)
        out   (0b2h), a
        pop   af
        ret

setEXOSPaging:
        di
        push  af
        in    a, (0b2h)
        inc   a
        jr    z, .l1
        push  bc
        push  de
        push  hl
        ld    a, 0ffh
        out   (0b2h), a
        ld    hl, 0
        add   hl, sp
        ex    de, hl
        ld    hl, decompStackTopP0
        or    a
        sbc   hl, de
        ld    b, h
        ld    c, l
        ld    hl, (savedStackPointer.p0)
        or    a
        sbc   hl, bc
        ld    sp, hl
        ex    de, hl
        ldir
        pop   hl
        pop   de
        pop   bc
        in    a, (0b1h)
        ld    (decompPage1Segment.p0), a
        ld    a, (page1Segment.p0)
        out   (0b1h), a
        ld    a, (0bffch)
        out   (0b0h), a
        ld    ixh, high variablesBase
        ld    iyh, high variablesBase
.l1:    pop   af
        ei
        ret

setDecompressPaging:
        di
        push  af
        in    a, (0b2h)
        inc   a
        jr    nz, .l1
        push  bc
        push  de
        push  hl
        ld    a, (page1Segment)
        out   (0b0h), a
        ld    hl, 0
        add   hl, sp
        ex    de, hl
        ld    hl, (savedStackPointer)
        or    a
        sbc   hl, de
        ld    b, h
        ld    c, l
        ld    hl, decompStackTopP0
        or    a
        sbc   hl, bc
        ld    sp, hl
        ex    de, hl
        ldir
        pop   hl
        pop   de
        pop   bc
        ld    ixh, high variablesBaseP0
        ld    iyh, high variablesBaseP0
        ld    a, (decompPage1Segment)
        out   (0b1h), a
        ld    a, (iy)
        out   (0b2h), a
.l1:    pop   af
        ret

; -----------------------------------------------------------------------------

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
slotBitsTable           equ 0000h
slotBitsTableL          equ slotBitsTable
slotBitsTableO1         equ slotBitsTableL + (nLengthSlots * 4)
slotBitsTableO2         equ slotBitsTableO1 + (nOffs1Slots * 4)
slotBitsTableO3         equ slotBitsTableO2 + (nOffs2Slots * 4)
decodeTableEnd          equ slotBitsTable + (totalSlots * 4)

decompressDataBlock:
        ld    ixl, low variablesBase
        bit   0, (ix + haveStartAddress.offs)
        exx
        jr    z, .l1
        call  read8Bits                 ; ignore start address
        call  read8Bits
.l1:    call  read8Bits                 ; read number of symbols - 1 (BC)
        exx
        ld    c, a                      ; NOTE: MSB is in C, and LSB is in B
        call  read8Bits_
        exx
        ld    b, a
        inc   b
        inc   c
        ld    a, 40h
        exx
        call  readBits                  ; read flag bits
        exx
        srl   a
        push  af                        ; save last block flag (A = 1, Z = 0)
        jr    c, .l2                    ; is compression enabled ?
        exx                             ; no, copy uncompressed literal data
        ld    bc, 0101h
        jp    .l13
.l2:    push  bc                        ; compression enabled:
        ld    a, 40h
        exx
        call  readBits                  ; get prefix size for length >= 3 bytes
        ld    b, a
        inc   b
        ld    a, 08h
        ld    d, 80h
.l3:    add   a, a
        srl   d                         ; D' = prefix size code for readBits
        djnz  .l3
        pop   bc                        ; store the number of symbols in BC'
        exx
        push  de                        ; save decompressed data write address
        add   a, low ((nLengthSlots + nOffs1Slots + nOffs2Slots) * 4)
        ld    (ix - 1), a
        ld    de, decompTablesBaseP0    ; initialize decode tables
        defb  01h                       ; = LD BC,nnnn, 00h = NOP
.l4:    ld    bc, 0                     ; set initial base value (len=1, offs=0)
        inc   c                         ; 1 is added to correct for the LDI
.l5:    ld    a, 10h
        exx
        call  readBits
        exx
        ld    l, a
        add   a, a
        add   a, l
        add   a, low bitCntTable
        ld    l, a
        adc   a, high bitCntTable
        sub   l
        ld    h, a
        inc   e
        ldi                             ; copy read bit count
        ld    a, c                      ; store and update base value
        ld    (de), a
        inc   e
        add   a, (hl)
        inc   hl
        ld    c, a
        ld    a, b
        ld    (de), a
        inc   e
        adc   a, (hl)
        ld    b, a
        ld    a, e
        cp    low slotBitsTableO1
        jr    z, .l4                    ; end of length decode table ?
        cp    low slotBitsTableO2
        jr    z, .l4                    ; end of offset table for length=1 ?
        cp    low slotBitsTableO3
        jr    z, .l4                    ; end of offset table for length=2 ?
        xor   (ix - 1)
        jr    nz, .l5                   ; continue until all tables are read
        ld    c, a
        ld    b, a
        pop   de                        ; DE = decompressed data write address
        exx
.l8:    sla   e
        jr    nc, .l14                  ; literal byte ?
        jp    nz, .l9
        inc   l
        call  z, readBlock
        ld    e, (hl)
        rl    e
        jr    nc, .l14
.l9:    ld    a, 0f8h
.l10:   sla   e
        jr    nc, copyLZMatch           ; LZ77 match ?
        jp    nz, .l11
        inc   l
        call  z, readBlock
        ld    e, (hl)
        rl    e
        jr    nc, copyLZMatch
.l11:   inc   a
        jr    nz, .l10
        inc   a
        call  readBits                  ; get literal sequence length - 17
        exx
        add   a, 16
        ld    b, a
        adc   a, 1
        sub   b
        inc   b                         ; B: (length - 1) LSB + 1
        ld    c, a                      ; C: (length - 1) MSB + 1
.l12:   exx                             ; copy literal sequence
.l13:   inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        inc   e
        call  z, writeBlock
        ld    (de), a
        djnz  .l12
        dec   c
        jr    nz, .l12
        jr    .l15
.l14:   inc   l                         ; copy literal byte
        call  z, readBlock
        ld    a, (hl)
        exx
        inc   e
        call  z, writeBlock
        ld    (de), a
.l15:   exx
        djnz  .l8
        dec   c
        jr    nz, .l8
        exx
        pop   af                        ; return with last block flag
        ret                             ; (A = 1, Z = 0 if last block)

lzOffsetError:
        ld    a, 0e6h                   ; .ESC (invalid escape sequence)
        jp    decompressDone

copyLZMatch:
        exx
        add   a, a
        add   a, a
        add   a, low (slotBitsTableL + 32 + 1)
        ld    h, high decompTablesBaseP0
        call  readBits16_               ; decode match length
        exx
        inc   l
        add   a, (hl)
        inc   l
        ld    c, a                      ; C = length LSB
        ld    a, b
        adc   a, (hl)
        ld    b, a                      ; B = length MSB
        push  bc
        jr    nz, .l4                   ; length >= 256 bytes ?
        dec   c
        jr    z, .l5                    ; length == 1 byte ?
        dec   c
        jr    nz, .l4                   ; length >= 3 bytes ?
        ld    a, 20h                    ; length == 2 bytes, read 3 prefix bits
        ld    b, low slotBitsTableO2 + 1
        jp    .l6
.l4:    ld    b, low slotBitsTableO3 + 1    ; length >= 3 bytes,
        exx
        ld    a, d                      ; variable prefix size
        jp    .l7
.l5:    ld    a, 40h                    ; length == 1 byte, read 2 prefix bits
        ld    b, low slotBitsTableO1 + 1
.l6:    exx
.l7:    call  readBits                  ; read offset prefix bits
        exx
        add   a, a
        add   a, a
        add   a, b
        call  readBits16_               ; decode match offset
        exx
        inc   l
        add   a, (hl)
        inc   l
        ld    c, a
        ld    a, b
        adc   a, (hl)
        jr    c, lzOffsetError          ; offset > 65536 bytes is not supported
        ld    h, a
        cp    high 4000h
        ld    a, e                      ; calculate LZ77 match read address
    if NO_BORDER_FX == 0
        out   (81h), a
    endif
        jr    c, .l23                   ; offset <= 16384 bytes?
        sub   c
        pop   bc                        ; BC = length (should be >= 3 here)
        ld    l, a
        ld    a, d
        sbc   a, h
        ld    h, a                      ; set up memory paging
        jr    c, .l21                   ; page 2 or 3?
        add   a, a                      ; page 0 or 1
        jp    p, .l10                   ; page 0?
        ld    a, (iy - 1)               ; page 1
        jr    .l12
.l10:   ld    a, (iy - 2)
.l11:   set   6, h                      ; read from page 1
.l12:   out   (0b1h), a
        inc   e                         ; copy match data
        call  z, writeBlock
        ldi
        dec   de
.l13:   inc   e
        jr    z, .l16
.l14:   bit   7, h
        jr    nz, .l17
.l15:   ldi
        dec   de
        jp    pe, .l13
        jp    decompressDataBlock.l15   ; return to main decompress loop
.l16:   call  writeBlock
        jr    .l14
.l17:   ld    h, high 4000h
        push  iy
        in    a, (0b1h)
.l18:   cp    (iy)
        dec   iy
        jr    nz, .l18
        ld    a, (iy + 2)
        out   (0b1h), a                 ; read next segment
        pop   iy
        jr    .l15
.l21:   res   7, h                      ; page 2 or 3
        add   a, a
        jp    p, .l22                   ; page 2 ?
        ld    a, (iy - 3)               ; page 3
        jr    .l12
.l22:   ld    a, (iy - 4)
        jr    .l11
.l23:   sub   c                         ; offset <= 16384 bytes:
        pop   bc                        ; BC = length
        ld    l, a
        ld    a, d
        sbc   a, h
        ld    h, a
        ld    a, (iy - 1)
        out   (0b1h), a
.l24:   inc   e                         ; copy match data
        jr    z, .l26
.l25:   ldi
        dec   de
        jp    pe, .l24
        jp    decompressDataBlock.l15   ; return to main decompress loop
.l26:   call  writeBlock
        jr    z, .l25
        ld    a, (iy - 1)
        out   (0b1h), a
        ld    a, h
        sub   high 4000h
        ld    h, a
        jr    .l25

; set decode table offset (L = A) and read match parameter bits (0 to 15) to BA
; returns with registers swapped (EXX)

readBits16_:
        ld    l, a
        xor   a
        ld    b, a
        or    (hl)
        exx
        ret   z                         ; 0 bits?
        jp    po, readBits              ; 1 to 8 bits?
        rla                             ; 9 to 15 bits
        call  readBits
        exx
        ld    b, a

read8Bits_:
        exx

read8Bits:
        ld    a, 01h

readBits:
        sla   e
        jr    z, .l1
        adc   a, a
        ret   c
        sla   e
        jr    z, .l1
        adc   a, a
        ret   c
        sla   e
        jr    z, .l1
        adc   a, a
        ret   c
        sla   e
        jr    z, .l1
        adc   a, a
        ret   c
        sla   e
        jr    z, .l1
        adc   a, a
        ret   c
        sla   e
        jr    z, .l1
        adc   a, a
        ret   c
        sla   e
        jr    z, .l1
        adc   a, a
        ret   c
        sla   e
        jr    z, .l1
        adc   a, a
        ret
.l1:    inc   l
        call  z, readBlock
        ld    e, (hl)
        rl    e
        adc   a, a
        ret   c
        sla   e
        adc   a, a
        ret   c
        sla   e
        adc   a, a
        ret   c
        sla   e
        adc   a, a
        ret   c
        sla   e
        adc   a, a
        ret   c
        sla   e
        adc   a, a
        ret   c
        sla   e
        adc   a, a
        ret   c
        sla   e
        adc   a, a
        ret

; byte 0: number of bits to read
;   00h = none
;   80h, 40h, ..., 02h, 01h (parity odd) = 1 to 8 bits
;   0c0h, 0a0h, ..., 82h, 81h (parity even) = 9 to 15 bits
; byte 1, byte 2: base value + 1

    macro convertBitCnt n
        defb  ((0100h >> n) & 0ffh) | ((8 - n) & 80h) | ((8000h >> n) & 7fh)
        defw  0001h + (0001h << n)
    endm

bitCntTable:
        convertBitCnt 0
        convertBitCnt 1
        convertBitCnt 2
        convertBitCnt 3
        convertBitCnt 4
        convertBitCnt 5
        convertBitCnt 6
        convertBitCnt 7
        convertBitCnt 8
        convertBitCnt 9
        convertBitCnt 10
        convertBitCnt 11
        convertBitCnt 12
        convertBitCnt 13
        convertBitCnt 14
        convertBitCnt 15

codeEnd:

    if BUILD_EXTENSION_ROM != 0
        size  16384
    endif

