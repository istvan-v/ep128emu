
        org   0c000h

        defm  "MOPS"
        defb  4
        defm  "FILE"

        defw  irqRoutine1
        defw  initDevice
        defb  9
        defw  irqRoutine2
        defw  characterIO
        defw  blockIO
        defw  fileOpen
        defw  fileClose
        defw  blockVerify
        defw  fileSeekSet
        defw  fileSeekCur
        defw  fileSeekEnd

; n = 0: initialization (close file)
; n = 1: open input file (DE = name address), store file name at IX+8
; n = 2: open output file (DE = name address), store file name at IX+8
; n = 3: close file
; n = 4: close file
; n = 5: read character to C
; n = 6: write character from C
; n = 7: get file size in BC:DE
; n = 8: get file position in BC:DE
; n = 9: set file position to BC:DE

    macro ep128emuSystemCall n
        defb  0edh, 0feh, 0feh, 0f0h | n
    endm

initDevice:
        push  ix
        ex    (sp), hl
        ld    a, l
        and   0f0h
        ld    l, a
        sub   10h
        rlca
        rlca
        and   03h
        or    80h
        ld    (0b05h), a
        ld    (0b0dh), a
        ld    (hl), 4
        inc   l
        ld    (hl), 'F'
        inc   l
        ld    (hl), 'I'
        inc   l
        ld    (hl), 'L'
        inc   l
        ld    (hl), 'E'
        inc   l
        set   1, l
        set   3, l
        ld    (hl), 0                   ; FFh = file was written to
        inc   hl
        ld    (hl), 0ffh                ; name length or FFh if file not open
        ld    l, a
        ld    a, 0f7h
.l1:    rlca
        dec   l
        jp    m, .l1
        ld    hl, 0b10h
        and   (hl)
        ld    (hl), a
        pop   hl
        ep128emuSystemCall  0
        or    a

irqRoutine1:
irqRoutine2:
        ret

characterIO:
        jp    p, .l1                    ; write character?
        ep128emuSystemCall  5
        or    a
        ret
.l1:    bit   7, (ix + 8)
        ld    a, 0e9h                   ; file not open
        ret   nz
        ld    (ix + 7), 0ffh
        ep128emuSystemCall  6
        or    a
        ret

blockIO:
        rla
        bit   7, (ix + 8)
        ld    a, 0e9h                   ; file not open
        ret   nz
        push  hl
        ex    de, hl
        ld    e, c
        ld    d, b
        jr    c, .l2                    ; read block?
        ld    (ix + 7), 0ffh
        jp    .l5
.l1:    ld    (hl), c
        inc   hl
        dec   de
.l2:    ld    a, e
        or    d
        jr    z, .l3
        ep128emuSystemCall  5
        or    a
        jp    z, .l1                    ; no error?
.l3:    ld    c, e
        ld    b, d
        ex    de, hl
        pop   hl
        ret
.l4:    inc   hl
        dec   de
.l5:    ld    a, e                      ; write block
        or    d
        jr    z, .l3
        ld    c, (hl)
        ep128emuSystemCall  6
        or    a
        jp    z, .l4                    ; no error?
        jr    .l3

checkCASExtension:
        push  ix
        ex    (sp), hl
        ld    a, l
        and   0f0h
        add   a, 16
        ld    l, a
        ld    a, (hl)
        add   a, 256 - 4
        jr    nc, .l1
        adc   a, l
        ld    l, a
        ld    a, '.'
        cp    (hl)
        jr    nz, .l1
        inc   l
        ld    a, 'C'
        cp    (hl)
        jr    nz, .l1
        inc   l
        ld    a, 'A'
        cp    (hl)
        jr    nz, .l1
        inc   l
        ld    a, 'S'
        cp    (hl)
.l1:    pop   hl
        ret

fileOpen:
        jp    m, fileOpenR              ; open input file?
        xor   a

; Carry = 1: open input file

addCASExtension:
        push  af
        push  bc
        push  de
        ld    a, (de)
        ld    b, a
        dec   a
        cp    25
        jr    c, .l2                    ; not empty or too long name?
.l1:    pop   de
        pop   bc
        pop   af
        or    a
        jp    z, fileOpenW
        ret
.l2:    inc   de
        ld    a, (de)
        cp    '.'
        jr    z, .l1                    ; there is already an extension?
        djnz  .l2
        ex    (sp), hl
        ld    c, (hl)
        add   hl, bc
        ld    a, c
        add   a, 13
        add   a, ixl
        ld    e, a
        ld    d, ixh
        ex    de, hl
        ld    (hl), 'S'                 ; add .CAS extension
        dec   hl
        ld    (hl), 'A'
        dec   hl
        ld    (hl), 'C'
        dec   hl
        ld    (hl), '.'
        dec   hl
        ex    de, hl
        lddr
        sub   e
        ld    (de), a
        pop   hl
        pop   bc
        pop   af
        jr    nc, fileOpenW

fileOpenR:
        ep128emuSystemCall  3
        ld    (ix + 7), 0
        ld    (ix + 8), 0ffh
        or    a
        jr    z, .l1
        cp    0e9h                      ; file not open
        ret   nz
.l1:    ep128emuSystemCall  1
        or    a
        jr    nz, .l5
        call  checkCASExtension
        jr    z, .l2
        xor   a
        ret
.l2:    push  bc
        ld    b, 80h                    ; skip .CAS header
.l3:    ep128emuSystemCall  5
        or    a
        jr    nz, .l4
        djnz  .l3
        pop   bc
        ret
.l4:    pop   bc
        ep128emuSystemCall  3
        ld    (ix + 8), 0ffh
        ld    a, 0e7h                   ; invalid .CAS header
        ret
.l5:    cp    0a1h                      ; file not found
        scf
        jp    z, addCASExtension
        ret

fileOpenW:
        ep128emuSystemCall  4
        ld    (ix + 7), 0
        ld    (ix + 8), 0ffh
        or    a
        jr    z, .l1
        cp    0e9h                      ; file not open
        ret   nz
.l1:    ep128emuSystemCall  2
        or    a
        ret   nz
        call  checkCASExtension
        jr    z, .l2
        xor   a
        ret
.l2:    push  bc                        ; write .CAS header for empty file
        push  de
        ld    bc, 0000h
        ld    de, 0080h
        call  writeCASHeader.l1
        pop   de
        pop   bc
        ret   z
        push  af
        ep128emuSystemCall  4
        ld    (ix + 7), 0
        ld    (ix + 8), 0ffh
        pop   af
        ret

updateCASHeader:
        ep128emuSystemCall  7
        or    a
        ret   nz

writeCASHeader:
        push  bc
        push  de
        ld    bc, 0000h
        ld    de, 0000h
        ep128emuSystemCall  9
        pop   de
        pop   bc
        or    a
        ret   nz
.l1:    ld    a, e
        or    d
        jr    nz, .l2
        dec   bc
.l2:    dec   de
        sla   e
        rl    d
        rl    c
        rl    b
        ld    a, 0e7h                   ; invalid .CAS header
        ret   nz
        ld    b, c
        ld    a, d
        or    b
        cp    1
        sbc   a, a                      ; A = FFh if 0 blocks
        or    e
        rrca
        inc   a
        ld    e, a                      ; E = number of bytes used in last block
        ld    c, 11h                    ; byte 0 = 11h
        ep128emuSystemCall  6
        or    a
        ret   nz
        ld    c, a                      ; byte 1 = 00h
        ep128emuSystemCall  6
        or    a
        ret   nz
        ld    c, d                      ; byte 2 = number of blocks LSB
        ep128emuSystemCall  6
        or    a
        ret   nz
        ld    c, b                      ; byte 3 = number of blocks MSB
        ep128emuSystemCall  6
        or    a
        ret   nz
        ld    a, e                      ; byte 4 = bytes used in last block
        ld    b, 7ch
.l3:    ld    c, a
        ep128emuSystemCall  6
        or    a
        ret   nz
        djnz  .l3
        ret

fileClose:
        bit   7, (ix + 8)
        ld    a, 0e9h                   ; file not open
        ret   nz
        bit   7, (ix + 7)
        jr    z, .l1                    ; file was not changed?
        ld    (ix + 7), 0
        call  checkCASExtension
        jr    nz, .l1                   ; not .CAS format?
        push  bc
        push  de
        call  updateCASHeader
        pop   de
        pop   bc
        or    a
        jr    z, .l1
        push  af
        ld    (ix + 8), 0ffh
        ep128emuSystemCall  3
        pop   af
        ret
.l1:    ld    (ix + 8), 0ffh
        ep128emuSystemCall  3
        or    a
        ret

blockVerify:
        push  hl
        ex    de, hl
        ld    e, c
        ld    d, b
        bit   7, (ix + 8)
        jr    z, .l2
        ld    a, 0e9h                   ; file not open
        jr    .l4
.l1:    ld    a, (hl)
        xor   c
        jr    nz, .l3
        inc   hl
        dec   de
.l2:    ld    a, e
        or    d
        jr    z, .l4
        ep128emuSystemCall  5
        or    a
        jp    z, .l1                    ; no error?
        defb  0cah                      ; = JP Z, nnnn
.l3:    ld    a, 0e8h                   ; verify error
.l4:    ld    c, e
        ld    b, d
        ex    de, hl
        pop   hl
        ret

fileSeekEnd:
        push  bc
        push  de
        ep128emuSystemCall  7
        or    a
        jp    z, fileSeekCur.l1
.l1:    pop   de
        pop   bc
        ret

fileSeekCur:
        push  bc
        push  de
        ep128emuSystemCall  8
        or    a
        jr    nz, fileSeekEnd.l1
.l1:    ex    (sp), hl
        add   hl, de
        ld    e, l
        ld    d, h
        pop   hl
        ex    (sp), hl
        adc   hl, bc
        ld    c, l
        ld    b, h
        pop   hl

fileSeekSet:
        ep128emuSystemCall  9
        or    a
        ret

        block 0e000h - $, 0ffh

