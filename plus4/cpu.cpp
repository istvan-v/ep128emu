
// plus4 -- portable Commodore PLUS/4 emulator
// Copyright (C) 2003-2007 Istvan Varga <istvanv@users.sourceforge.net>
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
    : M7501Registers(),
      currentOpcode(&(opcodeTable[0x0FFF])),
      interruptDelayRegister(0U),
      interruptFlag(false),
      resetFlag(true),
      haltFlag(false),
      reg_TMP(0),
      reg_L(0),
      reg_H(0),
      memoryReadCallbacks((MemoryReadFunc *) 0),
      memoryWriteCallbacks((MemoryWriteFunc *) 0),
      memoryCallbackUserData((void *) 0),
      breakPointTable((uint8_t *) 0),
      breakPointCnt(0),
      singleStepModeEnabled(false),
      haveBreakPoints(false),
      breakPointPriorityThreshold(0)
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
    if (breakPointTable)
      delete[] breakPointTable;
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
      bool    doneCycle = false;
      do {
        unsigned char n = *(currentOpcode++);
        switch (n) {
        case CPU_OP_RD_OPCODE:
          if (!(interruptFlag | resetFlag)) {
            doneCycle = true;
            uint8_t opNum = readMemory(reg_PC);
            if (haltFlag) {
              currentOpcode--;
              break;
            }
            if (singleStepModeEnabled)
              breakPointCallback(false, reg_PC, opNum);
            else if (haveBreakPoints)
              checkReadBreakPoint(reg_PC, opNum);
            reg_PC = (reg_PC + 1) & 0xFFFF;
            currentOpcode = &(opcodeTable[size_t(opNum) << 4]);
          }
          else if (resetFlag)
            currentOpcode = &(opcodeTable[size_t(0x101) << 4]);
          else
            currentOpcode = &(opcodeTable[size_t(0x100) << 4]);
          break;
        case CPU_OP_RD_TMP:
          doneCycle = true;
          reg_TMP = readMemory(reg_PC);
          if (haltFlag) {
            currentOpcode--;
            break;
          }
          if (haveBreakPoints)
            checkReadBreakPoint(reg_PC, reg_TMP);
          reg_PC = (reg_PC + 1) & 0xFFFF;
          break;
        case CPU_OP_RD_TMP_NODEBUG:
          doneCycle = true;
          reg_TMP = readMemory(reg_PC);
          if (haltFlag) {
            currentOpcode--;
            break;
          }
          reg_PC = (reg_PC + 1) & 0xFFFF;
          break;
        case CPU_OP_RD_L:
          doneCycle = true;
          reg_L = readMemory(reg_PC);
          if (haltFlag) {
            currentOpcode--;
            break;
          }
          if (haveBreakPoints)
            checkReadBreakPoint(reg_PC, reg_L);
          reg_PC = (reg_PC + 1) & 0xFFFF;
          break;
        case CPU_OP_RD_H:
          doneCycle = true;
          reg_H = readMemory(reg_PC);
          if (haltFlag) {
            currentOpcode--;
            break;
          }
          if (haveBreakPoints)
            checkReadBreakPoint(reg_PC, reg_H);
          reg_PC = (reg_PC + 1) & 0xFFFF;
          break;
        case CPU_OP_WAIT:
          doneCycle = true;
          if (haltFlag)
            currentOpcode--;
          break;
        case CPU_OP_LD_TMP_MEM:
          doneCycle = true;
          {
            uint16_t  addr = uint16_t(reg_L) | (uint16_t(reg_H) << 8);
            reg_TMP = readMemory(addr);
            if (haltFlag)
              currentOpcode--;
            else if (haveBreakPoints)
              checkReadBreakPoint(addr, reg_TMP);
          }
          break;
        case CPU_OP_LD_TMP_MEM_NODEBUG:
          doneCycle = true;
          reg_TMP = readMemory(uint16_t(reg_L) | (uint16_t(reg_H) << 8));
          if (haltFlag)
            currentOpcode--;
          break;
        case CPU_OP_LD_MEM_TMP:
          {
            uint16_t  addr = uint16_t(reg_L) | (uint16_t(reg_H) << 8);
            if (haveBreakPoints)
              checkWriteBreakPoint(addr, reg_TMP);
            writeMemory(addr, reg_TMP);
            doneCycle = true;
          }
          break;
        case CPU_OP_LD_MEM_TMP_NODEBUG:
          writeMemory(uint16_t(reg_L) | (uint16_t(reg_H) << 8), reg_TMP);
          doneCycle = true;
          break;
        case CPU_OP_LD_H_MEM:
          doneCycle = true;
          {
            uint16_t  addr = uint16_t(reg_L) | (uint16_t(reg_H) << 8);
            uint8_t   tmp = readMemory(addr);
            if (haltFlag) {
              currentOpcode--;
              break;
            }
            reg_H = tmp;
            if (haveBreakPoints)
              checkReadBreakPoint(addr, reg_H);
          }
          break;
        case CPU_OP_PUSH_TMP:
          // FIXME: should check breakpoints ?
          writeMemory(uint16_t(0x0100) | uint16_t(reg_SP), reg_TMP);
          reg_SP = (reg_SP - uint8_t(1)) & uint8_t(0xFF);
          doneCycle = true;
          break;
        case CPU_OP_POP_TMP:
          // FIXME: should check breakpoints ?
          doneCycle = true;
          if (!haltFlag) {
            reg_SP = (reg_SP + uint8_t(1)) & uint8_t(0xFF);
            reg_TMP = readMemory(uint16_t(0x0100) | uint16_t(reg_SP));
          }
          else
            currentOpcode--;
          break;
        case CPU_OP_PUSH_PCL:
          // FIXME: should check breakpoints ?
          writeMemory(uint16_t(0x0100) | uint16_t(reg_SP),
                      uint8_t(reg_PC) & uint8_t(0xFF));
          reg_SP = (reg_SP - uint8_t(1)) & uint8_t(0xFF);
          doneCycle = true;
          break;
        case CPU_OP_POP_PCL:
          // FIXME: should check breakpoints ?
          doneCycle = true;
          if (!haltFlag) {
            reg_SP = (reg_SP + uint8_t(1)) & uint8_t(0xFF);
            reg_PC = (reg_PC & uint16_t(0xFF00))
                     | uint16_t(readMemory(uint16_t(0x0100)
                                           | uint16_t(reg_SP)));
          }
          else
            currentOpcode--;
          break;
        case CPU_OP_PUSH_PCH:
          // FIXME: should check breakpoints ?
          writeMemory(uint16_t(0x0100) | uint16_t(reg_SP),
                      uint8_t(reg_PC >> 8) & uint8_t(0xFF));
          reg_SP = (reg_SP - uint8_t(1)) & uint8_t(0xFF);
          doneCycle = true;
          break;
        case CPU_OP_POP_PCH:
          // FIXME: should check breakpoints ?
          doneCycle = true;
          if (!haltFlag) {
            reg_SP = (reg_SP + uint8_t(1)) & uint8_t(0xFF);
            reg_PC = (reg_PC & uint16_t(0x00FF))
                     | (uint16_t(readMemory(uint16_t(0x0100)
                                            | uint16_t(reg_SP))) << 8);
          }
          else
            currentOpcode--;
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
              if (haltFlag) {
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
          if (!haltFlag) {
            reg_L = (reg_L + reg_XR) & uint8_t(0xFF);
            if (reg_L < reg_XR)
              reg_H = (reg_H + uint8_t(1)) & uint8_t(0xFF);
          }
          else
            currentOpcode--;
          break;
        case CPU_OP_ADDR_Y:
          {
            uint8_t tmp = (reg_L + reg_YR) & uint8_t(0xFF);
            if (tmp < reg_L) {
              doneCycle = true;
              if (haltFlag) {
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
          if (!haltFlag) {
            reg_L = (reg_L + reg_YR) & uint8_t(0xFF);
            if (reg_L < reg_YR)
              reg_H = (reg_H + uint8_t(1)) & uint8_t(0xFF);
          }
          else
            currentOpcode--;
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
            if (haltFlag)
              currentOpcode--;
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
              if (haltFlag) {
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
        case CPU_OP_SYS:
          doneCycle = true;
          if (!haltFlag) {
            uint32_t  nn = uint32_t(currentOpcode - &(opcodeTable[0])) >> 4;
            uint16_t  addr = (reg_PC + 0xFFFC) & 0xFFFF;
            nn = (nn << 8) | uint32_t(reg_TMP);
            nn = (nn << 8) | uint32_t(addr >> 8);
            nn = (nn << 8) | uint32_t(addr & 0xFF);
            uint64_t  tmp = nn * uint64_t(0xC2B0C3CCU);
            nn = (uint32_t(tmp) ^ uint32_t(tmp >> 32)) & 0xFFFFU;
            nn ^= uint32_t(reg_L);
            nn ^= (uint32_t(reg_H) << 8);
            if (!nn) {
              if (!this->systemCallback(reg_TMP))
                nn++;
            }
            if (nn)
              this->reset();
          }
          else
            currentOpcode--;
          break;
        case CPU_OP_INVALID_OPCODE:
          doneCycle = true;
          if (!haltFlag)
            this->reset();
          else
            currentOpcode--;
          break;
        }
      } while (!doneCycle);
    } while (--nCycles > 0);
  }

  void M7501::reset(bool isColdReset)
  {
    resetFlag = true;
    if (isColdReset) {
      reg_SR = uint8_t(0x24);
      reg_SP = uint8_t(0xFF);
      currentOpcode = &(opcodeTable[0x0FFF]);
      interruptDelayRegister = 0U;
      interruptFlag = false;
      haltFlag = false;
    }
  }

  bool M7501::systemCallback(uint8_t n)
  {
    (void) n;
    return false;
  }

  // --------------------------------------------------------------------------

  void M7501::checkReadBreakPoint(uint16_t addr, uint8_t value)
  {
    if (!singleStepModeEnabled) {
      uint8_t *tbl = breakPointTable;
      if (tbl[addr] >= breakPointPriorityThreshold && (tbl[addr] & 1) != 0)
        breakPointCallback(false, addr, value);
    }
  }

  void M7501::checkWriteBreakPoint(uint16_t addr, uint8_t value)
  {
    if (!singleStepModeEnabled) {
      uint8_t *tbl = breakPointTable;
      if (tbl[addr] >= breakPointPriorityThreshold && (tbl[addr] & 2) != 0)
        breakPointCallback(true, addr, value);
    }
  }

  void M7501::setBreakPoint(uint16_t addr, int priority, bool r, bool w)
  {
    uint8_t mode = (r ? 1 : 0) + (w ? 2 : 0);
    if (mode) {
      // create new breakpoint, or change existing one
      mode += uint8_t((priority > 0 ? (priority < 3 ? priority : 3) : 0) << 2);
      if (!breakPointTable) {
        breakPointTable = new uint8_t[65536];
        for (int i = 0; i < 65536; i++)
          breakPointTable[i] = 0;
      }
      haveBreakPoints = true;
      uint8_t&  bp = breakPointTable[addr];
      if (!bp)
        breakPointCnt++;
      if (bp > mode)
        mode = (bp & 12) + (mode & 3);
      mode |= (bp & 3);
      bp = mode;
    }
    else if (breakPointTable) {
      if (breakPointTable[addr]) {
        // remove a previously existing breakpoint
        breakPointCnt--;
        if (!breakPointCnt)
          haveBreakPoints = false;
      }
    }
  }

  void M7501::clearBreakPoints()
  {
    for (unsigned int addr = 0; addr < 65536; addr++)
      setBreakPoint(uint16_t(addr), 0, false, false);
  }

  void M7501::setBreakPointPriorityThreshold(int n)
  {
    breakPointPriorityThreshold = uint8_t((n > 0 ? (n < 4 ? n : 4) : 0) << 2);
  }

  int M7501::getBreakPointPriorityThreshold() const
  {
    return int(breakPointPriorityThreshold >> 2);
  }

  Ep128Emu::BreakPointList M7501::getBreakPointList()
  {
    Ep128Emu::BreakPointList  bplst;
    if (breakPointTable) {
      for (size_t i = 0; i < 65536; i++) {
        uint8_t bp = breakPointTable[i];
        if (bp)
          bplst.addMemoryBreakPoint(uint16_t(i),
                                    !!(bp & 1), !!(bp & 2), bp >> 2);
      }
    }
    return bplst;
  }

  void M7501::setSingleStepMode(bool isEnabled)
  {
    singleStepModeEnabled = isEnabled;
  }

  void M7501::breakPointCallback(bool isWrite, uint16_t addr, uint8_t value)
  {
    (void) isWrite;
    (void) addr;
    (void) value;
  }

  // --------------------------------------------------------------------------

  class ChunkType_M7501Snapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    M7501&  ref;
   public:
    ChunkType_M7501Snapshot(M7501& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_M7501Snapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_M7501_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void M7501::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    buf.writeByte(uint8_t(reg_PC) & 0xFF);
    buf.writeByte(uint8_t(reg_PC >> 8));
    buf.writeByte(reg_SR);
    buf.writeByte(reg_AC);
    buf.writeByte(reg_XR);
    buf.writeByte(reg_YR);
    buf.writeByte(reg_SP);
    buf.writeByte(reg_TMP);
    buf.writeByte(reg_L);
    buf.writeByte(reg_H);
    buf.writeUInt32(uint32_t(currentOpcode - &(opcodeTable[0])));
    buf.writeUInt32(interruptDelayRegister);
    buf.writeBoolean(interruptFlag);
    buf.writeBoolean(resetFlag);
    buf.writeBoolean(haltFlag);
  }

  void M7501::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_M7501_STATE, buf);
  }

  void M7501::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible M7501 snapshot format");
    }
    try {
      // load saved state
      reg_PC = (reg_PC & uint16_t(0xFF00)) | uint16_t(buf.readByte());
      reg_PC = (reg_PC & uint16_t(0x00FF)) | (uint16_t(buf.readByte()) << 8);
      reg_SR = buf.readByte();
      reg_AC = buf.readByte();
      reg_XR = buf.readByte();
      reg_YR = buf.readByte();
      reg_SP = buf.readByte();
      reg_TMP = buf.readByte();
      reg_L = buf.readByte();
      reg_H = buf.readByte();
      uint32_t  currentOpcodeIndex = buf.readUInt32();
      interruptDelayRegister = buf.readUInt32();
      interruptFlag = buf.readBoolean();
      resetFlag = buf.readBoolean();
      haltFlag = buf.readBoolean();
      if (currentOpcodeIndex < 4128U)
        currentOpcode = &(opcodeTable[currentOpcodeIndex]);
      else
        this->reset(true);
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "M7501 snapshot data");
    }
    catch (...) {
      try {
        this->reset(true);
      }
      catch (...) {
      }
      throw;
    }
  }

  void M7501::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_M7501Snapshot *p;
    p = new ChunkType_M7501Snapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace Plus4

