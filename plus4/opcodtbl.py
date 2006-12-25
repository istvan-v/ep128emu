#!/usr/bin/python

def generateOpcode(f, opNum, opName_):
    print >> f, '    // 0x%02X: %s' % (opNum, opName_)
    cnt = 0
    opName = opName_[:3].lower()
    if opName_.__len__() > 3:
        operand = opName_[4:].lower()
    else:
        operand = ''
    if operand == '#nn':
        print >> f, '    CPU_OP_RD_TMP,'
        if opName == 'nop':
            pass
        elif opName[:2] != 'ld':
            print >> f, '    CPU_OP_%s,' % opName.upper()
            cnt = cnt + 1
        else:
            print >> f, '    CPU_OP_SET_NZ,'
            print >> f, '    CPU_OP_LD_%s_TMP,' % opName[2].upper()
            cnt = cnt + 2
        cnt = cnt + 1
    elif operand != '':
        if opName[0] == 'b' and opName != 'bit':
            print >> f, '    CPU_OP_RD_L,'
            if opName in ['bpl', 'bvc', 'bne', 'bcc']:
                print >> f, '    CPU_OP_LD_TMP_00,'
            else:
                print >> f, '    CPU_OP_LD_TMP_FF,'
            if opName in ['bpl', 'bmi']:
                print >> f, '    CPU_OP_TEST_N,'
            elif opName in ['bvc', 'bvs']:
                print >> f, '    CPU_OP_TEST_V,'
            elif opName in ['bne', 'beq']:
                print >> f, '    CPU_OP_TEST_Z,'
            elif opName in ['bcc', 'bcs']:
                print >> f, '    CPU_OP_TEST_C,'
            print >> f, '    CPU_OP_BRANCH,'
            print >> f, '    CPU_OP_JMP_RELATIVE,'
            cnt = cnt + 5
        elif opName[0] != 'j':
            if operand == 'nn':
                print >> f, '    CPU_OP_RD_L,'
                print >> f, '    CPU_OP_ADDR_ZEROPAGE,'
                cnt = cnt + 2
            elif operand == 'nn,x' or operand == 'nn, x':
                print >> f, '    CPU_OP_RD_L,'
                print >> f, '    CPU_OP_ADDR_ZEROPAGE,'
                print >> f, '    CPU_OP_ADDR_X_SLOW,'
                print >> f, '    CPU_OP_ADDR_ZEROPAGE,'
                cnt = cnt + 4
            elif operand == 'nn,y' or operand == 'nn, y':
                print >> f, '    CPU_OP_RD_L,'
                print >> f, '    CPU_OP_ADDR_ZEROPAGE,'
                print >> f, '    CPU_OP_ADDR_Y_SLOW,'
                print >> f, '    CPU_OP_ADDR_ZEROPAGE,'
                cnt = cnt + 4
            elif operand == 'nnnn':
                print >> f, '    CPU_OP_RD_L,'
                print >> f, '    CPU_OP_RD_H,'
                cnt = cnt + 2
            elif operand == 'nnnn,x' or operand == 'nnnn, x':
                print >> f, '    CPU_OP_RD_L,'
                print >> f, '    CPU_OP_RD_H,'
                if opName in ['inc', 'dec', 'asl', 'lsr', 'rol', 'ror', 'sta']:
                    print >> f, '    CPU_OP_ADDR_X_SLOW,'
                else:
                    print >> f, '    CPU_OP_ADDR_X,'
                cnt = cnt + 3
            elif operand == 'nnnn,y' or operand == 'nnnn, y':
                print >> f, '    CPU_OP_RD_L,'
                print >> f, '    CPU_OP_RD_H,'
                if opName in ['inc', 'dec', 'asl', 'lsr', 'rol', 'ror', 'sta']:
                    print >> f, '    CPU_OP_ADDR_Y_SLOW,'
                else:
                    print >> f, '    CPU_OP_ADDR_Y,'
                cnt = cnt + 3
            elif operand == '(nn),y' or operand == '(nn), y':
                print >> f, '    CPU_OP_RD_L,'
                print >> f, '    CPU_OP_ADDR_ZEROPAGE,'
                print >> f, '    CPU_OP_LD_TMP_MEM,'
                print >> f, '    CPU_OP_INC_L,'
                print >> f, '    CPU_OP_LD_H_MEM,'
                print >> f, '    CPU_OP_LD_L_TMP,'
                if opName in ['inc', 'dec', 'asl', 'lsr', 'rol', 'ror', 'sta']:
                    print >> f, '    CPU_OP_ADDR_Y_SLOW,'
                else:
                    print >> f, '    CPU_OP_ADDR_Y,'
                cnt = cnt + 7
            elif operand == '(nn,x)' or operand == '(nn, x)':
                print >> f, '    CPU_OP_RD_L,'
                print >> f, '    CPU_OP_ADDR_ZEROPAGE,'
                print >> f, '    CPU_OP_ADDR_X_SLOW,'
                print >> f, '    CPU_OP_ADDR_ZEROPAGE,'
                print >> f, '    CPU_OP_LD_TMP_MEM,'
                print >> f, '    CPU_OP_INC_L,'
                print >> f, '    CPU_OP_LD_H_MEM,'
                print >> f, '    CPU_OP_LD_L_TMP,'
                cnt = cnt + 8
            else:
                print ' *** invalid addressing mode'
                raise SystemExit(-1)
            if opName[:2] != 'st' and opName != 'sax':
                print >> f, '    CPU_OP_LD_TMP_MEM,'
                cnt = cnt + 1
            if opName == 'nop':
                pass
            elif opName[:2] == 'ld':
                print >> f, '    CPU_OP_SET_NZ,'
                print >> f, '    CPU_OP_LD_%s_TMP,' % opName[2].upper()
                cnt = cnt + 2
            elif opName[:2] == 'st':
                print >> f, '    CPU_OP_LD_TMP_%s,' % opName[2].upper()
                print >> f, '    CPU_OP_LD_MEM_TMP,'
                cnt = cnt + 2
            elif opName in ['inc', 'dec', 'asl', 'lsr', 'rol', 'ror']:
                print >> f, '    CPU_OP_LD_MEM_TMP,'
                print >> f, '    CPU_OP_%s,' % opName.upper()
                print >> f, '    CPU_OP_LD_MEM_TMP,'
                cnt = cnt + 3
            elif opName == 'lax':
                print >> f, '    CPU_OP_SET_NZ,'
                print >> f, '    CPU_OP_LD_A_TMP,'
                print >> f, '    CPU_OP_LD_X_TMP,'
                cnt = cnt + 3
            elif opName == 'sax':
                print >> f, '    CPU_OP_SAX,'
                print >> f, '    CPU_OP_LD_MEM_TMP,'
                cnt = cnt + 2
            else:
                print >> f, '    CPU_OP_%s,' % opName.upper()
                cnt = cnt + 1
        elif opName == 'jmp' and operand == 'nnnn':
            print >> f, '    CPU_OP_RD_L,'
            print >> f, '    CPU_OP_RD_H,'
            print >> f, '    CPU_OP_LD_PC_HL,'
            cnt = cnt + 3
        elif opName == 'jmp' and operand == '(nnnn)':
            print >> f, '    CPU_OP_RD_L,'
            print >> f, '    CPU_OP_RD_H,'
            print >> f, '    CPU_OP_LD_TMP_MEM,'
            print >> f, '    CPU_OP_INC_L,'
            print >> f, '    CPU_OP_LD_H_MEM,'
            print >> f, '    CPU_OP_LD_L_TMP,'
            print >> f, '    CPU_OP_LD_PC_HL,'
            cnt = cnt + 7
        elif opName == 'jsr' and operand == 'nnnn':
            print >> f, '    CPU_OP_RD_L,'
            print >> f, '    CPU_OP_WAIT,'
            print >> f, '    CPU_OP_PUSH_PCH,'
            print >> f, '    CPU_OP_PUSH_PCL,'
            print >> f, '    CPU_OP_RD_H,'
            print >> f, '    CPU_OP_LD_PC_HL,'
            cnt = cnt + 6
        else:
            print ' *** invalid instruction'
            raise SystemExit(-1)
    else:
        if opName == 'brk':
            print >> f, '    CPU_OP_RD_TMP,'
            print >> f, '    CPU_OP_PUSH_PCH,'
            print >> f, '    CPU_OP_PUSH_PCL,'
            print >> f, '    CPU_OP_BRK,'
            print >> f, '    CPU_OP_PUSH_TMP,'
            print >> f, '    CPU_OP_LD_TMP_MEM,'
            print >> f, '    CPU_OP_INC_L,'
            print >> f, '    CPU_OP_LD_H_MEM,'
            print >> f, '    CPU_OP_LD_L_TMP,'
            print >> f, '    CPU_OP_LD_PC_HL,'
            cnt = cnt + 10
        elif opName == 'int':
            print >> f, '    CPU_OP_PUSH_PCH,'
            print >> f, '    CPU_OP_PUSH_PCL,'
            print >> f, '    CPU_OP_INTERRUPT,'
            print >> f, '    CPU_OP_PUSH_TMP,'
            print >> f, '    CPU_OP_LD_TMP_MEM,'
            print >> f, '    CPU_OP_INC_L,'
            print >> f, '    CPU_OP_LD_H_MEM,'
            print >> f, '    CPU_OP_LD_L_TMP,'
            print >> f, '    CPU_OP_LD_PC_HL,'
            cnt = cnt + 9
        elif opName == 'rst':
            print >> f, '    CPU_OP_PUSH_PCH,'
            print >> f, '    CPU_OP_PUSH_PCL,'
            print >> f, '    CPU_OP_RESET,'
            print >> f, '    CPU_OP_PUSH_TMP,'
            print >> f, '    CPU_OP_LD_TMP_MEM,'
            print >> f, '    CPU_OP_INC_L,'
            print >> f, '    CPU_OP_LD_H_MEM,'
            print >> f, '    CPU_OP_LD_L_TMP,'
            print >> f, '    CPU_OP_LD_PC_HL,'
            cnt = cnt + 9
        elif opName[:2] == 'se' or opName[:2] == 'cl':
            if opName[:2] == 'cl':
                print >> f, '    CPU_OP_LD_TMP_00,'
            else:
                print >> f, '    CPU_OP_LD_TMP_FF,'
            print >> f, '    CPU_OP_SET_%s,' % opName[2].upper()
            print >> f, '    CPU_OP_WAIT,'
            cnt = cnt + 3
        elif opName[:2] == 'in' or opName[:2] == 'de':
            print >> f, '    CPU_OP_LD_TMP_%s,' % opName[2].upper()
            print >> f, '    CPU_OP_%sC,' % opName[:2].upper()
            print >> f, '    CPU_OP_LD_%s_TMP,' % opName[2].upper()
            print >> f, '    CPU_OP_WAIT,'
            cnt = cnt + 4
        elif opName[0] == 't':
            if opName[1] != 's':
                print >> f, '    CPU_OP_LD_TMP_%s,' % opName[1].upper()
            else:
                print >> f, '    CPU_OP_LD_TMP_SP,'
            if opName[2] != 's':
                print >> f, '    CPU_OP_SET_NZ,'
                print >> f, '    CPU_OP_LD_%s_TMP,' % opName[2].upper()
                cnt = cnt + 1
            else:
                print >> f, '    CPU_OP_LD_SP_TMP,'
            print >> f, '    CPU_OP_WAIT,'
            cnt = cnt + 3
        elif opName in ['asl', 'lsr', 'rol', 'ror']:
            print >> f, '    CPU_OP_LD_TMP_A,'
            print >> f, '    CPU_OP_%s,' % opName.upper()
            print >> f, '    CPU_OP_LD_A_TMP,'
            print >> f, '    CPU_OP_WAIT,'
            cnt = cnt + 4
        elif opName == 'rts':
            print >> f, '    CPU_OP_RD_TMP,'
            print >> f, '    CPU_OP_WAIT,'
            print >> f, '    CPU_OP_POP_PCL,'
            print >> f, '    CPU_OP_POP_PCH,'
            print >> f, '    CPU_OP_RD_TMP,'
            cnt = cnt + 5
        elif opName == 'rti':
            print >> f, '    CPU_OP_RD_TMP,'
            print >> f, '    CPU_OP_WAIT,'
            print >> f, '    CPU_OP_POP_TMP,'
            print >> f, '    CPU_OP_LD_SR_TMP,'
            print >> f, '    CPU_OP_POP_PCL,'
            print >> f, '    CPU_OP_POP_PCH,'
            cnt = cnt + 6
        elif opName == 'pha':
            print >> f, '    CPU_OP_WAIT,'
            print >> f, '    CPU_OP_LD_TMP_A,'
            print >> f, '    CPU_OP_PUSH_TMP,'
            cnt = cnt + 3
        elif opName == 'php':
            print >> f, '    CPU_OP_WAIT,'
            print >> f, '    CPU_OP_LD_TMP_SR,'
            print >> f, '    CPU_OP_PUSH_TMP,'
            cnt = cnt + 3
        elif opName == 'pla':
            print >> f, '    CPU_OP_WAIT,'
            print >> f, '    CPU_OP_WAIT,'
            print >> f, '    CPU_OP_POP_TMP,'
            print >> f, '    CPU_OP_SET_NZ,'
            print >> f, '    CPU_OP_LD_A_TMP,'
            cnt = cnt + 5
        elif opName == 'plp':
            print >> f, '    CPU_OP_WAIT,'
            print >> f, '    CPU_OP_WAIT,'
            print >> f, '    CPU_OP_POP_TMP,'
            print >> f, '    CPU_OP_LD_SR_TMP,'
            cnt = cnt + 4
        elif opName == 'nop':
            print >> f, '    CPU_OP_WAIT,'
            cnt = cnt + 1
        elif opName == '???':
            print >> f, '    CPU_OP_INVALID_OPCODE,'
            print >> f, '    CPU_OP_LD_TMP_MEM,'
            print >> f, '    CPU_OP_LD_MEM_TMP,'
            print >> f, '    CPU_OP_PUSH_PCH,'
            print >> f, '    CPU_OP_PUSH_PCL,'
            print >> f, '    CPU_OP_RESET,'
            print >> f, '    CPU_OP_PUSH_TMP,'
            print >> f, '    CPU_OP_LD_TMP_MEM,'
            print >> f, '    CPU_OP_INC_L,'
            print >> f, '    CPU_OP_LD_H_MEM,'
            print >> f, '    CPU_OP_LD_L_TMP,'
            print >> f, '    CPU_OP_LD_PC_HL,'
            cnt = cnt + 12
        else:
            print ' *** invalid instruction'
            raise SystemExit(-1)
    while cnt < 16:
        if not (opNum == 0x0101 and cnt == 15):
            print >> f, '    CPU_OP_RD_OPCODE,'
        else:
            print >> f, '    CPU_OP_RD_OPCODE'
        cnt = cnt + 1

f = open('cpuoptbl.cpp', 'w')
print >> f, ''
print >> f, '#include "ep128emu.hpp"'
print >> f, '#include "cpu.hpp"'
print >> f, ''
print >> f, 'namespace Plus4 {'
print >> f, ''
print >> f, '  const unsigned char M7501::opcodeTable[4128] = {'
generateOpcode(f, 0x00, 'BRK')
generateOpcode(f, 0x01, 'ORA (nn, X)')
generateOpcode(f, 0x02, '???')
generateOpcode(f, 0x03, '???')
generateOpcode(f, 0x04, 'NOP nn')       # ???
generateOpcode(f, 0x05, 'ORA nn')
generateOpcode(f, 0x06, 'ASL nn')
generateOpcode(f, 0x07, '???')
generateOpcode(f, 0x08, 'PHP')
generateOpcode(f, 0x09, 'ORA #nn')
generateOpcode(f, 0x0A, 'ASL')
generateOpcode(f, 0x0B, '???')
generateOpcode(f, 0x0C, 'NOP nnnn')     # ???
generateOpcode(f, 0x0D, 'ORA nnnn')
generateOpcode(f, 0x0E, 'ASL nnnn')
generateOpcode(f, 0x0F, '???')
generateOpcode(f, 0x10, 'BPL nn')
generateOpcode(f, 0x11, 'ORA (nn), Y')
generateOpcode(f, 0x12, '???')
generateOpcode(f, 0x13, '???')
generateOpcode(f, 0x14, 'NOP nn, X')    # ???
generateOpcode(f, 0x15, 'ORA nn, X')
generateOpcode(f, 0x16, 'ASL nn, X')
generateOpcode(f, 0x17, '???')
generateOpcode(f, 0x18, 'CLC')
generateOpcode(f, 0x19, 'ORA nnnn, Y')
generateOpcode(f, 0x1A, 'NOP')          # ???
generateOpcode(f, 0x1B, '???')
generateOpcode(f, 0x1C, 'NOP nnnn, X')  # ???
generateOpcode(f, 0x1D, 'ORA nnnn, X')
generateOpcode(f, 0x1E, 'ASL nnnn, X')
generateOpcode(f, 0x1F, '???')
generateOpcode(f, 0x20, 'JSR nnnn')
generateOpcode(f, 0x21, 'AND (nn, X)')
generateOpcode(f, 0x22, '???')
generateOpcode(f, 0x23, '???')
generateOpcode(f, 0x24, 'BIT nn')
generateOpcode(f, 0x25, 'AND nn')
generateOpcode(f, 0x26, 'ROL nn')
generateOpcode(f, 0x27, '???')
generateOpcode(f, 0x28, 'PLP')
generateOpcode(f, 0x29, 'AND #nn')
generateOpcode(f, 0x2A, 'ROL')
generateOpcode(f, 0x2B, '???')
generateOpcode(f, 0x2C, 'BIT nnnn')
generateOpcode(f, 0x2D, 'AND nnnn')
generateOpcode(f, 0x2E, 'ROL nnnn')
generateOpcode(f, 0x2F, '???')
generateOpcode(f, 0x30, 'BMI nn')
generateOpcode(f, 0x31, 'AND (nn), Y')
generateOpcode(f, 0x32, '???')
generateOpcode(f, 0x33, '???')
generateOpcode(f, 0x34, 'NOP nn, X')    # ???
generateOpcode(f, 0x35, 'AND nn, X')
generateOpcode(f, 0x36, 'ROL nn, X')
generateOpcode(f, 0x37, '???')
generateOpcode(f, 0x38, 'SEC')
generateOpcode(f, 0x39, 'AND nnnn, Y')
generateOpcode(f, 0x3A, 'NOP')          # ???
generateOpcode(f, 0x3B, '???')
generateOpcode(f, 0x3C, 'NOP nnnn, X')  # ???
generateOpcode(f, 0x3D, 'AND nnnn, X')
generateOpcode(f, 0x3E, 'ROL nnnn, X')
generateOpcode(f, 0x3F, '???')
generateOpcode(f, 0x40, 'RTI')
generateOpcode(f, 0x41, 'EOR (nn, X)')
generateOpcode(f, 0x42, '???')
generateOpcode(f, 0x43, '???')
generateOpcode(f, 0x44, 'NOP nn')       # ???
generateOpcode(f, 0x45, 'EOR nn')
generateOpcode(f, 0x46, 'LSR nn')
generateOpcode(f, 0x47, '???')
generateOpcode(f, 0x48, 'PHA')
generateOpcode(f, 0x49, 'EOR #nn')
generateOpcode(f, 0x4A, 'LSR')
generateOpcode(f, 0x4B, '???')
generateOpcode(f, 0x4C, 'JMP nnnn')
generateOpcode(f, 0x4D, 'EOR nnnn')
generateOpcode(f, 0x4E, 'LSR nnnn')
generateOpcode(f, 0x4F, '???')
generateOpcode(f, 0x50, 'BVC nn')
generateOpcode(f, 0x51, 'EOR (nn), Y')
generateOpcode(f, 0x52, '???')
generateOpcode(f, 0x53, '???')
generateOpcode(f, 0x54, 'NOP nn, X')    # ???
generateOpcode(f, 0x55, 'EOR nn, X')
generateOpcode(f, 0x56, 'LSR nn, X')
generateOpcode(f, 0x57, '???')
generateOpcode(f, 0x58, 'CLI')
generateOpcode(f, 0x59, 'EOR nnnn, Y')
generateOpcode(f, 0x5A, 'NOP')          # ???
generateOpcode(f, 0x5B, '???')
generateOpcode(f, 0x5C, 'NOP nnnn, X')  # ???
generateOpcode(f, 0x5D, 'EOR nnnn, X')
generateOpcode(f, 0x5E, 'LSR nnnn, X')
generateOpcode(f, 0x5F, '???')
generateOpcode(f, 0x60, 'RTS')
generateOpcode(f, 0x61, 'ADC (nn, X)')
generateOpcode(f, 0x62, '???')
generateOpcode(f, 0x63, '???')
generateOpcode(f, 0x64, 'NOP nn')       # ???
generateOpcode(f, 0x65, 'ADC nn')
generateOpcode(f, 0x66, 'ROR nn')
generateOpcode(f, 0x67, '???')
generateOpcode(f, 0x68, 'PLA')
generateOpcode(f, 0x69, 'ADC #nn')
generateOpcode(f, 0x6A, 'ROR')
generateOpcode(f, 0x6B, '???')
generateOpcode(f, 0x6C, 'JMP (nnnn)')
generateOpcode(f, 0x6D, 'ADC nnnn')
generateOpcode(f, 0x6E, 'ROR nnnn')
generateOpcode(f, 0x6F, '???')
generateOpcode(f, 0x70, 'BVS nn')
generateOpcode(f, 0x71, 'ADC (nn), Y')
generateOpcode(f, 0x72, '???')
generateOpcode(f, 0x73, '???')
generateOpcode(f, 0x74, 'NOP nn, X')    # ???
generateOpcode(f, 0x75, 'ADC nn, X')
generateOpcode(f, 0x76, 'ROR nn, X')
generateOpcode(f, 0x77, '???')
generateOpcode(f, 0x78, 'SEI')
generateOpcode(f, 0x79, 'ADC nnnn, Y')
generateOpcode(f, 0x7A, 'NOP')          # ???
generateOpcode(f, 0x7B, '???')
generateOpcode(f, 0x7C, 'NOP nnnn, X')  # ???
generateOpcode(f, 0x7D, 'ADC nnnn, X')
generateOpcode(f, 0x7E, 'ROR nnnn, X')
generateOpcode(f, 0x7F, '???')
generateOpcode(f, 0x80, 'NOP #nn')      # ???
generateOpcode(f, 0x81, 'STA (nn, X)')
generateOpcode(f, 0x82, '???')
generateOpcode(f, 0x83, 'SAX (nn, X)')  # ???
generateOpcode(f, 0x84, 'STY nn')
generateOpcode(f, 0x85, 'STA nn')
generateOpcode(f, 0x86, 'STX nn')
generateOpcode(f, 0x87, 'SAX nn')       # ???
generateOpcode(f, 0x88, 'DEY')
generateOpcode(f, 0x89, 'NOP #nn')      # ???
generateOpcode(f, 0x8A, 'TXA')
generateOpcode(f, 0x8B, '???')
generateOpcode(f, 0x8C, 'STY nnnn')
generateOpcode(f, 0x8D, 'STA nnnn')
generateOpcode(f, 0x8E, 'STX nnnn')
generateOpcode(f, 0x8F, 'SAX nnnn')     # ???
generateOpcode(f, 0x90, 'BCC nn')
generateOpcode(f, 0x91, 'STA (nn), Y')
generateOpcode(f, 0x92, '???')
generateOpcode(f, 0x93, '???')
generateOpcode(f, 0x94, 'STY nn, X')
generateOpcode(f, 0x95, 'STA nn, X')
generateOpcode(f, 0x96, 'STX nn, Y')
generateOpcode(f, 0x97, 'SAX nn, Y')    # ???
generateOpcode(f, 0x98, 'TYA')
generateOpcode(f, 0x99, 'STA nnnn, Y')
generateOpcode(f, 0x9A, 'TXS')
generateOpcode(f, 0x9B, '???')
generateOpcode(f, 0x9C, '???')
generateOpcode(f, 0x9D, 'STA nnnn, X')
generateOpcode(f, 0x9E, '???')
generateOpcode(f, 0x9F, '???')
generateOpcode(f, 0xA0, 'LDY #nn')
generateOpcode(f, 0xA1, 'LDA (nn, X)')
generateOpcode(f, 0xA2, 'LDX #nn')
generateOpcode(f, 0xA3, 'LAX (nn, X)')  # ???
generateOpcode(f, 0xA4, 'LDY nn')
generateOpcode(f, 0xA5, 'LDA nn')
generateOpcode(f, 0xA6, 'LDX nn')
generateOpcode(f, 0xA7, 'LAX nn')       # ???
generateOpcode(f, 0xA8, 'TAY')
generateOpcode(f, 0xA9, 'LDA #nn')
generateOpcode(f, 0xAA, 'TAX')
generateOpcode(f, 0xAB, '???')
generateOpcode(f, 0xAC, 'LDY nnnn')
generateOpcode(f, 0xAD, 'LDA nnnn')
generateOpcode(f, 0xAE, 'LDX nnnn')
generateOpcode(f, 0xAF, 'LAX nnnn')     # ???
generateOpcode(f, 0xB0, 'BCS nn')
generateOpcode(f, 0xB1, 'LDA (nn), Y')
generateOpcode(f, 0xB2, '???')
generateOpcode(f, 0xB3, 'LAX (nn), Y')  # ???
generateOpcode(f, 0xB4, 'LDY nn, X')
generateOpcode(f, 0xB5, 'LDA nn, X')
generateOpcode(f, 0xB6, 'LDX nn, Y')
generateOpcode(f, 0xB7, 'LAX nn, Y')    # ???
generateOpcode(f, 0xB8, 'CLV')
generateOpcode(f, 0xB9, 'LDA nnnn, Y')
generateOpcode(f, 0xBA, 'TSX')
generateOpcode(f, 0xBB, '???')
generateOpcode(f, 0xBC, 'LDY nnnn, X')
generateOpcode(f, 0xBD, 'LDA nnnn, X')
generateOpcode(f, 0xBE, 'LDX nnnn, Y')
generateOpcode(f, 0xBF, 'LAX nnnn, Y')  # ???
generateOpcode(f, 0xC0, 'CPY #nn')
generateOpcode(f, 0xC1, 'CMP (nn, X)')
generateOpcode(f, 0xC2, '???')
generateOpcode(f, 0xC3, '???')
generateOpcode(f, 0xC4, 'CPY nn')
generateOpcode(f, 0xC5, 'CMP nn')
generateOpcode(f, 0xC6, 'DEC nn')
generateOpcode(f, 0xC7, '???')
generateOpcode(f, 0xC8, 'INY')
generateOpcode(f, 0xC9, 'CMP #nn')
generateOpcode(f, 0xCA, 'DEX')
generateOpcode(f, 0xCB, '???')
generateOpcode(f, 0xCC, 'CPY nnnn')
generateOpcode(f, 0xCD, 'CMP nnnn')
generateOpcode(f, 0xCE, 'DEC nnnn')
generateOpcode(f, 0xCF, '???')
generateOpcode(f, 0xD0, 'BNE nn')
generateOpcode(f, 0xD1, 'CMP (nn), Y')
generateOpcode(f, 0xD2, '???')
generateOpcode(f, 0xD3, '???')
generateOpcode(f, 0xD4, 'NOP nn, X')    # ???
generateOpcode(f, 0xD5, 'CMP nn, X')
generateOpcode(f, 0xD6, 'DEC nn, X')
generateOpcode(f, 0xD7, '???')
generateOpcode(f, 0xD8, 'CLD')
generateOpcode(f, 0xD9, 'CMP nnnn, Y')
generateOpcode(f, 0xDA, 'NOP')          # ???
generateOpcode(f, 0xDB, '???')
generateOpcode(f, 0xDC, 'NOP nnnn, X')  # ???
generateOpcode(f, 0xDD, 'CMP nnnn, X')
generateOpcode(f, 0xDE, 'DEC nnnn, X')
generateOpcode(f, 0xDF, '???')
generateOpcode(f, 0xE0, 'CPX #nn')
generateOpcode(f, 0xE1, 'SBC (nn, X)')
generateOpcode(f, 0xE2, '???')
generateOpcode(f, 0xE3, '???')
generateOpcode(f, 0xE4, 'CPX nn')
generateOpcode(f, 0xE5, 'SBC nn')
generateOpcode(f, 0xE6, 'INC nn')
generateOpcode(f, 0xE7, '???')
generateOpcode(f, 0xE8, 'INX')
generateOpcode(f, 0xE9, 'SBC #nn')
generateOpcode(f, 0xEA, 'NOP')
generateOpcode(f, 0xEB, 'SBC #nn')      # ???
generateOpcode(f, 0xEC, 'CPX nnnn')
generateOpcode(f, 0xED, 'SBC nnnn')
generateOpcode(f, 0xEE, 'INC nnnn')
generateOpcode(f, 0xEF, '???')
generateOpcode(f, 0xF0, 'BEQ nn')
generateOpcode(f, 0xF1, 'SBC (nn), Y')
generateOpcode(f, 0xF2, '???')
generateOpcode(f, 0xF3, '???')
generateOpcode(f, 0xF4, 'NOP nn, X')    # ???
generateOpcode(f, 0xF5, 'SBC nn, X')
generateOpcode(f, 0xF6, 'INC nn, X')
generateOpcode(f, 0xF7, '???')
generateOpcode(f, 0xF8, 'SED')
generateOpcode(f, 0xF9, 'SBC nnnn, Y')
generateOpcode(f, 0xFA, 'NOP')          # ???
generateOpcode(f, 0xFB, '???')
generateOpcode(f, 0xFC, 'NOP nnnn, X')  # ???
generateOpcode(f, 0xFD, 'SBC nnnn, X')
generateOpcode(f, 0xFE, 'INC nnnn, X')
generateOpcode(f, 0xFF, '???')
generateOpcode(f, 0x0100, 'INT')
generateOpcode(f, 0x0101, 'RST')
print >> f, '  };'
print >> f, ''
print >> f, '}       // namespace Plus4'
print >> f, ''
f.close()

