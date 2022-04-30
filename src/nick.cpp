
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/ep128emu/
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
#include "memory.hpp"
#include "nick.hpp"
#include "system.hpp"

namespace Ep128 {

  Nick::NickTables  Nick::t;

  Nick::NickTables::NickTables()
  {
    for (size_t i = 0; i < 256; i++) {
      fourColors[(i << 2) + 0] = (uint8_t(i >> 2) & 2) | (uint8_t(i >> 7) & 1);
      fourColors[(i << 2) + 1] = (uint8_t(i >> 1) & 2) | (uint8_t(i >> 6) & 1);
      fourColors[(i << 2) + 2] = (uint8_t(i)      & 2) | (uint8_t(i >> 5) & 1);
      fourColors[(i << 2) + 3] = (uint8_t(i << 1) & 2) | (uint8_t(i >> 4) & 1);
      sixteenColors[(i << 1) + 0] =
          (uint8_t(i << 2) & 8) | (uint8_t(i >> 3) & 4)
        | (uint8_t(i >> 2) & 2) | (uint8_t(i >> 7) & 1);
      sixteenColors[(i << 1) + 1] =
          (uint8_t(i << 3) & 8) | (uint8_t(i >> 2) & 4)
        | (uint8_t(i >> 1) & 2) | (uint8_t(i >> 6) & 1);
    }
  }

  // --------------------------------------------------------------------------

  EP128EMU_INLINE void Nick::renderByte2ColorsL(
      uint8_t b1, uint8_t paletteOffset)
  {
    const uint8_t *palette = &(lpb.palette[paletteOffset]);
    uint8_t   *buf = lineBufPtr;
    buf[0] = 0x03;
    buf[1] = palette[0];
    buf[2] = palette[1];
    buf[3] = b1;
    lineBufPtr = buf + 4;
  }

  EP128EMU_INLINE void Nick::renderByte4ColorsL(
      uint8_t b1, uint8_t paletteOffset)
  {
    const uint8_t *pixels = &(t.fourColors[size_t(b1) << 2]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = lineBufPtr;
    buf[0] = 0x04;
    buf[1] = palette[pixels[0] | paletteOffset];
    buf[2] = palette[pixels[1] | paletteOffset];
    buf[3] = palette[pixels[2] | paletteOffset];
    buf[4] = palette[pixels[3] | paletteOffset];
    lineBufPtr = buf + 5;
  }

  EP128EMU_INLINE void Nick::renderByte16ColorsL(uint8_t b1)
  {
    const uint8_t *pixels = &(t.sixteenColors[size_t(b1) << 1]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = lineBufPtr;
    buf[0] = 0x02;
    buf[1] = palette[pixels[0]];
    buf[2] = palette[pixels[1]];
    lineBufPtr = buf + 3;
  }

  EP128EMU_INLINE void Nick::renderByte16ColorsL(
      uint8_t b1, uint8_t paletteOffset)
  {
    const uint8_t *pixels = &(t.sixteenColors[size_t(b1) << 1]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = lineBufPtr;
    buf[0] = 0x02;
    buf[1] = palette[pixels[0] | paletteOffset];
    buf[2] = palette[pixels[1] | paletteOffset];
    lineBufPtr = buf + 3;
  }

  EP128EMU_INLINE void Nick::renderByte256ColorsL(uint8_t b1)
  {
    lineBufPtr[0] = 0x01;
    lineBufPtr[1] = b1;
    lineBufPtr += 2;
  }

  EP128EMU_INLINE void Nick::renderBytes2Colors(
      uint8_t b1, uint8_t b2, uint8_t paletteOffset1, uint8_t paletteOffset2)
  {
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = lineBufPtr;
    buf[0] = 0x06;
    buf[1] = palette[0 | paletteOffset1];
    buf[2] = palette[1 | paletteOffset1];
    buf[3] = b1;
    buf[4] = palette[0 | paletteOffset2];
    buf[5] = palette[1 | paletteOffset2];
    buf[6] = b2;
    lineBufPtr = buf + 7;
  }

  EP128EMU_INLINE void Nick::renderBytes4Colors(
      uint8_t b1, uint8_t b2, uint8_t paletteOffset1, uint8_t paletteOffset2)
  {
    const uint8_t *pixels = &(t.fourColors[size_t(b1) << 2]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = lineBufPtr;
    buf[0] = 0x08;
    buf[1] = palette[pixels[0] | paletteOffset1];
    buf[2] = palette[pixels[1] | paletteOffset1];
    buf[3] = palette[pixels[2] | paletteOffset1];
    buf[4] = palette[pixels[3] | paletteOffset1];
    pixels = &(t.fourColors[size_t(b2) << 2]);
    buf[5] = palette[pixels[0] | paletteOffset2];
    buf[6] = palette[pixels[1] | paletteOffset2];
    buf[7] = palette[pixels[2] | paletteOffset2];
    buf[8] = palette[pixels[3] | paletteOffset2];
    lineBufPtr = buf + 9;
  }

  EP128EMU_INLINE void Nick::renderBytes16Colors(uint8_t b1, uint8_t b2)
  {
    const uint8_t *pixels = &(t.sixteenColors[size_t(b1) << 1]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = lineBufPtr;
    buf[0] = 0x04;
    buf[1] = palette[pixels[0]];
    buf[2] = palette[pixels[1]];
    pixels = &(t.sixteenColors[size_t(b2) << 1]);
    buf[3] = palette[pixels[0]];
    buf[4] = palette[pixels[1]];
    lineBufPtr = buf + 5;
  }

  EP128EMU_INLINE void Nick::renderBytes16Colors(
      uint8_t b1, uint8_t b2, uint8_t paletteOffset1, uint8_t paletteOffset2)
  {
    const uint8_t *pixels = &(t.sixteenColors[size_t(b1) << 1]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = lineBufPtr;
    buf[0] = 0x04;
    buf[1] = palette[pixels[0] | paletteOffset1];
    buf[2] = palette[pixels[1] | paletteOffset1];
    pixels = &(t.sixteenColors[size_t(b2) << 1]);
    buf[3] = palette[pixels[0] | paletteOffset2];
    buf[4] = palette[pixels[1] | paletteOffset2];
    lineBufPtr = buf + 5;
  }

  EP128EMU_INLINE void Nick::renderBytes256Colors(uint8_t b1, uint8_t b2)
  {
    uint8_t   *buf = lineBufPtr;
    buf[0] = 0x02;
    buf[1] = b1;
    buf[2] = b2;
    lineBufPtr = buf + 3;
  }

  EP128EMU_INLINE void Nick::renderBytesAttribute(uint8_t b1, uint8_t attr)
  {
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = lineBufPtr;
    buf[0] = 0x03;
    buf[1] = palette[attr >> 4];
    buf[2] = palette[attr & 15];
    buf[3] = b1;
    lineBufPtr = buf + 4;
  }

  // --------------------------------------------------------------------------

  EP128EMU_REGPARM1 void Nick::render_Generic(Nick& nick)
  {
    // this function handles all the invalid and undocumented video modes
    switch (nick.lpb.videoMode) {
    case 1:                             // ---- PIXEL ----
      {
        uint8_t b1 = nick.videoMemory[nick.lpb.ld1Addr];
        nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
        uint8_t altColorMask1 = 0x00;
        if (nick.lpb.altInd0) {
          altColorMask1 = (b1 & 0x40) >> 4;
        }
        if (nick.lpb.altInd1) {
          if (b1 & 0x80)
            altColorMask1 = altColorMask1 | 0x02;
        }
        if (nick.lpb.lsbAlt) {
          if (b1 & 0x01) {
            b1 = b1 & 0xFE;
            altColorMask1 = altColorMask1 | 0x04;
          }
        }
        if (nick.lpb.msbAlt) {
          if (b1 & 0x80) {
            b1 = b1 & 0x7F;
            altColorMask1 = altColorMask1 | 0x02;
          }
        }
        uint8_t b2 = nick.videoMemory[nick.lpb.ld1Addr];
        uint8_t altColorMask2 = 0x00;
        nick.lpb.dataBusState = b2;
        if (nick.lpb.altInd0) {
          altColorMask2 = (b2 & 0x40) >> 4;
        }
        if (nick.lpb.altInd1) {
          if (b2 & 0x80)
            altColorMask2 = altColorMask2 | 0x02;
        }
        if (nick.lpb.lsbAlt) {
          if (b2 & 0x01) {
            b2 = b2 & 0xFE;
            altColorMask2 = altColorMask2 | 0x04;
          }
        }
        if (nick.lpb.msbAlt) {
          if (b2 & 0x80) {
            b2 = b2 & 0x7F;
            altColorMask2 = altColorMask2 | 0x02;
          }
        }
        switch (nick.lpb.colorMode) {
        case 0:                         // 2 colors
          nick.renderBytes2Colors(b1, b2, altColorMask1, altColorMask2);
          break;
        case 1:                         // 4 colors
          nick.renderBytes4Colors(b1, b2, altColorMask1, altColorMask2);
          break;
        case 2:                         // 16 colors
          nick.renderBytes16Colors(b1, b2, altColorMask1, altColorMask2);
          break;
        default:                        // 256 colors
          nick.renderBytes256Colors(b1, b2);
          break;
        }
      }
      break;
    case 2:                             // ---- ATTRIBUTE ----
      {
        uint8_t a = nick.videoMemory[nick.lpb.ld1Addr];
        uint8_t b = nick.videoMemory[nick.lpb.ld2Addr];
        nick.lpb.ld2Addr = (nick.lpb.ld2Addr + 1) & 0xFFFF;
        nick.lpb.dataBusState = b;
        if (nick.lpb.lsbAlt)
          b = b & 0xFE;
        if (nick.lpb.msbAlt)
          b = b & 0x7F;
        switch (nick.lpb.colorMode) {
        case 0:                         // 2 colors
          nick.renderBytesAttribute(b, a);
          break;
        case 1:                         // 4 colors
          {
            uint8_t c[2];
            c[0] = nick.lpb.palette[a >> 4];
            c[1] = nick.lpb.palette[a & 0x0F];
            uint8_t *buf_ = nick.lineBufPtr;
            buf_[0] = 0x04;
            buf_[1] = c[(b >> 7) & 1];
            buf_[2] = c[(b >> 6) & 1];
            buf_[3] = c[(b >> 5) & 1];
            buf_[4] = c[(b >> 4) & 1];
            nick.lineBufPtr = buf_ + 5;
          }
          break;
        case 2:                         // 16 colors
          {
            uint8_t c[2];
            c[0] = nick.lpb.palette[a >> 4];
            c[1] = nick.lpb.palette[a & 0x0F];
            uint8_t *buf_ = nick.lineBufPtr;
            buf_[0] = 0x02;
            buf_[1] = c[(b >> 7) & 1];
            buf_[2] = c[(b >> 6) & 1];
            nick.lineBufPtr = buf_ + 3;
          }
          break;
        default:                        // 256 colors
          nick.renderByte256ColorsL(b);
          break;
        }
      }
      break;
    case 3:                             // ---- CH256 ----
    case 4:                             // ---- CH128 ----
    case 5:                             // ---- CH64 ----
    case 6:                             // ---- invalid mode ----
      {
        uint8_t   c = nick.videoMemory[nick.lpb.ld1Addr];
        uint16_t  a = 0xFFFF;
        switch (nick.lpb.videoMode) {
        case 3:
          a = (nick.lpb.ld2Addr << 8) | uint16_t(c);
          break;
        case 4:
          a = (nick.lpb.ld2Addr << 7) | uint16_t(c & 0x7F);
          break;
        case 5:
          a = (nick.lpb.ld2Addr << 6) | uint16_t(c & 0x3F);
          break;
        }
        uint8_t   b = nick.videoMemory[a & 0xFFFF];
        uint8_t   altColorMask = 0x00;
        nick.lpb.dataBusState = b;
        if (nick.lpb.altInd0) {
          altColorMask = (c & 0x40) >> 4;
        }
        if (nick.lpb.altInd1) {
          if (c & 0x80)
            altColorMask = altColorMask | 0x02;
        }
        if (nick.lpb.lsbAlt) {
          if (b & 0x01) {
            b = b & 0xFE;
            altColorMask = altColorMask | 0x04;
          }
        }
        if (nick.lpb.msbAlt) {
          if (b & 0x80) {
            b = b & 0x7F;
            altColorMask = altColorMask | 0x02;
          }
        }
        switch (nick.lpb.colorMode) {
        case 0:                         // 2 colors
          nick.renderByte2ColorsL(b, altColorMask);
          break;
        case 1:                         // 4 colors
          nick.renderByte4ColorsL(b, altColorMask);
          break;
        case 2:                         // 16 colors
          nick.renderByte16ColorsL(b, altColorMask);
          break;
        default:                        // 256 colors
          nick.renderByte256ColorsL(b);
          break;
        }
      }
      break;
    case 7:                             // ---- LPIXEL ----
      {
        uint8_t b = nick.videoMemory[nick.lpb.ld1Addr];
        uint8_t altColorMask = 0x00;
        nick.lpb.dataBusState = b;
        if (nick.lpb.altInd0) {
          altColorMask = (b & 0x40) >> 4;
        }
        if (nick.lpb.altInd1) {
          if (b & 0x80)
            altColorMask = altColorMask | 0x02;
        }
        if (nick.lpb.lsbAlt) {
          if (b & 0x01) {
            b = b & 0xFE;
            altColorMask = altColorMask | 0x04;
          }
        }
        if (nick.lpb.msbAlt) {
          if (b & 0x80) {
            b = b & 0x7F;
            altColorMask = altColorMask | 0x02;
          }
        }
        switch (nick.lpb.colorMode) {
        case 0:                         // 2 colors
          nick.renderByte2ColorsL(b, altColorMask);
          break;
        case 1:                         // 4 colors
          nick.renderByte4ColorsL(b, altColorMask);
          break;
        case 2:                         // 16 colors
          nick.renderByte16ColorsL(b, altColorMask);
          break;
        default:                        // 256 colors
          nick.renderByte256ColorsL(b);
          break;
        }
      }
      break;
    default:                            // ---- VSYNC ----
      {
        nick.lpb.dataBusState = nick.videoMemory[nick.lpb.ld1Addr];
        nick.renderByte256ColorsL(0x00);
      }
      break;
    }
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
  }

  EP128EMU_REGPARM1 void Nick::render_Blank(Nick& nick)
  {
    nick.renderByte256ColorsL(0x00);
  }

  EP128EMU_REGPARM1 void Nick::render_Border(Nick& nick)
  {
    nick.renderByte256ColorsL(nick.borderColor);
  }

  EP128EMU_REGPARM1 void Nick::render_Sync(Nick& nick)
  {
    nick.lpb.dataBusState = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.renderByte256ColorsL(0x00);
  }

  EP128EMU_REGPARM1 void Nick::render_PIXEL_2(Nick& nick)
  {
    uint8_t   b1 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b2;
    nick.renderBytes2Colors(b1, b2, 0, 0);
  }

  EP128EMU_REGPARM1 void Nick::render_PIXEL_2_LSBALT(Nick& nick)
  {
    uint8_t   b1 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b2;
    nick.renderBytes2Colors(b1 & 0xFE, b2 & 0xFE,
                            (b1 & 0x01) << 2, (b2 & 0x01) << 2);
  }

  EP128EMU_REGPARM1 void Nick::render_PIXEL_2_MSBALT(Nick& nick)
  {
    uint8_t   b1 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b2;
    nick.renderBytes2Colors(b1 & 0x7F, b2 & 0x7F,
                            (b1 & 0x80) >> 6, (b2 & 0x80) >> 6);
  }

  EP128EMU_REGPARM1 void Nick::render_PIXEL_2_LSBALT_MSBALT(Nick& nick)
  {
    uint8_t   b1 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b2;
    nick.renderBytes2Colors(b1 & 0x7E, b2 & 0x7E,
                            ((b1 & 0x80) >> 6) | ((b1 & 0x01) << 2),
                            ((b2 & 0x80) >> 6) | ((b2 & 0x01) << 2));
  }

  EP128EMU_REGPARM1 void Nick::render_PIXEL_4(Nick& nick)
  {
    uint8_t   b1 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b2;
    nick.renderBytes4Colors(b1, b2, 0, 0);
  }

  EP128EMU_REGPARM1 void Nick::render_PIXEL_4_LSBALT(Nick& nick)
  {
    uint8_t   b1 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b2;
    nick.renderBytes4Colors(b1 & 0xFE, b2 & 0xFE,
                            (b1 & 0x01) << 2, (b2 & 0x01) << 2);
  }

  EP128EMU_REGPARM1 void Nick::render_PIXEL_16(Nick& nick)
  {
    uint8_t   b1 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b2;
    nick.renderBytes16Colors(b1, b2);
  }

  EP128EMU_REGPARM1 void Nick::render_PIXEL_256(Nick& nick)
  {
    uint8_t   b1 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b2;
    nick.renderBytes256Colors(b1, b2);
  }

  EP128EMU_REGPARM1 void Nick::render_ATTRIBUTE(Nick& nick)
  {
    nick.lpb.dataBusState = nick.videoMemory[nick.lpb.ld2Addr];
    nick.renderBytesAttribute(nick.lpb.dataBusState,
                              nick.videoMemory[nick.lpb.ld1Addr]);
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.ld2Addr = (nick.lpb.ld2Addr + 1) & 0xFFFF;
  }

  EP128EMU_REGPARM1 void Nick::render_CH256_2(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 8) & 0xFFFF)
                                 | uint16_t(ch)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte2ColorsL(b, 0);
  }

  EP128EMU_REGPARM1 void Nick::render_CH256_4(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 8) & 0xFFFF)
                                 | uint16_t(ch)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte4ColorsL(b, 0);
  }

  EP128EMU_REGPARM1 void Nick::render_CH256_16(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 8) & 0xFFFF)
                                 | uint16_t(ch)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte16ColorsL(b);
  }

  EP128EMU_REGPARM1 void Nick::render_CH256_256(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 8) & 0xFFFF)
                                 | uint16_t(ch)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte256ColorsL(b);
  }

  EP128EMU_REGPARM1 void Nick::render_CH128_2(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 7) & 0xFFFF)
                                 | uint16_t(ch & 0x7F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte2ColorsL(b, 0);
  }

  EP128EMU_REGPARM1 void Nick::render_CH128_2_ALTIND1(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 7) & 0xFFFF)
                                 | uint16_t(ch & 0x7F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte2ColorsL(b, (ch & 0x80) >> 6);
  }

  EP128EMU_REGPARM1 void Nick::render_CH128_4(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 7) & 0xFFFF)
                                 | uint16_t(ch & 0x7F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte4ColorsL(b, 0);
  }

  EP128EMU_REGPARM1 void Nick::render_CH128_16(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 7) & 0xFFFF)
                                 | uint16_t(ch & 0x7F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte16ColorsL(b);
  }

  EP128EMU_REGPARM1 void Nick::render_CH128_256(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 7) & 0xFFFF)
                                 | uint16_t(ch & 0x7F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte256ColorsL(b);
  }

  EP128EMU_REGPARM1 void Nick::render_CH64_2(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 6) & 0xFFFF)
                                 | uint16_t(ch & 0x3F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte2ColorsL(b, 0);
  }

  EP128EMU_REGPARM1 void Nick::render_CH64_2_ALTIND0(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 6) & 0xFFFF)
                                 | uint16_t(ch & 0x3F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte2ColorsL(b, (ch & 0x40) >> 4);
  }

  EP128EMU_REGPARM1 void Nick::render_CH64_2_ALTIND1(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 6) & 0xFFFF)
                                 | uint16_t(ch & 0x3F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte2ColorsL(b, (ch & 0x80) >> 6);
  }

  EP128EMU_REGPARM1 void Nick::render_CH64_2_ALTIND0_ALTIND1(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 6) & 0xFFFF)
                                 | uint16_t(ch & 0x3F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte2ColorsL(b, ((ch & 0x80) >> 6) + ((ch & 0x40) >> 4));
  }

  EP128EMU_REGPARM1 void Nick::render_CH64_4(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 6) & 0xFFFF)
                                 | uint16_t(ch & 0x3F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte4ColorsL(b, 0);
  }

  EP128EMU_REGPARM1 void Nick::render_CH64_4_ALTIND0(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 6) & 0xFFFF)
                                 | uint16_t(ch & 0x3F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte4ColorsL(b, (ch & 0x40) >> 4);
  }

  EP128EMU_REGPARM1 void Nick::render_CH64_16(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 6) & 0xFFFF)
                                 | uint16_t(ch & 0x3F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte16ColorsL(b);
  }

  EP128EMU_REGPARM1 void Nick::render_CH64_256(Nick& nick)
  {
    uint8_t ch = nick.videoMemory[nick.lpb.ld1Addr];
    uint8_t b = nick.videoMemory[((nick.lpb.ld2Addr << 6) & 0xFFFF)
                                 | uint16_t(ch & 0x3F)];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte256ColorsL(b);
  }

  EP128EMU_REGPARM1 void Nick::render_LPIXEL_2(Nick& nick)
  {
    nick.lpb.dataBusState = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.renderByte2ColorsL(nick.lpb.dataBusState, 0);
  }

  EP128EMU_REGPARM1 void Nick::render_LPIXEL_2_LSBALT(Nick& nick)
  {
    uint8_t b = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte2ColorsL(b & 0xFE, (b & 0x01) << 2);
  }

  EP128EMU_REGPARM1 void Nick::render_LPIXEL_2_MSBALT(Nick& nick)
  {
    uint8_t b = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte2ColorsL(b & 0x7F, (b & 0x80) >> 6);
  }

  EP128EMU_REGPARM1 void Nick::render_LPIXEL_2_LSBALT_MSBALT(Nick& nick)
  {
    uint8_t b = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte2ColorsL(b & 0x7E, ((b & 0x80) >> 6) | ((b & 0x01) << 2));
  }

  EP128EMU_REGPARM1 void Nick::render_LPIXEL_4(Nick& nick)
  {
    nick.lpb.dataBusState = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.renderByte4ColorsL(nick.lpb.dataBusState, 0);
  }

  EP128EMU_REGPARM1 void Nick::render_LPIXEL_4_LSBALT(Nick& nick)
  {
    uint8_t b = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.lpb.dataBusState = b;
    nick.renderByte4ColorsL(b & 0xFE, (b & 0x01) << 2);
  }

  EP128EMU_REGPARM1 void Nick::render_LPIXEL_16(Nick& nick)
  {
    nick.lpb.dataBusState = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.renderByte16ColorsL(nick.lpb.dataBusState);
  }

  EP128EMU_REGPARM1 void Nick::render_LPIXEL_256(Nick& nick)
  {
    nick.lpb.dataBusState = nick.videoMemory[nick.lpb.ld1Addr];
    nick.lpb.ld1Addr = (nick.lpb.ld1Addr + 1) & 0xFFFF;
    nick.renderByte256ColorsL(nick.lpb.dataBusState);
  }

  // --------------------------------------------------------------------------

  typedef EP128EMU_REGPARM1 void (*NickRenderFunc)(Nick&);

  EP128EMU_REGPARM1 void Nick::setRenderer()
  {
    static const unsigned char  rendererIndexTable[512] = {
       3,  3,  3,  3,   3,  3,  3,  3,   3,  3,  3,  3,   3,  3,  3,  3,  // 0
       3,  3,  3,  3,   3,  3,  3,  3,   3,  3,  3,  3,   3,  3,  3,  3,
       3,  3,  3,  3,   3,  3,  3,  3,   3,  3,  3,  3,   3,  3,  3,  3,
       3,  3,  3,  3,   3,  3,  3,  3,   3,  3,  3,  3,   3,  3,  3,  3,
       4,  0,  0,  0,   5,  0,  0,  0,   6,  0,  0,  0,   7,  0,  0,  0,  // 1
       8,  0,  0,  0,   9,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      10,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      11, 11, 11, 11,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      12, 12, 12, 12,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,  // 2
       0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
       0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
       0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      13,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,  // 3
      14,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      15,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      16, 16, 16, 16,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      17,  0, 18,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,  // 4
      19,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      20,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      21, 21, 21, 21,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      22, 23, 24, 25,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,  // 5
      26, 27,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      28,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      29, 29, 29, 29,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
       0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,  // 6
       0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
       0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
       0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      30,  0,  0,  0,  31,  0,  0,  0,  32,  0,  0,  0,  33,  0,  0,  0,  // 7
      34,  0,  0,  0,  35,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      36,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
      37, 37, 37, 37,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0
    };
    static const NickRenderFunc rendererFunctions[38] = {
      &render_Generic,          &render_Blank,                  // 0
      &render_Border,           &render_Sync,                   // 2
      &render_PIXEL_2,          &render_PIXEL_2_LSBALT,         // 4
      &render_PIXEL_2_MSBALT,   &render_PIXEL_2_LSBALT_MSBALT,  // 6
      &render_PIXEL_4,          &render_PIXEL_4_LSBALT,         // 8
      &render_PIXEL_16,         &render_PIXEL_256,              // 10
      &render_ATTRIBUTE,        &render_CH256_2,                // 12
      &render_CH256_4,          &render_CH256_16,               // 14
      &render_CH256_256,        &render_CH128_2,                // 16
      &render_CH128_2_ALTIND1,  &render_CH128_4,                // 18
      &render_CH128_16,         &render_CH128_256,              // 20
      &render_CH64_2,           &render_CH64_2_ALTIND0,         // 22
      &render_CH64_2_ALTIND1,   &render_CH64_2_ALTIND0_ALTIND1, // 24
      &render_CH64_4,           &render_CH64_4_ALTIND0,         // 26
      &render_CH64_16,          &render_CH64_256,               // 28
      &render_LPIXEL_2,         &render_LPIXEL_2_LSBALT,        // 30
      &render_LPIXEL_2_MSBALT,  &render_LPIXEL_2_LSBALT_MSBALT, // 32
      &render_LPIXEL_4,         &render_LPIXEL_4_LSBALT,        // 34
      &render_LPIXEL_16,        &render_LPIXEL_256              // 36
    };
    if (!displayEnabled) {
      if (EP128EMU_UNLIKELY(!lpb.videoMode))
        currentRenderer = &render_Blank;
      else
        currentRenderer = &render_Border;
      return;
    }
    int     n = (int(lpb.videoMode & 7) << 6) | (int(lpb.colorMode & 3) << 4)
                | (lpb.msbAlt ? 8 : 0) | (lpb.lsbAlt ? 4 : 0)
                | (lpb.altInd1 ? 2 : 0) | (lpb.altInd0 ? 1 : 0);
    currentRenderer = rendererFunctions[rendererIndexTable[n]];
  }

  EP128EMU_REGPARM1 void Nick::renderSlot_noData()
  {
    if (currentSlot < 8) {
      // FIXME: this is a hack for the case when slot 7 is not border,
      // on the real machine it is still HBLANK
      *(lineBufPtr++) = 0x01;
      // assume zero data bytes
      *(lineBufPtr++) =
          ((lpb.colorMode != 3 && lpb.videoMode != 0) ? lpb.palette[0] : 0x00);
    }
    else {
      // in slot >= 54, repeat the last data byte from the bus
      uint16_t  savedLD1 = lpb.ld1Addr;
      uint16_t  savedLD2 = lpb.ld2Addr;
      uint8_t   b0 = videoMemory[0xFFFE];
      uint8_t   b1 = videoMemory[0xFFFF];
      lpb.ld1Addr = 0xFFFE;
      lpb.ld2Addr = 0xFFFE;
      const_cast< uint8_t * >(videoMemory)[0xFFFE] = lpb.dataBusState;
      const_cast< uint8_t * >(videoMemory)[0xFFFF] = lpb.dataBusState;
      if (lpb.videoMode >= 3 && lpb.videoMode < 6) {
        // replace character modes with invalid mode to force LD2=0xFFFF
        uint8_t savedVideoMode = lpb.videoMode;
        lpb.videoMode = 6;
        render_Generic(*this);
        lpb.videoMode = savedVideoMode;
      }
      else {
        currentRenderer(*this);
      }
      const_cast< uint8_t * >(videoMemory)[0xFFFE] = b0;
      const_cast< uint8_t * >(videoMemory)[0xFFFF] = b1;
      lpb.ld1Addr = savedLD1;
      lpb.ld2Addr = savedLD2;
    }
  }

  EP128EMU_REGPARM1 void Nick::runOneSlot()
  {
    if (EP128EMU_UNLIKELY(currentSlot == lpb.rightMargin)) {
      displayEnabled = false;
      setRenderer();
      if (vsyncFlag) {
        vsyncFlag = false;
        vsyncStateChange(false, currentSlot);
      }
    }
    else if (EP128EMU_UNLIKELY(currentSlot == lpb.leftMargin)) {
      displayEnabled = true;
      setRenderer();
      bool  wasVsync = vsyncFlag;
      vsyncFlag = (lpb.videoMode == 0);
      if (vsyncFlag != wasVsync)
        vsyncStateChange(vsyncFlag, currentSlot);
    }
    if (EP128EMU_UNLIKELY(!(currentSlot >= 8 && currentSlot < 54))) {
      switch (currentSlot) {
      case 0:                           // slots 0 to 7: read LPB
        {
          lpb.nLines = 256 - int(videoMemory[lptCurrentAddr + 0]);
          uint8_t modeByte = videoMemory[lptCurrentAddr + 1];
          bool    irqState = bool(modeByte & 0x80);
          lpb.dataBusState = modeByte;
          if (irqState != lpb.interruptFlag) {
            lpb.interruptFlag = irqState;
            irqStateChange(irqState);
          }
          lpb.colorMode = (modeByte >> 5) & 3;
          lpb.vresMode = bool(modeByte & 0x10);
          lpb.videoMode = (modeByte >> 1) & 7;
          lpb.reloadFlag = bool(modeByte & 0x01);
          if (vsyncFlag && lpb.videoMode != 0) {
            vsyncFlag = false;
            vsyncStateChange(false, currentSlot);
          }
        }
        break;
      case 1:
        lpb.leftMargin = videoMemory[lptCurrentAddr + 2];
        lpb.lsbAlt = bool(lpb.leftMargin & 0x40);
        lpb.msbAlt = bool(lpb.leftMargin & 0x80);
        lpb.leftMargin &= 0x3F;
        lpb.rightMargin = videoMemory[lptCurrentAddr + 3];
        lpb.altInd1 = bool(lpb.rightMargin & 0x40);
        lpb.altInd0 = bool(lpb.rightMargin & 0x80);
        lpb.dataBusState = lpb.rightMargin;
        lpb.rightMargin &= 0x3F;
        break;
      case 2:
        lpb.dataBusState = videoMemory[lptCurrentAddr + 5];
        if (linesRemaining <= 0 || !lpb.vresMode) {
          lpb.ld1Addr = uint16_t(videoMemory[lptCurrentAddr + 4])
                        | (uint16_t(lpb.dataBusState) << 8);
        }
        break;
      case 3:
        {
          uint16_t  ld2Base = videoMemory[lptCurrentAddr + 6];
          lpb.dataBusState = videoMemory[lptCurrentAddr + 7];
          ld2Base = ld2Base | (uint16_t(lpb.dataBusState) << 8);
          if (lpb.videoMode >= 3 && lpb.videoMode <= 5)
            lpb.ld2Addr = (lpb.ld2Addr + 1) & 0xFFFF;
          if (linesRemaining <= 0) {
            lpb.ld2Addr = ld2Base;
            linesRemaining = lpb.nLines;
          }
        }
        break;
      case 4:
        lpb.palette[0] = videoMemory[lptCurrentAddr + 8];
        lpb.palette[1] = videoMemory[lptCurrentAddr + 9];
        lpb.dataBusState = lpb.palette[1];
        break;
      case 5:
        lpb.palette[2] = videoMemory[lptCurrentAddr + 10];
        lpb.palette[3] = videoMemory[lptCurrentAddr + 11];
        lpb.dataBusState = lpb.palette[3];
        break;
      case 6:
        lpb.palette[4] = videoMemory[lptCurrentAddr + 12];
        lpb.palette[5] = videoMemory[lptCurrentAddr + 13];
        lpb.dataBusState = lpb.palette[5];
        break;
      case 7:
        lpb.palette[6] = videoMemory[lptCurrentAddr + 14];
        lpb.palette[7] = videoMemory[lptCurrentAddr + 15];
        lpb.dataBusState = lpb.palette[7];
        lineBufPtr = lineBuf;           // begin display area
      case 54:
        if (EP128EMU_UNLIKELY(displayEnabled))
          renderSlot_noData();
        else
          currentRenderer(*this);
        break;
      case 55:                          // end of display area
        drawLine(lineBuf, size_t(lineBufPtr - lineBuf));
        break;
      case 56:
        linesRemaining--;
        if (linesRemaining == 0 || (lptFlags & 0x80) != 0) {
          if (port3Value & 0x40) {
            if (!(uint8_t(lpb.reloadFlag) | (lptFlags & 0x40)))
              lptCurrentAddr = (lptCurrentAddr + 0x0010) & 0xFFF0;
            else
              lptCurrentAddr = lptBaseAddr;
          }
        }
        lptFlags = (port3Value & ((~port3Value) >> 1)) & 0x40;
        currentSlot = uint8_t(-1);
        break;
      }
      if (EP128EMU_UNLIKELY(displayEnabled)) {
        lpb.ld1Addr = (lpb.ld1Addr + uint16_t(lpb.videoMode == 1) + 1) & 0xFFFF;
        lpb.ld2Addr = (lpb.ld2Addr + uint16_t(lpb.videoMode == 2)) & 0xFFFF;
      }
      currentSlot++;
      return;
    }
    currentSlot++;
    currentRenderer(*this);
  }

  Nick::Nick(Memory& m_)
  {
    lpb.nLines = 1;
    lpb.interruptFlag = false;
    lpb.vresMode = false;
    lpb.reloadFlag = true;
    lpb.colorMode = 0;
    lpb.videoMode = 7;
    lpb.altInd0 = false;
    lpb.altInd1 = false;
    lpb.lsbAlt = false;
    lpb.msbAlt = false;
    lpb.leftMargin = 8;
    lpb.rightMargin = 54;
    lpb.dataBusState = 0xFF;
    for (size_t i = 0; i < 16; i++)
      lpb.palette[i] = 0;
    lpb.ld1Addr = 0;
    lpb.ld2Addr = 0;
    lptBaseAddr = 0;
    lptCurrentAddr = 0;
    linesRemaining = 0;
    videoMemory = m_.getVideoMemory();
    currentRenderer = &render_Blank;
    displayEnabled = false;
    currentSlot = 0;
    borderColor = 0x00;
    lptFlags = 0x00;
    vsyncFlag = false;
    port0Value = 0x00;
    port3Value = 0xF0;
    try {
      uint32_t  *p = new uint32_t[129];     // for 513 bytes (57 * 9)
      lineBuf = reinterpret_cast<uint8_t *>(p);
      clearLineBuffer();
    }
    catch (...) {
      lineBuf = (uint8_t *) 0;
      throw;
    }
    randomizeRegisters();
  }

  Nick::~Nick()
  {
    if (lineBuf) {
      delete[] reinterpret_cast<uint32_t *>(lineBuf);
      lineBuf = (uint8_t *) 0;
    }
  }

  void Nick::clearLineBuffer()
  {
    uint32_t  *p = reinterpret_cast<uint32_t *>(lineBuf);
    for (int i = 0; i < 129; i++)
      p[i] = 0U;
    for (int i = 0; i < 114; i += 2) {
      lineBuf[i] = 0x01;
      lineBuf[i + 1] = borderColor;
    }
    lineBufPtr = lineBuf;
  }

  void Nick::convertPixelToRGB(uint8_t pixelValue,
                               float& r, float& g, float& b)
  {
    r = float(   ((pixelValue << 2) & 4)
               | ((pixelValue >> 2) & 2)
               | ((pixelValue >> 6) & 1)) / 7.0f;
    g = float(   ((pixelValue << 1) & 4)
               | ((pixelValue >> 3) & 2)
               | ((pixelValue >> 7) & 1)) / 7.0f;
    b = float(   ((pixelValue >> 1) & 2)
               | ((pixelValue >> 5) & 1)) / 3.0f;
  }

  uint8_t Nick::readPort(uint16_t portNum)
  {
    (void) portNum;
    return lpb.dataBusState;
  }

  void Nick::writePort(uint16_t portNum, uint8_t value)
  {
    lpb.dataBusState = value;
    switch (portNum & 3) {
    case 0:
      port0Value = value;
      {
        uint8_t fixBias = (value & 0x1F) << 3;
        lpb.palette[8]  = fixBias | 0;
        lpb.palette[9]  = fixBias | 1;
        lpb.palette[10] = fixBias | 2;
        lpb.palette[11] = fixBias | 3;
        lpb.palette[12] = fixBias | 4;
        lpb.palette[13] = fixBias | 5;
        lpb.palette[14] = fixBias | 6;
        lpb.palette[15] = fixBias | 7;
      }
      // Speaker control (bit 7) is handled in ep128vm
      break;
    case 1:
      borderColor = value;
      break;
    case 2:
      lptBaseAddr = (lptBaseAddr & 0xF000) | (uint16_t(value) << 4);
      break;
    case 3:
      lptBaseAddr = (lptBaseAddr & 0x0FF0) | (uint16_t(value & 0x0F) << 12);
      uint8_t flagBitsChanged = (value ^ port3Value) & 0xC0;
      port3Value = value;
      if (flagBitsChanged != 0) {
        if (!(value & 0x40)) {
          lptFlags = 0x00;
        }
        else {
          if (flagBitsChanged & 0x40)
            lptFlags = lptFlags | 0x80;
          if (!(value & 0x80))
            lptFlags = lptFlags | 0x40;
        }
      }
      break;
    }
  }

  uint8_t Nick::readPortDebug(uint16_t portNum) const
  {
    switch (portNum & 3) {
    case 0:
      return port0Value;
    case 1:
      return borderColor;
    case 2:
      return uint8_t((lptBaseAddr >> 4) & 0xFF);
    case 3:
      return port3Value;
    }
    return 0x00;                // not reached
  }

  void Nick::randomizeRegisters()
  {
    uint32_t  tmp = Ep128Emu::Timer::getRandomSeedFromTime();
    writePort(0, uint8_t(tmp & 0xFF));
    writePort(1, uint8_t((tmp >> 8) & 0xFF));
    writePort(2, uint8_t((tmp >> 16) & 0xFF));
    writePort(3, uint8_t(((tmp >> 24) & 0xFF) | 0xF0));
  }

  void Nick::irqStateChange(bool newState)
  {
    (void) newState;
  }

  void Nick::drawLine(const uint8_t *buf, size_t nBytes)
  {
    (void) buf;
    (void) nBytes;
  }

  void Nick::vsyncStateChange(bool newState, unsigned int currentSlot_)
  {
    (void) newState;
    (void) currentSlot_;
  }

  // --------------------------------------------------------------------------

  class ChunkType_NickSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Nick&   ref;
   public:
    ChunkType_NickSnapshot(Nick& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_NickSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_NICK_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void Nick::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x05000000U);       // version number
    buf.writeUInt32(uint32_t(lpb.nLines));
    buf.writeBoolean(lpb.interruptFlag);
    buf.writeBoolean(lpb.vresMode);
    buf.writeBoolean(lpb.reloadFlag);
    buf.writeByte(lpb.colorMode);
    buf.writeByte(lpb.videoMode);
    buf.writeBoolean(lpb.altInd0);
    buf.writeBoolean(lpb.altInd1);
    buf.writeBoolean(lpb.lsbAlt);
    buf.writeBoolean(lpb.msbAlt);
    buf.writeByte(lpb.leftMargin);
    buf.writeByte(lpb.rightMargin);
    for (size_t i = 0; i < 16; i++)
      buf.writeByte(lpb.palette[i]);
    buf.writeUInt32(lpb.ld1Addr);
    buf.writeUInt32(lpb.ld2Addr);
    buf.writeUInt32(lptBaseAddr);
    buf.writeUInt32(lptCurrentAddr);
    buf.writeUInt32(uint32_t(linesRemaining));
    buf.writeBoolean(displayEnabled);
    buf.writeByte(currentSlot);
    buf.writeByte(borderColor);
    buf.writeByte(lpb.dataBusState);
    buf.writeByte(lptFlags);
    buf.writeByte(port0Value);
    buf.writeByte(port3Value);
  }

  void Nick::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_NICK_STATE, buf);
  }

  void Nick::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (!(version >= 0x02000000U && version <= 0x05000000U)) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible Nick snapshot format");
    }
    try {
      // load saved state
      lpb.nLines = int(((buf.readUInt32() - 1) & 0xFF) + 1);
      lpb.interruptFlag = buf.readBoolean();
      lpb.vresMode = buf.readBoolean();
      lpb.reloadFlag = buf.readBoolean();
      lpb.colorMode = buf.readByte() & 3;
      lpb.videoMode = buf.readByte() & 7;
      lpb.altInd0 = buf.readBoolean();
      lpb.altInd1 = buf.readBoolean();
      lpb.lsbAlt = buf.readBoolean();
      lpb.msbAlt = buf.readBoolean();
      lpb.leftMargin = buf.readByte() & 0x3F;
      lpb.rightMargin = buf.readByte() & 0x3F;
      if (version < 0x03000000U) {
        (void) buf.readUInt32();        // was lpb.ld1Base
        (void) buf.readUInt32();        // was lpb.ld2Base
      }
      for (size_t i = 0; i < 16; i++)
        lpb.palette[i] = buf.readByte();
      lpb.ld1Addr = uint16_t(buf.readUInt32()) & 0xFFFF;
      lpb.ld2Addr = uint16_t(buf.readUInt32()) & 0xFFFF;
      if (version < 0x05000000U && lpb.videoMode >= 3 && lpb.videoMode <= 5)
        lpb.ld2Addr = lpb.ld2Addr >> (11 - lpb.videoMode);
      lptBaseAddr = uint16_t(buf.readUInt32()) & 0xFFF0;
      lptCurrentAddr = uint16_t(buf.readUInt32()) & 0xFFF0;
      linesRemaining = int(buf.readUInt32() % 257U);
      displayEnabled = buf.readBoolean();
      setRenderer();
      currentSlot = uint8_t(buf.readByte() % 57U);
      borderColor = buf.readByte();
      lpb.dataBusState = buf.readByte();
      clearLineBuffer();
      if (currentSlot >= 7)
        lineBufPtr = &(lineBuf[(currentSlot - 7) << 1]);
      if (version >= 0x04000000U) {
        lptFlags = buf.readByte() & 0xC0;
        port0Value = buf.readByte();
        port3Value = buf.readByte();
      }
      else {
        bool    lptClockEnabled = buf.readBoolean();
        port0Value = (lpb.palette[8] >> 3) & 0x1F;
        port3Value = uint8_t((lptBaseAddr >> 12) & 0x0F)
                     | (uint8_t(lptClockEnabled) << 6)
                     | 0xB0;
        if (version >= 0x03000000U) {
          bool    forceReloadFlag = buf.readBoolean();
          port3Value = port3Value & uint8_t(0xFF ^ (int(forceReloadFlag) << 7));
        }
        else {
          if (currentSlot > 0 && currentSlot < 4 &&
              linesRemaining == lpb.nLines) {
            linesRemaining = 0;
          }
        }
        lptFlags = (port3Value & ((~port3Value) >> 1)) & 0x40;
      }
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "Nick snapshot data");
    }
    catch (...) {
      // reset NICK
      for (uint16_t i = 0; i < 4; i++)
        writePort(i, 0x00);
      lptFlags = 0x00;
      displayEnabled = false;
      setRenderer();
      currentSlot = 0;
      clearLineBuffer();
      throw;
    }
  }

  void Nick::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_NickSnapshot  *p;
    p = new ChunkType_NickSnapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace Ep128

