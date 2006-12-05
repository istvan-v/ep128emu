
// plus4 -- portable Commodore PLUS/4 emulator
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
#include "cpu.hpp"
#include "ted.hpp"

namespace Plus4 {

  void TED7360::render_BMM_hires()
  {
    int     x = (int) video_column >> 1;
    uint8_t a = attr_buf[x], c = char_buf[x];
    int     bitmap_addr;
    int     bitmap;
    uint8_t c0, c1;

    bitmap_addr = bitmap_base_addr + (character_position << 3) + character_line;
    bitmap = readVideoMemory((uint16_t) bitmap_addr);
    c0 = (a & (uint8_t) 0x70) + (c & (uint8_t) 0x0F);
    c1 = ((a & (uint8_t) 0x07) << 4) + ((c & (uint8_t) 0xF0) >> 4);
    pixel_buf[8]  = ((bitmap & (uint8_t) 0x80) ? c1 : c0);
    pixel_buf[9]  = ((bitmap & (uint8_t) 0x40) ? c1 : c0);
    pixel_buf[10] = ((bitmap & (uint8_t) 0x20) ? c1 : c0);
    pixel_buf[11] = ((bitmap & (uint8_t) 0x10) ? c1 : c0);
    pixel_buf[12] = ((bitmap & (uint8_t) 0x08) ? c1 : c0);
    pixel_buf[13] = ((bitmap & (uint8_t) 0x04) ? c1 : c0);
    pixel_buf[14] = ((bitmap & (uint8_t) 0x02) ? c1 : c0);
    pixel_buf[15] = ((bitmap & (uint8_t) 0x01) ? c1 : c0);
  }

  void TED7360::render_BMM_multicolor()
  {
    int     x = (int) video_column >> 1;
    uint8_t a = attr_buf[x], c = char_buf[x];
    int     bitmap_addr;
    int     bitmap;
    uint8_t c_[4];

    bitmap_addr = bitmap_base_addr + (character_position << 3) + character_line;
    bitmap = readVideoMemory((uint16_t) bitmap_addr);
    c_[0] = memory_ram[0xFF15];
    c_[1] = ((a & (uint8_t) 0x07) << 4) + ((c & (uint8_t) 0xF0) >> 4);
    c_[2] = (a & (uint8_t) 0x70) + (c & (uint8_t) 0x0F);
    c_[3] = memory_ram[0xFF16];
    pixel_buf[9]  = pixel_buf[8]  = c_[(bitmap & (uint8_t) 0xC0) >> 6];
    pixel_buf[11] = pixel_buf[10] = c_[(bitmap & (uint8_t) 0x30) >> 4];
    pixel_buf[13] = pixel_buf[12] = c_[(bitmap & (uint8_t) 0x0C) >> 2];
    pixel_buf[15] = pixel_buf[14] = c_[bitmap & (uint8_t) 0x03];
  }

  void TED7360::render_char_128()
  {
    int     x = (int) video_column >> 1;
    uint8_t a = attr_buf[x], c = char_buf[x];
    int     bitmap_addr;
    int     bitmap;

    bitmap_addr = charset_base_addr + ((int) (c & (uint8_t) 0x7F) << 3)
                                    + (int) character_line;
    bitmap = readVideoMemory((uint16_t) bitmap_addr);
    if (cursor_position == character_position) {
      if (flash_state)
        bitmap ^= (uint8_t) 0xFF;
    }
    else if (a & (uint8_t) 0x80) {
      if (!flash_state)
        bitmap = (uint8_t) 0x00;
    }
    if (c & (uint8_t) 0x80)
      bitmap ^= (uint8_t) 0xFF;
    a &= (uint8_t) 0x7F;
    c = memory_ram[0xFF15];
    pixel_buf[8]  = ((bitmap & (uint8_t) 0x80) ? a : c);
    pixel_buf[9]  = ((bitmap & (uint8_t) 0x40) ? a : c);
    pixel_buf[10] = ((bitmap & (uint8_t) 0x20) ? a : c);
    pixel_buf[11] = ((bitmap & (uint8_t) 0x10) ? a : c);
    pixel_buf[12] = ((bitmap & (uint8_t) 0x08) ? a : c);
    pixel_buf[13] = ((bitmap & (uint8_t) 0x04) ? a : c);
    pixel_buf[14] = ((bitmap & (uint8_t) 0x02) ? a : c);
    pixel_buf[15] = ((bitmap & (uint8_t) 0x01) ? a : c);
  }

  void TED7360::render_char_256()
  {
    int     x = (int) video_column >> 1;
    uint8_t a = attr_buf[x], c = char_buf[x];
    int     bitmap_addr;
    int     bitmap;

    bitmap_addr = (charset_base_addr & 0xF800) + ((int) c << 3)
                                               + (int) character_line;
    bitmap = readVideoMemory((uint16_t) bitmap_addr);
    if (cursor_position == character_position) {
      if (flash_state)
        bitmap ^= (uint8_t) 0xFF;
    }
    else if (a & (uint8_t) 0x80) {
      if (!flash_state)
        bitmap = (uint8_t) 0x00;
    }
    a &= (uint8_t) 0x7F;
    c = memory_ram[0xFF15];
    pixel_buf[8]  = ((bitmap & (uint8_t) 0x80) ? a : c);
    pixel_buf[9]  = ((bitmap & (uint8_t) 0x40) ? a : c);
    pixel_buf[10] = ((bitmap & (uint8_t) 0x20) ? a : c);
    pixel_buf[11] = ((bitmap & (uint8_t) 0x10) ? a : c);
    pixel_buf[12] = ((bitmap & (uint8_t) 0x08) ? a : c);
    pixel_buf[13] = ((bitmap & (uint8_t) 0x04) ? a : c);
    pixel_buf[14] = ((bitmap & (uint8_t) 0x02) ? a : c);
    pixel_buf[15] = ((bitmap & (uint8_t) 0x01) ? a : c);
  }

  void TED7360::render_char_ECM()
  {
    int     x = (int) video_column >> 1;
    uint8_t a = attr_buf[x], c = char_buf[x];
    int     bitmap_addr;
    int     bitmap;

    bitmap_addr = (charset_base_addr & 0xF800)
                  + ((int) (c & (uint8_t) 0x3F) << 3) + (int) character_line;
    bitmap = readVideoMemory((uint16_t) bitmap_addr);
    a &= (uint8_t) 0x7F;
    c = memory_ram[0xFF15 + (int) ((c & (uint8_t) 0xC0) >> 6)];
    pixel_buf[8]  = ((bitmap & (uint8_t) 0x80) ? a : c);
    pixel_buf[9]  = ((bitmap & (uint8_t) 0x40) ? a : c);
    pixel_buf[10] = ((bitmap & (uint8_t) 0x20) ? a : c);
    pixel_buf[11] = ((bitmap & (uint8_t) 0x10) ? a : c);
    pixel_buf[12] = ((bitmap & (uint8_t) 0x08) ? a : c);
    pixel_buf[13] = ((bitmap & (uint8_t) 0x04) ? a : c);
    pixel_buf[14] = ((bitmap & (uint8_t) 0x02) ? a : c);
    pixel_buf[15] = ((bitmap & (uint8_t) 0x01) ? a : c);
  }

  void TED7360::render_char_MCM_128()
  {
    int     x = (int) video_column >> 1;
    uint8_t a = attr_buf[x], c = char_buf[x];
    int     bitmap_addr;
    int     bitmap;

    bitmap_addr = charset_base_addr + ((int) (c & (uint8_t) 0x7F) << 3)
                                    + (int) character_line;
    bitmap = readVideoMemory((uint16_t) bitmap_addr);
    if (a & (uint8_t) 0x08) {
      uint8_t c_[4];
      c_[0] = memory_ram[0xFF15];
      c_[1] = memory_ram[0xFF16];
      c_[2] = memory_ram[0xFF17];
      c_[3] = a & (uint8_t) 0x77;
      pixel_buf[9]  = pixel_buf[8]  = c_[(bitmap & (uint8_t) 0xC0) >> 6];
      pixel_buf[11] = pixel_buf[10] = c_[(bitmap & (uint8_t) 0x30) >> 4];
      pixel_buf[13] = pixel_buf[12] = c_[(bitmap & (uint8_t) 0x0C) >> 2];
      pixel_buf[15] = pixel_buf[14] = c_[bitmap & (uint8_t) 0x03];
    }
    else {
      a &= (uint8_t) 0x77;
      c = memory_ram[0xFF15];
      pixel_buf[8]  = ((bitmap & (uint8_t) 0x80) ? a : c);
      pixel_buf[9]  = ((bitmap & (uint8_t) 0x40) ? a : c);
      pixel_buf[10] = ((bitmap & (uint8_t) 0x20) ? a : c);
      pixel_buf[11] = ((bitmap & (uint8_t) 0x10) ? a : c);
      pixel_buf[12] = ((bitmap & (uint8_t) 0x08) ? a : c);
      pixel_buf[13] = ((bitmap & (uint8_t) 0x04) ? a : c);
      pixel_buf[14] = ((bitmap & (uint8_t) 0x02) ? a : c);
      pixel_buf[15] = ((bitmap & (uint8_t) 0x01) ? a : c);
    }
  }

  void TED7360::render_char_MCM_256()
  {
    int     x = (int) video_column >> 1;
    uint8_t a = attr_buf[x], c = char_buf[x];
    int     bitmap_addr;
    int     bitmap;

    bitmap_addr = (charset_base_addr & 0xF800) + ((int) c << 3)
                                               + (int) character_line;
    bitmap = readVideoMemory((uint16_t) bitmap_addr);
    if (a & (uint8_t) 0x08) {
      uint8_t c_[4];
      c_[0] = memory_ram[0xFF15];
      c_[1] = memory_ram[0xFF16];
      c_[2] = memory_ram[0xFF17];
      c_[3] = a & (uint8_t) 0x77;
      pixel_buf[9]  = pixel_buf[8]  = c_[(bitmap & (uint8_t) 0xC0) >> 6];
      pixel_buf[11] = pixel_buf[10] = c_[(bitmap & (uint8_t) 0x30) >> 4];
      pixel_buf[13] = pixel_buf[12] = c_[(bitmap & (uint8_t) 0x0C) >> 2];
      pixel_buf[15] = pixel_buf[14] = c_[bitmap & (uint8_t) 0x03];
    }
    else {
      a &= (uint8_t) 0x77;
      c = memory_ram[0xFF15];
      pixel_buf[8]  = ((bitmap & (uint8_t) 0x80) ? a : c);
      pixel_buf[9]  = ((bitmap & (uint8_t) 0x40) ? a : c);
      pixel_buf[10] = ((bitmap & (uint8_t) 0x20) ? a : c);
      pixel_buf[11] = ((bitmap & (uint8_t) 0x10) ? a : c);
      pixel_buf[12] = ((bitmap & (uint8_t) 0x08) ? a : c);
      pixel_buf[13] = ((bitmap & (uint8_t) 0x04) ? a : c);
      pixel_buf[14] = ((bitmap & (uint8_t) 0x02) ? a : c);
      pixel_buf[15] = ((bitmap & (uint8_t) 0x01) ? a : c);
    }
  }

  void TED7360::render_invalid_mode()
  {
    pixel_buf[8]  = (uint8_t) 0;
    pixel_buf[9]  = (uint8_t) 0;
    pixel_buf[10] = (uint8_t) 0;
    pixel_buf[11] = (uint8_t) 0;
    pixel_buf[12] = (uint8_t) 0;
    pixel_buf[13] = (uint8_t) 0;
    pixel_buf[14] = (uint8_t) 0;
    pixel_buf[15] = (uint8_t) 0;
  }

  void TED7360::render_init()
  {
    if ((memory_ram[0xFF06] & (uint8_t) 0x40) &&
        ((memory_ram[0xFF06] & (uint8_t) 0x20) ||
         (memory_ram[0xFF07] & (uint8_t) 0x10))) {
      // ECM + (BMM or MCM): invalid mode
      for (int i = 0; i < 8; i++)
        pixel_buf[i] = (uint8_t) 0;
      return;
    }
    // FIXME: for now, fill with background color in any other mode
    for (int i = 0; i < 8; i++)
      pixel_buf[i] = memory_ram[0xFF15];
  }

}       // namespace Plus4

