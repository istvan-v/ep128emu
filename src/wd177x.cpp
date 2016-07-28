
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
#include "wd177x.hpp"

#include <vector>

#ifdef WIN32
#  include <windows.h>
#  include <winioctl.h>
#elif defined(HAVE_LINUX_FD_H)
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <sys/ioctl.h>
#  include <linux/fd.h>
#endif

#ifdef WIN32

#define fopen ep_fopen
#define fclose ep_fclose
#define fseek ep_fseek
#define ftell ep_ftell
#define fread ep_fread
#define fwrite ep_fwrite
#define setvbuf ep_setvbuf

namespace std {

FILE* ep_fopen ( const char * filename, const char * mode )
{
	HANDLE hF= 0;

	if (!strcmp(mode,"rb"))
	{
		hF= CreateFile(filename,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING,0);
	}
	else if (!strcmp(mode,"r+b"))
	{
		hF= CreateFile(filename,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING,0);

		if (filename[0] == '\\' && filename[1] == '\\' && filename[2] == '.' && filename[3] == '\\')
		{
			DWORD bytes;

			BOOL ret= DeviceIoControl(hF,FSCTL_LOCK_VOLUME,0,0,0,0,&bytes,0);

			if (!ret)
			{
				CloseHandle(hF);

				hF= 0;
			}
		}
	}

	return (FILE*)(hF);
}

int ep_fclose ( FILE* stream )
{
	if (CloseHandle(HANDLE(stream)))
		return 0;
	else
		return EOF;
}

int ep_fseek ( FILE* stream, long int offset, int origin )
{
	DWORD method= 0;

	switch(origin)
	{
	case SEEK_SET:
		method= FILE_BEGIN;
		break;
	case SEEK_CUR:
		method= FILE_CURRENT;
		break;
	case SEEK_END:
		method= FILE_END;
		break;
	}

	DWORD ret= SetFilePointer(HANDLE(stream),offset,0,method);

	return ret!=INVALID_SET_FILE_POINTER?0:1;
}

long int ep_ftell ( FILE* stream )
{
	DWORD ret= SetFilePointer(HANDLE(stream),0,0,FILE_CURRENT);

	return ret;
}

size_t ep_fread ( void * ptr, size_t size, size_t count, FILE* stream )
{
	DWORD num= 0;

	ReadFile(HANDLE(stream),ptr,size*count,&num,0);

	return num;
}

size_t ep_fwrite ( const void * ptr, size_t size, size_t count, FILE* stream )
{
	DWORD num= 0;

	WriteFile(HANDLE(stream),ptr,size*count,&num,0);

	FlushFileBuffers(HANDLE(stream));

	return num;
}

int ep_setvbuf ( FILE* stream, char * buffer, int mode, size_t size )
{
	return 0;
}

}

#endif

namespace Ep128Emu {

  int checkFloppyDisk(const char *fileName,
                      int& nTracks, int& nSides, int& nSectorsPerTrack)
  {
#ifdef WIN32
    if (std::strlen(fileName) < 5)
      return 0;                 // return value == 0: regular file
    if (!(fileName[0] == '\\' && fileName[1] == '\\' &&
          fileName[2] == '.' && fileName[3] == '\\')) {
      return 0;
    }
    DISK_GEOMETRY diskGeometry;
    HANDLE  h = INVALID_HANDLE_VALUE;
    DWORD   tmp;
    bool    retryFlag = false;
    while (true) {
      h = CreateFileA(fileName, (DWORD) 0,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      (LPSECURITY_ATTRIBUTES) 0, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, (HANDLE) 0);
      if (h == INVALID_HANDLE_VALUE)
        return -1;              // return value == -1: error opening device
      if (DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY,
                          (LPVOID) 0, (DWORD) 0,
                          &diskGeometry, (DWORD) sizeof(diskGeometry),
                          &tmp, (LPOVERLAPPED) 0) == FALSE) {
        if (!retryFlag) {
          retryFlag = true;
          std::FILE *f = std::fopen(fileName, "rb");
          if (f)
            std::fclose(f);
          CloseHandle(h);
          continue;
        }
        CloseHandle(h);
        return -1;
      }
      break;
    }
    bool    errorFlag = false;
    if (!(diskGeometry.MediaType > Unknown &&
          diskGeometry.MediaType < RemovableMedia)) {
      errorFlag = true;
    }
    if (diskGeometry.Cylinders.QuadPart < (LONGLONG) 1 ||
        diskGeometry.Cylinders.QuadPart > (LONGLONG) 254 ||
        diskGeometry.TracksPerCylinder < (DWORD) 1 ||
        diskGeometry.TracksPerCylinder > (DWORD) 2 ||
        diskGeometry.SectorsPerTrack < (DWORD) 1 ||
        diskGeometry.SectorsPerTrack > (DWORD) 240 ||
        diskGeometry.BytesPerSector != (DWORD) 512) {
      errorFlag = true;
    }
    if (nTracks >= 1 && nTracks <= 254) {
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
      return -2;                // return value == -2: invalid disk geometry
    }
    nTracks = int(diskGeometry.Cylinders.QuadPart);
    nSides = int(diskGeometry.TracksPerCylinder);
    nSectorsPerTrack = int(diskGeometry.SectorsPerTrack);
    if (DeviceIoControl(h, IOCTL_DISK_IS_WRITABLE,
                        (LPVOID) 0, (DWORD) 0, (LPVOID) 0, (DWORD) 0,
                        &tmp, (LPOVERLAPPED) 0) == FALSE) {
      CloseHandle(h);
      return 1;                 // return value == 1: write protected disk
    }
    CloseHandle(h);
    return 2;                   // return value == 2: writable floppy disk
#elif defined(HAVE_LINUX_FD_H)
    int     fd = open(fileName, O_RDONLY);
    if (fd < 0)
      return -1;                // error opening image file
    struct floppy_drive_struct  driveState;
    struct floppy_struct  floppyParams;
    if (ioctl(fd, FDPOLLDRVSTAT, &driveState) != 0) {
      close(fd);
      return 0;                 // not a floppy device
    }
    if (ioctl(fd, FDGETPRM, &floppyParams) != 0) {
      close(fd);
      return 0;                 // not a floppy device
    }
    close(fd);
    if (floppyParams.track < 1U || floppyParams.track > 254U ||
        floppyParams.head < 1U || floppyParams.head > 2U ||
        floppyParams.sect < 1U || floppyParams.sect > 240U ||
        floppyParams.size != (floppyParams.track * floppyParams.head
                              * floppyParams.sect)) {
      return -2;
    }
    if ((nTracks >= 1 && nTracks <= 254 &&
         nTracks != int(floppyParams.track)) ||
        (nSides >= 1 && nSides <= 2 && nSides != int(floppyParams.head)) ||
        (nSectorsPerTrack >= 1 && nSectorsPerTrack <= 240 &&
         nSectorsPerTrack != int(floppyParams.sect))) {
      return -2;
    }
    nTracks = int(floppyParams.track);
    nSides = int(floppyParams.head);
    nSectorsPerTrack = int(floppyParams.sect);
    if (driveState.flags & FD_DISK_WRITABLE)
      return 2;
    return 1;
#else
    (void) fileName;
    (void) nTracks;
    (void) nSides;
    (void) nSectorsPerTrack;
    return 0;
#endif
  }

  // --------------------------------------------------------------------------

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
      bufferedTrack(0xFF),
      bufferedSide(0xFF),
      ledStateCounter(0U),
      trackBuffer((uint8_t *) 0),
      flagsBuffer((uint8_t *) 0),
      tmpBuffer((uint8_t *) 0),
      bufPos(-1L),
      trackDirtyFlag(false),
      writeTrackSectorsRemaining(0),
      writeTrackState(0xFF)
  {
    buf_.resize(0);
    this->reset();
  }

  WD177x::~WD177x()
  {
    closeDiskImage();
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
    // on error: clear data request and busy flag, and trigger interrupt
    statusRegister &= uint8_t(0xFC);
    statusRegister |= n;
    dataRequestFlag = false;
    if ((commandRegister & 0xE0) == 0xA0 || (commandRegister & 0xF0) == 0xF0) {
      if (bufPos >= 0L) {
        while (bufPos & 511L)           // partially written sector:
          trackBuffer[bufPos++] = 0x00; // fill the remaining bytes with zeros
      }
    }
    bufPos = -1L;
    commandRegister = 0x00;
    writeTrackSectorsRemaining = 0;
    writeTrackState = 0xFF;
    if (!interruptRequestFlag) {
      interruptRequestFlag = true;
      interruptRequest();
    }
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

  void WD177x::closeDiskImage()
  {
    if (!imageFile)
      return;
    (void) flushTrack();                // FIXME: errors are ignored here
    std::fclose(imageFile);
    imageFile = (std::FILE *) 0;
    nTracks = 0;
    nSides = 0;
    nSectorsPerTrack = 0;
    writeProtectFlag = false;
    bufferedTrack = 0xFF;
    bufferedSide = 0xFF;
    trackBuffer = (uint8_t *) 0;
    flagsBuffer = (uint8_t *) 0;
    tmpBuffer = (uint8_t *) 0;
    imageFileName.clear();
    buf_.resize(0);
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
    closeDiskImage();
    this->reset();
    if (fileName_ == "")
      return;
    unsigned char tmpBuf[512];
    bool    nTracksValid = (nTracks_ >= 1 && nTracks_ <= 254);
    bool    nSidesValid = (nSides_ >= 1 && nSides_ <= 2);
    bool    nSectorsPerTrackValid =
                (nSectorsPerTrack_ >= 1 && nSectorsPerTrack_ <= 240);
    bool    disableFATCheck =
        (nTracksValid && nSidesValid && nSectorsPerTrackValid);
    {
      int     diskType = checkFloppyDisk(fileName_.c_str(),
                                         nTracks_, nSides_, nSectorsPerTrack_);
      if (diskType > 0) {
        writeProtectFlag = (diskType == 1);
        nTracksValid = true;
        nSidesValid = true;
        nSectorsPerTrackValid = true;
      }
      else if (diskType == -2) {
        throw Exception("wd177x: invalid or inconsistent "
                        "disk image size parameters");
      }
      else if (diskType < 0) {
        throw Exception("wd177x: error opening disk image file");
      }
    }
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
      if (fileSize >= 512L && fileSize <= (254L * 2L * 240L * 512L)) {
        long    nSectors_ = fileSize / 512L;
        if (!nTracksValid && nSidesValid && nSectorsPerTrackValid) {
          nTracks_ = int(nSectors_ / (long(nSides_) * long(nSectorsPerTrack_)));
          nTracksValid = true;
        }
        else if (nTracksValid && !nSidesValid && nSectorsPerTrackValid) {
          nSides_ = int(nSectors_ / (long(nTracks_) * long(nSectorsPerTrack_)));
          nSidesValid = true;
        }
        else if (nTracksValid && nSidesValid && !nSectorsPerTrackValid) {
          nSectorsPerTrack_ = int(nSectors_ / (long(nTracks_) * long(nSides_)));
          nSectorsPerTrackValid = true;
        }
      }
      // try to find out geometry parameters from FAT filesystem
      if (!disableFATCheck && std::fseek(imageFile, 0L, SEEK_SET) >= 0) {
        if (std::fread(&(tmpBuf[0]), 1, 512, imageFile) == 512) {
          int     fatSectorSize = int(tmpBuf[0x0B]) | (int(tmpBuf[0x0C]) << 8);
          long    fatSectors = long(tmpBuf[0x13]) | (long(tmpBuf[0x14]) << 8);
          if (!fatSectors) {
            fatSectors =
                long(tmpBuf[0x20]) | (long(tmpBuf[0x21]) << 8)
                | (long(tmpBuf[0x22]) << 16) | (long(tmpBuf[0x23]) << 24);
          }
          int     fatSides = int(tmpBuf[0x1A]) | (int(tmpBuf[0x1B]) << 8);
          int     fatSectorsPerTrack =
              int(tmpBuf[0x18]) | (int(tmpBuf[0x19]) << 8);
          if ((tmpBuf[0x15] == 0xF0 || tmpBuf[0x15] >= 0xF8) &&
              (tmpBuf[0x0D] & 0x7F) != 0 &&
              (tmpBuf[0x0D] & (tmpBuf[0x0D] - 1)) == 0 &&
              fatSectorSize == 512 && fatSectors >= 1L &&
              fatSides >= 1 && fatSides <= 2 &&
              fatSectorsPerTrack >= 1 && fatSectorsPerTrack <= 240) {
            if ((fatSectors % long(fatSides * fatSectorsPerTrack)) == 0L) {
              long    fatTracks =
                  fatSectors / long(fatSides * fatSectorsPerTrack);
              if (fatTracks >= 1L && fatTracks <= 254L) {
                // found a valid FAT header, set or check geometry parameters
                if (!nTracksValid)
                  nTracks_ = int(fatTracks);
                if (!nSidesValid)
                  nSides_ = fatSides;
                if (!nSectorsPerTrackValid)
                  nSectorsPerTrack_ = fatSectorsPerTrack;
                if (nTracks_ != int(fatTracks) || nSides_ != fatSides ||
                    nSectorsPerTrack_ != fatSectorsPerTrack) {
                  throw Exception("wd177x: invalid or inconsistent "
                                  "disk image size parameters");
                }
              }
            }
          }
        }
      }
      if (!(nTracks_ >= 1 && nTracks_ <= 254 &&
            nSides_ >= 1 && nSides_ <= 2 &&
            nSectorsPerTrack_ >= 1 && nSectorsPerTrack_ <= 240))
        throw Exception("wd177x: cannot determine size of disk image");
      fileSize = long(nTracks_ * nSides_) * long(nSectorsPerTrack_) * 512L;
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
      buf_.resize(size_t(nSectorsPerTrack_) * 257);
    }
    catch (...) {
      closeDiskImage();
      throw;
    }
    nTracks = uint8_t(nTracks_);
    nSides = uint8_t(nSides_);
    nSectorsPerTrack = uint8_t(nSectorsPerTrack_);
    trackBuffer = reinterpret_cast< uint8_t * >(&(buf_.front()));
    flagsBuffer = &(trackBuffer[size_t(nSectorsPerTrack) * 512]);
    tmpBuffer = &(trackBuffer[size_t(nSectorsPerTrack) * 516]);
    clearFlagsBuffer();
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
      // set motor on and busy flag
      // reset CRC, seek error, data request, and IRQ
      dataRequestFlag = false;
      statusRegister = 0xA1;
      ledStateCounter = ledStateCount1;
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
      // set motor on and busy flag
      // reset data request, lost data, record not found, and interrupt request
      dataRequestFlag = false;
      statusRegister = 0x81;
      ledStateCounter = ledStateCount1;
      if (interruptRequestFlag) {
        interruptRequestFlag = false;
        clearInterruptRequest();
      }
      // select side (if enabled)
      if (isWD1773) {
        if (pc)
          currentSide = uint8_t(hs ? 1 : 0);
      }
      bufPos = -1L;
      if (!imageFile) {
        statusRegister = statusRegister | 0x10;     // record not found
        return;
      }
      if ((n & 0x20) == 0) {            // READ SECTOR
        if (!checkDiskPosition()) {
          statusRegister = statusRegister | 0x10;   // record not found
        }
        else {
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
          bufPos = long(sectorRegister - 1) * 512L;
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
          bufPos = long(sectorRegister - 1) * 512L;
        }
      }
      if (bufPos < 0L)
        setError();
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
      ledStateCounter = ledStateCount1;
      if (interruptRequestFlag) {
        interruptRequestFlag = false;
        clearInterruptRequest();
      }
      bufPos = -1L;
      if (!imageFile) {
        statusRegister = statusRegister | 0x10;     // record not found
        return;
      }
      if ((n & 0x20) == 0) {            // READ ADDRESS
        if (checkDiskPosition(true)) {
          bufPos = 0L;
          dataRequestFlag = true;
          statusRegister = statusRegister | 0x02;
        }
        else {
          statusRegister = statusRegister | 0x10;   // record not found
        }
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
          bufPos = 0L;
          writeTrackSectorsRemaining = nSectorsPerTrack;
          writeTrackState = 0;
        }
      }
      if (bufPos < 0L)
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
      if ((commandRegister & 0xE0) == 0xA0 ||
          (commandRegister & 0xF0) == 0xF0) {
        if (bufPos >= 0L) {
          while (bufPos & 511L) {       // partially written sector:
            trackBuffer[bufPos] = 0x00; // fill the remaining bytes with zeros
            bufPos++;
          }
        }
      }
      bufPos = -1L;
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
    if ((commandRegister & 0xE0) == 0xA0 && bufPos >= 0L) {
      // writing sector
      if (currentTrack != bufferedTrack || currentSide != bufferedSide)
        (void) updateBufferedTrack();   // FIXME: errors are ignored here
      trackDirtyFlag = true;
      trackBuffer[bufPos] = dataRegister;
      flagsBuffer[bufPos >> 9] = 0x81;
      bufPos++;
      if (!(bufPos & 511L)) {
        bufPos = -1L;
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
            bufPos = long(n - 1) * 512L;
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
          if (currentTrack != bufferedTrack || currentSide != bufferedSide)
            (void) updateBufferedTrack();   // FIXME: errors are ignored here
          trackDirtyFlag = true;
          trackBuffer[bufPos] = dataRegister;
          flagsBuffer[bufPos >> 9] = 0x81;
          bufPos++;
          if (!(bufPos & 511L))
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
    if (!(dataRequestFlag && bufPos >= 0L))
      return dataRegister;
    if ((commandRegister & 0xE0) == 0x80) {
      // reading sector
      if (currentTrack != bufferedTrack || currentSide != bufferedSide)
        (void) updateBufferedTrack();   // FIXME: errors are ignored here
      if (!flagsBuffer[bufPos >> 9]) {
        if (!readTrack()) {
          setError(0x08);               // CRC error
          dataRegister = 0x00;
          return 0x00;
        }
      }
      dataRegister = trackBuffer[bufPos++];
      if (!(bufPos & 511L)) {
        bufPos = -1L;
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
      case 0L:
        dataRegister = currentTrack;
        sectorRegister = dataRegister;
        break;
      case 1L:
        dataRegister = currentSide;
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
          tmpBuf[0] = currentTrack;
          tmpBuf[1] = currentSide;
          tmpBuf[2] = 0x01;
          tmpBuf[3] = 0x02;
          uint16_t  tmp = calculateCRC(&(tmpBuf[0]), 4, 0xB230);
          if (bufPos == 4L)
            dataRegister = uint8_t(tmp >> 8);
          else
            dataRegister = uint8_t(tmp & 0xFF);
        }
        break;
      }
      if (++bufPos >= 6L)
        setError();
    }
    return dataRegister;
  }

  uint8_t WD177x::readStatusRegisterDebug() const
  {
    uint8_t n = statusRegister;
    if (isWD1773)
      n = n & 0x7F;             // always ready
    return n;
  }

  uint8_t WD177x::readDataRegisterDebug() const
  {
    return dataRegister;
  }

  void WD177x::clearDiskChangeFlag()
  {
    diskChangeFlag = false;
  }

  void WD177x::setIsWD1773(bool isEnabled)
  {
    isWD1773 = isEnabled;
  }

  void WD177x::setEnableBusyFlagHack(bool isEnabled)
  {
    busyFlagHackEnabled = isEnabled;
  }

  void WD177x::copySector(int n)
  {
    if (n < 1 || n > int(nSectorsPerTrack) || flagsBuffer[n - 1] != 0x00)
      return;
    size_t    offs = size_t(n - 1) * 512;
    uint32_t  *srcPtr = reinterpret_cast< uint32_t * >(&(tmpBuffer[offs]));
    uint32_t  *dstPtr = reinterpret_cast< uint32_t * >(&(trackBuffer[offs]));
    for (size_t i = 0; i < 128; i += 4) {
      dstPtr[i] = srcPtr[i];
      dstPtr[i + 1] = srcPtr[i + 1];
      dstPtr[i + 2] = srcPtr[i + 2];
      dstPtr[i + 3] = srcPtr[i + 3];
    }
    flagsBuffer[n - 1] = 0x01;
  }

  void WD177x::clearDirtyFlag()
  {
    if (!trackDirtyFlag)
      return;
    for (size_t i = 0; i < size_t(nSectorsPerTrack); i++)
      flagsBuffer[i] &= uint8_t(0x01);
    trackDirtyFlag = false;
  }

  void WD177x::clearFlagsBuffer()
  {
    for (size_t i = 0; i < size_t(nSectorsPerTrack); i++)
      flagsBuffer[i] = 0x00;
    trackDirtyFlag = false;
  }

  void WD177x::clearBuffer(uint8_t *buf)
  {
    uint32_t  *p = reinterpret_cast< uint32_t * >(buf);
    size_t    n = size_t(nSectorsPerTrack) * 128;
    uint32_t  c = 0U;
    for (size_t i = 0; i < n; i += 4) {
      p[i] = c;
      p[i + 1] = c;
      p[i + 2] = c;
      p[i + 3] = c;
    }
  }

  bool WD177x::updateBufferedTrack()
  {
    if (currentTrack == bufferedTrack && currentSide == bufferedSide)
      return true;
    bool    retval = flushTrack();
    bufferedTrack = currentTrack;
    bufferedSide = currentSide;
    clearFlagsBuffer();
    return retval;
  }

  bool WD177x::readTrack()
  {
    if (!imageFile)
      return false;
    uint8_t firstSector = 0;
    uint8_t lastSector = 0;
    for (uint8_t i = 1; i <= nSectorsPerTrack; i++) {
      if (!flagsBuffer[i - 1]) {
        if (!firstSector)
          firstSector = i;
        lastSector = i;
      }
    }
    bool    errorFlag = false;
    if (currentTrack >= nTracks || currentSide >= nSides)
      errorFlag = true;
    if (!firstSector)
      return (!errorFlag);
    clearBuffer(tmpBuffer);
    if (!errorFlag) {
      size_t  offs = size_t(firstSector - 1) * 512;
      size_t  nBytes = size_t(lastSector + 1 - firstSector) * 512;
      long    filePos = (long(currentTrack) * long(nSides) + long(currentSide))
                        * long(nSectorsPerTrack);
      filePos = (filePos * 512L) + long(offs);
      if (std::fseek(imageFile, filePos, SEEK_SET) < 0) {
        errorFlag = true;
      }
      else {
        size_t  bytesRead =
            std::fread(&(tmpBuffer[offs]), sizeof(uint8_t), nBytes, imageFile);
        errorFlag = (bytesRead != nBytes);
      }
    }
    for (uint8_t i = firstSector; i <= lastSector; i++)
      copySector(i);
    return (!errorFlag);
  }

  bool WD177x::flushTrack()
  {
    if (!trackDirtyFlag)
      return true;
    if (bufferedTrack >= nTracks || bufferedSide >= nSides ||
        writeProtectFlag || !imageFile) {
      clearDirtyFlag();
      return false;
    }
    uint8_t lastSector = 0;
    bool    errorFlag = false;
    while (true) {
      uint8_t firstSector = 0;
      for (uint8_t i = lastSector + 1; i <= nSectorsPerTrack; i++) {
        uint8_t tmp = flagsBuffer[i - 1];
        if (tmp & 0x80) {
          if (!firstSector)
            firstSector = i;
          lastSector = i;
        }
        else if (!tmp) {
          if (firstSector)
            break;
        }
      }
      if (!firstSector) {
        clearDirtyFlag();
        return (!errorFlag);
      }
      size_t  offs = size_t(firstSector - 1) * 512;
      size_t  nBytes = size_t(lastSector + 1 - firstSector) * 512;
      long    filePos =
          (long(bufferedTrack) * long(nSides) + long(bufferedSide))
          * (long(nSectorsPerTrack) * 512L)
          + long(offs);
      if (std::fseek(imageFile, filePos, SEEK_SET) < 0) {
        errorFlag = true;
      }
      else {
        size_t  bytesWritten = std::fwrite(&(trackBuffer[offs]),
                                           sizeof(uint8_t), nBytes, imageFile);
        if (bytesWritten != nBytes)
          errorFlag = true;
      }
    }
    return false;       // not reached
  }

  uint8_t WD177x::getLEDState_()
  {
    if (ledStateCounter == 0U)
      return 0x00;
    if (ledStateCounter > ledStateCount1) {
      if (!trackDirtyFlag) {
        ledStateCounter = 0U;
        return 0x00;
      }
      if (--ledStateCounter == ledStateCount1) {
        ledStateCounter++;
        (void) flushTrack();            // FIXME: errors are ignored here
        ledStateCounter = 0U;
        return 0x00;
      }
      return 0x01;
    }
    if (statusRegister & 0x01) {
      ledStateCounter = ledStateCount1;
    }
    else if (--ledStateCounter == 0U) {
      statusRegister = statusRegister & 0x7F;   // reset motor on flag
      if (trackDirtyFlag) {
        ledStateCounter = ledStateCount2;
        return 0x01;
      }
      return 0x00;
    }
    return 0x03;
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
    (void) flushTrack();                // FIXME: errors are ignored here
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
    bufferedTrack = 0xFF;
    bufferedSide = 0xFF;
    ledStateCounter = (imageFile != (std::FILE *) 0 ? ledStateCount1 : 0U);
    bufPos = -1L;
    writeTrackSectorsRemaining = 0;
    writeTrackState = 0xFF;
    clearFlagsBuffer();
  }

  void WD177x::interruptRequest()
  {
  }

  void WD177x::clearInterruptRequest()
  {
  }

}       // namespace Ep128Emu

