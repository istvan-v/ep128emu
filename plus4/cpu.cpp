
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

static uint8_t dummyMemoryReadCallback(void *userData, uint16_t addr)
{
  (void) userData;
  (void) addr;
  return uint8_t(0xFF);
}

static void dummyMemoryWriteCallback(void *userData,
                                     uint16_t addr, uint8_t value)
{
  (void) userData;
  (void) addr;
  (void) value;
}

namespace Plus4 {

  M7501::M7501()
    : currentOpcode(&(opcodeTable[0x0FFF])),
      interruptDelayRegister(0U),
      interruptFlag(false),
      resetFlag(true),
      haltRequestFlag(false),
      haltFlag(false),
      reg_TMP(0),
      reg_L(0),
      reg_H(0),
      memoryReadCallbacks((MemoryReadFunc *) 0),
      memoryWriteCallbacks((MemoryWriteFunc *) 0),
      memoryCallbackUserData((void *) 0),
      reg_PC(0),
      reg_SR(uint8_t(0x24)),
      reg_AC(0),
      reg_XR(0),
      reg_YR(0),
      reg_SP(uint8_t(0xFF))
  {
    try {
      memoryReadCallbacks = new MemoryReadFunc[65536];
      for (size_t i = 0; i < 65536; i++)
        memoryReadCallbacks[i] = &dummyMemoryReadCallback;
      memoryWriteCallbacks = new MemoryWriteFunc[65536];
      for (size_t i = 0; i < 65536; i++)
        memoryWriteCallbacks[i] = &dummyMemoryWriteCallback;
    }
    catch (...) {
      if (memoryWriteCallbacks)
        delete[] memoryWriteCallbacks;
      if (memoryReadCallbacks)
        delete[] memoryReadCallbacks;
      throw;
    }
  }

  M7501::~M7501()
  {
    if (memoryWriteCallbacks)
      delete[] memoryWriteCallbacks;
    if (memoryReadCallbacks)
      delete[] memoryReadCallbacks;
  }

  void M7501::run(int nCycles)
  {
    do {
      if ((interruptDelayRegister & 1U) != 0U &&
          (reg_SR & uint8_t(0x04)) == uint8_t(0))
        interruptFlag = true;
      interruptDelayRegister >>= 1;
      if (haltFlag)
        continue;
      bool    doneCycle = false;
      do {
        unsigned char n = *(currentOpcode++);
        switch (n) {
        case CPU_OP_RD_OPCODE:
          {
            size_t  opNum = 0;
            if (resetFlag)
              opNum = 0x0101;
            else if (interruptFlag)
              opNum = 0x0100;
            else {
              doneCycle = true;
              if (haltRequestFlag) {
                haltFlag = true;
                currentOpcode--;
                break;
              }
              opNum = size_t(readMemory(reg_PC));
              reg_PC = (reg_PC + 1) & 0xFFFF;
            }
            currentOpcode = &(opcodeTable[opNum << 4]);
          }
          break;
        case CPU_OP_RD_TMP:
          doneCycle = true;
          if (!haltRequestFlag) {
            reg_TMP = readMemory(reg_PC);
            reg_PC = (reg_PC + 1) & 0xFFFF;
          }
          else {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        case CPU_OP_RD_L:
          doneCycle = true;
          if (!haltRequestFlag) {
            reg_L = readMemory(reg_PC);
            reg_PC = (reg_PC + 1) & 0xFFFF;
          }
          else {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        case CPU_OP_RD_H:
          doneCycle = true;
          if (!haltRequestFlag) {
            reg_H = readMemory(reg_PC);
            reg_PC = (reg_PC + 1) & 0xFFFF;
          }
          else {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        case CPU_OP_WAIT:
          doneCycle = true;
          if (haltRequestFlag) {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        case CPU_OP_LD_TMP_MEM:
          doneCycle = true;
          if (!haltRequestFlag)
            reg_TMP = readMemory(uint16_t(reg_L) | (uint16_t(reg_H) << 8));
          else {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        case CPU_OP_LD_MEM_TMP:
          writeMemory(uint16_t(reg_L) | (uint16_t(reg_H) << 8), reg_TMP);
          doneCycle = true;
          break;
        case CPU_OP_LD_H_MEM:
          doneCycle = true;
          if (!haltRequestFlag)
            reg_H = readMemory(uint16_t(reg_L) | (uint16_t(reg_H) << 8));
          else {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        case CPU_OP_PUSH_TMP:
          writeMemory(uint16_t(0x0100) | uint16_t(reg_SP), reg_TMP);
          reg_SP = (reg_SP - uint8_t(1)) & uint8_t(0xFF);
          doneCycle = true;
          break;
        case CPU_OP_POP_TMP:
          doneCycle = true;
          if (!haltRequestFlag) {
            reg_SP = (reg_SP + uint8_t(1)) & uint8_t(0xFF);
            reg_TMP = readMemory(uint16_t(0x0100) | uint16_t(reg_SP));
          }
          else {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        case CPU_OP_PUSH_PCL:
          writeMemory(uint16_t(0x0100) | uint16_t(reg_SP),
                      uint8_t(reg_PC) & uint8_t(0xFF));
          reg_SP = (reg_SP - uint8_t(1)) & uint8_t(0xFF);
          doneCycle = true;
          break;
        case CPU_OP_POP_PCL:
          doneCycle = true;
          if (!haltRequestFlag) {
            reg_SP = (reg_SP + uint8_t(1)) & uint8_t(0xFF);
            reg_PC = (reg_PC & uint16_t(0xFF00))
                     | uint16_t(readMemory(uint16_t(0x0100)
                                           | uint16_t(reg_SP)));
          }
          else {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        case CPU_OP_PUSH_PCH:
          writeMemory(uint16_t(0x0100) | uint16_t(reg_SP),
                      uint8_t(reg_PC >> 8) & uint8_t(0xFF));
          reg_SP = (reg_SP - uint8_t(1)) & uint8_t(0xFF);
          doneCycle = true;
          break;
        case CPU_OP_POP_PCH:
          doneCycle = true;
          if (!haltRequestFlag) {
            reg_SP = (reg_SP + uint8_t(1)) & uint8_t(0xFF);
            reg_PC = (reg_PC & uint16_t(0x00FF))
                     | (uint16_t(readMemory(uint16_t(0x0100)
                                            | uint16_t(reg_SP))) << 8);
          }
          else {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        case CPU_OP_LD_TMP_00:
          reg_TMP = uint8_t(0x00);
          break;
        case CPU_OP_LD_TMP_FF:
          reg_TMP = uint8_t(0xFF);
          break;
        case CPU_OP_LD_HL_PC:
          reg_L = uint8_t(reg_PC) & uint8_t(0xFF);
          reg_H = uint8_t(reg_PC >> 8) & uint8_t(0xFF);
          break;
        case CPU_OP_LD_TMP_L:
          reg_TMP = reg_L;
          break;
        case CPU_OP_LD_TMP_H:
          reg_TMP = reg_H;
          break;
        case CPU_OP_LD_TMP_SR:
          reg_TMP = reg_SR | uint8_t(0x20);
          break;
        case CPU_OP_LD_TMP_A:
          reg_TMP = reg_AC;
          break;
        case CPU_OP_LD_TMP_X:
          reg_TMP = reg_XR;
          break;
        case CPU_OP_LD_TMP_Y:
          reg_TMP = reg_YR;
          break;
        case CPU_OP_LD_TMP_SP:
          reg_TMP = reg_SP;
          break;
        case CPU_OP_LD_PC_HL:
          reg_PC = (reg_PC & uint16_t(0xFF00)) | uint16_t(reg_L);
          reg_PC = (reg_PC & uint16_t(0x00FF)) | (uint16_t(reg_H) << 8);
          break;
        case CPU_OP_LD_L_TMP:
          reg_L = reg_TMP;
          break;
        case CPU_OP_LD_H_TMP:
          reg_H = reg_TMP;
          break;
        case CPU_OP_LD_SR_TMP:
          reg_SR = reg_TMP | uint8_t(0x30);
          break;
        case CPU_OP_LD_A_TMP:
          reg_AC = reg_TMP;
          break;
        case CPU_OP_LD_X_TMP:
          reg_XR = reg_TMP;
          break;
        case CPU_OP_LD_Y_TMP:
          reg_YR = reg_TMP;
          break;
        case CPU_OP_LD_SP_TMP:
          reg_SP = reg_TMP;
          break;
        case CPU_OP_ADDR_ZEROPAGE:
          reg_H = uint8_t(0x00);
          break;
        case CPU_OP_ADDR_X:
          {
            uint8_t tmp = (reg_L + reg_XR) & uint8_t(0xFF);
            if (tmp < reg_L) {
              doneCycle = true;
              if (haltRequestFlag) {
                haltFlag = true;
                currentOpcode--;
                break;
              }
              reg_H = (reg_H + uint8_t(1)) & uint8_t(0xFF);
            }
            reg_L = tmp;
          }
          break;
        case CPU_OP_ADDR_X_SLOW:
          doneCycle = true;
          if (!haltRequestFlag) {
            reg_L = (reg_L + reg_XR) & uint8_t(0xFF);
            if (reg_L < reg_XR)
              reg_H = (reg_H + uint8_t(1)) & uint8_t(0xFF);
          }
          else {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        case CPU_OP_ADDR_Y:
          {
            uint8_t tmp = (reg_L + reg_YR) & uint8_t(0xFF);
            if (tmp < reg_L) {
              doneCycle = true;
              if (haltRequestFlag) {
                haltFlag = true;
                currentOpcode--;
                break;
              }
              reg_H = (reg_H + uint8_t(1)) & uint8_t(0xFF);
            }
            reg_L = tmp;
          }
          break;
        case CPU_OP_ADDR_Y_SLOW:
          doneCycle = true;
          if (!haltRequestFlag) {
            reg_L = (reg_L + reg_YR) & uint8_t(0xFF);
            if (reg_L < reg_YR)
              reg_H = (reg_H + uint8_t(1)) & uint8_t(0xFF);
          }
          else {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        case CPU_OP_TEST_N:
          reg_TMP = (((reg_SR ^ reg_TMP) & uint8_t(0x80)) == uint8_t(0x00) ?
                     uint8_t(0xFF) : uint8_t(0x00));
          break;
        case CPU_OP_TEST_V:
          reg_TMP = (((reg_SR ^ reg_TMP) & uint8_t(0x40)) == uint8_t(0x00) ?
                     uint8_t(0xFF) : uint8_t(0x00));
          break;
        case CPU_OP_TEST_Z:
          reg_TMP = (((reg_SR ^ reg_TMP) & uint8_t(0x02)) == uint8_t(0x00) ?
                     uint8_t(0xFF) : uint8_t(0x00));
          break;
        case CPU_OP_TEST_C:
          reg_TMP = (((reg_SR ^ reg_TMP) & uint8_t(0x01)) == uint8_t(0x00) ?
                     uint8_t(0xFF) : uint8_t(0x00));
          break;
        case CPU_OP_SET_N:
          reg_SR = (reg_SR & uint8_t(0x7F)) | (reg_TMP & uint8_t(0x80));
          break;
        case CPU_OP_SET_V:
          reg_SR = (reg_SR & uint8_t(0xBF)) | (reg_TMP & uint8_t(0x40));
          break;
        case CPU_OP_SET_B:
          reg_SR = (reg_SR & uint8_t(0xEF)) | (reg_TMP & uint8_t(0x10));
          break;
        case CPU_OP_SET_D:
          reg_SR = (reg_SR & uint8_t(0xF7)) | (reg_TMP & uint8_t(0x08));
          break;
        case CPU_OP_SET_I:
          reg_SR = (reg_SR & uint8_t(0xFB)) | (reg_TMP & uint8_t(0x04));
          break;
        case CPU_OP_SET_Z:
          reg_SR = (reg_SR & uint8_t(0xFD)) | (reg_TMP & uint8_t(0x02));
          break;
        case CPU_OP_SET_C:
          reg_SR = (reg_SR & uint8_t(0xFE)) | (reg_TMP & uint8_t(0x01));
          break;
        case CPU_OP_SET_NZ:
          reg_SR = (reg_SR & uint8_t(0x7D)) | (reg_TMP & uint8_t(0x80));
          reg_SR = reg_SR | (reg_TMP == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_ADC:
          {
            unsigned int  result = (unsigned int) (reg_SR & uint8_t(0x01));
            if (reg_SR & (uint8_t) 0x08) {
              // add in BCD mode
              result += (unsigned int) (reg_AC & uint8_t(0x0F));
              result += (unsigned int) (reg_TMP & uint8_t(0x0F));
              result += (result < 0x0AU ? 0x00U : 0x06U);
              result += (unsigned int) (reg_AC & uint8_t(0xF0));
              result += (unsigned int) (reg_TMP & uint8_t(0xF0));
              result += (result < 0xA0U ? 0x00U : 0x60U);
            }
            else {
              // add in binary mode
              result += ((unsigned int) reg_AC + (unsigned int) reg_TMP);
            }
            reg_SR &= uint8_t(0x3C);
            // carry
            reg_SR |= (result >= 0x0100U ? uint8_t(0x01) : uint8_t(0x00));
            // overflow
            reg_SR |= ((((reg_AC ^ ~reg_TMP) & (reg_AC ^ uint8_t(result)))
                        & uint8_t(0x80)) >> 1);
            reg_AC = uint8_t(result);
          }
          reg_SR = reg_SR | (reg_AC & uint8_t(0x80));
          reg_SR = reg_SR | (reg_AC == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_AND:
          reg_AC = reg_AC & reg_TMP;
          reg_SR = (reg_SR & uint8_t(0x7D)) | (reg_AC & uint8_t(0x80));
          reg_SR = reg_SR | (reg_AC == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_ASL:
          {
            uint16_t  tmp = uint16_t(reg_TMP);
            tmp = tmp << 1;
            reg_SR = (reg_SR & uint8_t(0x7C)) | uint8_t(tmp >> 8);
            reg_TMP = uint8_t(tmp) & uint8_t(0xFF);
          }
          reg_SR = reg_SR | (reg_TMP & uint8_t(0x80));
          reg_SR = reg_SR | (reg_TMP == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_BIT:
          reg_SR = (reg_SR & uint8_t(0x3D)) | (reg_TMP & uint8_t(0xC0));
          reg_SR = reg_SR | ((reg_AC & reg_TMP) == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_BRANCH:
          if (reg_TMP == uint8_t(0))
            currentOpcode++;
          else {
            doneCycle = true;
            if (haltRequestFlag) {
              haltFlag = true;
              currentOpcode--;
            }
          }
          break;
        case CPU_OP_BRK:
          interruptFlag = false;
          reg_TMP = reg_SR | uint8_t(0x10);
          reg_SR = reg_SR | uint8_t(0x34);
          reg_L = uint8_t(0xFE);
          reg_H = uint8_t(0xFF);
          break;
        case CPU_OP_CMP:
          {
            uint16_t  tmp = uint16_t(reg_AC)
                            + uint16_t(reg_TMP ^ uint8_t(0xFF));
            tmp++;
            reg_SR = reg_SR & uint8_t(0x7C);
            reg_SR = reg_SR | (uint8_t(tmp) & uint8_t(0x80));
            reg_SR = reg_SR | (uint8_t(tmp >> 8) & uint8_t(0x01));
            tmp = tmp & uint16_t(0xFF);
            reg_SR = reg_SR | (tmp == uint16_t(0) ?
                               uint8_t(0x02) : uint8_t(0x00));
          }
          break;
        case CPU_OP_CPX:
          {
            uint16_t  tmp = uint16_t(reg_XR)
                            + uint16_t(reg_TMP ^ uint8_t(0xFF));
            tmp++;
            reg_SR = reg_SR & uint8_t(0x7C);
            reg_SR = reg_SR | (uint8_t(tmp) & uint8_t(0x80));
            reg_SR = reg_SR | (uint8_t(tmp >> 8) & uint8_t(0x01));
            tmp = tmp & uint16_t(0xFF);
            reg_SR = reg_SR | (tmp == uint16_t(0) ?
                               uint8_t(0x02) : uint8_t(0x00));
          }
          break;
        case CPU_OP_CPY:
          {
            uint16_t  tmp = uint16_t(reg_YR)
                            + uint16_t(reg_TMP ^ uint8_t(0xFF));
            tmp++;
            reg_SR = reg_SR & uint8_t(0x7C);
            reg_SR = reg_SR | (uint8_t(tmp) & uint8_t(0x80));
            reg_SR = reg_SR | (uint8_t(tmp >> 8) & uint8_t(0x01));
            tmp = tmp & uint16_t(0xFF);
            reg_SR = reg_SR | (tmp == uint16_t(0) ?
                               uint8_t(0x02) : uint8_t(0x00));
          }
          break;
        case CPU_OP_DEC:
          reg_TMP = (reg_TMP - uint8_t(1)) & uint8_t(0xFF);
          reg_SR = (reg_SR & uint8_t(0x7D)) | (reg_TMP & uint8_t(0x80));
          reg_SR = reg_SR | (reg_TMP == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_DEC_HL:
          reg_H = reg_H - (reg_L == uint8_t(0) ? uint8_t(1) : uint8_t(0));
          reg_H = reg_H & uint8_t(0xFF);
          reg_L = (reg_L - uint8_t(1)) & uint8_t(0xFF);
          break;
        case CPU_OP_EOR:
          reg_AC = reg_AC ^ reg_TMP;
          reg_SR = (reg_SR & uint8_t(0x7D)) | (reg_AC & uint8_t(0x80));
          reg_SR = reg_SR | (reg_AC == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_INC:
          reg_TMP = (reg_TMP + uint8_t(1)) & uint8_t(0xFF);
          reg_SR = (reg_SR & uint8_t(0x7D)) | (reg_TMP & uint8_t(0x80));
          reg_SR = reg_SR | (reg_TMP == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_INC_L:
          reg_L = (reg_L + uint8_t(1)) & uint8_t(0xFF);
          break;
        case CPU_OP_INTERRUPT:
          interruptFlag = false;
          reg_TMP = reg_SR & uint8_t(0xEF);
          reg_SR = reg_SR | uint8_t(0x34);
          reg_L = uint8_t(0xFE);
          reg_H = uint8_t(0xFF);
          break;
        case CPU_OP_JMP_RELATIVE:
          {
            uint8_t tmp = (uint8_t(reg_PC) + reg_L) & uint8_t(0xFF);
            if ((((tmp ^ uint8_t(reg_PC)) & (reg_L ^ uint8_t(reg_PC)))
                 & uint8_t(0x80)) != uint8_t(0)) {
              doneCycle = true;
              if (haltRequestFlag) {
                haltFlag = true;
                currentOpcode--;
                break;
              }
              reg_PC = reg_PC + ((reg_L & uint8_t(0x80)) == uint8_t(0) ?
                                 uint16_t(0x0100) : uint16_t(0xFF00));
            }
            reg_PC = (reg_PC & uint16_t(0xFF00)) | uint16_t(tmp);
          }
          break;
        case CPU_OP_LSR:
          {
            uint16_t  tmp = uint16_t(reg_TMP);
            reg_SR = (reg_SR & uint8_t(0x7C)) | (uint8_t(tmp) & uint8_t(0x01));
            reg_TMP = uint8_t(tmp >> 1);
          }
          reg_SR = reg_SR | (reg_TMP == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_ORA:
          reg_AC = reg_AC | reg_TMP;
          reg_SR = (reg_SR & uint8_t(0x7D)) | (reg_AC & uint8_t(0x80));
          reg_SR = reg_SR | (reg_AC == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_RESET:
          interruptFlag = false;
          resetFlag = false;
          reg_TMP = reg_SR & uint8_t(0xEF);
          reg_SR = reg_SR | uint8_t(0x34);
          reg_L = uint8_t(0xFC);
          reg_H = uint8_t(0xFF);
          break;
        case CPU_OP_ROL:
          {
            uint16_t  tmp = uint16_t(reg_TMP);
            tmp = (tmp << 1) | uint16_t(reg_SR & uint8_t(0x01));
            reg_SR = (reg_SR & uint8_t(0x7C)) | uint8_t(tmp >> 8);
            reg_TMP = uint8_t(tmp) & uint8_t(0xFF);
          }
          reg_SR = reg_SR | (reg_TMP & uint8_t(0x80));
          reg_SR = reg_SR | (reg_TMP == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_ROR:
          {
            uint16_t  tmp = uint16_t(reg_TMP);
            tmp = tmp | (uint16_t(reg_SR & uint8_t(0x01)) << 8);
            reg_SR = (reg_SR & uint8_t(0x7C)) | (uint8_t(tmp) & uint8_t(0x01));
            reg_TMP = uint8_t(tmp >> 1);
          }
          reg_SR = reg_SR | (reg_TMP & uint8_t(0x80));
          reg_SR = reg_SR | (reg_TMP == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_SAX:
          reg_TMP = reg_AC & reg_XR;
          break;
        case CPU_OP_SBC:
          {
            unsigned int  result = (unsigned int) (reg_SR & uint8_t(0x01));
            reg_TMP ^= uint8_t(0xFF);
            if (reg_SR & uint8_t(0x08)) {
              // subtract in BCD mode
              result += (unsigned int) (reg_AC & uint8_t(0x0F));
              result += (unsigned int) (reg_TMP & uint8_t(0x0F));
              if (result < 0x10U)
                result = (result + 0x0AU) & 0x0FU;
              result += (unsigned int) (reg_AC & uint8_t(0xF0));
              result += (unsigned int) (reg_TMP & uint8_t(0xF0));
              if (result < 0x0100U)
                result = (result + 0xA0U) & 0xFFU;
            }
            else {
              // subtract in binary mode
              result += ((unsigned int) reg_AC + (unsigned int) reg_TMP);
            }
            reg_SR &= uint8_t(0x3C);
            // carry
            reg_SR |= (result >= 0x0100U ? uint8_t(0x01) : uint8_t(0x00));
            // overflow
            reg_SR |= ((((reg_AC ^ ~reg_TMP) & (reg_AC ^ uint8_t(result)))
                        & uint8_t(0x80)) >> 1);
            reg_AC = uint8_t(result);
          }
          reg_SR = reg_SR | (reg_AC & uint8_t(0x80));
          reg_SR = reg_SR | (reg_AC == uint8_t(0) ?
                             uint8_t(0x02) : uint8_t(0x00));
          break;
        case CPU_OP_INVALID_OPCODE:
          doneCycle = true;
          if (!haltRequestFlag) {
            reg_L = uint8_t(0x3E);
            reg_H = uint8_t(0xFF);
          }
          else {
            haltFlag = true;
            currentOpcode--;
          }
          break;
        }
      } while (!doneCycle);
    } while (--nCycles > 0);
  }

}       // namespace Plus4

