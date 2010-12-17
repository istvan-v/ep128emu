
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
#include "system.hpp"

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
      uint16_t  physicalPosition;       // offset (in bytes) of ID address mark
    };                                  // from the IDAM of the first sector
   protected:
    struct CPCDiskTrackInfo {
      CPCDiskSectorInfo *sectorTable;   // array of sector info structures
      uint8_t   nSectors;               // number of sectors (0 to 29)
      uint8_t   gapLen;                 // gap 3 length
      uint8_t   fillerByte;             // filler byte (unused)
      uint32_t  sectorTableFileOffset;  // start position of sector table
    };                                  // in image file (0: no table/raw file)
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
    void calculateSectorPositions(CPCDiskTrackInfo& t);
    bool openFloppyDevice(const char *fileName);
   public:
    CPCDiskImage();
    virtual ~CPCDiskImage();
    virtual void openDiskImage(const char *fileName);
    inline bool haveDisk() const
    {
      return (imageFile != (std::FILE *) 0);
    }
    inline bool getIsTrack0() const
    {
      return (currentCylinder == 0);
    }
    inline bool getIsWriteProtected() const
    {
      return writeProtectFlag;
    }
    inline uint8_t getPhysicalTrackSectors(int c, int h) const
    {
      if (c < 0 || c >= nCylinders || h < 0 || h >= nSides)
        return 0;
      return trackTable[c * nSides + h].nSectors;
    }
    inline uint8_t getCurrentTrackSectors(int h) const
    {
      return getPhysicalTrackSectors(currentCylinder, h);
    }
    // returns the ID (C, H, R, N) of the physical sector specified by c, h, s
    // NOTE: the numbering of 's' starts from 0
    virtual bool getPhysicalSectorID(FDC765::FDCSectorID& sectorID,
                                     int c, int h, int s) const;
    // returns the ID of a physical sector on the current cylinder
    inline bool getPhysicalSectorID(FDC765::FDCSectorID& sectorID,
                                    int h, int s) const
    {
      return getPhysicalSectorID(sectorID, currentCylinder, h, s);
    }
    // returns the next physical sector (starting from 0) on side 'h' of the
    // current cylinder at rotation angle 'n' / 'd' (0 to 1, 0 is the ID
    // address mark of the first sector),
    // or -1 if there are no sectors or the specified parameters are invalid
    virtual int getPhysicalSector(int h, int n, int d) const;
    // returns the position (0 to 1) of the ID address mark of physical sector
    // 's' on side 'h' of the current cylinder, multiplied by 'd',
    // or -1 if the sector does not exist or the parameters are invalid
    virtual int getPhysicalSectorPos(int h, int s, int d) const;
    // read/write physical sector on the current cylinder
    virtual FDC765::CPCDiskError
        readSector(uint8_t *buf, int physicalSide, int physicalSector,
                   uint8_t& statusRegister1, uint8_t& statusRegister2);
    // NOTE: bit 6 of 'statusRegister2' is used as input (1 = deleted sector)
    virtual FDC765::CPCDiskError
        writeSector(const uint8_t *buf, int physicalSide, int physicalSector,
                    uint8_t& statusRegister1, uint8_t& statusRegister2);
    virtual void stepIn(int nSteps = 1);
    virtual void stepOut(int nSteps = 1);
    EP128EMU_INLINE int getRandomNumber(int n)
    {
      return (Ep128Emu::getRandomNumber(randomSeed) % n);
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
    virtual bool haveDisk(int driveNum) const;
    virtual bool getIsTrack0(int driveNum) const;
    virtual bool getIsWriteProtected(int driveNum) const;
    virtual uint8_t getPhysicalTrackSectors(int c) const;
    virtual uint8_t getCurrentTrackSectors() const;
    // returns the ID (C, H, R, N) of the physical sector specified by c, s
    // NOTE: the numbering of 's' starts from 0
    virtual bool getPhysicalSectorID(FDCSectorID& sectorID, int c, int s) const;
    // returns the ID of a physical sector on the current cylinder
    virtual bool getPhysicalSectorID(FDCSectorID& sectorID, int s) const;
    // returns the next physical sector (starting from 0) on the current
    // cylinder and side at rotation angle 'n' / 'd' (0 to 1, 0 is the ID
    // address mark of the first sector),
    // or -1 if there are no sectors or the specified parameters are invalid
    virtual int getPhysicalSector(int n, int d) const;
    // returns the position (0 to 1) of the ID address mark of physical sector
    // 's' on the current cylinder and side, multiplied by 'd',
    // or -1 if the sector does not exist or the parameters are invalid
    virtual int getPhysicalSectorPos(int s, int d) const;
    // read/write physical sector on the current cylinder
    virtual CPCDiskError readSector(int physicalSector_,
                                    uint8_t& statusRegister1_,
                                    uint8_t& statusRegister2_);
    // NOTE: bit 6 of 'statusRegister2_' is used as input (1 = deleted sector)
    virtual CPCDiskError writeSector(int physicalSector_,
                                     uint8_t& statusRegister1_,
                                     uint8_t& statusRegister2_);
    virtual void stepIn(int driveNum, int nSteps = 1);
    virtual void stepOut(int driveNum, int nSteps = 1);
  };

}       // namespace CPC464

#endif  // EP128EMU_CPCDISK_HPP

