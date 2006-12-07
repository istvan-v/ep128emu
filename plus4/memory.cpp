
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

  uint8_t TED7360::getMemoryPage(int n) const
  {
    switch (n & 3) {
    case 0:
      return 0xFC;
    case 1:
      return 0xFD;
    case 2:
    case 3:
      if (!rom_enabled)
        return (uint8_t(n & 3) | 0xFC);
      return ((n & 1) == 0 ?
              uint8_t(rom_bank_low << 1) : uint8_t((rom_bank_high << 1) + 1));
    }
    return 0x00;    // not reached
  }

  uint8_t TED7360::read_ram_(uint16_t addr) const
  {
    return memory_ram[addr];
  }

  uint8_t TED7360::read_rom_low(uint16_t addr) const
  {
    return memory_rom[rom_bank_low][addr & (uint16_t) 0x7FFF];
  }

  uint8_t TED7360::read_rom_high(uint16_t addr) const
  {
    return memory_rom[rom_bank_high][addr & (uint16_t) 0x7FFF];
  }

  uint8_t TED7360::read_rom_FCxx(uint16_t addr) const
  {
    return memory_rom[0][addr & (uint16_t) 0x7FFF];
  }

  uint8_t TED7360::readMemory(uint16_t addr) const
  {
    if (rom_enabled)
      return (this->*(read_memory_rom[addr]))(addr);
    return (this->*(read_memory_ram[addr]))(addr);
  }

  uint8_t TED7360::readMemoryRaw(uint32_t addr) const
  {
    uint8_t   segment = uint8_t((addr >> 14) & 0xFF);
    if (segment >= 0xFC) {
      uint16_t  offs = uint16_t(addr & 0xFFFF);
      return (this->*(read_memory_ram[offs]))(offs);
    }
    else if (segment < 0x08) {
      if (segment == ((rom_bank_high << 1) + 1)) {
        uint16_t  offs = uint16_t(addr & 0xFFFF);
        return (this->*(read_memory_rom[offs]))(offs);
      }
      else
        return memory_rom[segment >> 1][uint16_t(addr & 0x7FFF)];
    }
    return 0xFF;
  }

  void TED7360::write_ram_(uint16_t addr, uint8_t value)
  {
    memory_ram[addr] = value;
  }

  void TED7360::writeMemory(uint16_t addr, uint8_t value)
  {
    (this->*(write_memory[addr]))(addr, value);
  }

  void TED7360::write_register_FDDx(uint16_t addr, uint8_t value)
  {
    (void) value;
    rom_bank_low = (unsigned char) ((int) addr & 3);
    rom_bank_high = (unsigned char) (((int) addr & 12) >> 2);
  }

  void TED7360::write_register_FF3E(uint16_t addr, uint8_t value)
  {
    (void) addr;
    (void) value;
    rom_enabled = true;
  }

  void TED7360::write_register_FF3F(uint16_t addr, uint8_t value)
  {
    (void) addr;
    (void) value;
    rom_enabled = false;
  }

  void TED7360::loadROM(int bankNum, int offs, int cnt, const uint8_t *buf)
  {
    int   i, j, k;

    for (i = offs, j = 0, k = cnt; k > 0; i++, j++, k--)
      memory_rom[bankNum][i & 0x7FFF] = buf[j];
  }

}       // namespace Plus4

