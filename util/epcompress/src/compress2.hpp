
// compressor utility for Enterprise 128 programs
// Copyright (C) 2007-2009 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EPCOMPRESS_COMPRESS2_HPP
#define EPCOMPRESS_COMPRESS2_HPP

#include "ep128emu.hpp"
#include "compress.hpp"

#include <vector>

namespace Ep128Compress {

  class Compressor_M2 : public Compressor {
   private:
    static const size_t minRepeatDist = 1;
    static const size_t maxRepeatDist = 65535;
    static const size_t minRepeatLen = 1;
    static const size_t maxRepeatLen = 512;
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
    static const size_t literalSequenceMinLength = lengthNumSlots + 9;
    // --------
   public:
    class EncodeTable {
     private:
      size_t    nSlots;
      size_t    nSymbols;
      size_t    nSymbolsUsed;
      size_t    nSymbolsEncoded;
      size_t    totalSlotWeight;
      size_t    unusedSymbolSize;
      size_t    minPrefixSize;
      size_t    maxPrefixSize;
      size_t    prefixOnlySymbolCnt;
      std::vector< size_t >   prefixSlotCntTable;
      std::vector< size_t >   slotPrefixSizeTable;
      std::vector< size_t >   slotWeightTable;
      std::vector< size_t >   slotBitsTable;
      std::vector< unsigned int >   slotBaseSymbolTable;
      std::vector< unsigned int >   symbolCntTable;
      std::vector< unsigned int >   unencodedSymbolCostTable;
      std::vector< unsigned short > symbolSlotNumTable;
      std::vector< unsigned short > symbolSizeTable;
      void setPrefixSize(size_t n);
      inline size_t calculateEncodedSize() const;
      inline size_t calculateEncodedSize(size_t firstSlot,
                                         unsigned int firstSymbol,
                                         size_t baseSize) const;
      size_t optimizeSlotBitsTable_fast();
      size_t optimizeSlotBitsTable();
     public:
      // If 'slotPrefixSizeTable_' is non-NULL, a variable prefix length
      // encoding is generated with 'nSlots_' slots, and the table is expected
      // to contain the prefix size in bits for each slot.
      // Otherwise, if 'minPrefixSize_' is greater than or equal to
      // 'maxPrefixSize_', a fixed prefix size of 'minPrefixSize_' bits will be
      // used with 'nSlots_' slots, and 'nSlots_' must be less than or equal to
      // 2 ^ 'minPrefixSize_'.
      // Finally, if 'maxPrefixSize_' is greater than 'minPrefixSize_', then
      // all fixed prefix sizes in the specified range are tried, and the one
      // that results in the smallest encoded size will be used. The number of
      // slots, which must be less than or equal to 2 ^ prefix_size, can be
      // specified for each prefix size in 'prefixSlotCntTable_' (the number of
      // elements is 'maxPrefixSize_' + 1 - 'minPrefixSize_'); if the table is
      // NULL, then the number of slots defaults to the maximum possible value
      // (2 ^ prefix_size).
      // In all cases, 'nSymbols_' is the highest value to be encoded + 1, so
      // the valid range will be 0 to 'nSymbols_' - 1.
      EncodeTable(size_t nSlots_, size_t nSymbols_,
                  const size_t *slotPrefixSizeTable_ = (size_t *) 0,
                  size_t minPrefixSize_ = 4,
                  size_t maxPrefixSize_ = 0,
                  const size_t *prefixSlotCntTable_ = (size_t *) 0);
      virtual ~EncodeTable();
      inline void addSymbol(unsigned int n, size_t unencodedCost = 16384)
      {
        symbolCntTable[n] += 1U;
        unencodedSymbolCostTable[n] += (unsigned int) unencodedCost;
        if (size_t(n) >= nSymbolsUsed)
          nSymbolsUsed = size_t(n) + 1;
      }
      inline void addPrefixOnlySymbol()
      {
        prefixOnlySymbolCnt++;
      }
      inline void setUnencodedSymbolSize(size_t n)
      {
        unusedSymbolSize = n;
      }
      inline size_t getSymbolsEncoded() const
      {
        return nSymbolsEncoded;
      }
      inline size_t getSymbolSize(unsigned int n) const
      {
        if (size_t(n) >= nSymbolsEncoded)
          return unusedSymbolSize;
        return symbolSizeTable[n];
      }
      inline size_t getSymbolSlotIndex(unsigned int n) const
      {
        if (size_t(n) >= nSymbolsEncoded)
          throw Ep128Emu::Exception("internal error: encoding invalid symbol");
        return size_t(symbolSlotNumTable[n]);
      }
      inline unsigned int encodeSymbol(unsigned int n) const
      {
        size_t  slotNum = getSymbolSlotIndex(n);
        return ((unsigned int) (slotBitsTable[slotNum] << 24)
                | (n - slotBaseSymbolTable[slotNum]));
      }
      inline size_t getSlotCnt() const
      {
        return slotBitsTable.size();
      }
      inline size_t getSlotPrefixSize(size_t n) const
      {
        return slotPrefixSizeTable[n];
      }
      inline size_t getSlotSize(size_t n) const
      {
        return slotBitsTable[n];
      }
      void updateTables(bool fastMode = false);
      void clear();
    };
    // --------
   private:
    class SearchTable {
     private:
      const std::vector< unsigned char >&   buf;
      // for each buffer position P, matchTableBuf[matchTable[P]] is the
      // first element of an array of interleaved length/offset pairs,
      // terminated with zero length and offset
      std::vector< size_t > matchTable;
      // space allocated for matchTable
      std::vector< unsigned short > matchTableBuf;
      size_t  matchTableBufPos;
      static void sortFunc(unsigned int *suffixArray,
                           const unsigned char *buf, size_t bufSize,
                           size_t startPos, size_t endPos,
                           unsigned int *tmpBuf);
      void addMatch(size_t bufPos, size_t matchPos, size_t matchLen);
     public:
      SearchTable(const std::vector< unsigned char >& inBuf);
      virtual ~SearchTable();
      inline const unsigned short * getMatches(size_t bufPos) const
      {
        return (&(matchTableBuf.front()) + matchTable[bufPos]);
      }
    };
    // --------
    struct LZMatchParameters {
      unsigned short  d;
      unsigned short  len;
      LZMatchParameters()
        : d(0),
          len(1)
      {
      }
      LZMatchParameters(const LZMatchParameters& r)
        : d(r.d),
          len(r.len)
      {
      }
      ~LZMatchParameters()
      {
      }
      inline LZMatchParameters& operator=(const LZMatchParameters& r)
      {
        d = r.d;
        len = r.len;
        return (*this);
      }
      inline void clear()
      {
        d = 0;
        len = 1;
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
    SearchTable   *searchTable;
    size_t        savedOutBufPos;
    unsigned char outputShiftReg;
    int           outputBitCnt;
    // --------
    void writeRepeatCode(std::vector< unsigned int >& buf, size_t d, size_t n);
    inline size_t getRepeatCodeLength(size_t d, size_t n) const;
    void optimizeMatches_noStats(LZMatchParameters *matchTable,
                                 size_t *bitCountTable,
                                 size_t offs, size_t nBytes);
    void optimizeMatches(LZMatchParameters *matchTable,
                         size_t *bitCountTable, size_t *offsSumTable,
                         size_t offs, size_t nBytes);
    size_t compressData_(std::vector< unsigned int >& tmpOutBuf,
                         const std::vector< unsigned char >& inBuf,
                         size_t offs, size_t nBytes, bool firstPass,
                         bool fastMode = false);
    bool compressData(std::vector< unsigned int >& tmpOutBuf,
                      const std::vector< unsigned char >& inBuf,
                      unsigned int startAddr, bool isLastBlock,
                      size_t offs = 0, size_t nBytes = 0x7FFFFFFFUL,
                      bool fastMode = false);
   public:
    Compressor_M2(std::vector< unsigned char >& outBuf_);
    virtual ~Compressor_M2();
    // if 'startAddr' is 0xFFFFFFFF, it is not stored in the compressed data
    virtual bool compressData(const std::vector< unsigned char >& inBuf,
                              unsigned int startAddr, bool isLastBlock,
                              bool enableProgressDisplay = false);
  };

}       // namespace Ep128Compress

#endif  // EPCOMPRESS_COMPRESS2_HPP

