
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

#include "ep128emu.hpp"
#include "compress.hpp"
#include "comprlib.hpp"
#include "compress6.hpp"

namespace Ep128Compress {

  void Compressor_M6::optimizeMatches(LZMatchParameters *matchTable,
                                      size_t *bitCountTable, size_t nBytes)
  {
    size_t  maxLen = (config.splitOptimizationDepth < 9 ? maxRepeatLen : 1023);
    size_t  minLen = config.minLength;
    for (size_t i = nBytes; i-- > 0; ) {
      size_t  bestSize = 0x7FFFFFFF;
      size_t  bestLen = 1;
      size_t  bestOffs = 0;
      const unsigned int  *matchPtr = searchTable->getMatches(i);
      size_t  len = *matchPtr;          // match length
      if (len >= minLen) {
        bestLen = len;
        bestOffs = *(++matchPtr) >> 10;
        bestSize = encoder.getLZ77SequenceSize(len, bestOffs)
                   + bitCountTable[i + len];
        if (len > maxLen) {
          // long LZ77 match
          if (bestOffs == 1) {
            // if a long RLE match is possible, use that
            matchTable[i].d = 1;
            matchTable[i].len = (unsigned int) len;
            bitCountTable[i] = bestSize;
            continue;
          }
          len = maxLen;
        }
        // otherwise check all possible LZ77 match lengths,
        for ( ; len > 0; len = (*matchPtr & 0x03FFU)) {
          unsigned int  d = *(matchPtr++) >> 10;
          while (len >= minLen) {
            size_t  nBits = encoder.getLZ77SequenceSize(len, d)
                            + bitCountTable[i + len];
            if (nBits < bestSize) {
              bestSize = nBits;
              bestOffs = d;
              bestLen = len;
            }
            len--;
          }
        }
      }
      if (encoder.getLiterals9Bit()) {
        // and literal byte
        size_t  nBits = bitCountTable[i + 1] + 9;
        if (nBits <= bestSize) {
          bestSize = nBits;
          bestOffs = 0;
          bestLen = 1;
          if ((i + 1) < nBytes && matchTable[i + 1].d == 0)
            bestLen = matchTable[i + 1].len + 1;
        }
      }
      else {
        size_t  nBitsBase = 1;
        for (size_t k = 1; (i + k) <= nBytes; k++) {
          // or all possible literal sequence lengths
          size_t  nBits = nBitsBase + (k << 3) + bitCountTable[i + k];
          if (nBits <= bestSize &&
              !((i + k) < nBytes && matchTable[i + k].d == 0)) {
            // a literal sequence can only be followed by an LZ77 match
            bestSize = nBits;
            bestOffs = 0;
            bestLen = k;
          }
          else if (nBits > (bestSize + 47)) {
            break;
          }
          if (!(k & (k + 1)))
            nBitsBase = nBitsBase + 2;
        }
      }
      matchTable[i].d = (unsigned int) bestOffs;
      matchTable[i].len = (unsigned int) bestLen;
      // store total compressed size in bits from this position
      bitCountTable[i] = bestSize;
    }
  }

  void Compressor_M6::compressData_(const std::vector< unsigned char >& inBuf)
  {
    size_t  nBytes = inBuf.size();
    encoder.clear();
    // compress data by searching for repeated byte sequences,
    // and replacing them with length/distance codes
    std::vector< LZMatchParameters >  matchTable(nBytes);
    std::vector< size_t >         bitCountTable(nBytes + 1, 0);
    optimizeMatches(&(matchTable.front()), &(bitCountTable.front()), nBytes);
    // write compressed data
    for (size_t i = 0; i < nBytes; ) {
      LZMatchParameters&  tmp = matchTable[i];
      if (tmp.d > 0U) {
        // write LZ77 match
        encoder.writeLZ77Sequence(tmp.len, tmp.d);
      }
      else {
        // write literal sequence
        encoder.writeLiteralSequence(&(inBuf.front()) + i, tmp.len);
      }
      i = i + tmp.len;
    }
    // end of data
    encoder.writeLZ77Sequence(0, 0);
  }

  Compressor_M6::Compressor_M6(std::vector< unsigned char >& outBuf_)
    : Compressor(outBuf_),
      searchTable((LZSearchTable *) 0)
  {
  }

  Compressor_M6::~Compressor_M6()
  {
    if (searchTable)
      delete searchTable;
  }

  bool Compressor_M6::compressData(const std::vector< unsigned char >& inBuf,
                                   unsigned int startAddr, bool isLastBlock,
                                   bool enableProgressDisplay)
  {
    (void) enableProgressDisplay;
    // allow start address 0100H (program with EXOS 5 header) for compatibility
    if ((startAddr != 0x0100U && startAddr != 0xFFFFFFFFU) || !isLastBlock) {
      throw Ep128Emu::Exception("Compressor_M6::compressData(): "
                                "internal error: "
                                "unsupported output format parameters");
    }
    if (searchTable) {
      delete searchTable;
      searchTable = (LZSearchTable *) 0;
    }
    encoder.clear();
    size_t        nBytes = inBuf.size();
    if (nBytes < 1)
      return true;
    if (nBytes > 0x00100000)
      throw Ep128Emu::Exception("input data size is too large");
    std::vector< unsigned int > outBufTmp;
    try {
      std::vector< unsigned char >  inBufRev;
      inBufRev.insert(inBufRev.end(), inBuf.begin(), inBuf.end());
      encoder.reverseBuffer(inBufRev);
      searchTable =
          new LZSearchTable(
              config.minLength,
              (config.splitOptimizationDepth < 9 ? maxRepeatLen : 1023),
              lengthMaxValue,
              (config.maxOffset < 1024 ? config.maxOffset : 1024),
              (config.maxOffset < 65536 ? config.maxOffset : 65536),
              (config.maxOffset < maxRepeatDist ?
               config.maxOffset : maxRepeatDist));
      searchTable->findMatches(&(inBufRev.front()), 0, nBytes);
      compressData_(inBufRev);
      delete searchTable;
      searchTable = (LZSearchTable *) 0;
      // pack output data
      encoder.flush();
      encoder.reverseBuffer();
      outBuf.insert(outBuf.end(), encoder.buf.begin(), encoder.buf.end());
      encoder.clear();
    }
    catch (...) {
      if (searchTable) {
        delete searchTable;
        searchTable = (LZSearchTable *) 0;
      }
      encoder.clear();
      throw;
    }
    return true;
  }

  void Compressor_M6::setEncoding(const char *s)
  {
    encoder.parseEncoding(s);
  }

}       // namespace Ep128Compress

