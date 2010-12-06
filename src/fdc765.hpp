
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

#ifndef EP128EMU_FDC765_HPP
#define EP128EMU_FDC765_HPP

#include "ep128emu.hpp"

namespace CPC464 {

  class FDC765 {
   public:
    struct FDCCommandParams {
      uint8_t   commandCode;
      uint8_t   unitNumber;             // drive number (0 to 3)
      uint8_t   physicalCylinders[4];   // stored separately for each drive
      uint8_t   physicalSide;
      bool      motorOn;
      uint8_t   cylinderID;
      uint8_t   headID;
      uint8_t   sectorID;
      uint8_t   sectorSizeCode;         // sector size = 0x80 << sectorSizeCode
      uint8_t   statusRegister0;
      uint8_t   statusRegister1;
      uint8_t   statusRegister2;
      uint8_t   statusRegister3;
    };
    typedef enum {
      CPCDISK_NO_ERROR = 0,
      CPCDISK_ERROR_UNKNOWN = -1,
      CPCDISK_ERROR_INVALID_COMMAND = -2,
      CPCDISK_ERROR_NO_DISK = -3,
      CPCDISK_ERROR_NOT_READY = -4,
      CPCDISK_ERROR_WRITE_PROTECTED = -5,
      CPCDISK_ERROR_INVALID_TRACK = -6,
      CPCDISK_ERROR_INVALID_SIDE = -7,
      CPCDISK_ERROR_BAD_CYLINDER = -8,
      CPCDISK_ERROR_WRONG_CYLINDER = -9,
      CPCDISK_ERROR_SECTOR_NOT_FOUND = -10,
      CPCDISK_ERROR_DELETED_SECTOR = -11,
      CPCDISK_ERROR_END_OF_CYLINDER = -12,
      CPCDISK_ERROR_READ_FAILED = -13,
      CPCDISK_ERROR_WRITE_FAILED = -14
    } CPCDiskError;
   protected:
    FDCCommandParams  cmdParams;
    // 0: idle
    // 1: command phase
    // 2: execution phase
    // 3: result phase
    uint8_t   fdcState;
    uint8_t   *sectorBuf;
    // ----------------
    void updateStatusRegisters(CPCDiskError errorCode);
   public:
    FDC765();
    virtual ~FDC765();
    virtual void reset();
    virtual void setMotorState(bool isEnabled);
    virtual uint8_t readMainStatusRegister() const;
    virtual uint8_t readDataRegister();
    virtual void writeDataRegister(uint8_t n);
   protected:
    virtual CPCDiskError readSector(uint8_t& statusRegister1,
                                    uint8_t& statusRegister2) = 0;
    virtual CPCDiskError writeSector(uint8_t& statusRegister1,
                                     uint8_t& statusRegister2) = 0;
    virtual void stepIn(int nSteps = 1) = 0;
    virtual void stepOut(int nSteps = 1) = 0;
    virtual bool haveDisk() const = 0;
    virtual bool getIsTrack0() const = 0;
    virtual bool getIsWriteProtected() const = 0;
  };

}       // namespace CPC464

#endif  // EP128EMU_FDC765_HPP

