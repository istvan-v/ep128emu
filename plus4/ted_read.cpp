
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

  uint8_t TED7360::read_register_unimplemented(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(0xFF);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_0000(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = ted.memory_ram[addr];
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_0001(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState =
        uint8_t(0xC0) | (ted.tape_write_state ? uint8_t(0x02) : uint8_t(0x00))
                      | (ted.tape_motor_state ? uint8_t(0x00) : uint8_t(0x08))
                      | (ted.tape_read_state ? uint8_t(0x10) : uint8_t(0x00));
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FD10(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = ted.user_port_state & (ted.tape_button_state ?
                                              uint8_t(0xFB) : uint8_t(0xFF));
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FD30(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(0xFF);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF00(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(ted.timer1_state & 0xFF);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF01(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(ted.timer1_state >> 8);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF02(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(ted.timer2_state & 0xFF);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF03(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(ted.timer2_state >> 8);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF04(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(ted.timer3_state & 0xFF);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF05(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(ted.timer3_state >> 8);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF06(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = ted.memory_ram[addr] & uint8_t(0x7F);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF09(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    uint8_t irq_state = ted.memory_ram[addr];
    irq_state = irq_state | uint8_t(0xA1);
    irq_state = (irq_state == uint8_t(0xA1) ? uint8_t(0x21) : irq_state);
    ted.dataBusState = irq_state;
    return irq_state;
  }

  uint8_t TED7360::read_register_FF0A(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = ted.memory_ram[addr] | uint8_t(0xA0);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF0C(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = ted.memory_ram[addr] | uint8_t(0xFC);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF10(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = (ted.memory_ram[addr] | uint8_t(0x7C)) & uint8_t(0x7F);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF12(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = ted.memory_ram[addr] | uint8_t(0xC0);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF13(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = (ted.rom_enabled ?
                        (ted.memory_ram[addr] | uint8_t(0x01))
                        : (ted.memory_ram[addr] & uint8_t(0xFE)));
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF14(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = ted.memory_ram[addr] | uint8_t(0x07);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF15_to_FF19(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = ted.memory_ram[addr] | uint8_t(0x80);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF1A(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(ted.character_position_reload >> 8)
                       | uint8_t(0xFC);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF1B(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(ted.character_position_reload & 0xFF);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF1C(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(ted.video_line >> 8) | uint8_t(0xFE);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF1D(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(ted.video_line & 0xFF);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF1E(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(ted.video_column << 1);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF1F(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = ((ted.memory_ram[addr] & uint8_t(0x78)) | uint8_t(0x80))
                       | ted.character_line;
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF3E(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(0x00);
    return ted.dataBusState;
  }

  uint8_t TED7360::read_register_FF3F(void *userData, uint16_t addr)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = uint8_t(0x00);
    return ted.dataBusState;
  }

}       // namespace Plus4

