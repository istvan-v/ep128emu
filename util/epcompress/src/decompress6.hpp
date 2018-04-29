
// compressor utility for Enterprise 128 programs
// Copyright (C) 2007-2018 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EPCOMPRESS_DECOMPRESS6_HPP
#define EPCOMPRESS_DECOMPRESS6_HPP

#include "ep128emu.hpp"
#include "compress.hpp"
#include <vector>

namespace Ep128Compress {

  class M6Encoder {
   public:
    std::vector< unsigned char >  buf;
   protected:
    std::vector< unsigned int > lengthEncodeTable;
    std::vector< unsigned int > offs1EncodeTable;
    std::vector< unsigned int > offs2EncodeTable;
    std::vector< unsigned int > offs3EncodeTable;
    std::vector< unsigned int > lengthDecodeTable;
    std::vector< unsigned int > offs1DecodeTable;
    std::vector< unsigned int > offs2DecodeTable;
    std::vector< unsigned int > offs3DecodeTable;
    bool    reverseDirection;
    bool    rightShifting;
    bool    literals9Bit;
    bool    flagBitsInverted;
    unsigned char shiftReg;
    unsigned char shiftRegBits;
    bool    prvLZ77Flag;
    size_t  bufPos;
    size_t  minLength;
    size_t  maxLength;
    size_t  maxOffs1;
    size_t  maxOffs2;
    size_t  maxOffs3;
    size_t  lengthBits;
    size_t  offs1Bits;
    size_t  offs2Bits;
    size_t  offs3Bits;
    // ----------------
    const char *parseEncoding(std::vector< unsigned int >& encodeTable,
                              std::vector< unsigned int >& decodeTable,
                              size_t& prefixBits, const char *s);
    static inline size_t getEncodedSize(size_t n,
                                        const std::vector< unsigned int >& t)
    {
      return size_t(((t[n << 1] + t[(n << 1) + 1]) & 0x7F000000U) >> 24);
    }
   public:
    M6Encoder();
    virtual ~M6Encoder();
    void parseEncoding(const char *s);
    inline bool getLiterals9Bit() const
    {
      return literals9Bit;
    }
    inline size_t getLiteralSequenceSize(size_t n) const
    {
      if (literals9Bit)
        return (n * 9);
      unsigned char nBits = 0;
      while (n >= (size_t(1) << (nBits + 1)))
        nBits++;
      return ((n << 3) + size_t((nBits << 1) + 1));
    }
    inline size_t getLZ77SequenceSize(size_t n, size_t d) const
    {
      const size_t  invalidSize = 0x0FFFFFFF;
      if (n < minLength || n > maxLength)
        return invalidSize;
      size_t  nBits = getEncodedSize(n + 1 - minLength, lengthEncodeTable) + 1;
      if (EP128EMU_UNLIKELY(n < 3)) {
        if (n < 2) {
          if (d > maxOffs1)
            return invalidSize;
          nBits = nBits + getEncodedSize(d, offs1EncodeTable);
        }
        else {
          if (d > maxOffs2)
            return invalidSize;
          nBits = nBits + getEncodedSize(d, offs2EncodeTable);
        }
      }
      else {
        if (d > maxOffs3)
          return invalidSize;
        nBits = nBits + getEncodedSize(d, offs3EncodeTable);
      }
      return nBits;
    }
    void reverseBuffer(std::vector< unsigned char >& tmpBuf) const;
    void reverseBuffer()
    {
      reverseBuffer(buf);
    }
    unsigned char readByte();
    unsigned int readBits(size_t nBits);
    // returns < 0 on literal sequence, 0 on end of data
    int readLength();
   protected:
    size_t decodeOffset(const std::vector< unsigned int >& decodeTable,
                        size_t prefixBits);
   public:
    size_t readOffset(size_t len);
    bool isEOF() const;
    void writeByte(unsigned char b);
    void writeBits(unsigned int b);
    void writeLiteralSequence(const unsigned char *tmpBuf, size_t nBytes);
    // end of data if len == 0
    void writeLZ77Sequence(size_t len, size_t offs);
    void flush();
    void clear();
  };

  class Decompressor_M6 : public Decompressor {
   private:
    M6Encoder   decoder;
   public:
    Decompressor_M6();
    virtual ~Decompressor_M6();
    // both functions assume "raw" compressed data block with no start address
    virtual void decompressData(
        std::vector< std::vector< unsigned char > >& outBuf,
        const std::vector< unsigned char >& inBuf);
    virtual void decompressData(
        std::vector< unsigned char >& outBuf,
        const std::vector< unsigned char >& inBuf);
    void setEncoding(const char *s);
  };

}       // namespace Ep128Compress

#endif  // EPCOMPRESS_DECOMPRESS6_HPP

