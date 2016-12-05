
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/ep128emu/
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
#include "tvc64vm.hpp"
#ifdef ENABLE_SDEXT
#  include "sdext.hpp"
#endif

namespace TVC64 {

  void TVC64VM::saveState(Ep128Emu::File& f)
  {
    memory.saveState(f);
    ioPorts.saveState(f);
    crtc.saveState(f);
    z80.saveState(f);
#ifdef ENABLE_SDEXT
    sdext.saveState(f);
#endif
    {
      Ep128Emu::File::Buffer  buf;
      buf.setPosition(0);
#ifndef ENABLE_SDEXT
      buf.writeUInt32(0x01000001);      // version number
#else
      buf.writeUInt32(0x01010001);      // bit 16 is set if SDExt is included
#endif
      for (uint8_t i = 0; i <= 4; i++)
        buf.writeByte(videoRenderer.getColor(i));
      buf.writeByte(videoRenderer.getVideoMode());
      buf.writeByte(z80OpcodeHalfCycles);
      buf.writeByte(tapeOutputSignal);
      buf.writeByte(crtcRegisterSelected);
      buf.writeByte(irqState);
      buf.writeByte(irqEnableMask);
      buf.writeByte(keyboardRow);
      buf.writeUInt32(toneGenCnt1);
      buf.writeUInt32(toneGenFreq);
      buf.writeByte(toneGenCnt2);
      buf.writeBoolean(toneGenEnabled);
      buf.writeByte(audioOutputLevel);
      buf.writeByte(vtdosROMPage);
      buf.writeUInt32(uint32_t(crtcFrequency));
      for (int i = 0; i < 16; i++)
        buf.writeByte(keyboardState[i]);
      f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_TVCVM_STATE, buf);
    }
  }

  void TVC64VM::saveMachineConfiguration(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    buf.writeUInt32(uint32_t(crtcFrequency));
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_TVCVM_CONFIG, buf);
  }

  void TVC64VM::recordDemo(Ep128Emu::File& f)
  {
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    tapeInputSignal = 0;
    stopDemo();
    resetKeyboard();
    // floppy and SD emulation are disabled while recording or playing demo
    resetFloppyDrives(false);
#ifdef ENABLE_SDEXT
    // FIXME: implement a better way of disabling SDExt during demo recording
    sdext.openImage((char *) 0);
#endif
    // save full snapshot, including timing and clock frequency settings
    saveMachineConfiguration(f);
    saveState(f);
    demoBuffer.clear();
    demoBuffer.writeUInt32(0x0002000B); // version 2.0.11
    demoFile = &f;
    isRecordingDemo = true;
    setCallback(&demoRecordCallback, this, true);
    demoTimeCnt = 0U;
  }

  void TVC64VM::stopDemo()
  {
#ifdef ENABLE_SDEXT
    // TODO: re-enable SDExt after stopping demo recording or playback
#endif
    stopDemoPlayback();
    stopDemoRecording(true);
  }

  bool TVC64VM::getIsRecordingDemo()
  {
    if (demoFile != (Ep128Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    return isRecordingDemo;
  }

  bool TVC64VM::getIsPlayingDemo() const
  {
    return isPlayingDemo;
  }

  // --------------------------------------------------------------------------

  void TVC64VM::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (!(version >= 0x01000000U && (version & 0xFFFEFFFFU) <= 0x01000001U)) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible TVC snapshot version");
    }
    stopDemo();
    snapshotLoadFlag = true;
    // reset floppy and SD card emulation, as the state of these is not saved
    resetFloppyDrives(true);
    try {
      z80.triggerInterrupt();
      videoRenderer.setVideoMemory(memory.getVideoMemory());
#ifdef ENABLE_SDEXT
      if (!(version & 0x00010000)) {
        // reset and disable SDExt if the snapshot is from an old version
        sdext.reset(2);
        sdext.setEnabled(false);
        sdext.openROMFile((char *) 0);
      }
#endif
      memory.setEnableSDExt();
      version = version & 0xFFFEFFFFU;
      for (uint8_t i = 0; i <= 4; i++)
        videoRenderer.setColor(i, buf.readByte());
      videoRenderer.setVideoMode(buf.readByte());
      z80OpcodeHalfCycles = buf.readByte();
      tapeOutputSignal = buf.readByte() & 0x01;
      crtcRegisterSelected = buf.readByte() & 0x1F;
      irqState = buf.readByte() & 0x1F;
      irqEnableMask = buf.readByte() & 0x1F;
      keyboardRow = buf.readByte() & 0x0F;
      toneGenCnt1 = buf.readUInt32() & 0x0FFFU;
      toneGenFreq = buf.readUInt32() & 0x0FFFU;
      toneGenCnt2 = buf.readByte() & 0x0F;
      toneGenEnabled = buf.readBoolean();
      audioOutputLevel = buf.readByte() & 0x0F;
      vtdosROMPage = buf.readByte() & 0x03;
      setTapeMotorState(bool(ioPorts.getLastValueWritten(0x05) & 0xC0));
      (void) buf.readUInt32();          // crtcFrequency (ignored)
      for (int i = 0; i < 16; i++)
        keyboardState[i] = buf.readByte();
      convertKeyboardState();
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "TVC snapshot data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void TVC64VM::loadMachineConfiguration(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000U) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible TVC "
                                "machine configuration format");
    }
    try {
      setVideoFrequency(buf.readUInt32());
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "TVC machine configuration data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void TVC64VM::loadDemo(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
#if 0
    if (version != 0x0002000B) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible TVC demo format");
    }
#endif
    (void) version;
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    tapeInputSignal = 0;
    stopDemo();
    resetKeyboard();
    // floppy and SD emulation are disabled while recording or playing demo
    resetFloppyDrives(false);
#ifdef ENABLE_SDEXT
    // FIXME: implement a better way of disabling SDExt during demo playback
    sdext.openImage((char *) 0);
#endif
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

  class ChunkType_TVC64VMConfig : public Ep128Emu::File::ChunkTypeHandler {
   private:
    TVC64VM&  ref;
   public:
    ChunkType_TVC64VMConfig(TVC64VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_TVC64VMConfig()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_TVCVM_CONFIG;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadMachineConfiguration(buf);
    }
  };

  class ChunkType_TVC64VMSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    TVC64VM&  ref;
   public:
    ChunkType_TVC64VMSnapshot(TVC64VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_TVC64VMSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_TVCVM_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  class ChunkType_DemoStream : public Ep128Emu::File::ChunkTypeHandler {
   private:
    TVC64VM&  ref;
   public:
    ChunkType_DemoStream(TVC64VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_DemoStream()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_TVC_DEMO;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadDemo(buf);
    }
  };

  void TVC64VM::registerChunkTypes(Ep128Emu::File& f)
  {
    ChunkType_TVC64VMConfig   *p1 = (ChunkType_TVC64VMConfig *) 0;
    ChunkType_TVC64VMSnapshot *p2 = (ChunkType_TVC64VMSnapshot *) 0;
    ChunkType_DemoStream      *p3 = (ChunkType_DemoStream *) 0;

    try {
      p1 = new ChunkType_TVC64VMConfig(*this);
      f.registerChunkType(p1);
      p1 = (ChunkType_TVC64VMConfig *) 0;
      p2 = new ChunkType_TVC64VMSnapshot(*this);
      f.registerChunkType(p2);
      p2 = (ChunkType_TVC64VMSnapshot *) 0;
      p3 = new ChunkType_DemoStream(*this);
      f.registerChunkType(p3);
      p3 = (ChunkType_DemoStream *) 0;
    }
    catch (...) {
      if (p1)
        delete p1;
      if (p2)
        delete p2;
      if (p3)
        delete p3;
      throw;
    }
    z80.registerChunkType(f);
    memory.registerChunkType(f);
    ioPorts.registerChunkType(f);
    crtc.registerChunkType(f);
  }

}       // namespace TVC64

