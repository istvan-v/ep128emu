
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

  uint8_t TED7360::read_ram_(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = ted.memory_ram[addr];
    return ted.dataBusState;
  }

  uint8_t TED7360::read_memory_8000(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    if (ted.rom_enabled)
      ted.dataBusState =
          ted.memory_rom[ted.rom_bank_low][addr & (uint16_t) 0x7FFF];
    else
      ted.dataBusState = ted.memory_ram[addr];
    return ted.dataBusState;
  }

  uint8_t TED7360::read_memory_C000(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    if (ted.rom_enabled)
      ted.dataBusState =
          ted.memory_rom[ted.rom_bank_high][addr & (uint16_t) 0x7FFF];
    else
      ted.dataBusState = ted.memory_ram[addr];
    return ted.dataBusState;
  }

  uint8_t TED7360::read_memory_FC00(void *userData, uint16_t addr)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    if (ted.rom_enabled)
      ted.dataBusState = ted.memory_rom[0][addr & (uint16_t) 0x7FFF];
    else
      ted.dataBusState = ted.memory_ram[addr];
    return ted.dataBusState;
  }

  uint8_t TED7360::readMemoryRaw(uint32_t addr) const
  {
    uint8_t   segment = uint8_t((addr >> 14) & 0xFF);
    if (segment >= 0xFC) {
      uint16_t  offs = uint16_t(addr & 0xFFFF);
      if ((offs > 0x0001 && offs < 0xFD00) || offs >= 0xFF40)
        return memory_ram[offs];
      else {
        // FIXME: this is an ugly hack to work around const declaration
        TED7360&  ted = *(const_cast<TED7360 *>(this));
        uint8_t savedDataBusState = dataBusState;
        uint8_t retval = ted.readMemory(offs);
        ted.dataBusState = savedDataBusState;
        return retval;
      }
    }
    else if (segment < 0x08) {
      uint16_t  offs = uint16_t(addr & 0x7FFF);
      if (segment == ((rom_bank_high << 1) + 1) &&
          (offs >= 0x7D00 && offs < 0x7F40)) {
        // FIXME: this is an ugly hack to work around const declaration
        TED7360&  ted = *(const_cast<TED7360 *>(this));
        uint8_t savedDataBusState = dataBusState;
        uint8_t retval = ted.readMemory(offs | uint16_t(0x8000));
        ted.dataBusState = savedDataBusState;
        return retval;
      }
      else
        return memory_rom[segment >> 1][offs];
    }
    return 0xFF;
  }

  void TED7360::write_ram_(void *userData, uint16_t addr, uint8_t value)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.memory_ram[addr] = value;
  }

  void TED7360::write_register_FDDx(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.rom_bank_low = (unsigned char) ((int) addr & 3);
    ted.rom_bank_high = (unsigned char) (((int) addr & 12) >> 2);
  }

  void TED7360::write_register_FF3E(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.rom_enabled = true;
  }

  void TED7360::write_register_FF3F(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.rom_enabled = false;
  }

  void TED7360::loadROM(int bankNum, int offs, int cnt, const uint8_t *buf)
  {
    int   i, j, k;

    for (i = offs, j = 0, k = cnt; k > 0; i++, j++, k--)
      memory_rom[bankNum][i & 0x7FFF] = buf[j];
  }

}       // namespace Plus4

