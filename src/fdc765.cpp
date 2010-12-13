
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
      if (fdcState == 2) {
        // unload head after read/write command
        headUnloadTimer = headUnloadTime;
        headLoadTimer = 0;
      }
      if (errorCode == CPCDISK_ERROR_INVALID_COMMAND) {
        sectorBuf[0] = 0x80;    // ST0
        totalDataBytes = 1U;
      }
      else {
        switch (cmdParams.commandCode & 0x1F) {
        case 0x02:                      // READ TRACK
        case 0x05:                      // WRITE SECTOR(S)
        case 0x06:                      // READ SECTOR(S)
        case 0x09:                      // WRITE DELETED SECTOR(S)
        case 0x0A:                      // READ SECTOR ID
        case 0x0C:                      // READ DELETED SECTOR(S)
        case 0x0D:                      // FORMAT TRACK
        case 0x11:                      // SCAN EQUAL
        case 0x19:                      // SCAN LESS THAN OR EQUAL
        case 0x1D:                      // SCAN GREATER THAN OR EQUAL
          {
            uint8_t statusRegister0 =
                cmdParams.unitNumber | (cmdParams.physicalSide << 2);
            if (errorCode != CPCDISK_NO_ERROR) {
              statusRegister0 = statusRegister0 | 0x40;
              if (errorCode == CPCDISK_ERROR_NOT_READY)
                statusRegister0 = statusRegister0 | 0x08;
              else if (errorCode == CPCDISK_ERROR_WRITE_PROTECTED)
                statusRegister1 = statusRegister1 | 0x02;
              else if (errorCode == CPCDISK_ERROR_SECTOR_NOT_FOUND)
                statusRegister1 = statusRegister1 | 0x05;
            }
            sectorBuf[0] = statusRegister0;
            sectorBuf[1] = statusRegister1;
            sectorBuf[2] = statusRegister2;
            sectorBuf[3] = cmdParams.cylinderID;
            sectorBuf[4] = cmdParams.headID;
            sectorBuf[5] = cmdParams.sectorID;
            sectorBuf[6] = cmdParams.sectorSizeCode;
            totalDataBytes = 7U;
          }
          break;
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
      interruptStatus[cmdParams.unitNumber] =
          interruptStatus[cmdParams.unitNumber] & 0x3C; // clear /RDY change
      totalDataBytes = 0U;
      dataBytesRemaining = 0U;
      indexPulsesRemaining = 0;
      executionPhaseFlags = 0x00;
      statusRegister1 = 0x00;
      statusRegister2 = 0x00;
      sectorDelay = -1;
      physicalSector = 0;
      if (!driveReady[cmdParams.unitNumber]) {
        startResultPhase(CPCDISK_ERROR_NOT_READY);
        break;
      }
      if ((cmdParams.commandCode & 0x13) == 0x01) {     // write commands:
        // FIXME: FORMAT TRACK is not implemented
        if (getIsWriteProtected(cmdParams.unitNumber) ||
            (cmdParams.commandCode & 0x1F) == 0x0D) {
          startResultPhase(CPCDISK_ERROR_WRITE_PROTECTED);
          break;
        }
      }
      // start execution phase
      fdcState = 2;
      dataDirectionIsRead = !(cmdParams.commandCode & 0x01);
      dataIsNotReady = true;
      headLoadTimer = (headUnloadTimer > 0 ? uint8_t(0) : headLoadTime);
      headUnloadTimer = 0;
      indexPulsesRemaining = 2;
      if ((cmdParams.commandCode & 0x80) != 0 &&
          (cmdParams.commandCode & 0x1F) != 0x02 &&
          (cmdParams.commandCode & 0x1F) != 0x0D) {
        // set multi-track flag, unless executing READ TRACK or FORMAT TRACK
        executionPhaseFlags = executionPhaseFlags | 0x80;
      }
      break;
    case 0x03:                          // SPECIFY PARAMETERS
      fdcState = 0;             // no execution or result phase
      break;
    case 0x04:                          // SENSE DRIVE STATE
      startResultPhase(CPCDISK_NO_ERROR);
      break;
    case 0x07:                          // RECALIBRATE / SEEK TO TRACK 0
      interruptStatus[cmdParams.unitNumber] =
          interruptStatus[cmdParams.unitNumber] & 0x3C; // clear /RDY change
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
      interruptStatus[cmdParams.unitNumber] =
          interruptStatus[cmdParams.unitNumber] & 0x3C; // clear /RDY change
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

  void FDC765::updateDriveReadyStatus()
  {
    bool    motorOn_ = (motorSpeed >= 100);
    for (int i = 0; i < 4; i++) {
      bool    newState = (motorOn_ && haveDisk(i));
      if (newState != driveReady[i]) {
        driveReady[i] = newState;
        interruptStatus[i] = interruptStatus[i] | 0xC0;
        if (fdcState == 2 && indexPulsesRemaining > 0 &&
            i == int(cmdParams.unitNumber)) {
          // abort read/write command in sector search mode
          startResultPhase(CPCDISK_ERROR_NOT_READY);
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

  bool FDC765::incrementSectorID()
  {
    if (((statusRegister1 & 0xB7) | (statusRegister2 & 0x7F)) != 0) {
      // error ?
      uint16_t  errorFlags = (uint16_t(statusRegister1 & 0xB7) << 8)
                             | uint16_t(statusRegister2 & 0x61);
      uint16_t  successFlags = 0x0000;
      switch (cmdParams.commandCode & 0x1F) {
      case 0x02:                        // READ TRACK
        errorFlags = errorFlags & 0x9100;
        break;
      case 0x05:                        // WRITE SECTOR(S)
      case 0x09:                        // WRITE DELETED SECTOR(S)
        errorFlags = errorFlags & 0xB700;
      case 0x06:                        // READ SECTOR(S)
      case 0x0C:                        // READ DELETED SECTOR(S)
        errorFlags = errorFlags & 0xB561;
      case 0x0D:                        // FORMAT TRACK
        errorFlags = errorFlags & 0x3000;
      case 0x11:                        // SCAN EQUAL
      case 0x19:                        // SCAN LESS THAN OR EQUAL
      case 0x1D:                        // SCAN GREATER THAN OR EQUAL
        errorFlags = errorFlags & 0xB561;
        successFlags = uint16_t((statusRegister2 ^ 0x04) & 0x0C);
        break;
      }
      if (errorFlags) {
        statusRegister2 = statusRegister2 & 0x73;
        startResultPhase(CPCDISK_ERROR_UNKNOWN);
        return false;
      }
      statusRegister2 = statusRegister2 & 0x7B;
      if (successFlags) {
        startResultPhase(CPCDISK_NO_ERROR);
        return false;
      }
    }
    if (cmdParams.sectorID == cmdParams.lastSectorID) {
      cmdParams.sectorID = 1;
      if (cmdParams.commandCode & 0x80) {
        // MT bit is set: toggle side
        cmdParams.physicalSide = (~cmdParams.physicalSide) & 0x01;
        cmdParams.headID = cmdParams.headID ^ 0x01;
        if (!(cmdParams.headID & 0x01))
          cmdParams.cylinderID = (cmdParams.cylinderID + 1) & 0xFF;
      }
      if ((executionPhaseFlags & 0x80) == 0 ||
          (cmdParams.headID & 0x01) == 0) {
        // if multi-track mode is not enabled, or already at end of side 1,
        // terminate command
        statusRegister1 = statusRegister1 | 0x80;       // end of cylinder
        if (cmdParams.commandCode & 0x10)
          statusRegister2 = statusRegister2 | 0x04;     // scan not satisfied
        startResultPhase(CPCDISK_ERROR_UNKNOWN);
        return false;
      }
    }
    else {
      cmdParams.sectorID = (cmdParams.sectorID + 1) & 0xFF;
    }
    // sector search mode
    dataIsNotReady = true;
    indexPulsesRemaining = 2;
    executionPhaseFlags = executionPhaseFlags & 0xFE;
    sectorDelay = -1;
    physicalSector = 0;
    return true;
  }

  EP128EMU_REGPARM1 void FDC765::runExecutionPhase()
  {
    if (indexPulsesRemaining > 0) {
      // searching for a sector
      if (!driveReady[cmdParams.unitNumber]) {
        // drive not ready, abort command
        startResultPhase(CPCDISK_ERROR_NOT_READY);
        return;
      }
      if (headLoadTimer > 0)    // need to wait until the head is loaded
        return;
      // gap 4a, sync, index address mark, gap 1, sync
      if (rotationAngles[cmdParams.unitNumber]
          == (6250 - (80 + 12 + 4 + 50 + 12))) {
        if (--indexPulsesRemaining < 1) {
          // sector not found after two index pulses
          startResultPhase(CPCDISK_ERROR_SECTOR_NOT_FOUND);
          return;
        }
      }
      if (--sectorDelay < 0) {
        int     nextSector =
            getPhysicalSector(rotationAngles[cmdParams.unitNumber], 6250);
        if (nextSector < 0) {
          // error: no sector is found
          startResultPhase(CPCDISK_ERROR_SECTOR_NOT_FOUND);
          return;
        }
        physicalSector = uint8_t(nextSector);
        sectorDelay = getPhysicalSectorPos(nextSector, 6250)
                      - rotationAngles[cmdParams.unitNumber];
        // ID address mark, ID, ID CRC
        sectorDelay = (sectorDelay >= 0 ? (sectorDelay + 4 + 4 + 2)
                                          : (sectorDelay + 6250 + 4 + 4 + 2));
      }
      if (sectorDelay > 0) {
        // head position is not yet at the ID address mark of the next sector
        return;
      }
      // found a sector, check its ID
      FDCSectorID sectorID;
      if (!getPhysicalSectorID(sectorID, physicalSector)) {
        startResultPhase(CPCDISK_ERROR_SECTOR_NOT_FOUND);
        return;
      }
      uint8_t cmdCode = cmdParams.commandCode & 0x1F;
      switch (cmdCode) {
      case 0x02:                        // READ TRACK
        if (sectorID.cylinderID != cmdParams.cylinderID) {
          statusRegister1 = statusRegister1 | 0x04;     // no data
          // wrong cylinder / bad cylinder
          statusRegister2 =
              statusRegister2
              | uint8_t(sectorID.cylinderID == 0xFF ? 0x12 : 0x10);
        }
        if (sectorID.headID != cmdParams.headID ||
            sectorID.sectorID != cmdParams.sectorID ||
            sectorID.sectorSizeCode != cmdParams.sectorSizeCode) {
          // sector ID does not match
          statusRegister1 = statusRegister1 | 0x04;
        }
        break;
        break;
      case 0x0A:                        // READ SECTOR ID
        // command is complete now
        cmdParams.cylinderID = sectorID.cylinderID;
        cmdParams.headID = sectorID.headID;
        cmdParams.sectorID = sectorID.sectorID;
        cmdParams.sectorSizeCode = sectorID.sectorSizeCode;
        statusRegister1 = sectorID.statusRegister1;
        statusRegister2 = sectorID.statusRegister2;
        if (sectorID.statusRegister1 & 0x05)
          startResultPhase(CPCDISK_ERROR_UNKNOWN);
        else
          startResultPhase(CPCDISK_NO_ERROR);
        return;
      default:
        if (sectorID.cylinderID != cmdParams.cylinderID) {
          // wrong cylinder / bad cylinder
          statusRegister2 =
              statusRegister2
              | uint8_t(sectorID.cylinderID == 0xFF ? 0x12 : 0x10);
          return;
        }
        if (sectorID.headID != cmdParams.headID ||
            sectorID.sectorID != cmdParams.sectorID ||
            sectorID.sectorSizeCode != cmdParams.sectorSizeCode) {
          // sector is not found yet
          return;
        }
        break;
      }
      if (((cmdCode == 0x02 || cmdCode == 0x06 || cmdCode >= 0x10) &&
           (sectorID.statusRegister2 & 0x40) != 0) ||
          (cmdCode == 0x0C && (sectorID.statusRegister2 & 0x40) == 0)) {
        // deleted sector flag does not match
        if (cmdParams.commandCode & 0x20) {
          // skip flag is set, ignore sector; search for the next one
          incrementSectorID();
          return;
        }
        else {
          // read sector, but set control mark bit
          statusRegister2 = statusRegister2 | 0x40;
        }
      }
      indexPulsesRemaining = 0;
      // delay until beginning of sector data (gap 2, sync, data address mark)
      sectorDelay = 22 + 12 + 4;
      if (cmdParams.sectorSizeCode == 0x00 && cmdCode < 0x10) {
        totalDataBytes =
            ((uint32_t(cmdParams.sectorDataLength) + 0x7FU) & 0x7FU) + 1U;
      }
      else {
        totalDataBytes = 0x80U << (cmdParams.sectorSizeCode & 0x07);
      }
      dataBytesRemaining = totalDataBytes;
      if ((cmdCode & 0x13) != 0x01) {
        // if read or scan command, then read sector data now
        uint8_t statusRegister1_ = 0x00;
        uint8_t statusRegister2_ = 0x00;
        CPCDiskError  errorCode =
            readSector(physicalSector, statusRegister1_, statusRegister2_);
        if (errorCode != CPCDISK_NO_ERROR) {
          statusRegister1_ = statusRegister1_ | 0x20;   // DE
          statusRegister2_ = statusRegister2_ | 0x20;   // DD
        }
        statusRegister1 = statusRegister1 | (statusRegister1_ & 0xA5);
        statusRegister2 = statusRegister2 | (statusRegister2_ & 0x21);
        if (cmdCode & 0x10)             // initialize scan command
          statusRegister2 = (statusRegister2 & 0x73) | 0x08;
      }
    }
    else if (dataBytesRemaining > 0U) {
      if (sectorDelay > 0) {
        if (--sectorDelay <= 0) {       // sector data has been reached
          dataIsNotReady = false;
          executionPhaseFlags = executionPhaseFlags & 0xFE;
        }
        return;
      }
      // sector data transfer to/from CPU
      if (!(executionPhaseFlags & 0x01))
        statusRegister1 = statusRegister1 | 0x10;       // overrun
      if (--dataBytesRemaining < 1U) {
        // data transfer complete
        if (interruptStatus[cmdParams.unitNumber] >= 0xC0) {
          // if /RDY has changed, abort command
          startResultPhase(CPCDISK_ERROR_NOT_READY);
          return;
        }
        if ((cmdParams.commandCode & 0x13) == 0x01) {   // write commands:
          // write data to disk image
          uint8_t statusRegister1_ = 0x00;
          uint8_t statusRegister2_ = (cmdParams.commandCode & 0x08) << 3;
          CPCDiskError  errorCode =
              writeSector(physicalSector, statusRegister1_, statusRegister2_);
          if (errorCode != CPCDISK_NO_ERROR) {
            statusRegister1_ = statusRegister1_ | 0x20; // DE
            statusRegister2_ = statusRegister2_ | 0x20; // DD
          }
          statusRegister1 = statusRegister1 | (statusRegister1_ & 0xA5);
          statusRegister2 = statusRegister2 | (statusRegister2_ & 0x21);
        }
        incrementSectorID();
      }
      else {
        dataIsNotReady = false;
        executionPhaseFlags = executionPhaseFlags & 0xFE;
      }
    }
    else {
      // NOTE: this should not be reached
      startResultPhase(CPCDISK_ERROR_NOT_READY);
    }
  }

  EP128EMU_REGPARM1 void FDC765::updateDrives()
  {
    if (motorStateChanging) {
      if (motorOn) {
        // motor spinning up
        if (motorSpeed >= 101) {
          motorSpeed = 102;
          motorStateChanging = false;
        }
        else if (++motorSpeed == 100) {
          updateDriveReadyStatus();
        }
      }
      else {
        // motor spinning down
        if (motorSpeed <= 1) {
          motorSpeed = 0;
          motorStateChanging = false;
        }
        else if (--motorSpeed == 99) {
          updateDriveReadyStatus();
        }
      }
    }
    if (headUnloadTimer > 0) {
      headUnloadTimer--;
      headLoadTimer = 0;
    }
    else if (headLoadTimer > 0) {
      headLoadTimer--;
    }
    for (int i = 0; i < 4; i++) {
      if (recalibrateSteps[i] > 0) {
        // RECALIBRATE / SEEK TO TRACK 0
        presentCylinderNumbers[i] = 0;
        newCylinderNumbers[i] = 0;
        if (interruptStatus[i] >= 0xC0) {
          // /RDY has changed: abort seek command
          seekComplete(i, true, true);
        }
        else if (seekTimers[i] > 1) {
          seekTimers[i]--;
        }
        else if (stepRate < 1) {
          // no seek time emulation
          stepOut(i, recalibrateSteps[i]);
          seekComplete(i, true, false);
        }
        else {
          seekTimers[i] = stepRate;
          stepOut(i, 1);
          recalibrateSteps[i]--;
          if (recalibrateSteps[i] < 1 || getIsTrack0(i))
            seekComplete(i, true, false);
        }
      }
      else if (newCylinderNumbers[i] != presentCylinderNumbers[i]) {
        // SEEK
        if (interruptStatus[i] >= 0xC0) {
          // /RDY has changed: abort seek command
          seekComplete(i, false, true);
        }
        else if (seekTimers[i] > 1) {
          seekTimers[i]--;
        }
        else if (stepRate < 1) {
          // no seek time emulation
          stepIn(i,
                 int(newCylinderNumbers[i]) - int(presentCylinderNumbers[i]));
          presentCylinderNumbers[i] = newCylinderNumbers[i];
          seekComplete(i, false, false);
        }
        else {
          seekTimers[i] = stepRate;
          if (newCylinderNumbers[i] > presentCylinderNumbers[i]) {
            stepIn(i, 1);
            presentCylinderNumbers[i]++;
          }
          else {
            stepOut(i, 1);
            presentCylinderNumbers[i]--;
          }
          if (presentCylinderNumbers[i] == newCylinderNumbers[i])
            seekComplete(i, false, false);
        }
      }
    }
    timeCounter2ms = 63;
  }

  // --------------------------------------------------------------------------

  FDC765::FDC765()
    : totalDataBytes(0U),
      dataBytesRemaining(0U),
      fdcState(0),
      dataDirectionIsRead(false),
      dataIsNotReady(false),
      motorOn(false),
      timeCounter2ms(1),
      motorSpeed(0),
      motorStateChanging(false),
      stepRate(6),
      headUnloadTime(16),
      headLoadTime(2),
      headUnloadTimer(0),
      headLoadTimer(0),
      indexPulsesRemaining(0),
      executionPhaseFlags(0x00),
      statusRegister1(0x00),
      statusRegister2(0x00),
      sectorDelay(-1),
      physicalSector(0),
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
    indexPulsesRemaining = 0;
    executionPhaseFlags = 0x00;
    statusRegister1 = 0x00;
    statusRegister2 = 0x00;
    sectorDelay = -1;
    physicalSector = 0;
    for (int i = 0; i < 4; i++) {
      newCylinderNumbers[i] = presentCylinderNumbers[i];
      recalibrateSteps[i] = 0;
      seekTimers[i] = 0;
      interruptStatus[i] = 0x00;
    }
  }

  uint8_t FDC765::readMainStatusRegister() const
  {
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
    uint8_t retval = 0xFF;
    if (dataDirectionIsRead && !dataIsNotReady) {
      if (dataBytesRemaining > 0U) {
        retval = sectorBuf[totalDataBytes - dataBytesRemaining];
        if (fdcState == 2) {            // execution phase of R/W command
          dataIsNotReady = true;
          executionPhaseFlags = executionPhaseFlags | 0x01;
          return retval;
        }
        dataBytesRemaining--;
      }
      if (dataBytesRemaining < 1U) {    // end of result phase
        fdcState = 0;
        dataDirectionIsRead = false;
      }
    }
    return retval;
  }

  void FDC765::writeDataRegister(uint8_t n)
  {
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
          if (cmdCode == 0x02)          // READ TRACK
            cmdParams.sectorID = 1;
          else if (cmdCode == 0x0D)     // FORMAT TRACK
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
    else if (fdcState == 2) {           // write data in execution phase
      if (dataBytesRemaining > 0U) {
        uint8_t *dataPtr = &(sectorBuf[totalDataBytes - dataBytesRemaining]);
        if (cmdParams.commandCode & 0x10) {     // scan commands:
          if (n != (*dataPtr) && n != 0xFF && (*dataPtr) != 0xFF) {
            // clear scan equal hit flag
            statusRegister2 = statusRegister2 & 0xF7;
            if ((cmdParams.commandCode & 0x08) == 0 ||
                ((cmdParams.commandCode & 0x1F) == 0x19 && (*dataPtr) > n) ||
                ((cmdParams.commandCode & 0x1F) == 0x1D && (*dataPtr) < n)) {
              // set scan not satisfied flag
              statusRegister2 = statusRegister2 | 0x04;
            }
          }
        }
        else {
          (*dataPtr) = n;
        }
        dataIsNotReady = true;
        executionPhaseFlags = executionPhaseFlags | 0x01;
      }
    }
  }

  uint8_t FDC765::readDataRegisterDebug() const
  {
    if (dataDirectionIsRead && dataBytesRemaining > 0U && !dataIsNotReady)
      return sectorBuf[totalDataBytes - dataBytesRemaining];
    return 0xFF;
  }

  uint8_t FDC765::debugRead(uint16_t addr) const
  {
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
    case 0x0008:
      return cmdParams.cylinderID;
    case 0x0009:
      return cmdParams.headID;
    case 0x000A:
      return cmdParams.sectorID;
    case 0x000B:
      return cmdParams.sectorSizeCode;
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

