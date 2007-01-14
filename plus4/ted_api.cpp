
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

  class ChunkType_TED7360Snapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    TED7360&  ref;
   public:
    ChunkType_TED7360Snapshot(TED7360& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_TED7360Snapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_TED_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  class ChunkType_Plus4Program : public Ep128Emu::File::ChunkTypeHandler {
   private:
    TED7360&  ref;
   public:
    ChunkType_Plus4Program(TED7360& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Plus4Program()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_PLUS4_PRG;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadProgram(buf);
    }
  };

  void TED7360::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    uint8_t   romBitmap = 0;
    for (uint8_t i = 0; i < 8; i++) {
      // find non-empty ROM segments
      romBitmap = romBitmap >> 1;
      if (segmentTable[i] != (uint8_t *) 0)
        romBitmap = romBitmap | uint8_t(0x80);
    }
    buf.writeByte(romBitmap);
    buf.writeByte(ramSegments);
    // save RAM segments
    for (size_t i = 0x08; i <= 0xFF; i++) {
      if (segmentTable[i] != (uint8_t *) 0) {
        for (size_t j = 0; j < 16384; j++)
          buf.writeByte(segmentTable[i][j]);
      }
    }
    // save ROM segments
    for (size_t i = 0x00; i < 0x08; i++) {
      if (segmentTable[i] != (uint8_t *) 0) {
        for (size_t j = 0; j < 16384; j++)
          buf.writeByte(segmentTable[i][j]);
      }
    }
    // save I/O and TED registers
    buf.writeByte(ioRegister_0000);
    buf.writeByte(ioRegister_0001);
    for (uint8_t i = 0x00; i <= 0x1F; i++)
      buf.writeByte(readMemoryCPU(uint16_t(0xFF00) | uint16_t(i)));
    // save memory paging
    buf.writeByte(hannesRegister);
    buf.writeByte((uint8_t(cpuMemoryReadMap) & uint8_t(0x80))
                  | (uint8_t(cpuMemoryReadMap >> 11) & uint8_t(0x0F)));
    // save internal registers
    buf.writeUInt32(tedRegisterWriteMask);
    buf.writeUInt32(uint32_t(cycle_count));
    buf.writeByte(video_column);
    buf.writeUInt32(uint32_t(video_line));
    buf.writeByte(uint8_t(character_line));
    buf.writeUInt32(uint32_t(character_position));
    buf.writeUInt32(uint32_t(character_position_reload));
    buf.writeByte(uint8_t(character_column));
    buf.writeUInt32(uint32_t(dma_position));
    buf.writeBoolean(flash_state);
    buf.writeBoolean(renderWindow);
    buf.writeBoolean(dmaWindow);
    buf.writeBoolean(bitmapFetchWindow);
    buf.writeBoolean(displayWindow);
    buf.writeBoolean(renderingDisplay);
    buf.writeBoolean(displayActive);
    buf.writeBoolean(horizontalBlanking);
    buf.writeBoolean(verticalBlanking);
    buf.writeBoolean(singleClockMode);
    buf.writeBoolean(timer1_run);
    buf.writeBoolean(timer2_run);
    buf.writeBoolean(timer3_run);
    buf.writeUInt32(uint32_t(timer1_state));
    buf.writeUInt32(uint32_t(timer1_reload_value));
    buf.writeUInt32(uint32_t(timer2_state));
    buf.writeUInt32(uint32_t(timer3_state));
    buf.writeUInt32(uint32_t(sound_channel_1_cnt));
    buf.writeUInt32(uint32_t(sound_channel_2_cnt));
    buf.writeByte(sound_channel_1_state);
    buf.writeByte(sound_channel_2_state);
    buf.writeByte(sound_channel_2_noise_state);
    buf.writeByte(sound_channel_2_noise_output);
    buf.writeByte(currentCharacter);
    buf.writeByte(currentAttribute);
    buf.writeByte(currentBitmap);
    buf.writeByte(pixelBufReadPos);
    buf.writeByte(pixelBufWritePos);
    buf.writeByte(dmaCycleCounter);
    buf.writeBoolean(attributeDMAFlag);
    buf.writeByte(savedCharacterLine);
    buf.writeUInt32(uint32_t(savedVideoLine));
    buf.writeBoolean(prvVideoInterruptState);
    buf.writeByte(dataBusState);
    buf.writeUInt32(uint32_t(keyboard_row_select_mask));
    for (int i = 0; i < 16; i++)
      buf.writeByte(keyboard_matrix[i]);
    buf.writeByte(user_port_state);
  }

  void TED7360::saveState(Ep128Emu::File& f)
  {
    {
      Ep128Emu::File::Buffer  buf;
      this->saveState(buf);
      f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_TED_STATE, buf);
    }
    M7501::saveState(f);
  }

  void TED7360::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible Plus/4 snapshot format");
    }
    try {
      this->reset(true);
      // load saved state
      uint8_t   romBitmap = buf.readByte();
      ramSegments = buf.readByte();
      if (!(ramSegments == 1 || ramSegments == 2 || ramSegments == 4 ||
            ramSegments == 16 || ramSegments == 64))
        throw Ep128Emu::Exception("incompatible Plus/4 snapshot data");
      // load RAM segments
      setRAMSize(size_t(ramSegments) << 4);
      for (size_t i = 0x08; i <= 0xFF; i++) {
        if (segmentTable[i] != (uint8_t *) 0) {
          for (size_t j = 0; j < 16384; j++)
            segmentTable[i][j] = buf.readByte();
        }
      }
      // load ROM segments
      for (uint8_t i = 0x00; i < 0x08; i++) {
        if (!(romBitmap & (uint8_t(1) << i)))
          loadROM(int(i >> 1), int(i & 1) << 14, 0, (uint8_t *) 0);
        else {
          uint8_t tmp = 0;
          loadROM(int(i >> 1), int(i & 1) << 14, 1, &tmp);
          for (size_t j = 0; j < 16384; j++)
            segmentTable[i][j] = buf.readByte();
        }
      }
      // load I/O and TED registers
      ioRegister_0000 = buf.readByte();
      ioRegister_0001 = buf.readByte();
      for (uint8_t i = 0x00; i <= 0x1F; i++)
        tedRegisters[i] = buf.readByte();
      // load memory paging
      hannesRegister = buf.readByte();
      uint8_t romSelect_ = buf.readByte() & uint8_t(0x8F);
      // update internal registers according to the new RAM image loaded
      write_register_FD16(this, 0xFD16, hannesRegister);
      if (romSelect_ & uint8_t(0x80))
        write_register_FF3E(this, 0xFF3E, 0x00);
      else
        write_register_FF3F(this, 0xFF3F, 0x00);
      write_register_FDDx(this, uint16_t(0xFDD0) | uint16_t(romSelect_), 0x00);
      writeMemory(0x0000, ioRegister_0000);
      writeMemory(0x0001, ioRegister_0001);
      writeMemory(0xFF06, tedRegisters[0x06]);
      writeMemory(0xFF07, tedRegisters[0x07]);
      for (uint8_t i = 0x0A; i < 0x1A; i++)
        writeMemory(uint16_t(0xFF00) | uint16_t(i), tedRegisters[i]);
      // load remaining internal registers from snapshot data
      tedRegisterWriteMask = buf.readUInt32();
      cycle_count = buf.readUInt32();
      video_column = buf.readByte() & 0x7F;
      cycle_count = (cycle_count & 0xFFFFFFFEUL)
                    | (unsigned long) (video_column & uint8_t(0x01));
      video_line = int(buf.readUInt32() & 0x01FF);
      character_line = buf.readByte() & 7;
      character_position = int(buf.readUInt32() & 0x03FF);
      character_position_reload = int(buf.readUInt32() & 0x03FF);
      character_column = buf.readByte();
      character_column = (character_column < 39 ? character_column : 39);
      dma_position = int(buf.readUInt32() & 0x03FF);
      flash_state = buf.readBoolean();
      renderWindow = buf.readBoolean();
      dmaWindow = buf.readBoolean();
      bitmapFetchWindow = buf.readBoolean();
      displayWindow = buf.readBoolean();
      renderingDisplay = buf.readBoolean();
      displayActive = buf.readBoolean();
      horizontalBlanking = buf.readBoolean();
      verticalBlanking = buf.readBoolean();
      singleClockMode = buf.readBoolean();
      timer1_run = buf.readBoolean();
      timer2_run = buf.readBoolean();
      timer3_run = buf.readBoolean();
      timer1_state = int(buf.readUInt32() & 0xFFFF);
      timer1_reload_value = int(buf.readUInt32() & 0xFFFF);
      timer2_state = int(buf.readUInt32() & 0xFFFF);
      timer3_state = int(buf.readUInt32() & 0xFFFF);
      sound_channel_1_cnt = int(buf.readUInt32() & 0x03FF);
      sound_channel_2_cnt = int(buf.readUInt32() & 0x03FF);
      sound_channel_1_state = uint8_t(buf.readByte() == uint8_t(0) ? 0 : 1);
      sound_channel_2_state = uint8_t(buf.readByte() == uint8_t(0) ? 0 : 1);
      sound_channel_2_noise_state = buf.readByte();
      sound_channel_2_noise_output =
          uint8_t(buf.readByte() == uint8_t(0) ? 0 : 1);
      currentCharacter = buf.readByte();
      currentAttribute = buf.readByte();
      currentBitmap = buf.readByte();
      pixelBufReadPos = buf.readByte() & 0x3C;
      pixelBufWritePos = buf.readByte() & 0x38;
      dmaCycleCounter = buf.readByte();
      attributeDMAFlag = buf.readBoolean();
      savedCharacterLine = buf.readByte() & 7;
      savedVideoLine = int(buf.readUInt32() & 0x01FF);
      prvVideoInterruptState = buf.readBoolean();
      dataBusState = buf.readByte();
      keyboard_row_select_mask = int(buf.readUInt32() & 0xFFFF);
      for (int i = 0; i < 16; i++)
        keyboard_matrix[i] = buf.readByte();
      user_port_state = buf.readByte();
      selectRenderer();
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "Plus/4 snapshot data");
    }
    catch (...) {
      try {
        this->reset(true);
      }
      catch (...) {
      }
      throw;
    }
  }

  void TED7360::saveProgram(Ep128Emu::File::Buffer& buf)
  {
    uint16_t  startAddr, endAddr, len;
    startAddr = uint16_t(readMemoryCPU(0x002B))
                | (uint16_t(readMemoryCPU(0x002C)) << 8);
    endAddr = uint16_t(readMemoryCPU(0x002D))
              | (uint16_t(readMemoryCPU(0x002E)) << 8);
    len = (endAddr > startAddr ? (endAddr - startAddr) : uint16_t(0));
    buf.writeUInt32(startAddr);
    buf.writeUInt32(len);
    while (len) {
      buf.writeByte(readMemoryCPU(startAddr, true));
      startAddr = (startAddr + 1) & 0xFFFF;
      len--;
    }
  }

  void TED7360::saveProgram(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveProgram(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_PLUS4_PRG, buf);
  }

  void TED7360::saveProgram(const char *fileName)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Ep128Emu::Exception("invalid plus4 program file name");
    std::FILE *f = std::fopen(fileName, "wb");
    if (!f)
      throw Ep128Emu::Exception("error opening plus4 program file");
    uint16_t  startAddr, endAddr, len;
    startAddr = uint16_t(readMemoryCPU(0x002B))
                | (uint16_t(readMemoryCPU(0x002C)) << 8);
    endAddr = uint16_t(readMemoryCPU(0x002D))
              | (uint16_t(readMemoryCPU(0x002E)) << 8);
    len = (endAddr > startAddr ? (endAddr - startAddr) : uint16_t(0));
    bool  err = true;
    if (std::fputc(int(startAddr & 0xFF), f) != EOF) {
      if (std::fputc(int((startAddr >> 8) & 0xFF), f) != EOF) {
        while (len) {
          int   c = readMemoryCPU(startAddr, true);
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
      throw Ep128Emu::Exception("error writing plus4 program file "
                                "-- is the disk full ?");
  }

  void TED7360::loadProgram(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    uint32_t  addr = buf.readUInt32();
    uint32_t  len = buf.readUInt32();
    if (addr >= 0x00010000U)
      throw Ep128Emu::Exception("invalid start address in plus4 program data");
    if (len >= 0x00010000U ||
        size_t(len) != (buf.getDataSize() - buf.getPosition()))
      throw Ep128Emu::Exception("invalid plus4 program length");
#if 0
    writeMemory(0x002B, uint8_t(addr & 0xFF));
    writeMemory(0x002C, uint8_t((addr >> 8) & 0xFF));
#endif
    while (len) {
      writeMemory(uint16_t(addr), buf.readByte());
      addr = (addr + 1) & 0xFFFF;
      len--;
    }
    writeMemory(0x002D, uint8_t(addr & 0xFF));
    writeMemory(0x002E, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x002F, uint8_t(addr & 0xFF));
    writeMemory(0x0030, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x0031, uint8_t(addr & 0xFF));
    writeMemory(0x0032, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x0033, readMemoryCPU(0x0037));
    writeMemory(0x0034, readMemoryCPU(0x0038));
#if 0
    writeMemory(0x0035, readMemoryCPU(0x0037));
    writeMemory(0x0036, readMemoryCPU(0x0038));
#endif
    writeMemory(0x009D, uint8_t(addr & 0xFF));
    writeMemory(0x009E, uint8_t((addr >> 8) & 0xFF));
  }

  void TED7360::loadProgram(const char *fileName)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Ep128Emu::Exception("invalid plus4 program file name");
    std::FILE *f = std::fopen(fileName, "rb");
    if (!f)
      throw Ep128Emu::Exception("error opening plus4 program file");
    uint16_t  addr;
    int       c;
    c = std::fgetc(f);
    if (c == EOF)
      throw Ep128Emu::Exception("unexpected end of plus4 program file");
    addr = uint16_t(c & 0xFF);
    c = std::fgetc(f);
    if (c == EOF)
      throw Ep128Emu::Exception("unexpected end of plus4 program file");
    addr |= uint16_t((c & 0xFF) << 8);
#if 0
    writeMemory(0x002B, uint8_t(addr & 0xFF));
    writeMemory(0x002C, uint8_t((addr >> 8) & 0xFF));
#endif
    size_t  len = 0;
    while (true) {
      c = std::fgetc(f);
      if (c == EOF)
        break;
      if (++len > 0xFFFF) {
        std::fclose(f);
        throw Ep128Emu::Exception("plus4 program file has invalid length");
      }
      writeMemory(addr, uint8_t(c));
      addr = (addr + 1) & 0xFFFF;
    }
    std::fclose(f);
    writeMemory(0x002D, uint8_t(addr & 0xFF));
    writeMemory(0x002E, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x002F, uint8_t(addr & 0xFF));
    writeMemory(0x0030, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x0031, uint8_t(addr & 0xFF));
    writeMemory(0x0032, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x0033, readMemoryCPU(0x0037));
    writeMemory(0x0034, readMemoryCPU(0x0038));
#if 0
    writeMemory(0x0035, readMemoryCPU(0x0037));
    writeMemory(0x0036, readMemoryCPU(0x0038));
#endif
    writeMemory(0x009D, uint8_t(addr & 0xFF));
    writeMemory(0x009E, uint8_t((addr >> 8) & 0xFF));
  }

  void TED7360::registerChunkTypes(Ep128Emu::File& f)
  {
    ChunkType_TED7360Snapshot *p1 = (ChunkType_TED7360Snapshot *) 0;
    ChunkType_Plus4Program    *p2 = (ChunkType_Plus4Program *) 0;

    try {
      p1 = new ChunkType_TED7360Snapshot(*this);
      f.registerChunkType(p1);
      p1 = (ChunkType_TED7360Snapshot *) 0;
      p2 = new ChunkType_Plus4Program(*this);
      f.registerChunkType(p2);
      p2 = (ChunkType_Plus4Program *) 0;
    }
    catch (...) {
      if (p2)
        delete p2;
      if (p1)
        delete p1;
      throw;
    }
    M7501::registerChunkType(f);
  }

}       // namespace Plus4

