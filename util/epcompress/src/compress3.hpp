
// compressor utility for Enterprise 128 programs
// Copyright (C) 2007-2010 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EPCOMPRESS_COMPRESS3_HPP
#define EPCOMPRESS_COMPRESS3_HPP

#include "ep128emu.hpp"
#include "compress.hpp"

#include <vector>

namespace Ep128Compress {

  class Compressor_M3 : public Compressor {
   private:
    static const size_t minRepeatDist = 1;
    static const size_t maxRepeatDist = 65535;
    static const size_t minRepeatLen = 1;
    static const size_t maxRepeatLen = 512;
    static const unsigned int lengthMaxValue = 65535U;
    // --------
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
    // --------
    SearchTable   *searchTable;
    // --------
    void writeRepeatCode(std::vector< unsigned int >& buf, size_t d, size_t n);
    inline size_t getRepeatCodeLength(size_t d, size_t n) const;
    void optimizeMatches(LZMatchParameters *matchTable,
                         size_t *bitCountTable, unsigned char *bitIncMaxTable,
                         size_t nBytes);
    void compressData_(std::vector< unsigned int >& tmpOutBuf,
                       const std::vector< unsigned char >& inBuf);
   public:
    Compressor_M3(std::vector< unsigned char >& outBuf_);
    virtual ~Compressor_M3();
    // 'startAddr' must be 0xFFFFFFFF
    // 'isLastBlock' must be true
    // 'enableProgressDisplay' is ignored
    virtual bool compressData(const std::vector< unsigned char >& inBuf,
                              unsigned int startAddr, bool isLastBlock,
                              bool enableProgressDisplay = false);
  };

}       // namespace Ep128Compress

#endif  // EPCOMPRESS_COMPRESS3_HPP

