
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

#include "ep128emu.hpp"
#include "tvcmem.hpp"

namespace TVC64 {

  void Memory::allocateSegment(uint8_t n, bool isROM)
  {
    if (n >= 0xFC && isROM)
      throw Ep128Emu::Exception("video memory cannot be ROM");
    if (n > 0x02 && n < 0xF8)
      throw Ep128Emu::Exception("invalid segment number");
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
      tbl = segmentBreakPointTable[pageTable[page >> 1]];
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
      tbl = segmentBreakPointTable[pageTable[page >> 1]];
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
      tbl = segmentBreakPointTable[pageTable[page >> 1]];
      if (tbl != (uint8_t *) 0 &&
          tbl[offs] >= breakPointPriorityThreshold && (tbl[offs] & 2) != 0) {
        breakPointCallback(true, addr, value);
      }
    }
  }

  Memory::Memory()
    : segmentTable((uint8_t **) 0),
      segmentROMTable((bool *) 0),
      currentPaging(0x3F00),
      totalRAMSegments(5),
      segment1IsExtension(false),
      breakPointTable((uint8_t *) 0),
      breakPointCnt(0),
      segmentBreakPointTable((uint8_t **) 0),
      segmentBreakPointCntTable((size_t *) 0),
      haveBreakPoints(false),
      breakPointPriorityThreshold(0),
      videoMemory((uint8_t *) 0),
      dummyMemory((uint8_t *) 0)
  {
    for (int i = 0; i < 4; i++)
      pageTable[i] = 0x00;
    for (int i = 0; i < 8; i++) {
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
      for (int i = 0xFC; i <= 0xFF; i++) {
        segmentTable[i] = &(videoMemory[(i & 3) << 14]);
        segmentROMTable[i] = false;
      }
      dummyMemory = new uint8_t[32768];
      for (int i = 0; i < 32768; i++)
        dummyMemory[i] = 0xFF;
      setPaging(0x3F00);
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
    for (int i = 0x00; i < 0xFC; i++) {
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

  uint8_t Memory::extensionRead(uint16_t addr)
  {
    (void) addr;
    return 0xFF;
  }

  uint8_t Memory::extensionReadNoDebug(uint16_t addr) const
  {
    (void) addr;
    return 0xFF;
  }

  void Memory::extensionWrite(uint16_t addr, uint8_t value)
  {
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
    totalRAMSegments = 5;
    if (n < 64)
      totalRAMSegments = 3;
    else if (n >= 104)
      totalRAMSegments = 8;
    for (uint8_t i = 0xF8 + (totalRAMSegments - 1); i < 0xFC; i++)
      deleteSegment(i);
    for (uint8_t i = 0xF8; i < (0xF8 + (totalRAMSegments - 1)) && i < 0xFC; i++)
      allocateSegment(i, false);
    if (totalRAMSegments < 8)
      setPaging(currentPaging | 0x3F00);
  }

  void Memory::loadROMSegment(uint8_t segment,
                              const uint8_t *data, size_t dataSize)
  {
    if (segment > 0x02)
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
    if (segment == 0x02 && dataSize > 0 && dataSize <= 8192) {
      std::memcpy(&(segmentTable[segment][0x2000]),
                  &(segmentTable[segment][0]), dataSize);
    }
  }

  void Memory::deleteSegment(uint8_t segment)
  {
    if (segment >= 0xFC)
      throw Ep128Emu::Exception("cannot delete video memory segments");
    if (segmentTable[segment])
      delete[] segmentTable[segment];
    segmentTable[segment] = (uint8_t*) 0;
    segmentROMTable[segment] = true;
    setPaging(currentPaging);
  }

  void Memory::deleteAllSegments()
  {
    for (unsigned int segment = 0x00U; segment < 0xFCU; segment++)
      deleteSegment(uint8_t(segment));
  }

  void Memory::setPaging(uint16_t n)
  {
    if (totalRAMSegments < 8)
      n = (n & 0xC0F8) | 0x3F00;
    currentPaging = n;
    switch (n & 0x0018) {
    case 0x00:
      pageTable[0] = 0x00;              // SYS
      break;
    case 0x08:
      pageTable[0] = 0x01;              // CART
      break;
    case 0x10:
      pageTable[0] = 0xF8;              // U0
      break;
    case 0x18:
      pageTable[0] = 0xFB;              // U3
      break;
    }
    if (!(n & 0x0004))
      pageTable[1] = 0xF9;              // U1
    else
      pageTable[1] = uint8_t(0xFC | ((n & 0x0300) >> 8));
    if (!(n & 0x0020))
      pageTable[2] = uint8_t(0xFC | ((n & 0x0C00) >> 10));
    else
      pageTable[2] = 0xFA;              // U2
    switch (n & 0x00C0) {
    case 0x00:
      pageTable[3] = 0x01;              // CART
      break;
    case 0x40:
      pageTable[3] = 0x00;              // SYS
      break;
    case 0x80:
      pageTable[3] = 0xFB;              // U3
      break;
    case 0xC0:
      pageTable[3] = 0x02;              // EXT
      break;
    }
    for (int i = 0; i < 8; i = i + 2) {
      long    offs = -(long(i) << 13);
      uint8_t segment = pageTable[i >> 1];
      if (segmentTable[segment] != (uint8_t *) 0)
        pageAddressTableR[i] = segmentTable[segment] + offs;
      else
        pageAddressTableR[i] = dummyMemory + offs;
      if (segmentTable[segment] != (uint8_t *) 0)
        pageAddressTableW[i] = segmentTable[segment] + offs;
      else
        pageAddressTableW[i] = dummyMemory + (0x4000L + offs);
      pageAddressTableR[i + 1] = pageAddressTableR[i];
      pageAddressTableW[i + 1] = pageAddressTableW[i];
    }
    if (pageTable[3] == 0x02 || (pageTable[3] == 0x01 && segment1IsExtension)) {
      // IOMEM is special case
      pageAddressTableR[6] = (uint8_t *) 0;
      pageAddressTableW[6] = (uint8_t *) 0;
      if (pageTable[3] == 0x01) {
        pageAddressTableR[7] = (uint8_t *) 0;
        pageAddressTableW[7] = (uint8_t *) 0;
      }
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

  class ChunkType_TVCMemSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Memory& ref;
   public:
    ChunkType_TVCMemSnapshot(Memory& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_TVCMemSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_TVCMEM_STATE;
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
    buf.writeBoolean(segment1IsExtension);
    buf.writeByte(totalRAMSegments);
    for (int i = 0xF8; i <= 0xFF; i++) {
      if (i == 0xFA && totalRAMSegments < 5)
        i = 0xFC;
      if (i == 0xFC && totalRAMSegments < 8)
        i = 0xFF;
      if (segmentTable[i] != (uint8_t *) 0) {
        for (size_t j = 0; j < 16384; j++)
          buf.writeByte(segmentTable[i][j]);
      }
      else {
        for (size_t j = 0; j < 16384; j++)
          buf.writeByte(0xFF);
      }
    }
    for (int i = 0x00; i <= 0x02; i++) {
      if (segmentTable[i] != (uint8_t *) 0 &&
          !(i == 0x01 && segment1IsExtension)) {
        buf.writeByte(uint8_t(i));
        for (size_t j = (i != 2 ? 0 : 8192); j < 16384; j++)
          buf.writeByte(segmentTable[i][j]);
      }
    }
  }

  void Memory::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_TVCMEM_STATE, buf);
  }

  void Memory::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible TVC memory snapshot format");
    }
    // reset memory
    deleteAllSegments();
    try {
      currentPaging = buf.readUInt16();
      segment1IsExtension = buf.readBoolean();
      // load RAM segments
      totalRAMSegments = buf.readByte();
      if (totalRAMSegments != 3 && totalRAMSegments != 5 &&
          totalRAMSegments != 8) {
        throw Ep128Emu::Exception("invalid RAM configuration in TVC snapshot");
      }
      // load RAM segments
      setRAMSize(size_t(totalRAMSegments) << 4);
      for (int i = 0xF8; i <= 0xFF; i++) {
        if (i == 0xFA && totalRAMSegments < 5)
          i = 0xFC;
        if (i == 0xFC && totalRAMSegments < 8)
          i = 0xFF;
        for (size_t j = 0; j < 16384; j++)
          segmentTable[i][j] = buf.readByte();
      }
      // load ROM segments
      while (buf.getPosition() < buf.getDataSize()) {
        uint8_t segment = buf.readByte();
        if (segment > 0x02)
          throw Ep128Emu::Exception("invalid ROM segment in TVC snapshot");
        allocateSegment(segment, true);
        for (size_t i = (segment != 0x02 ? 0 : 8192); i < 16384; i++)
          segmentTable[segment][i] = buf.readByte();
      }
      setPaging(currentPaging);
    }
    catch (...) {
      segment1IsExtension = false;
      setRAMSize(48);
      setPaging(0x3F00);
      throw;
    }
  }

  void Memory::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_TVCMemSnapshot  *p;
    p = new ChunkType_TVCMemSnapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace TVC64

