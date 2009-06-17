/*
 *  ENTER emulator (c) Copyright, Kevin Thacker 1995-2001
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

namespace Ep128 {

  EP128EMU_INLINE Z80_BYTE Z80::RD_BYTE_INDEX_(Z80_WORD Index)
  {
    SETUP_INDEXED_ADDRESS(Index);
    updateCycles(5);
    return readMemory(R.IndexPlusOffset);
  }

  /*----------------------------------*/
  /* write a byte of data using index */

  EP128EMU_INLINE void Z80::WR_BYTE_INDEX_(Z80_WORD Index, Z80_BYTE Data)
  {
    SETUP_INDEXED_ADDRESS(Index);
    updateCycles(5);
    writeMemory(R.IndexPlusOffset, Data);
  }

  EP128EMU_INLINE void Z80::LD_HL_n()
  {
    writeMemory(R.HL.W, readOpcodeByte(1));
  }

  /*---------------------------*/
  /* pop a word from the stack */

  EP128EMU_INLINE Z80_WORD Z80::POP()
  {
    Z80_WORD Data;

    Data = readMemoryWord(R.SP.W);
    R.SP.W += 2;
    return Data;
  }

  EP128EMU_INLINE void Z80::ADD_A_HL()
  {
    ADD_A_X(readMemory(R.HL.W));
  }

  EP128EMU_INLINE void Z80::ADD_A_n()
  {
    ADD_A_X(readOpcodeByte(1));
  }

  EP128EMU_INLINE void Z80::ADC_A_HL()
  {
    ADC_A_X(readMemory(R.HL.W));
  }

  EP128EMU_INLINE void Z80::ADC_A_n()
  {
    ADC_A_X(readOpcodeByte(1));
  }

  EP128EMU_INLINE void Z80::SUB_A_HL()
  {
    SUB_A_X(readMemory(R.HL.W));
  }

  EP128EMU_INLINE void Z80::SUB_A_n()
  {
    SUB_A_X(readOpcodeByte(1));
  }

  EP128EMU_INLINE void Z80::SBC_A_HL()
  {
    SBC_A_X(readMemory(R.HL.W));
  }

  EP128EMU_INLINE void Z80::SBC_A_n()
  {
    SBC_A_X(readOpcodeByte(1));
  }

  EP128EMU_INLINE void Z80::CP_A_HL()
  {
    CP_A_X(readMemory(R.HL.W));
  }

  EP128EMU_INLINE void Z80::CP_A_n()
  {
    CP_A_X(readOpcodeByte(1));
  }

  EP128EMU_INLINE void Z80::AND_A_n()
  {
    AND_A_X(readOpcodeByte(1));
  }

  EP128EMU_INLINE void Z80::AND_A_HL()
  {
    AND_A_X(readMemory(R.HL.W));
  }

  EP128EMU_INLINE void Z80::XOR_A_n()
  {
    XOR_A_X(readOpcodeByte(1));
  }

  EP128EMU_INLINE void Z80::XOR_A_HL()
  {
    XOR_A_X(readMemory(R.HL.W));
  }

  EP128EMU_INLINE void Z80::OR_A_HL()
  {
    OR_A_X(readMemory(R.HL.W));
  }

  EP128EMU_INLINE void Z80::OR_A_n()
  {
    OR_A_X(readOpcodeByte(1));
  }

  EP128EMU_INLINE void Z80::OUT_n_A()
  {
    /* A in upper byte of port, Data in lower byte of port */
    doOut((Z80_WORD) readOpcodeByte(1) | ((Z80_WORD) (R.AF.B.h) << 8),
          R.AF.B.h);
  }

  EP128EMU_INLINE void Z80::IN_A_n()
  {
    /* A in upper byte of port, data in lower byte of port */
    R.AF.B.h =
        doIn((Z80_WORD) readOpcodeByte(1) | ((Z80_WORD) (R.AF.B.h) << 8));
  }

  EP128EMU_INLINE void Z80::RRA()
  {
    RR(R.AF.B.h);
    A_SHIFTING_FLAGS;
  }

  EP128EMU_INLINE void Z80::RRD()
  {
    Z80_BYTE  tempByte = readMemory(R.HL.W);
    updateCycles(4);
    writeMemory(R.HL.W, (Z80_BYTE) (((tempByte >> 4) | (R.AF.B.h << 4))));
    R.AF.B.h = (R.AF.B.h & 0xF0) | (tempByte & 0x0F);

    Z80_FLAGS_REG = (Z80_FLAGS_REG & Z80_CARRY_FLAG)
                    | t.zeroSignParityTable[R.AF.B.h];
  }

  EP128EMU_INLINE void Z80::RLD()
  {
    Z80_BYTE  tempByte = readMemory(R.HL.W);
    updateCycles(4);
    writeMemory(R.HL.W, (Z80_BYTE) ((tempByte << 4) | (R.AF.B.h & 0x0F)));
    R.AF.B.h = (R.AF.B.h & 0xF0) | (tempByte >> 4);

    Z80_FLAGS_REG = (Z80_FLAGS_REG & Z80_CARRY_FLAG)
                    | t.zeroSignParityTable[R.AF.B.h];
  }

  /*---------------------------*/
  /* jump to a memory location */

  EP128EMU_INLINE void Z80::JP()
  {
    /* set program counter to sub-routine address */
    R.PC.W.l = readOpcodeWord(1);
  }

  /*------------------------------------*/
  /* jump relative to a memory location */

  EP128EMU_INLINE void Z80::JR()
  {
    R.PC.W.l = (uint16_t) (R.PC.W.l + (Z80_LONG) 2
                           + (Z80_BYTE_OFFSET) readOpcodeByte(1));
    updateCycles(5);
  }

  /*--------------------*/
  /* call a sub-routine */

  EP128EMU_INLINE void Z80::CALL()
  {
    Z80_WORD  tempWord = readOpcodeWord(1);
    /* store return address on stack */
    PUSH((Z80_WORD) (R.PC.W.l + 3));
    /* set program counter to sub-routine address */
    R.PC.W.l = tempWord;
  }

  EP128EMU_INLINE void Z80::DJNZ_dd()
  {
    /* decrement B */
    updateCycle();
    R.BC.B.h--;

    /* if zero */
    if (R.BC.B.h == 0) {
      /* continue */
      (void) readOpcodeByte(1);
      R.PC.W.l += 2;
    }
    else {
      /* branch */
      JR();
    }
  }

}       // namespace Ep128

