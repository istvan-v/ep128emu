
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2010 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "crtc6845.hpp"

static EP128EMU_REGPARM2 void defaultSyncStateChangeCallback(void *userData,
                                                             bool newState)
{
  (void) userData;
  (void) newState;
}

namespace CPC464 {

  CRTC6845::CRTC6845()
  {
    for (int i = 0; i < 16; i++)
      registers[i] = 0x00;
    hSyncStateChangeCallback = &defaultSyncStateChangeCallback;
    hSyncChangeCallbackUserData = (void *) 0;
    vSyncStateChangeCallback = &defaultSyncStateChangeCallback;
    vSyncChangeCallbackUserData = (void *) 0;
    setLightPenPosition(0x3FFF);
    this->reset();
  }

  CRTC6845::~CRTC6845()
  {
  }

  void CRTC6845::reset()
  {
    horizontalPos = 0;
    displayEnableFlags = 0x00;
    updateHSyncFlag(false);
    updateVSyncFlag(false);
    hSyncCnt = 0;
    vSyncCnt = 0;
    rowAddress = 0;
    verticalPos = 0;
    oddField = false;
    characterAddress =
        uint16_t(registers[13]) | (uint16_t(registers[12] & 0x3F) << 8);
    characterAddressLatch = characterAddress;
    cursorAddress = 0xFFFF;
    writeRegister(8, registers[8]);
    writeRegister(14, registers[14]);
    cursorFlashCnt = 0;
    endOfFrameFlag = 0x00;
    verticalTotalAdjustLatched = registers[5];
    skewShiftRegister = 0x00;
  }

  EP128EMU_REGPARM1 void CRTC6845::checkFrameEnd()
  {
    if (endOfFrameFlag & 0x80) {
      endOfFrameFlag = 0x01;
      rowAddress = (~rowAddressMask) & uint8_t(oddField);
      verticalPos = (verticalPos + 1) & 0x7F;
      if (verticalTotalAdjustLatched & rowAddressMask) {
        verticalTotalAdjustLatched = registers[5];
        return;                         // have vertical total adjust
      }
    }
    else {
      rowAddress = (rowAddress - rowAddressMask) & 0x1F;
      if ((rowAddress ^ verticalTotalAdjustLatched) & rowAddressMask) {
        verticalTotalAdjustLatched = registers[5];
        return;                         // still in vertical total adjust
      }
    }
    // end of frame
    endOfFrameFlag = 0x00;
    displayEnableFlags = 0xC2;
    updateVSyncFlag(false);
    vSyncCnt = 0;
    oddField = !oddField;
    rowAddress = (~rowAddressMask) & uint8_t(oddField);
    verticalPos = 0;
    if (!(registers[4] | (registers[9] & rowAddressMask))) {
      endOfFrameFlag = 0x80;
      verticalTotalAdjustLatched = registers[5];
    }
    characterAddress =
        uint16_t(registers[13]) | (uint16_t(registers[12] & 0x3F) << 8);
    characterAddressLatch = characterAddress;
    cursorFlashCnt = (cursorFlashCnt + 1) & 0xFF;
    switch (registers[10] & 0x60) {
    case 0x00:                          // no cursor flash
      cursorAddress = (cursorAddress & 0x3FFF) | 0x4000;
      break;
    case 0x20:                          // cursor disabled
      cursorAddress = cursorAddress | 0xC000;
      break;
    case 0x40:                          // cursor flash (16 frames)
      cursorAddress = (cursorAddress & 0x3FFF) | 0x4000
                      | (uint16_t(cursorFlashCnt & 0x08) << 12);
      break;
    case 0x60:                          // cursor flash (32 frames)
      cursorAddress = (cursorAddress & 0x3FFF) | 0x4000
                      | (uint16_t(cursorFlashCnt & 0x10) << 11);
      break;
    }
  }

  EP128EMU_REGPARM1 void CRTC6845::lineEnd()
  {
    horizontalPos = 0;
    displayEnableFlags = (displayEnableFlags | 0x40)
                         | ((displayEnableFlags & 0x80) >> 6);
    if (syncFlags & 0x01)
      updateHSyncFlag(false);
    hSyncCnt = 0;
    characterAddress = characterAddressLatch;
    if (((rowAddress ^ registers[11]) & rowAddressMask) == 0) {
      // cursor end line
      cursorAddress = cursorAddress | 0x4000;
    }
    if (endOfFrameFlag) {
      checkFrameEnd();
    }
    else if (!((rowAddress ^ registers[9]) & rowAddressMask)) {
      // character end line
      rowAddress = (~rowAddressMask) & uint8_t(oddField);
      verticalPos = (verticalPos + 1) & 0x7F;
      if (!verticalPos) {
        characterAddress =
            uint16_t(registers[13]) | (uint16_t(registers[12] & 0x3F) << 8);
        characterAddressLatch = characterAddress;
      }
    }
    else {
      // increment row address
      rowAddress = (rowAddress - rowAddressMask) & 0x1F;
      if (verticalPos == registers[4]) {
        if (!((rowAddress ^ registers[9]) & rowAddressMask)) {
          endOfFrameFlag = 0x80;
          verticalTotalAdjustLatched = registers[5];
        }
      }
    }
    if (((rowAddress ^ registers[10]) & rowAddressMask) == 0) {
      // cursor start line
      cursorAddress = cursorAddress & 0xBFFF;
    }
    if (syncFlags & 0x02) {
      // vertical sync
      if (!((++vSyncCnt ^ (registers[3] >> 4)) & 0x0F))
        updateVSyncFlag(false);
    }
    if (verticalPos == registers[6]) {
      // display end / vertical
      displayEnableFlags = 0x40;
    }
    if (verticalPos == registers[7]) {
      if ((rowAddress & rowAddressMask) == 0) {
        // vertical sync start
        updateVSyncFlag(true);
        vSyncCnt = 0;
      }
    }
  }

  EP128EMU_REGPARM2 void CRTC6845::updateHSyncFlag(bool newState)
  {
    if (uint8_t(newState) != (syncFlags & 0x01)) {
      syncFlags = (syncFlags & 0x02) | uint8_t(newState);
      hSyncStateChangeCallback(hSyncChangeCallbackUserData, newState);
    }
  }

  EP128EMU_REGPARM2 void CRTC6845::updateVSyncFlag(bool newState)
  {
    if (newState != bool(syncFlags & 0x02)) {
      syncFlags = (syncFlags & 0x01) | (uint8_t(newState) << 1);
      vSyncStateChangeCallback(vSyncChangeCallbackUserData, newState);
    }
  }

  EP128EMU_REGPARM2 uint8_t CRTC6845::readRegister(uint16_t addr) const
  {
    addr = addr & 0x001F;
    if (addr >= 12 && addr < 18)
      return registers[addr];
    return 0x00;
  }

  EP128EMU_REGPARM3 void CRTC6845::writeRegister(uint16_t addr, uint8_t value)
  {
    addr = addr & 0x001F;
    if (addr < 16) {
      switch (addr) {
#if 0
      case 0x0003:                      // sync widths
        // FIXME: VSYNC width may be ignored depending on the type of the CRTC
        value = value & 0x0F;
        break;
#endif
      case 0x0004:                      // vertical total: 7 bits
        value = value & 0x7F;
        break;
      case 0x0005:                      // vertical total adjust: 5 bits
        value = value & 0x1F;
        break;
      case 0x0006:                      // vertical displayed: 7 bits
        value = value & 0x7F;
        if (value == verticalPos)
          displayEnableFlags = displayEnableFlags & 0x40;
        break;
      case 0x0007:                      // vertical sync position: 7 bits
        value = value & 0x7F;
        if (value == verticalPos) {
          if (registers[7] != verticalPos) {
            updateVSyncFlag(true);
            vSyncCnt = 0;
          }
        }
        break;
      case 0x0008:                      // interlace mode and skew
        value = value & 0xF3;
        rowAddressMask = uint8_t(0x1F - int((value & 0x03) == 0x03));
        displayEnableMask = uint8_t((2 << ((value & 0x30) >> 3)) & 0x3F);
        cursorEnableMask = uint8_t((1 << ((value & 0xC0) >> 5)) & 0x3F);
        break;
      case 0x0009:                      // max. scan line address: 5 bits
        value = value & 0x1F;
        break;
      case 0x000A:                      // cursor start, mode: 7 bits
        value = value & 0x7F;
        break;
      case 0x000B:                      // cursor end: 5 bits
        value = value & 0x1F;
        break;
      case 0x000C:                      // start address H: 6 bits
        value = value & 0x3F;
        break;
      case 0x000E:                      // cursor position H: 6 bits
        value = value & 0x3F;
      case 0x000F:                      // cursor position L: 8 bits
        registers[addr] = value;
        cursorAddress =
            (cursorAddress & 0xC000)
            | (uint16_t(registers[14]) << 8) | uint16_t(registers[15]);
        return;
      }
      registers[addr] = value;
    }
  }

  EP128EMU_REGPARM2 uint8_t CRTC6845::readRegisterDebug(uint16_t addr) const
  {
    addr = addr & 0x001F;
    if (addr < 18)
      return registers[addr];
    return 0x00;
  }

  void CRTC6845::setHSyncStateChangeCallback(
      EP128EMU_REGPARM2 void (*func)(void *userData, bool newState),
      void *userData_)
  {
    if (!func) {
      hSyncStateChangeCallback = &defaultSyncStateChangeCallback;
      hSyncChangeCallbackUserData = (void *) 0;
    }
    else {
      hSyncStateChangeCallback = func;
      hSyncChangeCallbackUserData = userData_;
    }
  }

  void CRTC6845::setVSyncStateChangeCallback(
      EP128EMU_REGPARM2 void (*func)(void *userData, bool newState),
      void *userData_)
  {
    if (!func) {
      vSyncStateChangeCallback = &defaultSyncStateChangeCallback;
      vSyncChangeCallbackUserData = (void *) 0;
    }
    else {
      vSyncStateChangeCallback = func;
      vSyncChangeCallbackUserData = userData_;
    }
  }

  // --------------------------------------------------------------------------

  class ChunkType_CRTCSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    CRTC6845&   ref;
   public:
    ChunkType_CRTCSnapshot(CRTC6845& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_CRTCSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_M6845_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void CRTC6845::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000003U);       // version number
    for (int i = 0; i < 18; i++)
      buf.writeByte(registers[i]);
    buf.writeByte(horizontalPos);
    buf.writeByte((displayEnableFlags >> 6) ^ 0x03);
    buf.writeByte(syncFlags);
    buf.writeByte(hSyncCnt);
    buf.writeByte(vSyncCnt);
    buf.writeByte(rowAddress);
    buf.writeByte(verticalPos);
    buf.writeBoolean(oddField);
    buf.writeUInt16(characterAddress);
    buf.writeUInt16(characterAddressLatch);
    buf.writeByte(uint8_t(cursorAddress >> 14));
    buf.writeByte(cursorFlashCnt);
    buf.writeByte(endOfFrameFlag);
    buf.writeByte(verticalTotalAdjustLatched);
    buf.writeByte(skewShiftRegister & 0x3F);
  }

  void CRTC6845::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_M6845_STATE, buf);
  }

  void CRTC6845::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (!(version >= 0x01000000U && version <= 0x01000003U)) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible CRTC snapshot format");
    }
    try {
      // load saved state
      for (uint16_t i = 0; i < 16; i++)
        writeRegister(i, buf.readByte());
      registers[16] = buf.readByte() & 0x3F;
      registers[17] = buf.readByte();
      horizontalPos = buf.readByte();
      displayEnableFlags = ((buf.readByte() ^ 0x03) & 0x03) << 6;
      if (displayEnableFlags == 0xC0)
        displayEnableFlags = 0xC2;
      uint8_t newSyncFlags = buf.readByte();
      updateHSyncFlag(bool(newSyncFlags & 0x01));
      updateVSyncFlag(bool(newSyncFlags & 0x02));
      hSyncCnt = buf.readByte();
      vSyncCnt = buf.readByte();
      if (version < 0x01000003U) {
        hSyncCnt = registers[3] - hSyncCnt;
        vSyncCnt = (registers[3] >> 4) - vSyncCnt;
      }
      hSyncCnt = hSyncCnt & 0x0F;
      vSyncCnt = vSyncCnt & 0x0F;
      rowAddress = buf.readByte() & 0x1F;
      verticalPos = buf.readByte() & 0x7F;
      oddField = buf.readBoolean();
      characterAddress = buf.readUInt16() & 0x3FFF;
      characterAddressLatch = buf.readUInt16() & 0x3FFF;
      cursorAddress = (cursorAddress & 0x3FFF)
                      | (uint16_t(buf.readByte() & 0x03) << 14);
      cursorFlashCnt = buf.readByte();
      if (version < 0x01000003U) {
        endOfFrameFlag = uint8_t(buf.readBoolean());
        uint8_t verticalAdjustCnt = rowAddress;
        if (version == 0x01000002U)
          verticalAdjustCnt = buf.readByte();
        if (endOfFrameFlag) {
          rowAddress = (verticalAdjustCnt & rowAddressMask) | uint8_t(oddField);
          verticalPos = (registers[4] + 1) & 0x7F;
        }
        else if (verticalPos == registers[4] &&
                 !((rowAddress ^ registers[9]) & rowAddressMask)) {
          endOfFrameFlag = 0x80;
        }
        verticalTotalAdjustLatched = registers[5];
      }
      else {
        endOfFrameFlag = buf.readByte() & 0x81;
        verticalTotalAdjustLatched = buf.readByte() & 0x1F;
      }
      if (version < 0x01000001U)
        skewShiftRegister = 0x00;
      else
        skewShiftRegister = buf.readByte();
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "CRTC snapshot data");
    }
    catch (...) {
      // reset CRTC
      for (int i = 0; i < 16; i++)
        registers[i] = 0x00;
      setLightPenPosition(0x3FFF);
      this->reset();
      throw;
    }
  }

  void CRTC6845::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_CRTCSnapshot  *p;
    p = new ChunkType_CRTCSnapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace CPC464

