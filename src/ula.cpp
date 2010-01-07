
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
#include "ula.hpp"

namespace ZX128 {

  const uint16_t ULA::audioOutputLevelTable[32] = {
        0, 10000,     0, 10000,     0, 10000,     0, 10000,
     4342, 14342,  4342, 14342,  4342, 14342,  4342, 14342,
    43690, 53690, 43690, 53690, 43690, 53690, 43690, 53690,
    45590, 55590, 45590, 55590, 45590, 55590, 45590, 55590
  };

  void EP128EMU_REGPARM1 ULA::videoFunc_TBorder(void *userData)
  {
    ULA&    ula = *(reinterpret_cast<ULA *>(userData));
    ula.lineBufPtr[0] = 0x01;
    ula.lineBufPtr[1] = ula.borderColor;
    ula.lineBufPtr = ula.lineBufPtr + 2;
    ula.currentSlot++;
    if (ula.currentSlot == 40) {
      ula.runOneSlot_ = &ULA::videoFunc_TBorderHBlank;
      ula.drawLine(ula.lineBuf, size_t(ula.lineBufPtr - ula.lineBuf));
    }
    else if (ula.currentSlot == ula.slotsPerLine) {
      ula.currentSlot = 0;
    }
  }

  void EP128EMU_REGPARM1 ULA::videoFunc_TBorderHBlank(void *userData)
  {
    ULA&    ula = *(reinterpret_cast<ULA *>(userData));
    ula.lineBufPtr[0] = 0x01;
    ula.lineBufPtr[1] = 0x00;
    ula.lineBufPtr = ula.lineBufPtr + 2;
    ula.currentSlot++;
    if (ula.currentSlot == ula.hSyncEndSlot) {
      ula.currentLine++;
      if (ula.currentLine == ula.linesPerFrame) {
        ula.currentLine = 0;
        ula.ld1Ptr = &(ula.videoRAMPtr[0x1800]);
        ula.ld2Ptr = ula.videoRAMPtr;
        ula.runOneSlot_ = &ULA::videoFunc_DisplayLBorder;
      }
      else {
        ula.runOneSlot_ = &ULA::videoFunc_TBorder;
      }
      ula.lineBufPtr = ula.lineBuf;
    }
  }

  void EP128EMU_REGPARM1 ULA::videoFunc_DisplayLBorder(void *userData)
  {
    ULA&    ula = *(reinterpret_cast<ULA *>(userData));
    ula.lineBufPtr[0] = 0x01;
    ula.lineBufPtr[1] = ula.borderColor;
    ula.lineBufPtr = ula.lineBufPtr + 2;
    ula.currentSlot++;
    if (ula.currentSlot == ula.slotsPerLine) {
      ula.currentSlot = 0;
      ula.runOneSlot_ = &ULA::videoFunc_Display;
    }
  }

  void EP128EMU_REGPARM1 ULA::videoFunc_Display(void *userData)
  {
    ULA&    ula = *(reinterpret_cast<ULA *>(userData));
    uint8_t a = ula.ld1Ptr[ula.currentSlot];
    uint8_t b = ula.ld2Ptr[ula.currentSlot];
    uint8_t c0 = (a & 0x78) >> 3;
    uint8_t c1 = (a & 0x07) | ((a & 0x40) >> 3);
    if ((a & ula.flashCnt) & 0x80) {
      uint8_t tmp = c0;
      c0 = c1;
      c1 = tmp;
    }
    ula.lineBufPtr[0] = 0x03;
    ula.lineBufPtr[1] = c0;
    ula.lineBufPtr[2] = c1;
    ula.lineBufPtr[3] = b;
    ula.lineBufPtr = ula.lineBufPtr + 4;
    ula.currentSlot++;
    if (ula.currentSlot == 32)
      ula.runOneSlot_ = &ULA::videoFunc_DisplayRBorder;
  }

  void EP128EMU_REGPARM1 ULA::videoFunc_DisplayRBorder(void *userData)
  {
    ULA&    ula = *(reinterpret_cast<ULA *>(userData));
    ula.lineBufPtr[0] = 0x01;
    ula.lineBufPtr[1] = ula.borderColor;
    ula.lineBufPtr = ula.lineBufPtr + 2;
    ula.currentSlot++;
    if (ula.currentSlot == 40) {
      ula.runOneSlot_ = &ULA::videoFunc_DisplayHBlank;
      ula.drawLine(ula.lineBuf, size_t(ula.lineBufPtr - ula.lineBuf));
    }
  }

  void EP128EMU_REGPARM1 ULA::videoFunc_DisplayHBlank(void *userData)
  {
    ULA&    ula = *(reinterpret_cast<ULA *>(userData));
    ula.lineBufPtr[0] = 0x01;
    ula.lineBufPtr[1] = 0x00;
    ula.lineBufPtr = ula.lineBufPtr + 2;
    ula.currentSlot++;
    if (ula.currentSlot == ula.hSyncEndSlot) {
      ula.currentLine++;
      if (ula.currentLine == 192) {
        ula.runOneSlot_ = &ULA::videoFunc_BBorder;
      }
      else {
        if ((ula.currentLine & 7) != 0) {
          ula.ld2Ptr = ula.ld2Ptr + 0x0100;
        }
        else if ((ula.currentLine & 63) != 0) {
          ula.ld1Ptr = ula.ld1Ptr + 0x0020;
          ula.ld2Ptr = ula.ld2Ptr - 0x06E0;
        }
        else {
          ula.ld1Ptr = ula.ld1Ptr + 0x0020;
          ula.ld2Ptr = ula.ld2Ptr + 0x0020;
        }
        ula.runOneSlot_ = &ULA::videoFunc_DisplayLBorder;
      }
      ula.lineBufPtr = ula.lineBuf;
    }
  }

  void EP128EMU_REGPARM1 ULA::videoFunc_BBorder(void *userData)
  {
    ULA&    ula = *(reinterpret_cast<ULA *>(userData));
    ula.lineBufPtr[0] = 0x01;
    ula.lineBufPtr[1] = ula.borderColor;
    ula.lineBufPtr = ula.lineBufPtr + 2;
    ula.currentSlot++;
    if (ula.currentSlot == 40) {
      ula.runOneSlot_ = &ULA::videoFunc_BBorderHBlank;
      ula.drawLine(ula.lineBuf, size_t(ula.lineBufPtr - ula.lineBuf));
    }
    else if (ula.currentSlot == ula.slotsPerLine) {
      ula.currentSlot = 0;
    }
  }

  void EP128EMU_REGPARM1 ULA::videoFunc_BBorderHBlank(void *userData)
  {
    ULA&    ula = *(reinterpret_cast<ULA *>(userData));
    ula.lineBufPtr[0] = 0x01;
    ula.lineBufPtr[1] = 0x00;
    ula.lineBufPtr = ula.lineBufPtr + 2;
    ula.currentSlot++;
    if (ula.currentSlot == ula.hSyncEndSlot) {
      ula.currentLine++;
      if (ula.currentLine == 241)
        ula.runOneSlot_ = &ULA::videoFunc_VBlank;
      else
        ula.runOneSlot_ = &ULA::videoFunc_BBorder;
      ula.lineBufPtr = ula.lineBuf;
    }
  }

  void EP128EMU_REGPARM1 ULA::videoFunc_VBlank(void *userData)
  {
    ULA&    ula = *(reinterpret_cast<ULA *>(userData));
    ula.lineBufPtr[0] = 0x01;
    ula.lineBufPtr[1] = 0x00;
    ula.lineBufPtr = ula.lineBufPtr + 2;
    ula.currentSlot++;
    if (ula.currentSlot == 40) {
      ula.runOneSlot_ = &ULA::videoFunc_VBlankHBlank;
      ula.drawLine(ula.lineBuf, size_t(ula.lineBufPtr - ula.lineBuf));
    }
    else if (ula.currentSlot == ula.slotsPerLine) {
      ula.currentSlot = 0;
    }
  }

  void EP128EMU_REGPARM1 ULA::videoFunc_VBlankHBlank(void *userData)
  {
    ULA&    ula = *(reinterpret_cast<ULA *>(userData));
    ula.lineBufPtr[0] = 0x01;
    ula.lineBufPtr[1] = 0x00;
    ula.lineBufPtr = ula.lineBufPtr + 2;
    ula.currentSlot++;
    if (ula.currentSlot == ula.hSyncEndSlot) {
      ula.currentLine++;
      if (ula.currentLine == 258)
        ula.runOneSlot_ = &ULA::videoFunc_TBorder;
      else
        ula.runOneSlot_ = &ULA::videoFunc_VBlank;
      if (ula.currentLine == 243 && !ula.vsyncFlag) {
        ula.vsyncFlag = true;
        ula.vsyncStateChange(true, 7);
      }
      else if (ula.currentLine == 246 && ula.vsyncFlag) {
        ula.vsyncFlag = false;
        ula.vsyncStateChange(false, 7);
        ula.flashCnt = (ula.flashCnt + 8) & 0xF8;
      }
      else if (ula.currentLine == 247) {
        ula.irqPollEnableCallback(true);
      }
      else if (ula.currentLine == 249) {
        ula.irqPollEnableCallback(false);
      }
      ula.lineBufPtr = ula.lineBuf;
    }
  }

  void ULA::clearLineBuffer()
  {
    uint32_t  *p = reinterpret_cast<uint32_t *>(lineBuf);
    for (int i = 0; i < 57; i++)
      p[i] = 0U;
    for (int i = 0; i < 114; i += 2) {
      lineBuf[i] = 0x01;
      lineBuf[i + 1] = borderColor;
    }
    lineBufPtr = lineBuf;
  }

  EP128EMU_REGPARM2 bool ULA::getInterruptFlag_(int timeOffs) const
  {
    int     x = 0;
    int     y = 0;
    getVideoPosition(x, y, timeOffs + 6 - (int(spectrum128Mode) << 2));
    return (y == 248 && x < ((int(spectrum128Mode) + 8) << 3));
  }

  EP128EMU_REGPARM2 int ULA::getWaitHalfCycles_(int timeOffs) const
  {
    int     x = 0;
    int     y = 0;
    getVideoPosition(x, y, timeOffs + 6);
    if (y < 0 || y >= 192)
      return 0;
    if (x >= 256)
      return 0;
    return ((x & 14) < 12 ? (12 - (x & 14)) : 0);
  }

  EP128EMU_REGPARM2 uint8_t ULA::idleDataBusRead_(int timeOffs) const
  {
    int     x = 0;
    int     y = 0;
    getVideoPosition(x, y, timeOffs);
    if (y < 0 || y >= 192)
      return 0xFF;
    if (x >= 256 || (x & 14) >= 8)
      return 0xFF;
    int     offs = ((x & 0xF0) >> 3) | ((x & 0x04) >> 2);
    if ((x & 0x02) == 0)
      offs = offs | ((y & 0x07) << 8) | ((y & 0x38) << 2) | ((y & 0xC0) << 5);
    else
      offs = offs | ((y & 0xF8) << 2) | 0x1800;
    return videoRAMPtr[offs];
  }

  void ULA::setVideoPosition_()
  {
    irqPollEnableCallback(currentLine >= 247 && currentLine < 249);
    clearLineBuffer();
    if (currentLine >= 258) {
      if (currentSlot < 40 || currentSlot >= hSyncEndSlot)
        runOneSlot_ = &videoFunc_TBorder;
      else
        runOneSlot_ = &videoFunc_TBorderHBlank;
    }
    else if (currentLine < 192) {
      if (currentSlot >= hSyncEndSlot)
        runOneSlot_ = &videoFunc_DisplayLBorder;
      else if (currentSlot < 32)
        runOneSlot_ = &videoFunc_Display;
      else if (currentSlot < 40)
        runOneSlot_ = &videoFunc_DisplayRBorder;
      else
        runOneSlot_ = &videoFunc_DisplayHBlank;
    }
    else if (currentLine < 241) {
      if (currentSlot < 40 || currentSlot >= hSyncEndSlot)
        runOneSlot_ = &videoFunc_BBorder;
      else
        runOneSlot_ = &videoFunc_BBorderHBlank;
    }
    else {
      if (currentSlot < 40 || currentSlot >= hSyncEndSlot)
        runOneSlot_ = &videoFunc_VBlank;
      else
        runOneSlot_ = &videoFunc_VBlankHBlank;
    }
    if (currentSlot < hSyncEndSlot)
      lineBufPtr = &(lineBuf[(currentSlot + 8) << 1]);
    else
      lineBufPtr = &(lineBuf[(currentSlot - hSyncEndSlot) << 1]);
    ld1Ptr = videoRAMPtr + 0x1800;
    ld2Ptr = videoRAMPtr;
    if (currentLine < 192) {
      ld1Ptr = ld1Ptr + ((currentLine & 0xF8) << 2);
      ld2Ptr = ld2Ptr + (((currentLine & 0x07) << 8)
                         + ((currentLine & 0x38) << 2)
                         + ((currentLine & 0xC0) << 5));
    }
    bool    newVSyncFlag = (currentLine >= 243 && currentLine < 246);
    if (newVSyncFlag != vsyncFlag) {
      vsyncFlag = newVSyncFlag;
      vsyncStateChange(newVSyncFlag, 7);
    }
  }

  // --------------------------------------------------------------------------

  ULA::ULA(const uint8_t *videoRAMPtr_)
    : runOneSlot_(&videoFunc_TBorder),
      currentSlot(48),
      hSyncEndSlot(48),
      slotsPerLine(56),
      ioPortValue(0x00),
      borderColor(0x00),
      flashCnt(0x00),
      vsyncFlag(false),
      spectrum128Mode(false),
      currentLine(258),
      linesPerFrame(312),
      ld1Ptr(videoRAMPtr_ + 0x1800),
      ld2Ptr(videoRAMPtr_),
      lineBuf((uint8_t *) 0),
      lineBufPtr((uint8_t *) 0),
      videoRAMPtr(videoRAMPtr_),
      audioOutput(0),
      tapeInput(0),
      tapeOutput(0)
  {
    for (int i = 0; i < 8; i++)
      keyboardState[i] = 0xFF;
    uint32_t  *p = new uint32_t[57];    // for 228 bytes (57 * 4)
    lineBuf = reinterpret_cast<uint8_t *>(p);
    clearLineBuffer();
  }

  ULA::~ULA()
  {
    if (lineBuf) {
      delete[] reinterpret_cast<uint32_t *>(lineBuf);
      lineBuf = (uint8_t *) 0;
    }
  }

  void ULA::reset()
  {
    runOneSlot_ = &videoFunc_TBorder;
    currentSlot = hSyncEndSlot;
    flashCnt = 0x00;
    currentLine = 258;
    tapeOutput = 0;
    setVideoPosition_();
    writePort(0x00);
  }

  void ULA::setVideoPosition(size_t tStateCnt)
  {
    tStateCnt = tStateCnt + (size_t(slotsPerLine) * (248 * 4))
                - size_t(spectrum128Mode ? 1 : 3);
    currentSlot = uint8_t((tStateCnt / 4) % size_t(slotsPerLine));
    currentLine = int((tStateCnt / (size_t(slotsPerLine) * 4))
                      % size_t(linesPerFrame));
    if (currentSlot >= hSyncEndSlot)
      currentLine = (currentLine > 0 ? currentLine : linesPerFrame) - 1;
    setVideoPosition_();
  }

  void ULA::convertPixelToRGB(uint8_t pixelValue, float& r, float& g, float& b)
  {
    r = float((pixelValue & 0x02) >> 1);
    g = float((pixelValue & 0x04) >> 2);
    b = float(pixelValue & 0x01);
    if (!(pixelValue & 0x08)) {
      r *= 0.75f;
      g *= 0.75f;
      b *= 0.75f;
    }
  }

  uint8_t ULA::readPort(uint16_t addr) const
  {
    const uint8_t *p = &(keyboardState[7]);
    uint8_t tmp = ~(uint8_t(addr >> 8));
    uint8_t retval = uint8_t(audioOutput < 5120 ? 0xBF : 0xFF);
    while (tmp != 0) {
      if ((tmp & 0x80) != 0)
        retval = retval & (*p);
      tmp = (tmp & 0x7F) << 1;
      p--;
    }
    return retval;
  }

  void ULA::writePort(uint8_t value)
  {
    int     tmp = int(value & 0x18) - int(ioPortValue & 0x18);
    if (tmp != 0)
      tapeOutput = uint8_t(tmp >= 0);
    ioPortValue = value;
    borderColor = value & 0x07;
    updateAudioOutput();
  }

  uint8_t ULA::readPortDebug() const
  {
    return ioPortValue;
  }

  void ULA::setKeyboardState(int keyNum, bool isPressed)
  {
    int     row = (keyNum & 0x38) >> 3;
    uint8_t mask1 = uint8_t(1 << (keyNum & 0x07));
    uint8_t mask2 = (isPressed ? uint8_t(0x00) : mask1);
    keyboardState[row] = (keyboardState[row] & (~mask1)) | mask2 | 0xE0;
  }

  void ULA::setSpectrum128Mode(bool isSpectrum128)
  {
    if (currentSlot >= (hSyncEndSlot - 1)) {
      currentSlot = (currentSlot - uint8_t(spectrum128Mode))
                    + uint8_t(isSpectrum128);
    }
    spectrum128Mode = isSpectrum128;
    hSyncEndSlot = uint8_t(isSpectrum128) + 48;
    slotsPerLine = hSyncEndSlot + 8;
    if (currentSlot >= slotsPerLine)
      currentSlot = slotsPerLine - 1;
    linesPerFrame = 312 - int(isSpectrum128);
    if (currentLine >= linesPerFrame)
      currentLine = linesPerFrame - 1;
  }

  void ULA::drawLine(const uint8_t *buf, size_t nBytes)
  {
    (void) buf;
    (void) nBytes;
  }

  void ULA::vsyncStateChange(bool newState, unsigned int currentSlot_)
  {
    (void) newState;
    (void) currentSlot_;
  }

  void ULA::irqPollEnableCallback(bool isEnabled)
  {
    (void) isEnabled;
  }

  // --------------------------------------------------------------------------

  class ChunkType_ULASnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    ULA&    ref;
   public:
    ChunkType_ULASnapshot(ULA& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_ULASnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_ZXULA_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void ULA::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000000U);       // version number
    buf.writeByte(currentSlot);
    buf.writeByte(ioPortValue);
    buf.writeByte(flashCnt);
    buf.writeBoolean(tapeOutput != 0);
    buf.writeUInt32(uint32_t(currentLine));
    buf.writeBoolean(spectrum128Mode);
    for (int i = 0; i < 8; i++)
      buf.writeByte(keyboardState[i]);
  }

  void ULA::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_ZXULA_STATE, buf);
  }

  void ULA::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000U) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible ULA snapshot format");
    }
    try {
      // load saved state
      currentSlot = buf.readByte() & 0x3F;
      writePort(buf.readByte());
      flashCnt = buf.readByte() & 0xF8;
      tapeOutput = uint8_t(buf.readBoolean());
      currentLine = int(buf.readUInt32() & 0x01FFU);
      spectrum128Mode = buf.readBoolean();
      setSpectrum128Mode(spectrum128Mode);
      setVideoPosition_();
      for (int i = 0; i < 8; i++)
        keyboardState[i] = buf.readByte() | 0xE0;
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "ULA snapshot data");
    }
    catch (...) {
      // reset ULA
      setSpectrum128Mode(spectrum128Mode);
      for (int i = 0; i < 8; i++)
        keyboardState[i] = 0xFF;
      this->reset();
      throw;
    }
  }

  void ULA::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_ULASnapshot   *p = new ChunkType_ULASnapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace ZX128

