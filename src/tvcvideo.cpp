
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://sourceforge.net/projects/ep128emu/
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
#include "tvcvideo.hpp"

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
   0,  1,  1,  0,   2,  3,  0,  0,   2,  0,  3,  0,   0,  0,  0,  0,
   4,  5,  0,  0,   6,  7,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   4,  0,  5,  0,   0,  0,  0,  0,   6,  0,  7,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   8,  9,  0,  0,  10, 11,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
  12, 13,  0,  0,  14, 15,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
   8,  0,  9,  0,   0,  0,  0,  0,  10,  0, 11,  0,   0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
  12,  0, 13,  0,   0,  0,  0,  0,  14,  0, 15
};

namespace TVC64 {

  void TVCVideo::drawLine(const uint8_t *buf, size_t nBytes)
  {
    (void) buf;
    (void) nBytes;
  }

  void TVCVideo::vsyncStateChange(bool newState, unsigned int currentSlot_)
  {
    (void) newState;
    (void) currentSlot_;
  }

  // --------------------------------------------------------------------------

  TVCVideo::TVCVideo(const CPC464::CRTC6845& crtc_,
                     const uint8_t *videoMemory_)
    : crtc(crtc_),
      lineBufPtr((uint8_t *) 0),
      hSyncCnt(0),
      crtcHSyncCnt(0),
      crtcHSyncState(0),
      vSyncCnt(0),
      videoMemory(videoMemory_),
      lineBuf((uint8_t *) 0),
      borderColor(0x00),
      videoMode(0),
      hSyncMax(120),
      hSyncLen(8)
  {
    videoDelayBuf[0] = 0U;
    videoDelayBuf[1] = 0U;
    for (size_t i = 0; i < 4; i++)
      palette[i] = 0x00;
    lineBuf = reinterpret_cast<uint8_t *>(new uint32_t[112]);
    for (size_t i = 0; i < 448; i++)
      lineBuf[i] = uint8_t((~i) & 0x01);
    lineBufPtr = lineBuf;
    pixelBuf0[0] = 1;
    pixelBuf0[1] = 0x00;
    pixelBuf1[0] = 1;
    pixelBuf1[1] = 0x00;
  }

  TVCVideo::~TVCVideo()
  {
    delete[] reinterpret_cast<uint32_t *>(lineBuf);
  }

  void TVCVideo::convertPixelToRGB(uint8_t pixelValue,
                                   float& r, float& g, float& b)
  {
    r = float(int(bool(pixelValue & 0x0C)));
    g = float(int(bool(pixelValue & 0x30)));
    b = float(int(bool(pixelValue & 0x03)));
    if (!(pixelValue & 0xC0)) {
      r = r * 0.75f;
      g = g * 0.75f;
      b = b * 0.75f;
    }
  }

  void TVCVideo::setColor(uint8_t penNum, uint8_t c)
  {
    if (penNum & 0x04)
      borderColor = c & 0xAA;
    else
      palette[penNum & 0x03] = c & 0x55;
  }

  uint8_t TVCVideo::getColor(uint8_t penNum) const
  {
    if (penNum & 0x04)
      return borderColor;
    return palette[penNum & 0x03];
  }

  EP128EMU_REGPARM1 void TVCVideo::runOneCycle()
  {
    if (EP128EMU_EXPECT((unsigned int) hSyncCnt < 96U)) {
      const unsigned char *delayPtr =
          reinterpret_cast< unsigned char * >(&(videoDelayBuf[0]));
      uint8_t *pixelBuf =
          (!(hSyncCnt & 1U) ? &(pixelBuf0[0]) : &(pixelBuf1[0]));
      if (EP128EMU_UNLIKELY(delayPtr[0] != 0)) {
        pixelBuf[0] = 1;                // sync
        pixelBuf[1] = 0x00;
      }
      else if (delayPtr[1]) {
        uint8_t videoByte = delayPtr[3];
        switch (delayPtr[2]) {
        case 0:                         // 2 color mode
          pixelBuf[0] = 3;
          pixelBuf[1] = palette[0];
          pixelBuf[2] = palette[1];
          pixelBuf[3] = videoByte;
          break;
        case 1:                         // 4 color mode
          pixelBuf[0] = 4;
          pixelBuf[1] = palette[pixelConvTable_4[videoByte & 0x88]];
          pixelBuf[2] = palette[pixelConvTable_4[videoByte & 0x44]];
          pixelBuf[3] = palette[pixelConvTable_4[videoByte & 0x22]];
          pixelBuf[4] = palette[pixelConvTable_4[videoByte & 0x11]];
          break;
        case 2:                         // 16 color mode
        case 3:
          pixelBuf[0] = 2;
          pixelBuf[1] = pixelConvTable_16[videoByte & 0xAA];
          pixelBuf[2] = pixelConvTable_16[videoByte & 0x55];
          break;
        }
      }
      else {
        pixelBuf[0] = 1;                // border
        pixelBuf[1] = borderColor;
      }
      if (hSyncCnt & 1U) {
        if (EP128EMU_UNLIKELY(pixelBuf0[0] != pixelBuf1[0])) {
          uint8_t *p0 = &(pixelBuf0[0]);
          uint8_t *p1 = &(pixelBuf1[0]);
          if (*p0 > *p1) {
            uint8_t *tmp = p0;
            p0 = p1;
            p1 = tmp;
          }
          if (EP128EMU_UNLIKELY(*p0 == 3)) {
            // 2 colors and 4 colors in the same character
            // FIXME: this is a lossy conversion
            uint8_t tmp[2];
            uint8_t b = p0[3];
            tmp[0] = p0[1];
            tmp[1] = p0[2];
            p0[1] = tmp[(b >> 7) & 1];
            p0[2] = tmp[(b >> 5) & 1];
            p0[3] = tmp[(b >> 3) & 1];
            p0[4] = tmp[(b >> 1) & 1];
          }
          else if (*p1 == 3) {
            // 2 colors and 16 or border/blank
            if (*p0 == 1)
              p0[2] = p0[1];
            p0[3] = 0x0F;
          }
          else {
            // expand the lower resolution half of the character
            uint8_t i = *p0;
            uint8_t j = *p1;
            uint8_t k = 0;
            do {
              p0[j] = p0[i];
              j--;
              k = k + *p0;
              if (k >= *p1) {
                k = k - *p1;
                i--;
              }
            } while (j);
          }
          *p0 = *p1;
        }
        switch (pixelBuf0[0]) {
        case 1:
          lineBufPtr[0] = 2;
          lineBufPtr[1] = pixelBuf0[1];
          lineBufPtr[2] = pixelBuf1[1];
          lineBufPtr = lineBufPtr + 3;
          break;
        case 2:
          lineBufPtr[0] = 4;
          lineBufPtr[1] = pixelBuf0[1];
          lineBufPtr[2] = pixelBuf0[2];
          lineBufPtr[3] = pixelBuf1[1];
          lineBufPtr[4] = pixelBuf1[2];
          lineBufPtr = lineBufPtr + 5;
          break;
        case 3:
          lineBufPtr[0] = 6;
          lineBufPtr[1] = pixelBuf0[1];
          lineBufPtr[2] = pixelBuf0[2];
          lineBufPtr[3] = pixelBuf0[3];
          lineBufPtr[4] = pixelBuf1[1];
          lineBufPtr[5] = pixelBuf1[2];
          lineBufPtr[6] = pixelBuf1[3];
          lineBufPtr = lineBufPtr + 7;
          break;
        case 4:
          lineBufPtr[0] = 8;
          lineBufPtr[1] = pixelBuf0[1];
          lineBufPtr[2] = pixelBuf0[2];
          lineBufPtr[3] = pixelBuf0[3];
          lineBufPtr[4] = pixelBuf0[4];
          lineBufPtr[5] = pixelBuf1[1];
          lineBufPtr[6] = pixelBuf1[2];
          lineBufPtr[7] = pixelBuf1[3];
          lineBufPtr[8] = pixelBuf1[4];
          lineBufPtr = lineBufPtr + 9;
          break;
        }
      }
    }
    hSyncCnt++;
    if (EP128EMU_UNLIKELY(crtcHSyncCnt)) {      // horizontal sync
      if (crtcHSyncCnt == (2 + (hSyncLen >> 1))) {
        if (hSyncCnt >= (int(hSyncMax) - 20)) {
          drawLine(lineBuf, size_t(lineBufPtr - lineBuf));
          lineBufPtr = lineBuf;
          hSyncCnt = -1 - int(hSyncLen >> 1);
          hSyncMax = uint8_t(hSyncCnt + 110);
        }
      }
      else if (crtcHSyncCnt == 17) {
        hSyncLen = 8;
        if (vSyncCnt) {
          if (--vSyncCnt == 19)         // VSync start (delayed by 5 lines)
            vsyncStateChange(true, (crtc.getVSyncInterlace() ? 34U : 6U));
          else if (vSyncCnt == 16)      // VSync end
            vsyncStateChange(false, 6U);
        }
        crtcHSyncCnt = uint8_t(-1);     // will overflow to 0
      }
      crtcHSyncCnt++;
    }
    if ((unsigned int) (hSyncCnt + 2) >= 99U) {
      if (EP128EMU_UNLIKELY(hSyncCnt >= int(hSyncMax))) {
        drawLine(lineBuf, size_t(lineBufPtr - lineBuf));
        lineBufPtr = lineBuf;
        hSyncCnt -= int(hSyncMax);
      }
      return;
    }
    videoDelayBuf[0] = videoDelayBuf[1];
    (reinterpret_cast<unsigned char *>(&(videoDelayBuf[0])))[0] =
        crtcHSyncCnt | vSyncCnt;
    bool    displayEnabled = crtc.getDisplayEnabled();
    (reinterpret_cast<unsigned char *>(&(videoDelayBuf[0])))[5] =
        uint8_t(displayEnabled);
    (reinterpret_cast<unsigned char *>(&(videoDelayBuf[0])))[2] =
        uint8_t(videoMode);
    if (displayEnabled) {
      unsigned int  videoAddr =
          ((unsigned int) (crtc.getRowAddress() & 0x03) << 6)
          | (unsigned int) (crtc.getMemoryAddress() & 0x003F)
          | ((unsigned int) (crtc.getMemoryAddress() & 0x0FC0) << 2);
      (reinterpret_cast<unsigned char *>(&(videoDelayBuf[0])))[7] =
          videoMemory[videoAddr];
    }
  }

  void TVCVideo::reset()
  {
    videoDelayBuf[0] = 0U;
    videoDelayBuf[1] = 0U;
    for (size_t i = 0; i < 4; i++)
      palette[i] = 0x00;
    borderColor = 0x00;
    videoMode = 0;
    pixelBuf0[0] = 1;
    pixelBuf0[1] = 0x00;
    pixelBuf1[0] = 1;
    pixelBuf1[1] = 0x00;
  }

}       // namespace TVC64

