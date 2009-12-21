
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
#include "crtc6845.hpp"

namespace CPC464 {

  CRTC6845::CRTC6845()
  {
    for (int i = 0; i < 16; i++)
      registers[i] = 0x00;
    registers[16] = 0xFF;
    registers[17] = 0xFF;
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
    cursorAddress =
        uint16_t(registers[15]) | (uint16_t(registers[14] & 0x3F) << 8);
    cursorFlags = 0x03;
    cursorFlashCnt = 0;
    endOfFrameFlag = false;
  }

  EP128EMU_REGPARM1 void CRTC6845::runOneCycle()
  {
    if (horizontalPos == registers[0]) {
      // end of line
      horizontalPos = 0;
      displayEnableFlags = displayEnableFlags | 0x01;
      syncFlags = syncFlags & 0x02;
      hSyncCnt = 0;
      characterAddress = characterAddressLatch;
      bool    interlaceMode = ((registers[8] & 0x03) == 0x03);
      uint8_t rowAddressMask = 0x1F - uint8_t(interlaceMode);
      if (((rowAddress ^ registers[11]) & rowAddressMask) == 0) {
        // cursor end line
        cursorFlags = cursorFlags | 0x01;
      }
      // increment row address
      if (((rowAddress ^ registers[9]) & rowAddressMask) == 0) {
        // character end line
        rowAddress = uint8_t(interlaceMode && oddField);
        endOfFrameFlag = (verticalPos == registers[4]);
        verticalPos = (verticalPos + 1) & 0xFF;
      }
      else {
        rowAddress = (rowAddress + (uint8_t(interlaceMode) + 1)) & 0x1F;
      }
      if (endOfFrameFlag &&
          ((rowAddress ^ registers[5]) & rowAddressMask) == 0) {
        // end of frame
        endOfFrameFlag = false;
        displayEnableFlags = displayEnableFlags | 0x02;
        syncFlags = syncFlags & 0x01;
        vSyncCnt = 0;
        oddField = !oddField;
        rowAddress = uint8_t(interlaceMode && oddField);
        verticalPos = 0;
        characterAddress =
            uint16_t(registers[13]) | (uint16_t(registers[12] & 0x3F) << 8);
        characterAddressLatch = characterAddress;
        cursorFlashCnt = (cursorFlashCnt + 1) & 0xFF;
        switch (registers[10] & 0x60) {
        case 0x00:                      // no cursor flash
          cursorFlags = 0x01;
          break;
        case 0x20:                      // cursor disabled
          cursorFlags = 0x03;
          break;
        case 0x40:                      // cursor flash (16 frames)
          cursorFlags = ((cursorFlashCnt & 0x08) >> 2) | 0x01;
          break;
        case 0x60:                      // cursor flash (32 frames)
          cursorFlags = ((cursorFlashCnt & 0x10) >> 3) | 0x01;
          break;
        }
      }
      if (((rowAddress ^ registers[10]) & rowAddressMask) == 0) {
        // cursor start line
        cursorFlags = cursorFlags & 0x02;
      }
      if (vSyncCnt > 0) {
        // vertical sync
        if (--vSyncCnt == 0)
          syncFlags = syncFlags & 0x01;
      }
      if ((verticalPos >> uint8_t(interlaceMode)) == registers[6]) {
        // display end / vertical
        displayEnableFlags = displayEnableFlags & 0x01;
      }
      if (verticalPos == registers[7]) {
        // vertical sync start
        syncFlags = syncFlags | 0x02;
#if 0
        vSyncCnt = (((registers[3] >> 4) - 1) & 0x0F) + 1;
#else
        vSyncCnt = 16;                  // VSYNC length is fixed on the 6845
#endif
      }
    }
    else {
      horizontalPos = (horizontalPos + 1) & 0xFF;
      characterAddress = (characterAddress + 1) & 0x3FFF;
    }
    if (hSyncCnt > 0) {
      // horizontal sync
      if (--hSyncCnt == 0)
        syncFlags = syncFlags & 0x02;
    }
    if (horizontalPos == registers[1]) {
      // display end / horizontal
      displayEnableFlags = displayEnableFlags & 0x02;
      if (((rowAddress ^ registers[9])
           & (0x1F - uint8_t((registers[8] & 0x03) == 0x03))) == 0) {
        characterAddressLatch = characterAddress;
      }
    }
    if (horizontalPos == registers[2]) {
      // horizontal sync start
      syncFlags = syncFlags | 0x01;
      hSyncCnt = ((registers[3] - 1) & 0x0F) + 1;
    }
  }

  EP128EMU_REGPARM2 uint8_t CRTC6845::readRegister(uint16_t addr) const
  {
    addr = addr & 0x001F;
    if (addr < 18)
      return registers[addr];
    return 0x00;
  }

  EP128EMU_REGPARM3 void CRTC6845::writeRegister(uint16_t addr, uint8_t value)
  {
    addr = addr & 0x001F;
    if (addr < 18) {
      switch (addr) {

      default:
        registers[addr] = value;
        break;
      }
    }
  }

}       // namespace CPC464

