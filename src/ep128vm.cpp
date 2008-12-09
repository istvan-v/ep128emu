
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2008 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "memory.hpp"
#include "ioports.hpp"
#include "dave.hpp"
#include "nick.hpp"
#include "soundio.hpp"
#include "display.hpp"
#include "vm.hpp"
#include "ep128vm.hpp"
#include "debuglib.hpp"
#include "videorec.hpp"

#include <vector>
#include <ctime>

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

namespace Ep128 {

  inline void Ep128VM::updateCPUCycles(int cycles)
  {
    cpuCyclesRemaining -= (int64_t(cycles) << 32);
    if (memoryTimingEnabled) {
      while (cpuSyncToNickCnt >= cpuCyclesRemaining)
        cpuSyncToNickCnt -= cpuCyclesPerNickCycle;
    }
  }

  inline void Ep128VM::videoMemoryWait()
  {
    // use a fixed latency setting of 0.5625 Z80 cycles
    cpuCyclesRemaining -= (((cpuCyclesRemaining - cpuSyncToNickCnt)
                            + (int64_t(0xC8000000UL) << 1))
                           & (int64_t(-1) - int64_t(0xFFFFFFFFUL)));
    cpuSyncToNickCnt -= cpuCyclesPerNickCycle;
  }

  Ep128VM::Z80_::Z80_(Ep128VM& vm_)
    : Z80(),
      vm(vm_),
      defaultDeviceIsFILE(true)
  {
  }

  Ep128VM::Z80_::~Z80_()
  {
    closeAllFiles();
  }

  void Ep128VM::Z80_::ackInterruptFunction()
  {
    if (vm.spectrumEmulatorEnabled) {
      vm.spectrumEmulatorIOPorts[0] = 0xFF;
      vm.spectrumEmulatorIOPorts[1] = 0xFF;
      vm.spectrumEmulatorIOPorts[2] = 0xFF;
      vm.spectrumEmulatorIOPorts[3] = 0x1F;
      this->NMI_();
    }
  }

  uint8_t Ep128VM::Z80_::readMemory(uint16_t addr)
  {
    int     nCycles = 3;
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          nCycles++;
      }
      else
        vm.videoMemoryWait();
    }
    vm.updateCPUCycles(nCycles);
    return vm.memory.read(addr);
  }

  uint16_t Ep128VM::Z80_::readMemoryWord(uint16_t addr)
  {
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
    }
    else
      vm.updateCPUCycles(6);
    uint16_t  retval = vm.memory.read(addr);
    retval |= (uint16_t(vm.memory.read((addr + 1) & 0xFFFF) << 8));
    return retval;
  }

  uint8_t Ep128VM::Z80_::readOpcodeFirstByte()
  {
    int       nCycles = 4;
    uint16_t  addr = uint16_t(R.PC.W.l);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode < 2)
          nCycles++;
      }
      else
        vm.videoMemoryWait();
    }
    vm.updateCPUCycles(nCycles);
    if (!vm.singleStepMode)
      return vm.memory.readOpcode(addr);
    // single step mode
    return vm.checkSingleStepModeBreak();
  }

  uint8_t Ep128VM::Z80_::readOpcodeSecondByte()
  {
    int       nCycles = 4;
    uint16_t  addr = (uint16_t(R.PC.W.l) + uint16_t(1)) & uint16_t(0xFFFF);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode < 2)
          nCycles++;
      }
      else
        vm.videoMemoryWait();
    }
    vm.updateCPUCycles(nCycles);
    return vm.memory.readOpcode(addr);
  }

  uint8_t Ep128VM::Z80_::readOpcodeByte(int offset)
  {
    int       nCycles = 3;
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          nCycles++;
      }
      else
        vm.videoMemoryWait();
    }
    vm.updateCPUCycles(nCycles);
    return vm.memory.readOpcode(addr);
  }

  uint16_t Ep128VM::Z80_::readOpcodeWord(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
    }
    else
      vm.updateCPUCycles(6);
    uint16_t  retval = vm.memory.readOpcode(addr);
    retval |= (uint16_t(vm.memory.readOpcode((addr + 1) & 0xFFFF) << 8));
    return retval;
  }

  void Ep128VM::Z80_::writeMemory(uint16_t addr, uint8_t value)
  {
    int     nCycles = 3;
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          nCycles++;
      }
      else
        vm.videoMemoryWait();
    }
    vm.updateCPUCycles(nCycles);
    vm.memory.write(addr, value);
    if (vm.spectrumEmulatorEnabled) {
      uint32_t  tmp = uint32_t(addr) & 0x3FFFU;
      tmp = tmp | (uint32_t(vm.pageTable[addr >> 14]) << 14);
      if (tmp >= 0x3F9800U && tmp <= 0x3F9AFFU)
        vm.spectrumEmulatorNMI_AttrWrite(tmp, value);
    }
  }

  void Ep128VM::Z80_::writeMemoryWord(uint16_t addr, uint16_t value)
  {
    if (vm.spectrumEmulatorEnabled) {
      writeMemory(addr, uint8_t(value & 0xFF));
      writeMemory((addr + 1) & 0xFFFF, uint8_t(value >> 8));
      return;
    }
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
    }
    else
      vm.updateCPUCycles(6);
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

  void Ep128VM::Z80_::updateCycle()
  {
    vm.cpuCyclesRemaining -= (int64_t(1) << 32);
    // assume cpuFrequency > nickFrequency
    if (vm.cpuSyncToNickCnt >= vm.cpuCyclesRemaining)
      vm.cpuSyncToNickCnt -= vm.cpuCyclesPerNickCycle;
  }

  void Ep128VM::Z80_::updateCycles(int cycles)
  {
    vm.updateCPUCycles(cycles);
  }

  void Ep128VM::Z80_::tapePatch()
  {
    if ((R.PC.W.l & 0x3FFF) > 0x3FFC ||
        !(vm.fileIOEnabled &&
          vm.memory.isSegmentROM(vm.getMemoryPage(int(R.PC.W.l >> 14)))))
      return;
    if (vm.readMemory((uint16_t(R.PC.W.l) + 2) & 0xFFFF, true) != 0xFE)
      return;
    uint8_t n = vm.readMemory((uint16_t(R.PC.W.l) + 3) & 0xFFFF, true);
    if (n > 0x0F)
      return;
    if (vm.isRecordingDemo | vm.isPlayingDemo) {
      if (n >= 0x01 && n <= 0x0B) {
        // file I/O is disabled while recording or playing demo
        R.AF.B.h = 0xE7;
        return;
      }
    }
    std::map< uint8_t, std::FILE * >::iterator  i_;
    i_ = fileChannels.find(uint8_t(R.AF.B.h));
    if ((n >= 0x01 && n <= 0x02) && i_ != fileChannels.end()) {
      R.AF.B.h = 0xF9;          // channel already exists
      return;
    }
    if ((n >= 0x03 && n <= 0x0B) && i_ == fileChannels.end()) {
      R.AF.B.h = 0xFB;          // channel does not exist
      return;
    }
    switch (n) {
    case 0x00:                          // INTERRUPT
      R.AF.B.h = 0x00;
      break;
    case 0x01:                          // OPEN CHANNEL
      {
        std::string fileName;
        uint16_t    nameAddr = uint16_t(R.DE.W);
        uint8_t     nameLen = vm.readMemory(nameAddr, true);
        while (nameLen) {
          nameAddr = (nameAddr + 1) & 0xFFFF;
          char  c = char(vm.readMemory(nameAddr, true));
          if (c == '\0')
            break;
          fileName += c;
          nameLen--;
        }
        uint8_t   chn = uint8_t(R.AF.B.h);
        std::FILE *f = (std::FILE *) 0;
        R.AF.B.h = 0x00;
        try {
          int   err = vm.openFileInWorkingDirectory(f, fileName, "rb");
          if (err == 0)
            fileChannels.insert(std::pair< uint8_t, std::FILE * >(chn, f));
          else {
            switch (err) {
            case -2:
              R.AF.B.h = 0xA6;
              break;
            case -3:
              R.AF.B.h = 0xCF;
              break;
            case -4:
            case -5:
              R.AF.B.h = 0xA1;
              break;
            default:
              R.AF.B.h = 0xBA;
            }
          }
        }
        catch (...) {
          if (f)
            std::fclose(f);
          R.AF.B.h = 0xBA;
        }
      }
      break;
    case 0x02:                          // CREATE CHANNEL
      {
        std::string fileName;
        uint16_t    nameAddr = uint16_t(R.DE.W);
        uint8_t     nameLen = vm.readMemory(nameAddr, true);
        while (nameLen) {
          nameAddr = (nameAddr + 1) & 0xFFFF;
          char  c = char(vm.readMemory(nameAddr, true));
          if (c == '\0')
            break;
          fileName += c;
          nameLen--;
        }
        uint8_t   chn = uint8_t(R.AF.B.h);
        std::FILE *f = (std::FILE *) 0;
        R.AF.B.h = 0x00;
        try {
          int   err = vm.openFileInWorkingDirectory(f, fileName, "wb");
          if (err == 0)
            fileChannels.insert(std::pair< uint8_t, std::FILE * >(chn, f));
          else {
            switch (err) {
            case -2:
              R.AF.B.h = 0xA6;
              break;
            case -3:
              R.AF.B.h = 0xCF;
              break;
            case -4:
            case -5:
              R.AF.B.h = 0xA1;
              break;
            default:
              R.AF.B.h = 0xBA;
            }
          }
        }
        catch (...) {
          if (f)
            std::fclose(f);
          R.AF.B.h = 0xBA;
        }
      }
      break;
    case 0x03:                          // CLOSE CHANNEL
      if (std::fclose((*i_).second) == 0)
        R.AF.B.h = 0x00;
      else
        R.AF.B.h = 0xE4;
      (*i_).second = (std::FILE *) 0;
      fileChannels.erase(i_);
      break;
    case 0x04:                          // DESTROY CHANNEL
      // FIXME: this should remove the file;
      // for now, it is the same as "close channel"
      if (std::fclose((*i_).second) == 0)
        R.AF.B.h = 0x00;
      else
        R.AF.B.h = 0xE4;
      (*i_).second = (std::FILE *) 0;
      fileChannels.erase(i_);
      break;
    case 0x05:                          // READ CHARACTER
      {
        int   c = std::fgetc((*i_).second);
        R.AF.B.h = uint8_t(c != EOF ? 0x00 : 0xE4);
        R.BC.B.h = uint8_t(c & 0xFF);
      }
      break;
    case 0x06:                          // READ BLOCK
      R.AF.B.h = 0x00;
      while (R.BC.W != 0x0000) {
        int     c = std::fgetc((*i_).second);
        if (c == EOF) {
          R.AF.B.h = 0xE4;
          break;
        }
        writeUserMemory(uint16_t(R.DE.W), uint8_t(c & 0xFF));
        R.DE.W = (R.DE.W + 1) & 0xFFFF;
        R.BC.W = (R.BC.W - 1) & 0xFFFF;
      }
      break;
    case 0x07:                          // WRITE CHARACTER
      if (std::fputc(R.BC.B.h, (*i_).second) != EOF)
        R.AF.B.h = 0x00;
      else
        R.AF.B.h = 0xE4;
      if (std::fseek((*i_).second, 0L, SEEK_CUR) < 0)
        R.AF.B.h = 0xE4;
      break;
    case 0x08:                          // WRITE BLOCK
      R.AF.B.h = 0x00;
      while (R.BC.W != 0x0000) {
        uint8_t c = readUserMemory(uint16_t(R.DE.W));
        if (std::fputc(c, (*i_).second) == EOF) {
          R.AF.B.h = 0xE4;
          break;
        }
        R.DE.W = (R.DE.W + 1) & 0xFFFF;
        R.BC.W = (R.BC.W - 1) & 0xFFFF;
      }
      if (std::fseek((*i_).second, 0L, SEEK_CUR) < 0)
        R.AF.B.h = 0xE4;
      break;
    case 0x09:                          // CHANNEL READ STATUS
      R.BC.B.l = uint8_t(std::feof((*i_).second) == 0 ? 0x00 : 0xFF);
      R.AF.B.h = 0x00;
      break;
    case 0x0A:                          // SET/GET CHANNEL STATUS
      // TODO: implement this
      R.AF.B.h = 0xE7;
      R.BC.B.l = 0x00;
      break;
    case 0x0B:                          // SPECIAL FUNCTION
      R.AF.B.h = 0xE7;
      break;
    case 0x0C:                          // INITIALIZATION
      closeAllFiles();
      R.AF.B.h = 0x00;
      break;
    case 0x0D:                          // BUFFER MOVED
      R.AF.B.h = 0x00;
      break;
    case 0x0E:                          // get if default device is 'FILE:'
      R.AF.B.h = uint8_t(defaultDeviceIsFILE &&
                         !(vm.isRecordingDemo | vm.isPlayingDemo) ?
                         0x01 : 0x00);
      break;
    case 0x0F:                          // set if default device is 'FILE:'
      if (!(vm.isRecordingDemo | vm.isPlayingDemo))
        defaultDeviceIsFILE = (R.AF.B.h != 0x00);
      break;
    default:
      R.AF.B.h = 0xE7;
    }
  }

  uint8_t Ep128VM::Z80_::readUserMemory(uint16_t addr)
  {
    uint8_t   segment = vm.memory.readRaw(0x003FFFFCU | uint32_t(addr >> 14));
    uint32_t  addr_ = (uint32_t(segment) << 14) | uint32_t(addr & 0x3FFF);
    return vm.memory.readRaw(addr_);
  }

  void Ep128VM::Z80_::writeUserMemory(uint16_t addr, uint8_t value)
  {
    uint8_t   segment = vm.memory.readRaw(0x003FFFFCU | uint32_t(addr >> 14));
    uint32_t  addr_ = (uint32_t(segment) << 14) | uint32_t(addr & 0x3FFF);
    vm.memory.writeRaw(addr_, value);
  }

  void Ep128VM::Z80_::closeAllFiles()
  {
    std::map< uint8_t, std::FILE * >::iterator  i;
    for (i = fileChannels.begin(); i != fileChannels.end(); i++) {
      if ((*i).second != (std::FILE *) 0) {
        std::fclose((*i).second);
        (*i).second = (std::FILE *) 0;
      }
    }
    fileChannels.clear();
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
    if (!vm.memory.checkIgnoreBreakPoint(vm.z80.getReg().PC.W.l)) {
      int     bpType = int(isWrite) + 1;
      if (!isWrite && uint16_t(vm.z80.getReg().PC.W.l) == addr)
        bpType = 0;
      vm.breakPointCallback(vm.breakPointCallbackUserData, bpType, addr, value);
    }
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
    if (!vm.memory.checkIgnoreBreakPoint(vm.z80.getReg().PC.W.l)) {
      vm.breakPointCallback(vm.breakPointCallbackUserData,
                            int(isWrite) + 5, addr, value);
    }
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
    vm.memoryWaitMode = uint8_t(mode);
  }

  void Ep128VM::Dave_::setRemote1State(int state)
  {
    vm.isRemote1On = (state != 0);
    vm.setTapeMotorState(vm.isRemote1On || vm.isRemote2On);
  }

  void Ep128VM::Dave_::setRemote2State(int state)
  {
    vm.isRemote2On = (state != 0);
    vm.setTapeMotorState(vm.isRemote1On || vm.isRemote2On);
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
    vm.dave.setInt1State(int(newState));
  }

  void Ep128VM::Nick_::drawLine(const uint8_t *buf, size_t nBytes)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.drawLine(buf, nBytes);
    vm.videoCaptureHSyncFlag = true;
  }

  void Ep128VM::Nick_::vsyncStateChange(bool newState,
                                        unsigned int currentSlot_)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.vsyncStateChange(newState, currentSlot_);
    if (vm.videoCapture)
      vm.videoCapture->vsyncStateChange(newState, currentSlot_);
  }

  // --------------------------------------------------------------------------

  void Ep128VM::stopDemoPlayback()
  {
    if (isPlayingDemo) {
      isPlayingDemo = false;
      setCallback(&demoPlayCallback, this, false);
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
    setCallback(&demoRecordCallback, this, false);
    if (writeFile_ && demoFile != (Ep128Emu::File *) 0) {
      try {
        // put end of demo event
        writeDemoTimeCnt(demoBuffer, demoTimeCnt);
        demoTimeCnt = 0U;
        demoBuffer.writeByte(0x00);
        demoBuffer.writeByte(0x00);
        demoFile->addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_DEMO_STREAM,
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

  void Ep128VM::updateTimingParameters()
  {
    stopDemoPlayback();         // changing configuration implies stopping
    stopDemoRecording(false);   // any demo playback or recording
    cpuCyclesPerNickCycle =
        (int64_t(cpuFrequency) << 32) / int64_t(nickFrequency);
    daveCyclesPerNickCycle =
        (int64_t(daveFrequency) << 32) / int64_t(nickFrequency);
    if (haveTape()) {
      tapeSamplesPerNickCycle =
          (int64_t(getTapeSampleRate()) << 32) / int64_t(nickFrequency);
    }
    else
      tapeSamplesPerNickCycle = 0;
    cpuCyclesRemaining = 0;
    cpuSyncToNickCnt = 0;
    if (videoCapture)
      videoCapture->setClockFrequency(nickFrequency);
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

  uint8_t Ep128VM::nickPortReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    if (vm.memoryTimingEnabled)
      vm.videoMemoryWait();
    return (vm.nick.readPort(addr));
  }

  void Ep128VM::nickPortWriteCallback(void *userData,
                                      uint16_t addr, uint8_t value)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    if (vm.memoryTimingEnabled)
      vm.videoMemoryWait();
    vm.nick.writePort(addr, value);
  }

  uint8_t Ep128VM::nickPortDebugReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    return (vm.nick.readPortDebug(addr));
  }

  uint8_t Ep128VM::exdosPortReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    // floppy emulation is disabled while recording or playing demo
    if (!(vm.isRecordingDemo | vm.isPlayingDemo)) {
      if (vm.currentFloppyDrive <= 3) {
        Ep128Emu::WD177x& floppyDrive = vm.floppyDrives[vm.currentFloppyDrive];
        switch (addr) {
        case 0x00:
          return floppyDrive.readStatusRegister();
        case 0x01:
          return floppyDrive.readTrackRegister();
        case 0x02:
          return floppyDrive.readSectorRegister();
        case 0x03:
          return floppyDrive.readDataRegister();
        default:
          return uint8_t(  (floppyDrive.getInterruptRequestFlag() ? 0x3E : 0x3C)
                         | (floppyDrive.getDiskChangeFlag() ? 0x00 : 0x40)
                         | (floppyDrive.getDataRequestFlag() ? 0x80 : 0x00)
                         | (floppyDrive.haveDisk() ? 0x00 : 0x01));
        }
      }
    }
    return uint8_t((addr & 0x0008) ? 0x7D : 0x00);
  }

  void Ep128VM::exdosPortWriteCallback(void *userData,
                                       uint16_t addr, uint8_t value)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    // floppy emulation is disabled while recording or playing demo
    if (!(vm.isRecordingDemo | vm.isPlayingDemo)) {
      if (vm.currentFloppyDrive <= 3) {
        Ep128Emu::WD177x& floppyDrive = vm.floppyDrives[vm.currentFloppyDrive];
        switch (addr) {
        case 0x00:
          floppyDrive.writeCommandRegister(value);
          break;
        case 0x01:
          floppyDrive.writeTrackRegister(value);
          break;
        case 0x02:
          floppyDrive.writeSectorRegister(value);
          break;
        case 0x03:
          floppyDrive.writeDataRegister(value);
          break;
        }
      }
      if (addr & 0x0008) {
        vm.currentFloppyDrive = 0xFF;
        if ((value & 0x01) != 0)
          vm.currentFloppyDrive = 0;
        else if ((value & 0x02) != 0)
          vm.currentFloppyDrive = 1;
        else if ((value & 0x04) != 0)
          vm.currentFloppyDrive = 2;
        else if ((value & 0x08) != 0)
          vm.currentFloppyDrive = 3;
        else
          return;
        if ((value & 0x40) != 0)
          vm.floppyDrives[vm.currentFloppyDrive].clearDiskChangeFlag();
        vm.floppyDrives[vm.currentFloppyDrive].setSide((value & 0x10) >> 4);
      }
    }
  }

  uint8_t Ep128VM::exdosPortDebugReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    if (vm.currentFloppyDrive <= 3) {
      Ep128Emu::WD177x& floppyDrive = vm.floppyDrives[vm.currentFloppyDrive];
      switch (addr) {
      case 0x00:
        return floppyDrive.readStatusRegisterDebug();
      case 0x01:
        return floppyDrive.readTrackRegister();
      case 0x02:
        return floppyDrive.readSectorRegister();
      case 0x03:
        return floppyDrive.readDataRegisterDebug();
      default:
        return uint8_t(  (floppyDrive.getInterruptRequestFlag() ? 0x3E : 0x3C)
                       | (floppyDrive.getDiskChangeFlag() ? 0x00 : 0x40)
                       | (floppyDrive.getDataRequestFlag() ? 0x80 : 0x00)
                       | (floppyDrive.haveDisk() ? 0x00 : 0x01));
      }
    }
    return uint8_t((addr & 0x0008) ? 0x7D : 0x00);
  }

  uint8_t Ep128VM::spectrumEmulatorIOReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    switch (addr & 0xFF) {
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
      return vm.spectrumEmulatorIOPorts[addr & 3];
    case 0xFE:
      if (vm.spectrumEmulatorEnabled) {
        uint32_t  tmp = vm.z80.getReg().BC.B.h;
        if (vm.memory.readNoDebug(vm.z80.getReg().PC.W.l) == 0xDB)
          tmp = vm.z80.getReg().AF.B.h;
        tmp = ((tmp & 0xFFU) << 8) | (uint32_t(addr) & 0xFFU);
        tmp = (tmp & 0x3FFFU) | (uint32_t(vm.pageTable[tmp >> 14]) << 14);
        vm.spectrumEmulatorIOPorts[0] = uint8_t((tmp >> 8) & 0xFF);
        vm.spectrumEmulatorIOPorts[1] = uint8_t(tmp & 0xFF);
        vm.spectrumEmulatorIOPorts[2] = 0xFF;
        vm.spectrumEmulatorIOPorts[3] = 0x3F;
        vm.z80.NMI_();
      }
      break;
    }
    return 0xFF;
  }

  void Ep128VM::spectrumEmulatorIOWriteCallback(void *userData,
                                                uint16_t addr, uint8_t value)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    switch (addr & 0xFF) {
    case 0x44:
      vm.spectrumEmulatorEnabled = bool(value & 0x80);
      break;
    case 0xFE:
      if (vm.spectrumEmulatorEnabled) {
        uint32_t  tmp = vm.z80.getReg().BC.B.h;
        if (vm.memory.readNoDebug(vm.z80.getReg().PC.W.l) == 0xD3)
          tmp = vm.z80.getReg().AF.B.h;
        tmp = ((tmp & 0xFFU) << 8) | (uint32_t(addr) & 0xFFU);
        tmp = (tmp & 0x3FFFU) | (uint32_t(vm.pageTable[tmp >> 14]) << 14);
        vm.spectrumEmulatorIOPorts[0] = uint8_t((tmp >> 8) & 0xFF);
        vm.spectrumEmulatorIOPorts[1] = uint8_t(tmp & 0xFF);
        vm.spectrumEmulatorIOPorts[2] =
            uint8_t(((value & 0x06) >> 1) | ((value & 0x01) << 2)
                    | ((value & 0x40) >> 3) | (value & 0x30)
                    | ((value & 0x08) << 3) | ((value & 0x40) << 1));
        vm.spectrumEmulatorIOPorts[3] = 0x9F;
        vm.z80.NMI_();
      }
      break;
    }
  }

  uint8_t Ep128VM::cmosMemoryIOReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    switch (addr) {
    case 0x00:
      return vm.cmosMemoryRegisterSelect;
    case 0x01:
      {
        uint8_t tmp = vm.cmosMemoryRegisterSelect & 0x3F;
        switch (tmp) {
        case 0x0A:
          // NOTE: this register is assumed to be checked
          // before reading time and date
          vm.updateRTC();
          return 0x00;
        case 0x0B:
          return (vm.cmosMemory[tmp] & uint8_t(0x06));
        case 0x0C:
          return 0x00;
        case 0x0D:
          return 0x80;
        default:
          return (vm.cmosMemory[tmp]);
        }
      }
    }
    return 0xFF;
  }

  void Ep128VM::cmosMemoryIOWriteCallback(void *userData,
                                          uint16_t addr, uint8_t value)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    switch (addr) {
    case 0x00:
      vm.cmosMemoryRegisterSelect = value;
      break;
    case 0x01:
      vm.cmosMemory[vm.cmosMemoryRegisterSelect & 0x3F] = value;
      break;
    }
  }

  void Ep128VM::tapeCallback(void *userData)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    vm.tapeSamplesRemaining += vm.tapeSamplesPerNickCycle;
    if (vm.tapeSamplesRemaining > 0) {
      // assume tape sample rate < nickFrequency
      vm.tapeSamplesRemaining -= (int64_t(1) << 32);
      int   daveTapeInput = vm.runTape(int(vm.soundOutputSignal & 0xFFFFU));
      vm.dave.setTapeInput(daveTapeInput, daveTapeInput);
    }
  }

  void Ep128VM::demoPlayCallback(void *userData)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    while (!vm.demoTimeCnt) {
      if (vm.haveTape() &&
          vm.getIsTapeMotorOn() && vm.getTapeButtonState() != 0) {
        vm.stopDemoPlayback();
      }
      try {
        uint8_t evtType = vm.demoBuffer.readByte();
        uint8_t evtBytes = vm.demoBuffer.readByte();
        uint8_t evtData = 0;
        while (evtBytes) {
          evtData = vm.demoBuffer.readByte();
          evtBytes--;
        }
        switch (evtType) {
        case 0x00:
          vm.stopDemoPlayback();
          break;
        case 0x01:
          vm.dave.setKeyboardState(evtData, 1);
          break;
        case 0x02:
          vm.dave.setKeyboardState(evtData, 0);
          break;
        }
        vm.demoTimeCnt = readDemoTimeCnt(vm.demoBuffer);
      }
      catch (...) {
        vm.stopDemoPlayback();
      }
      if (!vm.isPlayingDemo) {
        vm.demoBuffer.clear();
        vm.demoTimeCnt = 0U;
        break;
      }
    }
    if (vm.demoTimeCnt)
      vm.demoTimeCnt--;
  }

  void Ep128VM::demoRecordCallback(void *userData)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    vm.demoTimeCnt++;
  }

  void Ep128VM::videoCaptureCallback(void *userData)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    if (vm.videoCaptureHSyncFlag) {
      vm.videoCaptureHSyncFlag = false;
      vm.videoCapture->horizontalSync();
    }
    vm.videoCapture->runOneCycle(vm.nick.getVideoOutput(),
                                 vm.soundOutputSignal);
  }

  uint8_t Ep128VM::checkSingleStepModeBreak()
  {
    uint16_t  addr = z80.getReg().PC.W.l;
    uint8_t   b0 = 0x00;
    if (singleStepMode == 3) {
      b0 = memory.readOpcode(addr);
      if (!singleStepMode)
        return b0;
    }
    else
      b0 = memory.readNoDebug(addr);
    int32_t   nxtAddr = int32_t(-1);
    if (singleStepMode == 2 || singleStepMode == 4) {
      // 'step over' or 'step into' mode
      if (singleStepModeNextAddr >= 0 &&
          int32_t(addr) != singleStepModeNextAddr) {
        return b0;
      }
      int     opType = 0;
      if (b0 < 0x80) {
        if (b0 == 0x10)
          opType = 0x12;                // DJNZ
        else if ((b0 & 0xE7) == 0x20)
          opType = 0x22;                // conditional JR
        else if (b0 == 0x76)
          opType = 0x71;                // HLT
      }
      else if (b0 != 0xED) {
        if (b0 == 0xCD)
          opType = 0x43;                // CALL
        else if ((b0 & 0xC7) == 0xC2 || (b0 & 0xC7) == 0xC4)
          opType = 0x33;                // conditional JP or CALL
        else if ((b0 & 0xC7) == 0xC7)
          opType = (b0 == 0xF7 ? 0x52 : 0x61);  // EXOS, RST
      }
      else {
        uint8_t b1 = memory.readNoDebug((addr + 1) & 0xFFFF);
        if ((b1 & 0xF4) == 0xB0)
          opType = 0x82;                // LDIR etc.
      }
      if (singleStepMode == 2) {
        // step over
        if (opType != 0)
          nxtAddr = int32_t((addr + uint16_t(opType & 0x0F)) & 0xFFFF);
      }
      else if (opType == 0x22 || opType == 0x33) {
        // step into
        uint16_t  tmp = memory.readNoDebug((addr + 1) & 0xFFFF);
        if (opType == 0x22) {           // conditional JR
          if (tmp & 0x80)
            tmp = tmp | 0xFF00;
          nxtAddr = int32_t((addr + 2 + tmp) & 0xFFFF);
        }
        else {                          // conditional JP or CALL
          tmp = tmp | (uint16_t(memory.readNoDebug((addr + 2) & 0xFFFF)) << 8);
          nxtAddr = int32_t(tmp);
        }
      }
    }
    singleStepModeNextAddr = nxtAddr;
    if (!memory.checkIgnoreBreakPoint(addr))
      breakPointCallback(breakPointCallbackUserData, 3, addr, b0);
    return b0;
  }

  void Ep128VM::spectrumEmulatorNMI_AttrWrite(uint32_t addr, uint8_t value)
  {
    spectrumEmulatorIOPorts[0] = uint8_t((addr >> 8) & 0xFF);
    spectrumEmulatorIOPorts[1] = uint8_t(addr & 0xFF);
    spectrumEmulatorIOPorts[2] =
        uint8_t(((value & 0x06) >> 1) | ((value & 0x01) << 2)
                | ((value & 0x40) >> 3) | (value & 0x30)
                | ((value & 0x08) << 3) | ((value & 0x40) << 1));
    spectrumEmulatorIOPorts[3] = 0xDF;
    z80.NMI_();
  }

  void Ep128VM::updateRTC()
  {
    std::time_t newTime = std::time((std::time_t *) 0);
    if (int64_t(newTime) == prvRTCTime)
      return;
    if (isRecordingDemo | isPlayingDemo)
      return;
    prvRTCTime = int64_t(newTime);
    std::tm   tmp;
    std::memcpy(&tmp, std::localtime(&newTime), sizeof(std::tm));
    cmosMemory[0x00] = uint8_t(tmp.tm_sec);
    cmosMemory[0x02] = uint8_t(tmp.tm_min);
    cmosMemory[0x04] = uint8_t(tmp.tm_hour);
    cmosMemory[0x06] = uint8_t(tmp.tm_wday + 1);
    cmosMemory[0x07] = uint8_t(tmp.tm_mday);
    cmosMemory[0x08] = uint8_t(tmp.tm_mon + 1);
    cmosMemory[0x09] = uint8_t((tmp.tm_year + 20) % 100);
    if (cmosMemory[0x0B] & 0x02) {
      // 12 hours mode
      int     h = cmosMemory[0x04];
      h = (h > 12 ? ((h - 12) | 0x80) : (h < 12 ? (h > 0 ? h : 12) : 0x8C));
      cmosMemory[0x04] = uint8_t(h);
    }
    if (!(cmosMemory[0x0B] & 0x04)) {
      // BCD mode
      for (int i = 0; i < 10; i++) {
        if (i < 6) {
          if (i & 1)
            continue;
          if (i == 4) {
            uint8_t b7 = cmosMemory[i] & 0x80;
            uint8_t b0to6 = cmosMemory[i] & 0x7F;
            cmosMemory[i] = uint8_t(((b0to6 / 10) << 4) | (b0to6 % 10)) | b7;
            continue;
          }
        }
        cmosMemory[i] =
            uint8_t(((cmosMemory[i] / 10) << 4) | (cmosMemory[i] % 10));
      }
    }
  }

  void Ep128VM::resetCMOSMemory()
  {
    cmosMemoryRegisterSelect = 0xFF;
    for (int i = 0; i < 63; i++)
      cmosMemory[i] = 0x00;
    cmosMemory[63] = 0xFF;
    prvRTCTime = int64_t(-1);
  }

  void Ep128VM::setCallback(void (*func)(void *userData), void *userData_,
                            bool isEnabled)
  {
    if (!func)
      return;
    const size_t  maxCallbacks = sizeof(callbacks) / sizeof(Ep128VMCallback);
    int     ndx = -1;
    for (size_t i = 0; i < maxCallbacks; i++) {
      if (callbacks[i].func == func && callbacks[i].userData == userData_) {
        ndx = int(i);
        break;
      }
    }
    if (ndx >= 0) {
      Ep128VMCallback   *prv = (Ep128VMCallback *) 0;
      Ep128VMCallback   *p = firstCallback;
      while (p) {
        if (p == &(callbacks[ndx])) {
          if (prv)
            prv->nxt = p->nxt;
          else
            firstCallback = p->nxt;
          break;
        }
        prv = p;
        p = p->nxt;
      }
      if (!isEnabled) {
        callbacks[ndx].func = (void (*)(void *)) 0;
        callbacks[ndx].userData = (void *) 0;
        callbacks[ndx].nxt = (Ep128VMCallback *) 0;
      }
    }
    if (!isEnabled)
      return;
    if (ndx < 0) {
      for (size_t i = 0; i < maxCallbacks; i++) {
        if (callbacks[i].func == (void (*)(void *)) 0) {
          ndx = int(i);
          break;
        }
      }
      if (ndx < 0)
        throw Ep128Emu::Exception("Ep128VM: too many callbacks");
    }
    callbacks[ndx].func = func;
    callbacks[ndx].userData = userData_;
    callbacks[ndx].nxt = (Ep128VMCallback *) 0;
    if (isEnabled) {
      Ep128VMCallback   *prv = (Ep128VMCallback *) 0;
      Ep128VMCallback   *p = firstCallback;
      while (p) {
        prv = p;
        p = p->nxt;
      }
      p = &(callbacks[ndx]);
      if (prv)
        prv->nxt = p;
      else
        firstCallback = p;
    }
  }

  // --------------------------------------------------------------------------

  Ep128VM::Ep128VM(Ep128Emu::VideoDisplay& display_,
                   Ep128Emu::AudioOutput& audioOutput_)
    : VirtualMachine(display_, audioOutput_),
      z80(*this),
      memory(*this),
      ioPorts(*this),
      dave(*this),
      nick(*this),
      cpuFrequency(4000000),
      daveFrequency(500000),
      nickFrequency(890625),
      nickCyclesRemaining(0),
      cpuCyclesPerNickCycle(0),
      cpuCyclesRemaining(0),
      cpuSyncToNickCnt(0),
      daveCyclesPerNickCycle(0),
      daveCyclesRemaining(0),
      memoryWaitMode(1),
      memoryTimingEnabled(true),
      singleStepMode(0),
      singleStepModeNextAddr(int32_t(-1)),
      tapeSamplesPerNickCycle(0),
      tapeSamplesRemaining(0),
      tapeCallbackFlag(false),
      isRemote1On(false),
      isRemote2On(false),
      videoCaptureHSyncFlag(false),
      soundOutputSignal(0U),
      demoFile((Ep128Emu::File *) 0),
      demoBuffer(),
      isRecordingDemo(false),
      isPlayingDemo(false),
      snapshotLoadFlag(false),
      demoTimeCnt(0U),
      currentFloppyDrive(0xFF),
      breakPointPriorityThreshold(0),
      cmosMemoryRegisterSelect(0xFF),
      spectrumEmulatorEnabled(false),
      prvRTCTime(int64_t(-1)),
      firstCallback((Ep128VMCallback *) 0),
      videoCapture((Ep128Emu::VideoCapture *) 0)
  {
    for (size_t i = 0; i < (sizeof(callbacks) / sizeof(Ep128VMCallback)); i++) {
      callbacks[i].func = (void (*)(void *)) 0;
      callbacks[i].userData = (void *) 0;
      callbacks[i].nxt = (Ep128VMCallback *) 0;
    }
    for (size_t i = 0; i < 4; i++) {
      pageTable[i] = 0x00;
      spectrumEmulatorIOPorts[i] = 0xFF;
    }
    updateTimingParameters();
    // register I/O callbacks
    ioPorts.setReadCallback(0xA0, 0xBF, &davePortReadCallback, this, 0xA0);
    ioPorts.setWriteCallback(0xA0, 0xBF, &davePortWriteCallback, this, 0xA0);
    ioPorts.setDebugReadCallback(0xB0, 0xB6, &davePortReadCallback, this, 0xA0);
    for (uint16_t i = 0x80; i <= 0x8F; i++) {
      ioPorts.setReadCallback(i, i, &nickPortReadCallback, this, (i & 0x8C));
      ioPorts.setWriteCallback(i, i, &nickPortWriteCallback, this, (i & 0x8C));
      ioPorts.setDebugReadCallback(i, i, &nickPortDebugReadCallback,
                                   this, (i & 0x8C));
    }
    // set up initial memory (segments 0xFC to 0xFF are always present)
    memory.loadSegment(0xFC, false, (uint8_t *) 0, 0);
    memory.loadSegment(0xFD, false, (uint8_t *) 0, 0);
    memory.loadSegment(0xFE, false, (uint8_t *) 0, 0);
    memory.loadSegment(0xFF, false, (uint8_t *) 0, 0);
    // hack to get some programs using interrupt mode 2 working
    z80.setVectorBase(0xFF);
    // use NICK colormap
    Ep128Emu::VideoDisplay::DisplayParameters
        dp(display.getDisplayParameters());
    dp.indexToRGBFunc = &Nick::convertPixelToRGB;
    display.setDisplayParameters(dp);
    setAudioConverterSampleRate(float(long(daveFrequency)));
    floppyDrives[0].setIsWD1773(false);
    floppyDrives[1].setIsWD1773(false);
    floppyDrives[2].setIsWD1773(false);
    floppyDrives[3].setIsWD1773(false);
    for (uint16_t i = 0x0010; i <= 0x001F; i++) {
      ioPorts.setReadCallback(i, i, &exdosPortReadCallback, this, i & 0x14);
      ioPorts.setWriteCallback(i, i, &exdosPortWriteCallback, this, i & 0x14);
      ioPorts.setDebugReadCallback(i, i,
                                   &exdosPortDebugReadCallback, this, i & 0x14);
    }
    ioPorts.setReadCallback(0x40, 0x43,
                            &spectrumEmulatorIOReadCallback, this, 0x00);
    ioPorts.setDebugReadCallback(0x40, 0x43,
                                 &spectrumEmulatorIOReadCallback, this, 0x00);
    ioPorts.setWriteCallback(0x44, 0x44,
                             &spectrumEmulatorIOWriteCallback, this, 0x00);
    ioPorts.setReadCallback(0xFE, 0xFE,
                            &spectrumEmulatorIOReadCallback, this, 0x00);
    ioPorts.setWriteCallback(0xFE, 0xFE,
                             &spectrumEmulatorIOWriteCallback, this, 0x00);
    ioPorts.setReadCallback(0x7E, 0x7F, &cmosMemoryIOReadCallback, this, 0x7E);
    ioPorts.setDebugReadCallback(0x7E, 0x7F,
                                 &cmosMemoryIOReadCallback, this, 0x7E);
    ioPorts.setWriteCallback(0x7E, 0x7F,
                             &cmosMemoryIOWriteCallback, this, 0x7E);
    // reset
    this->reset(true);
  }

  Ep128VM::~Ep128VM()
  {
    if (videoCapture) {
      delete videoCapture;
      videoCapture = (Ep128Emu::VideoCapture *) 0;
    }
    try {
      // FIXME: cannot handle errors here
      stopDemo();
    }
    catch (...) {
    }
  }

  void Ep128VM::run(size_t microseconds)
  {
    Ep128Emu::VirtualMachine::run(microseconds);
    if (snapshotLoadFlag) {
      snapshotLoadFlag = false;
      // if just loaded a snapshot, and not playing a demo,
      // clear keyboard state
      if (!isPlayingDemo) {
        for (int i = 0; i < 128; i++)
          dave.setKeyboardState(i, 0);
      }
    }
    bool    newTapeCallbackFlag =
        (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0);
    if (newTapeCallbackFlag != tapeCallbackFlag) {
      tapeCallbackFlag = newTapeCallbackFlag;
      if (!tapeCallbackFlag)
        dave.setTapeInput(0, 0);
      setCallback(&tapeCallback, this, tapeCallbackFlag);
    }
    nickCyclesRemaining +=
        ((int64_t(microseconds) << 26) * int64_t(nickFrequency)
         / int64_t(15625));     // 10^6 / 2^6
    if (nickCyclesRemaining < (int64_t(1) << 32))
      return;
    int     cycleCnt = int(nickCyclesRemaining >> 32);
    nickCyclesRemaining -= (int64_t(cycleCnt) << 32);
    do {
      Ep128VMCallback   *p = firstCallback;
      while (p) {
        Ep128VMCallback *nxt = p->nxt;
        p->func(p->userData);
        p = nxt;
      }
      daveCyclesRemaining += daveCyclesPerNickCycle;
      while (daveCyclesRemaining >= 0) {
        daveCyclesRemaining -= (int64_t(1) << 32);
        soundOutputSignal = dave.runOneCycle();
        sendAudioOutput(soundOutputSignal);
      }
      cpuCyclesRemaining += cpuCyclesPerNickCycle;
      cpuSyncToNickCnt += cpuCyclesPerNickCycle;
      while (cpuCyclesRemaining > 0)
        z80.executeInstruction();
      nick.runOneSlot();
    } while (--cycleCnt);
  }

  void Ep128VM::reset(bool isColdReset)
  {
    stopDemoPlayback();         // TODO: should be recorded as an event ?
    stopDemoRecording(false);
    z80.reset();
    ioPorts.reset();
    for (uint16_t i = 0x00A0; i <= 0x00BF; i++)
      ioPorts.writeDebug(i, 0x00);
    dave.reset(isColdReset);
    isRemote1On = false;
    isRemote2On = false;
    setTapeMotorState(false);
    currentFloppyDrive = 0xFF;
    floppyDrives[0].reset();
    floppyDrives[1].reset();
    floppyDrives[2].reset();
    floppyDrives[3].reset();
    ioPorts.writeDebug(0x44, 0x00);     // disable Spectrum emulator
    for (int i = 0; i < 4; i++)
      spectrumEmulatorIOPorts[i] = 0xFF;
    cmosMemoryRegisterSelect = 0xFF;
    if (isColdReset) {
      nick.randomizeRegisters();
      resetCMOSMemory();
      for (int i = 0; i < 256; i++) {
        if (memory.isSegmentRAM(uint8_t(i)))
          memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
      }
    }
  }

  void Ep128VM::resetMemoryConfiguration(size_t memSize)
  {
    stopDemo();
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
    stopDemo();
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

  void Ep128VM::setVideoFrequency(size_t freq_)
  {
    size_t  freq;
    // allow refresh rates in the range 10 Hz to 100 Hz
    freq = (freq_ > 178125 ? (freq_ < 1781250 ? freq_ : 1781250) : 178125);
    if (nickFrequency != freq) {
      nickFrequency = freq;
      updateTimingParameters();
    }
  }

  void Ep128VM::setSoundClockFrequency(size_t freq_)
  {
    size_t  freq;
    freq = (freq_ > 250000 ? (freq_ < 1000000 ? freq_ : 1000000) : 250000);
    if (daveFrequency != freq) {
      daveFrequency = freq;
      updateTimingParameters();
      setAudioConverterSampleRate(float(long(daveFrequency)));
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
      dave.setKeyboardState(keyCode, int(isPressed));
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

  void Ep128VM::getVMStatus(VMStatus& vmStatus_)
  {
    vmStatus_.tapeReadOnly = getIsTapeReadOnly();
    vmStatus_.tapePosition = getTapePosition();
    vmStatus_.tapeLength = getTapeLength();
    vmStatus_.tapeSampleRate = getTapeSampleRate();
    vmStatus_.tapeSampleSize = getTapeSampleSize();
    uint32_t  n = 0U;
    for (int i = 3; i >= 0; i--) {
      n = n << 8;
      n |= uint32_t(floppyDrives[i].getLEDState());
    }
    vmStatus_.floppyDriveLEDState = n;
    vmStatus_.isPlayingDemo = isPlayingDemo;
    if (demoFile != (Ep128Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    vmStatus_.isRecordingDemo = isRecordingDemo;
  }

  void Ep128VM::openVideoCapture(
      int frameRate_,
      bool yuvFormat_,
      void (*errorCallback_)(void *userData, const char *msg),
      void (*fileNameCallback_)(void *userData, std::string& fileName),
      void *userData_)
  {
    if (!videoCapture) {
      if (yuvFormat_) {
        videoCapture = new Ep128Emu::VideoCapture_YV12(&Nick::convertPixelToRGB,
                                                       frameRate_);
      }
      else {
        videoCapture = new Ep128Emu::VideoCapture_RLE8(&Nick::convertPixelToRGB,
                                                       frameRate_);
      }
      videoCapture->setClockFrequency(nickFrequency);
      setCallback(&videoCaptureCallback, this, true);
    }
    videoCapture->setErrorCallback(errorCallback_, userData_);
    videoCapture->setFileNameCallback(fileNameCallback_, userData_);
  }

  void Ep128VM::setVideoCaptureFile(const std::string& fileName_)
  {
    if (!videoCapture) {
      throw Ep128Emu::Exception("internal error: "
                                "video capture object does not exist");
    }
    videoCapture->openFile(fileName_.c_str());
  }

  void Ep128VM::closeVideoCapture()
  {
    if (videoCapture) {
      setCallback(&videoCaptureCallback, this, false);
      delete videoCapture;
      videoCapture = (Ep128Emu::VideoCapture *) 0;
    }
  }

  void Ep128VM::setDiskImageFile(int n, const std::string& fileName_,
                                 int nTracks_, int nSides_,
                                 int nSectorsPerTrack_)
  {
    if (n < 0 || n > 3)
      throw Ep128Emu::Exception("invalid floppy drive number");
    floppyDrives[n].setDiskImageFile(fileName_,
                                     nTracks_, nSides_, nSectorsPerTrack_);
  }

  uint32_t Ep128VM::getFloppyDriveLEDState()
  {
    uint32_t  n = 0U;
    for (int i = 3; i >= 0; i--) {
      n = n << 8;
      n |= uint32_t(floppyDrives[i].getLEDState());
    }
    return n;
  }

  void Ep128VM::setTapeFileName(const std::string& fileName)
  {
    Ep128Emu::VirtualMachine::setTapeFileName(fileName);
    setTapeMotorState(isRemote1On || isRemote2On);
    if (haveTape()) {
      tapeSamplesPerNickCycle =
          (int64_t(getTapeSampleRate()) << 32) / int64_t(nickFrequency);
    }
    tapeSamplesRemaining = 0;
  }

  void Ep128VM::tapePlay()
  {
    Ep128Emu::VirtualMachine::tapePlay();
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void Ep128VM::tapeRecord()
  {
    Ep128Emu::VirtualMachine::tapeRecord();
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void Ep128VM::setBreakPoints(const Ep128Emu::BreakPointList& bpList)
  {
    for (size_t i = 0; i < bpList.getBreakPointCnt(); i++) {
      const Ep128Emu::BreakPoint& bp = bpList.getBreakPoint(i);
      if (bp.isIO()) {
        ioPorts.setBreakPoint(bp.addr(), bp.priority(),
                              bp.isRead(), bp.isWrite());
      }
      else if (bp.haveSegment()) {
        memory.setBreakPoint(bp.segment(), bp.addr(), bp.priority(),
                             bp.isRead(), bp.isWrite(), bp.isExecute(),
                             bp.isIgnore());
      }
      else {
        memory.setBreakPoint(bp.addr(), bp.priority(),
                             bp.isRead(), bp.isWrite(), bp.isExecute(),
                             bp.isIgnore());
      }
    }
  }

  Ep128Emu::BreakPointList Ep128VM::getBreakPoints()
  {
    Ep128Emu::BreakPointList  bpl1(ioPorts.getBreakPointList());
    Ep128Emu::BreakPointList  bpl2(memory.getBreakPointList());
    for (size_t i = 0; i < bpl1.getBreakPointCnt(); i++) {
      const Ep128Emu::BreakPoint& bp = bpl1.getBreakPoint(i);
      bpl2.addIOBreakPoint(bp.addr(), bp.isRead(), bp.isWrite(), bp.priority());
    }
    return bpl2;
  }

  void Ep128VM::clearBreakPoints()
  {
    memory.clearAllBreakPoints();
    ioPorts.clearBreakPoints();
  }

  void Ep128VM::setBreakPointPriorityThreshold(int n)
  {
    breakPointPriorityThreshold = uint8_t(n > 0 ? (n < 4 ? n : 4) : 0);
    if (!(singleStepMode == 1 || singleStepMode == 2)) {
      memory.setBreakPointPriorityThreshold(n);
      ioPorts.setBreakPointPriorityThreshold(n);
    }
  }

  void Ep128VM::setSingleStepMode(int mode_)
  {
    mode_ = ((mode_ >= 0 && mode_ <= 4) ? mode_ : 0);
    if (mode_ == int(singleStepMode))
      return;
    singleStepMode = uint8_t(mode_);
    singleStepModeNextAddr = int32_t(-1);
    {
      int     tmp = 4;
      if (mode_ == 0 || mode_ == 3)
        tmp = int(breakPointPriorityThreshold);
      memory.setBreakPointPriorityThreshold(tmp);
      ioPorts.setBreakPointPriorityThreshold(tmp);
    }
    if (mode_ != 2 && mode_ != 4)
      return;
    // "step over" or "step into" mode:
    uint16_t  nxtAddr = uint16_t(z80.getReg().PC.W.l);
    int       opNum = readMemory(nxtAddr, true);
    nxtAddr = (nxtAddr + 1) & 0xFFFF;
    if (opNum == 0xED) {
      opNum = 0xED00 | int(readMemory(nxtAddr, true));
      nxtAddr = (nxtAddr + 1) & 0xFFFF;
    }
    int     opType = 0;
    if (opNum == 0x10)
      opType = 0x11;                    // DJNZ
    else if ((opNum | 0x18) == 0x38)
      opType = 0x21;                    // conditional JR
    else if ((opNum | 0x38) == 0xFA || (opNum | 0x38) == 0xFC)
      opType = 0x32;                    // conditional JP or CALL
    else if (opNum == 0xCD)
      opType = 0x42;                    // CALL
    else if (opNum == 0xF7)
      opType = 0x51;                    // EXOS
    else if ((opNum | 0x38) == 0xFF)
      opType = 0x60;                    // RST
    else if (opNum == 0x76)
      opType = 0x70;                    // HLT
    else if ((opNum | 0x0B) == 0xEDBB)
      opType = 0x80;                    // LDIR etc.
    if (mode_ == 2) {                   // step over
      if (opType != 0) {
        singleStepModeNextAddr =
            int32_t((nxtAddr + uint16_t(opType & 0x0F)) & 0xFFFF);
      }
    }
    else {                              // step into, find target address
      if (opType == 0x21) {
        // relative jump
        uint16_t  tmp = readMemory(nxtAddr, true);
        if (tmp & 0x80)
          tmp = tmp | 0xFF00;
        singleStepModeNextAddr = int32_t((nxtAddr + 1 + tmp) & 0xFFFF);
      }
      else if (opType == 0x32) {
        // absolute jump
        uint16_t  tmp = readMemory(nxtAddr, true);
        nxtAddr = (nxtAddr + 1) & 0xFFFF;
        tmp = tmp | (uint16_t(readMemory(nxtAddr, true)) << 8);
        singleStepModeNextAddr = int32_t(tmp);
      }
    }
  }

  void Ep128VM::setSingleStepModeNextAddress(int32_t addr)
  {
    if ((singleStepMode != 2 && singleStepMode != 4) || addr < 0)
      addr = int32_t(-1);
    else
      addr &= int32_t(0xFFFF);
    singleStepModeNextAddr = addr;
  }

  uint8_t Ep128VM::getMemoryPage(int n) const
  {
    return memory.getPage(uint8_t(n & 3));
  }

  uint8_t Ep128VM::readMemory(uint32_t addr, bool isCPUAddress) const
  {
    if (isCPUAddress)
      addr = ((uint32_t(memory.getPage(uint8_t(addr >> 14) & uint8_t(3))) << 14)
              | (addr & uint32_t(0x3FFF)));
    else
      addr &= uint32_t(0x003FFFFF);
    return memory.readRaw(addr);
  }

  void Ep128VM::writeMemory(uint32_t addr, uint8_t value, bool isCPUAddress)
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    if (isCPUAddress)
      addr = ((uint32_t(memory.getPage(uint8_t(addr >> 14) & uint8_t(3))) << 14)
              | (addr & uint32_t(0x3FFF)));
    else
      addr &= uint32_t(0x003FFFFF);
    memory.writeRaw(addr, value);
  }

  uint8_t Ep128VM::readIOPort(uint16_t addr) const
  {
    return ioPorts.readDebug(addr);
  }

  void Ep128VM::writeIOPort(uint16_t addr, uint8_t value)
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    ioPorts.writeDebug(addr, value);
  }

  uint16_t Ep128VM::getProgramCounter() const
  {
    return z80.getProgramCounter();
  }

  void Ep128VM::setProgramCounter(uint16_t addr)
  {
    if (addr != z80.getProgramCounter()) {
      if (isRecordingDemo | isPlayingDemo) {
        stopDemoPlayback();
        stopDemoRecording(false);
      }
      z80.setProgramCounter(addr);
    }
  }

  uint16_t Ep128VM::getStackPointer() const
  {
    return uint16_t(z80.getReg().SP.W);
  }

  void Ep128VM::listCPURegisters(std::string& buf) const
  {
    char    tmpBuf[256];
    const Z80_REGISTERS&  r = z80.getReg();
    std::sprintf(
        &(tmpBuf[0]),
        " PC   AF   BC   DE   HL   SP   IX   IY    F   ........\n"
        "%04X %04X %04X %04X %04X %04X %04X %04X   F'  ........\n"
        "      AF'  BC'  DE'  HL'  IM   I    R    IFF1 .\n"
        "     %04X %04X %04X %04X  %02X   %02X   %02X   IFF2 .",
        (unsigned int) z80.getProgramCounter(), (unsigned int) r.AF.W,
        (unsigned int) r.BC.W, (unsigned int) r.DE.W,
        (unsigned int) r.HL.W, (unsigned int) r.SP.W,
        (unsigned int) r.IX.W, (unsigned int) r.IY.W,
        (unsigned int) r.altAF.W, (unsigned int) r.altBC.W,
        (unsigned int) r.altDE.W, (unsigned int) r.altHL.W,
        (unsigned int) r.IM, (unsigned int) r.I, (unsigned int) r.R);
    static const char *z80Flags_ = "SZ1H1VNC";
    for (int i = 0; i < 8; i++) {
      tmpBuf[i + 46] = ((r.AF.B.l & uint8_t(128 >> i)) ? z80Flags_[i] : '-');
      tmpBuf[i + 101] =
          ((r.altAF.B.l & uint8_t(128 >> i)) ? z80Flags_[i] : '-');
    }
    tmpBuf[156] = '0' + char(bool(r.IFF1));
    tmpBuf[204] = '0' + char(bool(r.IFF2));
    buf = &(tmpBuf[0]);
  }

  void Ep128VM::listIORegisters(std::string& buf) const
  {
    static const uint8_t  ioPortTable_[] = {
      0x10, 0x11, 0x12, 0x13, 0x18, 0x40, 0x41, 0x42,
      0x43, 0x44, 0x80, 0x81, 0x82, 0x83, 0xA0, 0xA1,
      0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9,
      0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB4, 0xB5,
      0xB6, 0xBF
    };
    unsigned int  tmpBuf[40];
    for (size_t i = 0; i < (sizeof(ioPortTable_) / sizeof(uint8_t)); i++)
      tmpBuf[i] = ioPorts.readDebug(ioPortTable_[i]);
    char    tmpBuf2[320];
    char    *bufp = &(tmpBuf2[0]);
    int     n;
    n = std::sprintf(bufp,
                     "Disk  WD: %02X(%02X) %02X %02X %02X  EXDOS: %02X(%02X)\n",
                     tmpBuf[0],
                     (unsigned int) ioPorts.getLastValueWritten(0x10),
                     tmpBuf[1], tmpBuf[2], tmpBuf[3], tmpBuf[4],
                     (unsigned int) ioPorts.getLastValueWritten(0x18));
    bufp = bufp + n;
    n = std::sprintf(bufp,
                     "ZX    40: %02X %02X %02X %02X  44: %02X\n",
                     tmpBuf[5], tmpBuf[6], tmpBuf[7], tmpBuf[8], tmpBuf[9]);
    bufp = bufp + n;
    n = std::sprintf(bufp,
                     "Nick  80: %02X %02X %02X %02X  Slot: %02X\n",
                     tmpBuf[10], tmpBuf[11], tmpBuf[12], tmpBuf[13],
                     (unsigned int) nick.getCurrentSlot());
    bufp = bufp + n;
    n = std::sprintf(bufp,
                     "Nick  LPB: %04X,%02X  LD1: %04X  LD2: %04X\n",
                     (unsigned int) nick.getLPBAddress(),
                     (unsigned int) nick.getLPBLine(),
                     (unsigned int) nick.getLD1Address(),
                     (unsigned int) nick.getLD2Address());
    bufp = bufp + n;
    n = std::sprintf(bufp,
                     "Dave  A0: %02X %02X %02X %02X  %02X %02X %02X %02X\n"
                     "Dave  A8: %02X %02X %02X %02X  %02X %02X %02X %02X\n",
                     tmpBuf[14], tmpBuf[15], tmpBuf[16], tmpBuf[17],
                     tmpBuf[18], tmpBuf[19], tmpBuf[20], tmpBuf[21],
                     tmpBuf[22], tmpBuf[23], tmpBuf[24], tmpBuf[25],
                     tmpBuf[26], tmpBuf[27], tmpBuf[28], tmpBuf[29]);
    bufp = bufp + n;
    n = std::sprintf(bufp,
                     "Dave  B4: %02X(%02X) %02X(%02X) %02X(%02X)  BF: %02X",
                     tmpBuf[30],
                     (unsigned int) ioPorts.getLastValueWritten(0xB4),
                     tmpBuf[31],
                     (unsigned int) ioPorts.getLastValueWritten(0xB5),
                     tmpBuf[32],
                     (unsigned int) ioPorts.getLastValueWritten(0xB6),
                     tmpBuf[33]);
    buf = &(tmpBuf2[0]);
  }

  uint32_t Ep128VM::disassembleInstruction(std::string& buf,
                                           uint32_t addr, bool isCPUAddress,
                                           int32_t offs) const
  {
    return Z80Disassembler::disassembleInstruction(buf, (*this),
                                                   addr, isCPUAddress, offs);
  }

  Z80_REGISTERS& Ep128VM::getZ80Registers()
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    return z80.getReg();
  }

  void Ep128VM::saveState(Ep128Emu::File& f)
  {
    ioPorts.saveState(f);
    memory.saveState(f);
    nick.saveState(f);
    dave.saveState(f);
    z80.saveState(f);
    {
      Ep128Emu::File::Buffer  buf;
      buf.setPosition(0);
      buf.writeUInt32(0x01000001);      // version number
      buf.writeByte(memory.getPage(0));
      buf.writeByte(memory.getPage(1));
      buf.writeByte(memory.getPage(2));
      buf.writeByte(memory.getPage(3));
      buf.writeByte(memoryWaitMode & 3);
      buf.writeUInt32(uint32_t(cpuFrequency));
      buf.writeUInt32(uint32_t(daveFrequency));
      buf.writeUInt32(uint32_t(nickFrequency));
      buf.writeUInt32(62U);     // video memory latency for compatibility only
      buf.writeBoolean(memoryTimingEnabled);
      buf.writeInt64(cpuCyclesRemaining);
      buf.writeInt64(cpuSyncToNickCnt);
      buf.writeInt64(daveCyclesRemaining + int64_t(1)); // +1 for compatibility
      buf.writeBoolean(spectrumEmulatorEnabled);
      for (int i = 0; i < 4; i++)
        buf.writeByte(spectrumEmulatorIOPorts[i]);
      buf.writeByte(cmosMemoryRegisterSelect);
      buf.writeInt64(prvRTCTime);
      for (int i = 0; i < 64; i++)
        buf.writeByte(cmosMemory[i]);
      f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_STATE, buf);
    }
  }

  void Ep128VM::saveMachineConfiguration(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    buf.writeUInt32(uint32_t(cpuFrequency));
    buf.writeUInt32(uint32_t(daveFrequency));
    buf.writeUInt32(uint32_t(nickFrequency));
    buf.writeUInt32(62U);       // video memory latency for compatibility only
    buf.writeBoolean(memoryTimingEnabled);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_CONFIG, buf);
  }

  void Ep128VM::recordDemo(Ep128Emu::File& f)
  {
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    isRemote1On = false;
    isRemote2On = false;
    setTapeMotorState(false);
    dave.setTapeInput(0, 0);
    stopDemo();
    for (int i = 0; i < 128; i++)
      dave.setKeyboardState(i, 0);
    // floppy emulation is disabled while recording or playing demo
    currentFloppyDrive = 0xFF;
    floppyDrives[0].reset();
    floppyDrives[1].reset();
    floppyDrives[2].reset();
    floppyDrives[3].reset();
    z80.closeAllFiles();
    // save full snapshot, including timing and clock frequency settings
    saveMachineConfiguration(f);
    saveState(f);
    demoBuffer.clear();
    demoBuffer.writeUInt32(0x00020006); // version 2.0.6
    demoFile = &f;
    isRecordingDemo = true;
    setCallback(&demoRecordCallback, this, true);
    demoTimeCnt = 0U;
  }

  void Ep128VM::stopDemo()
  {
    stopDemoPlayback();
    stopDemoRecording(true);
  }

  bool Ep128VM::getIsRecordingDemo()
  {
    if (demoFile != (Ep128Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    return isRecordingDemo;
  }

  bool Ep128VM::getIsPlayingDemo() const
  {
    return isPlayingDemo;
  }

  // --------------------------------------------------------------------------

  void Ep128VM::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000001 && version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible ep128 snapshot format");
    }
    isRemote1On = false;
    isRemote2On = false;
    setTapeMotorState(false);
    stopDemo();
    snapshotLoadFlag = true;
    // reset floppy emulation, as its state is not saved
    currentFloppyDrive = 0xFF;
    floppyDrives[0].reset();
    floppyDrives[1].reset();
    floppyDrives[2].reset();
    floppyDrives[3].reset();
    z80.closeAllFiles();
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
      (void) tmpVideoMemoryLatency;
      bool      tmpMemoryTimingEnabled = buf.readBoolean();
      int64_t   tmp[3];
      tmp[0] = buf.readInt64();
      tmp[1] = buf.readInt64();
      tmp[2] = buf.readInt64();
      if (tmpCPUFrequency == cpuFrequency &&
          tmpDaveFrequency == daveFrequency &&
          tmpNickFrequency == nickFrequency &&
          tmpMemoryTimingEnabled == memoryTimingEnabled) {
        cpuCyclesRemaining = tmp[0];
        cpuSyncToNickCnt = tmp[1];
        daveCyclesRemaining = tmp[2] - int64_t(1);      // -1 for compatibility
      }
      else {
        cpuCyclesRemaining = 0;
        cpuSyncToNickCnt = 0;
        daveCyclesRemaining = 0;
      }
      if (version == 0x01000001) {
        ioPorts.writeDebug(0x44, uint8_t(buf.readBoolean()) << 7);
        for (int i = 0; i < 4; i++)
          spectrumEmulatorIOPorts[i] = buf.readByte();
        cmosMemoryRegisterSelect = buf.readByte();
        prvRTCTime = buf.readInt64();
        for (int i = 0; i < 64; i++)
          cmosMemory[i] = buf.readByte();
      }
      else {
        // loading snapshot saved by old version without Spectrum emulator
        // and real time clock support
        ioPorts.writeDebug(0x44, 0x00);
        for (int i = 0; i < 4; i++)
          spectrumEmulatorIOPorts[i] = 0xFF;
        resetCMOSMemory();
      }
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "ep128 snapshot data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Ep128VM::loadMachineConfiguration(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible ep128 "
                                "machine configuration format");
    }
    try {
      setCPUFrequency(buf.readUInt32());
      setSoundClockFrequency(buf.readUInt32());
      setVideoFrequency(buf.readUInt32());
      uint32_t  tmpVideoMemoryLatency = buf.readUInt32();
      (void) tmpVideoMemoryLatency;
      setEnableMemoryTimingEmulation(buf.readBoolean());
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "ep128 machine configuration data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Ep128VM::loadDemo(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
#if 0
    if (version != 0x00020006) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible ep128 demo format");
    }
#endif
    (void) version;
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    isRemote1On = false;
    isRemote2On = false;
    setTapeMotorState(false);
    dave.setTapeInput(0, 0);
    stopDemo();
    for (int i = 0; i < 128; i++)
      dave.setKeyboardState(i, 0);
    // floppy emulation is disabled while recording or playing demo
    currentFloppyDrive = 0xFF;
    floppyDrives[0].reset();
    floppyDrives[1].reset();
    floppyDrives[2].reset();
    floppyDrives[3].reset();
    // initialize time counter with first delta time
    demoTimeCnt = readDemoTimeCnt(buf);
    isPlayingDemo = true;
    setCallback(&demoPlayCallback, this, true);
    // copy any remaining demo data to local buffer
    demoBuffer.clear();
    demoBuffer.writeData(buf.getData() + buf.getPosition(),
                         buf.getDataSize() - buf.getPosition());
    demoBuffer.setPosition(0);
  }

  class ChunkType_Ep128VMConfig : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_Ep128VMConfig(Ep128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Ep128VMConfig()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_CONFIG;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadMachineConfiguration(buf);
    }
  };

  class ChunkType_Ep128VMSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_Ep128VMSnapshot(Ep128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Ep128VMSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  class ChunkType_DemoStream : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_DemoStream(Ep128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_DemoStream()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_DEMO_STREAM;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadDemo(buf);
    }
  };

  void Ep128VM::registerChunkTypes(Ep128Emu::File& f)
  {
    ChunkType_Ep128VMConfig   *p1 = (ChunkType_Ep128VMConfig *) 0;
    ChunkType_Ep128VMSnapshot *p2 = (ChunkType_Ep128VMSnapshot *) 0;
    ChunkType_DemoStream      *p3 = (ChunkType_DemoStream *) 0;

    try {
      p1 = new ChunkType_Ep128VMConfig(*this);
      f.registerChunkType(p1);
      p1 = (ChunkType_Ep128VMConfig *) 0;
      p2 = new ChunkType_Ep128VMSnapshot(*this);
      f.registerChunkType(p2);
      p2 = (ChunkType_Ep128VMSnapshot *) 0;
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
    ioPorts.registerChunkType(f);
    z80.registerChunkType(f);
    memory.registerChunkType(f);
    dave.registerChunkType(f);
    nick.registerChunkType(f);
  }

}       // namespace Ep128

