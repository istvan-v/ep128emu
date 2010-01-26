
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

namespace CPC464 {

  CRTC6845::CRTC6845()
  {
    for (int i = 0; i < 16; i++)
      registers[i] = 0x00;
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
    syncFlags = 0x00;
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
    endOfFrameFlag = false;
    verticalAdjustCnt = 0;
    skewShiftRegister = 0x00;
  }

  EP128EMU_REGPARM1 void CRTC6845::lineEnd()
  {
    horizontalPos = 0;
    displayEnableFlags = (displayEnableFlags | 0x40)
                         | ((displayEnableFlags & 0x80) >> 6);
    syncFlags = syncFlags & 0x02;
    hSyncCnt = 0;
    characterAddress = characterAddressLatch;
    if (((rowAddress ^ registers[11]) & rowAddressMask) == 0) {
      // cursor end line
      cursorAddress = cursorAddress | 0x4000;
    }
    // increment row address
    if (((rowAddress ^ registers[9]) & rowAddressMask) == 0) {
      // character end line
      rowAddress = (~rowAddressMask) & uint8_t(oddField);
      if (verticalPos == registers[4]) {
        endOfFrameFlag = true;
        verticalAdjustCnt = 0;
      }
      verticalPos = (verticalPos + 1) & 0x7F;
    }
    else {
      rowAddress = (rowAddress - rowAddressMask) & 0x1F;
    }
    if (endOfFrameFlag) {
      if (verticalAdjustCnt == registers[5]) {
        // end of frame
        endOfFrameFlag = false;
        verticalAdjustCnt = 0;
        displayEnableFlags = 0xC2;
        syncFlags = syncFlags & 0x01;
        vSyncCnt = 0;
        oddField = !oddField;
        rowAddress = (~rowAddressMask) & uint8_t(oddField);
        verticalPos = 0;
        characterAddress =
            uint16_t(registers[13]) | (uint16_t(registers[12] & 0x3F) << 8);
        characterAddressLatch = characterAddress;
        cursorFlashCnt = (cursorFlashCnt + 1) & 0xFF;
        switch (registers[10] & 0x60) {
        case 0x00:                      // no cursor flash
          cursorAddress = (cursorAddress & 0x3FFF) | 0x4000;
          break;
        case 0x20:                      // cursor disabled
          cursorAddress = cursorAddress | 0xC000;
          break;
        case 0x40:                      // cursor flash (16 frames)
          cursorAddress = (cursorAddress & 0x3FFF) | 0x4000
                          | (uint16_t(cursorFlashCnt & 0x08) << 12);
          break;
        case 0x60:                      // cursor flash (32 frames)
          cursorAddress = (cursorAddress & 0x3FFF) | 0x4000
                          | (uint16_t(cursorFlashCnt & 0x10) << 11);
          break;
        }
      }
      else {
        verticalAdjustCnt = (verticalAdjustCnt + 1) & 0x1F;
      }
    }
    if (((rowAddress ^ registers[10]) & rowAddressMask) == 0) {
      // cursor start line
      cursorAddress = cursorAddress & 0xBFFF;
    }
    if (vSyncCnt > 0) {
      // vertical sync
      if (--vSyncCnt == 0)
        syncFlags = syncFlags & 0x01;
    }
    if (verticalPos == registers[6]) {
      // display end / vertical
      displayEnableFlags = 0x40;
    }
    if (verticalPos == registers[7]) {
      if ((rowAddress & rowAddressMask) == 0) {
        // vertical sync start
        syncFlags = syncFlags | 0x02;
        vSyncCnt = (((registers[3] >> 4) - 1) & 0x0F) + 1;
      }
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
            syncFlags = syncFlags | 0x02;
            vSyncCnt = (((registers[3] >> 4) - 1) & 0x0F) + 1;
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
    buf.writeUInt32(0x01000002U);       // version number
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
    buf.writeBoolean(endOfFrameFlag);
    buf.writeByte(verticalAdjustCnt);
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
    if (!(version >= 0x01000000U && version <= 0x01000002U)) {
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
      syncFlags = buf.readByte() & 0x03;
      hSyncCnt = buf.readByte() & 0x1F;
      vSyncCnt = buf.readByte() & 0x1F;
      rowAddress = buf.readByte() & 0x1F;
      verticalPos = buf.readByte() & 0x7F;
      oddField = buf.readBoolean();
      characterAddress = buf.readUInt16() & 0x3FFF;
      characterAddressLatch = buf.readUInt16() & 0x3FFF;
      cursorAddress = (cursorAddress & 0x3FFF)
                      | (uint16_t(buf.readByte() & 0x03) << 14);
      cursorFlashCnt = buf.readByte();
      endOfFrameFlag = buf.readBoolean();
      if (version >= 0x01000002U)
        verticalAdjustCnt = buf.readByte() & 0x1F;
      else
        verticalAdjustCnt = rowAddress >> (0x1F - rowAddressMask);
      if (version >= 0x01000001U)
        skewShiftRegister = buf.readByte();
      else
        skewShiftRegister = 0x00;
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

