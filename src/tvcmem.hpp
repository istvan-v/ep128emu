
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://sourceforge.net/projects/ep128emu/
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

#ifndef EP128EMU_TVCMEM_HPP
#define EP128EMU_TVCMEM_HPP

#include "ep128emu.hpp"
#include "bplist.hpp"

namespace TVC64 {

  class Memory {
   private:
    uint8_t   **segmentTable;
    bool      *segmentROMTable;
    uint8_t   pageTable[4];
    uint16_t  currentPaging;            // configuration set with setPaging()
    uint8_t   totalRAMSegments;         // 3 (TVC32), 5 (TVC64) or 8 (TVC64+)
   protected:
    // if true, use external implementation of segment 01 (CART)
    bool      segment1IsExtension;
    std::vector< uint8_t >  extensionRAM;
   private:
    uint8_t   *breakPointTable;
    size_t    breakPointCnt;
    uint8_t   **segmentBreakPointTable;
    size_t    *segmentBreakPointCntTable;
    bool      haveBreakPoints;
    uint8_t   breakPointPriorityThreshold;
    uint8_t   *videoMemory; // 64K for segments FC to FF; always RAM
    uint8_t   *dummyMemory; // 2*16K dummy memory for invalid reads and writes
    uint8_t   *pageAddressTableR[8];
    uint8_t   *pageAddressTableW[8];
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
    void setRAMSize(size_t n);          // in kilobytes; 48, 80 or 128
    void loadROMSegment(uint8_t segment, const uint8_t *data, size_t dataSize);
    void deleteSegment(uint8_t segment);
    void deleteAllSegments();
    inline uint8_t read(uint16_t addr);
    inline uint8_t readOpcode(uint16_t addr);
    inline uint8_t readNoDebug(uint16_t addr) const;
    inline uint8_t readRaw(uint32_t addr) const;
    inline void write(uint16_t addr, uint8_t value);
    inline void writeRaw(uint32_t addr, uint8_t value);
    inline void writeROM(uint32_t addr, uint8_t value);
    // set memory paging:
    //   bits 0 to 7:   port 02h:
    //          b0, b1:   unused
    //          b2 (64+): 0 = page 1 is U1, 1 = page 1 is video RAM
    //          b3, b4:   00: page 0 = SYS ROM (segment 00h)
    //                    01: page 0 = CART ROM (segment 01h)
    //                    10: page 0 = U0 RAM (segment F8h)
    //                    11: page 0 = U3 RAM (segment FBh), TVC64+
    //          b5:       0 = page 2 is video RAM, 1 = page 2 is U2
    //          b6, b7:   00: page 3 = CART ROM (segment 01h)
    //                    01: page 3 = SYS ROM (segment 00h)
    //                    10: page 3 = U3 RAM (segment FBh)
    //                    11: page 3 = IOMEM/EXT ROM (segment 02h)
    //                          C000h-DFFFh: IOMEM
    //                          E000h-FFFFh: EXT ROM
    //   bits 8 to 13:  port 0Fh (b0 to b5, TVC64+ only):
    //          b8, b9:   video RAM segment seen by Z80 on page 1
    //          b10, b11: video RAM segment seen by Z80 on page 2
    //          b12, b13: video RAM segment used by display
    //   bits 14 to 15: port 03h (b6 to b7):
    //          00:       IOMEM page 0
    //          01:       IOMEM page 1
    //          02:       IOMEM page 2
    //          03:       IOMEM page 3
    void setPaging(uint16_t n);
    inline uint16_t getPaging() const;
    inline uint8_t getPage(uint8_t page) const;
    // get current video RAM page for display
    inline const uint8_t * getVideoMemory() const;
    inline bool isSegmentROM(uint8_t segment) const;
    inline bool isSegmentRAM(uint8_t segment) const;
    bool checkIgnoreBreakPoint(uint16_t addr) const;
    void clearRAM();
    Ep128Emu::BreakPointList getBreakPointList();
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
   protected:
    virtual void breakPointCallback(bool isWrite, uint16_t addr, uint8_t value);
    // these functions are used when accessing special memory areas like IOMEM
    virtual EP128EMU_REGPARM2 uint8_t extensionRead(uint16_t addr);
    virtual EP128EMU_REGPARM2 uint8_t extensionReadNoDebug(uint16_t addr) const;
    virtual EP128EMU_REGPARM3 void extensionWrite(uint16_t addr, uint8_t value);
  };

  // --------------------------------------------------------------------------

  inline uint8_t Memory::read(uint16_t addr)
  {
    uint8_t page = uint8_t(addr >> 13);
    uint8_t value;
    if (EP128EMU_UNLIKELY(!pageAddressTableR[page]))
      value = extensionRead(addr);
    else
      value = pageAddressTableR[page][addr];
    if (haveBreakPoints)
      checkReadBreakPoint(addr, page, value);
    return value;
  }

  inline uint8_t Memory::readOpcode(uint16_t addr)
  {
    uint8_t page = uint8_t(addr >> 13);
    uint8_t value;
    if (EP128EMU_UNLIKELY(!pageAddressTableR[page]))
      value = extensionRead(addr);
    else
      value = pageAddressTableR[page][addr];
    if (haveBreakPoints)
      checkExecuteBreakPoint(addr, page, value);
    return value;
  }

  inline uint8_t Memory::readNoDebug(uint16_t addr) const
  {
    uint8_t page = uint8_t(addr >> 13);
    if (EP128EMU_UNLIKELY(!pageAddressTableR[page]))
      return extensionReadNoDebug(addr);
    return pageAddressTableR[page][addr];
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
    uint8_t page = uint8_t(addr >> 13);
    if (EP128EMU_UNLIKELY(!pageAddressTableW[page])) {
      extensionWrite(addr, value);
      return;
    }
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

  inline uint16_t Memory::getPaging() const
  {
    return currentPaging;
  }

  inline uint8_t Memory::getPage(uint8_t page) const
  {
    return pageTable[page & 3];
  }

  inline const uint8_t * Memory::getVideoMemory() const
  {
    return &(videoMemory[(currentPaging & 0x3000) << 2]);
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

}       // namespace TVC64

#endif  // EP128EMU_TVCMEM_HPP

