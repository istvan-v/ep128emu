
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

