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

#include "z80.hpp"
#include "z80macros.hpp"
#include <cstring>

namespace Ep128 {

  Z80Tables Z80::t;

  void Z80::executeInterrupt()
  {
    R.IFF1 = 0;
    R.IFF2 = 0;

    if (R.Flags & Z80_EXECUTING_HALT_FLAG) {
      ADD_PC(1);
      R.Flags &= ~Z80_EXECUTING_HALT_FLAG;
    }

    ackInterruptFunction();

    switch (R.IM) {
    case 0x00:
      // unsupported
      break;
    case 0x01:
      {
        // The number of cycles required to complete the instruction
        // is two more than normal due to the two added wait states
        updateCycles(7);
        // push return address onto stack
        PUSH(R.PC.W.l);
        // set program counter address
        R.PC.W.l = 0x0038;
      }
      break;
    case 0x02:
      {
        Z80_WORD Vector;
        Z80_WORD Address;
        // 19 clock cycles for this mode. 8 for vector,
        // six for program counter, six to obtain jump address
        updateCycles(7);
        PUSH(R.PC.W.l);
        Vector = (R.I << 8) | (R.InterruptVectorBase);
        Address = readMemoryWord(Vector);
        R.PC.W.l = Address;
      }
      break;
    }
  }

  void Z80::setVectorBase(int Base)
  {
    R.InterruptVectorBase = Base & 0x0ff;
  }

  /*
    A value has even parity when all the binary digits added together give an
    even number. (result = true). A value has an odd parity when all the digits
    added together give an odd number. (result = false)
   */

  static inline bool isParityEven(uint8_t n)
  {
    unsigned int  tmp = n;
    tmp = tmp ^ (tmp >> 1);
    tmp = tmp ^ (tmp >> 2);
    tmp = tmp ^ (tmp >> 4);
    return !(tmp & 1);
  }

  Z80Tables::Z80Tables()
  {
    size_t  i;

    for (i = 0; i < 256; i++) {
      /* in flags register, if result has even parity, then */
      /* bit is set to 1, if result has odd parity, then bit */
      /* is set to 0. */
      parityTable[i] = (isParityEven(uint8_t(i)) ? Z80_PARITY_FLAG : 0);
    }

    for (i = 0; i < 256; i++) {
      zeroSignTable[i] = 0;

      if ((i & 0x0ff) == 0) {
        zeroSignTable[i] |= Z80_ZERO_FLAG;
      }

      if (i & 0x080) {
        zeroSignTable[i] |= Z80_SIGN_FLAG;
      }
    }

    for (i = 0; i < 256; i++) {
      zeroSignTable2[i] =
          zeroSignTable[i] | ((unsigned char) i
                              & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2));
    }

    for (i = 0; i < 256; i++) {
      /* included unused flag1 and 2 for AND, XOR, OR, IN, RLD, RRD */
      zeroSignParityTable[i] = parityTable[i] | zeroSignTable2[i];
    }

    // calculate DAA table

    for (i = 0; i < 2048; i++) {
      bool    carry = !!(i & 256);
      bool    halfCarry = !!(i & 512);
      bool    subtractFlag = !!(i & 1024);
      uint8_t l = uint8_t(i) & 0x0F;
      uint8_t h = uint8_t(i >> 4) & 0x0F;
      if (l > 9 || halfCarry) {
        if (!subtractFlag) {
          l += 6;
          if (l >= 0x10) {
            l &= 0x0F;
            h++;
            halfCarry = true;
          }
          else
            halfCarry = false;
        }
        else {
          if (l > 9 && h >= 9)
            carry = true;
          l -= 6;
          halfCarry = !!(l & 0x10);
        }
      }
      else
        halfCarry = false;
      if (h > 9 || carry) {
        if (!subtractFlag) {
          h += 6;
          if (h >= 0x10)
            carry = true;
        }
        else {
          if (h > 9)
            carry = true;
          h -= 6;
        }
      }
      uint8_t   a = ((h << 4) + l) & 0xFF;
      uint16_t  af = uint16_t(a) << 8;
      af |= (carry ? Z80_CARRY_FLAG : 0);
      af |= (subtractFlag ? Z80_SUBTRACT_FLAG : 0);
      af |= (halfCarry ? Z80_HALFCARRY_FLAG : 0);
      af |= uint16_t(zeroSignParityTable[a]);
      daaTable[i] = af;
    }
  }

  Z80::Z80()
  {
    std::memset(&R, 0, sizeof(Z80_REGISTERS));
    this->reset();
  }

  void Z80::reset()
  {
    R.PC.L = 0;
    R.I = 0;
    R.IM = 0;
    R.IFF1 = 0;
    R.IFF2 = 0;
    R.RBit7 = 0;
    R.R = 0;
    R.IX.W = 0x0ffff;
    R.IY.W = 0x0ffff;
    R.AF.B.l = Z80_ZERO_FLAG;
    R.Flags &= ~
        (Z80_EXECUTING_HALT_FLAG |
         Z80_CHECK_INTERRUPT_FLAG |
         Z80_EXECUTE_INTERRUPT_HANDLER_FLAG | Z80_INTERRUPT_FLAG |
         Z80_NMI_FLAG | Z80_SET_PC_FLAG);
    newPCAddress = -1;
  }

  void Z80::setProgramCounter(uint16_t addr)
  {
    if (addr == uint16_t(R.PC.W.l)) {
      R.Flags = R.Flags & (~Z80_SET_PC_FLAG);
      newPCAddress = -1;
      return;
    }
    newPCAddress = int32_t(addr);
    R.Flags = R.Flags | Z80_SET_PC_FLAG;
  }

  void Z80::NMI()
  {
    if (R.Flags & Z80_SET_PC_FLAG) {
      R.PC.W.l = Z80_WORD(newPCAddress);
      newPCAddress = -1;
      if (!(R.Flags & Z80_NMI_FLAG)) {
        R.Flags = R.Flags & (~(Z80_SET_PC_FLAG | Z80_EXECUTING_HALT_FLAG));
        return;
      }
      // if an NMI is pending at the same time as setting the program counter,
      // then set the PC first, and then immediately execute the NMI as well
    }
    R.Flags = R.Flags & (~(Z80_NMI_FLAG | Z80_SET_PC_FLAG));
    if (R.Flags & Z80_EXECUTING_HALT_FLAG) {
      ADD_PC(1);
      R.Flags &= ~Z80_EXECUTING_HALT_FLAG;
    }
    // disable maskable ints
    R.IFF1 = 0;
    // push return address on stack
    PUSH(R.PC.W.l);
    // set program counter address
    R.PC.W.l = 0x0066;
  }

  void Z80::NMI_()
  {
    R.Flags |= Z80_NMI_FLAG;
  }

  void Z80::triggerInterrupt()
  {
    R.Flags |= Z80_INTERRUPT_FLAG;
    R.Flags |= Z80_EXECUTE_INTERRUPT_HANDLER_FLAG;
  }

  void Z80::clearInterrupt()
  {
    R.Flags &= ~Z80_INTERRUPT_FLAG;
    R.Flags &= ~Z80_EXECUTE_INTERRUPT_HANDLER_FLAG;
  }

  void Z80::test()
  {
  }

  void Z80::CPI()
  {
    Z80_BYTE Result;

    R.TempByte = readMemory(R.HL.W);
    Result = R.AF.B.h - R.TempByte;

    Z80_FLAGS_REG = Z80_FLAGS_REG | Z80_SUBTRACT_FLAG;
    SET_ZERO_SIGN(Result);

    R.HL.W++;
    R.BC.W--;

    Z80_FLAGS_REG = Z80_FLAGS_REG & (~Z80_PARITY_FLAG);
    if (R.BC.W != 0) {
      Z80_FLAGS_REG = Z80_FLAGS_REG | Z80_PARITY_FLAG;
    }
  }

  void Z80::CPD()
  {
    Z80_BYTE Result;

    R.TempByte = readMemory(R.HL.W);
    Result = R.AF.B.h - R.TempByte;

    Z80_FLAGS_REG = Z80_FLAGS_REG | Z80_SUBTRACT_FLAG;
    SET_ZERO_SIGN(Result);

    R.HL.W--;
    R.BC.W--;

    Z80_FLAGS_REG = Z80_FLAGS_REG & (~Z80_PARITY_FLAG);
    if (R.BC.W != 0) {
      Z80_FLAGS_REG = Z80_FLAGS_REG | Z80_PARITY_FLAG;
    }
  }

  void Z80::OUTI()
  {
    R.BC.B.h--;
    SET_ZERO_FLAG(R.BC.B.h);
    R.TempByte = readMemory(R.HL.W);
    doOut(R.BC.W, R.TempByte);
    Z80_FLAGS_REG |= Z80_SUBTRACT_FLAG;
    R.HL.W++;
    updateCycles(5);
  }

  /* B is pre-decremented before execution */
  void Z80::OUTD()
  {
    R.BC.B.h--;
    SET_ZERO_FLAG(R.BC.B.h);
    R.TempByte = readMemory(R.HL.W);
    doOut(R.BC.W, R.TempByte);
    /* as per Zilog docs */
    Z80_FLAGS_REG |= Z80_SUBTRACT_FLAG;
    R.HL.W--;
    updateCycles(5);
  }

  void Z80::INI()
  {
    R.TempByte = doIn(R.BC.W);
    writeMemory(R.HL.W, R.TempByte);
    R.HL.W++;
    R.BC.B.h--;
    SET_ZERO_FLAG(R.BC.B.h);
    Z80_FLAGS_REG |= Z80_SUBTRACT_FLAG;
    updateCycles(5);
  }

  void Z80::IND()
  {
    R.TempByte = doIn(R.BC.W);
    writeMemory(R.HL.W, R.TempByte);
    R.HL.W--;
    R.BC.B.h--;
    SET_ZERO_FLAG(R.BC.B.h);
    Z80_FLAGS_REG |= Z80_SUBTRACT_FLAG;
    updateCycles(5);
  }

  /* half carry not set */
  void Z80::DAA()
  {
    int     i = R.AF.B.h;

    if (R.AF.B.l & Z80_CARRY_FLAG)
      i |= 256;
    if (R.AF.B.l & Z80_HALFCARRY_FLAG)
      i |= 512;
    if (R.AF.B.l & Z80_SUBTRACT_FLAG)
      i |= 1024;
    R.AF.W = t.daaTable[i];
  }

  // --------------------------------------------------------------------------

  Z80::~Z80()
  {
  }

  void Z80::ackInterruptFunction()
  {
  }

  uint8_t Z80::readMemory(uint16_t addr)
  {
    (void) addr;
    updateCycles(3);
    return 0;
  }

  void Z80::writeMemory(uint16_t addr, uint8_t value)
  {
    (void) addr;
    (void) value;
    updateCycles(3);
  }

  uint16_t Z80::readMemoryWord(uint16_t addr)
  {
    return (uint16_t(readMemory(addr))
            + (uint16_t(readMemory((addr + 1) & 0xFFFF)) << 8));
  }

  void Z80::writeMemoryWord(uint16_t addr, uint16_t value)
  {
    writeMemory(addr, uint8_t(value) & 0xFF);
    writeMemory((addr + 1) & 0xFFFF, uint8_t(value >> 8));
  }

  void Z80::doOut(uint16_t addr, uint8_t value)
  {
    (void) addr;
    (void) value;
  }

  uint8_t Z80::doIn(uint16_t addr)
  {
    (void) addr;
    return 0xFF;
  }

  uint8_t Z80::readOpcodeFirstByte()
  {
    updateCycle();
    return readMemory(uint16_t(R.PC.W.l));
  }

  uint8_t Z80::readOpcodeSecondByte()
  {
    updateCycle();
    return readMemory((uint16_t(R.PC.W.l) + uint16_t(1)) & 0xFFFF);
  }

  uint8_t Z80::readOpcodeByte(int offset)
  {
    return readMemory((uint16_t(R.PC.W.l) + uint16_t(offset)) & 0xFFFF);
  }

  uint16_t Z80::readOpcodeWord(int offset)
  {
    return readMemoryWord((uint16_t(R.PC.W.l) + uint16_t(offset)) & 0xFFFF);
  }

  void Z80::updateCycle()
  {
    updateCycles(1);
  }

  void Z80::updateCycles(int cycles)
  {
    (void) cycles;
  }

  void Z80::tapePatch()
  {
  }

  // --------------------------------------------------------------------------

  class ChunkType_Z80_Snapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Z80&    ref;
   public:
    ChunkType_Z80_Snapshot(Z80& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Z80_Snapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_Z80_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void Z80::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000001);        // version number
    buf.writeByte(R.PC.B.h);            // PC high
    buf.writeByte(R.PC.B.l);            // PC low
    buf.writeByte(R.AF.B.h);            // A
    buf.writeByte(R.AF.B.l);            // F
    buf.writeByte(R.BC.B.h);            // B
    buf.writeByte(R.BC.B.l);            // C
    buf.writeByte(R.DE.B.h);            // D
    buf.writeByte(R.DE.B.l);            // E
    buf.writeByte(R.HL.B.h);            // H
    buf.writeByte(R.HL.B.l);            // L
    buf.writeByte(R.SP.B.h);            // SP high
    buf.writeByte(R.SP.B.l);            // SP low
    buf.writeByte(R.IX.B.h);            // IX high
    buf.writeByte(R.IX.B.l);            // IX low
    buf.writeByte(R.IY.B.h);            // IY high
    buf.writeByte(R.IY.B.l);            // IY low
    buf.writeByte(R.altAF.B.h);         // A'
    buf.writeByte(R.altAF.B.l);         // F'
    buf.writeByte(R.altBC.B.h);         // B'
    buf.writeByte(R.altBC.B.l);         // C'
    buf.writeByte(R.altDE.B.h);         // D'
    buf.writeByte(R.altDE.B.l);         // E'
    buf.writeByte(R.altHL.B.h);         // H'
    buf.writeByte(R.altHL.B.l);         // L'
    buf.writeByte(R.I);                 // I
    buf.writeByte(R.R);                 // R
    buf.writeUInt32(R.IndexPlusOffset);
    buf.writeByte(R.IFF1);
    buf.writeByte(R.IFF2);
    buf.writeByte(R.RBit7);
    buf.writeByte(R.IM);
    buf.writeByte(R.TempByte);
    buf.writeByte(R.InterruptVectorBase);
    buf.writeUInt32(uint32_t(R.Flags));
    buf.writeInt32(newPCAddress);
  }

  void Z80::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_Z80_STATE, buf);
  }

  void Z80::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (!(version >= 0x01000000 && version <= 0x01000001)) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible Z80 snapshot format");
    }
    try {
      std::memset(&R, 0, sizeof(Z80_REGISTERS));
      this->reset();
      // load saved state
      R.PC.B.h = buf.readByte();        // PC high
      R.PC.B.l = buf.readByte();        // PC low
      R.AF.B.h = buf.readByte();        // A
      R.AF.B.l = buf.readByte();        // F
      R.BC.B.h = buf.readByte();        // B
      R.BC.B.l = buf.readByte();        // C
      R.DE.B.h = buf.readByte();        // D
      R.DE.B.l = buf.readByte();        // E
      R.HL.B.h = buf.readByte();        // H
      R.HL.B.l = buf.readByte();        // L
      R.SP.B.h = buf.readByte();        // SP high
      R.SP.B.l = buf.readByte();        // SP low
      R.IX.B.h = buf.readByte();        // IX high
      R.IX.B.l = buf.readByte();        // IX low
      R.IY.B.h = buf.readByte();        // IY high
      R.IY.B.l = buf.readByte();        // IY low
      R.altAF.B.h = buf.readByte();     // A'
      R.altAF.B.l = buf.readByte();     // F'
      R.altBC.B.h = buf.readByte();     // B'
      R.altBC.B.l = buf.readByte();     // C'
      R.altDE.B.h = buf.readByte();     // D'
      R.altDE.B.l = buf.readByte();     // E'
      R.altHL.B.h = buf.readByte();     // H'
      R.altHL.B.l = buf.readByte();     // L'
      R.I = buf.readByte();             // I
      R.R = buf.readByte();             // R
      R.IndexPlusOffset = uint16_t(buf.readUInt32()) & 0xFFFF;
      R.IFF1 = buf.readByte();
      R.IFF2 = buf.readByte();
      R.RBit7 = buf.readByte();
      R.IM = buf.readByte();
      R.TempByte = buf.readByte();
      R.InterruptVectorBase = buf.readByte();
      R.Flags = buf.readUInt32() & Z80_FLAGS_MASK;
      if (version >= 0x01000001) {
        newPCAddress = buf.readInt32();
        if (newPCAddress < 0 || !(R.Flags & Z80_SET_PC_FLAG))
          newPCAddress = -1;
        else
          newPCAddress = newPCAddress & 0xFFFF;
      }
      else
        R.Flags = R.Flags & (~Z80_SET_PC_FLAG);
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "Z80 snapshot data");
    }
    catch (...) {
      // reset Z80
      this->reset();
      throw;
    }
  }

  void Z80::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_Z80_Snapshot  *p;
    p = new ChunkType_Z80_Snapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace Ep128

