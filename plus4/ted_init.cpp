
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
    setMemoryCallbackUserData(this);
    for (int i = 0; i < 0x10000; i++) {
      setMemoryReadCallback(uint16_t(i), &read_ram_);
      setMemoryWriteCallback(uint16_t(i), &write_ram_);
    }
    for (int i = 0x8000; i < 0xC000; i++)
      setMemoryReadCallback(uint16_t(i), &read_memory_8000);
    for (int i = 0xC000; i < 0xFC00; i++)
      setMemoryReadCallback(uint16_t(i), &read_memory_C000);
    for (int i = 0xFC00; i < 0xFD00; i++)
      setMemoryReadCallback(uint16_t(i), &read_memory_FC00);
    for (int i = 0xFF40; i < 0x10000; i++)
      setMemoryReadCallback(uint16_t(i), &read_memory_C000);
    // TED register read
    setMemoryReadCallback(0x0000, &read_register_0000);
    setMemoryReadCallback(0x0001, &read_register_0001);
    for (int i = 0xFD00; i < 0xFF00; i++)
      setMemoryReadCallback(uint16_t(i), &read_register_unimplemented);
    for (int i = 0xFD10; i < 0xFD20; i++)
      setMemoryReadCallback(uint16_t(i), &read_register_FD10);
    for (int i = 0xFD30; i < 0xFD40; i++)
      setMemoryReadCallback(uint16_t(i), &read_register_FD30);
    for (int i = 0xFF00; i < 0xFF20; i++)
      setMemoryReadCallback(uint16_t(i), &read_ram_);
    setMemoryReadCallback(0xFF00, &read_register_FF00);
    setMemoryReadCallback(0xFF01, &read_register_FF01);
    setMemoryReadCallback(0xFF02, &read_register_FF02);
    setMemoryReadCallback(0xFF03, &read_register_FF03);
    setMemoryReadCallback(0xFF04, &read_register_FF04);
    setMemoryReadCallback(0xFF05, &read_register_FF05);
    setMemoryReadCallback(0xFF06, &read_register_FF06);
    setMemoryReadCallback(0xFF09, &read_register_FF09);
    setMemoryReadCallback(0xFF0A, &read_register_FF0A);
    setMemoryReadCallback(0xFF0C, &read_register_FF0C);
    setMemoryReadCallback(0xFF10, &read_register_FF10);
    setMemoryReadCallback(0xFF12, &read_register_FF12);
    setMemoryReadCallback(0xFF13, &read_register_FF13);
    setMemoryReadCallback(0xFF14, &read_register_FF14);
    setMemoryReadCallback(0xFF15, &read_register_FF15_to_FF19);
    setMemoryReadCallback(0xFF16, &read_register_FF15_to_FF19);
    setMemoryReadCallback(0xFF17, &read_register_FF15_to_FF19);
    setMemoryReadCallback(0xFF18, &read_register_FF15_to_FF19);
    setMemoryReadCallback(0xFF19, &read_register_FF15_to_FF19);
    setMemoryReadCallback(0xFF1A, &read_register_FF1A);
    setMemoryReadCallback(0xFF1B, &read_register_FF1B);
    setMemoryReadCallback(0xFF1C, &read_register_FF1C);
    setMemoryReadCallback(0xFF1D, &read_register_FF1D);
    setMemoryReadCallback(0xFF1E, &read_register_FF1E);
    setMemoryReadCallback(0xFF1F, &read_register_FF1F);
    setMemoryReadCallback(0xFF3E, &read_register_FF3E);
    setMemoryReadCallback(0xFF3F, &read_register_FF3F);
    // TED register write
    setMemoryWriteCallback(0x0000, &write_register_0000);
    setMemoryWriteCallback(0x0001, &write_register_0001);
    for (int i = 0xFD10; i < 0xFD20; i++)
      setMemoryWriteCallback(uint16_t(i), &write_register_FD1x);
    for (int i = 0xFD30; i < 0xFD40; i++)
      setMemoryWriteCallback(uint16_t(i), &write_register_FD3x);
    for (int i = 0xFDD0; i < 0xFDE0; i++)
      setMemoryWriteCallback(uint16_t(i), &write_register_FDDx);
    setMemoryWriteCallback(0xFF00, &write_register_FF00);
    setMemoryWriteCallback(0xFF01, &write_register_FF01);
    setMemoryWriteCallback(0xFF02, &write_register_FF02);
    setMemoryWriteCallback(0xFF03, &write_register_FF03);
    setMemoryWriteCallback(0xFF04, &write_register_FF04);
    setMemoryWriteCallback(0xFF05, &write_register_FF05);
    setMemoryWriteCallback(0xFF06, &write_register_FF06);
    setMemoryWriteCallback(0xFF07, &write_register_FF07);
    setMemoryWriteCallback(0xFF08, &write_register_FF08);
    setMemoryWriteCallback(0xFF09, &write_register_FF09);
    setMemoryWriteCallback(0xFF0A, &write_register_FF0A);
    setMemoryWriteCallback(0xFF0B, &write_register_FF0B);
    setMemoryWriteCallback(0xFF0C, &write_register_FF0C);
    setMemoryWriteCallback(0xFF0D, &write_register_FF0D);
    setMemoryWriteCallback(0xFF0E, &write_register_FF0E);
    setMemoryWriteCallback(0xFF0F, &write_register_FF0F);
    setMemoryWriteCallback(0xFF10, &write_register_FF10);
    setMemoryWriteCallback(0xFF12, &write_register_FF12);
    setMemoryWriteCallback(0xFF13, &write_register_FF13);
    setMemoryWriteCallback(0xFF14, &write_register_FF14);
    setMemoryWriteCallback(0xFF15, &write_register_FF15_to_FF19);
    setMemoryWriteCallback(0xFF16, &write_register_FF15_to_FF19);
    setMemoryWriteCallback(0xFF17, &write_register_FF15_to_FF19);
    setMemoryWriteCallback(0xFF18, &write_register_FF15_to_FF19);
    setMemoryWriteCallback(0xFF19, &write_register_FF15_to_FF19);
    setMemoryWriteCallback(0xFF1A, &write_register_FF1A);
    setMemoryWriteCallback(0xFF1B, &write_register_FF1B);
    setMemoryWriteCallback(0xFF1C, &write_register_FF1C);
    setMemoryWriteCallback(0xFF1D, &write_register_FF1D);
    setMemoryWriteCallback(0xFF1E, &write_register_FF1E);
    setMemoryWriteCallback(0xFF1F, &write_register_FF1F);
    setMemoryWriteCallback(0xFF3E, &write_register_FF3E);
    setMemoryWriteCallback(0xFF3F, &write_register_FF3F);
    // initialize memory paging
    rom_enabled = true;
    rom_enabled_for_video = true;
    rom_bank_low = (unsigned char) 0;
    rom_bank_high = (unsigned char) 0;
    render_func = &render_invalid_mode;
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
    video_column = 96;
    video_line = 224;
    character_line = 0;
    character_position = 0x0000;
    character_position_reload = 0x0000;
    character_column = 0;
    dma_position = 0x0000;
    attr_base_addr = 0x0000;
    bitmap_base_addr = 0x0000;
    charset_base_addr = 0x0000;
    horiz_scroll = 0;
    cursor_position = 0x03FF;
    ted_disabled = false;
    flash_state = false;
    renderWindow = false;
    dmaWindow = false;
    bitmapFetchWindow = false;
    displayWindow = false;
    renderingDisplay = false;
    displayActive = false;
    horizontalBlanking = true;
    verticalBlanking = true;
    singleClockMode = true;
    doubleClockModeEnabled = true;
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
      attr_buf[i] = uint8_t(0);
      attr_buf_tmp[i] = uint8_t(0);
      char_buf[i] = uint8_t(0);
    }
    for (int i = 0; i < 64; i++)
      pixel_buf[i] = uint8_t(0);
    for (int i = 0; i < 414; i++)
      line_buf[i] = uint8_t(0x08);
    line_buf_pos = 0;
    bitmapMode = false;
    currentCharacter = uint8_t(0x00);
    characterMask = uint8_t(0xFF);
    currentAttribute = uint8_t(0x00);
    currentBitmap = uint8_t(0x00);
    pixelBufReadPos = 0;
    pixelBufWritePos = 0;
    attributeDMACnt = 0;
    characterDMACnt = 0;
    videoInterruptLine = 0;
    prvVideoInterruptState = false;
    dataBusState = uint8_t(0xFF);
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
    }
    // reset CPU
    M7501::reset(cold_reset);
  }

  TED7360::~TED7360()
  {
  }

}       // namespace Plus4

