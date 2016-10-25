
LEFT_CHANNEL_ONLY       equ     0
moduleType              equ     's'

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
        defm  "SNDPLAY version 0.99\r\n"
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
        call  saveIRQRoutine            ; back up original EXOS IRQ routine
        ld    a, 0ffh
        out   (0b2h), a
        ld    hl, resetRoutine
        ld    (0bff8h), hl
        call  copyStatusLine
        ei
        ld    de, exdosCommandName      ; check if EXDOS is available
        exos  26
        cp    1
        sbc   a, a
        ld    (haveEXDOS), a
        ld    a, (loadingModule)
        or    a
        jr    z, .l1
        call  playAudioFile             ; if loading module: play a single file
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
        call  playAudioFile             ; play file
        ld    a, (inputFileChannel)
        exos  3                         ; close file,
        jr    .l2                       ; and continue with the next one

chooseInputFile:
        xor   a
        ld    (usingFILEExtension), a
        ld    de, fileCommandName       ; try to select a file
        exos  26                        ; with the 'FILE' extension
        ld    hl, fileNameBuffer
        or    a
        jr    nz, .l2
        inc   a
        ld    (usingFILEExtension), a
.l1:    ld    (fileNameBufferPos), hl
        ld    a, 1
        ld    (filesRemaining), a
        xor   a
        ret
.l2:    ld    hl, defaultFileName       ; if 'FILE' extension is not found:
        cp    0e5h                      ; use default name ("audiodat.bin")
        jr    nz, .l1
        or    a                         ; STOP key was pressed
        ret

loadINIFile:
        ld    a, (usingFILEExtension)
        or    a
        jp    nz, chooseInputFile
        ld    hl, fileNameBuffer        ; default to empty file name
        ld    (hl), a                   ; if EXDOS is not available
        ld    (fileNameBufferPos), hl
        inc   a
        ld    (filesRemaining), a
        ld    a, (haveEXDOS)
        or    a
        ret   z                         ; EXDOS not found ?
        ld    a, 1
        ld    de, iniFileName
        exos  1                         ; open .ini file
        or    a
        jp    nz, chooseInputFile       ; cannot open .ini file ?
        ld    hl, fileNameBuffer
        ld    (filesRemaining), a
.l1:    push  hl                        ; store position of length byte
        inc   l                         ; skip length byte
        jr    z, .l5                    ; buffer is full ?
.l2:    ld    a, 1                      ; skip whitespace
        exos  5
        or    a
        jr    nz, .l5                   ; EOF ?
        ld    a, b
        cp    021h
        jr    c, .l2
        ld    a, (filesRemaining)
        inc   a
        ld    (filesRemaining), a
.l3:    ld    (hl), b                   ; read and store file name
        inc   l
        jr    z, .l4                    ; until buffer is full,
        ld    a, 1
        exos  5
        or    a
        jr    nz, .l4                   ; EOF,
        ld    a, b
        cp    021h                      ; or whitespace
        jr    nc, .l3
        defb  011h                      ; LD DE, nnnn
.l4:    ld    b, 0ffh
        pop   de
        ld    a, l                      ; calculate and store name length
        scf
        sbc   a, e
        ld    (de), a
        ld    a, b
        cp    021h                      ; if not EOF yet,
        jr    c, .l1                    ; continue with next name
        defb  0feh                      ; CP nn
.l5:    pop   de
        ld    a, 1
        exos  3                         ; close .ini file
        xor   a
        ret

resetRoutine:
        di
        ld    sp, stackTop
        call  daveReset
        call  restoreIRQRoutine
        ld    a, 0ffh
        out   (0b2h), a
        ld    a, 01h
        out   (0b3h), a
        im    1
        ld    hl, (0bff4h)
        call  setLPTAddress
        ld    c, 40h
        exos  0
        ld    a, 6
        jp    0c00dh

setLPTAddress:
        ld    a, l
        ld    b, 4
.l1:    srl   h
        rra
        djnz  .l1
        out   (082h), a
        ld    a, h
        or    00ch
        out   (083h), a
        set   6, a
        out   (083h), a
        set   7, a
        out   (083h), a
        ret

; -----------------------------------------------------------------------------

readAudioFileHeader:
        ld    a, (loadingModule)
        or    a                         ; if loading a module, then the
        jr    nz, .l1                   ; file header has already been read
        ld    a, (inputFileChannel)
        ld    bc, 16
        ld    de, inputFileHeader
        exos  6
        or    a
        jr    nz, .l5
.l1:    ld    de, inputFileHeader
        ld    a, (de)                   ; byte 0: must be 000h
        inc   de
        or    a
        jr    nz, .l3
        ld    a, (de)                   ; byte 1: must be 073h ('s')
        inc   de
        cp    moduleType
        jr    nz, .l3
        ld    a, (de)                   ; sample format:
        inc   de
        ld    (sampleFormat), a
        cp    006h
        jr    c, .l4                    ; must be 06h, 07h,
        cp    085h                      ; 82h, 83h, or 84h
        jr    nc, .l4
        sub   008h
        cp    07ah
        jr    c, .l4
        ld    a, (de)                   ; number of channels:
        inc   de
        ld    (nChannels), a
        ld    hl, irqCodeBegin
        dec   a
        jr    z, .l2
        ld    hl, irqCode2Begin
.l2:    ld    (irqCodeAddr), hl
        cp    2
        jr    nc, .l4                   ; must be 1 or 2
        ld    a, (de)                   ; sample rate:
        inc   de
        ld    l, a
        ld    a, (de)
        inc   de
        ld    h, a
        ld    (sampleRate), hl
        ld    a, (de)
        inc   de
        or    a
        jr    nz, .l4                   ; must be less than 65536,
        or    h
        jr    z, .l4                    ; and greater than or equal to 256
        ld    a, (de)                   ; block size - 1:
        inc   de
        ld    (blockSizeM1), a
        cp    7
        jr    c, .l4                    ; block size must be >= 8,
        ld    b, a
        inc   a
        and   b
        jr    nz, .l4                   ; and power of two
        ld    a, (nChannels)
        dec   b
        add   a, b                      ; block size == 256 frames
        jr    c, .l4                    ; is not supported in stereo mode
        ld    a, (de)                   ; number of blocks:
        inc   de
        ld    l, a
        ld    a, (de)
        inc   de
        ld    h, a
        ld    a, (de)
        inc   de
        ld    (nBlocks), hl
        ld    (nBlocks + 2), a
        inc   de
        inc   de
        inc   de
        inc   de
        ld    a, (de)                   ; version byte: must be zero
        or    a
        jr    z, .l6
.l3:    ld    a, 0eeh                   ; .ITYPE
        defb  001h                      ; LD BC, nnnn
.l4:    ld    a, 0efh                   ; .ASCII
.l5:    add   a, 0ffh
        inc   a
        ret
.l6:    call  setupAudioDecoder
        ld    hl, (decoderRoutineAddr)
        ld    (decoderRoutineAddr2L), hl
        ld    (decoderRoutineAddr2R), hl
        xor   a
        ret

playAudioFile:
        di
        xor   a
        ld    (playAudioStatus), a
        ld    (splittingInputFile), a
        ld    (exitPlayAudioFile.l1 + 1), sp
        ei
        call  readAudioFileHeader
        jp    nz, playAudioFileError
        call  allocateMemory
        jp    nz, playAudioFileError
        di
        ld    a, 0c9h                   ; = RET
        ld    (00038h), a
        xor   a
        out   (0b4h), a
        call  createLPT
        ei
        call  loadAudioFile
        jp    nz, playAudioFileError
        di
        ld    hl, audioBuffer
        ld    (hl), 03fh
        ld    d, h
        ld    e, l
        inc   de
        ld    bc, 003ffh
        ldir
        ld    a, (nChannels)
        rrca
        rrca
        and   080h
        ld    iyl, a                    ; IY = audio buffer read position
        ld    iyh, high (audioBuffer + 00300h)
        exx
        ld    de, audioBuffer           ; DE' = audio buffer write position
        exx
        call  daveReset
        call  getDaveFrequency
        ld    de, (sampleRate)
        call  convertSampleRate
        or    00ch
        ld    (bfPortValue), a
        ld    (irqFrequencyCode), hl
        im    1
        call  initDAC
        ld    a, (nChannels)
        dec   a
        jp    nz, playAudio2Channels
.l1:    di
        ld    sp, (exitPlayAudioFile.l1 + 1)
        call  initAudioPlayback
.l2:    exx
        ld    a, d
.l3:    cp    iyh
        jr    z, .l3
.l4:    exx
        ld    hl, blocksRemaining
        dec   (hl)
        jr    z, .l7
.l5:    call  decodeBlock_U7            ; * call audio decoder routine
        exx
        ld    a, e
        or    a
        jp    nz, .l4
        ld    a, d
        inc   a
        and   003h
        or    high audioBuffer
        ld    d, a
        exx
        ld    hl, levelDisplayLAddr
        ld    a, ixl                    ; display last output sample (IXL)
        call  updateLevelDisplay
        ld    a, 8
        out   (0b5h), a
        in    a, (0b5h)
        add   a, a
        jp    m, .l2                    ; space key is not pressed ?
        di
.l6:    ld    a, 8                      ; wait until space key is released
        out   (0b5h), a
        in    a, (0b5h)
        add   a, a
        jp    p, .l6
        jr    playAudioFileDone
.l7:    inc   hl
        dec   (hl)
        jr    nz, .l5
        inc   hl
        dec   (hl)
        jr    nz, .l5
        ld    a, (splittingInputFile)
        or    a
        jr    z, .l1
        exx
        ld    a, d
        exx
.l8:    cp    iyh                       ; wait until the remaining audio data
        jr    z, .l8                    ; is played
.l9:    cp    iyh
        jr    nz, .l9

playAudioFileDone:
        xor   a                         ; return success

playAudioFileError:
        ld    (playAudioStatus), a

exitPlayAudioFile:
        di
.l1:    ld    sp, 00000h                ; * restore stack pointer
        call  daveReset
        call  restoreIRQRoutine
        ld    a, 0ffh
        out   (0b2h), a
        im    1
        ld    hl, (0bff4h)
        call  setLPTAddress
        ei
        call  freeAllMemory
        ld    a, (playAudioStatus)
        or    a
        ret

decoderRoutineAddr      equ     playAudioFile.l5 + 1

playAudio2Channels:
        di
        ld    sp, (exitPlayAudioFile.l1 + 1)
        call  initAudioPlayback
        ld    a, (blockSizeM1)
        xor   07fh
        ld    (.l4 + 5), a
.l1:    exx
        ld    a, d
.l2:    cp    iyh
        jr    z, .l2
.l3:    exx
        ld    hl, blocksRemaining
        dec   (hl)
        jr    z, .l7
.l4:    call  decodeBlock_U7            ; * left: call audio decoder routine
        exx
        ld    a, 0                      ; * A = (block size - 1) XOR 7Fh
        add   a, e
        ld    e, a
        exx
        ld    a, ixl
        ld    ixl, ixh                  ; IXL = previous sample (right channel)
        ld    ixh, a
.l5:    call  decodeBlock_U7            ; * right: call audio decoder routine
        ld    a, ixl
        ld    ixl, ixh                  ; IXL = previous sample (left channel)
        ld    ixh, a
        exx
        ld    a, e
        and   07fh
        ld    e, a
        jp    nz, .l3
        ld    a, d
        inc   a
        and   003h
        or    high audioBuffer
        ld    d, a
        exx
        ld    hl, levelDisplayLAddr     ; display last left output sample
        ld    a, ixl                    ; (IXL)
        call  updateLevelDisplay
        ld    hl, levelDisplayRAddr     ; display last right output sample
        ld    a, ixh                    ; (IXH)
        call  updateLevelDisplay
        ld    a, 8
        out   (0b5h), a
        in    a, (0b5h)
        add   a, a
        jp    m, .l1                    ; space key is not pressed ?
        di
.l6:    ld    a, 8                      ; wait until space key is released
        out   (0b5h), a
        in    a, (0b5h)
        add   a, a
        jp    p, .l6
        jp    playAudioFileDone
.l7:    inc   hl
        dec   (hl)
        jr    nz, .l4
        inc   hl
        dec   (hl)
        jr    nz, .l4
        ld    a, (splittingInputFile)
        or    a
        jr    z, playAudio2Channels
        exx
        ld    a, d
        exx
.l8:    cp    iyh                       ; wait until the remaining audio data
        jr    z, .l8                    ; is played
.l9:    cp    iyh
        jr    nz, .l9
        jp    playAudioFileDone         ; return success

decoderRoutineAddr2L    equ     playAudio2Channels.l4 + 1
decoderRoutineAddr2R    equ     playAudio2Channels.l5 + 1

; -----------------------------------------------------------------------------

allocateMemory:
        ld    hl, segmentTable
        in    a, (0b0h)
        ld    (hl), a
        inc   l
.l1:    exos  24
        or    a
        jr    nz, .l3
        ld    (hl), c
        inc   l
        jr    nz, .l1
.l2:    ld    a, 0f7h
        or    a
        ret
.l3:    ld    (hl), 0
        ld    h, a
        ld    a, l
        ld    (nSegments), a
        ld    a, h
        cp    07fh
        ret   nz                        ; not .SHARE ?
        inc   c
        ret   nz                        ; not segment FFh ?
        ld    h, d                      ; check EXOS boundary:
        ld    l, e
        ld    de, totalVideoDataSize
        or    a
        sbc   hl, de
        jr    c, .l2                    ; not enough space ?
        exos  23                        ; set user boundary
        or    a
        ret

freeAllMemory:
        ld    c, 0ffh
        exos  25
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

loadAudioFile:
        call  initProgressDisplay
        ld    hl, segmentTable
        ld    de, (fileReadBufferBegin)
        xor   a
        sub   e
        ld    c, a
        ld    a, 040h
        sbc   a, d
        ld    b, a
        set   6, d
        ld    a, (hl)
        inc   hl
        out   (0b1h), a
        ld    a, (inputFileChannel)
        exos  6
        or    a
        jr    nz, .l2
.l1:    call  displayLoadProgress
        ld    a, (hl)
        inc   hl
        or    a
        jr    z, .l3
        out   (0b1h), a
        ld    de, 04000h
        ld    bc, 04000h
        ld    a, (inputFileChannel)
        exos  6
        or    a
        jr    z, .l1
.l2:    push  af
        call  displayLoadProgress
        pop   af
        cp    0e4h                      ; EOF ?
        ret   nz
        xor   a
        ret
.l3:    ld    a, 1
        ld    (splittingInputFile), a
        xor   a
        ret

initAudioPlayback:
        di
        exx
        ld    hl, (fileReadBufferBegin)
        set   6, h
        dec   hl                        ; initialize data read position (HL')
.l1:    ld    a, d
        and   003h
        or    high audioBuffer
        ld    d, a
        ld    a, e
        or    a
        jr    z, .l3
        ld    a, 03fh
.l2:    ld    (de), a                   ; pad 256-sample block with silence
        inc   e
        jr    nz, .l2
        jr    .l1
.l3:    exx                             ; IXL = previous sample for L channel
        ld    ix, 03f3fh                ; IXH = previous sample for R channel
        ld    (segmentTablePos), a      ; A = 0
        ld    a, (segmentTable)
        out   (0b1h), a
        ld    hl, (nBlocks)
        inc   h
        inc   l
        ld    (blocksRemaining), hl
        ld    a, (nBlocks + 2)
        inc   a
        ld    (blocksRemaining + 2), a
        call  initLevelDisplay
        call  setupIRQRoutine
        ei
        ret

; -----------------------------------------------------------------------------

saveIRQRoutine:
        di
        ld    hl, irqRoutine
        ld    de, savedIRQRoutine
        ld    bc, irqCodeLength
        ldir
        ret

setupIRQRoutine:
        di
        ld    hl, (irqCodeAddr)
        ld    de, irqRoutine
        ld    bc, irqCodeLength
        ldir
        ld    a, 003h
        out   (0b4h), a
        ret

restoreIRQRoutine:
        di
        ld    hl, savedIRQRoutine
        ld    de, irqRoutine
        ld    bc, irqCodeLength
        ldir
        ret

irqCodeBegin:
        phase 00038h

irqRoutine:
        ex    af, af'
        ld    a, (iy)
        rrca
        out   (0a8h), a
  if LEFT_CHANNEL_ONLY == 0
        out   (0ach), a
  endif
        adc   a, 0
        out   (0abh), a
  if LEFT_CHANNEL_ONLY == 0
        out   (0afh), a
  endif
        ld    a, 003h
        inc   iyl
        jr    z, .l1
        out   (0b4h), a
        ex    af, af'
        ei
        ret
.l1:    inc   iyh
        out   (0b4h), a
        and   iyh
        or    high audioBuffer
        ld    iyh, a
        ex    af, af'
        ei
        ret

        dephase
irqCodeEnd:

irqCode2Begin:
        phase 00038h

        ex    af, af'
        ld    a, (iy - 128)
        rrca
        out   (0a8h), a
        adc   a, 0
        out   (0abh), a
        ld    a, (iy)
        rrca
        out   (0ach), a
        adc   a, 0
        out   (0afh), a
        ld    a, 003h
        inc   iyl
        jr    z, .l1
        out   (0b4h), a
        ex    af, af'
        ei
        ret
.l1:    ld    iyl, 080h
        inc   iyh
        out   (0b4h), a
        and   iyh
        or    high audioBuffer
        ld    iyh, a
        ex    af, af'
        ei
        ret

        dephase
irqCode2End:

irqCodeLength   equ     irqCode2End - irqCode2Begin

; -----------------------------------------------------------------------------

readBlock:
        push  af
        inc   h
        jp    m, .l1
        pop   af
        ret
.l1:    ld    hl, segmentTablePos
        inc   (hl)
        ld    l, (hl)
        ld    h, high segmentTable
        ld    a, (hl)
        or    a
        jr    z, .l2
        out   (0b1h), a
        ld    hl, 04000h
        pop   af
        ret
.l2:    di
        ld    a, 004h
        out   (0bfh), a
        push  bc
        push  de
        ld    a, (haveEXDOS)
        or    a
        call  z, daveReset
        call  restoreIRQRoutine
        ld    a, 0c9h                   ; = RET
        ld    (irqRoutine), a
        push  iy
        ei
        call  loadAudioFile
        jp    nz, playAudioFileError
        call  copyStatusLine
        call  initLevelDisplay
        call  setupIRQRoutine
        ld    a, (haveEXDOS)
        or    a
        call  z, initDAC
        pop   iy
        pop   de
        pop   bc
        ld    a, (bfPortValue)
        out   (0bfh), a
        xor   a
        ld    (segmentTablePos), a
        ld    a, (segmentTable)
        out   (0b1h), a
        ld    hl, (fileReadBufferBegin)
        set   6, h
        pop   af
        ei
        ret

; -----------------------------------------------------------------------------

setupAudioDecoder:
        ld    a, (sampleFormat)
        and   7
        cp    6
        push  af
        adc   a, 0fdh
        ld    l, a
        add   a, a
        add   a, l
        add   a, a
        ld    l, a
        ld    h, 0
        ld    de, decoderAddrTable
        add   hl, de
        ld    e, (hl)
        inc   hl
        ld    d, (hl)
        inc   hl
        ld    (decoderRoutineAddr), de
        ld    e, (hl)
        inc   hl
        ld    d, (hl)
        inc   hl
        ld    a, (blockSizeM1)
        ld    (de), a
        ld    e, (hl)
        inc   hl
        ld    d, (hl)
        ld    (fileReadBufferBegin), de
        pop   af
        ret   nc                        ; uncompressed format ?

readDiffTable:
        ld    a, (inputFileChannel)
        ld    de, playerDataEnd
        ld    bc, 2
        exos  6
        or    a
        jp    nz, playAudioFileError
        ld    hl, (playerDataEnd)
        ld    b, h
        ld    c, l
        ld    de, 03fffh - playerDataEnd
        dec   hl
        dec   hl
        sbc   hl, de
        jp    nc, playAudioFileError
        ld    de, playerDataEnd
        ld    a, (inputFileChannel)
        exos  6
        or    a
        jp    nz, playAudioFileError
        exx
        ld    hl, playerDataEnd
        ld    c, 080h
        ld    b, (hl)
        inc   hl
        ld    a, (hl)
        inc   hl
        ld    (.l1 + 1), a
        ld    a, b
        exx
        ld    c, a
        inc   c
        ld    a, (sampleFormat)
        cp    084h
        sbc   a, 080h                   ; 1, 2, or 4
        add   a, a
        add   a, a                      ; 4, 8, or 16
        ld    d, a                      ; D = decode table size
        ld    hl, diffTable
.l1:    ld    e, 0
        ld    b, d
.l2:    ld    a, 1                      ; read delta value
        exx
.l3:    sla   c
        jr    nz, .l4
        ld    c, (hl)
        inc   hl
        rl    c
.l4:    rla
        sla   c
        jr    nz, .l5
        ld    c, (hl)
        inc   hl
        rl    c
.l5:    jr    c, .l3
        dec   a
        exx
        add   a, e
        ld    e, a
        ld    (hl), a
        inc   hl
        djnz  .l2
        dec   c
        jr    nz, .l1
        ret

; -----------------------------------------------------------------------------

decodeBlock_U6:
        exx
.l1:    inc   l
        call  z, readBlock
        ld    a, (hl)
        rrca
        ld    c, a
        and   07eh
        ld    (de), a                   ; sample 0
        inc   e
        ld    a, c
        rrca
        rrca
        and   060h
        ld    c, a
        inc   l
        call  z, readBlock
        ld    a, (hl)
        rlca
        rlca
        rlca
        ld    b, a
        rlca
        rlca
        and   01eh
        or    c
        ld    (de), a                   ; sample 1
        inc   e
        ld    a, b
        and   078h
        ld    b, a
        inc   l
        call  z, readBlock
        ld    a, (hl)
        rlca
        ld    c, a
        rlca
        rlca
        and   006h
        or    b
        ld    (de), a                   ; sample 2
        inc   e
        ld    a, c
        and   07eh
        ld    (de), a                   ; sample 3
        inc   e
        inc   l
        call  z, readBlock
        ld    a, (hl)
        rrca
        ld    c, a
        and   07eh
        ld    (de), a                   ; sample 4
        inc   e
        ld    a, c
        rrca
        rrca
        and   060h
        ld    c, a
        inc   l
        call  z, readBlock
        ld    a, (hl)
        rlca
        rlca
        rlca
        ld    b, a
        rlca
        rlca
        and   01eh
        or    c
        ld    (de), a                   ; sample 5
        inc   e
        ld    a, b
        and   078h
        ld    b, a
        inc   l
        call  z, readBlock
        ld    a, (hl)
        rlca
        ld    c, a
        rlca
        rlca
        and   006h
        or    b
        ld    (de), a                   ; sample 6
        inc   e
        res   0, c
        ld    a, c
        ld    (de), a                   ; sample 7
        inc   e
        ld    a, e
.l2:    and   0ffh                      ; * block size - 1
        jp    nz, .l1
        ld    ixl, c
        exx
        ret

; -----------------------------------------------------------------------------

decodeBlock_U7:
        exx
.l1:    inc   l
        call  z, readBlock
        ld    a, (hl)
        rrca
        ld    (de), a                   ; sample 0
        inc   e
        rrca
        and   040h
        ld    c, a
        inc   l
        call  z, readBlock
        ld    a, (hl)
        rrca
        rrca
        ld    b, a
        and   03fh
        or    c
        ld    (de), a                   ; sample 1
        inc   e
        ld    a, b
        rrca
        and   060h
        ld    b, a
        inc   l
        call  z, readBlock
        ld    a, (hl)
        rrca
        rrca
        rrca
        ld    c, a
        and   01fh
        or    b
        ld    (de), a                   ; sample 2
        inc   e
        ld    a, c
        rrca
        and   070h
        ld    c, a
        inc   l
        call  z, readBlock
        ld    a, (hl)
        rlca
        rlca
        rlca
        rlca
        ld    b, a
        and   00fh
        or    c
        ld    (de), a                   ; sample 3
        inc   e
        ld    a, b
        rrca
        and   078h
        ld    b, a
        inc   l
        call  z, readBlock
        ld    a, (hl)
        rlca
        rlca
        rlca
        ld    c, a
        and   007h
        or    b
        ld    (de), a                   ; sample 4
        inc   e
        ld    a, c
        rrca
        and   07ch
        ld    c, a
        inc   l
        call  z, readBlock
        ld    a, (hl)
        ld    b, a
        rlca
        rlca
        and   003h
        or    c
        ld    (de), a                   ; sample 5
        inc   e
        inc   l
        call  z, readBlock
        ld    a, (hl)
        ld    c, a
        rla
        ld    a, b
        rla
        ld    (de), a                   ; sample 6
        inc   e
        ld    a, c
        ld    (de), a                   ; sample 7
        inc   e
        ld    a, e
.l2:    and   0ffh                      ; * block size - 1
        jp    nz, .l1
        ld    ixl, c
        exx
        ret

; -----------------------------------------------------------------------------

decodeBlock_ADPCM2:
        exx
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        rlca
        rlca
        ld    h, a
        and   0fch
        ld    l, a
        xor   h
        add   a, high diffTable
        ld    h, a
        ld    de, decodeTable
        ldi
        ldi
        ldi
        ldi

decodeSamples_ADPCM2:
        ld    h, d
        ld    b, 3
        ld    c, ixl
.l1:    exx
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        ld    e, a
        rlca                            ; sample 0
        rlca
        ld    d, a
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, d                      ; sample 1
        rlca
        rlca
        ld    d, a
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, d                      ; sample 2
        rlca
        rlca
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, e                      ; sample 3
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        ld    e, a
        rlca                            ; sample 4
        rlca
        ld    d, a
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, d                      ; sample 5
        rlca
        rlca
        ld    d, a
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, d                      ; sample 6
        rlca
        rlca
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, e                      ; sample 7
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        ld    a, e
        exx
.l2:    and   0ffh                      ; * block size - 1
        jp    nz, .l1
        ld    ixl, c
        ret

; -----------------------------------------------------------------------------

decodeBlock_ADPCM3:
        exx
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        rlca
        rlca
        rlca
        ld    h, a
        and   0f8h
        ld    l, a
        xor   h
        add   a, high diffTable
        ld    h, a
        ld    de, decodeTable
        ld    bc, 8
        ldir

decodeSamples_ADPCM3:
        ld    h, d
        ld    b, 7
        ld    c, ixl
.l1:    exx
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        ld    d, a
        rlca                            ; sample 0
        rlca
        rlca
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, d
        rrca                            ; sample 1
        rrca
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        rlca                            ; sample 2
        ld    e, a
        ld    a, d
        rla
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, e
        rlca                            ; sample 3
        rlca
        rlca
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, e
        rrca                            ; sample 4
        rrca
        ld    e, a
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        ld    d, a
        sla   e                         ; sample 5
        rla
        rla
        rla
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, d
        rrca                            ; sample 6
        rrca
        rrca
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, d
        and   b                         ; sample 7
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        ld    a, e
        exx
.l2:    and   0ffh                      ; * block size - 1
        jp    nz, .l1
        ld    ixl, c
        ret

; -----------------------------------------------------------------------------

decodeBlock_ADPCM4:
        exx
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        rlca
        rlca
        rlca
        rlca
        ld    h, a
        and   0f0h
        ld    l, a
        xor   h
        add   a, high diffTable
        ld    h, a
        ld    de, decodeTable
        ld    bc, 16
        ldir

decodeSamples_ADPCM4:
        ld    h, d
        ld    b, 15
        ld    c, ixl
.l1:    exx
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        ld    d, a
        rlca                            ; sample 0
        rlca
        rlca
        rlca
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, d                      ; sample 1
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        ld    d, a
        rlca                            ; sample 2
        rlca
        rlca
        rlca
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, d                      ; sample 3
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        ld    d, a
        rlca                            ; sample 4
        rlca
        rlca
        rlca
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, d                      ; sample 5
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        inc   l
        call  z, readBlock
        ld    a, (hl)
        exx
        ld    d, a
        rlca                            ; sample 6
        rlca
        rlca
        rlca
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        exx
        ld    a, d                      ; sample 7
        and   b
        ld    l, a
        ld    a, (hl)
        add   a, c
        ld    c, a
        exx
        ld    (de), a
        inc   e
        ld    a, e
        exx
.l2:    and   0ffh                      ; * block size - 1
        jp    nz, .l1
        ld    ixl, c
        ret

; -----------------------------------------------------------------------------

getDaveFrequency:
        di
        ld    a, 7
        out   (0a0h), a                 ; 31250 Hz
        xor   a
        out   (0a1h), a
        ld    a, 040h
        out   (0a7h), a
        ld    hl, 0
        ld    bc, 000b4h
        ld    de, 03320h
        out   (c), d
.l1:    in    a, (0b4h)
        and   e
        jr    z, .l1
        out   (c), d
        ld    d, 013h
.l2:    in    a, (0b4h)
        bit   1, a
        jr    z, .l3
        out   (c), d
        inc   hl
.l3:    and   e
        jp    z, .l2
        ld    a, 03ch
        out   (0b4h), a
        ex    de, hl                    ; with an LPT length of 312 lines,
        ld    bc, 15625                 ; DE should be 624 for 8 MHz clock freq
        call  mulDEByBCToHLIX
        ld    bc, 312
        add   ix, bc
        jr    nc, .l4
        inc   hl
.l4:    ld    bc, 624
        call  divHLIXByBCToDE
        ex    de, hl                    ; return clock frequency / 512 in HL
        ret

; input:
;   HL: Dave input clock frequency / 512
;   DE: sample rate in Hz
; output:
;   HL: sound generator frequency code
;   A:  bit 1 of value to be written to port BFh

convertSampleRate:
        ld    a, d
        or    e
        jr    nz, .l1
        ld    hl, 4095
        ld    a, 2
        ret
.l1:    push  de
        push  hl
        call  findNearestFrequency
        pop   ix
        pop   de
        push  bc
        push  hl
        push  de
        ld    hl, 0                     ; calculate input clock frequency / 768
        inc   ix
        add   ix, ix
        adc   hl, hl
        ld    bc, 3
        call  divHLIXByBCToDE
        ld    h, d
        ld    l, e
        pop   de
        call  findNearestFrequency
        pop   de
        xor   a
        sbc   hl, de
        pop   hl
        ret   nc
        ld    h, b
        ld    l, c
        ld    a, 2
        ret

findNearestFrequency:
        push  de                        ; input arguments:
        push  de                        ;   DE = sample rate
        ld    d, h                      ;   HL = sound clock frequency / 16
        ld    e, l
        ld    bc, 16
        call  mulDEByBCToHLIX
        pop   bc
        push  hl
        push  ix
        call  divHLIXByBCToDE
        pop   ix
        pop   hl
        ld    b, d                      ; BC = DAVE frequency code + 1
        ld    c, e
        pop   de                        ; DE = sample rate
        push  bc
        call  testFrequency
        ex    (sp), hl
        push  hl
        push  bc
        pop   hl
        pop   bc
        ex    (sp), hl
        push  bc
        inc   bc
        call  testFrequency
        ld    h, b
        ld    l, c
        pop   bc                        ; BC = DAVE frequency code
        pop   de
        scf
        sbc   hl, de
        jr    nc, .l1
        adc   hl, de                    ; HL = frequency error in Hz
        ret
.l1:    ld    h, d
        ld    l, e
        dec   bc
        ret

; input arguments:
;   HL:IX:  DAVE audio clock frequency (input clock frequency / 32 or 48)
;   DE:     sample rate
;   BC:     DAVE frequency code + 1
; output argument:
;   BC:     absolute value of frequency error in Hz

testFrequency:
        push  hl
        push  ix
        push  de
        call  divHLIXByBCToDE
        pop   hl
        push  hl
        or    a
        sbc   hl, de
        ld    b, h
        ld    c, l
        pop   de
        pop   ix
        pop   hl
        ret   nc
        ld    a, b
        cpl
        ld    b, a
        ld    a, c
        cpl
        ld    c, a
        inc   bc
        ret

initDAC:
        call  daveReset
        ld    a, (bfPortValue)
        out   (0bfh), a
        call  .waitLoop
        ld    a, 07fh
        out   (0a0h), a
        out   (0a2h), a
        out   (0a4h), a
        call  .waitLoop
        ld    a, 007h                   ; reset all oscillators
        out   (0a7h), a                 ; to zero phase and output state
        call  .waitLoop
        ld    a, 080h
        out   (0a1h), a                 ; channel 0 output is constant '1'
        ld    a, 00eh
        out   (0a6h), a                 ; 9-bit noise generator, clock is chn 1
        ld    b, 14
.l1:    call  .waitLoop
        ld    a, 080h                   ; channel 3 clock is -(chan. 3 output)
        out   (0a3h), a
        call  .waitLoop
        ld    a, 000h                   ; set channel 3 clock to '0'
        out   (0a3h), a                 ; channel 3 output should be '1'
        djnz  .l1                       ; after this loop
        call  .waitLoop
        ld    a, 00fh
        out   (0a6h), a                 ; channel 3 clock is chan. 2 output (0)
        call  .waitLoop
        ld    hl, (irqFrequencyCode)    ; set DAC sample rate
        ld    a, l
        out   (0a2h), a
        ld    a, h
        out   (0a3h), a
        call  .waitLoop
        ld    a, 065h                   ; set OSC1 as IRQ clock, and start it
        out   (0a7h), a
.waitLoop:
        ld    a, 64
.l2:    dec   a
        jr    nz, .l2
        ret

daveReset:
        ld    a, 004h
        out   (0bfh), a
        xor   a
        ld    bc, 000afh
.l1:    out   (c), a
        dec   c
        bit   5, c
        jr    nz, .l1
        ld    a, 03ch
        out   (0b4h), a
        ret

; -----------------------------------------------------------------------------

createLPT:
        xor   a
        out   (081h), a
        dec   a
        out   (0b2h), a
        ld    hl, (0bff4h)              ; copy status line LPB
        ld    de, 08000h
        ld    bc, 16
        ldir
        ld    a, 0f7h
        ld    (08000h), a
        ld    hl, lptData               ; decode and copy LPT data
.l1:    ld    a, (hl)
        inc   hl
        ld    (de), a
        inc   de
        or    a
        jr    nz, .l2
        ld    c, (hl)
        inc   hl
        dec   c
        jr    z, .l2
        push  hl
        ld    h, d
        ld    l, e
        dec   hl
        ldir
        pop   hl
.l2:    ld    a, l
        cp    low lptDataEnd
        ld    a, h
        sbc   a, high lptDataEnd
        jr    c, .l1
        call  copyStatusLine
        ld    hl, 0c000h
        call  setLPTAddress

initLevelDisplay:
        ld    a, 0ffh
        out   (0b2h), a
        ld    a, 10
        ld    (levelDisplayLLPBAddr + 2), a
        inc   a
        ld    (levelDisplayLAddr), a    ; set default right margins (level = 0)
        ld    (levelDisplayRAddr), a
        ld    a, 04eh                   ; video mode: 16-color LPIXEL
        ld    (levelDisplayLLPBAddr + 1), a
        ld    hl, ld1Addr1M1
        ld    (levelDisplayLLPBAddr + 4), hl
        xor   a                         ; palette color 0: black
        ld    (levelDisplayLLPBAddr + 8), a
        ld    a, 092h                   ; palette color 1: green
        ld    (levelDisplayLLPBAddr + 9), a
        ld    hl, levelDisplayData
        ld    de, ld1Addr1 & 0bfffh
.l1:    ld    a, (hl)
        or    a
        ret   z
        ld    c, a
        ld    b, 0
        inc   hl
        push  de
        ldi
        ex    (sp), hl
        ldir
        pop   hl
        jr    .l1

copyStatusLine:
        ld    a, 0ffh
        out   (0b2h), a
        ld    hl, (0bff6h)
        push  hl
        ld    (hl), 020h
        ld    d, h
        ld    e, l
        inc   de
        ld    c, 39
        ldir
        pop   hl
        ld    c, low ((40 - (statusLineDataEnd - statusLineData)) / 2)
        add   hl, bc
        ld    d, h
        ld    e, l
        ld    hl, statusLineData
        ld    c, low (statusLineDataEnd - statusLineData)
        ldir
        ret

; input arguments:
;   A:  sample value
;   HL: right margin address (on page 2)

updateLevelDisplay:
        and   07fh
        sub   07fh
        jr    z, .l1
        add   040h
        jr    c, .l1
        neg
.l1:    srl   a
        ld    c, a
        adc   a, 11
        srl   c
        srl   c
        adc   a, c
        ld    c, a
        ld    a, 0ffh
        out   (0b2h), a
        ld    b, (hl)
        ld    a, c
        cp    b
        jr    nc, .l2
        add   a, b
        add   a, b
        add   a, b
        rrca
        rrca
        and   03fh
.l2:    ld    (hl), a
        ret

initProgressDisplay:
        ld    a, 0ffh
        out   (0b2h), a
        ld    a, 00eh                   ; 2-color LPIXEL mode
        ld    (levelDisplayLLPBAddr + 1), a
        ld    a, 51
        ld    (levelDisplayLAddr), a
        ld    a, 11
        ld    (levelDisplayLLPBAddr + 2), a
        ld    (levelDisplayRAddr), a    ; clear level display
        ld    hl, ld1Addr1
        ld    (levelDisplayLLPBAddr + 4), hl
        ld    a, 0e0h                   ; palette color 0
        ld    (levelDisplayLLPBAddr + 8), a
        ld    a, 02eh                   ; palette color 1
        ld    (levelDisplayLLPBAddr + 9), a
        ld    hl, segmentTable

displayLoadProgress:
        push  hl
        push  ix
        ld    bc, segmentTable
        or    a
        sbc   hl, bc
        ex    de, hl
        ld    bc, 320
        call  mulDEByBCToHLIX
        ld    a, (nSegments)
        ld    c, a
        ld    b, 0
        call  divHLIXByBCToDE
        pop   ix
        ex    de, hl                    ; HL = number of segments loaded * 320
        ld    a, 0ffh                   ; / total number of segments allocated
        out   (0b2h), a
        ld    de, ld1Addr1 & 0bfffh
        ld    c, 39
        ld    a, l
        srl   h
        rra
        rra
        rra
        and   03fh
        jr    z, .l2
        ld    b, a
        ld    a, 0ffh
.l1:    ld    (de), a
        inc   de
        dec   c
        djnz  .l1
.l2:    ld    a, l
        and   7
        jr    z, .l5
        ld    b, a
        xor   a
.l3:    scf
        rra
        djnz  .l3
.l4:    ld    (de), a
        inc   de
        dec   c
        xor   a
.l5:    bit   7, c
        jr    z, .l4
        pop   hl
        ret

; -----------------------------------------------------------------------------

decoderAddrTable:
        defw  decodeBlock_ADPCM2, decodeSamples_ADPCM2.l2 + 1
        defw  diffTable + 00400h
        defw  decodeBlock_ADPCM3, decodeSamples_ADPCM3.l2 + 1
        defw  diffTable + 00800h
        defw  decodeBlock_ADPCM4, decodeSamples_ADPCM4.l2 + 1
        defw  diffTable + 01000h
        defw  decodeBlock_U6, decodeBlock_U6.l2 + 1, diffTable
        defw  decodeBlock_U7, decodeBlock_U7.l2 + 1, diffTable

; NOTE: the LPT data is stored with simple RLE compression:
; a sequence of N zero bytes is replaced with a zero byte followed by N.
; The last zero byte of the LPT is also the first byte of the level display.

lptData:
        defb  256 - 9, 002h, 63,  0, 13
        defb  256 - 9, 04eh, 10, 11,  low ld1Addr1M1, high ld1Addr1M1,  0, 2
        defb  0, 1,  092h, 0dah, 0d3h, 0dbh, 0cbh, 0d9h, 049h
        defb  256 - 9, 002h, 63,  0, 13
        defb  256 - 9, 04eh, 10, 11,  low ld1Addr1M1, high ld1Addr1M1,  0, 2
        defb  0, 1,  092h, 0dah, 0d3h, 0dbh, 0cbh, 0d9h, 049h
        defb  256 - 221, 002h, 63,  0, 13
        defb  256 - 3, 080h, 63,  0, 13
        defb  256 - 4,  0, 1,  6, 63,  0, 12
        defb  256 - 1,  0, 1,  63, 32,  0, 12
        defb  256 - 10, 002h, 6, 63,  0, 12
        defb  256 - 28, 003h, 63,  0, 13
lptDataEnd:

lptDataLPBCnt           equ     10      ; not including the status line
lptSize                 equ     (lptDataLPBCnt + 1) * 16
ld1Addr1                equ     0c000h + lptSize
ld1Addr1M1              equ     ld1Addr1 - 1
totalVideoDataSize      equ     lptSize + 40

levelDisplayData:
        defb  7, 040h, 7, 004h, 6, 044h, 6, 010h, 5, 050h, 5, 014h, 4, 054h
        defb  0

levelDisplayLLPBAddr    equ     08020h
levelDisplayRLPBAddr    equ     08040h
levelDisplayLAddr       equ     levelDisplayLLPBAddr + 3
levelDisplayRAddr       equ     levelDisplayRLPBAddr + 3

statusLineData:
        abyte 080h "SNDPLAY"
        defm  " version 0.99"
statusLineDataEnd:

iniFileName:
        defb  11
        defm  "SNDPLAY.INI"

exdosCommandName        equ     @IView.EXDFD

fileCommandName:
        defb  7
        defm  "FILE "
        defw  fileNameBuffer

defaultFileName:
        defb  12
        defm  "AUDIODAT.BIN"

playerCodeEnd:

        dephase

stackTop                equ     00100h
segmentTable            equ     (playerCodeEnd + 0ffh) & 0ff00h
decodeTable             equ     segmentTable + 00100h
variablesBegin          equ     decodeTable + 020h
inputFileHeader         equ     variablesBegin
irqCodeAddr             equ     inputFileHeader + 16
nSegments               equ     irqCodeAddr + 2
segmentTablePos         equ     nSegments + 1
fileReadBufferBegin     equ     segmentTablePos + 1
sampleFormat            equ     fileReadBufferBegin + 2
nChannels               equ     sampleFormat + 1
sampleRate              equ     nChannels + 1
blockSizeM1             equ     sampleRate + 3
nBlocks                 equ     blockSizeM1 + 1
haveEXDOS               equ     nBlocks + 3
blocksRemaining         equ     haveEXDOS + 1
bfPortValue             equ     blocksRemaining + 3
irqFrequencyCode        equ     bfPortValue + 1
loadingModule           equ     irqFrequencyCode + 2
inputFileChannel        equ     loadingModule + 1
fileNameBufferPos       equ     inputFileChannel + 1
filesRemaining          equ     fileNameBufferPos + 2
splittingInputFile      equ     filesRemaining + 1
playAudioStatus         equ     splittingInputFile + 1
usingFILEExtension      equ     playAudioStatus + 1
savedIRQRoutine         equ     usingFILEExtension + 1
fileNameBuffer          equ     decodeTable + 00100h
audioBuffer             equ     (fileNameBuffer + 00100h + 003ffh) & 0fc00h
diffTable               equ     audioBuffer + 00400h
playerDataEnd           equ     diffTable + 01000h

        assert  (savedIRQRoutine + irqCodeLength) <= fileNameBuffer

