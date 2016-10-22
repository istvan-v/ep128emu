
loaderCodeBegin:

extMain:
main:
        ld    a, c
        cp    2
        jr    z, parseCommand
        cp    6
        jp    z, loadModule
        cp    3
        ret   nz
        ld    a, b
        or    a
        jr    z, printShortDesc
        ld    hl, iviewCommandName
        call  checkCommandName
        ret   nz
        ld    bc, iviewDescriptionEnd-iviewShortDesc
        ld    de, iviewShortDesc
        ld    a, 0ffh
        rst   030h
        defb 8
        ld    c, 0
        ret

printShortDesc:
        push  de
        push  bc
        ld    de, iviewShortDesc
        ld    bc, iviewLongDesc-iviewShortDesc
        ld    a, 0ffh
        rst   030h
        defb 8
        pop   bc
        pop   de
        ret

parseCommand:
        ld    hl, iviewCommandName
        call  checkCommandName
        ret   nz
        call copyViewer
        pop   af
        ld    (imageFileChannel), a
        jp    iviewCommandMain
        xor   a
        ld    c, a
        ret

checkCommandName:
        ld    a, b
        cp    (hl)
        ret   nz
        inc   hl
        push  bc
        push  de
        inc   de
lc061:  ld    a, (de)
        cp    (hl)
        jr    nz, lc069
        inc   hl
        inc   de
        djnz  lc061
lc069:  pop   de
        pop   bc
        ret

iviewCommandName:
        defb 5
        defm "CVIEW"
iviewShortDesc:
        defm "IVIEW version 0.4c (CVIEW)"
        defb 00dh, 00ah
iviewLongDesc:
        defm "Image viewer for converted PC images."
        defb 00dh, 00ah
        defm "Written by Zozosoft, 2008"
        defb 00dh, 00ah
        defm "Compressed format support and converter "
        defm "program by IstvanV"
        defb 00dh, 00ah
iviewDescriptionEnd:

loadModule:
        ld    ixh, d
        ld    ixl, e
        ld    a, (ix+001h)
        cp    049h
        ret   nz
        push  bc
        call copyViewer
        pop   af
        ld    (imageFileChannel), a
        jp    loadIViewModule
        xor   a
        ld    c, a
        ret

copyViewer:
        ld    hl, loaderCodeEnd
        ld    de, viewerCodeBegin
        ld    bc, viewerCodeEnd-viewerCodeBegin
        ldir
        ld h, d
        ld l, e
        xor a
        ld (de), a
        inc de
        ld bc, decompressorDataEnd-(viewerCodeEnd+1)
        ldir
        in    a, (0b3h)
        cp    0fdh
        ret   nz
        ld    (segmentTable+1), a
        ld    a, coldReset&0ffh
        ld    (l0754+1), a
        ld    a, coldReset/256
        ld    (l0754+2), a
        ret

loaderCodeEnd:

; =============================================================================

        phase 00100h

viewerCodeBegin:

        ld    sp, 00100h

iviewCommandMain:
        ld    c, 060h
        rst   030h
        defb 0
        ld    sp, 00100h
        ld    hl, resetRoutine          ; set up warm reset address
        ld    (0bff8h), hl
        ld    de, exdosCommandName      ; check if EXDOS is available
        rst   030h
        defb 26
        ld    (exdosCommandStatus), a   ; 0: yes, non-zero: no
        jr    nz, l0121
l011a:  ld    de, iviewIniFileName      ; if EXDOS is available,
l011d:  ld    a, 1                      ; load IVIEW.INI file if it is present
        rst   030h
        defb 1
l0121:  ld    (iviewIniMissing), a      ; 0: have IVIEW.INI file
l0124:  ld    a, (iviewIniMissing)
        or    a
        call  z, readIViewIni
        jr    z, l015f
l012d:  ld    a, (exdosCommandStatus)   ; if no IVIEW.ini is available,
        or    a
        jr    nz, l015b
        ld    de, fileCommandName       ; but EXDOS is present,
        rst   030h                      ; then use the FILE extension to
        defb 26                         ; select an image file
        push  af
        ld    a, 1
        rst   030h                      ; close any previously opened file
        defb 3
        pop   af
        jp    nz, resetRoutine          ; no FILE extension ?
        ld    de, fileNameBuffer
        ld    a, 1
        rst   030h                      ; open input file
        defb 1
        ld    a, 1
        rst   030h                      ; check first byte (must be zero)
        defb 5
        ld    a, b
        or    a
        push  af
        ld    a, 1
        rst   030h                      ; close input file
        defb 3
        pop   af
        jr    z, l015f                  ; first byte is zero ?
        ld    de, fileNameBuffer        ; no, assume IVIEW.INI format
        jr    l011d                     ; and try again
l015b:  xor   a
        ld    (fileNameBuffer), a       ; no EXDOS, use empty file name
l015f:  ld    a, 256-(302/2)
        ld    (lptTBorder), a
        ld    (lptBBorder), a
        xor   a                         ; image file channel
        ld    (imageFileChannel), a
        ld    de, fileNameBuffer
        rst   030h                      ; open image file
        defb 1
        jr    nz, doneImage             ; failed ?
        ld    de, fileHeaderBuf
        ld    bc, 16
        rst   030h                      ; read header (16 bytes):
        defb 6
        jp    nz, resetRoutine          ; exit on error
        ex    af, af'
        ld    ix, fileHeaderBuf         ; IX = address of file header buffer
        ld    a, (ix+001h)              ; check file type:
        cp    00bh
        jp    z, readVLoadHeader        ; VLOAD ?
        cp    049h
        jr    nz, unknownFileFormat     ; if not IVIEW either, then error
        call  loadIViewImage

doneImage:
        ex    af, af'                   ; save error code
        ld    a, 0ffh                   ; restore memory paging
        out   (0b2h), a
        ld    hl, (0bff4h)              ; restore LPT
        set   6, h
        ld    b, 004h
l019f:  srl   h
        rr    l
        djnz  l019f
        ld    a, l
        out   (082h), a
        ld    a, h
        or    0c0h
        out   (083h), a
        call  freeAllMemory             ; clean up

unknownFileFormat:
        xor   a
        rst   030h                      ; close input file
        defb 3
        ex    af, af'                   ; get error code
        cp    0e5h                      ; .STOP ?
        jp    nz, l0124                 ; no, continue with next image
        ld    a, 1                      ; yes, close IVIEW.INI,
        rst   030h
        defb 3
        jp    l012d                     ; and return to FILE, or exit (reset)

readIViewIni:
        ld    iy, fileNameBuffer        ; read next entry from IVIEW.INI
        ld    (iy+0), 000h
        ld    hl, fileNameBuffer+1
l01cb:  ld    a, 1
        rst   030h                      ; read character
        defb 5
        jr    z, l01db
        ld    a, (iy+0)                 ; EOF or error:
        or    a                         ; have any characters been read ?
        jr    z, l01d9
        xor   a                         ; yes, return last file name
        ret
l01d9:  inc   a                         ; no, return error
        ret
l01db:  ld    a, b
        cp    021h                      ; whitespace ?
        jr    nc, l01e8
        ld    a, (iy+0)                 ; yes,
        or    a
        jr    z, l01cb                  ; skip if no characters were read yet,
        xor   a                         ; or done reading file name
        ret
l01e8:  ld    (hl), a                   ; add non-whitespace character
        inc   hl                        ; to file name
        inc   (iy+0)                    ; update length
        jr    nz, l01cb
        or    a                         ; error if longer than 255 characters
        ret

loadIViewModule:
        ld    c, 040h
        rst 030h
        defb 0
        ld    sp, 00100h
        ei
        ld    hl, resetRoutine
        ld    (0bff8h), hl
        call  loadIViewImage
        di
        ld    a, 0ffh
        out   (0b2h), a
        ld    hl, resetRoutine
        ld    (0bff8h), hl
        call  freeAllMemory
        xor   a
        out   (0b3h), a
        jp    0c000h

resetRoutine:
        ld    sp, 00100h
        ld    a, 0ffh
        out   (0b2h), a
        call  freeAllMemory
        ld    a, 006h
        ex    af, af'
        ld    a, 001h
        ld    hl, 0c00dh
        jp    0b217h

coldReset:
        di
        ld    a, 0ffh
        out   (0b2h), a
        xor   a
        out   (0b3h), a
        ld    de, 0bf00h
        ld    h, d
        ld    l, e
        ld    (de), a
        inc   de
        ld    bc, 000ffh
        ldir
        jp 0c000h

invalidParamsError:
        ld    a, 0dch                   ; .VDISP
        or    a
        ret

readVLoadHeader:
        ld    e, (ix+005h)
        ld    d, 000h
        ld    h, d
        ld    l, d
        ld    b, 009h
l024b:  add   hl, de
        djnz  l024b
        ld    (ix+006h), l
l0251:  ld    (ix+007h), h
        ld    a, (ix+002h)
        cp    001h
        jp    z, l0266
        cp    005h
        jp    nz, unknownFileFormat
        ld    a, (ix+004h)
        jr    l026a
l0266:  ld    a, (ix+004h)
        rrca
l026a:  ld    (ix+008h), a
        ld    a, (ix+003h)
        and   003h
        rrca
        rrca
        rrca
        set   4, a
        add   a, 002h
        bit   2, (ix+002h)
        jr    z, l0281
        add   a, 00ch
l0281:  ld    (ix+0), a
        xor   a
        ld    (ix+002h), a
        ld    (ix+003h), a
        ld    (ix+004h), a
        ld    (ix+005h), a
        ld    (ix+009h), a
        ld    (ix+00ah), a
        ld    (ix+00bh), a
        ld    (ix+00ch), a
        ld    (ix+00dh), a
        ld    (ix+00eh), a
        ld    (ix+00fh), a
        ld    b, 008h
        ld    hl, paletteBuf
l02ab:  ld    (hl), a
        dec   a
        inc   hl
        djnz  l02ab
        ld    hl, doneImage
        push  hl

loadIViewImage:
        ld    l, (ix+006h)              ; check height:
        ld    h, (ix+007h)
        ld    a, h
        or    l
        jp    z, invalidParamsError     ; must be greater than zero,
        ld    bc, 301
        sbc   hl, bc
        jp    nc, invalidParamsError    ; and less than or equal to 300
        ld    a, (ix+008h)              ; check width:
        or    a
        jp    z, invalidParamsError     ; must be greater than zero,
        cp    47
        jp    nc, invalidParamsError    ; and less than or equal to 46
        ld    a, (ix+00ah)
        cp    2                         ; compressed format ?
        jp    nc, invalidParamsError    ; unsupported compression type
        ld    (compressionType), a
        or    a
        jr    z, llivi1
        call  decompressImagePalette
        jp    nz, invalidParamsError
llivi1: ld    a, (ix+00bh)              ; number of animation frames/fields:
        or    a
        jr    nz, l02e4
        ld    (ix+00bh), 001h           ; set default (1 non-interlaced frame)
l02e4:  ld    a, (ix+002h)              ; video mode resolution:
        or    a
        jp    nz, invalidParamsError    ; only fixed mode is supported
        ld    a, (ix+003h)              ; FIXBIAS resolution:
        or    a
        jp    nz, invalidParamsError    ; must be fixed as well
        ld    l, (ix+006h)              ; height
        ld    h, (ix+007h)
        ld    a, (ix+004h)              ; palette resolution
        or    a
        jr    z, l0309                  ; fixed palette ?
        call  divHLByAToDE              ; no, height must be an integer
        ld    a, l                      ; multiple of palette resolution
        or    h
        jp    nz, invalidParamsError    ; if not, then error
        ex    de, hl
        jr    l0316
l0309:  dec   hl                        ; if fixed palette,
        ld    a, h
        or    a                         ; and less than or equal to 256 lines,
        ld    hl, 00001h                ; then there is only one palette line,
        jr    z, l0316
        inc   hl                        ; otherwise there are two palette lines
l0316:  ld    (nPaletteLines), hl       ; store the number of palette lines
        ex    de, hl
        ld    a, 16
        ld    hl, (16*2)+(lptTBorder-lptVBlank) ; size of borders + vblank
        call  multDEByAAddToHL          ; calculate the LPT size of one field
        ld    (lptFieldSize), hl
        ld    a, (ix+005h)              ; interlace flags:
        or    a
        jr    z, l033a                  ; non-interlaced image ?
        ex    de, hl
        ld    a, (ix+00ch)              ; animation speed (field time * 50)
        inc   a
        call  multDEByAToHL
        ex    de, hl
        ld    a, (ix+00bh)              ; animation frames
        call  multDEByAToHL             ; calculate total LPT size
l033a:  ld    (totalLPTSize), hl
        ld    a, (ix+001h)              ; check file type
        cp    049h
        jr    z, l0349                  ; standard IVIEW format ?
        ld    a, (ix+0)
        jr    l034f                     ; no, use first byte of header as mode
l0349:  ld    a, (imageFileChannel)
        call  readImageFileByte         ; read video mode
        ld    a, b
l034f:  ld    (lpbBuf_videoMode), a     ; store video mode
        and   00eh
        ld    h, (ix+008h)
        ld    l, 0
        cp    00eh
        jr    z, l036e                  ; LPIXEL ?
        ld    l, (ix+008h)
        cp    004h
        jr    z, l036e                  ; ATTRIBUTE ?
        cp    002h
        jp    nz, invalidParamsError    ; if not PIXEL either, then error
        ld    a, h
        add   a, h                      ; H = pixel bytes per line
        ld    h, a
        ld    l, 0                      ; L = attribute bytes per line
l036e:  push  hl
        ld    e, h
        ld    a, (ix+004h)              ; palette resolution
        or    a
        jr    nz, l037b                 ; not fixed palette ?
        ld    l, h
        ld    h, 0
        jr    l037e
l037b:  call  multEByAToHL
l037e:  ld    (pixelBytesPerLPB), hl
        pop   de
        push  de
        ld    a, (ix+004h)
        or    a
        jr    nz, l038e
        ld    l, e
        ld    h, 000h
        jr    l0391
l038e:  call  multEByAToHL
l0391:  ld    (attrBytesPerLPB), hl
        pop   de
        push  de
        ld    a, e
        ld    e, (ix+006h)
        ld    d, (ix+007h)
        call  multDEByAToHL
        ld    (attrBytesPerField), hl
        ex    de, hl
        ld    a, (ix+00bh)
        call  multDEByAToHL
        ld    (totalAttrBytes), hl
        pop   af
        ld    e, (ix+006h)
        ld    d, (ix+007h)
        call  multDEByAToHL
        ld    (pixelBytesPerField), hl
        ex    de, hl
        ld    a, (ix+00bh)
        call  multDEByAToHL
        ld    (totalPixelBytes), hl
        ld    bc, (totalAttrBytes)
        add   hl, bc
        ld    (videoDataRemaining), hl
        ld    hl, (videoMemoryBaseAddr)
        ld    bc, (totalLPTSize)
        add   hl, bc
        ld    (attrDataBaseAddr), hl
        ld    bc, (totalAttrBytes)
        add   hl, bc
        ld    (pixelDataBaseAddr), hl
        call  allocateMemory
        ld    e, (ix+006h)
        ld    a, (ix+007h)
        rra
        ld    a, (ix+006h)
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
l0410:  cp    l
        jr    z, l0411
        dec   e
        inc   l
        djnz  l0410
l0411:  ld    a, e
        ld    (lptTBorder), a
        ld    a, l
        ld    (lptBBorder), a
        ld    e, (ix+008h)
        srl   e
        ld    a, 01fh
        adc   a, e
        ld    (lpbBuf_rightMargin), a
        ld    a, 01fh
        sub   e
        ld    (lpbBuf_leftMargin), a
        ld    a, (lpbBuf_videoMode)
        and   00eh
        cp    004h
        jr    nz, l041f
        ld    a, 008h
        ld    (paletteSize), a
        ld    (attributeMode), a
        jr    l0457
l041f:  ld    a, (lpbBuf_videoMode)
        and   060h
        jr    nz, l0430
        ld    (attributeMode), a
        ld    a, 002h
        ld    (paletteSize), a
        jr    createLPT
l0430:  cp    020h
        jr    nz, l043f
        ld    a, 004h
        ld    (paletteSize), a
        xor   a
        ld    (attributeMode), a
        jr    createLPT
l043f:  cp    040h
        jr    nz, l044e
        ld    a, 008h
        ld    (paletteSize), a
        xor   a
        ld    (attributeMode), a
        jr    l0457
l044e:  xor   a
        ld    (paletteSize), a
        ld    (attributeMode), a
        jr    createLPT
l0457:  ld    a, (ix+001h)
        cp    049h
        jr    nz, createLPT
        ld    a, (imageFileChannel)
        call  readImageFileByte
        ld    a, b
        ld    (fixBias), a

createLPT:
        xor   a
        ld    (fieldNum), a
        ld    a, (segmentTable+0)
        out   (0b1h), a
        ld    de, (videoMemoryBaseAddr)
        res   7, d
        set   6, d
        ld    (lptWriteAddr), de
l047c:  ld    de, (lptWriteAddr)
        ld    hl, 03f06h
        ld    (vsyncBegin), hl
        ld    hl, 0203fh
        ld    (vsyncEnd), hl
        ld    a, 0fch
        ld    (nBlankLines), a
        ld    a, (fieldNum)
        bit   0, a
        jr    z, l04af
        bit   7, (ix+005h)
        jr    z, l04af
        ld    hl, 03f20h
        ld    (vsyncBegin), hl
        ld    hl, 0383fh
        ld    (vsyncEnd), hl
        ld    a, 0fbh
        ld    (nBlankLines), a
l04af:  ld    hl, lptBBorder
        ld    bc, (16*2)+(lptTBorder-lptVBlank)
        ldir
        ld    (lptWriteAddr), de
        ld    hl, (pixelDataBaseAddr)
        ld    a, (fieldNum)
        ld    de, (pixelBytesPerField)
        call  multDEByAAddToHL
        ld    a, (attributeMode)
        or    a
        jr    z, l04de
        ld    (lpbBuf_LD2), hl
        ld    hl, (attrDataBaseAddr)
        ld    a, (fieldNum)
        ld    de, (attrBytesPerField)
        call  multDEByAAddToHL
l04de:  ld    (lpbBuf_LD1), hl
        ld    a, (ix+004h)
        or    a
        jp    nz, l054d
        ld    a, (ix+001h)
        cp    049h
        jr    nz, l0500
        ld    a, (paletteSize)
        or    a
        jr    z, l0500
        ld    c, a
        ld    b, 0
        ld    de, paletteBuf
        ld    a, (imageFileChannel)
        call  readImageFileBlock
l0500:  ld    de, (lptWriteAddr)
        ld    hl, lpbBuf
        ld    bc, 16
        ld    a, (ix+007h)
        or    a
        jr    z, l053e
        ld    a, (ix+006h)
        or    a
        jr    z, l053e
        xor   a
        push  hl
        push  bc
        ld    (hl), a
        ldir
        ld    a, (attributeMode)
        or    a
        ld    bc, (pixelBytesPerLPB-1)
        jr    z, l0533
        ld    hl, (lpbBuf_LD2)
        ld    c, 0
        add   hl, bc
        ld    (lpbBuf_LD2), hl
        ld    bc, (attrBytesPerLPB-1)
l0533:  ld    hl, (lpbBuf_LD1)
        ld    c, 0
        add   hl, bc
        ld    (lpbBuf_LD1), hl
        pop   bc
        pop   hl
        xor   a
l053e:  sub   (ix+006h)
        ld    (hl), a
        push  de
        ldir
        ld    (lptWriteAddr), de
        pop   hl
        jp    l05a1
l054d:  xor   a
        sub   (ix+004h)
        ld    (lpbBuf_nLines), a
        ld    hl, (nPaletteLines)
l0557:  push  hl
        ld    bc, (paletteSize)
        ld    b, 000h
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
        jr    z, l058e
        ld    hl, (lpbBuf_LD2)
        ld    bc, (pixelBytesPerLPB)
        add   hl, bc
        ld    (lpbBuf_LD2), hl
        ld    bc, (attrBytesPerLPB)
        jr    l0592
l058e:  ld    bc, (pixelBytesPerLPB)
l0592:  ld    hl, (lpbBuf_LD1)
        add   hl, bc
        ld    (lpbBuf_LD1), hl
        pop   de
        pop   hl
        dec   hl
        ld    a, h
        or    l
        jr    nz, l0557
        ex    de, hl
l05a1:  inc   hl
        ld    a, (fieldNum)
        inc   a
        ld    (fieldNum), a
        cp    (ix+00bh)
        jp    nz, l047c
        set   0, (hl)

        ld    a, (fixBias)              ; set FIXBIAS
        out   (080h), a
        sla   a
        sla   a
        sla   a
        ld    d, a
        ld    bc, 0011ch                ; C = BIAS_VID, B = set EXOS variable
        rst   030h
        defb 16
        ld    a, (ix+009h)              ; set border color
        out   (081h), a
        ld    d, a
        ld    bc, 0011bh                ; C = BORD_VID, B = set EXOS variable
        rst   030h
        defb 16

loadVideoData:
        di
        xor   a
        ld    (systemSegmentBackup), a
        ld    hl, (videoMemoryBaseAddr)
        ld    b, 004h
l05d7:  srl   h
        rr    l
        djnz  l05d7
        ld    a, l
        out   (082h), a
        ld    a, h
        or    0c0h
        out   (083h), a
        ld    a, (compressionType)
        or    a
        jp    nz, decompressImageData
        ld    ix, segmentTable
        ld    de, (lptWriteAddr)
l05e0:  ld    bc, (videoDataRemaining)
        ld    a, b
        and   0c0h
        jr    z, l05e4
        ld    bc, 04000h
l05e4:  ld    hl, 08000h
        ld    a, (ix+0)
        or    a
        jr    z, l05f0
        cp    0ffh
        jr    nz, l05e8
        ld    hl, (exosBoundary)
        res   7, h
        set   6, h
        inc   hl
l05e8:  cp    0fch
        jr    nc, l05ea
        ld    (systemSegmentBackup), a
l05ea:  out   (0b1h), a
        inc   ix
        or    a
        sbc   hl, de
        jr    c, l05f0
        push  hl
        sbc   hl, bc
        pop   hl
        jr    nc, l05ec
        ld    b, h
        ld    c, l
l05ec:  ld    a, b
        or    c
        jr    z, l05f0
        ld    hl, (videoDataRemaining)
        sbc   hl, bc
        ld    (videoDataRemaining), hl
        ld    a, (imageFileChannel)
        rst   030h
        defb 6
        or    a
        jr    nz, l05f0
        ld    de, 04000h
        jr    l05e0
l05f0:  ld    a, 0ffh
        out   (0b2h), a
        ld    a, (imageFileChannel)
        rst   030h                      ; close image file
        defb 3

        di
        call  swapSegments

keyboardWait:
l06f2:  ld    d, 004h
l06f4:  ld    hl, 08000h
l06f7:  ld    b, 009h
        ld    a, b
l06fa:  cp    004h
        out   (0b5h), a
        in    a, (0b5h)
        jr    nz, l073e
        ld    e, 001h
        bit   7, a
        jr    z, l072b                  ; F1 ?
        inc   e
        bit   6, a
        jr    z, l072b                  ; F2 ?
        inc   e
        bit   2, a
        jr    z, l072b                  ; F3 ?
        inc   e
        bit   0, a
        jr    z, l072b                  ; F4 ?
        inc   e
        bit   4, a
        jr    z, l072b                  ; F5 ?
        inc   e
        bit   3, a
        jr    z, l072b                  ; F6 ?
        inc   e
        bit   5, a
        jr    z, l072b                  ; F7 ?
        inc   e
        bit   1, a
        jr    nz, l0741                 ; not F8 ?
l072b:  ld    a, (l06f2+1)
        sub   d
        ld    c, a
        ld    a, e
        ld    (l06f2+1), a              ; set new timeout
        sub   c
        jr    c, l073b
        jr    z, l073b
        ld    a, 001h
l073b:  ld    d, a
        jr    l0741
l073e:  inc   a
        jr    nz, l0754
l0741:  dec   b
        ld    a, b
        cp    0ffh
        jr    nz, l06fa                 ; test all rows of the keyboard matrix
        dec   hl
        ld    a, h
        or    l
        jr    nz, l06f7
        dec   d
        jr    nz, l06f4
        ld    a, (iviewIniMissing)      ; timeout elapsed:
        or    a                         ; if not a slideshow, then continue
        jr    nz, l06f4                 ; waiting for keyboard events

l0754:  jp    l0755

l0755:  xor   a
        ld    (stopKeyStatus), a
        ld    a, 007h
        out   (0b5h), a                 ; check STOP key:
        in    a, (0b5h)
        bit   0, a
        jr    z, l076d                  ; is pressed ?
        ld    a, 003h
        out   (0b5h), a                 ; pressing ESC also quits the viewer
        in    a, (0b5h)
        bit   7, a
        jr    nz, l0772
l076d:  ld    a, 0e5h
        ld    (stopKeyStatus), a        ; 0E5h: STOP/ESC key, 0: any other key

l0772:  call  swapSegments
        ld    a, (stopKeyStatus)        ; return with stop key status
        or    a
        ret

swapSegments:
        ld    a, (systemSegmentBackup)
        or    a
        ret   z
        di
        in    a, (0b1h)
        ld    b, a
        in    a, (0b2h)
        ld    c, a
        push  bc
        ld    a, (segmentTable+4)
        out   (0b1h), a
        ld    a, (segmentTable+3)
        out   (0b2h), a
        ld    (lsws02+1), sp
        ld    hl, 04000h
        ld    sp, 08000h
lsws01: pop   de
        ld    c, (hl)
        ld    (hl), e
        inc   l
        ld    b, (hl)
        ld    (hl), d
        push  bc
        pop   bc
        inc   l
        jp    nz, lsws01
        inc   h
        jp    p, lsws01
lsws02: ld    sp, 00000h
        pop   bc
        ld    a, b
        out   (0b1h), a
        ld    a, c
        out   (0b2h), a
        ret

allocateMemory:
        ld    a, 0feh
        ld    hl, imageFileChannel
l079d:  cp    (hl)
        jr    z, l07b0
        cp    1
        jr    nz, l07ac
        ld    a, (iviewIniMissing)
        or    a
        ld    a, 001h
        jr    z, l07b0
l07ac:  push  af
        rst   030h
        defb 3
        pop   af
l07b0:  sub   1
        jr    nc, l079d
        ld    hl, fileNameBuffer
        ld    (hl), 0
        in    a, (0b0h)
        cp    0fch                      ; is the machine an EP64 ?
        jr    nz, l07d7
        ld    (segmentTable+0), a       ; yes, need to move video data
        ld    hl, viewerDataEnd
        ld    a, (compressionType)
        or    a
        jr    z, l07c0
        ld    hl, decompressorDataEnd
l07c0:  ld    (videoMemoryBaseAddr), hl
        ex    de, hl
        ld    hl, (pixelDataBaseAddr)
        add   hl, de
        ld    (pixelDataBaseAddr), hl
        ld    hl, (attrDataBaseAddr)
        add   hl, de
        ld    (attrDataBaseAddr), hl
l07d7:  rst   030h
        defb 24
        jr    nz, l07ff
        ld    a, c
        cp    0fch
        jr    c, l07f9
        jr    nz, l07e7
        ld    (segmentTable+0), a
        jr    l07d7
l07e7:  cp    0fdh
        jr    nz, l07f0
        ld    (segmentTable+1), a
        jr    l07d7
l07f0:  cp    0feh
        jr    nz, l07f9
        ld    (segmentTable+2), a
        jr    l07d7
l07f9:  ld    a, (segmentTable+3)
        or    a
        jr    z, l07fc
        ld    (hl), c
        inc   hl
        ld    (hl), 0
        jr    l07d7
l07fc:  ld    a, c
        ld    (segmentTable+3), a
        jr    l07d7
l07ff:  cp    07fh
        ret   nz
        dec   de
        ld    (exosBoundary), de
        rst   030h
        defb 23
        ld    a, (segmentTable+3)
        or    a
        ld    a, 0ffh
        jr    nz, l0808
        ld    (segmentTable+3), a
        jr    l080c
l0808:  ld    (segmentTable+4), a
l080c:  ld    hl, fileNameBuffer
l0811:  ld    a, (hl)
        or    a
        ret   z
        ld    c, a
        rst   030h
        defb 25
        inc   hl
        jr    l0811

freeAllMemory:
        ld    b, 5
        ld    hl, segmentTable
lfam01: push  bc
        ld    c, (hl)
        ld    (hl), 0
        inc   hl
        rst   030h
        defb 25
        pop   bc
        djnz  lfam01
        ret

multEByAToHL:
        ld    d, 0
multDEByAToHL:
        ld    hl, 0
multDEByAAddToHL:
        ld    b, 8
l087a:  rrca
        jr    nc, l087e
        add   hl, de
l087e:  sla   e
        rl    d
        djnz  l087a
        ret

; divide HL by a, and return the result in DE (the LSB is also in A)
; the remainder is returned in HL

divHLByAToDE:
        ld    e, l
        ld    l, h
        ld    b, a
        call  l0894
        ld    d, a
        ld    h, l
        ld    l, e
        ld    b, c
        call  l0896
        ld    e, a
        ret
l0894:  ld    h, 0
l0896:  ld    c, 0
        ld    a, 1
l089a:  srl   b
        rr    c
        sbc   hl, bc
        jr    nc, l08a3
        add   hl, bc
l08a3:  rla
        jr    nc, l089a
        cpl
        ret

; -----------------------------------------------------------------------------

decompressImagePalette:
        push ix
        ld b, 4
ldip01: push bc
        push de
        push hl
        ld a, (imageFileChannel)
        rst 030h
        defb 5
        or a
        jr nz, ldip02
        pop hl
        pop de
        ld e, d
        ld d, l
        ld l, h
        ld h, b
        pop bc
        djnz ldip01
        ld a, h
        or l
        jr z, ldip02
        ld a, d
        or e
        jr z, ldip02
        ld (decompDataRemaining), hl
        ld hl, 04000h
        sbc hl, de
        jr c, ldip02
        ld (paletteDataReadAddr), hl
        ld d, h
        ld e, l
        ld bc, decompressorDataEnd
        sbc hl, bc
        jr c, ldip02
        ld hl, decompressBuffer
        ld ix, decompressTables
        ld iy, decompSegmentTable
        in a, (0b0h)
        ld (iy+0), a
        ld (iy+1), 0
        call decompressData
        jr nz, ldip02
        pop ix
        xor a
        ret
ldip02: pop ix
        scf
        sbc a, a
        ret

decompressImageData:
        ld b, 4
ldid01: push bc
        push de
        push hl
        ld a, (imageFileChannel)
        rst 030h
        defb 5
        or a
        jp nz, ldid02
        pop hl
        pop de
        ld e, d                         ; DE = uncompressed size
        ld d, l
        ld l, h                         ; HL = compressed size
        ld h, b
        pop bc
        djnz ldid01
        ld a, h
        or l
        jp z, ldid02                    ; both sizes must be non-zero
        ld a, d
        or e
        jp z, ldid02
        ld (decompDataRemaining), hl
        in a, (0b0h)                    ; check machine type
        cp 0fch
        jp c, ldid03                    ; not an EP64 ?
        ld hl, (lptWriteAddr)
        res 6, h
        ld (lptWriteAddr), hl
        inc hl
        inc hl
        inc hl
        inc hl
        add hl, de                      ; end address of uncompressed data
        jp c, resetRoutine              ; does not fit in 64K memory: error
        ld a, h
        cp 0c0h
        jr c, ldid03                    ; system segment not used ?
        ld a, (exosBoundary)
        add a, 002h
        ld c, a
        ld a, (exosBoundary+1)
        add a, 0c0h
        ld b, a
        push hl
        sbc hl, bc
        pop hl
        jr c, ldid03                    ; system data is not overwritten ?
        ld a, coldReset&0ffh            ; if it is, then cannot return
        ld (l0754+1), a                 ; after displaying the image,
        ld a, coldReset/256
        ld (l0754+2), a
        ld a, keyboardWait&0ffh         ; cannot close the input file,
        ld (ldid05+1), a
        ld a, keyboardWait/256
        ld (ldid05+2), a
        push hl                         ; and need to load all compressed data
        ld a, (segmentTable+1)          ; into memory first
        out (0b1h), a
        ld a, (segmentTable+2)
        out (0b2h), a
        ld a, (segmentTable+3)
        out (0b3h), a
        ld de, (lptWriteAddr)
        ld bc, (decompDataRemaining)
        ld a, (imageFileChannel)
        rst 030h
        defb 6
        or a
        jp nz, resetRoutine
        di
        ld h, d                         ; HL = end of compressed data
        ld l, e
        pop de                          ; DE = end of uncompressed data
        ld bc, (decompDataRemaining)    ; BC = length of compressed data
        dec de
        dec hl
        lddr
        ld h, d
        ld l, e
        inc hl                          ; HL = compressed data read address
        ld a, h
        rlca
        rlca
        and 3
        add a, segmentTable&0ffh
        ld c, a
        ld a, 0
        adc a, segmentTable/256
        ld b, a
        ld a, (bc)
        out (0b3h), a                   ; read from page 3
        set 7, h
        set 6, h
        ld a, l
        or a
        jr nz, ldid04
        dec h
        jr ldid04
ldid03: ld hl, decompressBuffer
ldid04: ld de, (lptWriteAddr)
        ld ix, decompressTables
        ld iy, decompSegmentTable
        ld a, (segmentTable)
        ld (iy+0), a
        ld a, (segmentTable+1)
        ld (iy+1), a
        ld a, (segmentTable+2)
        ld (iy+2), a
        ld a, (segmentTable+3)
        ld (iy+3), a
        ld (iy+4), 0
        call decompressData
        jr nz, ldid02
        xor a
ldid05: jp l05f0
ldid02: jp resetRoutine

readImageFileByte:
        push af
        ld a, (compressionType)
        or a
        jr nz, lrifb1
        pop af
        rst 030h
        defb 5
        ret
lrifb1: pop af
        push hl
        ld hl, (paletteDataReadAddr)
        bit 6, h
        jr nz, lrifb2
        ld b, (hl)
        inc hl
        ld (paletteDataReadAddr), hl
        pop hl
        xor a
        ret
lrifb2: pop hl
        ld b, 0ffh
        ld a, 0e5h
        add a, b
        ret

readImageFileBlock:
        push af
        ld a, (compressionType)
        or a
        jr nz, lribl1
        pop af
        rst 030h
        defb 6
        ret
lribl1: pop af
        push hl
        ld hl, (paletteDataReadAddr)
lribl2: ld a, b
        or c
        jr z, lribl4
lribl3: bit 6, h
        jr nz, lribl5
        ldi
        jp pe, lribl3
        ld (paletteDataReadAddr), hl
lribl4: pop hl
        xor a
        ret
lribl5: pop hl
        xor a
        sub 01ch
        ret

; -----------------------------------------------------------------------------

readBlock:
        push af
        bit 7, h
        jr z, lrbl01
        inc h
        jr z, lrbl04
        pop af
        di
        ret
lrbl04: ld h, 0c0h
        in a, (0b3h)
        inc a
        out (0b3h), a
        pop af
        di
        ret
lrbl01: push bc
        push de
        ld bc, (decompDataRemaining)
        ld a, b
        or a
        jr nz, lrbl03
        ld (decompDataRemaining), a
        ld (decompDataRemaining+1), a
        or c
        jp z, decompressError
lrbl02: ld a, (imageFileChannel)
        ld d, h
        ld e, l
        rst 030h
        defb 6
        or a
        jp nz, decompressError
        pop de
        pop bc
        pop af
        di
        ret
lrbl03: dec a
        ld (decompDataRemaining+1), a
        ld bc, 00100h
        jr lrbl02

; =============================================================================

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
        call getSegment                 ; get first output segment
        dec de
        exx
        ld e, 080h                      ; initialize shift register
        exx
        call read8Bits                  ; skip checksum byte
ldd_01: call decompressDataBlock        ; decompress all blocks
        jr z, ldd_01
        inc de
        defb 0feh                       ; = CP nn

decompressError:
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

getSegment:
        set 7, d                        ; write decompressed data to page 2
        res 6, d
        push af
        ld a, (iy+0)
        or a
        jr z, decompressError
        cp 0fch
        jr nc, lgs_01
        ld (systemSegmentBackup), a
lgs_01: out (0b2h), a                   ; use pre-allocated segment ?
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
slotBitsTableL          equ slotBitsTable
slotBitsTableO1         equ slotBitsTableL+nLengthSlots
slotBitsTableO2         equ slotBitsTableO1+nOffs1Slots
slotBitsTableO3         equ slotBitsTableO2+nOffs2Slots
decodeTableEnd          equ slotBitsTable+(totalSlots*3)

decompressDataBlock:
        call read8Bits                  ; read number of symbols - 1 (BC)
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
        call readEncodedValue           ; decode match length - 1
        ld l, b                         ; save length (L: LSB, C: MSB)
        ld c, a
        or b
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
        call readEncodedValue           ; decode match offset
        ld h, a
        ld a, e                         ; calculate LZ77 match read address
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
        jr z, lddb14                    ; return to main decompress loop
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

readEncodedValue:
        defb 0ddh
        ld l, a
        xor a
        ld h, a
        ld b, (ix+0)
        cp b
        jr z, lrev03
lrev01: exx
        sla e
        jr z, lrev04
lrev02: exx
        adc a, a
        rl h
        djnz lrev01
lrev03: add a, (ix+totalSlots)
        ld b, a
        ld a, h
        adc a, (ix+(totalSlots*2))
        ret                             ; return LSB in B, MSB in A
lrev04: inc l
        call z, readBlock
        ld e, (hl)
        rl e
        jp lrev02


; =============================================================================

exdosCommandName:
        defb 6
        defm "EXDOS"
        defb 0fdh
exdosCommandStatus:
        defb 000h
iviewIniFileName:
        defb 9
        defm "IVIEW.INI"
iviewIniMissing:
        defb 0ffh
fileCommandName:
        defb 7
        defm "FILE "
        defw fileNameBuffer

lpbBuf:
        defb 0ffh, 012h, 03fh, 000h, 000h, 000h, 000h, 000h
paletteBuf:
        defb 000h, 0ffh, 0feh, 0fdh, 0fch, 0fbh, 0fah, 0f9h

lpbBuf_nLines           equ     lpbBuf+0
lpbBuf_videoMode        equ     lpbBuf+1
lpbBuf_leftMargin       equ     lpbBuf+2
lpbBuf_rightMargin      equ     lpbBuf+3
lpbBuf_LD1              equ     lpbBuf+4
lpbBuf_LD2              equ     lpbBuf+6

lptBBorder:
        defb 069h, 012h, 03fh, 000h, 000h, 000h, 000h, 000h
        defb 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
lptVBlank:
        defb 0fdh, 000h, 03fh, 000h, 000h, 000h, 000h, 000h
        defb 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
        defb 0feh, 000h, 006h, 03fh, 000h, 000h, 000h, 000h
        defb 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
        defb 0ffh, 080h, 03fh, 020h, 000h, 000h, 000h, 000h
        defb 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
        defb 0fch, 002h, 006h, 03fh, 000h, 000h, 000h, 000h
        defb 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
lptTBorder:
        defb 069h, 012h, 03fh, 000h, 000h, 000h, 000h, 000h
        defb 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h

vsyncBegin              equ     lptVBlank+18
vsyncEnd                equ     lptVBlank+34
nBlankLines             equ     lptVBlank+48

videoMemoryBaseAddr:
        defw 00000h
attrDataBaseAddr:
        defw 00000h
pixelDataBaseAddr:
        defw 00000h
exosBoundary:
        defw 0bfffh

viewerCodeEnd:

; -----------------------------------------------------------------------------

segmentTable:
        defb 000h, 000h, 000h, 000h, 000h

stopKeyStatus:
        defb 000h                       ; 0E5h if STOP key is pressed
nPaletteLines:
        defw 00000h
lptFieldSize:
        defw 00000h
totalLPTSize:
        defw 00000h
attrBytesPerLPB:
        defw 00000h
attrBytesPerField:
        defw 00000h
totalAttrBytes:
        defw 00000h
pixelBytesPerLPB:
        defw 00000h
pixelBytesPerField:
        defw 00000h
totalPixelBytes:
        defw 00000h
videoDataRemaining:
        defw 00000h
paletteSize:
        defb 000h
attributeMode:
        defb 000h
fixBias:
        defb 000h
compressionType:
        defb 000h
lptWriteAddr:
        defw 00000h

fieldNum:
        defb 000h
systemSegmentBackup:
        defb 000h

systemDataLength:
        defw 00000h
systemDataAddress:
        defw 00000h
imageFileChannel:
        defb 000h


fileHeaderBuf:
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

fileNameBuffer:
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

        align 16

viewerDataEnd:

paletteDataReadAddr:
        defw 00000h
decompDataRemaining:
        defw 00000h
decompSegmentTable:
        defb 000h, 000h, 000h, 000h, 000h

        align 256

decompressBuffer:
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

        align 256

decompressTables:
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        defb 0, 0, 0, 0

        align 16

decompressorDataEnd:

        dephase

