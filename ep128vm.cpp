
// ep128emu -- portable Enterprise 128 emulator
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
#include "z80/z80.hpp"
#include "memory.hpp"
#include "ioports.hpp"
#include "dave.hpp"
#include "nick.hpp"
#include "tape.hpp"
#include "soundio.hpp"
#include "gldisp.hpp"
#include "ep128vm.hpp"

#include <cstdio>
#include <vector>

static void writeDemoTimeCnt(Ep128::File::Buffer& buf, uint64_t n)
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

static uint64_t readDemoTimeCnt(Ep128::File::Buffer& buf)
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

namespace Ep128 {

  Ep128VM::Z80_::Z80_(Ep128VM& vm_)
    : Z80(),
      vm(vm_)
  {
  }

  Ep128VM::Z80_::~Z80_()
  {
  }

  uint8_t Ep128VM::Z80_::readMemory(uint16_t addr)
  {
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          vm.memoryWaitCycle();
      }
      else
        vm.videoMemoryWait();
    }
    return (vm.memory.read(addr));
  }

  uint16_t Ep128VM::Z80_::readMemoryWord(uint16_t addr)
  {
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          vm.memoryWaitCycle();
      }
      else
        vm.videoMemoryWait();
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          vm.memoryWaitCycle();
      }
      else
        vm.videoMemoryWait();
    }
    uint16_t  retval = vm.memory.read(addr);
    retval |= (uint16_t(vm.memory.read((addr + 1) & 0xFFFF) << 8));
    return retval;
  }

  uint8_t Ep128VM::Z80_::readOpcodeFirstByte()
  {
    uint16_t  addr = uint16_t(R.PC.W.l);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode < 2)
          vm.memoryWaitCycle();
      }
      else
        vm.videoMemoryWait();
    }
    return (vm.memory.read(addr));
  }

  uint8_t Ep128VM::Z80_::readOpcodeByte(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          vm.memoryWaitCycle();
      }
      else
        vm.videoMemoryWait();
    }
    return (vm.memory.read(addr));
  }

  uint16_t Ep128VM::Z80_::readOpcodeWord(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          vm.memoryWaitCycle();
      }
      else
        vm.videoMemoryWait();
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          vm.memoryWaitCycle();
      }
      else
        vm.videoMemoryWait();
    }
    uint16_t  retval = vm.memory.read(addr);
    retval |= (uint16_t(vm.memory.read((addr + 1) & 0xFFFF) << 8));
    return retval;
  }

  void Ep128VM::Z80_::writeMemory(uint16_t addr, uint8_t value)
  {
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          vm.memoryWaitCycle();
      }
      else
        vm.videoMemoryWait();
    }
    vm.memory.write(addr, value);
  }

  void Ep128VM::Z80_::writeMemoryWord(uint16_t addr, uint16_t value)
  {
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          vm.memoryWaitCycle();
      }
      else
        vm.videoMemoryWait();
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          vm.memoryWaitCycle();
      }
      else
        vm.videoMemoryWait();
    }
    vm.memory.write(addr, uint8_t(value) & 0xFF);
    vm.memory.write((addr + 1) & 0xFFFF, uint8_t(value >> 8) & 0xFF);
  }

  void Ep128VM::Z80_::doOut(uint16_t addr, uint8_t value)
  {
    vm.ioPorts.write(addr, value);
  }

  uint8_t Ep128VM::Z80_::doIn(uint16_t addr)
  {
    return vm.ioPorts.read(addr);
  }

  void Ep128VM::Z80_::updateCycles(int cycles)
  {
    vm.updateCPUCycles(cycles);
  }

  // --------------------------------------------------------------------------

  Ep128VM::Memory_::Memory_(Ep128VM& vm_)
    : Memory(),
      vm(vm_)
  {
  }

  Ep128VM::Memory_::~Memory_()
  {
  }

  void Ep128VM::Memory_::breakPointCallback(bool isWrite,
                                            uint16_t addr, uint8_t value)
  {
    vm.breakPointCallback(false, isWrite, addr, value);
  }

  // --------------------------------------------------------------------------

  Ep128VM::IOPorts_::IOPorts_(Ep128VM& vm_)
    : IOPorts(),
      vm(vm_)
  {
  }

  Ep128VM::IOPorts_::~IOPorts_()
  {
  }

  void Ep128VM::IOPorts_::breakPointCallback(bool isWrite,
                                             uint16_t addr, uint8_t value)
  {
    vm.breakPointCallback(true, isWrite, addr, value);
  }

  // --------------------------------------------------------------------------

  Ep128VM::Dave_::Dave_(Ep128VM& vm_)
    : Dave(),
      vm(vm_)
  {
  }

  Ep128VM::Dave_::~Dave_()
  {
  }

  void Ep128VM::Dave_::setMemoryPage(uint8_t page, uint8_t segment)
  {
    vm.pageTable[page] = segment;
    vm.memory.setPage(page, segment);
  }

  void Ep128VM::Dave_::setMemoryWaitMode(int mode)
  {
    vm.memoryWaitMode = mode;
  }

  void Ep128VM::Dave_::setRemote1State(int state)
  {
    vm.isRemote1On = (state != 0);
    if (vm.tape)
      vm.tape->setIsMotorOn(vm.isRemote1On || vm.isRemote2On);
    if (vm.fastTapeModeEnabled &&
        vm.tape != (Tape *) 0 &&
        (vm.isRemote1On || vm.isRemote2On) &&
        (vm.tapePlaybackOn || vm.tapeRecordOn))
      vm.writingAudioOutput = false;
    else
      vm.writingAudioOutput =
          (vm.audioOutputEnabled && vm.audioOutput != (DaveConverter *) 0);
  }

  void Ep128VM::Dave_::setRemote2State(int state)
  {
    vm.isRemote2On = (state != 0);
    if (vm.tape)
      vm.tape->setIsMotorOn(vm.isRemote1On || vm.isRemote2On);
    if (vm.fastTapeModeEnabled &&
        vm.tape != (Tape *) 0 &&
        (vm.isRemote1On || vm.isRemote2On) &&
        (vm.tapePlaybackOn || vm.tapeRecordOn))
      vm.writingAudioOutput = false;
    else
      vm.writingAudioOutput =
          (vm.audioOutputEnabled && vm.audioOutput != (DaveConverter *) 0);
  }

  void Ep128VM::Dave_::interruptRequest()
  {
    vm.z80.triggerInterrupt();
  }

  void Ep128VM::Dave_::clearInterruptRequest()
  {
    vm.z80.clearInterrupt();
  }

  // --------------------------------------------------------------------------

  Ep128VM::Nick_::Nick_(Ep128VM& vm_)
    : Nick(vm_.memory),
      vm(vm_)
  {
  }

  Ep128VM::Nick_::~Nick_()
  {
  }

  void Ep128VM::Nick_::irqStateChange(bool newState)
  {
    vm.dave.setInt1State(newState ? 1 : 0);
  }

  void Ep128VM::Nick_::drawLine(const uint8_t *buf, size_t nBytes)
  {
    if (vm.displayEnabled)
      vm.display.drawLine(buf, nBytes);
  }

  void Ep128VM::Nick_::vsyncStateChange(bool newState,
                                        unsigned int currentSlot_)
  {
    if (vm.displayEnabled)
      vm.display.vsyncStateChange(newState, currentSlot_);
  }

  // --------------------------------------------------------------------------

  void Ep128VM::stopDemoPlayback()
  {
    if (isPlayingDemo) {
      isPlayingDemo = false;
      demoTimeCnt = 0U;
      demoBuffer.clear();
      // clear keyboard state at end of playback
      for (int i = 0; i < 128; i++)
        dave.setKeyboardState(i, 0);
    }
  }

  void Ep128VM::stopDemoRecording(bool writeFile_)
  {
    isRecordingDemo = false;
    if (writeFile_ && demoFile != (File *) 0) {
      try {
        // put end of demo event
        writeDemoTimeCnt(demoBuffer, demoTimeCnt);
        demoTimeCnt = 0U;
        demoBuffer.writeByte(0x00);
        demoBuffer.writeByte(0x00);
        demoFile->addChunk(File::EP128EMU_CHUNKTYPE_DEMO_STREAM, demoBuffer);
      }
      catch (...) {
        demoFile = (File *) 0;
        demoTimeCnt = 0U;
        demoBuffer.clear();
        throw;
      }
      demoFile = (File *) 0;
      demoTimeCnt = 0U;
      demoBuffer.clear();
    }
  }

  void Ep128VM::updateTimingParameters()
  {
    stopDemoPlayback();         // changing configuration implies stopping
    stopDemoRecording(false);   // any demo playback or recording
    cpuCyclesPerNickCycle =
        (int64_t(cpuFrequency) << 32) / int64_t(nickFrequency);
    videoMemoryLatencyCycles =
        (int64_t(videoMemoryLatency) << 23) * int64_t(cpuFrequency)
        / int64_t(1953125);     // 10^9 / 2^9
    // for optimization in videoMemoryWait()
    videoMemoryLatencyCycles += int64_t(0xFFFFFFFFUL);
    daveCyclesPerNickCycle =
        (int64_t(daveFrequency) << 32) / int64_t(nickFrequency);
    cpuCyclesRemaining = 0;
    cpuSyncToNickCnt = 0;
  }

  uint8_t Ep128VM::davePortReadCallback(void *userData, uint16_t addr)
  {
    return (reinterpret_cast<Ep128VM *>(userData)->dave.readPort(addr));
  }

  void Ep128VM::davePortWriteCallback(void *userData,
                                      uint16_t addr, uint8_t value)
  {
    reinterpret_cast<Ep128VM *>(userData)->dave.writePort(addr, value);
  }

  void Ep128VM::nickPortWriteCallback(void *userData,
                                      uint16_t addr, uint8_t value)
  {
    reinterpret_cast<Ep128VM *>(userData)->nick.writePort(addr, value);
  }

  Ep128VM::Ep128VM(Ep128Emu::OpenGLDisplay& display_)
    : display(display_),
      z80(*this),
      memory(*this),
      ioPorts(*this),
      dave(*this),
      nick(*this),
      audioOutput((DaveConverter *) 0),
      cpuFrequency(4000000),
      daveFrequency(500000),
      nickFrequency(890625),
      videoMemoryLatency(62),
      nickCyclesRemaining(0),
      cpuCyclesPerNickCycle(0),
      cpuCyclesRemaining(0),
      cpuSyncToNickCnt(0),
      videoMemoryLatencyCycles(0),
      daveCyclesPerNickCycle(0),
      daveCyclesRemaining(0),
      memoryWaitMode(1),
      memoryTimingEnabled(true),
      displayEnabled(true),
      audioOutputEnabled(true),
      audioOutputHighQuality(false),
      audioOutputSampleRate(48000.0f),
      audioOutputDevice(0),
      audioOutputLatency(0.1f),
      audioOutputPeriodsHW(4),
      audioOutputPeriodsSW(3),
      audioOutputVolume(0.7071f),
      audioOutputFilter1Freq(10.0f),
      audioOutputFilter2Freq(10.0f),
      audioOutputFileName(""),
      tapeFileName(""),
      tape((Tape *) 0),
      tapeSamplesPerDaveCycle(0),
      tapeSamplesRemaining(0),
      isRemote1On(false),
      isRemote2On(false),
      tapePlaybackOn(false),
      tapeRecordOn(false),
      fastTapeModeEnabled(false),
      writingAudioOutput(false),
      demoFile((File *) 0),
      demoBuffer(),
      isRecordingDemo(false),
      isPlayingDemo(false),
      snapshotLoadFlag(false),
      demoTimeCnt(0U)
  {
    for (size_t i = 0; i < 4; i++)
      pageTable[i] = 0x00;
    updateTimingParameters();
    // register I/O callbacks
    for (uint16_t i = 0xA0; i <= 0xBF; i++) {
      ioPorts.setReadCallback(i, i, &davePortReadCallback, this, 0xA0);
      ioPorts.setWriteCallback(i, i, &davePortWriteCallback, this, 0xA0);
    }
    for (uint16_t i = 0x80; i <= 0x8F; i++) {
      ioPorts.setWriteCallback(i, i, &nickPortWriteCallback, this, (i & 0x8C));
    }
    // set up initial memory (segments 0xFC to 0xFF are always present)
    memory.loadSegment(0xFC, false, (uint8_t *) 0, 0);
    memory.loadSegment(0xFD, false, (uint8_t *) 0, 0);
    memory.loadSegment(0xFE, false, (uint8_t *) 0, 0);
    memory.loadSegment(0xFF, false, (uint8_t *) 0, 0);
    // hack to get some programs using interrupt mode 2 working
    z80.setVectorBase(0xFF);
    // reset
    z80.reset();
    for (uint16_t i = 0x80; i <= 0x83; i++)
      nick.writePort(i, 0x00);
    dave.reset();
    // use NICK colormap
    Ep128Emu::OpenGLDisplay::DisplayParameters
        dp(display.getDisplayParameters());
    dp.indexToRGBFunc = &Nick::convertPixelToRGB;
    display.setDisplayParameters(dp);
  }

  Ep128VM::~Ep128VM()
  {
    if (tape)
      delete tape;
    if (audioOutput)
      delete audioOutput;
  }

  void Ep128VM::run(size_t microseconds)
  {
    nickCyclesRemaining +=
        ((int64_t(microseconds) << 26) * int64_t(nickFrequency)
         / int64_t(15625));     // 10^6 / 2^6
    if (fastTapeModeEnabled &&
        tape != (Tape *) 0 &&
        (isRemote1On || isRemote2On) &&
        (tapePlaybackOn || tapeRecordOn))
      writingAudioOutput = false;
    else
      writingAudioOutput =
          (audioOutputEnabled && audioOutput != (DaveConverter *) 0);
    if (snapshotLoadFlag) {
      snapshotLoadFlag = false;
      // if just loaded a snapshot, and not playing a demo,
      // clear keyboard state
      if (!isPlayingDemo) {
        for (int i = 0; i < 128; i++)
          dave.setKeyboardState(i, 0);
      }
    }
    while (nickCyclesRemaining > 0) {
      if (isPlayingDemo) {
        while (!demoTimeCnt) {
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
              dave.setKeyboardState(evtData, 1);
              break;
            case 0x02:
              dave.setKeyboardState(evtData, 0);
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
      daveCyclesRemaining += daveCyclesPerNickCycle;
      while (daveCyclesRemaining > 0) {
        daveCyclesRemaining -= (int64_t(1) << 32);
        uint32_t  tmp = dave.runOneCycle();
        if (writingAudioOutput)
          audioOutput->sendInputSignal(tmp);
        if (tape) {
          tapeSamplesRemaining += tapeSamplesPerDaveCycle;
          if (tapeSamplesRemaining > 0) {
            // assume tape sample rate < daveFrequency
            tapeSamplesRemaining -= (int64_t(1) << 32);
            tape->setInputSignal(int(tmp & 0x01FF));
            tape->runOneSample();
            int   daveTapeInput = (tapeRecordOn ? 0 : tape->getOutputSignal());
            dave.setTapeInput(daveTapeInput, daveTapeInput);
          }
        }
      }
      cpuCyclesRemaining += cpuCyclesPerNickCycle;
      cpuSyncToNickCnt += cpuCyclesPerNickCycle;
      while (cpuCyclesRemaining > 0)
        z80.executeInstruction();
      nick.runOneSlot();
      nickCyclesRemaining -= (int64_t(1) << 32);
      if (isRecordingDemo)
        demoTimeCnt++;
    }
  }

  void Ep128VM::reset(bool isColdReset)
  {
    stopDemoPlayback();         // TODO: should be recorded as an event ?
    stopDemoRecording(false);
    z80.reset();
    dave.reset();
    isRemote1On = false;
    isRemote2On = false;
    if (tape)
      tape->setIsMotorOn(false);
    if (isColdReset) {
      for (int i = 0; i < 256; i++) {
        if (memory.isSegmentRAM(uint8_t(i)))
          memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
      }
    }
  }

  void Ep128VM::resetMemoryConfiguration(size_t memSize)
  {
    // calculate new number of RAM segments
    size_t  nSegments = (memSize + 15) >> 4;
    nSegments = (nSegments > 4 ? (nSegments < 232 ? nSegments : 232) : 4);
    // delete all ROM segments
    for (int i = 0; i < 256; i++) {
      if (memory.isSegmentROM(uint8_t(i)))
        memory.deleteSegment(uint8_t(i));
    }
    // resize RAM
    for (int i = 0; i <= (0xFF - int(nSegments)); i++) {
      if (memory.isSegmentRAM(uint8_t(i)))
        memory.deleteSegment(uint8_t(i));
    }
    for (int i = 0xFF; i > (0xFF - int(nSegments)); i--) {
      if (!memory.isSegmentRAM(uint8_t(i)))
        memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
    }
    // cold reset
    this->reset(true);
  }

  void Ep128VM::loadROMSegment(uint8_t n, const char *fileName, size_t offs)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0') {
      // empty file name: delete segment
      if (memory.isSegmentROM(n))
        memory.deleteSegment(n);
      else if (memory.isSegmentRAM(n)) {
        memory.deleteSegment(n);
        // if there was RAM at the specified segment, relocate it
        for (int i = 0xFF; i >= 0x08; i--) {
          if (!(memory.isSegmentROM(uint8_t(i)) ||
                memory.isSegmentRAM(uint8_t(i)))) {
            memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
            break;
          }
        }
      }
    }
    // load file into memory
    std::vector<uint8_t>  buf;
    buf.resize(0x4000);
    std::FILE   *f = std::fopen(fileName, "rb");
    if (!f)
      throw Exception("cannot open ROM file");
    std::fseek(f, 0L, SEEK_END);
    if (ftell(f) < long(offs + 0x4000)) {
      std::fclose(f);
      throw Exception("ROM file is shorter than expected");
    }
    std::fseek(f, long(offs), SEEK_SET);
    std::fread(&(buf.front()), 1, 0x4000, f);
    std::fclose(f);
    if (memory.isSegmentRAM(n)) {
      memory.loadSegment(n, true, &(buf.front()), 0x4000);
      // if there was RAM at the specified segment, relocate it
      for (int i = 0xFF; i >= 0x08; i--) {
        if (!(memory.isSegmentROM(uint8_t(i)) ||
              memory.isSegmentRAM(uint8_t(i)))) {
          memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
          break;
        }
      }
    }
    else {
      // otherwise just load new segment, or replace existing ROM
      memory.loadSegment(n, true, &(buf.front()), 0x4000);
    }
  }

  void Ep128VM::setAudioOutputQuality(bool useHighQualityResample,
                                      float sampleRate_)
  {
    float   sampleRate = (sampleRate_ > 11025.0f ?
                          (sampleRate_ < 192000.0f ? sampleRate_ : 192000.0f)
                          : 11025.0f);
    if (audioOutputHighQuality != useHighQualityResample ||
        audioOutputSampleRate != sampleRate) {
      audioOutputHighQuality = useHighQualityResample;
      audioOutputSampleRate = sampleRate;
      if (audioOutput) {
        delete audioOutput;
        audioOutput = (DaveConverter *) 0;
        if (audioOutputHighQuality) {
          audioOutput = new Ep128Emu::AudioOutput_HighQuality(
              float(long(daveFrequency)), audioOutputSampleRate,
              audioOutputFilter1Freq, audioOutputFilter2Freq, audioOutputVolume,
              audioOutputDevice, audioOutputLatency,
              audioOutputPeriodsHW, audioOutputPeriodsSW,
              audioOutputFileName.c_str());
        }
        else {
          audioOutput = new Ep128Emu::AudioOutput_LowQuality(
              float(long(daveFrequency)), audioOutputSampleRate,
              audioOutputFilter1Freq, audioOutputFilter2Freq, audioOutputVolume,
              audioOutputDevice, audioOutputLatency,
              audioOutputPeriodsHW, audioOutputPeriodsSW,
              audioOutputFileName.c_str());
        }
      }
    }
  }

  void Ep128VM::setAudioOutputDeviceParameters(int devNum,
                                               float totalLatency_,
                                               int nPeriodsHW_,
                                               int nPeriodsSW_)
  {
    float   totalLatency = (totalLatency_ > 0.005f ?
                            (totalLatency_ < 0.5f ? totalLatency_ : 0.5f)
                            : 0.005f);
    int     nPeriodsHW = (nPeriodsHW_ > 2 ?
                          (nPeriodsHW_ < 16 ? nPeriodsHW_ : 16) : 2);
    int     nPeriodsSW = (nPeriodsSW_ > 2 ?
                          (nPeriodsSW_ < 16 ? nPeriodsSW_ : 16) : 2);
    if (audioOutputDevice != devNum ||
        audioOutputLatency != totalLatency ||
        audioOutputPeriodsHW != nPeriodsHW ||
        audioOutputPeriodsSW != nPeriodsSW ||
        audioOutput == (DaveConverter *) 0) {
      audioOutputDevice = devNum;
      audioOutputLatency = totalLatency;
      audioOutputPeriodsHW = nPeriodsHW;
      audioOutputPeriodsSW = nPeriodsSW;
      if (audioOutput) {
        delete audioOutput;
        audioOutput = (DaveConverter *) 0;
      }
      if (audioOutputHighQuality) {
        audioOutput = new Ep128Emu::AudioOutput_HighQuality(
            float(long(daveFrequency)), audioOutputSampleRate,
            audioOutputFilter1Freq, audioOutputFilter2Freq, audioOutputVolume,
            audioOutputDevice, audioOutputLatency,
            audioOutputPeriodsHW, audioOutputPeriodsSW,
            audioOutputFileName.c_str());
      }
      else {
        audioOutput = new Ep128Emu::AudioOutput_LowQuality(
            float(long(daveFrequency)), audioOutputSampleRate,
            audioOutputFilter1Freq, audioOutputFilter2Freq, audioOutputVolume,
            audioOutputDevice, audioOutputLatency,
            audioOutputPeriodsHW, audioOutputPeriodsSW,
            audioOutputFileName.c_str());
      }
    }
  }

  void Ep128VM::setAudioOutputFilters(float dcBlockFreq1_, float dcBlockFreq2_)
  {
    audioOutputFilter1Freq =
        (dcBlockFreq1_ > 1.0f ?
         (dcBlockFreq1_ < 1000.0f ? dcBlockFreq1_ : 1000.0f) : 1.0f);
    audioOutputFilter2Freq =
        (dcBlockFreq2_ > 1.0f ?
         (dcBlockFreq2_ < 1000.0f ? dcBlockFreq2_ : 1000.0f) : 1.0f);
    if (audioOutput)
      audioOutput->setDCBlockFilters(audioOutputFilter1Freq,
                                     audioOutputFilter2Freq);
  }

  void Ep128VM::setAudioOutputVolume(float ampScale_)
  {
    audioOutputVolume =
        (ampScale_ > 0.01f ? (ampScale_ < 1.0f ? ampScale_ : 1.0f) : 0.01f);
    if (audioOutput)
      audioOutput->setOutputVolume(audioOutputVolume);
  }

  void Ep128VM::setAudioOutputFileName(const char *fileName)
  {
    std::string fname("");
    if (fileName)
      fname = fileName;
    if (audioOutputFileName == fname)
      return;
    audioOutputFileName = fname;
    if (!audioOutput)
      return;
    if (typeid(*audioOutput) == typeid(Ep128Emu::AudioOutput_HighQuality)) {
      Ep128Emu::AudioOutput_HighQuality *p;
      p = dynamic_cast<Ep128Emu::AudioOutput_HighQuality *>(audioOutput);
      p->setOutputFile(fname);
    }
    else if (typeid(*audioOutput) == typeid(Ep128Emu::AudioOutput_LowQuality)) {
      Ep128Emu::AudioOutput_LowQuality  *p;
      p = dynamic_cast<Ep128Emu::AudioOutput_LowQuality *>(audioOutput);
      p->setOutputFile(fname);
    }
  }

  void Ep128VM::setEnableAudioOutput(bool isEnabled)
  {
    audioOutputEnabled = isEnabled;
  }

  void Ep128VM::setEnableDisplay(bool isEnabled)
  {
    if (displayEnabled != isEnabled) {
      displayEnabled = isEnabled;
      if (!isEnabled) {
        // clear display
        display.vsyncStateChange(true, 0);
        display.vsyncStateChange(false, 28);
        display.vsyncStateChange(true, 0);
        display.vsyncStateChange(false, 28);
      }
    }
  }

  void Ep128VM::setCPUFrequency(size_t freq_)
  {
    // NOTE: Z80 frequency should always be greater than NICK frequency,
    // so that the memory timing functions in ep128vm.hpp work correctly
    size_t  freq = (freq_ > 2000000 ? (freq_ < 250000000 ? freq_ : 250000000)
                                      : 2000000);
    if (cpuFrequency != freq) {
      cpuFrequency = freq;
      updateTimingParameters();
    }
  }

  void Ep128VM::setNickFrequency(size_t freq_)
  {
    size_t  freq;
    // allow refresh rates in the range 10 Hz to 100 Hz
    freq = (freq_ > 178125 ? (freq_ < 1781250 ? freq_ : 1781250) : 178125);
    if (nickFrequency != freq) {
      nickFrequency = freq;
      updateTimingParameters();
    }
  }

  void Ep128VM::setVideoMemoryLatency(size_t t_)
  {
    // NOTE: this should always be less than the length of one NICK cycle
    size_t  t = (t_ > 0 ? (t_ < 500 ? t_ : 500) : 0);
    if (videoMemoryLatency != t) {
      videoMemoryLatency = t;
      updateTimingParameters();
    }
  }

  void Ep128VM::setEnableMemoryTimingEmulation(bool isEnabled)
  {
    if (memoryTimingEnabled != isEnabled) {
      stopDemoPlayback();       // changing configuration implies stopping
      stopDemoRecording(false); // any demo playback or recording
      memoryTimingEnabled = isEnabled;
      cpuCyclesRemaining = 0;
      cpuSyncToNickCnt = 0;
    }
  }

  void Ep128VM::setKeyboardState(int keyCode, bool isPressed)
  {
    if (!isPlayingDemo)
      dave.setKeyboardState(keyCode, (isPressed ? 1 : 0));
    if (isRecordingDemo) {
      writeDemoTimeCnt(demoBuffer, demoTimeCnt);
      demoTimeCnt = 0U;
      demoBuffer.writeByte(isPressed ? 0x01 : 0x02);
      demoBuffer.writeByte(0x01);
      demoBuffer.writeByte(uint8_t(keyCode & 0x7F));
    }
  }

  void Ep128VM::setTapeFileName(const char *fileName)
  {
    std::string fname("");
    if (fileName)
      fname = fileName;
    if (tape) {
      if (fname == tapeFileName) {
        tape->seek(0.0);
        return;
      }
      delete tape;
      tape = (Tape *) 0;
    }
    if (fname.length() == 0)
      return;
    tape = new Tape(fname.c_str());
    if (tapeRecordOn)
      tape->record();
    else if (tapePlaybackOn)
      tape->play();
    if (isRemote1On || isRemote2On)
      tape->setIsMotorOn(true);
    tapeSamplesPerDaveCycle =
        (int64_t(tape->getSampleRate()) << 32) / int64_t(daveFrequency);
    tapeSamplesRemaining = 0;
  }

  void Ep128VM::tapePlay()
  {
    tapeRecordOn = false;
    if (tape) {
      tapePlaybackOn = true;
      tape->play();
      if (isRemote1On || isRemote2On)
        stopDemo();
    }
    else
      tapePlaybackOn = false;
  }

  void Ep128VM::tapeRecord()
  {
    if (tape) {
      tapePlaybackOn = true;
      tapeRecordOn = true;
      tape->record();
      if (isRemote1On || isRemote2On)
        stopDemo();
    }
    else {
      tapePlaybackOn = false;
      tapeRecordOn = false;
    }
  }

  void Ep128VM::tapeStop()
  {
    tapePlaybackOn = false;
    tapeRecordOn = false;
    if (tape)
      tape->stop();
  }

  void Ep128VM::tapeSeek(double t)
  {
    if (tape)
      tape->seek(t);
  }

  double Ep128VM::getTapePosition() const
  {
    if (tape)
      return tape->getPosition();
    return -1.0;
  }

  void Ep128VM::tapeSeekToCuePoint(bool isForward, double t)
  {
    if (tape)
      tape->seekToCuePoint(isForward, t);
  }

  void Ep128VM::tapeAddCuePoint()
  {
    if (tape)
      tape->addCuePoint();
  }

  void Ep128VM::tapeDeleteNearestCuePoint()
  {
    if (tape)
      tape->deleteNearestCuePoint();
  }

  void Ep128VM::tapeDeleteAllCuePoints()
  {
    if (tape)
      tape->deleteAllCuePoints();
  }

  void Ep128VM::setEnableFastTapeMode(bool isEnabled)
  {
    fastTapeModeEnabled = isEnabled;
  }

  void Ep128VM::setBreakPoints(const std::string& bpList)
  {
    BreakPointList  bpl(bpList);
    for (size_t i = 0; i < bpl.getBreakPointCnt(); i++) {
      const BreakPoint& bp = bpl.getBreakPoint(i);
      if (bp.isIO())
        ioPorts.setBreakPoint(bp.addr(),
                              bp.priority(), bp.isRead(), bp.isWrite());
      else if (bp.haveSegment())
        memory.setBreakPoint(bp.segment(), bp.addr(),
                             bp.priority(), bp.isRead(), bp.isWrite());
      else
        memory.setBreakPoint(bp.addr(),
                             bp.priority(), bp.isRead(), bp.isWrite());
    }
  }

  std::string Ep128VM::getBreakPoints()
  {
    return (ioPorts.getBreakPointList().getBreakPointList()
            + memory.getBreakPointList().getBreakPointList());
  }

  void Ep128VM::clearBreakPoints()
  {
    memory.clearAllBreakPoints();
    ioPorts.clearBreakPoints();
  }

  void Ep128VM::setBreakPointPriorityThreshold(int n)
  {
    memory.setBreakPointPriorityThreshold(n);
    ioPorts.setBreakPointPriorityThreshold(n);
  }

  uint8_t Ep128VM::getMemoryPage(int n) const
  {
    return memory.getPage(uint8_t(n & 3));
  }

  uint8_t Ep128VM::readMemory(uint32_t addr) const
  {
    return memory.readRaw(addr & uint32_t(0x003FFFFF));
  }

  const Z80_REGISTERS& Ep128VM::getCPURegisters() const
  {
    return z80.getReg();
  }

  void Ep128VM::saveState(File& f)
  {
    {
      File::Buffer  buf;
      buf.setPosition(0);
      buf.writeUInt32(0x01000000);      // version number
      buf.writeByte(memory.getPage(0));
      buf.writeByte(memory.getPage(1));
      buf.writeByte(memory.getPage(2));
      buf.writeByte(memory.getPage(3));
      buf.writeByte(uint8_t(memoryWaitMode & 3));
      buf.writeUInt32(uint32_t(cpuFrequency));
      buf.writeUInt32(uint32_t(daveFrequency));
      buf.writeUInt32(uint32_t(nickFrequency));
      buf.writeUInt32(uint32_t(videoMemoryLatency));
      buf.writeBoolean(memoryTimingEnabled);
      int64_t tmp[3];
      tmp[0] = cpuCyclesRemaining;
      tmp[1] = cpuSyncToNickCnt;
      tmp[2] = daveCyclesRemaining;
      for (size_t i = 0; i < 3; i++) {
        buf.writeUInt32(uint32_t(uint64_t(tmp[i]) >> 32)
                        & uint32_t(0xFFFFFFFFUL));
        buf.writeUInt32(uint32_t(uint64_t(tmp[i]))
                        & uint32_t(0xFFFFFFFFUL));
      }
      f.addChunk(File::EP128EMU_CHUNKTYPE_VM_STATE, buf);
    }
    ioPorts.saveState(f);
    z80.saveState(f);
    memory.saveState(f);
    dave.saveState(f);
    nick.saveState(f);
  }

  void Ep128VM::saveMachineConfiguration(File& f)
  {
    File::Buffer  buf;
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    buf.writeUInt32(uint32_t(cpuFrequency));
    buf.writeUInt32(uint32_t(daveFrequency));
    buf.writeUInt32(uint32_t(nickFrequency));
    buf.writeUInt32(uint32_t(videoMemoryLatency));
    buf.writeBoolean(memoryTimingEnabled);
    f.addChunk(File::EP128EMU_CHUNKTYPE_VM_CONFIG, buf);
  }

  void Ep128VM::recordDemo(File& f)
  {
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    isRemote1On = false;
    isRemote2On = false;
    if (tape)
      tape->setIsMotorOn(false);
    stopDemo();
    for (int i = 0; i < 128; i++)
      dave.setKeyboardState(i, 0);
    // save full snapshot, including timing and clock frequency settings
    saveMachineConfiguration(f);
    saveState(f);
    demoBuffer.clear();
    demoBuffer.writeUInt32(0x00010900); // version 1.9.0 (beta)
    demoFile = &f;
    isRecordingDemo = true;
  }

  void Ep128VM::stopDemo()
  {
    stopDemoPlayback();
    stopDemoRecording(true);
  }

  bool Ep128VM::getIsRecordingDemo()
  {
    if (demoFile != (File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    return isRecordingDemo;
  }

  bool Ep128VM::getIsPlayingDemo() const
  {
    return isPlayingDemo;
  }

  // --------------------------------------------------------------------------

  void Ep128VM::loadState(File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Exception("incompatible ep128 snapshot format");
    }
    isRemote1On = false;
    isRemote2On = false;
    if (tape)
      tape->setIsMotorOn(false);
    stopDemo();
    snapshotLoadFlag = true;
    try {
      uint8_t   p0, p1, p2, p3;
      p0 = buf.readByte();
      p1 = buf.readByte();
      p2 = buf.readByte();
      p3 = buf.readByte();
      pageTable[0] = p0;
      memory.setPage(0, p0);
      pageTable[1] = p1;
      memory.setPage(1, p1);
      pageTable[2] = p2;
      memory.setPage(2, p2);
      pageTable[3] = p3;
      memory.setPage(3, p3);
      memoryWaitMode = buf.readByte() & 3;
      uint32_t  tmpCPUFrequency = buf.readUInt32();
      uint32_t  tmpDaveFrequency = buf.readUInt32();
      uint32_t  tmpNickFrequency = buf.readUInt32();
      uint32_t  tmpVideoMemoryLatency = buf.readUInt32();
      bool      tmpMemoryTimingEnabled = buf.readBoolean();
      int64_t   tmp[3];
      for (size_t i = 0; i < 3; i++) {
        uint64_t  n = uint64_t(buf.readUInt32()) << 32;
        n |= uint64_t(buf.readUInt32());
        tmp[i] = int64_t(n);
      }
      if (tmpCPUFrequency == cpuFrequency &&
          tmpDaveFrequency == daveFrequency &&
          tmpNickFrequency == nickFrequency &&
          tmpVideoMemoryLatency == videoMemoryLatency &&
          tmpMemoryTimingEnabled == memoryTimingEnabled) {
        cpuCyclesRemaining = tmp[0];
        cpuSyncToNickCnt = tmp[1];
        daveCyclesRemaining = tmp[2];
      }
      else {
        cpuCyclesRemaining = 0;
        cpuSyncToNickCnt = 0;
        daveCyclesRemaining = 0;
      }
      if (buf.getPosition() != buf.getDataSize())
        throw Exception("trailing garbage at end of ep128 snapshot data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Ep128VM::loadMachineConfiguration(File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Exception("incompatible ep128 machine configuration format");
    }
    try {
      setCPUFrequency(buf.readUInt32());
      uint32_t  tmpDaveFrequency = buf.readUInt32();
      (void) tmpDaveFrequency;
      setNickFrequency(buf.readUInt32());
      setVideoMemoryLatency(buf.readUInt32());
      setEnableMemoryTimingEmulation(buf.readBoolean());
      if (buf.getPosition() != buf.getDataSize())
        throw Exception("trailing garbage at end of "
                        "ep128 machine configuration data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Ep128VM::loadDemo(File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
#if 0
    if (version != 0x00010900) {
      buf.setPosition(buf.getDataSize());
      throw Exception("incompatible ep128 demo format");
    }
#endif
    (void) version;
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    isRemote1On = false;
    isRemote2On = false;
    if (tape)
      tape->setIsMotorOn(false);
    stopDemo();
    for (int i = 0; i < 128; i++)
      dave.setKeyboardState(i, 0);
    // initialize time counter with first delta time
    demoTimeCnt = readDemoTimeCnt(buf);
    isPlayingDemo = true;
    // copy any remaining demo data to local buffer
    demoBuffer.clear();
    demoBuffer.writeData(buf.getData() + buf.getPosition(),
                         buf.getDataSize() - buf.getPosition());
    demoBuffer.setPosition(0);
  }

  class ChunkType_Ep128VMConfig : public File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_Ep128VMConfig(Ep128VM& ref_)
      : File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Ep128VMConfig()
    {
    }
    virtual File::ChunkType getChunkType() const
    {
      return File::EP128EMU_CHUNKTYPE_VM_CONFIG;
    }
    virtual void processChunk(File::Buffer& buf)
    {
      ref.loadMachineConfiguration(buf);
    }
  };

  class ChunkType_Ep128VMSnapshot : public File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_Ep128VMSnapshot(Ep128VM& ref_)
      : File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Ep128VMSnapshot()
    {
    }
    virtual File::ChunkType getChunkType() const
    {
      return File::EP128EMU_CHUNKTYPE_VM_STATE;
    }
    virtual void processChunk(File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  class ChunkType_DemoStream : public File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_DemoStream(Ep128VM& ref_)
      : File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_DemoStream()
    {
    }
    virtual File::ChunkType getChunkType() const
    {
      return File::EP128EMU_CHUNKTYPE_DEMO_STREAM;
    }
    virtual void processChunk(File::Buffer& buf)
    {
      ref.loadDemo(buf);
    }
  };

  void Ep128VM::registerChunkTypes(File& f)
  {
    ChunkType_Ep128VMConfig   *p1 = (ChunkType_Ep128VMConfig *) 0;
    ChunkType_Ep128VMSnapshot *p2 = (ChunkType_Ep128VMSnapshot *) 0;
    ChunkType_DemoStream      *p3 = (ChunkType_DemoStream *) 0;

    try {
      p1 = new ChunkType_Ep128VMConfig(*this);
      p2 = new ChunkType_Ep128VMSnapshot(*this);
      p3 = new ChunkType_DemoStream(*this);
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
    ioPorts.registerChunkType(f);
    z80.registerChunkType(f);
    memory.registerChunkType(f);
    dave.registerChunkType(f);
    nick.registerChunkType(f);
  }

  // --------------------------------------------------------------------------

  void Ep128VM::breakPointCallback(bool isIO, bool isWrite,
                                   uint16_t addr, uint8_t value)
  {
    (void) isIO;
    (void) isWrite;
    (void) addr;
    (void) value;
  }

}       // namespace Ep128

