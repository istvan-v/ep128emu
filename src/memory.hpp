
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EP128EMU_MEMORY_HPP
#define EP128EMU_MEMORY_HPP

#include "ep128emu.hpp"
#include "bplist.hpp"

namespace Ep128 {

  class Memory {
   private:
    uint8_t **segmentTable;
    bool    *segmentROMTable;
    uint8_t pageTable[4];
    uint8_t *breakPointTable;
    size_t  breakPointCnt;
    uint8_t **segmentBreakPointTable;
    size_t  *segmentBreakPointCntTable;
    bool    haveBreakPoints;
    uint8_t breakPointPriorityThreshold;
    uint8_t *videoMemory;   // 64K for segments FC, FD, FE, and FF; always RAM
    uint8_t *dummyMemory;   // 2*16K dummy memory for invalid reads and writes
    uint8_t *pageAddressTableR[4];
    uint8_t *pageAddressTableW[4];
    void allocateSegment(uint8_t n, bool isROM);
    void checkExecuteBreakPoint(uint16_t addr, uint8_t page, uint8_t value);
    void checkReadBreakPoint(uint16_t addr, uint8_t page, uint8_t value);
    void checkWriteBreakPoint(uint16_t addr, uint8_t page, uint8_t value);
   public:
    Memory();
    virtual ~Memory();
    void setBreakPoint(uint8_t segment, uint16_t addr,
                       int priority, bool r, bool w, bool x, bool ignoreFlag);
    void setBreakPoint(uint16_t addr,
                       int priority, bool r, bool w, bool x, bool ignoreFlag);
    void clearBreakPoints(uint8_t segment);
    void clearBreakPoints();
    void clearAllBreakPoints();
    void setBreakPointPriorityThreshold(int n);
    int getBreakPointPriorityThreshold();
    void loadSegment(uint8_t segment, bool isROM,
                     const uint8_t *data, size_t dataSize);
    void deleteSegment(uint8_t segment);
    void deleteAllSegments();
    inline uint8_t read(uint16_t addr);
    inline uint8_t readOpcode(uint16_t addr);
    inline uint8_t readNoDebug(uint16_t addr) const;
    inline uint8_t readRaw(uint32_t addr) const;
    inline void write(uint16_t addr, uint8_t value);
    inline void writeRaw(uint32_t addr, uint8_t value);
    inline void writeROM(uint32_t addr, uint8_t value);
    void setPage(uint8_t page, uint8_t segment);
    inline uint8_t getPage(uint8_t page) const;
    inline const uint8_t * getVideoMemory() const;
    inline bool isSegmentROM(uint8_t segment) const;
    inline bool isSegmentRAM(uint8_t segment) const;
    bool checkIgnoreBreakPoint(uint16_t addr) const;
    Ep128Emu::BreakPointList getBreakPointList();
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
   protected:
    virtual void breakPointCallback(bool isWrite, uint16_t addr, uint8_t value);
  };

  // --------------------------------------------------------------------------

  inline uint8_t Memory::read(uint16_t addr)
  {
    uint8_t page = uint8_t(addr >> 14);
    uint8_t value = pageAddressTableR[page][addr];
    if (haveBreakPoints)
      checkReadBreakPoint(addr, page, value);
    return value;
  }

  inline uint8_t Memory::readOpcode(uint16_t addr)
  {
    uint8_t page = uint8_t(addr >> 14);
    uint8_t value = pageAddressTableR[page][addr];
    if (haveBreakPoints)
      checkExecuteBreakPoint(addr, page, value);
    return value;
  }

  inline uint8_t Memory::readNoDebug(uint16_t addr) const
  {
    return pageAddressTableR[uint8_t(addr >> 14)][addr];
  }

  inline uint8_t Memory::readRaw(uint32_t addr) const
  {
    uint8_t segment, value;

    segment = uint8_t(addr >> 14);
    if (segmentTable[segment])
      value = segmentTable[segment][addr & 0x3FFF];
    else
      value = 0xFF;
    return value;
  }

  inline void Memory::write(uint16_t addr, uint8_t value)
  {
    uint8_t page = uint8_t(addr >> 14);
    if (haveBreakPoints)
      checkWriteBreakPoint(addr, page, value);
    pageAddressTableW[page][addr] = value;
  }

  inline void Memory::writeRaw(uint32_t addr, uint8_t value)
  {
    uint8_t segment = uint8_t(addr >> 14);
    if (!segmentROMTable[segment])
      segmentTable[segment][addr & 0x3FFF] = value;
  }

  inline void Memory::writeROM(uint32_t addr, uint8_t value)
  {
    uint8_t segment = uint8_t(addr >> 14);
    if (segmentTable[segment])
      segmentTable[segment][addr & 0x3FFF] = value;
  }

  inline uint8_t Memory::getPage(uint8_t page) const
  {
    return pageTable[page & 3];
  }

  inline const uint8_t * Memory::getVideoMemory() const
  {
    return videoMemory;
  }

  inline bool Memory::isSegmentROM(uint8_t segment) const
  {
    return (segmentTable[segment] != (uint8_t *) 0 &&
            segmentROMTable[segment]);
  }

  inline bool Memory::isSegmentRAM(uint8_t segment) const
  {
    return (segmentTable[segment] != (uint8_t *) 0 &&
            !segmentROMTable[segment]);
  }

}       // namespace Ep128

#endif  // EP128EMU_MEMORY_HPP

