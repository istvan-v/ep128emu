
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
      hSyncCnt(1),
      crtcHSyncCnt(0),
      crtcHSyncState(0),
      vSyncCnt(0),
      videoModeLatched(0),
      videoMemory(videoMemory_),
      lineBuf((uint8_t *) 0),
      borderColor(0x00),
      videoMode(0),
      hSyncMax(107),
      hSyncLen(4)
  {
    videoDelayBuf[0] = 0U;
    videoDelayBuf[1] = 0U;
    for (size_t i = 0; i < 16; i++)
      palette[i] = 0x00;
    lineBuf = reinterpret_cast<uint8_t *>(new uint32_t[112]);
    for (size_t i = 0; i < 448; i++)
      lineBuf[i] = uint8_t((~i) & 0x01);
    lineBufPtr = lineBuf;
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
    if (EP128EMU_EXPECT((unsigned int) hSyncCnt < 97U)) {
      if (EP128EMU_UNLIKELY((reinterpret_cast<unsigned char *>(
                                 &(videoDelayBuf[0])))[0] != 0)) {
        lineBufPtr[0] = 0x01;           // sync
        lineBufPtr[1] = 0x14;
        lineBufPtr = lineBufPtr + 2;
      }
      else if ((reinterpret_cast<unsigned char *>(&(videoDelayBuf[0])))[1]) {
        uint8_t videoByte0 =
            (reinterpret_cast<unsigned char *>(&(videoDelayBuf[0])))[2];
        uint8_t videoByte1 =
            (reinterpret_cast<unsigned char *>(&(videoDelayBuf[0])))[3];
        switch (videoModeLatched) {
        case 0:                         // 16 color mode
          {
            lineBufPtr[0] = 0x04;
            lineBufPtr[1] = palette[pixelConvTable_16[videoByte0 & 0xAA]];
            lineBufPtr[2] = palette[pixelConvTable_16[videoByte0 & 0x55]];
            lineBufPtr[3] = palette[pixelConvTable_16[videoByte1 & 0xAA]];
            lineBufPtr[4] = palette[pixelConvTable_16[videoByte1 & 0x55]];
            lineBufPtr = lineBufPtr + 5;
          }
          break;
        case 1:                         // 4 color mode
          {
            lineBufPtr[0] = 0x08;
            lineBufPtr[1] = palette[pixelConvTable_4[videoByte0 & 0x88]];
            lineBufPtr[2] = palette[pixelConvTable_4[videoByte0 & 0x44]];
            lineBufPtr[3] = palette[pixelConvTable_4[videoByte0 & 0x22]];
            lineBufPtr[4] = palette[pixelConvTable_4[videoByte0 & 0x11]];
            lineBufPtr[5] = palette[pixelConvTable_4[videoByte1 & 0x88]];
            lineBufPtr[6] = palette[pixelConvTable_4[videoByte1 & 0x44]];
            lineBufPtr[7] = palette[pixelConvTable_4[videoByte1 & 0x22]];
            lineBufPtr[8] = palette[pixelConvTable_4[videoByte1 & 0x11]];
            lineBufPtr = lineBufPtr + 9;
          }
          break;
        case 2:                         // 2 color mode
          lineBufPtr[0] = 0x06;
          lineBufPtr[1] = palette[0];
          lineBufPtr[2] = palette[1];
          lineBufPtr[3] = videoByte0;
          lineBufPtr[4] = palette[0];
          lineBufPtr[5] = palette[1];
          lineBufPtr[6] = videoByte1;
          lineBufPtr = lineBufPtr + 7;
          break;
        case 3:                         // 4 color mode (half resolution)
          {
            lineBufPtr[0] = 0x04;
            lineBufPtr[1] = palette[pixelConvTable_16[videoByte0 & 0xAA] & 3];
            lineBufPtr[2] = palette[pixelConvTable_16[videoByte0 & 0x55] & 3];
            lineBufPtr[3] = palette[pixelConvTable_16[videoByte1 & 0xAA] & 3];
            lineBufPtr[4] = palette[pixelConvTable_16[videoByte1 & 0x55] & 3];
            lineBufPtr = lineBufPtr + 5;
          }
          break;
        }
      }
      else {
        lineBufPtr[0] = 0x01;           // border
        lineBufPtr[1] = borderColor;
        lineBufPtr = lineBufPtr + 2;
      }
    }
    hSyncCnt += 2;
    if (EP128EMU_UNLIKELY(crtcHSyncCnt)) {      // horizontal sync
      if (crtcHSyncCnt == 3) {
        if (hSyncCnt >= (int(hSyncMax) - 8)) {
          if (EP128EMU_EXPECT(hSyncCnt & 1))
            drawLine(lineBuf, size_t(lineBufPtr - lineBuf));
          else
            shiftLineBuffer();
          lineBufPtr = lineBuf;
          hSyncCnt = -21 - int(hSyncLen);
          hSyncMax = 111 - hSyncLen;
        }
      }
      else if (crtcHSyncCnt == 7) {
        videoModeLatched = videoMode;
        hSyncLen = 4;
        if (vSyncCnt) {
          if (--vSyncCnt == 21)         // VSync start (delayed by 5 lines)
            vsyncStateChange(true, (crtc.getVSyncInterlace() ? 34U : 6U));
          else if (vSyncCnt == 15)      // VSync end
            vsyncStateChange(false, 6U);
        }
        crtcHSyncCnt = 0xFF;            // will overflow to 0
      }
      crtcHSyncCnt++;
    }
    if ((unsigned int) (hSyncCnt + 4) >= 101U) {
      if (EP128EMU_UNLIKELY(hSyncCnt >= int(hSyncMax))) {
        if (EP128EMU_EXPECT(hSyncCnt & 1))
          drawLine(lineBuf, size_t(lineBufPtr - lineBuf));
        else
          shiftLineBuffer();
        lineBufPtr = lineBuf;
        hSyncCnt = int(hSyncMax) - 132;
      }
      return;
    }
    videoDelayBuf[0] = videoDelayBuf[1];
    (reinterpret_cast<unsigned char *>(&(videoDelayBuf[0])))[0] =
        crtcHSyncState | vSyncCnt;
    bool    displayEnabled = crtc.getDisplayEnabled();
    (reinterpret_cast<unsigned char *>(&(videoDelayBuf[0])))[5] =
        uint8_t(displayEnabled);
    if (displayEnabled) {
      unsigned int  videoAddr =
          ((unsigned int) (crtc.getRowAddress() & 0x07) << 11)
          | ((unsigned int) (crtc.getMemoryAddress() & 0x03FF) << 1)
          | ((unsigned int) (crtc.getMemoryAddress() & 0x3000) << 2);
      (reinterpret_cast<unsigned char *>(&(videoDelayBuf[0])))[6] =
          videoMemory[videoAddr];
      (reinterpret_cast<unsigned char *>(&(videoDelayBuf[0])))[7] =
          videoMemory[videoAddr + 1U];
    }
  }

  EP128EMU_REGPARM1 void CPCVideo::shiftLineBuffer()
  {
    uint32_t  tmpBuf[108];              // 432 bytes (48 characters * 9)
    uint8_t   *p = reinterpret_cast<uint8_t *>(&(tmpBuf[0]));
    const uint8_t *p1 = &(lineBuf[0]);
    uint8_t   flagByte1 = *p1;
    const uint8_t *p2 = p1 + (int(flagByte1) + 1);
    for (int i = 0; i < 48; i++) {
      uint8_t flagByte2 = *p2;
      switch ((flagByte1 << 2) ^ flagByte2) {
      case 0x05:                        // 1, 1
        p[0] = 0x02;
        p[1] = p1[1];
        p[2] = p2[1];
        p = p + 3;
        p1 = p2;
        p2 = p2 + 2;
        break;
      case 0x00:                        // 1, 4
        p[0] = 0x04;
        {
          uint8_t tmp = p1[1];
          p[1] = tmp;
          p[2] = tmp;
        }
        p[3] = p2[1];
        p[4] = p2[2];
        p = p + 5;
        p1 = p2;
        p2 = p2 + 5;
        break;
      case 0x02:                        // 1, 6
        p[0] = 0x06;
        p[1] = p1[1];
        p[2] = 0x00;
        p[3] = 0x00;
        p[4] = p2[1];
        p[5] = p2[2];
        p[6] = p2[3];
        p = p + 7;
        p1 = p2;
        p2 = p2 + 7;
        break;
      case 0x0C:                        // 1, 8
        p[0] = 0x08;
        {
          uint8_t tmp = p1[1];
          p[1] = tmp;
          p[2] = tmp;
          p[3] = tmp;
          p[4] = tmp;
        }
        p[5] = p2[1];
        p[6] = p2[2];
        p[7] = p2[3];
        p[8] = p2[4];
        p = p + 9;
        p1 = p2;
        p2 = p2 + 9;
        break;
      case 0x11:                        // 4, 1
        p[0] = 0x04;
        p[1] = p1[3];
        p[2] = p1[4];
        {
          uint8_t tmp = p2[1];
          p[3] = tmp;
          p[4] = tmp;
        }
        p = p + 5;
        p1 = p2;
        p2 = p2 + 2;
        break;
      case 0x14:                        // 4, 4
        p[0] = 0x04;
        p[1] = p1[3];
        p[2] = p1[4];
        p[3] = p2[1];
        p[4] = p2[2];
        p = p + 5;
        p1 = p2;
        p2 = p2 + 5;
        break;
      case 0x16:                        // 4, 6
        p[0] = 0x06;
        p[1] = p1[3];
        p[2] = p1[4];
        p[3] = 0x0F;
        p[4] = p2[1];
        p[5] = p2[2];
        p[6] = p2[3];
        p = p + 7;
        p1 = p2;
        p2 = p2 + 7;
        break;
      case 0x18:                        // 4, 8
        p[0] = 0x08;
        {
          uint8_t tmp = p1[3];
          p[1] = tmp;
          p[2] = tmp;
          tmp = p1[4];
          p[3] = tmp;
          p[4] = tmp;
        }
        p[5] = p2[1];
        p[6] = p2[2];
        p[7] = p2[3];
        p[8] = p2[4];
        p = p + 9;
        p1 = p2;
        p2 = p2 + 9;
        break;
      case 0x19:                        // 6, 1
        p[0] = 0x06;
        p[1] = p1[4];
        p[2] = p1[5];
        p[3] = p1[6];
        p[4] = p2[1];
        p[5] = 0x00;
        p[6] = 0x00;
        p = p + 7;
        p1 = p2;
        p2 = p2 + 2;
        break;
      case 0x1C:                        // 6, 4
        p[0] = 0x06;
        p[1] = p1[4];
        p[2] = p1[5];
        p[3] = p1[6];
        p[4] = p2[1];
        p[5] = p2[2];
        p[6] = 0x0F;
        p = p + 7;
        p1 = p2;
        p2 = p2 + 5;
        break;
      case 0x1E:                        // 6, 6
        p[0] = 0x06;
        p[1] = p1[4];
        p[2] = p1[5];
        p[3] = p1[6];
        p[4] = p2[1];
        p[5] = p2[2];
        p[6] = p2[3];
        p = p + 7;
        p1 = p2;
        p2 = p2 + 7;
        break;
      case 0x10:                        // 6, 8
        p[0] = 0x06;                    // FIXME: this conversion is lossy
        p[1] = p1[4];
        p[2] = p1[5];
        p[3] = p1[6];
        p[4] = p2[1];
        p[5] = p2[3];
        p[6] = 0x0F;
        p = p + 7;
        p1 = p2;
        p2 = p2 + 9;
        break;
      case 0x21:                        // 8, 1
        p[0] = 0x08;
        p[1] = p1[5];
        p[2] = p1[6];
        p[3] = p1[7];
        p[4] = p1[8];
        {
          uint8_t tmp = p2[1];
          p[5] = tmp;
          p[6] = tmp;
          p[7] = tmp;
          p[8] = tmp;
        }
        p = p + 9;
        p1 = p2;
        p2 = p2 + 2;
        break;
      case 0x24:                        // 8, 4
        p[0] = 0x08;
        p[1] = p1[5];
        p[2] = p1[6];
        p[3] = p1[7];
        p[4] = p1[8];
        {
          uint8_t tmp = p2[1];
          p[5] = tmp;
          p[6] = tmp;
          tmp = p2[2];
          p[7] = tmp;
          p[8] = tmp;
        }
        p = p + 9;
        p1 = p2;
        p2 = p2 + 5;
        break;
      case 0x26:                        // 8, 6
        p[0] = 0x06;                    // FIXME: this conversion is lossy
        p[1] = p1[5];
        p[2] = p1[7];
        p[3] = 0x0F;
        p[4] = p2[1];
        p[5] = p2[2];
        p[6] = p2[3];
        p = p + 7;
        p1 = p2;
        p2 = p2 + 7;
        break;
      case 0x28:                        // 8, 8
        p[0] = 0x08;
        p[1] = p1[5];
        p[2] = p1[6];
        p[3] = p1[7];
        p[4] = p1[8];
        p[5] = p2[1];
        p[6] = p2[2];
        p[7] = p2[3];
        p[8] = p2[4];
        p = p + 9;
        p1 = p2;
        p2 = p2 + 9;
        break;
      default:
        for ( ; i < 48; i++) {          // NOTE: this should not be reached
          p[0] = 0x01;
          p[1] = 0x14;
          p = p + 2;
        }
        drawLine(reinterpret_cast<uint8_t *>(&(tmpBuf[0])),
                 size_t(p - reinterpret_cast<uint8_t *>(&(tmpBuf[0]))));
        return;
      }
      flagByte1 = flagByte2;
    }
    drawLine(reinterpret_cast<uint8_t *>(&(tmpBuf[0])),
             size_t(p - reinterpret_cast<uint8_t *>(&(tmpBuf[0]))));
  }

  void CPCVideo::reset()
  {
    videoModeLatched = 0;
    videoDelayBuf[0] = 0U;
    videoDelayBuf[1] = 0U;
    for (size_t i = 0; i < 16; i++)
      palette[i] = 0x00;
    borderColor = 0x00;
    videoMode = 0;
  }

}       // namespace CPC464

