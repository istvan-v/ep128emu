
; 0: use all available RAM for reading input file
; 1: use page 0 only for reading input file
; 2: use 1K buffer on page 0 for reading input file
BUFFERING_MODE          equ     0
; 0: 16K video buffer (segment FE)
; 1: 32K video buffer (segments FD, FE)
VIDEO_BUFFER_32K        equ     1

; minimum match length supported by compressed data format (1, 2, or 3)
LZ_FMT_MIN_LENGTH       equ     1
; minimum match length assumed to be actually used in compressed data
LZ_MIN_LENGTH           equ     LZ_FMT_MIN_LENGTH

; -----------------------------------------------------------------------------

moduleType              equ     'v'

lptBaseAddr             equ     0c000h
stackTop                equ     00100h
segmentTable            equ     00c00h  ; (playerCodeEnd + 0ffh) & 0ff00h
decodeTablesBegin       equ     segmentTable + 00100h
variablesEnd            equ     decodeTablesBegin + 00100h
frameTimer              equ     variablesEnd - 2
fieldsPlayed            equ     variablesEnd - 4
fieldNum                equ     variablesEnd - 5
fieldsBuffered          equ     variablesEnd - 6
bufferFields            equ     variablesEnd - 7
loadingModule           equ     variablesEnd - 8
inputFileChannel        equ     variablesEnd - 9
frameRate               equ     variablesEnd - 10
currentFrameRate        equ     variablesEnd - 11
paletteSize             equ     variablesEnd - 12
biasDataSize            equ     variablesEnd - 14
paletteDataSize         equ     variablesEnd - 16
fieldHeight             equ     variablesEnd - 17
imageWidth              equ     variablesEnd - 18
frameHeight             equ     variablesEnd - 20
interlaceMode           equ     variablesEnd - 21
bytesPerLine            equ     variablesEnd - 22
blockSize               equ     variablesEnd - 24
lptSize                 equ     variablesEnd - 26
fileNameBufferPos       equ     variablesEnd - 28
filesRemaining          equ     variablesEnd - 29
; bit 6: 1 if space key is currently not pressed; bit 7: 1 if it was pressed
spaceKeyState           equ     variablesEnd - 30
videoBufReadPos         equ     variablesEnd - 32
videoBufWritePos        equ     variablesEnd - 34
splittingInputFile      equ     variablesEnd - 35
playVideoStatus         equ     variablesEnd - 36
usingFILEExtension      equ     variablesEnd - 37
; NOTE: this is overwritten by the decompressor
inputFileHeader         equ     variablesEnd - 64
fileNameBuffer          equ     variablesEnd
playerDataEnd           equ     fileNameBuffer + 0100h
  if BUFFERING_MODE < 2
fileReadBufferBegin     equ     playerDataEnd
  else
fileReadBufferBegin     equ     3c00h
  endif

; -----------------------------------------------------------------------------

loaderCodeBegin:

parseCommand:
        ld    hl, stackTop
        ld    c, 60h                    ; free all allocated memory,
        call  exosReset                 ; and close all channels
        call  copyPlayerCode
        jp    main

loadModule:
        push  bc
        push  de
        call  copyPlayerCode
        pop   hl
        pop   bc
        ld    a, 1
        ld    (loadingModule), a
        ld    a, b
        ld    (inputFileChannel), a
        ld    de, inputFileHeader
        ld    bc, 16
        ldir
        ld    l, 1
        ld    h, a                      ; if loading a module:
.l1:    ld    a, l
        dec   a
        cp    h                         ; close all channels,
        jr    z, .l2                    ; but keep the input file open
        exos  3
.l2:    inc   l
        jr    nz, .l1
        ld    hl, stackTop
        ld    c, 40h
        call  exosReset                 ; free all allocated memory
        jp    main

copyPlayerCode:
        ld    hl, loaderCodeEnd
        ld    de, playerCodeBegin
        ld    bc, playerCodeEnd - playerCodeBegin
        ldir
        ld    l, e
        ld    h, d
        ld    (hl), c
        inc   de
        ld    bc, playerDataEnd - (playerCodeEnd + 1)
        ldir
        ret

shortHelpString:
        defm  "IPLAY version 1.04\r\n"
        defb  00h
longHelpString:
        defb  00h

loaderCodeEnd:

; =============================================================================

        phase 0100h

playerCodeBegin:

main:
        di
        ld    sp, stackTop              ; initialize stack pointer
        ld    hl, 0038h
        ld    de, savedIRQRoutine
        ld    bc, 4
        ldir                            ; back up original EXOS IRQ routine
        ld    a, 0ffh
        out   (0b2h), a
        ld    hl, resetRoutine
        ld    (0bff8h), hl
        ei
        ld    a, (loadingModule)
        or    a
        jr    z, .l1
        call  playVideoFile             ; if loading module: play a single file
        jp    resetRoutine
.l1:    call  loadINIFile               ; load playlist
        jp    nz, resetRoutine          ; STOP key was pressed ?
.l2:    ld    a, (filesRemaining)       ; get next file name
        or    a
        jr    nz, .l3                   ; not all files have been played yet ?
        ld    a, (usingFILEExtension)
        or    a
        jr    nz, .l1                   ; choose next file name if using FILE
        jp    resetRoutine
.l3:    dec   a
        ld    (filesRemaining), a
        ld    a, 1
        exos  3                         ; close channel first
        ld    hl, (fileNameBufferPos)   ; get name address,
        ld    d, h
        ld    e, l
        ld    c, (hl)                   ; and calculate the address of the
        ld    b, 0                      ; next file name
        inc   hl
        add   hl, bc
        ld    (fileNameBufferPos), hl
        ld    a, 1
        exos  1                         ; open file
        or    a
        jr    nz, .l2                   ; error: try next file
        ld    a, 1
        ld    (inputFileChannel), a
        call  playVideoFile             ; play file
        ld    a, (inputFileChannel)
        exos  3                         ; close file,
        jr    .l2                       ; and continue with the next one

chooseInputFile:
        xor   a
        ld    (usingFILEExtension), a
        ld    de, exdosCommandName      ; is EXDOS available ?
        exos  26
        or    a
        jr    z, .l1
        ld    hl, fileNameBuffer        ; no, use empty file name
        ld    (hl), 0                   ; for TAPE: or FILE: device
        jr    .l2
.l1:    ld    de, fileCommandName       ; try to select a file
        exos  26                        ; with the 'FILE' extension
        ld    hl, fileNameBuffer
        or    a
        jr    nz, .l3
        inc   a
        ld    (usingFILEExtension), a
.l2:    ld    (fileNameBufferPos), hl
        ld    a, 1
        ld    (filesRemaining), a
        xor   a
        ret
.l3:    ld    hl, defaultFileName       ; if 'FILE' extension is not found:
        cp    0e5h                      ; use default name ("videodat.bin")
        jr    nz, .l2
        or    a                         ; STOP key was pressed
        ret

loadINIFile:
        ld    a, (usingFILEExtension)
        or    a
        jp    nz, chooseInputFile
        ld    a, 1
        ld    de, iniFileName
        exos  1                         ; open .ini file
        or    a
        jp    nz, chooseInputFile       ; cannot open .ini file ?
        ld    hl, fileNameBuffer
        ld    (fileNameBufferPos), hl
        ld    (filesRemaining), a
.l2:    push  hl                        ; store position of length byte
        inc   l                         ; skip length byte
        jr    z, .l6                    ; buffer is full ?
.l3:    ld    a, 1                      ; skip whitespace
        exos  5
        or    a
        jr    nz, .l6                   ; EOF ?
        ld    a, b
        cp    021h
        jr    c, .l3
        ld    a, (filesRemaining)
        inc   a
        ld    (filesRemaining), a
.l4:    ld    (hl), b                   ; read and store file name
        inc   l
        jr    z, .l5                    ; until buffer is full,
        ld    a, 1
        exos  5
        or    a
        jr    nz, .l5                   ; EOF,
        ld    a, b
        cp    021h                      ; or whitespace
        jr    nc, .l4
        defb  011h                      ; LD DE, nnnn
.l5:    ld    b, 0ffh
        pop   de
        ld    a, l                      ; calculate and store name length
        scf
        sbc   a, e
        ld    (de), a
        ld    a, b
        cp    021h                      ; if not EOF yet,
        jr    c, .l2                    ; continue with next name
        defb  0feh                      ; CP nn
.l6:    pop   de
        ld    a, 1
        exos  3                         ; close .ini file
        xor   a
        ret

resetRoutine:
        di
        ld    a, 04h
        out   (0bfh), a
        ld    sp, stackTop
        ld    a, 0ffh
        out   (0b2h), a
        ld    a, 01h
        out   (0b3h), a
        ld    a, 3ch
        out   (0b4h), a
        ld    hl, (0bff4h)
        call  setLPTAddress
        ld    hl, savedIRQRoutine
        ld    de, 0038h
        ld    bc, 3
        ldir
        im    1
        ld    c, 40h
        exos  0
        ld    a, 6
        jp    0c00dh

; -----------------------------------------------------------------------------

readVideoFileHeader:
        ld    a, (loadingModule)
        or    a                         ; if loading a module, then the
        jr    nz, .l14                  ; file header has already been read
        ld    a, (inputFileChannel)
        ld    bc, 16
        ld    de, inputFileHeader
        exos  6
        or    a
        jp    nz, .l12
.l14:   ld    de, inputFileHeader
        ld    a, (de)                   ; byte 0: must be 000h
        inc   de
        or    a
        jp    nz, .l11
        ld    a, (de)                   ; byte 1: must be 076h ('v')
        inc   de
        cp    076h
        jp    nz, .l11
        ld    a, (de)                   ; video mode
        inc   de
        and   06eh
        ld    b, a
        ld    (videoMode), a
        and   00eh
        cp    002h
        jr    z, .l1
        cp    00eh
        jp    nz, .l10
.l1:    ld    a, b                      ; calculate palette size
        rlca
        rlca
        rlca
        and   3                         ; 0, 1, 2, 3
        cp    2
        sbc   a, 0feh                   ; 1, 2, 4, 5
        ld    b, a
        cp    5
        sbc   a, a
        and   b                         ; 1, 2, 4, 0
        add   a, a                      ; 2, 4, 8, 0
        ld    (paletteSize), a
        xor   a
        out   (080h), a
        ld    (biasDataSize), a
        ld    (biasDataSize + 1), a
        ld    a, (de)                   ; FIXBIAS
        inc   de
        cp    020h
        jr    nc, .l2
        out   (080h), a
        jr    .l3
.l2:    neg
        ld    (biasDataSize), a
.l3:    ld    a, (de)                   ; palette data size for one field
        inc   de
        ld    (paletteDataSize), a
        ld    a, (de)
        inc   de
        ld    (paletteDataSize + 1), a
        cp    8
        jp    nc, .l10
        ld    a, (de)                   ; field height (1-255)
        inc   de
        or    a
        jp    z, .l10
        ld    (fieldHeight), a
        ld    a, (de)                   ; width in characters
        inc   de
        ld    (imageWidth), a
        cp    47
        jp    nc, .l10                  ; must be less than 47,
        cp    5
        jp    c, .l10                   ; and greater than or equal to 5
        ld    b, a
        rra                             ; calculate margins
        adc   a, 01fh
        ld    (rightMargin), a
        sub   b
        ld    (leftMargin), a
        ld    a, (videoMode)            ; calculate pixel data size per line
        bit   3, a
        ld    a, 4
        jr    nz, .l4
        sla   b
.l4:    add   a, a
        cp    b
        jr    c, .l4                    ; round up to the nearest power of two
        ld    (bytesPerLine), a
        ld    a, (de)                   ; interlace mode
        inc   de
        dec   a
        cp    4
        jp    nc, .l10                  ; must be 1, 2, or 4
        inc   a
        cp    3
        jp    z, .l10
        ld    (interlaceMode), a
        ld    b, a
        ld    a, (de)                   ; border color
        inc   de
        ld    (paletteBuf + 0), a
        out   (081h), a
        inc   de                        ; ignore the number of fields
        inc   de
        ld    a, (de)                   ; frame rate
        inc   de
        or    a
        jp    z, .l10
        ld    (frameRate), a
        ld    (currentFrameRate), a
        ld    a, (de)                   ; vertical zoom (1, 2, 3, or 4)
        inc   de
        neg
        ld    (verticalZoom), a         ; store as LPB line count
        cp    0fch
        jp    c, .l10
        neg
        ld    c, a
        ld    a, (de)                   ; compression type (must be 1)
        inc   de
        dec   a
        jp    nz, .l10
        ld    a, (de)                   ; reserved byte (must be 0)
        or    a
        jp    nz, .l10
        ld    hl, 0
        ld    a, (fieldHeight)
        ld    e, a
        ld    d, h
.l5:    add   hl, de                    ; calculate frame height
        djnz  .l5                       ; ( = fieldHeight * interlaceMode (B))
        ld    (frameHeight), hl
        ld    b, c                      ; get vertical zoom (C)
        ex    de, hl
        ld    hl, 0
.l6:    add   hl, de                    ; calculate height with vertical zoom
        djnz  .l6
        ex    de, hl
        ld    a, d
        or    a
        jr    z, .l7                    ; must be <= 288 lines
        dec   a
        jp    nz, .l10
        ld    a, e
        cp    021h
        jp    nc, .l10
.l7:    ld    a, d                      ; calculate border sizes
        rra
        ld    a, e
        rra
        ld    e, a
        adc   a, 06dh
        ld    d, a                      ; D = top border
        ld    a, e
        add   a, 06eh                   ; A = bottom border
        ld    b, 3
        inc   a                         ; center image,
        jr    z, $ + 6                  ; but make sure that borders
        dec   d                         ; do not overflow
        djnz  $ - 4
        defb  0feh                      ; CP nn
        dec   a
        ld    (lptBBorder + 16), a
        ld    a, d
        ld    (lptTBorder), a
        ld    hl, (frameHeight)         ; calculate LPT size
        add   hl, hl
        add   hl, hl
        add   hl, hl
        add   hl, hl
        ld    bc, lptTBorderLen + lptBBorderLen + lptVBlankLen
        add   hl, bc
        ld    (lptSize), hl
        ld    a, (bytesPerLine)         ; calculate block size
        ld    e, a
        ld    d, 0
        ld    h, d
        ld    l, d
        ld    a, (fieldHeight)
        ld    b, a
.l8:    add   hl, de
        djnz  .l8
        ld    de, (biasDataSize)
        add   hl, de
        ld    de, (paletteDataSize)
        add   hl, de
        ld    (blockSize), hl
        ld    a, l
        add   a, a
        ld    a, h
        adc   a, a
        jr    z, .l10                   ; allowed range is 128 bytes
        rra
        and   0c0h                      ; to 16383 bytes
        jr    nz, .l10
        ld    d, h                      ; calculate the number of fields that
        ld    e, l                      ; can be stored in the video buffer
        ld    a, (interlaceMode)
        ld    b, a
        xor   a
        ld    h, a
        ld    l, a
.l9:    add   hl, de
        inc   a
  if VIDEO_BUFFER_32K == 0
        bit   6, h
  else
        bit   7, h
  endif
        jr    z, .l9
  if VIDEO_BUFFER_32K == 0
        ld    de, 04001h
  else
        ld    de, 08001h
  endif
        or    a
        sbc   hl, de
        adc   a, 0ffh
        sub   b
        jr    c, .l10                   ; there must be enough space for a
        jr    z, .l10                   ; whole frame and one field
        ld    (bufferFields), a
        ld    hl, paletteBuf            ; initialize palette
        ld    de, paletteBuf + 1
        ld    bc, 7
        ldir
        ld    hl, (paletteDataSize)
        ld    a, h
        or    l
        jr    z, .l13                   ; read fixed palette
        xor   a
        ret
.l10:   ld    a, 0dch                   ; .VDISP
        defb  001h                      ; LD BC, nnnn
.l11:   ld    a, 0eeh                   ; .ITYPE
.l12:   add   a, 0ffh
        inc   a
        ret
.l13:   ld    b, a                      ; A = 0
        ld    a, (paletteSize)
        or    a
        ret   z                         ; 256 color mode: no palette
        ld    c, a
        ld    de, paletteBuf
        ld    a, (inputFileChannel)
        exos  6                         ; read fixed palette
        jr    .l12

playVideoFile:
        xor   a
        ld    (playVideoStatus), a
        ld    (exitPlayVideoFile.l1 + 1), sp
        di
        ld    hl, 00038h
        ld    (hl), 0c3h                ; JP nnnn
        inc   hl
        ld    (hl), low irqRoutine
        inc   hl
        ld    (hl), high irqRoutine
        ld    a, 030h
        out   (0b4h), a
        im    1
        xor   a
        ld    h, a
        ld    l, a
        ld    (frameTimer), hl
        ld    (fieldsPlayed), hl
        ld    (fieldNum), a
        ld    (fieldsBuffered), a
        ld    (spaceKeyState), a
        ld    (splittingInputFile), a
  if VIDEO_BUFFER_32K == 0
        ld    hl, 08000h
  else
        ld    hl, 04000h
  endif
        ld    (videoBufReadPos), hl
        ld    (videoBufWritePos), hl
        ei
        call  readVideoFileHeader
        jp    nz, playVideoFileError
        call  setupIRQRoutine
        ei
        call  allocateMemory
        jp    nz, playVideoFileError
  if BUFFERING_MODE != 0
        call  freeFileReadBuffer
  endif
        call  createLPT
        ld    a, 004h
        out   (0bfh), a
        call  loadVideoFile
        jp    nz, playVideoFileError
        ld    a, 00ch
        out   (0bfh), a
        di
        ld    hl, 0
        ld    (frameTimer), hl
.l1:    xor   a
        ld    (fieldsPlayed), a
        ld    (fieldsPlayed + 1), a
        ld    (fieldNum), a
        ld    (spaceKeyState), a
  if BUFFERING_MODE == 0
        ld    iy, segmentTable          ; initialize decompressor
        ld    a, (iy)
        out   (0b1h), a
  endif
        exx
  if BUFFERING_MODE == 0
        ld    hl, 04000h + fileReadBufferBegin + 1
  else
        ld    hl, fileReadBufferBegin + 1
  endif
        ld    e, 080h
        exx
  if VIDEO_BUFFER_32K == 0
        ld    a, 0feh
        out   (0b2h), a
        out   (0b3h), a
    if BUFFERING_MODE != 0
        out   (0b1h), a
    endif
  endif
        ld    hl, fieldsBuffered
.l2:    ld    a, (bufferFields)
        ld    b, a
        ld    a, (hl)
        cp    b
        jr    c, .l5                    ; buffer is not full yet ?
        di
        ld    a, (frameRate)
        ld    c, a
        ld    a, (currentFrameRate)     ; if the buffer is full,
        dec   a                         ; increase playback speed
        cp    c
        jr    c, .l3
        ld    (currentFrameRate), a
.l3:    ei
.l4:    ld    a, (hl)
        cp    b
        jr    nc, .l4                   ; wait until buffer is not full
.l5:    ld    de, (videoBufWritePos)    ; uncompress one field
  if VIDEO_BUFFER_32K == 0
        set   7, d
        res   6, d
  else
        call  setDecompressPaging
  endif
        call  decompressDataBlock
        ex    af, af'                   ; save last block flag to AF'
  if VIDEO_BUFFER_32K == 0
        res   6, d
  else
        call  getDecompressPaging
  endif
        ld    (videoBufWritePos), de
        ld    hl, fieldsBuffered
        inc   (hl)
        ld    a, (spaceKeyState)
        add   a, a
        jr    c, .l8                    ; space key was pressed ?
        ex    af, af'
        jr    z, .l2                    ; end of stream not reached yet ?
        ei
.l6:    ld    a, (hl)                   ; wait until all the remaining
        or    a                         ; video data is played
        jr    nz, .l6
        ld    a, (splittingInputFile)   ; if the whole file is loaded into
        or    a                         ; memory, restart playback from the
        jr    z, .l1                    ; beginning, otherwise exit now
        ld    a, (currentFrameRate)
        ld    b, a
.l7:    ld    a, (frameTimer)
        cp    b
        jr    c, .l7
.l8:    xor   a                         ; return success

playVideoFileError:
        ld    (playVideoStatus), a

exitPlayVideoFile:
        di
        ld    a, 004h
        out   (0bfh), a
.l1:    ld    sp, 00000h                ; * restore stack pointer
        ld    a, 0ffh
        out   (0b2h), a
        ld    a, 03ch
        out   (0b4h), a
        ld    hl, savedIRQRoutine
        ld    de, 00038h
        ld    bc, 3
        ldir
        im    1
        ld    hl, (0bff4h)
        call  setLPTAddress
        ei
        call  freeAllMemory
        ld    a, (playVideoStatus)
        or    a
        ret

  if VIDEO_BUFFER_32K != 0

setDecompressPaging:
        ld    a, 0fdh
        set   7, d
        bit   6, d
        res   6, d
        jr    nz, .l1
    if BUFFERING_MODE != 0
        out   (0b1h), a
    endif
        out   (0b3h), a
        inc   a
        out   (0b2h), a
        ret
.l1:    out   (0b2h), a
        inc   a
    if BUFFERING_MODE != 0
        out   (0b1h), a
    endif
        out   (0b3h), a
        ret

getDecompressPaging:
        in    a, (0b2h)
        rrca
        rrca
        xor   d
        add   a, a
        jp    m, .l1
        set   7, d
        res   6, d
        ret
.l1:    res   7, d
        set   6, d
        ret

  endif

; -----------------------------------------------------------------------------

allocateMemory:
        ld    iy, segmentTable
        in    a, (0b0h)
        ld    (iy), a
        inc   iy
        ld    h, 0
.l1:    exos  24
        or    a
        jr    nz, .l3
        ld    a, c
  if VIDEO_BUFFER_32K == 0
        cp    0feh
  else
        cp    0fdh
  endif
        jr    c, .l2
        cp    0ffh
        jr    nc, .l2
        inc   h
        jr    .l1
.l2:    ld    (iy), a
        inc   iyl
        jr    nz, .l1
        ld    a, 0f7h                   ; this should not be reached
        or    a
        ret
.l3:    cp    07fh                      ; .SHARE
        ld    a, 0f7h                   ; .NORAM
        ret   nz                        ; not a shared segment ?
  if VIDEO_BUFFER_32K != 0
        dec   h
  endif
        dec   h
        ret   nz                        ; could not allocate video buffer ?
        inc   c
        ret   nz                        ; shared segment is not 0FFh ?
        ld    h, d
        ld    l, e
        ld    de, (lptSize)
        res   7, h
        res   6, h
        or    a
        sbc   hl, de
        ret   c                         ; not enough space for the LPT ?
        exos  23                        ; set boundary
        or    a
        ret

freeFileReadBuffer:
        ld    hl, segmentTable
        ld    b, 0ffh
.l1:    inc   hl
        ld    a, (hl)
        or    a
        jr    z, .l2
        push  bc
        ld    c, a
        exos  25
        pop   bc
        ld    (hl), 0
.l2:    djnz  .l1
        ret

freeAllMemory:
        call  freeFileReadBuffer
  if VIDEO_BUFFER_32K != 0
        ld    c, 0fdh
        exos  25
  endif
        ld    c, 0feh
        exos  25
        ld    c, 0ffh
        exos  25
        ret

loadVideoFile:
  if BUFFERING_MODE == 0
        ld    iy, segmentTable
        ld    a, (iy)
        inc   iy
        out   (0b1h), a
        ld    de, 04000h + fileReadBufferBegin
  else
        ld    de, fileReadBufferBegin
  endif
        ld    bc, 04000h - fileReadBufferBegin
        ld    a, (inputFileChannel)
        exos  6
        or    a
  if BUFFERING_MODE == 0
        jr    nz, .l2
.l1:    ld    a, (iy)
        inc   iy
        or    a
        jr    z, .l3
        out   (0b1h), a
        ld    de, 04000h
        ld    bc, 04000h
        ld    a, (inputFileChannel)
        exos  6
        or    a
        jr    z, .l1
  else
        jr    z, .l3
  endif
.l2:    or    a
        ret   z
        cp    0e4h                      ; EOF ?
        ret   nz
        xor   a
        ret
.l3:    ld    a, 1
        ld    (splittingInputFile), a
        xor   a
        ret

; -----------------------------------------------------------------------------

createLPT:
        ld    a, 0ffh
        out   (0b3h), a
        dec   a
        out   (0b2h), a
        ld    de, 0bf80h
        ld    h, d
        ld    l, e
        ld    a, (paletteBuf + 0)
        ld    b, a
        ld    a, (paletteSize)
        cp    1
        sbc   a, a
        and   b
        ld    (de), a
        inc   de
        ld    bc, 0007fh
        ldir
        ld    de, lptBaseAddr
        ld    hl, lptTBorder
        ld    bc, lptTBorderLen
        ldir
        push  de
        ld    hl, lptVideoLine
        ld    bc, lptVideoLineLen
        ldir
        ld    hl, (lptSize)
        ld    bc, -(lptTBorderLen+lptVideoLineLen+lptBBorderLen+lptVBlankLen)
        add   hl, bc
        ex    (sp), hl
        pop   bc
        ldir
        ld    hl, lptBBorder
        ld    bc, lptBBorderLen
        ldir
        ld    hl, lptVBlank
        ld    bc, lptVBlankLen
        ldir
        ld    hl, lptBaseAddr

setLPTAddress:
        ld    a, l
        ld    b, 4
.l1:    srl   h
        rra
        djnz  .l1
        out   (082h), a
        ld    a, h
        and   003h
        or    00ch
        out   (083h), a
        set   6, a
        out   (083h), a
        set   7, a
        out   (083h), a
        ret

setupIRQRoutine:
        di
        ld    a, (interlaceMode)
        dec   a
        ld    (irqRoutine.l4 + 1), a
        ld    de, (biasDataSize)
        ld    hl, (paletteDataSize)
        add   hl, de
        ld    a, h
        or    l
        jr    nz, .l1
        ld    a, 0c3h                   ; JP nnnn
        ld    hl, irqRoutine.l6
        jr    .l2
.l1:    ld    a, 001h                   ; LD BC, nnnn
.l2:    ld    (irqRoutine.l5), a
        ld    (irqRoutine.l5 + 1), hl
        ld    a, (fieldHeight)
        inc   a
        ld    (irqRoutine.l7 + 1), a
        ld    a, (interlaceMode)
        add   a, a
        add   a, a
        add   a, a
        add   a, a
        ld    c, a                      ; store interlaceMode * 16 in C
        dec   a
        ld    (irqRoutine.l8 + 1), a
        ld    a, (bytesPerLine)
        ld    (irqRoutine.l9 + 1), a
        ld    a, d
        or    e
        jr    nz, .l3
        ld    a, 0c3h                   ; JP nnnn
        ld    de, irqRoutine.l15
        jr    .l4
.l3:    ld    a, 001h                   ; LD BC, nnnn
.l4:    ld    (irqRoutine.l14), a
        ld    (irqRoutine.l14 + 1), de
        ld    hl, (paletteDataSize)
        ld    a, h
        or    l
        jr    nz, .l5
        ld    a, 0c3h                   ; JP nnnn
        ld    hl, irqRoutine.l22
        jr    .l7
.l5:    ld    a, (paletteSize)
        ld    b, a
        ld    a, (fieldHeight)
        ld    e, a
        ld    d, 0
        ld    h, d
        ld    l, d
.l6:    add   hl, de
        djnz  .l6
        ld    a, 001h                   ; LD BC, nnnn
.l7:    ld    (irqRoutine.l15), a
        ld    (irqRoutine.l15 + 1), hl
        ld    a, (paletteSize)
        neg
        add   a, c                      ; C = interlaceMode * 16
        ld    (irqRoutine.l19 + 1), a
        sub   c
        add   a, 8
        add   a, a
        ld    (irqRoutine.l17 + 1), a
        sub   irqRoutine.l20 - irqRoutine.l17
        ld    (irqRoutine.l20 + 1), a
        sub   irqRoutine.l21 - irqRoutine.l20
        ld    (irqRoutine.l21 + 1), a
        ret

irqRoutine:
        push  af
        push  hl
        ld    hl, spaceKeyState
        ld    a, 008h
        out   (0b5h), a
        in    a, (0b5h)                 ; is the space key pressed ?
        and   040h                      ; 000h: yes, 040h: no
        cp    (hl)
        rla
        rrca                            ; set bit 7 if state changed to pressed
        ld    (hl), a
        ld    hl, currentFrameRate
        ld    a, (frameTimer)           ; count 1/50s ticks
        inc   a
        jr    z, .l2
        ld    (frameTimer), a
        cp    (hl)
        jr    nc, .l2                   ; is it time to display a new field ?
.l1:    pop   hl
        ld    a, 030h
        out   (0b4h), a
        pop   af
        ei
        reti
.l2:    dec   a
        cp    (hl)                      ; carry is not set on buffer underrun
        ld    a, (fieldsBuffered)       ; are there any fields buffered ?
        dec   a
        inc   a
        jr    z, .l1                    ; not yet: buffer underrun
        dec   a
        ld    (fieldsBuffered), a
        jr    c, .l3                    ; on buffer underrun:
        inc   (hl)                      ; reduce playback speed
        jr    nz, .l3
        dec   (hl)
.l3:    push  bc
        push  de
  if VIDEO_BUFFER_32K != 0
        in    a, (0b1h)
        ld    (.l22 + 1), a             ; save memory paging,
  endif
        in    a, (0b2h)
        ld    (.l23 + 1), a
        in    a, (0b3h)
        ld    (.l24 + 1), a
        ld    (.l25 + 1), sp            ; and stack pointer
        ld    a, 0ffh
        out   (0b3h), a
        dec   a
        out   (0b2h), a
  if VIDEO_BUFFER_32K != 0
        dec   a
        out   (0b1h), a
  endif
        ld    hl, fieldsPlayed          ; update the number of fields played
        inc   (hl)                      ; (note: this is not actually used)
        jr    nz, .l4
        inc   hl
        inc   (hl)
.l4:    ld    d, 000h                   ; * D = interlaceMode - 1
        ld    hl, fieldNum
        ld    a, (hl)                   ; get field number
        and   d
        ld    e, a
        cpl                             ; calculate next field number
        xor   d                         ;   interlaceMode 1: 0,0,0,0,0,0,0,0...
        rra                             ;   interlaceMode 2: 0,1,0,1,0,1,0,1...
        xor   e                         ;   interlaceMode 4: 0,2,1,3,0,2,1,3...
        and   d
        ld    (hl), a
        ld    hl, (videoBufReadPos)     ; HL = address of field to be displayed
        set   7, h
  if VIDEO_BUFFER_32K != 0
        bit   6, h
        jr    z, $ + 4
        res   7, h
  else
        res   6, h
  endif
        ld    (.l13 + 1), hl
.l5:    ld    bc, 00000h                ; * BC = biasDataSize + paletteDataSize
        add   hl, bc                    ;   or JP .l6
.l6:    ld    a, e                      ; calculate LPT write address (DE)
        inc   a                         ; skip top border
        add   a, a
        add   a, a
        inc   a
        add   a, a
        add   a, a                      ; DE = address of LD1 pointer in the
        ld    e, a                      ; first LPB of the current field
        ld    d, high lptLine0Addr      ; assume LPT is aligned to 256 bytes
        add   a, 4
        ld    (.l16 + 1), a             ; store LPB palette address
.l7:    ld    b, 000h                   ; * B = fieldHeight + 1
.l8:    ld    sp, 00000h                ; * SP = (interlaceMode * 16) - 1
.l9:    ld    c, 000h                   ; * C = bytesPerLine
        ex    de, hl
        ld    a, e                      ; A = LD1 pointer to video buffer LSB
.l10:
  if VIDEO_BUFFER_32K != 0
        bit   6, d                      ; D = LD1 pointer to video buffer MSB
        jr    z, $ + 4
        res   7, d
  else
        res   6, d
  endif
.l11:   dec   b
        jr    z, .l12
        ld    (hl), a                   ; store in LPT
        inc   l
        ld    (hl), d
        add   hl, sp
        add   a, c
        jp    nc, .l11
        inc   d
        jp    .l10
.l12:   ld    e, a
        ex    de, hl
        xor   a
        ld    (frameTimer), a
        ld    (videoBufReadPos), hl     ; store new video buffer read position
.l13:   ld    hl, 00000h                ; * HL = data block start address
.l14:   ld    bc, 00000h                ; * BC = biasDataSize or JP .l15
        ld    a, (hl)                   ; read FIXBIAS
        add   hl, bc
        and   01fh
        out   (080h), a
  if VIDEO_BUFFER_32K == 0
        res   6, h
  endif
.l15:   ld    bc, 00000h                ; * BC = fieldHeight * paletteSize
.l16:   ld    de, lptLine0Addr          ;   or JP .l22
  if VIDEO_BUFFER_32K != 0
        ld    a, h
        rla
        rla
        sbc   a, a
        sub   2
        out   (0b1h), a
        xor   3
        out   (0b2h), a
        res   7, h
        set   6, h
  endif
.l17:   jr    .l18                      ; * JR +((8 - paletteSize) * 2)
.l18:   ldi                             ; copy palette data
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        jp    po, .l22
  if VIDEO_BUFFER_32K == 0
        res   6, h
  endif
.l19:   ld    a, 000h                   ; * A = (interlaceMode*16)-paletteSize
        add   a, e
        ld    e, a
.l20:   jr    nc, .l18                  ; * JR .l18 + ((8 - paletteSize) * 2)
        inc   d
.l21:   jr    .l18                      ; * JR .l18 + ((8 - paletteSize) * 2)
.l22:
  if VIDEO_BUFFER_32K != 0
        ld    a, 000h                   ; * restore page 1 segment
        out   (0b1h), a
  endif
.l23:   ld    a, 000h                   ; * restore page 2 segment
        out   (0b2h), a
.l24:   ld    a, 000h                   ; * restore page 3 segment
        out   (0b3h), a
        ld    a, 030h
        out   (0b4h), a
.l25:   ld    sp, 00000h                ; * restore stack pointer
        pop   de
        pop   bc
        pop   hl
        pop   af
        ei
        reti

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

nLengthSlots            equ 8
nOffs1Slots             equ 4
nOffs2Slots             equ 8
maxOffs3Slots           equ 32
totalSlots              equ nLengthSlots+nOffs1Slots+nOffs2Slots+maxOffs3Slots
; NOTE: the upper byte of the address of all table entries must be the same
slotBitsTable           equ decodeTablesBegin
slotBitsMSBCodeTable    equ slotBitsTable
slotBitsLSBCodeTable    equ slotBitsTable + 1
slotBaseLowTable        equ slotBitsTable + 2
slotBaseHighTable       equ slotBitsTable + 3
slotBitsTableL          equ slotBitsTable
  if LZ_FMT_MIN_LENGTH == 1
slotBitsTableO1         equ slotBitsTableL + (nLengthSlots * 4)
slotBitsTableO2         equ slotBitsTableO1 + (nOffs1Slots * 4)
slotBitsTableO3         equ slotBitsTableO2 + (nOffs2Slots * 4)
  endif
  if LZ_FMT_MIN_LENGTH == 2
slotBitsTableO2         equ slotBitsTableL + (nLengthSlots * 4)
slotBitsTableO3         equ slotBitsTableO2 + (nOffs2Slots * 4)
  endif
  if LZ_FMT_MIN_LENGTH == 3
slotBitsTableO3         equ slotBitsTableL + (nLengthSlots * 4)
  endif

decompressDataBlock:
        call  read8Bits                 ; read the number of symbols - 1 (BC)
        ld    b, a
        call  read8Bits
        ld    c, a
        ld    a, 040h
        call  readBits                  ; read flag bits
        srl   a
        push  af                        ; save last block flag (A=1,Z=0)
        jr    c, .l2                    ; is compression enabled ?
        inc   bc                        ; no, copy uncompressed literal data
        exx
        ld    bc, 00101h
        jr    .l10
.l2:    push  bc                        ; compression enabled:
        exx
        pop   bc                        ; store the number of symbols in BC'
        inc   c
        inc   b
        exx
        call  readDecodeTables
        exx
.l3:    sla   e
        jr    z, .l12
        jr    c, .l13                   ; LZ77 match or literal sequence ?
.l4:    ld    a, (hl)                   ; no, copy literal byte
        inc   l
        jr    z, .l8
.l5:    exx
        ld    (de), a
        inc   de
        exx
.l6:    dec   c
        jp    nz, .l3
.l7:    djnz  .l3
        exx
        pop   af                        ; return with last block flag
        ret                             ; (A=1,Z=0 if last block)
.l8:    call  readBlock
        jr    .l5
.l9:    exx
        call  read8Bits                 ; get literal sequence length - 17
        add   a, 17
        ld    c, a
        ld    b, 0
        rl    b
        exx
.l10:   push  hl
        exx
        pop   hl
        call  copyLiteralSequence
        cp    l
        jr    z, .l11
        push  hl
        exx
        pop   hl
        jp    .l6
.l11:   exx
        ld    l, a
        call  readBlock
        jp    .l6
.l12:   ld    e, (hl)
        inc   l
        call  z, readBlock
        rl    e
        jr    nc, .l4                   ; literal byte ?
.l13:   sla   e
        call  z, readByte
        ld    a, low slotBitsTableL
        jr    nc, copyLZMatch           ; LZ77 match / length slot 0 ?
        sla   e
        call  z, readByte
        ld    a, low (slotBitsTableL + 4)
        jr    nc, copyLZMatch           ; LZ77 match / length slot 1 ?
        sla   e
        call  z, readByte
        ld    a, low (slotBitsTableL + 8)
        jr    nc, copyLZMatch           ; LZ77 match / length slot 2 ?
        sla   e
        call  z, readByte
        ld    a, low (slotBitsTableL + 12)
        jr    nc, copyLZMatch           ; LZ77 match / length slot 3 ?
        sla   e
        call  z, readByte
        ld    a, low (slotBitsTableL + 16)
        jr    nc, copyLZMatch           ; LZ77 match / length slot 4 ?
        sla   e
        call  z, readByte
        ld    a, low (slotBitsTableL + 20)
        jr    nc, copyLZMatch           ; LZ77 match / length slot 5 ?
        sla   e
        call  z, readByte
        ld    a, low (slotBitsTableL + 24)
        jr    nc, copyLZMatch           ; LZ77 match / length slot 6 ?
        sla   e
        call  z, readByte
        jr    c, .l9                    ; literal sequence ?
        ld    a, low (slotBitsTableL + 28)

copyLZMatch:
        exx
; ----- inlined readEncodedValue ------
        ld    h, high slotBitsTableL    ; decode match length
        ld    l, a
        ld    a, (hl)
        inc   l
        ld    (.l4 - 34), a
        xor   a
        ld    b, a
        exx
        jr    .l4
.l1:    ld    e, (hl)
        rl    e
        inc   l
        jr    nz, .l5
        call  readBlock
        jr    .l5
.l2:    ld    e, (hl)
        rl    e
        inc   l
        jr    nz, .l6
        call  readBlock
        jr    .l6
.l3:    ld    e, (hl)
        rl    e
        inc   l
        jr    nz, .l7
        call  readBlock
        jr    .l7
.l4:    sla   e                         ; 15 bits: jump offset = 33
        call  z, readByte
        adc   a, a
        sla   e                         ; 14 bits: jump offset = 39
        call  z, readByte
        adc   a, a
        sla   e                         ; 13 bits: jump offset = 45
        call  z, readByte
        adc   a, a
        sla   e                         ; 12 bits: jump offset = 51
        call  z, readByte
        adc   a, a
        sla   e                         ; 11 bits: jump offset = 57
        call  z, readByte
        adc   a, a
        sla   e                         ; 10 bits: jump offset = 63
        call  z, readByte
        adc   a, a
        sla   e                         ; 9 bits: jump offset = 69
        call  z, readByte
        adc   a, a
        exx
        ld    b, a                      ; store MSB in B
        exx
        xor   a
        sla   e                         ; 8 bits: jump offset = 79
        call  z, readByte
        adc   a, a
        sla   e                         ; 7 bits: jump offset = 85
        call  z, readByte
        adc   a, a
        sla   e                         ; 6 bits: jump offset = 91
        call  z, readByte
        adc   a, a
        sla   e                         ; 5 bits: jump offset = 97
        call  z, readByte
        adc   a, a
        sla   e                         ; 4 bits: jump offset = 103
        call  z, readByte
        adc   a, a
        sla   e                         ; 3 bits: jump offset = 109
        jr    z, .l1
.l5:    adc   a, a
        sla   e                         ; 2 bits: jump offset = 114
        jr    z, .l2
.l6:    adc   a, a
        sla   e                         ; 1 bit: jump offset = 119
        jr    z, .l3
.l7:    adc   a, a
        exx                             ; 0 bits: jump offset = 124
        add   a, (hl)
        ld    c, a                      ; note: LSB is stored in C here
        ld    a, b
        inc   l
        adc   a, (hl)
        ld    b, a                      ; BC = match length
; -------------------------------------
  if LZ_MIN_LENGTH == 1
        jr    nz, .l9                   ; length >= 256 bytes ?
        ld    a, c
        dec   a
        jr    nz, .l8                   ; length > 1 byte ?
        ld    l, low slotBitsTableO1
        exx
        jp    .l15                      ; no, read 2 prefix bits
.l8:    dec   a
        jr    nz, .l9                   ; length > 2 bytes ?
        ld    l, low slotBitsTableO2
        exx
        jp    .l14                      ; no, read 3 prefix bits
  endif
  if LZ_MIN_LENGTH == 2
        jr    nz, .l9                   ; length >= 256 bytes ?
        ld    a, c
        sub   2
        jr    nz, .l9                   ; length != 2 bytes ?
        ld    l, low slotBitsTableO2
        exx
        jp    .l14                      ; no, read 3 prefix bits
  endif
.l9:    ld    l, low slotBitsTableO3
        xor   a
        exx                             ; length >= 3 bytes,
        jp    .l13                      ; variable prefix size
.l10:   ld    e, (hl)
        rl    e
        inc   l
        jr    nz, .l16
        call  readBlock
        jr    .l16
.l11:   ld    e, (hl)
        rl    e
        inc   l
        jr    nz, .l17
        call  readBlock
        jr    .l17
.l12:   sla   e                         ; read match offset prefix bits
        call  z, readByte
        adc   a, a
.l13:   sla   e
        call  z, readByte
        adc   a, a
.l14:   sla   e
        call  z, readByte
        adc   a, a
.l15:   sla   e
        jr    z, .l10
.l16:   adc   a, a
        sla   e
        jr    z, .l11
.l17:   adc   a, a
        exx
        add   a, a
        add   a, a
        add   a, l
; ----- inlined readEncodedValue ------
        ld    l, a                      ; decode match offset
        ld    a, (hl)
        inc   l
        ld    (.l21 - 34), a
        xor   a
        exx
        ld    d, a
        jr    .l21
.l18:   ld    e, (hl)
        rl    e
        inc   l
        jr    nz, .l22
        call  readBlock
        jr    .l22
.l19:   ld    e, (hl)
        rl    e
        inc   l
        jr    nz, .l23
        call  readBlock
        jr    .l23
.l20:   ld    e, (hl)
        rl    e
        inc   l
        jr    nz, .l24
        call  readBlock
        jr    .l24
.l21:   sla   e                         ; 15 bits: jump offset = 33
        call  z, readByte
        adc   a, a
        sla   e                         ; 14 bits: jump offset = 39
        call  z, readByte
        adc   a, a
        sla   e                         ; 13 bits: jump offset = 45
        call  z, readByte
        adc   a, a
        sla   e                         ; 12 bits: jump offset = 51
        call  z, readByte
        adc   a, a
        sla   e                         ; 11 bits: jump offset = 57
        call  z, readByte
        adc   a, a
        sla   e                         ; 10 bits: jump offset = 63
        call  z, readByte
        adc   a, a
        sla   e                         ; 9 bits: jump offset = 69
        call  z, readByte
        adc   a, a
        ld    d, a                      ; store MSB in D'
        ld    a, 0
        nop
        sla   e                         ; 8 bits: jump offset = 79
        call  z, readByte
        adc   a, a
        sla   e                         ; 7 bits: jump offset = 85
        call  z, readByte
        adc   a, a
        sla   e                         ; 6 bits: jump offset = 91
        call  z, readByte
        adc   a, a
        sla   e                         ; 5 bits: jump offset = 97
        call  z, readByte
        adc   a, a
        sla   e                         ; 4 bits: jump offset = 103
        call  z, readByte
        adc   a, a
        sla   e                         ; 3 bits: jump offset = 109
        jr    z, .l18
.l22:   adc   a, a
        sla   e                         ; 2 bits: jump offset = 114
        jr    z, .l19
.l23:   adc   a, a
        sla   e                         ; 1 bit: jump offset = 119
        jr    z, .l20
.l24:   adc   a, a
        exx                             ; 0 bits: jump offset = 124
        add   a, (hl)
        cpl
        inc   l
        ld    h, (hl)
        ld    l, a
        ld    a, h
        exx
        adc   a, d
        cpl
        exx
        ld    h, a
; -------------------------------------
        add   hl, de                    ; calculate LZ77 match read address
  if BUFFERING_MODE == 0
    if VIDEO_BUFFER_32K == 0
        set   7, h
        res   6, h
    else
        set   7, h
        ld    a, h
        adc   a, b
        jr    c, .l25
    endif
  endif
  if LZ_MIN_LENGTH > 1
        ldi
    if LZ_MIN_LENGTH > 2
        ldi
    endif
  endif
        ldir                            ; copy match data
        exx
        dec   c
        jp    nz, decompressDataBlock.l3    ; return to main decompress loop
        jp    decompressDataBlock.l7
  if (BUFFERING_MODE == 0) && (VIDEO_BUFFER_32K != 0)
.l25:   res   7, h
        in    a, (0b1h)
        ex    af, af'
        in    a, (0b3h)
        out   (0b1h), a
    if LZ_MIN_LENGTH > 1
        ldi
      if LZ_MIN_LENGTH > 2
        ldi
      endif
    endif
        ldir
        ex    af, af'
        out   (0b1h), a
        exx
        dec   c
        jp    nz, decompressDataBlock.l3
        jp    decompressDataBlock.l7
  endif

offs3ReadPrefixAddr     equ     copyLZMatch.l9 + 5

copyLiteralSequence:
        xor   a
        bit   1, l
        jr    z, .l1
        bit   0, l
        jr    z, .l4
        jr    .l5
.l1:    bit   0, l
        jr    nz, .l3
.l2:    ldi                             ; copy literal sequence
        ret   po
.l3:    ldi
        ret   po
.l4:    ldi
        ret   po
.l5:    ldi
        ret   po
        cp    l
        jp    nz, .l2
        exx
        ld    l, a
        call  readBlock
        push  hl
        exx
        pop   hl
        jp    .l2

readDecodeTables:
        ld    a, 040h
        call  readBits                  ; get prefix size for length >= 3 bytes
        ld    b, a
        add   a, a                      ; calculate jump address for reading
        add   a, b                      ; offset prefix bits
        add   a, a
        cpl
        add   a, low (copyLZMatch.l12 - 237)
        ld    l, a
        ld    a, 0
        adc   a, high (copyLZMatch.l12 - 237)
        ld    h, a
        ld    (offs3ReadPrefixAddr), hl
        inc   b
        ld    a, 002h
.l1:    add   a, a
        djnz  .l1
  if LZ_FMT_MIN_LENGTH == 1
        add   a, (totalSlots - maxOffs3Slots) - 3   ; total table size - 3
  endif
  if LZ_FMT_MIN_LENGTH == 2
        add   a, nLengthSlots + nOffs2Slots - 2     ; total table size - 2
  endif
  if LZ_FMT_MIN_LENGTH == 3
        add   a, nLengthSlots - 1                   ; total table size - 1
  endif
        ld    b, a                      ; store it in B
        push  de                        ; save decompressed data write address
        ld    de, slotBitsTableL        ; initialize decode tables
        ld    c, 255                    ; prevent LDIs from decrementing B
        ld    ix, LZ_FMT_MIN_LENGTH     ; set initial base value:
        jr    .l3                       ; it is minLength for length,
.l2:    ld    ix, 0                     ; and 0 for offsets
.l3:    ld    a, 010h
        call  readBits                  ; get the number of extra bits to read
        add   a, a
        add   a, a
        add   a, low bitShiftTable
        ld    l, a
        ld    a, 0
        adc   a, high bitShiftTable
        ld    h, a
        ldi                             ; read bits jump offset
        ld    a, ixl
        ld    (de), a                   ; store base value LSB
        inc   e
        add   a, (hl)                   ; calculate new base value
        inc   hl
        ld    ixl, a
        ld    a, ixh
        ld    (de), a                   ; store base value MSB
        inc   e
        inc   e
        adc   a, (hl)
        ld    ixh, a
        ld    a, e
  if LZ_FMT_MIN_LENGTH <= 1
        cp    low slotBitsTableO1
        jr    z, .l2                    ; end of length decode table ?
  endif
  if LZ_FMT_MIN_LENGTH <= 2
        cp    low slotBitsTableO2
        jr    z, .l2                    ; end of offset table for length=1 ?
  endif
        cp    low slotBitsTableO3
        jr    z, .l2                    ; end of offset table for length=2 ?
        djnz  .l3                       ; continue until all tables are read
        pop   de                        ; DE = decompressed data write address
        ret

read8Bits:
        ld    a, 001h

readBits:
        exx
.l1:    sla   e
        jr    z, .l2
        adc   a, a
        jp    nc, .l1
        exx
        ret
.l2:    ld    e, (hl)
        inc   l
        call  z, readBlock
.l3:    rl    e
        adc   a, a
        jp    nc, .l3
        exx
        ret

readByte:
        ld    e, (hl)
        rl    e
        inc   l
        ret   nz

readBlock:
        push  af
        inc   h
  if BUFFERING_MODE == 0
        jp    m, .l1
  else
        bit   6, h
        jr    nz, .l2
  endif
        pop   af
        ret
  if BUFFERING_MODE == 0
.l1:    inc   iy
        ld    a, (iy)
        or    a
        jr    z, .l2
        out   (0b1h), a
        ld    h, 040h
        pop   af
        ret
  endif
.l2:    push  bc
        push  de
        ld    a, 004h
        out   (0bfh), a
        ei
        call  loadVideoFile
        jp    nz, playVideoFileError
        ld    a, 00ch
        out   (0bfh), a
        pop   de
        pop   bc
  if BUFFERING_MODE == 0
        ld    iy, segmentTable
        ld    a, (iy)
        out   (0b1h), a
        ld    hl, 04000h + fileReadBufferBegin
  else
        ld    hl, fileReadBufferBegin
  endif
        pop   af
        ret

; -----------------------------------------------------------------------------

bitShiftTable:
        defb  124, 001h, 000h, 0,  119, 002h, 000h, 0
        defb  114, 004h, 000h, 0,  109, 008h, 000h, 0
        defb  103, 010h, 000h, 0,   97, 020h, 000h, 0
        defb   91, 040h, 000h, 0,   85, 080h, 000h, 0
        defb   79, 000h, 001h, 0,   69, 000h, 002h, 0
        defb   63, 000h, 004h, 0,   57, 000h, 008h, 0
        defb   51, 000h, 010h, 0,   45, 000h, 020h, 0
        defb   39, 000h, 040h, 0,   33, 000h, 080h, 0

; -----------------------------------------------------------------------------

lptTBorderLen           equ     00010h
lptVideoLineLen         equ     00010h
lptBBorderLen           equ     00020h
lptVBlankLen            equ     00040h
lptLine0Addr            equ     lptBaseAddr + lptTBorderLen

lptTBorder:
        defb  06dh, 002h, 03fh, 000h, 000h, 000h, 000h, 000h
        defb  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h

lptVideoLine:
verticalZoom:
        defb  0feh
videoMode:
        defb  02eh
leftMargin:
        defb  00fh
rightMargin:
        defb  02fh
        defw  0bf80h
        defw  00000h
paletteBuf:
        defb  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h

lptBBorder:
        defb  0ffh, 082h, 03fh, 000h, 000h, 000h, 000h, 000h
        defb  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
        defb  06eh, 002h, 03fh, 000h, 000h, 000h, 000h, 000h
        defb  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h

lptVBlank:
        defb  0fdh, 000h, 03fh, 000h, 000h, 000h, 000h, 000h
        defb  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
        defb  0feh, 000h, 006h, 03fh, 000h, 000h, 000h, 000h
        defb  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
        defb  0ffh, 000h, 03fh, 020h, 000h, 000h, 000h, 000h
        defb  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
        defb  0f4h, 003h, 006h, 03fh, 000h, 000h, 000h, 000h
        defb  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h

; -----------------------------------------------------------------------------

savedIRQRoutine:
        push  af
        scf
        jr    savedIRQRoutine + (00045h - 00038h)

iniFileName:
        defb  9
        defm  "IPLAY.INI"

exdosCommandName:
        defb  6
        defm  "EXDOS"
        defb  0fdh

fileCommandName:
        defb  7
        defm  "FILE "
        defw  fileNameBuffer

defaultFileName:
        defb  12
        defm  "VIDEODAT.BIN"

playerCodeEnd:

        assert  segmentTable == ((playerCodeEnd + 0ffh) & 0ff00h)
        dephase

