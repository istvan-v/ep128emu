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

  inline Z80_BYTE Z80::RD_BYTE_INDEX(Z80_WORD Index)
  {
    SETUP_INDEXED_ADDRESS(Index);
    return readMemory(R.IndexPlusOffset);
  }

  /*----------------------------------*/
  /* write a byte of data using index */

  inline void Z80::WR_BYTE_INDEX_OLD(Z80_WORD Index, Z80_BYTE Data)
  {
    Z80_BYTE_OFFSET Offset;
    Offset = (Z80_BYTE_OFFSET) readOpcodeByte(2);
    writeMemory(Z80_WORD(Index + Offset), Data);
  }

  inline void Z80::LD_HL_n()
  {
    R.TempByte = readOpcodeByte(1);
    writeMemory(R.HL.W, R.TempByte);
  }

  /*---------------------------*/
  /* pop a word from the stack */

  inline Z80_WORD Z80::POP()
  {
    Z80_WORD Data;

    Data = readMemoryWord(R.SP.W);
    R.SP.W += 2;
    return Data;
  }

  inline void Z80::ADD_A_HL()
  {
    R.TempByte = readMemory(R.HL.W);
    ADD_A_X(R.TempByte);
  }

  inline void Z80::ADD_A_n()
  {
    R.TempByte = readOpcodeByte(1);
    ADD_A_X(R.TempByte);
  }

  inline void Z80::ADC_A_HL()
  {
    R.TempByte = readMemory(R.HL.W);
    ADC_A_X(R.TempByte);
  }

  inline void Z80::ADC_A_n()
  {
    R.TempByte = readOpcodeByte(1);
    ADC_A_X(R.TempByte);
  }

  inline void Z80::SUB_A_HL()
  {
    R.TempByte = readMemory(R.HL.W);
    SUB_A_X(R.TempByte);
  }

  inline void Z80::SUB_A_n()
  {
    R.TempByte = readOpcodeByte(1);
    SUB_A_X(R.TempByte);
  }

  inline void Z80::SBC_A_HL()
  {
    R.TempByte = readMemory(R.HL.W);
    SBC_A_X(R.TempByte);
  }

  inline void Z80::SBC_A_n()
  {
    R.TempByte = readOpcodeByte(1);
    SBC_A_X(R.TempByte);
  }

  inline void Z80::CP_A_HL()
  {
    R.TempByte = readMemory(R.HL.W);
    CP_A_X(R.TempByte);
  }

  inline void Z80::CP_A_n()
  {
    R.TempByte = readOpcodeByte(1);
    CP_A_X(R.TempByte);
  }

  inline void Z80::AND_A_n()
  {
    R.TempByte = readOpcodeByte(1);
    AND_A_X(R.TempByte);
  }

  inline void Z80::AND_A_HL()
  {
    R.TempByte = readMemory(R.HL.W);
    AND_A_X(R.TempByte);
  }

  inline void Z80::XOR_A_n()
  {
    R.TempByte = readOpcodeByte(1);
    XOR_A_X(R.TempByte);
  }

  inline void Z80::XOR_A_HL()
  {
    R.TempByte = readMemory(R.HL.W);
    XOR_A_X(R.TempByte);
  }

  inline void Z80::OR_A_HL()
  {
    R.TempByte = readMemory(R.HL.W);
    OR_A_X(R.TempByte);
  }

  inline void Z80::OR_A_n()
  {
    R.TempByte = readOpcodeByte(1);
    OR_A_X(R.TempByte);
  }

  inline void Z80::OUT_n_A()
  {
    Z80_WORD Port;

    /* A in upper byte of port, Data in lower byte of port */
    Port = (Z80_WORD) readOpcodeByte(1) | ((Z80_WORD) (R.AF.B.h) << 8);
    updateCycles(6);
    flushCycles();
    doOut(Port, R.AF.B.h);
    updateCycles(5);
  }

  inline void Z80::IN_A_n()
  {
    Z80_WORD Port;

    Port = (Z80_WORD) readOpcodeByte(1) | ((Z80_WORD) (R.AF.B.h) << 8);
    updateCycles(6);
    flushCycles();
    /* a in upper byte of port, data in lower byte of port */
    R.AF.B.h = doIn(Port);
    updateCycles(5);
  }

  inline void Z80::RRA()
  {
    RR(R.AF.B.h);
    A_SHIFTING_FLAGS;
  }

  inline void Z80::RRD()
  {
    R.TempByte = readMemory(R.HL.W);
    writeMemory(R.HL.W, (Z80_BYTE) (((R.TempByte >> 4) | (R.AF.B.h << 4))));
    R.AF.B.h = (R.AF.B.h & 0x0f0) | (R.TempByte & 0x0f);

    Z80_BYTE  Flags;
    Flags = Z80_FLAGS_REG;
    Flags &= Z80_CARRY_FLAG;
    Flags |= t.zeroSignParityTable[R.AF.B.h];
    Z80_FLAGS_REG = Flags;
  }

  inline void Z80::RLD()
  {
    R.TempByte = readMemory(R.HL.W);
    writeMemory(R.HL.W, (Z80_BYTE) ((R.TempByte << 4) | (R.AF.B.h & 0x0f)));
    R.AF.B.h = (R.AF.B.h & 0x0f0) | (R.TempByte >> 4);

    Z80_BYTE  Flags;
    Flags = Z80_FLAGS_REG;
    Flags &= Z80_CARRY_FLAG;
    Flags |= t.zeroSignParityTable[R.AF.B.h];
    Z80_FLAGS_REG = Flags;
  }

  /*---------------------------*/
  /* jump to a memory location */

  inline void Z80::JP()
  {
    /* set program counter to sub-routine address */
    R.PC.W.l = readOpcodeWord(1);
  }

  /*------------------------------------*/
  /* jump relative to a memory location */

  inline void Z80::JR()
  {
    Z80_BYTE_OFFSET Offset;
    Offset = (Z80_BYTE_OFFSET) readOpcodeByte(1);
    R.PC.W.l = (uint16_t) (R.PC.W.l + (Z80_LONG) 2 + Offset);
  }

  /*--------------------*/
  /* call a sub-routine */

  inline void Z80::CALL()
  {
    /* store return address on stack */
    PUSH((Z80_WORD) (R.PC.W.l + 3));
    /* set program counter to sub-routine address */
    R.PC.W.l = readOpcodeWord(1);
  }

  inline void Z80::DJNZ_dd()
  {
    /* decrement B */
    R.BC.B.h--;

    /* if zero */
    if (R.BC.B.h == 0) {
      /* continue */
      R.PC.W.l += 2;
      updateCycles(8);
    }
    else {
      /* branch */
      JR();
      updateCycles(13);
    }
  }

}

