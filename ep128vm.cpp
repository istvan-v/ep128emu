
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

  void Ep128VM::updateTimingParameters()
  {
    cpuCyclesPerNickCycle =
        (int64_t(cpuFrequency) << 32) / int64_t(nickFrequency);
    videoMemoryLatencyCycles =
        (int64_t(videoMemoryLatency) << 23) * int64_t(cpuFrequency)
        / int64_t(1953125);     // 10^9 / 2^9
    // for optimization in videoMemoryWait()
    videoMemoryLatencyCycles += int64_t(0xFFFFFFFFUL);
    daveCyclesPerNickCycle =
        (int64_t(daveFrequency) << 32) / int64_t(nickFrequency);
    cpuCyclesRemaining = int64_t(0);
    cpuSyncToNickCnt = int64_t(0);
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
      writingAudioOutput(false)
  {
    for (size_t i = 0; i < 4; i++)
      pageTable[i] = 0x00;
    for (size_t i = 0; i < 32; i++)
      romBitmap[i] = 0x00;
    for (size_t i = 0; i < 32; i++)
      ramBitmap[i] = 0x00;
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
    ramBitmap[31] |= 0xF0;
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
    while (nickCyclesRemaining > 0) {
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
    }
  }

  void Ep128VM::reset(bool isColdReset)
  {
    z80.reset();
    dave.reset();
    isRemote1On = false;
    isRemote2On = false;
    if (tape)
      tape->setIsMotorOn(false);
    if (isColdReset) {
      for (int i = 0; i < 256; i++) {
        if ((ramBitmap[i >> 3] & (1 << (i & 7))) != 0)
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
      if ((romBitmap[i >> 3] & (1 << (i & 7))) != 0) {
        memory.deleteSegment(uint8_t(i));
        romBitmap[i >> 3] &= uint8_t((1 << (i & 7)) ^ 0xFF);
      }
    }
    // resize RAM
    for (int i = 0; i <= (0xFF - int(nSegments)); i++) {
      if ((ramBitmap[i >> 3] & (1 << (i & 7))) != 0) {
        memory.deleteSegment(uint8_t(i));
        ramBitmap[i >> 3] &= uint8_t((1 << (i & 7)) ^ 0xFF);
      }
    }
    for (int i = 0xFF; i > (0xFF - int(nSegments)); i--) {
      if ((ramBitmap[i >> 3] & (1 << (i & 7))) == 0) {
        memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
        ramBitmap[i >> 3] |= uint8_t(1 << (i & 7));
      }
    }
    // cold reset
    this->reset(true);
  }

  void Ep128VM::loadROMSegment(uint8_t n, const char *fileName, size_t offs)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0') {
      // empty file name: delete segment
      if ((romBitmap[n >> 3] & (1 << (n & 7))) != 0) {
        memory.deleteSegment(uint8_t(n));
        romBitmap[n >> 3] &= uint8_t((1 << (n & 7)) ^ 0xFF);
      }
      else if ((ramBitmap[n >> 3] & (1 << (n & 7))) != 0) {
        memory.deleteSegment(uint8_t(n));
        ramBitmap[n >> 3] &= uint8_t((1 << (n & 7)) ^ 0xFF);
        // if there was RAM at the specified segment, relocate it
        for (int i = 0xFF; i >= 0x08; i--) {
          if (((romBitmap[i >> 3] | ramBitmap[i >> 3]) & (1 << (i & 7))) == 0) {
            memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
            ramBitmap[i >> 3] |= uint8_t(1 << (i & 7));
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
    if ((ramBitmap[n >> 3] & (1 << (n & 7))) != 0) {
      memory.loadSegment(n, true, &(buf.front()), 0x4000);
      ramBitmap[n >> 3] &= uint8_t((1 << (n & 7)) ^ 0xFF);
      romBitmap[n >> 3] |= uint8_t(1 << (n & 7));
      // if there was RAM at the specified segment, relocate it
      for (int i = 0xFF; i >= 0x08; i--) {
        if (((romBitmap[i >> 3] | ramBitmap[i >> 3]) & (1 << (i & 7))) == 0) {
          memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
          ramBitmap[i >> 3] |= uint8_t(1 << (i & 7));
          break;
        }
      }
    }
    else {
      // otherwise just load new segment, or replace existing ROM
      memory.loadSegment(n, true, &(buf.front()), 0x4000);
      romBitmap[n >> 3] |= uint8_t(1 << (n & 7));
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
      memoryTimingEnabled = isEnabled;
      cpuCyclesRemaining = int64_t(0);
      cpuSyncToNickCnt = int64_t(0);
    }
  }

  void Ep128VM::setKeyboardState(int keyCode, bool isPressed)
  {
    dave.setKeyboardState(keyCode, (isPressed ? 1 : 0));
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
    tapeSamplesRemaining = int64_t(0);
  }

  void Ep128VM::tapePlay()
  {
    tapePlaybackOn = true;
    tapeRecordOn = false;
    if (tape)
      tape->play();
  }

  void Ep128VM::tapeRecord()
  {
    tapePlaybackOn = true;
    tapeRecordOn = true;
    if (tape)
      tape->record();
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

