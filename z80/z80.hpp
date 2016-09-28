/*
 *  ENTER emulator (c) Copyright, Kevin Thacker 1999-2001
 *
 *  This file is part of the ENTER emulator source code distribution.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 */

#ifndef __Z80_HEADER_INCLUDED__
#define __Z80_HEADER_INCLUDED__

#include "ep128emu.hpp"
#include <stddef.h>

#define Z80_ZERO_FLAG_BIT                       6
#define Z80_HALFCARRY_FLAG_BIT                  4
#define Z80_PARITY_FLAG_BIT                     2

#define Z80_SIGN_FLAG                           0x080
#define Z80_ZERO_FLAG                           0x040
#define Z80_UNUSED_FLAG1                        0x020
#define Z80_HALFCARRY_FLAG                      0x010
#define Z80_UNUSED_FLAG2                        0x008

#define Z80_PARITY_FLAG                         0x004
#define Z80_OVERFLOW_FLAG                       0x004

#define Z80_SUBTRACT_FLAG                       0x002
#define Z80_CARRY_FLAG                          0x001

#define Z80_EXECUTE_INTERRUPT_HANDLER_FLAG      0x0002
#define Z80_EXECUTING_HALT_FLAG                 0x0004
#define Z80_INTERRUPT_FLAG                      0x0008
#define Z80_NMI_FLAG                            0x0010
#define Z80_SET_PC_FLAG                         0x0020
#define Z80_FLAGS_MASK                          0x003E

#ifndef CPC_LSB_FIRST
#  if defined(__i386__) || defined(__x86_64__) || defined(WIN32)
#    define CPC_LSB_FIRST 1
#  endif
#endif

namespace Ep128 {

  /* size defines */
  typedef uint8_t   Z80_BYTE;
  typedef uint16_t  Z80_WORD;
  typedef int8_t    Z80_BYTE_OFFSET;
  typedef int16_t   Z80_WORD_OFFSET;
  typedef uint32_t  Z80_LONG;

#ifdef CPC_LSB_FIRST
  /* register pair definition */
  typedef union {
    /* read as a word */
    Z80_WORD W;
    /* read as seperate bytes, l for low, h for high bytes */
    struct {
      Z80_BYTE l;
      Z80_BYTE h;
    } B;
  } Z80_REGISTER_PAIR;
#else
  /* register pair definition */
  typedef union {
    /* read as a word */
    Z80_WORD W;

    /* read as seperate bytes, l for low, h for high bytes */
    struct {
      Z80_BYTE h;
      Z80_BYTE l;
    } B;
  } Z80_REGISTER_PAIR;
#endif

#ifdef CPC_LSB_FIRST
  typedef union {
    Z80_LONG L;

    struct {
      Z80_WORD l;
      Z80_WORD h;
    } W;

    struct {
      Z80_BYTE l;
      Z80_BYTE h;
      Z80_BYTE pad[2];
    } B;
  } Z80_REGISTER_LONG;
#else
  typedef union {
    Z80_LONG L;

    struct {
      Z80_WORD h;
      Z80_WORD l;
    } W;

    struct {
      Z80_BYTE pad[2];
      Z80_BYTE h;
      Z80_BYTE l;
    } B;
  } Z80_REGISTER_LONG;
#endif

  /* structure holds all register data */
  struct Z80_REGISTERS {
    Z80_REGISTER_LONG PC;
    Z80_REGISTER_PAIR AF;
    Z80_REGISTER_PAIR HL;
    Z80_REGISTER_PAIR SP;
    Z80_REGISTER_PAIR DE;
    Z80_REGISTER_PAIR BC;

    Z80_REGISTER_PAIR IX;
    Z80_REGISTER_PAIR IY;
    Z80_WORD IndexPlusOffset;

    Z80_REGISTER_PAIR altHL;
    Z80_REGISTER_PAIR altDE;
    Z80_REGISTER_PAIR altBC;
    Z80_REGISTER_PAIR altAF;

    /*! interrupt vector register. High byte of address */
    Z80_BYTE I;

    /*! refresh register */
    Z80_BYTE R;

    /*! interrupt status */
    Z80_BYTE IFF1;
    Z80_BYTE IFF2;

    /*! bit 7 of R register */
    Z80_BYTE RBit7;

    /*! interrupt mode 0,1,2 */
    Z80_BYTE IM;
    Z80_BYTE InterruptVectorBase;
    unsigned long Flags;
  };

#define Z80_FLAGS_REG               R.AF.B.l

#define Z80_TEST_CARRY_SET          ((Z80_FLAGS_REG & Z80_CARRY_FLAG)!=0)
#define Z80_TEST_CARRY_NOT_SET      (Z80_FLAGS_REG & Z80_CARRY_FLAG)==0
#define Z80_TEST_ZERO_SET           (Z80_FLAGS_REG & Z80_ZERO_FLAG)!=0
#define Z80_TEST_ZERO_NOT_SET       (Z80_FLAGS_REG & Z80_ZERO_FLAG)==0
#define Z80_TEST_MINUS              (Z80_FLAGS_REG & Z80_SIGN_FLAG)!=0
#define Z80_TEST_POSITIVE           (Z80_FLAGS_REG & Z80_SIGN_FLAG)==0

/* parity even. bit = 1, parity odd, bit = 0 */
#define Z80_TEST_PARITY_EVEN        ((Z80_FLAGS_REG & Z80_PARITY_FLAG)!=0)
#define Z80_TEST_PARITY_ODD         ((Z80_FLAGS_REG & Z80_PARITY_FLAG)==0)
#define Z80_KEEP_UNUSED_FLAGS       (Z80_UNUSED1_FLAG | Z80_UNUSED2_FLAG)

  struct Z80Tables {
    unsigned char zeroSignTable[256];
    unsigned char zeroSignTable2[256];
    unsigned char zeroSignParityTable[256];
    unsigned char parityTable[256];
    uint16_t      daaTable[2048];
    Z80Tables();
  };

  class Z80 {
   protected:
    static Z80Tables  t;
    Z80_REGISTERS   R;
    int32_t newPCAddress;
   private:
    EP128EMU_INLINE void Index_CB_ExecuteInstruction();
    EP128EMU_INLINE void FD_ExecuteInstruction();
    EP128EMU_INLINE void DD_ExecuteInstruction();
    EP128EMU_INLINE void ED_ExecuteInstruction();
    EP128EMU_INLINE void CB_ExecuteInstruction();
    EP128EMU_INLINE Z80_BYTE RD_BYTE_INDEX_(Z80_WORD Index);
    EP128EMU_INLINE void WR_BYTE_INDEX_(Z80_WORD Index, Z80_BYTE Data);
    EP128EMU_INLINE void LD_HL_n();
    EP128EMU_INLINE Z80_WORD POP();
    EP128EMU_INLINE void ADD_A_HL();
    EP128EMU_INLINE void ADD_A_n();
    EP128EMU_INLINE void ADC_A_HL();
    EP128EMU_INLINE void ADC_A_n();
    EP128EMU_INLINE void SUB_A_HL();
    EP128EMU_INLINE void SUB_A_n();
    EP128EMU_INLINE void SBC_A_HL();
    EP128EMU_INLINE void SBC_A_n();
    EP128EMU_INLINE void CP_A_HL();
    EP128EMU_INLINE void CP_A_n();
    EP128EMU_INLINE void AND_A_n();
    EP128EMU_INLINE void AND_A_HL();
    EP128EMU_INLINE void XOR_A_n();
    EP128EMU_INLINE void XOR_A_HL();
    EP128EMU_INLINE void OR_A_HL();
    EP128EMU_INLINE void OR_A_n();
    EP128EMU_INLINE void OUT_n_A();
    EP128EMU_INLINE void IN_A_n();
    EP128EMU_INLINE void RRA();
    EP128EMU_INLINE void RRD();
    EP128EMU_INLINE void RLD();
    EP128EMU_INLINE void JP();
    EP128EMU_INLINE void JR();
    EP128EMU_INLINE void CALL();
    EP128EMU_INLINE void DJNZ_dd();
    EP128EMU_REGPARM1 void CPI();
    EP128EMU_REGPARM1 void CPD();
    EP128EMU_REGPARM1 void OUTI();
    EP128EMU_REGPARM1 void OUTD();
    EP128EMU_REGPARM1 void INI();
    EP128EMU_REGPARM1 void IND();
    EP128EMU_REGPARM1 void DAA();
    // called after LD A,I and LD A,R to emulate the buggy behavior of P/V flag
    EP128EMU_REGPARM1 void checkNMOSBug();
   public:
    Z80();
    virtual ~Z80();
    void reset();
    const Z80_REGISTERS& getReg() const
    {
      return this->R;
    }
    Z80_REGISTERS& getReg()
    {
      return this->R;
    }
    void setProgramCounter(uint16_t addr);
    uint16_t getProgramCounter() const
    {
      if (newPCAddress >= 0)
        return uint16_t(newPCAddress);
      return uint16_t(this->R.PC.W.l);
    }
    /*!
     * Execute non-maskable interrupt immediately.
     */
    void NMI();
    /*!
     * Schedule non-maskable interrupt to be executed
     * after completing an instruction.
     */
    void NMI_();
    void triggerInterrupt();
    void clearInterrupt();
    void setVectorBase(int);
    void executeInstruction();
    /*!
     * Save snapshot.
     */
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    /*!
     * Load snapshot.
     */
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
   protected:
    /*!
     * Called when a maskable interrupt is to be executed. Subclasses should
     * call Z80::executeInterrupt(), or just return to skip the interrupt.
     */
    virtual EP128EMU_REGPARM1 void executeInterrupt();
    /*!
     * Read a byte from memory (3 cycles).
     */
    virtual EP128EMU_REGPARM2 uint8_t readMemory(uint16_t addr);
    /*!
     * Write a byte to memory (3 cycles).
     */
    virtual EP128EMU_REGPARM3 void writeMemory(uint16_t addr, uint8_t value);
    /*!
     * Read a 16-bit word from memory (6 cycles).
     */
    virtual EP128EMU_REGPARM2 uint16_t readMemoryWord(uint16_t addr);
    /*!
     * Write a 16-bit word to memory (6 cycles).
     */
    virtual EP128EMU_REGPARM3 void writeMemoryWord(uint16_t addr,
                                                   uint16_t value);
    /*!
     * Write a 16-bit word to the stack (7 cycles).
     */
    virtual EP128EMU_REGPARM2 void pushWord(uint16_t value);
    /*!
     * Write a byte to an I/O port (4 cycles).
     */
    virtual EP128EMU_REGPARM3 void doOut(uint16_t addr, uint8_t value);
    /*!
     * Read a byte from an I/O port (4 cycles).
     */
    virtual EP128EMU_REGPARM2 uint8_t doIn(uint16_t addr);
    /*!
     * Read the first byte of an opcode (4 cycles).
     */
    virtual EP128EMU_REGPARM1 uint8_t readOpcodeFirstByte();
    /*!
     * Read the second byte of an opcode (4 cycles).
     * If 'invalidOpcodeTable' is not NULL, and the opcode byte read is in the
     * table (the table element at that index is true), then the read should be
     * ignored (it takes 0 cycles and does not trigger breakpoints).
     */
    virtual EP128EMU_REGPARM2
        uint8_t readOpcodeSecondByte(const bool *invalidOpcodeTable =
                                         (bool *) 0);
    /*!
     * Read an opcode byte (3 cycles; 'Offset' should not be zero).
     */
    virtual EP128EMU_REGPARM2 uint8_t readOpcodeByte(int offset);
    /*!
     * Read an opcode word (6 cycles; 'Offset' should not be zero).
     */
    virtual EP128EMU_REGPARM2 uint16_t readOpcodeWord(int offset);
    virtual EP128EMU_REGPARM1 void updateCycle();
    virtual EP128EMU_REGPARM2 void updateCycles(int cycles);
    virtual EP128EMU_REGPARM1 void tapePatch();
   private:
    EP128EMU_INLINE void checkInterrupts()
    {
      if (EP128EMU_UNLIKELY(R.Flags & (Z80_EXECUTE_INTERRUPT_HANDLER_FLAG
                                       | Z80_NMI_FLAG | Z80_SET_PC_FLAG))) {
        if (EP128EMU_EXPECT(!(R.Flags & (Z80_NMI_FLAG | Z80_SET_PC_FLAG)))) {
          if (R.IFF1)
            executeInterrupt();
        }
        else {
          this->NMI();
        }
      }
    }
    EP128EMU_INLINE void checkNMI()
    {
      if (EP128EMU_UNLIKELY(R.Flags & (Z80_NMI_FLAG | Z80_SET_PC_FLAG)))
        this->NMI();
    }
  };

}       // namespace Ep128

#endif  // __Z80_HEADER_INCLUDED__

