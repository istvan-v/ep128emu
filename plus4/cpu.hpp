
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

#ifndef EP128EMU_PLUS4_CPU_HPP
#define EP128EMU_PLUS4_CPU_HPP

#include "ep128emu.hpp"
#include "bplist.hpp"

namespace Plus4 {

  class M7501Registers {
   public:
    uint16_t  reg_PC;
    uint8_t   reg_SR;
    uint8_t   reg_AC;
    uint8_t   reg_XR;
    uint8_t   reg_YR;
    uint8_t   reg_SP;
    M7501Registers()
      : reg_PC(0x0000),
        reg_SR(uint8_t(0x24)),
        reg_AC(0x00),
        reg_XR(0x00),
        reg_YR(0x00),
        reg_SP(uint8_t(0xFF))
    {
    }
  };

  class M7501 : protected M7501Registers {
   private:
    static const unsigned char  CPU_OP_RD_OPCODE            =  0;
    static const unsigned char  CPU_OP_RD_TMP               =  1;
    static const unsigned char  CPU_OP_RD_TMP_NODEBUG       =  2;
    static const unsigned char  CPU_OP_RD_L                 =  3;
    static const unsigned char  CPU_OP_RD_H                 =  4;
    static const unsigned char  CPU_OP_WAIT                 =  5;
    static const unsigned char  CPU_OP_LD_TMP_MEM           =  6;
    static const unsigned char  CPU_OP_LD_TMP_MEM_NODEBUG   =  7;
    static const unsigned char  CPU_OP_LD_MEM_TMP           =  8;
    static const unsigned char  CPU_OP_LD_MEM_TMP_NODEBUG   =  9;
    static const unsigned char  CPU_OP_LD_H_MEM             = 10;
    static const unsigned char  CPU_OP_PUSH_TMP             = 11;
    static const unsigned char  CPU_OP_POP_TMP              = 12;
    static const unsigned char  CPU_OP_PUSH_PCL             = 13;
    static const unsigned char  CPU_OP_POP_PCL              = 14;
    static const unsigned char  CPU_OP_PUSH_PCH             = 15;
    static const unsigned char  CPU_OP_POP_PCH              = 16;
    static const unsigned char  CPU_OP_LD_TMP_00            = 17;
    static const unsigned char  CPU_OP_LD_TMP_FF            = 18;
    static const unsigned char  CPU_OP_LD_HL_PC             = 19;
    static const unsigned char  CPU_OP_LD_TMP_L             = 20;
    static const unsigned char  CPU_OP_LD_TMP_H             = 21;
    static const unsigned char  CPU_OP_LD_TMP_SR            = 22;
    static const unsigned char  CPU_OP_LD_TMP_A             = 23;
    static const unsigned char  CPU_OP_LD_TMP_X             = 24;
    static const unsigned char  CPU_OP_LD_TMP_Y             = 25;
    static const unsigned char  CPU_OP_LD_TMP_SP            = 26;
    static const unsigned char  CPU_OP_LD_PC_HL             = 27;
    static const unsigned char  CPU_OP_LD_L_TMP             = 28;
    static const unsigned char  CPU_OP_LD_H_TMP             = 29;
    static const unsigned char  CPU_OP_LD_SR_TMP            = 30;
    static const unsigned char  CPU_OP_LD_A_TMP             = 31;
    static const unsigned char  CPU_OP_LD_X_TMP             = 32;
    static const unsigned char  CPU_OP_LD_Y_TMP             = 33;
    static const unsigned char  CPU_OP_LD_SP_TMP            = 34;
    static const unsigned char  CPU_OP_ADDR_X               = 35;
    static const unsigned char  CPU_OP_ADDR_X_SLOW          = 36;
    static const unsigned char  CPU_OP_ADDR_X_ZEROPAGE      = 37;
    static const unsigned char  CPU_OP_ADDR_X_SHY           = 38;
    static const unsigned char  CPU_OP_ADDR_Y               = 39;
    static const unsigned char  CPU_OP_ADDR_Y_SLOW          = 40;
    static const unsigned char  CPU_OP_ADDR_Y_ZEROPAGE      = 41;
    static const unsigned char  CPU_OP_ADDR_Y_SHA           = 42;
    static const unsigned char  CPU_OP_ADDR_Y_SHS           = 43;
    static const unsigned char  CPU_OP_ADDR_Y_SHX           = 44;
    static const unsigned char  CPU_OP_TEST_N               = 45;
    static const unsigned char  CPU_OP_TEST_V               = 46;
    static const unsigned char  CPU_OP_TEST_Z               = 47;
    static const unsigned char  CPU_OP_TEST_C               = 48;
    static const unsigned char  CPU_OP_SET_N                = 49;
    static const unsigned char  CPU_OP_SET_V                = 50;
    static const unsigned char  CPU_OP_SET_B                = 51;
    static const unsigned char  CPU_OP_SET_D                = 52;
    static const unsigned char  CPU_OP_SET_I                = 53;
    static const unsigned char  CPU_OP_SET_Z                = 54;
    static const unsigned char  CPU_OP_SET_C                = 55;
    static const unsigned char  CPU_OP_SET_NZ               = 56;
    static const unsigned char  CPU_OP_ADC                  = 57;
    static const unsigned char  CPU_OP_ANC                  = 58;
    static const unsigned char  CPU_OP_AND                  = 59;
    static const unsigned char  CPU_OP_ANE                  = 60;
    static const unsigned char  CPU_OP_ARR                  = 61;
    static const unsigned char  CPU_OP_ASL                  = 62;
    static const unsigned char  CPU_OP_BIT                  = 63;
    static const unsigned char  CPU_OP_BRANCH               = 64;
    static const unsigned char  CPU_OP_BRK                  = 65;
    static const unsigned char  CPU_OP_CMP                  = 66;
    static const unsigned char  CPU_OP_CPX                  = 67;
    static const unsigned char  CPU_OP_CPY                  = 68;
    static const unsigned char  CPU_OP_DEC                  = 69;
    static const unsigned char  CPU_OP_DEC_HL               = 70;
    static const unsigned char  CPU_OP_EOR                  = 71;
    static const unsigned char  CPU_OP_INC                  = 72;
    static const unsigned char  CPU_OP_INC_L                = 73;
    static const unsigned char  CPU_OP_INTERRUPT            = 74;
    static const unsigned char  CPU_OP_JMP_RELATIVE         = 75;
    static const unsigned char  CPU_OP_LAS                  = 76;
    static const unsigned char  CPU_OP_LSR                  = 77;
    static const unsigned char  CPU_OP_LXA                  = 78;
    static const unsigned char  CPU_OP_ORA                  = 79;
    static const unsigned char  CPU_OP_RESET                = 80;
    static const unsigned char  CPU_OP_ROL                  = 81;
    static const unsigned char  CPU_OP_ROR                  = 82;
    static const unsigned char  CPU_OP_SAX                  = 83;
    static const unsigned char  CPU_OP_SBC                  = 84;
    static const unsigned char  CPU_OP_SBX                  = 85;
    static const unsigned char  CPU_OP_SYS                  = 86;
    static const unsigned char  CPU_OP_INVALID_OPCODE       = 87;
    static const unsigned char  opcodeTable[4128];
    const unsigned char *currentOpcode;
    unsigned int  interruptDelayRegister;
    bool        interruptFlag;
    bool        resetFlag;
    bool        haltFlag;
    uint8_t     reg_TMP;
    uint8_t     reg_L;
    uint8_t     reg_H;
    typedef uint8_t (*MemoryReadFunc)(void *userData, uint16_t addr);
    typedef void (*MemoryWriteFunc)(void *userData,
                                    uint16_t addr, uint8_t value);
    MemoryReadFunc  *memoryReadCallbacks;
    MemoryWriteFunc *memoryWriteCallbacks;
    void        *memoryCallbackUserData;
    uint8_t     *breakPointTable;
    size_t      breakPointCnt;
    bool        singleStepModeEnabled;
    bool        haveBreakPoints;
    uint8_t     breakPointPriorityThreshold;
    void checkReadBreakPoint(uint16_t addr, uint8_t value);
    void checkWriteBreakPoint(uint16_t addr, uint8_t value);
   protected:
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
    virtual void reset(bool isColdReset = false);
    inline void setIsCPURunning(bool n)
    {
      haltFlag = !n;
    }
    inline const M7501Registers& getRegisters() const
    {
      return *(static_cast<const M7501Registers *>(this));
    }
    void setBreakPoint(uint16_t addr, int priority, bool r, bool w);
    void clearBreakPoints();
    void setBreakPointPriorityThreshold(int n);
    int getBreakPointPriorityThreshold() const;
    Ep128Emu::BreakPointList getBreakPointList();
    void setSingleStepMode(bool isEnabled);
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
   protected:
    virtual bool systemCallback(uint8_t n);
    virtual void breakPointCallback(bool isWrite, uint16_t addr, uint8_t value);
  };

}       // namespace Plus4

#endif  // EP128EMU_PLUS4_CPU_HPP

