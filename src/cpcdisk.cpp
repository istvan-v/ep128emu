
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
      if (buf[20] > 6) {
        throw Ep128Emu::Exception("invalid track header "
                                  "in CPC disk image file");
      }
      size_t  sectorSize = size_t(0x80) << buf[20];
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
    }
    sectorTableBuf = new CPCDiskSectorInfo[nSectorsTotal];
    size_t  sectorTableBufPos = 0;
    for (int i = 0; i < int(nTracks); i++) {
      // read all sector headers
      size_t  filePos = size_t(i) * trackSize + 256;
      readImageFile(buf, filePos, 256);
      if (buf[20] > 6) {
        throw Ep128Emu::Exception("invalid track header "
                                  "in CPC disk image file");
      }
      size_t  sectorSize = size_t(0x80) << buf[20];
      if (sectorSize > 0x1800)
        sectorSize = 0x1800;
      filePos = filePos + 256;
      trackTable[i].sectorTable = &(sectorTableBuf[sectorTableBufPos]);
      for (int j = 0; j < int(trackTable[i].nSectors); j++) {
        if (buf[(j << 3) + 27] > buf[20]) {
          throw Ep128Emu::Exception("invalid sector header "
                                    "in CPC disk image file");
        }
        sectorTableBuf[sectorTableBufPos].fileOffset = uint32_t(filePos);
        sectorTableBuf[sectorTableBufPos].dataSize =
            uint32_t(0x80U << buf[(j << 3) + 27]);
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
      if (buf[20] > 7 || buf[21] > 29) {
        throw Ep128Emu::Exception("invalid track header "
                                  "in CPC disk image file");
      }
      nSectorsTotal += size_t(buf[21]);
      trackTable[i].nSectors = buf[21];
      trackTable[i].gapLen = buf[22];
      trackTable[i].fillerByte = buf[23];
      filePos = filePos + (size_t(trackSizeMSBTable[i]) << 8);
    }
    sectorTableBuf = new CPCDiskSectorInfo[nSectorsTotal];
    filePos = 256;
    size_t  sectorTableBufPos = 0;
    for (int i = 0; i < nTracks; i++) {
      // read all sector headers
      readImageFile(buf, filePos, 256);
      if (buf[20] > 7) {
        throw Ep128Emu::Exception("invalid track header "
                                  "in CPC disk image file");
      }
      size_t  nextFilePos = filePos + (size_t(trackSizeMSBTable[i]) << 8);
      filePos = filePos + 256;
      trackTable[i].sectorTable = &(sectorTableBuf[sectorTableBufPos]);
      for (int j = 0; j < int(trackTable[i].nSectors); j++) {
        if (buf[(j << 3) + 27] > buf[20]) {
          throw Ep128Emu::Exception("invalid sector header "
                                    "in CPC disk image file");
        }
        uint32_t  sectorSize = uint32_t(0x80U << buf[(j << 3) + 27]);
        uint32_t  dataSize =
            uint32_t(buf[(j << 3) + 30]) | (uint32_t(buf[(j << 3) + 31]) << 8);
        if (dataSize < 1U ||
            ((dataSize & (sectorSize - 1U)) != 0U &&
             !(sectorSize == 0x2000U && dataSize == 0x1800U))) {
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

  // --------------------------------------------------------------------------

  CPCDiskImage::CPCDiskImage()
    : trackTable((CPCDiskTrackInfo *) 0),
      sectorTableBuf((CPCDiskSectorInfo *) 0),
      imageFile((std::FILE *) 0),
      nCylinders(0),
      nSides(0),
      writeProtectFlag(true)
  {
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

}       // namespace CPC464

