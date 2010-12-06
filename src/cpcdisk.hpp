
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

#ifndef EP128EMU_CPCDISK_HPP
#define EP128EMU_CPCDISK_HPP

#include "ep128emu.hpp"
#include "fdc765.hpp"

namespace CPC464 {

  class CPCDiskImage {
   public:
    struct CPCDiskSectorInfo {
      uint32_t  fileOffset;             // start position of sector data
      uint32_t  dataSize;               // size of sector data in image file
      uint8_t   trackNum;               // logical track number (C)
      uint8_t   sideNum;                // logical side number (H)
      uint8_t   sectorNum;              // logical sector number (R)
      uint8_t   sectorSizeCode;         // sector size (N); 128 * 2^N bytes
      uint8_t   statusRegister1;        // status register 1 (ST1) & 0xA5
      uint8_t   statusRegister2;        // status register 2 (ST2) & 0x61
    };
   protected:
    struct CPCDiskTrackInfo {
      CPCDiskSectorInfo *sectorTable;   // array of sector info structures
      uint8_t   nSectors;               // number of sectors (0 to 29)
      uint8_t   gapLen;                 // gap 3 length (unused)
      uint8_t   fillerByte;             // filler byte (unused)
      uint32_t  sectorTableFileOffset;  // start position of sector table
                                        // in image file (0: no table/raw file)
    };
    // ----------------
    CPCDiskTrackInfo  *trackTable;
    CPCDiskSectorInfo *sectorTableBuf;
    std::FILE *imageFile;
    int       nCylinders;               // number of cylinders (1 to 240)
    int       nSides;                   // number of sides (1 or 2)
    bool      writeProtectFlag;
    uint8_t   currentCylinder;          // drive head position
    int       randomSeed;               // for emulating weak sectors
    // ----------------
    void readImageFile(uint8_t *buf, size_t filePos, size_t nBytes);
    void parseDSKFileHeaders(uint8_t *buf, size_t fileSize);
    void parseEXTFileHeaders(uint8_t *buf, size_t fileSize);
    bool openFloppyDevice(const char *fileName);
    FDC765::CPCDiskError findSector(CPCDiskSectorInfo*& sectorPtr,
                                    const FDC765::FDCCommandParams& cmdParams);
   public:
    CPCDiskImage();
    virtual ~CPCDiskImage();
    virtual void openDiskImage(const char *fileName);
    virtual FDC765::CPCDiskError
        readSector(uint8_t *buf, const FDC765::FDCCommandParams& cmdParams,
                   uint8_t& statusRegister1, uint8_t& statusRegister2);
    virtual FDC765::CPCDiskError
        writeSector(const uint8_t *buf,
                    const FDC765::FDCCommandParams& cmdParams,
                    uint8_t& statusRegister1, uint8_t& statusRegister2);
    virtual void stepIn(int nSteps = 1);
    virtual void stepOut(int nSteps = 1);
    inline bool haveDisk() const
    {
      return (this->imageFile != (std::FILE *) 0);
    }
    inline bool getIsTrack0() const
    {
      return (this->currentCylinder == 0);
    }
    inline bool getIsWriteProtected() const
    {
      return this->writeProtectFlag;
    }
  };

  // --------------------------------------------------------------------------

  class FDC765_CPC : public FDC765 {
   private:
    CPCDiskImage  floppyDrives[4];
   public:
    FDC765_CPC();
    virtual ~FDC765_CPC();
    virtual void openDiskImage(int n, const char *fileName);
   protected:
    virtual CPCDiskError readSector(uint8_t& statusRegister1,
                                    uint8_t& statusRegister2);
    virtual CPCDiskError writeSector(uint8_t& statusRegister1,
                                     uint8_t& statusRegister2);
    virtual void stepIn(int nSteps = 1);
    virtual void stepOut(int nSteps = 1);
    virtual bool haveDisk() const;
    virtual bool getIsTrack0() const;
    virtual bool getIsWriteProtected() const;
  };

}       // namespace CPC464

#endif  // EP128EMU_CPCDISK_HPP

