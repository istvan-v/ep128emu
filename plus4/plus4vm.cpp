
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
#include "display.hpp"
#include "soundio.hpp"
#include "plus4vm.hpp"

#include <cstdio>
#include <vector>

static void writeDemoTimeCnt(Ep128Emu::File::Buffer& buf, uint64_t n)
{
  uint64_t  mask = uint64_t(0x7F) << 49;
  uint8_t   rshift = 49;
  while (rshift != 0 && !(n & mask)) {
    mask >>= 7;
    rshift -= 7;
  }
  while (rshift != 0) {
    buf.writeByte(uint8_t((n & mask) >> rshift) | 0x80);
    mask >>= 7;
    rshift -= 7;
  }
  buf.writeByte(uint8_t(n) & 0x7F);
}

static uint64_t readDemoTimeCnt(Ep128Emu::File::Buffer& buf)
{
  uint64_t  n = 0U;
  uint8_t   i = 8, c;
  do {
    c = buf.readByte();
    n = (n << 7) | uint64_t(c & 0x7F);
    i--;
  } while ((c & 0x80) != 0 && i != 0);
  return n;
}

namespace Plus4 {

  Plus4VM::TED7360_::TED7360_(Plus4VM& vm_)
    : TED7360(),
      vm(vm_),
      lineCnt_(0)
  {
  }

  Plus4VM::TED7360_::~TED7360_()
  {
  }

  void Plus4VM::TED7360_::playSample(int16_t sampleValue)
  {
    vm.sendAudioOutput(uint16_t(sampleValue & 0x7FFF),
                       uint16_t(sampleValue & 0x7FFF));
  }

  void Plus4VM::TED7360_::drawLine(const uint8_t *buf, int nPixels)
  {
    if (vm.getIsDisplayEnabled() && lineCnt_ < 500) {
      uint8_t tmpBuf[414];      // 9 * 46
      uint8_t *bufp = &(tmpBuf[0]);
      int     i;
      for (i = 44; i < 412 && i < nPixels; i++) {
        if ((i & 7) == 4)
          *(bufp++) = 0x08;
        *(bufp++) = buf[i];
      }
      for ( ; i < 412; i++) {
        if ((i & 7) == 4)
          *(bufp++) = 0x08;
        *(bufp++) = 0x00;
      }
      vm.display.drawLine(&(tmpBuf[0]), 414);
      if (++lineCnt_ == 8)
        vm.display.vsyncStateChange(false, 28);
    }
  }

  void Plus4VM::TED7360_::verticalSync()
  {
    if (lineCnt_ >= 100) {
      lineCnt_ = 0;
      if (vm.getIsDisplayEnabled())
        vm.display.vsyncStateChange(true, 8);
    }
  }

  // --------------------------------------------------------------------------

  void Plus4VM::stopDemoPlayback()
  {
    if (isPlayingDemo) {
      isPlayingDemo = false;
      demoTimeCnt = 0U;
      demoBuffer.clear();
      // clear keyboard state at end of playback
      for (int i = 0; i < 128; i++)
        ted->setKeyState(i, false);
    }
  }

  void Plus4VM::stopDemoRecording(bool writeFile_)
  {
    isRecordingDemo = false;
    if (writeFile_ && demoFile != (Ep128Emu::File *) 0) {
      try {
        // put end of demo event
        writeDemoTimeCnt(demoBuffer, demoTimeCnt);
        demoTimeCnt = 0U;
        demoBuffer.writeByte(0x00);
        demoBuffer.writeByte(0x00);
        demoFile->addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_PLUS4_DEMO,
                           demoBuffer);
      }
      catch (...) {
        demoFile = (Ep128Emu::File *) 0;
        demoTimeCnt = 0U;
        demoBuffer.clear();
        throw;
      }
      demoFile = (Ep128Emu::File *) 0;
      demoTimeCnt = 0U;
      demoBuffer.clear();
    }
  }

  Plus4VM::Plus4VM(Ep128Emu::VideoDisplay& display_,
                   Ep128Emu::AudioOutput& audioOutput_)
    : VirtualMachine(display_, audioOutput_),
      ted((TED7360_ *) 0),
      cpuFrequencyMultiplier(1),
      tedFrequency(886724),
      soundClockFrequency(221681),
      tedCyclesRemaining(0),
      tapeSamplesPerTEDCycle(0),
      tapeSamplesRemaining(0),
      demoFile((Ep128Emu::File *) 0),
      demoBuffer(),
      isRecordingDemo(false),
      isPlayingDemo(false),
      snapshotLoadFlag(false),
      demoTimeCnt(0U)
  {
    ted = new TED7360_(*this);
    try {
      // reset
      ted->reset(true);
      // use PLUS/4 colormap
      Ep128Emu::VideoDisplay::DisplayParameters
          dp(display.getDisplayParameters());
      dp.indexToRGBFunc = &TED7360::convertPixelToRGB;
      display.setDisplayParameters(dp);
      setAudioConverterSampleRate(float(long(soundClockFrequency)));
    }
    catch (...) {
      delete ted;
      throw;
    }
  }

  Plus4VM::~Plus4VM()
  {
    try {
      // FIXME: cannot handle errors here
      stopDemo();
    }
    catch (...) {
    }
    delete ted;
  }

  void Plus4VM::run(size_t microseconds)
  {
    Ep128Emu::VirtualMachine::run(microseconds);
    if (snapshotLoadFlag) {
      snapshotLoadFlag = false;
      // if just loaded a snapshot, and not playing a demo,
      // clear keyboard state
      if (!isPlayingDemo) {
        for (int i = 0; i < 128; i++)
          ted->setKeyState(i, false);
      }
    }
    tedCyclesRemaining +=
        ((int64_t(microseconds) << 26) * int64_t(tedFrequency)
         / int64_t(15625));     // 10^6 / 2^6
    while (tedCyclesRemaining > 0) {
      if (isPlayingDemo) {
        while (!demoTimeCnt) {
          if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
            stopDemoPlayback();
          try {
            uint8_t evtType = demoBuffer.readByte();
            uint8_t evtBytes = demoBuffer.readByte();
            uint8_t evtData = 0;
            while (evtBytes) {
              evtData = demoBuffer.readByte();
              evtBytes--;
            }
            switch (evtType) {
            case 0x00:
              stopDemoPlayback();
              break;
            case 0x01:
              ted->setKeyState(evtData, true);
              break;
            case 0x02:
              ted->setKeyState(evtData, false);
              break;
            }
            demoTimeCnt = readDemoTimeCnt(demoBuffer);
          }
          catch (...) {
            stopDemoPlayback();
          }
          if (!isPlayingDemo) {
            demoBuffer.clear();
            demoTimeCnt = 0U;
            break;
          }
        }
        if (demoTimeCnt)
          demoTimeCnt--;
      }
      if (haveTape()) {
        tapeSamplesRemaining += tapeSamplesPerTEDCycle;
        if (tapeSamplesRemaining > 0) {
          // assume tape sample rate < tedFrequency
          tapeSamplesRemaining -= (int64_t(1) << 32);
          setTapeMotorState(ted->getTapeMotorState());
          ted->setTapeInput(runTape(ted->getTapeOutput() ? 1 : 0) > 0);
        }
      }
      ted->run(2);
      tedCyclesRemaining -= (int64_t(1) << 32);
      if (isRecordingDemo)
        demoTimeCnt++;
    }
  }

  void Plus4VM::reset(bool isColdReset)
  {
    stopDemoPlayback();         // TODO: should be recorded as an event ?
    stopDemoRecording(false);
    ted->reset(isColdReset);
    setTapeMotorState(false);
  }

  void Plus4VM::resetMemoryConfiguration(size_t memSize)
  {
    (void) memSize;     // RAM size is always 64K
    stopDemo();
    // delete all ROM segments
    for (uint8_t n = 0; n < 8; n++)
      loadROMSegment(n, (char *) 0, 0);
    // cold reset
    this->reset(true);
  }

  void Plus4VM::loadROMSegment(uint8_t n, const char *fileName, size_t offs)
  {
    stopDemo();
    if (n >= 8)
      return;
    // clear segment first
    uint8_t tmpBuf[256];
    for (size_t i = 0; i < 256; i++)
      tmpBuf[i] = 0xFF;
    for (int i = 0; i < 16384; i += 256)
      ted->loadROM(int(n) >> 1, (int(n & 1) << 14) + i, 256, &(tmpBuf[0]));
    if (fileName == (char *) 0 || fileName[0] == '\0') {
      // empty file name: delete segment
      return;
    }
    // load file into memory
    std::vector<uint8_t>  buf;
    buf.resize(0x4000);
    std::FILE   *f = std::fopen(fileName, "rb");
    if (!f)
      throw Ep128Emu::Exception("cannot open ROM file");
    std::fseek(f, 0L, SEEK_END);
    if (ftell(f) < long(offs + 0x4000)) {
      std::fclose(f);
      throw Ep128Emu::Exception("ROM file is shorter than expected");
    }
    std::fseek(f, long(offs), SEEK_SET);
    std::fread(&(buf.front()), 1, 0x4000, f);
    std::fclose(f);
    ted->loadROM(int(n) >> 1, int(n & 1) << 14, 16384, &(buf.front()));
  }

  void Plus4VM::setCPUFrequency(size_t freq_)
  {
    size_t  freqMult = (freq_ + tedFrequency) / (tedFrequency << 1);
    freqMult = (freqMult > 1 ? (freqMult < 100 ? freqMult : 100) : 1);
    if (freqMult != cpuFrequencyMultiplier) {
      stopDemoPlayback();       // changing configuration implies stopping
      stopDemoRecording(false); // any demo playback or recording
      cpuFrequencyMultiplier = freqMult;
      ted->setCPUClockMultiplier(int(freqMult));
    }
  }

  void Plus4VM::setVideoFrequency(size_t freq_)
  {
    // TODO: implement this eventually
    (void) freq_;
  }

  void Plus4VM::setVideoMemoryLatency(size_t t_)
  {
    // this is not implemented for this machine
    (void) t_;
  }

  void Plus4VM::setEnableMemoryTimingEmulation(bool isEnabled)
  {
    // this is not implemented for this machine
    (void) isEnabled;
  }

  void Plus4VM::setKeyboardState(int keyCode, bool isPressed)
  {
    if (!isPlayingDemo)
      ted->setKeyState(keyCode, isPressed);
    if (isRecordingDemo) {
      if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0) {
        stopDemoRecording(false);
        return;
      }
      writeDemoTimeCnt(demoBuffer, demoTimeCnt);
      demoTimeCnt = 0U;
      demoBuffer.writeByte(isPressed ? 0x01 : 0x02);
      demoBuffer.writeByte(0x01);
      demoBuffer.writeByte(uint8_t(keyCode & 0x7F));
    }
  }

  void Plus4VM::setTapeFileName(const char *fileName)
  {
    Ep128Emu::VirtualMachine::setTapeFileName(fileName);
    setTapeMotorState(ted->getTapeMotorState());
    if (haveTape()) {
      tapeSamplesPerTEDCycle =
          (int64_t(getTapeSampleRate()) << 32) / int64_t(tedFrequency);
    }
    tapeSamplesRemaining = 0;
  }

  void Plus4VM::tapePlay()
  {
    Ep128Emu::VirtualMachine::tapePlay();
    ted->setTapeButtonState(getTapeButtonState() != 0);
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void Plus4VM::tapeRecord()
  {
    Ep128Emu::VirtualMachine::tapeRecord();
    ted->setTapeButtonState(getTapeButtonState() != 0);
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void Plus4VM::tapeStop()
  {
    Ep128Emu::VirtualMachine::tapeStop();
    ted->setTapeButtonState(false);
  }

  void Plus4VM::setBreakPoints(const std::string& bpList)
  {
    // TODO: implement this
    (void) bpList;
  }

  std::string Plus4VM::getBreakPoints()
  {
    // TODO: implement this
    return std::string("");
  }

  void Plus4VM::clearBreakPoints()
  {
    // TODO: implement this
  }

  void Plus4VM::setBreakPointPriorityThreshold(int n)
  {
    // TODO: implement this
    (void) n;
  }

  uint8_t Plus4VM::getMemoryPage(int n) const
  {
    return ted->getMemoryPage(n);
  }

  uint8_t Plus4VM::readMemory(uint32_t addr) const
  {
    return ted->readMemoryRaw(addr);
  }

  void Plus4VM::saveState(Ep128Emu::File& f)
  {
    // TODO: implement this
    (void) f;
  }

  void Plus4VM::saveMachineConfiguration(Ep128Emu::File& f)
  {
    // TODO: implement this
    (void) f;
  }

  void Plus4VM::saveProgram(Ep128Emu::File& f)
  {
    ted->saveProgram(f);
  }

  void Plus4VM::saveProgram(const char *fileName)
  {
    ted->saveProgram(fileName);
  }

  void Plus4VM::loadProgram(const char *fileName)
  {
    ted->loadProgram(fileName);
  }

  void Plus4VM::recordDemo(Ep128Emu::File& f)
  {
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    ted->setTapeMotorState(false);
    setTapeMotorState(false);
    stopDemo();
    for (int i = 0; i < 128; i++)
      ted->setKeyState(i, false);
    // save full snapshot, including timing and clock frequency settings
    saveMachineConfiguration(f);
    saveState(f);
    demoBuffer.clear();
    demoBuffer.writeUInt32(0x00010900); // version 1.9.0 (beta)
    demoFile = &f;
    isRecordingDemo = true;
  }

  void Plus4VM::stopDemo()
  {
    stopDemoPlayback();
    stopDemoRecording(true);
  }

  bool Plus4VM::getIsRecordingDemo()
  {
    if (demoFile != (Ep128Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    return isRecordingDemo;
  }

  bool Plus4VM::getIsPlayingDemo() const
  {
    return isPlayingDemo;
  }

  // --------------------------------------------------------------------------

  void Plus4VM::loadState(Ep128Emu::File::Buffer& buf)
  {
    // TODO: implement this
    (void) buf;
  }

  void Plus4VM::loadMachineConfiguration(Ep128Emu::File::Buffer& buf)
  {
    // TODO: implement this
    (void) buf;
  }

  void Plus4VM::loadDemo(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
#if 0
    if (version != 0x00010900) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible ep128 demo format");
    }
#endif
    (void) version;
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    ted->setTapeMotorState(false);
    setTapeMotorState(false);
    stopDemo();
    for (int i = 0; i < 128; i++)
      ted->setKeyState(i, false);
    // initialize time counter with first delta time
    demoTimeCnt = readDemoTimeCnt(buf);
    isPlayingDemo = true;
    // copy any remaining demo data to local buffer
    demoBuffer.clear();
    demoBuffer.writeData(buf.getData() + buf.getPosition(),
                         buf.getDataSize() - buf.getPosition());
    demoBuffer.setPosition(0);
  }

  class ChunkType_Plus4VMConfig : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Plus4VM&  ref;
   public:
    ChunkType_Plus4VMConfig(Plus4VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Plus4VMConfig()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_P4VM_CONFIG;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadMachineConfiguration(buf);
    }
  };

  class ChunkType_Plus4VMSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Plus4VM&  ref;
   public:
    ChunkType_Plus4VMSnapshot(Plus4VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Plus4VMSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_P4VM_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  class ChunkType_Plus4DemoStream : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Plus4VM&  ref;
   public:
    ChunkType_Plus4DemoStream(Plus4VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Plus4DemoStream()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_PLUS4_DEMO;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadDemo(buf);
    }
  };

  void Plus4VM::registerChunkTypes(Ep128Emu::File& f)
  {
    ChunkType_Plus4VMConfig   *p1 = (ChunkType_Plus4VMConfig *) 0;
    ChunkType_Plus4VMSnapshot *p2 = (ChunkType_Plus4VMSnapshot *) 0;
    ChunkType_Plus4DemoStream *p3 = (ChunkType_Plus4DemoStream *) 0;

    try {
      p1 = new ChunkType_Plus4VMConfig(*this);
      p2 = new ChunkType_Plus4VMSnapshot(*this);
      p3 = new ChunkType_Plus4DemoStream(*this);
      f.registerChunkType(p1);
      f.registerChunkType(p2);
      f.registerChunkType(p3);
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
    ted->registerChunkTypes(f);
  }

}       // namespace Plus4

