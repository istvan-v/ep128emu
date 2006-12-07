
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

#include "ep128emu.hpp"
#include "cpu.hpp"
#include "ted.hpp"

namespace Plus4 {

  void TED7360::write_register_0000(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
  }

  void TED7360::write_register_0001(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    tape_motor_state = ((value & (uint8_t) 0x08) ? false : true);
    tape_write_state = ((value & (uint8_t) 0x02) ? true : false);
  }

  void TED7360::write_register_FD1x(uint16_t addr, uint8_t value)
  {
    (void) addr;
    user_port_state = value;
  }

  void TED7360::write_register_FD3x(uint16_t addr, uint8_t value)
  {
    (void) addr;
    keyboard_row_select_mask = (int) value | 0xFF00;
  }

  void TED7360::write_register_FF00(uint16_t addr, uint8_t value)
  {
    (void) addr;
    timer1_run = false;
    timer1_state = (timer1_state & 0xFF00) | (int) value;
    timer1_reload_value = (timer1_reload_value & 0xFF00) | (int) value;
  }

  void TED7360::write_register_FF01(uint16_t addr, uint8_t value)
  {
    (void) addr;
    timer1_run = true;
    timer1_state = (timer1_state & 0x00FF) | ((int) value << 8);
  }

  void TED7360::write_register_FF02(uint16_t addr, uint8_t value)
  {
    (void) addr;
    timer2_run = false;
    timer2_state = (timer2_state & 0xFF00) | (int) value;
  }

  void TED7360::write_register_FF03(uint16_t addr, uint8_t value)
  {
    (void) addr;
    timer2_run = true;
    timer2_state = (timer2_state & 0x00FF) | ((int) value << 8);
  }

  void TED7360::write_register_FF04(uint16_t addr, uint8_t value)
  {
    (void) addr;
    timer3_run = false;
    timer3_state = (timer3_state & 0xFF00) | (int) value;
  }

  void TED7360::write_register_FF05(uint16_t addr, uint8_t value)
  {
    (void) addr;
    timer3_run = true;
    timer3_state = (timer3_state & 0x00FF) | ((int) value << 8);
  }

  void TED7360::write_register_FF06(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    int mode = (int) (value & (uint8_t) 0x60);
    mode |= (int) (memory_ram[0xFF07] & (uint8_t) 0x90);
    switch (mode) {
      case 0x00:  render_func = &Plus4::TED7360::render_char_128;       break;
      case 0x10:  render_func = &Plus4::TED7360::render_char_MCM_128;   break;
      case 0x20:  render_func = &Plus4::TED7360::render_BMM_hires;      break;
      case 0x30:  render_func = &Plus4::TED7360::render_BMM_multicolor; break;
      case 0x40:  render_func = &Plus4::TED7360::render_char_ECM;       break;
      case 0x80:  render_func = &Plus4::TED7360::render_char_256;       break;
      case 0x90:  render_func = &Plus4::TED7360::render_char_MCM_256;   break;
      case 0xA0:  render_func = &Plus4::TED7360::render_BMM_hires;      break;
      case 0xB0:  render_func = &Plus4::TED7360::render_BMM_multicolor; break;
      case 0xC0:  render_func = &Plus4::TED7360::render_char_ECM;       break;
      default:    render_func = &Plus4::TED7360::render_invalid_mode;
    }
  }

  void TED7360::write_register_FF07(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    horiz_scroll = (int) (value & (uint8_t) 0x07);
    if (value & (uint8_t) 0x08)
      display_mode &= (~8);
    else
      display_mode |= 8;        // 38 column mode
    ted_disabled = ((value & (uint8_t) 0x20) ? true : false);
    int mode = (int) (value & (uint8_t) 0x90);
    mode |= (int) (memory_ram[0xFF06] & (uint8_t) 0x60);
    switch (mode) {
      case 0x00:  render_func = &Plus4::TED7360::render_char_128;       break;
      case 0x10:  render_func = &Plus4::TED7360::render_char_MCM_128;   break;
      case 0x20:  render_func = &Plus4::TED7360::render_BMM_hires;      break;
      case 0x30:  render_func = &Plus4::TED7360::render_BMM_multicolor; break;
      case 0x40:  render_func = &Plus4::TED7360::render_char_ECM;       break;
      case 0x80:  render_func = &Plus4::TED7360::render_char_256;       break;
      case 0x90:  render_func = &Plus4::TED7360::render_char_MCM_256;   break;
      case 0xA0:  render_func = &Plus4::TED7360::render_BMM_hires;      break;
      case 0xB0:  render_func = &Plus4::TED7360::render_BMM_multicolor; break;
      case 0xC0:  render_func = &Plus4::TED7360::render_char_ECM;       break;
      default:    render_func = &Plus4::TED7360::render_invalid_mode;
    }
  }

  void TED7360::write_register_FF08(uint16_t addr, uint8_t value)
  {
    int     mask = keyboard_row_select_mask & (((int) value << 8) | 0xFF);
    uint8_t key_state = (uint8_t) 0xFF;
    for (int i = 0; i < 11; i++, mask >>= 1) {
      if (!(mask & 1))
        key_state &= keyboard_matrix[i];
    }
    memory_ram[addr] = key_state;
    keyboard_row_select_mask = 0xFFFF;
  }

  void TED7360::write_register_FF09(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] &= (value ^ (uint8_t) 0xFF);
  }

  void TED7360::write_register_FF0C(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    cursor_position &= 0x00FF;
    cursor_position |= ((int) (value & (uint8_t) 0x03) << 8);
  }

  void TED7360::write_register_FF0D(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    cursor_position &= 0x0300;
    cursor_position |= (int) value;
  }

  void TED7360::write_register_FF0E(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    sound_channel_1_reload &= 0x0300;
    sound_channel_1_reload |= (int) value;
  }

  void TED7360::write_register_FF0F(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    sound_channel_2_reload &= 0x0300;
    sound_channel_2_reload |= (int) value;
  }

  void TED7360::write_register_FF10(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    sound_channel_2_reload &= 0x00FF;
    sound_channel_2_reload |= ((int) (value & (uint8_t) 0x03) << 8);
  }

  void TED7360::write_register_FF12(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    sound_channel_1_reload &= 0x00FF;
    sound_channel_1_reload |= ((int) (value & (uint8_t) 0x03) << 8);
    rom_enabled_for_video = ((value & (uint8_t) 0x04) ? true : false);
    bitmap_base_addr = (int) (value & (uint8_t) 0x38) << 10;
  }

  void TED7360::write_register_FF13(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    if (value & (uint8_t) 0x02)
      display_mode |= 16;           // single clock mode
    else
      display_mode &= (~16);
    charset_base_addr = (int) (value & (uint8_t) 0xFC) << 8;
  }

  void TED7360::write_register_FF14(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    attr_base_addr = (int) (value & (uint8_t) 0xF8) << 8;
  }

  void TED7360::write_register_FF1A(uint16_t addr, uint8_t value)
  {
    (void) addr;
    character_position_reload &= 0x00FF;
    character_position_reload |= ((int) (value & (uint8_t) 0x03) << 8);
  }

  void TED7360::write_register_FF1B(uint16_t addr, uint8_t value)
  {
    (void) addr;
    character_position_reload &= 0x0300;
    character_position_reload |= (int) value;
  }

  void TED7360::write_register_FF1C(uint16_t addr, uint8_t value)
  {
    (void) addr;
    int lineNum = (video_line & 0x00FF) | ((int) (value & (uint8_t) 0x01) << 8);
    set_video_line(lineNum, false);
  }

  void TED7360::write_register_FF1D(uint16_t addr, uint8_t value)
  {
    (void) addr;
    int lineNum = (video_line & 0x0100) | (int) value;
    set_video_line(lineNum, false);
  }

  void TED7360::write_register_FF1E(uint16_t addr, uint8_t value)
  {
    (void) addr;
    // FIXME: writing the video column register is not supported
    (void) value;
  }

  void TED7360::write_register_FF1F(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
    character_line = (int) (value & (uint8_t) 0x07);
  }

}       // namespace Plus4

