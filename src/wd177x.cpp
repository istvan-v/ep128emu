
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
#include "ep_fdd.hpp"
#include "wd177x.hpp"

namespace Ep128Emu {

  WD177x::WD177x()
    : floppyDrive((FloppyDrive *) 0),
      commandRegister(0x00),
      statusRegister(0x00),
      trackRegister(0xFF),
      sectorRegister(1),
      dataRegister(0xFF),
      interruptRequestFlag(false),
      dataRequestFlag(false),
      isWD1773(false),
      steppingIn(false),
      busyFlagHackEnabled(false),
      busyFlagHack(false),
      writeTrackSectorsRemaining(0),
      writeTrackState(0xFF)
  {
    floppyDrive = &dummyFloppyDrive;
    this->reset(true);
  }

  WD177x::~WD177x()
  {
  }

  void WD177x::setFloppyDrive(FloppyDrive *d)
  {
    if (!d)
      d = &dummyFloppyDrive;
    if (d != floppyDrive) {
      if ((statusRegister & 0x01) != 0 || floppyDrive->isDataTransfer())
        setError(floppyDrive->isDataTransfer() ? 0x04 : 0x00);  // lost data
    }
    floppyDrive = d;
  }

  void WD177x::setError(uint8_t n)
  {
    // on error: clear data request and busy flag, and trigger interrupt
    statusRegister &= uint8_t(0xFC);
    statusRegister |= n;
    dataRequestFlag = false;
    floppyDrive->motorOff();
    if ((commandRegister & 0xE0) == 0xA0 || (commandRegister & 0xF0) == 0xF0) {
      if (floppyDrive->getSectorBufferPosition() > 0L) {
        // partially written sector: fill the remaining bytes with zeros
        floppyDrive->padSector();
      }
    }
    floppyDrive->endDataTransfer();
    commandRegister = 0x00;
    writeTrackSectorsRemaining = 0;
    writeTrackState = 0xFF;
    if (!interruptRequestFlag) {
      interruptRequestFlag = true;
      interruptRequest();
    }
  }

  void WD177x::doStep(int n, bool updateFlag)
  {
    if (!n)
      return;
    steppingIn = (n >= 0);
    if (floppyDrive != &dummyFloppyDrive) {
      floppyDrive->doStep(n);
      if (floppyDrive->getIsTrack0()) {
        trackRegister = 0;
        return;
      }
    }
    if (updateFlag)
      trackRegister = uint8_t((int(trackRegister) + n) & 0xFF);
  }

  void WD177x::writeCommandRegister(uint8_t n)
  {
    if ((statusRegister & 0x01) != 0 && (n & 0xF0) != 0xD0) {
      // ignore command if wd177x is busy, and the command is not
      // force interrupt
      return;
    }
    if ((statusRegister & 0x01) == 0)   // store new command
      commandRegister = n;
    if ((n & 0x80) == 0) {              // ---- Type I commands ----
      uint8_t r = n & 3;                // step rate (ignored)
      bool    v = ((n & 0x04) != 0);    // verify flag
      bool    h = ((n & 0x08) != 0);    // disable spin-up (ignored)
      bool    u = ((n & 0x10) != 0);    // update flag
      (void) r;
      (void) h;
      // set motor on and busy flag
      // reset CRC, seek error, data request, and IRQ
      dataRequestFlag = false;
      statusRegister = 0xA1;
      if (floppyDrive != &dummyFloppyDrive)
        floppyDrive->motorOn();
      if (interruptRequestFlag) {
        interruptRequestFlag = false;
        clearInterruptRequest();
      }
      if (!(n & 0xE0)) {                // SEEK
        if (!(n & 0x10)) {              // RESTORE
          trackRegister = 0xFF;
          dataRegister = 0;
        }
        doStep(int(dataRegister) - int(trackRegister), true);
      }
      else {                            // STEP
        if ((n & 0xE0) == 0x40)         // STEP IN
          steppingIn = true;
        else if ((n & 0xE0) == 0x60)    // STEP OUT
          steppingIn = false;
        doStep((steppingIn ? 1 : -1), u);
      }
      // command done: update flags and trigger interrupt
      if (floppyDrive->getIsWriteProtected())
        statusRegister = statusRegister | 0x40;
      if (v && !floppyDrive->checkCurrentTrack(trackRegister))
        statusRegister = statusRegister | 0x10; // seek error
      if (floppyDrive != &dummyFloppyDrive && floppyDrive->getIsTrack0())
        statusRegister = statusRegister | 0x04;
      if (floppyDrive->haveDisk())
        setError(0x02);                         // index pulse
      else
        setError();
    }
    else if ((n & 0xC0) == 0x80) {      // ---- Type II commands ----
      bool    a0 = ((n & 0x01) != 0);   // write deleted data mark (ignored)
      bool    pc = ((n & 0x02) != 0);   // disable write precompensation
                                        // / enable side compare
      bool    e = ((n & 0x04) != 0);    // settling delay (ignored)
      bool    hs = ((n & 0x08) != 0);   // disable spin-up / side select
      bool    m = ((n & 0x10) != 0);    // multiple sectors
      (void) a0;
      (void) e;
      (void) m;
      // set motor on and busy flag
      // reset data request, lost data, record not found, and interrupt request
      dataRequestFlag = false;
      statusRegister = 0x81;
      if (floppyDrive != &dummyFloppyDrive)
        floppyDrive->motorOn();
      if (interruptRequestFlag) {
        interruptRequestFlag = false;
        clearInterruptRequest();
      }
      // select side (if enabled)
      if (isWD1773) {
        if (pc)
          floppyDrive->setSide(uint8_t(hs ? 1 : 0));
      }
      floppyDrive->endDataTransfer();
      if ((n & 0x20) == 0) {            // READ SECTOR
        if (!floppyDrive->startDataTransfer(trackRegister, sectorRegister)) {
          statusRegister = statusRegister | 0x10;   // record not found
        }
        else {
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
        }
      }
      else {                            // WRITE SECTOR
        if (floppyDrive->getIsWriteProtected()) {
          statusRegister = statusRegister | 0x40;   // disk is write protected
        }
        else if (!floppyDrive->startDataTransfer(trackRegister,
                                                 sectorRegister)) {
          statusRegister = statusRegister | 0x10;   // record not found
        }
        else {
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
        }
      }
      if (!floppyDrive->isDataTransfer()) {
        if (floppyDrive->haveDisk())
          setError();
      }
    }
    else if ((n & 0xF0) != 0xD0) {      // ---- Type III commands ----
      bool    p = ((n & 0x02) != 0);    // disable write precompensation
      bool    e = ((n & 0x04) != 0);    // settling delay (ignored)
      bool    h = ((n & 0x08) != 0);    // disable spin-up (ignored)
      (void) p;
      (void) e;
      (void) h;
      // set motor on and busy flag
      // reset data request, lost data, record not found, and interrupt request
      dataRequestFlag = false;
      statusRegister = 0x81;
      if (floppyDrive != &dummyFloppyDrive)
        floppyDrive->motorOn();
      if (interruptRequestFlag) {
        interruptRequestFlag = false;
        clearInterruptRequest();
      }
      floppyDrive->endDataTransfer();
      if ((n & 0x20) == 0) {            // READ ADDRESS
        if (!floppyDrive->startDataTransfer(floppyDrive->getCurrentTrack(),
                                            1)) {
          statusRegister = statusRegister | 0x10;   // record not found
        }
        else {
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
        }
      }
      else if ((n & 0x10) == 0) {       // READ TRACK (FIXME: unimplemented)
        statusRegister = statusRegister | 0x10;     // record not found
      }
      else {                            // WRITE TRACK
        if (floppyDrive->getIsWriteProtected()) {
          statusRegister = statusRegister | 0x40;   // disk is write protected
        }
        else if (!floppyDrive->startDataTransfer(trackRegister, 1)) {
          statusRegister = statusRegister | 0x10;   // record not found
        }
        else {
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
          writeTrackSectorsRemaining = floppyDrive->getSectorsPerTrack();
          writeTrackState = 0;
        }
      }
      if (!floppyDrive->isDataTransfer()) {
        if (floppyDrive->haveDisk())
          setError();
      }
    }
    else {                              // ---- Type IV commands ----
      bool    i0 = ((n & 0x01) != 0);
      bool    i1 = ((n & 0x02) != 0);
      bool    i2 = ((n & 0x04) != 0);
      bool    i3 = ((n & 0x08) != 0);
      (void) i0;                        // FORCE INTERRUPT
      (void) i1;
      // reset status
      dataRequestFlag = false;
      statusRegister = (statusRegister & 0x01) | 0x20;
      if (floppyDrive->getIsWriteProtected())
        statusRegister = statusRegister | 0x40;
      if (floppyDrive != &dummyFloppyDrive && floppyDrive->getIsTrack0())
        statusRegister = statusRegister | 0x04;
      if (floppyDrive->haveDisk())
        statusRegister = statusRegister | 0x02; // index pulse
      if ((commandRegister & 0xE0) == 0xA0 ||
          (commandRegister & 0xF0) == 0xF0) {
        if (floppyDrive->getSectorBufferPosition() > 0L) {
          // partially written sector: fill the remaining bytes with zeros
          floppyDrive->padSector();
        }
      }
      floppyDrive->endDataTransfer();
      // FIXME: only immediate interrupt is implemented
      statusRegister = statusRegister & 0xFE;
      floppyDrive->motorOff();
      commandRegister = n;
      if (i2 || i3) {
        if (!interruptRequestFlag) {
          interruptRequestFlag = true;
          interruptRequest();
        }
      }
    }
  }

  uint8_t WD177x::readStatusRegister()
  {
    if (interruptRequestFlag && (commandRegister & 0xF8) != 0xD8) {
      interruptRequestFlag = false;
      clearInterruptRequest();
    }
    uint8_t n = statusRegister & 0x7F;
    if (isWD1773)                       // WD1773: bit 7 = not ready
      n = n | (uint8_t(floppyDrive == &dummyFloppyDrive) << 7);
    else                                // WD1770/2: bit 7 = motor on
      n = n | ((uint8_t(floppyDrive->getIsMotorOn()) | (n & 0x01)) << 7);
    if (busyFlagHackEnabled) {
      busyFlagHack = !busyFlagHack;
      if (busyFlagHack)
        n = n | 0x01;
    }
    return n;
  }

  void WD177x::writeTrackRegister(uint8_t n)
  {
    if ((statusRegister & 0x01) == 0)
      trackRegister = n;
  }

  uint8_t WD177x::readTrackRegister() const
  {
    return trackRegister;
  }

  void WD177x::writeSectorRegister(uint8_t n)
  {
    if ((statusRegister & 0x01) == 0)
      sectorRegister = n;
  }

  uint8_t WD177x::readSectorRegister() const
  {
    return sectorRegister;
  }

  void WD177x::writeDataRegister(uint8_t n)
  {
    dataRegister = n;
    if (!dataRequestFlag)
      return;
    if ((commandRegister & 0xE0) == 0xA0 && floppyDrive->isDataTransfer()) {
      // writing sector
      // FIXME: errors are ignored here
      (void) floppyDrive->updateBufferedTrack();
      if (floppyDrive->writeByte(dataRegister)) {
        // end of sector: clear data request and busy flag
        floppyDrive->endDataTransfer();
        dataRequestFlag = false;
        statusRegister = statusRegister & 0xFC;
        if (commandRegister & 0x10) {
          // multiple sectors: continue with writing next sector
          sectorRegister++;
          writeCommandRegister(commandRegister);
          return;
        }
        setError();
      }
    }
    else if (EP128EMU_UNLIKELY((commandRegister & 0xF0) == 0xF0 &&
                               writeTrackState != 0xFF)) {
      // writing track
      while (true) {
        int     bytesReceived = int(writeTrackState >> 5);
        if (writeTrackState < 0xE0)
          writeTrackState = writeTrackState + 0x20;
        uint8_t curState = writeTrackState & 0x1F;
        uint8_t nxtState = curState + 1;
        switch (curState) {
        case 0:                         // gap
        case 4:
        case 13:
          if (n != 0x4E) {
            if (bytesReceived >= 7) {
              writeTrackState = nxtState;
              continue;
            }
            else {
              setError(0x04);
            }
          }
          break;
        case 1:                         // sync
        case 5:
        case 14:
          if (n != 0x00) {
            if (bytesReceived >= 7) {
              writeTrackState = nxtState;
              continue;
            }
            else {
              setError(0x04);
            }
          }
          break;
        case 2:                         // address mark
          if (bytesReceived > 3 || (n != 0xF6 && bytesReceived != 3)) {
            setError(0x04);
          }
          else if (bytesReceived == 3) {
            writeTrackState = nxtState;
            continue;
          }
          break;
        case 6:
        case 15:
          if (bytesReceived > 3 || (n != 0xF5 && bytesReceived != 3)) {
            setError(0x04);
          }
          else if (bytesReceived == 3) {
            writeTrackState = nxtState;
            continue;
          }
          break;
        case 3:
          if (n == 0xFC)
            writeTrackState = nxtState;
          else
            setError(0x04);
          break;
        case 7:                         // sector ID
          if (n == 0xFE)
            writeTrackState = nxtState;
          else
            setError(0x04);
          break;
        case 16:                        // sector data
          if (n == 0xFB)
            writeTrackState = nxtState;
          else
            setError(0x04);
          break;
        case 8:                         // track number
          if (n == floppyDrive->getCurrentTrack())
            writeTrackState = nxtState;
          else
            setError(0x14);
          break;
        case 9:                         // side number
          if (n == floppyDrive->getCurrentSide())
            writeTrackState = nxtState;
          else
            setError(0x14);
          break;
        case 10:                        // sector number
          if (floppyDrive->startDataTransfer(trackRegister, n))
            writeTrackState = nxtState;
          else
            setError(0x14);
          break;
        case 11:                        // sector length (must be 512 bytes)
          if (n == 0x02)
            writeTrackState = nxtState;
          else
            setError(0x04);
          break;
        case 12:                        // CRC
        case 18:
          if (n == 0xF7)
            writeTrackState = nxtState;
          else
            setError(0x04);
          break;
        case 17:                        // sector data, store in buffer
          if (n >= 0xF5 && n <= 0xF7) {
            setError(0x0C);
            return;
          }
          // FIXME: errors are ignored here
          (void) floppyDrive->updateBufferedTrack();
          if (floppyDrive->writeByte(dataRegister))
            writeTrackState = nxtState;
          break;
        default:                        // gap at end of sector / track
          if (n != 0x4E) {
            setError(0x04);
            return;
          }
          if (bytesReceived >= 6) {
            if (writeTrackSectorsRemaining > 0)
              writeTrackSectorsRemaining--;
            if (writeTrackSectorsRemaining > 0)
              writeTrackState = 4;          // continue with next sector
            else
              writeTrackState = nxtState;   // done formatting all sectors
            if (writeTrackState == 31) {
              // done formatting track
              // FIXME: errors are ignored here
              (void) floppyDrive->flushTrack();
              setError();
            }
          }
          break;
        }
        break;
      }
    }
  }

  uint8_t WD177x::readDataRegister()
  {
    if (!(dataRequestFlag && floppyDrive->isDataTransfer()))
      return dataRegister;
    if ((commandRegister & 0xE0) == 0x80) {
      // reading sector
      // FIXME: errors are ignored here
      (void) floppyDrive->updateBufferedTrack();
      if (!floppyDrive->getSectorData()) {
        setError(0x08);                 // CRC error
        dataRegister = 0x00;
        return 0x00;
      }
      if (floppyDrive->readByte(dataRegister)) {
        // end of sector: clear data request and busy flag
        floppyDrive->endDataTransfer();
        dataRequestFlag = false;
        statusRegister = statusRegister & 0xFC;
        if (commandRegister & 0x10) {
          // multiple sectors: continue with reading next sector
          sectorRegister++;
          writeCommandRegister(commandRegister);
        }
        else {
          setError();
        }
      }
    }
    else if ((commandRegister & 0xF0) == 0xC0) {        // read address
      switch (floppyDrive->getSectorBufferPosition()) {
      case 0L:
        dataRegister = floppyDrive->getCurrentTrack();
        sectorRegister = dataRegister;
        break;
      case 1L:
        dataRegister = floppyDrive->getCurrentSide();
        break;
      case 2L:
        dataRegister = 0x01;            // assume first sector of track
        break;
      case 3L:
        dataRegister = 0x02;            // 512 bytes per sector
        break;
      case 4L:                          // CRC high byte
      case 5L:                          // CRC low byte
        {
          uint8_t   tmpBuf[4];
          tmpBuf[0] = floppyDrive->getCurrentTrack();
          tmpBuf[1] = floppyDrive->getCurrentSide();
          tmpBuf[2] = 0x01;
          tmpBuf[3] = 0x02;
          uint16_t  tmp = calculateCRC(&(tmpBuf[0]), 4, 0xB230);
          if (floppyDrive->getSectorBufferPosition() == 4L)
            dataRegister = uint8_t(tmp >> 8);
          else
            dataRegister = uint8_t(tmp & 0xFF);
        }
        break;
      }
      uint8_t tmp = 0;
      (void) floppyDrive->readByte(tmp);
      if (floppyDrive->getSectorBufferPosition() >= 6L)
        setError();
    }
    return dataRegister;
  }

  uint8_t WD177x::readStatusRegisterDebug() const
  {
    uint8_t n = statusRegister & 0x7F;
    if (isWD1773)                       // WD1773: bit 7 = not ready
      n = n | (uint8_t(floppyDrive == &dummyFloppyDrive) << 7);
    else                                // WD1770/2: bit 7 = motor on
      n = n | ((uint8_t(floppyDrive->getIsMotorOn()) | (n & 0x01)) << 7);
    return n;
  }

  uint8_t WD177x::readDataRegisterDebug() const
  {
    return dataRegister;
  }

  void WD177x::setIsWD1773(bool isEnabled)
  {
    isWD1773 = isEnabled;
  }

  void WD177x::setEnableBusyFlagHack(bool isEnabled)
  {
    busyFlagHackEnabled = isEnabled;
  }

  uint16_t WD177x::calculateCRC(const uint8_t *buf, size_t nBytes, uint16_t n)
  {
    size_t  nBits = nBytes << 3;
    int     bitCnt = 0;
    uint8_t bitBuf = 0;

    while (nBits--) {
      if (bitCnt == 0) {
        bitBuf = *(buf++);
        bitCnt = 8;
      }
      if ((bitBuf ^ uint8_t(n >> 8)) & 0x80)
        n = (n << 1) ^ 0x1021;
      else
        n = (n << 1);
      n = n & 0xFFFF;
      bitBuf = (bitBuf << 1) & 0xFF;
      bitCnt--;
    }
    return n;
  }

  void WD177x::reset(bool isColdReset)
  {
    if (interruptRequestFlag) {
      interruptRequestFlag = false;
      clearInterruptRequest();
    }
    if ((statusRegister & 0x01) != 0) {
      // terminate any unfinished command
      interruptRequestFlag = true;  // hack to avoid triggering an interrupt
      writeCommandRegister(0xD8);
    }
    floppyDrive->motorOff();
    floppyDrive->reset();
    commandRegister = 0x00;
    statusRegister = 0x00;
    sectorRegister = 1;
    if (isColdReset) {
      trackRegister = 0xFF;
      dataRegister = 0xFF;
    }
    interruptRequestFlag = false;
    dataRequestFlag = false;
    steppingIn = false;
    busyFlagHack = false;
    writeTrackSectorsRemaining = 0;
    writeTrackState = 0xFF;
  }

  void WD177x::interruptRequest()
  {
  }

  void WD177x::clearInterruptRequest()
  {
  }

}       // namespace Ep128Emu

