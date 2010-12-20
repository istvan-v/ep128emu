
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

#ifndef EP128EMU_CRTC6845_HPP
#define EP128EMU_CRTC6845_HPP

#include "ep128emu.hpp"

namespace CPC464 {

  class CRTC6845 {
   protected:
    // register  0: horizontal total - 1
    // register  1: horizontal displayed
    // register  2: horizontal sync position
    // register  3: (Vsync_lines & 15) * 16 + (Hsync_chars & 15)
    // register  4: vertical total - 1 (in characters)
    // register  5: vertical total adjust (in scanlines)
    // register  6: vertical displayed (in characters, / 2 if interlaced video)
    // register  7: vertical sync position (in characters)
    // register  8: mode control
    //                0, 1: non-interlaced mode
    //                2:    interlace VSYNC only
    //                3:    interlaced video mode
    // register  9: scanlines per character - 1 (0 to 31)
    // register 10: cursor start line (0 to 31)
    //                +  0: no flash
    //                + 32: cursor not displayed
    //                + 64: flash (16 frames)
    //                + 96: flash (32 frames)
    // register 11: cursor end line (0 to 31)
    // register 12: start address high (bits 8 to 13)
    // register 13: start address low (bits 0 to 7)
    // register 14: cursor address high (bits 8 to 13)
    // register 15: cursor address low (bits 0 to 7)
    // register 16: lightpen address high (bits 8 to 13)
    // register 17: lightpen address low (bits 0 to 7)
    uint8_t     registers[18];
    uint8_t     horizontalPos;
    // bit 1: 1: display enabled (b6=1 and b7=1)
    // bit 6: 1: display enabled H
    // bit 7: 1: display enabled V
    uint8_t     displayEnableFlags;
    // bit 0: horizontal sync
    // bit 1: vertical sync
    uint8_t     syncFlags;
    uint8_t     hSyncCnt;
    uint8_t     vSyncCnt;
    uint8_t     rowAddress;             // 0 to 31
    uint8_t     verticalPos;
    bool        oddField;               // for interlace
    uint16_t    characterAddress;       // 0 to 0x3FFF
    uint16_t    characterAddressLatch;
    // bits 0-13: copied from registers 14 and 15
    // bit 14:    0 if row address is in cursor line range
    // bit 15:    0 if cursor enable / flash is on
    uint16_t    cursorAddress;
    uint8_t     rowAddressMask;         // 0x1E if R8 == 3, 0x1F otherwise
    uint8_t     cursorFlashCnt;
    // 0x00 = last character row was not reached yet
    // 0x80 = start vertical total adjust in next line
    // 0x01 = vertical total adjust is active
    uint8_t     endOfFrameFlag;
    uint8_t     verticalTotalAdjustLatched;
    // bit 0: current cursor enable output
    // bit 1: current display enable output
    // bit 2: cursor enable delayed by one cycle
    // bit 3: display enable delayed by one cycle
    // bit 4: cursor enable delayed by two cycles
    // bit 5: display enable delayed by two cycles
    uint8_t     skewShiftRegister;
    // 2, 8, 32, or 0 depending on bits 4 and 5 of register 8
    uint8_t     displayEnableMask;
    // 1, 4, 16, or 0 depending on bits 6 and 7 of register 8
    uint8_t     cursorEnableMask;
    EP128EMU_REGPARM2 void (*hSyncStateChangeCallback)(void *userData,
                                                       bool newState);
    void        *hSyncChangeCallbackUserData;
    EP128EMU_REGPARM2 void (*vSyncStateChangeCallback)(void *userData,
                                                       bool newState);
    void        *vSyncChangeCallbackUserData;
    // --------
    EP128EMU_REGPARM1 void checkFrameEnd();
    EP128EMU_REGPARM1 void lineEnd();
    EP128EMU_REGPARM2 void updateHSyncFlag(bool newState);
    EP128EMU_REGPARM2 void updateVSyncFlag(bool newState);
   public:
    CRTC6845();
    virtual ~CRTC6845();
    void reset();
    EP128EMU_INLINE void runOneCycle()
    {
      if (EP128EMU_UNLIKELY(horizontalPos == this->registers[0])) {
        // end of line
        this->lineEnd();
      }
      else {
        horizontalPos = (horizontalPos + 1) & 0xFF;
        characterAddress = (characterAddress + 1) & 0x3FFF;
      }
      if (syncFlags & 0x01) {
        // horizontal sync
        if (!((++hSyncCnt ^ this->registers[3]) & 0x0F))
          updateHSyncFlag(false);
      }
      if (horizontalPos == this->registers[1]) {
        // display end / horizontal
        displayEnableFlags = displayEnableFlags & 0x80;
        if (((rowAddress ^ this->registers[9]) & rowAddressMask) == 0)
          characterAddressLatch = characterAddress;
      }
      if (horizontalPos == this->registers[2]) {
        // horizontal sync start
        updateHSyncFlag(true);
        hSyncCnt = 0;
        if (EP128EMU_UNLIKELY(!(this->registers[3] & 0x0F)))
          updateHSyncFlag(false);       // HSync width = 0
      }
      skewShiftRegister = (skewShiftRegister << 2) | displayEnableFlags
                          | uint8_t(characterAddress == cursorAddress);
    }
    EP128EMU_INLINE bool getDisplayEnabled() const
    {
      return bool(skewShiftRegister & displayEnableMask);
    }
    EP128EMU_INLINE bool getHSyncState() const
    {
      return bool(syncFlags & 0x01);
    }
    EP128EMU_INLINE bool getVSyncState() const
    {
      return bool(syncFlags & 0x02);
    }
    EP128EMU_INLINE bool getVSyncInterlace() const
    {
      return (bool(this->registers[8] & 0x01) && oddField);
    }
    EP128EMU_INLINE uint8_t getRowAddress() const
    {
      return rowAddress;
    }
    EP128EMU_INLINE uint16_t getMemoryAddress() const
    {
      return characterAddress;
    }
    EP128EMU_INLINE bool getCursorEnabled() const
    {
      return bool(skewShiftRegister & cursorEnableMask);
    }
    EP128EMU_INLINE void getVideoPosition(int& xPos, int& yPos) const
    {
      xPos = horizontalPos;
      yPos = (int(verticalPos)
              * (int(this->registers[9] | (0x1F - rowAddressMask)) + 1))
             + int(rowAddress);
    }
    EP128EMU_INLINE void setLightPenPosition(uint16_t addr)
    {
      this->registers[16] = uint8_t((addr >> 8) & 0x3F);
      this->registers[17] = uint8_t(addr & 0xFF);
    }
    EP128EMU_REGPARM2 uint8_t readRegister(uint16_t addr) const;
    EP128EMU_REGPARM3 void writeRegister(uint16_t addr, uint8_t value);
    EP128EMU_REGPARM2 uint8_t readRegisterDebug(uint16_t addr) const;
    void setHSyncStateChangeCallback(
        EP128EMU_REGPARM2 void (*func)(void *userData, bool newState),
        void *userData_);
    void setVSyncStateChangeCallback(
        EP128EMU_REGPARM2 void (*func)(void *userData, bool newState),
        void *userData_);
    // --------
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
  };

}       // namespace CPC464

#endif  // EP128EMU_CRTC6845_HPP

