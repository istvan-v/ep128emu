
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
#include "cpcmem.hpp"
#include "cpcio.hpp"
#include "ay3_8912.hpp"
#include "crtc6845.hpp"
#include "cpcvideo.hpp"
#include "soundio.hpp"
#include "display.hpp"
#include "vm.hpp"
#include "cpc464vm.hpp"
#include "debuglib.hpp"
#include "videorec.hpp"

#include <vector>

static const uint8_t  keyboardConvTable[256] = {
  //   N   BSLASH        B        C        V        X        Z   LSHIFT
  99, 46,  99, 22,  99, 54,  99, 62,  99, 55,  99, 63,  99, 71,  99, 21,
  //   H     LOCK        G        D        F        S        A     CTRL
  99, 44,  99, 70,  99, 52,  99, 61,  99, 53,  99, 60,  99, 69,  99, 23,
  //   U        Q        Y        R        T        E        W      TAB
  99, 42,  99, 67,  99, 43,  99, 50,  99, 51,  99, 58,  99, 59,  99, 68,
  //   7        1        6        4        5        3        2      ESC
  99, 41,  99, 64,  99, 48,  99, 56,  99, 49,  99, 57,  99, 65,  99, 66,
  //  F4       F8       F3       F6       F5       F7       F2       F1
  99, 20,  99, 11,  99,  5,  99,  4,  99, 12,  99, 10,  99, 14,  99, 13,
  //   8                 9        -        0        ^    ERASE
  99, 40,  99, 99,  99, 33,  99, 25,  99, 32,  99, 24,  99, 79,  99, 99,
  //   J                 K        ;        L        :        ]
  99, 45,  99, 99,  99, 37,  99, 28,  99, 36,  99, 29,  99, 19,  99, 99,
  // STOP    DOWN    RIGHT       UP     HOLD     LEFT    ENTER      ALT
  99,  3,  99,  2,  99,  1,  99,  0,  99, 15,  99,  8,  99,  6,  99,  7,
  //   M   DELETE        ,        /        .   RSHIFT    SPACE   INSERT
  99, 38,  99, 16,  99, 39,  99, 30,  99, 31,  99, 18,  99, 47,  99,  9,
  //   I                 O        @        P        [
  99, 35,  99, 99,  99, 34,  99, 26,  99, 27,  99, 17,  99, 99,  99, 99,
  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,
  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,
  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,
  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,  99, 99,
  // JOY1R  JOY1L    JOY1D    JOY1U    JOY1F
  99, 75,  99, 74,  99, 73,  99, 72,  76, 77,  99, 99,  99, 99,  99, 99,
  // JOY2R  JOY2L    JOY2D    JOY2U    JOY2F
  99, 51,  99, 50,  99, 49,  99, 48,  52, 53,  99, 99,  99, 99,  99, 99
};

namespace CPC464 {

  EP128EMU_INLINE void CPC464VM::updateCPUCycles(int cycles)
  {
    z80OpcodeHalfCycles = z80OpcodeHalfCycles + uint8_t(cycles << 1);
  }

  EP128EMU_INLINE void CPC464VM::updateCPUHalfCycles(int halfCycles)
  {
    z80OpcodeHalfCycles = z80OpcodeHalfCycles + uint8_t(halfCycles);
  }

  EP128EMU_INLINE void CPC464VM::memoryWait()
  {
    updateCPUHalfCycles((((~(int(z80OpcodeHalfCycles) + 3)) + 2) & 6) + 5);
    while (z80OpcodeHalfCycles >= 8)
      runOneCycle();
  }

  EP128EMU_INLINE void CPC464VM::memoryWaitM1()
  {
    updateCPUHalfCycles((((~(int(z80OpcodeHalfCycles) + 3)) + 2) & 6) + 4);
    while (z80OpcodeHalfCycles >= 8)
      runOneCycle();
  }

  EP128EMU_INLINE void CPC464VM::ioPortWait()
  {
    updateCPUHalfCycles((((~(int(z80OpcodeHalfCycles) + 5)) + 2) & 6) + 7);
    while (z80OpcodeHalfCycles >= 8)
      runOneCycle();
  }

  EP128EMU_REGPARM1 void CPC464VM::runOneCycle()
  {
    CPC464VMCallback *p = firstCallback;
    while (p) {
      CPC464VMCallback *nxt = p->nxt;
      p->func(p->userData);
      p = nxt;
    }
    if (--ayCycleCnt == 0) {
      ayCycleCnt = 8;
      uint16_t  tmpA = 0;
      uint16_t  tmpB = 0;
      uint16_t  tmpC = 0;
      ay3.runOneCycle(tmpA, tmpB, tmpC);
      tmpB = tmpB + (uint16_t(tapeInputSignal) << 12);
      uint32_t  tmpL = (((uint32_t(tmpA) * 3U) + 1U) >> 1) + uint32_t(tmpB);
      uint32_t  tmpR = (((uint32_t(tmpC) * 3U) + 1U) >> 1) + uint32_t(tmpB);
      soundOutputSignal = (tmpR << 16) | tmpL;
      sendAudioOutput(soundOutputSignal);
    }
    videoRenderer.runOneCycle();
    bool    crtcPrvHSyncState = crtc.getHSyncState();
    bool    crtcPrvVSyncState = crtc.getVSyncState();
    crtc.runOneCycle();
    if (uint8_t(crtc.getHSyncState()) < uint8_t(crtcPrvHSyncState)) {
      gateArrayIRQCounter++;
      if (gateArrayVSyncDelay > 0) {
        if (--gateArrayVSyncDelay == 0) {
          if (gateArrayIRQCounter >= 32)
            z80.triggerInterrupt();
          gateArrayIRQCounter = 0;
        }
      }
      if (gateArrayIRQCounter >= 52) {
        gateArrayIRQCounter = 0;
        z80.triggerInterrupt();
      }
    }
    if (uint8_t(crtc.getVSyncState()) > uint8_t(crtcPrvVSyncState)) {
      gateArrayVSyncDelay = 2;
    }
    crtcCyclesRemainingH--;
    z80OpcodeHalfCycles = z80OpcodeHalfCycles - 8;
  }

  // --------------------------------------------------------------------------

  CPC464VM::Z80_::Z80_(CPC464VM& vm_)
    : Ep128::Z80(),
      vm(vm_)
  {
  }

  CPC464VM::Z80_::~Z80_()
  {
  }

  EP128EMU_REGPARM1 void CPC464VM::Z80_::executeInterrupt()
  {
    vm.gateArrayIRQCounter = vm.gateArrayIRQCounter & 0x1F;
    clearInterrupt();
    Ep128::Z80::executeInterrupt();
  }

  EP128EMU_REGPARM2 uint8_t CPC464VM::Z80_::readMemory(uint16_t addr)
  {
    vm.memoryWait();
    uint8_t   retval = vm.memory.read(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM2 uint16_t CPC464VM::Z80_::readMemoryWord(uint16_t addr)
  {
    vm.memoryWait();
    uint16_t  retval = vm.memory.read(addr);
    vm.updateCPUHalfCycles(1);
    addr = (addr + 1) & 0xFFFF;
    vm.memoryWait();
    retval |= (uint16_t(vm.memory.read(addr) << 8));
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM1 uint8_t CPC464VM::Z80_::readOpcodeFirstByte()
  {
    uint16_t  addr = uint16_t(R.PC.W.l);
    vm.memoryWaitM1();
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

  EP128EMU_REGPARM1 uint8_t CPC464VM::Z80_::readOpcodeSecondByte()
  {
    uint16_t  addr = (uint16_t(R.PC.W.l) + uint16_t(1)) & uint16_t(0xFFFF);
    vm.memoryWaitM1();
    uint8_t   retval = vm.memory.readOpcode(addr);
    vm.updateCPUHalfCycles(4);
    return retval;
  }

  EP128EMU_REGPARM2 uint8_t CPC464VM::Z80_::readOpcodeByte(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    vm.memoryWait();
    uint8_t   retval = vm.memory.readOpcode(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM2 uint16_t CPC464VM::Z80_::readOpcodeWord(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    vm.memoryWait();
    uint16_t  retval = vm.memory.readOpcode(addr);
    vm.updateCPUHalfCycles(1);
    addr = (addr + 1) & 0xFFFF;
    vm.memoryWait();
    retval |= (uint16_t(vm.memory.readOpcode(addr) << 8));
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM3 void CPC464VM::Z80_::writeMemory(uint16_t addr,
                                                     uint8_t value)
  {
    vm.memoryWait();
    vm.memory.write(addr, value);
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM3 void CPC464VM::Z80_::writeMemoryWord(uint16_t addr,
                                                         uint16_t value)
  {
    vm.memoryWait();
    vm.memory.write(addr, uint8_t(value) & 0xFF);
    vm.updateCPUHalfCycles(1);
    addr = (addr + 1) & 0xFFFF;
    vm.memoryWait();
    vm.memory.write(addr, uint8_t(value >> 8));
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM2 void CPC464VM::Z80_::pushWord(uint16_t value)
  {
    vm.updateCPUHalfCycles(2);
    R.SP.W -= 2;
    uint16_t  addr = R.SP.W;
    vm.memoryWait();
    vm.memory.write((addr + 1) & 0xFFFF, uint8_t(value >> 8));
    vm.updateCPUHalfCycles(1);
    vm.memoryWait();
    vm.memory.write(addr, uint8_t(value) & 0xFF);
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM3 void CPC464VM::Z80_::doOut(uint16_t addr, uint8_t value)
  {
    vm.ioPortWait();
    vm.ioPorts.write(addr, value);
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM2 uint8_t CPC464VM::Z80_::doIn(uint16_t addr)
  {
    vm.ioPortWait();
    uint8_t   retval = vm.ioPorts.read(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM1 void CPC464VM::Z80_::updateCycle()
  {
    vm.updateCPUHalfCycles(2);
    while (vm.z80OpcodeHalfCycles >= 8)
      vm.runOneCycle();
  }

  EP128EMU_REGPARM2 void CPC464VM::Z80_::updateCycles(int cycles)
  {
    vm.updateCPUCycles(cycles);
    while (vm.z80OpcodeHalfCycles >= 8)
      vm.runOneCycle();
  }

  // --------------------------------------------------------------------------

  CPC464VM::Memory_::Memory_(CPC464VM& vm_)
    : Memory(),
      vm(vm_)
  {
  }

  CPC464VM::Memory_::~Memory_()
  {
  }

  void CPC464VM::Memory_::breakPointCallback(bool isWrite,
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

  CPC464VM::IOPorts_::IOPorts_(CPC464VM& vm_)
    : IOPorts(),
      vm(vm_)
  {
  }

  CPC464VM::IOPorts_::~IOPorts_()
  {
  }

  void CPC464VM::IOPorts_::breakPointCallback(bool isWrite,
                                              uint16_t addr, uint8_t value)
  {
    if (!vm.memory.checkIgnoreBreakPoint(vm.z80.getReg().PC.W.l)) {
      vm.breakPointCallback(vm.breakPointCallbackUserData,
                            int(isWrite) + 5, addr, value);
    }
  }

  // --------------------------------------------------------------------------

  CPC464VM::CPCVideo_::CPCVideo_(CPC464VM& vm_,
                                 const CRTC6845& crtc_,
                                 const uint8_t *videoMemory_)
    : CPCVideo(crtc_, videoMemory_),
      vm(vm_)
  {
  }

  CPC464VM::CPCVideo_::~CPCVideo_()
  {
  }

  void CPC464VM::CPCVideo_::drawLine(const uint8_t *buf, size_t nBytes)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.drawLine(buf, nBytes);
    if (vm.videoCapture)
      vm.videoCapture->horizontalSync(buf, nBytes);
  }

  void CPC464VM::CPCVideo_::vsyncStateChange(bool newState,
                                             unsigned int currentSlot_)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.vsyncStateChange(newState, currentSlot_);
    if (vm.videoCapture)
      vm.videoCapture->vsyncStateChange(newState, currentSlot_);
  }

  // --------------------------------------------------------------------------

  void CPC464VM::stopDemoPlayback()
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

  void CPC464VM::stopDemoRecording(bool writeFile_)
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
        demoFile->addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_CPC_DEMO,
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

  EP128EMU_REGPARM1 void CPC464VM::updatePPIState()
  {
    uint8_t ppiPortAInput = 0xFF;
    uint8_t ppiPortBInput =
        uint8_t((int(tapeInputSignal) << 7) | int(crtc.getVSyncState()) | 0x7E);
    uint8_t ppiPortCInput = 0xEF;
    if ((ppiControlRegister & 0x04) != 0) {     // mode 1 is not supported
      ppiPortCState = ppiPortCInput | 0xF0;
      ppiPortBState = ppiPortBInput;
    }
    else {
      if ((ppiControlRegister & 0x01) != 0)     // port C b0-b3: mode 0, input
        ppiPortCState = ppiPortCInput | 0xF0;
      else                                      // port C b0-b3: mode 0, output
        ppiPortCState = ppiPortCRegister | 0xF0;
      if ((ppiControlRegister & 0x02) != 0)     // port B: mode 0, input
        ppiPortBState = ppiPortBInput;
      else                                      // port B: mode 0, output
        ppiPortBState = ppiPortBInput & ppiPortBRegister;
    }
    ay3.setPortAInput(cpcKeyboardState[ppiPortCState & 0x0F]);
    if ((ppiControlRegister & 0x60) != 0x00) {  // modes 1, 2 are not supported
      ppiPortCState &= (ppiPortCInput | 0x0F);
      ppiPortAState = ppiPortAInput;
    }
    else {
      if ((ppiControlRegister & 0x08) != 0)     // port C b4-b7: mode 0, input
        ppiPortCState &= (ppiPortCInput | 0x0F);
      else                                      // port C b4-b7: mode 0, output
        ppiPortCState &= (ppiPortCRegister | 0x0F);
      if ((ppiPortCState & 0xC0) == 0x40)   // read AY register
        ppiPortAInput = ay3.readRegister(ayRegisterSelected & 0x0F);
      if ((ppiControlRegister & 0x10) != 0)     // port A: mode 0, input
        ppiPortAState = ppiPortAInput;
      else                                      // port A: mode 0, output
        ppiPortAState = ppiPortAInput & ppiPortARegister;
    }
    setTapeMotorState(bool(ppiPortCState & 0x10));
    switch (ppiPortCState & 0xC0) {
    case 0x80:                          // write AY register
      ay3.writeRegister(ayRegisterSelected & 0x0F, ppiPortAState);
      break;
    case 0xC0:                          // select AY register
      ayRegisterSelected = ppiPortAState;
      break;
    }
  }

  uint8_t CPC464VM::ioPortReadCallback(void *userData, uint16_t addr)
  {
    CPC464VM&  vm = *(reinterpret_cast<CPC464VM *>(userData));
    uint8_t   retval = 0xFF;
    if ((addr & 0x4000) == 0) {         // CRTC (register number is in A8-A9)
      if ((addr & 0x0300) == 0x0300)    // read CRTC register
        retval &= vm.crtc.readRegister(vm.crtcRegisterSelected & 0x1F);
    }
    if ((addr & 0x0800) == 0) {         // PPI (register number is in A8-A9)
      vm.updatePPIState();
      switch (addr & 0x0300) {
      case 0x0000:              // port A data
        retval &= vm.ppiPortAState;
        break;
      case 0x0100:              // port B data
        retval &= vm.ppiPortBState;
        break;
      case 0x0200:              // port C data
        retval &= vm.ppiPortCState;
        break;
      }
    }
#if 0
    if ((addr & 0x0400) == 0) {         // expansion peripherals (floppy etc.)
    }
#endif
    return retval;
  }

  void CPC464VM::ioPortWriteCallback(void *userData,
                                     uint16_t addr, uint8_t value)
  {
    CPC464VM&  vm = *(reinterpret_cast<CPC464VM *>(userData));
    if ((addr & 0xC000) == 0x4000) {    // gate array
      switch (value & 0xC0) {
      case 0x00:                // select pen
        vm.gateArrayPenSelected = value & 0x3F;
        break;
      case 0x40:                // select color
        vm.videoRenderer.setColor(vm.gateArrayPenSelected & 0x1F, value & 0x3F);
        break;
      case 0x80:                // video mode, ROM disable, interrupt delay
        vm.videoRenderer.setVideoMode(value & 0x03);
        vm.memory.setPaging((vm.memory.getPaging() & 0xFF3F)
                            | uint16_t(((~value) & 0x0C) << 4));
        if (value & 0x10)
          vm.gateArrayIRQCounter = 0;
        break;
      }
    }
    if ((addr & 0x8000) == 0) {         // RAM configuration
      if ((value & 0xC0) == 0xC0) {
        vm.memory.setPaging((vm.memory.getPaging() & 0xFFC0)
                            | uint16_t(value & 0x3F));
      }
    }
    if ((addr & 0x4000) == 0) {         // CRTC (register number is in A8-A9)
      switch (addr & 0x0300) {
      case 0x0000:              // select CRTC register
        vm.crtcRegisterSelected = value;
        break;
      case 0x0100:              // write CRTC register
        vm.crtc.writeRegister(vm.crtcRegisterSelected & 0x1F, value);
        break;
      }
    }
    if ((addr & 0x2000) == 0) {         // ROM configuration
      vm.memory.setPaging((vm.memory.getPaging() & 0x00FF)
                          | (uint16_t(value) << 8));
    }
#if 0
    if ((addr & 0x1000) == 0) {         // printer port
    }
#endif
    if ((addr & 0x0800) == 0) {         // PPI (register number is in A8-A9)
      switch (addr & 0x0300) {
      case 0x0000:              // port A data
        vm.ppiPortARegister = value;
        break;
      case 0x0100:              // port B data
        vm.ppiPortBRegister = value;
        break;
      case 0x0200:              // port C data
        vm.ppiPortCRegister = value;
        break;
      case 0x0300:              // control register
        if (!(value & 0x80)) {
          if (!(value & 0x01))
            vm.ppiPortCRegister &= uint8_t(0xFF ^ (1 << ((value & 0x0E) >> 1)));
          else
            vm.ppiPortCRegister |= uint8_t(1 << ((value & 0x0E) >> 1));
        }
        else {
          vm.ppiControlRegister = value;
        }
        break;
      }
      vm.updatePPIState();
    }
#if 0
    if ((addr & 0x0400) == 0) {         // expansion peripherals (floppy etc.)
    }
#endif
  }

  uint8_t CPC464VM::ioPortDebugReadCallback(void *userData, uint16_t addr)
  {
    CPC464VM&  vm = *(reinterpret_cast<CPC464VM *>(userData));
    if (addr < 0x0080) {
      switch (addr & 0x00F0) {
      case 0x0000:                      // CRTC registers
      case 0x0010:
        return vm.crtc.readRegisterDebug(addr & 0x001F);
      case 0x0020:                      // gate array palette
      case 0x0030:
        return vm.videoRenderer.getColor(addr & 0x001F);
      case 0x0040:                      // AY registers
        if ((addr & 0x000F) == 0x000E)
          vm.updatePPIState();
        return vm.ay3.readRegister(addr & 0x000F);
      case 0x0050:                      // PPI registers
        vm.updatePPIState();
        switch (addr & 0x000F) {
        case 0x0000:            // port A state
        case 0x0008:
          return vm.ppiPortAState;
        case 0x0001:            // port B state
        case 0x0009:
          return vm.ppiPortBState;
        case 0x0002:            // port C state
        case 0x000A:
          return vm.ppiPortCState;
        case 0x0003:            // control register
        case 0x0007:
        case 0x000B:
        case 0x000F:
          return vm.ppiControlRegister;
        case 0x0004:            // port A register
        case 0x000C:
          return vm.ppiPortARegister;
        case 0x0005:            // port B register
        case 0x000D:
          return vm.ppiPortBRegister;
        case 0x0006:            // port C register
        case 0x000E:
          return vm.ppiPortCRegister;
        }
      case 0x0060:                      // keyboard matrix
        return vm.cpcKeyboardState[addr & 0x000F];
      case 0x0070:                      // misc. I/O registers
        switch (addr & 0x000F) {
        case 0x0000:            // CRTC register selected
          return vm.crtcRegisterSelected;
        case 0x0001:            // CRTC flags
          return (uint8_t(vm.crtc.getDisplayEnabled())
                  | (uint8_t(vm.crtc.getHSyncState()) << 1)
                  | (uint8_t(vm.crtc.getVSyncState()) << 2)
                  | (uint8_t(vm.crtc.getVSyncInterlace()) << 3)
                  | (uint8_t(vm.crtc.getCursorEnabled()) << 4));
        case 0x0002:            // CRTC address low
          return uint8_t((vm.crtc.getMemoryAddress() & 0x007F) << 1);
        case 0x0003:            // CRTC address high
          return (uint8_t((vm.crtc.getMemoryAddress() & 0x0380) >> 7)
                  | uint8_t((vm.crtc.getRowAddress() & 0x07) << 3)
                  | uint8_t((vm.crtc.getMemoryAddress() & 0x3000) >> 6));
        case 0x0004:            // video mode
          return vm.videoRenderer.getVideoMode();
        case 0x0005:            // gate array pen selected
          return vm.gateArrayPenSelected;
        case 0x0006:            // gate array IRQ counter
          return vm.gateArrayIRQCounter;
        case 0x0007:            // AY register selected
          return vm.ayRegisterSelected;
        case 0x0008:            // RAM configuration
          return uint8_t((vm.memory.getPaging() & 0x00FF) ^ 0x00C0);
        case 0x0009:            // ROM bank selected
          return uint8_t(vm.memory.getPaging() >> 8);
        default:
          return 0xFF;
        }
      }
    }
    uint8_t   retval = 0xFF;
    if ((addr & 0xC000) == 0x4000) {    // gate array
      switch (addr & 0x00C0) {
      case 0x00:                // pen selected
        retval &= vm.gateArrayPenSelected;
        break;
      case 0x40:                // color selected for current pen
        retval &= uint8_t(vm.videoRenderer.getColor(vm.gateArrayPenSelected
                                                    & 0x1F) | 0x40);
        break;
      case 0x80:                // video mode, ROM disable
        retval &= uint8_t(vm.videoRenderer.getVideoMode()
                          | (((~(vm.memory.getPaging())) & 0x00C0) >> 4));
        break;
      }
    }
    if ((addr & 0x8000) == 0) {         // RAM configuration
      if ((addr & 0x00C0) == 0x00C0)
        retval &= uint8_t((vm.memory.getPaging() & 0x003F) | 0x00C0);
    }
    if ((addr & 0x4000) == 0) {         // CRTC (register number is in A8-A9)
      switch (addr & 0x0300) {
      case 0x0000:
        retval &= vm.crtcRegisterSelected;
        break;
      case 0x0100:
      case 0x0300:
        retval &= vm.crtc.readRegisterDebug(vm.crtcRegisterSelected & 0x1F);
        break;
      }
    }
    if ((addr & 0x2000) == 0) {         // ROM configuration
      retval &= uint8_t(vm.memory.getPaging() >> 8);
    }
#if 0
    if ((addr & 0x1000) == 0) {         // printer port
    }
#endif
    if ((addr & 0x0800) == 0) {         // PPI (register number is in A8-A9)
      vm.updatePPIState();
      switch (addr & 0x0300) {
      case 0x0000:              // port A data
        retval &= vm.ppiPortAState;
        break;
      case 0x0100:              // port B data
        retval &= vm.ppiPortBState;
        break;
      case 0x0200:              // port C data
        retval &= vm.ppiPortCState;
        break;
      case 0x0300:              // control register
        retval &= vm.ppiControlRegister;
        break;
      }
    }
#if 0
    if ((addr & 0x0400) == 0) {         // expansion peripherals (floppy etc.)
    }
#endif
    return retval;
  }

  void CPC464VM::tapeCallback(void *userData)
  {
    CPC464VM&  vm = *(reinterpret_cast<CPC464VM *>(userData));
    vm.tapeSamplesRemaining += vm.tapeSamplesPerCRTCCycle;
    if (vm.tapeSamplesRemaining > 0) {
      // assume tape sample rate < crtcFrequency
      vm.tapeSamplesRemaining -= (int64_t(1) << 32);
      uint8_t prvTapeInput = vm.tapeInputSignal;
      vm.tapeInputSignal =
          uint8_t(vm.runTape(int((vm.ppiPortCState & 0x20) >> 5)));
      if (vm.tapeInputSignal != prvTapeInput)
        vm.updatePPIState();
    }
  }

  void CPC464VM::demoPlayCallback(void *userData)
  {
    CPC464VM&  vm = *(reinterpret_cast<CPC464VM *>(userData));
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

  void CPC464VM::demoRecordCallback(void *userData)
  {
    CPC464VM&  vm = *(reinterpret_cast<CPC464VM *>(userData));
    vm.demoTimeCnt++;
  }

  void CPC464VM::videoCaptureCallback(void *userData)
  {
    CPC464VM&  vm = *(reinterpret_cast<CPC464VM *>(userData));
    vm.videoCapture->runOneCycle(vm.soundOutputSignal);
  }

  uint8_t CPC464VM::checkSingleStepModeBreak()
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

  void CPC464VM::convertKeyboardState()
  {
    for (int i = 0; i < 16; i++)
      cpcKeyboardState[i] = 0xFF;
    const uint8_t *p = &(keyboardConvTable[0]);
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 8; j++) {
        if ((keyboardState[i] & uint8_t(1 << j)) == 0) {
          for (int k = 0; k < 2; k++) {
            int     cpcKeyNum = p[k];
            if (cpcKeyNum < 80) {
              cpcKeyboardState[cpcKeyNum >> 3] &=
                  uint8_t(0xFF ^ (1 << (cpcKeyNum & 7)));
            }
          }
        }
        p = p + 2;
      }
    }
    updatePPIState();
  }

  void CPC464VM::resetKeyboard()
  {
    for (int i = 0; i < 16; i++) {
      keyboardState[i] = 0xFF;
      cpcKeyboardState[i] = 0xFF;
    }
    updatePPIState();
  }

  void CPC464VM::setCallback(void (*func)(void *userData), void *userData_,
                             bool isEnabled)
  {
    if (!func)
      return;
    const size_t  maxCallbacks = sizeof(callbacks) / sizeof(CPC464VMCallback);
    int     ndx = -1;
    for (size_t i = 0; i < maxCallbacks; i++) {
      if (callbacks[i].func == func && callbacks[i].userData == userData_) {
        ndx = int(i);
        break;
      }
    }
    if (ndx >= 0) {
      CPC464VMCallback   *prv = (CPC464VMCallback *) 0;
      CPC464VMCallback   *p = firstCallback;
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
        callbacks[ndx].nxt = (CPC464VMCallback *) 0;
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
        throw Ep128Emu::Exception("CPC464VM: too many callbacks");
    }
    callbacks[ndx].func = func;
    callbacks[ndx].userData = userData_;
    callbacks[ndx].nxt = (CPC464VMCallback *) 0;
    if (isEnabled) {
      CPC464VMCallback   *prv = (CPC464VMCallback *) 0;
      CPC464VMCallback   *p = firstCallback;
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

  CPC464VM::CPC464VM(Ep128Emu::VideoDisplay& display_,
                     Ep128Emu::AudioOutput& audioOutput_)
    : VirtualMachine(display_, audioOutput_),
      z80(*this),
      memory(*this),
      ioPorts(*this),
      ay3(),
      crtc(),
      videoRenderer(*this, crtc, memory.getVideoMemory()),
      crtcCyclesRemainingL(0U),
      crtcCyclesRemainingH(0),
      ayRegisterSelected(0x00),
      ayCycleCnt(4),
      z80OpcodeHalfCycles(0),
      ppiPortARegister(0x00),
      ppiPortAState(0xFF),
      ppiPortBRegister(0x00),
      ppiPortBState(0xFF),
      ppiPortCRegister(0x00),
      ppiPortCState(0xFF),
      ppiControlRegister(0x9B),
      tapeInputSignal(0),
      crtcRegisterSelected(0x00),
      gateArrayIRQCounter(0),
      gateArrayVSyncDelay(0),
      gateArrayPenSelected(0x00),
      singleStepMode(0),
      singleStepModeNextAddr(int32_t(-1)),
      tapeCallbackFlag(false),
      soundOutputSignal(0U),
      demoFile((Ep128Emu::File *) 0),
      demoBuffer(),
      isRecordingDemo(false),
      isPlayingDemo(false),
      snapshotLoadFlag(false),
      demoTimeCnt(0UL),
      breakPointPriorityThreshold(0),
      firstCallback((CPC464VMCallback *) 0),
      videoCapture((Ep128Emu::VideoCapture *) 0),
      tapeSamplesPerCRTCCycle(0L),
      tapeSamplesRemaining(0L),
      crtcFrequency(1000000)
  {
    for (size_t i = 0;
         i < (sizeof(callbacks) / sizeof(CPC464VMCallback));
         i++) {
      callbacks[i].func = (void (*)(void *)) 0;
      callbacks[i].userData = (void *) 0;
      callbacks[i].nxt = (CPC464VMCallback *) 0;
    }
    // register I/O callbacks
    ioPorts.setCallbackUserData((void *) this);
    ioPorts.setReadCallback(&ioPortReadCallback);
    ioPorts.setWriteCallback(&ioPortWriteCallback);
    ioPorts.setDebugReadCallback(&ioPortDebugReadCallback);
    // hack to get some programs using interrupt mode 2 working
    z80.setVectorBase(0xFF);
    // use CPC gate array colormap
    Ep128Emu::VideoDisplay::DisplayParameters
        dp(display.getDisplayParameters());
    dp.indexToRGBFunc = &CPCVideo::convertPixelToRGB;
    display.setDisplayParameters(dp);
    setAudioConverterSampleRate(float(long(crtcFrequency >> 3)));
    // reset
    resetKeyboard();            // this also initializes the PPI state
    resetMemoryConfiguration(128);
  }

  CPC464VM::~CPC464VM()
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

  void CPC464VM::run(size_t microseconds)
  {
    Ep128Emu::VirtualMachine::run(microseconds);
    if (snapshotLoadFlag) {
      snapshotLoadFlag = false;
      // if just loaded a snapshot, and not playing a demo,
      // clear keyboard state
      if (!isPlayingDemo)
        resetKeyboard();
    }
    bool    newTapeCallbackFlag =
        (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0);
    if (newTapeCallbackFlag != tapeCallbackFlag) {
      tapeCallbackFlag = newTapeCallbackFlag;
      if (!tapeCallbackFlag && tapeInputSignal != 0) {
        tapeInputSignal = 0;
        updatePPIState();
      }
      setCallback(&tapeCallback, this, tapeCallbackFlag);
    }
    z80OpcodeHalfCycles = z80OpcodeHalfCycles & 0xFE;
    int64_t crtcCyclesRemaining =
        int64_t(crtcCyclesRemainingL) + (int64_t(crtcCyclesRemainingH) << 32)
        + ((int64_t(microseconds) << 26) * int64_t(crtcFrequency)
           / int64_t(15625));   // 10^6 / 2^6
    crtcCyclesRemainingL =
        uint32_t(uint64_t(crtcCyclesRemaining) & 0xFFFFFFFFUL);
    crtcCyclesRemainingH = int32_t(crtcCyclesRemaining >> 32);
    while (crtcCyclesRemainingH > 0) {
      z80.executeInstruction();
      while (z80OpcodeHalfCycles >= 8)
        runOneCycle();
    }
  }

  void CPC464VM::reset(bool isColdReset)
  {
    stopDemoPlayback();         // TODO: should be recorded as an event ?
    stopDemoRecording(false);
    z80.reset();
    memory.setPaging(0x00C0);
    ay3.reset();
    crtc.reset();
    videoRenderer.reset();
    if (isColdReset)
      resetKeyboard();
    ayRegisterSelected = 0x00;
    ppiPortARegister = 0x00;
    ppiPortBRegister = 0x00;
    ppiPortCRegister = 0x00;
    ppiControlRegister = 0x9B;
    updatePPIState();
    crtcRegisterSelected = 0x00;
    gateArrayIRQCounter = 0;
    gateArrayVSyncDelay = 0;
    gateArrayPenSelected = 0x00;
  }

  void CPC464VM::resetMemoryConfiguration(size_t memSize)
  {
    stopDemo();
    // delete all ROM segments
    for (int i = 0; i < 256; i++) {
      if (memory.isSegmentROM(uint8_t(i)))
        memory.deleteSegment(uint8_t(i));
    }
    // resize RAM
    memory.setRAMSize(memSize);
    // cold reset
    this->reset(true);
  }

  void CPC464VM::loadROMSegment(uint8_t n, const char *fileName, size_t offs)
  {
    stopDemo();
    if (n < 0xC0 && n != 0x80)
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
    memory.loadROMSegment(n, &(buf.front()), 0x4000);
  }

  void CPC464VM::setVideoFrequency(size_t freq_)
  {
    size_t  freq;
    // allow refresh rates in the range 10 Hz to 100 Hz
    freq = ((freq_ > 400000 ? (freq_ < 2000000 ? freq_ : 2000000) : 400000) + 4)
           & (~(size_t(7)));
    if (freq == crtcFrequency)
      return;
    crtcFrequency = freq;
    stopDemoPlayback();         // changing configuration implies stopping
    stopDemoRecording(false);   // any demo playback or recording
    setAudioConverterSampleRate(float(long(crtcFrequency >> 3)));
    if (haveTape()) {
      tapeSamplesPerCRTCCycle =
          (int64_t(getTapeSampleRate()) << 32) / int64_t(crtcFrequency);
    }
    else {
      tapeSamplesPerCRTCCycle = 0L;
    }
    if (videoCapture)
      videoCapture->setClockFrequency(crtcFrequency);
  }

  void CPC464VM::setKeyboardState(int keyCode, bool isPressed)
  {
    if (!isPlayingDemo) {
      int     rowNum = (keyCode & 0x78) >> 3;
      uint8_t mask = uint8_t(1 << (keyCode & 0x07));
      keyboardState[rowNum] = (keyboardState[rowNum] & (~mask))
                              | ((uint8_t(isPressed) - 1) & mask);
      convertKeyboardState();
    }
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

  void CPC464VM::getVMStatus(VMStatus& vmStatus_)
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

  void CPC464VM::openVideoCapture(
      int frameRate_,
      bool yuvFormat_,
      void (*errorCallback_)(void *userData, const char *msg),
      void (*fileNameCallback_)(void *userData, std::string& fileName),
      void *userData_)
  {
    if (!videoCapture) {
      if (yuvFormat_) {
        videoCapture = new Ep128Emu::VideoCapture_YV12(
                               &CPCVideo::convertPixelToRGB, frameRate_);
      }
      else {
        videoCapture = new Ep128Emu::VideoCapture_RLE8(
                               &CPCVideo::convertPixelToRGB, frameRate_);
      }
      videoCapture->setClockFrequency(crtcFrequency);
      setCallback(&videoCaptureCallback, this, true);
    }
    videoCapture->setErrorCallback(errorCallback_, userData_);
    videoCapture->setFileNameCallback(fileNameCallback_, userData_);
  }

  void CPC464VM::setVideoCaptureFile(const std::string& fileName_)
  {
    if (!videoCapture) {
      throw Ep128Emu::Exception("internal error: "
                                "video capture object does not exist");
    }
    videoCapture->openFile(fileName_.c_str());
  }

  void CPC464VM::closeVideoCapture()
  {
    if (videoCapture) {
      setCallback(&videoCaptureCallback, this, false);
      delete videoCapture;
      videoCapture = (Ep128Emu::VideoCapture *) 0;
    }
  }

  void CPC464VM::setTapeFileName(const std::string& fileName)
  {
    Ep128Emu::VirtualMachine::setTapeFileName(fileName);
    if (haveTape()) {
      tapeSamplesPerCRTCCycle =
          (int64_t(getTapeSampleRate()) << 32) / int64_t(crtcFrequency);
    }
    tapeSamplesRemaining = 0;
  }

  void CPC464VM::tapePlay()
  {
    Ep128Emu::VirtualMachine::tapePlay();
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void CPC464VM::tapeRecord()
  {
    Ep128Emu::VirtualMachine::tapeRecord();
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void CPC464VM::tapeStop()
  {
    Ep128Emu::VirtualMachine::tapeStop();
  }

  void CPC464VM::tapeSeek(double t)
  {
    Ep128Emu::VirtualMachine::tapeSeek(t);
  }

  void CPC464VM::setBreakPoints(const Ep128Emu::BreakPointList& bpList)
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

  Ep128Emu::BreakPointList CPC464VM::getBreakPoints()
  {
    Ep128Emu::BreakPointList  bpl1(ioPorts.getBreakPointList());
    Ep128Emu::BreakPointList  bpl2(memory.getBreakPointList());
    for (size_t i = 0; i < bpl1.getBreakPointCnt(); i++) {
      const Ep128Emu::BreakPoint& bp = bpl1.getBreakPoint(i);
      bpl2.addIOBreakPoint(bp.addr(), bp.isRead(), bp.isWrite(), bp.priority());
    }
    return bpl2;
  }

  void CPC464VM::clearBreakPoints()
  {
    memory.clearAllBreakPoints();
    ioPorts.clearBreakPoints();
  }

  void CPC464VM::setBreakPointPriorityThreshold(int n)
  {
    breakPointPriorityThreshold = uint8_t(n > 0 ? (n < 4 ? n : 4) : 0);
    if (!(singleStepMode == 1 || singleStepMode == 2)) {
      memory.setBreakPointPriorityThreshold(n);
      ioPorts.setBreakPointPriorityThreshold(n);
    }
  }

  void CPC464VM::setSingleStepMode(int mode_)
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

  void CPC464VM::setSingleStepModeNextAddress(int32_t addr)
  {
    if ((singleStepMode != 2 && singleStepMode != 4) || addr < 0)
      addr = int32_t(-1);
    else
      addr &= int32_t(0xFFFF);
    singleStepModeNextAddr = addr;
  }

  uint8_t CPC464VM::getMemoryPage(int n) const
  {
    return memory.getPage(uint8_t(n & 3));
  }

  uint8_t CPC464VM::readMemory(uint32_t addr, bool isCPUAddress) const
  {
    if (isCPUAddress)
      addr = ((uint32_t(memory.getPage(uint8_t(addr >> 14) & uint8_t(3))) << 14)
              | (addr & uint32_t(0x3FFF)));
    else
      addr &= uint32_t(0x003FFFFF);
    return memory.readRaw(addr);
  }

  void CPC464VM::writeMemory(uint32_t addr, uint8_t value, bool isCPUAddress)
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

  uint8_t CPC464VM::readIOPort(uint16_t addr) const
  {
    return ioPorts.readDebug(addr);
  }

  void CPC464VM::writeIOPort(uint16_t addr, uint8_t value)
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    ioPorts.writeDebug(addr, value);
  }

  uint16_t CPC464VM::getProgramCounter() const
  {
    return z80.getProgramCounter();
  }

  void CPC464VM::setProgramCounter(uint16_t addr)
  {
    if (addr != z80.getProgramCounter()) {
      if (isRecordingDemo | isPlayingDemo) {
        stopDemoPlayback();
        stopDemoRecording(false);
      }
      z80.setProgramCounter(addr);
    }
  }

  uint16_t CPC464VM::getStackPointer() const
  {
    return uint16_t(z80.getReg().SP.W);
  }

  void CPC464VM::listCPURegisters(std::string& buf) const
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

  void CPC464VM::listIORegisters(std::string& buf) const
  {
    unsigned int  tmpBuf[16];
    char    tmpBuf2[320];
    char    *bufp = &(tmpBuf2[0]);
    int     n;
    for (uint8_t i = 0; i < 16; i++)
      tmpBuf[i] = crtc.readRegisterDebug(i);
    n = std::sprintf(bufp,
                     "CRTC 0-7: %02X %02X %02X %02X  %02X %02X %02X %02X\n"
                     "CRTC 8-F: %02X %02X %02X %02X  %02X %02X %02X %02X\n",
                     tmpBuf[0], tmpBuf[1], tmpBuf[2], tmpBuf[3],
                     tmpBuf[4], tmpBuf[5], tmpBuf[6], tmpBuf[7],
                     tmpBuf[8], tmpBuf[9], tmpBuf[10], tmpBuf[11],
                     tmpBuf[12], tmpBuf[13], tmpBuf[14], tmpBuf[15]);
    bufp = bufp + n;
    n = std::sprintf(bufp,
                     "8255 PPI: AO: %02X AI: %02X  BO: %02X BI: %02X\n"
                     "8255 PPI: CO: %02X CI: %02X  Control: %02X\n",
                     (unsigned int) ppiPortARegister,
                     (unsigned int) ppiPortAState,
                     (unsigned int) ppiPortBRegister,
                     (unsigned int) ppiPortBState,
                     (unsigned int) ppiPortCRegister,
                     (unsigned int) ppiPortCState,
                     (unsigned int) ppiControlRegister);
    bufp = bufp + n;
    {
      int     xPos = 0;
      int     yPos = 0;
      getVideoPosition(xPos, yPos);
      n = std::sprintf(bufp,
                       "AY3 sel.: %02X  Mem: %04X  VidXY: %3d,%3d\n",
                       (unsigned int) ayRegisterSelected,
                       (unsigned int) (memory.getPaging() ^ 0x00C0),
                       xPos, yPos);
    }
    bufp = bufp + n;
    for (uint8_t i = 0; i < 16; i++)
      tmpBuf[i] = ay3.readRegister(i);
    n = std::sprintf(bufp,
                     "AY3 reg.: %02X %02X %02X %02X  %02X %02X %02X %02X\n"
                     "          %02X %02X %02X %02X  %02X %02X %02X %02X\n",
                     tmpBuf[0], tmpBuf[1], tmpBuf[2], tmpBuf[3],
                     tmpBuf[4], tmpBuf[5], tmpBuf[6], tmpBuf[7],
                     tmpBuf[8], tmpBuf[9], tmpBuf[10], tmpBuf[11],
                     tmpBuf[12], tmpBuf[13], tmpBuf[14], tmpBuf[15]);
    bufp = bufp + n;
    buf = &(tmpBuf2[0]);
  }

  uint32_t CPC464VM::disassembleInstruction(std::string& buf,
                                            uint32_t addr, bool isCPUAddress,
                                            int32_t offs) const
  {
    return Ep128::Z80Disassembler::disassembleInstruction(
               buf, (*this), addr, isCPUAddress, offs);
  }

  void CPC464VM::getVideoPosition(int& xPos, int& yPos) const
  {
    crtc.getVideoPosition(xPos, yPos);
  }

  Ep128::Z80_REGISTERS& CPC464VM::getZ80Registers()
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    return z80.getReg();
  }

}       // namespace CPC464

