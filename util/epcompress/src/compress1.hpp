
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

#ifndef EPCOMPRESS_COMPRESS1_HPP
#define EPCOMPRESS_COMPRESS1_HPP

#include "ep128emu.hpp"
#include "compress.hpp"
#include "comprlib.hpp"

#include <vector>

namespace Ep128Compress {

  class Compressor_M1 : public Compressor {
   private:
    static const size_t minRepeatDist = 1;
    static const size_t maxRepeatDist = 524288;
    static const size_t minRepeatLen = 1;
    static const size_t maxRepeatLen = 512;
    static const size_t maxSequenceDist = 32;   // for matches with delta value
    static const size_t seqOffsetBits = 5;
    static const unsigned int lengthMaxValue = 65535U;
    static const size_t lengthNumSlots = 8;
    static const size_t lengthPrefixSizeTable[lengthNumSlots];
    static const unsigned int offs1MaxValue = 4096U;
    static const size_t offs1NumSlots = 4;
    static const size_t offs1PrefixSize = 2;
    static const unsigned int offs2MaxValue = 16384U;
    static const size_t offs2NumSlots = 8;
    static const size_t offs2PrefixSize = 3;
    static const unsigned int offs3MaxValue = (unsigned int) maxRepeatDist;
    static const size_t offs3SlotCntTable[4];
    static const size_t literalSequenceMinLength = 18;
    static const size_t literalSequenceMaxLength = 271;
    // --------
    class DSearchTable : public LZSearchTable {
     private:
      std::vector< unsigned short > seqLenTable;
      std::vector< unsigned char >  seqDistTable;
      std::vector< unsigned char >  seqDiffTable;
     public:
      DSearchTable(size_t minLength, size_t maxLength, size_t lengthMaxValue,
                   size_t maxOffs1, size_t maxOffs2, size_t maxOffs);
      virtual ~DSearchTable();
      void findMatches(const unsigned char *buf, size_t bufSize);
      inline size_t getSequenceLength(size_t bufPos) const
      {
        return size_t(seqLenTable[bufPos]);
      }
      inline size_t getSequenceOffset(size_t bufPos) const
      {
        return size_t(seqDistTable[bufPos]);
      }
      inline unsigned char getSequenceDeltaValue(size_t bufPos) const
      {
        return seqDiffTable[bufPos];
      }
    };
    // --------
    struct LZMatchParameters {
      unsigned short  d;
      unsigned short  len;
      unsigned char   seqDiff;
      LZMatchParameters()
        : d(0),
          len(1),
          seqDiff(0x00)
      {
      }
      LZMatchParameters(const LZMatchParameters& r)
        : d(r.d),
          len(r.len),
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
        seqDiff = r.seqDiff;
        return (*this);
      }
      inline void clear()
      {
        d = 0;
        len = 1;
        seqDiff = 0x00;
      }
    };
    struct SplitOptimizationBlock {
      size_t  startPos;
      size_t  nBytes;
    };
    // --------
    EncodeTable   lengthEncodeTable;
    EncodeTable   offs1EncodeTable;
    EncodeTable   offs2EncodeTable;
    EncodeTable   offs3EncodeTable;
    size_t        offs3NumSlots;
    size_t        offs3PrefixSize;
    DSearchTable  *searchTable;
    size_t        savedOutBufPos;
    unsigned char outputShiftReg;
    int           outputBitCnt;
    // --------
    void writeRepeatCode(std::vector< unsigned int >& buf, size_t d, size_t n);
    inline size_t getRepeatCodeLength(size_t d, size_t n) const;
    void writeSequenceCode(std::vector< unsigned int >& buf,
                           unsigned char seqDiff, size_t d, size_t n);
    inline size_t getSequenceCodeLength(size_t d, size_t n) const;
    void optimizeMatches_noStats(LZMatchParameters *matchTable,
                                 size_t *bitCountTable,
                                 size_t offs, size_t nBytes);
    void optimizeMatches(LZMatchParameters *matchTable,
                         size_t *bitCountTable, uint64_t *offsSumTable,
                         size_t offs, size_t nBytes);
    void compressData_(std::vector< unsigned int >& tmpOutBuf,
                       const std::vector< unsigned char >& inBuf,
                       size_t offs, size_t nBytes, bool firstPass,
                       bool fastMode = false);
    bool compressData(std::vector< unsigned int >& tmpOutBuf,
                      const std::vector< unsigned char >& inBuf,
                      bool isLastBlock, size_t offs = 0,
                      size_t nBytes = 0x7FFFFFFFUL, bool fastMode = false);
   public:
    Compressor_M1(std::vector< unsigned char >& outBuf_);
    virtual ~Compressor_M1();
    // 'startAddr' must be 0xFFFFFFFF
    virtual bool compressData(const std::vector< unsigned char >& inBuf,
                              unsigned int startAddr, bool isLastBlock,
                              bool enableProgressDisplay = false);
  };

}       // namespace Ep128Compress

#endif  // EPCOMPRESS_COMPRESS1_HPP

