
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

#ifndef EP128EMU_PLUS4_CPU_HPP
#define EP128EMU_PLUS4_CPU_HPP

#include "ep128emu.hpp"

namespace Plus4 {

  class M7501 {
   private:
    static const unsigned char  CPU_OP_RD_OPCODE            =  0;
    static const unsigned char  CPU_OP_RD_TMP               =  1;
    static const unsigned char  CPU_OP_RD_L                 =  2;
    static const unsigned char  CPU_OP_RD_H                 =  3;
    static const unsigned char  CPU_OP_WAIT                 =  4;
    static const unsigned char  CPU_OP_LD_TMP_MEM           =  5;
    static const unsigned char  CPU_OP_LD_MEM_TMP           =  6;
    static const unsigned char  CPU_OP_LD_H_MEM             =  7;
    static const unsigned char  CPU_OP_PUSH_TMP             =  8;
    static const unsigned char  CPU_OP_POP_TMP              =  9;
    static const unsigned char  CPU_OP_PUSH_PCL             = 10;
    static const unsigned char  CPU_OP_POP_PCL              = 11;
    static const unsigned char  CPU_OP_PUSH_PCH             = 12;
    static const unsigned char  CPU_OP_POP_PCH              = 13;
    static const unsigned char  CPU_OP_LD_TMP_00            = 14;
    static const unsigned char  CPU_OP_LD_TMP_FF            = 15;
    static const unsigned char  CPU_OP_LD_HL_PC             = 16;
    static const unsigned char  CPU_OP_LD_TMP_L             = 17;
    static const unsigned char  CPU_OP_LD_TMP_H             = 18;
    static const unsigned char  CPU_OP_LD_TMP_SR            = 19;
    static const unsigned char  CPU_OP_LD_TMP_A             = 20;
    static const unsigned char  CPU_OP_LD_TMP_X             = 21;
    static const unsigned char  CPU_OP_LD_TMP_Y             = 22;
    static const unsigned char  CPU_OP_LD_TMP_SP            = 23;
    static const unsigned char  CPU_OP_LD_PC_HL             = 24;
    static const unsigned char  CPU_OP_LD_L_TMP             = 25;
    static const unsigned char  CPU_OP_LD_H_TMP             = 26;
    static const unsigned char  CPU_OP_LD_SR_TMP            = 27;
    static const unsigned char  CPU_OP_LD_A_TMP             = 28;
    static const unsigned char  CPU_OP_LD_X_TMP             = 29;
    static const unsigned char  CPU_OP_LD_Y_TMP             = 30;
    static const unsigned char  CPU_OP_LD_SP_TMP            = 31;
    static const unsigned char  CPU_OP_ADDR_ZEROPAGE        = 32;
    static const unsigned char  CPU_OP_ADDR_X               = 33;
    static const unsigned char  CPU_OP_ADDR_X_SLOW          = 34;
    static const unsigned char  CPU_OP_ADDR_Y               = 35;
    static const unsigned char  CPU_OP_ADDR_Y_SLOW          = 36;
    static const unsigned char  CPU_OP_TEST_N               = 37;
    static const unsigned char  CPU_OP_TEST_V               = 38;
    static const unsigned char  CPU_OP_TEST_Z               = 39;
    static const unsigned char  CPU_OP_TEST_C               = 40;
    static const unsigned char  CPU_OP_SET_N                = 41;
    static const unsigned char  CPU_OP_SET_V                = 42;
    static const unsigned char  CPU_OP_SET_B                = 43;
    static const unsigned char  CPU_OP_SET_D                = 44;
    static const unsigned char  CPU_OP_SET_I                = 45;
    static const unsigned char  CPU_OP_SET_Z                = 46;
    static const unsigned char  CPU_OP_SET_C                = 47;
    static const unsigned char  CPU_OP_SET_NZ               = 48;
    static const unsigned char  CPU_OP_ADC                  = 49;
    static const unsigned char  CPU_OP_AND                  = 50;
    static const unsigned char  CPU_OP_ASL                  = 51;
    static const unsigned char  CPU_OP_BIT                  = 52;
    static const unsigned char  CPU_OP_BRANCH               = 53;
    static const unsigned char  CPU_OP_BRK                  = 54;
    static const unsigned char  CPU_OP_CMP                  = 55;
    static const unsigned char  CPU_OP_CPX                  = 56;
    static const unsigned char  CPU_OP_CPY                  = 57;
    static const unsigned char  CPU_OP_DEC                  = 58;
    static const unsigned char  CPU_OP_DEC_HL               = 59;
    static const unsigned char  CPU_OP_EOR                  = 60;
    static const unsigned char  CPU_OP_INC                  = 61;
    static const unsigned char  CPU_OP_INC_L                = 62;
    static const unsigned char  CPU_OP_INTERRUPT            = 63;
    static const unsigned char  CPU_OP_JMP_RELATIVE         = 64;
    static const unsigned char  CPU_OP_LSR                  = 65;
    static const unsigned char  CPU_OP_ORA                  = 66;
    static const unsigned char  CPU_OP_RESET                = 67;
    static const unsigned char  CPU_OP_ROL                  = 68;
    static const unsigned char  CPU_OP_ROR                  = 69;
    static const unsigned char  CPU_OP_SAX                  = 70;
    static const unsigned char  CPU_OP_SBC                  = 71;
    static const unsigned char  CPU_OP_INVALID_OPCODE       = 72;
    static const unsigned char  opcodeTable[4128];
    const unsigned char *currentOpcode;
    unsigned int  interruptDelayRegister;
    bool        interruptFlag;
    bool        resetFlag;
    bool        haltRequestFlag;
   protected:
    bool        haltFlag;
   private:
    uint8_t     reg_TMP;
    uint8_t     reg_L;
    uint8_t     reg_H;
    typedef uint8_t (*MemoryReadFunc)(void *userData, uint16_t addr);
    typedef void (*MemoryWriteFunc)(void *userData,
                                    uint16_t addr, uint8_t value);
    MemoryReadFunc  *memoryReadCallbacks;
    MemoryWriteFunc *memoryWriteCallbacks;
    void        *memoryCallbackUserData;
   protected:
    uint16_t    reg_PC;
    uint8_t     reg_SR;
    uint8_t     reg_AC;
    uint8_t     reg_XR;
    uint8_t     reg_YR;
    uint8_t     reg_SP;
    inline uint8_t readMemory(uint16_t addr)
    {
      return (memoryReadCallbacks[addr](memoryCallbackUserData, addr));
    }
    inline void writeMemory(uint16_t addr, uint8_t value)
    {
      memoryWriteCallbacks[addr](memoryCallbackUserData, addr, value);
    }
   public:
    M7501();
    virtual ~M7501();
    inline void setMemoryReadCallback(uint16_t addr_,
                                      uint8_t (*func)(void *userData,
                                                      uint16_t addr))
    {
      memoryReadCallbacks[addr_] = func;
    }
    inline void setMemoryWriteCallback(uint16_t addr_,
                                       void (*func)(void *userData,
                                                    uint16_t addr,
                                                    uint8_t value))
    {
      memoryWriteCallbacks[addr_] = func;
    }
    inline void setMemoryCallbackUserData(void *userData)
    {
      memoryCallbackUserData = userData;
    }
    void run(int nCycles = 1);
    inline void interruptRequest()
    {
      interruptDelayRegister |= 4U;     // delay interrupt requests by 2 cycles
    }
    inline void reset(bool isColdReset = false)
    {
      (void) isColdReset;
      resetFlag = true;
    }
    inline void setIsCPURunning(bool n)
    {
      if (n) {
        haltRequestFlag = false;
        haltFlag = false;
      }
      else
        haltRequestFlag = true;
    }
  };

}       // namespace Plus4

#endif  // EP128EMU_PLUS4_CPU_HPP

