
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
#include "tvcmem.hpp"
#include "ioports.hpp"
#include "crtc6845.hpp"
#include "tvcvideo.hpp"
#include "soundio.hpp"
#include "display.hpp"
#include "vm.hpp"
#include "tvc64vm.hpp"
#include "debuglib.hpp"
#include "videorec.hpp"

#include <vector>

static const uint8_t  keyboardConvTable[128] = {
  //   N   BSLASH        B        C        V        X        Z   LSHIFT
      52,      45,      48,      49,      55,      50,      54,      51,
  //   H     LOCK        G        D        F        S        A     CTRL
      36,      53,      32,      33,      39,      34,      38,      60,
  //   U        Q        Y        R        T        E        W      TAB
      31,      22,      20,      23,      16,      17,      18,      19,
  //   7        1        6        4        5        3        2      ESC
      15,       6,       4,       7,       0,       1,       2,       5,
  //  F4       F8       F3       F6       F5       F7       F2       F1
      35,      24,      37,       8,      12,      28,      19,      21,
  //   8                 9        -        0        ^    ERASE
       9,      99,      10,      11,      14,      13,      40,      99,
  //   J                 K        ;        L        :        ]
      47,      99,      41,      46,      42,      43,      29,      99,
  // STOP    DOWN    RIGHT       UP     HOLD     LEFT    ENTER      ALT
      59,      66,      69,      65,      12,      70,      44,      56,
  //   M   DELETE        ,        /        .   RSHIFT    SPACE   INSERT
      63,      40,      57,      62,      58,      51,      61,      64,
  //   I                 O        @        P        [
      25,      99,      26,       3,      30,      27,      99,      99,
      99,      99,      99,      99,      99,      99,      99,      99,
      99,      99,      99,      99,      99,      99,      99,      99,
      99,      99,      99,      99,      99,      99,      99,      99,
      99,      99,      99,      99,      99,      99,      99,      99,
  // JOY1R  JOY1L    JOY1D    JOY1U    JOY1F   JOY1F2   JOY1F3
      69,      70,      66,      65,      67,      68,      99,      99,
  // JOY2R  JOY2L    JOY2D    JOY2U    JOY2F   JOY2F2   JOY2F3
      77,      78,      74,      73,      75,      76,      99,      99
};

// TODO: accurate emulation of output levels
static const uint16_t audioOutputLevelTable[16] = {
      0,  2000,  4000,  6000,  8000, 10000, 12000, 14000,
  16000, 18000, 20000, 22000, 24000, 26000, 28000, 30000
};

namespace TVC64 {

  EP128EMU_INLINE void TVC64VM::updateCPUCycles(int cycles)
  {
    z80OpcodeHalfCycles = z80OpcodeHalfCycles + uint8_t(cycles << 1);
  }

  EP128EMU_INLINE void TVC64VM::updateCPUHalfCycles(int halfCycles)
  {
    z80OpcodeHalfCycles = z80OpcodeHalfCycles + uint8_t(halfCycles);
  }

  EP128EMU_INLINE void TVC64VM::memoryWait()
  {
    updateCPUHalfCycles(((~(int(z80OpcodeHalfCycles) + 3)) & 6) + 5);
    do {
      runOneCycle();
    } while (EP128EMU_UNLIKELY(z80OpcodeHalfCycles >= 8));
  }

  EP128EMU_INLINE void TVC64VM::memoryWaitM1()
  {
    updateCPUHalfCycles(((~(int(z80OpcodeHalfCycles) + 3)) & 6) + 4);
    // assume z80OpcodeHalfCycles is always even here, so it should be >= 8
    do {
      runOneCycle();
    } while (EP128EMU_UNLIKELY(z80OpcodeHalfCycles >= 8));
  }

  EP128EMU_INLINE void TVC64VM::ioPortWait()
  {
    updateCPUHalfCycles(((~(int(z80OpcodeHalfCycles) + 5)) & 6) + 7);
    do {
      runOneCycle();
    } while (z80OpcodeHalfCycles >= 8);
  }

  EP128EMU_REGPARM1 void TVC64VM::runOneCycle()
  {
    TVC64VMCallback *p = firstCallback;
    while (p) {
      TVC64VMCallback *nxt = p->nxt;
      p->func(p->userData);
      p = nxt;
    }
    if (++toneGenCnt1 >= 4096U) {
      toneGenCnt1 = toneGenFreq;
      toneGenCnt2++;
    }
    if (++toneGenCnt1 >= 4096U) {
      toneGenCnt1 = toneGenFreq;
      toneGenCnt2++;
    }
    if (--audioCycleCnt == 0) {
      audioCycleCnt = 4;
      soundOutputSignal = uint32_t(tapeInputSignal) << 12;
      toneGenCnt2 = toneGenCnt2 & 0x0F;
      if (toneGenCnt2 & 0x08)
        soundOutputSignal += uint32_t(audioOutputLevelTable[audioOutputLevel]);
    }
    videoRenderer.runOneCycle();
    crtc.runOneCycle();
    crtcCyclesRemainingH--;
    z80OpcodeHalfCycles = z80OpcodeHalfCycles - 4;
  }

  // --------------------------------------------------------------------------

  TVC64VM::Z80_::Z80_(TVC64VM& vm_)
    : Ep128::Z80(),
      vm(vm_)
  {
  }

  TVC64VM::Z80_::~Z80_()
  {
  }

  EP128EMU_REGPARM1 void TVC64VM::Z80_::executeInterrupt()
  {
    vm.updateCPUHalfCycles((~(int(vm.z80OpcodeHalfCycles) + 7)) & 6);
    vm.gateArrayIRQCounter = vm.gateArrayIRQCounter & 0x1F;
    clearInterrupt();
    Ep128::Z80::executeInterrupt();
  }

  EP128EMU_REGPARM2 uint8_t TVC64VM::Z80_::readMemory(uint16_t addr)
  {
    vm.memoryWait();
    uint8_t   retval = vm.memory.read(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM2 uint16_t TVC64VM::Z80_::readMemoryWord(uint16_t addr)
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

  EP128EMU_REGPARM1 uint8_t TVC64VM::Z80_::readOpcodeFirstByte()
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

  EP128EMU_REGPARM2 uint8_t TVC64VM::Z80_::readOpcodeSecondByte(
      const bool *invalidOpcodeTable)
  {
    uint16_t  addr = (uint16_t(R.PC.W.l) + uint16_t(1)) & uint16_t(0xFFFF);
    if (invalidOpcodeTable) {
      uint8_t b = vm.memory.readNoDebug(addr);
      if (EP128EMU_UNLIKELY(invalidOpcodeTable[b]))
        return b;
    }
    vm.memoryWaitM1();
    uint8_t   retval = vm.memory.readOpcode(addr);
    vm.updateCPUHalfCycles(4);
    return retval;
  }

  EP128EMU_REGPARM2 uint8_t TVC64VM::Z80_::readOpcodeByte(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    vm.memoryWait();
    uint8_t   retval = vm.memory.readOpcode(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM2 uint16_t TVC64VM::Z80_::readOpcodeWord(int offset)
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

  EP128EMU_REGPARM3 void TVC64VM::Z80_::writeMemory(uint16_t addr,
                                                    uint8_t value)
  {
    vm.memoryWait();
    vm.memory.write(addr, value);
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM3 void TVC64VM::Z80_::writeMemoryWord(uint16_t addr,
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

  EP128EMU_REGPARM2 void TVC64VM::Z80_::pushWord(uint16_t value)
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

  EP128EMU_REGPARM3 void TVC64VM::Z80_::doOut(uint16_t addr, uint8_t value)
  {
    vm.ioPortWait();
    vm.ioPorts.write(addr, value);
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM2 uint8_t TVC64VM::Z80_::doIn(uint16_t addr)
  {
    vm.ioPortWait();
    uint8_t   retval = vm.ioPorts.read(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM1 void TVC64VM::Z80_::updateCycle()
  {
    vm.updateCPUHalfCycles(2);
    while (vm.z80OpcodeHalfCycles >= 8)
      vm.runOneCycle();
  }

  EP128EMU_REGPARM2 void TVC64VM::Z80_::updateCycles(int cycles)
  {
    vm.updateCPUCycles(cycles);
    while (vm.z80OpcodeHalfCycles >= 8)
      vm.runOneCycle();
  }

  // --------------------------------------------------------------------------

  TVC64VM::Memory_::Memory_(TVC64VM& vm_)
    : Memory(),
      vm(vm_)
  {
  }

  TVC64VM::Memory_::~Memory_()
  {
  }

  void TVC64VM::Memory_::breakPointCallback(bool isWrite,
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

  TVC64VM::IOPorts_::IOPorts_(TVC64VM& vm_)
    : Ep128::IOPorts(),
      vm(vm_)
  {
  }

  TVC64VM::IOPorts_::~IOPorts_()
  {
  }

  void TVC64VM::IOPorts_::breakPointCallback(bool isWrite,
                                             uint16_t addr, uint8_t value)
  {
    if (!vm.memory.checkIgnoreBreakPoint(vm.z80.getReg().PC.W.l)) {
      vm.breakPointCallback(vm.breakPointCallbackUserData,
                            int(isWrite) + 5, addr, value);
    }
  }

  // --------------------------------------------------------------------------

  TVC64VM::TVCVideo_::TVCVideo_(TVC64VM& vm_,
                                const CPC464::CRTC6845& crtc_,
                                const uint8_t *videoMemory_)
    : TVCVideo(crtc_, videoMemory_),
      vm(vm_)
  {
  }

  TVC64VM::TVCVideo_::~TVCVideo_()
  {
  }

  void TVC64VM::TVCVideo_::drawLine(const uint8_t *buf, size_t nBytes)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.drawLine(buf, nBytes);
    if (vm.videoCapture)
      vm.videoCapture->horizontalSync(buf, nBytes);
  }

  void TVC64VM::TVCVideo_::vsyncStateChange(bool newState,
                                            unsigned int currentSlot_)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.vsyncStateChange(newState, currentSlot_);
    if (vm.videoCapture)
      vm.videoCapture->vsyncStateChange(newState, currentSlot_);
  }

  // --------------------------------------------------------------------------

  void TVC64VM::stopDemoPlayback()
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

  void TVC64VM::stopDemoRecording(bool writeFile_)
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
        demoFile->addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_TVC_DEMO,
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

  uint8_t TVC64VM::ioPortReadCallback(void *userData, uint16_t addr)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
    uint8_t   retval = 0xFF;
    addr = addr & 0x7F;
#if 0
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
    if ((addr & 0x0480) == 0) {         // expansion peripherals (floppy etc.)
      if (!(vm.isRecordingDemo | vm.isPlayingDemo)) {
        switch (addr & 0x0101) {
        case 0x0100:            // main status register
          retval &= vm.floppyDrive->readMainStatusRegister();
          break;
        case 0x0101:            // data register
          retval &= vm.floppyDrive->readDataRegister();
          break;
        }
      }
    }
#endif
    return retval;
  }

  void TVC64VM::ioPortWriteCallback(void *userData,
                                     uint16_t addr, uint8_t value)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
    addr = addr & 0x7F;
#if 0
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
        if (value & 0x10) {
          vm.gateArrayIRQCounter = 0;
          vm.z80.clearInterrupt();
        }
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
    if ((addr & 0x1000) == 0) {         // printer port
    }
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
          vm.ppiPortARegister = 0x00;
          vm.ppiPortBRegister = 0x00;
          vm.ppiPortCRegister = 0x00;
          vm.ppiControlRegister = value;
        }
        break;
      }
      vm.updatePPIState();
    }
    if ((addr & 0x0480) == 0) {         // expansion peripherals (floppy etc.)
      if (!(vm.isRecordingDemo | vm.isPlayingDemo)) {
        switch (addr & 0x0101) {
        case 0x0000:            // floppy drive motor control
        case 0x0001:
          vm.floppyDrive->setMotorState(bool(value & 0x01));
          break;
        case 0x0101:            // data register
          vm.floppyDrive->writeDataRegister(value);
          break;
        }
      }
    }
#endif
  }

  uint8_t TVC64VM::ioPortDebugReadCallback(void *userData, uint16_t addr)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
    addr = addr & 0x7F;
    uint8_t   retval = vm.ioPorts.getLastValueWritten(addr);
#if 0
    if (addr < 0x00A0) {
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
        return vm.tvcKeyboardState[addr & 0x000F];
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
    if ((addr & 0x1000) == 0) {         // printer port
    }
#endif
    return retval;
  }

  EP128EMU_REGPARM2 void TVC64VM::hSyncStateChangeCallback(void *userData,
                                                            bool newState)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
    if (!newState) {
      vm.gateArrayIRQCounter++;
      if (vm.gateArrayVSyncDelay > 0) {
        if (--vm.gateArrayVSyncDelay == 0) {
          if (vm.gateArrayIRQCounter >= 32)
            vm.z80.triggerInterrupt();
          vm.gateArrayIRQCounter = 0;
        }
      }
      if (vm.gateArrayIRQCounter >= 52) {
        vm.gateArrayIRQCounter = 0;
        vm.z80.triggerInterrupt();
      }
    }
    vm.videoRenderer.crtcHSyncStateChange(newState);
  }

  EP128EMU_REGPARM2 void TVC64VM::vSyncStateChangeCallback(void *userData,
                                                            bool newState)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
    if (newState)
      vm.gateArrayVSyncDelay = 2;
    vm.videoRenderer.crtcVSyncStateChange(newState);
  }

  void TVC64VM::tapeCallback(void *userData)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
    vm.tapeSamplesRemaining += vm.tapeSamplesPerCRTCCycle;
    if (vm.tapeSamplesRemaining >= 0) {
      // assume tape sample rate < crtcFrequency
      vm.tapeSamplesRemaining -= (int64_t(1) << 32);
      vm.tapeInputSignal = uint8_t(vm.runTape(vm.tapeOutputSignal));
    }
  }

  void TVC64VM::demoPlayCallback(void *userData)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
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

  void TVC64VM::demoRecordCallback(void *userData)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
    vm.demoTimeCnt++;
  }

  void TVC64VM::videoCaptureCallback(void *userData)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
    vm.videoCapture->runOneCycle(vm.soundOutputSignal);
  }

  uint8_t TVC64VM::checkSingleStepModeBreak()
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

  void TVC64VM::convertKeyboardState()
  {
    for (int i = 0; i < 16; i++)
      tvcKeyboardState[i] = 0xFF;
    const uint8_t *p = &(keyboardConvTable[0]);
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 8; j++) {
        if ((keyboardState[i] & uint8_t(1 << j)) == 0) {
          int     tvcKeyNum = *p;
          if (tvcKeyNum < 80) {
            tvcKeyboardState[tvcKeyNum >> 3] &=
                uint8_t(0xFF ^ (1 << (tvcKeyNum & 7)));
          }
        }
        p++;
      }
    }
  }

  void TVC64VM::resetKeyboard()
  {
    for (int i = 0; i < 16; i++) {
      keyboardState[i] = 0xFF;
      tvcKeyboardState[i] = 0xFF;
    }
  }

  void TVC64VM::setCallback(void (*func)(void *userData), void *userData_,
                            bool isEnabled)
  {
    if (!func)
      return;
    const size_t  maxCallbacks = sizeof(callbacks) / sizeof(TVC64VMCallback);
    int     ndx = -1;
    for (size_t i = 0; i < maxCallbacks; i++) {
      if (callbacks[i].func == func && callbacks[i].userData == userData_) {
        ndx = int(i);
        break;
      }
    }
    if (ndx >= 0) {
      TVC64VMCallback   *prv = (TVC64VMCallback *) 0;
      TVC64VMCallback   *p = firstCallback;
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
        callbacks[ndx].nxt = (TVC64VMCallback *) 0;
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
        throw Ep128Emu::Exception("TVC64VM: too many callbacks");
    }
    callbacks[ndx].func = func;
    callbacks[ndx].userData = userData_;
    callbacks[ndx].nxt = (TVC64VMCallback *) 0;
    if (isEnabled) {
      TVC64VMCallback   *prv = (TVC64VMCallback *) 0;
      TVC64VMCallback   *p = firstCallback;
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

  TVC64VM::TVC64VM(Ep128Emu::VideoDisplay& display_,
                   Ep128Emu::AudioOutput& audioOutput_)
    : VirtualMachine(display_, audioOutput_),
      z80(*this),
      memory(*this),
      ioPorts(*this),
      crtc(),
      videoRenderer(*this, crtc, memory.getVideoMemory()),
      crtcCyclesRemainingL(0U),
      crtcCyclesRemainingH(0),
      z80OpcodeHalfCycles(0),
      tapeInputSignal(0),
      tapeOutputSignal(0),
      crtcRegisterSelected(0x00),
      gateArrayIRQCounter(0),
      gateArrayVSyncDelay(0),
      gateArrayPenSelected(0x00),
      singleStepMode(0),
      singleStepModeNextAddr(int32_t(-1)),
      tapeCallbackFlag(false),
      prvTapeCallbackFlag(false),
      toneGenCnt1(0U),
      toneGenFreq(0U),
      toneGenCnt2(0),
      toneGenEnabled(false),
      audioCycleCnt(2),
      audioOutputLevel(0),
      soundOutputSignal(0U),
      demoFile((Ep128Emu::File *) 0),
      demoBuffer(),
      isRecordingDemo(false),
      isPlayingDemo(false),
      snapshotLoadFlag(false),
      demoTimeCnt(0UL),
      breakPointPriorityThreshold(0),
      firstCallback((TVC64VMCallback *) 0),
      videoCapture((Ep128Emu::VideoCapture *) 0),
      tapeSamplesPerCRTCCycle(0L),
      tapeSamplesRemaining(-1L),
      crtcFrequency(1000000)
  {
    for (size_t i = 0;
         i < (sizeof(callbacks) / sizeof(TVC64VMCallback));
         i++) {
      callbacks[i].func = (void (*)(void *)) 0;
      callbacks[i].userData = (void *) 0;
      callbacks[i].nxt = (TVC64VMCallback *) 0;
    }
    // register I/O callbacks
    ioPorts.setReadCallback(
        0x0000, 0x007F, &ioPortReadCallback, (void *) this, 0x0000);
    ioPorts.setDebugReadCallback(
        0x0000, 0x007F, &ioPortDebugReadCallback, (void *) this, 0x0000);
    ioPorts.setWriteCallback(
        0x0000, 0x007F, &ioPortWriteCallback, (void *) this, 0x0000);
    crtc.setHSyncStateChangeCallback(&hSyncStateChangeCallback, (void *) this);
    crtc.setVSyncStateChangeCallback(&vSyncStateChangeCallback, (void *) this);
    // hack to get some programs using interrupt mode 2 working
    z80.setVectorBase(0xFF);
    // use TVC gate array colormap
    Ep128Emu::VideoDisplay::DisplayParameters
        dp(display.getDisplayParameters());
    dp.indexToRGBFunc = &TVCVideo::convertPixelToRGB;
    display.setDisplayParameters(dp);
    setAudioConverterSampleRate(float(long(crtcFrequency >> 2)));
    // reset
    resetKeyboard();
    resetMemoryConfiguration(64);
  }

  TVC64VM::~TVC64VM()
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

  void TVC64VM::run(size_t microseconds)
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
      if (newTapeCallbackFlag == prvTapeCallbackFlag) {
        tapeCallbackFlag = newTapeCallbackFlag;
        tapeInputSignal = 0;
        setTapeMotorState(newTapeCallbackFlag);
        setCallback(&tapeCallback, this, newTapeCallbackFlag);
      }
      prvTapeCallbackFlag = newTapeCallbackFlag;
    }
    z80OpcodeHalfCycles = z80OpcodeHalfCycles & 0xFE;
    int64_t crtcCyclesRemaining =
        int64_t(crtcCyclesRemainingL) + (int64_t(crtcCyclesRemainingH) << 32)
        + ((int64_t(microseconds) << 26) * int64_t(crtcFrequency)
           / int64_t(15625));   // 10^6 / 2^6
    crtcCyclesRemainingL =
        uint32_t(uint64_t(crtcCyclesRemaining) & 0xFFFFFFFFUL);
    crtcCyclesRemainingH = int32_t(crtcCyclesRemaining >> 32);
    while (EP128EMU_EXPECT(crtcCyclesRemainingH > 0)) {
      z80.executeInstruction();
      while (EP128EMU_UNLIKELY(z80OpcodeHalfCycles >= 8))
        runOneCycle();
    }
  }

  void TVC64VM::reset(bool isColdReset)
  {
    stopDemoPlayback();         // TODO: should be recorded as an event ?
    stopDemoRecording(false);
    z80.reset();
    memory.setPaging(0x00C0);
    crtc.reset();
    videoRenderer.reset();
    if (isColdReset)
      resetKeyboard();
    crtcRegisterSelected = 0x00;
    gateArrayIRQCounter = 0;
    gateArrayVSyncDelay = 0;
    gateArrayPenSelected = 0x00;
  }

  void TVC64VM::resetMemoryConfiguration(size_t memSize)
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

  void TVC64VM::loadROMSegment(uint8_t n, const char *fileName, size_t offs)
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

  void TVC64VM::setVideoFrequency(size_t freq_)
  {
    size_t  freq;
    // allow refresh rates in the range 10 Hz to 100 Hz
    freq = ((freq_ > 400000 ? (freq_ < 2000000 ? freq_ : 2000000) : 400000) + 2)
           & (~(size_t(3)));
    if (freq == crtcFrequency)
      return;
    crtcFrequency = freq;
    stopDemoPlayback();         // changing configuration implies stopping
    stopDemoRecording(false);   // any demo playback or recording
    setAudioConverterSampleRate(float(long(crtcFrequency >> 2)));
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

  void TVC64VM::setKeyboardState(int keyCode, bool isPressed)
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

  void TVC64VM::getVMStatus(VMStatus& vmStatus_)
  {
    vmStatus_.tapeReadOnly = getIsTapeReadOnly();
    vmStatus_.tapePosition = getTapePosition();
    vmStatus_.tapeLength = getTapeLength();
    vmStatus_.tapeSampleRate = getTapeSampleRate();
    vmStatus_.tapeSampleSize = getTapeSampleSize();
    vmStatus_.floppyDriveLEDState = 0U;
    vmStatus_.isPlayingDemo = isPlayingDemo;
    if (demoFile != (Ep128Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    vmStatus_.isRecordingDemo = isRecordingDemo;
  }

  void TVC64VM::openVideoCapture(
      int frameRate_,
      bool yuvFormat_,
      void (*errorCallback_)(void *userData, const char *msg),
      void (*fileNameCallback_)(void *userData, std::string& fileName),
      void *userData_)
  {
    if (!videoCapture) {
      if (yuvFormat_) {
        videoCapture = new Ep128Emu::VideoCapture_YV12(
                               &TVCVideo::convertPixelToRGB, frameRate_);
      }
      else {
        videoCapture = new Ep128Emu::VideoCapture_RLE8(
                               &TVCVideo::convertPixelToRGB, frameRate_);
      }
      videoCapture->setClockFrequency(crtcFrequency);
      setCallback(&videoCaptureCallback, this, true);
    }
    videoCapture->setErrorCallback(errorCallback_, userData_);
    videoCapture->setFileNameCallback(fileNameCallback_, userData_);
  }

  void TVC64VM::setVideoCaptureFile(const std::string& fileName_)
  {
    if (!videoCapture) {
      throw Ep128Emu::Exception("internal error: "
                                "video capture object does not exist");
    }
    videoCapture->openFile(fileName_.c_str());
  }

  void TVC64VM::closeVideoCapture()
  {
    if (videoCapture) {
      setCallback(&videoCaptureCallback, this, false);
      delete videoCapture;
      videoCapture = (Ep128Emu::VideoCapture *) 0;
    }
  }

  void TVC64VM::setTapeFileName(const std::string& fileName)
  {
    Ep128Emu::VirtualMachine::setTapeFileName(fileName);
    if (haveTape()) {
      tapeSamplesPerCRTCCycle =
          (int64_t(getTapeSampleRate()) << 32) / int64_t(crtcFrequency);
    }
    tapeSamplesRemaining = -1L;
  }

  void TVC64VM::tapePlay()
  {
    Ep128Emu::VirtualMachine::tapePlay();
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void TVC64VM::tapeRecord()
  {
    Ep128Emu::VirtualMachine::tapeRecord();
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void TVC64VM::tapeStop()
  {
    Ep128Emu::VirtualMachine::tapeStop();
  }

  void TVC64VM::tapeSeek(double t)
  {
    Ep128Emu::VirtualMachine::tapeSeek(t);
  }

  void TVC64VM::setBreakPoint(const Ep128Emu::BreakPoint& bp, bool isEnabled)
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

  void TVC64VM::clearBreakPoints()
  {
    memory.clearAllBreakPoints();
    ioPorts.clearBreakPoints();
  }

  void TVC64VM::setBreakPointPriorityThreshold(int n)
  {
    breakPointPriorityThreshold = uint8_t(n > 0 ? (n < 4 ? n : 4) : 0);
    if (!(singleStepMode == 1 || singleStepMode == 2)) {
      memory.setBreakPointPriorityThreshold(n);
      ioPorts.setBreakPointPriorityThreshold(n);
    }
  }

  void TVC64VM::setSingleStepMode(int mode_)
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

  void TVC64VM::setSingleStepModeNextAddress(int32_t addr)
  {
    if ((singleStepMode != 2 && singleStepMode != 4) || addr < 0)
      addr = int32_t(-1);
    else
      addr &= int32_t(0xFFFF);
    singleStepModeNextAddr = addr;
  }

  uint8_t TVC64VM::getMemoryPage(int n) const
  {
    return memory.getPage(uint8_t(n & 3));
  }

  uint8_t TVC64VM::readMemory(uint32_t addr, bool isCPUAddress) const
  {
    if (isCPUAddress)
      addr = ((uint32_t(memory.getPage(uint8_t(addr >> 14) & uint8_t(3))) << 14)
              | (addr & uint32_t(0x3FFF)));
    else
      addr &= uint32_t(0x003FFFFF);
    return memory.readRaw(addr);
  }

  void TVC64VM::writeMemory(uint32_t addr, uint8_t value, bool isCPUAddress)
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

  void TVC64VM::writeROM(uint32_t addr, uint8_t value)
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    memory.writeROM(addr & uint32_t(0x003FFFFF), value);
  }

  uint8_t TVC64VM::readIOPort(uint16_t addr) const
  {
    return ioPorts.readDebug(addr);
  }

  void TVC64VM::writeIOPort(uint16_t addr, uint8_t value)
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    ioPorts.writeDebug(addr, value);
  }

  uint16_t TVC64VM::getProgramCounter() const
  {
    return z80.getProgramCounter();
  }

  void TVC64VM::setProgramCounter(uint16_t addr)
  {
    if (addr != z80.getProgramCounter()) {
      if (isRecordingDemo | isPlayingDemo) {
        stopDemoPlayback();
        stopDemoRecording(false);
      }
      z80.setProgramCounter(addr);
    }
  }

  uint16_t TVC64VM::getStackPointer() const
  {
    return uint16_t(z80.getReg().SP.W);
  }

  void TVC64VM::listCPURegisters(std::string& buf) const
  {
    listZ80Registers(buf, z80);
  }

  void TVC64VM::listIORegisters(std::string& buf) const
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
    if (!(crtcRegisterSelected & 0x10)) {
      bufp[((int(crtcRegisterSelected) & 15) * 3)
           + ((int(crtcRegisterSelected) & 12) >> 2)
           + ((crtcRegisterSelected & 8) == 0 ? 9 : 18)] = '>';
    }
    bufp = bufp + n;
    {
      int     xPos = 0;
      int     yPos = 0;
      getVideoPosition(xPos, yPos);
      n = std::sprintf(bufp,
                       "Mem: %04X  VidXY:%3d,%3d  CRTC MA: %04X\n",
                       (unsigned int) (memory.getPaging() ^ 0x00C0),
                       xPos, yPos, (unsigned int) crtc.getMemoryAddress());
    }
    bufp = bufp + n;
    buf = &(tmpBuf2[0]);
  }

  uint32_t TVC64VM::disassembleInstruction(std::string& buf,
                                            uint32_t addr, bool isCPUAddress,
                                            int32_t offs) const
  {
    return Ep128::Z80Disassembler::disassembleInstruction(
               buf, (*this), addr, isCPUAddress, offs);
  }

  void TVC64VM::getVideoPosition(int& xPos, int& yPos) const
  {
    crtc.getVideoPosition(xPos, yPos);
  }

  Ep128::Z80_REGISTERS& TVC64VM::getZ80Registers()
  {
    if (isRecordingDemo | isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    return z80.getReg();
  }

}       // namespace TVC64

