
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
#include "ide.hpp"
#ifdef ENABLE_SDEXT
#  include "sdext.hpp"
#endif

#include <vector>
#include <ctime>

namespace Ep128 {

  inline void Ep128VM::updateCPUCycles(int cycles)
  {
    cpuCyclesRemaining -= (int64_t(cycles) << 32);
  }

  EP128EMU_REGPARM1 void Ep128VM::videoMemoryWait()
  {
    cpuCyclesRemaining -= (int64_t(2) << 32);   // 2 cycles
    uint32_t  tmp = uint32_t((uint64_t(cpuCyclesRemaining)
                              * nickCyclesPerCPUCycleD2) >> 45) & 0x0003FFFFU;
    cpuCyclesRemaining -= ((int64_t(videoMemoryWaitMult)
                            * int32_t(tmp + videoMemoryWaitCycles))
                           & (int64_t(-1) << 31));
  }

  EP128EMU_REGPARM1 void Ep128VM::videoMemoryWait_M1()
  {
    cpuCyclesRemaining -= (int64_t(3) << 31);   // 1.5 cycles
    uint32_t  tmp = uint32_t((uint64_t(cpuCyclesRemaining)
                              * nickCyclesPerCPUCycleD2) >> 45) & 0x0003FFFFU;
    cpuCyclesRemaining -= ((int64_t(videoMemoryWaitMult)
                            * int32_t(tmp + videoMemoryWaitCycles_M1))
                           & (int64_t(-1) << 31));
  }

  EP128EMU_REGPARM1 void Ep128VM::videoMemoryWait_IO()
  {
    // -1 cycle (3 is needed, but Z80_::doIn()/doOut() already waits 4)
    cpuCyclesRemaining += (int64_t(1) << 32);
    uint32_t  tmp = uint32_t((uint64_t(cpuCyclesRemaining)
                              * nickCyclesPerCPUCycleD2) >> 45) & 0x0003FFFFU;
    cpuCyclesRemaining -= ((int64_t(videoMemoryWaitMult)
                            * int32_t(tmp + videoMemoryWaitCycles_IO))
                           & (int64_t(-1) << 31));
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

  EP128EMU_REGPARM1 void Ep128VM::Z80_::executeInterrupt()
  {
    if (vm.spectrumEmulatorEnabled) {
      vm.spectrumEmulatorIOPorts[0] = 0xFF;
      vm.spectrumEmulatorIOPorts[1] = 0xFF;
      vm.spectrumEmulatorIOPorts[2] = 0xFF;
      vm.spectrumEmulatorIOPorts[3] = 0x1F;
      this->NMI_();
    }
    Z80::executeInterrupt();
  }

  EP128EMU_REGPARM2 uint8_t Ep128VM::Z80_::readMemory(uint16_t addr)
  {
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles;
      else
        vm.videoMemoryWait();
    }
    else {
      vm.cpuCyclesRemaining -= (int64_t(3) << 32);
    }
    return vm.memory.read(addr);
  }

  EP128EMU_REGPARM2 uint16_t Ep128VM::Z80_::readMemoryWord(uint16_t addr)
  {
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles;
      else
        vm.videoMemoryWait();
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles;
      else
        vm.videoMemoryWait();
    }
    else {
      vm.cpuCyclesRemaining -= (int64_t(6) << 32);
    }
    uint16_t  retval = vm.memory.read(addr);
    retval |= (uint16_t(vm.memory.read((addr + 1) & 0xFFFF) << 8));
    return retval;
  }

  EP128EMU_REGPARM1 uint8_t Ep128VM::Z80_::readOpcodeFirstByte()
  {
    uint16_t  addr = uint16_t(R.PC.W.l);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles_M1;
      else
        vm.videoMemoryWait_M1();
    }
    else {
      vm.cpuCyclesRemaining -= (int64_t(4) << 32);
    }
    if (!vm.singleStepMode)
      return vm.memory.readOpcode(addr);
    // single step mode
    return vm.checkSingleStepModeBreak();
  }

  EP128EMU_REGPARM2 uint8_t Ep128VM::Z80_::readOpcodeSecondByte(
      const bool *invalidOpcodeTable)
  {
    uint16_t  addr = (uint16_t(R.PC.W.l) + uint16_t(1)) & uint16_t(0xFFFF);
    if (invalidOpcodeTable) {
      uint8_t b = vm.memory.readNoDebug(addr);
      if (EP128EMU_UNLIKELY(invalidOpcodeTable[b]))
        return b;
    }
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles_M1;
      else
        vm.videoMemoryWait_M1();
    }
    else {
      vm.cpuCyclesRemaining -= (int64_t(4) << 32);
    }
    return vm.memory.readOpcode(addr);
  }

  EP128EMU_REGPARM2 uint8_t Ep128VM::Z80_::readOpcodeByte(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles;
      else
        vm.videoMemoryWait();
    }
    else {
      vm.cpuCyclesRemaining -= (int64_t(3) << 32);
    }
    return vm.memory.readOpcode(addr);
  }

  EP128EMU_REGPARM2 uint16_t Ep128VM::Z80_::readOpcodeWord(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles;
      else
        vm.videoMemoryWait();
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles;
      else
        vm.videoMemoryWait();
    }
    else {
      vm.cpuCyclesRemaining -= (int64_t(6) << 32);
    }
    uint16_t  retval = vm.memory.readOpcode(addr);
    retval |= (uint16_t(vm.memory.readOpcode((addr + 1) & 0xFFFF) << 8));
    return retval;
  }

  EP128EMU_REGPARM3 void Ep128VM::Z80_::writeMemory(uint16_t addr,
                                                    uint8_t value)
  {
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles;
      else
        vm.videoMemoryWait();
    }
    else {
      vm.cpuCyclesRemaining -= (int64_t(3) << 32);
    }
    vm.memory.write(addr, value);
    if (vm.spectrumEmulatorEnabled) {
      uint32_t  tmp = uint32_t(addr) & 0x3FFFU;
      tmp = tmp | (uint32_t(vm.pageTable[addr >> 14]) << 14);
      if (tmp >= 0x3F9800U && tmp <= 0x3F9AFFU)
        vm.spectrumEmulatorNMI_AttrWrite(tmp, value);
    }
  }

  EP128EMU_REGPARM3 void Ep128VM::Z80_::writeMemoryWord(uint16_t addr,
                                                        uint16_t value)
  {
    if (vm.spectrumEmulatorEnabled) {
      writeMemory(addr, uint8_t(value) & 0xFF);
      writeMemory((addr + 1) & 0xFFFF, uint8_t(value >> 8));
      return;
    }
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles;
      else
        vm.videoMemoryWait();
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles;
      else
        vm.videoMemoryWait();
    }
    else {
      vm.cpuCyclesRemaining -= (int64_t(6) << 32);
    }
    vm.memory.write(addr, uint8_t(value) & 0xFF);
    vm.memory.write((addr + 1) & 0xFFFF, uint8_t(value >> 8));
  }

  EP128EMU_REGPARM2 void Ep128VM::Z80_::pushWord(uint16_t value)
  {
    vm.cpuCyclesRemaining -= (int64_t(1) << 32);
    R.SP.W -= 2;
    uint16_t  addr = R.SP.W;
    if (vm.spectrumEmulatorEnabled) {
      writeMemory((addr + 1) & 0xFFFF, uint8_t(value >> 8));
      writeMemory(addr, uint8_t(value) & 0xFF);
      return;
    }
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles;
      else
        vm.videoMemoryWait();
      if (vm.pageTable[addr >> 14] < 0xFC)
        vm.cpuCyclesRemaining -= vm.memoryWaitCycles;
      else
        vm.videoMemoryWait();
    }
    else {
      vm.cpuCyclesRemaining -= (int64_t(6) << 32);
    }
    vm.memory.write((addr + 1) & 0xFFFF, uint8_t(value >> 8));
    vm.memory.write(addr, uint8_t(value) & 0xFF);
  }

  EP128EMU_REGPARM3 void Ep128VM::Z80_::doOut(uint16_t addr, uint8_t value)
  {
    vm.cpuCyclesRemaining -= (int64_t(3) << 32);
    if (vm.cpuCyclesRemaining < -(vm.cpuCyclesPerNickCycle))
      vm.runDevices();
    vm.cpuCyclesRemaining -= (int64_t(1) << 32);
    vm.ioPorts.write(addr, value);
  }

  EP128EMU_REGPARM2 uint8_t Ep128VM::Z80_::doIn(uint16_t addr)
  {
    vm.cpuCyclesRemaining -= (int64_t(3) << 32);
    if (vm.cpuCyclesRemaining < -(vm.cpuCyclesPerNickCycle))
      vm.runDevices();
    vm.cpuCyclesRemaining -= (int64_t(1) << 32);
    return vm.ioPorts.read(addr);
  }

  EP128EMU_REGPARM1 void Ep128VM::Z80_::updateCycle()
  {
    vm.cpuCyclesRemaining -= (int64_t(1) << 32);
  }

  EP128EMU_REGPARM2 void Ep128VM::Z80_::updateCycles(int cycles)
  {
    vm.updateCPUCycles(cycles);
  }

  EP128EMU_REGPARM1 void Ep128VM::Z80_::tapePatch()
  {
    uint8_t n;
    if (!((R.PC.W.l & 0x3FFF) < 0x3FFC &&
          vm.memory.isSegmentROM(vm.getMemoryPage(int(R.PC.W.l >> 14))) &&
          vm.readMemory(uint16_t(R.PC.W.l + 2), true) == 0xFE &&
          (n = vm.readMemory(uint16_t(R.PC.W.l + 3), true)) < 0x10)) {
      return;
    }
    if ((!vm.fileIOEnabled) | vm.isRecordingDemo | vm.isPlayingDemo) {
      // file I/O is disabled while recording or playing demo
      if (n != 0x0F)
        R.AF.B.h = uint8_t((n >= 0x01 && n <= 0x0B) ? 0xE7 : 0x00);
      return;
    }
    std::map< uint8_t, std::FILE * >::iterator  i_ = fileChannels.end();
    if (n >= 0x01 && n <= 0x0B) {
      i_ = fileChannels.find(uint8_t(R.AF.B.h));
      if ((n < 0x03) != (i_ == fileChannels.end())) {
        // 0xF9 = channel already exists
        // 0xFB = channel does not exist
        R.AF.B.h = (n < 0x03 ? 0xF9 : 0xFB);
        return;
      }
    }
    switch (n) {
    case 0x00:                          // INTERRUPT
      R.AF.B.h = 0x00;
      break;
    case 0x01:                          // OPEN CHANNEL
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
          int     err = vm.openFileInWorkingDirectory(
                            f, fileName, (n == 0x01 ? "r+b" : "w+b"));
          if (err == -5) {
            if (n == 0x01) {
              // if opening the file in RW mode failed,
              // try again in read only mode
              err = vm.openFileInWorkingDirectory(f, fileName, "rb");
            }
          }
          if (err == 0) {
            fileChannels.insert(std::pair< uint8_t, std::FILE * >(chn, f));
          }
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
    case 0x04:                          // DESTROY CHANNEL (FIXME: same as 0x03)
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
      {
        long    filePos, fileSize;
        if (!(R.BC.B.l & 0x01)) {
          R.BC.B.l = 0x00;
          // if not setting a new file position, save the original position
          if ((filePos = std::ftell((*i_).second)) < 0L) {
            R.AF.B.h = 0xA1;            // invalid file attributes
            break;
          }
        }
        else {
          R.BC.B.l = 0x00;
          filePos = long(readUserMemory(uint16_t(R.DE.W)))
                    | (long(readUserMemory(uint16_t(R.DE.W + 1))) << 8)
                    | (long(readUserMemory(uint16_t(R.DE.W + 2))) << 16)
                    | (long(readUserMemory(uint16_t(R.DE.W + 2))) << 24);
          if (filePos & long(0x80000000UL)) {
            R.AF.B.h = 0xAE;            // negative position: invalid parameter
            break;
          }
        }
        // get file size
        if (std::fseek((*i_).second, 0L, SEEK_END) != 0 ||
            (fileSize = std::ftell((*i_).second)) < 0L) {
          R.AF.B.h = 0xA1;
          break;
        }
        // set or restore file position
        if (filePos > fileSize && filePos >= 0x01000000L) {
          R.AF.B.h = 0xAE;              // invalid parameter
          break;
        }
        if (std::fseek((*i_).second, filePos, SEEK_SET) != 0) {
          R.AF.B.h = 0xA1;
          break;
        }
        writeUserMemory(uint16_t(R.DE.W), uint8_t(filePos & 0xFFL));
        writeUserMemory(uint16_t(R.DE.W + 1), uint8_t((filePos >> 8) & 0xFFL));
        writeUserMemory(uint16_t(R.DE.W + 2), uint8_t((filePos >> 16) & 0xFFL));
        writeUserMemory(uint16_t(R.DE.W + 3), uint8_t((filePos >> 24) & 0xFFL));
        writeUserMemory(uint16_t(R.DE.W + 4), uint8_t(fileSize & 0xFFL));
        writeUserMemory(uint16_t(R.DE.W + 5), uint8_t((fileSize >> 8) & 0xFFL));
        writeUserMemory(uint16_t(R.DE.W + 6),
                        uint8_t((fileSize >> 16) & 0xFFL));
        writeUserMemory(uint16_t(R.DE.W + 7),
                        uint8_t((fileSize >> 24) & 0xFFL));
        R.AF.B.h = 0x00;
        R.BC.B.l = 0x03;                // file position and size are valid
      }
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
      R.AF.B.h = uint8_t(defaultDeviceIsFILE);
      break;
    case 0x0F:                          // set if default device is 'FILE:'
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
    mode = mode & 3;
    if (mode == int(vm.memoryWaitMode))
      return;
    vm.memoryWaitMode = uint8_t(mode);
    vm.setMemoryWaitTiming();
  }

  void Ep128VM::Dave_::setRemote1State(int state)
  {
    vm.remoteControlState =
        (vm.remoteControlState & 0x02) | uint8_t(bool(state));
    vm.setTapeMotorState(bool(vm.remoteControlState));
  }

  void Ep128VM::Dave_::setRemote2State(int state)
  {
    vm.remoteControlState =
        (vm.remoteControlState & 0x01) | (uint8_t(bool(state)) << 1);
    vm.setTapeMotorState(bool(vm.remoteControlState));
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
    if (vm.videoCapture)
      vm.videoCapture->horizontalSync(buf, nBytes);
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
        demoBuffer.writeUIntVLen(demoTimeCnt);
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
    nickCyclesPerCPUCycleD2 =
        uint32_t((uint64_t(1) << 63) / uint64_t(cpuCyclesPerNickCycle));
    daveCyclesPerNickCycle =
        (int64_t(daveFrequency) << 32) / int64_t(nickFrequency);
    if (haveTape()) {
      tapeSamplesPerNickCycle =
          (int64_t(getTapeSampleRate()) << 32) / int64_t(nickFrequency);
    }
    else {
      tapeSamplesPerNickCycle = 0L;
    }
    cpuCyclesRemaining = -1L;
    daveCyclesRemaining = -1L;
    waitCycleCnt = (waitCycleCnt > 0 ?
                    (waitCycleCnt < 4 ? waitCycleCnt : 4) : 0);
    setMemoryWaitTiming();
    videoMemoryLatency =
        (videoMemoryLatency > 100000 ?
         (videoMemoryLatency < 2000000 ? videoMemoryLatency : 2000000)
         : 100000);
    videoMemoryLatency_M1 =
        (videoMemoryLatency_M1 > 100000 ?
         (videoMemoryLatency_M1 < 2000000 ? videoMemoryLatency_M1 : 2000000)
         : 100000);
    videoMemoryLatency_IO =
        (videoMemoryLatency_IO > 100000 ?
         (videoMemoryLatency_IO < 2000000 ? videoMemoryLatency_IO : 2000000)
         : 100000);
    videoMemoryWaitMult = uint32_t((cpuCyclesPerNickCycle + 0x00020000L) >> 18);
    videoMemoryWaitCycles =
        uint32_t((int64_t(2) * int32_t(nickCyclesPerCPUCycleD2) + 8192L) >> 14)
        + uint32_t((int64_t(videoMemoryLatency) * int32_t(nickFrequency << 7)
                    + 244140625L) / 488281250L);        // = 10^12 / 2048
    videoMemoryWaitCycles_M1 =
        uint32_t((int64_t(5) * int32_t(nickCyclesPerCPUCycleD2) + 8192L) >> 14)
        + uint32_t((int64_t(videoMemoryLatency_M1) * int32_t(nickFrequency << 7)
                    + 244140625L) / 488281250L);
    videoMemoryWaitCycles_IO =
        uint32_t((int64_t(2) * int32_t(nickCyclesPerCPUCycleD2) + 8192L) >> 14)
        + uint32_t((int64_t(videoMemoryLatency_IO) * int32_t(nickFrequency << 7)
                    + 244140625L) / 488281250L);
    if (videoCapture)
      videoCapture->setClockFrequency(nickFrequency);
  }

  void Ep128VM::setMemoryWaitTiming()
  {
    switch (memoryWaitMode) {
    case 0:                     // wait on every memory access
      memoryWaitCycles_M1 = int64_t(waitCycleCnt + 4) << 32;
      memoryWaitCycles = int64_t(waitCycleCnt + 3) << 32;
      break;
    case 1:                     // wait on M1 reads only
      memoryWaitCycles_M1 = int64_t(waitCycleCnt + 4) << 32;
      memoryWaitCycles = int64_t(3) << 32;
      break;
    default:                    // no wait
      memoryWaitCycles_M1 = int64_t(4) << 32;
      memoryWaitCycles = int64_t(3) << 32;
      break;
    }
  }

  EP128EMU_REGPARM1 void Ep128VM::runDevices()
  {
    do {
      nick.runOneSlot();
      nickCyclesRemainingH--;
      Ep128VMCallback   *p = firstCallback;
      while (p) {
        Ep128VMCallback *nxt = p->nxt;
        p->func(p->userData);
        p = nxt;
      }
      daveCyclesRemaining += daveCyclesPerNickCycle;
      if (daveCyclesRemaining >= 0L) {
        do {
          daveCyclesRemaining -= (int64_t(1) << 32);
          soundOutputSignal = dave.runOneCycle();
          sendAudioOutput(soundOutputSignal + externalDACOutput);
        } while (EP128EMU_UNLIKELY(daveCyclesRemaining >= 0L));
      }
      cpuCyclesRemaining += cpuCyclesPerNickCycle;
    } while (cpuCyclesRemaining < -cpuCyclesPerNickCycle);
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
      vm.videoMemoryWait_IO();
    return (vm.nick.readPort(addr));
  }

  void Ep128VM::nickPortWriteCallback(void *userData,
                                      uint16_t addr, uint8_t value)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    if (vm.memoryTimingEnabled)
      vm.videoMemoryWait_IO();
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
    if (vm.isRecordingDemo | vm.isPlayingDemo)
      return uint8_t((addr & 0x0008) ? 0x7D : 0x00);
    switch (addr) {
    case 0x00:
      return vm.wd177x.readStatusRegister();
    case 0x01:
      return vm.wd177x.readTrackRegister();
    case 0x02:
      return vm.wd177x.readSectorRegister();
    case 0x03:
      return vm.wd177x.readDataRegister();
    }
    return uint8_t(  (vm.wd177x.getInterruptRequestFlag() ? 0x3E : 0x3C)
                   | (vm.wd177x.getFloppyDrive().getDiskChangeFlag() ?
                      0x00 : 0x40)
                   | (vm.wd177x.getDataRequestFlag() ? 0x80 : 0x00)
                   | (vm.wd177x.getFloppyDrive().haveDisk() ? 0x00 : 0x01));
  }

  void Ep128VM::exdosPortWriteCallback(void *userData,
                                       uint16_t addr, uint8_t value)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    // floppy emulation is disabled while recording or playing demo
    if (vm.isRecordingDemo | vm.isPlayingDemo)
      return;
    if (addr & 0x0008) {
      if (!(value & 0x0F)) {
        vm.wd177x.setFloppyDrive((Ep128Emu::FloppyDrive *) 0);
        return;
      }
      if (value & 0x01)
        vm.wd177x.setFloppyDrive(&(vm.floppyDrives[0]));
      else if (value & 0x02)
        vm.wd177x.setFloppyDrive(&(vm.floppyDrives[1]));
      else if (value & 0x04)
        vm.wd177x.setFloppyDrive(&(vm.floppyDrives[2]));
      else
        vm.wd177x.setFloppyDrive(&(vm.floppyDrives[3]));
      if (value & 0x40)
        vm.wd177x.getFloppyDrive().setDiskChangeFlag(false);
      vm.wd177x.getFloppyDrive().setSide((value & 0x10) >> 4);
    }
    else {
      switch (addr) {
      case 0x00:
        vm.wd177x.writeCommandRegister(value);
        break;
      case 0x01:
        vm.wd177x.writeTrackRegister(value);
        break;
      case 0x02:
        vm.wd177x.writeSectorRegister(value);
        break;
      case 0x03:
        vm.wd177x.writeDataRegister(value);
        break;
      }
    }
  }

  uint8_t Ep128VM::exdosPortDebugReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    switch (addr) {
    case 0x00:
      return vm.wd177x.readStatusRegisterDebug();
    case 0x01:
      return vm.wd177x.readTrackRegister();
    case 0x02:
      return vm.wd177x.readSectorRegister();
    case 0x03:
      return vm.wd177x.readDataRegisterDebug();
    }
    return uint8_t(  (vm.wd177x.getInterruptRequestFlag() ? 0x3E : 0x3C)
                   | (vm.wd177x.getFloppyDrive().getDiskChangeFlag() ?
                      0x00 : 0x40)
                   | (vm.wd177x.getDataRequestFlag() ? 0x80 : 0x00)
                   | (vm.wd177x.getFloppyDrive().haveDisk() ? 0x00 : 0x01));
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

  uint8_t Ep128VM::externalDACIOReadCallback(void *userData, uint16_t addr)
  {
#if 0
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    return vm.externalDACIOPorts[addr & 3];
#else
    (void) userData;
    (void) addr;
#endif
    return 0xFF;
  }

  void Ep128VM::externalDACIOWriteCallback(void *userData,
                                           uint16_t addr, uint8_t value)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    if (value != vm.externalDACIOPorts[addr & 3]) {
      vm.externalDACIOPorts[addr & 3] = value;
      vm.externalDACOutput =
          (uint32_t(vm.externalDACIOPorts[0])
           + uint32_t(vm.externalDACIOPorts[1])
           + ((uint32_t(vm.externalDACIOPorts[2])
               + uint32_t(vm.externalDACIOPorts[3])) << 16U)) * 45U;
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

  uint8_t Ep128VM::ideDriveIOReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    // IDE emulation is disabled while recording or playing demo
    if (!(vm.isRecordingDemo | vm.isPlayingDemo))
      return vm.ideInterface->readPort(addr);
    return 0xFF;
  }

  void Ep128VM::ideDriveIOWriteCallback(void *userData,
                                        uint16_t addr, uint8_t value)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    // IDE emulation is disabled while recording or playing demo
    if (!(vm.isRecordingDemo | vm.isPlayingDemo))
      vm.ideInterface->writePort(addr, value);
  }

  void Ep128VM::mouseRTSWriteCallback(void *userData,
                                      uint16_t addr, uint8_t value)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    if ((value ^ vm.prvB7PortState) & 0x02) {
      if (!vm.mouseTimer) {
        if (EP128EMU_UNLIKELY(!vm.mouseEmulationEnabled)) {
          // mouse data is requested for the first time since reset()
          vm.mouseEmulationEnabled = true;
          vm.mouseDeltaX = 0;
          vm.mouseDeltaY = 0;
          vm.mouseButtonState = 0x00;
          vm.mouseWheelDelta = 0x00;
        }
        vm.setCallback(&mouseTimerCallback, userData, true);
        uint8_t   dx = uint8_t(vm.mouseDeltaX) & 0xFF;
        uint8_t   dy = uint8_t(vm.mouseDeltaY) & 0xFF;
        uint32_t  mouseData_ =
            uint32_t(vm.mouseWheelDelta)
            | (uint32_t(((vm.mouseButtonState >> 2) & 0x07) | 0x10) << 8)
            | (uint32_t(dx) << 24) | (uint32_t(dy) << 16);
        // ExCnt = 4
        // PS/2 mouse ID = 4
        // hardware version = 0x14
        // firmware version = 0x19
        // EnterMice ID = 0x5D
        vm.mouseData = (uint64_t(mouseData_) << 32) | 0x4414195DULL;
        vm.mouseDeltaX = 0;
        vm.mouseDeltaY = 0;
        vm.mouseWheelDelta = 0x00;
      }
      // send next nibble to DAVE (port 0xB6)
      uint8_t daveInput = uint8_t(vm.mouseData >> 60) & 0x0F;
      vm.mouseData = vm.mouseData << 4;
      daveInput = daveInput | (((~vm.mouseButtonState) & 0x03) << 4);
      vm.dave.setMouseInput(daveInput);
      // 1500 us
      vm.mouseTimer = (uint32_t(vm.nickFrequency) * 1573U + 0x00080000U) >> 20;
    }
    vm.prvB7PortState = value;
    vm.davePortWriteCallback(userData, addr, value);
  }

  void Ep128VM::mouseTimerCallback(void *userData)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    if (EP128EMU_EXPECT(vm.mouseTimer > 1U)) {
      vm.mouseTimer--;
      return;
    }
    vm.mouseTimer = 0U;
    vm.mouseData = 0ULL;
    vm.setCallback(&mouseTimerCallback, userData, false);
    vm.dave.clearMouseInput();
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
        uint8_t   evtType = vm.demoBuffer.readByte();
        uint8_t   evtBytes = vm.demoBuffer.readByte();
        uint32_t  evtData_ = 0U;
        unsigned char *evtData = reinterpret_cast< unsigned char * >(&evtData_);
        for (uint8_t i = 0; i < evtBytes; i++)
          evtData[i & 3] = vm.demoBuffer.readByte();
        switch (evtType) {
        case 0x00:
          vm.stopDemoPlayback();
          break;
        case 0x01:
          vm.dave.setKeyboardState(evtData[0], 1);
          break;
        case 0x02:
          vm.dave.setKeyboardState(evtData[0], 0);
          break;
        case 0x03:
          // FIXME: ugly hack to enable mouse input only from the demo file
          vm.isPlayingDemo = false;
          vm.setMouseState(int8_t(evtData[0]), int8_t(evtData[1]),
                           evtData[2], evtData[3]);
          vm.isPlayingDemo = true;
          break;
        }
        vm.demoTimeCnt = vm.demoBuffer.readUIntVLen();
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
    vm.videoCapture->runOneCycle(vm.soundOutputSignal + vm.externalDACOutput);
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
    prvRTCTime = -1L;
  }

  void Ep128VM::resetFloppyDrives(bool isColdReset)
  {
    wd177x.setFloppyDrive((Ep128Emu::FloppyDrive *) 0);
    wd177x.reset(isColdReset);
    for (int i = 0; i < 4; i++) {
      floppyDrives[i].reset();
      if (isColdReset)
        floppyDrives[i].setDiskChangeFlag(true);
    }
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
      nickCyclesRemainingL(0U),
      nickCyclesRemainingH(0),
      cpuCyclesPerNickCycle(0L),
      cpuCyclesRemaining(-1L),
      daveCyclesPerNickCycle(0L),
      daveCyclesRemaining(-1L),
      memoryWaitCycles_M1(0L),
      memoryWaitCycles(0L),
      memoryWaitMode(1),
      memoryTimingEnabled(true),
      singleStepMode(0),
      singleStepModeNextAddr(int32_t(-1)),
      tapeCallbackFlag(false),
      remoteControlState(0x00),
      soundOutputSignal(0U),
      externalDACOutput(0U),
      demoFile((Ep128Emu::File *) 0),
      demoBuffer(),
      isRecordingDemo(false),
      isPlayingDemo(false),
      snapshotLoadFlag(false),
      demoTimeCnt(0U),
      breakPointPriorityThreshold(0),
      cmosMemoryRegisterSelect(0xFF),
      spectrumEmulatorEnabled(false),
      prvRTCTime(-1L),
      firstCallback((Ep128VMCallback *) 0),
      videoCapture((Ep128Emu::VideoCapture *) 0),
      nickCyclesPerCPUCycleD2(0U),
      videoMemoryWaitMult(0U),
      videoMemoryWaitCycles(0U),
      videoMemoryWaitCycles_M1(0U),
      videoMemoryWaitCycles_IO(0U),
      tapeSamplesPerNickCycle(0),
      tapeSamplesRemaining(0),
      cpuFrequency(4000000),
      daveFrequency(500000),
      nickFrequency(889846),
      waitCycleCnt(1),
      videoMemoryLatency(359455),
      videoMemoryLatency_M1(355589),
      videoMemoryLatency_IO(362928),
      ideInterface((IDEInterface *) 0),
      mouseEmulationEnabled(false),
      prvB7PortState(0x00),
      mouseTimer(0U),
      mouseData(0U),
      mouseDeltaX(0),
      mouseDeltaY(0),
      mouseButtonState(0x00),
      mouseWheelDelta(0x00)
  {
    for (size_t i = 0; i < (sizeof(callbacks) / sizeof(Ep128VMCallback)); i++) {
      callbacks[i].func = (void (*)(void *)) 0;
      callbacks[i].userData = (void *) 0;
      callbacks[i].nxt = (Ep128VMCallback *) 0;
    }
    for (size_t i = 0; i < 4; i++) {
      pageTable[i] = 0x00;
      spectrumEmulatorIOPorts[i] = 0xFF;
      externalDACIOPorts[i] = 0x00;
    }
    updateTimingParameters();
    // register I/O callbacks
    ioPorts.setReadCallback(0xA0, 0xBF, &davePortReadCallback, this, 0xA0);
    ioPorts.setWriteCallback(0xA0, 0xBF, &davePortWriteCallback, this, 0xA0);
    ioPorts.setDebugReadCallback(0xB0, 0xB6, &davePortReadCallback, this, 0xA0);
    ioPorts.setWriteCallback(0xB7, 0xB7, &mouseRTSWriteCallback, this, 0xA0);
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
    wd177x.setIsWD1773(false);
    ideInterface = new IDEInterface();
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
    ioPorts.setReadCallback(0xF0, 0xF3,
                            &externalDACIOReadCallback, this, 0xF0);
    ioPorts.setWriteCallback(0xF0, 0xF3,
                             &externalDACIOWriteCallback, this, 0xF0);
    ioPorts.setReadCallback(0x7E, 0x7F, &cmosMemoryIOReadCallback, this, 0x7E);
    ioPorts.setDebugReadCallback(0x7E, 0x7F,
                                 &cmosMemoryIOReadCallback, this, 0x7E);
    ioPorts.setWriteCallback(0x7E, 0x7F,
                             &cmosMemoryIOWriteCallback, this, 0x7E);
    ioPorts.setReadCallback(0xEC, 0xEF, &ideDriveIOReadCallback, this, 0xEC);
    ioPorts.setDebugReadCallback(0xEC, 0xEF,
                                 &ideDriveIOReadCallback, this, 0xEC);
    ioPorts.setWriteCallback(0xEC, 0xEF, &ideDriveIOWriteCallback, this, 0xEC);
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
    delete ideInterface;
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
    {
      int64_t tmp =
          int64_t(nickCyclesRemainingL) | (int64_t(nickCyclesRemainingH) << 32);
      tmp += ((int64_t(microseconds) << 26) * int64_t(nickFrequency)
              / int64_t(15625));        // 10^6 / 2^6
      nickCyclesRemainingL = uint32_t(tmp & 0xFFFFFFFFLL);
      nickCyclesRemainingH = int32_t(uint32_t(uint64_t(tmp) >> 32));
    }
    if (EP128EMU_UNLIKELY(nickCyclesRemainingH < 1))
      return;
    do {
      Ep128VMCallback   *p = firstCallback;
      while (p) {
        Ep128VMCallback *nxt = p->nxt;
        p->func(p->userData);
        p = nxt;
      }
      daveCyclesRemaining += daveCyclesPerNickCycle;
      if (daveCyclesRemaining >= 0L) {
        do {
          daveCyclesRemaining -= (int64_t(1) << 32);
          soundOutputSignal = dave.runOneCycle();
          sendAudioOutput(soundOutputSignal + externalDACOutput);
        } while (EP128EMU_UNLIKELY(daveCyclesRemaining >= 0L));
      }
      cpuCyclesRemaining += cpuCyclesPerNickCycle;
      while (cpuCyclesRemaining >= 0L)
        z80.executeInstruction();
      nick.runOneSlot();
    } while (EP128EMU_EXPECT(--nickCyclesRemainingH > 0));
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
    setMemoryWaitTiming();
    remoteControlState = 0x00;
    setTapeMotorState(false);
    resetFloppyDrives(isColdReset);
    ideInterface->reset(int(isColdReset) + 1);
    ioPorts.writeDebug(0x44, 0x00);     // disable Spectrum emulator
    for (int i = 0; i < 4; i++) {
      spectrumEmulatorIOPorts[i] = 0xFF;
      // reset external DAC
      ioPorts.writeDebug(uint16_t(0xF0 + i), 0x00);
    }
    externalDACOutput = 0U;
    cmosMemoryRegisterSelect = 0xFF;
    if (isColdReset) {
      nick.randomizeRegisters();
      resetCMOSMemory();
      writeMemory(0x003FFFF8U, 0x00, false);
      writeMemory(0x003FFFF9U, 0x00, false);
      for (uint32_t i = 0x003FF200U; i <= 0x003FF2FFU; i++)
        writeMemory(i, 0xFF, false);
    }
    dave.setMouseInput(0xFF);
    mouseEmulationEnabled = false;
    prvB7PortState = 0x00;
    mouseTimer = 0U;
    mouseData = 0ULL;
    mouseDeltaX = 0;
    mouseDeltaY = 0;
    mouseButtonState = 0x00;
    mouseWheelDelta = 0x00;
#ifdef ENABLE_SDEXT
    sdext_init();
#endif
  }

  void Ep128VM::setCPUFrequency(size_t freq_)
  {
    // NOTE: Z80 frequency should always be greater than NICK frequency,
    // so that the memory timing functions in ep128vm.hpp work correctly
    size_t  freq = (freq_ > 2000000 ? (freq_ < 250000000 ? freq_ : 250000000)
                                      : 2000000);
    int     waitCycleCnt_ = (freq > 5000000 ? 2 : 1);
    if (cpuFrequency != freq || waitCycleCnt != waitCycleCnt_) {
      cpuFrequency = freq;
      waitCycleCnt = waitCycleCnt_;
      updateTimingParameters();
    }
  }

  void Ep128VM::setVideoFrequency(size_t freq_)
  {
    size_t  freq;
    // allow refresh rates in the range 10 Hz to 100 Hz
    freq = (freq_ > 178125 ? (freq_ < 1781250 ? freq_ : 1781250) : 178125);
    // calculate 5/16 NICK cycles + 8.271 ns in picoseconds
    int     videoMemoryLatency_ =
        int(((int64_t(1220703125L) << 8) + int64_t(freq >> 1))  // 10^12 * 5/16
            / int32_t(freq))
        + 8271;
    int     videoMemoryLatency_M1_ = videoMemoryLatency_ - 3866;
    int     videoMemoryLatency_IO_ = videoMemoryLatency_ + 3473;
    if (nickFrequency != freq ||
        videoMemoryLatency != videoMemoryLatency_ ||
        videoMemoryLatency_M1 != videoMemoryLatency_M1_ ||
        videoMemoryLatency_IO != videoMemoryLatency_IO_) {
      nickFrequency = freq;
      videoMemoryLatency = videoMemoryLatency_;
      videoMemoryLatency_M1 = videoMemoryLatency_M1_;
      videoMemoryLatency_IO = videoMemoryLatency_IO_;
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
      demoBuffer.writeUIntVLen(demoTimeCnt);
      demoTimeCnt = 0U;
      demoBuffer.writeByte(isPressed ? 0x01 : 0x02);
      demoBuffer.writeByte(0x01);
      demoBuffer.writeByte(uint8_t(keyCode & 0x7F));
    }
  }

  void Ep128VM::setMouseState(int8_t dX, int8_t dY,
                              uint8_t buttonState, uint8_t mouseWheelEvents)
  {
    if (EP128EMU_UNLIKELY(isRecordingDemo | isPlayingDemo)) {
      if (isPlayingDemo)
        return;
      if (EP128EMU_UNLIKELY(haveTape() && getIsTapeMotorOn() &&
                            getTapeButtonState() != 0)) {
        stopDemoRecording(false);
      }
      else if (mouseEmulationEnabled) {
        demoBuffer.writeUIntVLen(demoTimeCnt);
        demoTimeCnt = 0U;
        demoBuffer.writeByte(0x03);     // event type (mouse)
        demoBuffer.writeByte(0x04);     // number of data bytes
        demoBuffer.writeByte(uint8_t(dX));
        demoBuffer.writeByte(uint8_t(dY));
        demoBuffer.writeByte(buttonState);
        demoBuffer.writeByte(mouseWheelEvents);
      }
    }
    int     dX_ = int(dX) + int(mouseDeltaX);
    int     dY_ = int(dY) + int(mouseDeltaY);
    mouseDeltaX = int8_t(dX_ > -128 ? (dX_ < 127 ? dX_ : 127) : -128);
    mouseDeltaY = int8_t(dY_ > -128 ? (dY_ < 127 ? dY_ : 127) : -128);
    mouseButtonState = buttonState;
    if (!(mouseTimer | (buttonState & 0x03)))
      dave.setMouseInput(0xFF);
    if (mouseWheelEvents) {
      if (mouseWheelEvents & 0x01)      // up
        mouseWheelDelta = (mouseWheelDelta + 1) & 0xFF;
      if (mouseWheelEvents & 0x02)      // down
        mouseWheelDelta = (mouseWheelDelta - 1) & 0xFF;
      if (((mouseWheelDelta + 0x10) & 0xFF) >= 0x20)    // overflow
        mouseWheelDelta = ((mouseWheelDelta & 0x80) ? 0xF8 : 0x07);
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
    vmStatus_.floppyDriveLEDState = n | ideInterface->getLEDState();
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
    if (n < 0 || n > 7)
      throw Ep128Emu::Exception("invalid disk drive number");
    if (n < 4) {
      if (&(wd177x.getFloppyDrive()) == &(floppyDrives[n])) {
        wd177x.setFloppyDrive((Ep128Emu::FloppyDrive *) 0);
        wd177x.setFloppyDrive(&(floppyDrives[n]));
      }
      floppyDrives[n].setDiskImageFile(fileName_,
                                       nTracks_, nSides_, nSectorsPerTrack_);
    }
    else {
      ideInterface->setImageFile(n & 3, fileName_.c_str());
    }
  }

  uint32_t Ep128VM::getFloppyDriveLEDState()
  {
    uint32_t  n = 0U;
    for (int i = 3; i >= 0; i--) {
      n = n << 8;
      n |= uint32_t(floppyDrives[i].getLEDState());
    }
    return (n | ideInterface->getLEDState());
  }

  void Ep128VM::setTapeFileName(const std::string& fileName)
  {
    Ep128Emu::VirtualMachine::setTapeFileName(fileName);
    setTapeMotorState(bool(remoteControlState));
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

  void Ep128VM::setBreakPoint(const Ep128Emu::BreakPoint& bp, bool isEnabled)
  {
    if (EP128EMU_UNLIKELY(!isEnabled)) {
      if (bp.isIO()) {
        ioPorts.setBreakPoint(bp.addr(), 0, false, false);
      }
      else if (bp.haveSegment()) {
        memory.setBreakPoint(bp.segment(), bp.addr(), 0,
                             false, false, false, false);
      }
      else {
        memory.setBreakPoint(bp.addr(), 0, false, false, false, false);
      }
    }
    else {
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

  void Ep128VM::writeROM(uint32_t addr, uint8_t value)
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    memory.writeROM(addr & uint32_t(0x003FFFFF), value);
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
    listZ80Registers(buf, z80);
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

  const Z80_REGISTERS& Ep128VM::getZ80Registers() const
  {
    return z80.getReg();
  }

  void Ep128VM::getVideoPosition(int& xPos, int& yPos) const
  {
    xPos = nick.getCurrentSlot();
    yPos = ((nick.getLPBAddress() & 0xFFF0) << 4) | nick.getLPBLine();
  }

}       // namespace Ep128

