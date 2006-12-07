
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

  TED7360::TED7360() : M7501()
  {
    // clear memory
    for (int i = 0; i < 0x10000; i++) {
      uint8_t   tmp = ((i & 1) ? (uint8_t) 0xFF : (uint8_t) 0x00);
      tmp = ((i & 0x40) ? (tmp & (uint8_t) 0xF7) : (tmp | (uint8_t) 0x08));
      memory_ram[i] = ((i & 0xFF) ? tmp : (uint8_t) 0xFF);
    }
    for (int j = 0; j < 4; j++) {
      for (int i = 0; i < 0x8000; i++) {
        memory_rom[j][i] = (uint8_t) 0xFF;
      }
    }
    for (int i = 0; i < 0x10000; i++) {
      // memory map for reading (ROM enabled)
      read_memory_rom[i] = &Plus4::TED7360::read_ram_;
      // memory map for reading (ROM disabled)
      read_memory_ram[i] = &Plus4::TED7360::read_ram_;
      // memory map for writing
      write_memory[i] = &Plus4::TED7360::write_ram_;
    }
    // ROM read
    for (int i = 0x8000; i < 0xC000; i++)
      read_memory_rom[i] = &Plus4::TED7360::read_rom_low;
    for (int i = 0xC000; i < 0x10000; i++)
      read_memory_rom[i] = &Plus4::TED7360::read_rom_high;
    for (int i = 0xFC00; i < 0xFD00; i++)
      read_memory_rom[i] = &Plus4::TED7360::read_rom_FCxx;
    // TED register read
    read_memory_rom[0] = &Plus4::TED7360::read_register_0000;
    read_memory_rom[1] = &Plus4::TED7360::read_register_0001;
    for (int i = 0xFD00; i < 0xFF00; i++)
      read_memory_rom[i] = &Plus4::TED7360::read_register_unimplemented;
    for (int i = 0xFD10; i < 0xFD20; i++)
      read_memory_rom[i] = &Plus4::TED7360::read_register_FD10;
    for (int i = 0xFD30; i < 0xFD40; i++)
      read_memory_rom[i] = &Plus4::TED7360::read_register_FD30;
    for (int i = 0xFF00; i < 0xFF20; i++)
      read_memory_rom[i] = &Plus4::TED7360::read_ram_;
    read_memory_rom[0xFF00] = &Plus4::TED7360::read_register_FF00;
    read_memory_rom[0xFF01] = &Plus4::TED7360::read_register_FF01;
    read_memory_rom[0xFF02] = &Plus4::TED7360::read_register_FF02;
    read_memory_rom[0xFF03] = &Plus4::TED7360::read_register_FF03;
    read_memory_rom[0xFF04] = &Plus4::TED7360::read_register_FF04;
    read_memory_rom[0xFF05] = &Plus4::TED7360::read_register_FF05;
    read_memory_rom[0xFF06] = &Plus4::TED7360::read_register_FF06;
    read_memory_rom[0xFF09] = &Plus4::TED7360::read_register_FF09;
    read_memory_rom[0xFF0A] = &Plus4::TED7360::read_register_FF0A;
    read_memory_rom[0xFF0C] = &Plus4::TED7360::read_register_FF0C;
    read_memory_rom[0xFF10] = &Plus4::TED7360::read_register_FF10;
    read_memory_rom[0xFF12] = &Plus4::TED7360::read_register_FF12;
    read_memory_rom[0xFF13] = &Plus4::TED7360::read_register_FF13;
    read_memory_rom[0xFF14] = &Plus4::TED7360::read_register_FF14;
    read_memory_rom[0xFF15] = &Plus4::TED7360::read_register_FF15_to_FF19;
    read_memory_rom[0xFF16] = &Plus4::TED7360::read_register_FF15_to_FF19;
    read_memory_rom[0xFF17] = &Plus4::TED7360::read_register_FF15_to_FF19;
    read_memory_rom[0xFF18] = &Plus4::TED7360::read_register_FF15_to_FF19;
    read_memory_rom[0xFF19] = &Plus4::TED7360::read_register_FF15_to_FF19;
    read_memory_rom[0xFF1A] = &Plus4::TED7360::read_register_FF1A;
    read_memory_rom[0xFF1B] = &Plus4::TED7360::read_register_FF1B;
    read_memory_rom[0xFF1C] = &Plus4::TED7360::read_register_FF1C;
    read_memory_rom[0xFF1D] = &Plus4::TED7360::read_register_FF1D;
    read_memory_rom[0xFF1E] = &Plus4::TED7360::read_register_FF1E;
    read_memory_rom[0xFF1F] = &Plus4::TED7360::read_register_FF1F;
    read_memory_rom[0xFF3E] = &Plus4::TED7360::read_register_FF3E;
    read_memory_rom[0xFF3F] = &Plus4::TED7360::read_register_FF3F;
    // TED register write
    write_memory[0x0000] = &Plus4::TED7360::write_register_0000;
    write_memory[0x0000] = &Plus4::TED7360::write_register_0001;
    for (int i = 0xFD10; i < 0xFD20; i++)
      write_memory[i] = &Plus4::TED7360::write_register_FD1x;
    for (int i = 0xFD30; i < 0xFD40; i++)
      write_memory[i] = &Plus4::TED7360::write_register_FD3x;
    for (int i = 0xFDD0; i < 0xFDE0; i++)
      write_memory[i] = &Plus4::TED7360::write_register_FDDx;
    write_memory[0xFF00] = &Plus4::TED7360::write_register_FF00;
    write_memory[0xFF01] = &Plus4::TED7360::write_register_FF01;
    write_memory[0xFF02] = &Plus4::TED7360::write_register_FF02;
    write_memory[0xFF03] = &Plus4::TED7360::write_register_FF03;
    write_memory[0xFF04] = &Plus4::TED7360::write_register_FF04;
    write_memory[0xFF05] = &Plus4::TED7360::write_register_FF05;
    write_memory[0xFF06] = &Plus4::TED7360::write_register_FF06;
    write_memory[0xFF07] = &Plus4::TED7360::write_register_FF07;
    write_memory[0xFF08] = &Plus4::TED7360::write_register_FF08;
    write_memory[0xFF09] = &Plus4::TED7360::write_register_FF09;
    write_memory[0xFF0C] = &Plus4::TED7360::write_register_FF0C;
    write_memory[0xFF0D] = &Plus4::TED7360::write_register_FF0D;
    write_memory[0xFF0E] = &Plus4::TED7360::write_register_FF0E;
    write_memory[0xFF0F] = &Plus4::TED7360::write_register_FF0F;
    write_memory[0xFF10] = &Plus4::TED7360::write_register_FF10;
    write_memory[0xFF12] = &Plus4::TED7360::write_register_FF12;
    write_memory[0xFF13] = &Plus4::TED7360::write_register_FF13;
    write_memory[0xFF14] = &Plus4::TED7360::write_register_FF14;
    write_memory[0xFF1A] = &Plus4::TED7360::write_register_FF1A;
    write_memory[0xFF1B] = &Plus4::TED7360::write_register_FF1B;
    write_memory[0xFF1C] = &Plus4::TED7360::write_register_FF1C;
    write_memory[0xFF1D] = &Plus4::TED7360::write_register_FF1D;
    write_memory[0xFF1E] = &Plus4::TED7360::write_register_FF1E;
    write_memory[0xFF1F] = &Plus4::TED7360::write_register_FF1F;
    write_memory[0xFF3E] = &Plus4::TED7360::write_register_FF3E;
    write_memory[0xFF3F] = &Plus4::TED7360::write_register_FF3F;
    // TED registers are always available regardless of ROM paging
    for (int i = 0x0000; i < 0x0002; i++)
      read_memory_ram[i] = read_memory_rom[i];
    for (int i = 0xFD00; i < 0xFF20; i++)
      read_memory_ram[i] = read_memory_rom[i];
    for (int i = 0xFF3E; i < 0xFF40; i++)
      read_memory_ram[i] = read_memory_rom[i];
    // initialize memory paging
    rom_enabled = true;
    rom_enabled_for_video = true;
    rom_bank_low = (unsigned char) 0;
    rom_bank_high = (unsigned char) 0;
    // ------------ set up display method tables ------------
    render_func = &Plus4::TED7360::render_invalid_mode;
    for (int i = 0; i < 114; i += 2) {
      displayFuncType func;
      bool            DRAM_refresh;
      DRAM_refresh = ((i >= 76 && i < 86) ? true : false);
      // vertical blanking
      func = &Plus4::TED7360::run_blank_CPU;
      if (DRAM_refresh)
        func = &Plus4::TED7360::run_blank_no_CPU;
      display_func_table[0][i]      = func;
      display_func_table[0][i + 1]  = &Plus4::TED7360::run_blank_CPU;
      // border areas (no data fetch)
      func = &Plus4::TED7360::run_border_CPU;
      if (DRAM_refresh)
        func = &Plus4::TED7360::run_border_no_CPU;
      display_func_table[1][i]      = func;
      display_func_table[1][i + 1]  = &Plus4::TED7360::run_border_CPU;
      // border, but render display
      if (i >= 102)
        func = &Plus4::TED7360::run_border_no_CPU;
      if (i < 80)
        func = &Plus4::TED7360::run_border_render;
      if (i == 112)
        func = &Plus4::TED7360::run_border_render_init;
      display_func_table[2][i]      = func;
      display_func_table[2][i + 1]  = &Plus4::TED7360::run_border_CPU;
      // border, DMA (character line 0)
      display_func_table[3][i] = func;
      func = &Plus4::TED7360::run_border_CPU;
      if (i < 76 || i >= 110)
        func = &Plus4::TED7360::run_border_DMA_0;
      if (i == 108)
        func = &Plus4::TED7360::run_border_no_CPU;
      display_func_table[3][i + 1]  = func;
    }
    // ... horizontal blanking
    for (int j = 1; j < 4; j++) {
      for (int i = 88; i < 106; i++) {
        if (display_func_table[j][i] == &Plus4::TED7360::run_border_no_CPU)
          display_func_table[j][i] = &Plus4::TED7360::run_blank_no_CPU;
        else if (display_func_table[j][i] == &Plus4::TED7360::run_border_CPU)
          display_func_table[j][i] = &Plus4::TED7360::run_blank_CPU;
      }
    }
    // ... border, DMA (character line 7)
    for (int i = 0; i < 114; i++) {
      if (display_func_table[3][i] != &Plus4::TED7360::run_border_DMA_0)
        display_func_table[4][i] = display_func_table[3][i];
      else
        display_func_table[4][i] = &Plus4::TED7360::run_border_DMA_7;
    }
    // ... 38 column mode (for the above modes, same as 40 column mode)
    for (int j = 0; j < 5; j++) {
      for (int i = 0; i < 114; i++)
        display_func_table[j + 8][i] = display_func_table[j][i];
    }
    // ... display area
    for (int j = 2; j < 5; j++) {
      for (int i = 0; i < 114; i++) {
        if (i >= 80) {
          // border
          display_func_table[j + 3][i] =  display_func_table[j][i];
          display_func_table[j + 11][i] = display_func_table[j][i];
          continue;
        }
        if (display_func_table[j][i] == &Plus4::TED7360::run_border_CPU)
          display_func_table[j + 3][i] =  &Plus4::TED7360::run_display_CPU;
        else if (display_func_table[j][i] == &Plus4::TED7360::run_border_no_CPU)
          display_func_table[j + 3][i] =  &Plus4::TED7360::run_display_no_CPU;
        else if (display_func_table[j][i] == &Plus4::TED7360::run_border_DMA_0)
          display_func_table[j + 3][i] =  &Plus4::TED7360::run_display_DMA_0;
        else if (display_func_table[j][i] == &Plus4::TED7360::run_border_DMA_7)
          display_func_table[j + 3][i] =  &Plus4::TED7360::run_display_DMA_7;
        else if (display_func_table[j][i] == &Plus4::TED7360::run_border_render)
          display_func_table[j + 3][i] =  &Plus4::TED7360::run_display_render;
        // 38 column mode
        if (i >= 2 && i < 78)
          display_func_table[j + 11][i] = display_func_table[j + 3][i];
        else
          display_func_table[j + 11][i] = display_func_table[j][i];
      }
    }
    // ... single clock mode
    for (int j = 0; j < 16; j++) {
      for (int i = 0; i < 114; i++) {
        if (i & 1)
          display_func_table[j + 16][i] = display_func_table[j][i];
        else if (display_func_table[j][i] == &Plus4::TED7360::run_blank_CPU)
          display_func_table[j + 16][i] = &Plus4::TED7360::run_blank_no_CPU;
        else if (display_func_table[j][i] == &Plus4::TED7360::run_border_CPU)
          display_func_table[j + 16][i] = &Plus4::TED7360::run_border_no_CPU;
        else if (display_func_table[j][i] == &Plus4::TED7360::run_display_CPU)
          display_func_table[j + 16][i] = &Plus4::TED7360::run_display_no_CPU;
        else
          display_func_table[j + 16][i] = display_func_table[j][i];
      }
    }
    // ------------ done setting up display method tables ------------
    // clear memory used by TED registers
    memory_ram[0x0000] = (uint8_t) 0x0F;
    memory_ram[0x0001] = (uint8_t) 0xC8;
    for (int i = 0xFD00; i < 0xFF00; i++)
      memory_ram[i] = (uint8_t) 0xFF;
    for (int i = 0xFF00; i < 0xFF20; i++)
      memory_ram[i] = (uint8_t) 0;
    memory_ram[0xFF08] = (uint8_t) 0xFF;
    memory_ram[0xFF0C] = (uint8_t) 0xFF;
    memory_ram[0xFF0D] = (uint8_t) 0xFF;
    memory_ram[0xFF13] = (uint8_t) 0x01;
    memory_ram[0xFF1D] = (uint8_t) 224;
    memory_ram[0xFF1E] = (uint8_t) 192;
    // set internal TED registers
    cycle_count = 0UL;
    cpu_clock_multiplier = 1;
    display_mode = 0;
    video_column = 96;
    video_interrupt_shift_register = 0;
    video_line = 224;
    character_line = 0;
    character_position = 0;
    character_position_reload = 0;
    attr_base_addr = 0;
    bitmap_base_addr = 0;
    charset_base_addr = 0;
    horiz_scroll = 0;
    cursor_position = 0x03FF;
    ted_disabled = false;
    flash_state = false;
    display_enabled = false;
    render_enabled = false;
    DMA_enabled = false;
    vertical_blanking = false;
    timer1_run = true;                          // timers
    timer2_run = true;
    timer3_run = true;
    timer1_state = 0;
    timer1_reload_value = 0;
    timer2_state = 0;
    timer3_state = 0;
    sound_channel_1_cnt = 0;                    // sound generators
    sound_channel_1_reload = 0;
    sound_channel_2_cnt = 0;
    sound_channel_2_reload = 0;
    sound_channel_1_state = (uint8_t) 1;
    sound_channel_2_state = (uint8_t) 1;
    sound_channel_2_noise_state = (uint8_t) 0xFF;
    sound_channel_2_noise_output = (uint8_t) 1;
    for (int i = 0; i < 40; i++) {              // video buffers
      attr_buf[i] = (uint8_t) 0;
      char_buf_tmp[i] = (uint8_t) 0;
      char_buf[i] = (uint8_t) 0;
    }
    for (int i = 0; i < 16; i++)
      pixel_buf[i] = (uint8_t) 0;
    for (int i = 0; i < 456; i++)
      line_buf[i] = (uint8_t) 0;
    keyboard_row_select_mask = 0xFFFF;          // keyboard matrix
    for (int i = 0; i < 16; i++)
      keyboard_matrix[i] = (uint8_t) 0xFF;
    user_port_state = (uint8_t) 0xFF;           // external ports
    tape_motor_state = false;
    tape_read_state = false;
    tape_write_state = false;
    tape_button_state = false;
    // set start address for CPU
    reg_PC = (uint16_t) memory_rom[0][0xFFFC];
    reg_PC += ((uint16_t) memory_rom[0][0xFFFD] << 8);
  }

  void TED7360::reset(bool cold_reset)
  {
    if (cold_reset) {
      // reset TED registers
      writeMemory((uint16_t) 0x0000, (uint8_t) 0x0F);
      writeMemory((uint16_t) 0x0001, (uint8_t) 0xC8);
      writeMemory((uint16_t) 0xFDD0, (uint8_t) 0);
      for (int i = 0xFD00; i < 0xFF00; i++)
        memory_ram[i] = (uint8_t) 0xFF;
      for (int i = 0xFF00; i < 0xFF20; i++)
        writeMemory((uint16_t) i, (uint8_t) 0);
      writeMemory((uint16_t) 0xFF08, (uint8_t) 0xFF);
      writeMemory((uint16_t) 0xFF0C, (uint8_t) 0xFF);
      writeMemory((uint16_t) 0xFF0D, (uint8_t) 0xFF);
      writeMemory((uint16_t) 0xFF13, (uint8_t) 0x01);
      writeMemory((uint16_t) 0xFF1D, (uint8_t) 224);
      writeMemory((uint16_t) 0xFF3E, (uint8_t) 0);
      video_interrupt_shift_register = 0;
    }
    // reset CPU
    M7501::reset(cold_reset);
  }

  TED7360::~TED7360()
  {
  }

}       // namespace Plus4

