
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

  void TED7360::write_register_0000(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.ioRegister_0000 = value;
  }

  void TED7360::write_register_0001(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.ioRegister_0001 = (value & uint8_t(0x1F)) | uint8_t(0xC0);
    ted.tape_motor_state = ((value & uint8_t(0x08)) ? false : true);
    ted.tape_write_state = ((value & uint8_t(0x02)) ? true : false);
    ted.serialPort.setDATA(0, !(value & uint8_t(0x01)));
    ted.serialPort.setCLK(0, !(value & uint8_t(0x02)));
    ted.serialPort.setATN(!(value & uint8_t(0x04)));
  }

  void TED7360::write_register_FD1x(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.user_port_state = value;
  }

  void TED7360::write_register_FD3x(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.keyboard_row_select_mask = int(value) | 0xFF00;
  }

  void TED7360::write_register_FFxx(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    uint8_t   n = uint8_t(addr) & uint8_t(0xFF);
    ted.tedRegisterWriteMask |= (uint32_t(1) << n);
    ted.tedRegisters[n] = value;
  }

  void TED7360::write_register_FF00(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.timer1_run = false;
    ted.timer1_state = (ted.timer1_state & 0xFF00) | int(value);
    ted.timer1_reload_value = (ted.timer1_reload_value & 0xFF00) | int(value);
  }

  void TED7360::write_register_FF01(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.timer1_run = true;
    int   tmp = int(value) << 8;
    ted.timer1_state = (ted.timer1_state & 0x00FF) | tmp;
    ted.timer1_reload_value = (ted.timer1_reload_value & 0x00FF) | tmp;
  }

  void TED7360::write_register_FF02(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisterWriteMask = ted.tedRegisterWriteMask | 0x00000004U;
    ted.timer2_run = false;
    ted.timer2_state = (ted.timer2_state & 0xFF00) | int(value);
  }

  void TED7360::write_register_FF03(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    if (!ted.timer2_run) {
      ted.tedRegisterWriteMask = ted.tedRegisterWriteMask | 0x00000008U;
      ted.timer2_run = true;
    }
    ted.timer2_state = (ted.timer2_state & 0x00FF) | (int(value) << 8);
  }

  void TED7360::write_register_FF04(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.timer3_run = false;
    ted.timer3_state = (ted.timer3_state & 0xFF00) | int(value);
  }

  void TED7360::write_register_FF05(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.timer3_run = true;
    ted.timer3_state = (ted.timer3_state & 0x00FF) | (int(value) << 8);
  }

  void TED7360::write_register_FF06(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    if (((ted.tedRegisters[0x06] ^ value) & uint8_t(0x07)) != uint8_t(0)) {
      // if vertical scroll has changed:
      if (ted.renderWindow) {
        // check if DMA should be requested
        if (!((uint8_t(ted.savedVideoLine) ^ value) & uint8_t(0x07))) {
          switch (ted.video_column) {
          case 95:
          case 96:
            ted.dmaWindow = true;
            break;
          case 97:
          case 98:
            break;
          default:
            ted.dmaWindow = true;
            if (ted.savedVideoLine != 203) {
              if (ted.dmaCycleCounter == 0 &&
                  (ted.video_column >= 99 || ted.video_column < 75))
                ted.dmaCycleCounter = 2;
              ted.dmaFlags = ted.dmaFlags | 1;
              ted.dma_position = ted.dma_position & 0x03FF;
            }
            break;
          }
        }
        else if (ted.dmaFlags & 1) {
          // abort an already started DMA transfer
          ted.dmaFlags = ted.dmaFlags & 2;
          if (!ted.dmaFlags) {
            ted.dmaCycleCounter = 0;
            ted.setIsCPURunning(true);
          }
        }
      }
    }
    ted.tedRegisters[0x06] = value;
    ted.selectRenderer();
  }

  void TED7360::write_register_FF07(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    uint8_t   bitsChanged = value ^ ted.tedRegisters[0x07];
    ted.tedRegisters[0x07] = value;
    ted.horiz_scroll = int(value & uint8_t(0x07));
    ted.ted_disabled = !!(value & uint8_t(0x20));
    if (bitsChanged & uint8_t(0x60)) {
      if (bitsChanged & uint8_t(0x20)) {
        if (ted.ted_disabled) {
          if (ted.video_column & uint8_t(0x01)) {
            ted.singleClockModeFlags |= uint8_t(0x01);
            ted.video_column++;
            ted.video_column &= uint8_t(0x7F);
          }
        }
      }
      if (bitsChanged & uint8_t(0x40))
        ted.ntscModeChangeCallback(!!(value & uint8_t(0x40)));
    }
    ted.selectRenderer();
  }

  void TED7360::write_register_FF08(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    int     mask = ted.keyboard_row_select_mask & ((int(value) << 8) | 0xFF);
    uint8_t key_state = uint8_t(0xFF);
    for (int i = 0; i < 11; i++, mask >>= 1) {
      if (!(mask & 1))
        key_state &= ted.keyboard_matrix[i];
    }
    ted.tedRegisters[0x08] = key_state;
    ted.keyboard_row_select_mask = 0xFFFF;
  }

  void TED7360::write_register_FF09(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    // bit 2 (light pen interrupt) is always set
    ted.tedRegisters[0x09] = (ted.tedRegisters[0x09] & (value ^ uint8_t(0xFF)))
                             | uint8_t(0x04);
  }

  void TED7360::write_register_FF0A(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x0A] = value;
    ted.videoInterruptLine = (ted.videoInterruptLine & 0x00FF)
                             | (int(value & uint8_t(0x01)) << 8);
  }

  void TED7360::write_register_FF0B(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x0B] = value;
    ted.videoInterruptLine = (ted.videoInterruptLine & 0x0100)
                             | int(value & uint8_t(0xFF));
  }

  void TED7360::write_register_FF0C(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x0C] = value;
    ted.cursor_position &= 0x00FF;
    ted.cursor_position |= (int(value & uint8_t(0x03)) << 8);
  }

  void TED7360::write_register_FF0D(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x0D] = value;
    ted.cursor_position &= 0x0300;
    ted.cursor_position |= int(value);
  }

  void TED7360::write_register_FF0E(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x0E] = value;
    ted.sound_channel_1_reload &= 0x0300;
    ted.sound_channel_1_reload |= int(value);
  }

  void TED7360::write_register_FF0F(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x0F] = value;
    ted.sound_channel_2_reload &= 0x0300;
    ted.sound_channel_2_reload |= int(value);
  }

  void TED7360::write_register_FF10(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x10] = value;
    ted.sound_channel_2_reload &= 0x00FF;
    ted.sound_channel_2_reload |= (int(value & uint8_t(0x03)) << 8);
  }

  void TED7360::write_register_FF12(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x12] = value;
    ted.sound_channel_1_reload &= 0x00FF;
    ted.sound_channel_1_reload |= (int(value & uint8_t(0x03)) << 8);
    ted.tedBitmapReadMap = (ted.tedBitmapReadMap & 0x7F78U)
                           | ((unsigned int) (value & uint8_t(0x04)) << 5);
    ted.bitmap_base_addr = int(value & uint8_t(0x38)) << 10;
  }

  void TED7360::write_register_FF13(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x13] = value;
    ted.singleClockModeFlags &= uint8_t(0x01);
    ted.singleClockModeFlags |= (value & uint8_t(0x02));
    ted.selectRenderer();
  }

  void TED7360::write_register_FF14(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x14] = value;
    ted.attr_base_addr = int(value & uint8_t(0xF8)) << 8;
  }

  void TED7360::write_register_FF15_to_FF19(void *userData,
                                            uint16_t addr, uint8_t value)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    uint8_t   n = uint8_t(addr) & uint8_t(0xFF);
    ted.tedRegisterWriteMask |= (uint32_t(1) << n);
    ted.tedRegisters[n] = value | uint8_t(0x80);
  }

  void TED7360::write_register_FF1A(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.character_position_reload &= 0x00FF;
    ted.character_position_reload |= (int(value & uint8_t(0x03)) << 8);
  }

  void TED7360::write_register_FF1B(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.character_position_reload &= 0x0300;
    ted.character_position_reload |= int(value);
  }

  void TED7360::write_register_FF1C(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisterWriteMask = ted.tedRegisterWriteMask | 0x10000000U;
    ted.video_line =
        (ted.video_line & 0x00FF) | (int(value & uint8_t(0x01)) << 8);
  }

  void TED7360::write_register_FF1D(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisterWriteMask = ted.tedRegisterWriteMask | 0x20000000U;
    ted.video_line = (ted.video_line & 0x0100) | int(value);
  }

  void TED7360::write_register_FF1E(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisterWriteMask = ted.tedRegisterWriteMask | 0x40000000U;
    // NOTE: values written to this register are inverted
    uint8_t   newVal = ((value ^ uint8_t(0xFF)) & uint8_t(0xFC)) >> 1;
    ted.video_column = (ted.video_column & uint8_t(0x01)) | newVal;
  }

  void TED7360::write_register_FF1F(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x1F] = value;
    ted.character_line = int(value & uint8_t(0x07));
  }

}       // namespace Plus4

