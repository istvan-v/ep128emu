
// plus4 -- portable Commodore PLUS/4 emulator
// Copyright (C) 2003-2007 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "disasm.hpp"

#include <cstdio>
#include <vector>

#include "resid/sid.hpp"
#include "p4floppy.hpp"
#include "vc1541.hpp"
#include "vc1581.hpp"

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
    for (uint16_t i = 0x00; i <= 0x1F; i++) {
      setMemoryReadCallback(uint16_t(0xFD40) + i, &sidRegisterRead);
      setMemoryWriteCallback(uint16_t(0xFD40) + i, &sidRegisterWrite);
      setMemoryReadCallback(uint16_t(0xFE80) + i, &sidRegisterRead);
      setMemoryWriteCallback(uint16_t(0xFE80) + i, &sidRegisterWrite);
    }
  }

  Plus4VM::TED7360_::~TED7360_()
  {
  }

  void Plus4VM::TED7360_::playSample(int16_t sampleValue)
  {
    int32_t tmp = vm.soundOutputAccumulator + 16777216;
    tmp = ((tmp >> 3) + (tmp >> 4) - 3145728) + int32_t(sampleValue);
    vm.soundOutputAccumulator = 0;
    vm.sendMonoAudioOutput(tmp);
  }

  void Plus4VM::TED7360_::drawLine(const uint8_t *buf, size_t nBytes)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.drawLine(buf, nBytes);
  }

  void Plus4VM::TED7360_::verticalSync(bool newState_,
                                       unsigned int currentSlot_)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.vsyncStateChange(newState_, currentSlot_);
  }

  void Plus4VM::TED7360_::ntscModeChangeCallback(bool isNTSC_)
  {
    vm.updateTimingParameters(isNTSC_);
  }

  bool Plus4VM::TED7360_::systemCallback(uint8_t n)
  {
    if (getMemoryPage(int(reg_PC >> 14)) >= 0x80 ||
        !(reg_PC >= 0x8004 && reg_PC < 0xFD00))
      return false;
    switch (n) {
    case 0x01:                          // load file
    case 0x02:                          // verify file
      vm.stopDemoPlayback();
      vm.stopDemoRecording(false);
      {
        std::string fileName;
        uint8_t   nameLen = readMemoryCPU(0x00AB);
        uint16_t  nameAddr = uint16_t(readMemoryCPU(0x00AF))
                             | (uint16_t(readMemoryCPU(0x00B0)) << 8);
        while (nameLen) {
          char    c = char(readMemoryCPU(nameAddr, true));
          if (c == '\0')
            break;
          fileName += c;
          nameLen--;
          nameAddr = (nameAddr + 1) & 0xFFFF;
        }
        std::FILE *f = (std::FILE *) 0;
        int       err = vm.openFileInWorkingDirectory(f, fileName, "rb");
        if (!err) {
          reg_AC = 0xF9;
          int     c;
          c = std::fgetc(f);
          if (c != EOF) {
            uint16_t  addr = uint16_t(c & 0xFF);
            c = std::fgetc(f);
            if (c != EOF) {
              addr |= (uint16_t(c & 0xFF) << 8);
              reg_AC = 0x00;
              if (readMemoryCPU(0x00AD) == 0x00)
                addr = uint16_t(readMemoryCPU(0x00B4))
                       | (uint16_t(readMemoryCPU(0x00B5)) << 8);
              unsigned int  nBytes = 0U;
              do {
                c = std::fgetc(f);
                if (c == EOF)
                  break;
                if (n == 0x01)          // load
                  writeMemoryCPU(addr, uint8_t(c & 0xFF));
                else if (uint8_t(c & 0xFF) != readMemoryCPU(addr, true)) {
                  reg_AC = 0xF8;        // verify error
                  break;
                }
                addr = (addr + 1) & 0xFFFF;
              } while (++nBytes < 0xFFFFU);
              writeMemoryCPU(0x009D, uint8_t(addr) & 0xFF);
              writeMemoryCPU(0x009E, uint8_t(addr >> 8) & 0xFF);
            }
          }
          std::fclose(f);
        }
        else
          reg_AC = uint8_t(err + 256);
        return true;
      }
      break;
    case 0x03:                          // save file
      vm.stopDemoPlayback();
      vm.stopDemoRecording(false);
      {
        std::string fileName;
        uint8_t   nameLen = readMemoryCPU(0x00AB);
        uint16_t  nameAddr = uint16_t(readMemoryCPU(0x00AF))
                             | (uint16_t(readMemoryCPU(0x00B0)) << 8);
        while (nameLen) {
          char    c = char(readMemoryCPU(nameAddr, true));
          if (c == '\0')
            break;
          fileName += c;
          nameLen--;
          nameAddr = (nameAddr + 1) & 0xFFFF;
        }
        std::FILE *f = (std::FILE *) 0;
        int       err = vm.openFileInWorkingDirectory(f, fileName, "wb");
        if (!err) {
          reg_AC = 0xFA;
          uint8_t   c = readMemoryCPU(0x00B2);
          uint16_t  addr = uint16_t(c);
          if (std::fputc(c, f) != EOF) {
            c = readMemoryCPU(0x00B3);
            addr |= (uint16_t(c) << 8);
            if (std::fputc(c, f) != EOF) {
              uint16_t  endAddr = uint16_t(readMemoryCPU(0x009D))
                                  | (uint16_t(readMemoryCPU(0x009E)) << 8);
              endAddr = endAddr & 0xFFFF;
              while (addr != endAddr) {
                c = readMemoryCPU(addr, true);
                if (std::fputc(c, f) == EOF)
                  break;
                addr = (addr + 1) & 0xFFFF;
              }
              if (addr == endAddr)
                reg_AC = 0x00;
            }
          }
          std::fclose(f);
        }
        else
          reg_AC = uint8_t(err + 256);
        return true;
      }
      break;
    }
    return false;
  }

  void Plus4VM::TED7360_::breakPointCallback(bool isWrite,
                                             uint16_t addr, uint8_t value)
  {
    if (vm.noBreakOnDataRead && !isWrite) {
      if (vm.ted->getRegisters().reg_PC != addr)
        return;
    }
    vm.breakPointCallback(vm.breakPointCallbackUserData,
                          false, isWrite, addr, value);
  }

  uint8_t Plus4VM::TED7360_::sidRegisterRead(void *userData, uint16_t addr)
  {
    TED7360_& ted = *(reinterpret_cast<TED7360_ *>(userData));
    if (ted.vm.sidEnabled) {
      ted.dataBusState =
          uint8_t(ted.vm.sid_->read(uint8_t(addr) & uint8_t(0x1F)));
    }
    return ted.dataBusState;
  }

  void Plus4VM::TED7360_::sidRegisterWrite(void *userData,
                                           uint16_t addr, uint8_t value)
  {
    TED7360_& ted = *(reinterpret_cast<TED7360_ *>(userData));
    ted.dataBusState = value;
    ted.vm.sidEnabled = true;
    ted.vm.sid_->write(uint8_t(addr) & uint8_t(0x1F), value);
  }

  // --------------------------------------------------------------------------

  void Plus4VM::stopDemoPlayback()
  {
    if (isPlayingDemo) {
      isPlayingDemo = false;
      demoTimeCnt = 0U;
      demoBuffer.clear();
      // tape button state sensing is disabled while recording or playing demo
      ted->setTapeButtonState(!isRecordingDemo && getTapeButtonState() != 0);
      // clear keyboard state at end of playback
      for (int i = 0; i < 128; i++)
        ted->setKeyState(i, false);
    }
  }

  void Plus4VM::stopDemoRecording(bool writeFile_)
  {
    isRecordingDemo = false;
    // tape button state sensing is disabled while recording or playing demo
    ted->setTapeButtonState(!isPlayingDemo && getTapeButtonState() != 0);
    if (writeFile_ && demoFile != (Ep128Emu::File *) 0) {
      // if file object is still open:
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

  void Plus4VM::updateTimingParameters(bool ntscMode_)
  {
    size_t  singleClockFreq = tedInputClockFrequency;
    if (!ntscMode_)
      singleClockFreq = ((singleClockFreq + 40) / 80) << 2;
    else
      singleClockFreq = ((singleClockFreq + 32) / 64) << 2;
    tedTimesliceLength = int64_t(((uint64_t(1000000) << 32)
                                  + (singleClockFreq >> 1))
                                 / singleClockFreq);
    size_t  freqMult = cpuClockFrequency;
    if (freqMult > 1000)
      freqMult = (freqMult + singleClockFreq) / (singleClockFreq << 1);
    freqMult = (freqMult > 1 ? (freqMult < 100 ? freqMult : 100) : 1);
    ted->setCPUClockMultiplier(freqMult);
    if ((singleClockFreq >> 2) != soundClockFrequency) {
      soundClockFrequency = singleClockFreq >> 2;
      setAudioConverterSampleRate(float(long(soundClockFrequency)));
    }
  }

  void Plus4VM::resetFloppyDrives(uint8_t driveMask_, bool deleteUnusedDrives_)
  {
    for (int i = 0; i < 4; i++) {
      if (driveMask_ & 0x01) {
        if (floppyDrives[i]) {
          if (floppyDrives[i]->haveDisk() || !deleteUnusedDrives_)
            floppyDrives[i]->reset();
          else {
            // "garbage collect" unused floppy drives to improve performance
            delete floppyDrives[i];
            floppyDrives[i] = (FloppyDrive *) 0;
            ted->getSerialPort().removeDevice(i + 8);
          }
        }
      }
      driveMask_ = driveMask_ >> 1;
    }
  }

  Plus4VM::Plus4VM(Ep128Emu::VideoDisplay& display_,
                   Ep128Emu::AudioOutput& audioOutput_)
    : VirtualMachine(display_, audioOutput_),
      ted((TED7360_ *) 0),
      cpuClockFrequency(1),
      tedInputClockFrequency(17734475),
      soundClockFrequency(0),
      tedTimesliceLength(0),
      tedTimeRemaining(0),
      tapeTimesliceLength(0),
      tapeTimeRemaining(0),
      floppyTimeRemaining(0),
      demoFile((Ep128Emu::File *) 0),
      demoBuffer(),
      isRecordingDemo(false),
      isPlayingDemo(false),
      snapshotLoadFlag(false),
      demoTimeCnt(0U),
      sid_((SID *) 0),
      soundOutputAccumulator(0),
      sidEnabled(false),
      floppyROM_1541((uint8_t *) 0),
      floppyROM_1581_0((uint8_t *) 0),
      floppyROM_1581_1((uint8_t *) 0)
  {
    for (int i = 0; i < 4; i++)
      floppyDrives[i] = (FloppyDrive *) 0;
    ted = new TED7360_(*this);
    try {
      updateTimingParameters(false);
      // reset
      ted->reset(true);
      // use PLUS/4 colormap
      Ep128Emu::VideoDisplay::DisplayParameters
          dp(display.getDisplayParameters());
      dp.indexToRGBFunc = &TED7360::convertPixelToRGB;
      display.setDisplayParameters(dp);
      sid_ = new SID();
      sid_->set_chip_model(MOS8580);
      sid_->enable_external_filter(false);
      sid_->reset();
    }
    catch (...) {
      if (sid_)
        delete sid_;
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
    for (int i = 0; i < 4; i++) {
      if (floppyDrives[i]) {
        delete floppyDrives[i];
        floppyDrives[i] = (FloppyDrive *) 0;
      }
    }
    if (floppyROM_1541)
      delete[] floppyROM_1541;
    if (floppyROM_1581_0)
      delete[] floppyROM_1581_0;
    if (floppyROM_1581_1)
      delete[] floppyROM_1581_1;
    delete sid_;
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
    bool    haveFloppy = false;
    for (int i = 0; i < 4; i++) {
      if (floppyDrives[i])
        haveFloppy = true;
    }
    tedTimeRemaining += (int64_t(microseconds) << 32);
    while (tedTimeRemaining > 0) {
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
        tapeTimeRemaining += tedTimesliceLength;
        if (tapeTimeRemaining > 0) {
          // assume tape sample rate < single clock frequency
          tapeTimeRemaining -= tapeTimesliceLength;
          setTapeMotorState(ted->getTapeMotorState());
          ted->setTapeInput(runTape(ted->getTapeOutput() ? 1 : 0) > 0);
        }
      }
      ted->runOneCycle();
      if (sidEnabled) {
        sid_->clock();
        soundOutputAccumulator += int32_t(sid_->output());
      }
      if (haveFloppy) {
        floppyTimeRemaining += tedTimesliceLength;
        while (floppyTimeRemaining > 0) {
          // use a timeslice of fixed 1 us length (1 or 2 cycles, depending
          // on drive type)
          floppyTimeRemaining -= (int64_t(1) << 32);
          if (!(isRecordingDemo || isPlayingDemo)) {
            for (int i = 0; i < 4; i++) {
              if (floppyDrives[i])
                floppyDrives[i]->run(ted->getSerialPort());
            }
          }
          else {
            // disable floppy disk I/O while recording or playing a demo
            for (int i = 0; i < 4; i++) {
              if (floppyDrives[i]) {
                ted->getSerialPort().removeDevices(0xFFFE);
                floppyDrives[i]->run(ted->getSerialPort());
              }
            }
            ted->getSerialPort().removeDevices(0xFFFE);
          }
        }
      }
      tedTimeRemaining -= tedTimesliceLength;
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
    sid_->reset();
    if (isColdReset)
      sidEnabled = false;
    resetFloppyDrives(0x0F, isColdReset);
  }

  void Plus4VM::resetMemoryConfiguration(size_t memSize)
  {
    try {
      stopDemo();
      // delete all ROM segments
      for (uint8_t n = 0; n < 8; n++)
        loadROMSegment(n, (char *) 0, 0);
      for (int i = 0; i < 4; i++) {
        if (floppyDrives[i]) {
          for (int n = 0; n < 3; n++)
            floppyDrives[i]->setROMImage(n, (uint8_t *) 0);
        }
      }
      if (floppyROM_1541) {
        delete[] floppyROM_1541;
        floppyROM_1541 = (uint8_t *) 0;
      }
      if (floppyROM_1581_0) {
        delete[] floppyROM_1581_0;
        floppyROM_1581_0 = (uint8_t *) 0;
      }
      if (floppyROM_1581_1) {
        delete[] floppyROM_1581_1;
        floppyROM_1581_1 = (uint8_t *) 0;
      }
      // set new RAM size
      ted->setRAMSize(memSize);
    }
    catch (...) {
      try {
        this->reset(true);
      }
      catch (...) {
      }
      throw;
    }
    // cold reset
    this->reset(true);
  }

  void Plus4VM::loadROMSegment(uint8_t n, const char *fileName, size_t offs)
  {
    stopDemo();
    int     floppyROMSegment = -1;
    uint8_t **floppyROMPtr = (uint8_t **) 0;
    if (n >= 8) {
      switch (n) {
      case 0x10:
        floppyROMSegment = 2;
        floppyROMPtr = &floppyROM_1541;
        break;
      case 0x30:
        floppyROMSegment = 0;
        floppyROMPtr = &floppyROM_1581_0;
        break;
      case 0x31:
        floppyROMSegment = 1;
        floppyROMPtr = &floppyROM_1581_1;
        break;
      default:
        return;
      }
    }
    // clear segment first
    if (floppyROMSegment < 0) {
      ted->loadROM(int(n >> 1), int(n & 1) << 14, 0, (uint8_t *) 0);
    }
    else {
      for (int i = 0; i < 4; i++) {
        if (floppyDrives[i])
          floppyDrives[i]->setROMImage(floppyROMSegment, (uint8_t *) 0);
      }
    }
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
    if (floppyROMSegment < 0) {
      ted->loadROM(int(n) >> 1, int(n & 1) << 14, 16384, &(buf.front()));
    }
    else {
      if ((*floppyROMPtr) == (uint8_t *) 0)
        (*floppyROMPtr) = new uint8_t[16384];
      for (int i = 0; i < 16384; i++)
        (*floppyROMPtr)[i] = buf[i];
      for (int i = 0; i < 4; i++) {
        if (floppyDrives[i])
          floppyDrives[i]->setROMImage(floppyROMSegment, (*floppyROMPtr));
      }
    }
  }

  void Plus4VM::setCPUFrequency(size_t freq_)
  {
    size_t  freq = (freq_ > 1 ? (freq_ < 150000000 ? freq_ : 150000000) : 1);
    if (freq == cpuClockFrequency)
      return;
    stopDemoPlayback();         // changing configuration implies stopping
    stopDemoRecording(false);   // any demo playback or recording
    cpuClockFrequency = freq;
    updateTimingParameters(ted->getIsNTSCMode());
  }

  void Plus4VM::setVideoFrequency(size_t freq_)
  {
    size_t  freq = (freq_ > 7159090 ? (freq_ < 35468950 ? freq_ : 35468950)
                                      : 7159090);
    if (freq == tedInputClockFrequency)
      return;
    stopDemoPlayback();         // changing configuration implies stopping
    stopDemoRecording(false);   // any demo playback or recording
    tedInputClockFrequency = freq;
    updateTimingParameters(ted->getIsNTSCMode());
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

  void Plus4VM::setDiskImageFile(int n, const std::string& fileName_,
                                 int nTracks_, int nSides_,
                                 int nSectorsPerTrack_)
  {
    (void) nTracks_;
    (void) nSides_;
    (void) nSectorsPerTrack_;
    if (n < 0 || n > 3)
      throw Ep128Emu::Exception("invalid floppy drive number");
    if (fileName_.length() == 0) {
      // remove disk
      if (floppyDrives[n])
        floppyDrives[n]->setDiskImageFile(fileName_);
    }
    else {
      // insert or replace disk
      bool    isD64 = false;
      {
        // find out file type
        std::FILE *f = std::fopen(fileName_.c_str(), "rb");
        if (f) {
          if (std::fseek(f, 0L, SEEK_END) >= 0) {
            long    fSize = std::ftell(f);
            isD64 = (fSize == 174848L || fSize == 175531L);
          }
          std::fclose(f);
        }
        else
          throw Ep128Emu::Exception("error opening disk image file");
      }
      if (floppyDrives[n]) {
        if ((typeid(*(floppyDrives[n])) == typeid(VC1581) && isD64) ||
            (typeid(*(floppyDrives[n])) != typeid(VC1581) && !isD64)) {
          // need to change drive type
          delete floppyDrives[n];
          floppyDrives[n] = (FloppyDrive *) 0;
          ted->getSerialPort().removeDevice(n + 8);
        }
      }
      if (!floppyDrives[n]) {
        if (isD64)
          floppyDrives[n] = new VC1541(n + 8);
        else
          floppyDrives[n] = new VC1581(n + 8);
        floppyDrives[n]->setROMImage(0, floppyROM_1581_0);
        floppyDrives[n]->setROMImage(1, floppyROM_1581_1);
        floppyDrives[n]->setROMImage(2, floppyROM_1541);
      }
      floppyDrives[n]->setDiskImageFile(fileName_);
    }
  }

  void Plus4VM::setTapeFileName(const std::string& fileName)
  {
    Ep128Emu::VirtualMachine::setTapeFileName(fileName);
    setTapeMotorState(ted->getTapeMotorState());
    if (haveTape()) {
      int64_t tmp = getTapeSampleRate();
      tapeTimesliceLength = ((int64_t(1000000) << 32) + (tmp >> 1)) / tmp;
    }
    else
      tapeTimesliceLength = 0;
    tapeTimeRemaining = 0;
  }

  void Plus4VM::tapePlay()
  {
    Ep128Emu::VirtualMachine::tapePlay();
    // tape button state sensing is disabled while recording or playing demo
    ted->setTapeButtonState(!(isRecordingDemo || isPlayingDemo) &&
                            getTapeButtonState() != 0);
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void Plus4VM::tapeRecord()
  {
    Ep128Emu::VirtualMachine::tapeRecord();
    // tape button state sensing is disabled while recording or playing demo
    ted->setTapeButtonState(!(isRecordingDemo || isPlayingDemo) &&
                            getTapeButtonState() != 0);
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void Plus4VM::tapeStop()
  {
    Ep128Emu::VirtualMachine::tapeStop();
    ted->setTapeButtonState(false);
  }

  void Plus4VM::setBreakPoints(const Ep128Emu::BreakPointList& bpList)
  {
    for (size_t i = 0; i < bpList.getBreakPointCnt(); i++) {
      const Ep128Emu::BreakPoint& bp = bpList.getBreakPoint(i);
      if (bp.isIO())
        throw Ep128Emu::Exception("setting breakpoints on I/O ports is not "
                                  "supported on this machine");
      if (bp.haveSegment())
        throw Ep128Emu::Exception("segment:offset format breakpoints are not "
                                  "supported on this machine");
    }
    for (size_t i = 0; i < bpList.getBreakPointCnt(); i++) {
      const Ep128Emu::BreakPoint& bp = bpList.getBreakPoint(i);
      ted->setBreakPoint(bp.addr(), bp.priority(), bp.isRead(), bp.isWrite());
    }
  }

  Ep128Emu::BreakPointList Plus4VM::getBreakPoints()
  {
    return (ted->getBreakPointList());
  }

  void Plus4VM::clearBreakPoints()
  {
    ted->clearBreakPoints();
  }

  void Plus4VM::setBreakPointPriorityThreshold(int n)
  {
    ted->setBreakPointPriorityThreshold(n);
  }

  void Plus4VM::setSingleStepMode(bool isEnabled)
  {
    ted->setSingleStepMode(isEnabled);
  }

  uint8_t Plus4VM::getMemoryPage(int n) const
  {
    return ted->getMemoryPage(n);
  }

  uint8_t Plus4VM::readMemory(uint32_t addr, bool isCPUAddress) const
  {
    if (isCPUAddress)
      return ted->readMemoryCPU(uint16_t(addr & 0xFFFFU));
    return ted->readMemoryRaw(addr & uint32_t(0x003FFFFF));
  }

  void Plus4VM::writeMemory(uint32_t addr, uint8_t value, bool isCPUAddress)
  {
    if (isRecordingDemo || isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    if (isCPUAddress)
      ted->writeMemoryCPU(uint16_t(addr & 0xFFFFU), value);
    else
      ted->writeMemoryRaw(addr & uint32_t(0x003FFFFF), value);
  }

  uint16_t Plus4VM::getProgramCounter() const
  {
    return ted->getRegisters().reg_PC;
  }

  uint16_t Plus4VM::getStackPointer() const
  {
    return (uint16_t(0x0100)
            | uint16_t((ted->getRegisters().reg_SP + uint8_t(1))
                       & uint8_t(0xFF)));
  }

  void Plus4VM::listCPURegisters(std::string& buf) const
  {
    char    tmpBuf[96];
    const M7501Registers& r = ted->getRegisters();
    std::sprintf(&(tmpBuf[0]),
                 " PC  SR AC XR YR SP\n"
                 "%04X %02X %02X %02X %02X %02X",
                 (unsigned int) r.reg_PC, (unsigned int) r.reg_SR,
                 (unsigned int) r.reg_AC, (unsigned int) r.reg_XR,
                 (unsigned int) r.reg_YR, (unsigned int) r.reg_SP);
    buf = &(tmpBuf[0]);
  }

  uint32_t Plus4VM::disassembleInstruction(std::string& buf,
                                           uint32_t addr, bool isCPUAddress,
                                           int32_t offs) const
  {
    return M7501Disassembler::disassembleInstruction(buf, (*this),
                                                     addr, isCPUAddress, offs);
  }

  const M7501Registers& Plus4VM::getCPURegisters() const
  {
    return ted->getRegisters();
  }

  void Plus4VM::saveState(Ep128Emu::File& f)
  {
    ted->saveState(f);
    sid_->saveState(f);
    {
      Ep128Emu::File::Buffer  buf;
      buf.setPosition(0);
      buf.writeUInt32(0x01000000);      // version number
      buf.writeUInt32(uint32_t(cpuClockFrequency));
      buf.writeUInt32(uint32_t(tedInputClockFrequency));
      buf.writeUInt32(uint32_t(soundClockFrequency));
      buf.writeBoolean(sidEnabled);
      f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_P4VM_STATE, buf);
    }
  }

  void Plus4VM::saveMachineConfiguration(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    buf.writeUInt32(uint32_t(cpuClockFrequency));
    buf.writeUInt32(uint32_t(tedInputClockFrequency));
    buf.writeUInt32(uint32_t(soundClockFrequency));
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_P4VM_CONFIG, buf);
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
    ted->setTapeInput(false);
    setTapeMotorState(false);
    ted->getSerialPort().removeDevices(0xFFFE);
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
    demoTimeCnt = 0U;
    // tape button state sensing is disabled while recording or playing demo
    ted->setTapeButtonState(false);
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
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible plus4 snapshot format");
    }
    ted->setTapeMotorState(false);
    setTapeMotorState(false);
    stopDemo();
    snapshotLoadFlag = true;
    resetFloppyDrives(0x0F, true);
    try {
      uint32_t  tmpCPUClockFrequency = buf.readUInt32();
      uint32_t  tmpTEDInputClockFrequency = buf.readUInt32();
      uint32_t  tmpSoundClockFrequency = buf.readUInt32();
      (void) tmpCPUClockFrequency;
      (void) tmpTEDInputClockFrequency;
      (void) tmpSoundClockFrequency;
      sidEnabled = buf.readBoolean();
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "plus4 snapshot data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Plus4VM::loadMachineConfiguration(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible plus4 "
                                "machine configuration format");
    }
    try {
      uint32_t  tmpCPUClockFrequency = buf.readUInt32();
      uint32_t  tmpTEDInputClockFrequency = buf.readUInt32();
      uint32_t  tmpSoundClockFrequency = buf.readUInt32();
      (void) tmpSoundClockFrequency;
      setCPUFrequency(tmpCPUClockFrequency);
      setVideoFrequency(tmpTEDInputClockFrequency);
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "plus4 machine configuration data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Plus4VM::loadDemo(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
#if 0
    if (version != 0x00010900) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible plus4 demo format");
    }
#endif
    (void) version;
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    ted->setTapeMotorState(false);
    ted->setTapeInput(false);
    setTapeMotorState(false);
    ted->getSerialPort().removeDevices(0xFFFE);
    stopDemo();
    for (int i = 0; i < 128; i++)
      ted->setKeyState(i, false);
    // initialize time counter with first delta time
    demoTimeCnt = readDemoTimeCnt(buf);
    isPlayingDemo = true;
    // tape button state sensing is disabled while recording or playing demo
    ted->setTapeButtonState(false);
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
      f.registerChunkType(p1);
      p1 = (ChunkType_Plus4VMConfig *) 0;
      p2 = new ChunkType_Plus4VMSnapshot(*this);
      f.registerChunkType(p2);
      p2 = (ChunkType_Plus4VMSnapshot *) 0;
      p3 = new ChunkType_Plus4DemoStream(*this);
      f.registerChunkType(p3);
      p3 = (ChunkType_Plus4DemoStream *) 0;
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
    sid_->registerChunkType(f);
  }

}       // namespace Plus4

