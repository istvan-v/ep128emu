
// plus4 -- portable Commodore Plus/4 emulator
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
#include "cpu.hpp"
#include "cia8520.hpp"
#include "wd177x.hpp"
#include "serial.hpp"
#include "p4floppy.hpp"
#include "vc1581.hpp"

namespace Plus4 {

  VC1581::CIA8520_::~CIA8520_()
  {
  }

  void VC1581::CIA8520_::interruptCallback(bool irqState)
  {
    vc1581.interruptRequestFlag = irqState;
  }

  uint8_t VC1581::readRAM(void *userData, uint16_t addr)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = vc1581.memory_ram[addr & 0x1FFF];
    return vc1581.dataBusState;
  }

  uint8_t VC1581::readDummy(void *userData, uint16_t addr)
  {
    (void) addr;
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    return vc1581.dataBusState;
  }

  uint8_t VC1581::readCIA8520(void *userData, uint16_t addr)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = vc1581.cia.readRegister(addr & 0x000F);
    return vc1581.dataBusState;
  }

  uint8_t VC1581::readWD177x(void *userData, uint16_t addr)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    switch (addr & 3) {
    case 0:
      vc1581.dataBusState = vc1581.wd177x.readStatusRegister();
      break;
    case 1:
      vc1581.dataBusState = vc1581.wd177x.readTrackRegister();
      break;
    case 2:
      vc1581.dataBusState = vc1581.wd177x.readSectorRegister();
      break;
    case 3:
      vc1581.dataBusState = vc1581.wd177x.readDataRegister();
      break;
    }
    return vc1581.dataBusState;
  }

  uint8_t VC1581::readROM0(void *userData, uint16_t addr)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    if (vc1581.memory_rom_0)
      vc1581.dataBusState = vc1581.memory_rom_0[addr & 0x3FFF];
    return vc1581.dataBusState;
  }

  uint8_t VC1581::readROM1(void *userData, uint16_t addr)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    if (vc1581.memory_rom_1)
      vc1581.dataBusState = vc1581.memory_rom_1[addr & 0x3FFF];
    return vc1581.dataBusState;
  }

  void VC1581::writeRAM(void *userData, uint16_t addr, uint8_t value)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = value & uint8_t(0xFF);
    vc1581.memory_ram[addr & 0x1FFF] = vc1581.dataBusState;
  }

  void VC1581::writeDummy(void *userData, uint16_t addr, uint8_t value)
  {
    (void) addr;
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = value & uint8_t(0xFF);
  }

  void VC1581::writeCIA8520(void *userData, uint16_t addr, uint8_t value)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = value & uint8_t(0xFF);
    vc1581.cia.writeRegister(addr & 0x000F, vc1581.dataBusState);
  }

  void VC1581::writeWD177x(void *userData, uint16_t addr, uint8_t value)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = value & uint8_t(0xFF);
    switch (addr & 3) {
    case 0:
      vc1581.wd177x.writeCommandRegister(vc1581.dataBusState);
      break;
    case 1:
      vc1581.wd177x.writeTrackRegister(vc1581.dataBusState);
      break;
    case 2:
      vc1581.wd177x.writeSectorRegister(vc1581.dataBusState);
      break;
    case 3:
      vc1581.wd177x.writeDataRegister(vc1581.dataBusState);
      break;
    }
  }

  VC1581::VC1581(int driveNum_)
    : FloppyDrive(driveNum_),
      cpu(),
      cia(*this),
      wd177x(),
      memory_rom_0((uint8_t *) 0),
      memory_rom_1((uint8_t *) 0),
      deviceNumber(driveNum_),
      dataBusState(0),
      interruptRequestFlag(false),
      ciaPortAInput(0),
      ciaPortBInput(0),
      diskChangeCnt(0)
  {
    // initialize memory map
    cpu.setMemoryCallbackUserData((void *) this);
    for (uint16_t i = 0x0000; i <= 0x1FFF; i++) {
      cpu.setMemoryReadCallback(i, &readRAM);
      cpu.setMemoryWriteCallback(i, &writeRAM);
    }
    for (uint16_t i = 0x2000; i <= 0x7FFF; i++) {
      cpu.setMemoryReadCallback(i, &readDummy);
      cpu.setMemoryWriteCallback(i, &writeDummy);
    }
    for (uint16_t i = 0x4000; i <= 0x43FF; i++) {
      cpu.setMemoryReadCallback(i, &readCIA8520);
      cpu.setMemoryWriteCallback(i, &writeCIA8520);
    }
    for (uint16_t i = 0x6000; i <= 0x63FF; i++) {
      cpu.setMemoryReadCallback(i, &readWD177x);
      cpu.setMemoryWriteCallback(i, &writeWD177x);
    }
    for (uint16_t i = 0x8000; i <= 0xBFFF; i++)
      cpu.setMemoryReadCallback(i, &readROM0);
    for (uint32_t i = 0xC000; i <= 0xFFFF; i++)
      cpu.setMemoryReadCallback(uint16_t(i), &readROM1);
    for (uint32_t i = 0x8000; i <= 0xFFFF; i++)
      cpu.setMemoryWriteCallback(uint16_t(i), &writeDummy);
    // clear RAM
    for (uint16_t i = 0; i < 8192; i++)
      memory_ram[i] = 0x00;
    // select drive number
    ciaPortAInput = uint8_t((driveNum_ & 3) << 3) | uint8_t(0x02);
    // configure WD177x emulation
    wd177x.setIsWD1773(false);
    wd177x.setEnableBusyFlagHack(true);
    this->reset();
  }

  VC1581::~VC1581()
  {
  }

  void VC1581::setROMImage(int n, const uint8_t *romData_)
  {
    switch (n) {
    case 0:
      memory_rom_0 = romData_;
      break;
    case 1:
      memory_rom_1 = romData_;
      break;
    }
  }

  void VC1581::setDiskImageFile(const std::string& fileName_)
  {
    try {
      wd177x.setDiskImageFile(fileName_, 80, 2, 10);
    }
    catch (...) {
      // not ready, disk changed
      diskChangeCnt = 350000;
      ciaPortAInput = (ciaPortAInput & uint8_t(0x7F)) | uint8_t(0x02);
      // update write protect flag
      ciaPortBInput |= uint8_t(0x40);
      if (wd177x.getIsWriteProtected())
        ciaPortBInput &= uint8_t(0xBF);
      throw;
    }
    // not ready, disk changed
    diskChangeCnt = 350000;
    ciaPortAInput = (ciaPortAInput & uint8_t(0x7F)) | uint8_t(0x02);
    // update write protect flag
    ciaPortBInput |= uint8_t(0x40);
    if (wd177x.getIsWriteProtected())
      ciaPortBInput &= uint8_t(0xBF);
  }

  bool VC1581::haveDisk() const
  {
    return wd177x.haveDisk();
  }

  void VC1581::run(SerialBus& serialBus_, size_t t)
  {
    {
      uint8_t n = ciaPortBInput & uint8_t(0x7A);
      n |= (serialBus_.getDATA() & uint8_t(0x01));
      n |= (serialBus_.getCLK() & uint8_t(0x04));
      n |= (serialBus_.getATN() & uint8_t(0x80));
      cia.setPortB(n ^ uint8_t(0x85));
    }
    cia.setFlagState(!!(serialBus_.getATN()));
    while (t--) {
      if (interruptRequestFlag)
        cpu.interruptRequest();
      cpu.run(2);
      if (diskChangeCnt) {
        diskChangeCnt--;
        if (!diskChangeCnt)
          ciaPortAInput = (ciaPortAInput | uint8_t(0x80)) & uint8_t(0xFD);
      }
      cia.setPortA(ciaPortAInput);
      cia.run(2);
      wd177x.setSide(cia.getPortA() & 0x01);
    }
    uint8_t n = cia.getPortB();
    serialBus_.setCLK(deviceNumber, !(n & uint8_t(0x08)));
    if (!(((serialBus_.getATN() ^ uint8_t(0xFF)) & n) & uint8_t(0x10)))
      serialBus_.setDATA(deviceNumber, !(n & uint8_t(0x02)));
    else
      serialBus_.setDATA(deviceNumber, false);
  }

  void VC1581::reset()
  {
    interruptRequestFlag = false;
    cpu.reset(true);
    cia.reset();
    wd177x.reset();
    diskChangeCnt = 350000;
    ciaPortAInput = (ciaPortAInput & uint8_t(0x7F)) | uint8_t(0x02);
  }

}       // namespace Plus4

