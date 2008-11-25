
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2008 Istvan Varga <istvanv@users.sourceforge.net>
// http://sourceforge.net/projects/ep128emu/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "ep128emu.hpp"
#include "vm.hpp"
#include "debuglib.hpp"

#include <vector>

namespace Ep128 {

  //   0    1    2    3    4    5    6    7    8    9
  const char * Z80Disassembler::opcodeNames =
    " ADC  ADD  AND  BIT  CALL CCF  CP   CPD  CPDR CPI "        // 00
    " CPIR CPL  DAA  DEC  DI   DJNZ EI   EX   EXX  HALT"        // 10
    " IM   IN   INC  IND  INDR INI  INIR JP   JR   LD  "        // 20
    " LDD  LDDR LDI  LDIR NEG  NOP  OR   OTDR OTIR OUT "        // 30
    " OUTD OUTI POP  PUSH RES  RET  RETI RETN RL   RLA "        // 40
    " RLC  RLCA RLD  RR   RRA  RRC  RRCA RRD  RST  SBC "        // 50
    " SCF  SET  SLA  SLL  SRA  SRL  SUB  XOR  EXOS";            // 60

  const char * Z80Disassembler::operandTypes[75] = {
    (char *) 0,         //  0: <none>
    "0",                //  1: 0
    "1",                //  2: 1
    "2",                //  3: 2
    "3",                //  4: 3
    "4",                //  5: 4
    "5",                //  6: 5
    "6",                //  7: 6
    "7",                //  8: 7
    "00",               //  9: 00
    "08",               // 10: 08
    "10",               // 11: 10
    "18",               // 12: 18
    "20",               // 13: 20
    "28",               // 14: 28
    "30",               // 15: 30
    "38",               // 16: 38
    "%02X",             // 17: nn
    "(%02X)",           // 18: (nn)
    "%0*X",             // 19: +/- nn (relative for JR and DJNZ)
    "%04X",             // 20: nnnn
    "(%04X)",           // 21: (nnnn)
    "B",                // 22: B
    "C",                // 23: C
    "D",                // 24: D
    "E",                // 25: E
    "H",                // 26: H
    "L",                // 27: L
    "(HL)",             // 28: (HL)
    "A",                // 29: A
    "IXH",              // 30: IXH (H with DD prefix)
    "IXL",              // 31: IXL (L with DD prefix)
    "(IX%c%02X)",       // 32: (IX+nn) ((HL) with DD prefix + offset)
    "A",                // 33: A (A with DD prefix)
    "IYH",              // 34: IYH (H with FD prefix)
    "IYL",              // 35: IYL (L with FD prefix)
    "(IY%c%02X)",       // 36: (IY+nn) ((HL) with FD prefix + offset)
    "A",                // 37: A (A with FD prefix)
    "(BC)",             // 38: (BC)
    "(DE)",             // 39: (DE)
    "(HL)",             // 40: (HL) (for JP)
    "(SP)",             // 41: (SP)
    "(BC)",             // 42: (BC) ((BC) with DD prefix)
    "(DE)",             // 43: (DE) ((DE) with DD prefix)
    "(IX)",             // 44: (IX) ((HL) with DD prefix)
    "(SP)",             // 45: (SP) ((SP) with DD prefix)
    "(BC)",             // 46: (BC) ((BC) with FD prefix)
    "(DE)",             // 47: (DE) ((DE) with FD prefix)
    "(IY)",             // 48: (IY) ((HL) with FD prefix)
    "(SP)",             // 49: (SP) ((SP) with FD prefix)
    "BC",               // 50: BC
    "DE",               // 51: DE
    "HL",               // 52: HL
    "SP",               // 53: SP
    "BC",               // 54: BC (BC with DD prefix)
    "DE",               // 55: DE (DE with DD prefix)
    "IX",               // 56: IX (HL with DD prefix)
    "SP",               // 57: SP (SP with DD prefix)
    "BC",               // 58: BC (BC with FD prefix)
    "DE",               // 59: DE (DE with FD prefix)
    "IY",               // 60: IY (HL with FD prefix)
    "SP",               // 61: SP (SP with FD prefix)
    "(C)",              // 62: (C) (for IN and OUT)
    "I",                // 63: I
    "R",                // 64: R
    "AF'",              // 65: AF'
    "AF",               // 66: AF
    "C",                // 67: C (branch on carry set)
    "NC",               // 68: NC (branch on carry clear)
    "Z",                // 69: Z (branch on zero)
    "NZ",               // 70: NZ (branch on non-zero)
    "M",                // 71: M (branch on negative)
    "P",                // 72: P (branch on non-negative)
    "PE",               // 73: PE (branch on parity even)
    "PO"                // 74: PO (branch on parity odd)
  };

  const unsigned char Z80Disassembler::opcodeTable[768] = {
    // name, operand1, operand2
     35,   0,   0,      // 00: NOP
     29,  50,  20,      // 01: LD BC, nnnn
     29,  38,  29,      // 02: LD (BC), A
     22,  50,   0,      // 03: INC BC
     22,  22,   0,      // 04: INC B
     13,  22,   0,      // 05: DEC B
     29,  22,  17,      // 06: LD B, nn
     51,   0,   0,      // 07: RLCA
     17,  66,  65,      // 08: EX AF, AF'
      1,  52,  50,      // 09: ADD HL, BC
     29,  29,  38,      // 0A: LD A, (BC)
     13,  50,   0,      // 0B: DEC BC
     22,  23,   0,      // 0C: INC C
     13,  23,   0,      // 0D: DEC C
     29,  23,  17,      // 0E: LD C, nn
     56,   0,   0,      // 0F: RRCA
     15,  19,   0,      // 10: DJNZ nn
     29,  51,  20,      // 11: LD DE, nnnn
     29,  39,  29,      // 12: LD (DE), A
     22,  51,   0,      // 13: INC DE
     22,  24,   0,      // 14: INC D
     13,  24,   0,      // 15: DEC D
     29,  24,  17,      // 16: LD D, nn
     49,   0,   0,      // 17: RLA
     28,  19,   0,      // 18: JR nn
      1,  52,  51,      // 19: ADD HL, DE
     29,  29,  39,      // 1A: LD A, (DE)
     13,  51,   0,      // 1B: DEC DE
     22,  25,   0,      // 1C: INC E
     13,  25,   0,      // 1D: DEC E
     29,  25,  17,      // 1E: LD E, nn
     54,   0,   0,      // 1F: RRA
     28,  70,  19,      // 20: JR NZ, nn
     29,  52,  20,      // 21: LD HL, nnnn
     29,  21,  52,      // 22: LD (nnnn), HL
     22,  52,   0,      // 23: INC HL
     22,  26,   0,      // 24: INC H
     13,  26,   0,      // 25: DEC H
     29,  26,  17,      // 26: LD H, nn
     12,   0,   0,      // 27: DAA
     28,  69,  19,      // 28: JR Z, nn
      1,  52,  52,      // 29: ADD HL, HL
     29,  52,  21,      // 2A: LD HL, (nnnn)
     13,  52,   0,      // 2B: DEC HL
     22,  27,   0,      // 2C: INC L
     13,  27,   0,      // 2D: DEC L
     29,  27,  17,      // 2E: LD L, nn
     11,   0,   0,      // 2F: CPL
     28,  68,  19,      // 30: JR NC, nn
     29,  53,  20,      // 31: LD SP, nnnn
     29,  21,  29,      // 32: LD (nnnn), A
     22,  53,   0,      // 33: INC SP
     22,  28,   0,      // 34: INC (HL)
     13,  28,   0,      // 35: DEC (HL)
     29,  28,  17,      // 36: LD (HL), nn
     60,   0,   0,      // 37: SCF
     28,  67,  19,      // 38: JR C, nn
      1,  52,  53,      // 39: ADD HL, SP
     29,  29,  21,      // 3A: LD A, (nnnn)
     13,  53,   0,      // 3B: DEC SP
     22,  29,   0,      // 3C: INC A
     13,  29,   0,      // 3D: DEC A
     29,  29,  17,      // 3E: LD A, nn
      5,   0,   0,      // 3F: CCF
     29,  22,  22,      // 40: LD B, B
     29,  22,  23,      // 41: LD B, C
     29,  22,  24,      // 42: LD B, D
     29,  22,  25,      // 43: LD B, E
     29,  22,  26,      // 44: LD B, H
     29,  22,  27,      // 45: LD B, L
     29,  22,  28,      // 46: LD B, (HL)
     29,  22,  29,      // 47: LD B, A
     29,  23,  22,      // 48: LD C, B
     29,  23,  23,      // 49: LD C, C
     29,  23,  24,      // 4A: LD C, D
     29,  23,  25,      // 4B: LD C, E
     29,  23,  26,      // 4C: LD C, H
     29,  23,  27,      // 4D: LD C, L
     29,  23,  28,      // 4E: LD C, (HL)
     29,  23,  29,      // 4F: LD C, A
     29,  24,  22,      // 50: LD D, B
     29,  24,  23,      // 51: LD D, C
     29,  24,  24,      // 52: LD D, D
     29,  24,  25,      // 53: LD D, E
     29,  24,  26,      // 54: LD D, H
     29,  24,  27,      // 55: LD D, L
     29,  24,  28,      // 56: LD D, (HL)
     29,  24,  29,      // 57: LD D, A
     29,  25,  22,      // 58: LD E, B
     29,  25,  23,      // 59: LD E, C
     29,  25,  24,      // 5A: LD E, D
     29,  25,  25,      // 5B: LD E, E
     29,  25,  26,      // 5C: LD E, H
     29,  25,  27,      // 5D: LD E, L
     29,  25,  28,      // 5E: LD E, (HL)
     29,  25,  29,      // 5F: LD E, A
     29,  26,  22,      // 60: LD H, B
     29,  26,  23,      // 61: LD H, C
     29,  26,  24,      // 62: LD H, D
     29,  26,  25,      // 63: LD H, E
     29,  26,  26,      // 64: LD H, H
     29,  26,  27,      // 65: LD H, L
     29,  26,  28,      // 66: LD H, (HL)
     29,  26,  29,      // 67: LD H, A
     29,  27,  22,      // 68: LD L, B
     29,  27,  23,      // 69: LD L, C
     29,  27,  24,      // 6A: LD L, D
     29,  27,  25,      // 6B: LD L, E
     29,  27,  26,      // 6C: LD L, H
     29,  27,  27,      // 6D: LD L, L
     29,  27,  28,      // 6E: LD L, (HL)
     29,  27,  29,      // 6F: LD L, A
     29,  28,  22,      // 70: LD (HL), B
     29,  28,  23,      // 71: LD (HL), C
     29,  28,  24,      // 72: LD (HL), D
     29,  28,  25,      // 73: LD (HL), E
     29,  28,  26,      // 74: LD (HL), H
     29,  28,  27,      // 75: LD (HL), L
     19,   0,   0,      // 76: HALT
     29,  28,  29,      // 77: LD (HL), A
     29,  29,  22,      // 78: LD A, B
     29,  29,  23,      // 79: LD A, C
     29,  29,  24,      // 7A: LD A, D
     29,  29,  25,      // 7B: LD A, E
     29,  29,  26,      // 7C: LD A, H
     29,  29,  27,      // 7D: LD A, L
     29,  29,  28,      // 7E: LD A, (HL)
     29,  29,  29,      // 7F: LD A, A
      1,  29,  22,      // 80: ADD A, B
      1,  29,  23,      // 81: ADD A, C
      1,  29,  24,      // 82: ADD A, D
      1,  29,  25,      // 83: ADD A, E
      1,  29,  26,      // 84: ADD A, H
      1,  29,  27,      // 85: ADD A, L
      1,  29,  28,      // 86: ADD A, (HL)
      1,  29,  29,      // 87: ADD A, A
      0,  29,  22,      // 88: ADC A, B
      0,  29,  23,      // 89: ADC A, C
      0,  29,  24,      // 8A: ADC A, D
      0,  29,  25,      // 8B: ADC A, E
      0,  29,  26,      // 8C: ADC A, H
      0,  29,  27,      // 8D: ADC A, L
      0,  29,  28,      // 8E: ADC A, (HL)
      0,  29,  29,      // 8F: ADC A, A
     66,  22,   0,      // 90: SUB B
     66,  23,   0,      // 91: SUB C
     66,  24,   0,      // 92: SUB D
     66,  25,   0,      // 93: SUB E
     66,  26,   0,      // 94: SUB H
     66,  27,   0,      // 95: SUB L
     66,  28,   0,      // 96: SUB (HL)
     66,  29,   0,      // 97: SUB A
     59,  29,  22,      // 98: SBC A, B
     59,  29,  23,      // 99: SBC A, C
     59,  29,  24,      // 9A: SBC A, D
     59,  29,  25,      // 9B: SBC A, E
     59,  29,  26,      // 9C: SBC A, H
     59,  29,  27,      // 9D: SBC A, L
     59,  29,  28,      // 9E: SBC A, (HL)
     59,  29,  29,      // 9F: SBC A, A
      2,  22,   0,      // A0: AND B
      2,  23,   0,      // A1: AND C
      2,  24,   0,      // A2: AND D
      2,  25,   0,      // A3: AND E
      2,  26,   0,      // A4: AND H
      2,  27,   0,      // A5: AND L
      2,  28,   0,      // A6: AND (HL)
      2,  29,   0,      // A7: AND A
     67,  22,   0,      // A8: XOR B
     67,  23,   0,      // A9: XOR C
     67,  24,   0,      // AA: XOR D
     67,  25,   0,      // AB: XOR E
     67,  26,   0,      // AC: XOR H
     67,  27,   0,      // AD: XOR L
     67,  28,   0,      // AE: XOR (HL)
     67,  29,   0,      // AF: XOR A
     36,  22,   0,      // B0: OR B
     36,  23,   0,      // B1: OR C
     36,  24,   0,      // B2: OR D
     36,  25,   0,      // B3: OR E
     36,  26,   0,      // B4: OR H
     36,  27,   0,      // B5: OR L
     36,  28,   0,      // B6: OR (HL)
     36,  29,   0,      // B7: OR A
      6,  22,   0,      // B8: CP B
      6,  23,   0,      // B9: CP C
      6,  24,   0,      // BA: CP D
      6,  25,   0,      // BB: CP E
      6,  26,   0,      // BC: CP H
      6,  27,   0,      // BD: CP L
      6,  28,   0,      // BE: CP (HL)
      6,  29,   0,      // BF: CP A
     45,  70,   0,      // C0: RET NZ
     42,  50,   0,      // C1: POP BC
     27,  70,  20,      // C2: JP NZ, nnnn
     27,  20,   0,      // C3: JP nnnn
      4,  70,  20,      // C4: CALL NZ, nnnn
     43,  50,   0,      // C5: PUSH BC
      1,  29,  17,      // C6: ADD A, nn
     58,   9,   0,      // C7: RST 00
     45,  69,   0,      // C8: RET Z
     45,   0,   0,      // C9: RET
     27,  69,  20,      // CA: JP Z, nnnn
    255,   0,   0,      // CB: (--> opcodeTableCB[])
      4,  69,  20,      // CC: CALL Z, nnnn
      4,  20,   0,      // CD: CALL nnnn
      0,  29,  17,      // CE: ADC A, nn
     58,  10,   0,      // CF: RST 08
     45,  68,   0,      // D0: RET NC
     42,  51,   0,      // D1: POP DE
     27,  68,  20,      // D2: JP NC, nnnn
     39,  18,  29,      // D3: OUT (nn), A
      4,  68,  20,      // D4: CALL NC, nnnn
     43,  51,   0,      // D5: PUSH DE
     66,  17,   0,      // D6: SUB nn
     58,  11,   0,      // D7: RST 10
     45,  67,   0,      // D8: RET C
     18,   0,   0,      // D9: EXX
     27,  67,  20,      // DA: JP C, nnnn
     21,  29,  18,      // DB: IN A, (nn)
      4,  67,  20,      // DC: CALL C, nnnn
    255,   0,   0,      // DD: (prefix: use IX instead of HL)
     59,  29,  17,      // DE: SBC A, nn
     58,  12,   0,      // DF: RST 18
     45,  74,   0,      // E0: RET PO
     42,  52,   0,      // E1: POP HL
     27,  74,  20,      // E2: JP PO, nnnn
     17,  41,  52,      // E3: EX (SP), HL
      4,  74,  20,      // E4: CALL PO, nnnn
     43,  52,   0,      // E5: PUSH HL
      2,  17,   0,      // E6: AND nn
     58,  13,   0,      // E7: RST 20
     45,  73,   0,      // E8: RET PE
     27,  40,   0,      // E9: JP (HL)
     27,  73,  20,      // EA: JP PE, nnnn
     17,  51,  52,      // EB: EX DE, HL
      4,  73,  20,      // EC: CALL PE, nnnn
    255,   0,   0,      // ED: (--> opcodeTableED[])
     67,  17,   0,      // EE: XOR nn
     58,  14,   0,      // EF: RST 28
     45,  72,   0,      // F0: RET P
     42,  66,   0,      // F1: POP AF
     27,  72,  20,      // F2: JP P, nnnn
     14,   0,   0,      // F3: DI
      4,  72,  20,      // F4: CALL P, nnnn
     43,  66,   0,      // F5: PUSH AF
     36,  17,   0,      // F6: OR nn
     68,  17,   0,      // F7: RST 30 ( = EXOS nn)
     45,  71,   0,      // F8: RET M
     29,  53,  52,      // F9: LD SP, HL
     27,  71,  20,      // FA: JP M, nnnn
     16,   0,   0,      // FB: EI
      4,  71,  20,      // FC: CALL M, nnnn
    255,   0,   0,      // FD: (prefix: use IY instead of HL)
      6,  17,   0,      // FE: CP nn
     58,  16,   0       // FF: RST 38
  };

  const unsigned char Z80Disassembler::opcodeTableCB[768] = {
    // name, operand1, operand2
     50,  22,   0,      // 00: RLC B
     50,  23,   0,      // 01: RLC C
     50,  24,   0,      // 02: RLC D
     50,  25,   0,      // 03: RLC E
     50,  26,   0,      // 04: RLC H
     50,  27,   0,      // 05: RLC L
     50,  28,   0,      // 06: RLC (HL)
     50,  29,   0,      // 07: RLC A
     55,  22,   0,      // 08: RRC B
     55,  23,   0,      // 09: RRC C
     55,  24,   0,      // 0A: RRC D
     55,  25,   0,      // 0B: RRC E
     55,  26,   0,      // 0C: RRC H
     55,  27,   0,      // 0D: RRC L
     55,  28,   0,      // 0E: RRC (HL)
     55,  29,   0,      // 0F: RRC A
     48,  22,   0,      // 10: RL B
     48,  23,   0,      // 11: RL C
     48,  24,   0,      // 12: RL D
     48,  25,   0,      // 13: RL E
     48,  26,   0,      // 14: RL H
     48,  27,   0,      // 15: RL L
     48,  28,   0,      // 16: RL (HL)
     48,  29,   0,      // 17: RL A
     53,  22,   0,      // 18: RR B
     53,  23,   0,      // 19: RR C
     53,  24,   0,      // 1A: RR D
     53,  25,   0,      // 1B: RR E
     53,  26,   0,      // 1C: RR H
     53,  27,   0,      // 1D: RR L
     53,  28,   0,      // 1E: RR (HL)
     53,  29,   0,      // 1F: RR A
     62,  22,   0,      // 20: SLA B
     62,  23,   0,      // 21: SLA C
     62,  24,   0,      // 22: SLA D
     62,  25,   0,      // 23: SLA E
     62,  26,   0,      // 24: SLA H
     62,  27,   0,      // 25: SLA L
     62,  28,   0,      // 26: SLA (HL)
     62,  29,   0,      // 27: SLA A
     64,  22,   0,      // 28: SRA B
     64,  23,   0,      // 29: SRA C
     64,  24,   0,      // 2A: SRA D
     64,  25,   0,      // 2B: SRA E
     64,  26,   0,      // 2C: SRA H
     64,  27,   0,      // 2D: SRA L
     64,  28,   0,      // 2E: SRA (HL)
     64,  29,   0,      // 2F: SRA A
     63,  22,   0,      // 30: SLL B
     63,  23,   0,      // 31: SLL C
     63,  24,   0,      // 32: SLL D
     63,  25,   0,      // 33: SLL E
     63,  26,   0,      // 34: SLL H
     63,  27,   0,      // 35: SLL L
     63,  28,   0,      // 36: SLL (HL)
     63,  29,   0,      // 37: SLL A
     65,  22,   0,      // 38: SRL B
     65,  23,   0,      // 39: SRL C
     65,  24,   0,      // 3A: SRL D
     65,  25,   0,      // 3B: SRL E
     65,  26,   0,      // 3C: SRL H
     65,  27,   0,      // 3D: SRL L
     65,  28,   0,      // 3E: SRL (HL)
     65,  29,   0,      // 3F: SRL A
      3,   1,  22,      // 40: BIT 0, B
      3,   1,  23,      // 41: BIT 0, C
      3,   1,  24,      // 42: BIT 0, D
      3,   1,  25,      // 43: BIT 0, E
      3,   1,  26,      // 44: BIT 0, H
      3,   1,  27,      // 45: BIT 0, L
      3,   1,  28,      // 46: BIT 0, (HL)
      3,   1,  29,      // 47: BIT 0, A
      3,   2,  22,      // 48: BIT 1, B
      3,   2,  23,      // 49: BIT 1, C
      3,   2,  24,      // 4A: BIT 1, D
      3,   2,  25,      // 4B: BIT 1, E
      3,   2,  26,      // 4C: BIT 1, H
      3,   2,  27,      // 4D: BIT 1, L
      3,   2,  28,      // 4E: BIT 1, (HL)
      3,   2,  29,      // 4F: BIT 1, A
      3,   3,  22,      // 50: BIT 2, B
      3,   3,  23,      // 51: BIT 2, C
      3,   3,  24,      // 52: BIT 2, D
      3,   3,  25,      // 53: BIT 2, E
      3,   3,  26,      // 54: BIT 2, H
      3,   3,  27,      // 55: BIT 2, L
      3,   3,  28,      // 56: BIT 2, (HL)
      3,   3,  29,      // 57: BIT 2, A
      3,   4,  22,      // 58: BIT 3, B
      3,   4,  23,      // 59: BIT 3, C
      3,   4,  24,      // 5A: BIT 3, D
      3,   4,  25,      // 5B: BIT 3, E
      3,   4,  26,      // 5C: BIT 3, H
      3,   4,  27,      // 5D: BIT 3, L
      3,   4,  28,      // 5E: BIT 3, (HL)
      3,   4,  29,      // 5F: BIT 3, A
      3,   5,  22,      // 60: BIT 4, B
      3,   5,  23,      // 61: BIT 4, C
      3,   5,  24,      // 62: BIT 4, D
      3,   5,  25,      // 63: BIT 4, E
      3,   5,  26,      // 64: BIT 4, H
      3,   5,  27,      // 65: BIT 4, L
      3,   5,  28,      // 66: BIT 4, (HL)
      3,   5,  29,      // 67: BIT 4, A
      3,   6,  22,      // 68: BIT 5, B
      3,   6,  23,      // 69: BIT 5, C
      3,   6,  24,      // 6A: BIT 5, D
      3,   6,  25,      // 6B: BIT 5, E
      3,   6,  26,      // 6C: BIT 5, H
      3,   6,  27,      // 6D: BIT 5, L
      3,   6,  28,      // 6E: BIT 5, (HL)
      3,   6,  29,      // 6F: BIT 5, A
      3,   7,  22,      // 70: BIT 6, B
      3,   7,  23,      // 71: BIT 6, C
      3,   7,  24,      // 72: BIT 6, D
      3,   7,  25,      // 73: BIT 6, E
      3,   7,  26,      // 74: BIT 6, H
      3,   7,  27,      // 75: BIT 6, L
      3,   7,  28,      // 76: BIT 6, (HL)
      3,   7,  29,      // 77: BIT 6, A
      3,   8,  22,      // 78: BIT 7, B
      3,   8,  23,      // 79: BIT 7, C
      3,   8,  24,      // 7A: BIT 7, D
      3,   8,  25,      // 7B: BIT 7, E
      3,   8,  26,      // 7C: BIT 7, H
      3,   8,  27,      // 7D: BIT 7, L
      3,   8,  28,      // 7E: BIT 7, (HL)
      3,   8,  29,      // 7F: BIT 7, A
     44,   1,  22,      // 80: RES 0, B
     44,   1,  23,      // 81: RES 0, C
     44,   1,  24,      // 82: RES 0, D
     44,   1,  25,      // 83: RES 0, E
     44,   1,  26,      // 84: RES 0, H
     44,   1,  27,      // 85: RES 0, L
     44,   1,  28,      // 86: RES 0, (HL)
     44,   1,  29,      // 87: RES 0, A
     44,   2,  22,      // 88: RES 1, B
     44,   2,  23,      // 89: RES 1, C
     44,   2,  24,      // 8A: RES 1, D
     44,   2,  25,      // 8B: RES 1, E
     44,   2,  26,      // 8C: RES 1, H
     44,   2,  27,      // 8D: RES 1, L
     44,   2,  28,      // 8E: RES 1, (HL)
     44,   2,  29,      // 8F: RES 1, A
     44,   3,  22,      // 90: RES 2, B
     44,   3,  23,      // 91: RES 2, C
     44,   3,  24,      // 92: RES 2, D
     44,   3,  25,      // 93: RES 2, E
     44,   3,  26,      // 94: RES 2, H
     44,   3,  27,      // 95: RES 2, L
     44,   3,  28,      // 96: RES 2, (HL)
     44,   3,  29,      // 97: RES 2, A
     44,   4,  22,      // 98: RES 3, B
     44,   4,  23,      // 99: RES 3, C
     44,   4,  24,      // 9A: RES 3, D
     44,   4,  25,      // 9B: RES 3, E
     44,   4,  26,      // 9C: RES 3, H
     44,   4,  27,      // 9D: RES 3, L
     44,   4,  28,      // 9E: RES 3, (HL)
     44,   4,  29,      // 9F: RES 3, A
     44,   5,  22,      // A0: RES 4, B
     44,   5,  23,      // A1: RES 4, C
     44,   5,  24,      // A2: RES 4, D
     44,   5,  25,      // A3: RES 4, E
     44,   5,  26,      // A4: RES 4, H
     44,   5,  27,      // A5: RES 4, L
     44,   5,  28,      // A6: RES 4, (HL)
     44,   5,  29,      // A7: RES 4, A
     44,   6,  22,      // A8: RES 5, B
     44,   6,  23,      // A9: RES 5, C
     44,   6,  24,      // AA: RES 5, D
     44,   6,  25,      // AB: RES 5, E
     44,   6,  26,      // AC: RES 5, H
     44,   6,  27,      // AD: RES 5, L
     44,   6,  28,      // AE: RES 5, (HL)
     44,   6,  29,      // AF: RES 5, A
     44,   7,  22,      // B0: RES 6, B
     44,   7,  23,      // B1: RES 6, C
     44,   7,  24,      // B2: RES 6, D
     44,   7,  25,      // B3: RES 6, E
     44,   7,  26,      // B4: RES 6, H
     44,   7,  27,      // B5: RES 6, L
     44,   7,  28,      // B6: RES 6, (HL)
     44,   7,  29,      // B7: RES 6, A
     44,   8,  22,      // B8: RES 7, B
     44,   8,  23,      // B9: RES 7, C
     44,   8,  24,      // BA: RES 7, D
     44,   8,  25,      // BB: RES 7, E
     44,   8,  26,      // BC: RES 7, H
     44,   8,  27,      // BD: RES 7, L
     44,   8,  28,      // BE: RES 7, (HL)
     44,   8,  29,      // BF: RES 7, A
     61,   1,  22,      // C0: SET 0, B
     61,   1,  23,      // C1: SET 0, C
     61,   1,  24,      // C2: SET 0, D
     61,   1,  25,      // C3: SET 0, E
     61,   1,  26,      // C4: SET 0, H
     61,   1,  27,      // C5: SET 0, L
     61,   1,  28,      // C6: SET 0, (HL)
     61,   1,  29,      // C7: SET 0, A
     61,   2,  22,      // C8: SET 1, B
     61,   2,  23,      // C9: SET 1, C
     61,   2,  24,      // CA: SET 1, D
     61,   2,  25,      // CB: SET 1, E
     61,   2,  26,      // CC: SET 1, H
     61,   2,  27,      // CD: SET 1, L
     61,   2,  28,      // CE: SET 1, (HL)
     61,   2,  29,      // CF: SET 1, A
     61,   3,  22,      // D0: SET 2, B
     61,   3,  23,      // D1: SET 2, C
     61,   3,  24,      // D2: SET 2, D
     61,   3,  25,      // D3: SET 2, E
     61,   3,  26,      // D4: SET 2, H
     61,   3,  27,      // D5: SET 2, L
     61,   3,  28,      // D6: SET 2, (HL)
     61,   3,  29,      // D7: SET 2, A
     61,   4,  22,      // D8: SET 3, B
     61,   4,  23,      // D9: SET 3, C
     61,   4,  24,      // DA: SET 3, D
     61,   4,  25,      // DB: SET 3, E
     61,   4,  26,      // DC: SET 3, H
     61,   4,  27,      // DD: SET 3, L
     61,   4,  28,      // DE: SET 3, (HL)
     61,   4,  29,      // DF: SET 3, A
     61,   5,  22,      // E0: SET 4, B
     61,   5,  23,      // E1: SET 4, C
     61,   5,  24,      // E2: SET 4, D
     61,   5,  25,      // E3: SET 4, E
     61,   5,  26,      // E4: SET 4, H
     61,   5,  27,      // E5: SET 4, L
     61,   5,  28,      // E6: SET 4, (HL)
     61,   5,  29,      // E7: SET 4, A
     61,   6,  22,      // E8: SET 5, B
     61,   6,  23,      // E9: SET 5, C
     61,   6,  24,      // EA: SET 5, D
     61,   6,  25,      // EB: SET 5, E
     61,   6,  26,      // EC: SET 5, H
     61,   6,  27,      // ED: SET 5, L
     61,   6,  28,      // EE: SET 5, (HL)
     61,   6,  29,      // EF: SET 5, A
     61,   7,  22,      // F0: SET 6, B
     61,   7,  23,      // F1: SET 6, C
     61,   7,  24,      // F2: SET 6, D
     61,   7,  25,      // F3: SET 6, E
     61,   7,  26,      // F4: SET 6, H
     61,   7,  27,      // F5: SET 6, L
     61,   7,  28,      // F6: SET 6, (HL)
     61,   7,  29,      // F7: SET 6, A
     61,   8,  22,      // F8: SET 7, B
     61,   8,  23,      // F9: SET 7, C
     61,   8,  24,      // FA: SET 7, D
     61,   8,  25,      // FB: SET 7, E
     61,   8,  26,      // FC: SET 7, H
     61,   8,  27,      // FD: SET 7, L
     61,   8,  28,      // FE: SET 7, (HL)
     61,   8,  29       // FF: SET 7, A
  };

  const unsigned char Z80Disassembler::opcodeTableED[768] = {
    // name, operand1, operand2
    255,   0,   0,      // 00: ???
    255,   0,   0,      // 01: ???
    255,   0,   0,      // 02: ???
    255,   0,   0,      // 03: ???
    255,   0,   0,      // 04: ???
    255,   0,   0,      // 05: ???
    255,   0,   0,      // 06: ???
    255,   0,   0,      // 07: ???
    255,   0,   0,      // 08: ???
    255,   0,   0,      // 09: ???
    255,   0,   0,      // 0A: ???
    255,   0,   0,      // 0B: ???
    255,   0,   0,      // 0C: ???
    255,   0,   0,      // 0D: ???
    255,   0,   0,      // 0E: ???
    255,   0,   0,      // 0F: ???
    255,   0,   0,      // 10: ???
    255,   0,   0,      // 11: ???
    255,   0,   0,      // 12: ???
    255,   0,   0,      // 13: ???
    255,   0,   0,      // 14: ???
    255,   0,   0,      // 15: ???
    255,   0,   0,      // 16: ???
    255,   0,   0,      // 17: ???
    255,   0,   0,      // 18: ???
    255,   0,   0,      // 19: ???
    255,   0,   0,      // 1A: ???
    255,   0,   0,      // 1B: ???
    255,   0,   0,      // 1C: ???
    255,   0,   0,      // 1D: ???
    255,   0,   0,      // 1E: ???
    255,   0,   0,      // 1F: ???
    255,   0,   0,      // 20: ???
    255,   0,   0,      // 21: ???
    255,   0,   0,      // 22: ???
    255,   0,   0,      // 23: ???
    255,   0,   0,      // 24: ???
    255,   0,   0,      // 25: ???
    255,   0,   0,      // 26: ???
    255,   0,   0,      // 27: ???
    255,   0,   0,      // 28: ???
    255,   0,   0,      // 29: ???
    255,   0,   0,      // 2A: ???
    255,   0,   0,      // 2B: ???
    255,   0,   0,      // 2C: ???
    255,   0,   0,      // 2D: ???
    255,   0,   0,      // 2E: ???
    255,   0,   0,      // 2F: ???
    255,   0,   0,      // 30: ???
    255,   0,   0,      // 31: ???
    255,   0,   0,      // 32: ???
    255,   0,   0,      // 33: ???
    255,   0,   0,      // 34: ???
    255,   0,   0,      // 35: ???
    255,   0,   0,      // 36: ???
    255,   0,   0,      // 37: ???
    255,   0,   0,      // 38: ???
    255,   0,   0,      // 39: ???
    255,   0,   0,      // 3A: ???
    255,   0,   0,      // 3B: ???
    255,   0,   0,      // 3C: ???
    255,   0,   0,      // 3D: ???
    255,   0,   0,      // 3E: ???
    255,   0,   0,      // 3F: ???
     21,  22,  62,      // 40: IN B, (C)
     39,  62,  22,      // 41: OUT (C), B
     59,  52,  50,      // 42: SBC HL, BC
     29,  21,  50,      // 43: LD (nnnn), BC
     34,   0,   0,      // 44: NEG
     47,   0,   0,      // 45: RETN
     20,   1,   0,      // 46: IM 0
     29,  63,  29,      // 47: LD I, A
     21,  23,  62,      // 48: IN C, (C)
     39,  62,  23,      // 49: OUT (C), C
      0,  52,  50,      // 4A: ADC HL, BC
     29,  50,  21,      // 4B: LD BC, (nnnn)
    255,   0,   0,      // 4C: ???
     46,   0,   0,      // 4D: RETI
    255,   0,   0,      // 4E: ???
     29,  64,  29,      // 4F: LD R, A
     21,  24,  62,      // 50: IN D, (C)
     39,  62,  24,      // 51: OUT (C), D
     59,  52,  51,      // 52: SBC HL, DE
     29,  21,  51,      // 53: LD (nnnn), DE
    255,   0,   0,      // 54: ???
    255,   0,   0,      // 55: ???
     20,   2,   0,      // 56: IM 1
     29,  29,  63,      // 57: LD A, I
     21,  25,  62,      // 58: IN E, (C)
     39,  62,  25,      // 59: OUT (C), E
      0,  52,  51,      // 5A: ADC HL, DE
     29,  51,  21,      // 5B: LD DE, (nnnn)
    255,   0,   0,      // 5C: ???
    255,   0,   0,      // 5D: ???
     20,   3,   0,      // 5E: IM 2
     29,  29,  64,      // 5F: LD A, R
     21,  26,  62,      // 60: IN H, (C)
     39,  62,  26,      // 61: OUT (C), H
     59,  52,  52,      // 62: SBC HL, HL
     29,  21,  52,      // 63: LD (nnnn), HL
    255,   0,   0,      // 64: ???
    255,   0,   0,      // 65: ???
    255,   0,   0,      // 66: ???
     57,   0,   0,      // 67: RRD
     21,  27,  62,      // 68: IN L, (C)
     39,  62,  27,      // 69: OUT (C), L
      0,  52,  52,      // 6A: ADC HL, HL
     29,  52,  21,      // 6B: LD HL, (nnnn)
    255,   0,   0,      // 6C: ???
    255,   0,   0,      // 6D: ???
    255,   0,   0,      // 6E: ???
     52,   0,   0,      // 6F: RLD
    255,   0,   0,      // 70: ???
    255,   0,   0,      // 71: ???
     59,  52,  53,      // 72: SBC HL, SP
     29,  21,  53,      // 73: LD (nnnn), SP
    255,   0,   0,      // 74: ???
    255,   0,   0,      // 75: ???
    255,   0,   0,      // 76: ???
    255,   0,   0,      // 77: ???
     21,  29,  62,      // 78: IN A, (C)
     39,  62,  29,      // 79: OUT (C), A
      0,  52,  53,      // 7A: ADC HL, SP
     29,  53,  21,      // 7B: LD SP, (nnnn)
    255,   0,   0,      // 7C: ???
    255,   0,   0,      // 7D: ???
    255,   0,   0,      // 7E: ???
    255,   0,   0,      // 7F: ???
    255,   0,   0,      // 80: ???
    255,   0,   0,      // 81: ???
    255,   0,   0,      // 82: ???
    255,   0,   0,      // 83: ???
    255,   0,   0,      // 84: ???
    255,   0,   0,      // 85: ???
    255,   0,   0,      // 86: ???
    255,   0,   0,      // 87: ???
    255,   0,   0,      // 88: ???
    255,   0,   0,      // 89: ???
    255,   0,   0,      // 8A: ???
    255,   0,   0,      // 8B: ???
    255,   0,   0,      // 8C: ???
    255,   0,   0,      // 8D: ???
    255,   0,   0,      // 8E: ???
    255,   0,   0,      // 8F: ???
    255,   0,   0,      // 90: ???
    255,   0,   0,      // 91: ???
    255,   0,   0,      // 92: ???
    255,   0,   0,      // 93: ???
    255,   0,   0,      // 94: ???
    255,   0,   0,      // 95: ???
    255,   0,   0,      // 96: ???
    255,   0,   0,      // 97: ???
    255,   0,   0,      // 98: ???
    255,   0,   0,      // 99: ???
    255,   0,   0,      // 9A: ???
    255,   0,   0,      // 9B: ???
    255,   0,   0,      // 9C: ???
    255,   0,   0,      // 9D: ???
    255,   0,   0,      // 9E: ???
    255,   0,   0,      // 9F: ???
     32,   0,   0,      // A0: LDI
      9,   0,   0,      // A1: CPI
     25,   0,   0,      // A2: INI
     41,   0,   0,      // A3: OUTI
    255,   0,   0,      // A4: ???
    255,   0,   0,      // A5: ???
    255,   0,   0,      // A6: ???
    255,   0,   0,      // A7: ???
     30,   0,   0,      // A8: LDD
      7,   0,   0,      // A9: CPD
     23,   0,   0,      // AA: IND
     40,   0,   0,      // AB: OUTD
    255,   0,   0,      // AC: ???
    255,   0,   0,      // AD: ???
    255,   0,   0,      // AE: ???
    255,   0,   0,      // AF: ???
     33,   0,   0,      // B0: LDIR
     10,   0,   0,      // B1: CPIR
     26,   0,   0,      // B2: INIR
     38,   0,   0,      // B3: OTIR
    255,   0,   0,      // B4: ???
    255,   0,   0,      // B5: ???
    255,   0,   0,      // B6: ???
    255,   0,   0,      // B7: ???
     31,   0,   0,      // B8: LDDR
      8,   0,   0,      // B9: CPDR
     24,   0,   0,      // BA: INDR
     37,   0,   0,      // BB: OTDR
    255,   0,   0,      // BC: ???
    255,   0,   0,      // BD: ???
    255,   0,   0,      // BE: ???
    255,   0,   0,      // BF: ???
    255,   0,   0,      // C0: ???
    255,   0,   0,      // C1: ???
    255,   0,   0,      // C2: ???
    255,   0,   0,      // C3: ???
    255,   0,   0,      // C4: ???
    255,   0,   0,      // C5: ???
    255,   0,   0,      // C6: ???
    255,   0,   0,      // C7: ???
    255,   0,   0,      // C8: ???
    255,   0,   0,      // C9: ???
    255,   0,   0,      // CA: ???
    255,   0,   0,      // CB: ???
    255,   0,   0,      // CC: ???
    255,   0,   0,      // CD: ???
    255,   0,   0,      // CE: ???
    255,   0,   0,      // CF: ???
    255,   0,   0,      // D0: ???
    255,   0,   0,      // D1: ???
    255,   0,   0,      // D2: ???
    255,   0,   0,      // D3: ???
    255,   0,   0,      // D4: ???
    255,   0,   0,      // D5: ???
    255,   0,   0,      // D6: ???
    255,   0,   0,      // D7: ???
    255,   0,   0,      // D8: ???
    255,   0,   0,      // D9: ???
    255,   0,   0,      // DA: ???
    255,   0,   0,      // DB: ???
    255,   0,   0,      // DC: ???
    255,   0,   0,      // DD: ???
    255,   0,   0,      // DE: ???
    255,   0,   0,      // DF: ???
    255,   0,   0,      // E0: ???
    255,   0,   0,      // E1: ???
    255,   0,   0,      // E2: ???
    255,   0,   0,      // E3: ???
    255,   0,   0,      // E4: ???
    255,   0,   0,      // E5: ???
    255,   0,   0,      // E6: ???
    255,   0,   0,      // E7: ???
    255,   0,   0,      // E8: ???
    255,   0,   0,      // E9: ???
    255,   0,   0,      // EA: ???
    255,   0,   0,      // EB: ???
    255,   0,   0,      // EC: ???
    255,   0,   0,      // ED: ???
    255,   0,   0,      // EE: ???
    255,   0,   0,      // EF: ???
    255,   0,   0,      // F0: ???
    255,   0,   0,      // F1: ???
    255,   0,   0,      // F2: ???
    255,   0,   0,      // F3: ???
    255,   0,   0,      // F4: ???
    255,   0,   0,      // F5: ???
    255,   0,   0,      // F6: ???
    255,   0,   0,      // F7: ???
    255,   0,   0,      // F8: ???
    255,   0,   0,      // F9: ???
    255,   0,   0,      // FA: ???
    255,   0,   0,      // FB: ???
    255,   0,   0,      // FC: ???
    255,   0,   0,      // FD: ???
    255,   0,   0,      // FE: ???
    255,   0,   0       // FF: ???
  };

  const unsigned char Z80Disassembler::alternateOperandTypeTable[75] = {
     0,  9, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 20, 21,  0,
    19,  0, 17, 67, 17, 17,  0,  0, 40, 17,
     0,  0,  0, 17,  0,  0,  0, 17, 18, 18,
     0,  0, 18, 18, 32,  0, 18, 18, 36,  0,
    17, 17,  0,  0, 17, 17,  0,  0, 17, 17,
     0,  0, 18,  0,  0,  0, 17, 17,  0,  0,
     0,  0,  0,  0,  0
  };

  const unsigned char Z80Disassembler::noPrefixOperandTypeTable[75] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    26, 27, 28, 29, 26, 27, 28, 29, 38, 39,
    40, 41, 38, 39, 40, 41, 38, 39, 40, 41,
    50, 51, 52, 53, 50, 51, 52, 53, 50, 51,
    52, 53, 62, 63, 64, 65, 66, 67, 68, 69,
    70, 71, 72, 73, 74
  };

  uint32_t Z80Disassembler::disassembleInstruction(
      std::string& buf, const Ep128Emu::VirtualMachine& vm,
      uint32_t addr, bool isCPUAddress, int32_t offs)
  {
    char      tmpBuf[48];
    uint8_t   opcodeBuf[8];
    int       opcodeBytes = 0;
    uint32_t  addrMask = (isCPUAddress ? 0x0000FFFFU : 0x003FFFFFU);
    addr &= addrMask;
    uint32_t  baseAddr = (addr + uint32_t(offs)) & addrMask;
    bool      useIX = false;
    bool      useIY = false;
    bool      useIndexOffset = false;
    bool      haveIndexOffset = false;
    bool      invalidOpcode = false;
    int       indexOffset = 0;
    const unsigned char *opcodeTablePtr = &(opcodeTable[0]);
    uint32_t  operand = 0U;
    uint8_t   opNum = vm.readMemory(addr, isCPUAddress) & 0xFF;
    opcodeBuf[opcodeBytes++] = opNum;
    addr = (addr + 1U) & addrMask;
    if (opNum == 0xDD)
      useIX = true;
    else if (opNum == 0xFD)
      useIY = true;
    if (useIX || useIY) {
      opNum = vm.readMemory(addr, isCPUAddress) & 0xFF;
      opcodeBuf[opcodeBytes++] = opNum;
      addr = (addr + 1U) & addrMask;
    }
    if (opNum == 0xCB) {
      opcodeTablePtr = &(opcodeTableCB[0]);
      useIndexOffset = (useIX || useIY);
      if (useIndexOffset) {
        indexOffset = vm.readMemory(addr, isCPUAddress) & 0xFF;
        opcodeBuf[opcodeBytes++] = uint8_t(indexOffset);
        addr = (addr + 1U) & addrMask;
        haveIndexOffset = true;
      }
      opNum = vm.readMemory(addr, isCPUAddress) & 0xFF;
      opcodeBuf[opcodeBytes++] = opNum;
      addr = (addr + 1U) & addrMask;
    }
    else if (opNum == 0xED) {
      opcodeTablePtr = &(opcodeTableED[0]);
      invalidOpcode = (useIX || useIY);
      opNum = vm.readMemory(addr, isCPUAddress) & 0xFF;
      opcodeBuf[opcodeBytes++] = opNum;
      addr = (addr + 1U) & addrMask;
    }
    opcodeTablePtr = (opcodeTablePtr + (size_t(opNum) * 3));
    if (opcodeTablePtr[0] > 68)
      invalidOpcode = true;
    unsigned char operand1Type = opcodeTablePtr[1];
    unsigned char operand2Type = opcodeTablePtr[2];
    if (useIndexOffset && !(operand1Type == 28 || operand2Type == 28))
      invalidOpcode = true;
    if ((useIX || useIY) && !invalidOpcode) {
      if (operand1Type == 28 || operand1Type == 40 ||
          operand2Type == 28 || operand2Type == 40) {
        if (operand1Type == 28 || operand2Type == 28)
          useIndexOffset = true;
        if (operand1Type == 28 || operand1Type == 40)
          operand1Type += (unsigned char) (useIX ? 4 : 8);
        if (operand2Type == 28 || operand2Type == 40)
          operand2Type += (unsigned char) (useIX ? 4 : 8);
      }
      else if (operand1Type == 26 || operand1Type == 27 ||
               operand1Type == 52 ||
               operand2Type == 26 || operand2Type == 27 ||
               operand2Type == 52) {
        if (operand1Type == 26 || operand1Type == 27 || operand1Type == 52)
          operand1Type += (unsigned char) (useIX ? 4 : 8);
        if (operand2Type == 26 || operand2Type == 27 || operand2Type == 52)
          operand2Type += (unsigned char) (useIX ? 4 : 8);
      }
      else
        invalidOpcode = true;
    }
    if (useIndexOffset && !haveIndexOffset) {
      indexOffset = vm.readMemory(addr, isCPUAddress) & 0xFF;
      opcodeBuf[opcodeBytes++] = uint8_t(indexOffset);
      addr = (addr + 1U) & addrMask;
      haveIndexOffset = true;
    }
    if (operand1Type >= 17 && operand1Type < 22) {
      uint8_t tmp = vm.readMemory(addr, isCPUAddress) & 0xFF;
      opcodeBuf[opcodeBytes++] = tmp;
      addr = (addr + 1U) & addrMask;
      operand = uint32_t(tmp);
      if (operand1Type >= 20) {
        tmp = vm.readMemory(addr, isCPUAddress) & 0xFF;
        opcodeBuf[opcodeBytes++] = tmp;
        addr = (addr + 1U) & addrMask;
        operand |= (uint32_t(tmp) << 8);
      }
      else if (operand1Type == 19) {
        if (operand & uint32_t(0x80))
          operand |= uint32_t(0xFFFFFF00UL);
        operand = (addr + operand + uint32_t(offs)) & addrMask;
      }
    }
    if (operand2Type >= 17 && operand2Type < 22) {
      uint8_t tmp = vm.readMemory(addr, isCPUAddress) & 0xFF;
      opcodeBuf[opcodeBytes++] = tmp;
      addr = (addr + 1U) & addrMask;
      operand = uint32_t(tmp);
      if (operand2Type >= 20) {
        tmp = vm.readMemory(addr, isCPUAddress) & 0xFF;
        opcodeBuf[opcodeBytes++] = tmp;
        addr = (addr + 1U) & addrMask;
        operand |= (uint32_t(tmp) << 8);
      }
      else if (operand2Type == 19) {
        if (operand & uint32_t(0x80))
          operand |= uint32_t(0xFFFFFF00UL);
        operand = (addr + operand + uint32_t(offs)) & addrMask;
      }
    }
    char  *bufp = &(tmpBuf[0]);
    int   n = 0;
    if (isCPUAddress)
      n = std::sprintf(bufp, "  %04X ", (unsigned int) baseAddr);
    else
      n = std::sprintf(bufp, "%06X ", (unsigned int) baseAddr);
    bufp = bufp + n;
    for (int i = 0; i < opcodeBytes && i < 4; i++) {
      n = std::sprintf(bufp, " %02X", opcodeBuf[i]);
      bufp = bufp + n;
    }
    for (int i = opcodeBytes; i < 4; i++) {
      n = std::sprintf(bufp, "   ");
      bufp = bufp + n;
    }
    if (!invalidOpcode) {
      n = std::sprintf(bufp, " %.5s",
                       &(opcodeNames[size_t(opcodeTablePtr[0]) * 5]));
      bufp = bufp + n;
      if (operand1Type != 0 || operand2Type != 0)
        *(bufp++) = ' ';
      if (operand1Type != 0) {
        *(bufp++) = ' ';
        if ((operand1Type == 32 || operand1Type == 36) && indexOffset == 0)
          operand1Type = operand1Type + 12;
        if (operand1Type >= 17 && operand1Type < 22) {
          if (operand1Type != 19)
            n = std::sprintf(bufp, operandTypes[operand1Type],
                             (unsigned int) operand);
          else
            n = std::sprintf(bufp, operandTypes[operand1Type],
                             int(isCPUAddress ? 4 : 6), (unsigned int) operand);
        }
        else if (operand1Type == 32 || operand1Type == 36)
          n = std::sprintf(bufp, operandTypes[operand1Type],
                           int(indexOffset < 128 ? '+' : '-'),
                           (unsigned int) (indexOffset < 128 ?
                                           indexOffset : (256 - indexOffset)));
        else
          n = std::sprintf(bufp, "%s", operandTypes[operand1Type]);
        bufp = bufp + n;
      }
      if (operand1Type != 0 && operand2Type != 0)
        *(bufp++) = ',';
      if (operand2Type != 0) {
        *(bufp++) = ' ';
        if ((operand2Type == 32 || operand2Type == 36) && indexOffset == 0)
          operand2Type = operand2Type + 12;
        if (operand2Type >= 17 && operand2Type < 22) {
          if (operand2Type != 19)
            n = std::sprintf(bufp, operandTypes[operand2Type],
                             (unsigned int) operand);
          else
            n = std::sprintf(bufp, operandTypes[operand2Type],
                             int(isCPUAddress ? 4 : 6), (unsigned int) operand);
        }
        else if (operand2Type == 32 || operand2Type == 36)
          n = std::sprintf(bufp, operandTypes[operand2Type],
                           int(indexOffset < 128 ? '+' : '-'),
                           (unsigned int) (indexOffset < 128 ?
                                           indexOffset : (256 - indexOffset)));
        else
          n = std::sprintf(bufp, "%s", operandTypes[operand2Type]);
        bufp = bufp + n;
      }
    }
    else {
      n = std::sprintf(bufp, "  ???");
      bufp = bufp + n;
    }
    buf = &(tmpBuf[0]);
    return addr;
  }

  uint32_t Z80Disassembler::getNextInstructionAddr(
      const Ep128Emu::VirtualMachine& vm, uint32_t addr, bool isCPUAddress)
  {
    uint32_t  addrMask = (isCPUAddress ? 0x0000FFFFU : 0x003FFFFFU);
    addr &= addrMask;
    bool      useIX = false;
    bool      useIY = false;
    bool      useIndexOffset = false;
    bool      haveIndexOffset = false;
    bool      invalidOpcode = false;
    const unsigned char *opcodeTablePtr = &(opcodeTable[0]);
    uint8_t   opNum = vm.readMemory(addr, isCPUAddress) & 0xFF;
    addr = (addr + 1U) & addrMask;
    if (opNum == 0xDD)
      useIX = true;
    else if (opNum == 0xFD)
      useIY = true;
    if (useIX || useIY) {
      opNum = vm.readMemory(addr, isCPUAddress) & 0xFF;
      addr = (addr + 1U) & addrMask;
    }
    if (opNum == 0xCB) {
      opcodeTablePtr = &(opcodeTableCB[0]);
      useIndexOffset = (useIX || useIY);
      if (useIndexOffset) {
        addr = (addr + 1U) & addrMask;
        haveIndexOffset = true;
      }
      opNum = vm.readMemory(addr, isCPUAddress) & 0xFF;
      addr = (addr + 1U) & addrMask;
    }
    else if (opNum == 0xED) {
      opcodeTablePtr = &(opcodeTableED[0]);
      invalidOpcode = (useIX || useIY);
      opNum = vm.readMemory(addr, isCPUAddress) & 0xFF;
      addr = (addr + 1U) & addrMask;
    }
    opcodeTablePtr = (opcodeTablePtr + (size_t(opNum) * 3));
    if (opcodeTablePtr[0] > 68)
      invalidOpcode = true;
    unsigned char operand1Type = opcodeTablePtr[1];
    unsigned char operand2Type = opcodeTablePtr[2];
    if (useIndexOffset && !(operand1Type == 28 || operand2Type == 28))
      invalidOpcode = true;
    if ((useIX || useIY) && !invalidOpcode) {
      if (operand1Type == 28 || operand2Type == 28)
        useIndexOffset = true;
    }
    if (useIndexOffset && !haveIndexOffset) {
      addr = (addr + 1U) & addrMask;
    }
    if (operand1Type >= 17 && operand1Type < 22) {
      addr = (addr + 1U) & addrMask;
      if (operand1Type >= 20)
        addr = (addr + 1U) & addrMask;
    }
    if (operand2Type >= 17 && operand2Type < 22) {
      addr = (addr + 1U) & addrMask;
      if (operand2Type >= 20)
        addr = (addr + 1U) & addrMask;
    }
    return addr;
  }

  void Z80Disassembler::parseOperand(const std::vector< std::string >& args,
                                     size_t argOffs, size_t argCnt, int& opType,
                                     bool& haveOpValue, uint32_t& opValue)
  {
    opType = 0;
    haveOpValue = false;
    opValue = 0U;
    if (argCnt < 1)
      return;
    opType = -1;
    for (size_t i = argOffs; i < (argOffs + argCnt); i++) {
      uint32_t  tmp = 0U;
      if (Ep128Emu::parseHexNumber(tmp, args[i].c_str())) {
        opValue = tmp;
        haveOpValue = true;
        break;
      }
    }
    if (argCnt == 1) {
      for (int i = 1; i < 75; i++) {
        if (i == 17)
          i = 22;
        else if (i == 38)
          i = 50;
        else if (i == 28 || i == 32 || i == 36 || i == 62 || i == 65)
          i++;
        if (args[argOffs] == operandTypes[i]) {
          opType = i;
          break;
        }
        if (opType < 0 && haveOpValue) {
          if (opValue <= 7U)
            opType = int(opValue + 1U);
          else if (!(opValue & (~(uint32_t(0x38)))))
            opType = int((opValue >> 3) + 9U);
          else
            opType = (args[argOffs].length() <= 2 ? 17 : 20);
        }
      }
    }
    else if (argCnt == 2) {
      // AF'
      if (args[argOffs] == "AF" && args[argOffs + 1] == "'")
        opType = 65;
    }
    else if (argCnt == 3) {
      // (nn), (nnnn), (C), (BC), (DE), (HL), (SP), (IX), (IY)
      if (args[argOffs] == "(" && args[argOffs + 2] == ")") {
        if (args[argOffs + 1] == "C")
          opType = 62;
        else if (args[argOffs + 1] == "BC")
          opType = 38;
        else if (args[argOffs + 1] == "DE")
          opType = 39;
        else if (args[argOffs + 1] == "HL")
          opType = 28;
        else if (args[argOffs + 1] == "SP")
          opType = 41;
        else if (args[argOffs + 1] == "IX")
          opType = 44;
        else if (args[argOffs + 1] == "IY")
          opType = 48;
        else if (haveOpValue)
          opType = (args[argOffs + 1].length() <= 2 ? 18 : 21);
      }
    }
    else if (argCnt == 5) {
      // (IX+nn), (IX-nn), (IY+nn), (IY-nn)
      if (args[argOffs] == "(" &&
          (args[argOffs + 2] == "+" || args[argOffs + 2] == "-") &&
          haveOpValue && args[argOffs + 4] == ")") {
        if (args[argOffs + 1] == "IX")
          opType = 32;
        else if (args[argOffs + 1] == "IY")
          opType = 36;
      }
      if (opType >= 0) {
        if ((args[argOffs + 2] == "+" && opValue > 0x7FU) ||
            (args[argOffs + 2] == "-" && opValue > 0x80U)) {
          throw Ep128Emu::Exception("index offset is out of range");
        }
        if (opValue == 0U)
          opType += 12;         // allow code like 'JP (IX+0)'
      }
    }
    if (opType < 0)
      throw Ep128Emu::Exception("assembler syntax error");
  }

  uint32_t Z80Disassembler::assembleInstruction(
      const std::vector< std::string >& args, Ep128Emu::VirtualMachine& vm,
      bool isCPUAddress, int32_t offs)
  {
    uint32_t  addrMask = (isCPUAddress ? 0x0000FFFFU : 0x003FFFFFU);
    int32_t   startAddr = -1;
    int       opcodeNum = -1;
    size_t    op1Offs = 0;
    size_t    op1Cnt = 0;
    size_t    op2Offs = 0;
    size_t    op2Cnt = 0;
    // find start address, opcode name, and operand groups
    for (size_t i = 0; i < args.size(); i++) {
      const std::string&  s = args[i];
      if (i == 0 && (s == "A" || s == "."))
        continue;
      if (startAddr < 0) {
        uint32_t  tmp = Ep128Emu::parseHexNumberEx(s.c_str());
        if (tmp > addrMask)
          throw Ep128Emu::Exception("assemble address is out of range");
        startAddr = int32_t(tmp);
        continue;
      }
      if (opcodeNum < 0) {
        if (s.length() >= 2 && s.length() <= 4) {
          char    tmpBuf[4];
          for (size_t j = 0; j < 4; j++) {
            if (j < s.length())
              tmpBuf[j] = s[j];
            else
              tmpBuf[j] = ' ';
          }
          for (int j = 0; j < 69 && opcodeNum < 0; j++) {
            for (int k = 0; k < 4; k++) {
              if (tmpBuf[k] != opcodeNames[(j * 5) + k + 1])
                break;
              if (k == 3)
                opcodeNum = j;
            }
          }
        }
        if (opcodeNum < 0) {
          if (Ep128Emu::parseHexNumberEx(s.c_str()) > 0xFFU)
            throw Ep128Emu::Exception("byte value is out of range");
        }
        continue;
      }
      if (!op1Offs)
        op1Offs = i;
      if (s == ",") {
        if (op2Offs != 0 || op1Offs == i)
          throw Ep128Emu::Exception("assembler syntax error");
        op1Cnt = i - op1Offs;
        op2Offs = i + 1;
      }
    }
    if (op2Offs) {
      if (op2Offs < args.size())
        op2Cnt = args.size() - op2Offs;
      else
        throw Ep128Emu::Exception("assembler syntax error");
    }
    if (op1Offs) {
      if (!op2Offs)
        op1Cnt = args.size() - op1Offs;
    }
    if (opcodeNum < 0)                  // no valid opcode name was found
      throw Ep128Emu::Exception("assembler syntax error");
    // parse operands
    int       op1Type = 0;
    uint32_t  op1Value = 0U;
    bool      haveOp1Value = false;
    int       op2Type = 0;
    uint32_t  op2Value = 0U;
    bool      haveOp2Value = false;
    parseOperand(args, op1Offs, op1Cnt, op1Type, haveOp1Value, op1Value);
    parseOperand(args, op2Offs, op2Cnt, op2Type, haveOp2Value, op2Value);
    // look up this combination of opcode and operand types in the
    // disassembler tables, and if not found, try again if there are
    // similar operand types
    int       savedOp1Type = op1Type;
    int       savedOp2Type = op2Type;
    int       op1TypeN = int(noPrefixOperandTypeTable[op1Type]);
    int       op2TypeN = int(noPrefixOperandTypeTable[op2Type]);
    bool      op1RetryFlag = false;
    bool      op2RetryFlag = false;
    int       opcodeByte = -1;
    bool      cbFlag = false;
    bool      ddFlag = false;
    bool      edFlag = false;
    bool      fdFlag = false;
    while (true) {
      for (int i = 0; i < 256; i++) {
        int     tmp = i * 3;
        if (int(opcodeTable[tmp]) == opcodeNum &&
            int(opcodeTable[tmp + 1]) == op1TypeN &&
            int(opcodeTable[tmp + 2]) == op2TypeN) {
          opcodeByte = i;
          break;
        }
        if (int(opcodeTableCB[tmp]) == opcodeNum &&
            int(opcodeTableCB[tmp + 1]) == op1TypeN &&
            int(opcodeTableCB[tmp + 2]) == op2TypeN) {
          opcodeByte = i;
          cbFlag = true;
          break;
        }
        if (int(opcodeTableED[tmp]) == opcodeNum &&
            int(opcodeTableED[tmp + 1]) == op1TypeN &&
            int(opcodeTableED[tmp + 2]) == op2TypeN) {
          opcodeByte = i;
          edFlag = true;
          break;
        }
      }
      if (opcodeByte >= 0)
        break;
      if (alternateOperandTypeTable[op1Type] != 0) {
        op1Type = int(alternateOperandTypeTable[op1Type]);
      }
      else if (alternateOperandTypeTable[op2Type] != 0) {
        op1Type = savedOp1Type;
        op2Type = int(alternateOperandTypeTable[op2Type]);
      }
      else if (op1Type == 19 && op1Value <= 0xFFU && !op1RetryFlag) {
        op1RetryFlag = true;
        op1Type = 17;
        op2Type = savedOp2Type;
      }
      else if (op1Type == 21 && op1Value <= 0xFFU && !op1RetryFlag) {
        op1RetryFlag = true;
        op1Type = 18;
        op2Type = savedOp2Type;
      }
      else if (op2Type == 19 && op2Value <= 0xFFU && !op2RetryFlag) {
        op2RetryFlag = true;
        op1Type = savedOp1Type;
        op2Type = 17;
      }
      else if (op2Type == 21 && op2Value <= 0xFFU && !op2RetryFlag) {
        op2RetryFlag = true;
        op1Type = savedOp1Type;
        op2Type = 18;
      }
      else
        throw Ep128Emu::Exception("invalid operands");
      op1TypeN = int(noPrefixOperandTypeTable[op1Type]);
      op2TypeN = int(noPrefixOperandTypeTable[op2Type]);
    }
    // check operands for any numeric values, and calculate relative offsets
    int       indexOffset = -1;
    int       operandBytes = 0;
    uint32_t  opValue = 0U;
    if (op1TypeN != op1Type) {
      if ((op1Type - op1TypeN) == 4)
        ddFlag = true;
      else
        fdFlag = true;
      if (op1Type == 32 || op1Type == 36) {
        if (opcodeNum == 27) {
          // index + non-zero offset is not allowed for JP
          throw Ep128Emu::Exception("invalid operand type");
        }
        indexOffset = 0;
        if (haveOp1Value) {
          indexOffset = int(op1Value);
          if (args[op1Offs + 2] == "-")
            indexOffset = (256 - indexOffset) & 0xFF;
        }
      }
    }
    if (op2TypeN != op2Type) {
      if ((op2Type - op2TypeN) == 4)
        ddFlag = true;
      else
        fdFlag = true;
      if (op2Type == 32 || op2Type == 36) {
        indexOffset = 0;
        if (haveOp2Value) {
          indexOffset = int(op2Value);
          if (args[op2Offs + 2] == "-")
            indexOffset = (256 - indexOffset) & 0xFF;
        }
      }
    }
    if (ddFlag && fdFlag)
      throw Ep128Emu::Exception("invalid operand types");
    if (op1Type == 17 || op2Type == 17) {
      opValue = (op1Type == 17 ? op1Value : op2Value);
      if (opValue > 0xFFU)
        throw Ep128Emu::Exception("byte value is out of range");
      operandBytes = 1;
    }
    else if (op1Type == 18 || op2Type == 18) {
      opValue = (op1Type == 18 ? op1Value : op2Value);
      if (opValue > 0xFFU)
        throw Ep128Emu::Exception("port address is out of range");
      operandBytes = 1;
    }
    else if (op1Type == 19 || op2Type == 19) {
      uint32_t  addr = (op1Type == 19 ? op1Value : op2Value);
      if (addr > addrMask)
        throw Ep128Emu::Exception("address is out of range");
      int     jumpOffset = int(int32_t(addr) - (startAddr + 2));
      jumpOffset = ((jumpOffset + int((addrMask >> 1) + 1U)) & int(addrMask))
                   - int((addrMask >> 1) + 1U);
      if (jumpOffset < -128 || jumpOffset > 127)
        throw Ep128Emu::Exception("relative jump address is out of range");
      if (jumpOffset < 0)
        jumpOffset += 256;
      opValue = uint32_t(jumpOffset);
      operandBytes = 1;
    }
    else if (op1Type == 20 || op2Type == 20) {
      opValue = (op1Type == 20 ? op1Value : op2Value);
      if (opValue > 0xFFFFU)
        throw Ep128Emu::Exception("16 bit value is out of range");
      operandBytes = 2;
    }
    else if (op1Type == 21 || op2Type == 21) {
      opValue = (op1Type == 21 ? op1Value : op2Value);
      if (opValue > 0xFFFFU)
        throw Ep128Emu::Exception("address is out of range");
      operandBytes = 2;
    }
    // write the assembled instruction to memory
    startAddr = (startAddr + offs) & int32_t(addrMask);
    uint32_t  addr = uint32_t(startAddr);
    if (ddFlag) {
      vm.writeMemory(addr, 0xDD, isCPUAddress);
      addr = (addr + 1U) & addrMask;
    }
    if (fdFlag) {
      vm.writeMemory(addr, 0xFD, isCPUAddress);
      addr = (addr + 1U) & addrMask;
    }
    if (cbFlag) {
      vm.writeMemory(addr, 0xCB, isCPUAddress);
      addr = (addr + 1U) & addrMask;
      if (indexOffset >= 0) {
        vm.writeMemory(addr, uint8_t(indexOffset), isCPUAddress);
        addr = (addr + 1U) & addrMask;
      }
    }
    if (edFlag) {
      vm.writeMemory(addr, 0xED, isCPUAddress);
      addr = (addr + 1U) & addrMask;
    }
    vm.writeMemory(addr, uint8_t(opcodeByte), isCPUAddress);
    addr = (addr + 1U) & addrMask;
    if (indexOffset >= 0 && !cbFlag) {
      vm.writeMemory(addr, uint8_t(indexOffset), isCPUAddress);
      addr = (addr + 1U) & addrMask;
    }
    if (operandBytes > 0) {
      vm.writeMemory(addr, uint8_t(opValue & 0xFFU), isCPUAddress);
      addr = (addr + 1U) & addrMask;
      if (operandBytes > 1) {
        vm.writeMemory(addr, uint8_t((opValue >> 8) & 0xFFU), isCPUAddress);
        addr = (addr + 1U) & addrMask;
      }
    }
    return uint32_t(startAddr);
  }

}       // namespace Ep128

namespace Ep128Emu {

  void tokenizeString(std::vector<std::string>& args, const char *s)
  {
    args.resize(0);
    if (!s)
      return;
    std::string curToken = "";
    int         mode = 0;         // 0: skipping space, 1: token, 2: string
    while (*s != '\0') {
      if (mode == 0) {
        if (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') {
          s++;
          continue;
        }
        mode = (*s != '"' ? 1 : 2);
      }
      if (mode == 1) {
        if (args.size() == 0 && curToken.length() > 0) {
          // allow no space between command and first hexadecimal argument
          if ((*s >= '0' && *s <= '9') ||
              (*s >= 'A' && *s <= 'F') || (*s >= 'a' && *s <= 'f')) {
            args.push_back(curToken);
            curToken = "";
            continue;
          }
        }
        if ((*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') ||
            *s == '_' || *s == '?') {
          curToken += (*s);
          s++;
          continue;
        }
        else if (*s >= 'a' && *s <= 'z') {
          // convert to upper case
          curToken += ((*s - 'a') + 'A');
          s++;
          continue;
        }
        else {
          if (curToken.length() > 0) {
            args.push_back(curToken);
            curToken = "";
          }
          if (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') {
            mode = 0;
            s++;
            continue;
          }
          if (*s != '"') {
            curToken = (*s);
            args.push_back(curToken);
            curToken = "";
            mode = 0;
            s++;
            continue;
          }
          mode = 2;
        }
      }
      if (*s == '"' && curToken.length() > 0) {
        // closing quote character is not stored
        args.push_back(curToken);
        curToken = "";
        mode = 0;
      }
      else {
        curToken += (*s);
      }
      s++;
    }
    if (curToken.length() > 0)
      args.push_back(curToken);
  }

  bool parseHexNumber(uint32_t& n, const char *s)
  {
    n = 0U;
    size_t  i;
    for (i = 0; s[i] != '\0'; i++) {
      if (s[i] >= '0' && s[i] <= '9')
        n = (n << 4) | uint32_t(s[i] - '0');
      else if (s[i] >= 'A' && s[i] <= 'F')
        n = (n << 4) | uint32_t((s[i] - 'A') + 10);
      else if (s[i] >= 'a' && s[i] <= 'f')
        n = (n << 4) | uint32_t((s[i] - 'a') + 10);
      else
        return ((s[i] == 'H' || s[i] == 'h') && s[i + 1] == '\0' && i != 0);
    }
    return (i != 0);
  }

  uint32_t parseHexNumberEx(const char *s, uint32_t mask_)
  {
    uint32_t  n = 0U;
    if (!parseHexNumber(n, s))
      throw Ep128Emu::Exception("invalid hexadecimal number format");
    return (n & mask_);
  }

}       // namespace Ep128Emu

