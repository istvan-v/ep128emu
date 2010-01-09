
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2010 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "z80/z80.hpp"
#include "zxmemory.hpp"
#include "ay3_8912.hpp"
#include "ula.hpp"
#include "vm.hpp"
#include "zx128vm.hpp"

namespace ZX128 {

  void ZX128VM::saveState(Ep128Emu::File& f)
  {
    memory.saveState(f);
    ula.saveState(f);
    ay3.saveState(f);
    z80.saveState(f);
    {
      Ep128Emu::File::Buffer  buf;
      buf.setPosition(0);
      buf.writeUInt32(0x01000000);      // version number
      buf.writeByte(spectrum128PageRegister);
      buf.writeByte(ayRegisterSelected);
      buf.writeByte(ayCycleCnt - 1);
      buf.writeByte(z80OpcodeHalfCycles);
      buf.writeUInt32(uint32_t(ulaFrequency));
      for (int i = 0; i < 16; i++)
        buf.writeByte(keyboardState[i]);
      f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_ZXVM_STATE, buf);
    }
  }

  void ZX128VM::saveMachineConfiguration(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    buf.writeUInt32(uint32_t(ulaFrequency));
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_ZXVM_CONFIG, buf);
  }

  void ZX128VM::recordDemo(Ep128Emu::File& f)
  {
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    ula.setTapeInput(0);
    stopDemo();
    resetKeyboard();
    z80.closeTapeFile();
    // save full snapshot, including timing and clock frequency settings
    saveMachineConfiguration(f);
    saveState(f);
    demoBuffer.clear();
    demoBuffer.writeUInt32(0x00020008); // version 2.0.8
    demoFile = &f;
    isRecordingDemo = true;
    setCallback(&demoRecordCallback, this, true);
    demoTimeCnt = 0U;
  }

  void ZX128VM::stopDemo()
  {
    stopDemoPlayback();
    stopDemoRecording(true);
  }

  bool ZX128VM::getIsRecordingDemo()
  {
    if (demoFile != (Ep128Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    return isRecordingDemo;
  }

  bool ZX128VM::getIsPlayingDemo() const
  {
    return isPlayingDemo;
  }

  // --------------------------------------------------------------------------

  void ZX128VM::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000U) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible zx128 snapshot version");
    }
    stopDemo();
    snapshotLoadFlag = true;
    z80.closeTapeFile();
    try {
      spectrum128PageRegister = buf.readByte();
      for (uint8_t i = 0x08; i < 0x80; i++)
        memory.deleteSegment(i);
      for (uint8_t i = 0x82; (i & 0xFF) != 0; i++)
        memory.deleteSegment(i);
      spectrum128Mode = memory.isSegmentRAM(0x07);
      initializeMemoryPaging();
      ayRegisterSelected = buf.readByte();
      ayCycleCnt = (buf.readByte() & 3) + 1;
      z80OpcodeHalfCycles = buf.readByte();
      (void) buf.readUInt32();          // ulaFrequency (ignored)
      for (int i = 0; i < 16; i++)
        keyboardState[i] = buf.readByte();
      convertKeyboardState();
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "zx128 snapshot data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void ZX128VM::loadMachineConfiguration(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000U) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible zx128 "
                                "machine configuration format");
    }
    try {
      setVideoFrequency(buf.readUInt32());
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "zx128 machine configuration data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void ZX128VM::loadDemo(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
#if 0
    if (version != 0x00020008) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible zx128 demo format");
    }
#endif
    (void) version;
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    ula.setTapeInput(0);
    stopDemo();
    resetKeyboard();
    // initialize time counter with first delta time
    demoTimeCnt = buf.readUIntVLen();
    isPlayingDemo = true;
    setCallback(&demoPlayCallback, this, true);
    // copy any remaining demo data to local buffer
    demoBuffer.clear();
    demoBuffer.writeData(buf.getData() + buf.getPosition(),
                         buf.getDataSize() - buf.getPosition());
    demoBuffer.setPosition(0);
  }

  // --------------------------------------------------------------------------

  void ZX128VM::loadSNAFile(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    uint8_t zx128Paging_ = 0x00;
    if (buf.getDataSize() == 49179) {
      if (spectrum128Mode || memory.getPage(3) == 0xFF)
        throw Ep128Emu::Exception("snapshot file requires Spectrum 48 mode");
    }
    else if (buf.getDataSize() == 131103 || buf.getDataSize() == 147487) {
      if (!spectrum128Mode)
        throw Ep128Emu::Exception("snapshot file requires Spectrum 128 mode");
      zx128Paging_ = *(buf.getData() + 49181);
    }
    else {
      throw Ep128Emu::Exception("invalid SNA data size");
    }
    stopDemo();
    this->reset(true);
    snapshotLoadFlag = true;
    try {
      Ep128::Z80_REGISTERS& r = z80.getReg();
      r.I = buf.readByte();
      r.altHL.B.l = buf.readByte();
      r.altHL.B.h = buf.readByte();
      r.altDE.B.l = buf.readByte();
      r.altDE.B.h = buf.readByte();
      r.altBC.B.l = buf.readByte();
      r.altBC.B.h = buf.readByte();
      r.altAF.B.l = buf.readByte();
      r.altAF.B.h = buf.readByte();
      r.HL.B.l = buf.readByte();
      r.HL.B.h = buf.readByte();
      r.DE.B.l = buf.readByte();
      r.DE.B.h = buf.readByte();
      r.BC.B.l = buf.readByte();
      r.BC.B.h = buf.readByte();
      r.IY.B.l = buf.readByte();
      r.IY.B.h = buf.readByte();
      r.IX.B.l = buf.readByte();
      r.IX.B.h = buf.readByte();
      uint8_t tmp = buf.readByte();
      if (!(tmp & 0x04)) {
        r.IFF1 = 0;
        r.IFF2 = 0;
      }
      else {
        r.IFF1 = 1;
        r.IFF2 = 1;
      }
      r.R = buf.readByte();
      r.AF.B.l = buf.readByte();
      r.AF.B.h = buf.readByte();
      r.SP.B.l = buf.readByte();
      r.SP.B.h = buf.readByte();
      tmp = buf.readByte();
      if (tmp > 2)
        throw Ep128Emu::Exception("error in SNA file data");
      r.IM = tmp;
      ula.writePort(buf.readByte() & 0x07);
      ZX128VM::ioPortWriteCallback((void *) this, 0x7FFD, zx128Paging_);
      for (uint32_t i = 0x4000U; i <= 0xFFFFU; i++)
        writeMemory(i, buf.readByte(), true);
      if (!spectrum128Mode) {
        r.PC.B.l = readMemory(r.SP.W, true);
        r.SP.W = (r.SP.W + 1) & 0xFFFF;
        r.PC.B.h = readMemory(r.SP.W, true);
        r.SP.W = (r.SP.W + 1) & 0xFFFF;
      }
      else {
        r.PC.B.l = buf.readByte();
        r.PC.B.h = buf.readByte();
        (void) buf.readByte();          // Spectrum 128 page register
        (void) buf.readByte();          // TR-DOS ROM paging (ignored)
        for (int i = 0; i < 8; i++) {
          if (i != 5 && i != 2 && i != int(zx128Paging_ & 0x07)) {
            for (uint32_t j = 0x0000U; j <= 0x3FFFU; j++)
              writeMemory((uint32_t(i) << 14) | j, buf.readByte(), false);
          }
        }
      }
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of .SNA data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  // --------------------------------------------------------------------------

  void ZX128VM::loadZ80FileRAMData(Ep128Emu::File::Buffer& buf,
                                   uint32_t addr, size_t nBytes)
  {
    if (nBytes >= 0xC000) {     // nBytes == 0xC000: 48K uncompressed (V1) data
      if (nBytes == 0xFFFF)     // nBytes == 0xFFFF: 16K uncompressed (V2) data
        nBytes = 0x4000;
      for ( ; nBytes > 0; nBytes--) {
        writeMemory(addr, buf.readByte(), false);
        addr = (addr + 1U) & 0x003FFFFFU;
      }
    }
    else {                      // nBytes == 0x0000: V1 compressed data
      size_t  bufStartPos = buf.getPosition();
      while (true) {            // any other value is V2 compressed data size
        uint8_t c = buf.readByte();
        if (c == 0x00 && nBytes == 0) {
          size_t  bufPos = buf.getPosition();
          if (buf.readByte() == 0xED) {
            if (buf.readUInt16() == 0xED00)
              break;
          }
          buf.setPosition(bufPos);
        }
        uint8_t cnt = 1;
        if (c == 0xED) {
          size_t  bufPos = buf.getPosition();
          if (buf.readByte() == 0xED) {
            cnt = buf.readByte();
            c = buf.readByte();
          }
          else {
            buf.setPosition(bufPos);
          }
        }
        for ( ; cnt > 0; cnt--) {
          writeMemory(addr, c, false);
          addr = (addr + 1U) & 0x003FFFFFU;
        }
        if (nBytes > 0) {
          if (buf.getPosition() >= (bufStartPos + nBytes)) {
            if (buf.getPosition() == (bufStartPos + nBytes))
              break;
            throw Ep128Emu::Exception("error in Z80 file data");
          }
        }
      }
    }
    if ((addr & 0x3FFFU) != 0U)
      throw Ep128Emu::Exception("error in Z80 file data");
  }

  void ZX128VM::loadZ80File(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    stopDemo();
    this->reset(true);
    snapshotLoadFlag = true;
    try {
      Ep128::Z80_REGISTERS& r = z80.getReg();
      r.AF.W = buf.readUInt16();
      r.BC.B.l = buf.readByte();
      r.BC.B.h = buf.readByte();
      r.HL.B.l = buf.readByte();
      r.HL.B.h = buf.readByte();
      r.PC.B.l = buf.readByte();
      r.PC.B.h = buf.readByte();
      r.SP.B.l = buf.readByte();
      r.SP.B.h = buf.readByte();
      r.I = buf.readByte();
      r.R = buf.readByte() & 0x7F;
      uint8_t tmp = buf.readByte();
      if (tmp == 0xFF)
        tmp = 0x01;             // for compatibility
      r.R = r.R | ((tmp & 0x01) << 7);
      ula.writePort((tmp & 0x0E) >> 1);
      bool    compressionFlag = bool(tmp & 0x20);
      r.DE.B.l = buf.readByte();
      r.DE.B.h = buf.readByte();
      r.altBC.B.l = buf.readByte();
      r.altBC.B.h = buf.readByte();
      r.altDE.B.l = buf.readByte();
      r.altDE.B.h = buf.readByte();
      r.altHL.B.l = buf.readByte();
      r.altHL.B.h = buf.readByte();
      r.altAF.W = buf.readUInt16();
      r.IY.B.l = buf.readByte();
      r.IY.B.h = buf.readByte();
      r.IX.B.l = buf.readByte();
      r.IX.B.h = buf.readByte();
      r.IFF1 = Ep128::Z80_BYTE(bool(buf.readByte()));
      r.IFF2 = Ep128::Z80_BYTE(bool(buf.readByte()));
      tmp = buf.readByte();
      if ((tmp & 0x03) > 2)
        throw Ep128Emu::Exception("error in Z80 file data");
      r.IM = tmp & 0x03;
      if (r.PC.W.l != 0x0000) {         // version 1 file format (48K only)
        if (spectrum128Mode || memory.getPage(3) == 0xFF)
          throw Ep128Emu::Exception("snapshot file requires Spectrum 48 mode");
        loadZ80FileRAMData(buf, 0U, (compressionFlag ? 0U : 0xC000U));
      }
      else {                            // version 2 or 3 file format
        uint16_t  hdrLen = buf.readByte();
        hdrLen = hdrLen | (uint16_t(buf.readByte()) << 8);
        if (!(hdrLen == 23 || hdrLen == 54 || hdrLen == 55))
          throw Ep128Emu::Exception("unsupported Z80 file format version");
        r.PC.B.l = buf.readByte();
        r.PC.B.h = buf.readByte();
        tmp = buf.readByte();
        uint8_t   machineType = 48;
        if (((tmp - (uint8_t(hdrLen > 23) + 0x03)) & 0xFF) < 0x02)
          machineType = 128;
        else if (tmp >= 0x02)
          throw Ep128Emu::Exception("unsupported machine type in Z80 file");
        tmp = buf.readByte();
        ZX128VM::ioPortWriteCallback((void *) this, 0x7FFD, tmp);
        (void) buf.readByte();
        tmp = buf.readByte();
        if ((tmp & 0x80) != 0 && machineType == 48)
          machineType = 16;
        if (machineType == 128 && !spectrum128Mode)
          throw Ep128Emu::Exception("snapshot file requires Spectrum 128 mode");
        if (machineType == 48 && (spectrum128Mode || memory.getPage(3) == 0xFF))
          throw Ep128Emu::Exception("snapshot file requires Spectrum 48 mode");
        if (machineType == 16 && (spectrum128Mode || memory.getPage(2) != 0xFF))
          throw Ep128Emu::Exception("snapshot file requires Spectrum 16 mode");
        ZX128VM::ioPortWriteCallback((void *) this, 0xFFFD, buf.readByte());
        for (uint8_t i = 0; i < 16; i++) {
          tmp = buf.readByte();
          if (spectrum128Mode)
            ay3.writeRegister(i, tmp);
        }
        if (hdrLen > 23) {
          size_t  tStateCnt = buf.readByte();
          tStateCnt = tStateCnt | (size_t(buf.readByte()) << 8);
          tStateCnt =
              tStateCnt + (size_t(buf.readByte())
                           * size_t(spectrum128Mode ? (57 * 311) : (56 * 312)));
          ula.setVideoPosition(tStateCnt);
          for (uint16_t i = 26; i < hdrLen; i++)
            (void) buf.readByte();      // ignore any remaining V3 header data
        }
        while (buf.getPosition() < buf.getDataSize()) {
          uint16_t  nBytes = buf.readByte();
          nBytes = nBytes | (uint16_t(buf.readByte()) << 8);
          uint8_t   pageNum = buf.readByte();
          if (machineType == 128 && pageNum >= 3 && pageNum < 11) {
            pageNum = pageNum - 3;
          }
          else if (machineType == 48 &&
                   (pageNum == 4 || pageNum == 5 || pageNum == 8)) {
            pageNum = uint8_t((pageNum + 2) % 5);
          }
          else if (machineType == 16 && pageNum == 8) {
            pageNum = 0x00;
          }
          else {
            pageNum = 0xC0;             // invalid page (should be an error ?)
          }
          loadZ80FileRAMData(buf, uint32_t(pageNum) << 14, nBytes);
        }
      }
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of .Z80 data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  // --------------------------------------------------------------------------

  class ChunkType_ZX128VMConfig : public Ep128Emu::File::ChunkTypeHandler {
   private:
    ZX128VM&  ref;
   public:
    ChunkType_ZX128VMConfig(ZX128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_ZX128VMConfig()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_ZXVM_CONFIG;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadMachineConfiguration(buf);
    }
  };

  class ChunkType_ZX128VMSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    ZX128VM&  ref;
   public:
    ChunkType_ZX128VMSnapshot(ZX128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_ZX128VMSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_ZXVM_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  class ChunkType_DemoStream : public Ep128Emu::File::ChunkTypeHandler {
   private:
    ZX128VM&  ref;
   public:
    ChunkType_DemoStream(ZX128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_DemoStream()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_ZX_DEMO;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadDemo(buf);
    }
  };

  class ChunkType_ZX128SNAFile : public Ep128Emu::File::ChunkTypeHandler {
   private:
    ZX128VM&  ref;
   public:
    ChunkType_ZX128SNAFile(ZX128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_ZX128SNAFile()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_ZX_SNA_FILE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadSNAFile(buf);
    }
  };

  class ChunkType_ZX128Z80File : public Ep128Emu::File::ChunkTypeHandler {
   private:
    ZX128VM&  ref;
   public:
    ChunkType_ZX128Z80File(ZX128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_ZX128Z80File()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_ZX_Z80_FILE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadZ80File(buf);
    }
  };

  void ZX128VM::registerChunkTypes(Ep128Emu::File& f)
  {
    ChunkType_ZX128VMConfig   *p1 = (ChunkType_ZX128VMConfig *) 0;
    ChunkType_ZX128VMSnapshot *p2 = (ChunkType_ZX128VMSnapshot *) 0;
    ChunkType_DemoStream      *p3 = (ChunkType_DemoStream *) 0;
    ChunkType_ZX128SNAFile    *p4 = (ChunkType_ZX128SNAFile *) 0;
    ChunkType_ZX128Z80File    *p5 = (ChunkType_ZX128Z80File *) 0;

    try {
      p1 = new ChunkType_ZX128VMConfig(*this);
      f.registerChunkType(p1);
      p1 = (ChunkType_ZX128VMConfig *) 0;
      p2 = new ChunkType_ZX128VMSnapshot(*this);
      f.registerChunkType(p2);
      p2 = (ChunkType_ZX128VMSnapshot *) 0;
      p3 = new ChunkType_DemoStream(*this);
      f.registerChunkType(p3);
      p3 = (ChunkType_DemoStream *) 0;
      p4 = new ChunkType_ZX128SNAFile(*this);
      f.registerChunkType(p4);
      p4 = (ChunkType_ZX128SNAFile *) 0;
      p5 = new ChunkType_ZX128Z80File(*this);
      f.registerChunkType(p5);
      p5 = (ChunkType_ZX128Z80File *) 0;
    }
    catch (...) {
      if (p1)
        delete p1;
      if (p2)
        delete p2;
      if (p3)
        delete p3;
      if (p4)
        delete p4;
      if (p5)
        delete p5;
      throw;
    }
    z80.registerChunkType(f);
    memory.registerChunkType(f);
    ay3.registerChunkType(f);
    ula.registerChunkType(f);
  }

}       // namespace ZX128

