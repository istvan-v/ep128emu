
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
    static const unsigned char  CPU_OP_LD_TMP_MEM           =  5;
    static const unsigned char  CPU_OP_LD_TMP_MEM_NODEBUG   =  6;
    static const unsigned char  CPU_OP_LD_MEM_TMP           =  7;
    static const unsigned char  CPU_OP_LD_MEM_TMP_NODEBUG   =  8;
    static const unsigned char  CPU_OP_LD_H_MEMP1_L_TMP     =  9;
    static const unsigned char  CPU_OP_LD_DUMMY_MEM_PC      = 10;
    static const unsigned char  CPU_OP_LD_DUMMY_MEM_SP      = 11;
    static const unsigned char  CPU_OP_PUSH_TMP             = 12;
    static const unsigned char  CPU_OP_POP_TMP              = 13;
    static const unsigned char  CPU_OP_PUSH_PCL             = 14;
    static const unsigned char  CPU_OP_POP_PCL              = 15;
    static const unsigned char  CPU_OP_PUSH_PCH             = 16;
    static const unsigned char  CPU_OP_POP_PCH              = 17;
    static const unsigned char  CPU_OP_LD_TMP_SR            = 18;
    static const unsigned char  CPU_OP_LD_TMP_A             = 19;
    static const unsigned char  CPU_OP_LD_PC_HL             = 20;
    static const unsigned char  CPU_OP_LD_SR_TMP            = 21;
    static const unsigned char  CPU_OP_LD_A_TMP             = 22;
    static const unsigned char  CPU_OP_ADDR_X               = 23;
    static const unsigned char  CPU_OP_ADDR_X_SLOW          = 24;
    static const unsigned char  CPU_OP_ADDR_X_ZEROPAGE      = 25;
    static const unsigned char  CPU_OP_ADDR_X_SHY           = 26;
    static const unsigned char  CPU_OP_ADDR_Y               = 27;
    static const unsigned char  CPU_OP_ADDR_Y_SLOW          = 28;
    static const unsigned char  CPU_OP_ADDR_Y_ZEROPAGE      = 29;
    static const unsigned char  CPU_OP_ADDR_Y_SHA           = 30;
    static const unsigned char  CPU_OP_ADDR_Y_SHS           = 31;
    static const unsigned char  CPU_OP_ADDR_Y_SHX           = 32;
    static const unsigned char  CPU_OP_SET_NZ               = 33;
    static const unsigned char  CPU_OP_ADC                  = 34;
    static const unsigned char  CPU_OP_ANC                  = 35;
    static const unsigned char  CPU_OP_AND                  = 36;
    static const unsigned char  CPU_OP_ANE                  = 37;
    static const unsigned char  CPU_OP_ARR                  = 38;
    static const unsigned char  CPU_OP_ASL                  = 39;
    static const unsigned char  CPU_OP_BCC                  = 40;
    static const unsigned char  CPU_OP_BCS                  = 41;
    static const unsigned char  CPU_OP_BEQ                  = 42;
    static const unsigned char  CPU_OP_BIT                  = 43;
    static const unsigned char  CPU_OP_BMI                  = 44;
    static const unsigned char  CPU_OP_BNE                  = 45;
    static const unsigned char  CPU_OP_BPL                  = 46;
    static const unsigned char  CPU_OP_BRK                  = 47;
    static const unsigned char  CPU_OP_BVC                  = 48;
    static const unsigned char  CPU_OP_BVS                  = 49;
    static const unsigned char  CPU_OP_CLC                  = 50;
    static const unsigned char  CPU_OP_CLD                  = 51;
    static const unsigned char  CPU_OP_CLI                  = 52;
    static const unsigned char  CPU_OP_CLV                  = 53;
    static const unsigned char  CPU_OP_CMP                  = 54;
    static const unsigned char  CPU_OP_CPX                  = 55;
    static const unsigned char  CPU_OP_CPY                  = 56;
    static const unsigned char  CPU_OP_DEC                  = 57;
    static const unsigned char  CPU_OP_DEX                  = 58;
    static const unsigned char  CPU_OP_DEY                  = 59;
    static const unsigned char  CPU_OP_EOR                  = 60;
    static const unsigned char  CPU_OP_INC                  = 61;
    static const unsigned char  CPU_OP_INTERRUPT            = 62;
    static const unsigned char  CPU_OP_INX                  = 63;
    static const unsigned char  CPU_OP_INY                  = 64;
    static const unsigned char  CPU_OP_JMP_RELATIVE         = 65;
    static const unsigned char  CPU_OP_LAS                  = 66;
    static const unsigned char  CPU_OP_LAX                  = 67;
    static const unsigned char  CPU_OP_LDA                  = 68;
    static const unsigned char  CPU_OP_LDX                  = 69;
    static const unsigned char  CPU_OP_LDY                  = 70;
    static const unsigned char  CPU_OP_LSR                  = 71;
    static const unsigned char  CPU_OP_LXA                  = 72;
    static const unsigned char  CPU_OP_ORA                  = 73;
    static const unsigned char  CPU_OP_RESET                = 74;
    static const unsigned char  CPU_OP_ROL                  = 75;
    static const unsigned char  CPU_OP_ROR                  = 76;
    static const unsigned char  CPU_OP_SAX                  = 77;
    static const unsigned char  CPU_OP_SBC                  = 78;
    static const unsigned char  CPU_OP_SBX                  = 79;
    static const unsigned char  CPU_OP_SEC                  = 80;
    static const unsigned char  CPU_OP_SED                  = 81;
    static const unsigned char  CPU_OP_SEI                  = 82;
    static const unsigned char  CPU_OP_STA                  = 83;
    static const unsigned char  CPU_OP_STX                  = 84;
    static const unsigned char  CPU_OP_STY                  = 85;
    static const unsigned char  CPU_OP_SYS                  = 86;
    static const unsigned char  CPU_OP_TAX                  = 87;
    static const unsigned char  CPU_OP_TAY                  = 88;
    static const unsigned char  CPU_OP_TSX                  = 89;
    static const unsigned char  CPU_OP_TXA                  = 90;
    static const unsigned char  CPU_OP_TXS                  = 91;
    static const unsigned char  CPU_OP_TYA                  = 92;
    static const unsigned char  CPU_OP_INVALID_OPCODE       = 93;
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
    inline void setOverflowFlag()
    {
      reg_SR |= uint8_t(0x40);
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

