
// plus4 -- portable Commodore PLUS/4 emulator
// Copyright (C) 2003-2007 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "cpu.hpp"
#include "ted.hpp"

#ifdef REGPARM
#  undef REGPARM
#endif
#if defined(__GNUC__) && (__GNUC__ >= 3) && defined(__i386__) && !defined(__ICC)
#  define REGPARM __attribute__ ((__regparm__ (3)))
#else
#  define REGPARM
#endif

namespace Plus4 {

  TED7360::RenderTables TED7360::renderTables;

  REGPARM void TED7360::render_BMM_hires(TED7360& ted, uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    if (nextCharCnt == 0) {
      uint8_t b = ted.currentBitmap;
      ted.bitmapHShiftRegister = b << 4;
      ted.bitmapMShiftRegister = renderTables.mcmBitmapConversionTable[b] >> 8;
      uint8_t a = ted.currentAttribute;
      uint8_t c = ted.currentCharacter;
      ted.shiftRegisterAttribute = a;
      ted.shiftRegisterCharacter = c;
      ted.shiftRegisterCursorFlag = ted.cursorFlag;
      uint8_t c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      uint8_t c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      bufp[0] = ((b & uint8_t(0x80)) ? c1 : c0);
      bufp[1] = ((b & uint8_t(0x40)) ? c1 : c0);
      bufp[2] = ((b & uint8_t(0x20)) ? c1 : c0);
      bufp[3] = ((b & uint8_t(0x10)) ? c1 : c0);
    }
    else {
      uint8_t b = ted.bitmapHShiftRegister;
      uint8_t a = ted.shiftRegisterAttribute;
      uint8_t c = ted.shiftRegisterCharacter;
      uint8_t c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      uint8_t c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      switch (nextCharCnt) {
      case 1:
        bufp[0] = ((b & uint8_t(0x80)) ? c1 : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 3;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 6;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        bufp[1] = ((b & uint8_t(0x80)) ? c1 : c0);
        bufp[2] = ((b & uint8_t(0x40)) ? c1 : c0);
        bufp[3] = ((b & uint8_t(0x20)) ? c1 : c0);
        break;
      case 2:
        bufp[0] = ((b & uint8_t(0x80)) ? c1 : c0);
        bufp[1] = ((b & uint8_t(0x40)) ? c1 : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 2;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 4;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        bufp[2] = ((b & uint8_t(0x80)) ? c1 : c0);
        bufp[3] = ((b & uint8_t(0x40)) ? c1 : c0);
        break;
      case 3:
        bufp[0] = ((b & uint8_t(0x80)) ? c1 : c0);
        bufp[1] = ((b & uint8_t(0x40)) ? c1 : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? c1 : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 1;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 2;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        bufp[3] = ((b & uint8_t(0x80)) ? c1 : c0);
        break;
      default:
        bufp[0] = ((b & uint8_t(0x80)) ? c1 : c0);
        bufp[1] = ((b & uint8_t(0x40)) ? c1 : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? c1 : c0);
        bufp[3] = ((b & uint8_t(0x10)) ? c1 : c0);
        ted.bitmapHShiftRegister = ted.bitmapHShiftRegister << 4;
        ted.bitmapMShiftRegister = ted.bitmapMShiftRegister >> 8;
        break;
      }
    }
  }

  REGPARM void TED7360::render_BMM_multicolor(TED7360& ted,
                                              uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    uint8_t c_[4];
    c_[0] = (!(ted.tedRegisterWriteMask & 0x00200000U) ?
             ted.tedRegisters[0x15] : uint8_t(0x7F));
    c_[3] = (!(ted.tedRegisterWriteMask & 0x00400000U) ?
             ted.tedRegisters[0x16] : uint8_t(0x7F));
    if (nextCharCnt == 0) {
      ted.bitmapHShiftRegister = ted.currentBitmap << 4;
      uint16_t  b = renderTables.mcmBitmapConversionTable[ted.currentBitmap];
      ted.bitmapMShiftRegister = b >> 8;
      uint8_t a = ted.currentAttribute;
      uint8_t c = ted.currentCharacter;
      ted.shiftRegisterAttribute = a;
      ted.shiftRegisterCharacter = c;
      ted.shiftRegisterCursorFlag = ted.cursorFlag;
      c_[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      c_[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      bufp[0] = c_[b & 3];
      c_[0] = ted.tedRegisters[0x15];
      c_[3] = ted.tedRegisters[0x16];
      bufp[1] = c_[(b >> 2) & 3];
      bufp[2] = c_[(b >> 4) & 3];
      bufp[3] = c_[(b >> 6) & 3];
    }
    else {
      uint16_t  b = ted.bitmapMShiftRegister;
      uint8_t   a = ted.shiftRegisterAttribute;
      uint8_t   c = ted.shiftRegisterCharacter;
      c_[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      c_[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      switch (nextCharCnt) {
      case 1:
        bufp[0] = c_[b & 3];
        ted.bitmapHShiftRegister = ted.currentBitmap << 3;
        b = renderTables.mcmBitmapConversionTable[ted.currentBitmap];
        ted.bitmapMShiftRegister = b >> 6;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        c_[0] = ted.tedRegisters[0x15];
        c_[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        c_[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        c_[3] = ted.tedRegisters[0x16];
        bufp[1] = c_[b & 3];
        bufp[2] = c_[(b >> 2) & 3];
        bufp[3] = c_[(b >> 4) & 3];
        break;
      case 2:
        bufp[0] = c_[b & 3];
        c_[0] = ted.tedRegisters[0x15];
        c_[3] = ted.tedRegisters[0x16];
        bufp[1] = c_[(b >> 2) & 3];
        ted.bitmapHShiftRegister = ted.currentBitmap << 2;
        b = renderTables.mcmBitmapConversionTable[ted.currentBitmap];
        ted.bitmapMShiftRegister = b >> 4;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        c_[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        c_[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        bufp[2] = c_[b & 3];
        bufp[3] = c_[(b >> 2) & 3];
        break;
      case 3:
        bufp[0] = c_[b & 3];
        c_[0] = ted.tedRegisters[0x15];
        c_[3] = ted.tedRegisters[0x16];
        bufp[1] = c_[(b >> 2) & 3];
        bufp[2] = c_[(b >> 4) & 3];
        ted.bitmapHShiftRegister = ted.currentBitmap << 1;
        b = renderTables.mcmBitmapConversionTable[ted.currentBitmap];
        ted.bitmapMShiftRegister = b >> 2;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        c_[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        c_[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        bufp[3] = c_[b & 3];
        break;
      default:
        bufp[0] = c_[b & 3];
        c_[0] = ted.tedRegisters[0x15];
        c_[3] = ted.tedRegisters[0x16];
        bufp[1] = c_[(b >> 2) & 3];
        bufp[2] = c_[(b >> 4) & 3];
        bufp[3] = c_[(b >> 6) & 3];
        ted.bitmapHShiftRegister = ted.bitmapHShiftRegister << 4;
        ted.bitmapMShiftRegister = ted.bitmapMShiftRegister >> 8;
        break;
      }
    }
  }

  REGPARM void TED7360::render_char_128(TED7360& ted, uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    uint8_t c0 = (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                  ted.tedRegisters[0x15] : uint8_t(0x7F));
    if (nextCharCnt == 0) {
      uint8_t b = ted.currentBitmap;
      ted.bitmapHShiftRegister = b << 4;
      ted.bitmapMShiftRegister = renderTables.mcmBitmapConversionTable[b] >> 8;
      uint8_t a = ted.currentAttribute;
      ted.shiftRegisterAttribute = a;
      ted.shiftRegisterCharacter = ted.currentCharacter;
      ted.shiftRegisterCursorFlag = ted.cursorFlag;
      if (ted.shiftRegisterCursorFlag)
        b = b ^ ted.flashState;
      else if (a & uint8_t(0x80))
        b = b & ted.flashState;
      if (ted.shiftRegisterCharacter & uint8_t(0x80))
        b = b ^ uint8_t(0xFF);
      a &= uint8_t(0x7F);
      bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
      c0 = ted.tedRegisters[0x15];
      bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
      bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
      bufp[3] = ((b & uint8_t(0x10)) ? a : c0);
    }
    else {
      uint8_t b = ted.bitmapHShiftRegister;
      uint8_t a = ted.shiftRegisterAttribute;
      if (ted.shiftRegisterCursorFlag)
        b = b ^ ted.flashState;
      else if (a & uint8_t(0x80))
        b = b & ted.flashState;
      if (ted.shiftRegisterCharacter & uint8_t(0x80))
        b = b ^ uint8_t(0xFF);
      a &= uint8_t(0x7F);
      switch (nextCharCnt) {
      case 1:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 3;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 6;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (ted.shiftRegisterCursorFlag)
          b = b ^ ted.flashState;
        else if (a & uint8_t(0x80))
          b = b & ted.flashState;
        if (ted.shiftRegisterCharacter & uint8_t(0x80))
          b = b ^ uint8_t(0xFF);
        a &= uint8_t(0x7F);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x20)) ? a : c0);
        break;
      case 2:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 2;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 4;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (ted.shiftRegisterCursorFlag)
          b = b ^ ted.flashState;
        else if (a & uint8_t(0x80))
          b = b & ted.flashState;
        if (ted.shiftRegisterCharacter & uint8_t(0x80))
          b = b ^ uint8_t(0xFF);
        a &= uint8_t(0x7F);
        bufp[2] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x40)) ? a : c0);
        break;
      case 3:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 1;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 2;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (ted.shiftRegisterCursorFlag)
          b = b ^ ted.flashState;
        else if (a & uint8_t(0x80))
          b = b & ted.flashState;
        if (ted.shiftRegisterCharacter & uint8_t(0x80))
          b = b ^ uint8_t(0xFF);
        a &= uint8_t(0x7F);
        bufp[3] = ((b & uint8_t(0x80)) ? a : c0);
        break;
      default:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x10)) ? a : c0);
        ted.bitmapHShiftRegister = ted.bitmapHShiftRegister << 4;
        ted.bitmapMShiftRegister = ted.bitmapMShiftRegister >> 8;
        break;
      }
    }
  }

  REGPARM void TED7360::render_char_256(TED7360& ted, uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    uint8_t c0 = (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                  ted.tedRegisters[0x15] : uint8_t(0x7F));
    if (nextCharCnt == 0) {
      uint8_t b = ted.currentBitmap;
      ted.bitmapHShiftRegister = b << 4;
      ted.bitmapMShiftRegister = renderTables.mcmBitmapConversionTable[b] >> 8;
      uint8_t a = ted.currentAttribute;
      ted.shiftRegisterAttribute = a;
      ted.shiftRegisterCharacter = ted.currentCharacter;
      ted.shiftRegisterCursorFlag = ted.cursorFlag;
      if (ted.shiftRegisterCursorFlag)
        b = b ^ ted.flashState;
      else if (a & uint8_t(0x80))
        b = b & ted.flashState;
      a &= uint8_t(0x7F);
      bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
      c0 = ted.tedRegisters[0x15];
      bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
      bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
      bufp[3] = ((b & uint8_t(0x10)) ? a : c0);
    }
    else {
      uint8_t b = ted.bitmapHShiftRegister;
      uint8_t a = ted.shiftRegisterAttribute;
      if (ted.shiftRegisterCursorFlag)
        b = b ^ ted.flashState;
      else if (a & uint8_t(0x80))
        b = b & ted.flashState;
      a &= uint8_t(0x7F);
      switch (nextCharCnt) {
      case 1:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 3;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 6;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (ted.shiftRegisterCursorFlag)
          b = b ^ ted.flashState;
        else if (a & uint8_t(0x80))
          b = b & ted.flashState;
        a &= uint8_t(0x7F);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x20)) ? a : c0);
        break;
      case 2:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 2;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 4;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (ted.shiftRegisterCursorFlag)
          b = b ^ ted.flashState;
        else if (a & uint8_t(0x80))
          b = b & ted.flashState;
        a &= uint8_t(0x7F);
        bufp[2] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x40)) ? a : c0);
        break;
      case 3:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 1;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 2;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (ted.shiftRegisterCursorFlag)
          b = b ^ ted.flashState;
        else if (a & uint8_t(0x80))
          b = b & ted.flashState;
        a &= uint8_t(0x7F);
        bufp[3] = ((b & uint8_t(0x80)) ? a : c0);
        break;
      default:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x10)) ? a : c0);
        ted.bitmapHShiftRegister = ted.bitmapHShiftRegister << 4;
        ted.bitmapMShiftRegister = ted.bitmapMShiftRegister >> 8;
        break;
      }
    }
  }

  REGPARM void TED7360::render_char_ECM(TED7360& ted, uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    if (nextCharCnt == 0) {
      uint8_t b = ted.currentBitmap;
      ted.bitmapHShiftRegister = b << 4;
      ted.bitmapMShiftRegister = renderTables.mcmBitmapConversionTable[b] >> 8;
      uint8_t a = ted.currentAttribute;
      uint8_t c = ted.currentCharacter;
      ted.shiftRegisterAttribute = a;
      ted.shiftRegisterCharacter = c;
      ted.shiftRegisterCursorFlag = ted.cursorFlag;
      a &= uint8_t(0x7F);
      c = (c >> 6) + uint8_t(0x15);
      uint8_t c0 = (!(ted.tedRegisterWriteMask & (1U << c)) ?
                    ted.tedRegisters[c] : uint8_t(0x7F));
      bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
      c0 = ted.tedRegisters[c];
      bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
      bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
      bufp[3] = ((b & uint8_t(0x10)) ? a : c0);
    }
    else {
      uint8_t b = ted.bitmapHShiftRegister;
      uint8_t a = ted.shiftRegisterAttribute & uint8_t(0x7F);
      uint8_t c = (ted.shiftRegisterCharacter >> 6) + uint8_t(0x15);
      uint8_t c0 = (!(ted.tedRegisterWriteMask & (1U << c)) ?
                    ted.tedRegisters[c] : uint8_t(0x7F));
      switch (nextCharCnt) {
      case 1:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 3;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 6;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        a &= uint8_t(0x7F);
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
        bufp[1] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x20)) ? a : c0);
        break;
      case 2:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[c];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 2;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 4;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        a &= uint8_t(0x7F);
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
        bufp[2] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x40)) ? a : c0);
        break;
      case 3:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[c];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 1;
        ted.bitmapMShiftRegister =
            renderTables.mcmBitmapConversionTable[b] >> 2;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        a &= uint8_t(0x7F);
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
        bufp[3] = ((b & uint8_t(0x80)) ? a : c0);
        break;
      default:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[c];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x10)) ? a : c0);
        ted.bitmapHShiftRegister = ted.bitmapHShiftRegister << 4;
        ted.bitmapMShiftRegister = ted.bitmapMShiftRegister >> 8;
        break;
      }
    }
  }

  REGPARM void TED7360::render_char_MCM(TED7360& ted, uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    uint8_t c_[4];
    c_[0] = ted.tedRegisters[0x15];
    c_[1] = ted.tedRegisters[0x16];
    c_[2] = ted.tedRegisters[0x17];
    if (nextCharCnt == 0) {
      uint8_t   b = ted.currentBitmap;
      ted.bitmapHShiftRegister = b << 4;
      uint16_t  b2 = renderTables.mcmBitmapConversionTable[b];
      ted.bitmapMShiftRegister = b2 >> 8;
      uint8_t   a = ted.currentAttribute;
      ted.shiftRegisterAttribute = a;
      c_[3] = a & uint8_t(0x77);
      ted.shiftRegisterCharacter = ted.currentCharacter;
      ted.shiftRegisterCursorFlag = ted.cursorFlag;
      if (a & uint8_t(0x08)) {
        int     tmp = b2 & 3;
        if (tmp == 3 || !(ted.tedRegisterWriteMask & (0x00200000U << tmp)))
          bufp[0] = c_[tmp];
        else
          bufp[0] = uint8_t(0x7F);
        bufp[1] = c_[(b2 >> 2) & 3];
        bufp[2] = c_[(b2 >> 4) & 3];
        bufp[3] = c_[(b2 >> 6) & 3];
      }
      else {
        bufp[0] = ((b & uint8_t(0x80)) ?
                   c_[3] : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                            c_[0] : uint8_t(0x7F)));
        bufp[1] = ((b & uint8_t(0x40)) ? c_[3] : c_[0]);
        bufp[2] = ((b & uint8_t(0x20)) ? c_[3] : c_[0]);
        bufp[3] = ((b & uint8_t(0x10)) ? c_[3] : c_[0]);
      }
    }
    else {
      uint8_t   b = ted.bitmapHShiftRegister;
      uint16_t  b2 = ted.bitmapMShiftRegister;
      uint8_t   a = ted.shiftRegisterAttribute;
      c_[3] = a & uint8_t(0x77);
      switch (nextCharCnt) {
      case 1:
        if (a & uint8_t(0x08)) {
          int     tmp = b2 & 3;
          if (tmp == 3 || !(ted.tedRegisterWriteMask & (0x00200000U << tmp)))
            bufp[0] = c_[tmp];
          else
            bufp[0] = uint8_t(0x7F);
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     c_[3] : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                              c_[0] : uint8_t(0x7F)));
        }
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 3;
        b2 = renderTables.mcmBitmapConversionTable[b];
        ted.bitmapMShiftRegister = b2 >> 6;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        c_[3] = a & uint8_t(0x77);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (a & uint8_t(0x08)) {
          bufp[1] = c_[b2 & 3];
          bufp[2] = c_[(b2 >> 2) & 3];
          bufp[3] = c_[(b2 >> 4) & 3];
        }
        else {
          bufp[1] = ((b & uint8_t(0x80)) ? c_[3] : c_[0]);
          bufp[2] = ((b & uint8_t(0x40)) ? c_[3] : c_[0]);
          bufp[3] = ((b & uint8_t(0x20)) ? c_[3] : c_[0]);
        }
        break;
      case 2:
        if (a & uint8_t(0x08)) {
          int     tmp = b2 & 3;
          if (tmp == 3 || !(ted.tedRegisterWriteMask & (0x00200000U << tmp)))
            bufp[0] = c_[tmp];
          else
            bufp[0] = uint8_t(0x7F);
          bufp[1] = c_[(b2 >> 2) & 3];
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     c_[3] : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                              c_[0] : uint8_t(0x7F)));
          bufp[1] = ((b & uint8_t(0x40)) ? c_[3] : c_[0]);
        }
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 2;
        b2 = renderTables.mcmBitmapConversionTable[b];
        ted.bitmapMShiftRegister = b2 >> 4;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        c_[3] = a & uint8_t(0x77);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (a & uint8_t(0x08)) {
          bufp[2] = c_[b2 & 3];
          bufp[3] = c_[(b2 >> 2) & 3];
        }
        else {
          bufp[2] = ((b & uint8_t(0x80)) ? c_[3] : c_[0]);
          bufp[3] = ((b & uint8_t(0x40)) ? c_[3] : c_[0]);
        }
        break;
      case 3:
        if (a & uint8_t(0x08)) {
          int     tmp = b2 & 3;
          if (tmp == 3 || !(ted.tedRegisterWriteMask & (0x00200000U << tmp)))
            bufp[0] = c_[tmp];
          else
            bufp[0] = uint8_t(0x7F);
          bufp[1] = c_[(b2 >> 2) & 3];
          bufp[2] = c_[(b2 >> 4) & 3];
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     c_[3] : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                              c_[0] : uint8_t(0x7F)));
          bufp[1] = ((b & uint8_t(0x40)) ? c_[3] : c_[0]);
          bufp[2] = ((b & uint8_t(0x20)) ? c_[3] : c_[0]);
        }
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 1;
        b2 = renderTables.mcmBitmapConversionTable[b];
        ted.bitmapMShiftRegister = b2 >> 2;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        c_[3] = a & uint8_t(0x77);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (a & uint8_t(0x08))
          bufp[3] = c_[b2 & 3];
        else
          bufp[3] = ((b & uint8_t(0x80)) ? c_[3] : c_[0]);
        break;
      default:
        if (a & uint8_t(0x08)) {
          int     tmp = b2 & 3;
          if (tmp == 3 || !(ted.tedRegisterWriteMask & (0x00200000U << tmp)))
            bufp[0] = c_[tmp];
          else
            bufp[0] = uint8_t(0x7F);
          bufp[1] = c_[(b2 >> 2) & 3];
          bufp[2] = c_[(b2 >> 4) & 3];
          bufp[3] = c_[(b2 >> 6) & 3];
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     c_[3] : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                              c_[0] : uint8_t(0x7F)));
          bufp[1] = ((b & uint8_t(0x40)) ? c_[3] : c_[0]);
          bufp[2] = ((b & uint8_t(0x20)) ? c_[3] : c_[0]);
          bufp[3] = ((b & uint8_t(0x10)) ? c_[3] : c_[0]);
        }
        ted.bitmapHShiftRegister = ted.bitmapHShiftRegister << 4;
        ted.bitmapMShiftRegister = ted.bitmapMShiftRegister >> 8;
        break;
      }
    }
  }

  REGPARM void TED7360::render_invalid_mode(TED7360& ted,
                                            uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    if ((unsigned int) nextCharCnt < 4U) {
      uint8_t b = ted.currentBitmap;
      ted.bitmapHShiftRegister = b << (4 - nextCharCnt);
      ted.bitmapMShiftRegister =
          renderTables.mcmBitmapConversionTable[b] >> ((4 - nextCharCnt) << 1);
      ted.shiftRegisterAttribute = ted.currentAttribute;
      ted.shiftRegisterCharacter = ted.currentCharacter;
      ted.shiftRegisterCursorFlag = ted.cursorFlag;
    }
    else {
      ted.bitmapHShiftRegister = ted.bitmapHShiftRegister << 4;
      ted.bitmapMShiftRegister = ted.bitmapMShiftRegister >> 8;
    }
    if (bufp) {
      bufp[0] = uint8_t(0x00);
      bufp[1] = uint8_t(0x00);
      bufp[2] = uint8_t(0x00);
      bufp[3] = uint8_t(0x00);
    }
  }

  void TED7360::selectRenderer()
  {
    uint8_t mode =   (tedRegisters[0x06] & uint8_t(0x60))
                   | (tedRegisters[0x07] & uint8_t(0x90));
    switch (mode) {
    case 0x00:
      bitmapMode = false;
      render_func = &render_char_128;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xFC)) << 8;
      characterMask = uint8_t(0x7F);
      break;
    case 0x10:
      bitmapMode = false;
      render_func = &render_char_MCM;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xFC)) << 8;
      characterMask = uint8_t(0x7F);
      break;
    case 0x20:
      bitmapMode = true;
      render_func = &render_BMM_hires;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0x30:
      bitmapMode = true;
      render_func = &render_BMM_multicolor;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0x40:
      bitmapMode = false;
      render_func = &render_char_ECM;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0x3F);
      break;
    case 0x50:
      bitmapMode = false;
      render_func = &render_invalid_mode;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0x3F);
      break;
    case 0x60:
      bitmapMode = true;
      render_func = &render_invalid_mode;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0x70:
      bitmapMode = true;
      render_func = &render_invalid_mode;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0x80:
      bitmapMode = false;
      render_func = &render_char_256;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0xFF);
      break;
    case 0x90:
      bitmapMode = false;
      render_func = &render_char_MCM;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0xFF);
      break;
    case 0xA0:
      bitmapMode = true;
      render_func = &render_BMM_hires;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0xB0:
      bitmapMode = true;
      render_func = &render_BMM_multicolor;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0xC0:
      bitmapMode = false;
      render_func = &render_char_ECM;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0x3F);
      break;
    case 0xD0:
      bitmapMode = false;
      render_func = &render_invalid_mode;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0x3F);
      break;
    case 0xE0:
      bitmapMode = true;
      render_func = &render_invalid_mode;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0xF0:
      bitmapMode = true;
      render_func = &render_invalid_mode;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    }
  }

  void TED7360::resampleAndDrawLine(uint8_t invertColors)
  {
    const uint8_t *colorTable = (uint8_t *) 0;
    if (line_buf_pos >= 487) {
      switch (invertColors) {
      case 1:
      case 2:
        colorTable = &(renderTables.colorTable_NTSC_HalfInvPhase[0]);
        break;
      case 3:
        colorTable = &(renderTables.colorTable_NTSC_InvPhase[0]);
        break;
      default:
        colorTable = &(renderTables.colorTable_NTSC[0]);
        break;
      }
    }
    else {
      switch (invertColors) {
      case 1:
      case 2:
        colorTable = &(renderTables.colorTable_HalfInvPhase[0]);
        break;
      case 3:
        colorTable = &(renderTables.colorTable_InvPhase[0]);
        break;
      }
    }
    if (line_buf_pos >= 460) {
      uint8_t tmpBuf[816];
      int     nPixels = int(((unsigned int) (line_buf_pos - 1) * 8U) / 9U);
      int     cnt = 768;
      int     cnt2 = 0;
      int     readPos = 0;
      int     writePos = 0;
      uint8_t c = 0;
      if (colorTable) {
        for (int i = 0; i < 768; i++) {
          if (!(i & 15))
            tmpBuf[writePos++] = 16;
          if (!cnt2) {
            readPos++;
            cnt2 = 8;
          }
          if (cnt >= 768) {
            cnt = cnt - 768;
            c = colorTable[line_buf[readPos++]];
            cnt2--;
          }
          cnt = cnt + nPixels;
          tmpBuf[writePos++] = c;
        }
      }
      else {
        for (int i = 0; i < 768; i++) {
          if (!(i & 15))
            tmpBuf[writePos++] = 16;
          if (!cnt2) {
            readPos++;
            cnt2 = 8;
          }
          if (cnt >= 768) {
            cnt = cnt - 768;
            c = line_buf[readPos++];
            cnt2--;
          }
          cnt = cnt + nPixels;
          tmpBuf[writePos++] = c;
        }
      }
      drawLine(&(tmpBuf[0]), 816);
    }
    else {
      if (colorTable) {
        int     cnt = 0;
        for (int i = 0; i < line_buf_pos; i++) {
          if (!cnt) {
            i++;
            cnt = 8;
          }
          cnt--;
          line_buf[i] = colorTable[line_buf[i]];
        }
      }
      drawLine(&(line_buf[0]), 432);
    }
  }

}       // namespace Plus4

// ----------------------------------------------------------------------------

static uint8_t findNearestColor(float u, float v,
                                const float *uTbl_, const float *vTbl_)
{
  float   minDiff = 1000000.0f;
  uint8_t n = 0;
  for (uint8_t i = 1; i <= 31; i++) {
    float   d = ((uTbl_[i] - u) * (uTbl_[i] - u))
                + ((vTbl_[i] - v) * (vTbl_[i] - v));
    if (d < minDiff) {
      minDiff = d;
      n = i;
    }
  }
  if (n >= 0x10)
    n = n + 0x70;
  return n;
}

namespace Plus4 {

  TED7360::RenderTables::RenderTables()
  {
    for (unsigned int i = 0x00U; i <= 0xFFU; i++) {
      unsigned int  tmp = ((i & 0x03U) << 12) | ((i & 0x0CU) << 6)
                          | (i & 0x30U) | ((i & 0xC0U) >> 6);
      mcmBitmapConversionTable[i] = uint16_t(tmp | (tmp << 2));
    }
    float   colorTable_U[32];
    float   colorTable_V[32];
    for (uint8_t i = 0x00; i <= 0x1F; i++) {
      float   r, g, b;
      TED7360::convertPixelToRGB(uint8_t(i < 0x10 ? i : (i + 0x70)), r, g, b);
      // Y = (0.299 * R) + (0.587 * G) + (0.114 * B)
      // U = 0.492 * (B - Y)
      // V = 0.877 * (R - Y)
      float   y = (0.299f * r) + (0.587f * g) + (0.114f * b);
      colorTable_U[i] = 0.492f * (b - y);
      colorTable_V[i] = 0.877f * (r - y);
    }
    for (unsigned int i = 0x00U; i <= 0xFFU; i++) {
      if ((i & 0x0FU) <= 1U) {
        colorTable_NTSC[i] = uint8_t(i & 0x7FU);
        colorTable_InvPhase[i] = uint8_t(i & 0x7FU);
        colorTable_NTSC_InvPhase[i] = uint8_t(i & 0x7FU);
        colorTable_HalfInvPhase[i] = uint8_t(i & 0x7FU);
        colorTable_NTSC_HalfInvPhase[i] = uint8_t(i & 0x7FU);
      }
      else if (!(i & 0xF0U)) {
        float   u = colorTable_U[i];
        float   v = colorTable_V[i];
        float   u_ = u * 0.8386706f + v * 0.544639f;    // -33 degrees
        float   v_ = v * 0.8386706f - u * 0.544639f;
        colorTable_NTSC[i] =
            findNearestColor(u_, v_, &(colorTable_U[0]), &(colorTable_V[0]));
        colorTable_InvPhase[i] =
            findNearestColor(u, -v, &(colorTable_U[0]), &(colorTable_V[0]));
        colorTable_NTSC_InvPhase[i] =
            findNearestColor(u_, -v_, &(colorTable_U[0]), &(colorTable_V[0]));
        colorTable_HalfInvPhase[i] =
            findNearestColor(u, 0.0f, &(colorTable_U[0]), &(colorTable_V[0]));
        colorTable_NTSC_HalfInvPhase[i] =
            findNearestColor(u_, 0.0f, &(colorTable_U[0]), &(colorTable_V[0]));
      }
      else {
        colorTable_NTSC[i] =
            uint8_t(colorTable_NTSC[i & 0x0FU] | (i & 0x70U));
        colorTable_InvPhase[i] =
            uint8_t(colorTable_InvPhase[i & 0x0FU] | (i & 0x70U));
        colorTable_NTSC_InvPhase[i] =
            uint8_t(colorTable_NTSC_InvPhase[i & 0x0FU] | (i & 0x70U));
        colorTable_HalfInvPhase[i] =
            uint8_t(colorTable_HalfInvPhase[i & 0x0FU] | (i & 0x70U));
        colorTable_NTSC_HalfInvPhase[i] =
            uint8_t(colorTable_NTSC_HalfInvPhase[i & 0x0FU] | (i & 0x70U));
      }
    }
  }

}       // namespace Plus4

