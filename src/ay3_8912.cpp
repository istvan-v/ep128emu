
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2009 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "ay3_8912.hpp"

namespace ZX128 {

  const uint8_t AY3_8912::registerMaskTable[16] = {
    0xFF, 0x0F, 0xFF, 0x0F, 0xFF, 0x0F, 0x1F, 0xFF,
    0x1F, 0x1F, 0x1F, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF
  };

  const uint16_t AY3_8912::amplitudeTable[16] = {
        0,   300,   447,   635,   925,  1351,  1851,  2991,
     3695,  5782,  7705,  9829, 12460, 15014, 18528, 21845
  };

  void AY3_8912::resetRegisters()
  {
    for (int i = 0; i < 16; i++)
      registers[i] = 0x00;
    tgFreqA = 0;
    tgCntA = 0;
    tgFreqB = 0;
    tgCntB = 0;
    tgFreqC = 0;
    tgCntC = 0;
    ngFreq = 0;
    ngCnt = 0;
    ngShiftReg = 0x0000FFFFU;
    tgStateA = false;
    tgStateB = false;
    tgStateC = false;
    ngState = false;
    envFreq = 0U;
    envCnt = 0U;
    envState = 0;
    envDir = 0;
    envHold = false;
    envAlternate = false;
    envAttack = false;
    envContinue = false;
    tgDisabledA = false;
    tgDisabledB = false;
    tgDisabledC = false;
    ngDisabledA = false;
    ngDisabledB = false;
    ngDisabledC = false;
    amplitudeA = 0;
    amplitudeB = 0;
    amplitudeC = 0;
    envEnabledA = false;
    envEnabledB = false;
    envEnabledC = false;
    for (uint16_t i = 0; i < 16; i++)
      writeRegister(i, 0x00);
  }

  AY3_8912::AY3_8912()
  {
    resetRegisters();
    portAInput = 0xFF;
  }

  AY3_8912::~AY3_8912()
  {
  }

  void AY3_8912::reset()
  {
    resetRegisters();
  }

  uint8_t AY3_8912::readRegister(uint16_t addr) const
  {
    if ((addr & 0x0F) >= 0x0E) {
      if (!(addr & 0x01))
        return getPortAState();
      return ((registers[7] & 0x80) == 0 ? uint8_t(0xFF) : registers[15]);
    }
    return registers[addr & 0x0F];
  }

  void AY3_8912::writeRegister(uint16_t addr, uint8_t value)
  {
    addr = addr & 0x0F;
    registers[addr] = value & registerMaskTable[addr];
    switch (addr) {
    case 0:                             // tone generator A frequency
    case 1:
      tgFreqA = (int(registers[1]) << 8) | int(registers[0]);
      break;
    case 2:                             // tone generator B frequency
    case 3:
      tgFreqB = (int(registers[3]) << 8) | int(registers[2]);
      break;
    case 4:                             // tone generator C frequency
    case 5:
      tgFreqC = (int(registers[5]) << 8) | int(registers[4]);
      break;
    case 6:                             // noise generator frequency
      ngFreq = registers[6];
      break;
    case 7:                             // mixer
      tgDisabledA = bool(registers[7] & 0x01);
      tgDisabledB = bool(registers[7] & 0x02);
      tgDisabledC = bool(registers[7] & 0x04);
      ngDisabledA = bool(registers[7] & 0x08);
      ngDisabledB = bool(registers[7] & 0x10);
      ngDisabledC = bool(registers[7] & 0x20);
      break;
    case 8:                             // channel A amplitude / envelope mode
      envEnabledA = bool(registers[8] & 0x10);
      if (envEnabledA)
        amplitudeA = amplitudeTable[envState >> 1];
      else
        amplitudeA = amplitudeTable[registers[8] & 0x0F];
      break;
    case 9:                             // channel B amplitude / envelope mode
      envEnabledB = bool(registers[9] & 0x10);
      if (envEnabledB)
        amplitudeB = amplitudeTable[envState >> 1];
      else
        amplitudeB = amplitudeTable[registers[9] & 0x0F];
      break;
    case 10:                            // channel C amplitude / envelope mode
      envEnabledC = bool(registers[10] & 0x10);
      if (envEnabledC)
        amplitudeC = amplitudeTable[envState >> 1];
      else
        amplitudeC = amplitudeTable[registers[10] & 0x0F];
      break;
    case 11:                            // envelope generator frequency
    case 12:
      envFreq = (uint32_t(registers[12]) << 8) | uint32_t(registers[11]);
      break;
    case 13:                            // envelope generator mode / restart
      envHold = bool(registers[13] & 0x01);
      envAlternate = bool(registers[13] & 0x02);
      envAttack = bool(registers[13] & 0x04);
      envContinue = bool(registers[13] & 0x08);
      envCnt = envFreq;
      if (envAttack) {
        envState = 0;
        envDir = 1;
      }
      else {
        envState = 31;
        envDir = -1;
      }
      if (envEnabledA)
        amplitudeA = amplitudeTable[envState >> 1];
      if (envEnabledB)
        amplitudeB = amplitudeTable[envState >> 1];
      if (envEnabledC)
        amplitudeC = amplitudeTable[envState >> 1];
      break;
    }
  }

  uint16_t AY3_8912::runOneCycle()
  {
    uint16_t  audioOutput = 0;
    if ((tgStateA | tgDisabledA) & (ngState | ngDisabledA))
      audioOutput += amplitudeA;
    if ((tgStateB | tgDisabledB) & (ngState | ngDisabledB))
      audioOutput += amplitudeB;
    if ((tgStateC | tgDisabledC) & (ngState | ngDisabledC))
      audioOutput += amplitudeC;
    if (tgCntA <= 1) {
      tgCntA = tgFreqA;
      tgStateA = !tgStateA;
    }
    else {
      tgCntA--;
    }
    if (tgCntB <= 1) {
      tgCntB = tgFreqB;
      tgStateB = !tgStateB;
    }
    else {
      tgCntB--;
    }
    if (tgCntC <= 1) {
      tgCntC = tgFreqC;
      tgStateC = !tgStateC;
    }
    else {
      tgCntC--;
    }
    if (!(ngCnt & 0x7E)) {
      ngCnt = (ngCnt ^ 0x80) | ngFreq;
      if (!(ngCnt & 0x80)) {
        ngState = bool(ngShiftReg & 0x00008000U);
        ngShiftReg = ((ngShiftReg & 0x0000FFFFU) << 1)
                     | ((~((ngShiftReg >> 16) ^ (ngShiftReg >> 13))) & 1U);
      }
    }
    else {
      ngCnt--;
    }
    if (envCnt <= 1) {
      envCnt = envFreq;
      envState += envDir;
      if (envState < 0 || envState > 31) {
        if (envHold || !envContinue) {
          envState = ((envAlternate == envAttack || !envContinue) ? 0 : 31);
          envDir = 0;
        }
        else if (!envAlternate) {
          envState = (envState < 0 ? 31 : 0);
        }
        else if (envDir > 0) {
          envDir = -1;
          envState = 31;
        }
        else {
          envDir = 1;
          envState = 0;
        }
      }
      if (envEnabledA)
        amplitudeA = amplitudeTable[envState >> 1];
      if (envEnabledB)
        amplitudeB = amplitudeTable[envState >> 1];
      if (envEnabledC)
        amplitudeC = amplitudeTable[envState >> 1];
    }
    else {
      envCnt--;
    }
    return audioOutput;
  }

  // --------------------------------------------------------------------------

  class ChunkType_AY3Snapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    AY3_8912& ref;
   public:
    ChunkType_AY3Snapshot(AY3_8912& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_AY3Snapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_ZXAY3_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void AY3_8912::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000001U);       // version number
    for (int i = 0; i < 16; i++)
      buf.writeByte(registers[i]);
    buf.writeUInt32(uint32_t(tgCntA));
    buf.writeUInt32(uint32_t(tgCntB));
    buf.writeUInt32(uint32_t(tgCntC));
    buf.writeByte(uint8_t(ngCnt));
    buf.writeUInt32(ngShiftReg);
    buf.writeBoolean(tgStateA);
    buf.writeBoolean(tgStateB);
    buf.writeBoolean(tgStateC);
    buf.writeUInt32(envCnt);
    buf.writeByte(uint8_t(envState));
    buf.writeByte(uint8_t(envDir & 0xFF));
  }

  void AY3_8912::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_ZXAY3_STATE, buf);
  }

  void AY3_8912::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (!(version >= 0x01000000U && version <= 0x01000001U)) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible AY3 snapshot format");
    }
    try {
      // load saved state
      for (int i = 0; i < 16; i++)
        writeRegister(uint16_t(i), buf.readByte());
      tgCntA = int(buf.readUInt32() & 0x0FFFU);
      tgCntB = int(buf.readUInt32() & 0x0FFFU);
      tgCntC = int(buf.readUInt32() & 0x0FFFU);
      ngCnt = buf.readByte() & 0x9F;
      ngShiftReg = buf.readUInt32() & 0x0001FFFFU;
      if (ngShiftReg == 0x0001FFFFU)
        ngShiftReg = 0U;
      ngState = bool(ngShiftReg & 0x00010000U);
      tgStateA = buf.readBoolean();
      tgStateB = buf.readBoolean();
      tgStateC = buf.readBoolean();
      if (version == 0x01000000U)
        (void) buf.readBoolean();       // was ngState
      envCnt = buf.readUInt32();
      envState = buf.readByte();
      if (version == 0x01000000U)
        envState = envState << 1;
      envState = envState & 0x1F;
      if (envEnabledA)
        amplitudeA = amplitudeTable[envState >> 1];
      if (envEnabledB)
        amplitudeB = amplitudeTable[envState >> 1];
      if (envEnabledC)
        amplitudeC = amplitudeTable[envState >> 1];
      envDir = buf.readByte();
      if (envDir != 0)
        envDir = (envDir < 0x80 ? 1 : -1);
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "AY3 snapshot data");
    }
    catch (...) {
      // reset AY3
      this->reset();
      throw;
    }
  }

  void AY3_8912::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_AY3Snapshot   *p = new ChunkType_AY3Snapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace ZX128

