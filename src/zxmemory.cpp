
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2009 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "zxmemory.hpp"

namespace ZX128 {

  void Memory::allocateSegment(uint8_t n, bool isROM)
  {
    if (segmentTable[n] == (uint8_t *) 0)
      segmentTable[n] = new uint8_t[16384];
    segmentROMTable[n] = isROM;
    for (uint8_t i = 0; i < 4; i++)
      setPage(i, getPage(i));
  }

  void Memory::checkExecuteBreakPoint(uint16_t addr, uint8_t page,
                                      uint8_t value)
  {
    const uint8_t *tbl = breakPointTable;
    if (tbl != (uint8_t *) 0 &&
        tbl[addr] >= breakPointPriorityThreshold && (tbl[addr] & 36) == 4) {
      breakPointCallback(false, addr, value);
    }
    else {
      uint16_t  offs = addr & 0x3FFF;
      tbl = segmentBreakPointTable[pageTable[page]];
      if (tbl != (uint8_t *) 0 &&
          tbl[offs] >= breakPointPriorityThreshold && (tbl[offs] & 36) == 4) {
        breakPointCallback(false, addr, value);
      }
    }
  }

  void Memory::checkReadBreakPoint(uint16_t addr, uint8_t page, uint8_t value)
  {
    const uint8_t *tbl = breakPointTable;
    if (tbl != (uint8_t *) 0 &&
        tbl[addr] >= breakPointPriorityThreshold && (tbl[addr] & 33) == 1) {
      breakPointCallback(false, addr, value);
    }
    else {
      uint16_t  offs = addr & 0x3FFF;
      tbl = segmentBreakPointTable[pageTable[page]];
      if (tbl != (uint8_t *) 0 &&
          tbl[offs] >= breakPointPriorityThreshold && (tbl[offs] & 33) == 1) {
        breakPointCallback(false, addr, value);
      }
    }
  }

  void Memory::checkWriteBreakPoint(uint16_t addr, uint8_t page, uint8_t value)
  {
    const uint8_t *tbl = breakPointTable;
    if (tbl != (uint8_t *) 0 &&
        tbl[addr] >= breakPointPriorityThreshold && (tbl[addr] & 34) == 2) {
      breakPointCallback(true, addr, value);
    }
    else {
      uint16_t  offs = addr & 0x3FFF;
      tbl = segmentBreakPointTable[pageTable[page]];
      if (tbl != (uint8_t *) 0 &&
          tbl[offs] >= breakPointPriorityThreshold && (tbl[offs] & 34) == 2) {
        breakPointCallback(true, addr, value);
      }
    }
  }

  Memory::Memory()
  {
    segmentTable = (uint8_t **) 0;
    segmentROMTable = (bool *) 0;
    breakPointTable = (uint8_t *) 0;
    breakPointCnt = 0;
    segmentBreakPointTable = (uint8_t **) 0;
    segmentBreakPointCntTable = (size_t *) 0;
    haveBreakPoints = false;
    breakPointPriorityThreshold = 0;
    dummyMemory = (uint8_t *) 0;
    for (int i = 0; i < 4; i++) {
      pageTable[i] = 0;
      pageAddressTableR[i] = (uint8_t *) 0;
      pageAddressTableW[i] = (uint8_t *) 0;
    }
    try {
      segmentTable = new uint8_t*[256];
      for (int i = 0; i < 256; i++)
        segmentTable[i] = (uint8_t *) 0;
      segmentROMTable = new bool[256];
      for (int i = 0; i < 256; i++)
        segmentROMTable[i] = true;
      segmentBreakPointTable = new uint8_t*[256];
      for (int i = 0; i < 256; i++)
        segmentBreakPointTable[i] = (uint8_t *) 0;
      segmentBreakPointCntTable = new size_t[256];
      for (int i = 0; i < 256; i++)
        segmentBreakPointCntTable[i] = 0;
      dummyMemory = new uint8_t[32768];
      for (int i = 0; i < 32768; i++)
        dummyMemory[i] = 0xFF;
      for (uint8_t i = 0; i < 4; i++)
        setPage(i, 0x00);
    }
    catch (...) {
      if (segmentTable) {
        delete[] segmentTable;
        segmentTable = (uint8_t **) 0;
      }
      if (segmentROMTable) {
        delete[] segmentROMTable;
        segmentROMTable = (bool *) 0;
      }
      if (segmentBreakPointTable) {
        delete[] segmentBreakPointTable;
        segmentBreakPointTable = (uint8_t **) 0;
      }
      if (segmentBreakPointCntTable) {
        delete[] segmentBreakPointCntTable;
        segmentBreakPointCntTable = (size_t *) 0;
      }
      if (dummyMemory) {
        delete[] dummyMemory;
        dummyMemory = (uint8_t *) 0;
      }
      throw;
    }
  }

  Memory::~Memory()
  {
    for (int i = 0; i < 256; i++) {
      if (segmentTable[i])
        delete[] segmentTable[i];
    }
    delete[] dummyMemory;
    delete[] segmentTable;
    delete[] segmentROMTable;
    if (breakPointTable)
      delete[] breakPointTable;
    for (int i = 0; i < 256; i++) {
      if (segmentBreakPointTable[i])
        delete[] segmentBreakPointTable[i];
    }
    delete[] segmentBreakPointTable;
    delete[] segmentBreakPointCntTable;
    segmentTable = (uint8_t **) 0;
    segmentROMTable = (bool *) 0;
    pageTable[0] = 0;
    pageTable[1] = 0;
    pageTable[2] = 0;
    pageTable[3] = 0;
    breakPointTable = (uint8_t *) 0;
    breakPointCnt = 0;
    segmentBreakPointTable = (uint8_t **) 0;
    segmentBreakPointCntTable = (size_t *) 0;
    haveBreakPoints = false;
    breakPointPriorityThreshold = 0;
  }

  void Memory::setBreakPoint(uint8_t segment, uint16_t addr, int priority,
                             bool r, bool w, bool x, bool ignoreFlag)
  {
    uint8_t mode =
        (r ? 1 : 0) + (w ? 2 : 0) + (x ? 4 : 0) + (ignoreFlag ? 32 : 0);
    if (mode) {
      // create new breakpoint, or change existing one
      mode += uint8_t((priority > 0 ? (priority < 3 ? priority : 3) : 0) << 3);
      if (!segmentBreakPointTable[segment]) {
        segmentBreakPointTable[segment] = new uint8_t[16384];
        for (int i = 0; i < 16384; i++)
          segmentBreakPointTable[segment][i] = 0;
      }
      haveBreakPoints = true;
      uint8_t&  bp = segmentBreakPointTable[segment][addr & 0x3FFF];
      if (!bp)
        segmentBreakPointCntTable[segment]++;
      if (bp > mode)
        mode = (bp & 56) + (mode & 7);
      mode |= (bp & 7);
      bp = mode;
    }
    else if (segmentBreakPointTable[segment]) {
      if (segmentBreakPointTable[segment][addr & 0x3FFF]) {
        // remove a previously existing breakpoint
        segmentBreakPointCntTable[segment]--;
        if (!segmentBreakPointCntTable[segment]) {
          delete[] segmentBreakPointTable[segment];
          segmentBreakPointTable[segment] = (uint8_t *) 0;
        }
      }
    }
  }

  void Memory::setBreakPoint(uint16_t addr, int priority,
                             bool r, bool w, bool x, bool ignoreFlag)
  {
    uint8_t mode =
        (r ? 1 : 0) + (w ? 2 : 0) + (x ? 4 : 0) + (ignoreFlag ? 32 : 0);
    if (mode) {
      // create new breakpoint, or change existing one
      mode += uint8_t((priority > 0 ? (priority < 3 ? priority : 3) : 0) << 3);
      if (!breakPointTable) {
        breakPointTable = new uint8_t[65536];
        for (int i = 0; i < 65536; i++)
          breakPointTable[i] = 0;
      }
      haveBreakPoints = true;
      uint8_t&  bp = breakPointTable[addr];
      if (!bp)
        breakPointCnt++;
      if (bp > mode)
        mode = (bp & 56) + (mode & 7);
      mode |= (bp & 7);
      bp = mode;
    }
    else if (breakPointTable) {
      if (breakPointTable[addr]) {
        // remove a previously existing breakpoint
        breakPointCnt--;
        if (!breakPointCnt) {
          delete[] breakPointTable;
          breakPointTable = (uint8_t *) 0;
        }
      }
    }
  }

  void Memory::clearBreakPoints(uint8_t segment)
  {
    for (uint16_t addr = 0; addr < 16384; addr++)
      setBreakPoint(segment, addr, 0, false, false, false, false);
  }

  void Memory::clearBreakPoints()
  {
    for (unsigned int addr = 0; addr < 65536; addr++)
      setBreakPoint((uint16_t) addr, 0, false, false, false, false);
  }

  void Memory::clearAllBreakPoints()
  {
    clearBreakPoints();
    for (unsigned int segment = 0; segment < 256; segment++)
      clearBreakPoints((uint8_t) segment);
    haveBreakPoints = false;
  }

  void Memory::breakPointCallback(bool isWrite, uint16_t addr, uint8_t value)
  {
    (void) isWrite;
    (void) addr;
    (void) value;
  }

  void Memory::setBreakPointPriorityThreshold(int n)
  {
    breakPointPriorityThreshold = uint8_t((n > 0 ? (n < 4 ? n : 4) : 0) << 3);
  }

  int Memory::getBreakPointPriorityThreshold()
  {
    return int(breakPointPriorityThreshold >> 3);
  }

  void Memory::loadSegment(uint8_t segment, bool isROM,
                           const uint8_t *data, size_t dataSize)
  {
    if (!data)
      dataSize = 0;
    if (isROM && !dataSize) {
      // ROM segment with no data == delete segment
      deleteSegment(segment);
      return;
    }
    // allocate memory for segment if necessary
    allocateSegment(segment, isROM);
    size_t  i = 0;
    if (dataSize) {
      while (true) {
        segmentTable[segment][i & 0x3FFF] = data[i];
        if (++i >= dataSize)
          break;
        if ((i & 0x3FFF) == 0) {
          segment = (segment + 1) & 0xFF;
          // allocate memory for segment if necessary
          allocateSegment(segment, isROM);
        }
      }
    }
    for ( ; i < 0x4000 || (i & 0x3FFF) != 0; i++)
      segmentTable[segment][i & 0x3FFF] = 0x00;
  }

  void Memory::deleteSegment(uint8_t segment)
  {
    if (segmentTable[segment])
      delete[] segmentTable[segment];
    segmentTable[segment] = (uint8_t *) 0;
    segmentROMTable[segment] = true;
    for (uint8_t i = 0; i < 4; i++)
      setPage(i, getPage(i));
  }

  void Memory::deleteAllSegments()
  {
    for (unsigned int segment = 0U; segment < 256U; segment++)
      deleteSegment(uint8_t(segment));
  }

  void Memory::setPage(uint8_t page, uint8_t segment)
  {
    page = page & 3;
    pageTable[page] = segment;
    long    offs = -(long(page) << 14);
    if (segmentTable[segment] != (uint8_t *) 0) {
      pageAddressTableR[page] = segmentTable[segment] + offs;
      if (!segmentROMTable[segment])
        pageAddressTableW[page] = segmentTable[segment] + offs;
      else
        pageAddressTableW[page] = dummyMemory + (0x4000L + offs);
    }
    else {
      pageAddressTableR[page] = dummyMemory + offs;
      pageAddressTableW[page] = dummyMemory + (0x4000L + offs);
    }
  }

  bool Memory::checkIgnoreBreakPoint(uint16_t addr) const
  {
    const uint8_t *tbl = breakPointTable;
    if (tbl != (uint8_t *) 0 && (tbl[addr] & 32) != 0) {
      return true;
    }
    else {
      uint16_t  offs = addr & 0x3FFF;
      tbl = segmentBreakPointTable[pageTable[addr >> 14]];
      if (tbl != (uint8_t *) 0 && (tbl[offs] & 32) != 0) {
        return true;
      }
    }
    return false;
  }

  Ep128Emu::BreakPointList Memory::getBreakPointList()
  {
    Ep128Emu::BreakPointList  bplst;
    if (breakPointTable) {
      for (size_t i = 0; i < 65536; i++) {
        uint8_t bp = breakPointTable[i];
        if (bp)
          bplst.addMemoryBreakPoint(uint16_t(i),
                                    bool(bp & 1), bool(bp & 2), bool(bp & 4),
                                    bool(bp & 32), bp >> 3);
      }
    }
    for (size_t j = 0; j < 256; j++) {
      if (segmentBreakPointTable[j]) {
        for (size_t i = 0; i < 16384; i++) {
          uint8_t bp = segmentBreakPointTable[j][i];
          if (bp)
            bplst.addMemoryBreakPoint(uint8_t(j), uint16_t(i),
                                      bool(bp & 1), bool(bp & 2), bool(bp & 4),
                                      bool(bp & 32), bp >> 3);
        }
      }
    }
    return bplst;
  }

  // --------------------------------------------------------------------------

  class ChunkType_MemorySnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Memory& ref;
   public:
    ChunkType_MemorySnapshot(Memory& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_MemorySnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_ZXMEM_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void Memory::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000001U);       // version number
    buf.writeByte(pageTable[0]);
    buf.writeByte(pageTable[1]);
    buf.writeByte(pageTable[2]);
    buf.writeByte(pageTable[3]);
    if (segmentTable != (uint8_t **) 0 && segmentROMTable != (bool *) 0) {
      for (size_t i = 0; i < 256; i++) {
        if (segmentTable[i] != (uint8_t *) 0) {
          buf.writeByte(uint8_t(i));
          buf.writeBoolean(segmentROMTable[i]);
          for (size_t j = 0; j < 16384; j++)
            buf.writeByte(segmentTable[i][j]);
        }
      }
    }
  }

  void Memory::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_ZXMEM_STATE, buf);
  }

  void Memory::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000001U) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible memory snapshot format");
    }
    // reset memory
    deleteAllSegments();
    setPage(0, 0x00);
    setPage(1, 0x00);
    setPage(2, 0x00);
    setPage(3, 0x00);
    // now load saved state
    setPage(0, buf.readByte());
    setPage(1, buf.readByte());
    setPage(2, buf.readByte());
    setPage(3, buf.readByte());
    while (buf.getPosition() < buf.getDataSize()) {
      uint8_t segment = buf.readByte();
      // allocate space
      loadSegment(segment, false, (uint8_t *) 0, 0);
      // set ROM flag and load data
      allocateSegment(segment, buf.readBoolean());
      for (size_t i = 0; i < 16384; i++)
        segmentTable[segment][i] = buf.readByte();
    }
  }

  void Memory::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_MemorySnapshot  *p;
    p = new ChunkType_MemorySnapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace ZX128

