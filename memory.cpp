
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

#include "ep128emu.hpp"
#include "memory.hpp"

namespace Ep128 {

  Memory::Memory()
  {
    segmentTable = (uint8_t**) 0;
    segmentROMTable = (bool*) 0;
    pageTable[0] = 0;
    pageTable[1] = 0;
    pageTable[2] = 0;
    pageTable[3] = 0;
    breakPointTable = (uint8_t*) 0;
    breakPointCnt = 0;
    segmentBreakPointTable = (uint8_t**) 0;
    segmentBreakPointCntTable = (size_t*) 0;
    haveBreakPoints = false;
    breakPointPriorityThreshold = 0;
    try {
      segmentTable = new uint8_t*[256];
      for (int i = 0; i < 256; i++)
        segmentTable[i] = (uint8_t*) 0;
      segmentROMTable = new bool[256];
      for (int i = 0; i < 256; i++)
        segmentROMTable[i] = true;
      segmentBreakPointTable = new uint8_t*[256];
      for (int i = 0; i < 256; i++)
        segmentBreakPointTable[i] = (uint8_t*) 0;
      segmentBreakPointCntTable = new size_t[256];
      for (int i = 0; i < 256; i++)
        segmentBreakPointCntTable[i] = 0;
    }
    catch (...) {
      if (segmentTable) {
        delete[] segmentTable;
        segmentTable = (uint8_t**) 0;
      }
      if (segmentROMTable) {
        delete[] segmentROMTable;
        segmentROMTable = (bool*) 0;
      }
      if (segmentBreakPointTable) {
        delete[] segmentBreakPointTable;
        segmentBreakPointTable = (uint8_t**) 0;
      }
      if (segmentBreakPointCntTable) {
        delete[] segmentBreakPointCntTable;
        segmentBreakPointCntTable = (size_t*) 0;
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
    segmentTable = (uint8_t**) 0;
    segmentROMTable = (bool*) 0;
    pageTable[0] = 0;
    pageTable[1] = 0;
    pageTable[2] = 0;
    pageTable[3] = 0;
    breakPointTable = (uint8_t*) 0;
    breakPointCnt = 0;
    segmentBreakPointTable = (uint8_t**) 0;
    segmentBreakPointCntTable = (size_t*) 0;
    haveBreakPoints = false;
    breakPointPriorityThreshold = 0;
  }

  void Memory::setBreakPoint(uint8_t segment, uint16_t addr,
                             int priority, bool r, bool w)
  {
    uint8_t mode = (r ? 1 : 0) + (w ? 2 : 0);
    if (mode) {
      // create new breakpoint, or change existing one
      mode += (uint8_t) ((priority > 0 ? (priority < 3 ? priority : 3) : 0)
                         << 2);
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
        mode = (bp & 12) + (mode & 3);
      mode |= (bp & 3);
      bp = mode;
    }
    else if (segmentBreakPointTable[segment]) {
      if (segmentBreakPointTable[segment][addr & 0x3FFF]) {
        // remove a previously existing breakpoint
        segmentBreakPointCntTable[segment]--;
        if (!segmentBreakPointCntTable[segment]) {
          delete[] segmentBreakPointTable[segment];
          segmentBreakPointTable[segment] = (uint8_t*) 0;
        }
      }
    }
  }

  void Memory::setBreakPoint(uint16_t addr, int priority, bool r, bool w)
  {
    uint8_t mode = (r ? 1 : 0) + (w ? 2 : 0);
    if (mode) {
      // create new breakpoint, or change existing one
      mode += (uint8_t) ((priority > 0 ? (priority < 3 ? priority : 3) : 0)
                         << 2);
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
        mode = (bp & 12) + (mode & 3);
      mode |= (bp & 3);
      bp = mode;
    }
    else if (breakPointTable) {
      if (breakPointTable[addr]) {
        // remove a previously existing breakpoint
        breakPointCnt--;
        if (!breakPointCnt) {
          delete[] breakPointTable;
          breakPointTable = (uint8_t*) 0;
        }
      }
    }
  }

  void Memory::clearBreakPoints(uint8_t segment)
  {
    for (uint16_t addr = 0; addr < 16384; addr++)
      setBreakPoint(segment, addr, 0, false, false);
  }

  void Memory::clearBreakPoints()
  {
    for (unsigned int addr = 0; addr < 65536; addr++)
      setBreakPoint((uint16_t) addr, 0, false, false);
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
    breakPointPriorityThreshold =
        (uint8_t) ((n > 0 ? (n < 4 ? n : 4) : 0) << 2);
  }

  int Memory::getBreakPointPriorityThreshold()
  {
    return ((int) breakPointPriorityThreshold >> 2);
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
    if (!segmentTable[segment]) {
      // allocate memory for segment if necessary
      segmentTable[segment] = new uint8_t[0x4000];
    }
    segmentROMTable[segment] = isROM;
    size_t  i = 0;
    if (dataSize) {
      while (true) {
        segmentTable[segment][i & 0x3FFF] = data[i];
        if (++i >= dataSize)
          break;
        if ((i & 0x3FFF) == 0) {
          segment = (segment + 1) & 0xFF;
          if (!segmentTable[segment]) {
            // allocate memory for segment if necessary
            segmentTable[segment] = new uint8_t[0x4000];
          }
          segmentROMTable[segment] = isROM;
        }
      }
    }
    for ( ; i < 0x4000 || (i & 0x3FFF) != 0; i++)
      segmentTable[segment][i & 0x3FFF] = 0xFF;
  }

  void Memory::deleteSegment(uint8_t segment)
  {
    if (segmentTable[segment])
      delete[] segmentTable[segment];
    segmentTable[segment] = (uint8_t*) 0;
    segmentROMTable[segment] = true;
  }

  void Memory::deleteAllSegments()
  {
    for (unsigned int segment = 0; segment < 256; segment++)
      deleteSegment((uint8_t) segment);
  }

  Ep128Emu::BreakPointList Memory::getBreakPointList()
  {
    Ep128Emu::BreakPointList  bplst;
    if (breakPointTable) {
      for (size_t i = 0; i < 65536; i++) {
        uint8_t bp = breakPointTable[i];
        if (bp)
          bplst.addMemoryBreakPoint(uint16_t(i),
                                    !!(bp & 1), !!(bp & 2), bp >> 2);
      }
    }
    for (size_t j = 0; j < 256; j++) {
      if (segmentBreakPointTable[j]) {
        for (size_t i = 0; i < 16384; i++) {
          uint8_t bp = segmentBreakPointTable[j][i];
          if (bp)
            bplst.addMemoryBreakPoint(uint8_t(j), uint16_t(i),
                                      !!(bp & 1), !!(bp & 2), bp >> 2);
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
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_MEMORY_STATE;
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
    buf.writeByte(pageTable[0]);
    buf.writeByte(pageTable[1]);
    buf.writeByte(pageTable[2]);
    buf.writeByte(pageTable[3]);
    if (segmentTable != (uint8_t**) 0 && segmentROMTable != (bool*) 0) {
      for (size_t i = 0; i < 256; i++) {
        if (segmentTable[i] != (uint8_t*) 0) {
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
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_MEMORY_STATE, buf);
  }

  void Memory::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible memory snapshot format");
    }
    // reset memory
    deleteAllSegments();
    pageTable[0] = 0x00;
    pageTable[1] = 0x00;
    pageTable[2] = 0x00;
    pageTable[3] = 0x00;
    // now load saved state
    pageTable[0] = buf.readByte();
    pageTable[1] = buf.readByte();
    pageTable[2] = buf.readByte();
    pageTable[3] = buf.readByte();
    while (buf.getPosition() < buf.getDataSize()) {
      uint8_t segment = buf.readByte();
      // allocate space
      loadSegment(segment, false, (uint8_t*) 0, 0);
      // set ROM flag and load data
      segmentROMTable[segment] = buf.readBoolean();
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

}       // namespace Ep128

