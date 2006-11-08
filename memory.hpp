
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

#ifndef EP128EMU_MEMORY_HPP
#define EP128EMU_MEMORY_HPP

#include "ep128.hpp"
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
   public:
    Memory();
    virtual ~Memory();
    void setBreakPoint(uint8_t segment, uint16_t addr,
                       int priority, bool r, bool w);
    void setBreakPoint(uint16_t addr, int priority, bool r, bool w);
    void clearBreakPoints(uint8_t segment);
    void clearBreakPoints();
    void clearAllBreakPoints();
    virtual void breakPointCallback(bool isWrite, uint16_t addr, uint8_t value);
    void setBreakPointPriorityThreshold(int n);
    int getBreakPointPriorityThreshold();
    void loadSegment(uint8_t segment, bool isROM,
                     const uint8_t *data, size_t dataSize);
    void deleteSegment(uint8_t segment);
    void deleteAllSegments();
    inline uint8_t read(uint16_t addr);
    inline uint8_t readRaw(uint32_t addr);
    inline void write(uint16_t addr, uint8_t value);
    inline void setPage(uint8_t page, uint8_t segment);
    inline uint8_t getPage(uint8_t page);
    BreakPointList getBreakPointList();
    void saveState(File::Buffer&);
    void saveState(File&);
    void loadState(File::Buffer&);
    void registerChunkType(File&);
  };

  // --------------------------------------------------------------------------

  inline uint8_t Memory::read(uint16_t addr)
  {
    uint16_t  offs = addr & 0x3FFF;
    uint8_t   segment = (uint8_t) (addr >> 14), value;

    if (segmentTable[segment])
      value = segmentTable[segment][offs];
    else
      value = 0xFF;
    if (haveBreakPoints) {
      uint8_t *tbl = breakPointTable;
      if (tbl != (uint8_t*) 0 &&
          tbl[addr] >= breakPointPriorityThreshold && (tbl[addr] & 1) != 0)
        breakPointCallback(false, addr, value);
      else {
        tbl = segmentBreakPointTable[segment];
        if (tbl != (uint8_t*) 0 &&
            tbl[offs] >= breakPointPriorityThreshold && (tbl[offs] & 1) != 0)
          breakPointCallback(false, addr, value);
      }
    }
    return value;
  }

  inline uint8_t Memory::readRaw(uint32_t addr)
  {
    uint8_t segment, value;

    segment = (uint8_t) (addr >> 14);
    if (segmentTable[segment])
      value = segmentTable[segment][addr & 0x3FFF];
    else
      value = 0xFF;
    return value;
  }

  inline void Memory::write(uint16_t addr, uint8_t value)
  {
    uint16_t  offs = addr & 0x3FFF;
    uint8_t   segment = (uint8_t) (addr >> 14);

    if (haveBreakPoints) {
      uint8_t *tbl = breakPointTable;
      if (tbl != (uint8_t*) 0 &&
          tbl[addr] >= breakPointPriorityThreshold && (tbl[addr] & 2) != 0)
        breakPointCallback(true, addr, value);
      else {
        tbl = segmentBreakPointTable[segment];
        if (tbl != (uint8_t*) 0 &&
            tbl[offs] >= breakPointPriorityThreshold && (tbl[offs] & 2) != 0)
          breakPointCallback(true, addr, value);
      }
    }
    if (!segmentROMTable[segment])
      segmentTable[segment][offs] = value;
  }

  inline void Memory::setPage(uint8_t page, uint8_t segment)
  {
    pageTable[page & 3] = segment;
  }

  inline uint8_t Memory::getPage(uint8_t page)
  {
    return pageTable[page & 3];
  }

}

#endif  // EP128EMU_MEMORY_HPP

