
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
      if (trackSizeMSB < 1)
        throw Ep128Emu::Exception("invalid CPC disk image file header");
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
        if (err < 0) {
          err = -2;
          std::fclose(imageFile);
        }
        else {
          std::fseek(imageFile, 0L, SEEK_SET);
        }
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

  FDC765::CPCDiskError CPCDiskImage::findSector(
      CPCDiskSectorInfo*& sectorPtr, const FDC765::FDCCommandParams& cmdParams)
  {
    sectorPtr = (CPCDiskSectorInfo *) 0;
    if (imageFile == (std::FILE *) 0 || trackTable == (CPCDiskTrackInfo *) 0)
      return FDC765::CPCDISK_ERROR_NO_DISK;
    if (int(currentCylinder) >= nCylinders)
      return FDC765::CPCDISK_ERROR_INVALID_TRACK;
    if (int(cmdParams.physicalSide) >= nSides)
      return FDC765::CPCDISK_ERROR_INVALID_SIDE;
    CPCDiskTrackInfo& currentTrack =
        trackTable[int(currentCylinder) * nSides + int(cmdParams.physicalSide)];
    for (uint8_t i = 0; i < currentTrack.nSectors; i++) {
      if (currentTrack.sectorTable[i].trackNum != cmdParams.cylinderID) {
        if (currentTrack.sectorTable[i].trackNum == 0xFF)
          return FDC765::CPCDISK_ERROR_BAD_CYLINDER;
        return FDC765::CPCDISK_ERROR_WRONG_CYLINDER;
      }
      if (currentTrack.sectorTable[i].sideNum == cmdParams.headID &&
          currentTrack.sectorTable[i].sectorNum == cmdParams.sectorID &&
          currentTrack.sectorTable[i].sectorSizeCode
          == cmdParams.sectorSizeCode) {
        sectorPtr = &(currentTrack.sectorTable[i]);
        return FDC765::CPCDISK_NO_ERROR;
      }
    }
    return FDC765::CPCDISK_ERROR_SECTOR_NOT_FOUND;
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
                            Ep128Emu::Timer::getRandomSeedFromTime());
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

  FDC765::CPCDiskError CPCDiskImage::readSector(
      uint8_t *buf, const FDC765::FDCCommandParams& cmdParams,
      uint8_t& statusRegister1, uint8_t& statusRegister2)
  {
    CPCDiskSectorInfo     *sectorPtr = (CPCDiskSectorInfo *) 0;
    FDC765::CPCDiskError  err = findSector(sectorPtr, cmdParams);
    if (err != FDC765::CPCDISK_NO_ERROR)
      return err;
    statusRegister1 =
        (statusRegister1 & 0x5A) | (sectorPtr->statusRegister1 & 0xA5);
    statusRegister2 =
        (statusRegister2 & 0x9E) | (sectorPtr->statusRegister2 & 0x61);
    if (((cmdParams.commandCode & 0x1F) == 0x06 &&
         (statusRegister2 & 0x40) != 0) ||
        ((cmdParams.commandCode & 0x1F) == 0x0C &&
         (statusRegister2 & 0x40) == 0)) {
      statusRegister2 = statusRegister2 | 0x40;
      if (cmdParams.commandCode & 0x20)
        return FDC765::CPCDISK_ERROR_DELETED_SECTOR;
    }
    else {
      statusRegister2 = statusRegister2 & 0xBF;
    }
    size_t  filePos = size_t(sectorPtr->fileOffset);
    size_t  dataSize = size_t(sectorPtr->dataSize);
    size_t  sectorBytes = size_t(0x80) << (sectorPtr->sectorSizeCode & 0x07);
    if (dataSize > sectorBytes) {       // weak sector
      filePos = filePos
                + (sectorBytes * (size_t(Ep128Emu::getRandomNumber(randomSeed))
                                  % (dataSize / sectorBytes)));
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
      const uint8_t *buf, const FDC765::FDCCommandParams& cmdParams,
      uint8_t& statusRegister1, uint8_t& statusRegister2)
  {
    if (writeProtectFlag && imageFile != (std::FILE *) 0)
      return FDC765::CPCDISK_ERROR_WRITE_PROTECTED;
    CPCDiskSectorInfo     *sectorPtr = (CPCDiskSectorInfo *) 0;
    FDC765::CPCDiskError  err = findSector(sectorPtr, cmdParams);
    if (err != FDC765::CPCDISK_NO_ERROR)
      return err;
    statusRegister1 =
        (statusRegister1 & 0x5A) | (sectorPtr->statusRegister1 & 0xA5);
    statusRegister2 =
        (statusRegister2 & 0x9E) | (sectorPtr->statusRegister2 & 0x61);
    if (((cmdParams.commandCode & 0x1F) == 0x05 &&
         (statusRegister2 & 0x40) != 0) ||
        ((cmdParams.commandCode & 0x1F) == 0x09 &&
         (statusRegister2 & 0x40) == 0)) {
      CPCDiskTrackInfo& currentTrack =
          trackTable[int(currentCylinder) * nSides
                     + int(cmdParams.physicalSide)];
      err = FDC765::CPCDISK_ERROR_WRITE_FAILED;
      if (currentTrack.sectorTableFileOffset != 0U) {
        // deleted sector flag changed: update status register 2 in image file
        if (std::fseek(imageFile,
                       long(currentTrack.sectorTableFileOffset)
                       + (long(sectorPtr - currentTrack.sectorTable) * 8L)
                       + 5L, SEEK_SET) >= 0) {
          uint8_t newStatusRegister2 = (sectorPtr->statusRegister2 & 0xBF)
                                       | ((cmdParams.commandCode & 0x08) << 3);
          if (std::fputc(newStatusRegister2, imageFile) != EOF) {
            sectorPtr->statusRegister2 = newStatusRegister2;
            err = FDC765::CPCDISK_NO_ERROR;
          }
        }
      }
    }
    statusRegister2 = statusRegister2 & 0xBF;
    size_t  filePos = size_t(sectorPtr->fileOffset);
    size_t  dataSize = size_t(sectorPtr->dataSize);
    size_t  sectorBytes = size_t(0x80) << (sectorPtr->sectorSizeCode & 0x07);
    if (dataSize > sectorBytes) {       // FIXME: writing to weak sector
      filePos = filePos
                + (sectorBytes * (size_t(Ep128Emu::getRandomNumber(randomSeed))
                                  % (dataSize / sectorBytes)));
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

  bool CPCDiskImage::getPhysicalSectorID(
      uint8_t& cylinderID, uint8_t& headID, uint8_t& sectorID,
      uint8_t& sectorSizeCode, int c, int h, int s) const
  {
    if (c >= 0 && c < nCylinders && h >= 0 && h < nSides) {
      const CPCDiskTrackInfo& t = trackTable[c * nSides + h];
      if (s >= 0 && s < int(t.nSectors)) {
        cylinderID = t.sectorTable[s].trackNum;
        headID = t.sectorTable[s].sideNum;
        sectorID = t.sectorTable[s].sectorNum;
        sectorSizeCode = t.sectorTable[s].sectorSizeCode;
        return true;
      }
    }
    cylinderID = 0;
    headID = 0;
    sectorID = 0;
    sectorSizeCode = 0;
    return false;
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
    floppyDrives[n & 3].openDiskImage(fileName);
  }

  FDC765::CPCDiskError FDC765_CPC::readSector(uint8_t& statusRegister1,
                                              uint8_t& statusRegister2)
  {
    return floppyDrives[cmdParams.unitNumber].readSector(
               sectorBuf, cmdParams, statusRegister1, statusRegister2);
  }

  FDC765::CPCDiskError FDC765_CPC::writeSector(uint8_t& statusRegister1,
                                               uint8_t& statusRegister2)
  {
    return floppyDrives[cmdParams.unitNumber].writeSector(
               sectorBuf, cmdParams, statusRegister1, statusRegister2);
  }

  void FDC765_CPC::stepIn(int nSteps)
  {
    floppyDrives[cmdParams.unitNumber].stepIn(nSteps);
  }

  void FDC765_CPC::stepOut(int nSteps)
  {
    floppyDrives[cmdParams.unitNumber].stepOut(nSteps);
  }

  bool FDC765_CPC::haveDisk() const
  {
    return floppyDrives[cmdParams.unitNumber].haveDisk();
  }

  bool FDC765_CPC::getIsTrack0() const
  {
    return floppyDrives[cmdParams.unitNumber].getIsTrack0();
  }

  bool FDC765_CPC::getIsWriteProtected() const
  {
    return floppyDrives[cmdParams.unitNumber].getIsWriteProtected();
  }

}       // namespace CPC464

