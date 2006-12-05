
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
#include "fileio.hpp"
#include "cpu.hpp"
#include "ted.hpp"

static const float brightnessToYTable[8] = {
   0.20f,  0.25f,  0.27f,  0.33f,  0.45f,  0.57f,  0.65f,  0.83f
};

static const float colorToUTable[16] = {
   0.00f,  0.00f, -0.04f,  0.03f,  0.09f, -0.08f,  0.14f, -0.12f,
  -0.10f, -0.12f, -0.12f,  0.01f,  0.00f,  0.11f,  0.14f, -0.12f
};

static const float colorToVTable[16] = {
   0.00f,  0.00f,  0.14f, -0.14f,  0.11f, -0.13f, -0.04f,  0.03f,
   0.11f,  0.08f, -0.05f,  0.14f, -0.15f, -0.09f,  0.00f, -0.09f
};

namespace Plus4 {

  void TED7360::convertPixelToRGB(uint8_t color,
                                  float& red, float& green, float& blue)
  {
    uint8_t c = color & 0x0F;
    uint8_t b = (color & 0x70) >> 4;
    float   y = 0.06f;
    float   u, v;
    if (c)
      y = brightnessToYTable[b];
    u = colorToUTable[c];
    v = colorToVTable[c];
    // R = (V / 0.877) + Y
    // B = (U / 0.492) + Y
    // G = (Y - ((R * 0.299) + (B * 0.114))) / 0.587
    red = (v / 0.877f) + y;
    blue = (u / 0.492f) + y;
    green = (y - ((red * 0.299f) + (blue * 0.114f))) / 0.587f;
    red = (red > 0.0f ? (red < 1.0f ? red : 1.0f) : 0.0f);
    green = (green > 0.0f ? (green < 1.0f ? green : 1.0f) : 0.0f);
    blue = (blue > 0.0f ? (blue < 1.0f ? blue : 1.0f) : 0.0f);
  }

  void TED7360::setCPUClockMultiplier(int clk)
  {
    if (clk < 1)
      cpu_clock_multiplier = 1;
    else if (clk > 100)
      cpu_clock_multiplier = 100;
    else
      cpu_clock_multiplier = clk;
  }

  // set the state of the specified key
  // valid key numbers are:
  //
  //     0: Del          1: Return       2: £            3: Help
  //     4: F1           5: F2           6: F3           7: @
  //     8: 3            9: W           10: A           11: 4
  //    12: Z           13: S           14: E           15: Shift
  //    16: 5           17: R           18: D           19: 6
  //    20: C           21: F           22: T           23: X
  //    24: 7           25: Y           26: G           27: 8
  //    28: B           29: H           30: U           31: V
  //    32: 9           33: I           34: J           35: 0
  //    36: M           37: K           38: O           39: N
  //    40: Down        41: P           42: L           43: Up
  //    44: .           45: :           46: -           47: ,
  //    48: Left        49: *           50: ;           51: Right
  //    52: Esc         53: =           54: +           55: /
  //    56: 1           57: Home        58: Ctrl        59: 2
  //    60: Space       61: C=          62: Q           63: Stop
  //
  //    72: Joy2 Up     73: Joy2 Down   74: Joy2 Left   75: Joy2 Right
  //    79: Joy2 Fire
  //    80: Joy1 Up     81: Joy1 Down   82: Joy1 Left   83: Joy1 Right
  //    86: Joy1 Fire

  void TED7360::setKeyState(int keyNum, bool isPressed)
  {
    int     ndx = (keyNum & 0x78) >> 3;
    int     mask = 1 << (keyNum & 7);

    if (isPressed)
      keyboard_matrix[ndx] &= ((uint8_t) mask ^ (uint8_t) 0xFF);
    else
      keyboard_matrix[ndx] |= (uint8_t) mask;
  }

  // --------------------------------------------------------------------------

  void TED7360::saveState(Ep128::File::Buffer& buf)
  {
    // TODO: implement this
    (void) buf;
  }

  void TED7360::saveState(Ep128::File& f)
  {
    Ep128::File::Buffer buf;
    this->saveState(buf);
    f.addChunk(Ep128::File::EP128EMU_CHUNKTYPE_TED_STATE, buf);
  }

  void TED7360::loadState(Ep128::File::Buffer& buf)
  {
    // TODO: implement this
    (void) buf;
  }

  void TED7360::saveProgram(Ep128::File::Buffer& buf)
  {
    uint16_t  startAddr, endAddr, len;
    startAddr =
        uint16_t(memory_ram[0x002B]) | (uint16_t(memory_ram[0x002C]) << 8);
    endAddr =
        uint16_t(memory_ram[0x002D]) | (uint16_t(memory_ram[0x002E]) << 8);
    len = (endAddr > startAddr ? (endAddr - startAddr) : uint16_t(0));
    buf.writeUInt32(startAddr);
    buf.writeUInt32(len);
    while (len) {
      buf.writeByte((this->*(read_memory_ram[startAddr]))(startAddr));
      startAddr = (startAddr + 1) & 0xFFFF;
      len--;
    }
  }

  void TED7360::saveProgram(Ep128::File& f)
  {
    Ep128::File::Buffer buf;
    this->saveProgram(buf);
    f.addChunk(Ep128::File::EP128EMU_CHUNKTYPE_PLUS4_PRG, buf);
  }

  void TED7360::saveProgram(const char *fileName)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Ep128::Exception("invalid plus4 program file name");
    std::FILE *f = std::fopen(fileName, "wb");
    if (!f)
      throw Ep128::Exception("error opening plus4 program file");
    uint16_t  startAddr, endAddr, len;
    startAddr =
        uint16_t(memory_ram[0x002B]) | (uint16_t(memory_ram[0x002C]) << 8);
    endAddr =
        uint16_t(memory_ram[0x002D]) | (uint16_t(memory_ram[0x002E]) << 8);
    len = (endAddr > startAddr ? (endAddr - startAddr) : uint16_t(0));
    bool  err = true;
    if (std::fputc(int(startAddr & 0xFF), f) != EOF) {
      if (std::fputc(int((startAddr >> 8) & 0xFF), f) != EOF) {
        while (len) {
          int   c = (this->*(read_memory_ram[startAddr]))(startAddr);
          if (std::fputc(c, f) == EOF)
            break;
          startAddr = (startAddr + 1) & 0xFFFF;
          len--;
        }
        err = (len != 0);
      }
    }
    if (std::fclose(f) != 0)
      err = true;
    if (err)
      throw Ep128::Exception("error writing plus4 program file "
                             "-- is the disk full ?");
  }

  void TED7360::loadProgram(Ep128::File::Buffer& buf)
  {
    buf.setPosition(0);
    uint32_t  addr = buf.readUInt32();
    uint32_t  len = buf.readUInt32();
    if (addr >= 0x00010000U)
      throw Ep128::Exception("invalid start address in plus4 program data");
    if (len >= 0x00010000U ||
        size_t(len) != (buf.getDataSize() - buf.getPosition()))
      throw Ep128::Exception("invalid plus4 program length");
    memory_ram[0x002B] = uint8_t(addr & 0xFF);
    memory_ram[0x002C] = uint8_t((addr >> 8) & 0xFF);
    while (len) {
      (this->*(write_memory[addr]))(uint16_t(addr), buf.readByte());
      addr = (addr + 1) & 0xFFFF;
      len--;
    }
    memory_ram[0x002D] = uint8_t(addr & 0xFF);
    memory_ram[0x002E] = uint8_t((addr >> 8) & 0xFF);
    memory_ram[0x002F] = uint8_t(addr & 0xFF);
    memory_ram[0x0030] = uint8_t((addr >> 8) & 0xFF);
    memory_ram[0x0031] = uint8_t(addr & 0xFF);
    memory_ram[0x0032] = uint8_t((addr >> 8) & 0xFF);
    memory_ram[0x0033] = 0x00;
    memory_ram[0x0034] = 0xFD;
    memory_ram[0x0035] = 0xFE;
    memory_ram[0x0036] = 0xFC;
    memory_ram[0x0037] = 0x00;
    memory_ram[0x0038] = 0xFD;
  }

  void TED7360::loadProgram(const char *fileName)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Ep128::Exception("invalid plus4 program file name");
    std::FILE *f = std::fopen(fileName, "rb");
    if (!f)
      throw Ep128::Exception("error opening plus4 program file");
    uint16_t  addr;
    int       c;
    c = std::fgetc(f);
    if (c == EOF)
      throw Ep128::Exception("unexpected end of plus4 program file");
    addr = uint16_t(c & 0xFF);
    c = std::fgetc(f);
    if (c == EOF)
      throw Ep128::Exception("unexpected end of plus4 program file");
    addr |= uint16_t((c & 0xFF) << 8);
    memory_ram[0x002B] = uint8_t(addr & 0xFF);
    memory_ram[0x002C] = uint8_t((addr >> 8) & 0xFF);
    size_t  len = 0;
    while (true) {
      c = std::fgetc(f);
      if (c == EOF)
        break;
      if (++len > 0xFFFF) {
        std::fclose(f);
        throw Ep128::Exception("plus4 program file has invalid length");
      }
      (this->*(write_memory[addr]))(addr, uint8_t(c));
      addr = (addr + 1) & 0xFFFF;
    }
    std::fclose(f);
    memory_ram[0x002D] = uint8_t(addr & 0xFF);
    memory_ram[0x002E] = uint8_t((addr >> 8) & 0xFF);
    memory_ram[0x002F] = uint8_t(addr & 0xFF);
    memory_ram[0x0030] = uint8_t((addr >> 8) & 0xFF);
    memory_ram[0x0031] = uint8_t(addr & 0xFF);
    memory_ram[0x0032] = uint8_t((addr >> 8) & 0xFF);
    memory_ram[0x0033] = 0x00;
    memory_ram[0x0034] = 0xFD;
    memory_ram[0x0035] = 0xFE;
    memory_ram[0x0036] = 0xFC;
    memory_ram[0x0037] = 0x00;
    memory_ram[0x0038] = 0xFD;
  }

  void TED7360::registerChunkTypes(Ep128::File& f)
  {
    // TODO: implement this
    (void) f;
  }

}       // namespace Plus4

