
BUILDING_CVIEW_COM      equ     ($ < 0c000h)

    if BUILDING_CVIEW_COM != 0
CVIEW_BUFFER_SIZE       equ     1000h
BUILD_EXTENSION_ROM     equ     0
    endif

        assert  (CVIEW_BUFFER_SIZE >= 0100h) && (CVIEW_BUFFER_SIZE <= 2000h)
        assert  (CVIEW_BUFFER_SIZE & 0ffh) == 0

    if BUILDING_CVIEW_COM == 0

loaderCodeBegin:

loadModule:
        ld    ixh, d
        ld    ixl, e
        push  bc
        call  copyViewer
        pop   af
        ld    (imageFileChannel), a
        jp    loadIViewModule

parseCommand:
        ld    hl, iviewCommandMain      ; copyViewer clears all variables,
        push  hl                        ; imageFileChannel will be zero

copyViewer:
        ld    hl, loaderCodeEnd
        ld    de, viewerCodeBegin
        ld    bc, viewerCodeEnd - viewerCodeBegin
        ldir
        ld    l, e
        ld    h, d
        inc   de
        ld    (hl), c
        ld    bc, decompressorDataEnd - (viewerCodeEnd + 1)
        ldir
    if BUILD_EXTENSION_ROM == 0
        in    a, (0b3h)
        cp    0fdh                      ; running as extension on an EP64?
        ret   nz
        ld    (segmentTable + 1), a
        ld    a, 21h                    ; = LD HL, nnnn
        ld    (keyboardWait.mouseInput1), a
        ld    (keyboardWait.mouseInput2), a
        ld    a, 37h                    ; = SCF
        ld    (keyboardWait.l8), a
    endif
        ret

shortHelpString:
        defm  "IVIEW version 0.41c (CVIEW)\r\n"
        defb  00h
longHelpString:
        defm  "Image viewer for converted PC images.\r\n"
        defm  "Written by Zozosoft, 2008\r\n"
        defm  "Compressed format support and\r\n"
        defm  "converter program by IstvanV\r\n"
        defb  00h

loaderCodeEnd:

    else

      macro exos n
        rst   30h
        defb  n
      endm

      macro EXOS N
        rst   30h
        defb  N
      endm

        output  "cview.com"
        org     00f0h
        defw    0500h, viewerCodeEnd - iviewCommandMain, 0, 0, 0, 0, 0, 0

    endif

; =============================================================================

        phase 0100h

viewerCodeBegin:

iviewCommandMain:
    if BUILDING_CVIEW_COM == 0
        ld    c, 60h
        call  exosReset_SP0100
    else
        di
        ld    sp, 3800h
        ld    c, 40h
        exos  0
        di
        ld    a, 0ffh
        out   (0b2h), a
    endif
        ld    hl, resetRoutine          ; set up warm reset address
        ld    (0bff8h), hl
    if BUILDING_CVIEW_COM != 0
        ld    hl, viewerCodeEnd
        ld    de, viewerCodeEnd + 1
        ld    bc, decompressorDataEnd - (viewerCodeEnd + 1)
        ld    (hl), 00h
        ldir
        ld    sp, 0100h
        ei
    endif
        ld    de, exdosCommandName      ; check if EXDOS is available
        exos  26
        ld    (exdosCommandStatus), a   ; 0: yes, non-zero: no
        jr    nz, .l2
        ld    de, iviewIniFileName      ; if EXDOS is available,
.l1:    ld    a, 1                      ; load IVIEW.INI file if it is present
        exos  1
.l2:    ld    (iviewIniMissing), a      ; 0: have IVIEW.INI file
.l3:    ld    a, (iviewIniMissing)
        or    a
        call  z, readIViewIni
        jr    z, .l6
.l4:    ld    a, (exdosCommandStatus)   ; if no IVIEW.ini is available,
        or    a
        jr    nz, .l5                   ; but EXDOS is present...
        inc   a
        exos  3                         ; close any previously opened file
        ld    de, fileCommandName       ; ...then use the FILE extension
        exos  26                        ; to select an image file
        jr    nz, .l7                   ; no FILE extension?
        ld    de, fileNameBuffer
        inc   a
        exos  1                         ; open input file
        ld    a, 1
        exos  5                         ; check first byte (must be zero)
        or    b
        push  af
        ld    a, 1
        exos  3                         ; close input file
        pop   af
        jr    z, .l6                    ; first byte is zero?
        ld    de, fileNameBuffer        ; no, assume IVIEW.INI format
        jr    .l1                       ; and try again
.l5:    xor   a
        ld    (fileNameBuffer), a       ; no EXDOS, use empty file name
.l6:    ld    a, 256 - (302 / 2)
        ld    (lptTBorder), a
        ld    (lptBBorder), a
        xor   a                         ; image file channel
        ld    (imageFileChannel), a
        ld    de, fileNameBuffer
        exos  1                         ; open image file
        cp    0a6h
        jp    z, resetRoutine           ; "Invalid file name" from FILE: ?
        or    a
        jr    nz, doneImage             ; open failed?
        ld    de, fileHeaderBuf
        ld    bc, 16
        exos  6                         ; read header (16 bytes):
.l7:    jp    nz, resetRoutine          ; exit on error
        ex    af, af'
        ld    ix, fileHeaderBuf         ; IX = address of file header buffer
        ld    a, (ix + 1)               ; check file type:
        cp    0bh
        jp    z, readVLoadHeader        ; VLOAD?
        cp    49h
        jr    nz, unknownFileFormat     ; if not IVIEW either, then error
        call  loadIViewImage

doneImage:
        ex    af, af'                   ; save error code
        ld    a, 0ffh                   ; restore memory paging
        out   (0b2h), a
        ld    hl, (0bff4h)              ; restore LPT
        set   6, h
        call  setLPTAddress
        call  freeAllMemory             ; clean up

unknownFileFormat:
        xor   a
        exos  3                         ; close input file
        ex    af, af'                   ; get error code
        cp    0e5h                      ; .STOP ?
        jp    nz, iviewCommandMain.l3   ; no, continue with next image
        ld    a, 1                      ; yes, close IVIEW.INI,
        exos  3
        jp    iviewCommandMain.l4       ; and return to FILE, or exit (reset)

    if BUILDING_CVIEW_COM != 0
        module  IView
        include "iviewinc.s"
        endmod
    endif

readIViewIni:
        ld    hl, fileNameBuffer        ; read next entry from IVIEW.INI
        jp    @IView.INIREADHL

loadIViewModule:
    if BUILDING_CVIEW_COM == 0
        ld    c, 40h
        call  exosReset_SP0100
        ld    hl, resetRoutine
        ld    (0bff8h), hl
        ei
        call  loadIViewImage
        di
        ld    a, 0ffh
        out   (0b2h), a
        ld    hl, resetRoutine
        ld    (0bff8h), hl
        call  freeAllMemory
    endif
.l1:    di
        xor   a
        out   (0b1h), a
        out   (0b2h), a
        out   (0b3h), a
        jp    0c000h

setLPTAddress:
        ld    a, 1ch
.l1:    add   hl, hl
        rla
        jr    nc, .l1
        ld    c, 82h
        out   (c), h
        out   (83h), a
        ret

resetRoutine:
        ld    sp, 0100h
        ld    a, 0ffh
        out   (0b2h), a
        call  freeAllMemory
        ld    a, 01h
        out   (0b3h), a
        ld    a, 06h
        jp    0c00dh

coldReset:
        di
        xor   a
        dec   a
        out   (0b3h), a
        sbc   hl, hl
        ld    (0fff8h), hl
        ld    (0f217h), hl
        jr    loadIViewModule.l1

invParamsErr_POP:
        pop   hl

invalidParamsError:
        ld    a, 0dch                   ; .VDISP
        or    a
        ret

readVLoadHeader:
        ld    hl, paletteBuf
        call  @IView.VLOADINIT
        jp    nz, unknownFileFormat
        call  loadIViewImage
        jp    doneImage

loadIViewImage:
        ld    l, (ix + 6)               ; check height:
        ld    h, (ix + 7)
        push  hl
        dec   hl                        ; must be greater than zero,
        ld    bc, 65536 - 300           ; and less than or equal to 300
        add   hl, bc
        jr    c, invParamsErr_POP
        ld    a, (ix + 8)               ; check width:
        dec   a                         ; must be greater than zero,
        cp    46                        ; and less than or equal to 46
        jr    nc, invParamsErr_POP
        ld    a, (ix + 10)
        cp    2                         ; compressed format ?
        jr    nc, invParamsErr_POP      ; unsupported compression type
        ld    (compressionType), a
        or    a
        jr    z, .l1
        call  decompressImagePalette
        jr    nz, invParamsErr_POP
.l1:    xor   a
        cp    (ix + 11)                 ; number of animation frames/fields:
        jr    nz, .l2
        inc   (ix + 11)                 ; set default (1 non-interlaced frame)
.l2:    cp    (ix + 2)                  ; video mode resolution:
        jr    nz, invParamsErr_POP      ; only fixed mode is supported
        cp    (ix + 3)                  ; FIXBIAS resolution:
        jr    nz, invParamsErr_POP      ; must be fixed as well
        pop   hl                        ; height
        or    (ix + 4)                  ; palette resolution
        jr    z, .l3                    ; fixed palette ?
        call  divHLByAToDE              ; no, height must be an integer
        ld    a, l                      ; multiple of palette resolution
        or    h
        jr    nz, invalidParamsError    ; if not, then error
        ex    de, hl
        jr    .l4
.l3:    dec   hl                        ; if fixed palette,
        ld    a, h
        or    a                         ; and less than or equal to 256 lines,
        ld    hl, 1                     ; then there is only one palette line,
        jr    z, .l4
        inc   hl                        ; otherwise there are two palette lines
.l4:    ld    (nPaletteLines), hl       ; store the number of palette lines
        ex    de, hl
        ld    a, 16
        ld    hl, 4 * (lptTBorderEnd - lptBBorder)  ; size of borders + vblank
        call  multDEByAAddToHL          ; calculate the LPT size of one field
        ld    (lptFieldSize), hl
        ld    a, (ix + 5)               ; interlace flags:
        or    a
        jr    z, .l5                    ; non-interlaced image ?
        ex    de, hl
        ld    a, (ix + 12)              ; animation speed (field time * 50)
        inc   a
        call  multDEByAToHL
        ex    de, hl
        ld    a, (ix + 11)              ; animation frames
        call  multDEByAToHL             ; calculate total LPT size
.l5:    ld    (totalLPTSize), hl
        ld    a, (ix + 1)               ; check file type
        cp    49h
        jr    z, .l6                    ; standard IVIEW format ?
        ld    a, (ix)
        jr    .l7                       ; no, use first byte of header as mode
.l6:    ld    a, (imageFileChannel)
        call  readImageFileByte         ; read video mode
        ld    a, b
.l7:    ld    (lpbBuf_videoMode), a     ; store video mode
        and   0eh
        ld    l, 0                      ; L = attribute bytes per line
        ld    h, (ix + 8)               ; H = pixel bytes per line
        cp    0eh
        jr    z, .l9                    ; LPIXEL ?
        cp    04h
        jr    z, .l8                    ; ATTRIBUTE ?
        cp    02h
        jp    nz, invalidParamsError    ; if not PIXEL either, then error
        add   hl, hl
        defb  0feh                      ; = CP nn
.l8:    ld    l, h
.l9:    push  hl
        ld    e, h
        ld    a, (ix + 4)               ; palette resolution
        or    a
        jr    nz, .l10                  ; not fixed palette ?
        ld    l, h
        ld    h, a                      ; A = 0
.l10:   call  nz, multEByAToHL
        ld    (pixelBytesPerLPB), hl
        pop   de
        push  de
        ld    a, (ix + 4)
        or    a
        jr    nz, .l11
        ld    l, e
        ld    h, a
.l11:   call  nz, multEByAToHL
        ld    (attrBytesPerLPB), hl
        pop   de
        push  de
        ld    a, e
        ld    e, (ix + 6)
        ld    d, (ix + 7)
        push  de
        call  multDEByAToHL
        ld    (attrBytesPerField), hl
        ex    de, hl
        ld    a, (ix + 11)
        call  multDEByAToHL
        ld    (totalAttrBytes), hl
        pop   de
        pop   af
        call  multDEByAToHL
        ld    (pixelBytesPerField), hl
        ex    de, hl
        ld    a, (ix + 11)
        call  multDEByAToHL
        ld    (totalPixelBytes), hl
        ld    bc, (totalAttrBytes)
        add   hl, bc
        ld    (videoDataRemaining), hl
        ld    hl, 0000h
        ld    (videoMemoryBaseAddr), hl
        ld    hl, (totalLPTSize)
        ld    (attrDataBaseAddr), hl
        ld    bc, (totalAttrBytes)
        add   hl, bc
        ld    (pixelDataBaseAddr), hl
        call  allocateMemory
        ld    a, (ix + 7)
        rra
        ld    a, (ix + 6)
        rra
        ld    b, a
        ld    a, (lptTBorder)
        adc   a, b
        ld    e, a
        ld    a, (lptBBorder)
        add   a, b
        ld    l, a
        ld    b, 7
        ld    a, 0ffh
.l12:   cp    l
        jr    z, .l13
        dec   e
        inc   l
        djnz  .l12
.l13:   ld    a, e
        ld    (lptTBorder), a
        ld    a, l
        ld    (lptBBorder), a
        ld    e, (ix + 8)
        srl   e
        ld    a, 1fh
        adc   a, e
        ld    (lpbBuf_rightMargin), a
        ld    a, 1fh
        sub   e
        ld    (lpbBuf_leftMargin), a
        ld    hl, paletteSize
        ld    a, (lpbBuf_videoMode)
        and   0eh
        sub   04h
        cp    1
        sbc   a, a                      ; A = 0: (L)PIXEL, A = FFh: ATTRIBUTE
        ld    (attributeMode), a
        jr    nz, .l14
        ld    a, (lpbBuf_videoMode)     ; PIXEL or LPIXEL mode:
        add   a, 20h                    ; 2, 4, 16, 256 colors:
        and   60h                       ; A = 20h, 40h, 60h, 00h
        rra
        rra
        rra
        rra                             ; A = 02h, 04h, 06h, 00h
        ld    (hl), a                   ; store palette size
        cp    6
        jr    nz, createLPT
.l14:   ld    (hl), 8                   ; 16 colors or ATTRIBUTE: need FIXBIAS
        ld    a, (ix + 1)
        cp    49h
        jr    nz, createLPT
        ld    a, (imageFileChannel)
        call  readImageFileByte
        ld    a, b
        ld    (fixBias), a

createLPT:
        xor   a
        ld    (fieldNum), a
        ld    a, (segmentTable + 0)
        out   (0b1h), a
        ld    hl, (videoMemoryBaseAddr)
        res   7, h
        set   6, h
        ld    (lptWriteAddr), hl
.l1:    ld    de, (lptWriteAddr)
        ld    hl, 3f06h
        ld    bc, 203fh
        ld    a, (fieldNum)
        rrca                            ; check for second field
        and   (ix + 5)                  ; and interlace
        ld    a, 0fch
        jp    p, .l2
        ld    l, b
        ld    b, 38h
        dec   a
.l2:    ld    (vsyncBegin), hl
        ld    (vsyncEnd), bc
        ld    (nBlankLines), a
        ld    hl, lptBBorder
        ld    bc, lptTBorderEnd - lptBBorder
        xor   a
.l3:    ldi
        ldi
        ldi
        ldi
        ld    b, 6
.l4:    ld    (de), a
        inc   e
        ld    (de), a
        inc   de
        djnz  .l4
        cp    c
        jr    nz, .l3
        ld    (lptWriteAddr), de
        ld    hl, (pixelDataBaseAddr)
        ld    a, (fieldNum)
        ld    de, (pixelBytesPerField)
        call  multDEByAAddToHL
        ld    a, (attributeMode)
        or    a
        jr    z, .l5
        ld    (lpbBuf_LD2), hl
        ld    hl, (attrDataBaseAddr)
        ld    a, (fieldNum)
        ld    de, (attrBytesPerField)
        call  multDEByAAddToHL
.l5:    ld    (lpbBuf_LD1), hl
        ld    a, (ix + 4)
        or    a
        jp    nz, .l9                   ; palette resolution > 0?
        ld    a, (ix + 1)
        cp    49h
        jr    nz, .l6
        ld    a, (paletteSize)
        or    a
        jr    z, .l6
        ld    c, a
        ld    a, (fieldNum)
        cp    1
        sbc   a, a                      ; first field: A = FFh, 0 otherwise
        or    (ix + 5)                  ; check interlace flags
        and   04h
        jr    z, .l6                    ; not first field or palette interlace?
        ld    b, 0
        ld    de, paletteBuf
        ld    a, (imageFileChannel)
        call  readImageFileBlock
.l6:    ld    de, (lptWriteAddr)
        ld    hl, lpbBuf
        ld    bc, 16
        ld    a, (ix + 7)
        or    a
        jr    z, .l8
        ld    a, (ix + 6)
        or    a
        jr    z, .l8
        xor   a
        push  hl
        push  bc
        ld    (hl), a
        ldir
        ld    a, (attributeMode)
        or    a
        ld    a, (pixelBytesPerLPB)
        jr    z, .l7
        ld    hl, (lpbBuf_LD2)
        ld    b, a                      ; C = 0
        add   hl, bc
        ld    (lpbBuf_LD2), hl
        ld    a, (attrBytesPerLPB)
.l7:    ld    hl, (lpbBuf_LD1)
        ld    b, a
        add   hl, bc
        ld    (lpbBuf_LD1), hl
        pop   bc
        pop   hl
        xor   a
.l8:    sub   (ix + 6)
        ld    (hl), a
        push  de
        ldir
        ld    (lptWriteAddr), de
        pop   hl
        jp    .l13
.l9:    xor   a
        sub   (ix + 4)
        ld    (lpbBuf_nLines), a
        ld    hl, (nPaletteLines)
.l10:   push  hl
        ld    a, (paletteSize)
        ld    c, a
        ld    b, 0
        ld    de, paletteBuf
        ld    a, (imageFileChannel)
        call  readImageFileBlock
        ld    hl, lpbBuf
        ld    de, (lptWriteAddr)
        ld    bc, 16
        push  de
        ldir
        ld    (lptWriteAddr), de
        ld    a, (attributeMode)
        or    a
        jr    z, .l11
        ld    hl, (lpbBuf_LD2)
        ld    bc, (pixelBytesPerLPB)
        add   hl, bc
        ld    (lpbBuf_LD2), hl
        ld    bc, (attrBytesPerLPB)
        jr    .l12
.l11:   ld    bc, (pixelBytesPerLPB)
.l12:   ld    hl, (lpbBuf_LD1)
        add   hl, bc
        ld    (lpbBuf_LD1), hl
        pop   de
        pop   hl
        dec   hl
        ld    a, h
        or    l
        jr    nz, .l10
        ex    de, hl
.l13:   inc   hl
        ld    a, (fieldNum)
        inc   a
        ld    (fieldNum), a
        cp    (ix + 11)
        jp    nz, .l1
        set   0, (hl)

setBorderAndBias:
        ld    a, (fixBias)              ; set FIXBIAS
        out   (80h), a
        add   a, a
        add   a, a
        add   a, a
        ld    d, a
        ld    bc, 011ch                 ; C = BIAS_VID, B = set EXOS variable
        exos  16
        ld    a, (ix + 9)               ; set border color
        out   (081h), a
        ld    d, a
        ld    bc, 011bh                 ; C = BORD_VID, B = set EXOS variable
        exos  16

loadVideoData:
        di
        xor   a
        ld    (systemSegmentBackup), a
        ld    hl, (videoMemoryBaseAddr)
        call  setLPTAddress
        ld    a, (compressionType)
        or    a
        jp    nz, decompressImageData
        ld    ix, segmentTable
        ld    de, (lptWriteAddr)
.l2:    ld    bc, (videoDataRemaining)
        ld    a, b
        and   0c0h
        jr    z, .l3
        ld    bc, 4000h
.l3:    ld    hl, 8000h
        ld    a, (ix)
        or    a
        jr    z, .l7
        cp    0ffh
        jr    nz, .l4
        ld    hl, (exosBoundary)
        res   7, h
        set   6, h
        inc   hl
.l4:    cp    0fch
        jr    nc, .l5
        ld    (systemSegmentBackup), a
.l5:    out   (0b1h), a
        inc   ix
        or    a
        sbc   hl, de
        jr    c, .l7
        sbc   hl, bc
        add   hl, bc
        jr    nc, .l6
        ld    b, h
        ld    c, l
.l6:    ld    a, b
        or    c
        jr    z, .l7
        ld    hl, (videoDataRemaining)
        sbc   hl, bc
        ld    (videoDataRemaining), hl
        ld    a, (imageFileChannel)
        exos  6
        ld    de, 4000h
        or    a
        jr    z, .l2
.l7:    ld    a, 0ffh
        out   (0b2h), a
        ld    a, (imageFileChannel)
        exos  3                         ; close image file

keyboardWait:
        di
        call  swapSegments
.l1:    ld    de, 36                    ; * timer speed = 65536 / (TIME * 50)
        ld    hl, 0
.l2:    exx
.mouseInput1:
        call  @File.MOUSEINPUT          ; * replaced with LD HL, nnnn
        exx                             ;   if running as extension on an EP64
        or    a
.mouseInput2:
        rra                             ; * modified similarly to .mouseInput1
        jr    nz, .l8                   ; left or right mouse button pressed?
        ld    b, 10
.l4:    ld    a, b
        dec   a
        cp    4
        out   (0b5h), a
        in    a, (0b5h)
        cpl
        jr    nz, .l6                   ; not keyboard row 4?
        or    a
        jr    z, .l7                    ; no function key pressed?
        ld    de, timerSpeedTable + 8
.l5:    dec   de
        rla
        jr    nc, .l5
        ld    a, (de)
        ld    (.l1 + 1), a
        ld    e, a
        xor   a
        ld    d, a
.l6:    or    a
        rla                             ; clear bit 0
        jr    nz, .l8
.l7:    djnz  .l4                       ; test all rows of the keyboard matrix
        add   hl, de                    ; update timer
        jr    nc, .l2
        ld    a, (iviewIniMissing)      ; timeout elapsed:
        or    a                         ; if not a slideshow, then continue
        jr    nz, .l2                   ; waiting for keyboard events
.l8:    or    a                         ; * replaced with SCF (37h) on low RAM
        jp    c, coldReset
        ld    bc, 07b5h
        out   (c), b
        in    b, (c)                    ; check STOP key (B bit 0)
        cpl                             ; A bit 0 = right mouse button state
        and   b
        rrca
        ld    b, 3
        out   (c), b
        in    b, (c)                    ; check ESC key (B bit 7)
        and   b
        cp    80h                       ; Carry = 1 if STOP, ESC, or RMB
        sbc   a, a
        push  af
        call  swapSegments
        pop   af
        and   0e5h                      ; .STOP
        ret                             ; return with stop key status

swapSegments:
        ld    a, (systemSegmentBackup)
        or    a
        ret   z
        di
        ld    c, 0b2h
        in    l, (c)
        dec   c
        in    h, (c)
        push  hl
        ld    hl, (segmentTable + 3)
        out   (c), h
        inc   c
        out   (c), l
        ld    (.l2 + 1), sp
        ld    hl, 4000h
        ld    sp, 8000h
.l1:    pop   de
        ld    c, (hl)
        ld    (hl), e
        inc   l
        ld    b, (hl)
        ld    (hl), d
        push  bc
        pop   bc
        inc   l
        jp    nz, .l1
        inc   h
        jp    p, .l1
.l2:    ld    sp, 0000h
        pop   bc
        ld    a, b
        out   (0b1h), a
        ld    a, c
        out   (0b2h), a
        ret

allocateMemory:
        ld    a, (imageFileChannel)     ; close all channels, except
        ld    l, 0ffh                   ; the image file and IVIEW.INI
        ld    h, a
.l1:    ld    a, l
        dec   a
        cp    h
        jr    z, .l3
        dec   a
        jr    nz, .l2
        ld    a, (iviewIniMissing)
        or    a
        jr    z, .l3
        xor   a
.l2:    inc   a
        exos  3
.l3:    dec   l
        jr    nz, .l1
        ld    hl, tmpSegmentTable
        ld    (hl), 0
        in    a, (0b0h)
        cp    0fch                      ; is the machine an EP64?
        jr    nz, .l5
        ld    (segmentTable + 0), a     ; yes, need to move video data
        ld    hl, viewerDataEnd
        ld    a, (compressionType)
        or    a
        jr    z, .l4
        ld    hl, decompressorDataEnd
.l4:    ld    (videoMemoryBaseAddr), hl
        ex    de, hl
        ld    hl, (pixelDataBaseAddr)
        add   hl, de
        ld    (pixelDataBaseAddr), hl
        ld    hl, (attrDataBaseAddr)
        add   hl, de
        ld    (attrDataBaseAddr), hl
.l5:    exos  24
        jr    nz, .l8
        ld    a, c
        sub   0fch
        jr    c, .l6
        add   a, low segmentTable
        ld    e, a
        adc   a, high segmentTable
        sub   e
        ld    d, a
        ld    a, c
        ld    (de), a
        jr    .l5
.l6:    ld    a, (segmentTable + 3)
        or    a
        jr    z, .l7
        ld    (hl), c
        inc   hl
        ld    (hl), 0
        jr    .l5
.l7:    ld    a, c
        ld    (segmentTable + 3), a
        jr    .l5
.l8:    cp    7fh                       ; .SHARE
        ret   nz
        dec   de
        ld    (exosBoundary), de
        exos  23
        ld    a, (segmentTable + 3)
        or    a
        ld    a, 0ffh
        jr    nz, .l9
        ld    (segmentTable + 3), a
        jr    .l10
.l9:    ld    (segmentTable + 4), a
.l10:   ld    hl, tmpSegmentTable
.l11:   ld    a, (hl)
        or    a
        ret   z
        ld    c, a
        exos  25
        inc   hl
        jr    .l11

freeAllMemory:
        ld    b, 5
        ld    hl, segmentTable
.l1:    push  bc
        ld    c, (hl)
        ld    (hl), 0
        inc   hl
        exos  25
        pop   bc
        djnz  .l1
        ret

multHLByDEToHLBC        equ     @IView.WSZORZAS
multEByAToHL            equ     @IView.SZORZAS00
multDEByAToHL           equ     @IView.SZORZAS0
multDEByAAddToHL        equ     @IView.SZORZAS
; divide HL by A, and return the result in DE (the LSB is also in A)
; the remainder is returned in HL
divHLByAToDE            equ     @IView.W_MODULO

; -----------------------------------------------------------------------------

getDataBlockSizes:
        ld    b, 4
.l1:    push  bc
        push  de
        push  hl
        ld    a, (imageFileChannel)
        exos  5
        or    a
        jr    nz, .l2
        pop   hl
        pop   de
        ld    e, d
        ld    d, l
        ld    l, h
        ld    h, b
        pop   bc
        djnz  .l1
        ld    a, h                      ; HL = compressed size (must be > 0)
        or    l
        jr    z, .l2
        ld    a, d                      ; DE = uncompressed size (must be > 0)
        or    e
        ret   nz
.l2:    scf
        ret

decompressImagePalette:
        push  ix
        call  getDataBlockSizes
        jr    c, .l1
        ld    (decompDataRemaining), hl
        ld    hl, 4000h
        sbc   hl, de
        jr    c, .l1
        ld    (paletteDataReadAddr), hl
        ld    d, h
        ld    e, l
        in    a, (0b0h)
        cp    0fch                              ; decompressor input buffer
        sbc   a, a                              ; size is 256 bytes
        and   high (CVIEW_BUFFER_SIZE - 256)    ; if running on an EP64
        add   a, high decompressorDataEnd
        ld    b, a
        ld    c, low decompressorDataEnd
        sbc   hl, bc
        jr    c, .l1
        ld    iy, decompSegmentTable
        in    a, (0b0h)
        ld    (iy), a
        ld    (iy + 1), 0
        call  decompDataFromBuf         ; Carry = 0: use buffer
.l1:    pop   ix
        sbc   a, a                      ; Carry = 1 on error -> A = FFh
        ret

decompressImageData:
        call  getDataBlockSizes
        jp    c, resetRoutine
        ld    (decompDataRemaining), hl
        in    a, (0b3h)
        ld    (.l2 + 1), a              ; save page 3 segment
        in    a, (0b0h)                 ; check machine type
        cp    0fch
        jp    c, .l1                    ; not an EP64 ?
        ld    hl, (lptWriteAddr)
        res   6, h
        ld    (lptWriteAddr), hl
        inc   hl
        inc   hl
        inc   hl
        inc   hl
        add   hl, de                    ; end address of uncompressed data
        jp    c, resetRoutine           ; does not fit in 64K memory: error
        ld    a, h
        cp    0c0h
        jr    c, .l1                    ; system segment not used?
        ld    a, (exosBoundary)
        add   a, 2
        ld    c, a
        ld    a, (exosBoundary + 1)
        add   a, 0c0h
        ld    b, a
        push  hl
        sbc   hl, bc
        pop   hl
        jr    c, .l1                    ; system data is not overwritten?
        push  hl
        ld    a, 37h    ; (= SCF)         if it is, then cannot return
        ld    (keyboardWait.l8), a      ; after displaying the image,
        ld    hl, keyboardWait.l1
        ld    (.l3 + 1), hl             ; cannot close the input file,
        ld    hl, segmentTable + 1      ; and need to load all compressed data
        ld    c, 0b1h                   ; into memory first
        outi
        inc   c
        outi
        inc   c
        outi
        ld    de, (lptWriteAddr)
        ld    bc, (decompDataRemaining)
        ld    a, (imageFileChannel)
        exos  6
        or    a
        jr    nz, .l4
        di
        ld    h, d                      ; HL = end of compressed data
        ld    l, e
        pop   de                        ; DE = end of uncompressed data
        ld    bc, (decompDataRemaining) ; BC = length of compressed data
        dec   de
        dec   hl
        lddr
        ld    h, d
        ld    l, e
        inc   hl                        ; HL = compressed data read address
        ld    a, h
        rlca
        rlca
        and   3
        add   a, low segmentTable
        ld    c, a
        adc   a, high segmentTable
        sub   c
        ld    b, a
        ld    a, (bc)
        out   (0b3h), a                 ; read from page 3
        set   7, h
        set   6, h
        ld    a, l
        cp    1
        sbc   a, a
        add   a, h                      ; decrement H if L is zero
        ld    h, a
        defb  0c6h                      ; = ADD A, 0AFh (sets carry as A > BEh)
.l1:    xor   a
        push  hl
        ld    hl, segmentTable
        ld    de, decompSegmentTable
        ld    iy, decompSegmentTable
        ld    bc, 4
        ldir
        pop   hl
        ld    a, c
        ld    (de), a
        ld    de, (lptWriteAddr)
        call  decompDataFromBuf         ; Carry = 0: use buffer
.l2:    ld    a, 00h                    ; * restore page 3 segment
        out   (0b3h), a
.l3:    jp    nc, loadVideoData.l7      ; * jump to keyboardWait.l1 on low RAM
.l4:    jp    resetRoutine

readImageFileByte:
        ld    b, a
        ld    a, (compressionType)
        or    a
        jr    nz, .l1
        ld    a, b
        exos  5
        ret
.l1:    push  hl
        ld    hl, (paletteDataReadAddr)
        bit   6, h
        jr    nz, .l2
        ld    b, (hl)
        inc   hl
        ld    (paletteDataReadAddr), hl
        pop   hl
        xor   a
        ret
.l2:    pop   hl
        ld    b, 0ffh
        ld    a, 0e5h
        add   a, b                      ; E4h = end of file
        ret

readImageFileBlock:
        push  af
        ld    a, (compressionType)
        or    a
        jr    nz, .l1
        pop   af
        exos  6
        ret
.l1:    pop   af
        ld    a, c
        or    b
        ret   z
        push  hl
        ld    hl, (paletteDataReadAddr)
        ld    a, high 3fffh
.l2:    cp    h
        jr    c, .l3
        ldi
        jp    pe, .l2
        ld    (paletteDataReadAddr), hl
        pop   hl
        xor   a
        ret
.l3:    pop   hl
        ld    a, 0e4h                   ; end of file
        ret

; -----------------------------------------------------------------------------

readBlock:
        push  af
        bit   7, h
        jr    z, .l3
        inc   h
        jr    z, .l2
.l1:    pop   af
        di
        ret
.l2:    ld    h, 0c0h
        in    a, (0b3h)
        inc   a
        out   (0b3h), a
        jr    .l1
.l3:    in    a, (0b0h)
        cp    0fch
        ld    a, high 0100h
        jr    nc, .l4
        inc   h
        ld    a, h
        cp    high (decompressBuffer + CVIEW_BUFFER_SIZE)
        jr    c, .l1
        ld    a, high CVIEW_BUFFER_SIZE
.l4:    push  bc
        push  de
        ld    b, a                      ; EP64: BC = 0100h,
        ld    c, 0                      ; EP128: BC = CVIEW_BUFFER_SIZE
        ld    hl, (decompDataRemaining)
        sbc   hl, bc                    ; NOTE: carry is always 0 here
        jr    nc, .l5
        add   hl, bc
        ld    b, h
        ld    c, l
        ld    hl, 0
.l5:    ld    (decompDataRemaining), hl
        ld    hl, decompressBuffer
        ld    a, b
        or    c
        jp    z, decompressError
        ld    d, h
        ld    e, l
        ld    a, (imageFileChannel)
        exos  6
        or    a
        jp    nz, decompressError
        pop   de
        pop   bc
        jr    .l1

; =============================================================================

decompDataFromBuf:
        jr    c, decompressData         ; Carry = 1: do not use buffer
        in    a, (0b0h)
        cp    0fch
        sbc   a, a                              ; 0 on EP64
        and   high (CVIEW_BUFFER_SIZE - 256)    ; A = (buffer size / 256) - 1
        ld    hl, decompressBuffer
        ld    b, a
        add   a, h
        ld    h, a
        ld    a, b
        defb  0feh                      ; = CP nn

decompressData:
        xor   a
        add   a, high decompressTables
        ld    ixh, a
        ld    (decompressDataBlock.l3 + 2), a
        ld    (copyLZMatch.l1 + 1), a
        push  hl
        exx
        pop   hl                        ; HL' = compressed data read address
        dec   l
        exx
        ld    hl, 0
        add   hl, sp
        ld    ixl, decodeTableEnd + 6
        ld    sp, ix
        in    a, (0b2h)                 ; save memory paging,
        push  af
        in    a, (0b1h)
        push  af
        push  hl                        ; and stack pointer
        ld    sp, hl
        call  getSegment                ; get first output segment
        dec   de
        exx
        ld    e, 80h                    ; initialize shift register
        call  read8Bits                 ; skip checksum byte
        exx
.l1:    call  decompressDataBlock       ; decompress all blocks
        jr    z, .l1
        inc   de
        defb  0feh                      ; = CP nn

decompressError:
        xor   a

decompressDone:
        ld    c, a                      ; save error flag: 0: error, 1: success
        ld    ixl, decodeTableEnd
        ld    sp, ix
        pop   hl                        ; restore stack pointer,
        pop   af                        ; and memory paging
        out   (0b1h), a
        pop   af
        out   (0b2h), a
        ld    sp, hl
        exx
        inc   l
        push  hl
        exx
        pop   hl
        ld    a, c                      ; on success: return A=0, Z=1, C=0
        sub   1                         ; on error: return A=0FFh, Z=0, C=1
        ret

writeBlock:
        inc   d
        bit   6, d
        ret   z
        inc   iy

getSegment:
        set   7, d                      ; write decompressed data to page 2
        res   6, d
        push  af
        ld    a, (iy)
        or    a
        jr    z, decompressError
        cp    0fch
        jr    nc, .l1
        ld    (systemSegmentBackup), a
.l1:    out   (0b2h), a                 ; use pre-allocated segment?
        pop   af
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

nLengthSlots        equ     8
nOffs1Slots         equ     4
nOffs2Slots         equ     8
maxOffs3Slots       equ     32
totalSlots          equ     nLengthSlots+nOffs1Slots+nOffs2Slots+maxOffs3Slots
; NOTE: the upper byte of the address of all table entries must be the same
slotBitsTable       equ     0000h
slotBitsTableL      equ     slotBitsTable
slotBitsTableO1     equ     slotBitsTableL + (nLengthSlots * 4)
slotBitsTableO2     equ     slotBitsTableO1 + (nOffs1Slots * 4)
slotBitsTableO3     equ     slotBitsTableO2 + (nOffs2Slots * 4)
decodeTableEnd      equ     slotBitsTable + (totalSlots * 4)

decompressDataBlock:
        exx
        call  read8Bits                 ; read number of symbols - 1 (BC)
        exx
        ld    c, a                      ; NOTE: MSB is in C, and LSB is in B
        exx
        call  read8Bits
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
        jr    c, .l1                    ; is compression enabled ?
        exx                             ; no, copy uncompressed literal data
        ld    bc, 0101h
        jp    .l13
.l1:    push  bc                        ; compression enabled:
        ld    a, 40h
        exx
        call  readBits                  ; get prefix size for length >= 3 bytes
        ld    b, a
        inc   b
        ld    a, 08h
        ld    d, 80h
.l2:    add   a, a
        srl   d                         ; D' = prefix size code for readBits
        djnz  .l2
        pop   bc                        ; store the number of symbols in BC'
        exx
        push  de                        ; save decompressed data write address
        add   a, low ((nLengthSlots + nOffs1Slots + nOffs2Slots) * 4)
        ld    ixl, a
.l3:    ld    de, decompressTables      ; * initialize decode tables
        scf
.l4:    ld    bc, 1                     ; set initial base value (len=3, offs=2)
        rl    c                         ; 2 is added to correct for the LDIs
.l5:    ld    a, 10h
        exx
        call  readBits
        exx
        ld    hl, bitCntTable
        add   a, a
        add   a, a
        add   a, l
        ld    l, a
        adc   a, h
        sub   l
        ld    h, a
        ldi                             ; copy read bit count (MSB, LSB)
        ldi
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
        xor   ixl
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

copyLZMatch:
        exx
        add   a, a
        add   a, a
        add   a, low (slotBitsTableL + 32)
        ld    l, a                      ; decode match length
.l1:    ld    h, high decompressTables  ; *
        ld    a, (hl)
        inc   l
        or    a
        jp    z, .l2
        exx
        call  readBits
        exx
.l2:    ld    b, a
        ld    a, (hl)
        inc   l
        or    a
        exx
        call  nz, readBits
        exx
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
        ld    b, low slotBitsTableO2
        jp    .l6
.l4:    ld    b, low slotBitsTableO3    ; length >= 3 bytes,
        exx
        ld    a, d                      ; variable prefix size
        jp    .l7
.l5:    ld    a, 40h                    ; length == 1 byte, read 2 prefix bits
        ld    b, low slotBitsTableO1
.l6:    exx
.l7:    call  readBits                  ; read offset prefix bits
        exx
        add   a, a
        add   a, a
        add   a, b
        ld    l, a                      ; decode match offset
        ld    a, (hl)
        inc   l
        or    a
        exx
        call  nz, readBits
        exx
        ld    b, a
        ld    a, (hl)
        inc   l
        or    a
        exx
        call  nz, readBits
        exx
        add   a, (hl)
        inc   l
        ld    c, a
        ld    a, b
        adc   a, (hl)
        ld    h, a
        cp    high 4000h
        ld    a, e                      ; calculate LZ77 match read address
        jr    c, .l23                   ; offset <= 16384 bytes ?
        sub   c
        pop   bc                        ; BC = length
        ld    l, a
        ld    a, d
        sbc   a, h
        ld    h, a                      ; set up memory paging
        jr    c, .l21                   ; page 2 or 3 ?
        add   a, a                      ; page 0 or 1
        jp    p, .l10                   ; page 0 ?
        ld    a, (iy - 1)               ; page 1
        jr    .l12
.l10:   ld    a, (iy - 2)
.l11:   set   6, h                      ; read from page 1
.l12:   out   (0b1h), a
.l13:   inc   e                         ; copy match data
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

read8Bits:
        ld    a, 001h

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

bitCntTable:
        defw  0000h, 0003h, 8000h, 0004h, 4000h, 0006h, 2000h, 000ah
        defw  1000h, 0012h, 0800h, 0022h, 0400h, 0042h, 0200h, 0082h
        defw  0100h, 0102h, 0180h, 0202h, 0140h, 0402h, 0120h, 0802h
        defw  0110h, 1002h, 0108h, 2002h, 0104h, 4002h, 0102h, 8002h

; =============================================================================

exdosCommandName        equ     @IView.EXDFD
exdosCommandStatus:
        defb 000h
iviewIniFileName        equ     @IView.INIFILE
iviewIniMissing:
        defb 0ffh
fileCommandName:
        defb 7
        defm "FILE "
        defw fileNameBuffer

; wait time = 65536 / (N * 50) seconds
timerSpeedTable:
        ;     F4  F8  F3  F6  F5  F7  F2  F1
        defb  36, 18, 48, 24, 29, 21, 73, 145

lpbBuf:
        defb 0ffh, 012h, 03fh, 000h, 000h, 000h, 000h, 000h
paletteBuf:
        defb 000h, 0ffh, 0feh, 0fdh, 0fch, 0fbh, 0fah, 0f9h

lpbBuf_nLines           equ     lpbBuf + 0
lpbBuf_videoMode        equ     lpbBuf + 1
lpbBuf_leftMargin       equ     lpbBuf + 2
lpbBuf_rightMargin      equ     lpbBuf + 3
lpbBuf_LD1              equ     lpbBuf + 4
lpbBuf_LD2              equ     lpbBuf + 6

lptBBorder:
        defb 069h, 012h, 03fh, 000h
lptVBlank:
        defb 0fdh, 000h, 03fh, 000h
        defb 0feh, 000h, 006h, 03fh
        defb 0ffh, 080h, 03fh, 020h
        defb 0fch, 002h, 006h, 03fh
lptTBorder:
        defb 069h, 012h, 03fh, 000h
lptTBorderEnd:

vsyncBegin              equ     lptVBlank + 6
vsyncEnd                equ     lptVBlank + 10
nBlankLines             equ     lptVBlank + 12

videoMemoryBaseAddr:
        defw 00000h
attrDataBaseAddr:
        defw 00000h
pixelDataBaseAddr:
        defw 00000h
exosBoundary:
        defw 0bfffh

    if BUILDING_CVIEW_COM != 0
        module  File
        include "mouse.s"
        endmod
    endif

viewerCodeEnd:

; -----------------------------------------------------------------------------

segmentTable            equ     viewerCodeEnd

nPaletteLines           equ     segmentTable + 5
lptFieldSize            equ     nPaletteLines + 2
totalLPTSize            equ     lptFieldSize + 2
attrBytesPerLPB         equ     totalLPTSize + 2
attrBytesPerField       equ     attrBytesPerLPB + 2
totalAttrBytes          equ     attrBytesPerField + 2
pixelBytesPerLPB        equ     totalAttrBytes + 2
pixelBytesPerField      equ     pixelBytesPerLPB + 2
totalPixelBytes         equ     pixelBytesPerField + 2
videoDataRemaining      equ     totalPixelBytes + 2
paletteSize             equ     videoDataRemaining + 2
attributeMode           equ     paletteSize + 1
fixBias                 equ     attributeMode + 1
compressionType         equ     fixBias + 1
lptWriteAddr            equ     compressionType + 1

fieldNum                equ     lptWriteAddr + 2
systemSegmentBackup     equ     fieldNum + 1

systemDataLength        equ     systemSegmentBackup + 1
systemDataAddress       equ     systemDataLength + 2
imageFileChannel        equ     systemDataAddress + 2

fileHeaderBuf           equ     imageFileChannel + 1
fileNameBuffer          equ     fileHeaderBuf + 16
; NOTE: the + 2 is to avoid overwriting paletteDataReadAddr
tmpSegmentTable         equ     fileNameBuffer + 2

viewerDataEnd           equ     (fileNameBuffer + 256 + 15) & 0fff0h

paletteDataReadAddr     equ     fileNameBuffer
decompDataRemaining     equ     paletteDataReadAddr + 2
decompSegmentTable      equ     decompDataRemaining + 2
decompressBuffer        equ     (decompSegmentTable + 5 + 255) & 0ff00h
decompressTables        equ     (decompressBuffer + 256 + 255) & 0ff00h

decompressorDataEnd     equ     (decompressTables + 216 + 15) & 0fff0h

        dephase

