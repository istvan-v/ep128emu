
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

  REGPARM void TED7360::render_BMM_hires(TED7360& ted, uint8_t *bufp, int offs)
  {
    if ((offs & 7) >= 3) {
      // faster code for the case when all pixels are in the same character
      uint8_t a, c;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      uint8_t c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      uint8_t c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      uint8_t b = uint8_t(ted.bitmapHShiftRegister >> (offs - 3));
      bufp[0] = ((b & uint8_t(0x08)) ? c1 : c0);
      bufp[1] = ((b & uint8_t(0x04)) ? c1 : c0);
      bufp[2] = ((b & uint8_t(0x02)) ? c1 : c0);
      bufp[3] = ((b & uint8_t(0x01)) ? c1 : c0);
    }
    else {
      uint8_t a, c;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      uint8_t c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      uint8_t c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      bufp[0] = ((ted.bitmapHShiftRegister & (1U << offs)) ? c1 : c0);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      bufp[1] = ((ted.bitmapHShiftRegister & (1U << offs)) ? c1 : c0);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      bufp[2] = ((ted.bitmapHShiftRegister & (1U << offs)) ? c1 : c0);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      bufp[3] = ((ted.bitmapHShiftRegister & (1U << offs)) ? c1 : c0);
    }
  }

  REGPARM void TED7360::render_BMM_multicolor(TED7360& ted,
                                              uint8_t *bufp, int offs)
  {
    uint8_t a, c;
    uint8_t c0 = (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                  ted.tedRegisters[0x15] : uint8_t(0xFF));
    uint8_t c3 = (!(ted.tedRegisterWriteMask & 0x00400000U) ?
                  ted.tedRegisters[0x16] : uint8_t(0xFF));
    if ((offs & 7) >= 3) {
      // faster code for the case when all pixels are in the same character
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      uint8_t c_[4];
      c_[0] = c0;
      c_[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      c_[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      c_[3] = c3;
      uint8_t b0 = uint8_t(ted.bitmapM0ShiftRegister >> (offs - 3));
      uint8_t b1 = uint8_t(ted.bitmapM1ShiftRegister >> (offs - 3));
      bufp[0] = c_[((b1 & uint8_t(0x08)) >> 2) | ((b0 & uint8_t(0x08)) >> 3)];
      c_[0] = ted.tedRegisters[0x15];
      c_[3] = ted.tedRegisters[0x16];
      bufp[1] = c_[((b1 & uint8_t(0x04)) >> 1) | ((b0 & uint8_t(0x04)) >> 2)];
      bufp[2] = c_[(b1 & uint8_t(0x02)) | ((b0 & uint8_t(0x02)) >> 1)];
      bufp[3] = c_[((b1 & uint8_t(0x01)) << 1) | (b0 & uint8_t(0x01))];
    }
    else {
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      if (!(ted.bitmapM1ShiftRegister & (1U << offs))) {
        if (!(ted.bitmapM0ShiftRegister & (1U << offs)))        // color 0
          bufp[0] = c0;
        else                                                    // color 1
          bufp[0] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      }
      else {
        if (!(ted.bitmapM0ShiftRegister & (1U << offs)))        // color 2
          bufp[0] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        else                                                    // color 3
          bufp[0] = c3;
      }
      offs--;
      c0 = ted.tedRegisters[0x15];
      c3 = ted.tedRegisters[0x16];
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      if (!(ted.bitmapM1ShiftRegister & (1U << offs))) {
        if (!(ted.bitmapM0ShiftRegister & (1U << offs)))        // color 0
          bufp[1] = c0;
        else                                                    // color 1
          bufp[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      }
      else {
        if (!(ted.bitmapM0ShiftRegister & (1U << offs)))        // color 2
          bufp[1] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        else                                                    // color 3
          bufp[1] = c3;
      }
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      if (!(ted.bitmapM1ShiftRegister & (1U << offs))) {
        if (!(ted.bitmapM0ShiftRegister & (1U << offs)))        // color 0
          bufp[2] = c0;
        else                                                    // color 1
          bufp[2] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      }
      else {
        if (!(ted.bitmapM0ShiftRegister & (1U << offs)))        // color 2
          bufp[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        else                                                    // color 3
          bufp[2] = c3;
      }
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      if (!(ted.bitmapM1ShiftRegister & (1U << offs))) {
        if (!(ted.bitmapM0ShiftRegister & (1U << offs)))        // color 0
          bufp[3] = c0;
        else                                                    // color 1
          bufp[3] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      }
      else {
        if (!(ted.bitmapM0ShiftRegister & (1U << offs)))        // color 2
          bufp[3] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        else                                                    // color 3
          bufp[3] = c3;
      }
    }
  }

  REGPARM void TED7360::render_char_128(TED7360& ted, uint8_t *bufp, int offs)
  {
    uint8_t a, c;
    uint8_t c0 = (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                  ted.tedRegisters[0x15] : uint8_t(0xFF));
    if ((offs & 7) >= 3) {
      // faster code for the case when all pixels are in the same character
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      uint8_t b = uint8_t(ted.bitmapHShiftRegister >> (offs - 3));
      if (ted.cursorShiftRegister & (1U << offs))
        b = b ^ ted.flashState;
      else if (a & uint8_t(0x80))
        b = b & ted.flashState;
      if (c & uint8_t(0x80))
        b = b ^ uint8_t(0xFF);
      bufp[0] = ((b & uint8_t(0x08)) ? a : c0);
      c0 = ted.tedRegisters[0x15];
      bufp[1] = ((b & uint8_t(0x04)) ? a : c0);
      bufp[2] = ((b & uint8_t(0x02)) ? a : c0);
      bufp[3] = ((b & uint8_t(0x01)) ? a : c0);
    }
    else {
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      bool    b = bool(ted.bitmapHShiftRegister & (1U << offs));
      if (ted.cursorShiftRegister & (1U << offs))
        b = (ted.flashState ? (!b) : b);
      else if (a & uint8_t(0x80))
        b = (ted.flashState ? b : false);
      bufp[0] = ((!(c & uint8_t(0x80)) ? b : !b) ? a : c0);
      c0 = ted.tedRegisters[0x15];
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      b = bool(ted.bitmapHShiftRegister & (1U << offs));
      if (ted.cursorShiftRegister & (1U << offs))
        b = (ted.flashState ? (!b) : b);
      else if (a & uint8_t(0x80))
        b = (ted.flashState ? b : false);
      bufp[1] = ((!(c & uint8_t(0x80)) ? b : !b) ? a : c0);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      b = bool(ted.bitmapHShiftRegister & (1U << offs));
      if (ted.cursorShiftRegister & (1U << offs))
        b = (ted.flashState ? (!b) : b);
      else if (a & uint8_t(0x80))
        b = (ted.flashState ? b : false);
      bufp[2] = ((!(c & uint8_t(0x80)) ? b : !b) ? a : c0);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      b = bool(ted.bitmapHShiftRegister & (1U << offs));
      if (ted.cursorShiftRegister & (1U << offs))
        b = (ted.flashState ? (!b) : b);
      else if (a & uint8_t(0x80))
        b = (ted.flashState ? b : false);
      bufp[3] = ((!(c & uint8_t(0x80)) ? b : !b) ? a : c0);
    }
  }

  REGPARM void TED7360::render_char_256(TED7360& ted, uint8_t *bufp, int offs)
  {
    uint8_t a;
    uint8_t c0 = (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                  ted.tedRegisters[0x15] : uint8_t(0xFF));
    if ((offs & 7) >= 3) {
      // faster code for the case when all pixels are in the same character
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      uint8_t b = uint8_t(ted.bitmapHShiftRegister >> (offs - 3));
      if (ted.cursorShiftRegister & (1U << offs))
        b = b ^ ted.flashState;
      else if (a & uint8_t(0x80))
        b = b & ted.flashState;
      bufp[0] = ((b & uint8_t(0x08)) ? a : c0);
      c0 = ted.tedRegisters[0x15];
      bufp[1] = ((b & uint8_t(0x04)) ? a : c0);
      bufp[2] = ((b & uint8_t(0x02)) ? a : c0);
      bufp[3] = ((b & uint8_t(0x01)) ? a : c0);
    }
    else {
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      bool    b = bool(ted.bitmapHShiftRegister & (1U << offs));
      if (ted.cursorShiftRegister & (1U << offs))
        b = (ted.flashState ? (!b) : b);
      else if (a & uint8_t(0x80))
        b = (ted.flashState ? b : false);
      bufp[0] = (b ? a : c0);
      c0 = ted.tedRegisters[0x15];
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      b = bool(ted.bitmapHShiftRegister & (1U << offs));
      if (ted.cursorShiftRegister & (1U << offs))
        b = (ted.flashState ? (!b) : b);
      else if (a & uint8_t(0x80))
        b = (ted.flashState ? b : false);
      bufp[1] = (b ? a : c0);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      b = bool(ted.bitmapHShiftRegister & (1U << offs));
      if (ted.cursorShiftRegister & (1U << offs))
        b = (ted.flashState ? (!b) : b);
      else if (a & uint8_t(0x80))
        b = (ted.flashState ? b : false);
      bufp[2] = (b ? a : c0);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      b = bool(ted.bitmapHShiftRegister & (1U << offs));
      if (ted.cursorShiftRegister & (1U << offs))
        b = (ted.flashState ? (!b) : b);
      else if (a & uint8_t(0x80))
        b = (ted.flashState ? b : false);
      bufp[3] = (b ? a : c0);
    }
  }

  REGPARM void TED7360::render_char_ECM(TED7360& ted, uint8_t *bufp, int offs)
  {
    uint8_t a, c, c0;
    if ((offs & 7) >= 3) {
      // faster code for the case when all pixels are in the same character
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      uint8_t b = uint8_t(ted.bitmapHShiftRegister >> (offs - 3));
      c0 = (c >> 6) + uint8_t(0x15);
      c0 = (!(ted.tedRegisterWriteMask & (1U << c0)) ?
            ted.tedRegisters[c0] : uint8_t(0xFF));
      bufp[0] = ((b & uint8_t(0x08)) ? a : c0);
      c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
      bufp[1] = ((b & uint8_t(0x04)) ? a : c0);
      bufp[2] = ((b & uint8_t(0x02)) ? a : c0);
      bufp[3] = ((b & uint8_t(0x01)) ? a : c0);
    }
    else {
      bool    b;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c0 = (c >> 6) + uint8_t(0x15);
      c0 = (!(ted.tedRegisterWriteMask & (1U << c0)) ?
            ted.tedRegisters[c0] : uint8_t(0xFF));
      b = bool(ted.bitmapHShiftRegister & (1U << offs));
      bufp[0] = (b ? a : c0);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      b = bool(ted.bitmapHShiftRegister & (1U << offs));
      bufp[1] = (b ? a : ted.tedRegisters[(c >> 6) + uint8_t(0x15)]);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      b = bool(ted.bitmapHShiftRegister & (1U << offs));
      bufp[2] = (b ? a : ted.tedRegisters[(c >> 6) + uint8_t(0x15)]);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      c = uint8_t(ted.characterShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      b = bool(ted.bitmapHShiftRegister & (1U << offs));
      bufp[3] = (b ? a : ted.tedRegisters[(c >> 6) + uint8_t(0x15)]);
    }
  }

  REGPARM void TED7360::render_char_MCM(TED7360& ted, uint8_t *bufp, int offs)
  {
    uint8_t a;
    if ((offs & 7) >= 3) {
      // faster code for the case when all pixels are in the same character
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      if (a & uint8_t(0x08)) {
        uint8_t b0 = uint8_t(ted.bitmapM0ShiftRegister >> (offs - 3));
        uint8_t b1 = uint8_t(ted.bitmapM1ShiftRegister >> (offs - 3));
        uint8_t c_[4];
        c_[0] = ted.tedRegisters[0x15];
        c_[1] = ted.tedRegisters[0x16];
        c_[2] = ted.tedRegisters[0x17];
        c_[3] = a & uint8_t(0x77);
        uint8_t tmp = ((b1 & uint8_t(0x08)) >> 2) | ((b0 & uint8_t(0x08)) >> 3);
        if (tmp == 3 || !(ted.tedRegisterWriteMask & (0x00200000U << tmp)))
          bufp[0] = c_[tmp];
        else
          bufp[0] = uint8_t(0xFF);
        bufp[1] = c_[((b1 & uint8_t(0x04)) >> 1) | ((b0 & uint8_t(0x04)) >> 2)];
        bufp[2] = c_[(b1 & uint8_t(0x02)) | ((b0 & uint8_t(0x02)) >> 1)];
        bufp[3] = c_[((b1 & uint8_t(0x01)) << 1) | (b0 & uint8_t(0x01))];
      }
      else {
        uint8_t c0 = (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                      ted.tedRegisters[0x15] : uint8_t(0xFF));
        uint8_t b = uint8_t(ted.bitmapHShiftRegister >> (offs - 3));
        bufp[0] = ((b & uint8_t(0x08)) ? a : c0);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x04)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x02)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x01)) ? a : c0);
      }
    }
    else {
      uint8_t c0 = (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                    ted.tedRegisters[0x15] : uint8_t(0xFF));
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      if (a & uint8_t(0x08)) {
        if (!(ted.bitmapM1ShiftRegister & (1U << offs))) {
          if (!(ted.bitmapM0ShiftRegister & (1U << offs)))      // color 0
            bufp[0] = c0;
          else                                                  // color 1
            bufp[0] = (!(ted.tedRegisterWriteMask & 0x00400000U) ?
                       ted.tedRegisters[0x16] : uint8_t(0xFF));
        }
        else {
          if (!(ted.bitmapM0ShiftRegister & (1U << offs)))      // color 2
            bufp[0] = (!(ted.tedRegisterWriteMask & 0x00800000U) ?
                       ted.tedRegisters[0x17] : uint8_t(0xFF));
          else                                                  // color 3
            bufp[0] = a & uint8_t(0x77);
        }
      }
      else
        bufp[0] = ((ted.bitmapHShiftRegister & (1U << offs)) ? a : c0);
      offs--;
      c0 = ted.tedRegisters[0x15];
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      if (a & uint8_t(0x08)) {
        if (!(ted.bitmapM1ShiftRegister & (1U << offs))) {
          if (!(ted.bitmapM0ShiftRegister & (1U << offs)))      // color 0
            bufp[1] = c0;
          else                                                  // color 1
            bufp[1] = ted.tedRegisters[0x16];
        }
        else {
          if (!(ted.bitmapM0ShiftRegister & (1U << offs)))      // color 2
            bufp[1] = ted.tedRegisters[0x17];
          else                                                  // color 3
            bufp[1] = a & uint8_t(0x77);
        }
      }
      else
        bufp[1] = ((ted.bitmapHShiftRegister & (1U << offs)) ? a : c0);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      if (a & uint8_t(0x08)) {
        if (!(ted.bitmapM1ShiftRegister & (1U << offs))) {
          if (!(ted.bitmapM0ShiftRegister & (1U << offs)))      // color 0
            bufp[2] = c0;
          else                                                  // color 1
            bufp[2] = ted.tedRegisters[0x16];
        }
        else {
          if (!(ted.bitmapM0ShiftRegister & (1U << offs)))      // color 2
            bufp[2] = ted.tedRegisters[0x17];
          else                                                  // color 3
            bufp[2] = a & uint8_t(0x77);
        }
      }
      else
        bufp[2] = ((ted.bitmapHShiftRegister & (1U << offs)) ? a : c0);
      offs--;
      a = uint8_t(ted.attributeShiftRegister >> (offs & 0x18)) & uint8_t(0xFF);
      if (a & uint8_t(0x08)) {
        if (!(ted.bitmapM1ShiftRegister & (1U << offs))) {
          if (!(ted.bitmapM0ShiftRegister & (1U << offs)))      // color 0
            bufp[3] = c0;
          else                                                  // color 1
            bufp[3] = ted.tedRegisters[0x16];
        }
        else {
          if (!(ted.bitmapM0ShiftRegister & (1U << offs)))      // color 2
            bufp[3] = ted.tedRegisters[0x17];
          else                                                  // color 3
            bufp[3] = a & uint8_t(0x77);
        }
      }
      else
        bufp[3] = ((ted.bitmapHShiftRegister & (1U << offs)) ? a : c0);
    }
  }

  REGPARM void TED7360::render_invalid_mode(TED7360& ted,
                                            uint8_t *bufp, int offs)
  {
    (void) ted;
    (void) offs;
    bufp[0] = uint8_t(0);
    bufp[1] = uint8_t(0);
    bufp[2] = uint8_t(0);
    bufp[3] = uint8_t(0);
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
    default:
      bitmapMode = true;
      render_func = &render_invalid_mode;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
    }
  }

}       // namespace Plus4

