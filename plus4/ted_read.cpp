
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

  uint8_t TED7360::read_register_unimplemented(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) 0xFF;
  }

  uint8_t TED7360::read_register_0000(uint16_t addr) const
  {
    return memory_ram[addr];
  }

  uint8_t TED7360::read_register_0001(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t(0xC0)
            | (tape_write_state ? uint8_t(0x02) : uint8_t(0x00))
            | (tape_motor_state ? uint8_t(0x00) : uint8_t(0x08))
            | (tape_read_state ? uint8_t(0x10) : uint8_t(0x00)));
  }

  uint8_t TED7360::read_register_FD10(uint16_t addr) const
  {
    (void) addr;
    return (user_port_state
            & (tape_button_state ? (uint8_t) 0xFB : (uint8_t) 0xFF));
  }

  uint8_t TED7360::read_register_FD30(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) 0xFF;
  }

  uint8_t TED7360::read_register_FF00(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) (timer1_state & 0xFF);
  }

  uint8_t TED7360::read_register_FF01(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) (timer1_state >> 8);
  }

  uint8_t TED7360::read_register_FF02(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) (timer2_state & 0xFF);
  }

  uint8_t TED7360::read_register_FF03(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) (timer2_state >> 8);
  }

  uint8_t TED7360::read_register_FF04(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) (timer3_state & 0xFF);
  }

  uint8_t TED7360::read_register_FF05(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) (timer3_state >> 8);
  }

  uint8_t TED7360::read_register_FF06(uint16_t addr) const
  {
    return (memory_ram[addr] & (uint8_t) 0x7F);
  }

  uint8_t TED7360::read_register_FF09(uint16_t addr) const
  {
    uint8_t irq_state = memory_ram[addr];
    if (video_interrupt_shift_register)
      irq_state |= (uint8_t) 0x02;
    irq_state = irq_state | uint8_t(0xA1);
    irq_state = (irq_state == uint8_t(0xA1) ? uint8_t(0x21) : irq_state);
    return irq_state;
  }

  uint8_t TED7360::read_register_FF0A(uint16_t addr) const
  {
    return (memory_ram[addr] | (uint8_t) 0xA0);
  }

  uint8_t TED7360::read_register_FF0C(uint16_t addr) const
  {
    return (memory_ram[addr] | (uint8_t) 0xFC);
  }

  uint8_t TED7360::read_register_FF10(uint16_t addr) const
  {
    return ((memory_ram[addr] | (uint8_t) 0x7C) & (uint8_t) 0x7F);
  }

  uint8_t TED7360::read_register_FF12(uint16_t addr) const
  {
    return (memory_ram[addr] | (uint8_t) 0xC0);
  }

  uint8_t TED7360::read_register_FF13(uint16_t addr) const
  {
    return (rom_enabled ? (memory_ram[addr] | (uint8_t) 0x01)
                          : (memory_ram[addr] & (uint8_t) 0xFE));
  }

  uint8_t TED7360::read_register_FF14(uint16_t addr) const
  {
    return (memory_ram[addr] | (uint8_t) 0x07);
  }

  uint8_t TED7360::read_register_FF15_to_FF19(uint16_t addr) const
  {
    return (memory_ram[addr] | (uint8_t) 0x80);
  }

  uint8_t TED7360::read_register_FF1A(uint16_t addr) const
  {
    (void) addr;
    return ((uint8_t) (character_position_reload >> 8) | (uint8_t) 0xFF);
  }

  uint8_t TED7360::read_register_FF1B(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) (character_position_reload & 0xFF);
  }

  uint8_t TED7360::read_register_FF1C(uint16_t addr) const
  {
    (void) addr;
    return ((uint8_t) (video_line >> 8) | (uint8_t) 0xFE);
  }

  uint8_t TED7360::read_register_FF1D(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) (video_line & 0xFF);
  }

  uint8_t TED7360::read_register_FF1E(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) (video_column << 1);
  }

  uint8_t TED7360::read_register_FF1F(uint16_t addr) const
  {
    return (((memory_ram[addr] & (uint8_t) 0x78) | (uint8_t) 0x80)
            | character_line);
  }

  uint8_t TED7360::read_register_FF3E(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) 0x00;
  }

  uint8_t TED7360::read_register_FF3F(uint16_t addr) const
  {
    (void) addr;
    return (uint8_t) 0x00;
  }

}       // namespace Plus4

