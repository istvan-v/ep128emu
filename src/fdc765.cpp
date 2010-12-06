
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
#include "fdc765.hpp"

namespace CPC464 {

  void FDC765::updateStatusRegisters(CPCDiskError errorCode)
  {
    cmdParams.statusRegister0 =
        cmdParams.unitNumber | (cmdParams.physicalSide << 2) | 0x40;
    cmdParams.statusRegister1 = 0x00;
    cmdParams.statusRegister2 = 0x00;
    cmdParams.statusRegister3 =
        cmdParams.unitNumber | (cmdParams.physicalSide << 2)
        | (uint8_t(getIsTrack0()) << 4)
        | (uint8_t(cmdParams.motorOn && haveDisk()) << 5)
        | (uint8_t(getIsWriteProtected()) << 6);
    switch (errorCode) {
    case CPCDISK_NO_ERROR:
      cmdParams.statusRegister0 = cmdParams.statusRegister0 & 0x3F;
      break;
    case CPCDISK_ERROR_INVALID_COMMAND:
      cmdParams.statusRegister0 = (cmdParams.statusRegister0 & 0x3F) | 0x80;
      break;
    case CPCDISK_ERROR_NO_DISK:
    case CPCDISK_ERROR_NOT_READY:
      cmdParams.statusRegister0 = cmdParams.statusRegister0 | 0x08;
      break;
    case CPCDISK_ERROR_WRITE_PROTECTED:
      cmdParams.statusRegister1 = cmdParams.statusRegister1 | 0x02;
      break;
    case CPCDISK_ERROR_INVALID_TRACK:
    case CPCDISK_ERROR_INVALID_SIDE:
      cmdParams.statusRegister1 = cmdParams.statusRegister1 | 0x05;
      break;
    case CPCDISK_ERROR_BAD_CYLINDER:
      cmdParams.statusRegister1 = cmdParams.statusRegister1 | 0x04;
      cmdParams.statusRegister2 = cmdParams.statusRegister2 | 0x02;
      break;
    case CPCDISK_ERROR_WRONG_CYLINDER:
      cmdParams.statusRegister1 = cmdParams.statusRegister1 | 0x04;
      cmdParams.statusRegister2 = cmdParams.statusRegister2 | 0x10;
      break;
    case CPCDISK_ERROR_SECTOR_NOT_FOUND:
      cmdParams.statusRegister1 = cmdParams.statusRegister1 | 0x05;
      break;
    case CPCDISK_ERROR_DELETED_SECTOR:
      cmdParams.statusRegister2 = cmdParams.statusRegister2 | 0x40;
      break;
    case CPCDISK_ERROR_END_OF_CYLINDER:
      cmdParams.statusRegister1 = cmdParams.statusRegister1 | 0x80;
      break;
    case CPCDISK_ERROR_READ_FAILED:
    case CPCDISK_ERROR_WRITE_FAILED:
      cmdParams.statusRegister1 = cmdParams.statusRegister1 | 0x20;
      break;
    default:
      break;
    }
  }

  // --------------------------------------------------------------------------

  FDC765::FDC765()
    : fdcState(0),
      sectorBuf((uint8_t *) 0)
  {
    std::memset(&cmdParams, 0x00, sizeof(FDCCommandParams));
    sectorBuf = new uint8_t[0x4000];    // maximum sector size (N=7) is 16K
    std::memset(&(sectorBuf[0]), 0x00, 0x4000);
  }

  FDC765::~FDC765()
  {
    delete[] sectorBuf;
  }

  void FDC765::reset()
  {
    std::memset(&cmdParams, 0x00, sizeof(FDCCommandParams));
    fdcState = 0;
  }

  void FDC765::setMotorState(bool isEnabled)
  {
    cmdParams.motorOn = isEnabled;
  }

  uint8_t FDC765::readMainStatusRegister() const
  {

  }

  uint8_t FDC765::readDataRegister()
  {

  }

  void FDC765::writeDataRegister(uint8_t n)
  {

  }

}       // namespace CPC464

