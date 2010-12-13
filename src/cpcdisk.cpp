
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
#include "cpcdisk.hpp"
#include "wd177x.hpp"
#include "system.hpp"

static const char *dskFileHeader = "MV - CPCEMU Disk-File\r\nDisk-Info\r\n";
static const char *dskTrackHeader = "Track-Info\r\n";
static const char *extFileHeader = "EXTENDED CPC DSK File\r\nDisk-Info\r\n";

namespace CPC464 {

  void CPCDiskImage::readImageFile(uint8_t *buf, size_t filePos, size_t nBytes)
  {
    if (std::fseek(imageFile, long(filePos), SEEK_SET) < 0)
      throw Ep128Emu::Exception("error seeking CPC disk image file");
    if (std::fread(buf, sizeof(uint8_t), nBytes, imageFile) != nBytes)
      throw Ep128Emu::Exception("error reading CPC disk image file");
  }

  void CPCDiskImage::parseDSKFileHeaders(uint8_t *buf, size_t fileSize)
  {
    size_t  nTracks = size_t(nCylinders * nSides);
    size_t  trackSize = size_t(buf[50]) | (size_t(buf[51]) << 8);
    if (trackSize < 256)
      throw Ep128Emu::Exception("invalid CPC disk image file header");
    if ((nTracks * trackSize + 256) > fileSize)
      throw Ep128Emu::Exception("unexpected end of CPC disk image file");
    trackTable = new CPCDiskTrackInfo[nTracks];
    size_t  nSectorsTotal = 0;
    for (int i = 0; i < int(nTracks); i++) {
      // read all track headers, and calculate the total number of sectors
      trackTable[i].sectorTable = (CPCDiskSectorInfo *) 0;
      size_t  filePos = size_t(i) * trackSize + 256;
      readImageFile(buf, filePos, 256);
      for (int j = 0; j < int(sizeof(dskTrackHeader) / sizeof(char)); j++) {
        if (buf[j] != uint8_t(dskTrackHeader[j])) {
          throw Ep128Emu::Exception("invalid track header "
                                    "in CPC disk image file");
        }
      }
      size_t  sectorSize = size_t(0x80) << (buf[20] & 0x07);
      if (sectorSize > 0x1800)
        sectorSize = 0x1800;
      size_t  nSectors = buf[21];
      if (nSectors > 29 || (nSectors * sectorSize + 256) > trackSize) {
        throw Ep128Emu::Exception("invalid track header "
                                  "in CPC disk image file");
      }
      nSectorsTotal += nSectors;
      trackTable[i].nSectors = uint8_t(nSectors);
      trackTable[i].gapLen = buf[22];
      trackTable[i].fillerByte = buf[23];
      trackTable[i].sectorTableFileOffset = uint32_t(filePos + 24);
    }
    sectorTableBuf = new CPCDiskSectorInfo[nSectorsTotal];
    size_t  sectorTableBufPos = 0;
    for (int i = 0; i < int(nTracks); i++) {
      // read all sector headers
      size_t  filePos = size_t(i) * trackSize + 256;
      readImageFile(buf, filePos, 256);
      filePos = filePos + 256;
      buf[20] = buf[20] & 0x07;
      size_t  sectorSize = size_t(0x80) << buf[20];
      if (sectorSize > 0x1800)
        sectorSize = 0x1800;
      trackTable[i].sectorTable = &(sectorTableBuf[sectorTableBufPos]);
      for (int j = 0; j < int(trackTable[i].nSectors); j++) {
        if ((buf[(j << 3) + 27] & 0x07) > buf[20]) {
          throw Ep128Emu::Exception("invalid sector header "
                                    "in CPC disk image file");
        }
        sectorTableBuf[sectorTableBufPos].fileOffset = uint32_t(filePos);
        sectorTableBuf[sectorTableBufPos].dataSize =
            uint32_t(0x80U << (buf[(j << 3) + 27] & 0x07));
        if (sectorTableBuf[sectorTableBufPos].dataSize > 0x1800U)
          sectorTableBuf[sectorTableBufPos].dataSize = 0x1800U;
        sectorTableBuf[sectorTableBufPos].trackNum = buf[(j << 3) + 24];
        sectorTableBuf[sectorTableBufPos].sideNum = buf[(j << 3) + 25];
        sectorTableBuf[sectorTableBufPos].sectorNum = buf[(j << 3) + 26];
        sectorTableBuf[sectorTableBufPos].sectorSizeCode = buf[(j << 3) + 27];
        sectorTableBuf[sectorTableBufPos].statusRegister1 =
            buf[(j << 3) + 28] & 0xA5;
        sectorTableBuf[sectorTableBufPos].statusRegister2 =
            buf[(j << 3) + 29] & 0x61;
        filePos = filePos + sectorSize;
        sectorTableBufPos++;
      }
    }
  }

  void CPCDiskImage::parseEXTFileHeaders(uint8_t *buf, size_t fileSize)
  {
    int     nTracks = nCylinders * nSides;
    if (nTracks > 204) {
      throw Ep128Emu::Exception("invalid number of tracks "
                                "in CPC disk image file");
    }
    uint8_t trackSizeMSBTable[204];
    size_t  fileSizeRequired = 256;
    for (int i = 0; i < nTracks; i++) {
      uint8_t trackSizeMSB = buf[i + 52];
      trackSizeMSBTable[i] = trackSizeMSB;
      fileSizeRequired = fileSizeRequired + (size_t(trackSizeMSB) << 8);
    }
    if (fileSizeRequired > fileSize)
      throw Ep128Emu::Exception("unexpected end of CPC disk image file");
    trackTable = new CPCDiskTrackInfo[size_t(nTracks)];
    size_t  filePos = 256;
    size_t  nSectorsTotal = 0;
    for (int i = 0; i < nTracks; i++) {
      // read all track headers, and calculate the total number of sectors
      trackTable[i].sectorTable = (CPCDiskSectorInfo *) 0;
      if (trackSizeMSBTable[i] < 1) {
        // empty track
        trackTable[i].nSectors = 0;
        trackTable[i].gapLen = 0x4E;
        trackTable[i].fillerByte = 0xE5;
        trackTable[i].sectorTableFileOffset = 0U;
        continue;
      }
      readImageFile(buf, filePos, 256);
      for (int j = 0; j < int(sizeof(dskTrackHeader) / sizeof(char)); j++) {
        if (buf[j] != uint8_t(dskTrackHeader[j])) {
          throw Ep128Emu::Exception("invalid track header "
                                    "in CPC disk image file");
        }
      }
      if (buf[21] > 29) {
        throw Ep128Emu::Exception("invalid track header "
                                  "in CPC disk image file");
      }
      nSectorsTotal += size_t(buf[21]);
      trackTable[i].nSectors = buf[21];
      trackTable[i].gapLen = buf[22];
      trackTable[i].fillerByte = buf[23];
      trackTable[i].sectorTableFileOffset = uint32_t(filePos + 24);
      filePos = filePos + (size_t(trackSizeMSBTable[i]) << 8);
    }
    sectorTableBuf = new CPCDiskSectorInfo[nSectorsTotal];
    filePos = 256;
    size_t  sectorTableBufPos = 0;
    for (int i = 0; i < nTracks; i++) {
      // read all sector headers
      if (trackSizeMSBTable[i] < 1)
        continue;
      readImageFile(buf, filePos, 256);
      size_t  nextFilePos = filePos + (size_t(trackSizeMSBTable[i]) << 8);
      filePos = filePos + 256;
      buf[20] = buf[20] & 0x07;
      trackTable[i].sectorTable = &(sectorTableBuf[sectorTableBufPos]);
      for (int j = 0; j < int(trackTable[i].nSectors); j++) {
        if ((buf[(j << 3) + 27] & 0x07) > buf[20]) {
          throw Ep128Emu::Exception("invalid sector header "
                                    "in CPC disk image file");
        }
        uint32_t  sectorSize = uint32_t(0x80U << (buf[(j << 3) + 27] & 0x07));
        uint32_t  dataSize =
            uint32_t(buf[(j << 3) + 30]) | (uint32_t(buf[(j << 3) + 31]) << 8);
        if (dataSize < 1U ||
            ((dataSize & (sectorSize - 1U)) != 0U &&
             !(dataSize >= 0x1000U && dataSize < sectorSize))) {
          throw Ep128Emu::Exception("invalid sector header "
                                    "in CPC disk image file");
        }
        sectorTableBuf[sectorTableBufPos].fileOffset = uint32_t(filePos);
        sectorTableBuf[sectorTableBufPos].dataSize = dataSize;
        sectorTableBuf[sectorTableBufPos].trackNum = buf[(j << 3) + 24];
        sectorTableBuf[sectorTableBufPos].sideNum = buf[(j << 3) + 25];
        sectorTableBuf[sectorTableBufPos].sectorNum = buf[(j << 3) + 26];
        sectorTableBuf[sectorTableBufPos].sectorSizeCode = buf[(j << 3) + 27];
        sectorTableBuf[sectorTableBufPos].statusRegister1 =
            buf[(j << 3) + 28] & 0xA5;
        sectorTableBuf[sectorTableBufPos].statusRegister2 =
            buf[(j << 3) + 29] & 0x61;
        filePos = filePos + size_t(dataSize);
        if (filePos > nextFilePos) {
          throw Ep128Emu::Exception("unexpected end of track data "
                                    "in CPC disk image file");
        }
        sectorTableBufPos++;
      }
      filePos = nextFilePos;
    }
  }

  bool CPCDiskImage::openFloppyDevice(const char *fileName)
  {
    // file name is assumed to be non-NULL and non-empty
    nCylinders = 0;
    nSides = 0;
    int     nSectorsPerTrack = 0;
    int     err = Ep128Emu::checkFloppyDisk(fileName, nCylinders, nSides,
                                            nSectorsPerTrack);
    if (err > 0) {
      if (err == 1)
        imageFile = std::fopen(fileName, "rb");
      else
        imageFile = std::fopen(fileName, "r+b");
      if (!imageFile) {
        err = -1;
      }
      else {
        std::setvbuf(imageFile, (char *) 0, _IONBF, 0);
        err = -err;
        if (std::fseek(imageFile,
                       long(nCylinders * nSides) * long(nSectorsPerTrack)
                       * 512L - 512L, SEEK_SET) >= 0) {
          uint8_t tmpBuf[512];
          if (std::fread(&(tmpBuf[0]), 1, 512, imageFile) == 512) {
            if (std::fread(&(tmpBuf[0]), 1, 512, imageFile) == 0)
              err = -err;
          }
        }
        if (err < 0)
          err = -2;
        else
          std::fseek(imageFile, 0L, SEEK_SET);
      }
    }
    if (err <= 0) {
      nCylinders = 0;
      nSides = 0;
      if (err < 0) {
        if (err == -2) {
          throw Ep128Emu::Exception("CPCDiskImage: "
                                    "invalid or unknown disk geometry");
        }
        throw Ep128Emu::Exception("error opening CPC disk image or device");
      }
      return false;
    }
    writeProtectFlag = (err == 1);
    int     nTracks = nCylinders * nSides;
    trackTable = new CPCDiskTrackInfo[size_t(nTracks)];
    sectorTableBuf = new CPCDiskSectorInfo[size_t(nTracks * nSectorsPerTrack)];
    for (int i = 0; i < nTracks; i++) {
      int     sectorTableBufPos = i * nSectorsPerTrack;
      trackTable[i].sectorTable = &(sectorTableBuf[sectorTableBufPos]);
      trackTable[i].nSectors = uint8_t(nSectorsPerTrack);
      trackTable[i].gapLen = 0x52;
      trackTable[i].fillerByte = 0xE5;
      trackTable[i].sectorTableFileOffset = 0U;
      for (int j = 0; j < nSectorsPerTrack; j++) {
        sectorTableBuf[sectorTableBufPos].fileOffset =
            uint32_t(sectorTableBufPos) * 512U;
        sectorTableBuf[sectorTableBufPos].dataSize = 512U;
        sectorTableBuf[sectorTableBufPos].trackNum = uint8_t(i / nSides);
        sectorTableBuf[sectorTableBufPos].sideNum = uint8_t(i % nSides);
        sectorTableBuf[sectorTableBufPos].sectorNum = uint8_t(j + 1);
        sectorTableBuf[sectorTableBufPos].sectorSizeCode = 0x02;
        sectorTableBuf[sectorTableBufPos].statusRegister1 = 0x00;
        sectorTableBuf[sectorTableBufPos].statusRegister2 = 0x00;
        sectorTableBufPos++;
      }
    }
    return true;
  }

  // --------------------------------------------------------------------------

  CPCDiskImage::CPCDiskImage()
    : trackTable((CPCDiskTrackInfo *) 0),
      sectorTableBuf((CPCDiskSectorInfo *) 0),
      imageFile((std::FILE *) 0),
      nCylinders(0),
      nSides(0),
      writeProtectFlag(true),
      currentCylinder(1),
      randomSeed(0)
  {
    Ep128Emu::setRandomSeed(randomSeed,
                            uint32_t(uintptr_t((void *) this) & 0xFFFFFFFFUL));
    Ep128Emu::setRandomSeed(randomSeed,
                            uint32_t(Ep128Emu::getRandomNumber(randomSeed))
                            ^ Ep128Emu::Timer::getRandomSeedFromTime());
  }

  CPCDiskImage::~CPCDiskImage()
  {
    // close image file
    openDiskImage((char *) 0);
  }

  void CPCDiskImage::openDiskImage(const char *fileName)
  {
    // close any previous image file first
    if (imageFile)
      std::fclose(imageFile);           // FIXME: errors are ignored here
    imageFile = (std::FILE *) 0;
    if (trackTable)
      delete[] trackTable;
    trackTable = (CPCDiskTrackInfo *) 0;
    if (sectorTableBuf)
      delete[] sectorTableBuf;
    sectorTableBuf = (CPCDiskSectorInfo *) 0;
    nCylinders = 0;
    nSides = 0;
    writeProtectFlag = true;
    // empty or NULL file name: close image
    if (fileName == (char *) 0 || fileName[0] == '\0')
      return;
    try {
      if (openFloppyDevice(fileName))
        return;
      // open image file
      imageFile = std::fopen(fileName, "r+b");
      if (!imageFile) {
        imageFile = std::fopen(fileName, "rb");
        if (!imageFile)
          throw Ep128Emu::Exception("error opening CPC disk image file");
      }
      else {
        writeProtectFlag = false;
      }
      std::setvbuf(imageFile, (char *) 0, _IONBF, 0);
      if (std::fseek(imageFile, 0L, SEEK_END) < 0)
        throw Ep128Emu::Exception("error seeking CPC disk image file");
      long    fileSize = std::ftell(imageFile);
      if (fileSize < 512L)
        throw Ep128Emu::Exception("invalid CPC disk image file");
      // check file header
      uint8_t tmpBuf[256];
      readImageFile(&(tmpBuf[0]), 0, 256);
      bool    isExtendedFormat = false;
      for (int i = 0; i < 8; i++) {
        if (tmpBuf[i] != uint8_t(dskFileHeader[i])) {
          for (int j = 0; j < 8; j++) {
            if (tmpBuf[j] != uint8_t(extFileHeader[j]))
              throw Ep128Emu::Exception("unknown CPC disk image file format");
          }
          isExtendedFormat = true;
          break;
        }
      }
      nCylinders = tmpBuf[48];
      nSides = tmpBuf[49];
      if (nCylinders < 1 || nCylinders > 240 || nSides < 1 || nSides > 2)
        throw Ep128Emu::Exception("invalid CPC disk image file header");
      if (!isExtendedFormat)            // standard disk image format
        parseDSKFileHeaders(&(tmpBuf[0]), size_t(fileSize));
      else                              // extended disk image format
        parseEXTFileHeaders(&(tmpBuf[0]), size_t(fileSize));
    }
    catch (...) {
      openDiskImage((char *) 0);
      throw;
    }
  }

  bool CPCDiskImage::getPhysicalSectorID(
      FDC765::FDCSectorID& sectorID, int c, int h, int s) const
  {
    if (c >= 0 && c < nCylinders && h >= 0 && h < nSides) {
      const CPCDiskTrackInfo& t = trackTable[c * nSides + h];
      if (s >= 0 && s < int(t.nSectors)) {
        CPCDiskSectorInfo&  s_ = t.sectorTable[s];
        sectorID.cylinderID = s_.trackNum;
        sectorID.headID = s_.sideNum;
        sectorID.sectorID = s_.sectorNum;
        sectorID.sectorSizeCode = s_.sectorSizeCode;
        sectorID.statusRegister1 = s_.statusRegister1 & 0xA5;
        sectorID.statusRegister2 = s_.statusRegister2 & 0x61;
        return true;
      }
    }
    sectorID.cylinderID = 0;
    sectorID.headID = 0;
    sectorID.sectorID = 0;
    sectorID.sectorSizeCode = 0;
    sectorID.statusRegister1 = 0x05;
    sectorID.statusRegister2 = 0x00;
    return false;
  }

  int CPCDiskImage::getPhysicalSector(int h, int n, int d) const
  {
    if (int(currentCylinder) >= nCylinders ||
        h < 0 || h >= nSides || n < 0 || d <= 0) {
      return -1;
    }
    CPCDiskTrackInfo& t = trackTable[int(currentCylinder) * nSides + h];
    if (t.nSectors < 1)
      return -1;
    // total track size is 6250 bytes, assuming rotation speed = 300 RPM
    // and data rate = 250 kbps
    int     headPosBytes = (n * 6250 + (d >> 1)) / d;
    int     curPos = 0;
    for (int s = 0; s < int(t.nSectors); s++) {
      if (curPos >= headPosBytes)
        return s;
      // ID address mark, ID, ID CRC, gap 2, sync, data address mark,
      // data, data CRC, gap 3, sync
      curPos = curPos + (4 + 4 + 2 + 22 + 12 + 4
                         + (0x0080 << (t.sectorTable[s].sectorSizeCode & 0x07))
                         + 2 + int(t.gapLen) + 12);
    }
    return 0;
  }

  int CPCDiskImage::getPhysicalSectorPos(int h, int s, int d) const
  {
    if (int(currentCylinder) >= nCylinders || h < 0 || h >= nSides || d <= 0)
      return -1;
    CPCDiskTrackInfo& t = trackTable[int(currentCylinder) * nSides + h];
    if (s < 0 || s >= int(t.nSectors))
      return -1;
    int     curPos = 0;
    for (int i = 0; i < s; i++) {
      // ID address mark, ID, ID CRC, gap 2, sync, data address mark,
      // data, data CRC, gap 3, sync
      curPos = curPos + (4 + 4 + 2 + 22 + 12 + 4
                         + (0x0080 << (t.sectorTable[i].sectorSizeCode & 0x07))
                         + 2 + int(t.gapLen) + 12);
    }
    // total track size is 6250 bytes, assuming rotation speed = 300 RPM
    // and data rate = 250 kbps
    return ((curPos * d + 3125) / 6250);
  }

  FDC765::CPCDiskError CPCDiskImage::readSector(
      uint8_t *buf, int physicalSide, int physicalSector,
      uint8_t& statusRegister1, uint8_t& statusRegister2)
  {
    if (imageFile == (std::FILE *) 0 || trackTable == (CPCDiskTrackInfo *) 0)
      return FDC765::CPCDISK_ERROR_NO_DISK;
    if (int(currentCylinder) >= nCylinders)
      return FDC765::CPCDISK_ERROR_INVALID_TRACK;
    if (physicalSide < 0 || physicalSide >= nSides)
      return FDC765::CPCDISK_ERROR_INVALID_SIDE;
    CPCDiskTrackInfo&   t =
        trackTable[int(currentCylinder) * nSides + physicalSide];
    if (physicalSector < 0 || physicalSector >= int(t.nSectors))
      return FDC765::CPCDISK_ERROR_SECTOR_NOT_FOUND;
    CPCDiskSectorInfo&  s = t.sectorTable[physicalSector];
    size_t  filePos = size_t(s.fileOffset);
    size_t  dataSize = size_t(s.dataSize);
    size_t  sectorBytes = size_t(0x80) << (s.sectorSizeCode & 0x07);
    statusRegister1 = (statusRegister1 & 0x5A) | (s.statusRegister1 & 0xA5);
    statusRegister2 = (statusRegister2 & 0x9E) | (s.statusRegister2 & 0x61);
    if (dataSize > sectorBytes) {       // weak sector
      filePos = filePos
                + (sectorBytes
                   * size_t(getRandomNumber(int(dataSize) / int(sectorBytes))));
    }
    if (std::fseek(imageFile, long(filePos), SEEK_SET) < 0)
      return FDC765::CPCDISK_ERROR_SECTOR_NOT_FOUND;
    size_t  nBytes = (dataSize < sectorBytes ? dataSize : sectorBytes);
    if (std::fread(buf, sizeof(uint8_t), nBytes, imageFile) != nBytes)
      return FDC765::CPCDISK_ERROR_READ_FAILED;
    if (dataSize < sectorBytes) {
      for (size_t i = dataSize; i < sectorBytes; i++)
        buf[i] = buf[i - dataSize];
    }
    return FDC765::CPCDISK_NO_ERROR;
  }

  FDC765::CPCDiskError CPCDiskImage::writeSector(
      const uint8_t *buf, int physicalSide, int physicalSector,
      uint8_t& statusRegister1, uint8_t& statusRegister2)
  {
    if (imageFile == (std::FILE *) 0 || trackTable == (CPCDiskTrackInfo *) 0)
      return FDC765::CPCDISK_ERROR_NO_DISK;
    if (writeProtectFlag)
      return FDC765::CPCDISK_ERROR_WRITE_PROTECTED;
    if (int(currentCylinder) >= nCylinders)
      return FDC765::CPCDISK_ERROR_INVALID_TRACK;
    if (physicalSide < 0 || physicalSide >= nSides)
      return FDC765::CPCDISK_ERROR_INVALID_SIDE;
    CPCDiskTrackInfo&   t =
        trackTable[int(currentCylinder) * nSides + physicalSide];
    if (physicalSector < 0 || physicalSector >= int(t.nSectors))
      return FDC765::CPCDISK_ERROR_SECTOR_NOT_FOUND;
    CPCDiskSectorInfo&  s = t.sectorTable[physicalSector];
    FDC765::CPCDiskError  err = FDC765::CPCDISK_NO_ERROR;
    if ((statusRegister2 ^ s.statusRegister2) & 0x40) {
      err = FDC765::CPCDISK_ERROR_WRITE_FAILED;
      if (t.sectorTableFileOffset != 0U) {
        // deleted sector flag changed: update status register 2 in image file
        if (std::fseek(imageFile, long(t.sectorTableFileOffset)
                                  + (long(&s - t.sectorTable) * 8L) + 5L,
                       SEEK_SET) >= 0) {
          uint8_t newStatusRegister2 =
              (s.statusRegister2 & 0xBF) | (statusRegister2 & 0x40);
          if (std::fputc(newStatusRegister2, imageFile) != EOF) {
            s.statusRegister2 = newStatusRegister2;
            err = FDC765::CPCDISK_NO_ERROR;
          }
        }
      }
    }
    size_t  filePos = size_t(s.fileOffset);
    size_t  dataSize = size_t(s.dataSize);
    size_t  sectorBytes = size_t(0x80) << (s.sectorSizeCode & 0x07);
    statusRegister1 = (statusRegister1 & 0x5A) | (s.statusRegister1 & 0xA5);
    statusRegister2 = (statusRegister2 & 0x9E) | (s.statusRegister2 & 0x61);
    if (dataSize > sectorBytes) {       // FIXME: writing to weak sector
      filePos = filePos
                + (sectorBytes
                   * size_t(getRandomNumber(int(dataSize) / int(sectorBytes))));
      err = FDC765::CPCDISK_ERROR_WRITE_FAILED;
    }
    if (std::fseek(imageFile, long(filePos), SEEK_SET) < 0)
      return FDC765::CPCDISK_ERROR_SECTOR_NOT_FOUND;
    size_t  nBytes = (dataSize < sectorBytes ? dataSize : sectorBytes);
    if (std::fwrite(buf, sizeof(uint8_t), nBytes, imageFile) != nBytes)
      return FDC765::CPCDISK_ERROR_WRITE_FAILED;
    return err;
  }

  void CPCDiskImage::stepIn(int nSteps)
  {
    int     tmp = int(currentCylinder) + nSteps;
    currentCylinder = uint8_t(tmp > 0 ? (tmp < 84 ? tmp : 84) : 0);
  }

  void CPCDiskImage::stepOut(int nSteps)
  {
    int     tmp = int(currentCylinder) - nSteps;
    currentCylinder = uint8_t(tmp > 0 ? (tmp < 84 ? tmp : 84) : 0);
  }

  // ==========================================================================

  FDC765_CPC::FDC765_CPC()
    : FDC765()
  {
  }

  FDC765_CPC::~FDC765_CPC()
  {
  }

  void FDC765_CPC::openDiskImage(int n, const char *fileName)
  {
    rotationAngles[n & 3] = uint8_t(floppyDrives[n & 3].getRandomNumber(100));
    floppyDrives[n & 3].openDiskImage((char *) 0);
    updateDriveReadyStatus();
    if (fileName != (char *) 0 && fileName[0] != '\0') {
      floppyDrives[n & 3].openDiskImage(fileName);
      updateDriveReadyStatus();
    }
  }

  bool FDC765_CPC::haveDisk(int driveNum) const
  {
    return floppyDrives[driveNum & 3].haveDisk();
  }

  bool FDC765_CPC::getIsTrack0(int driveNum) const
  {
    return floppyDrives[driveNum & 3].getIsTrack0();
  }

  bool FDC765_CPC::getIsWriteProtected(int driveNum) const
  {
    return floppyDrives[driveNum & 3].getIsWriteProtected();
  }

  uint8_t FDC765_CPC::getPhysicalTrackSectors(int c) const
  {
    return floppyDrives[cmdParams.unitNumber].getPhysicalTrackSectors(
               c, cmdParams.physicalSide);
  }

  uint8_t FDC765_CPC::getCurrentTrackSectors() const
  {
    return floppyDrives[cmdParams.unitNumber].getCurrentTrackSectors(
               cmdParams.physicalSide);
  }

  bool FDC765_CPC::getPhysicalSectorID(FDC765::FDCSectorID& sectorID,
                                       int c, int s) const
  {
    return floppyDrives[cmdParams.unitNumber].getPhysicalSectorID(
               sectorID, c, cmdParams.physicalSide, s);
  }

  bool FDC765_CPC::getPhysicalSectorID(FDC765::FDCSectorID& sectorID,
                                       int s) const
  {
    return floppyDrives[cmdParams.unitNumber].getPhysicalSectorID(
               sectorID, cmdParams.physicalSide, s);
  }

  int FDC765_CPC::getPhysicalSector(int n, int d) const
  {
    return floppyDrives[cmdParams.unitNumber].getPhysicalSector(
               cmdParams.physicalSide, n, d);
  }

  int FDC765_CPC::getPhysicalSectorPos(int s, int d) const
  {
    return floppyDrives[cmdParams.unitNumber].getPhysicalSectorPos(
               cmdParams.physicalSide, s, d);
  }

  FDC765::CPCDiskError FDC765_CPC::readSector(int physicalSector_,
                                              uint8_t& statusRegister1_,
                                              uint8_t& statusRegister2_)
  {
    return floppyDrives[cmdParams.unitNumber].readSector(
               sectorBuf, cmdParams.physicalSide, physicalSector_,
               statusRegister1_, statusRegister2_);
  }

  FDC765::CPCDiskError FDC765_CPC::writeSector(int physicalSector_,
                                               uint8_t& statusRegister1_,
                                               uint8_t& statusRegister2_)
  {
    return floppyDrives[cmdParams.unitNumber].writeSector(
               sectorBuf, cmdParams.physicalSide, physicalSector_,
               statusRegister1_, statusRegister2_);
  }

  void FDC765_CPC::stepIn(int driveNum, int nSteps)
  {
    floppyDrives[driveNum & 3].stepIn(nSteps);
  }

  void FDC765_CPC::stepOut(int driveNum, int nSteps)
  {
    floppyDrives[driveNum & 3].stepOut(nSteps);
  }

}       // namespace CPC464

