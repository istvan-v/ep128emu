
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2019 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "system.hpp"

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

namespace std {

  FILE *ep_fopen(const char *filename, const char *mode);
  int ep_fclose(FILE *stream);
  int ep_fseek(FILE *stream, long int offset, int origin);
  long int ep_ftell(FILE *stream);
  size_t ep_fread(void *ptr, size_t size, size_t count, FILE *stream);
  size_t ep_fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
  int ep_setvbuf(FILE *stream, char *buffer, int mode, size_t size);

}       // namespace std

#define fopen ep_fopen
#define fclose ep_fclose
#define fseek ep_fseek
#define ftell ep_ftell
#define fread ep_fread
#define fwrite ep_fwrite
#define setvbuf ep_setvbuf

#endif  // WIN32

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

  FloppyDrive::FloppyDrive()
    : imageFileName(""),
      imageFile((std::FILE *) 0),
      nTracks(0),
      nSides(0),
      nSectorsPerTrack(0),
      currentTrack(0),
      currentSide(0),
      writeProtectFlag(false),
      diskChangeFlag(false),
      bufferedTrack(0xFF),
      bufferedSide(0xFF),
      motorOnInput(false),
      isMotorOn(false),
      ledStateCounter(0U),
      trackBuffer((uint8_t *) 0),
      flagsBuffer((uint8_t *) 0),
      tmpBuffer((uint8_t *) 0),
      bufPos(-1L),
      trackDirtyFlag(false)
  {
    buf_.resize(0);
    this->reset();
  }

  FloppyDrive::~FloppyDrive()
  {
    closeDiskImage();
  }

  void FloppyDrive::doStep(int n)
  {
    int     tmp = int(currentTrack) + n;
    int     maxTrack =
        (nTracks > 80 ? (nTracks < 254 ? (nTracks + 1) : 254) : 81);
    currentTrack = uint8_t(tmp > 0 ? (tmp < maxTrack ? tmp : maxTrack) : 0);
  }

  bool FloppyDrive::startDataTransfer(uint8_t trackNum, uint8_t sectorNum)
  {
    if (!(isMotorOn && currentTrack < nTracks && currentSide < nSides &&
          currentTrack == trackNum &&
          sectorNum >= 1 && sectorNum <= nSectorsPerTrack)) {
      return false;
    }
    bufPos = long(sectorNum - 1) << 9;
    return true;
  }

  void FloppyDrive::closeDiskImage()
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

  void FloppyDrive::setDiskImageFile(const std::string& fileName_,
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
    diskChangeFlag = true;
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
        throw Exception("FDD: invalid or inconsistent "
                        "disk image size parameters");
      }
      else if (diskType < 0) {
        throw Exception("FDD: error opening disk image file");
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
          throw Exception("FDD: error opening disk image file");
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
                  throw Exception("FDD: invalid or inconsistent "
                                  "disk image size parameters");
                }
              }
            }
          }
        }
      }
      if (!(nTracks_ >= 1 && nTracks_ <= 254 &&
            nSides_ >= 1 && nSides_ <= 2 &&
            nSectorsPerTrack_ >= 1 && nSectorsPerTrack_ <= 240)) {
        if (!(nTracksValid | nSidesValid | nSectorsPerTrackValid
              | (fileSize != (80L * 2 * 9 * 512)))) {
          nTracks_ = 80;
          nSides_ = 2;
          nSectorsPerTrack_ = 9;
        }
        else {
          throw Exception("FDD: cannot determine size of disk image");
        }
      }
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
        throw Exception("FDD: invalid or inconsistent "
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

  void FloppyDrive::padSector()
  {
    if (bufPos < 0L || !(bufPos & 511L))
      return;
    do {
      trackBuffer[bufPos] = 0x00;
      bufPos++;
    } while (bufPos & 511L);
    bufPos = -1L;
  }

  void FloppyDrive::copySector(int n)
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

  void FloppyDrive::clearDirtyFlag()
  {
    if (!trackDirtyFlag)
      return;
    for (size_t i = 0; i < size_t(nSectorsPerTrack); i++)
      flagsBuffer[i] &= uint8_t(0x01);
    trackDirtyFlag = false;
  }

  void FloppyDrive::clearFlagsBuffer()
  {
    for (size_t i = 0; i < size_t(nSectorsPerTrack); i++)
      flagsBuffer[i] = 0x00;
    trackDirtyFlag = false;
  }

  void FloppyDrive::clearBuffer(uint8_t *buf)
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

  bool FloppyDrive::updateBufferedTrack_()
  {
    bool    retval = flushTrack();
    bufferedTrack = currentTrack;
    bufferedSide = currentSide;
    clearFlagsBuffer();
    return retval;
  }

  bool FloppyDrive::readTrack()
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

  bool FloppyDrive::flushTrack()
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

  uint8_t FloppyDrive::getLEDState_()
  {
    if (EP128EMU_UNLIKELY(ledStateCounter == 0U))
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
    if (motorOnInput) {
      if (ledStateCounter < ledStateCount1)
        ledStateCounter++;
    }
    else if (--ledStateCounter == 0U) {
      isMotorOn = false;
      if (trackDirtyFlag) {
        ledStateCounter = ledStateCount2;
        return 0x01;
      }
      return 0x00;
    }
    return 0x03;
  }

  void FloppyDrive::reset()
  {
    (void) flushTrack();                // FIXME: errors are ignored here
    currentTrack = 0;
    currentSide = 0;
    bufferedTrack = 0xFF;
    bufferedSide = 0xFF;
    if (imageFile) {
      isMotorOn = true;
      ledStateCounter = ledStateCount1;
    }
    else {
      isMotorOn = false;
      ledStateCounter = 0U;
    }
    bufPos = -1L;
    clearFlagsBuffer();
  }

}       // namespace Ep128Emu

#ifdef WIN32

#undef fopen
#undef fclose
#undef fseek
#undef ftell
#undef fread
#undef fwrite
#undef setvbuf

namespace std {

  FILE *ep_fopen(const char *filename, const char *mode)
  {
    wchar_t tmpBuf[512];
    wchar_t *fileName_ = &(tmpBuf[0]);
    Ep128Emu::convertUTF8(fileName_, filename, 512);
    HANDLE  hF = 0;

    if (!strcmp(mode, "rb")) {
      hF = CreateFileW(LPCWSTR(fileName_), GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH
                       | FILE_FLAG_NO_BUFFERING, 0);
    }
    else if (!strcmp(mode, "r+b")) {
      hF = CreateFileW(LPCWSTR(fileName_), GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH
                       | FILE_FLAG_NO_BUFFERING, 0);

      if (filename[0] == '\\' && filename[1] == '\\' &&
          filename[2] == '.' && filename[3] == '\\') {
        DWORD bytes;
        BOOL  ret = DeviceIoControl(hF, FSCTL_LOCK_VOLUME,
                                    0, 0, 0, 0, &bytes, 0);

        if (!ret) {
          CloseHandle(hF);
          hF = 0;
        }
      }
    }

    return (FILE *) hF;
  }

  int ep_fclose(FILE *stream)
  {
    if (CloseHandle(HANDLE(stream)))
      return 0;
    else
      return EOF;
  }

  int ep_fseek(FILE *stream, long int offset, int origin)
  {
    DWORD method = 0;

    switch (origin) {
    case SEEK_SET:
      method = FILE_BEGIN;
      break;
    case SEEK_CUR:
      method = FILE_CURRENT;
      break;
    case SEEK_END:
      method = FILE_END;
      break;
    }

    DWORD ret = SetFilePointer(HANDLE(stream), offset, 0, method);

    return (ret != INVALID_SET_FILE_POINTER ? 0 : 1);
  }

  long int ep_ftell(FILE *stream)
  {
    DWORD ret = SetFilePointer(HANDLE(stream), 0, 0, FILE_CURRENT);
    return ret;
  }

  size_t ep_fread(void *ptr, size_t size, size_t count, FILE *stream)
  {
    DWORD num = 0;
    ReadFile(HANDLE(stream), ptr, size * count, &num, 0);
    return num;
  }

  size_t ep_fwrite(const void *ptr, size_t size, size_t count, FILE *stream)
  {
    DWORD num = 0;
    WriteFile(HANDLE(stream), ptr, size * count, &num, 0);
    FlushFileBuffers(HANDLE(stream));
    return num;
  }

  int ep_setvbuf(FILE *stream, char *buffer, int mode, size_t size)
  {
    return 0;
  }

}       // namespace std

#endif  // WIN32

