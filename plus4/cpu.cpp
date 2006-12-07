
// plus4 -- portable Commodore PLUS/4 emulator
// Copyright (C) 2003-2006 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "cpu.hpp"

namespace Plus4 {

  const unsigned char M7501::cpu_opcode_length[256] = {
    1, 2, 0, 0,  0, 2, 2, 0,  1, 2, 1, 0,  0, 3, 3, 0,      // 00
    2, 2, 0, 0,  0, 2, 2, 0,  1, 3, 1, 0,  0, 3, 3, 0,      // 10
    3, 2, 0, 0,  2, 2, 2, 0,  1, 2, 1, 0,  3, 3, 3, 0,      // 20
    2, 2, 0, 0,  0, 2, 2, 0,  1, 3, 1, 0,  0, 3, 3, 0,      // 30
    1, 2, 0, 0,  0, 2, 2, 0,  1, 2, 1, 0,  3, 3, 3, 0,      // 40
    2, 2, 0, 0,  0, 2, 2, 0,  1, 3, 1, 0,  0, 3, 3, 0,      // 50
    1, 2, 0, 0,  0, 2, 2, 0,  1, 2, 1, 0,  3, 3, 3, 0,      // 60
    2, 2, 0, 0,  0, 2, 2, 0,  1, 3, 1, 0,  0, 3, 3, 0,      // 70
    0, 2, 0, 0,  2, 2, 2, 0,  1, 0, 1, 0,  3, 3, 3, 0,      // 80
    2, 2, 0, 0,  2, 2, 2, 0,  1, 3, 1, 0,  0, 3, 0, 0,      // 90
    2, 2, 2, 0,  2, 2, 2, 0,  1, 2, 1, 0,  3, 3, 3, 0,      // A0
    2, 2, 0, 0,  2, 2, 2, 0,  1, 3, 1, 0,  3, 3, 3, 0,      // B0
    2, 2, 0, 0,  2, 2, 2, 0,  1, 2, 1, 0,  3, 3, 3, 0,      // C0
    2, 2, 0, 0,  0, 2, 2, 0,  1, 3, 1, 0,  0, 3, 3, 0,      // D0
    2, 2, 0, 0,  2, 2, 2, 0,  1, 2, 1, 0,  3, 3, 3, 0,      // E0
    2, 2, 0, 0,  0, 2, 2, 0,  1, 3, 1, 0,  0, 3, 3, 0       // F0
  };

  M7501::M7501()
  {
    reg_PC = (uint16_t) 0;
    reg_SR = (uint8_t) 0x24;
    reg_AC = (uint8_t) 0;
    reg_XR = (uint8_t) 0;
    reg_YR = (uint8_t) 0;
    reg_SP = (uint8_t) 0xFF;
    cyclesRemaining = 1;
    currentOpcode = 0;
    currentOperand = 0;
  }

  void M7501::reset(bool cold_reset)
  {
    (void) cold_reset;
    reg_PC = (uint16_t) readMemory((uint16_t) 0xFFFC);
    reg_PC += ((uint16_t) readMemory((uint16_t) 0xFFFD) << 8);
    reg_SR |= (uint8_t) 0x04;
    cyclesRemaining = 1;
    currentOpcode = 0;
    currentOperand = 0;
  }

  void M7501::interruptRequest_()
  {
    op_push((uint8_t) (reg_PC >> 8));
    op_push((uint8_t) reg_PC);
    reg_SR &= (uint8_t) 0xEF;
    op_push(reg_SR);
    reg_PC = (uint16_t) readMemory((uint16_t) 0xFFFE);
    reg_PC += ((uint16_t) readMemory((uint16_t) 0xFFFF) << 8);
    reg_SR |= (uint8_t) 0x14;
    cyclesRemaining += 6;       // FIXME: this may be incorrect
  }

  void M7501::runOneOpcode_()
  {
    unsigned char byteCnt;

    currentOpcode = (int) readMemory(reg_PC);
    reg_PC++;
    byteCnt = cpu_opcode_length[currentOpcode];
    if (!byteCnt) {
      // reset on invalid opcode
      this->reset(true);
      return;
    }
    if (byteCnt > (unsigned char) 1) {
      currentOperand = (int) readMemory(reg_PC);
      reg_PC++;
    }
    if (byteCnt > (unsigned char) 2) {
      currentOperand += ((int) readMemory(reg_PC) << 8);
      reg_PC++;
    }
    switch (currentOpcode) {
    case (int) 0x00:                                    // BRK
      reg_PC++;
      op_push((uint8_t) (reg_PC >> 8));
      op_push((uint8_t) reg_PC);
      reg_SR |= (uint8_t) 0x10;
      op_push(reg_SR);
      reg_PC = (uint16_t) readMemory((uint16_t) 0xFFFE);
      reg_PC += ((uint16_t) readMemory((uint16_t) 0xFFFF) << 8);
      reg_SR |= (uint8_t) 0x04;
      cyclesRemaining += 6;
      break;
    case (int) 0x01:                                    // ORA (nn,X)
      op_ora(reg_AC, readMemory(op_addr_nn_X_indirect()));
      cyclesRemaining = 4;
      break;
    case (int) 0x05:                                    // ORA nn
      op_ora(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0x06:                                    // ASL nn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_asl(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0x08:                                    // PHP
      op_push(reg_SR);
      cyclesRemaining = 2;
      break;
    case (int) 0x09:                                    // ORA #nn
      op_ora(reg_AC, (uint8_t) currentOperand);
      break;
    case (int) 0x0A:                                    // ASL
      op_asl(reg_AC);
      cyclesRemaining = 1;
      break;
    case (int) 0x0D:                                    // ORA nnnn
      op_ora(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0x0E:                                    // ASL nnnn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_asl(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0x10:                                    // BPL nn
      op_branch(reg_SR & (uint8_t) 0x80 ? false : true);
      break;
    case (int) 0x11:                                    // ORA (nn),Y
      cyclesRemaining = 3;
      op_ora(reg_AC, readMemory(op_addr_nn_Y_indirect_wait()));
      break;
    case (int) 0x15:                                    // ORA nn,X
      op_ora(reg_AC, readMemory(op_addr_nn_X()));
      cyclesRemaining = 2;
      break;
    case (int) 0x16:                                    // ASL nn,X
      {
        uint16_t  addr = op_addr_nn_X();
        uint8_t   value = readMemory(addr);
        op_asl(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    case (int) 0x18:                                    // CLC
      reg_SR &= (uint8_t) 0xFE;
      cyclesRemaining = 1;
      break;
    case (int) 0x19:                                    // ORA nnnn,Y
      cyclesRemaining = 1;
      op_ora(reg_AC, readMemory(op_addr_nnnn_Y_wait()));
      break;
    case (int) 0x1A:                                    // NOP
      cyclesRemaining = 1;
      break;
    case (int) 0x1D:                                    // ORA nnnn,X
      cyclesRemaining = 1;
      op_ora(reg_AC, readMemory(op_addr_nnnn_X_wait()));
      break;
    case (int) 0x1E:                                    // ASL nnnn,X
      {
        uint16_t  addr = op_addr_nnnn_X();
        uint8_t   value = readMemory(addr);
        op_asl(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    case (int) 0x20:                                    // JSR nnnn
      reg_PC--;
      op_push((uint8_t) (reg_PC >> 8));
      op_push((uint8_t) reg_PC);
      reg_PC = (uint16_t) currentOperand;
      cyclesRemaining = 3;
      break;
    case (int) 0x21:                                    // AND (nn,X)
      op_and(reg_AC, readMemory(op_addr_nn_X_indirect()));
      cyclesRemaining = 4;
      break;
    case (int) 0x24:                                    // BIT nn
      op_bit(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0x25:                                    // AND nn
      op_and(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0x26:                                    // ROL nn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_rol(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0x28:                                    // PLP
      reg_SR = op_pop() | (uint8_t) 0x20;
      cyclesRemaining = 3;
      break;
    case (int) 0x29:                                    // AND #nn
      op_and(reg_AC, (uint8_t) currentOperand);
      break;
    case (int) 0x2A:                                    // ROL
      op_rol(reg_AC);
      cyclesRemaining = 1;
      break;
    case (int) 0x2C:                                    // BIT nnnn
      op_bit(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0x2D:                                    // AND nnnn
      op_and(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0x2E:                                    // ROL nnnn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_rol(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0x30:                                    // BMI nn
      op_branch(reg_SR & (uint8_t) 0x80 ? true : false);
      break;
    case (int) 0x31:                                    // AND (nn),Y
      cyclesRemaining = 3;
      op_and(reg_AC, readMemory(op_addr_nn_Y_indirect_wait()));
      break;
    case (int) 0x35:                                    // AND nn,X
      op_and(reg_AC, readMemory(op_addr_nn_X()));
      cyclesRemaining = 2;
      break;
    case (int) 0x36:                                    // ROL nn,X
      {
        uint16_t  addr = op_addr_nn_X();
        uint8_t   value = readMemory(addr);
        op_rol(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    case (int) 0x38:                                    // SEC
      reg_SR |= (uint8_t) 0x01;
      cyclesRemaining = 1;
      break;
    case (int) 0x39:                                    // AND nnnn,Y
      cyclesRemaining = 1;
      op_and(reg_AC, readMemory(op_addr_nnnn_Y_wait()));
      break;
    case (int) 0x3A:                                    // NOP
      cyclesRemaining = 1;
      break;
    case (int) 0x3D:                                    // AND nnnn,X
      cyclesRemaining = 1;
      op_and(reg_AC, readMemory(op_addr_nnnn_X_wait()));
      break;
    case (int) 0x3E:                                    // ROL nnnn,X
      {
        uint16_t  addr = op_addr_nnnn_X();
        uint8_t   value = readMemory(addr);
        op_rol(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    case (int) 0x40:                                    // RTI
      reg_SR = op_pop() | (uint8_t) 0x20;
      reg_PC = (uint16_t) op_pop();
      reg_PC += ((uint16_t) op_pop() << 8);
      cyclesRemaining = 5;
      break;
    case (int) 0x41:                                    // EOR (nn,X)
      op_eor(reg_AC, readMemory(op_addr_nn_X_indirect()));
      cyclesRemaining = 4;
      break;
    case (int) 0x45:                                    // EOR nn
      op_eor(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0x46:                                    // LSR nn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_lsr(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0x48:                                    // PHA
      op_push(reg_AC);
      cyclesRemaining = 2;
      break;
    case (int) 0x49:                                    // EOR #nn
      op_eor(reg_AC, (uint8_t) currentOperand);
      break;
    case (int) 0x4A:                                    // LSR
      op_lsr(reg_AC);
      cyclesRemaining = 1;
      break;
    case (int) 0x4C:                                    // JMP nnnn
      reg_PC = (uint16_t) currentOperand;
      break;
    case (int) 0x4D:                                    // EOR nnnn
      op_eor(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0x4E:                                    // LSR nnnn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_lsr(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0x50:                                    // BVC nn
      op_branch(reg_SR & (uint8_t) 0x40 ? false : true);
      break;
    case (int) 0x51:                                    // EOR (nn),Y
      cyclesRemaining = 3;
      op_eor(reg_AC, readMemory(op_addr_nn_Y_indirect_wait()));
      break;
    case (int) 0x55:                                    // EOR nn,X
      op_eor(reg_AC, readMemory(op_addr_nn_X()));
      cyclesRemaining = 2;
      break;
    case (int) 0x56:                                    // LSR nn,X
      {
        uint16_t  addr = op_addr_nn_X();
        uint8_t   value = readMemory(addr);
        op_lsr(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    case (int) 0x58:                                    // CLI
      reg_SR &= (uint8_t) 0xFB;
      cyclesRemaining = 1;
      break;
    case (int) 0x59:                                    // EOR nnnn,Y
      cyclesRemaining = 1;
      op_eor(reg_AC, readMemory(op_addr_nnnn_Y_wait()));
      break;
    case (int) 0x5A:                                    // NOP
      cyclesRemaining = 1;
      break;
    case (int) 0x5D:                                    // EOR nnnn,X
      cyclesRemaining = 1;
      op_eor(reg_AC, readMemory(op_addr_nnnn_X_wait()));
      break;
    case (int) 0x5E:                                    // LSR nnnn,X
      {
        uint16_t  addr = op_addr_nnnn_X();
        uint8_t   value = readMemory(addr);
        op_lsr(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    case (int) 0x60:                                    // RTS
      reg_PC = (uint16_t) op_pop();
      reg_PC += ((uint16_t) op_pop() << 8);
      reg_PC++;
      cyclesRemaining = 5;
      break;
    case (int) 0x61:                                    // ADC (nn,X)
      op_adc(reg_AC, readMemory(op_addr_nn_X_indirect()));
      cyclesRemaining = 4;
      break;
    case (int) 0x65:                                    // ADC nn
      op_adc(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0x66:                                    // ROR nn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_ror(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0x68:                                    // PLA
      reg_AC = op_pop();
      op_setnz(reg_AC);
      cyclesRemaining = 3;
      break;
    case (int) 0x69:                                    // ADC #nn
      op_adc(reg_AC, (uint8_t) currentOperand);
      break;
    case (int) 0x6A:                                    // ROR
      op_ror(reg_AC);
      cyclesRemaining = 1;
      break;
    case (int) 0x6C:                                    // JMP (nnnn)
      {
        uint16_t  addr = (uint16_t) currentOperand;
        reg_PC = (uint16_t) readMemory(addr);
        addr++;
        reg_PC += ((uint16_t) readMemory(addr) << 8);
      }
      cyclesRemaining = 2;
      break;
    case (int) 0x6D:                                    // ADC nnnn
      op_adc(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0x6E:                                    // ROR nnnn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_ror(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0x70:                                    // BVS nn
      op_branch(reg_SR & (uint8_t) 0x40 ? true : false);
      break;
    case (int) 0x71:                                    // ADC (nn),Y
      cyclesRemaining = 3;
      op_adc(reg_AC, readMemory(op_addr_nn_Y_indirect_wait()));
      break;
    case (int) 0x75:                                    // ADC nn,X
      op_adc(reg_AC, readMemory(op_addr_nn_X()));
      cyclesRemaining = 2;
      break;
    case (int) 0x76:                                    // ROR nn,X
      {
        uint16_t  addr = op_addr_nn_X();
        uint8_t   value = readMemory(addr);
        op_ror(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    case (int) 0x78:                                    // SEI
      reg_SR |= (uint8_t) 0x04;
      cyclesRemaining = 1;
      break;
    case (int) 0x79:                                    // ADC nnnn,Y
      cyclesRemaining = 1;
      op_adc(reg_AC, readMemory(op_addr_nnnn_Y_wait()));
      break;
    case (int) 0x7A:                                    // NOP
      cyclesRemaining = 1;
      break;
    case (int) 0x7D:                                    // ADC nnnn,X
      cyclesRemaining = 1;
      op_adc(reg_AC, readMemory(op_addr_nnnn_X_wait()));
      break;
    case (int) 0x7E:                                    // ROR nnnn,X
      {
        uint16_t  addr = op_addr_nnnn_X();
        uint8_t   value = readMemory(addr);
        op_ror(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    case (int) 0x81:                                    // STA (nn,X)
      writeMemory(op_addr_nn_X_indirect(), reg_AC);
      cyclesRemaining = 4;
      break;
    case (int) 0x84:                                    // STY nn
      writeMemory((uint16_t) currentOperand, reg_YR);
      cyclesRemaining = 1;
      break;
    case (int) 0x85:                                    // STA nn
      writeMemory((uint16_t) currentOperand, reg_AC);
      cyclesRemaining = 1;
      break;
    case (int) 0x86:                                    // STX nn
      writeMemory((uint16_t) currentOperand, reg_XR);
      cyclesRemaining = 1;
      break;
    case (int) 0x88:                                    // DEY
      op_dec(reg_YR);
      cyclesRemaining = 1;
      break;
    case (int) 0x8A:                                    // TXA
      reg_AC = reg_XR;
      op_setnz(reg_AC);
      cyclesRemaining = 1;
      break;
    case (int) 0x8C:                                    // STY nnnn
      writeMemory((uint16_t) currentOperand, reg_YR);
      cyclesRemaining = 1;
      break;
    case (int) 0x8D:                                    // STA nnnn
      writeMemory((uint16_t) currentOperand, reg_AC);
      cyclesRemaining = 1;
      break;
    case (int) 0x8E:                                    // STX nnnn
      writeMemory((uint16_t) currentOperand, reg_XR);
      cyclesRemaining = 1;
      break;
    case (int) 0x90:                                    // BCC nn
      op_branch(reg_SR & (uint8_t) 0x01 ? false : true);
      break;
    case (int) 0x91:                                    // STA (nn),Y
      writeMemory(op_addr_nn_Y_indirect(), reg_AC);
      cyclesRemaining = 4;
      break;
    case (int) 0x94:                                    // STY nn,X
      writeMemory(op_addr_nn_X(), reg_YR);
      cyclesRemaining = 2;
      break;
    case (int) 0x95:                                    // STA nn,X
      writeMemory(op_addr_nn_X(), reg_AC);
      cyclesRemaining = 2;
      break;
    case (int) 0x96:                                    // STX nn,Y
      writeMemory(op_addr_nn_Y(), reg_XR);
      cyclesRemaining = 2;
      break;
    case (int) 0x98:                                    // TYA
      reg_AC = reg_YR;
      op_setnz(reg_AC);
      cyclesRemaining = 1;
      break;
    case (int) 0x99:                                    // STA nnnn,Y
      writeMemory(op_addr_nnnn_Y(), reg_AC);
      cyclesRemaining = 2;
      break;
    case (int) 0x9A:                                    // TXS
      reg_SP = reg_XR;
      cyclesRemaining = 1;
      break;
    case (int) 0x9D:                                    // STA nnnn,X
      writeMemory(op_addr_nnnn_X(), reg_AC);
      cyclesRemaining = 2;
      break;
    case (int) 0xA0:                                    // LDY #nn
      reg_YR = (uint8_t) currentOperand;
      op_setnz(reg_YR);
      break;
    case (int) 0xA1:                                    // LDA (nn,X)
      reg_AC = readMemory(op_addr_nn_X_indirect());
      op_setnz(reg_AC);
      cyclesRemaining = 4;
      break;
    case (int) 0xA2:                                    // LDX #nn
      reg_XR = (uint8_t) currentOperand;
      op_setnz(reg_XR);
      break;
    case (int) 0xA4:                                    // LDY nn
      reg_YR = readMemory((uint16_t) currentOperand);
      op_setnz(reg_YR);
      cyclesRemaining = 1;
      break;
    case (int) 0xA5:                                    // LDA nn
      reg_AC = readMemory((uint16_t) currentOperand);
      op_setnz(reg_AC);
      cyclesRemaining = 1;
      break;
    case (int) 0xA6:                                    // LDX nn
      reg_XR = readMemory((uint16_t) currentOperand);
      op_setnz(reg_XR);
      cyclesRemaining = 1;
      break;
    case (int) 0xA8:                                    // TAY
      reg_YR = reg_AC;
      op_setnz(reg_YR);
      cyclesRemaining = 1;
      break;
    case (int) 0xA9:                                    // LDA #nn
      reg_AC = (uint8_t) currentOperand;
      op_setnz(reg_AC);
      break;
    case (int) 0xAA:                                    // TAX
      reg_XR = reg_AC;
      op_setnz(reg_XR);
      cyclesRemaining = 1;
      break;
    case (int) 0xAC:                                    // LDY nnnn
      reg_YR = readMemory((uint16_t) currentOperand);
      op_setnz(reg_YR);
      cyclesRemaining = 1;
      break;
    case (int) 0xAD:                                    // LDA nnnn
      reg_AC = readMemory((uint16_t) currentOperand);
      op_setnz(reg_AC);
      cyclesRemaining = 1;
      break;
    case (int) 0xAE:                                    // LDX nnnn
      reg_XR = readMemory((uint16_t) currentOperand);
      op_setnz(reg_XR);
      cyclesRemaining = 1;
      break;
    case (int) 0xB0:                                    // BCS nn
      op_branch(reg_SR & (uint8_t) 0x01 ? true : false);
      break;
    case (int) 0xB1:                                    // LDA (nn),Y
      cyclesRemaining = 3;
      reg_AC = readMemory(op_addr_nn_Y_indirect_wait());
      op_setnz(reg_AC);
      break;
    case (int) 0xB4:                                    // LDY nn,X
      reg_YR = readMemory(op_addr_nn_X());
      op_setnz(reg_YR);
      cyclesRemaining = 2;
      break;
    case (int) 0xB5:                                    // LDA nn,X
      reg_AC = readMemory(op_addr_nn_X());
      op_setnz(reg_AC);
      cyclesRemaining = 2;
      break;
    case (int) 0xB6:                                    // LDX nn,Y
      reg_XR = readMemory(op_addr_nn_Y());
      op_setnz(reg_XR);
      cyclesRemaining = 2;
      break;
    case (int) 0xB8:                                    // CLV
      reg_SR &= (uint8_t) 0xBF;
      cyclesRemaining = 1;
      break;
    case (int) 0xB9:                                    // LDA nnnn,Y
      cyclesRemaining = 1;
      reg_AC = readMemory(op_addr_nnnn_Y_wait());
      op_setnz(reg_AC);
      break;
    case (int) 0xBA:                                    // TSX
      reg_XR = reg_SP;
      op_setnz(reg_XR);
      cyclesRemaining = 1;
      break;
    case (int) 0xBC:                                    // LDY nnnn,X
      cyclesRemaining = 1;
      reg_YR = readMemory(op_addr_nnnn_X_wait());
      op_setnz(reg_YR);
      break;
    case (int) 0xBD:                                    // LDA nnnn,X
      cyclesRemaining = 1;
      reg_AC = readMemory(op_addr_nnnn_X_wait());
      op_setnz(reg_AC);
      break;
    case (int) 0xBE:                                    // LDX nnnn,Y
      cyclesRemaining = 1;
      reg_XR = readMemory(op_addr_nnnn_Y_wait());
      op_setnz(reg_XR);
      break;
    case (int) 0xC0:                                    // CPY #nn
      op_cmp(reg_YR, (uint8_t) currentOperand);
      break;
    case (int) 0xC1:                                    // CMP (nn,X)
      op_cmp(reg_AC, readMemory(op_addr_nn_X_indirect()));
      cyclesRemaining = 4;
      break;
    case (int) 0xC4:                                    // CPY nn
      op_cmp(reg_YR, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0xC5:                                    // CMP nn
      op_cmp(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0xC6:                                    // DEC nn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_dec(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0xC8:                                    // INY
      op_inc(reg_YR);
      cyclesRemaining = 1;
      break;
    case (int) 0xC9:                                    // CMP #nn
      op_cmp(reg_AC, (uint8_t) currentOperand);
      break;
    case (int) 0xCA:                                    // DEX
      op_dec(reg_XR);
      cyclesRemaining = 1;
      break;
    case (int) 0xCC:                                    // CPY nnnn
      op_cmp(reg_YR, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0xCD:                                    // CMP nnnn
      op_cmp(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0xCE:                                    // DEC nnnn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_dec(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0xD0:                                    // BNE nn
      op_branch(reg_SR & (uint8_t) 0x02 ? false : true);
      break;
    case (int) 0xD1:                                    // CMP (nn),Y
      cyclesRemaining = 3;
      op_cmp(reg_AC, readMemory(op_addr_nn_Y_indirect_wait()));
      break;
    case (int) 0xD5:                                    // CMP nn,X
      op_cmp(reg_AC, readMemory(op_addr_nn_X()));
      cyclesRemaining = 2;
      break;
    case (int) 0xD6:                                    // DEC nn,X
      {
        uint16_t  addr = op_addr_nn_X();
        uint8_t   value = readMemory(addr);
        op_dec(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    case (int) 0xD8:                                    // CLD
      reg_SR &= (uint8_t) 0xF7;
      cyclesRemaining = 1;
      break;
    case (int) 0xD9:                                    // CMP nnnn,Y
      cyclesRemaining = 1;
      op_cmp(reg_AC, readMemory(op_addr_nnnn_Y_wait()));
      break;
    case (int) 0xDA:                                    // NOP
      cyclesRemaining = 1;
      break;
    case (int) 0xDD:                                    // CMP nnnn,X
      cyclesRemaining = 1;
      op_cmp(reg_AC, readMemory(op_addr_nnnn_X_wait()));
      break;
    case (int) 0xDE:                                    // DEC nnnn,X
      {
        uint16_t  addr = op_addr_nnnn_X();
        uint8_t   value = readMemory(addr);
        op_dec(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    case (int) 0xE0:                                    // CPX #nn
      op_cmp(reg_XR, (uint8_t) currentOperand);
      break;
    case (int) 0xE1:                                    // SBC (nn,X)
      op_sbc(reg_AC, readMemory(op_addr_nn_X_indirect()));
      cyclesRemaining = 4;
      break;
    case (int) 0xE4:                                    // CPX nn
      op_cmp(reg_XR, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0xE5:                                    // SBC nn
      op_sbc(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0xE6:                                    // INC nn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_inc(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0xE8:                                    // INX
      op_inc(reg_XR);
      cyclesRemaining = 1;
      break;
    case (int) 0xE9:                                    // SBC #nn
      op_sbc(reg_AC, (uint8_t) currentOperand);
      break;
    case (int) 0xEA:                                    // NOP
      cyclesRemaining = 1;
      break;
    case (int) 0xEC:                                    // CPX nnnn
      op_cmp(reg_XR, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0xED:                                    // SBC nnnn
      op_sbc(reg_AC, readMemory((uint16_t) currentOperand));
      cyclesRemaining = 1;
      break;
    case (int) 0xEE:                                    // INC nnnn
      {
        uint8_t value = readMemory((uint16_t) currentOperand);
        op_inc(value);
        writeMemory((uint16_t) currentOperand, value);
      }
      cyclesRemaining = 3;
      break;
    case (int) 0xF0:                                    // BEQ nn
      op_branch(reg_SR & (uint8_t) 0x02 ? true : false);
      break;
    case (int) 0xF1:                                    // SBC (nn),Y
      cyclesRemaining = 3;
      op_sbc(reg_AC, readMemory(op_addr_nn_Y_indirect_wait()));
      break;
    case (int) 0xF5:                                    // SBC nn,X
      cyclesRemaining = 2;
      op_sbc(reg_AC, readMemory(op_addr_nn_X()));
      break;
    case (int) 0xF6:                                    // INC nn,X
      {
        uint16_t  addr = op_addr_nn_X();
        uint8_t   value = readMemory(addr);
        op_inc(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    case (int) 0xF8:                                    // SED
      reg_SR |= (uint8_t) 0x08;
      cyclesRemaining = 1;
      break;
    case (int) 0xF9:                                    // SBC nnnn,Y
      cyclesRemaining = 1;
      op_sbc(reg_AC, readMemory(op_addr_nnnn_Y_wait()));
      break;
    case (int) 0xFA:                                    // NOP
      cyclesRemaining = 1;
      break;
    case (int) 0xFD:                                    // SBC nnnn,X
      cyclesRemaining = 1;
      op_sbc(reg_AC, readMemory(op_addr_nnnn_X_wait()));
      break;
    case (int) 0xFE:                                    // INC nnnn,X
      {
        uint16_t  addr = op_addr_nnnn_X();
        uint8_t   value = readMemory(addr);
        op_inc(value);
        writeMemory(addr, value);
      }
      cyclesRemaining = 4;
      break;
    // -----------------------------------------------------------------
    case (int) 0x02:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x03:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x04:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x07:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x0B:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x0C:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x0F:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x12:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x13:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x14:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x17:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x1B:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x1C:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x1F:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x22:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x23:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x27:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x2B:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x2F:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x32:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x33:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x34:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x37:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x3B:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x3C:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x3F:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x42:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x43:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x44:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x47:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x4B:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x4F:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x52:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x53:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x54:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x57:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x5B:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x5C:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x5F:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x62:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x63:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x64:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x67:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x6B:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x6F:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x72:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x73:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x74:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x77:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x7B:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x7C:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x7F:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x80:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x82:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x83:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x87:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x89:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x8B:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x8F:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x92:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x93:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x97:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x9B:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x9C:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x9E:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0x9F:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xA3:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xA7:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xAB:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xAF:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xB2:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xB3:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xB7:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xBB:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xBF:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xC2:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xC3:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xC7:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xCB:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xCF:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xD2:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xD3:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xD4:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xD7:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xDB:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xDC:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xDF:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xE2:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xE3:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xE7:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xEB:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xEF:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xF2:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xF3:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xF4:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xF7:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xFB:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xFC:                                    // ???
      cyclesRemaining = 1;
      break;
    case (int) 0xFF:                                    // ???
      cyclesRemaining = 1;
      break;
    }
    cyclesRemaining += (int) byteCnt;
  }

}       // namespace Plus4

