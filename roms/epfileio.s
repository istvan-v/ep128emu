
        output  "epfileio.rom"

    macro exos n
        rst   030h
        defb  n
    endm

    macro ep128emuSystemCall n
        defb  0edh, 0feh, 0feh, n
    endm

        org   0c000h

        defm  "EXOS_ROM"
        defw  (deviceChain & 03fffh) + 04000h

        jp    main_

        align 16
l_c010:
        defw  0                         ; XX_NEXT_LOW, XX_NEXT_HI
        defw  0fffdh                    ; XX_RAM_LOW, XX_RAM_HI
l_c014: defb  000h                      ; DD_TYPE
        defb  000h                      ; DD_IRQFLAG
        defb  000h                      ; DD_FLAGS
        defw  (entryPointTable & 03fffh) + 04000h   ; DD_TAB_LOW, DD_TAB_HI
        defb  030h                      ; DD_TAB_SEG
        defb  000h                      ; DD_UNIT_COUNT
fileDeviceName:
        defb  4                         ; DD_NAME
        defm  "FILE"
deviceChain:
        defb  low (deviceChain - l_c014)  ; XX_SIZE

        block 5, 000h

deviceNameTable:
        defb  4
        defm  "TAPE"
        defb  4
        defm  "DISK"
        defb  4
        defm  "FILE"

entryPointTable:
        defw  irqRoutine
        defw  openChannel
        defw  createChannel
        defw  closeChannel
        defw  destroyChannel
        defw  readCharacter
        defw  readBlock
        defw  writeCharacter
        defw  writeBlock
        defw  channelReadStatus
        defw  setChannelStatus
        defw  specialFunction
        defw  initDevice
        defw  bufferMoved
        defw  invalidFunction
        defw  invalidFunction

irqRoutine:
        ep128emuSystemCall  0
        ret

        align 16
openChannel:
        push  af
        ep128emuSystemCall  1
        cp    0
        jr    z, openChannel_
        inc   sp
        inc   sp
        ret

createChannel:
        push  af
        ep128emuSystemCall  2
        cp    0
        jr    z, createChannel_
        inc   sp
        inc   sp
        ret

closeChannel:
        ep128emuSystemCall  3
        ret

destroyChannel:
        ep128emuSystemCall  4
        ret

readCharacter:
        ep128emuSystemCall  5
        ret

readBlock:
        jp    readBlock_

writeCharacter:
        ep128emuSystemCall  7
        ret

writeBlock:
        jp    writeBlock_

channelReadStatus:
        ep128emuSystemCall  9
        ret

setChannelStatus:
        jp    setChannelStatus_

specialFunction:
        ep128emuSystemCall  11
        ret

initDevice:
        ep128emuSystemCall  12
        jr    main_.l1

bufferMoved:
        ep128emuSystemCall  13
        ret

        align 16
defDevCommandBase:
        defm  "DEF_DEV_"
        defb  000h, 000h

invalidFunction:
        ld    a, 0e7h                   ; .NOFN
        ret

openChannel_:
        pop   af
        push  af                        ; A = channel number
        ld    bc, 0
        ld    de, 1
        exos  27
        cp    0
        jr    nz, .l1
        inc   sp
        inc   sp
        ret
.l1:    pop   hl
        push  af
        ld    a, h
        ep128emuSystemCall  3
        pop   af
        ret

createChannel_:
        pop   af
        push  af                        ; A = channel number
        ld    bc, 0
        ld    de, 1
        exos  27
        cp    0
        jr    nz, .l1
        inc   sp
        inc   sp
        ret
.l1:    pop   hl
        push  af
        ld    a, h
        ep128emuSystemCall  4
        pop   af
        ret

main_:
        ld    a, c
        cp    2
        jr    z, parseCommand
        cp    5
        jr    z, errorString
        cp    8
        ret   nz
.l1:    ep128emuSystemCall  14
        or    a
        ret   z
        ld    a, 2                      ; FILE:
        jr    setDefaultDevice

errorString:
        ld    l, b
        ld    h, (high errMsgTable) / 2
        add   hl, hl
        ld    a, (hl)
        inc   hl
        ld    h, (hl)
        ld    l, a
        ld    a, h
        cp    0
        jr    z, .l1
        ex    de, hl
        in    a, (0b3h)
        ld    b, a
        ld    c, 0
.l1:    ret

        assert  (deviceNameTable & 0ff00h) == ((deviceNameTable + 10) & 0ff00h)

parseCommand:
        ld    a, b
        cp    12                        ; length of DEF_DEV_FILE
        ret   nz
        push  bc
        push  de
        push  hl
        ld    b, 8
        ld    hl, defDevCommandBase
.l1:    inc   de
        ld    a, (de)
        cp    (hl)
        jr    nz, .l5
        inc   hl
        djnz  .l1
        ld    c, b
.l2:    push  de
        ld    a, c
        add   a, a
        add   a, a
        add   a, c
        ld    hl, deviceNameTable
        add   a, l
        ld    l, a
        ld    b, 4
.l3:    inc   de
        inc   hl
        ld    a, (de)
        cp    (hl)
        jr    nz, .l4
        djnz  .l3
.l4:    pop   de
        jr    z, .l6
        inc   c
        ld    a, 2
        cp    c
        jr    nc, .l2
.l5:    pop   hl
        pop   de
        pop   bc
        xor   a
        ret
.l6:    ld    a, c
        pop   hl
        pop   de
        srl   a
        ep128emuSystemCall  15
        ld    a, c
        pop   bc
        ld    c, 0

; A = 0: TAPE:
; A = 1: DISK:
; A = 2: FILE:

setDefaultDevice:
        push  bc
        push  de
        ld    b, a
        add   a, a
        add   a, a
        add   a, b
        ld    b, a
        cp    1
        sbc   a, a
        inc   a
        ld    c, a                      ; C = 0 for TAPE:, C = 1 otherwise
        ld    de, deviceNameTable
        ld    a, b
        add   a, e
        ld    e, a
        exos  19
        pop   de
        pop   bc
        ret

readBlockLoop:
        ld    a, l
        and   3
        jr    z, .l2
        dec   a
        jr    z, .l3
        dec   a
        jr    z, .l4
        jr    .l5
.l1:    inc   e
        ret   z
.l2:    ld    a, c
        ep128emuSystemCall  5
        or    a
        ret   nz
        ld    (hl), b
        inc   l
        inc   e
        ret   z
.l3:    ld    a, c
        ep128emuSystemCall  5
        or    a
        ret   nz
        ld    (hl), b
        inc   l
        inc   e
        ret   z
.l4:    ld    a, c
        ep128emuSystemCall  5
        or    a
        ret   nz
        ld    (hl), b
        inc   l
        inc   e
        ret   z
.l5:    ld    a, c
        ep128emuSystemCall  5
        or    a
        ret   nz
        ld    (hl), b
        inc   l
        jp    nz, .l1
        inc   h
        jp    p, .l1
        ld    a, ixl
        inc   a
        or    0fch
        ld    ixl, a
        ld    h, 040h
        ld    a, (ix)
        out   (0b1h), a
        jr    .l1

writeBlockLoop:
        ld    a, l
        and   3
        jr    z, .l2
        dec   a
        jr    z, .l3
        dec   a
        jr    z, .l4
        jr    .l5
.l1:    inc   e
        ret   z
.l2:    ld    a, c
        ld    b, (hl)
        ep128emuSystemCall  7
        or    a
        ret   nz
        inc   l
        inc   e
        ret   z
.l3:    ld    a, c
        ld    b, (hl)
        ep128emuSystemCall  7
        or    a
        ret   nz
        inc   l
        inc   e
        ret   z
.l4:    ld    a, c
        ld    b, (hl)
        ep128emuSystemCall  7
        or    a
        ret   nz
        inc   l
        inc   e
        ret   z
.l5:    ld    a, c
        ld    b, (hl)
        ep128emuSystemCall  7
        or    a
        ret   nz
        inc   l
        jp    nz, .l1
        inc   h
        jp    p, .l1
        ld    a, ixl
        inc   a
        or    0fch
        ld    ixl, a
        ld    h, 040h
        ld    a, (ix)
        out   (0b1h), a
        jr    .l1

writeBlock_:
        push  iy
        ld    iy, writeBlockLoop
        jr    readBlock_.l1

readBlock_:
        push  iy
        ld    iy, readBlockLoop
.l1:    push  hl
        ld    l, c
        ld    c, a
        ld    a, l
        cpl
        ld    l, a
        ld    a, b
        cpl
        ld    h, a
        push  ix
        ld    a, d
        rlca
        rlca
        or    0fch
        ld    ixl, a
        ld    ixh, 0bfh
        res   7, d
        set   6, d
        in    a, (0b1h)
        push  af
        ex    de, hl
        ld    a, (ix)
        out   (0b1h), a
        inc   e
        jr    nz, .l2
        inc   d
        jr    z, .l3
.l2:    call  .l5
        jr    nz, .l4
        inc   d
        jr    nz, .l2
.l3:    xor   a
.l4:    ex    de, hl
        ld    ixh, a
        pop   af
        out   (0b1h), a
        xor   a
        sub   l
        ld    c, a
        ld    a, 0
        sbc   a, h
        ld    b, a
        ld    a, ixl
        rrca
        rrca
        set   7, d
        and   d
        ld    d, a
        ld    a, ixh
        pop   ix
        pop   hl
        pop   iy
        or    a
        ret
.l5:    jp    (iy)

setChannelStatus_:
        ep128emuSystemCall  10
        ld    c, 0
        or    a
        ret   nz
        ld    hl, 8
        add   hl, de
        ld    e, l
        ld    d, h
        ld    a, h
        rlca
        rlca
        ld    hl, 0bffch
        or    l
        ld    l, a
        ld    bc, 0803h
.l1:    ld    a, (hl)
        out   (0b1h), a
        inc   l
        res   7, d
        set   6, e
        xor   a
.l2:    cp    b
        ret   z
        ld    (de), a
        dec   b
        inc   de
        bit   7, d
        jr    z, .l2
        jr    .l1

    if ($ & 001ffh) != 0
        block 00200h - ($ & 001ffh), 000h
    endif

errMsgTable:
        defw         0,        0,        0,        0,        0,        0  ; 00h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; 10h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; 20h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; 30h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; 40h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; 50h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; 60h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; 70h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; 80h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; 90h
        defw         0, errMsg97,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0, errMsgA1,        0,        0,        0,        0  ; A0h
        defw  errMsgA6, errMsgA7,        0,        0, errMsgAA,        0
        defw  errMsgAC,        0, errMsgAE,        0
        defw         0,        0,        0,        0,        0,        0  ; B0h
        defw         0,        0, errMsgB8,        0, errMsgBA,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; C0h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0, errMsgCF
        defw         0,        0,        0,        0,        0,        0  ; D0h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; E0h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0
        defw         0,        0,        0,        0,        0,        0  ; F0h
        defw         0,        0,        0,        0,        0,        0
        defw         0,        0,        0,        0

        phase $ - 08000h

errMsgCF:
        defb  14
        defm  "File not found"
errMsgBA:
        defb  9
        defm  "Not ready"
errMsgB8:
        defb  10
        defm  "Data error"
errMsgAE:
        defb  17
        defm  "Invalid parameter"
errMsgAC:
        defb  9
        defm  "Disk full"
errMsgAA:
        defb  19
        defm  "Directory not found"
errMsgA7:
        defb  14
        defm  "Read only file"
errMsgA6:
        defb  17
        defm  "Invalid file name"
errMsgA1:
        defb  23
        defm  "Invalid file attributes"
errMsg97:
        defb  11
        defm  "File exists"

        dephase

        size  16384

