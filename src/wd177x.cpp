
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2008 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "wd177x.hpp"

#include <vector>

#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)

#include <windows.h>
#include <winioctl.h>

static int checkFloppyDisk(const std::string& fileName,
                           int& nTracks, int& nSides, int& nSectorsPerTrack)
{
  if (fileName.length() < 5)
    return 0;                   // return value == 0: regular file
  if (!(fileName[0] == '\\' && fileName[1] == '\\' &&
        fileName[2] == '.' && fileName[3] == '\\')) {
    return 0;
  }
  HANDLE  h = CreateFileA(fileName.c_str(), (DWORD) 0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          (LPSECURITY_ATTRIBUTES) 0, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, (HANDLE) 0);
  if (h == INVALID_HANDLE_VALUE)
    return -1;                  // return value == -1: error opening device
  DISK_GEOMETRY diskGeometry;
  DWORD   tmp;
  if (DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY,
                      (LPVOID) 0, (DWORD) 0,
                      &diskGeometry, (DWORD) sizeof(diskGeometry),
                      &tmp, (LPOVERLAPPED) 0) == FALSE) {
    CloseHandle(h);
    return -1;
  }
  bool    errorFlag = false;
  if (!(diskGeometry.MediaType > Unknown &&
        diskGeometry.MediaType < RemovableMedia)) {
    errorFlag = true;
  }
  if (diskGeometry.Cylinders.QuadPart < (LONGLONG) 1 ||
      diskGeometry.Cylinders.QuadPart > (LONGLONG) 240 ||
      diskGeometry.TracksPerCylinder < (DWORD) 1 ||
      diskGeometry.TracksPerCylinder > (DWORD) 2 ||
      diskGeometry.SectorsPerTrack < (DWORD) 1 ||
      diskGeometry.SectorsPerTrack > (DWORD) 240 ||
      diskGeometry.BytesPerSector != (DWORD) 512) {
    errorFlag = true;
  }
  if (nTracks >= 1 && nTracks <= 240) {
    if (nTracks != int(diskGeometry.Cylinders.QuadPart))
      errorFlag = true;
  }
  if (nSides >= 1 && nSides <= 2) {
    if (nSides != int(diskGeometry.TracksPerCylinder))
      errorFlag = true;
  }
  if (nSectorsPerTrack >= 1 && nSectorsPerTrack <= 240) {
    if (nSectorsPerTrack != int(diskGeometry.SectorsPerTrack))
      errorFlag = true;
  }
  if (errorFlag) {
    CloseHandle(h);
    return -2;                  // return value == -2: invalid disk geometry
  }
  nTracks = int(diskGeometry.Cylinders.QuadPart);
  nSides = int(diskGeometry.TracksPerCylinder);
  nSectorsPerTrack = int(diskGeometry.SectorsPerTrack);
  if (DeviceIoControl(h, IOCTL_DISK_IS_WRITABLE,
                      (LPVOID) 0, (DWORD) 0, (LPVOID) 0, (DWORD) 0,
                      &tmp, (LPOVERLAPPED) 0) == FALSE) {
    CloseHandle(h);
    return 1;                   // return value == 1: write protected disk
  }
  CloseHandle(h);
  return 2;                     // return value == 2: writable floppy disk
}

#endif

namespace Ep128Emu {

  WD177x::WD177x()
    : imageFileName(""),
      imageFile((std::FILE *) 0),
      nTracks(0),
      nSides(0),
      nSectorsPerTrack(0),
      commandRegister(0),
      statusRegister(0),
      trackRegister(0),
      sectorRegister(0),
      dataRegister(0),
      currentTrack(0),
      currentSide(0),
      writeProtectFlag(false),
      diskChangeFlag(true),
      interruptRequestFlag(false),
      dataRequestFlag(false),
      isWD1773(false),
      steppingIn(false),
      busyFlagHackEnabled(false),
      busyFlagHack(false),
      bufPos(512),
      trackNotReadFlag(true),
      trackDirtyFlag(false),
      writeTrackSectorsRemaining(0),
      writeTrackState(0xFF)
  {
    trackBuffer.resize(0);
    this->reset();
  }

  WD177x::~WD177x()
  {
    if (imageFile) {
      (void) flushTrack();              // FIXME: errors are ignored here
      trackDirtyFlag = false;
      std::fclose(imageFile);
      imageFile = (std::FILE *) 0;
    }
  }

  bool WD177x::checkDiskPosition(bool ignoreSectorRegister)
  {
    if (!imageFile) {
      return false;
    }
    if (currentTrack >= nTracks || currentTrack != trackRegister ||
        currentSide >= nSides ||
        ((sectorRegister < 1 || sectorRegister > nSectorsPerTrack) &&
         !ignoreSectorRegister)) {
      return false;
    }
    return true;
  }

  void WD177x::setError(uint8_t n)
  {
    commandRegister = 0x00;
    // on error: clear data request and busy flag, and trigger interrupt
    statusRegister &= uint8_t(0xFC);
    statusRegister |= n;
    dataRequestFlag = false;
    bufPos = 512;
    writeTrackSectorsRemaining = 0;
    writeTrackState = 0xFF;
    if (!interruptRequestFlag) {
      interruptRequestFlag = true;
      interruptRequest();
    }
  }

  void WD177x::doStep(bool updateFlag)
  {
    if (trackDirtyFlag) {
      (void) flushTrack();              // FIXME: errors are ignored here
      trackDirtyFlag = false;
    }
    if (steppingIn) {
      currentTrack++;
      if (updateFlag)
        trackRegister++;
    }
    else {
      currentTrack--;
      if (updateFlag)
        trackRegister--;
      if (currentTrack == 0)
        trackRegister = 0;
    }
    trackNotReadFlag = true;
  }

  void WD177x::setDiskImageFile(const std::string& fileName_,
                                int nTracks_, int nSides_,
                                int nSectorsPerTrack_)
  {
    if ((fileName_ == "" && imageFileName == "") ||
        (fileName_ == imageFileName &&
         nTracks_ == int(nTracks) &&
         nSides_ == int(nSides) &&
         nSectorsPerTrack_ == int(nSectorsPerTrack))) {
      return;
    }
    if (imageFile) {
      (void) flushTrack();              // FIXME: errors are ignored here
      trackDirtyFlag = false;
      std::fclose(imageFile);
      imageFile = (std::FILE *) 0;
    }
    imageFileName = "";
    nTracks = 0;
    nSides = 0;
    nSectorsPerTrack = 0;
    writeProtectFlag = false;
    this->reset();
    if (fileName_ == "")
      return;
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    {
      int     diskType = checkFloppyDisk(fileName_,
                                         nTracks_, nSides_, nSectorsPerTrack_);
      if (diskType == 1) {
        writeProtectFlag = true;
      }
      else if (diskType == -2) {
        throw Exception("wd177x: invalid or inconsistent "
                        "disk image size parameters");
      }
      else if (diskType < 0) {
        throw Exception("wd177x: error opening disk image file");
      }
    }
#endif
    unsigned char tmpBuf[512];
    bool    nTracksValid = (nTracks_ >= 1 && nTracks_ <= 240);
    bool    nSidesValid = (nSides_ >= 1 && nSides_ <= 2);
    bool    nSectorsPerTrackValid =
                (nSectorsPerTrack_ >= 1 && nSectorsPerTrack_ <= 240);
    try {
      if (!writeProtectFlag)
        imageFile = std::fopen(fileName_.c_str(), "r+b");
      if (!imageFile) {
        imageFile = std::fopen(fileName_.c_str(), "rb");
        if (imageFile)
          writeProtectFlag = true;
        else
          throw Exception("wd177x: error opening disk image file");
      }
      std::setvbuf(imageFile, (char *) 0, _IONBF, 0);
      long    fileSize = -1L;
      if (std::fseek(imageFile, 0L, SEEK_END) >= 0)
        fileSize = std::ftell(imageFile);
      if (fileSize > (240L * 2L * 240L * 512L))
        fileSize = -1L;
      long    nSectors_ = fileSize / 512L;
      if (!nTracksValid) {
        if (nSectors_ > 0L && nSidesValid && nSectorsPerTrackValid) {
          nTracks_ = int(nSectors_ / (long(nSides_) * long(nSectorsPerTrack_)));
          nTracksValid = true;
        }
      }
      else if (!nSidesValid) {
        if (nSectors_ > 0L && nTracksValid && nSectorsPerTrackValid) {
          nSides_ = int(nSectors_ / (long(nTracks_) * long(nSectorsPerTrack_)));
          nSidesValid = true;
        }
      }
      else if (!nSectorsPerTrackValid) {
        if (nSectors_ > 0L && nTracksValid && nSidesValid) {
          nSectorsPerTrack_ = int(nSectors_ / (long(nTracks_) * long(nSides_)));
          nSectorsPerTrackValid = true;
        }
      }
      if (!(nTracksValid && nSidesValid && nSectorsPerTrackValid)) {
        // try to find out geometry parameters from FAT filesystem
        if (std::fseek(imageFile, 0L, SEEK_SET) >= 0) {
          if (std::fread(&(tmpBuf[0]), 1, 512, imageFile) == 512) {
            nSectors_ = long(tmpBuf[0x13]) | (long(tmpBuf[0x14]) << 8);
            if (!nSectors_) {
              nSectors_ =
                  long(tmpBuf[0x20]) | (long(tmpBuf[0x21]) << 8)
                  | (long(tmpBuf[0x22]) << 16) | (long(tmpBuf[0x23]) << 24);
            }
            if (!nSidesValid)
              nSides_ = int(tmpBuf[0x1A]) | (int(tmpBuf[0x1B]) << 8);
            if (!nSectorsPerTrackValid)
              nSectorsPerTrack_ = int(tmpBuf[0x18]) | (int(tmpBuf[0x19]) << 8);
            if (!nTracksValid) {
              if (nSides_ >= 1 && nSides_ <= 2 &&
                  nSectorsPerTrack_ >= 1 && nSectorsPerTrack_ <= 240 &&
                  nSectors_ >= 1L && nSectors_ <= (240L * 2L * 240L)) {
                nTracks_ =
                    int(nSectors_ / (long(nSides_) * long(nSectorsPerTrack_)));
              }
            }
          }
        }
      }
      if (!(nTracks_ >= 1 && nTracks_ <= 240 &&
            nSides_ >= 1 && nSides_ <= 2 &&
            nSectorsPerTrack_ >= 1 && nSectorsPerTrack_ <= 240))
        throw Exception("wd177x: cannot determine size of disk image");
      nSectors_ = long(nTracks_) * long(nSides_) * long(nSectorsPerTrack_);
      fileSize = nSectors_ * 512L;
      bool    err = true;
      if (std::fseek(imageFile, fileSize - 512L, SEEK_SET) >= 0) {
        if (std::fread(&(tmpBuf[0]), 1, 512, imageFile) == 512) {
          if (std::fread(&(tmpBuf[0]), 1, 512, imageFile) == 0) {
            err = false;
          }
        }
      }
      if (err) {
        throw Exception("wd177x: invalid or inconsistent "
                        "disk image size parameters");
      }
      std::fseek(imageFile, 0L, SEEK_SET);
      imageFileName = fileName_;
      trackBuffer.resize(size_t(nSectorsPerTrack_) * 512);
    }
    catch (...) {
      if (imageFile) {
        std::fclose(imageFile);
        imageFile = (std::FILE *) 0;
      }
      writeProtectFlag = false;
      throw;
    }
    nTracks = uint8_t(nTracks_);
    nSides = uint8_t(nSides_);
    nSectorsPerTrack = uint8_t(nSectorsPerTrack_);
    this->reset();
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
      // set busy flag; reset CRC, seek error, data request, and IRQ
      dataRequestFlag = false;
      statusRegister = 0x21;
      if (interruptRequestFlag) {
        interruptRequestFlag = false;
        clearInterruptRequest();
      }
      if ((n & 0xF0) == 0) {            // RESTORE
        trackRegister = currentTrack;
        dataRegister = 0;
        if (dataRegister < trackRegister) {
          steppingIn = false;
          do {
            doStep(true);
          } while (trackRegister != dataRegister);
        }
      }
      else if ((n & 0xF0) == 0x10) {    // SEEK
        if (dataRegister > trackRegister) {
          steppingIn = true;
          do {
            doStep(true);
          } while (trackRegister != dataRegister);
        }
        else {
          steppingIn = false;
          do {
            doStep(true);
          } while (trackRegister != dataRegister);
        }
      }
      else {                            // STEP
        if ((n & 0xE0) == 0x40)         // STEP IN
          steppingIn = true;
        else if ((n & 0xE0) == 0x60)    // STEP OUT
          steppingIn = false;
        doStep(u);
      }
      // command done: update flags and trigger interrupt
      if (writeProtectFlag)
        statusRegister = statusRegister | 0x40;
      if (v && (imageFile == (std::FILE *) 0 ||
                currentTrack >= nTracks || currentTrack != trackRegister))
        statusRegister = statusRegister | 0x10; // seek error
      if (currentTrack == 0)
        statusRegister = statusRegister | 0x04;
      if (imageFile)
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
      // set busy flag; reset data request, lost data, record not found,
      // and interrupt request
      dataRequestFlag = false;
      statusRegister = 0x01;
      if (interruptRequestFlag) {
        interruptRequestFlag = false;
        clearInterruptRequest();
      }
      // select side (if enabled)
      if (isWD1773) {
        if (pc)
          currentSide = uint8_t(hs ? 1 : 0);
      }
      bufPos = 512;
      if ((n & 0x20) == 0) {            // READ SECTOR
        if (!checkDiskPosition()) {
          statusRegister = statusRegister | 0x10;   // record not found
        }
        else {
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
          bufPos = 0;
        }
      }
      else {                            // WRITE SECTOR
        if (writeProtectFlag) {
          statusRegister = statusRegister | 0x40;   // disk is write protected
        }
        else if (!checkDiskPosition()) {
          statusRegister = statusRegister | 0x10;   // record not found
        }
        else {
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
          bufPos = 0;
        }
      }
      if (bufPos >= 512)
        setError();
    }
    else if ((n & 0xF0) != 0xD0) {      // ---- Type III commands ----
      bool    p = ((n & 0x02) != 0);    // disable write precompensation
      bool    e = ((n & 0x04) != 0);    // settling delay (ignored)
      bool    h = ((n & 0x08) != 0);    // disable spin-up (ignored)
      (void) p;
      (void) e;
      (void) h;
      // set busy flag; reset data request, lost data, record not found,
      // and interrupt request
      dataRequestFlag = false;
      statusRegister = 0x01;
      if (interruptRequestFlag) {
        interruptRequestFlag = false;
        clearInterruptRequest();
      }
      bufPos = 512;
      if ((n & 0x20) == 0) {            // READ ADDRESS
        if (checkDiskPosition(true)) {
          bufPos = 506;
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
        }
        else
          statusRegister = statusRegister | 0x10;   // record not found
      }
      else if ((n & 0x10) == 0) {       // READ TRACK (FIXME: unimplemented)
        statusRegister = statusRegister | 0x80;     // not ready
      }
      else {                            // WRITE TRACK
        if (writeProtectFlag) {
          statusRegister = statusRegister | 0x40;   // disk is write protected
        }
        else if (!checkDiskPosition(true)) {
          statusRegister = statusRegister | 0x10;   // record not found
        }
        else {
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
          bufPos = 0;
          writeTrackSectorsRemaining = nSectorsPerTrack;
          writeTrackState = 0;
        }
      }
      if (bufPos >= 512)
        setError();
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
      if (writeProtectFlag)
        statusRegister = statusRegister | 0x40;
      if (imageFile) {
        if (currentTrack == 0)
          statusRegister = statusRegister | 0x04;
        statusRegister = statusRegister | 0x02; // index pulse
      }
      // FIXME: only immediate interrupt is implemented
      statusRegister = statusRegister & 0xFE;
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
    uint8_t n = statusRegister;
    if (isWD1773)
      n = n & 0x7F;             // always ready
    else
      n = n | 0x80;             // motor is always on
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
    if ((commandRegister & 0xE0) == 0xA0 && bufPos < 512) {
      // writing sector
      if (sectorRegister < 1 || sectorRegister > nSectorsPerTrack) {
        setError(0x10);                 // record not found
        return;
      }
      if (trackNotReadFlag) {
        if (!readTrack()) {
          setError(0x0C);               // CRC error, lost data
          trackNotReadFlag = true;
          return;
        }
      }
      trackDirtyFlag = true;
      trackBuffer[size_t(sectorRegister - 1) * 512 + bufPos] = dataRegister;
      bufPos++;
      if (bufPos >= 512) {
        bufPos = 0;
        // clear data request and busy flag
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
    else if ((commandRegister & 0xF0) == 0xF0 && writeTrackState != 0xFF) {
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
          if (n == currentTrack)
            writeTrackState = nxtState;
          else
            setError(0x14);
          break;
        case 9:                         // side number
          if (n == currentSide)
            writeTrackState = nxtState;
          else
            setError(0x14);
          break;
        case 10:                        // sector number
          if (n >= 1 && n <= nSectorsPerTrack) {
            bufPos = size_t(n - 1) * 512;
            writeTrackState = nxtState;
          }
          else {
            setError(0x14);
          }
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
          if (trackNotReadFlag) {
            if (!readTrack()) {
              setError(0x0C);           // CRC error, lost data
              trackNotReadFlag = true;
              return;
            }
          }
          trackDirtyFlag = true;
          trackBuffer[bufPos] = dataRegister;
          bufPos++;
          if ((bufPos & 511) == 0)
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
              (void) flushTrack();      // FIXME: errors are ignored here
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
    if (!(dataRequestFlag && bufPos < 512))
      return dataRegister;
    if ((commandRegister & 0xE0) == 0x80) {
      // reading sector
      if (sectorRegister < 1 || sectorRegister > nSectorsPerTrack) {
        setError(0x10);                 // record not found
        dataRegister = 0x00;
        return 0x00;
      }
      if (trackNotReadFlag) {
        if (!readTrack()) {
          setError(0x08);               // CRC error
          dataRegister = 0x00;
          return 0x00;
        }
      }
      dataRegister = trackBuffer[size_t(sectorRegister - 1) * 512 + bufPos];
      bufPos++;
      if (bufPos >= 512) {
        bufPos = 0;
        // clear data request and busy flag
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
      switch (bufPos) {
      case 506:
        dataRegister = currentTrack;
        break;
      case 507:
        dataRegister = currentSide;
        break;
      case 508:
        dataRegister = 0x01;            // assume first sector of track
        sectorRegister = dataRegister;
        break;
      case 509:
        dataRegister = 0x02;            // 512 bytes per sector
        break;
      case 510:                         // CRC high byte
      case 511:                         // CRC low byte
        {
          uint8_t   tmpBuf[4];
          tmpBuf[0] = currentTrack;
          tmpBuf[1] = currentSide;
          tmpBuf[2] = 0x01;
          tmpBuf[3] = 0x02;
          uint16_t  tmp = calculateCRC(&(tmpBuf[0]), 4, 0xB230);
          if (bufPos == 510)
            dataRegister = uint8_t(tmp >> 8);
          else
            dataRegister = uint8_t(tmp & 0xFF);
        }
        break;
      }
      if (++bufPos >= 512)
        setError();
    }
    return dataRegister;
  }

  uint8_t WD177x::readStatusRegisterDebug() const
  {
    uint8_t n = statusRegister;
    if (isWD1773)
      n = n & 0x7F;             // always ready
    else
      n = n | 0x80;             // motor is always on
    return n;
  }

  uint8_t WD177x::readDataRegisterDebug() const
  {
    return dataRegister;
  }

  bool WD177x::getDiskChangeFlag() const
  {
    return diskChangeFlag;
  }

  void WD177x::clearDiskChangeFlag()
  {
    diskChangeFlag = false;
  }

  void WD177x::setIsWD1773(bool isEnabled)
  {
    isWD1773 = isEnabled;
  }

  bool WD177x::getInterruptRequestFlag() const
  {
    return interruptRequestFlag;
  }

  bool WD177x::getDataRequestFlag() const
  {
    return dataRequestFlag;
  }

  bool WD177x::haveDisk() const
  {
    return (imageFile != (std::FILE *) 0);
  }

  bool WD177x::getIsWriteProtected() const
  {
    return writeProtectFlag;
  }

  void WD177x::setEnableBusyFlagHack(bool isEnabled)
  {
    busyFlagHackEnabled = isEnabled;
  }

  bool WD177x::readTrack()
  {
    if (!trackNotReadFlag)
      return true;
    if (!imageFile) {
      trackNotReadFlag = false;
      return false;
    }
    size_t  nBytes = size_t(nSectorsPerTrack) * 512;
    std::memset(&(trackBuffer.front()), 0x00, nBytes);
    if (currentTrack >= nTracks) {
      trackNotReadFlag = false;
      return false;
    }
    long    filePos =
        (long(currentTrack) * long(nSides) + long(currentSide)) * long(nBytes);
    if (std::fseek(imageFile, filePos, SEEK_SET) < 0) {
      trackNotReadFlag = false;
      return false;
    }
    size_t  bytesRead =
        std::fread(&(trackBuffer.front()), sizeof(uint8_t), nBytes, imageFile);
    trackNotReadFlag = false;
    return (bytesRead == nBytes);
  }

  bool WD177x::flushTrack()
  {
    if (!trackDirtyFlag)
      return true;
    if (currentTrack >= nTracks || writeProtectFlag || !imageFile) {
      trackDirtyFlag = false;
      return false;
    }
    size_t  nBytes = size_t(nSectorsPerTrack) * 512;
    long    filePos =
        (long(currentTrack) * long(nSides) + long(currentSide)) * long(nBytes);
    if (std::fseek(imageFile, filePos, SEEK_SET) < 0) {
      trackDirtyFlag = false;
      return false;
    }
    size_t  bytesWritten =
        std::fwrite(&(trackBuffer.front()), sizeof(uint8_t), nBytes, imageFile);
    trackDirtyFlag = false;
    return (bytesWritten == nBytes);
  }

  uint16_t WD177x::calculateCRC(const uint8_t *buf_, size_t nBytes, uint16_t n)
  {
    size_t  nBits = nBytes << 3;
    int     bitCnt = 0;
    uint8_t bitBuf = 0;

    while (nBits--) {
      if (bitCnt == 0) {
        bitBuf = *(buf_++);
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

  void WD177x::reset()
  {
    (void) flushTrack();                // FIXME: errors are ignored here
    trackDirtyFlag = false;
    if (interruptRequestFlag) {
      interruptRequestFlag = false;
      clearInterruptRequest();
    }
    if ((statusRegister & 0x01) != 0) {
      // terminate any unfinished command
      interruptRequestFlag = true;  // hack to avoid triggering an interrupt
      writeCommandRegister(0xD8);
    }
    commandRegister = 0;
    statusRegister = 0x20;
    if (writeProtectFlag)
      statusRegister = statusRegister | 0x40;
    if (imageFile)
      statusRegister = statusRegister | 0x06;   // track 0, index pulse
    trackRegister = 0;
    sectorRegister = 0;
    dataRegister = 0;
    currentTrack = 0;
    currentSide = 0;
    diskChangeFlag = true;
    interruptRequestFlag = false;
    dataRequestFlag = false;
    steppingIn = false;
    busyFlagHack = false;
    bufPos = 512;
    trackNotReadFlag = true;
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

