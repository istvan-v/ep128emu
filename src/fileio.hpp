
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/ep128emu/
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

#ifndef EP128EMU_FILEIO_HPP
#define EP128EMU_FILEIO_HPP

#include "ep128emu.hpp"
#include <map>

namespace Ep128Emu {

  class File {
   public:
    class Buffer {
     private:
      unsigned char *buf;
      size_t  curPos, dataSize, allocSize;
     public:
      unsigned char readByte();
      bool readBoolean();
      int16_t readInt16();
      uint16_t readUInt16();
      int32_t readInt32();
      uint32_t readUInt32();
      int64_t readInt64();
      uint64_t readUInt64();
      uint64_t readUIntVLen();
      double readFloat();
      std::string readString();
      void writeByte(unsigned char n);
      void writeBoolean(bool n);
      void writeInt16(int16_t n);
      void writeUInt16(uint16_t n);
      void writeInt32(int32_t n);
      void writeUInt32(uint32_t n);
      void writeInt64(int64_t n);
      void writeUInt64(uint64_t n);
      void writeUIntVLen(uint64_t n);
      void writeFloat(double n);
      void writeString(const std::string& n);
      void writeData(const unsigned char *buf_, size_t nBytes);
      void setPosition(size_t pos);
      void clear();
      inline size_t getPosition() const
      {
        return curPos;
      }
      inline size_t getDataSize() const
      {
        return dataSize;
      }
      inline const unsigned char * getData() const
      {
        return buf;
      }
      Buffer();
      Buffer(const unsigned char *buf_, size_t nBytes);
      ~Buffer();
    };
    // ----------------
    typedef enum {
      EP128EMU_CHUNKTYPE_END_OF_FILE =    0x00000000,
      EP128EMU_CHUNKTYPE_Z80_STATE =      0x45508001,
      EP128EMU_CHUNKTYPE_MEMORY_STATE =   0x45508002,
      EP128EMU_CHUNKTYPE_IO_STATE =       0x45508003,
      EP128EMU_CHUNKTYPE_DAVE_STATE =     0x45508004,
      EP128EMU_CHUNKTYPE_NICK_STATE =     0x45508005,
      EP128EMU_CHUNKTYPE_BREAKPOINTS =    0x45508006,
      EP128EMU_CHUNKTYPE_CONFIG_DB =      0x45508007,
      EP128EMU_CHUNKTYPE_VM_CONFIG =      0x45508008,
      EP128EMU_CHUNKTYPE_VM_STATE =       0x45508009,
      EP128EMU_CHUNKTYPE_DEMO_STREAM =    0x4550800A,
      EP128EMU_CHUNKTYPE_M7501_STATE =    0x4550800B,
      EP128EMU_CHUNKTYPE_TED_STATE =      0x4550800C,
      EP128EMU_CHUNKTYPE_P4VM_CONFIG =    0x4550800D,
      EP128EMU_CHUNKTYPE_P4VM_STATE =     0x4550800E,
      EP128EMU_CHUNKTYPE_PLUS4_DEMO =     0x4550800F,
      EP128EMU_CHUNKTYPE_PLUS4_PRG =      0x45508010,
      EP128EMU_CHUNKTYPE_SID_STATE =      0x45508011,
      EP128EMU_CHUNKTYPE_SDEXT_STATE =    0x45508018,
      EP128EMU_CHUNKTYPE_ZXMEM_STATE =    0x45508020,
      EP128EMU_CHUNKTYPE_ZXIO_STATE =     0x45508021,
      EP128EMU_CHUNKTYPE_ZXAY3_STATE =    0x45508022,
      EP128EMU_CHUNKTYPE_ZXULA_STATE =    0x45508023,
      EP128EMU_CHUNKTYPE_ZXVM_CONFIG =    0x45508024,
      EP128EMU_CHUNKTYPE_ZXVM_STATE =     0x45508025,
      EP128EMU_CHUNKTYPE_ZX_DEMO =        0x45508026,
      EP128EMU_CHUNKTYPE_ZX_SNA_FILE =    0x45508027,
      EP128EMU_CHUNKTYPE_ZX_Z80_FILE =    0x45508028,
      EP128EMU_CHUNKTYPE_CPCMEM_STATE =   0x45508030,
      EP128EMU_CHUNKTYPE_CPCIO_STATE =    0x45508031,
      EP128EMU_CHUNKTYPE_M6845_STATE =    0x45508032,
      EP128EMU_CHUNKTYPE_CPCVID_STATE =   0x45508033,
      EP128EMU_CHUNKTYPE_CPCVM_CONFIG =   0x45508034,
      EP128EMU_CHUNKTYPE_CPCVM_STATE =    0x45508035,
      EP128EMU_CHUNKTYPE_CPC_DEMO =       0x45508036,
      EP128EMU_CHUNKTYPE_CPC_SNA_FILE =   0x45508037,
      EP128EMU_CHUNKTYPE_TVCMEM_STATE =   0x45508040,
      EP128EMU_CHUNKTYPE_TVCVID_STATE =   0x45508041,
      EP128EMU_CHUNKTYPE_TVCVM_CONFIG =   0x45508042,
      EP128EMU_CHUNKTYPE_TVCVM_STATE =    0x45508043,
      EP128EMU_CHUNKTYPE_TVC_DEMO =       0x45508044
    } ChunkType;
    // ----------------
    class ChunkTypeHandler {
     public:
      ChunkTypeHandler()
      {
      }
      virtual ~ChunkTypeHandler();
      virtual ChunkType getChunkType() const = 0;
      virtual void processChunk(Buffer& buf) = 0;
    };
   private:
    Buffer  buf;
    std::map< int, ChunkTypeHandler * > chunkTypeDB;
    void loadZXSnapshotFile(std::FILE *f, const char *fileName);
    void loadCompressedFile(std::FILE *f);
   public:
    void addChunk(ChunkType type, const Buffer& buf_);
    void processAllChunks();
    void writeFile(const char *fileName, bool useHomeDirectory = false,
                   bool enableCompression = false);
    void registerChunkType(ChunkTypeHandler *);
    File();
    File(const char *fileName, bool useHomeDirectory = false);
    ~File();
    inline size_t getBufferDataSize() const
    {
      return buf.getDataSize();
    }
    inline const unsigned char *getBufferData() const
    {
      return buf.getData();
    }
    static EP128EMU_REGPARM2 uint32_t hash_32(const unsigned char *buf,
                                              size_t nBytes);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_FILEIO_HPP

