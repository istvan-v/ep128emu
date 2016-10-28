
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

// Deflate, ZLib and PNG format specifications:
//   https://www.ietf.org/rfc/rfc1951.txt
//   https://www.ietf.org/rfc/rfc1950.txt
//   https://tools.ietf.org/html/rfc2083

#include "ep128emu.hpp"
#include "system.hpp"
#include "pngwrite.hpp"

#define DEFLATE_MAX_THREADS     4
#if 0
#  define PNGWRITE_DEBUG        1
#endif

namespace Ep128Emu {

  // 0:       unused symbol
  // 1 to 15: code length in bits
  // 16:      repeat the last code length 3 to 6 times (2 extra bits)
  // 17:      repeat unused symbol 3 to 10 times (3 extra bits)
  // 18:      repeat unused symbol 11 to 138 times (7 extra bits)

  static const unsigned char deflateCodeLengthCodeTable[19] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
  };

  static const unsigned char deflateCodeLengthIndexTable[19] = {
    3, 17, 15, 13, 11, 9, 7, 5, 4, 6, 8, 10, 12, 14, 16, 18, 0, 1, 2
  };

  // --------------------------------------------------------------------------

  class HuffmanNode {
   private:
    // bits 40 to 63: weight
    // bits 30 to 39: value
    // bits 15 to 29: buffer position + 1 of next node in package (0 = none)
    // bits  0 to 14: buffer position + 1 of this node
    uint64_t    nodeData;
   public:
    HuffmanNode()
      : nodeData(0UL)
    {
    }
    ~HuffmanNode()
    {
    }
    inline void initNode(unsigned int value_, size_t weight_)
    {
      nodeData = (uint64_t(weight_) << 40) | (uint64_t(value_) << 30);
    }
    inline void setBufPos(HuffmanNode *buf)
    {
      nodeData = (nodeData & (uint64_t(-(int64_t(0x40000000L)))))
                 | uint64_t((this - buf) + 1);
    }
    inline size_t getBufPos() const
    {
      return (size_t(nodeData & 0x7FFFUL) - 1);
    }
    inline bool operator<(const HuffmanNode& r) const
    {
      return (nodeData < r.nodeData);
    }
    inline size_t weight() const
    {
      return size_t((nodeData >> 40) & 0x00FFFFFFUL);
    }
    inline unsigned int value() const
    {
      return (unsigned int) ((nodeData >> 30) & 0x03FFUL);
    }
    inline HuffmanNode *nextNode(HuffmanNode *buf)
    {
      size_t  nextPos = size_t((nodeData >> 15) & 0x7FFFUL);
      if (!nextPos)
        return (HuffmanNode *) 0;
      return &(buf[nextPos - 1]);
    }
    inline void merge(HuffmanNode& r, HuffmanNode *buf)
    {
      nodeData = nodeData + (r.nodeData & (uint64_t(0xFFFFFF00UL) << 32));
      r.nodeData = r.nodeData & ((uint64_t(1) << 40) - 1UL);
      HuffmanNode *p = this;
      if (nodeData & 0x3FFF8000UL) {
        if (!(r.nodeData & 0x3FFF8000UL)) {
          r.nodeData = r.nodeData | (nodeData & 0x3FFF8000UL);
          nodeData = nodeData ^ (nodeData & 0x3FFF8000UL);
        }
        else {
          do {
            p = &(buf[((p->nodeData & 0x3FFF8000UL) >> 15) - 1]);
          } while (p->nodeData & 0x3FFF8000UL);
        }
      }
      p->nodeData = p->nodeData | ((r.nodeData & 0x7FFFUL) << 15);
    }
    static void sortNodes(HuffmanNode *startPtr, HuffmanNode *endPtr,
                          HuffmanNode *tmpBuf);
  };

  void HuffmanNode::sortNodes(HuffmanNode *startPtr, HuffmanNode *endPtr,
                              HuffmanNode *tmpBuf)
  {
    size_t  n = size_t(endPtr - startPtr);
    if (n < 2)
      return;
    size_t  m = n >> 1;
    if (m > 1)
      sortNodes(startPtr, startPtr + m, tmpBuf);
    if ((n - m) > 1)
      sortNodes(startPtr + m, endPtr, tmpBuf);
    size_t  i = 0;
    size_t  j = m;
    for (size_t k = 0; k < n; k++) {
      if (EP128EMU_UNLIKELY(j >= n))
        tmpBuf[k].nodeData = startPtr[i++].nodeData;
      else if (EP128EMU_UNLIKELY(i >= m) || startPtr[j] < startPtr[i])
        tmpBuf[k].nodeData = startPtr[j++].nodeData;
      else
        tmpBuf[k].nodeData = startPtr[i++].nodeData;
    }
    for (i = 0; i < n; i++)
      startPtr[i].nodeData = tmpBuf[i].nodeData;
  }

  // --------------------------------------------------------------------------

  Compressor_ZLib::HuffmanEncoder::HuffmanEncoder(size_t maxSymbolCnt_,
                                                  size_t minSymbolCnt_)
    : minSymbolCnt(minSymbolCnt_),
      symbolRangeUsed(minSymbolCnt_),
      nodeBuf((void *) 0)
  {
    symbolCounts.resize(maxSymbolCnt_, 0U);
    encodeTable.resize(maxSymbolCnt_, 0U);
    nodeBuf = new HuffmanNode[maxSymbolCnt_ * 20];
  }

  Compressor_ZLib::HuffmanEncoder::~HuffmanEncoder()
  {
    delete[] reinterpret_cast< HuffmanNode * >(nodeBuf);
  }

  void Compressor_ZLib::HuffmanEncoder::updateTables(
      bool reverseBits, size_t maxCodeLen, const unsigned char *codeLengthTable)
  {
    symbolRangeUsed = 0;
    if (codeLengthTable) {
      // use a preset encoding table
      for (size_t i = 0; i < encodeTable.size(); i++) {
        symbolCounts[i] = 0;
        if (codeLengthTable[i]) {
          encodeTable[i] = (unsigned int) codeLengthTable[i] << 24;
          symbolRangeUsed = i + 1;
        }
      }
    }
    else {
      for (size_t i = 0; i < encodeTable.size(); i++)
        encodeTable[i] = 0U;
      // calculate the size of the Huffman tree
      size_t  n = 0;
      for (size_t i = 0; i < symbolCounts.size(); i++) {
        if (symbolCounts[i] > 0) {
          symbolRangeUsed = i + 1;
          n++;
        }
      }
      // check for trivial cases (symbols used <= 2)
      if (n == 1 || n == 2) {
        n = 0;
        for (size_t i = 0; i < symbolRangeUsed; i++) {
          if (symbolCounts[i] > 0) {
            symbolCounts[i] = 0;
            encodeTable[i] = 0x01000000U | (unsigned int) n;
            n++;
          }
        }
      }
      else {
        // build Huffman tree
        HuffmanNode *buf0 = reinterpret_cast< HuffmanNode * >(nodeBuf);
        HuffmanNode *buf1 = buf0 + n;
        HuffmanNode *buf2 = buf1 + n;
        HuffmanNode *allocPtr = buf2 + (n << 1);
        n = 0;
        for (size_t i = 0; i < symbolRangeUsed; i++) {
          if (symbolCounts[i] > 0) {
            buf0[n].initNode((unsigned int) i, symbolCounts[i]);
            symbolCounts[i] = 0;
            n++;
          }
        }
        HuffmanNode::sortNodes(buf0, buf0 + n, buf1);
        for (size_t i = 0; i < n; i++)
          buf0[i].setBufPos(buf0);
        size_t  buf1Size = 0;
        for (size_t i = 0; i < maxCodeLen; i++) {
          size_t  j, k, l;
          // merge buffers
          for (j = 0, k = 0, l = 0; j < n || k < buf1Size; l++) {
            if (EP128EMU_UNLIKELY(k >= buf1Size)) {
              *allocPtr = buf0[j++];
              allocPtr->setBufPos(buf0);
              buf2[l] = *allocPtr;
              allocPtr++;
            }
            else if (EP128EMU_UNLIKELY(j >= n) || buf1[k] < buf0[j]) {
              buf2[l] = buf1[k++];
            }
            else {
              *allocPtr = buf0[j++];
              allocPtr->setBufPos(buf0);
              buf2[l] = *allocPtr;
              allocPtr++;
            }
          }
          buf1Size = l >> 1;
          // package pairs of nodes
          for (j = 0; j < buf1Size; j++) {
            k = j << 1;
            l = k + 1;
            buf2[k].merge(buf2[l], buf0);
            buf0[buf2[k].getBufPos()] = buf2[k];
            buf0[buf2[l].getBufPos()] = buf2[l];
            buf1[j] = buf2[k];
          }
        }
        for (size_t i = 0; i < buf1Size; i++) {
          HuffmanNode *p = buf1 + i;
          do {
            encodeTable[p->value()] = encodeTable[p->value()] + 0x01000000U;
            p = p->nextNode(buf0);
          } while (p);
        }
      }
    }
    // convert Huffman tree to canonical codes
    unsigned int  sizeCounts[20];
    unsigned int  sizeCodes[20];
    for (size_t i = 0; i <= maxCodeLen; i++)
      sizeCounts[i] = 0U;
    for (size_t i = 0; i < symbolRangeUsed; i++) {
      size_t  symLen = encodeTable[i] >> 24;
      if (symLen > maxCodeLen)
        throw Exception("internal error in HuffmanEncoder::updateTables()");
      sizeCounts[symLen] = sizeCounts[symLen] + 1U;
    }
    sizeCounts[0] = 0U;
    sizeCodes[0] = 0U;
    for (size_t i = 1; i <= maxCodeLen; i++)
      sizeCodes[i] = (sizeCodes[i - 1] + sizeCounts[i - 1]) << 1;
    for (size_t i = 0; i < symbolRangeUsed; i++) {
      if (encodeTable[i] != 0U) {
        unsigned int  nBits = encodeTable[i] >> 24;
        unsigned int  huffCode = sizeCodes[nBits];
        sizeCodes[nBits]++;
        if (reverseBits) {
          // Deflate format stores Huffman codes in most significant bit first
          // order, but everything else is least significant bit first
          huffCode = ((huffCode & 0x00FFU) << 8) | ((huffCode & 0xFF00U) >> 8);
          huffCode = ((huffCode & 0x0F0FU) << 4) | ((huffCode & 0xF0F0U) >> 4);
          huffCode = ((huffCode & 0x3333U) << 2) | ((huffCode & 0xCCCCU) >> 2);
          huffCode = ((huffCode & 0x5555U) << 1) | ((huffCode & 0xAAAAU) >> 1);
          huffCode = huffCode >> (16U - nBits);
        }
        encodeTable[i] = encodeTable[i] | huffCode;
      }
    }
    if (encodeTable.size() == 19 && minSymbolCnt == 4 && maxCodeLen == 7) {
      // Deflate specific hack for the code length encoder
      symbolRangeUsed = 4;
      for (size_t i = 0; i < encodeTable.size(); i++) {
        if (encodeTable[i] != 0U) {
          if (size_t(deflateCodeLengthIndexTable[i]) >= symbolRangeUsed)
            symbolRangeUsed = size_t(deflateCodeLengthIndexTable[i]) + 1;
        }
      }
    }
    if (symbolRangeUsed < minSymbolCnt)
      symbolRangeUsed = minSymbolCnt;
  }

  void Compressor_ZLib::HuffmanEncoder::updateDeflateSymLenCnts(
      HuffmanEncoder& symLenEncoder) const
  {
    unsigned int  prvCode = 0xFFFFFFFFU;
    for (size_t i = 0; i < symbolRangeUsed; i++) {
      unsigned int  codeLength = encodeTable[i] >> 24;
      unsigned int  rleCode = (codeLength == 0U ? codeLength : prvCode) << 24;
      unsigned int  rleLength = 0U;
      unsigned int  maxLength = (!rleCode ? 138U : 6U);
      while ((i + rleLength) < symbolRangeUsed &&
             ((encodeTable[i + rleLength] ^ rleCode) & 0xFF000000U) == 0U) {
        if (++rleLength >= maxLength)
          break;
      }
      prvCode = codeLength;
      if (rleLength >= 3U) {
        symLenEncoder.addSymbol(!rleCode ? (rleLength < 11U ? 17U : 18U) : 16U);
        i = i + (rleLength - 1U);
      }
      else {
        symLenEncoder.addSymbol(codeLength);
      }
    }
  }

  void Compressor_ZLib::HuffmanEncoder::writeDeflateEncoding(
      std::vector< unsigned int >& outBuf) const
  {
    // write code length encoding table
    for (size_t i = 0; i < symbolRangeUsed; i++) {
      outBuf.push_back(0x03000000U
                       | (encodeTable[deflateCodeLengthCodeTable[i]] >> 24));
    }
  }

  void Compressor_ZLib::HuffmanEncoder::writeDeflateEncoding(
      std::vector< unsigned int >& outBuf,
      const HuffmanEncoder& symLenEncoder) const
  {
    unsigned int  prvCode = 0xFFFFFFFFU;
    for (size_t i = 0; i < symbolRangeUsed; i++) {
      unsigned int  codeLength = encodeTable[i] >> 24;
      unsigned int  rleCode = (codeLength == 0U ? codeLength : prvCode) << 24;
      unsigned int  rleLength = 0U;
      unsigned int  maxLength = (!rleCode ? 138U : 6U);
      while ((i + rleLength) < symbolRangeUsed &&
             ((encodeTable[i + rleLength] ^ rleCode) & 0xFF000000U) == 0U) {
        if (++rleLength >= maxLength)
          break;
      }
      prvCode = codeLength;
      if (rleLength >= 3U) {
        if (rleCode) {
          rleCode = symLenEncoder.encodeSymbol(16U);
          rleCode = (rleCode + 0x02000000U)
                    | ((rleLength - 3U) << (rleCode >> 24));
        }
        else if (rleLength < 11U) {
          rleCode = symLenEncoder.encodeSymbol(17U);
          rleCode = (rleCode + 0x03000000U)
                    | ((rleLength - 3U) << (rleCode >> 24));
        }
        else {
          rleCode = symLenEncoder.encodeSymbol(18U);
          rleCode = (rleCode + 0x07000000U)
                    | ((rleLength - 11U) << (rleCode >> 24));
        }
        outBuf.push_back(rleCode);
        i = i + (rleLength - 1U);
      }
      else {
        outBuf.push_back(symLenEncoder.encodeSymbol(codeLength));
      }
    }
  }

  void Compressor_ZLib::HuffmanEncoder::clear()
  {
    symbolRangeUsed = minSymbolCnt;
    for (size_t i = 0; i < symbolCounts.size(); i++) {
      symbolCounts[i] = 0U;
      encodeTable[i] = 0U;
    }
  }

  // --------------------------------------------------------------------------

  void Compressor_ZLib::SearchTable::sortFunc(
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
        if (len1 > Compressor_ZLib::maxMatchLen)
          len1 = Compressor_ZLib::maxMatchLen;
        size_t  len2 = bufSize - pos2;
        if (len2 > Compressor_ZLib::maxMatchLen)
          len2 = Compressor_ZLib::maxMatchLen;
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

  void Compressor_ZLib::SearchTable::addMatch(size_t bufPos,
                                              size_t matchLen, size_t offs)
  {
    if (matchLen > 0) {
      if (matchLen > Compressor_ZLib::maxMatchLen)
        matchLen = Compressor_ZLib::maxMatchLen;
    }
    if ((matchTableBuf.size() + 2) > matchTableBuf.capacity()) {
      size_t  tmp = 1024;
      while (tmp < (matchTableBuf.size() + 2))
        tmp = tmp << 1;
      matchTableBuf.reserve(tmp);
    }
    if (matchTable[bufPos] == 0xFFFFFFFFU)
      matchTable[bufPos] = (unsigned int) matchTableBuf.size();
    matchTableBuf.push_back((unsigned short) matchLen);
    matchTableBuf.push_back((unsigned short) offs);
  }

  Compressor_ZLib::SearchTable::SearchTable(const unsigned char *buf,
                                            size_t bufSize)
  {
    if (bufSize < 1) {
      throw Exception("Compressor_ZLib::SearchTable::SearchTable(): "
                      "zero input buffer size");
    }
    matchTable.resize(bufSize, 0xFFFFFFFFU);
    matchTableBuf.reserve(1024);
    std::vector< unsigned short > rleLengthTable(bufSize, 0);
    // find RLE (offset = 1) matches
    {
      size_t        rleLength = 1;
      unsigned int  rleByte = (unsigned int) buf[bufSize - 1];
      for (size_t i = bufSize - 1; i > 0; ) {
        i--;
        if ((unsigned int) buf[i] != rleByte) {
          rleByte = (unsigned int) buf[i];
          rleLength = 0;
        }
        if (rleLength >= Compressor_ZLib::minMatchLen)
          rleLengthTable[i + 1] = (unsigned short) rleLength;
        if (rleLength < size_t(Compressor_ZLib::maxMatchLen))
          rleLength++;
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
    for (size_t startPos = 0; startPos < bufSize; ) {
      size_t  startPos_ = 0;
      if (startPos >= Compressor_ZLib::maxMatchDist)
        startPos_ = startPos - Compressor_ZLib::maxMatchDist;
      size_t  endPos = startPos + Compressor_ZLib::maxMatchDist;
      if (endPos > bufSize || bufSize <= (Compressor_ZLib::maxMatchDist * 3))
        endPos = bufSize;
      size_t  nBytes = endPos - startPos_;
      tmpBuf.resize(nBytes);
      suffixArray.resize(nBytes);
      prvMatchLenTable.resize(nBytes);
      nxtMatchLenTable.resize(nBytes);
      for (size_t i = 0; i < nBytes; i++)
        suffixArray[i] = (unsigned int) (startPos_ + i);
      sortFunc(&(suffixArray.front()), buf, bufSize, 0, nBytes,
               &(tmpBuf.front()));
      for (size_t i = 0; i < nBytes; i++) {
        if (i == 0) {
          prvMatchLenTable[i] = 0;
        }
        else {
          size_t  len = 0;
          size_t  p1 = suffixArray[i - 1];
          size_t  p2 = suffixArray[i];
          size_t  maxLen = bufSize - (p1 > p2 ? p1 : p2);
          maxLen = (maxLen < Compressor_ZLib::maxMatchLen ?
                    maxLen : Compressor_ZLib::maxMatchLen);
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
          Compressor_ZLib::maxMatchLen + 1,
          (unsigned long) Compressor_ZLib::maxMatchDist);
      for (size_t i_ = 0; i_ < nBytes; i_++) {
        size_t  i = suffixArray[i_];
        if (i < startPos)
          continue;
        size_t  rleLen = rleLengthTable[i];
        if (rleLen > Compressor_ZLib::maxMatchLen)
          rleLen = Compressor_ZLib::maxMatchLen;
        size_t  minLen = Compressor_ZLib::minMatchLen;
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
        unsigned long prvDist = Compressor_ZLib::maxMatchDist;
        for (size_t k = maxLen; k >= Compressor_ZLib::minMatchLen; k--) {
          unsigned long d = offsTable[k];
          offsTable[k] = Compressor_ZLib::maxMatchDist;
          if (d < prvDist) {
            prvDist = d;
            addMatch(i, k, size_t(d + 1UL));
            if (d == 0UL)
              break;
          }
        }
        addMatch(i, 0, 0);
      }
      startPos = endPos;
    }
  }

  Compressor_ZLib::SearchTable::~SearchTable()
  {
  }

  // --------------------------------------------------------------------------

  void Compressor_ZLib::setDefaultEncoding()
  {
    huffmanEncoderL.clear();
    huffmanEncoderD.clear();
    huffmanEncoderC.clear();
    unsigned char   codeLengthTable[288];
    // literal / length encoder:
    // 144 * 8 bits, 112 * 9 bits, 24 * 7 bits, 8 * 8 bits
    for (unsigned int i = 0U; i < 288U; i++)
      codeLengthTable[i] = (i < 144U ? 8 : (i < 256U ? 9 : (i < 280U ? 7 : 8)));
    huffmanEncoderL.updateTables(true, 15, codeLengthTable);
    // distance encoder: same weight for all symbols
    for (unsigned int i = 0U; i < 32U; i++)
      codeLengthTable[i] = 5;
    huffmanEncoderD.updateTables(true, 15, codeLengthTable);
  }

  static EP128EMU_REGPARM1 unsigned int log2N(unsigned int n)
  {
    unsigned int  r = 0U;
    if (n >= 256U) {
      n = n >> 8;
      r = 8U;
    }
    if (n >= 16U) {
      n = n >> 4;
      r = r | 4U;
    }
    if (n >= 4U) {
      n = n >> 2;
      r = r | 2U;
    }
    return (r | (n >> 1));
  }

  // Length / literal codes:
  //     0 to 255:    literal byte
  //     256:         end of block
  //     257 to 260:  length = 3 to 6 bytes
  //     261 to 264:  length = 7 to 10 bytes (0 extra bits)
  //     265 to 268:  length = 11 to 18 bytes (1 extra bit)
  //     269 to 272:  length = 19 to 34 bytes (2 extra bits)
  //     273 to 276:  length = 35 to 66 bytes (3 extra bits)
  //     277 to 280:  length = 67 to 130 bytes (4 extra bits)
  //     281 to 284:  length = 131 to 258 bytes (5 extra bits)
  //     285:         length = 258 bytes
  //     286 to 287:  invalid codes
  // Fixed Huffman encoding of length / literal codes:
  //     0 to 143:    8 bits ( 00110000b to  10111111b)
  //     144 to 255:  9 bits (110010000b to 111111111b)
  //     256 to 279:  7 bits (  0000000b to   0010111b)
  //     280 to 287:  8 bits ( 11000000b to  11000111b)

  static unsigned int getLengthCode(size_t n)
  {
    if (n <= 11)
      return ((unsigned int) n + 254U);
    if (n >= 258)
      return 285U;
    n = n - 3;
    unsigned int  r = log2N((unsigned int) n);
    return ((r << 2) + (unsigned int) (n >> (r - 2U)) + 249U);
  }

  // Distance codes:
  //     0 to 1:      offset = 1 to 2 bytes
  //     2 to 3:      offset = 3 to 4 bytes (0 extra bits)
  //     4 to 5:      offset = 5 to 8 bytes (1 extra bit)
  //     6 to 7:      offset = 9 to 16 bytes (2 extra bits)
  //     8 to 9:      offset = 17 to 32 bytes (3 extra bits)
  //     10 to 11:    offset = 33 to 64 bytes (4 extra bits)
  //     12 to 13:    offset = 65 to 128 bytes (5 extra bits)
  //     14 to 15:    offset = 129 to 256 bytes (6 extra bits)
  //     16 to 17:    offset = 257 to 512 bytes (7 extra bits)
  //     18 to 19:    offset = 513 to 1024 bytes (8 extra bits)
  //     20 to 21:    offset = 1025 to 2048 bytes (9 extra bits)
  //     22 to 23:    offset = 2049 to 4096 bytes (10 extra bits)
  //     24 to 25:    offset = 4097 to 8192 bytes (11 extra bits)
  //     26 to 27:    offset = 8193 to 16384 bytes (12 extra bits)
  //     28 to 29:    offset = 16385 to 32768 bytes (13 extra bits)
  //     30 to 31:    invalid codes

  static unsigned int getDistanceCode(size_t d)
  {
    if (d <= 5)
      return ((unsigned int) d - 1U);
    d = d - 1;
    unsigned int  r = log2N((unsigned int) d);
    return ((r << 1) + (unsigned int) (d >> (r - 1U)) - 2U);
  }

  void Compressor_ZLib::writeMatchCode(std::vector< unsigned int >& buf,
                                       size_t d, size_t n)
  {
    unsigned int  lenCode = getLengthCode(n);
    unsigned int  lenBits = ((lenCode >= 265U && lenCode < 285U) ?
                             ((lenCode - 261U) >> 2) : 0U);
    huffmanEncoderL.addSymbol(lenCode);
    lenCode = huffmanEncoderL.encodeSymbol(lenCode);
    lenCode = (lenCode + (lenBits << 24))
              | (((unsigned int) (n - minMatchLen) & ((1U << lenBits) - 1U))
                 << ((lenCode >> 24) & 0x7FU));
    buf.push_back(lenCode);
    unsigned int  distCode = getDistanceCode(d);
    unsigned int  distBits = (distCode >= 4U ? ((distCode - 2U) >> 1) : 0U);
    huffmanEncoderD.addSymbol(distCode);
    distCode = huffmanEncoderD.encodeSymbol(distCode);
    if (EP128EMU_UNLIKELY(((distCode >> 24) + distBits) > 24U)) {
      buf.push_back(distCode);
      distCode = 0U;
    }
    distCode = (distCode + (distBits << 24))
               | (((unsigned int) (d - minMatchDist) & ((1U << distBits) - 1U))
                  << ((distCode >> 24) & 0x7FU));
    buf.push_back(distCode);
  }

  inline size_t Compressor_ZLib::getLengthCodeLength(size_t n) const
  {
    unsigned int  lenCode = getLengthCode(n);
    unsigned int  lenBits = ((lenCode >= 265U && lenCode < 285U) ?
                             ((lenCode - 261U) >> 2) : 0U);
    return (huffmanEncoderL.getSymbolSize(lenCode) + size_t(lenBits));
  }

  inline size_t Compressor_ZLib::getDistanceCodeLength(size_t d) const
  {
    unsigned int  distCode = getDistanceCode(d);
    unsigned int  distBits = (distCode >= 4U ? ((distCode - 2U) >> 1) : 0U);
    return (huffmanEncoderD.getSymbolSize(distCode) + size_t(distBits));
  }

  inline size_t Compressor_ZLib::getMatchCodeLength(size_t n, size_t d) const
  {
    return (getLengthCodeLength(n) + getDistanceCodeLength(d));
  }

  void Compressor_ZLib::optimizeMatches(LZMatchParameters *matchTable,
                                        const unsigned char *inBuf,
                                        size_t *bitCountTable,
                                        size_t *offsSumTable,
                                        size_t offs, size_t nBytes)
  {
    unsigned short  lengthCodeLengthTable[256];
    for (size_t i = minMatchLen; i <= maxMatchLen; i++) {
      lengthCodeLengthTable[i - minMatchLen] =
          (unsigned short) getLengthCodeLength(i);
    }
    for (size_t i = nBytes; i-- > 0; ) {
      size_t  bestSize = 0x7FFFFFFF;
      size_t  bestLen = 1;
      size_t  bestOffs = 0;
      const unsigned short  *matchPtr = searchTable->getMatches(offs + i);
      size_t  len = matchPtr[0];        // match length
      if (len > (nBytes - i))
        len = nBytes - i;
      if (len >= maxMatchLen) {
        // long RLE or LZ77 match
        bestSize = getMatchCodeLength(len, matchPtr[1]);
        bestOffs = matchPtr[1];
        bestLen = len;
        if (bestSize < (maxMatchLen * 15)) {
          bestSize = bestSize + bitCountTable[i + len];
          if (bestOffs <= 1) {
            // if an RLE match with the maximum length is possible, use that
            matchTable[i].d = (unsigned short) bestOffs;
            matchTable[i].len = (unsigned short) bestLen;
            bitCountTable[i] = bestSize;
            offsSumTable[i] = offsSumTable[i + bestLen] + bestOffs;
            continue;
          }
        }
        else {
          bestSize = bestSize + bitCountTable[i + len];
        }
        // reduce the length range searched for better performance
        len = 128;
      }
      // check all possible LZ77 match lengths,
      for ( ; len > 0; matchPtr = matchPtr + 2, len = matchPtr[0]) {
        if (len > (nBytes - i))
          len = nBytes - i;
        unsigned int  d = matchPtr[1];
        if (len >= minMatchLen) {
          size_t  nxtLen = matchPtr[2];
          size_t  nBitsBase = getDistanceCodeLength(d);
          nxtLen = (nxtLen > minMatchLen ? nxtLen : minMatchLen);
          do {
            size_t  nBits = nBitsBase
                            + size_t(lengthCodeLengthTable[len - minMatchLen])
                            + bitCountTable[i + len];
            if (nBits > bestSize)
              continue;
            if (nBits == bestSize) {
              if ((offsSumTable[i + len] + d)
                  > (offsSumTable[i + bestLen] + bestOffs)) {
                continue;
              }
            }
            bestSize = nBits;
            bestOffs = d;
            bestLen = len;
          } while (--len >= nxtLen);
        }
      }
      // and literal byte:
      size_t  nBits = bitCountTable[i + 1]
                      + huffmanEncoderL.getSymbolSize(inBuf[offs + i]);
      do {
        if (nBits > bestSize)
          break;
        if (nBits == bestSize) {
          if (offsSumTable[i + 1] > (offsSumTable[i + bestLen] + bestOffs))
            break;
        }
        bestSize = nBits;
        bestOffs = 0;
        bestLen = 1;
      } while (false);
      matchTable[i].d = (unsigned short) bestOffs;
      matchTable[i].len = (unsigned short) bestLen;
      bitCountTable[i] = bestSize;
      offsSumTable[i] = offsSumTable[i + bestLen] + bestOffs;
    }
  }

  void Compressor_ZLib::compressData(std::vector< unsigned int >& tmpOutBuf,
                                     const unsigned char *inBuf,
                                     size_t offs, size_t nBytes,
                                     bool firstPass)
  {
    size_t  endPos = offs + nBytes;
    tmpOutBuf.clear();
    if (firstPass) {
      // try fixed Huffman encoding first
      setDefaultEncoding();
    }
    else {
      // generate optimal encode tables for length and offset values
      huffmanEncoderL.updateTables(true, 15);
      huffmanEncoderD.updateTables(true, 15);
    }
    // compress data by searching for repeated byte sequences,
    // and replacing them with length/distance codes
    std::vector< LZMatchParameters >  matchTable(nBytes);
    {
      std::vector< size_t > bitCountTable(nBytes + 1, 0);
      std::vector< size_t > offsSumTable(nBytes + 1, 0);
      optimizeMatches(&(matchTable.front()), inBuf, &(bitCountTable.front()),
                      &(offsSumTable.front()), offs, nBytes);
    }
    // write block header and encode tables
    if (firstPass) {
      // bit 0 of the block header is the last block flag, b1 and b2
      // set the compression type (00b = none, 01b = fixed Huffman codes,
      // 10b = dynamic Huffman codes, 11b = invalid)
      tmpOutBuf.push_back(0x03000002U);
    }
    else {
      tmpOutBuf.push_back(0x03000004U);
      huffmanEncoderL.updateDeflateSymLenCnts(huffmanEncoderC);
      huffmanEncoderD.updateDeflateSymLenCnts(huffmanEncoderC);
      huffmanEncoderC.updateTables(true, 7);
      tmpOutBuf.push_back(
          0x0E000000U
          | (unsigned int) (huffmanEncoderL.getSymbolRangeUsed() - 257)
          | (unsigned int) ((huffmanEncoderD.getSymbolRangeUsed() - 1) << 5)
          | (unsigned int) ((huffmanEncoderC.getSymbolRangeUsed() - 4) << 10));
      huffmanEncoderC.writeDeflateEncoding(tmpOutBuf);
      huffmanEncoderL.writeDeflateEncoding(tmpOutBuf, huffmanEncoderC);
      huffmanEncoderD.writeDeflateEncoding(tmpOutBuf, huffmanEncoderC);
    }
    // write compressed data
    for (size_t i = offs; i < endPos; ) {
      LZMatchParameters&  tmp = matchTable[i - offs];
      if (tmp.d > 0) {
        // write LZ77 match
        writeMatchCode(tmpOutBuf, tmp.d, tmp.len);
        i = i + tmp.len;
      }
      else {
        // write literal byte
        huffmanEncoderL.addSymbol(inBuf[i]);
        tmpOutBuf.push_back(huffmanEncoderL.encodeSymbol(inBuf[i]));
        i++;
      }
    }
    // write end of block symbol (256)
    huffmanEncoderL.addSymbol(0x0100);
    tmpOutBuf.push_back(huffmanEncoderL.encodeSymbol(0x0100));
  }

  // --------------------------------------------------------------------------

  Compressor_ZLib::Compressor_ZLib()
    : searchTable((SearchTable *) 0),
      huffmanEncoderL(288, 257),
      huffmanEncoderD(32, 1),
      huffmanEncoderC(19, 4)
  {
  }

  Compressor_ZLib::~Compressor_ZLib()
  {
    if (searchTable)
      delete searchTable;
  }

  void Compressor_ZLib::compressDataBlock(std::vector< unsigned int >& outBuf,
                                          const unsigned char *inBuf,
                                          size_t offs, size_t nBytes,
                                          size_t bufSize, bool isLastBlock)
  {
    outBuf.clear();
    if ((offs + nBytes) > bufSize)
      nBytes = bufSize - offs;
    if (!inBuf || nBytes < 1)
      return;
    // FIXME: this assumes that the data is compressed in fixed size blocks
    // that maxMatchDist is an integer multiple of
    {
      size_t  searchTableStart = (offs / maxMatchDist) * maxMatchDist;
      size_t  searchTableEnd = searchTableStart + maxMatchDist;
      bool    searchTableNeeded = (offs == searchTableStart);
      if (searchTableStart >= maxMatchDist)
        searchTableStart = searchTableStart - maxMatchDist;
      if (searchTableEnd > bufSize)
        searchTableEnd = bufSize;
      inBuf = inBuf + searchTableStart;
      offs = offs - searchTableStart;
      if (searchTableNeeded) {
        searchTableEnd = searchTableEnd - searchTableStart;
        if (searchTable) {
          delete searchTable;
          searchTable = (SearchTable *) 0;
        }
        searchTable = new SearchTable(inBuf, searchTableEnd);
      }
    }
    size_t  endPos = offs + nBytes;
    std::vector< uint64_t >     hashTable;
    std::vector< unsigned int > tmpBuf;
    size_t  bestSize = 0x7FFFFFFF;
    bool    doneFlag = false;
    for (size_t i = 0; i < 8; i++) {
      if (doneFlag)     // if the compression cannot be optimized further,
        continue;       // quit the loop earlier
      tmpBuf.clear();
      compressData(tmpBuf, inBuf, offs, nBytes, (i == 0));
      // calculate compressed size and hash value
      size_t    compressedSize = 0;
      uint64_t  h = 1UL;
      for (size_t j = 0; j < tmpBuf.size(); j++) {
        compressedSize += size_t((tmpBuf[j] & 0x7F000000U) >> 24);
        h = h ^ uint64_t(tmpBuf[j]);
        h = uint32_t(h) * uint64_t(0xC2B0C3CCUL);
        h = (h ^ (h >> 32)) & 0xFFFFFFFFUL;
      }
      h = h | (uint64_t(compressedSize) << 32);
      if (compressedSize < bestSize) {
        // found a better compression, so save it
        bestSize = compressedSize;
        outBuf.resize(tmpBuf.size());
        std::memcpy(&(outBuf.front()), &(tmpBuf.front()),
                    tmpBuf.size() * sizeof(unsigned int));
      }
      for (size_t j = 0; j < hashTable.size(); j++) {
        if (hashTable[j] == h) {
          // if the exact same compressed data was already generated earlier,
          // the remaining optimize iterations can be skipped
          doneFlag = true;
          break;
        }
      }
      if (!doneFlag)
        hashTable.push_back(h);         // save hash value
    }
    size_t  uncompressedSize = (nBytes + 4) * 8 + 3;
    if (bestSize >= uncompressedSize) {
      // if cannot reduce the data size, store without compression
      outBuf.resize((endPos - offs) + 5);
      outBuf[0] = 0x03000000U | (unsigned int) isLastBlock;
      outBuf[1] = 0x88000000U | (unsigned int) (nBytes & 0xFF);
      outBuf[2] = 0x88000000U | (unsigned int) ((nBytes >> 8) & 0xFF);
      outBuf[3] = outBuf[1] ^ 0xFFU;
      outBuf[4] = outBuf[2] ^ 0xFFU;
      for (size_t i = 0; i < nBytes; i++)
        outBuf[i + 5] = 0x88000000U | (unsigned int) inBuf[offs + i];
    }
    else {
      outBuf[0] = outBuf[0] | (unsigned int) isLastBlock;
    }
  }

  // ==========================================================================

  class ZLibCompressorThread : public Thread {
   private:
    Compressor_ZLib compressor;
   public:
    std::vector< unsigned int > outBuf;
    const unsigned char *inBuf;
    size_t  inBufSize;
    size_t  startPos;
    size_t  blockSize;
    bool    errorFlag;
    // --------
    ZLibCompressorThread();
    virtual ~ZLibCompressorThread();
    virtual void run();
  };

  ZLibCompressorThread::ZLibCompressorThread()
    : inBuf((unsigned char *) 0),
      inBufSize(0),
      startPos(0),
      blockSize(Compressor_ZLib::maxMatchDist),
      errorFlag(false)
  {
  }

  ZLibCompressorThread::~ZLibCompressorThread()
  {
  }

  void ZLibCompressorThread::run()
  {
    try {
      if (!inBuf)
        return;
      std::vector< unsigned int > tmpBuf;
      size_t  dataSizePos = 0;
      while (startPos < inBufSize) {
        size_t  nBytes = blockSize;
        if ((startPos + nBytes) > inBufSize)
          nBytes = inBufSize - startPos;
        compressor.compressDataBlock(tmpBuf, inBuf, startPos, nBytes, inBufSize,
                                     ((startPos + nBytes) >= inBufSize));
        // append compressed data to output buffer
        size_t  prvSize = outBuf.size();
        if ((startPos % Compressor_ZLib::maxMatchDist) == 0) {
          dataSizePos = prvSize;
          outBuf.resize(prvSize + tmpBuf.size() + 1);
          outBuf[dataSizePos] = (unsigned int) tmpBuf.size();
          prvSize++;
        }
        else {
          outBuf.resize(prvSize + tmpBuf.size());
          outBuf[dataSizePos] =
              outBuf[dataSizePos] + (unsigned int) tmpBuf.size();
        }
        std::memcpy(&(outBuf.front()) + prvSize, &(tmpBuf.front()),
                    tmpBuf.size() * sizeof(unsigned int));
        startPos = startPos + blockSize;
        if ((startPos % Compressor_ZLib::maxMatchDist) == 0) {
          startPos = startPos + (Compressor_ZLib::maxMatchDist
                                 * (DEFLATE_MAX_THREADS - 1));
        }
      }
    }
    catch (std::exception) {
      errorFlag = true;
    }
  }

  // --------------------------------------------------------------------------

  void Compressor_ZLib::compressData(std::vector< unsigned char >& outBuf,
                                     const unsigned char *inBuf,
                                     size_t inBufSize, size_t blockSize)
  {
    outBuf.clear();
    if (inBufSize < 1 || !inBuf)
      return;
    ZLibCompressorThread  *compressorThreads[DEFLATE_MAX_THREADS];
    for (int i = 0; i < DEFLATE_MAX_THREADS; i++)
      compressorThreads[i] = (ZLibCompressorThread *) 0;
    try {
      size_t  startPos = 0;
      int     nThreads = 0;
      while (nThreads < DEFLATE_MAX_THREADS && startPos < inBufSize) {
        compressorThreads[nThreads] = new ZLibCompressorThread();
        compressorThreads[nThreads]->inBuf = inBuf;
        compressorThreads[nThreads]->inBufSize = inBufSize;
        compressorThreads[nThreads]->startPos = startPos;
        compressorThreads[nThreads]->blockSize = blockSize;
        startPos = startPos + Compressor_ZLib::maxMatchDist;
        nThreads++;
      }
      for (int i = 0; i < nThreads; i++)
        compressorThreads[i]->start();
      // calculate Adler-32 checksum of input data
      unsigned int  adler32Sum;
      {
        unsigned int  tmp1 = 1U;
        unsigned int  tmp2 = 0U;
        for (size_t i = 0; i < inBufSize; i++) {
          tmp1 = tmp1 + (unsigned int) inBuf[i];
          if (tmp1 >= 65521U)
            tmp1 -= 65521U;
          tmp2 = tmp2 + tmp1;
          if (tmp2 >= 65521U)
            tmp2 -= 65521U;
        }
        adler32Sum = tmp1 | (tmp2 << 16);
      }
      for (int i = 0; i < nThreads; i++) {
        compressorThreads[i]->join();
        // startPos is now the read position of the output buffer of the thread
        compressorThreads[i]->startPos = 0;
      }
      unsigned char shiftReg = 0x80;
      for (int i = 0; true; i++) {
        if (i >= nThreads)
          i = 0;
        if (!compressorThreads[i]) {
          // end of compressed data for all threads
          if (shiftReg != 0x80) {
            while (!(shiftReg & 0x01))
              shiftReg = shiftReg >> 1;
            shiftReg = (shiftReg >> 1) & 0x7F;
            outBuf.push_back(shiftReg);
            shiftReg = 0x80;
          }
          // store Adler-32 checksum
          outBuf.push_back((unsigned char) ((adler32Sum >> 24) & 0xFFU));
          outBuf.push_back((unsigned char) ((adler32Sum >> 16) & 0xFFU));
          outBuf.push_back((unsigned char) ((adler32Sum >> 8) & 0xFFU));
          outBuf.push_back((unsigned char) (adler32Sum & 0xFFU));
          break;
        }
        if (compressorThreads[i]->errorFlag)
          throw Exception("error compressing data");
        if (compressorThreads[i]->startPos
            >= compressorThreads[i]->outBuf.size()) {
          // end of compressed data for this thread
          delete compressorThreads[i];
          compressorThreads[i] = (ZLibCompressorThread *) 0;
          continue;
        }
        // pack output data
        if (outBuf.size() < 1) {
          // write ZLib header:
          //   CINFO = 7 (32K dictionary size)
          //   CM = 8 (Deflate method)
          //   FLEVEL = 3 (high compression level)
          //   FDICT = 0 (no preset dictionary)
          //   FCHECK = 26 ((0x78DA % 31) == 0)
          outBuf.push_back(0x78);
          outBuf.push_back(0xDA);
        }
        startPos = compressorThreads[i]->startPos + 1;
        compressorThreads[i]->startPos =
            startPos + size_t(compressorThreads[i]->outBuf[startPos - 1]);
        for (size_t j = startPos; j < compressorThreads[i]->startPos; j++) {
          unsigned int  c = compressorThreads[i]->outBuf[j];
          if (c >= 0x80000000U) {
            // special case for literal bytes, which are stored byte-aligned
            if (shiftReg != 0x80) {
              while (!(shiftReg & 0x01))
                shiftReg = shiftReg >> 1;
              shiftReg = (shiftReg >> 1) & 0x7F;
              outBuf.push_back(shiftReg);
              shiftReg = 0x80;
            }
            outBuf.push_back((unsigned char) ((c >> 24) & 0xFFU));
          }
          else {
            unsigned int  nBits = c >> 24;
            while (nBits-- > 0U) {
              unsigned char b = (unsigned char) bool(c & 1U);
              bool          srFull = bool(shiftReg & 0x01);
              c = c >> 1;
              shiftReg = ((shiftReg >> 1) & 0x7F) | (b << 7);
              if (srFull) {
                outBuf.push_back(shiftReg);
                shiftReg = 0x80;
              }
            }
          }
        }
      }
    }
    catch (...) {
      for (int i = 0; i < DEFLATE_MAX_THREADS; i++) {
        if (compressorThreads[i])
          delete compressorThreads[i];
      }
      outBuf.clear();
      throw;
    }
  }

  // ==========================================================================

  static void writePNGUInt32(unsigned char *buf, uint32_t n)
  {
    buf[0] = (unsigned char) ((n >> 24) & 0xFFU);
    buf[1] = (unsigned char) ((n >> 16) & 0xFFU);
    buf[2] = (unsigned char) ((n >> 8) & 0xFFU);
    buf[3] = (unsigned char) (n & 0xFFU);
  }

  static void writePNGChunk(std::FILE *f, uint32_t chunkID,
                            const unsigned char *buf, size_t bufSize)
  {
    static const uint32_t crc32Table[4] = {
      0x00000000U, 0x76DC4190U, 0xEDB88320U, 0x9B64C2B0U
    };
    unsigned char hdrBuf[12];
    writePNGUInt32(&(hdrBuf[0]), uint32_t(bufSize));
    writePNGUInt32(&(hdrBuf[4]), chunkID);
    uint32_t  crc = ~0U;
    for (long i = -4L; i < long(bufSize); i++) {
      unsigned char c = (EP128EMU_UNLIKELY(i < 0L) ? hdrBuf[8L + i] : buf[i]);
      for (int j = 0; j < 4; j++) {
        crc = (crc >> 2) ^ crc32Table[(crc ^ uint32_t(c)) & 3U];
        c = c >> 2;
      }
    }
    writePNGUInt32(&(hdrBuf[8]), ~crc);
    if (std::fwrite(&(hdrBuf[0]), sizeof(unsigned char), 8, f) != 8 ||
        std::fwrite(buf, sizeof(unsigned char), bufSize, f) != bufSize ||
        std::fwrite(&(hdrBuf[8]), sizeof(unsigned char), 4, f) != 4) {
      throw Ep128Emu::Exception("error writing PNG image file");
    }
  }

  void writePNGImage(const char *fileName,
                     const unsigned char *inBuf, int w, int h, int nColors,
                     bool optimizePalette, size_t blockSize)
  {
    static const char *pngSignature = "\211PNG\r\n\032\n";
#ifdef PNGWRITE_DEBUG
    Timer   tt;
    double  t0 = tt.getRealTime();
#endif
    if (!fileName || !fileName[0])
      throw Exception("invalid PNG image file name");
    std::FILE *f = (std::FILE *) 0;
    try {
      std::vector< unsigned char >  hdrBuf(1024, 0x00);
      unsigned char *hdrBufP = &(hdrBuf.front());
      unsigned char *paletteIndexTable = hdrBufP + 768;
      unsigned char *colorReplaceTable = hdrBufP + 512;
      nColors = (nColors > 0 ? (nColors < 256 ? nColors : 256) : 0);
      const unsigned char *imgDataPtr = inBuf + size_t(nColors * 3);
      size_t        lineBytesI = size_t(w);
      unsigned char bitDepth = 8;
      if (nColors) {
        if (optimizePalette) {
          unsigned char *colorsUsed = hdrBufP + 256;
          size_t  imgDataSize = size_t(h) * lineBytesI;
          for (size_t i = 0; i < imgDataSize; i++)
            colorsUsed[imgDataPtr[i]] = 1;
          int     n = 0;
          for (int i = 0; i < 256; i++) {
            if (colorsUsed[i]) {
              if (i >= nColors || n >= nColors) {
                throw Exception("internal error in writePNGImage(): "
                                "palette index is out of range");
              }
              paletteIndexTable[n] = (unsigned char) i;
              colorReplaceTable[i] = (unsigned char) n;
              n++;
            }
          }
          nColors = n;
        }
        else {
          for (int i = 0; i < nColors; i++) {
            paletteIndexTable[i] = (unsigned char) i;
            colorReplaceTable[i] = (unsigned char) i;
          }
        }
        if (nColors <= 16)
          bitDepth = (nColors > 4 ? 4 : (nColors > 2 ? 2 : 1));
      }
      else {
        lineBytesI = lineBytesI * 3;
        optimizePalette = false;
      }
      std::vector< unsigned char >  outBuf;
      {
        size_t  lineBytesO = (lineBytesI * size_t(bitDepth) + 7) >> 3;
        std::vector< unsigned char >  imgDataBuf(size_t(h) * (lineBytesO + 1));
        const unsigned char *srcp = imgDataPtr;
        unsigned char   *dstp = &(imgDataBuf.front());
        for (int y = 0; y < h; y++) {
          *(dstp++) = 0;                // filter type (none)
          switch (bitDepth & (~(optimizePalette ? 0 : 8))) {
          case 1:
            for (int x = 0; x < w; x++)
              dstp[x >> 3] |= (colorReplaceTable[srcp[x]] << ((~x) & 7));
            break;
          case 2:
            for (int x = 0; x < w; x++)
              dstp[x >> 2] |= (colorReplaceTable[srcp[x]] << (((~x) & 3) << 1));
            break;
          case 4:
            for (int x = 0; x < w; x++)
              dstp[x >> 1] |= (colorReplaceTable[srcp[x]] << (((~x) & 1) << 2));
            break;
          case 8:
            for (int x = 0; x < w; x++)
              dstp[x] = colorReplaceTable[srcp[x]];
            break;
          default:
            std::memcpy(dstp, srcp, lineBytesO);
            break;
          }
          srcp = srcp + lineBytesI;
          dstp = dstp + lineBytesO;
        }
        Compressor_ZLib::compressData(outBuf, &(imgDataBuf.front()),
                                      imgDataBuf.size(), blockSize);
      }
      f = std::fopen(fileName, "wb");
      if (!f)
        throw Ep128Emu::Exception("error opening PNG image file");
      if (std::fwrite(pngSignature, sizeof(char), 8, f) != 8)
        throw Ep128Emu::Exception("error writing PNG image file");
      writePNGUInt32(hdrBufP, uint32_t(w));
      writePNGUInt32(hdrBufP + 4, uint32_t(h));
      hdrBufP[8] = bitDepth;                    // bit depth
      hdrBufP[9] = (nColors > 0 ? 3 : 2);       // color type
      hdrBufP[10] = 0;                          // compression method (Deflate)
      hdrBufP[11] = 0;                          // filter method (adaptive)
      hdrBufP[12] = 0;                          // interlace method (none)
      writePNGChunk(f, 0x49484452, hdrBufP, 13);    // "IHDR"
      writePNGUInt32(hdrBufP, uint32_t(int(100000.0 / 2.2 + 0.5)));
      writePNGChunk(f, 0x67414D41, hdrBufP, 4);     // "gAMA"
      if (nColors > 0) {
        for (int i = 0; i < nColors; i++) {
          hdrBufP[i * 3] = inBuf[int(paletteIndexTable[i]) * 3];
          hdrBufP[i * 3 + 1] = inBuf[int(paletteIndexTable[i]) * 3 + 1];
          hdrBufP[i * 3 + 2] = inBuf[int(paletteIndexTable[i]) * 3 + 2];
        }
        writePNGChunk(f, 0x504C5445, hdrBufP, size_t(nColors) * 3);   // "PLTE"
      }
      writePNGChunk(f, 0x49444154, &(outBuf.front()), outBuf.size()); // "IDAT"
      writePNGChunk(f, 0x49454E44, hdrBufP, 0);     // "IEND"
      int     err = std::fflush(f) | std::fclose(f);
      f = (std::FILE *) 0;
      if (err != 0)
        throw Ep128Emu::Exception("error writing PNG image file");
    }
    catch (...) {
      if (f) {
        std::fclose(f);
        std::remove(fileName);
      }
      throw;
    }
#ifdef PNGWRITE_DEBUG
    double  t1 = tt.getRealTime();
    std::fprintf(stderr, "Compression time = %f\n", t1 - t0);
#endif
  }

}       // namespace Ep128Emu

