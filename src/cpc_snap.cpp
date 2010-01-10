
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
#include "cpc464vm.hpp"

namespace CPC464 {

  void CPC464VM::saveState(Ep128Emu::File& f)
  {
    memory.saveState(f);
    crtc.saveState(f);
    ay3.saveState(f);
    z80.saveState(f);
    {
      Ep128Emu::File::Buffer  buf;
      buf.setPosition(0);
      buf.writeUInt32(0x01000000);      // version number
      for (uint8_t i = 0; i <= 16; i++)
        buf.writeByte(videoRenderer.getColor(i));
      buf.writeByte(videoRenderer.getVideoMode());
      buf.writeByte(ayRegisterSelected);
      buf.writeByte(ayCycleCnt - 1);
      buf.writeByte(z80OpcodeHalfCycles);
      buf.writeByte(ppiPortARegister);
      buf.writeByte(ppiPortBRegister);
      buf.writeByte(ppiPortCRegister);
      buf.writeByte(ppiControlRegister);
      buf.writeByte(crtcRegisterSelected);
      buf.writeByte(gateArrayIRQCounter);
      buf.writeByte(gateArrayVSyncDelay);
      buf.writeByte(gateArrayPenSelected);
      buf.writeUInt32(uint32_t(crtcFrequency));
      for (int i = 0; i < 16; i++)
        buf.writeByte(keyboardState[i]);
      f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_CPCVM_STATE, buf);
    }
  }

  void CPC464VM::saveMachineConfiguration(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    buf.writeUInt32(uint32_t(crtcFrequency));
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_CPCVM_CONFIG, buf);
  }

  void CPC464VM::recordDemo(Ep128Emu::File& f)
  {
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    tapeInputSignal = 0;
    updatePPIState();
    stopDemo();
    resetKeyboard();
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

  void CPC464VM::stopDemo()
  {
    stopDemoPlayback();
    stopDemoRecording(true);
  }

  bool CPC464VM::getIsRecordingDemo()
  {
    if (demoFile != (Ep128Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    return isRecordingDemo;
  }

  bool CPC464VM::getIsPlayingDemo() const
  {
    return isPlayingDemo;
  }

  // --------------------------------------------------------------------------

  void CPC464VM::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000U) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible cpc464 snapshot version");
    }
    stopDemo();
    snapshotLoadFlag = true;
    try {
      for (uint8_t i = 0; i <= 16; i++)
        videoRenderer.setColor(i, buf.readByte());
      videoRenderer.setVideoMode(buf.readByte());
      ayRegisterSelected = buf.readByte();
      ayCycleCnt = (buf.readByte() & 7) + 1;
      z80OpcodeHalfCycles = buf.readByte();
      ppiPortARegister = buf.readByte();
      ppiPortBRegister = buf.readByte();
      ppiPortCRegister = buf.readByte();
      ppiControlRegister = buf.readByte() | 0x80;
      updatePPIState();
      crtcRegisterSelected = buf.readByte();
      gateArrayIRQCounter = buf.readByte() & 0x3F;
      gateArrayVSyncDelay = buf.readByte() & 0x03;
      gateArrayPenSelected = buf.readByte() & 0x3F;
      (void) buf.readUInt32();          // crtcFrequency (ignored)
      for (int i = 0; i < 16; i++)
        keyboardState[i] = buf.readByte();
      convertKeyboardState();
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "cpc464 snapshot data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void CPC464VM::loadMachineConfiguration(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000U) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible cpc464 "
                                "machine configuration format");
    }
    try {
      setVideoFrequency(buf.readUInt32());
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "cpc464 machine configuration data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void CPC464VM::loadDemo(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
#if 0
    if (version != 0x00020008) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible cpc464 demo format");
    }
#endif
    (void) version;
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    tapeInputSignal = 0;
    updatePPIState();
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

  void CPC464VM::loadSNAFile(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    stopDemo();
    this->reset(true);
    snapshotLoadFlag = true;
    try {
      uint8_t version = buf.readByte();
      Ep128::Z80_REGISTERS& r = z80.getReg();
      r.AF.B.l = buf.readByte();
      r.AF.B.h = buf.readByte();
      r.BC.B.l = buf.readByte();
      r.BC.B.h = buf.readByte();
      r.DE.B.l = buf.readByte();
      r.DE.B.h = buf.readByte();
      r.HL.B.l = buf.readByte();
      r.HL.B.h = buf.readByte();
      r.R = buf.readByte();
      r.I = buf.readByte();
      r.IFF1 = buf.readByte() & 0x01;
      r.IFF2 = buf.readByte() & 0x01;
      r.IX.B.l = buf.readByte();
      r.IX.B.h = buf.readByte();
      r.IY.B.l = buf.readByte();
      r.IY.B.h = buf.readByte();
      r.SP.B.l = buf.readByte();
      r.SP.B.h = buf.readByte();
      r.PC.B.l = buf.readByte();
      r.PC.B.h = buf.readByte();
      r.IM = buf.readByte() & 0x03;
      if (r.IM > 2)
        throw Ep128Emu::Exception("error in CPC snapshot data");
      r.altAF.B.l = buf.readByte();
      r.altAF.B.h = buf.readByte();
      r.altBC.B.l = buf.readByte();
      r.altBC.B.h = buf.readByte();
      r.altDE.B.l = buf.readByte();
      r.altDE.B.h = buf.readByte();
      r.altHL.B.l = buf.readByte();
      r.altHL.B.h = buf.readByte();
      gateArrayPenSelected = buf.readByte() & 0x3F;
      for (uint8_t i = 0; i <= 16; i++)
        videoRenderer.setColor(i, buf.readByte());
      uint8_t   tmp = buf.readByte();   // video mode and ROM disable
      videoRenderer.setVideoMode(tmp & 0x03);
      uint16_t  memConfig = uint16_t((~tmp) & 0x0C) << 4;
      tmp = buf.readByte();             // RAM configuration
      memConfig = memConfig | uint16_t(tmp & 0x3F);
      crtcRegisterSelected = buf.readByte();
      for (uint8_t i = 0; i < 16; i++)
        crtc.writeRegister(i, buf.readByte());
      crtc.setLightPenPosition(buf.readUInt16());
      tmp = buf.readByte();             // ROM bank selected
      memConfig = memConfig | (uint16_t(tmp) << 8);
      memory.setPaging(memConfig);
      ppiPortARegister = buf.readByte();
      ppiPortBRegister = buf.readByte();
      ppiPortCRegister = buf.readByte();
      ppiControlRegister = buf.readByte();
      ayRegisterSelected = buf.readByte();
      for (uint8_t i = 0; i < 16; i++)
        ay3.writeRegister(i, buf.readByte());
      updatePPIState();
      uint16_t  ramSize = buf.readByte();
      ramSize = ramSize | (uint16_t(buf.readByte()) << 8);
      if (ramSize > 576 || ((ramSize - 64) & (ramSize - 128)) != 0)
        throw Ep128Emu::Exception("invalid RAM size in CPC snapshot");
      if (version >= 2) {
        uint8_t machineType = buf.readByte();
        if (machineType >= 4)
          throw Ep128Emu::Exception("unsupported machine type in CPC snapshot");
        if (machineType < 3) {
          if (machineType == 0 && memory.readRaw(0x00200006U) != 0x80)
            throw Ep128Emu::Exception("CPC snapshot requires CPC464 ROM");
          if (machineType == 1 && memory.readRaw(0x00200006U) != 0x7B)
            throw Ep128Emu::Exception("CPC snapshot requires CPC664 ROM");
          if (machineType == 2 && memory.readRaw(0x00200006U) != 0x91)
            throw Ep128Emu::Exception("CPC snapshot requires CPC6128 ROM");
          if (!(memory.isSegmentRAM(uint8_t(ramSize >> 4) - 1) &&
                !memory.isSegmentRAM(uint8_t(ramSize >> 4)))) {
            memory.setRAMSize(ramSize);
          }
        }
      }
      if (!(memory.isSegmentRAM(uint8_t(ramSize >> 4) - 1) &&
            !memory.isSegmentRAM(uint8_t(ramSize >> 4)))) {
        if (ramSize == 64)
          throw Ep128Emu::Exception("CPC snapshot requires 64K RAM size");
        else if (ramSize == 128)
          throw Ep128Emu::Exception("CPC snapshot requires 128K RAM size");
        else if (ramSize == 192)
          throw Ep128Emu::Exception("CPC snapshot requires 192K RAM size");
        else if (ramSize == 320)
          throw Ep128Emu::Exception("CPC snapshot requires 320K RAM size");
        else
          throw Ep128Emu::Exception("CPC snapshot requires 576K RAM size");
      }
      while (buf.getPosition() < 0xF0)
        (void) buf.readByte();
      for (uint32_t i = 0U; i < (uint32_t(ramSize) << 10); i++)
        memory.writeRaw(i, buf.readByte());
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  // --------------------------------------------------------------------------

  class ChunkType_CPC464VMConfig : public Ep128Emu::File::ChunkTypeHandler {
   private:
    CPC464VM&   ref;
   public:
    ChunkType_CPC464VMConfig(CPC464VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_CPC464VMConfig()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_CPCVM_CONFIG;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadMachineConfiguration(buf);
    }
  };

  class ChunkType_CPC464VMSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    CPC464VM&   ref;
   public:
    ChunkType_CPC464VMSnapshot(CPC464VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_CPC464VMSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_CPCVM_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  class ChunkType_DemoStream : public Ep128Emu::File::ChunkTypeHandler {
   private:
    CPC464VM&   ref;
   public:
    ChunkType_DemoStream(CPC464VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_DemoStream()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_CPC_DEMO;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadDemo(buf);
    }
  };

  class ChunkType_CPC464SNAFile : public Ep128Emu::File::ChunkTypeHandler {
   private:
    CPC464VM&   ref;
   public:
    ChunkType_CPC464SNAFile(CPC464VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_CPC464SNAFile()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_CPC_SNA_FILE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadSNAFile(buf);
    }
  };

  void CPC464VM::registerChunkTypes(Ep128Emu::File& f)
  {
    ChunkType_CPC464VMConfig    *p1 = (ChunkType_CPC464VMConfig *) 0;
    ChunkType_CPC464VMSnapshot  *p2 = (ChunkType_CPC464VMSnapshot *) 0;
    ChunkType_DemoStream        *p3 = (ChunkType_DemoStream *) 0;
    ChunkType_CPC464SNAFile     *p4 = (ChunkType_CPC464SNAFile *) 0;

    try {
      p1 = new ChunkType_CPC464VMConfig(*this);
      f.registerChunkType(p1);
      p1 = (ChunkType_CPC464VMConfig *) 0;
      p2 = new ChunkType_CPC464VMSnapshot(*this);
      f.registerChunkType(p2);
      p2 = (ChunkType_CPC464VMSnapshot *) 0;
      p3 = new ChunkType_DemoStream(*this);
      f.registerChunkType(p3);
      p3 = (ChunkType_DemoStream *) 0;
      p4 = new ChunkType_CPC464SNAFile(*this);
      f.registerChunkType(p4);
      p4 = (ChunkType_CPC464SNAFile *) 0;
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
      throw;
    }
    z80.registerChunkType(f);
    memory.registerChunkType(f);
    ay3.registerChunkType(f);
    crtc.registerChunkType(f);
  }

}       // namespace CPC464

