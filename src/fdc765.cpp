
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
        | (uint8_t(motorSpeed >= 100 && haveDisk()) << 5)
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

  void FDC765::processFDCCommand()
  {

  }

  void FDC765::setMotorState_(bool isEnabled)
  {
    updateDrives();
    motorOn = isEnabled;
    motorStateChanging = true;
  }

  uint32_t FDC765::updateDrives_()
  {
    uint32_t  dTime = (timeCounter - prvTimeCounter) & uint32_t(0xFFFFFFFFUL);
    prvTimeCounter = timeCounter;
    if (dTime < 1U)
      return 0U;
    uint32_t  angleChange = (motorOn ? dTime : uint32_t(0));
    if (motorStateChanging) {
      if (motorOn) {
        // motor spinning up
        if (dTime >= 102U || (uint32_t(motorSpeed) + dTime) >= 102U) {
          angleChange = angleChange - (uint32_t(102 - motorSpeed) >> 1);
          motorSpeed = 102;
          motorStateChanging = false;
        }
        else {
          angleChange = angleChange - (dTime >> 1);
          motorSpeed = motorSpeed + uint8_t(dTime);
        }
      }
      else {
        // motor spinning down
        if (dTime >= uint32_t(motorSpeed)) {
          angleChange = angleChange + (uint32_t(motorSpeed) >> 1);
          motorSpeed = 0;
          motorStateChanging = false;
        }
        else {
          angleChange = angleChange + (dTime >> 1);
          motorSpeed = motorSpeed - uint8_t(dTime);
        }
      }
    }
    for (int i = 0; i < 4; i++) {
      rotationAngles[i] =
          uint8_t((uint32_t(rotationAngles[i]) + angleChange) % 100U);
    }
    return dTime;
  }

  // --------------------------------------------------------------------------

  FDC765::FDC765()
    : timeCounter(0U),
      prvTimeCounter(0U),
      totalDataBytes(0U),
      dataBytesRemaining(0U),
      fdcState(0),
      dataDirectionIsRead(false),
      motorOn(false),
      motorSpeed(0),
      motorStateChanging(false),
      sectorBuf((uint8_t *) 0)
  {
    std::memset(&cmdParams, 0x00, sizeof(FDCCommandParams));
    for (int i = 0; i < 4; i++) {
      physicalCylinders[i] = 0;
      rotationAngles[i] = 0;
    }
    sectorBuf = new uint8_t[0x4000];    // maximum sector size (N=7) is 16K
    std::memset(&(sectorBuf[0]), 0x00, 0x4000);
  }

  FDC765::~FDC765()
  {
    delete[] sectorBuf;
  }

  void FDC765::reset()
  {
    updateDrives();
    std::memset(&cmdParams, 0x00, sizeof(FDCCommandParams));
    totalDataBytes = 0U;
    dataBytesRemaining = 0U;
    fdcState = 0;
    dataDirectionIsRead = false;
    if (motorOn) {
      motorOn = false;
      motorStateChanging = true;
    }
  }

  uint8_t FDC765::readMainStatusRegister()
  {
    updateDrives();
    return ((uint8_t(fdcState == 2) << 5) | (uint8_t(dataDirectionIsRead) << 6)
            | 0x80);
  }

  uint8_t FDC765::readDataRegister()
  {
    updateDrives();
    if (!dataDirectionIsRead)           // wrong data direction
      return 0xFF;
    if (dataBytesRemaining > 0U) {
      dataBytesRemaining--;
      return sectorBuf[totalDataBytes - (dataBytesRemaining + 1U)];
    }
    return 0xFF;
  }

  void FDC765::writeDataRegister(uint8_t n)
  {
    updateDrives();
    if (dataDirectionIsRead)            // wrong data direction
      return;
    if (fdcState == 0) {                // start new command
      cmdParams.commandCode = n;
      bool    invalidCommand = false;
      switch (n & 0x1F) {
      case 0x02:                        // READ TRACK
        if (n & 0x80) {
          invalidCommand = true;
          break;
        }
        totalDataBytes = 8U;
        break;
      case 0x03:                        // SPECIFY PARAMETERS
      case 0x0F:                        // SEEK
        if (n & 0xE0) {
          invalidCommand = true;
          break;
        }
        totalDataBytes = 2U;
        break;
      case 0x04:                        // SENSE DRIVE STATE
      case 0x07:                        // RECALIBRATE / SEEK TO TRACK 0
        if (n & 0xE0) {
          invalidCommand = true;
          break;
        }
        totalDataBytes = 1U;
        break;
      case 0x05:                        // WRITE SECTOR(S)
      case 0x09:                        // WRITE DELETED SECTOR(S)
        if (n & 0x20) {
          invalidCommand = true;
          break;
        }
        totalDataBytes = 8U;
        break;
      case 0x06:                        // READ SECTOR(S)
      case 0x0C:                        // READ DELETED SECTOR(S)
      case 0x11:                        // SCAN EQUAL
      case 0x19:                        // SCAN LESS THAN OR EQUAL
      case 0x1D:                        // SCAN GREATER THAN OR EQUAL
        totalDataBytes = 8U;
        break;
      case 0x08:                        // SENSE INTERRUPT STATE
        if (n & 0xE0) {
          invalidCommand = true;
          break;
        }
        totalDataBytes = 0U;
        break;
      case 0x0A:                        // READ SECTOR ID
        if (n & 0xA0) {
          invalidCommand = true;
          break;
        }
        totalDataBytes = 1U;
        break;
      case 0x0D:                        // FORMAT TRACK
        if (n & 0xA0) {
          invalidCommand = true;
          break;
        }
        totalDataBytes = 5U;
        break;
      default:                          // anything else is invalid command
        invalidCommand = true;
        break;
      }
      if (invalidCommand) {
        updateStatusRegisters(CPCDISK_ERROR_INVALID_COMMAND);
        sectorBuf[0] = cmdParams.statusRegister0;
        totalDataBytes = 1U;
        dataBytesRemaining = 1U;
        fdcState = 3;
        dataDirectionIsRead = true;
        return;
      }
      dataBytesRemaining = totalDataBytes;
      if (totalDataBytes > 0U)
        fdcState = 1;
      else
        processFDCCommand();
    }
    else if (fdcState == 1) {           // store command parameters
      if (dataBytesRemaining > 0U) {
        uint32_t  paramIndex = totalDataBytes - dataBytesRemaining;
        uint8_t   cmdCode = cmdParams.commandCode & 0x1F;
        switch (paramIndex) {
        case 0U:
          if (cmdCode == 0x03) {        // SPECIFY PARAMETERS

          }
          else {                        // unit, head for any other command
            cmdParams.unitNumber = n & 0x03;
            cmdParams.physicalSide = (n & 0x04) >> 2;
          }
          break;
        case 1U:
          switch (cmdCode) {
          case 0x03:                    // SPECIFY PARAMETERS
            break;
          case 0x0D:                    // FORMAT TRACK
            cmdParams.sectorSizeCode = n;
            break;
          case 0x0F:                    // SEEK
            physicalCylinders[cmdParams.unitNumber] = n;
            break;
          default:                      // track ID for any other command
            cmdParams.cylinderID = n;
            break;
          }
          break;
        case 2U:
          if (cmdCode == 0x0D)          // FORMAT TRACK
            cmdParams.sectorCnt = n;
          else                          // side ID for any other command
            cmdParams.headID = n;
          break;
        case 3U:
          if (cmdCode == 0x0D)          // FORMAT TRACK
            cmdParams.gapLen = n;
          else                          // sector ID for any other command
            cmdParams.sectorID = n;
          break;
        case 4U:
          if (cmdCode == 0x0D)          // FORMAT TRACK
            cmdParams.fillerByte = n;
          else                          // sector size for any other command
            cmdParams.sectorSizeCode = n;
          break;
        case 5U:
          if (cmdCode == 0x02)          // READ TRACK
            cmdParams.sectorCnt = n;
          else                          // last sector ID for any other command
            cmdParams.lastSectorID = n;
          break;
        case 6U:
          cmdParams.gapLen = n;
          break;
        case 7U:
          cmdParams.sectorDataLength = n;
          break;
        }
        dataBytesRemaining--;
      }
      if (dataBytesRemaining < 1U)
        processFDCCommand();
    }
    else if (fdcState == 2) {           // write data

    }
  }

  uint8_t FDC765::readDataRegisterDebug()
  {
    updateDrives();
    if (dataDirectionIsRead && dataBytesRemaining > 0U)
      return sectorBuf[totalDataBytes - dataBytesRemaining];
    return 0xFF;
  }

  uint8_t FDC765::debugRead(uint16_t addr)
  {
    updateDrives();
    switch (addr & 0x001F) {
    case 0x0000:
      return fdcState;
    case 0x0001:
      return cmdParams.commandCode;
    }
    return 0xFF;
  }

}       // namespace CPC464

