
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2006 Istvan Varga <istvanv@users.sourceforge.net>
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

#include <cstdio>
#include <string>
#include <vector>

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
      bufPos(512)
  {
    buf.resize(512);
    this->reset();
    diskChangeFlag = true;
  }

  WD177x::~WD177x()
  {
    try {
      setDiskImageFile("", 0, 0, 0);
    }
    catch (...) {
    }
  }

  bool WD177x::setFilePosition()
  {
    if (!imageFile)
      return false;
    if (currentTrack >= nTracks || currentTrack != trackRegister ||
        currentSide >= nSides ||
        sectorRegister < 1 || sectorRegister > nSectorsPerTrack)
      return false;
    size_t  filePos =
        ((size_t(currentTrack) * size_t(nSides) + size_t(currentSide))
         * size_t(nSectorsPerTrack) + size_t(sectorRegister - 1))
        * 512;
    if (std::fseek(imageFile, long(filePos), SEEK_SET) < 0)
      return false;
    return true;
  }

  void WD177x::doStep(bool updateFlag)
  {
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
  }

  void WD177x::setDiskImageFile(const std::string& fileName_,
                        int nTracks_, int nSides_, int nSectorsPerTrack_)
  {
    if ((fileName_ == "" && imageFileName == "") ||
        (fileName_ == imageFileName &&
         nTracks_ == int(nTracks) &&
         nSides_ == int(nSides) &&
         nSectorsPerTrack_ == int(nSectorsPerTrack)))
      return;
    if (imageFile) {
      std::fclose(imageFile);
      imageFile = (std::FILE *) 0;
    }
    imageFileName = "";
    nTracks = 0;
    nSides = 0;
    nSectorsPerTrack = 0;
    writeProtectFlag = false;
    this->reset();
    diskChangeFlag = true;
    if (fileName_ == "")
      return;
    bool    nTracksValid = (nTracks_ >= 1 && nTracks_ <= 240);
    bool    nSidesValid = (nSides_ >= 1 && nSides_ <= 2);
    bool    nSectorsPerTrackValid =
                (nSectorsPerTrack_ >= 1 && nSectorsPerTrack_ <= 240);
    try {
      imageFile = std::fopen(fileName_.c_str(), "r+b");
      if (!imageFile) {
        imageFile = std::fopen(fileName_.c_str(), "rb");
        if (imageFile)
          writeProtectFlag = true;
        else
          throw Exception("wd177x: error opening disk image file");
      }
      else
        std::setvbuf(imageFile, (char *) 0, _IONBF, 0);
      long    fileSize = -1L;
      if (std::fseek(imageFile, 0L, SEEK_END) >= 0)
        fileSize = std::ftell(imageFile);
      if (!(fileSize > 0L && fileSize <= (240L * 2L * 240L * 512L))) {
        if (!(nTracksValid && nSidesValid && nSectorsPerTrackValid))
          throw Exception("wd177x: cannot determine size of disk image");
        fileSize =
            long(nTracks_) * long(nSides_) * long(nSectorsPerTrack_) * 512L;
      }
      else {
        if (nTracks_ <= 0 && nSidesValid && nSectorsPerTrackValid)
          nTracks_ =
              int(fileSize / (long(nSides_) * long(nSectorsPerTrack_) * 512L));
        else if (nSides_ <= 0 && nTracksValid && nSectorsPerTrackValid)
          nSides_ =
              int(fileSize / (long(nTracks_) * long(nSectorsPerTrack_) * 512L));
        else if (nSectorsPerTrack_ <= 0 && nTracksValid && nSidesValid)
          nSectorsPerTrack_ =
              int(fileSize / (long(nTracks_) * long(nSides_) * 512L));
      }
      if (!(nTracks_ >= 1 && nTracks_ <= 240) &&
           (nSides_ >= 1 && nSides_ <= 2) &&
           (nSectorsPerTrack_ >= 1 && nSectorsPerTrack_ <= 240))
        throw Exception("wd177x: invalid or inconsistent "
                        "disk image size parameters");
      fileSize =
          long(nTracks_) * long(nSides_) * long(nSectorsPerTrack_) * 512L;
      bool    err = true;
      if (std::fseek(imageFile, fileSize - 512L, SEEK_SET) >= 0) {
        int   i = 512;
        do {
          if (std::fgetc(imageFile) == EOF) {
            err = (i != 0);
            break;
          }
        } while (--i >= 0);
      }
      if (err)
        throw Exception("wd177x: invalid or inconsistent "
                        "disk image size parameters");
      std::fseek(imageFile, 0L, SEEK_SET);
      imageFileName = fileName_;
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
    diskChangeFlag = true;
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
      if (imageFile) {
        if (currentTrack == 0)
          statusRegister = statusRegister | 0x04;
        statusRegister = statusRegister | 0x02; // index pulse
      }
      statusRegister = statusRegister & 0xFE;   // clear busy flag
      if (!interruptRequestFlag) {
        interruptRequestFlag = true;
        interruptRequest();
      }
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
        if (!setFilePosition())
          statusRegister = statusRegister | 0x10;   // record not found
        else if (std::fread(&(buf[0]), 1, 512, imageFile) != 512)
          statusRegister = statusRegister | 0x08;   // CRC error
        else {
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
          bufPos = 0;
        }
      }
      else {                            // WRITE SECTOR
        if (writeProtectFlag)
          statusRegister = statusRegister | 0x40;   // disk is write protected
        else if (!setFilePosition())
          statusRegister = statusRegister | 0x10;   // record not found
        else {
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
          bufPos = 0;
        }
      }
      if (bufPos >= 512) {
        // on error: clear busy flag, and trigger interrupt
        statusRegister = statusRegister & 0xFE;
        if (!interruptRequestFlag) {
          interruptRequestFlag = true;
          interruptRequest();
        }
      }
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
        if (imageFile != (std::FILE *) 0 &&
            currentTrack < nTracks &&
            currentSide < nSides) {
          buf[506] = currentTrack;
          buf[507] = currentSide;
          buf[508] = 0x01;          // assume first sector of track
          buf[509] = 0x02;          // 512 bytes per sector
          buf[510] = 0xFF;          // FIXME: CRC is not implemented
          buf[511] = 0xFF;
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
      else {                            // WRITE TRACK (FIXME: unimplemented)
        statusRegister = statusRegister | 0x20;     // write error
      }
      if (bufPos >= 512) {
        // on error: clear busy flag, and trigger interrupt
        statusRegister = statusRegister & 0xFE;
        if (!interruptRequestFlag) {
          interruptRequestFlag = true;
          interruptRequest();
        }
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
      if (writeProtectFlag)
        statusRegister = statusRegister | 0x40;
      if (imageFile) {
        if (currentTrack == 0)
          statusRegister = statusRegister | 0x04;
        statusRegister = statusRegister | 0x02; // index pulse
      }
      // if a write sector command is interrupted, may need to flush buffer
      if ((commandRegister & 0xE0) == 0xA0 && (statusRegister & 0x01) != 0) {
        if (bufPos > 0) {
          for ( ; bufPos < 512; bufPos++)
            buf[bufPos] = 0;
          if (imageFile != (std::FILE *) 0 && !writeProtectFlag)
            std::fwrite(&(buf[0]), 1, 512, imageFile);
        }
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
    if (dataRequestFlag && (commandRegister & 0xE0) == 0xA0 && bufPos < 512) {
      // writing sector
      buf[bufPos++] = dataRegister;
      if (bufPos >= 512) {
        bufPos = 0;
        // clear data request and busy flag
        dataRequestFlag = false;
        statusRegister = statusRegister & 0xFC;
        if (setFilePosition()) {
          if (std::fwrite(&(buf[0]), 1, 512, imageFile) == 512) {
            if (commandRegister & 0x10) {
              // multiple sectors: continue with writing next sector
              sectorRegister++;
              writeCommandRegister(commandRegister);
              return;
            }
          }
          else
            statusRegister = statusRegister | 0x20; // write error
        }
        else
          statusRegister = statusRegister | 0x10;   // record not found
        commandRegister = 0x00;
        if (!interruptRequestFlag) {
          interruptRequestFlag = true;
          interruptRequest();
        }
      }
    }
  }

  uint8_t WD177x::readDataRegister()
  {
    if (dataRequestFlag && bufPos < 512) {
      // reading sector
      dataRegister = buf[bufPos++];
      if (bufPos >= 512) {
        bufPos = 0;
        // clear data request and busy flag
        dataRequestFlag = false;
        statusRegister = statusRegister & 0xFC;
        if ((commandRegister & 0xF0) == 0x90) {
          // multiple sectors: continue with reading next sector
          sectorRegister++;
          writeCommandRegister(commandRegister);
        }
        else {
          commandRegister = 0x00;
          if (!interruptRequestFlag) {
            interruptRequestFlag = true;
            interruptRequest();
          }
        }
      }
    }
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

  void WD177x::setSide(int n)
  {
    currentSide = uint8_t(n) & 1;
  }

  bool WD177x::getInterruptRequestFlag() const
  {
    return interruptRequestFlag;
  }

  bool WD177x::getDataRequestFlag() const
  {
    return dataRequestFlag;
  }

  void WD177x::reset()
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
    bufPos = 512;
  }

  void WD177x::interruptRequest()
  {
  }

  void WD177x::clearInterruptRequest()
  {
  }

}       // namespace Ep128Emu

