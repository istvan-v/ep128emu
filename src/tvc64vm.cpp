
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
#ifdef ENABLE_SDEXT
#  include "sdext.hpp"
#endif

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

static const uint16_t audioOutputLevelTable[16] = {
      0,  1948,  4095,  6043,  7986,  9933, 12081, 14029,
  15971, 17919, 20067, 22014, 23957, 25905, 28052, 30000
};

namespace TVC64 {

  EP128EMU_INLINE void TVC64VM::updateCPUCycles(int cycles)
  {
    z80HalfCycleCnt = uint8_t((z80HalfCycleCnt + (cycles << 1)) & 0xFF);
  }

  EP128EMU_INLINE void TVC64VM::updateCPUHalfCycles(int halfCycles)
  {
    z80HalfCycleCnt = uint8_t((z80HalfCycleCnt + halfCycles) & 0xFF);
  }

  EP128EMU_INLINE void TVC64VM::memoryWait(uint16_t addr)
  {
    if (memory.getPage(addr >> 14) >= 0xFC) {
      updateCPUHalfCycles(4);
      updateCPUHalfCycles(4 - int(z80HalfCycleCnt & 3));
      updateCPUHalfCycles(1);
      runDevices();
      return;
    }
    updateCPUHalfCycles(5);
  }

  EP128EMU_INLINE void TVC64VM::memoryWaitM1(uint16_t addr)
  {
    if (memory.getPage(addr >> 14) >= 0xFC) {
      updateCPUHalfCycles(3);
      updateCPUHalfCycles(4 - int(z80HalfCycleCnt & 3));
      updateCPUHalfCycles(1);
      runDevices();
      return;
    }
    updateCPUHalfCycles(4);
  }

  EP128EMU_INLINE void TVC64VM::ioPortWait(uint16_t addr)
  {
    updateCPUHalfCycles(6);
    // FIXME: is clock stretching needed when accessing video related I/O ports?
    (void) addr;
    updateCPUHalfCycles(1);
    runDevices();
  }

  EP128EMU_INLINE void TVC64VM::updateSndIntState(bool cursorState)
  {
    uint8_t sndIntState = ((toneGenCnt2 << 1) & (irqEnableMask & 0x10))
                          | uint8_t(cursorState);
    if (EP128EMU_UNLIKELY(bool(sndIntState) != prvSndIntState)) {
      irqState = irqState | (uint8_t(prvSndIntState) << 4);
      prvSndIntState = bool(sndIntState);
    }
  }

  EP128EMU_REGPARM1 void TVC64VM::runDevices()
  {
    uint8_t m = machineHalfCycleCnt;
    uint8_t n = (z80HalfCycleCnt - m) & 0xFE;
    if (EP128EMU_UNLIKELY(!n))
      return;
    machineHalfCycleCnt += n;
    n = n >> 1;
    m = m >> 1;
    bool    cursorState = crtc.getCursorEnabled();
    updateSndIntState(cursorState);
    toneGenCnt1 = toneGenCnt1 - uint32_t(m & 1);
    n = n + (m & 1);
    uint8_t b = n & 1;
    n = n >> 1;
    m = m >> 1;
    crtcCyclesRemainingH -= int32_t(n);
    while (n--) {
      toneGenCnt1 = toneGenCnt1 + 2U;
      while (EP128EMU_UNLIKELY(toneGenCnt1 >= 4096U)) {
        toneGenCnt1 = toneGenFreq;
        toneGenCnt2 = (toneGenCnt2 + 1) & 0x0F;
        if (EP128EMU_UNLIKELY(!(toneGenCnt2 & 7))) {
          if ((irqEnableMask & 0x10) != 0 && (n + b) > 0)
            updateSndIntState(cursorState);
        }
      }
      TVC64VMCallback *p = firstCallback;
      while (EP128EMU_UNLIKELY(bool(p))) {
        TVC64VMCallback *nxt = p->nxt;
        p->func(p->userData);
        p = nxt;
      }
      m++;
      if (EP128EMU_UNLIKELY(!(m & 3))) {
        uint32_t  tmp = uint32_t(tapeInputSignal + tapeOutputSignal) << 12;
        if (((toneGenCnt2 | 0xF7) + uint8_t(toneGenEnabled)) & 0xFF)
          tmp += uint32_t(audioOutputLevelTable[audioOutputLevel]);
        soundOutputSignal = tmp | (tmp << 16);
        sendAudioOutput(soundOutputSignal);
      }
      videoRenderer.runOneCycle();
      crtc.runOneCycle();
      if (EP128EMU_UNLIKELY(crtc.getCursorEnabled() != cursorState)) {
        if (n + b) {
          cursorState = !cursorState;
          updateSndIntState(cursorState);
        }
      }
    }
    if (b) {
      toneGenCnt1++;
      if (EP128EMU_UNLIKELY(toneGenCnt1 >= 4096U)) {
        toneGenCnt1 = toneGenFreq;
        toneGenCnt2 = (toneGenCnt2 + 1) & 0x0F;
      }
    }
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
    vm.runDevices();
    if (EP128EMU_UNLIKELY(bool(vm.irqState)))
      Ep128::Z80::executeInterrupt();
  }

  EP128EMU_REGPARM2 uint8_t TVC64VM::Z80_::readMemory(uint16_t addr)
  {
    vm.memoryWait(addr);
    uint8_t   retval = vm.memory.read(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM2 uint16_t TVC64VM::Z80_::readMemoryWord(uint16_t addr)
  {
    vm.memoryWait(addr);
    uint16_t  retval = vm.memory.read(addr);
    vm.updateCPUHalfCycles(1);
    addr = (addr + 1) & 0xFFFF;
    vm.memoryWait(addr);
    retval |= (uint16_t(vm.memory.read(addr) << 8));
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM1 uint8_t TVC64VM::Z80_::readOpcodeFirstByte()
  {
    uint16_t  addr = uint16_t(R.PC.W.l);
    vm.memoryWaitM1(addr);
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
    vm.memoryWaitM1(addr);
    uint8_t   retval = vm.memory.readOpcode(addr);
    vm.updateCPUHalfCycles(4);
    return retval;
  }

  EP128EMU_REGPARM2 uint8_t TVC64VM::Z80_::readOpcodeByte(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    vm.memoryWait(addr);
    uint8_t   retval = vm.memory.readOpcode(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM2 uint16_t TVC64VM::Z80_::readOpcodeWord(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    vm.memoryWait(addr);
    uint16_t  retval = vm.memory.readOpcode(addr);
    vm.updateCPUHalfCycles(1);
    addr = (addr + 1) & 0xFFFF;
    vm.memoryWait(addr);
    retval |= (uint16_t(vm.memory.readOpcode(addr) << 8));
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM3 void TVC64VM::Z80_::writeMemory(uint16_t addr,
                                                    uint8_t value)
  {
    vm.memoryWait(addr);
    vm.memory.write(addr, value);
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM3 void TVC64VM::Z80_::writeMemoryWord(uint16_t addr,
                                                        uint16_t value)
  {
    vm.memoryWait(addr);
    vm.memory.write(addr, uint8_t(value) & 0xFF);
    vm.updateCPUHalfCycles(1);
    addr = (addr + 1) & 0xFFFF;
    vm.memoryWait(addr);
    vm.memory.write(addr, uint8_t(value >> 8));
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM2 void TVC64VM::Z80_::pushWord(uint16_t value)
  {
    vm.updateCPUHalfCycles(2);
    R.SP.W -= 2;
    uint16_t  addr = (R.SP.W + 1) & 0xFFFF;
    vm.memoryWait(addr);
    vm.memory.write(addr, uint8_t(value >> 8));
    vm.updateCPUHalfCycles(1);
    addr = (addr - 1) & 0xFFFF;
    vm.memoryWait(addr);
    vm.memory.write(addr, uint8_t(value) & 0xFF);
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM3 void TVC64VM::Z80_::doOut(uint16_t addr, uint8_t value)
  {
    vm.ioPortWait(addr);
    vm.ioPorts.write(addr, value);
    vm.updateCPUHalfCycles(1);
  }

  EP128EMU_REGPARM2 uint8_t TVC64VM::Z80_::doIn(uint16_t addr)
  {
    vm.ioPortWait(addr);
    uint8_t   retval = vm.ioPorts.read(addr);
    vm.updateCPUHalfCycles(1);
    return retval;
  }

  EP128EMU_REGPARM1 void TVC64VM::Z80_::updateCycle()
  {
    vm.updateCPUHalfCycles(2);
  }

  EP128EMU_REGPARM2 void TVC64VM::Z80_::updateCycles(int cycles)
  {
    vm.updateCPUCycles(cycles);
  }

  // --------------------------------------------------------------------------

  TVC64VM::Memory_::Memory_(TVC64VM& vm_)
    : Memory(),
      vm(vm_)
  {
    extensionRAM.resize(0x1000, 0xFF);  // VT-DOS RAM
  }

  TVC64VM::Memory_::~Memory_()
  {
  }

  void TVC64VM::Memory_::setEnableSDExt()
  {
    segment1IsExtension = vm.sdext.isSDExtSegment(0x07);
    setPaging(getPaging());
  }

  void TVC64VM::Memory_::breakPointCallback(bool isWrite,
                                            uint16_t addr, uint8_t value)
  {
    vm.runDevices();
    if (!vm.memory.checkIgnoreBreakPoint(vm.z80.getReg().PC.W.l)) {
      int     bpType = int(isWrite) + 1;
      if (!isWrite && uint16_t(vm.z80.getReg().PC.W.l) == addr)
        bpType = 0;
      vm.breakPointCallback(vm.breakPointCallbackUserData, bpType, addr, value);
    }
  }

  EP128EMU_REGPARM2 uint8_t TVC64VM::Memory_::extensionRead(uint16_t addr)
  {
    if (getPage(uint8_t(addr >> 14)) == 0x02) { // EXT
      if (EP128EMU_UNLIKELY(getPaging() & 0xC000))
        return 0xFF;                    // IOMEM 0 is not paged in
      if (addr & 0x1000)
        return extensionRAM[addr & 0x0FFF];
      return readRaw(0x0000C000U | (uint32_t(vm.vtdosROMPage) << 12)
                     | uint32_t(addr & 0x0FFF));
    }
    vm.runDevices();
    return vm.sdext.readCartP3(addr);
  }

  EP128EMU_REGPARM2 uint8_t TVC64VM::Memory_::extensionReadNoDebug(
                                                  uint16_t addr) const
  {
    if (getPage(uint8_t(addr >> 14)) == 0x02) { // EXT
      if (EP128EMU_UNLIKELY(getPaging() & 0xC000))
        return 0xFF;                    // IOMEM 0 is not paged in
      if (addr & 0x1000)
        return extensionRAM[addr & 0x0FFF];
      return readRaw(0x0000C000U | (uint32_t(vm.vtdosROMPage) << 12)
                     | uint32_t(addr & 0x0FFF));
    }
    return vm.sdext.readCartP3Debug(addr);
  }

  EP128EMU_REGPARM3 void TVC64VM::Memory_::extensionWrite(uint16_t addr,
                                                          uint8_t value)
  {
    if (getPage(uint8_t(addr >> 14)) == 0x02) { // EXT
      if (EP128EMU_UNLIKELY(getPaging() & 0xC000))
        return;                         // IOMEM 0 is not paged in
      if (addr & 0x1000)
        extensionRAM[addr & 0x0FFF] = value;
    }
    vm.runDevices();
    vm.sdext.writeCartP3(addr, value);
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
    switch (addr) {
    // 0x00-0x0F: unused (write only)
    case 0x10:                          // extension 0: floppy drive controller
    case 0x11:
    case 0x12:
    case 0x13:
      // floppy emulation is disabled while recording or playing demo
      if (EP128EMU_UNLIKELY(vm.isRecordingDemo | vm.isPlayingDemo)) {
        retval = 0x00;
      }
      else {
        switch (addr & 3) {
        case 0:
          retval = vm.wd177x.readStatusRegister();
          break;
        case 1:
          retval = vm.wd177x.readTrackRegister();
          break;
        case 2:
          retval = vm.wd177x.readSectorRegister();
          break;
        case 3:
          retval = vm.wd177x.readDataRegister();
          break;
        }
      }
      break;
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
      // floppy emulation is disabled while recording or playing demo
      if (vm.isRecordingDemo | vm.isPlayingDemo) {
        retval = 0x7D;
      }
      else {
        retval = uint8_t(vm.wd177x.getInterruptRequestFlag())
                 | (vm.wd177x.getFloppyDrive().getDiskChangeFlag() ?
                    0x00 : 0x40)
                 | (vm.wd177x.getDataRequestFlag() ? 0x80 : 0x00);
      }
      break;
    // 0x20-0x2F: extension 1 (unimplemented)
    // 0x30-0x3F: extension 2 (unimplemented)
    // 0x40-0x4F: extension 3 (unimplemented)
    case 0x50:                          // toggle tape output (repeated 8 times)
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
      vm.tapeOutputSignal = ~(vm.tapeOutputSignal) & 0x01;
      break;
    case 0x58:                          // keyboard matrix (repeated twice)
    case 0x5C:
      retval = vm.tvcKeyboardState[vm.keyboardRow];
      break;
    case 0x59:                          // IRQ state (repeated twice)
    case 0x5D:
      // bit 7: printer ready (unimplemented)
      // bit 6: color output enabled (1 = yes)
      retval = (~vm.irqState & 0x1F) | ((vm.tapeInputSignal & 1) << 5) | 0xC0;
      break;
    case 0x5A:                          // extension card ID byte
    case 0x5E:
      // extension 0 is a floppy drive, the others are unused
      retval = 0xFE | uint8_t(vm.memory.readRaw(0xC000U) == 0xFF);
      break;
    case 0x5B:                          // reset tone generator (repeated twice)
    case 0x5F:
      vm.toneGenCnt1 = vm.toneGenFreq;
      vm.toneGenCnt2 = 0;
      break;
    case 0x70:                          // CRTC registers (repeated 8 times)
    case 0x72:
    case 0x74:
    case 0x76:
    case 0x78:
    case 0x7A:
    case 0x7C:
    case 0x7E:
      retval = vm.ioPorts.getLastValueWritten(0x70);
      break;
    case 0x71:
    case 0x73:
    case 0x75:
    case 0x77:
    case 0x79:
    case 0x7B:
    case 0x7D:
    case 0x7F:
      retval = vm.crtc.readRegister(vm.crtcRegisterSelected);
      break;
    }
    return retval;
  }

  void TVC64VM::ioPortWriteCallback(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
    addr = addr & 0x7F;
    switch (addr) {
    case 0x00:                          // border color
      vm.videoRenderer.setColor(4, value);
      break;
    case 0x01:                          // printer data output (unimplemented)
      break;
    case 0x02:                          // main memory paging register
      vm.memory.setPaging((vm.memory.getPaging() & 0xFF00)
                          | uint16_t(value & 0xFC));
      break;
    case 0x03:                          // extension (IOMEM) paging register
      vm.memory.setPaging((vm.memory.getPaging() & 0x3FFF)
                          | (uint16_t(value & 0xC0) << 8));
      vm.keyboardRow = value & 0x0F;
      break;
    case 0x04:                          // tone generator frequency LSB
      vm.toneGenFreq = (vm.toneGenFreq & 0x0F00U) | uint32_t(value);
      break;
    case 0x05:                          // tone generator frequency b8..b11
      vm.toneGenFreq = (vm.toneGenFreq & 0xFFU) | (uint32_t(value & 0x0F) << 8);
      vm.toneGenEnabled = bool(value & 0x10);
      vm.irqEnableMask = (vm.irqEnableMask & 0x0F) | ((value & 0x20) >> 1);
      vm.setTapeMotorState(bool(value & 0xC0));
      break;
    case 0x06:                          // video mode, audio output level
      // bit 7 (0 = printer output is valid) is ignored
      vm.videoRenderer.setVideoMode(value & 0x03);
      vm.audioOutputLevel = (value & 0x3C) >> 2;
      break;
    case 0x07:                          // clear tone generator / cursor IRQ
      vm.irqState = vm.irqState & 0x0F;
      break;
    case 0x0C:                          // video RAM paging register (TVC64+)
      vm.memory.setPaging((vm.memory.getPaging() & 0xC0FF)
                          | (uint16_t(value & 0x3F) << 8));
      vm.videoRenderer.setVideoMemory(vm.memory.getVideoMemory());
      break;
    case 0x0D:
    case 0x0E:
    case 0x0F:
      vm.ioPorts.writeDebug(addr & 0x7C, value);
      break;
    case 0x10:                          // extension 0: floppy drive controller
    case 0x11:
    case 0x12:
    case 0x13:
      // floppy emulation is disabled while recording or playing demo
      if (!(vm.isRecordingDemo | vm.isPlayingDemo)) {
        switch (addr & 3) {
        case 0:
          if (!(value & 0x80))
            vm.wd177x.getFloppyDrive().setDiskChangeFlag(false);
          vm.wd177x.writeCommandRegister(value);
          break;
        case 1:
          vm.wd177x.writeTrackRegister(value);
          break;
        case 2:
          vm.wd177x.writeSectorRegister(value);
          break;
        case 3:
          vm.wd177x.writeDataRegister(value);
          break;
        }
      }
      break;
    case 0x14:
      // floppy emulation is disabled while recording or playing demo
      if (!(vm.isRecordingDemo | vm.isPlayingDemo)) {
        vm.wd177x.getFloppyDrive().motorOff();
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
          vm.wd177x.getFloppyDrive().motorOn();
        else
          vm.wd177x.getFloppyDrive().motorOff();
        vm.wd177x.getFloppyDrive().setSide((value >> 7) & 1);
      }
      break;
    case 0x15:
    case 0x16:
    case 0x17:
      vm.ioPorts.writeDebug(addr & 0x7C, value);
      break;
    case 0x18:
      vm.vtdosROMPage = (value >> 4) & 0x03;
      break;
    case 0x19:
    case 0x1A:
    case 0x1B:
      vm.ioPorts.writeDebug(addr & 0x7C, value);
      break;
    // 0x20-0x2F: extension 1 (unimplemented)
    // 0x30-0x3F: extension 2 (unimplemented)
    // 0x40-0x4F: extension 3 (unimplemented)
    case 0x50:                          // toggle tape output (repeated 8 times)
      vm.tapeOutputSignal = ~(vm.tapeOutputSignal) & 0x01;
      break;
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
      vm.ioPorts.writeDebug(addr & 0x78, value);
      break;
    case 0x58:                          // extension 0 IRQ (repeated twice)
    case 0x59:                          // extension 1 IRQ
    case 0x5A:                          // extension 2 IRQ
    case 0x5B:                          // extension 3 IRQ
      {
        uint8_t n = uint8_t(addr & 3);
        uint8_t m = ~(uint8_t(1 << n));
        uint8_t b = (value & 0x80) >> (7 - n);
        vm.irqState = vm.irqState & (m | b);
        vm.irqEnableMask = (vm.irqEnableMask & m) | b;
      }
      break;
    case 0x5C:
    case 0x5D:
    case 0x5E:
    case 0x5F:
      vm.ioPorts.writeDebug(addr & 0x7B, value);
      break;
    case 0x60:                          // palette registers (repeated 4 times)
    case 0x61:
    case 0x62:
    case 0x63:
      vm.videoRenderer.setColor(addr & 3, value);
      break;
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x68:
    case 0x69:
    case 0x6A:
    case 0x6B:
    case 0x6C:
    case 0x6D:
    case 0x6E:
    case 0x6F:
      vm.ioPorts.writeDebug(addr & 0x73, value);
      break;
    case 0x70:                          // CRTC registers (repeated 8 times)
      vm.crtcRegisterSelected = value & 0x1F;
      break;
    case 0x71:
      vm.crtc.writeRegister(vm.crtcRegisterSelected, value);
      break;
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
    case 0x78:
    case 0x79:
    case 0x7A:
    case 0x7B:
    case 0x7C:
    case 0x7D:
    case 0x7E:
    case 0x7F:
      vm.ioPorts.writeDebug(addr & 0x71, value);
      break;
    }
  }

  uint8_t TVC64VM::ioPortDebugReadCallback(void *userData, uint16_t addr)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
    addr = addr & 0x7F;
    if (addr >= 0x70)
      addr = addr & 0x71;
    else if (addr >= 0x60)
      addr = addr & 0x73;
    else if (addr >= 0x58)
      addr = addr & 0x7B;
    else if (addr >= 0x50)
      addr = addr & 0x78;
    else if ((addr >= 0x0C && addr < 0x10) || (addr >= 0x14 && addr < 0x1C))
      addr = addr & 0x7C;
    uint8_t   retval = vm.ioPorts.getLastValueWritten(addr);
    switch (addr) {
    case 0x10:                          // extension 0: floppy drive controller
    case 0x11:
    case 0x12:
    case 0x13:
      switch (addr & 3) {
      case 0:
        retval = vm.wd177x.readStatusRegisterDebug();
        break;
      case 1:
        retval = vm.wd177x.readTrackRegister();
        break;
      case 2:
        retval = vm.wd177x.readSectorRegister();
        break;
      case 3:
        retval = vm.wd177x.readDataRegisterDebug();
        break;
      }
      break;
    case 0x14:
      retval = uint8_t(vm.wd177x.getInterruptRequestFlag())
               | (vm.wd177x.getFloppyDrive().getDiskChangeFlag() ? 0x00 : 0x40)
               | (vm.wd177x.getDataRequestFlag() ? 0x80 : 0x00);
      break;
    case 0x58:                          // keyboard matrix
      retval = vm.tvcKeyboardState[vm.keyboardRow];
      break;
    case 0x59:                          // IRQ state (repeated twice)
      // bit 7: printer ready (unimplemented)
      // bit 6: color output enabled (1 = yes)
      retval = (~vm.irqState & 0x1F) | ((vm.tapeInputSignal & 1) << 5) | 0xC0;
      break;
    case 0x5A:                          // extension card ID byte
      // extension 0 is a floppy drive, the others are unused
      retval = 0xFE | uint8_t(vm.memory.readRaw(0xC000U) == 0xFF);
      break;
    case 0x71:                          // CRTC registers
      retval = vm.crtc.readRegister(vm.crtcRegisterSelected);
      break;
    }
    return retval;
  }

  EP128EMU_REGPARM2 void TVC64VM::hSyncStateChangeCallback(void *userData,
                                                           bool newState)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
    vm.videoRenderer.crtcHSyncStateChange(newState);
  }

  EP128EMU_REGPARM2 void TVC64VM::vSyncStateChangeCallback(void *userData,
                                                           bool newState)
  {
    TVC64VM&  vm = *(reinterpret_cast<TVC64VM *>(userData));
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
    runDevices();
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

  void TVC64VM::resetFloppyDrives(bool isColdReset)
  {
    wd177x.setFloppyDrive((Ep128Emu::FloppyDrive *) 0);
    wd177x.reset(isColdReset);
    for (int i = 0; i < 4; i++) {
      floppyDrives[i].reset();
      floppyDrives[i].motorOff();
      if (isColdReset)
        floppyDrives[i].setDiskChangeFlag(true);
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
      z80HalfCycleCnt(0),
      machineHalfCycleCnt(0),
      tapeInputSignal(0),
      tapeOutputSignal(0),
      crtcRegisterSelected(0x00),
      irqState(0x00),
      irqEnableMask(0x00),
      singleStepMode(0),
      singleStepModeNextAddr(int32_t(-1)),
      tapeCallbackFlag(false),
      prvTapeCallbackFlag(false),
      keyboardRow(0),
      prvSndIntState(false),
      toneGenCnt1(0U),
      toneGenFreq(0U),
      toneGenCnt2(0),
      toneGenEnabled(false),
      audioOutputLevel(0),
      soundOutputSignal(0U),
      demoFile((Ep128Emu::File *) 0),
      demoBuffer(),
      isRecordingDemo(false),
      isPlayingDemo(false),
      snapshotLoadFlag(false),
      demoTimeCnt(0UL),
      vtdosROMPage(0),
      breakPointPriorityThreshold(0),
      firstCallback((TVC64VMCallback *) 0),
      videoCapture((Ep128Emu::VideoCapture *) 0),
      tapeSamplesPerCRTCCycle(0L),
      tapeSamplesRemaining(-1L),
      crtcFrequency(1562500)
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
    wd177x.setIsWD1773(true);
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
    resetMemoryConfiguration(80);
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
        setCallback(&tapeCallback, this, newTapeCallbackFlag);
      }
      prvTapeCallbackFlag = newTapeCallbackFlag;
    }
    machineHalfCycleCnt = machineHalfCycleCnt & 0xFE;
    int64_t crtcCyclesRemaining =
        int64_t(crtcCyclesRemainingL) + (int64_t(crtcCyclesRemainingH) << 32)
        + ((int64_t(microseconds) << 26) * int64_t(crtcFrequency)
           / int64_t(15625));   // 10^6 / 2^6
    crtcCyclesRemainingL =
        uint32_t(uint64_t(crtcCyclesRemaining) & 0xFFFFFFFFUL);
    crtcCyclesRemainingH = int32_t(crtcCyclesRemaining >> 32);
    z80.triggerInterrupt();
    while (EP128EMU_EXPECT(crtcCyclesRemainingH > 0)) {
      z80.executeInstruction();
      if ((z80HalfCycleCnt - machineHalfCycleCnt) & 0xFE)
        runDevices();
    }
  }

  void TVC64VM::reset(bool isColdReset)
  {
    stopDemoPlayback();         // TODO: should be recorded as an event ?
    stopDemoRecording(false);
    z80.reset();
    memory.setEnableSDExt();
    videoRenderer.setVideoMemory(memory.getVideoMemory());
    crtc.reset();
    videoRenderer.reset();
    tapeOutputSignal = 0;
    ioPorts.writeDebug(0x02, 0x00);
    ioPorts.writeDebug(0x03, 0x00);
    ioPorts.writeDebug(0x05, 0x00);
    ioPorts.writeDebug(0x06, 0x00);
    ioPorts.writeDebug(0x0C, 0x00);
    ioPorts.writeDebug(0x70, 0x00);
    prvSndIntState = false;
    irqState = 0x00;
    irqEnableMask = 0x00;
    vtdosROMPage = 0;
    resetFloppyDrives(isColdReset);
    if (isColdReset) {
      ioPorts.writeDebug(0x04, 0x00);
      toneGenCnt1 = 0U;
      toneGenCnt2 = 0;
      resetKeyboard();
      memory.clearRAM();
    }
#ifdef ENABLE_SDEXT
    sdext.reset(int(isColdReset));
#endif
  }

  void TVC64VM::resetMemoryConfiguration(size_t memSize)
  {
    stopDemo();
    // delete all ROM segments
    for (uint8_t i = 0x00; i < 0xF8; i++) {
      if (memory.isSegmentROM(i))
        memory.deleteSegment(i);
    }
    // resize RAM
    memory.setRAMSize(memSize);
    videoRenderer.setVideoMemory(memory.getVideoMemory());
    // cold reset
    this->reset(true);
  }

  void TVC64VM::loadROMSegment(uint8_t n, const char *fileName, size_t offs)
  {
    stopDemo();
    if (n > 0x03)
      throw Ep128Emu::Exception("internal error: invalid ROM segment number");
    if (fileName == (char *) 0 || fileName[0] == '\0') {
      // empty file name: delete segment
      memory.deleteSegment(n);
      return;
    }
    // load file into memory
    std::vector<uint8_t>  buf;
    buf.resize(0x4000, 0xFF);
    std::FILE *f = std::fopen(fileName, "rb");
    if (!f)
      throw Ep128Emu::Exception("cannot open ROM file");
    std::fseek(f, 0L, SEEK_END);
    long    dataSize = std::ftell(f) - long(offs);
    if (dataSize < 0x0400L) {
      std::fclose(f);
      throw Ep128Emu::Exception("ROM file is shorter than expected");
    }
    if (n == 0x02)
      dataSize = (dataSize < 0x2000L ? dataSize : 0x2000L);
    else
      dataSize = (dataSize < 0x4000L ? dataSize : 0x4000L);
    std::fseek(f, long(offs), SEEK_SET);
    std::fread(&(buf.front()), sizeof(uint8_t), size_t(dataSize), f);
    std::fclose(f);
    // load new segment, or replace existing ROM
    memory.loadROMSegment(n, &(buf.front()), size_t(dataSize));
  }

#ifdef ENABLE_SDEXT
  void TVC64VM::configureSDCard(bool isEnabled, const std::string& romFileName)
  {
    stopDemo();
    sdext.reset(2);
    sdext.setEnabled(isEnabled);
    memory.setEnableSDExt();
    sdext.openROMFile(romFileName.c_str());
  }
#endif

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
    uint32_t  n = 0U;
    for (int i = 3; i >= 0; i--) {
      n = n << 8;
      n |= uint32_t(floppyDrives[i].getLEDState());
    }
#ifdef ENABLE_SDEXT
    n = n | sdext.getLEDState();
#endif
    vmStatus_.floppyDriveLEDState = n;
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

  void TVC64VM::setDiskImageFile(int n, const std::string& fileName_,
                                 int nTracks_, int nSides_,
                                 int nSectorsPerTrack_)
  {
#ifndef ENABLE_SDEXT
    if (n < 0 || n > 7)
#else
    if (n < 0 || n > 8)
#endif
      throw Ep128Emu::Exception("invalid disk drive number");
    if (n < 4) {
      if (&(wd177x.getFloppyDrive()) == &(floppyDrives[n])) {
        wd177x.setFloppyDrive((Ep128Emu::FloppyDrive *) 0);
        wd177x.setFloppyDrive(&(floppyDrives[n]));
      }
      floppyDrives[n].setDiskImageFile(fileName_,
                                       nTracks_, nSides_, nSectorsPerTrack_);
    }
#ifdef ENABLE_SDEXT
    else if (n >= 8) {
      stopDemo();
      sdext.openImage(fileName_.c_str());
      return;
    }
#endif
  }

  uint32_t TVC64VM::getFloppyDriveLEDState()
  {
    uint32_t  n = 0U;
    for (int i = 3; i >= 0; i--) {
      n = n << 8;
      n |= uint32_t(floppyDrives[i].getLEDState());
    }
#ifdef ENABLE_SDEXT
    n = n | sdext.getLEDState();
#endif
    return n;
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
      return memory.readNoDebug(uint16_t(addr & 0xFFFFU));
    return memory.readRaw(addr & 0x003FFFFFU);
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
                       "VidXY:%3d,%3d  CRTC MA: %04X  RA: %02X\n"
                       "Mode:%2d  Palette: %02X %02X %02X %02X  %02X\n",
                       xPos, yPos, (unsigned int) crtc.getMemoryAddress(),
                       (unsigned int) crtc.getRowAddress(),
                       int(videoRenderer.getVideoMode()),
                       (unsigned int) videoRenderer.getColor(0),
                       (unsigned int) videoRenderer.getColor(1),
                       (unsigned int) videoRenderer.getColor(2),
                       (unsigned int) videoRenderer.getColor(3),
                       (unsigned int) videoRenderer.getColor(4));
    }
    bufp = bufp + n;
    n = std::sprintf(bufp,
                     "Mem: %02X %X  VRAM: %02X  IRQ: %02X  Mask: %02X\n",
                     (unsigned int) (memory.getPaging() & 0xFF),
                     (unsigned int) ((memory.getPaging() >> 14) & 3),
                     (unsigned int) ((memory.getPaging() >> 8) & 0x3F),
                     (unsigned int) (~irqState & 0x1F),
                     (unsigned int) irqEnableMask);
    bufp = bufp + n;
    n = std::sprintf(bufp,
                     "Snd Freq: %04X  Cnt1: %04X  Cnt2: %02X\n"
                     "Volume: %02X  E/D: %X\n",
                     (unsigned int) toneGenFreq,
                     (unsigned int) toneGenCnt1, (unsigned int) toneGenCnt2,
                     (unsigned int) audioOutputLevel,
                     (unsigned int) toneGenEnabled);
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

