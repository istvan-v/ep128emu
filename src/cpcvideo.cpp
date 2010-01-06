
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
#include "cpcvideo.hpp"

static const uint8_t  pixelConvTable_4[137] = {
   0,  2,  2,  0,   2,  0,  0,  0,   2,  0,  0,  0,   0,  0,  0,  0,
   1,  3,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   1,  0,  3,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   1,  0,  0,  0,   3,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   1,  0,  0,  0,   0,  0,  0,  0,   3
};

static const uint8_t  pixelConvTable_16[171] = {
   0,  8,  8,  0,   2, 10,  0,  0,   2,  0, 10,  0,   0,  0,  0,  0,
   4, 12,  0,  0,   6, 14,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   4,  0, 12,  0,   0,  0,  0,  0,   6,  0, 14,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   1,  9,  0,  0,   3, 11,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   5, 13,  0,  0,   7, 15,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   1,  0,  9,  0,   0,  0,  0,  0,   3,  0, 11,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   5,  0, 13,  0,   0,  0,  0,  0,   7,  0, 15
};

static const int16_t cpcColorTable[32] = {
  // RGB     RGB     RGB     RGB
  0x0111, 0x0111, 0x0021, 0x0221,   // white, white, sea green, pastel yellow
  0x0001, 0x0201, 0x0011, 0x0211,   // blue, purple, cyan, pink
  0x0201, 0x0221, 0x0220, 0x0222,   // purple, p. yellow, b. yellow, b. white
  0x0200, 0x0202, 0x0210, 0x0212,   // b. red, b. magenta, orange, p. magenta
  0x0001, 0x0021, 0x0020, 0x0022,   // blue, sea green, bright green, b. cyan
  0x0000, 0x0002, 0x0010, 0x0012,   // black, bright blue, green, sky blue
  0x0101, 0x0121, 0x0120, 0x0122,   // magenta, pastel green, lime, pastel cyan
  0x0100, 0x0102, 0x0110, 0x0112    // red, mauve, yellow, pastel blue
};

namespace CPC464 {

  void CPCVideo::drawLine(const uint8_t *buf, size_t nBytes)
  {
    (void) buf;
    (void) nBytes;
  }

  void CPCVideo::vsyncStateChange(bool newState, unsigned int currentSlot_)
  {
    (void) newState;
    (void) currentSlot_;
  }

  // --------------------------------------------------------------------------

  CPCVideo::CPCVideo(const CRTC6845& crtc_, const uint8_t *videoMemory_)
    : crtc(crtc_),
      lineBufPtr((uint8_t *) 0),
      oldLineBufPtr((uint8_t *) 0),
      prvHSyncState(false),
      prvVSyncState(false),
      videoMode(0),
      borderColor(0x00),
      hSyncCnt(0),
      videoMemory(videoMemory_),
      lineBuf((uint8_t *) 0)
  {
    for (size_t i = 0; i < 16; i++)
      palette[i] = 0x00;
    lineBuf = reinterpret_cast<uint8_t *>(new uint32_t[162]);
    for (size_t i = 0; i < 648; i++)
      lineBuf[i] = uint8_t((~i) & 0x01);
    lineBufPtr = lineBuf;
    oldLineBufPtr = lineBuf;
  }

  CPCVideo::~CPCVideo()
  {
    delete[] reinterpret_cast<uint32_t *>(lineBuf);
  }

  void CPCVideo::convertPixelToRGB(uint8_t pixelValue,
                                   float& r, float& g, float& b)
  {
    r = float(cpcColorTable[pixelValue & 0x1F] & 0x0300) / 512.0f;
    g = float(cpcColorTable[pixelValue & 0x1F] & 0x0030) / 32.0f;
    b = float(cpcColorTable[pixelValue & 0x1F] & 0x0003) / 2.0f;
  }

  void CPCVideo::setColor(uint8_t penNum, uint8_t c)
  {
    if (penNum & 0x10)
      borderColor = c & 0x3F;
    else
      palette[penNum & 0x0F] = c & 0x3F;
  }

  uint8_t CPCVideo::getColor(uint8_t penNum) const
  {
    if (penNum & 0x10)
      return borderColor;
    return palette[penNum & 0x0F];
  }

  EP128EMU_REGPARM1 void CPCVideo::runOneCycle()
  {
    oldLineBufPtr = lineBufPtr;
    bool    hSyncState = crtc.getHSyncState();
    bool    vSyncState = crtc.getVSyncState();
    if (hSyncState | vSyncState) {
      lineBufPtr[0] = 0x01;             // sync
      lineBufPtr[1] = 0x00;
      lineBufPtr = lineBufPtr + 2;
    }
    else if (crtc.getDisplayEnabled()) {
      unsigned int  videoAddr =
          ((unsigned int) (crtc.getRowAddress() & 0x07) << 11)
          | ((unsigned int) (crtc.getMemoryAddress() & 0x03FF) << 1)
          | ((unsigned int) (crtc.getMemoryAddress() & 0x3000) << 2);
      switch (videoMode) {
      case 0:                           // 16 color mode
        {
          lineBufPtr[0] = 0x04;
          uint8_t b = videoMemory[videoAddr];
          lineBufPtr[1] = palette[pixelConvTable_16[b & 0xAA]];
          lineBufPtr[2] = palette[pixelConvTable_16[b & 0x55]];
          b = videoMemory[videoAddr + 1];
          lineBufPtr[3] = palette[pixelConvTable_16[b & 0xAA]];
          lineBufPtr[4] = palette[pixelConvTable_16[b & 0x55]];
          lineBufPtr = lineBufPtr + 5;
        }
        break;
      case 1:                           // 4 color mode
        {
          lineBufPtr[0] = 0x08;
          uint8_t b = videoMemory[videoAddr];
          lineBufPtr[1] = palette[pixelConvTable_4[b & 0x88]];
          lineBufPtr[2] = palette[pixelConvTable_4[b & 0x44]];
          lineBufPtr[3] = palette[pixelConvTable_4[b & 0x22]];
          lineBufPtr[4] = palette[pixelConvTable_4[b & 0x11]];
          b = videoMemory[videoAddr + 1];
          lineBufPtr[5] = palette[pixelConvTable_4[b & 0x88]];
          lineBufPtr[6] = palette[pixelConvTable_4[b & 0x44]];
          lineBufPtr[7] = palette[pixelConvTable_4[b & 0x22]];
          lineBufPtr[8] = palette[pixelConvTable_4[b & 0x11]];
          lineBufPtr = lineBufPtr + 9;
        }
        break;
      case 2:                           // 2 color mode
        lineBufPtr[0] = 0x06;
        lineBufPtr[1] = palette[0];
        lineBufPtr[2] = palette[1];
        lineBufPtr[3] = videoMemory[videoAddr];
        lineBufPtr[4] = palette[0];
        lineBufPtr[5] = palette[1];
        lineBufPtr[6] = videoMemory[videoAddr + 1];
        lineBufPtr = lineBufPtr + 7;
        break;
      case 3:                           // 4 color mode (half resolution)
        {
          lineBufPtr[0] = 0x04;
          uint8_t b = videoMemory[videoAddr];
          lineBufPtr[1] = palette[pixelConvTable_16[b & 0xAA] & 0x03];
          lineBufPtr[2] = palette[pixelConvTable_16[b & 0x55] & 0x03];
          b = videoMemory[videoAddr + 1];
          lineBufPtr[3] = palette[pixelConvTable_16[b & 0xAA] & 0x03];
          lineBufPtr[4] = palette[pixelConvTable_16[b & 0x55] & 0x03];
          lineBufPtr = lineBufPtr + 5;
        }
        break;
      }
    }
    else {
      lineBufPtr[0] = 0x01;             // border
      lineBufPtr[1] = borderColor;
      lineBufPtr = lineBufPtr + 2;
    }
    if (hSyncCnt == 48)
      drawLine(lineBuf, size_t(lineBufPtr - lineBuf));
    if (hSyncState && !prvHSyncState) { // horizontal sync
      if (hSyncCnt >= 48)
        hSyncCnt = -10;
    }
    if (hSyncCnt == 0)
      lineBufPtr = lineBuf;
    if (hSyncCnt >= 60)
      hSyncCnt = -10;
    if (vSyncState != prvVSyncState)
      vsyncStateChange(vSyncState, (crtc.getVSyncInterlace() ? 34U : 6U));
    prvHSyncState = hSyncState;
    prvVSyncState = vSyncState;
  }

  void CPCVideo::reset()
  {
    videoMode = 0;
    borderColor = 0x00;
    for (size_t i = 0; i < 16; i++)
      palette[i] = 0x00;
  }

}       // namespace CPC464

