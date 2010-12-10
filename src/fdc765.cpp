
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

  void FDC765::startResultPhase(FDC765::CPCDiskError errorCode)
  {
    totalDataBytes = 0U;
    if (fdcState != 0) {
      if (errorCode == CPCDISK_ERROR_INVALID_COMMAND) {
        sectorBuf[0] = 0x80;    // ST0
        totalDataBytes = 1U;
      }
      else {
        switch (cmdParams.commandCode & 0x1F) {
        case 0x04:                      // SENSE DRIVE STATE
          sectorBuf[0] =        // ST3
              cmdParams.unitNumber | (cmdParams.physicalSide << 2)
              | (uint8_t(getIsTrack0(cmdParams.unitNumber)) << 4)
              | (uint8_t(driveReady[cmdParams.unitNumber]) << 5)
              | (uint8_t(getIsWriteProtected(cmdParams.unitNumber)) << 6);
          totalDataBytes = 1U;
          break;
        case 0x08:                      // SENSE INTERRUPT STATE
          {
            uint8_t statusRegister0 = 0x80;     // IC if no interrupt occured
            for (int i = 0; i < 4; i++) {
              if (interruptStatus[i]) {
                if (interruptStatus[i] & 0x20) {
                  statusRegister0 =
                      uint8_t(i | (interruptStatus[i] & 0x3C)
                              | ((interruptStatus[i] & 0x08) << 3));
                  interruptStatus[i] = interruptStatus[i] & 0xC0;
                }
                else {
                  interruptStatus[i] = 0x00;
                  statusRegister0 =
                      uint8_t(i | (int(!driveReady[i]) << 3) | 0xC0);
                }
                break;
              }
            }
            sectorBuf[0] = statusRegister0;
            if (statusRegister0 == 0x80) {
              totalDataBytes = 1U;
            }
            else {
              sectorBuf[1] = presentCylinderNumbers[statusRegister0 & 0x03];
              totalDataBytes = 2U;
            }
          }
          break;
        }
      }
    }
    dataBytesRemaining = totalDataBytes;
    if (totalDataBytes > 0U) {
      fdcState = 3;
      dataDirectionIsRead = true;
    }
    else {
      fdcState = 0;
      dataDirectionIsRead = false;
    }
    dataIsNotReady = false;
  }

  void FDC765::processFDCCommand()
  {
    switch (cmdParams.commandCode & 0x1F) {
    case 0x02:                          // READ TRACK
    case 0x05:                          // WRITE SECTOR(S)
    case 0x06:                          // READ SECTOR(S)
    case 0x09:                          // WRITE DELETED SECTOR(S)
    case 0x0A:                          // READ SECTOR ID
    case 0x0C:                          // READ DELETED SECTOR(S)
    case 0x0D:                          // FORMAT TRACK
    case 0x11:                          // SCAN EQUAL
    case 0x19:                          // SCAN LESS THAN OR EQUAL
    case 0x1D:                          // SCAN GREATER THAN OR EQUAL

      break;
    case 0x03:                          // SPECIFY PARAMETERS
      fdcState = 0;             // no execution or result phase
      break;
    case 0x04:                          // SENSE DRIVE STATE
      startResultPhase(CPCDISK_NO_ERROR);
      break;
    case 0x07:                          // RECALIBRATE / SEEK TO TRACK 0
      presentCylinderNumbers[cmdParams.unitNumber] = 0;
      newCylinderNumbers[cmdParams.unitNumber] = 0;
      if (!driveReady[cmdParams.unitNumber]) {
        // drive not ready
        seekComplete(cmdParams.unitNumber, true, true);
      }
      else if (getIsTrack0(cmdParams.unitNumber)) {
        seekComplete(cmdParams.unitNumber, true, false);
      }
      else {
        recalibrateSteps[cmdParams.unitNumber] = 77;
        seekTimers[cmdParams.unitNumber] = stepRate;
      }
      fdcState = 0;
      break;
    case 0x08:                          // SENSE INTERRUPT STATE
      startResultPhase(CPCDISK_NO_ERROR);
      break;
    case 0x0F:                          // SEEK
      recalibrateSteps[cmdParams.unitNumber] = 0;
      if (!driveReady[cmdParams.unitNumber]) {
        // drive not ready
        seekComplete(cmdParams.unitNumber, false, true);
      }
      else if (newCylinderNumbers[cmdParams.unitNumber]
               == presentCylinderNumbers[cmdParams.unitNumber]) {
        seekComplete(cmdParams.unitNumber, false, false);
      }
      else {
        seekTimers[cmdParams.unitNumber] = stepRate;
      }
      fdcState = 0;
      break;
    default:                            // INVALID COMMAND
      startResultPhase(CPCDISK_ERROR_INVALID_COMMAND);
      break;
    }
  }

  void FDC765::setMotorState_(bool isEnabled)
  {
    updateDrives();
    motorOn = isEnabled;
    motorStateChanging = true;
  }

  void FDC765::updateDriveReadyStatus()
  {
    if (motorSpeed < 100) {
      for (int i = 0; i < 4; i++) {
        if (driveReady[i]) {
          driveReady[i] = false;
          interruptStatus[i] = interruptStatus[i] | 0xC0;
          if (newCylinderNumbers[i] != presentCylinderNumbers[i] ||
              recalibrateSteps[i] != 0) {
            // abort seek command
            seekComplete(i, bool(recalibrateSteps[i]), true);
          }
        }
      }
    }
    else {
      for (int i = 0; i < 4; i++) {
        bool    newState = haveDisk(i);
        if (newState != driveReady[i]) {
          driveReady[i] = newState;
          interruptStatus[i] = interruptStatus[i] | 0xC0;
          if (newCylinderNumbers[i] != presentCylinderNumbers[i] ||
              recalibrateSteps[i] != 0) {
            // abort seek command
            seekComplete(i, bool(recalibrateSteps[i]), true);
          }
        }
      }
    }
  }

  void FDC765::seekComplete(int driveNum, bool isRecalibrate, bool isNotReady)
  {
    driveNum = driveNum & 3;
    newCylinderNumbers[driveNum] = presentCylinderNumbers[driveNum];
    recalibrateSteps[driveNum] = 0;
    seekTimers[driveNum] = 0;
    interruptStatus[driveNum] =
        (interruptStatus[driveNum] & 0xC0) | 0x20
        | (uint8_t(isRecalibrate && !(isNotReady | getIsTrack0(driveNum))) << 4)
        | (uint8_t(isNotReady) << 3) | (cmdParams.physicalSide << 2);
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
        updateDriveReadyStatus();
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
        updateDriveReadyStatus();
      }
    }
    if (headUnloadTimer > 0) {
      headLoadTimer = 0;
      if (dTime >= uint32_t(headUnloadTimer))
        headUnloadTimer = 0;
      else
        headUnloadTimer = headUnloadTimer - uint8_t(dTime);
    }
    else if (headLoadTimer > 0) {
      if (dTime >= uint32_t(headLoadTimer))
        headLoadTimer = 0;
      else
        headLoadTimer = headLoadTimer - uint8_t(dTime);
    }
    for (int i = 0; i < 4; i++) {
      rotationAngles[i] =
          uint8_t((uint32_t(rotationAngles[i]) + angleChange) % 100U);
      if (recalibrateSteps[i] > 0) {
        // RECALIBRATE / SEEK TO TRACK 0
        presentCylinderNumbers[i] = 0;
        newCylinderNumbers[i] = 0;
        if (dTime < uint32_t(seekTimers[i])) {
          seekTimers[i] = seekTimers[i] - uint8_t(dTime);
        }
        else if (stepRate < 1) {
          // no seek time emulation
          stepOut(i, recalibrateSteps[i]);
          seekComplete(i, true, false);
        }
        else {
          int     stepCnt =
              int((dTime - uint32_t(seekTimers[i])) / uint32_t(stepRate)) + 1;
          seekTimers[i] =
              uint8_t((dTime - uint32_t(seekTimers[i])) % uint32_t(stepRate));
          if (stepCnt > int(recalibrateSteps[i]))
            stepCnt = recalibrateSteps[i];
          stepOut(i, stepCnt);
          recalibrateSteps[i] = recalibrateSteps[i] - uint8_t(stepCnt);
          if (recalibrateSteps[i] < 1 || getIsTrack0(i))
            seekComplete(i, true, false);
        }
      }
      else if (newCylinderNumbers[i] != presentCylinderNumbers[i]) {
        // SEEK
        if (dTime < uint32_t(seekTimers[i])) {
          seekTimers[i] = seekTimers[i] - uint8_t(dTime);
        }
        else if (stepRate < 1) {
          // no seek time emulation
          int     stepCnt =
              int(newCylinderNumbers[i]) - int(presentCylinderNumbers[i]);
          stepIn(i, stepCnt);
          presentCylinderNumbers[i] = newCylinderNumbers[i];
          seekComplete(i, false, false);
        }
        else {
          int     stepCnt =
              int((dTime - uint32_t(seekTimers[i])) / uint32_t(stepRate)) + 1;
          seekTimers[i] =
              uint8_t((dTime - uint32_t(seekTimers[i])) % uint32_t(stepRate));
          int     maxStepCnt =
              int(newCylinderNumbers[i]) - int(presentCylinderNumbers[i]);
          if (stepCnt >= std::abs(maxStepCnt))
            stepCnt = maxStepCnt;
          else if (maxStepCnt < 0)
            stepCnt = -stepCnt;
          stepIn(stepCnt);
          presentCylinderNumbers[i] =
              uint8_t(int(presentCylinderNumbers[i]) + stepCnt);
          if (presentCylinderNumbers[i] == newCylinderNumbers[i])
            seekComplete(i, false, false);
        }
      }
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
      dataIsNotReady(false),
      motorOn(false),
      motorSpeed(0),
      motorStateChanging(false),
      stepRate(6),
      headUnloadTime(16),
      headLoadTime(2),
      headUnloadTimer(0),
      headLoadTimer(0),
      sectorBuf((uint8_t *) 0)
  {
    std::memset(&cmdParams, 0x00, sizeof(FDCCommandParams));
    for (int i = 0; i < 4; i++) {
      presentCylinderNumbers[i] = 0;
      newCylinderNumbers[i] = 0;
      recalibrateSteps[i] = 0;
      seekTimers[i] = 0;
      driveReady[i] = false;
      interruptStatus[i] = 0x00;
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
    dataIsNotReady = false;
    if (motorOn) {
      motorOn = false;
      motorStateChanging = true;
    }
    stepRate = 6;
    headUnloadTime = 16;
    headLoadTime = 2;
    headUnloadTimer = 0;
    headLoadTimer = 0;
    for (int i = 0; i < 4; i++) {
      newCylinderNumbers[i] = presentCylinderNumbers[i];
      recalibrateSteps[i] = 0;
      seekTimers[i] = 0;
      interruptStatus[i] = 0x00;
    }
  }

  uint8_t FDC765::readMainStatusRegister()
  {
    updateDrives();
    uint8_t retval = (uint8_t(fdcState != 0) << 4)
                     | (uint8_t(fdcState == 2) << 5)
                     | (uint8_t(dataDirectionIsRead) << 6)
                     | (uint8_t(!dataIsNotReady) << 7);
    for (int i = 0; i < 4; i++) {
      if (newCylinderNumbers[i] != presentCylinderNumbers[i] ||
          recalibrateSteps[i] != 0) {
        // drive is busy in seek mode
        retval = retval | uint8_t(0x01 << i);
      }
    }
    return retval;
  }

  uint8_t FDC765::readDataRegister()
  {
    updateDrives();
    uint8_t retval = 0xFF;
    if (dataDirectionIsRead && !dataIsNotReady) {
      if (dataBytesRemaining > 0U) {
        retval = sectorBuf[totalDataBytes - dataBytesRemaining];
        dataBytesRemaining--;
      }
      if (dataBytesRemaining < 1U) {
        if (fdcState == 2) {            // execution phase of R/W command

        }
        else {                          // end of result phase
          fdcState = 0;
          dataDirectionIsRead = false;
        }
      }
    }
    return retval;
  }

  void FDC765::writeDataRegister(uint8_t n)
  {
    updateDrives();
    if (dataDirectionIsRead || dataIsNotReady)  // wrong data direction
      return;                                   // or not ready to process data
    if (fdcState == 0) {                // start new command
      fdcState = 1;
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
        startResultPhase(CPCDISK_ERROR_INVALID_COMMAND);
        return;
      }
      dataBytesRemaining = totalDataBytes;
      if (totalDataBytes < 1U)
        processFDCCommand();
    }
    else if (fdcState == 1) {           // store command parameters
      if (dataBytesRemaining > 0U) {
        uint32_t  paramIndex = totalDataBytes - dataBytesRemaining;
        uint8_t   cmdCode = cmdParams.commandCode & 0x1F;
        switch (paramIndex) {
        case 0U:
          if (cmdCode == 0x03) {        // SPECIFY PARAMETERS
            // set step rate and head unload time in 2ms and 32ms units
            stepRate = uint8_t(16 - ((n >> 4) & 0x0F));
            headUnloadTime = uint8_t((n & 0x0F) << 4);
            if (headUnloadTime == 0)
              headUnloadTime = 0xFF;
          }
          else {                        // unit, head for any other command
            cmdParams.unitNumber = n & 0x03;
            cmdParams.physicalSide = (n & 0x04) >> 2;
          }
          break;
        case 1U:
          switch (cmdCode) {
          case 0x03:                    // SPECIFY PARAMETERS
            // set head load time in 4ms units
            // FIXME: the DMA bit is ignored
            headLoadTime = n & 0xFE;
            if (headLoadTime == 0)
              headLoadTime = 0xFF;
            break;
          case 0x0D:                    // FORMAT TRACK
            cmdParams.sectorSizeCode = n;
            break;
          case 0x0F:                    // SEEK
            newCylinderNumbers[cmdParams.unitNumber] = n;
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
    if (dataDirectionIsRead && dataBytesRemaining > 0U && !dataIsNotReady)
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
    case 0x0002:
      return uint8_t(motorOn);
    case 0x0003:
      return cmdParams.physicalSide;
    case 0x0004:
      return uint8_t((totalDataBytes - dataBytesRemaining) & 0xFFU);
    case 0x0005:
      return uint8_t(((totalDataBytes - dataBytesRemaining) >> 8) & 0xFFU);
    case 0x0006:
      return uint8_t(totalDataBytes & 0xFFU);
    case 0x0007:
      return uint8_t((totalDataBytes >> 8) & 0xFFU);
    case 0x0018:
    case 0x0019:
    case 0x001A:
    case 0x001B:
      return (uint8_t(addr & 0x0003) | (cmdParams.physicalSide << 2)
              | (uint8_t(getIsTrack0(addr & 0x0003)) << 4)
              | (uint8_t(driveReady[addr & 0x0003]) << 5)
              | (uint8_t(getIsWriteProtected(addr & 0x0003)) << 6));
    case 0x001C:
    case 0x001D:
    case 0x001E:
    case 0x001F:
      return presentCylinderNumbers[addr & 0x0003];
    }
    return 0xFF;
  }

}       // namespace CPC464

