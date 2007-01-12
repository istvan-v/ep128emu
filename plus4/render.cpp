
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

namespace Plus4 {

  void TED7360::render_BMM_hires(void *ted_)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(ted_));
    uint8_t   a = ted.currentAttribute;
    uint8_t   c = ted.currentCharacter;
    uint8_t   bitmap = ted.currentBitmap;
    uint8_t   c0 = (a & uint8_t(0x70)) + (c & uint8_t(0x0F));
    uint8_t   c1 = ((a & uint8_t(0x07)) << 4) + ((c & uint8_t(0xF0)) >> 4);
    uint8_t   *buf = &(ted.pixel_buf[ted.pixelBufWritePos]);
    buf[0] = ((bitmap & uint8_t(0x80)) ? c1 : c0);
    buf[1] = ((bitmap & uint8_t(0x40)) ? c1 : c0);
    buf[2] = ((bitmap & uint8_t(0x20)) ? c1 : c0);
    buf[3] = ((bitmap & uint8_t(0x10)) ? c1 : c0);
    buf[4] = ((bitmap & uint8_t(0x08)) ? c1 : c0);
    buf[5] = ((bitmap & uint8_t(0x04)) ? c1 : c0);
    buf[6] = ((bitmap & uint8_t(0x02)) ? c1 : c0);
    buf[7] = ((bitmap & uint8_t(0x01)) ? c1 : c0);
  }

  void TED7360::render_BMM_multicolor(void *ted_)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(ted_));
    uint8_t   a = ted.currentAttribute;
    uint8_t   c = ted.currentCharacter;
    uint8_t   bitmap = ted.currentBitmap;
    uint8_t   c_[4];
    c_[0] = uint8_t(0x80);
    c_[1] = ((a & uint8_t(0x07)) << 4) + ((c & uint8_t(0xF0)) >> 4);
    c_[2] = (a & uint8_t(0x70)) + (c & uint8_t(0x0F));
    c_[3] = uint8_t(0x81);
    uint8_t   *buf = &(ted.pixel_buf[ted.pixelBufWritePos]);
    buf[1] = buf[0] = c_[(bitmap & uint8_t(0xC0)) >> 6];
    buf[3] = buf[2] = c_[(bitmap & uint8_t(0x30)) >> 4];
    buf[5] = buf[4] = c_[(bitmap & uint8_t(0x0C)) >> 2];
    buf[7] = buf[6] = c_[bitmap & uint8_t(0x03)];
  }

  void TED7360::render_char_128(void *ted_)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(ted_));
    uint8_t   a = ted.currentAttribute;
    uint8_t   c = ted.currentCharacter;
    uint8_t   bitmap = ted.currentBitmap;
    if (ted.cursor_position == ted.character_position) {
      if (ted.flash_state)
        bitmap ^= uint8_t(0xFF);
    }
    else if (a & uint8_t(0x80)) {
      if (!ted.flash_state)
        bitmap = uint8_t(0x00);
    }
    if (c & uint8_t(0x80))
      bitmap ^= uint8_t(0xFF);
    uint8_t   c0 = uint8_t(0x80);
    uint8_t   c1 = a & uint8_t(0x7F);
    uint8_t   *buf = &(ted.pixel_buf[ted.pixelBufWritePos]);
    buf[0] = ((bitmap & uint8_t(0x80)) ? c1 : c0);
    buf[1] = ((bitmap & uint8_t(0x40)) ? c1 : c0);
    buf[2] = ((bitmap & uint8_t(0x20)) ? c1 : c0);
    buf[3] = ((bitmap & uint8_t(0x10)) ? c1 : c0);
    buf[4] = ((bitmap & uint8_t(0x08)) ? c1 : c0);
    buf[5] = ((bitmap & uint8_t(0x04)) ? c1 : c0);
    buf[6] = ((bitmap & uint8_t(0x02)) ? c1 : c0);
    buf[7] = ((bitmap & uint8_t(0x01)) ? c1 : c0);
  }

  void TED7360::render_char_256(void *ted_)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(ted_));
    uint8_t   a = ted.currentAttribute;
    uint8_t   bitmap = ted.currentBitmap;
    if (ted.cursor_position == ted.character_position) {
      if (ted.flash_state)
        bitmap ^= uint8_t(0xFF);
    }
    else if (a & uint8_t(0x80)) {
      if (!ted.flash_state)
        bitmap = uint8_t(0x00);
    }
    uint8_t   c0 = uint8_t(0x80);
    uint8_t   c1 = a & uint8_t(0x7F);
    uint8_t   *buf = &(ted.pixel_buf[ted.pixelBufWritePos]);
    buf[0] = ((bitmap & uint8_t(0x80)) ? c1 : c0);
    buf[1] = ((bitmap & uint8_t(0x40)) ? c1 : c0);
    buf[2] = ((bitmap & uint8_t(0x20)) ? c1 : c0);
    buf[3] = ((bitmap & uint8_t(0x10)) ? c1 : c0);
    buf[4] = ((bitmap & uint8_t(0x08)) ? c1 : c0);
    buf[5] = ((bitmap & uint8_t(0x04)) ? c1 : c0);
    buf[6] = ((bitmap & uint8_t(0x02)) ? c1 : c0);
    buf[7] = ((bitmap & uint8_t(0x01)) ? c1 : c0);
  }

  void TED7360::render_char_ECM(void *ted_)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(ted_));
    uint8_t   a = ted.currentAttribute;
    uint8_t   c = ted.currentCharacter;
    uint8_t   bitmap = ted.currentBitmap;
    uint8_t   c0 = uint8_t(0x80) | ((c & uint8_t(0xC0)) >> 6);
    uint8_t   c1 = a & uint8_t(0x7F);
    uint8_t   *buf = &(ted.pixel_buf[ted.pixelBufWritePos]);
    buf[0] = ((bitmap & uint8_t(0x80)) ? c1 : c0);
    buf[1] = ((bitmap & uint8_t(0x40)) ? c1 : c0);
    buf[2] = ((bitmap & uint8_t(0x20)) ? c1 : c0);
    buf[3] = ((bitmap & uint8_t(0x10)) ? c1 : c0);
    buf[4] = ((bitmap & uint8_t(0x08)) ? c1 : c0);
    buf[5] = ((bitmap & uint8_t(0x04)) ? c1 : c0);
    buf[6] = ((bitmap & uint8_t(0x02)) ? c1 : c0);
    buf[7] = ((bitmap & uint8_t(0x01)) ? c1 : c0);
  }

  void TED7360::render_char_MCM_128(void *ted_)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(ted_));
    uint8_t   a = ted.currentAttribute;
    uint8_t   bitmap = ted.currentBitmap;
    uint8_t   *buf = &(ted.pixel_buf[ted.pixelBufWritePos]);
    if (a & uint8_t(0x08)) {
      uint8_t c_[4];
      c_[0] = uint8_t(0x80);
      c_[1] = uint8_t(0x81);
      c_[2] = uint8_t(0x82);
      c_[3] = a & uint8_t(0x77);
      buf[1] = buf[0] = c_[(bitmap & uint8_t(0xC0)) >> 6];
      buf[3] = buf[2] = c_[(bitmap & uint8_t(0x30)) >> 4];
      buf[5] = buf[4] = c_[(bitmap & uint8_t(0x0C)) >> 2];
      buf[7] = buf[6] = c_[bitmap & uint8_t(0x03)];
    }
    else {
      uint8_t c0 = uint8_t(0x80);
      uint8_t c1 = a & uint8_t(0x77);
      buf[0] = ((bitmap & uint8_t(0x80)) ? c1 : c0);
      buf[1] = ((bitmap & uint8_t(0x40)) ? c1 : c0);
      buf[2] = ((bitmap & uint8_t(0x20)) ? c1 : c0);
      buf[3] = ((bitmap & uint8_t(0x10)) ? c1 : c0);
      buf[4] = ((bitmap & uint8_t(0x08)) ? c1 : c0);
      buf[5] = ((bitmap & uint8_t(0x04)) ? c1 : c0);
      buf[6] = ((bitmap & uint8_t(0x02)) ? c1 : c0);
      buf[7] = ((bitmap & uint8_t(0x01)) ? c1 : c0);
    }
  }

  void TED7360::render_char_MCM_256(void *ted_)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(ted_));
    uint8_t   a = ted.currentAttribute;
    uint8_t   bitmap = ted.currentBitmap;
    uint8_t   *buf = &(ted.pixel_buf[ted.pixelBufWritePos]);
    if (a & uint8_t(0x08)) {
      uint8_t c_[4];
      c_[0] = uint8_t(0x80);
      c_[1] = uint8_t(0x81);
      c_[2] = uint8_t(0x82);
      c_[3] = a & uint8_t(0x77);
      buf[1] = buf[0] = c_[(bitmap & uint8_t(0xC0)) >> 6];
      buf[3] = buf[2] = c_[(bitmap & uint8_t(0x30)) >> 4];
      buf[5] = buf[4] = c_[(bitmap & uint8_t(0x0C)) >> 2];
      buf[7] = buf[6] = c_[bitmap & uint8_t(0x03)];
    }
    else {
      uint8_t c0 = uint8_t(0x80);
      uint8_t c1 = a & uint8_t(0x77);
      buf[0] = ((bitmap & uint8_t(0x80)) ? c1 : c0);
      buf[1] = ((bitmap & uint8_t(0x40)) ? c1 : c0);
      buf[2] = ((bitmap & uint8_t(0x20)) ? c1 : c0);
      buf[3] = ((bitmap & uint8_t(0x10)) ? c1 : c0);
      buf[4] = ((bitmap & uint8_t(0x08)) ? c1 : c0);
      buf[5] = ((bitmap & uint8_t(0x04)) ? c1 : c0);
      buf[6] = ((bitmap & uint8_t(0x02)) ? c1 : c0);
      buf[7] = ((bitmap & uint8_t(0x01)) ? c1 : c0);
    }
  }

  void TED7360::render_invalid_mode(void *ted_)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(ted_));
    uint8_t   *buf = &(ted.pixel_buf[ted.pixelBufWritePos]);
    buf[0] = uint8_t(0);
    buf[1] = uint8_t(0);
    buf[2] = uint8_t(0);
    buf[3] = uint8_t(0);
    buf[4] = uint8_t(0);
    buf[5] = uint8_t(0);
    buf[6] = uint8_t(0);
    buf[7] = uint8_t(0);
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
      render_func = &render_char_MCM_128;
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
      render_func = &render_char_MCM_256;
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

