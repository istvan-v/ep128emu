
;read EnterMice mouse input, returns:
;  Z flag = input is valid
;  H = X_REL
;  L = Y_REL
;  A bits 0 and 1 = buttons 1 and 2
;  D bits 0 to 2 = buttons 3 to 5
;  E bits 0 to 3 = mouse wheel Y
MOUSEINPUT:
                DI
VSYNCWAIT1:     IN A,(0B4H)
                AND 10H
                JR Z,VSYNCWAIT1
VSYNCWAIT2:     IN A,(0B4H)
                AND 10H
                JR NZ,VSYNCWAIT2
                LD C,4
READ_LOOP_1:    LD H,L
                LD L,D
                LD D,E
                LD E,01H
                LD A,02H                ;RTS low
READ_LOOP_2:    OUT (0B7H),A
                LD B,17                 ;25.6 us at 10 MHz without wait states
M_WAIT_LOOP:    DJNZ M_WAIT_LOOP
                LD B,4
READ_LOOP_3:    LD A,B
                OUT (0B5H),A
                IN A,(0B6H)
                RRA                     ;data is read from K column
                RRA
                RL E
                DJNZ READ_LOOP_3
                LD A,B                  ;RTS high
                JR NC,READ_LOOP_2
                DEC C
                JR NZ,READ_LOOP_1
                OUT (0B5H),A            ;A = 0
                IN A,(0B6H)
                CPL
                AND 06H                 ;bit 0 = left button,
                RRA                     ;bit 1 = right button
                BIT 7,D                 ;bits 5 to 7 of button state must be 0
                RET                     ;IRQ disabled, Z = mouse detected

