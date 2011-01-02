
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2011 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "cpcmem.hpp"

namespace CPC464 {

  void Memory::allocateSegment(uint8_t n, bool isROM)
  {
    if (n < 0x04 && isROM)
      throw Ep128Emu::Exception("video memory cannot be ROM");
    if (segmentTable[n] == (uint8_t *) 0)
      segmentTable[n] = new uint8_t[16384];
    segmentROMTable[n] = isROM;
    setPaging(currentPaging);
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
      tbl = segmentBreakPointTable[pageTableR[page]];
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
        tbl[addr] >= breakPointPriorityThreshold && (tbl[addr] & 1) != 0) {
      breakPointCallback(false, addr, value);
    }
    else {
      uint16_t  offs = addr & 0x3FFF;
      tbl = segmentBreakPointTable[pageTableR[page]];
      if (tbl != (uint8_t *) 0 &&
          tbl[offs] >= breakPointPriorityThreshold && (tbl[offs] & 1) != 0) {
        breakPointCallback(false, addr, value);
      }
    }
  }

  void Memory::checkWriteBreakPoint(uint16_t addr, uint8_t page, uint8_t value)
  {
    const uint8_t *tbl = breakPointTable;
    if (tbl != (uint8_t *) 0 &&
        tbl[addr] >= breakPointPriorityThreshold && (tbl[addr] & 2) != 0) {
      breakPointCallback(true, addr, value);
    }
    else {
      uint16_t  offs = addr & 0x3FFF;
      tbl = segmentBreakPointTable[pageTableW[page]];
      if (tbl != (uint8_t *) 0 &&
          tbl[offs] >= breakPointPriorityThreshold && (tbl[offs] & 2) != 0) {
        breakPointCallback(true, addr, value);
      }
    }
  }

  Memory::Memory()
    : segmentTable((uint8_t **) 0),
      segmentROMTable((bool *) 0),
      currentPaging(0x00C0),
      expansionRAMBlocks(0),
      breakPointTable((uint8_t *) 0),
      breakPointCnt(0),
      segmentBreakPointTable((uint8_t **) 0),
      segmentBreakPointCntTable((size_t *) 0),
      haveBreakPoints(false),
      breakPointPriorityThreshold(0),
      videoMemory((uint8_t *) 0),
      dummyMemory((uint8_t *) 0)
  {
    for (int i = 0; i < 4; i++) {
      pageTableR[i] = 0x00;
      pageTableW[i] = 0x00;
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
      videoMemory = new uint8_t[65536];
      for (int i = 0; i < 65536; i++)
        videoMemory[i] = 0xFF;
      for (int i = 0x00; i < 0x04; i++) {
        segmentTable[i] = &(videoMemory[i << 14]);
        segmentROMTable[i] = false;
      }
      dummyMemory = new uint8_t[32768];
      for (int i = 0; i < 32768; i++)
        dummyMemory[i] = 0xFF;
      setPaging(0x00C0);
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
      if (videoMemory) {
        delete[] videoMemory;
        videoMemory = (uint8_t *) 0;
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
    for (int i = 0x04; i <= 0xFF; i++) {
      if (segmentTable[i])
        delete[] segmentTable[i];
    }
    delete[] dummyMemory;
    delete[] videoMemory;
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

  void Memory::setRAMSize(size_t n)
  {
    expansionRAMBlocks = 0;
    if (n >= 96) {
      if (n < 160)
        expansionRAMBlocks = 1;
      else if (n < 256)
        expansionRAMBlocks = 2;
      else if (n < 448)
        expansionRAMBlocks = 4;
      else
        expansionRAMBlocks = 8;
    }
    for (uint8_t i = ((expansionRAMBlocks << 2) + 0x04); i <= 0x7F; i++)
      deleteSegment(i);
    for (uint8_t i = 0x04; i < ((expansionRAMBlocks << 2) + 0x04); i++)
      allocateSegment(i, false);
  }

  void Memory::loadROMSegment(uint8_t segment,
                              const uint8_t *data, size_t dataSize)
  {
    if (segment < 0xC0 && segment != 0x80)
      throw Ep128Emu::Exception("internal error: invalid ROM segment number");
    if (!data)
      dataSize = 0;
    if (!dataSize) {
      // ROM segment with no data == delete segment
      deleteSegment(segment);
      return;
    }
    // allocate memory for segment if necessary
    allocateSegment(segment, true);
    size_t  i = 0;
    if (dataSize) {
      while (true) {
        segmentTable[segment][i & 0x3FFF] = data[i];
        if (++i >= dataSize)
          break;
        if ((i & 0x3FFF) == 0) {
          segment = (segment + 1) & 0xFF;
          // allocate memory for segment if necessary
          allocateSegment(segment, true);
        }
      }
    }
    for ( ; i < 0x4000 || (i & 0x3FFF) != 0; i++)
      segmentTable[segment][i & 0x3FFF] = 0xFF;
  }

  void Memory::deleteSegment(uint8_t segment)
  {
    if (segment < 0x04)
      throw Ep128Emu::Exception("cannot delete video memory segments");
    if (segmentTable[segment])
      delete[] segmentTable[segment];
    segmentTable[segment] = (uint8_t*) 0;
    segmentROMTable[segment] = true;
    setPaging(currentPaging);
  }

  void Memory::deleteAllSegments()
  {
    for (unsigned int segment = 0x04U; segment <= 0xFFU; segment++)
      deleteSegment(uint8_t(segment));
  }

  void Memory::setPaging(uint16_t n)
  {
    currentPaging = n;
    switch (expansionRAMBlocks) {
    case 1:                             // 128K
      n = n & 0xFFC7;
      break;
    case 2:                             // 192K
      n = n & 0xFFCF;
      break;
    case 4:                             // 320K
      n = n & 0xFFDF;
      break;
    case 8:                             // 576K
      n = n & 0xFFFF;
      break;
    default:                            // 64K
      n = n & 0xFFC0;
      break;
    }
    // if an invalid expansion ROM is selected, default to BASIC
    if (n >= 0x4000 || segmentTable[(n >> 8) | 0xC0] == (uint8_t *) 0)
      n = n & 0x00FF;
    // set RAM paging
    pageTableR[0] = 0x00;
    pageTableR[1] = 0x01;
    pageTableR[2] = 0x02;
    pageTableR[3] = 0x03;
    switch (n & 0x0007) {
    case 1:
      pageTableR[3] = (uint8_t(n & 0x0038) >> 1) + 0x07;
      break;
    case 2:
      pageTableR[0] = (uint8_t(n & 0x0038) >> 1) + 0x04;
      pageTableR[1] = (uint8_t(n & 0x0038) >> 1) + 0x05;
      pageTableR[2] = (uint8_t(n & 0x0038) >> 1) + 0x06;
      pageTableR[3] = (uint8_t(n & 0x0038) >> 1) + 0x07;
      break;
    case 3:
      pageTableR[1] = 0x03;
      pageTableR[3] = (uint8_t(n & 0x0038) >> 1) + 0x07;
      break;
    case 4:
    case 5:
    case 6:
    case 7:
      pageTableR[1] = (uint8_t(n & 0x0038) >> 1) + (uint8_t(n & 0x0003) | 0x04);
      break;
    }
    pageTableW[0] = pageTableR[0];
    pageTableW[1] = pageTableR[1];
    pageTableW[2] = pageTableR[2];
    pageTableW[3] = pageTableR[3];
    // set ROM paging (if ROM is enabled)
    if ((n & 0x0040) != 0)
      pageTableR[0] = 0x80;
    if ((n & 0x0080) != 0)
      pageTableR[3] = uint8_t(n >> 8) | 0xC0;
    for (uint8_t i = 0; i < 4; i++) {
      long    offs = -(long(i) << 14);
      uint8_t segment = pageTableR[i];
      if (segmentTable[segment] != (uint8_t *) 0)
        pageAddressTableR[i] = segmentTable[segment] + offs;
      else
        pageAddressTableR[i] = dummyMemory + offs;
      segment = pageTableW[i];
      if (segmentTable[segment] != (uint8_t *) 0)
        pageAddressTableW[i] = segmentTable[segment] + offs;
      else
        pageAddressTableW[i] = dummyMemory + (0x4000L + offs);
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
      tbl = segmentBreakPointTable[pageTableR[addr >> 14]];
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

  class ChunkType_CPCMemSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Memory& ref;
   public:
    ChunkType_CPCMemSnapshot(Memory& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_CPCMemSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_CPCMEM_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void Memory::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    buf.writeUInt16(currentPaging);
    buf.writeByte(expansionRAMBlocks);
    for (uint8_t i = 0; i < ((expansionRAMBlocks << 2) + 0x04); i++) {
      if (segmentTable[i] != (uint8_t *) 0) {
        for (size_t j = 0; j < 16384; j++)
          buf.writeByte(segmentTable[i][j]);
      }
      else {
        for (size_t j = 0; j < 16384; j++)
          buf.writeByte(0xFF);
      }
    }
    for (size_t i = 0x80; i <= 0xFF; i++) {
      if (i == 0x81)
        i = 0xC0;
      if (segmentTable[i] != (uint8_t *) 0) {
        buf.writeByte(uint8_t(i));
        for (size_t j = 0; j < 16384; j++)
          buf.writeByte(segmentTable[i][j]);
      }
    }
  }

  void Memory::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_CPCMEM_STATE, buf);
  }

  void Memory::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible CPC memory snapshot format");
    }
    // reset memory
    deleteAllSegments();
    try {
      currentPaging = buf.readUInt16();
      // load RAM segments
      expansionRAMBlocks = buf.readByte();
      setRAMSize((size_t(expansionRAMBlocks) << 6) + 64);
      for (uint8_t i = 0; i < ((expansionRAMBlocks << 2) + 0x04); i++) {
        if (segmentTable[i] != (uint8_t *) 0) {
          for (size_t j = 0; j < 16384; j++)
            segmentTable[i][j] = buf.readByte();
        }
        else {
          for (size_t j = 0; j < 16384; j++)
            (void) buf.readByte();
        }
      }
      // load ROM segments
      while (buf.getPosition() < buf.getDataSize()) {
        uint8_t segment = buf.readByte();
        if (segment >= 0xC0 || segment == 0x80)
          allocateSegment(segment, true);
        if (segmentTable[segment] != (uint8_t *) 0) {
          for (size_t i = 0; i < 16384; i++)
            segmentTable[segment][i] = buf.readByte();
        }
        else {
          for (size_t i = 0; i < 16384; i++)
            (void) buf.readByte();
        }
      }
      setPaging(currentPaging);
    }
    catch (...) {
      setRAMSize(64);
      setPaging(0x00C0);
      throw;
    }
  }

  void Memory::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_CPCMemSnapshot  *p;
    p = new ChunkType_CPCMemSnapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace CPC464

