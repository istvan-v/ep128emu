
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
      hSyncCnt(-1),
      crtcHSyncCnt(0),
      crtcHSyncState(0),
      vSyncCnt(0),
      videoMemory(videoMemory_),
      lineBuf((uint8_t *) 0),
      borderColor(0x00),
      videoMode(0),
      hSyncLen(8),
      hSyncPos(10)
  {
    videoDelayBuf[0] = 0U;
    videoDelayBuf[1] = 0U;
    for (size_t i = 0; i < 4; i++)
      palette[i] = 0x00;
    lineBuf = reinterpret_cast<uint8_t *>(new uint32_t[112]);
    for (size_t i = 0; i < 448; i++)
      lineBuf[i] = uint8_t((~i) & 0x01);
    lineBufPtr = lineBuf;
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
      r = r * (4.0f / 7.0f);
      g = g * (4.0f / 7.0f);
      b = b * (4.0f / 7.0f);
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
    hSyncCnt++;
    if (EP128EMU_UNLIKELY(crtcHSyncCnt)) {      // horizontal sync
      if (crtcHSyncCnt == hSyncPos) {
        if (hSyncCnt >= 96) {
          drawLine(lineBuf, size_t(lineBufPtr - lineBuf));
          lineBufPtr = lineBuf;
          hSyncCnt = -2;
        }
      }
      else if (crtcHSyncCnt == 17) {
        hSyncLen = 8;
        hSyncPos = (hSyncLen >> 1) + 6;
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
    if (EP128EMU_EXPECT((unsigned int) hSyncCnt < 96U)) {
      const unsigned char *delayPtr =
          reinterpret_cast< unsigned char * >(&(videoDelayBuf[0]));
      if (!(hSyncCnt & 1U)) {
        if (delayPtr[1] > delayPtr[0]) {
          uint8_t videoByte = delayPtr[3];
          switch (delayPtr[2]) {
          case 0:                       // 2 color mode
            lineBufPtr[0] = 6;
            lineBufPtr[1] = palette[0];
            lineBufPtr[2] = palette[1];
            lineBufPtr[3] = videoByte;
            break;
          case 1:                       // 4 color mode
            lineBufPtr[0] = 8;
            lineBufPtr[1] = palette[pixelConvTable_4[videoByte & 0x88]];
            lineBufPtr[2] = palette[pixelConvTable_4[videoByte & 0x44]];
            lineBufPtr[3] = palette[pixelConvTable_4[videoByte & 0x22]];
            lineBufPtr[4] = palette[pixelConvTable_4[videoByte & 0x11]];
            break;
          case 2:                       // 16 color mode
          case 3:
            lineBufPtr[0] = 4;
            lineBufPtr[1] = videoByte & 0xAA;
            lineBufPtr[2] = videoByte & 0x55;
            break;
          }
        }
        else {
          lineBufPtr[0] = 1;            // border or sync
          lineBufPtr[1] = (!(delayPtr[0]) ? borderColor : 0x00);
        }
      }
      else {
        if (delayPtr[1] > delayPtr[0]) {
          uint8_t videoByte = delayPtr[3];
          switch (delayPtr[2]) {
          case 0:                       // 2 color mode
            if (EP128EMU_UNLIKELY(lineBufPtr[0] != 6)) {
              if (EP128EMU_EXPECT(lineBufPtr[0] == 1)) {
                lineBufPtr[0] = 6;
                lineBufPtr[2] = lineBufPtr[1];
                lineBufPtr[3] = 0x00;
              }
              else if (lineBufPtr[0] == 4) {
                lineBufPtr[0] = 6;
                lineBufPtr[3] = 0x0F;
              }
              else {
                // FIXME: this is a lossy conversion
                lineBufPtr[5] = palette[(videoByte >> 7) & 1];
                lineBufPtr[6] = palette[(videoByte >> 5) & 1];
                lineBufPtr[7] = palette[(videoByte >> 3) & 1];
                lineBufPtr[8] = palette[(videoByte >> 1) & 1];
                break;
              }
            }
            lineBufPtr[4] = palette[0];
            lineBufPtr[5] = palette[1];
            lineBufPtr[6] = videoByte;
            break;
          case 1:                       // 4 color mode
            if (EP128EMU_UNLIKELY(lineBufPtr[0] != 8)) {
              switch (lineBufPtr[0]) {
              case 1:
                lineBufPtr[2] = lineBufPtr[1];
                lineBufPtr[3] = lineBufPtr[1];
                lineBufPtr[4] = lineBufPtr[1];
                break;
              case 2:
                lineBufPtr[4] = lineBufPtr[2];
                lineBufPtr[3] = lineBufPtr[2];
                lineBufPtr[2] = lineBufPtr[1];
                break;
              case 3:
                // FIXME: this is a lossy conversion
                {
                  uint8_t c0 = lineBufPtr[((lineBufPtr[3] >> 7) & 1) + 1];
                  uint8_t c1 = lineBufPtr[((lineBufPtr[3] >> 5) & 1) + 1];
                  uint8_t c2 = lineBufPtr[((lineBufPtr[3] >> 3) & 1) + 1];
                  uint8_t c3 = lineBufPtr[((lineBufPtr[3] >> 1) & 1) + 1];
                  lineBufPtr[1] = c0;
                  lineBufPtr[2] = c1;
                  lineBufPtr[3] = c2;
                  lineBufPtr[4] = c3;
                }
                break;
              }
              lineBufPtr[0] = 8;
            }
            lineBufPtr[5] = palette[pixelConvTable_4[videoByte & 0x88]];
            lineBufPtr[6] = palette[pixelConvTable_4[videoByte & 0x44]];
            lineBufPtr[7] = palette[pixelConvTable_4[videoByte & 0x22]];
            lineBufPtr[8] = palette[pixelConvTable_4[videoByte & 0x11]];
            break;
          case 2:                       // 16 color mode
          case 3:
            if (EP128EMU_UNLIKELY(lineBufPtr[0] != 4)) {
              if (EP128EMU_EXPECT(lineBufPtr[0] == 1)) {
                lineBufPtr[0] = 4;
                lineBufPtr[2] = lineBufPtr[1];
              }
              else {
                if (lineBufPtr[0] == 6) {
                  lineBufPtr[4] = videoByte & 0xAA;
                  lineBufPtr[5] = videoByte & 0x55;
                  lineBufPtr[6] = 0x0F;
                }
                else {
                  lineBufPtr[5] = videoByte & 0xAA;
                  lineBufPtr[6] = videoByte & 0xAA;
                  lineBufPtr[7] = videoByte & 0x55;
                  lineBufPtr[8] = videoByte & 0x55;
                }
                break;
              }
            }
            lineBufPtr[3] = videoByte & 0xAA;
            lineBufPtr[4] = videoByte & 0x55;
            break;
          }
        }
        else {                          // border or sync
          uint8_t c = (!(delayPtr[0]) ? borderColor : 0x00);
          if (EP128EMU_EXPECT(lineBufPtr[0] == 1)) {
            if (EP128EMU_UNLIKELY(c != lineBufPtr[1])) {
              lineBufPtr[0] = 2;
              lineBufPtr[2] = c;
            }
          }
          else if (lineBufPtr[0] == 4) {
            lineBufPtr[3] = c;
            lineBufPtr[4] = c;
          }
          else if (lineBufPtr[0] == 6) {
            lineBufPtr[4] = c;
            lineBufPtr[5] = c;
            lineBufPtr[6] = 0x00;
          }
          else {
            lineBufPtr[5] = c;
            lineBufPtr[6] = c;
            lineBufPtr[7] = c;
            lineBufPtr[8] = c;
          }
        }
        lineBufPtr = lineBufPtr + (lineBufPtr[0] + 1);
      }
    }
    else if (EP128EMU_UNLIKELY(hSyncCnt >= 101)) {
      drawLine(lineBuf, size_t(lineBufPtr - lineBuf));
      lineBufPtr = lineBuf;
      hSyncCnt = -2;
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
  }

}       // namespace TVC64

