
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

static const uint16_t mcmBitmapConversionTable[256] = {
  0x0000, 0x5000, 0xA000, 0xF000, 0x0500, 0x5500, 0xA500, 0xF500,
  0x0A00, 0x5A00, 0xAA00, 0xFA00, 0x0F00, 0x5F00, 0xAF00, 0xFF00,
  0x0050, 0x5050, 0xA050, 0xF050, 0x0550, 0x5550, 0xA550, 0xF550,
  0x0A50, 0x5A50, 0xAA50, 0xFA50, 0x0F50, 0x5F50, 0xAF50, 0xFF50,
  0x00A0, 0x50A0, 0xA0A0, 0xF0A0, 0x05A0, 0x55A0, 0xA5A0, 0xF5A0,
  0x0AA0, 0x5AA0, 0xAAA0, 0xFAA0, 0x0FA0, 0x5FA0, 0xAFA0, 0xFFA0,
  0x00F0, 0x50F0, 0xA0F0, 0xF0F0, 0x05F0, 0x55F0, 0xA5F0, 0xF5F0,
  0x0AF0, 0x5AF0, 0xAAF0, 0xFAF0, 0x0FF0, 0x5FF0, 0xAFF0, 0xFFF0,
  0x0005, 0x5005, 0xA005, 0xF005, 0x0505, 0x5505, 0xA505, 0xF505,
  0x0A05, 0x5A05, 0xAA05, 0xFA05, 0x0F05, 0x5F05, 0xAF05, 0xFF05,
  0x0055, 0x5055, 0xA055, 0xF055, 0x0555, 0x5555, 0xA555, 0xF555,
  0x0A55, 0x5A55, 0xAA55, 0xFA55, 0x0F55, 0x5F55, 0xAF55, 0xFF55,
  0x00A5, 0x50A5, 0xA0A5, 0xF0A5, 0x05A5, 0x55A5, 0xA5A5, 0xF5A5,
  0x0AA5, 0x5AA5, 0xAAA5, 0xFAA5, 0x0FA5, 0x5FA5, 0xAFA5, 0xFFA5,
  0x00F5, 0x50F5, 0xA0F5, 0xF0F5, 0x05F5, 0x55F5, 0xA5F5, 0xF5F5,
  0x0AF5, 0x5AF5, 0xAAF5, 0xFAF5, 0x0FF5, 0x5FF5, 0xAFF5, 0xFFF5,
  0x000A, 0x500A, 0xA00A, 0xF00A, 0x050A, 0x550A, 0xA50A, 0xF50A,
  0x0A0A, 0x5A0A, 0xAA0A, 0xFA0A, 0x0F0A, 0x5F0A, 0xAF0A, 0xFF0A,
  0x005A, 0x505A, 0xA05A, 0xF05A, 0x055A, 0x555A, 0xA55A, 0xF55A,
  0x0A5A, 0x5A5A, 0xAA5A, 0xFA5A, 0x0F5A, 0x5F5A, 0xAF5A, 0xFF5A,
  0x00AA, 0x50AA, 0xA0AA, 0xF0AA, 0x05AA, 0x55AA, 0xA5AA, 0xF5AA,
  0x0AAA, 0x5AAA, 0xAAAA, 0xFAAA, 0x0FAA, 0x5FAA, 0xAFAA, 0xFFAA,
  0x00FA, 0x50FA, 0xA0FA, 0xF0FA, 0x05FA, 0x55FA, 0xA5FA, 0xF5FA,
  0x0AFA, 0x5AFA, 0xAAFA, 0xFAFA, 0x0FFA, 0x5FFA, 0xAFFA, 0xFFFA,
  0x000F, 0x500F, 0xA00F, 0xF00F, 0x050F, 0x550F, 0xA50F, 0xF50F,
  0x0A0F, 0x5A0F, 0xAA0F, 0xFA0F, 0x0F0F, 0x5F0F, 0xAF0F, 0xFF0F,
  0x005F, 0x505F, 0xA05F, 0xF05F, 0x055F, 0x555F, 0xA55F, 0xF55F,
  0x0A5F, 0x5A5F, 0xAA5F, 0xFA5F, 0x0F5F, 0x5F5F, 0xAF5F, 0xFF5F,
  0x00AF, 0x50AF, 0xA0AF, 0xF0AF, 0x05AF, 0x55AF, 0xA5AF, 0xF5AF,
  0x0AAF, 0x5AAF, 0xAAAF, 0xFAAF, 0x0FAF, 0x5FAF, 0xAFAF, 0xFFAF,
  0x00FF, 0x50FF, 0xA0FF, 0xF0FF, 0x05FF, 0x55FF, 0xA5FF, 0xF5FF,
  0x0AFF, 0x5AFF, 0xAAFF, 0xFAFF, 0x0FFF, 0x5FFF, 0xAFFF, 0xFFFF
};

static const uint8_t colorTable_NTSC[256] = {
  0x00, 0x01, 0x0B, 0x05, 0x0E, 0x0F, 0x0D, 0x08,
  0x0B, 0x08, 0x07, 0x04, 0x05, 0x03, 0x0D, 0x07,
  0x10, 0x11, 0x1B, 0x15, 0x1E, 0x1F, 0x1D, 0x18,
  0x1B, 0x18, 0x17, 0x14, 0x15, 0x13, 0x1D, 0x17,
  0x20, 0x21, 0x2B, 0x25, 0x2E, 0x2F, 0x2D, 0x28,
  0x2B, 0x28, 0x27, 0x24, 0x25, 0x23, 0x2D, 0x27,
  0x30, 0x31, 0x3B, 0x35, 0x3E, 0x3F, 0x3D, 0x38,
  0x3B, 0x38, 0x37, 0x34, 0x35, 0x33, 0x3D, 0x37,
  0x40, 0x41, 0x4B, 0x45, 0x4E, 0x4F, 0x4D, 0x48,
  0x4B, 0x48, 0x47, 0x44, 0x45, 0x43, 0x4D, 0x47,
  0x50, 0x51, 0x5B, 0x55, 0x5E, 0x5F, 0x5D, 0x58,
  0x5B, 0x58, 0x57, 0x54, 0x55, 0x53, 0x5D, 0x57,
  0x60, 0x61, 0x6B, 0x65, 0x6E, 0x6F, 0x6D, 0x68,
  0x6B, 0x68, 0x67, 0x64, 0x65, 0x63, 0x6D, 0x67,
  0x70, 0x71, 0x7B, 0x75, 0x7E, 0x7F, 0x7D, 0x78,
  0x7B, 0x78, 0x77, 0x74, 0x75, 0x73, 0x7D, 0x77,
  0x00, 0x01, 0x0B, 0x05, 0x0E, 0x0F, 0x0D, 0x08,
  0x0B, 0x08, 0x07, 0x04, 0x05, 0x03, 0x0D, 0x07,
  0x10, 0x11, 0x1B, 0x15, 0x1E, 0x1F, 0x1D, 0x18,
  0x1B, 0x18, 0x17, 0x14, 0x15, 0x13, 0x1D, 0x17,
  0x20, 0x21, 0x2B, 0x25, 0x2E, 0x2F, 0x2D, 0x28,
  0x2B, 0x28, 0x27, 0x24, 0x25, 0x23, 0x2D, 0x27,
  0x30, 0x31, 0x3B, 0x35, 0x3E, 0x3F, 0x3D, 0x38,
  0x3B, 0x38, 0x37, 0x34, 0x35, 0x33, 0x3D, 0x37,
  0x40, 0x41, 0x4B, 0x45, 0x4E, 0x4F, 0x4D, 0x48,
  0x4B, 0x48, 0x47, 0x44, 0x45, 0x43, 0x4D, 0x47,
  0x50, 0x51, 0x5B, 0x55, 0x5E, 0x5F, 0x5D, 0x58,
  0x5B, 0x58, 0x57, 0x54, 0x55, 0x53, 0x5D, 0x57,
  0x60, 0x61, 0x6B, 0x65, 0x6E, 0x6F, 0x6D, 0x68,
  0x6B, 0x68, 0x67, 0x64, 0x65, 0x63, 0x6D, 0x67,
  0x70, 0x71, 0x7B, 0x75, 0x7E, 0x7F, 0x7D, 0x78,
  0x7B, 0x78, 0x77, 0x74, 0x75, 0x73, 0x7D, 0x77
};

static const uint8_t colorTable_InvPhase[256] = {
  0x00, 0x01, 0x0C, 0x0B, 0x0D, 0x08, 0x0E, 0x0A,
  0x05, 0x0F, 0x07, 0x03, 0x02, 0x04, 0x0E, 0x09,
  0x10, 0x11, 0x1C, 0x1B, 0x1D, 0x18, 0x1E, 0x1A,
  0x15, 0x1F, 0x17, 0x13, 0x12, 0x14, 0x1E, 0x19,
  0x20, 0x21, 0x2C, 0x2B, 0x2D, 0x28, 0x2E, 0x2A,
  0x25, 0x2F, 0x27, 0x23, 0x22, 0x24, 0x2E, 0x29,
  0x30, 0x31, 0x3C, 0x3B, 0x3D, 0x38, 0x3E, 0x3A,
  0x35, 0x3F, 0x37, 0x33, 0x32, 0x34, 0x3E, 0x39,
  0x40, 0x41, 0x4C, 0x4B, 0x4D, 0x48, 0x4E, 0x4A,
  0x45, 0x4F, 0x47, 0x43, 0x42, 0x44, 0x4E, 0x49,
  0x50, 0x51, 0x5C, 0x5B, 0x5D, 0x58, 0x5E, 0x5A,
  0x55, 0x5F, 0x57, 0x53, 0x52, 0x54, 0x5E, 0x59,
  0x60, 0x61, 0x6C, 0x6B, 0x6D, 0x68, 0x6E, 0x6A,
  0x65, 0x6F, 0x67, 0x63, 0x62, 0x64, 0x6E, 0x69,
  0x70, 0x71, 0x7C, 0x7B, 0x7D, 0x78, 0x7E, 0x7A,
  0x75, 0x7F, 0x77, 0x73, 0x72, 0x74, 0x7E, 0x79,
  0x00, 0x01, 0x0C, 0x0B, 0x0D, 0x08, 0x0E, 0x0A,
  0x05, 0x0F, 0x07, 0x03, 0x02, 0x04, 0x0E, 0x09,
  0x10, 0x11, 0x1C, 0x1B, 0x1D, 0x18, 0x1E, 0x1A,
  0x15, 0x1F, 0x17, 0x13, 0x12, 0x14, 0x1E, 0x19,
  0x20, 0x21, 0x2C, 0x2B, 0x2D, 0x28, 0x2E, 0x2A,
  0x25, 0x2F, 0x27, 0x23, 0x22, 0x24, 0x2E, 0x29,
  0x30, 0x31, 0x3C, 0x3B, 0x3D, 0x38, 0x3E, 0x3A,
  0x35, 0x3F, 0x37, 0x33, 0x32, 0x34, 0x3E, 0x39,
  0x40, 0x41, 0x4C, 0x4B, 0x4D, 0x48, 0x4E, 0x4A,
  0x45, 0x4F, 0x47, 0x43, 0x42, 0x44, 0x4E, 0x49,
  0x50, 0x51, 0x5C, 0x5B, 0x5D, 0x58, 0x5E, 0x5A,
  0x55, 0x5F, 0x57, 0x53, 0x52, 0x54, 0x5E, 0x59,
  0x60, 0x61, 0x6C, 0x6B, 0x6D, 0x68, 0x6E, 0x6A,
  0x65, 0x6F, 0x67, 0x63, 0x62, 0x64, 0x6E, 0x69,
  0x70, 0x71, 0x7C, 0x7B, 0x7D, 0x78, 0x7E, 0x7A,
  0x75, 0x7F, 0x77, 0x73, 0x72, 0x74, 0x7E, 0x79
};

static const uint8_t colorTable_NTSC_InvPhase[256] = {
  0x00, 0x01, 0x03, 0x02, 0x06, 0x09, 0x04, 0x0F,
  0x0C, 0x05, 0x0A, 0x0D, 0x08, 0x0B, 0x04, 0x07,
  0x10, 0x11, 0x13, 0x12, 0x16, 0x19, 0x14, 0x1F,
  0x1C, 0x15, 0x1A, 0x1D, 0x18, 0x1B, 0x14, 0x17,
  0x20, 0x21, 0x23, 0x22, 0x26, 0x29, 0x24, 0x2F,
  0x2C, 0x25, 0x2A, 0x2D, 0x28, 0x2B, 0x24, 0x27,
  0x30, 0x31, 0x33, 0x32, 0x36, 0x39, 0x34, 0x3F,
  0x3C, 0x35, 0x3A, 0x3D, 0x38, 0x3B, 0x34, 0x37,
  0x40, 0x41, 0x43, 0x42, 0x46, 0x49, 0x44, 0x4F,
  0x4C, 0x45, 0x4A, 0x4D, 0x48, 0x4B, 0x44, 0x47,
  0x50, 0x51, 0x53, 0x52, 0x56, 0x59, 0x54, 0x5F,
  0x5C, 0x55, 0x5A, 0x5D, 0x58, 0x5B, 0x54, 0x57,
  0x60, 0x61, 0x63, 0x62, 0x66, 0x69, 0x64, 0x6F,
  0x6C, 0x65, 0x6A, 0x6D, 0x68, 0x6B, 0x64, 0x67,
  0x70, 0x71, 0x73, 0x72, 0x76, 0x79, 0x74, 0x7F,
  0x7C, 0x75, 0x7A, 0x7D, 0x78, 0x7B, 0x74, 0x77,
  0x00, 0x01, 0x03, 0x02, 0x06, 0x09, 0x04, 0x0F,
  0x0C, 0x05, 0x0A, 0x0D, 0x08, 0x0B, 0x04, 0x07,
  0x10, 0x11, 0x13, 0x12, 0x16, 0x19, 0x14, 0x1F,
  0x1C, 0x15, 0x1A, 0x1D, 0x18, 0x1B, 0x14, 0x17,
  0x20, 0x21, 0x23, 0x22, 0x26, 0x29, 0x24, 0x2F,
  0x2C, 0x25, 0x2A, 0x2D, 0x28, 0x2B, 0x24, 0x27,
  0x30, 0x31, 0x33, 0x32, 0x36, 0x39, 0x34, 0x3F,
  0x3C, 0x35, 0x3A, 0x3D, 0x38, 0x3B, 0x34, 0x37,
  0x40, 0x41, 0x43, 0x42, 0x46, 0x49, 0x44, 0x4F,
  0x4C, 0x45, 0x4A, 0x4D, 0x48, 0x4B, 0x44, 0x47,
  0x50, 0x51, 0x53, 0x52, 0x56, 0x59, 0x54, 0x5F,
  0x5C, 0x55, 0x5A, 0x5D, 0x58, 0x5B, 0x54, 0x57,
  0x60, 0x61, 0x63, 0x62, 0x66, 0x69, 0x64, 0x6F,
  0x6C, 0x65, 0x6A, 0x6D, 0x68, 0x6B, 0x64, 0x67,
  0x70, 0x71, 0x73, 0x72, 0x76, 0x79, 0x74, 0x7F,
  0x7C, 0x75, 0x7A, 0x7D, 0x78, 0x7B, 0x74, 0x77
};

namespace Plus4 {

  REGPARM void TED7360::render_BMM_hires(TED7360& ted, uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    if (nextCharCnt == 0) {
      uint8_t b = ted.currentBitmap;
      ted.bitmapHShiftRegister = b << 4;
      ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 8;
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
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 6;
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
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 4;
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
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 2;
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
             ted.tedRegisters[0x15] : uint8_t(0xFF));
    c_[3] = (!(ted.tedRegisterWriteMask & 0x00400000U) ?
             ted.tedRegisters[0x16] : uint8_t(0xFF));
    if (nextCharCnt == 0) {
      ted.bitmapHShiftRegister = ted.currentBitmap << 4;
      uint16_t  b = mcmBitmapConversionTable[ted.currentBitmap];
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
        b = mcmBitmapConversionTable[ted.currentBitmap];
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
        b = mcmBitmapConversionTable[ted.currentBitmap];
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
        b = mcmBitmapConversionTable[ted.currentBitmap];
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
                  ted.tedRegisters[0x15] : uint8_t(0xFF));
    if (nextCharCnt == 0) {
      uint8_t b = ted.currentBitmap;
      ted.bitmapHShiftRegister = b << 4;
      ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 8;
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
      switch (nextCharCnt) {
      case 1:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 3;
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 6;
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
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 4;
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
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 2;
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
                  ted.tedRegisters[0x15] : uint8_t(0xFF));
    if (nextCharCnt == 0) {
      uint8_t b = ted.currentBitmap;
      ted.bitmapHShiftRegister = b << 4;
      ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 8;
      uint8_t a = ted.currentAttribute;
      ted.shiftRegisterAttribute = a;
      ted.shiftRegisterCharacter = ted.currentCharacter;
      ted.shiftRegisterCursorFlag = ted.cursorFlag;
      if (ted.shiftRegisterCursorFlag)
        b = b ^ ted.flashState;
      else if (a & uint8_t(0x80))
        b = b & ted.flashState;
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
      switch (nextCharCnt) {
      case 1:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 3;
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 6;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (ted.shiftRegisterCursorFlag)
          b = b ^ ted.flashState;
        else if (a & uint8_t(0x80))
          b = b & ted.flashState;
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
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 4;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (ted.shiftRegisterCursorFlag)
          b = b ^ ted.flashState;
        else if (a & uint8_t(0x80))
          b = b & ted.flashState;
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
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 2;
        a = ted.currentAttribute;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = ted.currentCharacter;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        if (ted.shiftRegisterCursorFlag)
          b = b ^ ted.flashState;
        else if (a & uint8_t(0x80))
          b = b & ted.flashState;
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
      ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 8;
      uint8_t a = ted.currentAttribute;
      uint8_t c = ted.currentCharacter;
      ted.shiftRegisterAttribute = a;
      ted.shiftRegisterCharacter = c;
      ted.shiftRegisterCursorFlag = ted.cursorFlag;
      uint8_t c0 = (c >> 6) + uint8_t(0x15);
      c0 = (!(ted.tedRegisterWriteMask & (1U << c0)) ?
            ted.tedRegisters[c0] : uint8_t(0xFF));
      bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
      c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
      bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
      bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
      bufp[3] = ((b & uint8_t(0x10)) ? a : c0);
    }
    else {
      uint8_t b = ted.bitmapHShiftRegister;
      uint8_t a = ted.shiftRegisterAttribute;
      uint8_t c = ted.shiftRegisterCharacter;
      uint8_t c0 = (c >> 6) + uint8_t(0x15);
      c0 = (!(ted.tedRegisterWriteMask & (1U << c0)) ?
            ted.tedRegisters[c0] : uint8_t(0xFF));
      switch (nextCharCnt) {
      case 1:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 3;
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 6;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
        bufp[1] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x20)) ? a : c0);
        break;
      case 2:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 2;
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 4;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
        bufp[2] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x40)) ? a : c0);
        break;
      case 3:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 1;
        ted.bitmapMShiftRegister = mcmBitmapConversionTable[b] >> 2;
        a = ted.currentAttribute;
        c = ted.currentCharacter;
        ted.shiftRegisterAttribute = a;
        ted.shiftRegisterCharacter = c;
        ted.shiftRegisterCursorFlag = ted.cursorFlag;
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
        bufp[3] = ((b & uint8_t(0x80)) ? a : c0);
        break;
      default:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
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
      uint16_t  b2 = mcmBitmapConversionTable[b];
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
          bufp[0] = uint8_t(0xFF);
        bufp[1] = c_[(b2 >> 2) & 3];
        bufp[2] = c_[(b2 >> 4) & 3];
        bufp[3] = c_[(b2 >> 6) & 3];
      }
      else {
        bufp[0] = ((b & uint8_t(0x80)) ?
                   c_[3] : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                            c_[0] : uint8_t(0xFF)));
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
            bufp[0] = uint8_t(0xFF);
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     c_[3] : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                              c_[0] : uint8_t(0xFF)));
        }
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 3;
        b2 = mcmBitmapConversionTable[b];
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
            bufp[0] = uint8_t(0xFF);
          bufp[1] = c_[(b2 >> 2) & 3];
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     c_[3] : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                              c_[0] : uint8_t(0xFF)));
          bufp[1] = ((b & uint8_t(0x40)) ? c_[3] : c_[0]);
        }
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 2;
        b2 = mcmBitmapConversionTable[b];
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
            bufp[0] = uint8_t(0xFF);
          bufp[1] = c_[(b2 >> 2) & 3];
          bufp[2] = c_[(b2 >> 4) & 3];
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     c_[3] : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                              c_[0] : uint8_t(0xFF)));
          bufp[1] = ((b & uint8_t(0x40)) ? c_[3] : c_[0]);
          bufp[2] = ((b & uint8_t(0x20)) ? c_[3] : c_[0]);
        }
        b = ted.currentBitmap;
        ted.bitmapHShiftRegister = b << 1;
        b2 = mcmBitmapConversionTable[b];
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
            bufp[0] = uint8_t(0xFF);
          bufp[1] = c_[(b2 >> 2) & 3];
          bufp[2] = c_[(b2 >> 4) & 3];
          bufp[3] = c_[(b2 >> 6) & 3];
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     c_[3] : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                              c_[0] : uint8_t(0xFF)));
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
          mcmBitmapConversionTable[b] >> ((4 - nextCharCnt) << 1);
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

  void TED7360::resampleAndDrawLine(bool invertColors)
  {
    if (line_buf_pos >= 451) {
      uint8_t tmpBuf[816];
      int     nPixels = int(((unsigned int) (line_buf_pos - 1) * 8U) / 9U);
      int     cnt = 768;
      int     cnt2 = 0;
      int     readPos = 0;
      int     writePos = 0;
      uint8_t c = 0;
      const uint8_t *colorTable = (uint8_t *) 0;
      if (nPixels >= 416)
        colorTable = (invertColors ?
                      &(colorTable_NTSC_InvPhase[0]) : &(colorTable_NTSC[0]));
      else if (invertColors)
        colorTable = &(colorTable_InvPhase[0]);
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
      if (invertColors) {
        int     cnt = 0;
        for (int i = 0; i < line_buf_pos; i++) {
          if (!cnt) {
            i++;
            cnt = 8;
          }
          cnt--;
          line_buf[i] = colorTable_InvPhase[line_buf[i]];
        }
      }
      drawLine(&(line_buf[0]), 432);
    }
  }

}       // namespace Plus4

