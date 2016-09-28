
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
#include "zxmemory.hpp"
#include "zxioport.hpp"
#include "ay3_8912.hpp"
#include "ula.hpp"
#include "soundio.hpp"
#include "display.hpp"
#include "vm.hpp"
#include "zx128vm.hpp"
#include "debuglib.hpp"
#include "videorec.hpp"

#include <vector>

static const uint8_t  keyboardConvTable[256] = {
  //   N   BSLASH        B        C        V        X        Z   LSHIFT
  99, 59,  57, 40,  99, 60,  99,  3,  99,  4,  99,  2,  99,  1,  99,  0,
  //   H     LOCK        G        D        F        S        A     CTRL
  99, 52,   0, 25,  99, 12,  99, 10,  99, 11,  99,  9,  99,  8,  99, 57,
  //   U        Q        Y        R        T        E        W      TAB
  99, 43,  99, 16,  99, 44,  99, 19,  99, 20,  99, 18,  99, 17,  99, 99,
  //   7        1        6        4        5        3        2      ESC
  99, 35,  99, 24,  99, 36,  99, 27,  99, 28,  99, 26,  99, 25,   0, 24,
  //  F4       F8       F3       F6       F5       F7       F2       F1
   0, 57,  99, 99,   0, 33,  99, 99,   0, 24,  99, 99,   0, 27,   0, 26,
  //   8                 9        -        0        ^    ERASE
  99, 34,  99, 99,  99, 33,  57, 51,  99, 32,  57, 60,   0, 32,  99, 99,
  //   J                 K        ;        L        :        ]
  99, 51,  99, 99,  99, 50,  57, 41,  99, 49,  57,  1,  57, 49,  99, 99,
  // STOP    DOWN    RIGHT       UP     HOLD     LEFT    ENTER      ALT
   0, 56,   0, 36,   0, 34,   0, 35,  99, 99,   0, 28,  99, 48,  99, 57,
  //   M   DELETE        ,        /        .   RSHIFT    SPACE   INSERT
  99, 58,   0, 32,  57, 59,  57,  4,  57, 58,  99,  0,  99, 56,  99, 99,
  //   I                 O        @        P        [
  99, 42,  99, 99,  99, 41,  57, 25,  99, 40,  57, 50,  99, 99,  99, 99,
  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,
  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,
  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,
  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,
  // JOY1R  JOY1L    JOY1D    JOY1U    JOY1F
  99, 64,  99, 65,  99, 66,  99, 67,  99, 68,  99, 99,  99, 99,  99, 99,
  // JOY2R  JOY2L    JOY2D    JOY2U    JOY2F
  99, 64,  99, 65,  99, 66,  99, 67,  99, 68,  99, 99,  99, 99,  99, 99
};

namespace ZX128 {

  EP128EMU_INLINE bool ZX128VM::isContendedAddress(uint16_t addr) const
  {
    if ((addr & 0xC000) != 0xC000)
      return bool(addr & 0x4000);
    return (spectrum128Mode && bool(spectrum128PageRegister & 0x01));
  }

  EP128EMU_INLINE void ZX128VM::updateCPUCycles(int cycles)
  {
    z80OpcodeHalfCycles = z80OpcodeHalfCycles + uint8_t(cycles << 1);
  }

  EP128EMU_INLINE void ZX128VM::updateCPUHalfCycles(int halfCycles)
  {
    z80OpcodeHalfCycles = z80OpcodeHalfCycles + uint8_t(halfCycles);
  }

  EP128EMU_INLINE void ZX128VM::contendedWait(int halfCycles,
                                              int contentionOffset)
  {
    updateCPUHalfCycles(ula.getWaitHalfCycles(int(z80OpcodeHalfCycles)
                                              + contentionOffset)
                        + halfCycles);
  }

  EP128EMU_INLINE void ZX128VM::memoryWait(uint16_t addr)
  {
    if (isContendedAddress(addr))
      contendedWait(5, 1);
    else
      updateCPUHalfCycles(5);
  }

  EP128EMU_INLINE void ZX128VM::memoryWaitM1(uint16_t addr)
  {
    if (isContendedAddress(addr))
      contendedWait(4, 1);
    else
      updateCPUHalfCycles(4);
  }

  EP128EMU_INLINE void ZX128VM::ioPortWait(uint16_t addr)
  {
    if (isContendedAddress(addr)) {
      contendedWait(3, 1);
      if ((addr & 0x01) == 0) {
        contendedWait(4, 0);
      }
      else {
        contendedWait(2, 0);
        contendedWait(2, 0);
        contendedWait(0, 0);
      }
    }
    else if ((addr & 0x01) == 0) {
      contendedWait(7, 3);
    }
    else {
      updateCPUHalfCycles(7);
    }
  }

  EP128EMU_REGPARM1 void ZX128VM::runOneCycle()
  {
    ZX128VMCallback *p = firstCallback;
    while (p) {
      ZX128VMCallback *nxt = p->nxt;
      p->func(p->userData);
      p = nxt;
    }
    if (--ayCycleCnt == 0) {
      ayCycleCnt = 4;
      uint32_t  tmp = soundOutputAccumulator;
      soundOutputAccumulator = 0U;
      if (spectrum128Mode) {
        uint16_t  tmpA = 0;
        uint16_t  tmpB = 0;
        uint16_t  tmpC = 0;
        ay3.runOneCycle(tmpA, tmpB, tmpC);
        tmp = tmp + (uint32_t(tmpA + tmpB + tmpC) << 2);
      }
      tmp = (tmp * 8864U + 0x8000U) & 0xFFFF0000U;
      tmp = tmp | (tmp >> 16);
      soundOutputSignal = tmp;
      sendAudioOutput(tmp);
    }
    ula.runOneSlot();
    soundOutputAccumulator += uint32_t(ula.getSoundOutput());
    ulaCyclesRemainingH--;
    z80OpcodeHalfCycles = z80OpcodeHalfCycles - 8;
  }

  // --------------------------------------------------------------------------

  ZX128VM::Z80_::Z80_(ZX128VM& vm_)
    : Ep128::Z80(),
      vm(vm_),
      tapFile((std::FILE *) 0),
      tapeBlockBytesLeft(0),
      tapeLeaderCnt(0),
      tapeShiftRegCnt(0),
      tapeShiftReg(0x00)
  {
    addressBusState.W = 0x0000;
  }

  ZX128VM::Z80_::~Z80_()
  {
    closeTapeFile();
  }

  EP128EMU_REGPARM1 void ZX128VM::Z80_::executeInterrupt()
  {
    if (EP128EMU_UNLIKELY(vm.ula.getInterruptFlag(int(vm.z80OpcodeHalfCycles)
                                                  - 2))) {
      Ep128::Z80::executeInterrupt();
    }
  }

  EP128EMU_REGPARM2 uint8_t ZX128VM::Z80_::readMemory(uint16_t addr)
  {
    addressBusState.W = addr;
    vm.memoryWait(addr);
    uint8_t   retval = vm.memory.read(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM2 uint16_t ZX128VM::Z80_::readMemoryWord(uint16_t addr)
  {
    vm.memoryWait(addr);
    uint16_t  retval = vm.memory.read(addr);
    vm.updateCPUHalfCycles(1);
    addr = (addr + 1) & 0xFFFF;
    addressBusState.W = addr;
    vm.memoryWait(addr);
    retval |= (uint16_t(vm.memory.read(addr) << 8));
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM1 uint8_t ZX128VM::Z80_::readOpcodeFirstByte()
  {
    addressBusState.B.h = R.I;
    uint16_t  addr = uint16_t(R.PC.W.l);
    vm.memoryWaitM1(addr);
    if (addr == 0x05E7) {
      readTapeFile();
      addr = uint16_t(R.PC.W.l);
    }
    if (!vm.singleStepMode) {
      uint8_t   retval = vm.memory.readOpcode(addr);
      vm.updateCPUHalfCycles(4);
      return retval;
    }
    // single step mode
    uint8_t   retval = vm.checkSingleStepModeBreak();
    vm.updateCPUHalfCycles(4);
    return retval;
  }

  EP128EMU_REGPARM2 uint8_t ZX128VM::Z80_::readOpcodeSecondByte(
      const bool *invalidOpcodeTable)
  {
    uint16_t  addr = (uint16_t(R.PC.W.l) + uint16_t(1)) & uint16_t(0xFFFF);
    if (invalidOpcodeTable) {
      uint8_t b = vm.memory.readNoDebug(addr);
      if (EP128EMU_UNLIKELY(invalidOpcodeTable[b]))
        return b;
    }
    addressBusState.B.h = R.I;
    vm.memoryWaitM1(addr);
    uint8_t   retval = vm.memory.readOpcode(addr);
    vm.updateCPUHalfCycles(4);
    return retval;
  }

  EP128EMU_REGPARM2 uint8_t ZX128VM::Z80_::readOpcodeByte(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    addressBusState.W = addr;
    vm.memoryWait(addr);
    uint8_t   retval = vm.memory.readOpcode(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM2 uint16_t ZX128VM::Z80_::readOpcodeWord(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    vm.memoryWait(addr);
    uint16_t  retval = vm.memory.readOpcode(addr);
    vm.updateCPUHalfCycles(1);
    addr = (addr + 1) & 0xFFFF;
    addressBusState.W = addr;
    vm.memoryWait(addr);
    retval |= (uint16_t(vm.memory.readOpcode(addr) << 8));
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM3 void ZX128VM::Z80_::writeMemory(uint16_t addr,
                                                    uint8_t value)
  {
    addressBusState.W = addr;
    vm.memoryWait(addr);
    while (vm.z80OpcodeHalfCycles >= 8)
      vm.runOneCycle();
    vm.memory.write(addr, value);
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM3 void ZX128VM::Z80_::writeMemoryWord(uint16_t addr,
                                                        uint16_t value)
  {
    vm.memoryWait(addr);
    while (vm.z80OpcodeHalfCycles >= 8)
      vm.runOneCycle();
    vm.memory.write(addr, uint8_t(value) & 0xFF);
    vm.updateCPUHalfCycles(1);
    addr = (addr + 1) & 0xFFFF;
    addressBusState.W = addr;
    vm.memoryWait(addr);
    while (vm.z80OpcodeHalfCycles >= 8)
      vm.runOneCycle();
    vm.memory.write(addr, uint8_t(value >> 8));
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM2 void ZX128VM::Z80_::pushWord(uint16_t value)
  {
    if (vm.isContendedAddress(addressBusState.W))
      vm.contendedWait(2, 1);
    else
      vm.updateCPUHalfCycles(2);
    R.SP.W -= 2;
    uint16_t  addr = R.SP.W;
    addressBusState.W = addr;
    vm.memoryWait((addr + 1) & 0xFFFF);
    while (vm.z80OpcodeHalfCycles >= 8)
      vm.runOneCycle();
    vm.memory.write((addr + 1) & 0xFFFF, uint8_t(value >> 8));
    vm.updateCPUHalfCycles(1);
    vm.memoryWait(addr);
    while (vm.z80OpcodeHalfCycles >= 8)
      vm.runOneCycle();
    vm.memory.write(addr, uint8_t(value) & 0xFF);
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM3 void ZX128VM::Z80_::doOut(uint16_t addr, uint8_t value)
  {
    addressBusState.W = addr;
    vm.ioPortWait(addr);
    while (vm.z80OpcodeHalfCycles >= 8)
      vm.runOneCycle();
    vm.ioPorts.write(addr, value);
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM2 uint8_t ZX128VM::Z80_::doIn(uint16_t addr)
  {
    addressBusState.W = addr;
    vm.ioPortWait(addr);
    uint8_t   retval = vm.ioPorts.read(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM1 void ZX128VM::Z80_::updateCycle()
  {
    if (vm.isContendedAddress(addressBusState.W))
      vm.contendedWait(2, 1);
    else
      vm.updateCPUHalfCycles(2);
  }

  EP128EMU_REGPARM2 void ZX128VM::Z80_::updateCycles(int cycles)
  {
    if (vm.isContendedAddress(addressBusState.W)) {
      do {
        vm.contendedWait(2, 1);
      } while (--cycles > 0);
    }
    else {
      vm.updateCPUCycles(cycles);
    }
  }

  void ZX128VM::Z80_::readTapeFile()
  {
    if (vm.spectrum128Mode && (vm.spectrum128PageRegister & 0x10) == 0)
      return;
    if (vm.isRecordingDemo | vm.isPlayingDemo
        | (!vm.fileIOEnabled) | vm.haveTape()) {
      return;
    }
    R.PC.W.l = 0x0604;                  // address of RET from routine
    R.AF.B.l = R.AF.B.l & 0xBE;         // clear Z and C flags for STOP/error
    if (!tapFile) {
      std::string fileName("");
      if (vm.openFileInWorkingDirectory(tapFile, fileName, "rb") != 0) {
        vm.ula.setKeyboardState(56, true);
        return;
      }
    }
    if (!(vm.keyboardState[7] & 0x01))
      return;                           // STOP key is pressed
    int     waitTime = 0;
    if (tapeBlockBytesLeft < 1) {
      do {
        int     c = std::fgetc(tapFile);
        if (c == EOF)
          break;
        tapeBlockBytesLeft = uint16_t(c & 0xFF);
        c = std::fgetc(tapFile);
        if (c == EOF) {
          tapeBlockBytesLeft = 0;
          break;
        }
        tapeBlockBytesLeft = tapeBlockBytesLeft | (uint16_t(c & 0xFF) << 8);
      } while (tapeBlockBytesLeft < 1);
      if (tapeBlockBytesLeft > 0)
        tapeLeaderCnt = 1024;
      tapeShiftRegCnt = 0;
      tapeShiftReg = 0x00;
    }
    if (tapeLeaderCnt > 0) {
      waitTime = (tapeLeaderCnt > 2 ? 24 : 8);
      tapeLeaderCnt--;
    }
    else if (tapeBlockBytesLeft > 0) {
      if (tapeShiftRegCnt < 1) {
        int     c = std::fgetc(tapFile);
        if (c == EOF) {
          tapeBlockBytesLeft = 0;
        }
        else {
          tapeShiftRegCnt = 16;
          tapeShiftReg = uint8_t(c & 0xFF);
        }
      }
      if (tapeShiftRegCnt > 0) {
        tapeShiftRegCnt--;
        waitTime = ((tapeShiftReg & 0x80) == 0 ? 8 : 16);
        if (!(tapeShiftRegCnt & 0x01)) {
          tapeShiftReg = (tapeShiftReg & 0x7F) << 1;
          if (tapeShiftRegCnt == 0)
            tapeBlockBytesLeft--;
        }
      }
    }
    if (waitTime > 0) {
      R.BC.B.h = (R.BC.B.h + uint8_t(waitTime)) & 0xFF;
      R.BC.B.l = R.BC.B.l ^ 0xFF;
      vm.ula.writePort(((R.BC.B.l ^ (tapeShiftReg >> 5)) & 0x07) | 0x08);
      R.AF.B.l = R.AF.B.l | 0x01;       // success: clear Z, set C
      if (waitTime > 12)
        R.PC.W.l = 0x0603;
    }
    else {
      R.BC.B.h = 0x00;
      R.AF.B.l = R.AF.B.l | 0x40;       // error: set Z, clear C
    }
  }

  void ZX128VM::Z80_::rewindTapeFile()
  {
    tapeBlockBytesLeft = 0;
    tapeLeaderCnt = 0;
    tapeShiftRegCnt = 0;
    tapeShiftReg = 0x00;
    if (tapFile)
      std::fseek(tapFile, 0L, SEEK_SET);
  }

  void ZX128VM::Z80_::closeTapeFile()
  {
    tapeBlockBytesLeft = 0;
    tapeLeaderCnt = 0;
    tapeShiftRegCnt = 0;
    tapeShiftReg = 0x00;
    if (tapFile) {
      std::fclose(tapFile);
      tapFile = (std::FILE *) 0;
    }
  }

  // --------------------------------------------------------------------------

  ZX128VM::Memory_::Memory_(ZX128VM& vm_)
    : Memory(),
      vm(vm_)
  {
  }

  ZX128VM::Memory_::~Memory_()
  {
  }

  void ZX128VM::Memory_::breakPointCallback(bool isWrite,
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

  ZX128VM::IOPorts_::IOPorts_(ZX128VM& vm_)
    : IOPorts(),
      vm(vm_)
  {
  }

  ZX128VM::IOPorts_::~IOPorts_()
  {
  }

  void ZX128VM::IOPorts_::breakPointCallback(bool isWrite,
                                             uint16_t addr, uint8_t value)
  {
    if (!vm.memory.checkIgnoreBreakPoint(vm.z80.getReg().PC.W.l)) {
      vm.breakPointCallback(vm.breakPointCallbackUserData,
                            int(isWrite) + 5, addr, value);
    }
  }

  // --------------------------------------------------------------------------

  ZX128VM::ULA_::ULA_(ZX128VM& vm_)
    : ULA(vm_.memory.getSegmentData(0x00)),
      vm(vm_)
  {
  }

  ZX128VM::ULA_::~ULA_()
  {
  }

  void ZX128VM::ULA_::drawLine(const uint8_t *buf, size_t nBytes)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.drawLine(buf, nBytes);
    if (vm.videoCapture)
      vm.videoCapture->horizontalSync(buf, nBytes);
  }

  void ZX128VM::ULA_::vsyncStateChange(bool newState, unsigned int currentSlot_)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.vsyncStateChange(newState, currentSlot_);
    if (vm.videoCapture)
      vm.videoCapture->vsyncStateChange(newState, currentSlot_);
  }

  void ZX128VM::ULA_::irqPollEnableCallback(bool isEnabled)
  {
    if (isEnabled)
      vm.z80.triggerInterrupt();
    else
      vm.z80.clearInterrupt();
  }

  // --------------------------------------------------------------------------

  void ZX128VM::stopDemoPlayback()
  {
    if (isPlayingDemo) {
      isPlayingDemo = false;
      setCallback(&demoPlayCallback, this, false);
      demoTimeCnt = 0U;
      demoBuffer.clear();
      // clear keyboard state at end of playback
      resetKeyboard();
    }
  }

  void ZX128VM::stopDemoRecording(bool writeFile_)
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
        demoFile->addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_ZX_DEMO,
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

  uint8_t ZX128VM::ioPortReadCallback(void *userData, uint16_t addr)
  {
    ZX128VM&  vm = *(reinterpret_cast<ZX128VM *>(userData));
    uint8_t   retval = 0xFF;
    if ((addr & 0xE0) == 0)
      retval = vm.joystickState;
    else if ((addr & 0x01) == 0)
      retval = vm.ula.readPort(addr);
    else if ((addr & 0xC002) == 0xC000 && vm.spectrum128Mode)
      retval = vm.ay3.readRegister(vm.ayRegisterSelected & 0x0F);
    else
      retval = vm.ula.idleDataBusRead(vm.z80OpcodeHalfCycles);
    if ((addr & 0x8002) == 0 && vm.spectrum128Mode) {
      if ((vm.spectrum128PageRegister & 0x20) == 0) {
        vm.spectrum128PageRegister = retval;
        vm.memory.setPage(0, ((retval & 0x10) >> 4) | 0x80);
        vm.memory.setPage(3, retval & 0x07);
        vm.ula.setVideoMemory(vm.memory.getSegmentData(((retval & 0x08) >> 2)
                                                       | 0x05));
      }
    }
    return retval;
  }

  void ZX128VM::ioPortWriteCallback(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    ZX128VM&  vm = *(reinterpret_cast<ZX128VM *>(userData));
    if ((addr & 0x01) == 0)
      vm.ula.writePort(value);
    if ((addr & 0x02) == 0 && vm.spectrum128Mode) {
      if ((addr & 0x8000) == 0) {
        if ((vm.spectrum128PageRegister & 0x20) == 0) {
          vm.spectrum128PageRegister = value;
          vm.memory.setPage(0, ((value & 0x10) >> 4) | 0x80);
          vm.memory.setPage(3, value & 0x07);
          vm.ula.setVideoMemory(vm.memory.getSegmentData(((value & 0x08) >> 2)
                                                         | 0x05));
        }
      }
      else if ((addr & 0x4000) != 0) {
        vm.ayRegisterSelected = value;
      }
      else {
        vm.ay3.writeRegister(vm.ayRegisterSelected & 0x0F, value);
      }
    }
  }

  uint8_t ZX128VM::ioPortDebugReadCallback(void *userData, uint16_t addr)
  {
    ZX128VM&  vm = *(reinterpret_cast<ZX128VM *>(userData));
    uint8_t   retval = 0xFF;
    if (addr < 0x0100) {
      if ((addr & 0xE0) == 0)
        retval = vm.joystickState;
      else if ((addr & 0x01) == 0)
        retval = vm.ula.readPortDebug();
      else if ((addr & 0x02) == 0 && vm.spectrum128Mode)
        retval = vm.spectrum128PageRegister;
    }
    else {
      if ((addr & 0x8002) == 0)
        addr = 0xFFFF;
      retval = ZX128VM::ioPortReadCallback(userData, addr);
    }
    return retval;
  }

  void ZX128VM::tapeCallback(void *userData)
  {
    ZX128VM&  vm = *(reinterpret_cast<ZX128VM *>(userData));
    vm.tapeSamplesRemaining += vm.tapeSamplesPerULACycle;
    if (vm.tapeSamplesRemaining > 0) {
      // assume tape sample rate < ulaFrequency
      vm.tapeSamplesRemaining -= (int64_t(1) << 32);
      vm.ula.setTapeInput(vm.runTape(vm.ula.getTapeOutput()));
    }
  }

  void ZX128VM::demoPlayCallback(void *userData)
  {
    ZX128VM&  vm = *(reinterpret_cast<ZX128VM *>(userData));
    while (!vm.demoTimeCnt) {
      if (vm.haveTape() && vm.getTapeButtonState() != 0)
        vm.stopDemoPlayback();
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
        case 0x02:
          {
            int     rowNum = (evtData & 0x78) >> 3;
            uint8_t mask = uint8_t(1 << (evtData & 0x07));
            vm.keyboardState[rowNum] = (vm.keyboardState[rowNum] & (~mask))
                                       | uint8_t((1 - evtType) & mask);
          }
          vm.convertKeyboardState();
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

  void ZX128VM::demoRecordCallback(void *userData)
  {
    ZX128VM&  vm = *(reinterpret_cast<ZX128VM *>(userData));
    vm.demoTimeCnt++;
  }

  void ZX128VM::videoCaptureCallback(void *userData)
  {
    ZX128VM&  vm = *(reinterpret_cast<ZX128VM *>(userData));
    vm.videoCapture->runOneCycle(vm.soundOutputSignal);
  }

  uint8_t ZX128VM::checkSingleStepModeBreak()
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

  void ZX128VM::convertKeyboardState()
  {
    for (int i = 0; i < 64; i++)
      ula.setKeyboardState(i, false);
    joystickState = 0x00;
    const uint8_t *p = &(keyboardConvTable[0]);
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 8; j++) {
        if ((keyboardState[i] & uint8_t(1 << j)) == 0) {
          for (int k = 0; k < 2; k++) {
            int     zxKeyNum = p[k];
            if (zxKeyNum < 0x40)
              ula.setKeyboardState(zxKeyNum, true);
            else if (zxKeyNum < 0x45)
              joystickState = joystickState | uint8_t(1 << (zxKeyNum & 0x07));
          }
        }
        p = p + 2;
      }
    }
  }

  void ZX128VM::resetKeyboard()
  {
    for (int i = 0; i < 16; i++)
      keyboardState[i] = 0xFF;
    convertKeyboardState();
  }

  void ZX128VM::initializeMemoryPaging()
  {
    ula.setSpectrum128Mode(spectrum128Mode);
    if (spectrum128Mode) {              // Spectrum 128K
      memory.setPage(0, ((spectrum128PageRegister & 0x10) >> 4) | 0x80);
      memory.setPage(1, 0x05);
      memory.setPage(2, 0x02);
      memory.setPage(3, spectrum128PageRegister & 0x07);
      ula.setVideoMemory(memory.getSegmentData(((spectrum128PageRegister & 0x08)
                                                >> 2) | 0x05));
    }
    else {                              // Spectrum 16K or 48K
      spectrum128PageRegister = 0x00;
      memory.setPage(0, 0x80);
      memory.setPage(1, uint8_t(memory.isSegmentRAM(0x00) ? 0x00 : 0xFF));
      memory.setPage(2, uint8_t(memory.isSegmentRAM(0x01) ? 0x01 : 0xFF));
      memory.setPage(3, uint8_t(memory.isSegmentRAM(0x02) ? 0x02 : 0xFF));
      ula.setVideoMemory(memory.getSegmentData(0x00));
    }
  }

  void ZX128VM::setCallback(void (*func)(void *userData), void *userData_,
                            bool isEnabled)
  {
    if (!func)
      return;
    const size_t  maxCallbacks = sizeof(callbacks) / sizeof(ZX128VMCallback);
    int     ndx = -1;
    for (size_t i = 0; i < maxCallbacks; i++) {
      if (callbacks[i].func == func && callbacks[i].userData == userData_) {
        ndx = int(i);
        break;
      }
    }
    if (ndx >= 0) {
      ZX128VMCallback   *prv = (ZX128VMCallback *) 0;
      ZX128VMCallback   *p = firstCallback;
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
        callbacks[ndx].nxt = (ZX128VMCallback *) 0;
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
        throw Ep128Emu::Exception("ZX128VM: too many callbacks");
    }
    callbacks[ndx].func = func;
    callbacks[ndx].userData = userData_;
    callbacks[ndx].nxt = (ZX128VMCallback *) 0;
    if (isEnabled) {
      ZX128VMCallback   *prv = (ZX128VMCallback *) 0;
      ZX128VMCallback   *p = firstCallback;
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

  ZX128VM::ZX128VM(Ep128Emu::VideoDisplay& display_,
                   Ep128Emu::AudioOutput& audioOutput_)
    : VirtualMachine(display_, audioOutput_),
      z80(*this),
      memory(*this),
      ioPorts(*this),
      ay3(),
      ula(*this),
      ulaCyclesRemainingL(0U),
      ulaCyclesRemainingH(0),
      spectrum128Mode(true),
      spectrum128PageRegister(0x00),
      ayRegisterSelected(0x00),
      ayCycleCnt(2),
      z80OpcodeHalfCycles(0),
      joystickState(0x00),
      tapeCallbackFlag(false),
      singleStepMode(0),
      singleStepModeNextAddr(int32_t(-1)),
      soundOutputAccumulator(0U),
      soundOutputSignal(0U),
      demoFile((Ep128Emu::File *) 0),
      demoBuffer(),
      isRecordingDemo(false),
      isPlayingDemo(false),
      snapshotLoadFlag(false),
      demoTimeCnt(0UL),
      breakPointPriorityThreshold(0),
      firstCallback((ZX128VMCallback *) 0),
      videoCapture((Ep128Emu::VideoCapture *) 0),
      tapeSamplesPerULACycle(0L),
      tapeSamplesRemaining(0L),
      ulaFrequency(886724)
  {
    for (size_t i = 0; i < (sizeof(callbacks) / sizeof(ZX128VMCallback)); i++) {
      callbacks[i].func = (void (*)(void *)) 0;
      callbacks[i].userData = (void *) 0;
      callbacks[i].nxt = (ZX128VMCallback *) 0;
    }
    // register I/O callbacks
    ioPorts.setCallbackUserData((void *) this);
    ioPorts.setReadCallback(&ioPortReadCallback);
    ioPorts.setWriteCallback(&ioPortWriteCallback);
    ioPorts.setDebugReadCallback(&ioPortDebugReadCallback);
    // hack to get some programs using interrupt mode 2 working
    z80.setVectorBase(0xFF);
    // use ULA colormap
    Ep128Emu::VideoDisplay::DisplayParameters
        dp(display.getDisplayParameters());
    dp.indexToRGBFunc = &ULA::convertPixelToRGB;
    display.setDisplayParameters(dp);
    setAudioConverterSampleRate(float(long(ulaFrequency >> 2)));
    // reset
    resetKeyboard();
    resetMemoryConfiguration(128);
  }

  ZX128VM::~ZX128VM()
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

  void ZX128VM::run(size_t microseconds)
  {
    Ep128Emu::VirtualMachine::run(microseconds);
    if (snapshotLoadFlag) {
      snapshotLoadFlag = false;
      // if just loaded a snapshot, and not playing a demo,
      // clear keyboard state
      if (!isPlayingDemo)
        resetKeyboard();
    }
    bool    newTapeCallbackFlag = (haveTape() && getTapeButtonState() != 0);
    if (newTapeCallbackFlag != tapeCallbackFlag) {
      tapeCallbackFlag = newTapeCallbackFlag;
      if (!tapeCallbackFlag)
        ula.setTapeInput(0);
      setCallback(&tapeCallback, this, tapeCallbackFlag);
    }
    z80OpcodeHalfCycles = z80OpcodeHalfCycles & 0xFE;
    int64_t ulaCyclesRemaining =
        int64_t(ulaCyclesRemainingL) + (int64_t(ulaCyclesRemainingH) << 32)
        + ((int64_t(microseconds) << 26) * int64_t(ulaFrequency)
           / int64_t(15625));   // 10^6 / 2^6
    ulaCyclesRemainingL = uint32_t(uint64_t(ulaCyclesRemaining) & 0xFFFFFFFFUL);
    ulaCyclesRemainingH = int32_t(ulaCyclesRemaining >> 32);
    while (EP128EMU_EXPECT(ulaCyclesRemainingH > 0)) {
      z80.executeInstruction();
      if (EP128EMU_EXPECT(z80OpcodeHalfCycles >= 8)) {
        do {
          runOneCycle();
        } while (z80OpcodeHalfCycles >= 8);
      }
    }
  }

  void ZX128VM::reset(bool isColdReset)
  {
    stopDemoPlayback();         // TODO: should be recorded as an event ?
    stopDemoRecording(false);
    z80.reset();
    spectrum128PageRegister = 0x00;
    ayRegisterSelected = 0x00;
    initializeMemoryPaging();
    ay3.reset();
    ula.reset();
    if (isColdReset) {
      resetKeyboard();
      z80.closeTapeFile();
    }
  }

  void ZX128VM::resetMemoryConfiguration(size_t memSize)
  {
    stopDemo();
    // calculate new number of RAM segments
    size_t  nSegments = 3;
    spectrum128Mode = false;
    if (memSize < 32) {
      nSegments = 1;
    }
    else if (memSize >= 88) {
      nSegments = 8;
      spectrum128Mode = true;
    }
    ula.setSpectrum128Mode(spectrum128Mode);
    // delete all ROM segments
    for (int i = 0; i < 256; i++) {
      if (memory.isSegmentROM(uint8_t(i)))
        memory.deleteSegment(uint8_t(i));
    }
    // resize RAM
    for (int i = int(nSegments); i < 256; i++) {
      if (memory.isSegmentRAM(uint8_t(i)))
        memory.deleteSegment(uint8_t(i));
    }
    for (int i = 0; i < int(nSegments); i++)
      memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
    // cold reset
    this->reset(true);
  }

  void ZX128VM::loadROMSegment(uint8_t n, const char *fileName, size_t offs)
  {
    stopDemo();
    if (n < 0x80 || n >= 0x82)
      throw Ep128Emu::Exception("internal error: invalid ROM segment number");
    if (fileName == (char *) 0 || fileName[0] == '\0') {
      // empty file name: delete segment
      memory.deleteSegment(n);
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
    // load new segment, or replace existing ROM
    memory.loadSegment(n, true, &(buf.front()), 0x4000);
  }

  void ZX128VM::setVideoFrequency(size_t freq_)
  {
    size_t  freq;
    // allow refresh rates in the range 10 Hz to 100 Hz
    freq = ((freq_ > 400000 ? (freq_ < 2000000 ? freq_ : 2000000) : 400000) + 2)
           & (~(size_t(3)));
    if (freq == ulaFrequency)
      return;
    ulaFrequency = freq;
    stopDemoPlayback();         // changing configuration implies stopping
    stopDemoRecording(false);   // any demo playback or recording
    setAudioConverterSampleRate(float(long(ulaFrequency >> 2)));
    if (haveTape()) {
      tapeSamplesPerULACycle =
          (int64_t(getTapeSampleRate()) << 32) / int64_t(ulaFrequency);
    }
    else {
      tapeSamplesPerULACycle = 0L;
    }
    if (videoCapture)
      videoCapture->setClockFrequency(ulaFrequency);
  }

  void ZX128VM::setKeyboardState(int keyCode, bool isPressed)
  {
    if (!isPlayingDemo) {
      int     rowNum = (keyCode & 0x78) >> 3;
      uint8_t mask = uint8_t(1 << (keyCode & 0x07));
      keyboardState[rowNum] = (keyboardState[rowNum] & (~mask))
                              | ((uint8_t(isPressed) - 1) & mask);
      convertKeyboardState();
    }
    if (isRecordingDemo) {
      if (haveTape() && getTapeButtonState() != 0) {
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

  void ZX128VM::getVMStatus(VMStatus& vmStatus_)
  {
    vmStatus_.tapeReadOnly = getIsTapeReadOnly();
    vmStatus_.tapePosition = getTapePosition();
    vmStatus_.tapeLength = getTapeLength();
    vmStatus_.tapeSampleRate = getTapeSampleRate();
    vmStatus_.tapeSampleSize = getTapeSampleSize();
    vmStatus_.isPlayingDemo = isPlayingDemo;
    if (demoFile != (Ep128Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    vmStatus_.isRecordingDemo = isRecordingDemo;
  }

  void ZX128VM::openVideoCapture(
      int frameRate_,
      bool yuvFormat_,
      void (*errorCallback_)(void *userData, const char *msg),
      void (*fileNameCallback_)(void *userData, std::string& fileName),
      void *userData_)
  {
    if (!videoCapture) {
      if (yuvFormat_) {
        videoCapture = new Ep128Emu::VideoCapture_YV12(&ULA::convertPixelToRGB,
                                                       frameRate_);
      }
      else {
        videoCapture = new Ep128Emu::VideoCapture_RLE8(&ULA::convertPixelToRGB,
                                                       frameRate_);
      }
      videoCapture->setClockFrequency(ulaFrequency);
      setCallback(&videoCaptureCallback, this, true);
    }
    videoCapture->setErrorCallback(errorCallback_, userData_);
    videoCapture->setFileNameCallback(fileNameCallback_, userData_);
  }

  void ZX128VM::setVideoCaptureFile(const std::string& fileName_)
  {
    if (!videoCapture) {
      throw Ep128Emu::Exception("internal error: "
                                "video capture object does not exist");
    }
    videoCapture->openFile(fileName_.c_str());
  }

  void ZX128VM::closeVideoCapture()
  {
    if (videoCapture) {
      setCallback(&videoCaptureCallback, this, false);
      delete videoCapture;
      videoCapture = (Ep128Emu::VideoCapture *) 0;
    }
  }

  void ZX128VM::setTapeFileName(const std::string& fileName)
  {
    Ep128Emu::VirtualMachine::setTapeFileName(fileName);
    if (haveTape()) {
      setTapeMotorState(true);
      tapeSamplesPerULACycle =
          (int64_t(getTapeSampleRate()) << 32) / int64_t(ulaFrequency);
    }
    else {
      setTapeMotorState(false);
    }
    tapeSamplesRemaining = 0;
    z80.closeTapeFile();
  }

  void ZX128VM::tapePlay()
  {
    Ep128Emu::VirtualMachine::tapePlay();
    if (haveTape() && getTapeButtonState() != 0)
      stopDemo();
  }

  void ZX128VM::tapeRecord()
  {
    Ep128Emu::VirtualMachine::tapeRecord();
    if (haveTape() && getTapeButtonState() != 0)
      stopDemo();
  }

  void ZX128VM::tapeStop()
  {
    Ep128Emu::VirtualMachine::tapeStop();
    z80.closeTapeFile();
  }

  void ZX128VM::tapeSeek(double t)
  {
    Ep128Emu::VirtualMachine::tapeSeek(t);
    if (t <= 0.0)
      z80.rewindTapeFile();
  }

  void ZX128VM::setBreakPoint(const Ep128Emu::BreakPoint& bp, bool isEnabled)
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

  void ZX128VM::clearBreakPoints()
  {
    memory.clearAllBreakPoints();
    ioPorts.clearBreakPoints();
  }

  void ZX128VM::setBreakPointPriorityThreshold(int n)
  {
    breakPointPriorityThreshold = uint8_t(n > 0 ? (n < 4 ? n : 4) : 0);
    if (!(singleStepMode == 1 || singleStepMode == 2)) {
      memory.setBreakPointPriorityThreshold(n);
      ioPorts.setBreakPointPriorityThreshold(n);
    }
  }

  void ZX128VM::setSingleStepMode(int mode_)
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

  void ZX128VM::setSingleStepModeNextAddress(int32_t addr)
  {
    if ((singleStepMode != 2 && singleStepMode != 4) || addr < 0)
      addr = int32_t(-1);
    else
      addr &= int32_t(0xFFFF);
    singleStepModeNextAddr = addr;
  }

  uint8_t ZX128VM::getMemoryPage(int n) const
  {
    return memory.getPage(uint8_t(n & 3));
  }

  uint8_t ZX128VM::readMemory(uint32_t addr, bool isCPUAddress) const
  {
    if (isCPUAddress)
      addr = ((uint32_t(memory.getPage(uint8_t(addr >> 14) & uint8_t(3))) << 14)
              | (addr & uint32_t(0x3FFF)));
    else
      addr &= uint32_t(0x003FFFFF);
    return memory.readRaw(addr);
  }

  void ZX128VM::writeMemory(uint32_t addr, uint8_t value, bool isCPUAddress)
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

  void ZX128VM::writeROM(uint32_t addr, uint8_t value)
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    memory.writeROM(addr & uint32_t(0x003FFFFF), value);
  }

  uint8_t ZX128VM::readIOPort(uint16_t addr) const
  {
    return ioPorts.readDebug(addr);
  }

  void ZX128VM::writeIOPort(uint16_t addr, uint8_t value)
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    ioPorts.writeDebug(addr, value);
  }

  uint16_t ZX128VM::getProgramCounter() const
  {
    return z80.getProgramCounter();
  }

  void ZX128VM::setProgramCounter(uint16_t addr)
  {
    if (addr != z80.getProgramCounter()) {
      if (isRecordingDemo | isPlayingDemo) {
        stopDemoPlayback();
        stopDemoRecording(false);
      }
      z80.setProgramCounter(addr);
    }
  }

  uint16_t ZX128VM::getStackPointer() const
  {
    return uint16_t(z80.getReg().SP.W);
  }

  void ZX128VM::listCPURegisters(std::string& buf) const
  {
    char    tmpBuf[256];
    const Ep128::Z80_REGISTERS& r = z80.getReg();
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

  void ZX128VM::listIORegisters(std::string& buf) const
  {
    unsigned int  tmpBuf[16];
    char    tmpBuf2[320];
    char    *bufp = &(tmpBuf2[0]);
    int     n;
    n = std::sprintf(bufp,
                     "Joy   1F: %02X\n", (unsigned int) joystickState);
    bufp = bufp + n;
    n = std::sprintf(bufp,
                     "128 7FFD: %02X\n",
                     (unsigned int) spectrum128PageRegister);
    bufp = bufp + n;
    n = std::sprintf(bufp,
                     "AY3 FFFD: %02X\n", (unsigned int) ayRegisterSelected);
    bufp = bufp + n;
    for (uint8_t i = 0; i < 16; i++)
      tmpBuf[i] = ay3.readRegister(i);
    n = std::sprintf(bufp,
                     "AY3 BFFD: %02X %02X %02X %02X  %02X %02X %02X %02X\n"
                     "          %02X %02X %02X %02X  %02X %02X %02X %02X\n",
                     tmpBuf[0], tmpBuf[1], tmpBuf[2], tmpBuf[3],
                     tmpBuf[4], tmpBuf[5], tmpBuf[6], tmpBuf[7],
                     tmpBuf[8], tmpBuf[9], tmpBuf[10], tmpBuf[11],
                     tmpBuf[12], tmpBuf[13], tmpBuf[14], tmpBuf[15]);
    bufp = bufp + n;
    int     tmpX = 0;
    int     tmpY = 0;
    getVideoPosition(tmpX, tmpY);
    n = std::sprintf(bufp,
                     "ULA   FE: %02X    X: %3d  Y: %3d\n",
                     (unsigned int) ula.readPortDebug(), tmpX, tmpY);
    buf = &(tmpBuf2[0]);
  }

  uint32_t ZX128VM::disassembleInstruction(std::string& buf,
                                           uint32_t addr, bool isCPUAddress,
                                           int32_t offs) const
  {
    return Ep128::Z80Disassembler::disassembleInstruction(
               buf, (*this), addr, isCPUAddress, offs);
  }

  void ZX128VM::getVideoPosition(int& xPos, int& yPos) const
  {
    ula.getVideoPosition(xPos, yPos, z80OpcodeHalfCycles);
  }

  Ep128::Z80_REGISTERS& ZX128VM::getZ80Registers()
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    return z80.getReg();
  }

}       // namespace ZX128

