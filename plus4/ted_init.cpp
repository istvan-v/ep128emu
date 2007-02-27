
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

  TED7360::TED7360() : M7501()
  {
    // create initial memory map
    ramSegments = 0;
    for (int i = 0; i < 256; i++)
      segmentTable[i] = (uint8_t *) 0;
    try {
      setRAMSize(64);
    }
    catch (...) {
      for (int i = 0; i < 256; i++) {
        if (segmentTable[i] != (uint8_t *) 0) {
          delete[] segmentTable[i];
          segmentTable[i] = (uint8_t *) 0;
        }
      }
      throw;
    }
    setMemoryCallbackUserData(this);
    for (uint16_t i = 0x0000; i <= 0x0FFF; i++) {
      setMemoryReadCallback(i, &read_memory_0000_to_0FFF);
      setMemoryWriteCallback(i, &write_memory_0000_to_0FFF);
    }
    for (uint16_t i = 0x1000; i <= 0x3FFF; i++) {
      setMemoryReadCallback(i, &read_memory_1000_to_3FFF);
      setMemoryWriteCallback(i, &write_memory_1000_to_3FFF);
    }
    for (uint16_t i = 0x4000; i <= 0x7FFF; i++) {
      setMemoryReadCallback(i, &read_memory_4000_to_7FFF);
      setMemoryWriteCallback(i, &write_memory_4000_to_7FFF);
    }
    for (uint16_t i = 0x8000; i <= 0xBFFF; i++) {
      setMemoryReadCallback(i, &read_memory_8000_to_BFFF);
      setMemoryWriteCallback(i, &write_memory_8000_to_BFFF);
    }
    for (uint16_t i = 0xC000; i <= 0xFBFF; i++) {
      setMemoryReadCallback(i, &read_memory_C000_to_FBFF);
      setMemoryWriteCallback(i, &write_memory_C000_to_FBFF);
    }
    for (uint16_t i = 0xFC00; i <= 0xFCFF; i++) {
      setMemoryReadCallback(i, &read_memory_FC00_to_FCFF);
      setMemoryWriteCallback(i, &write_memory_FC00_to_FCFF);
    }
    for (uint16_t i = 0xFD00; i <= 0xFEFF; i++) {
      setMemoryReadCallback(i, &read_memory_FD00_to_FEFF);
      setMemoryWriteCallback(i, &write_memory_FD00_to_FEFF);
    }
    for (uint16_t i = 0xFF00; i <= 0xFF1F; i++) {
      setMemoryReadCallback(i, &read_register_FFxx);
      setMemoryWriteCallback(i, &write_register_FFxx);
    }
    for (uint32_t i = 0xFF20; i <= 0xFFFF; i++) {
      setMemoryReadCallback(uint16_t(i), &read_memory_FF00_to_FFFF);
      setMemoryWriteCallback(uint16_t(i), &write_memory_FF00_to_FFFF);
    }
    // TED register read
    setMemoryReadCallback(0x0000, &read_register_0000);
    setMemoryReadCallback(0x0001, &read_register_0001);
    for (uint16_t i = 0xFD00; i <= 0xFD0F; i++)
      setMemoryReadCallback(i, &read_register_FD0x);
    for (uint16_t i = 0xFD10; i <= 0xFD1F; i++)
      setMemoryReadCallback(i, &read_register_FD1x);
    setMemoryReadCallback(0xFD16, &read_register_FD16);
    for (uint16_t i = 0xFD30; i <= 0xFD3F; i++)
      setMemoryReadCallback(i, &read_register_FD3x);
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
    setMemoryReadCallback(0xFF3E, &read_register_FF3E_FF3F);
    setMemoryReadCallback(0xFF3F, &read_register_FF3E_FF3F);
    // TED register write
    setMemoryWriteCallback(0x0000, &write_register_0000);
    setMemoryWriteCallback(0x0001, &write_register_0001);
    for (uint16_t i = 0xFD10; i <= 0xFD1F; i++)
      setMemoryWriteCallback(i, &write_register_FD1x);
    setMemoryWriteCallback(0xFD16, &write_register_FD16);
    for (uint16_t i = 0xFD30; i <= 0xFD3F; i++)
      setMemoryWriteCallback(i, &write_register_FD3x);
    for (uint16_t i = 0xFDD0; i <= 0xFDDF; i++)
      setMemoryWriteCallback(i, &write_register_FDDx);
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
    // initialize external ports
    user_port_state = uint8_t(0xFF);
    tape_read_state = false;
    tape_button_state = false;
    // set internal TED registers
    tedRegisters[0x07] = uint8_t(0x00);     // default to PAL mode
    this->initRegisters();
    cpu_clock_multiplier = 1;
    for (int i = 0; i < 16; i++)                // keyboard matrix
      keyboard_matrix[i] = uint8_t(0xFF);
  }

  void TED7360::initRegisters()
  {
    // initialize memory paging
    hannesRegister = uint8_t(0xFF);
    memoryReadMap = 0x06F8U;
    memoryWriteMap = 0x0678U;
    cpuMemoryReadMap = 0x06F8U;
    tedDMAReadMap = 0x07F8U;
    tedBitmapReadMap = 0x07F8U;
    // clear memory used by TED registers
    ioRegister_0000 = uint8_t(0x0F);
    ioRegister_0001 = uint8_t(0xC8);
    if (tedRegisters[0x07] & uint8_t(0x40)) {
      tedRegisters[0x07] = uint8_t(0x00);
      ntscModeChangeCallback(false);
    }
    tedRegisterWriteMask = 0U;
    for (uint8_t i = 0x00; i <= 0x1F; i++)
      tedRegisters[i] = uint8_t(0x00);
    tedRegisters[0x08] = uint8_t(0xFF);
    tedRegisters[0x0C] = uint8_t(0xFF);
    tedRegisters[0x0D] = uint8_t(0xFF);
    tedRegisters[0x12] = uint8_t(0xC4);
    tedRegisters[0x13] = uint8_t(0x01);
    tedRegisters[0x1D] = uint8_t(0xE0);
    tedRegisters[0x1E] = uint8_t(0xC6);
    // set internal TED registers
    render_func = &render_invalid_mode;
    cycle_count = 0;
    video_column = 99;
    video_line = 224;
    character_line = 0;
    character_position = 0x0000;
    character_position_reload = 0x0000;
    character_column = 0;
    dma_position = 0x03FB;
    dma_position_reload = 0x03FE;
    attr_base_addr = 0x0000;
    bitmap_base_addr = 0x0000;
    charset_base_addr = 0x0000;
    horiz_scroll = 0;
    cursor_position = 0x03FF;
    ted_disabled = false;
    flashState = 0x00;
    renderWindow = false;
    dmaWindow = false;
    bitmapFetchWindow = false;
    displayWindow = false;
    renderingDisplay = false;
    displayActive = false;
    horizontalBlanking = true;
    verticalBlanking = true;
    singleClockModeFlags = 0x01;
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
    sound_channel_1_state = uint8_t(1);
    sound_channel_2_state = uint8_t(1);
    sound_channel_2_noise_state = uint8_t(0xFF);
    sound_channel_2_noise_output = uint8_t(1);
    for (int i = 0; i < 64; i++) {              // video buffers
      attr_buf[i] = uint8_t(0);
      attr_buf_tmp[i] = uint8_t(0);
      char_buf[i] = uint8_t(0);
    }
    pixelBuf1[0] = 0U;
    pixelBuf1[1] = 0U;
    pixelBuf2[0] = 0U;
    pixelBuf2[1] = 0U;
    pixelBuf2[2] = 0U;
    for (int i = 0; i < 432; i++)
      line_buf[i] = uint8_t(0x08);
    for (int i = 0; i < 4; i++)
      line_buf_tmp[i] = uint8_t(0);
    line_buf_pos = 432;
    bitmapMode = false;
    currentCharacter = uint8_t(0x00);
    characterMask = uint8_t(0xFF);
    currentAttribute = uint8_t(0x00);
    currentBitmap = uint8_t(0x00);
    cursorFlag = false;
    dmaCycleCounter = 0;
    dmaFlags = 0;
    savedCharacterLine = 0;
    savedVideoLine = 224;
    videoInterruptLine = 0;
    prvVideoInterruptState = false;
    dataBusState = uint8_t(0xFF);
    keyboard_row_select_mask = 0xFFFF;
    tape_motor_state = false;
    tape_write_state = false;
    serialPort.removeDevices(0xFFFF);
    serialPort.setATN(true);
  }

  void TED7360::reset(bool cold_reset)
  {
    if (cold_reset) {
      // reset TED registers
      this->initRegisters();
      // make sure that RAM size is detected correctly
      for (uint8_t j = 0xC0; j < 0xFF; j++) {
        if ((j & uint8_t(0x02)) != uint8_t(0) &&
            !(j == uint8_t(0xFE) && segmentTable[0xFC] == (uint8_t *) 0))
          continue;
        if (segmentTable[j] != (uint8_t *) 0) {
          for (uint16_t i = 0x3FFD; i > 0x3FF5; i--) {
            if (segmentTable[j][i] != readMemory(i | 0xC000))
              break;
            if (i == 0x3FF6) {
              segmentTable[j][i] =
                  (segmentTable[j][i] + uint8_t(1)) & uint8_t(0xFF);
            }
          }
        }
      }
      // force RAM testing
      writeMemory(0x0508, 0x00);
    }
    // reset CPU
    M7501::reset(cold_reset);
  }

  TED7360::~TED7360()
  {
    for (int i = 0; i < 256; i++) {
      if (segmentTable[i] != (uint8_t *) 0) {
        delete[] segmentTable[i];
        segmentTable[i] = (uint8_t *) 0;
      }
    }
  }

}       // namespace Plus4

