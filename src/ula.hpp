
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

#ifndef EP128EMU_ULA_HPP
#define EP128EMU_ULA_HPP

#include "ep128emu.hpp"

namespace ZX128 {

  class ULA {
   private:
    static const uint16_t audioOutputLevelTable[32];
    EP128EMU_REGPARM1 void (*runOneSlot_)(void *userData);
    uint8_t   currentSlot;
    uint8_t   hSyncEndSlot;             // 48K: 48, 128K: 49
    uint8_t   slotsPerLine;             // 48K: 56, 128K: 57
    uint8_t   ioPortValue;              // last value written to I/O port 0xFE
    uint8_t   borderColor;
    uint8_t   flashCnt;                 // incremented by 8 from 0 to 0xF8
    bool      vsyncFlag;
    bool      spectrum128Mode;
    int       currentLine;
    int       linesPerFrame;
    const uint8_t *ld1Ptr;              // attribute data pointer
    const uint8_t *ld2Ptr;              // bitmap data pointer
    uint8_t   *lineBuf;                 // 57 slots = 456 pixels
    uint8_t   *lineBufPtr;
    const uint8_t *videoRAMPtr;         // pointer to video RAM segment
    uint16_t  audioOutput;
    uint8_t   tapeInput;
    uint8_t   tapeOutput;
    uint8_t   keyboardState[8];
    // --------
    static EP128EMU_REGPARM1 void videoFunc_TBorder(void *userData);
    static EP128EMU_REGPARM1 void videoFunc_TBorderHBlank(void *userData);
    static EP128EMU_REGPARM1 void videoFunc_DisplayLBorder(void *userData);
    static EP128EMU_REGPARM1 void videoFunc_Display(void *userData);
    static EP128EMU_REGPARM1 void videoFunc_DisplayRBorder(void *userData);
    static EP128EMU_REGPARM1 void videoFunc_DisplayHBlank(void *userData);
    static EP128EMU_REGPARM1 void videoFunc_BBorder(void *userData);
    static EP128EMU_REGPARM1 void videoFunc_BBorderHBlank(void *userData);
    static EP128EMU_REGPARM1 void videoFunc_VBlank(void *userData);
    static EP128EMU_REGPARM1 void videoFunc_VBlankHBlank(void *userData);
    void clearLineBuffer();
    EP128EMU_REGPARM2 bool getInterruptFlag_(int timeOffs) const;
    EP128EMU_REGPARM2 int getWaitHalfCycles_(int timeOffs) const;
    EP128EMU_REGPARM2 uint8_t idleDataBusRead_(int timeOffs) const;
    void setVideoPosition_();
    inline void updateAudioOutput()
    {
      audioOutput = audioOutputLevelTable[(ioPortValue & 0x18) | tapeInput];
    }
   public:
    ULA(const uint8_t *videoRAMPtr_);
    virtual ~ULA();
    void reset();
    inline void setVideoMemory(const uint8_t *videoRAMPtr_)
    {
      videoRAMPtr = videoRAMPtr_;
    }
    inline void runOneSlot()
    {
      runOneSlot_((void *) this);
    }
    inline uint16_t getSoundOutput() const
    {
      return audioOutput;
    }
    inline void setTapeInput(int tapeInput_)
    {
      tapeInput = uint8_t(tapeInput_ > 0);
      updateAudioOutput();
    }
    inline int getTapeOutput() const
    {
      return int(tapeOutput > 0);
    }
    inline void getVideoPosition(int& xPos, int& yPos) const
    {
      xPos = int(currentSlot) << 3;
      yPos = currentLine;
    }
    EP128EMU_INLINE void getVideoPosition(int& xPos, int& yPos,
                                          int timeOffs) const
    {
      int     tmpY = currentLine;
      if (timeOffs < 0) {
        timeOffs = timeOffs + (int(slotsPerLine) << 3);
        tmpY = (tmpY > 0 ? tmpY : linesPerFrame) - 1;
      }
      uint8_t tmpX = currentSlot + uint8_t(timeOffs >> 3);
      if (currentSlot < hSyncEndSlot) {
        if (tmpX >= hSyncEndSlot) {
          tmpX = (tmpX < slotsPerLine ? tmpX : (tmpX - slotsPerLine));
          tmpY = (tmpY < (linesPerFrame - 1) ? (tmpY + 1) : 0);
        }
      }
      else if (tmpX >= slotsPerLine) {
        tmpX = tmpX - slotsPerLine;
        if (tmpX >= hSyncEndSlot)
          tmpY = (tmpY < (linesPerFrame - 1) ? (tmpY + 1) : 0);
      }
      xPos = (int(tmpX) << 3) + (timeOffs & 7);
      yPos = tmpY;
    }
    EP128EMU_INLINE bool getInterruptFlag(int timeOffs = 0) const
    {
      if (EP128EMU_EXPECT(currentLine < 247 || currentLine >= 250))
        return false;
      return getInterruptFlag_(timeOffs);
    }
    EP128EMU_INLINE int getWaitHalfCycles(int timeOffs = 0) const
    {
      if (currentLine >= 193 && currentLine < 310)
        return 0;
      return getWaitHalfCycles_(timeOffs);
    }
    EP128EMU_INLINE uint8_t idleDataBusRead(int timeOffs = 0) const
    {
      if (currentLine >= 193 && currentLine < 310)
        return 0xFF;
      return idleDataBusRead_(timeOffs);
    }
    void setVideoPosition(size_t tStateCnt);
    static void convertPixelToRGB(uint8_t pixelValue,
                                  float& r, float& g, float& b);
    uint8_t readPort(uint16_t portNum) const;
    void writePort(uint8_t value);
    uint8_t readPortDebug() const;
    //        +-----+-----+-----+-----+-----+
    //        |  0  |  1  |  2  |  3  |  4  |
    // +------+-----+-----+-----+-----+-----+
    // | 0x00 | CAP |  Z  |  X  |  C  |  V  |
    // +------+-----+-----+-----+-----+-----+
    // | 0x08 |  A  |  S  |  D  |  F  |  G  |
    // +------+-----+-----+-----+-----+-----+
    // | 0x10 |  Q  |  W  |  E  |  R  |  T  |
    // +------+-----+-----+-----+-----+-----+
    // | 0x18 |  1  |  2  |  3  |  4  |  5  |
    // +------+-----+-----+-----+-----+-----+
    // | 0x20 |  0  |  9  |  8  |  7  |  6  |
    // +------+-----+-----+-----+-----+-----+
    // | 0x28 |  P  |  O  |  I  |  U  |  Y  |
    // +------+-----+-----+-----+-----+-----+
    // | 0x30 | ENT |  L  |  K  |  J  |  H  |
    // +------+-----+-----+-----+-----+-----+
    // | 0x38 | SPC | SYM |  M  |  N  |  B  |
    // +------+-----+-----+-----+-----+-----+
    void setKeyboardState(int keyNum, bool isPressed);
    void setSpectrum128Mode(bool isSpectrum128);
   protected:
    /*!
     * drawLine() is called after rendering each line.
     * 'buf' defines a line of 768 pixels, as 48 groups of 16 pixels each,
     * in the following format: the first byte defines the number of
     * additional bytes that encode the 16 pixels to be displayed. The data
     * length also determines the pixel format, and can have the following
     * values:
     *   0x01: one 8-bit color index (pixel width = 16)
     *   0x02: two 8-bit color indices (pixel width = 8)
     *   0x03: two 8-bit color indices for background (bit value = 0) and
     *         foreground (bit value = 1) color, followed by a 8-bit bitmap
     *         (msb first, pixel width = 2)
     *   0x04: four 8-bit color indices (pixel width = 4)
     *   0x06: similar to 0x03, but there are two sets of colors/bitmap
     *         (c0a, c1a, bitmap_a, c0b, c1b, bitmap_b) and the pixel width
     *         is 1
     *   0x08: eight 8-bit color indices (pixel width = 2)
     * The buffer is aligned to 4 bytes, and contains 'nBytes' (in the range
     * of 96 to 432) bytes of data.
     */
    virtual void drawLine(const uint8_t *buf, size_t nBytes);
    /*!
     * Called at the beginning (newState = true) and end (newState = false)
     * of VSYNC. 'currentSlot_' is the position within the current line
     * (0 to 56).
     */
    virtual void vsyncStateChange(bool newState, unsigned int currentSlot_);
    /*!
     * Called at lines 247 and 249 to signal if the state of the /INT line
     * should be checked by calling getInterruptFlag() at the end of every
     * Z80 instruction.
     */
    virtual void irqPollEnableCallback(bool isEnabled);
   public:
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
  };

}       // namespace ZX128

#endif  // EP128EMU_ULA_HPP

