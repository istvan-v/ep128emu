
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
#include "memory.hpp"
#include "nick.hpp"
#include "system.hpp"

namespace Ep128 {

  NickRenderer::NickTables  NickRenderer::t;

  NickRenderer::NickTables::NickTables()
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

  EP128EMU_INLINE void NickRenderer::renderByte2ColorsL(
      uint8_t*& buf_, uint8_t b1, uint8_t paletteOffset)
  {
    const uint8_t *palette = &(lpb.palette[paletteOffset]);
    uint8_t   *buf = buf_;
    buf[0] = 0x03;
    buf[1] = palette[0];
    buf[2] = palette[1];
    buf[3] = b1;
    buf_ = buf + 4;
  }

  EP128EMU_INLINE void NickRenderer::renderByte4ColorsL(
      uint8_t*& buf_, uint8_t b1, uint8_t paletteOffset)
  {
    const uint8_t *pixels = &(t.fourColors[size_t(b1) << 2]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = buf_;
    buf[0] = 0x04;
    buf[1] = palette[pixels[0] | paletteOffset];
    buf[2] = palette[pixels[1] | paletteOffset];
    buf[3] = palette[pixels[2] | paletteOffset];
    buf[4] = palette[pixels[3] | paletteOffset];
    buf_ = buf + 5;
  }

  EP128EMU_INLINE void NickRenderer::renderByte16ColorsL(
      uint8_t*& buf_, uint8_t b1)
  {
    const uint8_t *pixels = &(t.sixteenColors[size_t(b1) << 1]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = buf_;
    buf[0] = 0x02;
    buf[1] = palette[pixels[0]];
    buf[2] = palette[pixels[1]];
    buf_ = buf + 3;
  }

  EP128EMU_INLINE void NickRenderer::renderByte16ColorsL(
      uint8_t*& buf_, uint8_t b1, uint8_t paletteOffset)
  {
    const uint8_t *pixels = &(t.sixteenColors[size_t(b1) << 1]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = buf_;
    buf[0] = 0x02;
    buf[1] = palette[pixels[0] | paletteOffset];
    buf[2] = palette[pixels[1] | paletteOffset];
    buf_ = buf + 3;
  }

  EP128EMU_INLINE void NickRenderer::renderByte256ColorsL(
      uint8_t*& buf_, uint8_t b1)
  {
    buf_[0] = 0x01;
    buf_[1] = b1;
    buf_ += 2;
  }

  EP128EMU_INLINE void NickRenderer::renderBytes2Colors(
      uint8_t*& buf_, uint8_t b1, uint8_t b2,
      uint8_t paletteOffset1, uint8_t paletteOffset2)
  {
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = buf_;
    buf[0] = 0x06;
    buf[1] = palette[0 | paletteOffset1];
    buf[2] = palette[1 | paletteOffset1];
    buf[3] = b1;
    buf[4] = palette[0 | paletteOffset2];
    buf[5] = palette[1 | paletteOffset2];
    buf[6] = b2;
    buf_ = buf + 7;
  }

  EP128EMU_INLINE void NickRenderer::renderBytes4Colors(
      uint8_t*& buf_, uint8_t b1, uint8_t b2,
      uint8_t paletteOffset1, uint8_t paletteOffset2)
  {
    const uint8_t *pixels = &(t.fourColors[size_t(b1) << 2]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = buf_;
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
    buf_ = buf + 9;
  }

  EP128EMU_INLINE void NickRenderer::renderBytes16Colors(
      uint8_t*& buf_, uint8_t b1, uint8_t b2)
  {
    const uint8_t *pixels = &(t.sixteenColors[size_t(b1) << 1]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = buf_;
    buf[0] = 0x04;
    buf[1] = palette[pixels[0]];
    buf[2] = palette[pixels[1]];
    pixels = &(t.sixteenColors[size_t(b2) << 1]);
    buf[3] = palette[pixels[0]];
    buf[4] = palette[pixels[1]];
    buf_ = buf + 5;
  }

  EP128EMU_INLINE void NickRenderer::renderBytes16Colors(
      uint8_t*& buf_, uint8_t b1, uint8_t b2,
      uint8_t paletteOffset1, uint8_t paletteOffset2)
  {
    const uint8_t *pixels = &(t.sixteenColors[size_t(b1) << 1]);
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = buf_;
    buf[0] = 0x04;
    buf[1] = palette[pixels[0] | paletteOffset1];
    buf[2] = palette[pixels[1] | paletteOffset1];
    pixels = &(t.sixteenColors[size_t(b2) << 1]);
    buf[3] = palette[pixels[0] | paletteOffset2];
    buf[4] = palette[pixels[1] | paletteOffset2];
    buf_ = buf + 5;
  }

  EP128EMU_INLINE void NickRenderer::renderBytes256Colors(
      uint8_t*& buf_, uint8_t b1, uint8_t b2)
  {
    uint8_t   *buf = buf_;
    buf[0] = 0x02;
    buf[1] = b1;
    buf[2] = b2;
    buf_ = buf + 3;
  }

  EP128EMU_INLINE void NickRenderer::renderBytesAttribute(
      uint8_t*& buf_, uint8_t b1, uint8_t attr)
  {
    const uint8_t *palette = &(lpb.palette[0]);
    uint8_t   *buf = buf_;
    buf[0] = 0x03;
    buf[1] = palette[attr >> 4];
    buf[2] = palette[attr & 15];
    buf[3] = b1;
    buf_ = buf + 4;
  }

  // --------------------------------------------------------------------------

  EP128EMU_REGPARM2 void NickRenderer_Blank::doRender(uint8_t*& buf)
  {
    renderByte256ColorsL(buf, 0);
  }

  EP128EMU_REGPARM2 void NickRenderer_Generic::doRender(uint8_t*& buf)
  {
    switch (lpb.videoMode) {
    case 1:                             // ---- PIXEL ----
      {
        uint8_t b1 = videoMemory[lpb.ld1Addr];
        lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
        uint8_t altColorMask1 = 0x00;
        if (lpb.altInd0) {
          altColorMask1 = (b1 & 0x40) >> 4;
        }
        if (lpb.altInd1) {
          if (b1 & 0x80)
            altColorMask1 = altColorMask1 | 0x02;
        }
        if (lpb.lsbAlt) {
          if (b1 & 0x01) {
            b1 = b1 & 0xFE;
            altColorMask1 = altColorMask1 | 0x04;
          }
        }
        if (lpb.msbAlt) {
          if (b1 & 0x80) {
            b1 = b1 & 0x7F;
            altColorMask1 = altColorMask1 | 0x02;
          }
        }
        uint8_t b2 = videoMemory[lpb.ld1Addr];
        lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
        uint8_t altColorMask2 = 0x00;
        if (lpb.altInd0) {
          altColorMask2 = (b2 & 0x40) >> 4;
        }
        if (lpb.altInd1) {
          if (b2 & 0x80)
            altColorMask2 = altColorMask2 | 0x02;
        }
        if (lpb.lsbAlt) {
          if (b2 & 0x01) {
            b2 = b2 & 0xFE;
            altColorMask2 = altColorMask2 | 0x04;
          }
        }
        if (lpb.msbAlt) {
          if (b2 & 0x80) {
            b2 = b2 & 0x7F;
            altColorMask2 = altColorMask2 | 0x02;
          }
        }
        switch (lpb.colorMode) {
        case 0:                         // 2 colors
          renderBytes2Colors(buf, b1, b2, altColorMask1, altColorMask2);
          break;
        case 1:                         // 4 colors
          renderBytes4Colors(buf, b1, b2, altColorMask1, altColorMask2);
          break;
        case 2:                         // 16 colors
          renderBytes16Colors(buf, b1, b2, altColorMask1, altColorMask2);
          break;
        default:                        // 256 colors
          renderBytes256Colors(buf, b1, b2);
          break;
        }
      }
      break;
    case 2:                             // ---- ATTRIBUTE ----
      {
        uint8_t a = videoMemory[lpb.ld1Addr];
        lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
        uint8_t b = videoMemory[lpb.ld2Addr];
        lpb.ld2Addr = (lpb.ld2Addr + 1) & 0xFFFF;
        if (lpb.lsbAlt)
          b = b & 0xFE;
        if (lpb.msbAlt)
          b = b & 0x7F;
        switch (lpb.colorMode) {
        case 0:                         // 2 colors
          renderBytesAttribute(buf, b, a);
          break;
        case 1:                         // 4 colors
          {
            uint8_t c[2];
            c[0] = lpb.palette[a >> 4];
            c[1] = lpb.palette[a & 0x0F];
            uint8_t *buf_ = buf;
            buf_[0] = 0x04;
            buf_[1] = c[(b >> 7) & 1];
            buf_[2] = c[(b >> 6) & 1];
            buf_[3] = c[(b >> 5) & 1];
            buf_[4] = c[(b >> 4) & 1];
            buf = buf_ + 5;
          }
          break;
        case 2:                         // 16 colors
          {
            uint8_t c[2];
            c[0] = lpb.palette[a >> 4];
            c[1] = lpb.palette[a & 0x0F];
            uint8_t *buf_ = buf;
            buf_[0] = 0x02;
            buf_[1] = c[(b >> 7) & 1];
            buf_[2] = c[(b >> 6) & 1];
            buf = buf_ + 3;
          }
          break;
        default:                        // 256 colors
          renderByte256ColorsL(buf, b);
          break;
        }
      }
      break;
    case 3:                             // ---- CH256 ----
    case 4:                             // ---- CH128 ----
    case 5:                             // ---- CH64 ----
    case 6:                             // ---- invalid mode ----
      {
        uint8_t   c = videoMemory[lpb.ld1Addr];
        lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
        uint16_t  a = 0xFFFF;
        switch (lpb.videoMode) {
        case 3:
          a = lpb.ld2Addr | uint16_t(c);
          break;
        case 4:
          a = lpb.ld2Addr | uint16_t(c & 0x7F);
          break;
        case 5:
          a = lpb.ld2Addr | uint16_t(c & 0x3F);
          break;
        }
        uint8_t   b = videoMemory[a];
        uint8_t   altColorMask = 0x00;
        if (lpb.altInd0) {
          altColorMask = (c & 0x40) >> 4;
        }
        if (lpb.altInd1) {
          if (c & 0x80)
            altColorMask = altColorMask | 0x02;
        }
        if (lpb.lsbAlt) {
          if (b & 0x01) {
            b = b & 0xFE;
            altColorMask = altColorMask | 0x04;
          }
        }
        if (lpb.msbAlt) {
          if (b & 0x80) {
            b = b & 0x7F;
            altColorMask = altColorMask | 0x02;
          }
        }
        switch (lpb.colorMode) {
        case 0:                         // 2 colors
          renderByte2ColorsL(buf, b, altColorMask);
          break;
        case 1:                         // 4 colors
          renderByte4ColorsL(buf, b, altColorMask);
          break;
        case 2:                         // 16 colors
          renderByte16ColorsL(buf, b, altColorMask);
          break;
        default:                        // 256 colors
          renderByte256ColorsL(buf, b);
          break;
        }
      }
      break;
    case 7:                             // ---- LPIXEL ----
      {
        uint8_t b = videoMemory[lpb.ld1Addr];
        lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
        uint8_t altColorMask = 0x00;
        if (lpb.altInd0) {
          altColorMask = (b & 0x40) >> 4;
        }
        if (lpb.altInd1) {
          if (b & 0x80)
            altColorMask = altColorMask | 0x02;
        }
        if (lpb.lsbAlt) {
          if (b & 0x01) {
            b = b & 0xFE;
            altColorMask = altColorMask | 0x04;
          }
        }
        if (lpb.msbAlt) {
          if (b & 0x80) {
            b = b & 0x7F;
            altColorMask = altColorMask | 0x02;
          }
        }
        switch (lpb.colorMode) {
        case 0:                         // 2 colors
          renderByte2ColorsL(buf, b, altColorMask);
          break;
        case 1:                         // 4 colors
          renderByte4ColorsL(buf, b, altColorMask);
          break;
        case 2:                         // 16 colors
          renderByte16ColorsL(buf, b, altColorMask);
          break;
        default:                        // 256 colors
          renderByte256ColorsL(buf, b);
          break;
        }
      }
      break;
    default:                            // ---- VSYNC ----
      {
        renderByte256ColorsL(buf, 0x00);
      }
      break;
    }
  }

  EP128EMU_REGPARM2 void NickRenderer_PIXEL_2::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes2Colors(buf, b1, b2, 0, 0);
  }

  EP128EMU_REGPARM2 void NickRenderer_PIXEL_2_LSBALT::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes2Colors(buf,
                       b1 & 0xFE, b2 & 0xFE,
                       (b1 & 0x01) << 2, (b2 & 0x01) << 2);
  }

  EP128EMU_REGPARM2 void NickRenderer_PIXEL_2_MSBALT::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes2Colors(buf,
                       b1 & 0x7F, b2 & 0x7F,
                       (b1 & 0x80) >> 6, (b2 & 0x80) >> 6);
  }

  EP128EMU_REGPARM2 void
      NickRenderer_PIXEL_2_LSBALT_MSBALT::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes2Colors(buf,
                       b1 & 0x7E, b2 & 0x7E,
                       ((b1 & 0x80) >> 6) | ((b1 & 0x01) << 2),
                       ((b2 & 0x80) >> 6) | ((b2 & 0x01) << 2));
  }

  EP128EMU_REGPARM2 void NickRenderer_PIXEL_4::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes4Colors(buf, b1, b2, 0, 0);
  }

  EP128EMU_REGPARM2 void NickRenderer_PIXEL_4_LSBALT::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes4Colors(buf,
                       b1 & 0xFE, b2 & 0xFE,
                       (b1 & 0x01) << 2, (b2 & 0x01) << 2);
  }

  EP128EMU_REGPARM2 void NickRenderer_PIXEL_16::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes16Colors(buf, b1, b2);
  }

  EP128EMU_REGPARM2 void NickRenderer_PIXEL_256::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes256Colors(buf, b1, b2);
  }

  EP128EMU_REGPARM2 void NickRenderer_ATTRIBUTE::doRender(uint8_t*& buf)
  {
    renderBytesAttribute(buf,
                         videoMemory[lpb.ld2Addr], videoMemory[lpb.ld1Addr]);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    lpb.ld2Addr = (lpb.ld2Addr + 1) & 0xFFFF;
  }

  EP128EMU_REGPARM2 void NickRenderer_CH256_2::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch)];
    renderByte2ColorsL(buf, b, 0);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH256_4::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch)];
    renderByte4ColorsL(buf, b, 0);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH256_16::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch)];
    renderByte16ColorsL(buf, b);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH256_256::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch)];
    renderByte256ColorsL(buf, b);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH128_2::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x7F)];
    renderByte2ColorsL(buf, b, 0);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH128_2_ALTIND1::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x7F)];
    renderByte2ColorsL(buf, b, (ch & 0x80) >> 6);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH128_4::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x7F)];
    renderByte4ColorsL(buf, b, 0);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH128_16::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x7F)];
    renderByte16ColorsL(buf, b);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH128_256::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x7F)];
    renderByte256ColorsL(buf, b);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH64_2::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x3F)];
    renderByte2ColorsL(buf, b, 0);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH64_2_ALTIND0::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x3F)];
    renderByte2ColorsL(buf, b, (ch & 0x40) >> 4);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH64_2_ALTIND1::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x3F)];
    renderByte2ColorsL(buf, b, (ch & 0x80) >> 6);
  }

  EP128EMU_REGPARM2 void
      NickRenderer_CH64_2_ALTIND0_ALTIND1::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x3F)];
    renderByte2ColorsL(buf, b, ((ch & 0x80) >> 6) + ((ch & 0x40) >> 4));
  }

  EP128EMU_REGPARM2 void NickRenderer_CH64_4::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x3F)];
    renderByte4ColorsL(buf, b, 0);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH64_4_ALTIND0::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x3F)];
    renderByte4ColorsL(buf, b, (ch & 0x40) >> 4);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH64_16::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x3F)];
    renderByte16ColorsL(buf, b);
  }

  EP128EMU_REGPARM2 void NickRenderer_CH64_256::doRender(uint8_t*& buf)
  {
    uint8_t ch = videoMemory[lpb.ld1Addr];
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = videoMemory[lpb.ld2Addr | uint16_t(ch & 0x3F)];
    renderByte256ColorsL(buf, b);
  }

  EP128EMU_REGPARM2 void NickRenderer_LPIXEL_2::doRender(uint8_t*& buf)
  {
    renderByte2ColorsL(buf, videoMemory[lpb.ld1Addr], 0);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  EP128EMU_REGPARM2 void NickRenderer_LPIXEL_2_LSBALT::doRender(uint8_t*& buf)
  {
    uint8_t b = videoMemory[lpb.ld1Addr];
    renderByte2ColorsL(buf, b & 0xFE, (b & 0x01) << 2);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  EP128EMU_REGPARM2 void NickRenderer_LPIXEL_2_MSBALT::doRender(uint8_t*& buf)
  {
    uint8_t b = videoMemory[lpb.ld1Addr];
    renderByte2ColorsL(buf, b & 0x7F, (b & 0x80) >> 6);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  EP128EMU_REGPARM2 void
      NickRenderer_LPIXEL_2_LSBALT_MSBALT::doRender(uint8_t*& buf)
  {
    uint8_t b = videoMemory[lpb.ld1Addr];
    renderByte2ColorsL(buf, b & 0x7E, ((b & 0x80) >> 6) | ((b & 0x01) << 2));
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  EP128EMU_REGPARM2 void NickRenderer_LPIXEL_4::doRender(uint8_t*& buf)
  {
    renderByte4ColorsL(buf, videoMemory[lpb.ld1Addr], 0);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  EP128EMU_REGPARM2 void NickRenderer_LPIXEL_4_LSBALT::doRender(uint8_t*& buf)
  {
    uint8_t b = videoMemory[lpb.ld1Addr];
    renderByte4ColorsL(buf, b & 0xFE, (b & 0x01) << 2);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  EP128EMU_REGPARM2 void NickRenderer_LPIXEL_16::doRender(uint8_t*& buf)
  {
    renderByte16ColorsL(buf, videoMemory[lpb.ld1Addr]);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  EP128EMU_REGPARM2 void NickRenderer_LPIXEL_256::doRender(uint8_t*& buf)
  {
    renderByte256ColorsL(buf, videoMemory[lpb.ld1Addr]);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  // --------------------------------------------------------------------------

  NickRendererTable::NickRendererTable(NickLPB& lpb,
                                       const uint8_t *videoMemory_)
  {
    t = (NickRenderer**) 0;
    try {
      t = new NickRenderer*[512];
      for (size_t i = 0; i < 512; i++)
        t[i] = (NickRenderer*) 0;
      for (size_t i = 0; i < 512; i++) {
        uint8_t videoMode = uint8_t(i) & 7;
        uint8_t colorMode = uint8_t(i >> 3) & 3;
        bool    altInd0 = bool(i & 0x0020);
        bool    altInd1 = bool(i & 0x0040);
        bool    lsbAlt  = bool(i & 0x0080);
        bool    msbAlt  = bool(i & 0x0100);
        // invalid/undocumented modes are handled by the generic renderer
        if (videoMode == 6) {
          continue;
        }
        else if (videoMode == 1 || videoMode == 7) {
          if ((colorMode < 3 && (altInd0 || altInd1)) ||
              (colorMode >= 1 && msbAlt) || (colorMode >= 2 && lsbAlt)) {
            continue;
          }
        }
        else if (videoMode == 2) {
          if (colorMode != 0 || lsbAlt || msbAlt)
            continue;
        }
        else if (videoMode != 0) {
          if (lsbAlt || msbAlt ||
              (colorMode == 1 && altInd1) ||
              ((colorMode == 2 || videoMode == 3) && (altInd0 || altInd1)) ||
              ((videoMode == 3 || videoMode == 4) && altInd0)) {
            continue;
          }
        }
        switch (videoMode) {
        case 0:
          t[i] = new NickRenderer_Blank(lpb, videoMemory_);
          break;
        case 1:
          switch (colorMode) {
          case 0:
            if (lsbAlt) {
              if (msbAlt)
                t[i] = new NickRenderer_PIXEL_2_LSBALT_MSBALT(lpb,
                                                              videoMemory_);
              else
                t[i] = new NickRenderer_PIXEL_2_LSBALT(lpb, videoMemory_);
            }
            else if (msbAlt)
              t[i] = new NickRenderer_PIXEL_2_MSBALT(lpb, videoMemory_);
            else
              t[i] = new NickRenderer_PIXEL_2(lpb, videoMemory_);
            break;
          case 1:
            if (lsbAlt)
              t[i] = new NickRenderer_PIXEL_4_LSBALT(lpb, videoMemory_);
            else
              t[i] = new NickRenderer_PIXEL_4(lpb, videoMemory_);
            break;
          case 2:
            t[i] = new NickRenderer_PIXEL_16(lpb, videoMemory_);
            break;
          case 3:
            t[i] = new NickRenderer_PIXEL_256(lpb, videoMemory_);
            break;
          }
          break;
        case 2:
          t[i] = new NickRenderer_ATTRIBUTE(lpb, videoMemory_);
          break;
        case 3:
          switch (colorMode) {
          case 0:
            t[i] = new NickRenderer_CH256_2(lpb, videoMemory_);
            break;
          case 1:
            t[i] = new NickRenderer_CH256_4(lpb, videoMemory_);
            break;
          case 2:
            t[i] = new NickRenderer_CH256_16(lpb, videoMemory_);
            break;
          case 3:
            t[i] = new NickRenderer_CH256_256(lpb, videoMemory_);
            break;
          }
          break;
        case 4:
          switch (colorMode) {
          case 0:
            if (altInd1)
              t[i] = new NickRenderer_CH128_2_ALTIND1(lpb, videoMemory_);
            else
              t[i] = new NickRenderer_CH128_2(lpb, videoMemory_);
            break;
          case 1:
            t[i] = new NickRenderer_CH128_4(lpb, videoMemory_);
            break;
          case 2:
            t[i] = new NickRenderer_CH128_16(lpb, videoMemory_);
            break;
          case 3:
            t[i] = new NickRenderer_CH128_256(lpb, videoMemory_);
            break;
          }
          break;
        case 5:
          switch (colorMode) {
          case 0:
            if (altInd0) {
              if (altInd1)
                t[i] = new NickRenderer_CH64_2_ALTIND0_ALTIND1(lpb,
                                                               videoMemory_);
              else
                t[i] = new NickRenderer_CH64_2_ALTIND0(lpb, videoMemory_);
            }
            else if (altInd1)
              t[i] = new NickRenderer_CH64_2_ALTIND1(lpb, videoMemory_);
            else
              t[i] = new NickRenderer_CH64_2(lpb, videoMemory_);
            break;
          case 1:
            if (altInd0)
              t[i] = new NickRenderer_CH64_4_ALTIND0(lpb, videoMemory_);
            else
              t[i] = new NickRenderer_CH64_4(lpb, videoMemory_);
            break;
          case 2:
            t[i] = new NickRenderer_CH64_16(lpb, videoMemory_);
            break;
          case 3:
            t[i] = new NickRenderer_CH64_256(lpb, videoMemory_);
            break;
          }
          break;
        case 7:
          switch (colorMode) {
          case 0:
            if (lsbAlt) {
              if (msbAlt)
                t[i] = new NickRenderer_LPIXEL_2_LSBALT_MSBALT(lpb,
                                                               videoMemory_);
              else
                t[i] = new NickRenderer_LPIXEL_2_LSBALT(lpb, videoMemory_);
            }
            else if (msbAlt)
              t[i] = new NickRenderer_LPIXEL_2_MSBALT(lpb, videoMemory_);
            else
              t[i] = new NickRenderer_LPIXEL_2(lpb, videoMemory_);
            break;
          case 1:
            if (lsbAlt)
              t[i] = new NickRenderer_LPIXEL_4_LSBALT(lpb, videoMemory_);
            else
              t[i] = new NickRenderer_LPIXEL_4(lpb, videoMemory_);
            break;
          case 2:
            t[i] = new NickRenderer_LPIXEL_16(lpb, videoMemory_);
            break;
          case 3:
            t[i] = new NickRenderer_LPIXEL_256(lpb, videoMemory_);
            break;
          }
          break;
        }
      }
      for (size_t i = 0; i < 512; i++) {
        if (t[i] == (NickRenderer *) 0)
          t[i] = new NickRenderer_Generic(lpb, videoMemory_);
      }
    }
    catch (...) {
      if (t) {
        for (size_t i = 0; i < 512; i++) {
          if (t[i]) {
            delete t[i];
            t[i] = (NickRenderer *) 0;
          }
        }
        delete[] t;
        t = (NickRenderer **) 0;
      }
      throw;
    }
  }

  NickRendererTable::~NickRendererTable()
  {
    if (t) {
      for (size_t i = 0; i < 512; i++) {
        if (t[i]) {
          delete t[i];
          t[i] = (NickRenderer*) 0;
        }
      }
      delete[] t;
      t = (NickRenderer**) 0;
    }
  }

  NickRenderer& NickRendererTable::getRenderer(const NickLPB& lpb)
  {
    size_t  n =    size_t(lpb.videoMode & 7)
                + (size_t(lpb.colorMode & 3) << 3)
                + (lpb.altInd0 ? 0x0020 : 0) + (lpb.altInd1 ? 0x0040 : 0)
                + (lpb.lsbAlt  ? 0x0080 : 0) + (lpb.msbAlt  ? 0x0100 : 0);
    return *(t[n]);
  }

  // --------------------------------------------------------------------------

  void Nick::runOneSlot()
  {
    if (currentSlot < 8) {
      // read LPB
      switch (currentSlot) {
      case 0:
        {
          lpb.nLines = 256 - int(videoMemory[lptCurrentAddr + 0]);
          uint8_t modeByte = videoMemory[lptCurrentAddr + 1];
          bool    irqState = bool(modeByte & 0x80);
          if (irqState != lpb.interruptFlag) {
            lpb.interruptFlag = irqState;
            irqStateChange(irqState);
          }
          lpb.colorMode = (modeByte >> 5) & 3;
          lpb.vresMode = bool(modeByte & 0x10);
          lpb.videoMode = (modeByte >> 1) & 7;
          borderColor = (lpb.videoMode != 0 ? savedBorderColor : uint8_t(0));
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
        lpb.rightMargin &= 0x3F;
        break;
      case 2:
        if (linesRemaining <= 0 || !lpb.vresMode) {
          lpb.ld1Addr = uint16_t(videoMemory[lptCurrentAddr + 4])
                        | (uint16_t(videoMemory[lptCurrentAddr + 5]) << 8);
        }
        break;
      case 3:
        {
          uint16_t  ld2Base = videoMemory[lptCurrentAddr + 6];
          ld2Base = ld2Base | (uint16_t(videoMemory[lptCurrentAddr + 7]) << 8);
          switch (lpb.videoMode) {
          case 3:                   // CH256
            ld2Base = (ld2Base << 8) & 0xFF00;
            lpb.ld2Addr = (lpb.ld2Addr + 0x0100) & 0xFFFF;
            break;
          case 4:                   // CH128
            ld2Base = (ld2Base << 7) & 0xFF80;
            lpb.ld2Addr = (lpb.ld2Addr + 0x0080) & 0xFFFF;
            break;
          case 5:                   // CH64
            ld2Base = (ld2Base << 6) & 0xFFC0;
            lpb.ld2Addr = (lpb.ld2Addr + 0x0040) & 0xFFFF;
            break;
          }
          if (linesRemaining <= 0) {
            lpb.ld2Addr = ld2Base;
            linesRemaining = lpb.nLines;
          }
        }
        break;
      case 4:
        lpb.palette[0] = videoMemory[lptCurrentAddr + 8];
        lpb.palette[1] = videoMemory[lptCurrentAddr + 9];
        break;
      case 5:
        lpb.palette[2] = videoMemory[lptCurrentAddr + 10];
        lpb.palette[3] = videoMemory[lptCurrentAddr + 11];
        break;
      case 6:
        lpb.palette[4] = videoMemory[lptCurrentAddr + 12];
        lpb.palette[5] = videoMemory[lptCurrentAddr + 13];
        break;
      case 7:
        lpb.palette[6] = videoMemory[lptCurrentAddr + 14];
        lpb.palette[7] = videoMemory[lptCurrentAddr + 15];
        break;
      }
    }
    if (currentSlot == lpb.rightMargin) {
      currentRenderer = (NickRenderer *) 0;
      if (vsyncFlag) {
        vsyncFlag = false;
        vsyncStateChange(false, currentSlot);
      }
    }
    else if (currentSlot == lpb.leftMargin) {
      currentRenderer = &(renderers.getRenderer(lpb));
      bool  wasVsync = vsyncFlag;
      vsyncFlag = (lpb.videoMode == 0);
      if (vsyncFlag != wasVsync)
        vsyncStateChange(vsyncFlag, currentSlot);
    }
    switch (currentSlot) {
    case 7:                             // begin display area
      lineBufPtr = lineBuf;
      break;
    case 55:                            // end of display area
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
      currentSlot = 0xFF;
      break;
    }
    oldLineBufPtr = lineBufPtr;
    if (!currentRenderer) {
      *(lineBufPtr++) = 0x01;
      *(lineBufPtr++) = borderColor;
    }
    else if (currentSlot >= 8 && currentSlot < 54) {
      currentRenderer->doRender(lineBufPtr);
    }
    else {
      uint8_t dummyBuf[16];
      uint8_t *dummyBufPtr = &(dummyBuf[0]);
      currentRenderer->doRender(dummyBufPtr);
      *(lineBufPtr++) = 0x01;
      *(lineBufPtr++) = lpb.palette[0];
    }
    currentSlot++;
  }

  Nick::Nick(Memory& m_)
    : renderers(lpb, m_.getVideoMemory())
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
    for (size_t i = 0; i < 16; i++)
      lpb.palette[i] = 0;
    lpb.ld1Addr = 0;
    lpb.ld2Addr = 0;
    lptBaseAddr = 0;
    lptCurrentAddr = 0;
    linesRemaining = 0;
    videoMemory = m_.getVideoMemory();
    currentRenderer = (NickRenderer*) 0;
    currentSlot = 0;
    borderColor = 0;
    dataBusState = 0xFF;
    lptFlags = 0x00;
    savedBorderColor = 0;
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
    oldLineBufPtr = lineBuf;
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
    return dataBusState;
  }

  void Nick::writePort(uint16_t portNum, uint8_t value)
  {
    dataBusState = value;
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
      // TODO: implement speaker control (bit 7)
      break;
    case 1:
      savedBorderColor = value;
      if (lpb.videoMode != 0)
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
      return savedBorderColor;
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
    buf.writeUInt32(0x04000000U);       // version number
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
    if (currentRenderer)
      buf.writeBoolean(true);
    else
      buf.writeBoolean(false);
    buf.writeByte(currentSlot);
    buf.writeByte(savedBorderColor);
    buf.writeByte(dataBusState);
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
    if (!(version >= 0x02000000U && version <= 0x04000000U)) {
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
      lptBaseAddr = uint16_t(buf.readUInt32()) & 0xFFF0;
      lptCurrentAddr = uint16_t(buf.readUInt32()) & 0xFFF0;
      linesRemaining = int(buf.readUInt32() % 257U);
      currentRenderer = (NickRenderer *) 0;
      if (buf.readBoolean())
        currentRenderer = &(renderers.getRenderer(lpb));
      currentSlot = uint8_t(buf.readByte() % 57U);
      savedBorderColor = buf.readByte();
      borderColor = (lpb.videoMode != 0 ? savedBorderColor : uint8_t(0));
      dataBusState = buf.readByte();
      clearLineBuffer();
      lineBufPtr = &(lineBuf[(currentSlot >= 7 ?
                              (currentSlot - 7) : (currentSlot + 50)) << 1]);
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

