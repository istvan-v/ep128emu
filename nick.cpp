
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2006 Istvan Varga <istvanv@users.sourceforge.net>
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

#include "ep128.hpp"
#include "memory.hpp"
#include "nick.hpp"

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

  inline void NickRenderer::renderByte2ColorsL(uint8_t*& buf_, uint8_t b1,
                                               uint8_t paletteOffset)
  {
    uint8_t   *buf = buf_;
    buf[0] = 0x03;
    buf[1] = lpb.palette[0 | paletteOffset];
    buf[2] = lpb.palette[1 | paletteOffset];
    buf[3] = b1;
    buf_ = buf + 4;
  }

  inline void NickRenderer::renderByte4ColorsL(uint8_t*& buf_, uint8_t b1,
                                               uint8_t paletteOffset)
  {
    const uint8_t *pixels = &(t.fourColors[size_t(b1) << 2]);
    uint8_t   c[4];
    c[0] = lpb.palette[0 | paletteOffset];
    c[1] = lpb.palette[1 | paletteOffset];
    c[2] = lpb.palette[2 | paletteOffset];
    c[3] = lpb.palette[3 | paletteOffset];
    uint8_t   *buf = buf_;
    buf[0] = 0x04;
    buf[1] = c[pixels[0]];
    buf[2] = c[pixels[1]];
    buf[3] = c[pixels[2]];
    buf[4] = c[pixels[3]];
    buf_ = buf + 5;
  }

  inline void NickRenderer::renderByte16ColorsL(uint8_t*& buf_, uint8_t b1)
  {
    const uint8_t *pixels = &(t.sixteenColors[size_t(b1) << 1]);
    uint8_t   *buf = buf_;
    buf[0] = 0x02;
    buf[1] = lpb.palette[pixels[0]];
    buf[2] = lpb.palette[pixels[1]];
    buf_ = buf + 3;
  }

  inline void NickRenderer::renderByte256ColorsL(uint8_t*& buf_, uint8_t b1)
  {
    buf_[0] = 0x01;
    buf_[1] = b1;
    buf_ += 2;
  }

  inline void NickRenderer::renderBytes2Colors(uint8_t*& buf_,
                                               uint8_t b1, uint8_t b2,
                                               uint8_t paletteOffset1,
                                               uint8_t paletteOffset2)
  {
    uint8_t   *buf = buf_;
    buf[0] = 0x06;
    buf[1] = lpb.palette[0 | paletteOffset1];
    buf[2] = lpb.palette[1 | paletteOffset1];
    buf[3] = b1;
    buf[4] = lpb.palette[0 | paletteOffset2];
    buf[5] = lpb.palette[1 | paletteOffset2];
    buf[6] = b2;
    buf_ = buf + 7;
  }

  inline void NickRenderer::renderBytes4Colors(uint8_t*& buf_,
                                               uint8_t b1, uint8_t b2,
                                               uint8_t paletteOffset1,
                                               uint8_t paletteOffset2)
  {
    uint8_t   *buf = buf_;
    buf[0] = 0x08;
    const uint8_t *pixels = &(t.fourColors[size_t(b1) << 2]);
    uint8_t   c[4];
    c[0] = lpb.palette[0 | paletteOffset1];
    c[1] = lpb.palette[1 | paletteOffset1];
    c[2] = lpb.palette[2 | paletteOffset1];
    c[3] = lpb.palette[3 | paletteOffset1];
    buf[1] = c[pixels[0]];
    buf[2] = c[pixels[1]];
    buf[3] = c[pixels[2]];
    buf[4] = c[pixels[3]];
    pixels = &(t.fourColors[size_t(b2) << 2]);
    c[0] = lpb.palette[0 | paletteOffset2];
    c[1] = lpb.palette[1 | paletteOffset2];
    c[2] = lpb.palette[2 | paletteOffset2];
    c[3] = lpb.palette[3 | paletteOffset2];
    buf[5] = c[pixels[0]];
    buf[6] = c[pixels[1]];
    buf[7] = c[pixels[2]];
    buf[8] = c[pixels[3]];
    buf_ = buf + 9;
  }

  inline void NickRenderer::renderBytes16Colors(uint8_t*& buf_,
                                                uint8_t b1, uint8_t b2)
  {
    uint8_t   *buf = buf_;
    buf[0] = 0x04;
    const uint8_t *pixels = &(t.sixteenColors[size_t(b1) << 1]);
    buf[1] = lpb.palette[pixels[0]];
    buf[2] = lpb.palette[pixels[1]];
    pixels = &(t.sixteenColors[size_t(b2) << 1]);
    buf[3] = lpb.palette[pixels[0]];
    buf[4] = lpb.palette[pixels[1]];
    buf_ = buf + 5;
  }

  inline void NickRenderer::renderBytes256Colors(uint8_t*& buf_,
                                                 uint8_t b1, uint8_t b2)
  {
    uint8_t   *buf = buf_;
    buf[0] = 0x02;
    buf[1] = b1;
    buf[2] = b2;
    buf_ = buf + 3;
  }

  inline void NickRenderer::renderBytesAttribute(uint8_t*& buf_,
                                                 uint8_t b1, uint8_t attr)
  {
    uint8_t   *buf = buf_;
    buf[0] = 0x03;
    buf[1] = lpb.palette[attr >> 4];
    buf[2] = lpb.palette[attr & 15];
    buf[3] = b1;
    buf_ = buf + 4;
  }

  inline uint8_t NickRenderer::readVideoMemory(uint16_t addr)
  {
    return m.readRaw(uint32_t(addr) + VIDEO_MEMORY_BASE_ADDR);
  }

  // --------------------------------------------------------------------------

#ifdef REGPARM
#  undef REGPARM
#endif
#if defined(__GNUC__) && (__GNUC__ >= 3) && defined(__i386__) && !defined(__ICC)
#  define REGPARM __attribute__ ((__regparm__ (3)))
#else
#  define REGPARM
#endif

  REGPARM void NickRenderer_Blank::doRender(uint8_t*& buf)
  {
    renderByte256ColorsL(buf, 0);
  }

  REGPARM void NickRenderer_PIXEL_2::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes2Colors(buf, b1, b2, 0, 0);
  }

  REGPARM void NickRenderer_PIXEL_2_LSBALT::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes2Colors(buf,
                       b1 & 0xFE, b2 & 0xFE,
                       (b1 & 0x01) << 2, (b2 & 0x01) << 2);
  }

  REGPARM void NickRenderer_PIXEL_2_MSBALT::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes2Colors(buf,
                       b1 & 0x7F, b2 & 0x7F,
                       (b1 & 0x80) >> 6, (b2 & 0x80) >> 6);
  }

  REGPARM void NickRenderer_PIXEL_2_LSBALT_MSBALT::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes2Colors(buf,
                       b1 & 0x7E, b2 & 0x7E,
                       ((b1 & 0x80) >> 6) | ((b1 & 0x01) << 2),
                       ((b2 & 0x80) >> 6) | ((b2 & 0x01) << 2));
  }

  REGPARM void NickRenderer_PIXEL_4::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes4Colors(buf, b1, b2, 0, 0);
  }

  REGPARM void NickRenderer_PIXEL_4_LSBALT::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes4Colors(buf,
                       b1 & 0xFE, b2 & 0xFE,
                       (b1 & 0x01) << 2, (b2 & 0x01) << 2);
  }

  REGPARM void NickRenderer_PIXEL_16::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes16Colors(buf, b1, b2);
  }

  REGPARM void NickRenderer_PIXEL_256::doRender(uint8_t*& buf)
  {
    uint8_t   b1 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t   b2 = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    renderBytes256Colors(buf, b1, b2);
  }

  REGPARM void NickRenderer_ATTRIBUTE::doRender(uint8_t*& buf)
  {
    renderBytesAttribute(buf,
                         readVideoMemory(lpb.ld2Addr),
                         readVideoMemory(lpb.ld1Addr));
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    lpb.ld2Addr = (lpb.ld2Addr + 1) & 0xFFFF;
  }

  REGPARM void NickRenderer_CH256_2::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch));
    renderByte2ColorsL(buf, b, 0);
  }

  REGPARM void NickRenderer_CH256_4::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch));
    renderByte4ColorsL(buf, b, 0);
  }

  REGPARM void NickRenderer_CH256_16::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch));
    renderByte16ColorsL(buf, b);
  }

  REGPARM void NickRenderer_CH256_256::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch));
    renderByte256ColorsL(buf, b);
  }

  REGPARM void NickRenderer_CH128_2::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x7F));
    renderByte2ColorsL(buf, b, 0);
  }

  REGPARM void NickRenderer_CH128_2_ALTIND1::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x7F));
    renderByte2ColorsL(buf, b, (ch & 0x80) >> 6);
  }

  REGPARM void NickRenderer_CH128_4::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x7F));
    renderByte4ColorsL(buf, b, 0);
  }

  REGPARM void NickRenderer_CH128_16::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x7F));
    renderByte16ColorsL(buf, b);
  }

  REGPARM void NickRenderer_CH128_256::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x7F));
    renderByte256ColorsL(buf, b);
  }

  REGPARM void NickRenderer_CH64_2::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x3F));
    renderByte2ColorsL(buf, b, 0);
  }

  REGPARM void NickRenderer_CH64_2_ALTIND0::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x3F));
    renderByte2ColorsL(buf, b, (ch & 0x40) >> 4);
  }

  REGPARM void NickRenderer_CH64_2_ALTIND1::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x3F));
    renderByte2ColorsL(buf, b, (ch & 0x80) >> 6);
  }

  REGPARM void NickRenderer_CH64_2_ALTIND0_ALTIND1::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x3F));
    renderByte2ColorsL(buf, b, ((ch & 0x80) >> 6) + ((ch & 0x40) >> 4));
  }

  REGPARM void NickRenderer_CH64_4::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x3F));
    renderByte4ColorsL(buf, b, 0);
  }

  REGPARM void NickRenderer_CH64_4_ALTIND0::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x3F));
    renderByte4ColorsL(buf, b, (ch & 0x40) >> 4);
  }

  REGPARM void NickRenderer_CH64_16::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x3F));
    renderByte16ColorsL(buf, b);
  }

  REGPARM void NickRenderer_CH64_256::doRender(uint8_t*& buf)
  {
    uint8_t ch = readVideoMemory(lpb.ld1Addr);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
    uint8_t b = readVideoMemory(lpb.ld2Addr | uint16_t(ch & 0x3F));
    renderByte256ColorsL(buf, b);
  }

  REGPARM void NickRenderer_INVALID_MODE::doRender(uint8_t*& buf)
  {
    renderByte256ColorsL(buf, 0);
  }

  REGPARM void NickRenderer_LPIXEL_2::doRender(uint8_t*& buf)
  {
    renderByte2ColorsL(buf, readVideoMemory(lpb.ld1Addr), 0);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  REGPARM void NickRenderer_LPIXEL_2_LSBALT::doRender(uint8_t*& buf)
  {
    uint8_t b = readVideoMemory(lpb.ld1Addr);
    renderByte2ColorsL(buf, b & 0xFE, (b & 0x01) << 2);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  REGPARM void NickRenderer_LPIXEL_2_MSBALT::doRender(uint8_t*& buf)
  {
    uint8_t b = readVideoMemory(lpb.ld1Addr);
    renderByte2ColorsL(buf, b & 0x7F, (b & 0x80) >> 6);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  REGPARM void NickRenderer_LPIXEL_2_LSBALT_MSBALT::doRender(uint8_t*& buf)
  {
    uint8_t b = readVideoMemory(lpb.ld1Addr);
    renderByte2ColorsL(buf, b & 0x7E, ((b & 0x80) >> 6) | ((b & 0x01) << 2));
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  REGPARM void NickRenderer_LPIXEL_4::doRender(uint8_t*& buf)
  {
    renderByte4ColorsL(buf, readVideoMemory(lpb.ld1Addr), 0);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  REGPARM void NickRenderer_LPIXEL_4_LSBALT::doRender(uint8_t*& buf)
  {
    uint8_t b = readVideoMemory(lpb.ld1Addr);
    renderByte4ColorsL(buf, b & 0xFE, (b & 0x01) << 2);
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  REGPARM void NickRenderer_LPIXEL_16::doRender(uint8_t*& buf)
  {
    renderByte16ColorsL(buf, readVideoMemory(lpb.ld1Addr));
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

  REGPARM void NickRenderer_LPIXEL_256::doRender(uint8_t*& buf)
  {
    renderByte256ColorsL(buf, readVideoMemory(lpb.ld1Addr));
    lpb.ld1Addr = (lpb.ld1Addr + 1) & 0xFFFF;
  }

#ifdef REGPARM
#  undef REGPARM
#endif

  // --------------------------------------------------------------------------

  NickRendererTable::NickRendererTable(NickLPB& lpb, Memory& m)
  {
    t = (NickRenderer**) 0;
    try {
      t = new NickRenderer*[512];
      for (size_t i = 0; i < 512; i++)
        t[i] = (NickRenderer*) 0;
      for (size_t i = 0; i < 512; i++) {
        uint8_t videoMode = uint8_t(i) & 7;
        uint8_t colorMode = uint8_t(i >> 3) & 3;
        bool    altInd0 = !!(i & 0x0020);
        bool    altInd1 = !!(i & 0x0040);
        bool    lsbAlt  = !!(i & 0x0080);
        bool    msbAlt  = !!(i & 0x0100);
        switch (videoMode) {
        case 0:
          t[i] = new NickRenderer_Blank(lpb, m);
          break;
        case 1:
          switch (colorMode) {
          case 0:
            if (lsbAlt) {
              if (msbAlt)
                t[i] = new NickRenderer_PIXEL_2_LSBALT_MSBALT(lpb, m);
              else
                t[i] = new NickRenderer_PIXEL_2_LSBALT(lpb, m);
            }
            else if (msbAlt)
              t[i] = new NickRenderer_PIXEL_2_MSBALT(lpb, m);
            else
              t[i] = new NickRenderer_PIXEL_2(lpb, m);
            break;
          case 1:
            if (lsbAlt)
              t[i] = new NickRenderer_PIXEL_4_LSBALT(lpb, m);
            else
              t[i] = new NickRenderer_PIXEL_4(lpb, m);
            break;
          case 2:
            t[i] = new NickRenderer_PIXEL_16(lpb, m);
            break;
          case 3:
            t[i] = new NickRenderer_PIXEL_256(lpb, m);
            break;
          }
          break;
        case 2:
          t[i] = new NickRenderer_ATTRIBUTE(lpb, m);
          break;
        case 3:
          switch (colorMode) {
          case 0:
            t[i] = new NickRenderer_CH256_2(lpb, m);
            break;
          case 1:
            t[i] = new NickRenderer_CH256_4(lpb, m);
            break;
          case 2:
            t[i] = new NickRenderer_CH256_16(lpb, m);
            break;
          case 3:
            t[i] = new NickRenderer_CH256_256(lpb, m);
            break;
          }
          break;
        case 4:
          switch (colorMode) {
          case 0:
            if (altInd1)
              t[i] = new NickRenderer_CH128_2_ALTIND1(lpb, m);
            else
              t[i] = new NickRenderer_CH128_2(lpb, m);
            break;
          case 1:
            t[i] = new NickRenderer_CH128_4(lpb, m);
            break;
          case 2:
            t[i] = new NickRenderer_CH128_16(lpb, m);
            break;
          case 3:
            t[i] = new NickRenderer_CH128_256(lpb, m);
            break;
          }
          break;
        case 5:
          switch (colorMode) {
          case 0:
            if (altInd0) {
              if (altInd1)
                t[i] = new NickRenderer_CH64_2_ALTIND0_ALTIND1(lpb, m);
              else
                t[i] = new NickRenderer_CH64_2_ALTIND0(lpb, m);
            }
            else if (altInd1)
              t[i] = new NickRenderer_CH64_2_ALTIND1(lpb, m);
            else
              t[i] = new NickRenderer_CH64_2(lpb, m);
            break;
          case 1:
            if (altInd0)
              t[i] = new NickRenderer_CH64_4_ALTIND0(lpb, m);
            else
              t[i] = new NickRenderer_CH64_4(lpb, m);
            break;
          case 2:
            t[i] = new NickRenderer_CH64_16(lpb, m);
            break;
          case 3:
            t[i] = new NickRenderer_CH64_256(lpb, m);
            break;
          }
          break;
        case 6:
          t[i] = new NickRenderer_INVALID_MODE(lpb, m);
          break;
        case 7:
          switch (colorMode) {
          case 0:
            if (lsbAlt) {
              if (msbAlt)
                t[i] = new NickRenderer_LPIXEL_2_LSBALT_MSBALT(lpb, m);
              else
                t[i] = new NickRenderer_LPIXEL_2_LSBALT(lpb, m);
            }
            else if (msbAlt)
              t[i] = new NickRenderer_LPIXEL_2_MSBALT(lpb, m);
            else
              t[i] = new NickRenderer_LPIXEL_2(lpb, m);
            break;
          case 1:
            if (lsbAlt)
              t[i] = new NickRenderer_LPIXEL_4_LSBALT(lpb, m);
            else
              t[i] = new NickRenderer_LPIXEL_4(lpb, m);
            break;
          case 2:
            t[i] = new NickRenderer_LPIXEL_16(lpb, m);
            break;
          case 3:
            t[i] = new NickRenderer_LPIXEL_256(lpb, m);
            break;
          }
          break;
        }
      }
    }
    catch (...) {
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

  inline uint8_t Nick::readVideoMemory(uint16_t addr)
  {
    return m.readRaw(uint32_t(addr) + VIDEO_MEMORY_BASE_ADDR);
  }

  void Nick::runOneSlot()
  {
    if (currentSlot < 8) {
      // read LPB
      switch (currentSlot) {
      case 0:
        {
          lpb.nLines = 256 - int(readVideoMemory(lptCurrentAddr + 0));
          if (linesRemaining <= 0)
            linesRemaining = lpb.nLines;
          uint8_t modeByte = readVideoMemory(lptCurrentAddr + 1);
          bool    irqState = !!(modeByte & 0x80);
          if (irqState != lpb.interruptFlag) {
            lpb.interruptFlag = irqState;
            irqStateChange(irqState);
          }
          lpb.colorMode = (modeByte >> 5) & 3;
          lpb.vresMode = !!(modeByte & 0x10);
          lpb.videoMode = (modeByte >> 1) & 7;
          lpb.reloadFlag = !!(modeByte & 0x01);
        }
        break;
      case 1:
        lpb.leftMargin = readVideoMemory(lptCurrentAddr + 2);
        lpb.lsbAlt = !!(lpb.leftMargin & 0x40);
        lpb.msbAlt = !!(lpb.leftMargin & 0x80);
        lpb.leftMargin &= 0x3F;
        lpb.rightMargin = readVideoMemory(lptCurrentAddr + 3);
        lpb.altInd1 = !!(lpb.rightMargin & 0x40);
        lpb.altInd0 = !!(lpb.rightMargin & 0x80);
        lpb.rightMargin &= 0x3F;
        break;
      case 2:
        lpb.ld1Base =    uint16_t(readVideoMemory(lptCurrentAddr + 4))
                      | (uint16_t(readVideoMemory(lptCurrentAddr + 5)) << 8);
        if (linesRemaining == lpb.nLines || !lpb.vresMode)
          lpb.ld1Addr = lpb.ld1Base;
        break;
      case 3:
        lpb.ld2Base =    uint16_t(readVideoMemory(lptCurrentAddr + 6))
                      | (uint16_t(readVideoMemory(lptCurrentAddr + 7)) << 8);
        switch (lpb.videoMode) {
        case 3:                     // CH256
          lpb.ld2Base &= 0xFF00;
          lpb.ld2Addr = (lpb.ld2Addr + 0x0100) & 0xFFFF;
          break;
        case 4:                     // CH128
          lpb.ld2Base &= 0xFF80;
          lpb.ld2Addr = (lpb.ld2Addr + 0x0080) & 0xFFFF;
          break;
        case 5:                     // CH64
          lpb.ld2Base &= 0xFFC0;
          lpb.ld2Addr = (lpb.ld2Addr + 0x0040) & 0xFFFF;
          break;
        }
        if (linesRemaining == lpb.nLines)
          lpb.ld2Addr = lpb.ld2Base;
        break;
      case 4:
        lpb.palette[0] = readVideoMemory(lptCurrentAddr + 8);
        lpb.palette[1] = readVideoMemory(lptCurrentAddr + 9);
        break;
      case 5:
        lpb.palette[2] = readVideoMemory(lptCurrentAddr + 10);
        lpb.palette[3] = readVideoMemory(lptCurrentAddr + 11);
        break;
      case 6:
        lpb.palette[4] = readVideoMemory(lptCurrentAddr + 12);
        lpb.palette[5] = readVideoMemory(lptCurrentAddr + 13);
        break;
      case 7:
        lpb.palette[6] = readVideoMemory(lptCurrentAddr + 14);
        lpb.palette[7] = readVideoMemory(lptCurrentAddr + 15);
        break;
      }
    }
    else if (currentSlot < 54) {
      // display area
      if (!currentRenderer) {
        if (currentSlot >= lpb.leftMargin) {
          currentRenderer = &(renderers.getRenderer(lpb));
          if (lpb.videoMode == 0)
            vsyncStateChange(true, currentSlot);
        }
      }
      else {
        if (currentSlot >= lpb.rightMargin) {
          currentRenderer = (NickRenderer *) 0;
          if (lpb.videoMode == 0)
            vsyncStateChange(false, currentSlot);
        }
      }
      if (!currentRenderer) {
        *(lineBufPtr++) = 0x01;
        *(lineBufPtr++) = borderColor;
      }
      else {
        currentRenderer->doRender(lineBufPtr);
      }
    }
    else if (currentSlot == 54) {
      if (lpb.videoMode != 0)
        currentRenderer = (NickRenderer *) 0;
      size_t  nBytes = size_t(lineBufPtr - lineBuf);
      lineBufPtr = lineBuf;
      for (size_t i = nBytes; (i & 3) != 0; i++)
        lineBuf[i] = 0;
      drawLine(lineBuf, nBytes);
    }
    else if (currentSlot == 56) {
      if (lptClockEnabled) {
        if (--linesRemaining == 0) {
          if (!lpb.reloadFlag)
            lptCurrentAddr = (lptCurrentAddr + 0x0010) & 0xFFF0;
          else
            lptCurrentAddr = lptBaseAddr;
        }
      }
      currentSlot = 0;
      currentSlot--;
    }
    currentSlot++;
  }

  Nick::Nick(Memory& m_)
    : m(m_), renderers(lpb, m_)
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
    lpb.ld1Base = 0;
    lpb.ld2Base = 0;
    for (size_t i = 0; i < 16; i++)
      lpb.palette[i] = 0;
    lpb.ld1Addr = 0;
    lpb.ld2Addr = 0;
    lptBaseAddr = 0;
    lptCurrentAddr = 0;
    linesRemaining = 0;
    currentRenderer = (NickRenderer*) 0;
    currentSlot = 0;
    borderColor = 0;
    lptClockEnabled = true;
    try {
      uint32_t  *p = new uint32_t[196];     // for 782 bytes (46 * 17)
      for (size_t i = 0; i < 196; i++)
        p[i] = 0;
      lineBuf = reinterpret_cast<uint8_t *>(p);
      lineBufPtr = lineBuf;
    }
    catch (...) {
      lineBuf = (uint8_t *) 0;
      throw;
    }
  }

  Nick::~Nick()
  {
    if (lineBuf) {
      delete[] reinterpret_cast<uint32_t *>(lineBuf);
      lineBuf = (uint8_t *) 0;
    }
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

  void Nick::writePort(uint16_t portNum, uint8_t value)
  {
    switch (portNum & 3) {
    case 0:
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
      borderColor = value;
      break;
    case 2:
      lptBaseAddr = (lptBaseAddr & 0xF000) | (uint16_t(value) << 4);
      break;
    case 3:
      lptBaseAddr = (lptBaseAddr & 0x0FF0) | (uint16_t(value & 0x0F) << 12);
      lptClockEnabled = !!(value & 0x40);
      if (!lptClockEnabled)
        linesRemaining = 0;
      if (!(value & 0x80))
        lptCurrentAddr = lptBaseAddr;
      break;
    }
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

  class ChunkType_NickSnapshot : public File::ChunkTypeHandler {
   private:
    Nick&   ref;
   public:
    ChunkType_NickSnapshot(Nick& ref_)
      : File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_NickSnapshot()
    {
    }
    virtual File::ChunkType getChunkType() const
    {
      return File::EP128EMU_CHUNKTYPE_NICK_STATE;
    }
    virtual void processChunk(File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void Nick::saveState(File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
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
    buf.writeUInt32(lpb.ld1Base);
    buf.writeUInt32(lpb.ld2Base);
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
    buf.writeByte(borderColor);
    buf.writeBoolean(lptClockEnabled);
  }

  void Nick::saveState(File& f)
  {
    File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(File::EP128EMU_CHUNKTYPE_NICK_STATE, buf);
  }

  void Nick::loadState(File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Exception("incompatible Nick snapshot format");
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
      lpb.ld1Base = uint16_t(buf.readUInt32()) & 0xFFFF;
      lpb.ld2Base = uint16_t(buf.readUInt32()) & 0xFFFF;
      switch (lpb.videoMode) {
      case 3:                   // CH256
        lpb.ld2Base &= 0xFF00;
        break;
      case 4:                   // CH128
        lpb.ld2Base &= 0xFF80;
        break;
      case 5:                   // CH64
        lpb.ld2Base &= 0xFFC0;
        break;
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
      borderColor = buf.readByte();
      lineBufPtr = lineBuf;
      if (currentSlot > 8 && currentSlot <= 54) {
        size_t  i = currentSlot - 8;
        do {
          *(lineBufPtr++) = 0x01;
          *(lineBufPtr++) = borderColor;
        } while (--i);
      }
      lptClockEnabled = buf.readBoolean();
      if (buf.getPosition() != buf.getDataSize())
        throw Exception("trailing garbage at end of Nick snapshot data");
    }
    catch (...) {
      // reset NICK
      writePort(0, 0);
      writePort(1, 0);
      writePort(2, 0);
      writePort(3, 0);
      currentSlot = 0;
      lineBufPtr = lineBuf;
      throw;
    }
  }

  void Nick::registerChunkType(File& f)
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

