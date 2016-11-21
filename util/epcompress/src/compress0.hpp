
// compressor utility for Enterprise 128 programs
// Copyright (C) 2007-2016 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EPCOMPRESS_COMPRESS0_HPP
#define EPCOMPRESS_COMPRESS0_HPP

#include "ep128emu.hpp"
#include "compress.hpp"
#include "comprlib.hpp"

#include <vector>

namespace Ep128Compress {

  class Compressor_M0 : public Compressor {
   private:
    static const size_t minRepeatDist = 1;
    static const size_t maxRepeatDist = 65536;
    static const size_t minRepeatLen = 2;
    static const size_t maxRepeatLen = 256;
    // --------
    class DSearchTable : public LZSearchTable {
     private:
      std::vector< std::vector< unsigned char > >   seqDiffTable;
      std::vector< std::vector< unsigned short > >  maxSeqLenTable;
     public:
      DSearchTable(size_t minLength, size_t maxLength, size_t maxOffs);
      virtual ~DSearchTable();
      void findMatches(const unsigned char *buf, size_t bufSize);
      inline size_t getSequenceLength(size_t bufPos, size_t d) const
      {
        return size_t(maxSeqLenTable[d - Compressor_M0::minRepeatDist][bufPos]);
      }
      inline unsigned char getSequenceDeltaValue(size_t bufPos, size_t d) const
      {
        return seqDiffTable[d - Compressor_M0::minRepeatDist][bufPos];
      }
    };
    // --------
    struct LZMatchParameters {
      unsigned int    d;
      unsigned short  len;
      bool            seqFlag;
      unsigned char   seqDiff;
      LZMatchParameters()
        : d(0),
          len(1),
          seqFlag(false),
          seqDiff(0x00)
      {
      }
      LZMatchParameters(const LZMatchParameters& r)
        : d(r.d),
          len(r.len),
          seqFlag(r.seqFlag),
          seqDiff(r.seqDiff)
      {
      }
      ~LZMatchParameters()
      {
      }
      inline LZMatchParameters& operator=(const LZMatchParameters& r)
      {
        d = r.d;
        len = r.len;
        seqFlag = r.seqFlag;
        seqDiff = r.seqDiff;
        return (*this);
      }
      inline void clear()
      {
        d = 0;
        len = 1;
        seqFlag = false;
        seqDiff = 0x00;
      }
    };
    // --------
    struct BitCountTableEntry {
      long    totalBits;
      unsigned int  prvDistances[4];
    };
    // --------
    struct SplitOptimizationBlock {
      std::vector< unsigned int > buf;
      size_t  startPos;
      size_t  nBytes;
      size_t  compressedSize;
      bool    isLastBlock;
    };
    // --------
    unsigned short  *lengthCodeTable;
    unsigned char   *lengthBitsTable;
    unsigned int    *lengthValueTable;
    unsigned short  *distanceCodeTable;
    unsigned char   *distanceBitsTable;
    unsigned int    *distanceValueTable;
    size_t          *tmpCharBitsTable;
    DSearchTable    *searchTable;
    size_t          prvDistances[4];
    unsigned char   outputShiftReg;
    int             outputBitCnt;
    // for literals and distance codes
    HuffmanEncoder  huffmanEncoder1;
    // for length codes
    HuffmanEncoder  huffmanEncoder2;
    // --------
    void huffmanCompressBlock(std::vector< unsigned int >& ioBuf);
    void initializeLengthCodeTables();
    void writeRepeatCode(std::vector< unsigned int >& buf, size_t d, size_t n);
    void writeSequenceCode(std::vector< unsigned int >& buf,
                           unsigned char seqDiff, size_t d, size_t n);
    void optimizeMatches(LZMatchParameters *matchTable,
                         BitCountTableEntry *bitCountTable,
                         const size_t *lengthBitsTable_,
                         const unsigned char *inBuf,
                         size_t offs, size_t nBytes);
    void compressData_(std::vector< unsigned int >& tmpOutBuf,
                       const std::vector< unsigned char >& inBuf,
                       unsigned int startAddr, size_t offs, size_t nBytes);
    bool compressData(std::vector< unsigned int >& tmpOutBuf,
                      const std::vector< unsigned char >& inBuf,
                      unsigned int startAddr, bool isLastBlock,
                      size_t offs = 0, size_t nBytes = 0x7FFFFFFFUL);
   public:
    Compressor_M0(std::vector< unsigned char >& outBuf_);
    virtual ~Compressor_M0();
    // if 'startAddr' is 0xFFFFFFFF, it is not stored in the compressed data
    virtual bool compressData(const std::vector< unsigned char >& inBuf,
                              unsigned int startAddr, bool isLastBlock,
                              bool enableProgressDisplay = false);
  };

}       // namespace Ep128Compress

#endif  // EPCOMPRESS_COMPRESS0_HPP

