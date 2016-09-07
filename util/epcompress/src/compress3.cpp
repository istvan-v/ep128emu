
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

#include "ep128emu.hpp"
#include "compress.hpp"
#include "compress3.hpp"

namespace Ep128Compress {

  static size_t getSymbolSize(unsigned int n)
  {
    size_t  nBits = 0;
    while (n > 1U) {
      n = n >> 1;
      nBits++;
    }
    return nBits;
  }

  static unsigned int encodeSymbol(unsigned int n)
  {
    unsigned char nBits = 0;
    unsigned int  tmp = n;
    while (tmp > 1U) {
      tmp = tmp >> 1;
      nBits++;
    }
    n = ((unsigned int) nBits << 24) | (n & ((1U << nBits) - 1U));
    return n;
  }

  void Compressor_M3::SearchTable::sortFunc(
      unsigned int *suffixArray, const unsigned char *buf, size_t bufSize,
      size_t startPos, size_t endPos, unsigned int *tmpBuf)
  {
    if ((startPos + 1) >= endPos)
      return;
    size_t  splitPos = (startPos + endPos) >> 1;
    if ((startPos + 2) < endPos) {
      sortFunc(suffixArray, buf, bufSize, startPos, splitPos, tmpBuf);
      sortFunc(suffixArray, buf, bufSize, splitPos, endPos, tmpBuf);
    }
    size_t  i = startPos;
    size_t  j = splitPos;
    for (size_t k = 0; k < (endPos - startPos); k++) {
      if (i >= splitPos) {
        tmpBuf[k] = suffixArray[j];
        j++;
      }
      else if (j >= endPos) {
        tmpBuf[k] = suffixArray[i];
        i++;
      }
      else {
        size_t  pos1 = suffixArray[i];
        size_t  pos2 = suffixArray[j];
        size_t  len1 = bufSize - pos1;
        if (len1 > Compressor_M3::maxRepeatLen)
          len1 = Compressor_M3::maxRepeatLen;
        size_t  len2 = bufSize - pos2;
        if (len2 > Compressor_M3::maxRepeatLen)
          len2 = Compressor_M3::maxRepeatLen;
        size_t  l = (len1 < len2 ? len1 : len2);
        const void  *ptr1 = (const void *) (&(buf[pos1]));
        const void  *ptr2 = (const void *) (&(buf[pos2]));
        int     c = std::memcmp(ptr1, ptr2, l);
        if (c == 0)
          c = (pos1 < pos2 ? -1 : 1);
        if (c < 0) {
          tmpBuf[k] = suffixArray[i];
          i++;
        }
        else {
          tmpBuf[k] = suffixArray[j];
          j++;
        }
      }
    }
    for (size_t k = 0; k < (endPos - startPos); k++)
      suffixArray[startPos + k] = tmpBuf[k];
  }

  void Compressor_M3::SearchTable::addMatch(size_t bufPos,
                                            size_t matchPos, size_t matchLen)
  {
    size_t  offs = 0;
    if (matchLen > 0) {
      if (matchPos >= bufPos)
        return;
      offs = bufPos - matchPos;
      if (offs > Compressor_M3::maxRepeatDist)
        return;
      if (matchLen > Compressor_M3::maxRepeatLen)
        matchLen = Compressor_M3::maxRepeatLen;
    }
    if ((matchTableBuf.size() + 2) > matchTableBuf.capacity()) {
      size_t  tmp = 1024;
      while (tmp < (matchTableBuf.size() + 2))
        tmp = tmp << 1;
      matchTableBuf.reserve(tmp);
    }
    if (matchTable[bufPos] == 0x7FFFFFFF)
      matchTable[bufPos] = matchTableBuf.size();
    matchTableBuf.push_back((unsigned short) matchLen);
    matchTableBuf.push_back((unsigned short) offs);
  }

  Compressor_M3::SearchTable::SearchTable(
      const std::vector< unsigned char >& inBuf)
    : buf(inBuf)
  {
    if (buf.size() < 1) {
      throw Ep128Emu::Exception("Compressor_M3::SearchTable::SearchTable(): "
                                "zero input buffer size");
    }
    matchTable.resize(buf.size(), 0x7FFFFFFF);
    matchTableBuf.reserve(1024);
    std::vector< unsigned short > rleLengthTable(buf.size(), 0);
    // find RLE (offset = 1) matches
    {
      size_t        rleLength = 1;
      unsigned int  rleByte = (unsigned int) buf[buf.size() - 1];
      for (size_t i = buf.size() - 1; i > 0; ) {
        i--;
        if ((unsigned int) buf[i] != rleByte) {
          rleByte = (unsigned int) buf[i];
          rleLength = 0;
        }
        if (rleLength >= Compressor_M3::minRepeatLen)
          rleLengthTable[i + 1] = (unsigned short) rleLength;
        if (rleLength < size_t(Compressor_M3::lengthMaxValue))
          rleLength++;
      }
    }
    std::vector< unsigned short > offs1Table(
        buf.size(), (unsigned short) Compressor_M3::maxRepeatDist);
    std::vector< unsigned short > offs2Table(
        buf.size(), (unsigned short) Compressor_M3::maxRepeatDist);
    {
      std::vector< size_t > lastPosTable(65536, size_t(0x7FFFFFFF));
      // find 1-byte matches
      for (size_t i = 0; i < buf.size(); i++) {
        unsigned int  c = (unsigned int) buf[i];
        if (lastPosTable[c] < i) {
          size_t  d = i - (lastPosTable[c] + 1);
          if (d < Compressor_M3::maxRepeatDist)
            offs1Table[i] = (unsigned short) d;
        }
        lastPosTable[c] = i;
      }
      // find 2-byte matches
      for (size_t i = 0; i < 256; i++)
        lastPosTable[i] = 0x7FFFFFFF;
      for (size_t i = 0; (i + 1) < buf.size(); i++) {
        unsigned int  c = (unsigned int) buf[i]
                          | ((unsigned int) buf[i + 1] << 8);
        if (lastPosTable[c] < i) {
          size_t  d = i - (lastPosTable[c] + 1);
          if (d < Compressor_M3::maxRepeatDist)
            offs2Table[i] = (unsigned short) d;
        }
        lastPosTable[c] = i;
      }
    }
    // buffer positions sorted alphabetically by bytes at each position
    std::vector< unsigned int >   suffixArray;
    // suffixArray[n] matches prvMatchLenTable[n] characters with
    // suffixArray[n - 1]
    std::vector< unsigned short > prvMatchLenTable;
    // suffixArray[n] matches nxtMatchLenTable[n] characters with
    // suffixArray[n + 1]
    std::vector< unsigned short > nxtMatchLenTable;
    std::vector< unsigned int >   tmpBuf;
    for (size_t startPos = 0; startPos < buf.size(); ) {
      size_t  startPos_ = 0;
      if (startPos >= Compressor_M3::maxRepeatDist)
        startPos_ = startPos - Compressor_M3::maxRepeatDist;
      size_t  endPos = startPos + Compressor_M3::maxRepeatDist;
      if (endPos > buf.size() ||
          buf.size() <= (Compressor_M3::maxRepeatDist * 3)) {
        endPos = buf.size();
      }
      size_t  nBytes = endPos - startPos_;
      tmpBuf.resize(nBytes);
      suffixArray.resize(nBytes);
      prvMatchLenTable.resize(nBytes);
      nxtMatchLenTable.resize(nBytes);
      for (size_t i = 0; i < nBytes; i++)
        suffixArray[i] = (unsigned int) (startPos_ + i);
      sortFunc(&(suffixArray.front()), &(buf.front()), buf.size(), 0, nBytes,
               &(tmpBuf.front()));
      for (size_t i = 0; i < nBytes; i++) {
        if (i == 0) {
          prvMatchLenTable[i] = 0;
        }
        else {
          size_t  len = 0;
          size_t  p1 = suffixArray[i - 1];
          size_t  p2 = suffixArray[i];
          size_t  maxLen = buf.size() - (p1 > p2 ? p1 : p2);
          maxLen = (maxLen < Compressor_M3::maxRepeatLen ?
                    maxLen : Compressor_M3::maxRepeatLen);
          while (len < maxLen && buf[p1] == buf[p2]) {
            len++;
            p1++;
            p2++;
          }
          prvMatchLenTable[i] = (unsigned short) len;
          nxtMatchLenTable[i - 1] = prvMatchLenTable[i];
        }
        nxtMatchLenTable[i] = 0;
      }
      // find all matches
      std::vector< unsigned long >  offsTable(
          Compressor_M3::maxRepeatLen + 1,
          (unsigned long) Compressor_M3::maxRepeatDist);
      for (size_t i_ = 0; i_ < nBytes; i_++) {
        size_t  i = suffixArray[i_];
        if (i < startPos)
          continue;
        size_t  rleLen = rleLengthTable[i];
        if (rleLen > Compressor_M3::maxRepeatLen)
          rleLen = Compressor_M3::maxRepeatLen;
        size_t  minLen = Compressor_M3::minRepeatLen;
        if (rleLen >= minLen) {
          minLen = rleLen + 1;
          offsTable[rleLen] = 0UL;
        }
        if (minLen < 3)
          minLen = 3;
        size_t  maxLen = minLen - 1;
        size_t  matchLen = prvMatchLenTable[i_];
        if (matchLen >= minLen) {
          if (matchLen > maxLen)
            maxLen = matchLen;
          unsigned long d = offsTable[matchLen];
          size_t  ndx = i_;
          while (true) {
            ndx--;
            unsigned long tmp =
                (unsigned long) i - ((unsigned long) suffixArray[ndx] + 1UL);
            d = (tmp < d ? tmp : d);
            if (size_t(prvMatchLenTable[ndx]) < matchLen) {
              if (d < offsTable[matchLen])
                offsTable[matchLen] = d;
              matchLen = prvMatchLenTable[ndx];
              if (matchLen < minLen)
                break;
            }
          }
        }
        matchLen = nxtMatchLenTable[i_];
        if (matchLen >= minLen) {
          if (matchLen > maxLen)
            maxLen = matchLen;
          unsigned long d = offsTable[matchLen];
          size_t  ndx = i_;
          while (true) {
            ndx++;
            unsigned long tmp =
                (unsigned long) i - ((unsigned long) suffixArray[ndx] + 1UL);
            d = (tmp < d ? tmp : d);
            if (size_t(nxtMatchLenTable[ndx]) < matchLen) {
              if (d < offsTable[matchLen])
                offsTable[matchLen] = d;
              matchLen = nxtMatchLenTable[ndx];
              if (matchLen < minLen)
                break;
            }
          }
        }
        offsTable[1] = offs1Table[i];
        offsTable[2] = offs2Table[i];
        unsigned long prvDist = Compressor_M3::maxRepeatDist;
        for (size_t k = maxLen; k >= Compressor_M3::minRepeatLen; k--) {
          unsigned long d = offsTable[k];
          offsTable[k] = Compressor_M3::maxRepeatDist;
          if (d < prvDist) {
            prvDist = d;
            addMatch(i, i - size_t(d + 1UL), k);
            if (d == 0UL)
              break;
          }
        }
        addMatch(i, 0, 0);
      }
      startPos = endPos;
    }
    // find very long matches
    for (size_t i = buf.size() - 1; i > 0; ) {
      if (size_t(matchTableBuf[matchTable[i]]) == Compressor_M3::maxRepeatLen) {
        size_t          len = Compressor_M3::maxRepeatLen;
        unsigned short  offs = matchTableBuf[matchTable[i] + 1];
        while (--i > 0) {
          // NOTE: the offset always changes when the match length decreases
          if (matchTableBuf[matchTable[i] + 1] != offs)
            break;
          len = (len < size_t(Compressor_M3::lengthMaxValue) ? (len + 1) : len);
          matchTableBuf[matchTable[i]] = (unsigned short) len;
        }
        continue;
      }
      i--;
    }
  }

  Compressor_M3::SearchTable::~SearchTable()
  {
  }

  // --------------------------------------------------------------------------

  void Compressor_M3::writeRepeatCode(std::vector< unsigned int >& buf,
                                      size_t d, size_t n)
  {
    if (d > 0) {
      n--;
      if (n == 1) {
        if (d > 510)
          throw Ep128Emu::Exception("internal error: match offset overflow");
        d++;
      }
    }
    unsigned char nBits = (unsigned char) getSymbolSize((unsigned int) n);
    buf.push_back(((unsigned int) (nBits + 1) << 24)
                  | ((1U << (nBits + 1)) - 2U));
    if (n > 1)
      buf.push_back(encodeSymbol((unsigned int) n));
    if (d < 1)
      return;
    nBits = (unsigned char) getSymbolSize((unsigned int) d);
    buf.push_back((n == 1 ? 0x03000000U : 0x04000000U)
                  | (unsigned int) (nBits - (unsigned char) (n == 1)));
    if (d > 1)
      buf.push_back(encodeSymbol((unsigned int) d));
  }

  inline size_t Compressor_M3::getRepeatCodeLength(size_t d, size_t n) const
  {
    size_t  nBits = 0;
    if (d == 0) {
      nBits = (getSymbolSize((unsigned int) n) << 1) + 1;
    }
    else {
      nBits = (getSymbolSize((unsigned int) (n - 1)) << 1)
              + getSymbolSize(d + size_t(n == 2)) + (n == 2 ? 4 : 5);
    }
    return nBits;
  }

  void Compressor_M3::optimizeMatches(LZMatchParameters *matchTable,
                                      size_t *bitCountTable,
                                      unsigned char *bitIncMaxTable,
                                      size_t nBytes)
  {
    for (size_t i = nBytes; --i > 0; ) {
      size_t  bestSize = 0x7FFFFFFF;
      size_t  bestLen = 1;
      size_t  bestOffs = 0;
      const unsigned short  *matchPtr = searchTable->getMatches(i);
      size_t  len = matchPtr[0];        // match length
      if (len > (nBytes - i)) {
        while (size_t(matchPtr[2]) >= (nBytes - i))
          matchPtr = matchPtr + 2;
        len = nBytes - i;
      }
      if (len > 2 || (len == 2 && matchPtr[1] <= 510)) {
        size_t  d = matchPtr[1];
        bestLen = len;
        bestOffs = d;
        bestSize = getRepeatCodeLength(d, len) + size_t((i + len) < nBytes)
                   + bitCountTable[i + len];
        if (len > maxRepeatLen) {
          // long LZ77 match
          if (d == 1) {
            // if a long RLE match is possible, use that
            matchTable[i].d = 1;
            matchTable[i].len = (unsigned short) len;
            bitCountTable[i] = bestSize;
            bitIncMaxTable[i] = bitIncMaxTable[i + len];
            continue;
          }
          len = maxRepeatLen;
        }
        // otherwise check all possible LZ77 match lengths,
        size_t  nxtLen = (matchPtr[2] > 2U ? matchPtr[2] : 2U);
        while (true) {
          if (--len <= nxtLen) {
            if (len <= 2) {
              if (matchPtr[2] == 2)
                d = matchPtr[3];
              if (len == 2 && d <= 510) {
                size_t  nBits = (getSymbolSize((unsigned int) (len - 1)) << 1)
                                + getSymbolSize(d + 1) + 5
                                + bitCountTable[i + len];
                if (nBits < bestSize) {
                  bestSize = nBits;
                  bestOffs = d;
                  bestLen = len;
                }
              }
              break;
            }
            len = nxtLen;
            matchPtr = matchPtr + 2;
            d = matchPtr[1];
            nxtLen = (matchPtr[2] > 2U ? matchPtr[2] : 2U);
          }
          size_t  nBits = (getSymbolSize((unsigned int) (len - 1)) << 1)
                          + getSymbolSize(d) + 6 + bitCountTable[i + len];
          if (nBits < bestSize) {
            bestSize = nBits;
            bestOffs = d;
            bestLen = len;
          }
        }
      }
      {
        size_t  nBitsBase = 9;
        for (size_t k = 1; (i + k) <= nBytes; k++) {
          // and all possible literal sequence lengths
          size_t  nBits = bitCountTable[i + k] + nBitsBase;
          nBitsBase += size_t((k & (k + 1)) != 0 ? 8 : 10);
          if (nBits <= bestSize &&
              !((i + k) < nBytes && matchTable[i + k].d == 0)) {
            // a literal sequence can only be followed by an LZ77 match
            bestSize = nBits;
            bestOffs = 0;
            bestLen = k;
          }
          else if (nBits > (bestSize + 31)) {
            break;
          }
        }
      }
      matchTable[i].d = (unsigned short) bestOffs;
      matchTable[i].len = (unsigned short) bestLen;
      // store total compressed size in bits from this position
      bitCountTable[i] = bestSize;
      // store maximum size increase in bits from this position
      bitIncMaxTable[i] = bitIncMaxTable[i + bestLen];
      if (bestSize > ((nBytes - i) << 3)) {
        unsigned char tmp = (unsigned char) (bestSize - ((nBytes - i) << 3));
        if (tmp > bitIncMaxTable[i])
          bitIncMaxTable[i] = tmp;
      }
    }
    // at position 0: only a literal sequence is possible, with no flag bit
    {
      size_t  bestSize = 0x7FFFFFFF;
      size_t  bestLen = 1;
      size_t  nBitsBase = 9;
      for (size_t k = 1; k <= nBytes; k++) {
        // check all possible literal sequence lengths
        size_t  nBits = bitCountTable[k] + nBitsBase;
        nBitsBase += size_t((k & (k + 1)) != 0 ? 8 : 10);
        // check if the compressed data would
        // overflow a 2-byte decompressor buffer
        if (nBits <= bestSize &&
            !((k < nBytes && matchTable[k].d == 0) ||
              size_t(bitIncMaxTable[k]) >= (((nBits + 7) & 7) + 16))) {
          bestSize = nBits;
          bestLen = k;
        }
      }
      matchTable[0].d = 0;
      matchTable[0].len = (unsigned short) bestLen;
      bitCountTable[0] = bestSize;
    }
  }

  void Compressor_M3::compressData_(std::vector< unsigned int >& tmpOutBuf,
                                    const std::vector< unsigned char >& inBuf)
  {
    size_t  nBytes = inBuf.size();
    tmpOutBuf.clear();
    // compress data by searching for repeated byte sequences,
    // and replacing them with length/distance codes
    std::vector< LZMatchParameters >  matchTable(nBytes);
    std::vector< size_t >         bitCountTable(nBytes + 1, 0);
    std::vector< unsigned char >  bitIncMaxTable(nBytes + 1, 0x00);
    optimizeMatches(&(matchTable.front()), &(bitCountTable.front()),
                    &(bitIncMaxTable.front()), nBytes);
    if (size_t(matchTable[0].len) >= nBytes &&
        (bitCountTable[1] + 1) < ((nBytes - 1) << 3)) {
      std::fprintf(stderr, "WARNING: disabled compression to avoid "
                           "Z80 decompressor buffer overflow\n");
    }
    // write compressed data
    bool    prvLiteralSeqFlag = false;
    for (size_t i = 0; i < nBytes; ) {
      LZMatchParameters&  tmp = matchTable[i];
      if (tmp.d > 0) {
        // write LZ77 match
        if (!prvLiteralSeqFlag)
          tmpOutBuf.push_back(0x01000001U);
        writeRepeatCode(tmpOutBuf, tmp.d, tmp.len);
        i = i + tmp.len;
        prvLiteralSeqFlag = false;
      }
      else {
        // write literal sequence
        if (i != 0)
          tmpOutBuf.push_back(0x01000000U);
        writeRepeatCode(tmpOutBuf, 0, tmp.len);
        for (size_t j = 0; j < size_t(tmp.len); j++) {
          tmpOutBuf.push_back(0x88000000U | (unsigned int) inBuf[i]);
          i++;
        }
        prvLiteralSeqFlag = true;
      }
    }
  }

  Compressor_M3::Compressor_M3(std::vector< unsigned char >& outBuf_)
    : Compressor(outBuf_),
      searchTable((SearchTable *) 0)
  {
  }

  Compressor_M3::~Compressor_M3()
  {
    if (searchTable)
      delete searchTable;
  }

  bool Compressor_M3::compressData(const std::vector< unsigned char >& inBuf,
                                   unsigned int startAddr, bool isLastBlock,
                                   bool enableProgressDisplay)
  {
    (void) enableProgressDisplay;
    // allow start address 0100H (program with EXOS 5 header) for compatibility
    if ((startAddr != 0x0100U && startAddr != 0xFFFFFFFFU) || !isLastBlock) {
      throw Ep128Emu::Exception("Compressor_M3::compressData(): "
                                "internal error: "
                                "unsupported output format parameters");
    }
    if (searchTable) {
      delete searchTable;
      searchTable = (SearchTable *) 0;
    }
    size_t        savedOutBufPos = 0x7FFFFFFF;
    unsigned char outputShiftReg = 0xFF;
    int           outputBitCnt = 0;
    size_t        nBytes = inBuf.size();
    if (nBytes < 1)
      return true;
    if (nBytes > 65535)
      throw Ep128Emu::Exception("input data size is too large");
    std::vector< unsigned int > outBufTmp;
    try {
      std::vector< unsigned char >  inBufRev(nBytes);
      for (size_t i = 0; i < nBytes; i++)
        inBufRev[(nBytes - 1) - i] = inBuf[i];          // reverse input data
      searchTable = new SearchTable(inBufRev);
      std::vector< unsigned int >   tmpBuf;
      compressData_(tmpBuf, inBufRev);
      // calculate compressed size
      size_t  compressedSize = 0;
      for (size_t i = 0; i < tmpBuf.size(); i++)
        compressedSize += size_t((tmpBuf[i] & 0x7F000000U) >> 24);
      compressedSize = (compressedSize + 7) & (~(size_t(7)));
      size_t  uncompressedSize = nBytes * 8;
      if (compressedSize >= uncompressedSize) {
        // if cannot reduce the data size, store without compression
        outBufTmp.push_back(0x88000000U);
        outBufTmp.push_back(0x88000000U);
        for (size_t i = 0; i < nBytes; i++)
          outBufTmp.push_back(0x88000000U | (unsigned int) inBufRev[i]);
        outBufTmp.push_back(0x88000000U | (unsigned int) (nBytes >> 8));
        outBufTmp.push_back(0x88000000U | (unsigned int) (nBytes & 0xFF));
      }
      else {
        size_t  tmp = compressedSize >> 3;
        outBufTmp.push_back(0x88000000U
                            | (unsigned int) ((nBytes - tmp) >> 8));
        outBufTmp.push_back(0x88000000U
                            | (unsigned int) ((nBytes - tmp) & 0xFF));
        // append compressed data to output buffer
        for (size_t i = 0; i < tmpBuf.size(); i++)
          outBufTmp.push_back(tmpBuf[i]);
        outBufTmp.push_back(0x88000000U | (unsigned int) (tmp >> 8));
        outBufTmp.push_back(0x88000000U | (unsigned int) (tmp & 0xFF));
      }
      delete searchTable;
      searchTable = (SearchTable *) 0;
    }
    catch (...) {
      if (searchTable) {
        delete searchTable;
        searchTable = (SearchTable *) 0;
      }
      throw;
    }
    // pack output data
    std::vector< unsigned char >  tmpOutBuf;
    for (size_t i = 0; i < outBufTmp.size(); i++) {
      unsigned int  c = outBufTmp[i];
      if (c >= 0x80000000U) {
        // special case for literal bytes, which are stored byte-aligned
        if (outputBitCnt > 0 && savedOutBufPos >= tmpOutBuf.size()) {
          // reserve space for the shift register to be stored later when
          // it is full, and save the write position
          savedOutBufPos = tmpOutBuf.size();
          tmpOutBuf.push_back((unsigned char) 0x00);
        }
        unsigned int  n = ((c & 0x7F000000U) + 0x07000000U) >> 27;
        while (n > 0U) {
          n--;
          tmpOutBuf.push_back((unsigned char) ((c >> (n * 8U)) & 0xFFU));
        }
      }
      else {
        unsigned int  nBits = c >> 24;
        c = c & 0x00FFFFFFU;
        for (unsigned int j = nBits; j > 0U; ) {
          j--;
          unsigned int  b = (unsigned int) (bool(c & (1U << j)));
          outputShiftReg = ((outputShiftReg & 0xFE) >> 1)
                           | (unsigned char) (b << 7);
          if (++outputBitCnt >= 8) {
            if (savedOutBufPos >= tmpOutBuf.size()) {
              tmpOutBuf.push_back(outputShiftReg);
            }
            else {
              // store at saved position if any literal bytes were inserted
              tmpOutBuf[savedOutBufPos] = outputShiftReg;
              savedOutBufPos = 0x7FFFFFFF;
            }
            outputShiftReg = 0xFF;
            outputBitCnt = 0;
          }
        }
      }
    }
    while (outputBitCnt != 0) {
      outputShiftReg = ((outputShiftReg & 0xFE) >> 1) | 0x80;
      if (++outputBitCnt >= 8) {
        if (savedOutBufPos >= tmpOutBuf.size()) {
          tmpOutBuf.push_back(outputShiftReg);
        }
        else {
          // store at saved position if any literal bytes were inserted
          tmpOutBuf[savedOutBufPos] = outputShiftReg;
          savedOutBufPos = 0x7FFFFFFF;
        }
        outputShiftReg = 0xFF;
        outputBitCnt = 0;
      }
    }
    // reverse output data
    for (size_t i = tmpOutBuf.size(); i-- > 0; )
      outBuf.push_back(tmpOutBuf[i]);
    return true;
  }

}       // namespace Ep128Compress

