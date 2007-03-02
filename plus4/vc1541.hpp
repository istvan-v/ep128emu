
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

#ifndef EP128EMU_VC1541_HPP
#define EP128EMU_VC1541_HPP

#include "ep128emu.hpp"
#include "cpu.hpp"
#include "via6522.hpp"
#include "serial.hpp"
#include "p4floppy.hpp"

namespace Plus4 {

  class VC1541 : public FloppyDrive {
   private:
    class VIA6522_ : public VIA6522 {
     private:
      VC1541& vc1541;
     public:
      VIA6522_(VC1541& vc1541_);
      virtual ~VIA6522_();
      virtual void irqStateChangeCallback(bool newState);
    };
    M7501       cpu;
    VIA6522_    via1;                   // serial port interface (1800..1BFF)
    VIA6522_    via2;                   // floppy control (1C00..1FFF)
    const uint8_t *memory_rom;          // 16K ROM, 8000..FFFF
    uint8_t     memory_ram[2048];       // 2K RAM, 0000..0FFF
    uint8_t     trackBuffer_GCR[8192];
    uint8_t     trackBuffer_D64[5376];  // for 21 256-byte sectors
    uint8_t     deviceNumber;
    uint8_t     diskID;
    uint8_t     dataBusState;
    uint8_t     via1PortBInput;
    bool        interruptRequestFlag;
    bool        writeProtectFlag;
    bool        trackDirtyFlag;
    bool        headLoadedFlag;
    bool        prvByteWasFF;           // for finding sync
    bool        syncFlag;               // true if found sync
    uint8_t     motorUpdateCnt;         // decrements from 15 to 0
    uint8_t     shiftRegisterBitCnt;    // 0 to 7, byte ready on 0
    int         shiftRegisterBitCntFrac;    // 0 to 65535
    int         headPosition;           // index to track buffer
    int         currentTrack;           // 0 to 40 (1 to 35 are valid tracks)
    int         currentTrackFrac;       // -65536 to 65536 (-32768 and 32768
                                        // are "half tracks")
    int         steppingDirection;      // 1: stepping in, -1: stepping out,
                                        // 0: not stepping
    int         currentTrackStepperMotorPhase;
    int         spindleMotorSpeed;      // 0 (stopped) to 65536 (full speed)
    std::FILE   *imageFile;
    // ----------------
    static uint8_t readMemory_RAM(void *userData, uint16_t addr);
    static uint8_t readMemory_Dummy(void *userData, uint16_t addr);
    static uint8_t readMemory_VIA1(void *userData, uint16_t addr);
    static uint8_t readMemory_VIA2(void *userData, uint16_t addr);
    static uint8_t readMemory_ROM(void *userData, uint16_t addr);
    static void writeMemory_RAM(void *userData, uint16_t addr, uint8_t value);
    static void writeMemory_Dummy(void *userData, uint16_t addr, uint8_t value);
    static void writeMemory_VIA1(void *userData, uint16_t addr, uint8_t value);
    static void writeMemory_VIA2(void *userData, uint16_t addr, uint8_t value);
    static void gcrEncodeFourBytes(uint8_t *outBuf, const uint8_t *inBuf);
    static bool gcrDecodeFourBytes(uint8_t *outBuf, const uint8_t *inBuf);
    void gcrEncodeTrack(int trackNum, int nSectors, int nBytes);
    int gcrDecodeTrack(int trackNum, int nSectors, int nBytes);
    bool updateMotors();
    bool readTrack(int trackNum = -1);
    bool flushTrack(int trackNum = -1);
    bool setCurrentTrack(int trackNum);
   public:
    VC1541(int driveNum_ = 8);
    virtual ~VC1541();
    // use 'romData_' (should point to 16384 bytes of data which is expected
    // to remain valid until either a new address is set or the object is
    // destroyed, or can be NULL for no ROM data) for ROM bank 'n'; allowed
    // values for 'n' are:
    //   0: 1581 low
    //   1: 1581 high
    //   2: 1541
    // if this drive type does not use the selected ROM bank, the function call
    // is ignored
    virtual void setROMImage(int n, const uint8_t *romData_);
    // open disk image file 'fileName_' (an empty file name means no disk)
    virtual void setDiskImageFile(const std::string& fileName_);
    // returns true if there is a disk image file opened
    virtual bool haveDisk() const;
    // run floppy emulation for 't' microseconds
    virtual void run(SerialBus& serialBus_, size_t t = 1);
    // reset floppy drive
    virtual void reset();
  };

}       // namespace Plus4

#endif  // EP128EMU_VC1541_HPP

