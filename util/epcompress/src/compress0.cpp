
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

#include "ep128emu.hpp"
#include "compress.hpp"
#include "comprlib.hpp"
#include "compress0.hpp"

#include <list>
#include <map>

static size_t writeGammaCode(std::vector< unsigned int >& outBuf, size_t n)
{
  unsigned int  c = 0U;
  unsigned char nBits = 1;
  while (n > 1) {
    c = c | ((2U | ((unsigned int) n & 1U)) << nBits);
    n = n >> 1;
    nBits = nBits + 2;
  }
  outBuf.push_back(((unsigned int) nBits << 24) | c);
  return nBits;
}

static inline size_t estimateSymbolLength(size_t cnt, size_t totalCount)
{
  size_t  nBits = 0;
  do {
    cnt = cnt << 1;
    nBits++;
  } while (cnt < totalCount);
  return nBits;
}

namespace Ep128Compress {

  Compressor_M0::DSearchTable::DSearchTable(size_t minLength, size_t maxLength,
                                            size_t maxOffs)
    : LZSearchTable(minLength, maxLength, maxLength, 0, maxOffs, maxOffs)
  {
  }

  Compressor_M0::DSearchTable::~DSearchTable()
  {
  }

  void Compressor_M0::DSearchTable::findMatches(const unsigned char *buf,
                                                size_t bufSize)
  {
    if (bufSize < 1) {
      throw Ep128Emu::Exception("Compressor_M0::DSearchTable::DSearchTable(): "
                                "zero input buffer size");
    }
    // find matches with delta value (offset range: 1..4, delta range: -64..64)
    seqDiffTable.clear();
    maxSeqLenTable.clear();
    seqDiffTable.resize(4);
    maxSeqLenTable.resize(4);
    for (size_t i = 0; i < 4; i++) {
      seqDiffTable[i].resize(bufSize);
      maxSeqLenTable[i].resize(bufSize);
      for (size_t j = 0; j < bufSize; j++) {
        seqDiffTable[i][j] = 0x00;
        maxSeqLenTable[i][j] = 0;
      }
    }
    for (size_t i = Compressor_M0::minRepeatDist;
         (i + Compressor_M0::minRepeatLen) <= bufSize;
         i++) {
      for (size_t j = 1; j <= 4; j++) {
        if (i >= j) {
          unsigned char d = (buf[i] - buf[i - j]) & 0xFF;
          if (d != 0x00 && (d >= 0xC0 || d <= 0x40)) {
            size_t  k = i;
            size_t  l = i - j;
            while (k < bufSize && (k - i) < Compressor_M0::maxRepeatLen &&
                   buf[k] == ((buf[l] + d) & 0xFF)) {
              k++;
              l++;
            }
            k = k - i;
            if (k >= Compressor_M0::minRepeatLen) {
              seqDiffTable[j - 1][i] = d;
              maxSeqLenTable[j - 1][i] = (unsigned short) k;
            }
          }
        }
      }
    }
    LZSearchTable::findMatches(buf, 0, bufSize);
  }

  // --------------------------------------------------------------------------

  static size_t calculateHuffmanEncoding(
      std::vector< unsigned int >& outBuf,
      const std::vector< unsigned int >& inBuf,
      HuffmanEncoder& huffmanEncoder, std::vector< unsigned int >& encodeTable)
  {
    unsigned int  symbolCnts[324];
    size_t  n = inBuf.size();
    size_t  totalCnt = 0;
    size_t  nSymbols = encodeTable.size();
    for (size_t i = 0; i < nSymbols; i++)
      symbolCnts[i] = 0U;
    huffmanEncoder.clear();
    unsigned int  symOffs = (nSymbols < 256 ? 0x180U : 0U);
    for (size_t i = 4; i < n; i++) {
      unsigned int  c = inBuf[i] - symOffs;
      if (c < 324U) {
        symbolCnts[c] = symbolCnts[c] + 1U;
        totalCnt++;
        huffmanEncoder.addSymbol(c);
      }
    }
    size_t  uncompressedSize = totalCnt * size_t(nSymbols < 256 ? 5 : 9) + 1;
    size_t  compressedSize = 1;
    // create optimal encoding table
    huffmanEncoder.updateTables(false, 16);
    for (unsigned int c = 0U; c < (unsigned int) nSymbols; c++) {
      if (huffmanEncoder.getSymbolSize(c) <= 16) {
        unsigned int  tmp = huffmanEncoder.encodeSymbol(c);
        encodeTable[c] = tmp;
        compressedSize += size_t(symbolCnts[c] * (tmp >> 24));
      }
    }
    // create decode table to be written to the output buffer,
    // and calculate its size
    outBuf.push_back(0x01000001U);
    size_t  savedBufSize = outBuf.size();
    for (unsigned int l = 1U; l <= 16U; l++) {
      size_t  sizeCnt = 0;
      for (unsigned int c = 0U; c < nSymbols; c++)
        sizeCnt += size_t((encodeTable[c] >> 24) == l);
      if (EP128EMU_UNLIKELY(sizeCnt >= 256)) {
        // compatibility hack for new FAST_HUFFMAN decompressor code
        if (EP128EMU_UNLIKELY(l >= 16U || sizeCnt > 256)) {
          std::fprintf(stderr, "WARNING: Huffman encoding disabled "
                               "for compatibility reasons\n");
          compressedSize = uncompressedSize;
        }
        else {
          sizeCnt--;
          for (unsigned int c = nSymbols; c-- > 0U; ) {
            if ((encodeTable[c] >> 24) == l) {
              encodeTable[c] =
                  ((encodeTable[c] & 0x003FFFFFU) << 1) | ((l + 1U) << 24);
              break;
            }
          }
        }
      }
      compressedSize += writeGammaCode(outBuf, sizeCnt + 1);
      unsigned int  prvCode = 0U - 1U;
      for (unsigned int c = 0U; c < nSymbols; c++) {
        if ((encodeTable[c] >> 24) == l) {
          size_t  d = size_t(c - prvCode);
          prvCode = c;
          compressedSize += writeGammaCode(outBuf, d);
        }
      }
    }
    // if the size cannot be reduced, store the data without compression
    if (compressedSize >= uncompressedSize) {
      outBuf.resize(savedBufSize);
      outBuf[savedBufSize - 1] = 0x01000000U;
      for (unsigned int i = 0U; i < (unsigned int) nSymbols; i++)
        encodeTable[i] = (nSymbols < 256 ? 0x05000000U : 0x09000000U) | i;
    }
    return totalCnt;
  }

  void Compressor_M0::huffmanCompressBlock(std::vector< unsigned int >& ioBuf)
  {
    size_t  n = ioBuf.size();
    if (n <= 4)
      return;
    std::vector< unsigned int > encodeTable1(324, 0U);
    std::vector< unsigned int > encodeTable2(28, 0U);
    std::vector< unsigned int > tmpBuf;
    size_t  charCnt1 =
        calculateHuffmanEncoding(tmpBuf, ioBuf, huffmanEncoder1, encodeTable1);
    size_t  charCnt2 =
        calculateHuffmanEncoding(tmpBuf, ioBuf, huffmanEncoder2, encodeTable2);
    ioBuf.resize(n + tmpBuf.size());    // reserve space for the decode table
    for (size_t i = n; i-- > 4; ) {
      if (ioBuf[i] <= 0x17FU)
        ioBuf[i + tmpBuf.size()] = encodeTable1[ioBuf[i]];
      else if (ioBuf[i] <= 0x1FFU)
        ioBuf[i + tmpBuf.size()] = encodeTable2[ioBuf[i] & 0x7FU];
      else
        ioBuf[i + tmpBuf.size()] = ioBuf[i];
    }
    // insert decode table after the block header
    for (size_t i = 0; i < tmpBuf.size(); i++)
      ioBuf[i + 4] = tmpBuf[i];
    // update estimated symbol sizes
    for (size_t i = 0x0000; i < 0x0144; i++) {
      size_t  nBits = size_t(encodeTable1[i] >> 24);
      if (nBits == 0)
        nBits = estimateSymbolLength(1, charCnt1 + 1);
      tmpCharBitsTable[i] = nBits;
    }
    for (size_t i = 0x0180; i < 0x019C; i++) {
      size_t  nBits = size_t(encodeTable2[i - 0x0180] >> 24);
      if (nBits == 0)
        nBits = estimateSymbolLength(1, charCnt2 + 1);
      tmpCharBitsTable[i] = nBits;
    }
  }

  void Compressor_M0::initializeLengthCodeTables()
  {
    for (size_t i = minRepeatDist; i <= maxRepeatDist; i++) {
      size_t  lCode = 0;
      size_t  lBits = 0;
      size_t  lValue = i - minRepeatDist;
      if (lValue <= 7) {
        lCode = lValue;
        lValue = 0;
      }
      else {
        while (lValue >= (size_t(1) << lBits))
          lBits++;
        lCode = ((lValue >> (lBits - 3)) & 3) | ((lBits - 2) << 2);
        lBits = lBits - 3;
        lValue = lValue & ((size_t(1) << lBits) - 1);
      }
      size_t  j = (i - minRepeatDist) + minRepeatLen;
      if (j >= minRepeatLen && j <= maxRepeatLen) {
        lengthCodeTable[j] = (unsigned short) (lCode | 0x0180);
        lengthBitsTable[j] = (unsigned char) lBits;
        lengthValueTable[j] = (unsigned int) (lValue | (lBits << 24));
      }
      distanceCodeTable[i] = (unsigned short) (lCode | 0x0100);
      distanceBitsTable[i] = (unsigned char) lBits;
      distanceValueTable[i] = (unsigned int) (lValue | (lBits << 24));
    }
  }

  void Compressor_M0::writeRepeatCode(std::vector< unsigned int >& buf,
                                      size_t d, size_t n)
  {
    if (d > 8) {
      for (size_t i = 0; i < 4; i++) {
        if (d == prvDistances[i]) {
          unsigned int  c = 0x0140U | (unsigned int) i;
          if (tmpCharBitsTable[c] >= (tmpCharBitsTable[distanceCodeTable[d]]
                                      + size_t(distanceBitsTable[d]))) {
            break;
          }
          buf.push_back(c);
          c = (unsigned int) lengthCodeTable[n];
          buf.push_back(c);
          if (lengthBitsTable[n] > 0)
            buf.push_back(lengthValueTable[n]);
          return;
        }
      }
      for (size_t i = 3; i > 0; i--)
        prvDistances[i] = prvDistances[i - 1];
      prvDistances[0] = d;
    }
    unsigned int  c = (unsigned int) distanceCodeTable[d];
    buf.push_back(c);
    if (distanceBitsTable[d] > 0)
      buf.push_back(distanceValueTable[d]);
    c = (unsigned int) lengthCodeTable[n];
    buf.push_back(c);
    if (lengthBitsTable[n] > 0)
      buf.push_back(lengthValueTable[n]);
  }

  void Compressor_M0::writeSequenceCode(std::vector< unsigned int >& buf,
                                        unsigned char seqDiff,
                                        size_t d, size_t n)
  {
    unsigned int  c = (unsigned int) distanceCodeTable[d] | 0x003CU;
    buf.push_back(c);
    seqDiff = (seqDiff + 0x40) & 0xFF;
    if (seqDiff > 0x40)
      seqDiff--;
    buf.push_back(0x07000000U | (unsigned int) seqDiff);
    c = (unsigned int) lengthCodeTable[n];
    buf.push_back(c);
    if (lengthBitsTable[n] > 0)
      buf.push_back(lengthValueTable[n]);
  }

  void Compressor_M0::optimizeMatches(LZMatchParameters *matchTable,
                                      BitCountTableEntry *bitCountTable,
                                      const size_t *lengthBitsTable_,
                                      const unsigned char *inBuf,
                                      size_t offs, size_t nBytes)
  {
    for (size_t i = nBytes; i-- > 0; ) {
      long    bestSize = 0x7FFFFFFFL;
      size_t  bestLen = 1;
      size_t  bestOffs = 0;
      bool    bestSeqFlag = false;
      unsigned char bestSeqDiff = 0x00;
      size_t  minLen =
          (config.minLength > minRepeatLen ? config.minLength : minRepeatLen);
      size_t  maxLen = nBytes - i;
      // check LZ77 matches
      const unsigned int  *matchPtr = searchTable->getMatches(offs + i);
      while (size_t(*matchPtr & 0x03FFU) >= minLen) {
        size_t  len = *matchPtr & 0x03FFU;
        size_t  d = *(matchPtr++) >> 10;
        len = (len < maxLen ? len : maxLen);
        size_t  offsBits = tmpCharBitsTable[distanceCodeTable[d]];
        if (d > 8) {
          // long offset: need to search the previous offsets table
          offsBits += size_t(distanceBitsTable[d]);
          for ( ; len >= minLen; len--) {
            const BitCountTableEntry& nxtMatch = bitCountTable[i + len];
            long    nBits = long(lengthBitsTable_[len]) + nxtMatch.totalBits;
            if (size_t(nxtMatch.prvDistances[0]) == d) {
              nBits += long(tmpCharBitsTable[0x0140] < offsBits ?
                            tmpCharBitsTable[0x0140] : offsBits);
            }
            else if (size_t(nxtMatch.prvDistances[1]) == d) {
              nBits += long(tmpCharBitsTable[0x0141] < offsBits ?
                            tmpCharBitsTable[0x0141] : offsBits);
            }
            else if (size_t(nxtMatch.prvDistances[2]) == d) {
              nBits += long(tmpCharBitsTable[0x0142] < offsBits ?
                            tmpCharBitsTable[0x0142] : offsBits);
            }
            else if (size_t(nxtMatch.prvDistances[3]) == d) {
              nBits += long(tmpCharBitsTable[0x0143] < offsBits ?
                            tmpCharBitsTable[0x0143] : offsBits);
            }
            else {
              nBits += long(offsBits);
            }
            if (nBits > bestSize)
              continue;
            if (nBits == bestSize) {
              if (d >= bestOffs)
                continue;
            }
            bestSize = nBits;
            bestLen = len;
            bestOffs = d;
          }
        }
        else {
          // short offset
          for ( ; len >= minLen; len--) {
            const BitCountTableEntry& nxtMatch = bitCountTable[i + len];
            long    nBits = long(lengthBitsTable_[len] + offsBits)
                            + nxtMatch.totalBits;
            if (nBits > bestSize)
              continue;
            if (nBits == bestSize) {
              if (d >= bestOffs)
                continue;
            }
            bestSize = nBits;
            bestLen = len;
            bestOffs = d;
          }
        }
      }
      // check matches with delta value
      for (size_t d = 1; d <= 4; d++) {
        size_t  len = searchTable->getSequenceLength(offs + i, d);
        if (len < minLen)
          continue;
        if (d > config.maxOffset)
          break;
        unsigned char seqDiff = searchTable->getSequenceDeltaValue(offs + i, d);
        size_t  offsBits = tmpCharBitsTable[0x013B + d] + 7;
        len = (len < maxLen ? len : maxLen);
        for ( ; len >= minLen; len--) {
          const BitCountTableEntry& nxtMatch = bitCountTable[i + len];
          long    nBits = long(lengthBitsTable_[len] + offsBits)
                          + nxtMatch.totalBits;
          if (nBits > bestSize)
            continue;
          if (nBits == bestSize) {
            if (d >= bestOffs)
              continue;
          }
          bestSize = nBits;
          bestLen = len;
          bestOffs = d;
          bestSeqFlag = true;
          bestSeqDiff = seqDiff;
        }
      }
      // check literal byte
      {
        long    nBits = long(tmpCharBitsTable[inBuf[offs + i]])
                        + bitCountTable[i + 1].totalBits;
        if (nBits <= bestSize) {
          bestSize = nBits;
          bestLen = 1;
          bestOffs = 0;
          bestSeqFlag = false;
          bestSeqDiff = 0x00;
        }
      }
      matchTable[i].d = (unsigned int) bestOffs;
      matchTable[i].len = (unsigned short) bestLen;
      matchTable[i].seqFlag = bestSeqFlag;
      matchTable[i].seqDiff = bestSeqDiff;
      bitCountTable[i] = bitCountTable[i + bestLen];
      bitCountTable[i].totalBits = bestSize;
      if (bestOffs > 8) {
        for (int nn = 0; true; nn++) {
          if (size_t(bitCountTable[i].prvDistances[nn]) == bestOffs)
            break;
          if (nn >= 3) {
            while (--nn >= 0) {
              bitCountTable[i].prvDistances[nn + 1] =
                  bitCountTable[i].prvDistances[nn];
            }
            bitCountTable[i].prvDistances[0] = (unsigned int) bestOffs;
            break;
          }
        }
      }
    }
  }

  void Compressor_M0::compressData_(std::vector< unsigned int >& tmpOutBuf,
                                    const std::vector< unsigned char >& inBuf,
                                    unsigned int startAddr, size_t offs,
                                    size_t nBytes)
  {
    size_t  endPos = offs + nBytes;
    // write data header (start address, 2's complement of the number of bytes,
    // last block flag, and compression enabled flag)
    tmpOutBuf.clear();
    if (startAddr < 0x80000000U) {
      tmpOutBuf.push_back(0x10000000U | (unsigned int) (startAddr + offs));
      tmpOutBuf.push_back(0x10000000U | (unsigned int) (65536 - nBytes));
    }
    else {
      // hack to keep the header size constant
      tmpOutBuf.push_back(0x08000000U | (unsigned int) ((65536 - nBytes) >> 8));
      tmpOutBuf.push_back(0x08000000U
                          | (unsigned int) ((65536 - nBytes) & 0xFF));
    }
    tmpOutBuf.push_back(0x01000000U);   // reserved for last block flag
    tmpOutBuf.push_back(0x01000001U);
    // compress data by searching for repeated byte sequences,
    // and replacing them with length/distance codes
    for (size_t i = 0; i < 4; i++)
      prvDistances[i] = 0;
    std::vector< LZMatchParameters >  matchTable(nBytes);
    {
      std::vector< BitCountTableEntry > bitCountTable(nBytes + 1);
      std::vector< size_t > lengthBitsTable_(maxRepeatLen + 1, 0x7FFF);
      for (size_t i = minRepeatLen; i <= maxRepeatLen; i++) {
        lengthBitsTable_[i] = tmpCharBitsTable[lengthCodeTable[i]]
                              + size_t(lengthBitsTable[i]);
      }
      bitCountTable[nBytes].totalBits = 0L;
      for (size_t i = 0; i < 4; i++)
        bitCountTable[nBytes].prvDistances[i] = 0;
      optimizeMatches(&(matchTable.front()), &(bitCountTable.front()),
                      &(lengthBitsTable_.front()), &(inBuf.front()),
                      offs, nBytes);
    }
    for (size_t i = offs; i < endPos; ) {
      LZMatchParameters&  tmp = matchTable[i - offs];
      if (tmp.len >= minRepeatLen) {
        if (tmp.seqFlag)
          writeSequenceCode(tmpOutBuf, tmp.seqDiff, tmp.d, tmp.len);
        else
          writeRepeatCode(tmpOutBuf, tmp.d, tmp.len);
        i = i + tmp.len;
      }
      else {
        unsigned int  c = (unsigned int) inBuf[i];
        i++;
        tmpOutBuf.push_back(c);
      }
    }
  }

  bool Compressor_M0::compressData(std::vector< unsigned int >& tmpOutBuf,
                                   const std::vector< unsigned char >& inBuf,
                                   unsigned int startAddr, bool isLastBlock,
                                   size_t offs, size_t nBytes)
  {
    // the 'offs' and 'nBytes' parameters allow compressing a buffer
    // as multiple chunks for possibly improved statistical compression
    if (nBytes < 1 || offs >= inBuf.size())
      return true;
    if (nBytes > (inBuf.size() - offs))
      nBytes = inBuf.size() - offs;
    size_t  endPos = offs + nBytes;
    for (size_t i = 0; i < 512; i++)
      tmpCharBitsTable[i] = (i < 0x0180 ? 9 : 5);
    std::vector< uint64_t >     hashTable;
    std::vector< unsigned int > bestBuf;
    std::vector< unsigned int > tmpBuf;
    size_t  bestSize = 0x7FFFFFFF;
    bool    doneFlag = false;
    for (size_t i = 0; i < config.optimizeIterations; i++) {
      if (progressDisplayEnabled) {
        if (!setProgressPercentage(int(progressCnt * uint64_t(100)
                                       / progressMax))) {
          return false;
        }
        progressCnt += nBytes;
      }
      if (doneFlag)     // if the compression cannot be optimized further,
        continue;       // quit the loop earlier
      tmpBuf.clear();
      compressData_(tmpBuf, inBuf, startAddr, offs, nBytes);
      // apply statistical compression
      huffmanCompressBlock(tmpBuf);
      // calculate compressed size and hash value
      size_t    compressedSize = 0;
      uint64_t  h = 1UL;
      for (size_t j = 0; j < tmpBuf.size(); j++) {
        compressedSize += size_t(tmpBuf[j] >> 24);
        h = h ^ uint64_t(tmpBuf[j]);
        h = uint32_t(h) * uint64_t(0xC2B0C3CCUL);
        h = (h ^ (h >> 32)) & 0xFFFFFFFFUL;
      }
      h = h | (uint64_t(compressedSize) << 32);
      if (compressedSize < bestSize) {
        // found a better compression, so save it
        bestSize = compressedSize;
        bestBuf.resize(tmpBuf.size());
        std::memcpy(&(bestBuf.front()), &(tmpBuf.front()),
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
    size_t  uncompressedSize = nBytes * 8 + (startAddr < 0x80000000U ? 34 : 18);
    size_t  outBufOffset = tmpOutBuf.size();
    if (bestSize >= uncompressedSize) {
      // if cannot reduce the data size, store without compression
      for (size_t i = 0; i < 3; i++)
        tmpOutBuf.push_back(bestBuf[i]);
      tmpOutBuf.push_back(0x01000000U);
      for (size_t i = offs; i < endPos; i++)
        tmpOutBuf.push_back(0x08000000U | (unsigned int) inBuf[i]);
    }
    else {
      // append compressed data to output buffer
      for (size_t i = 0; i < bestBuf.size(); i++)
        tmpOutBuf.push_back(bestBuf[i]);
    }
    // store last block flag
    tmpOutBuf[outBufOffset + 2] = (isLastBlock ? 0x01000001U : 0x01000000U);
    return true;
  }

  // --------------------------------------------------------------------------

  Compressor_M0::Compressor_M0(std::vector< unsigned char >& outBuf_)
    : Compressor(outBuf_),
      lengthCodeTable((unsigned short *) 0),
      lengthBitsTable((unsigned char *) 0),
      lengthValueTable((unsigned int *) 0),
      distanceCodeTable((unsigned short *) 0),
      distanceBitsTable((unsigned char *) 0),
      distanceValueTable((unsigned int *) 0),
      tmpCharBitsTable((size_t *) 0),
      searchTable((DSearchTable *) 0),
      outputShiftReg(0x00),
      outputBitCnt(0),
      huffmanEncoder1(324, 0),
      huffmanEncoder2(28, 0)
  {
    try {
      lengthCodeTable = new unsigned short[maxRepeatLen + 1];
      lengthBitsTable = new unsigned char[maxRepeatLen + 1];
      lengthValueTable = new unsigned int[maxRepeatLen + 1];
      distanceCodeTable = new unsigned short[maxRepeatDist + 1];
      distanceBitsTable = new unsigned char[maxRepeatDist + 1];
      distanceValueTable = new unsigned int[maxRepeatDist + 1];
      tmpCharBitsTable = new size_t[512];
      initializeLengthCodeTables();
      for (size_t i = 0; i < 512; i++)
        tmpCharBitsTable[i] = (i < 0x0180 ? 9 : 5);
      for (size_t i = 0; i < 4; i++)
        prvDistances[i] = 0;
    }
    catch (...) {
      if (lengthCodeTable)
        delete[] lengthCodeTable;
      if (lengthBitsTable)
        delete[] lengthBitsTable;
      if (lengthValueTable)
        delete[] lengthValueTable;
      if (distanceCodeTable)
        delete[] distanceCodeTable;
      if (distanceBitsTable)
        delete[] distanceBitsTable;
      if (distanceValueTable)
        delete[] distanceValueTable;
      if (tmpCharBitsTable)
        delete[] tmpCharBitsTable;
      throw;
    }
  }

  Compressor_M0::~Compressor_M0()
  {
    delete[] lengthCodeTable;
    delete[] lengthBitsTable;
    delete[] lengthValueTable;
    delete[] distanceCodeTable;
    delete[] distanceBitsTable;
    delete[] distanceValueTable;
    delete[] tmpCharBitsTable;
    if (searchTable)
      delete searchTable;
  }

  bool Compressor_M0::compressData(const std::vector< unsigned char >& inBuf,
                                   unsigned int startAddr, bool isLastBlock,
                                   bool enableProgressDisplay)
  {
    if (outputBitCnt < 0)
      throw Ep128Emu::Exception("internal error: compressing to closed buffer");
    if (inBuf.size() < 1)
      return true;
    progressDisplayEnabled = enableProgressDisplay;
    try {
      if (enableProgressDisplay) {
        progressMessage("Compressing data");
        setProgressPercentage(0);
      }
      if (searchTable) {
        delete searchTable;
        searchTable = (DSearchTable *) 0;
      }
      searchTable =
          new DSearchTable(
                  (config.minLength > minRepeatLen ?
                   config.minLength : minRepeatLen), maxRepeatLen,
                  (config.maxOffset < maxRepeatDist ?
                   config.maxOffset : maxRepeatDist));
      searchTable->findMatches(&(inBuf.front()), inBuf.size());
      if (config.optimizeIterations > 16)
        config.optimizeIterations = 16;
      // split large files to improve statistical compression
      std::list< SplitOptimizationBlock >   splitPositions;
      std::map< uint64_t, bool >    splitOptimizationCache;
      size_t  splitDepth = config.splitOptimizationDepth;
      size_t  splitCnt = size_t(1) << splitDepth;
      while (((inBuf.size() + splitCnt - 1) / splitCnt) > 65536) {
        splitDepth++;                   // limit block size to <= 64K
        splitCnt = splitCnt << 1;
      }
      while (splitCnt > inBuf.size()) {
        splitDepth--;                   // avoid zero block size
        splitCnt = splitCnt >> 1;
      }
      progressCnt = 0;
      progressMax = config.optimizeIterations * inBuf.size();
      if (config.blockSize < 1) {
        size_t  tmp = 0;
        size_t  tmp2 = (inBuf.size() + splitCnt - 1) / splitCnt;
        do {
          tmp++;
          tmp2 = tmp2 << 1;
        } while (tmp < splitDepth && tmp2 <= 65536);
        progressMax = progressMax * (tmp + size_t(tmp > 1));
      }
      {
        // create initial block list
        size_t  tmp = 0;
        for (size_t startPos = 0; startPos < inBuf.size(); ) {
          SplitOptimizationBlock  tmpBlock;
          tmpBlock.startPos = startPos;
          if (config.blockSize < 1) {
            tmp = tmp + inBuf.size();
            tmpBlock.nBytes = tmp / splitCnt;
            tmp = tmp % splitCnt;
          }
          else {
            // force block size specified by the user
            tmpBlock.nBytes = (config.blockSize < (inBuf.size() - startPos) ?
                               config.blockSize : (inBuf.size() - startPos));
          }
          startPos = startPos + tmpBlock.nBytes;
          tmpBlock.compressedSize = 0;
          tmpBlock.isLastBlock = (startPos >= inBuf.size()) && isLastBlock;
          tmpBlock.buf.clear();
          if (!compressData(tmpBlock.buf, inBuf, startAddr,
                            tmpBlock.isLastBlock,
                            tmpBlock.startPos, tmpBlock.nBytes)) {
            delete searchTable;
            searchTable = (DSearchTable *) 0;
            if (progressDisplayEnabled)
              progressMessage("");
            return false;
          }
          // calculate compressed size
          for (size_t j = 0; j < tmpBlock.buf.size(); j++)
            tmpBlock.compressedSize += size_t(tmpBlock.buf[j] >> 24);
          splitPositions.push_back(tmpBlock);
        }
      }
      while (config.blockSize < 1) {
        bool    mergeFlag = false;
        std::list< SplitOptimizationBlock >::iterator i_0 =
            splitPositions.begin();
        while (i_0 != splitPositions.end()) {
          std::list< SplitOptimizationBlock >::iterator i_1 = i_0;
          i_1++;
          if (i_1 == splitPositions.end())
            break;
          uint64_t  cacheKey = uint64_t((*i_0).startPos)
                               | (uint64_t((*i_1).startPos) << 20)
                               | (uint64_t((*i_1).nBytes) << 40);
          if (splitOptimizationCache.find(cacheKey)
              != splitOptimizationCache.end()) {
            // if this pair of blocks was already tested earlier,
            // skip testing again
            i_0 = i_1;
            continue;
          }
          if (((*i_0).nBytes + (*i_1).nBytes) > 65536) {
            i_0 = i_1;
            continue;                   // limit block size to <= 64K
          }
          SplitOptimizationBlock  tmpBlock;
          tmpBlock.startPos = (*i_0).startPos;
          tmpBlock.nBytes = (*i_0).nBytes + (*i_1).nBytes;
          tmpBlock.compressedSize = 0;
          tmpBlock.isLastBlock = (*i_1).isLastBlock;
          tmpBlock.buf.clear();
          if ((progressCnt + (config.optimizeIterations * tmpBlock.nBytes))
              > progressMax) {
            progressMax =
                progressCnt + (config.optimizeIterations * tmpBlock.nBytes);
          }
          if (!compressData(tmpBlock.buf, inBuf, startAddr,
                            tmpBlock.isLastBlock,
                            tmpBlock.startPos, tmpBlock.nBytes)) {
            delete searchTable;
            searchTable = (DSearchTable *) 0;
            if (progressDisplayEnabled)
              progressMessage("");
            return false;
          }
          // calculate compressed size after merging blocks
          for (size_t j = 0; j < tmpBlock.buf.size(); j++)
            tmpBlock.compressedSize += size_t(tmpBlock.buf[j] >> 24);
          if (tmpBlock.compressedSize
              <= ((*i_0).compressedSize + (*i_1).compressedSize)) {
            // splitting does not reduce size, so use merged block
            (*i_0) = tmpBlock;
            i_0 = splitPositions.erase(i_1);
            mergeFlag = true;
          }
          else {
            i_0 = i_1;
            // blocks not merged: remember block positions and sizes, so that
            // the same pair of blocks will not need to be tested again
            splitOptimizationCache[cacheKey] = true;
          }
        }
        if (!mergeFlag)
          break;
      }
      delete searchTable;
      searchTable = (DSearchTable *) 0;
      std::vector< unsigned int >   outBufTmp;
      std::list< SplitOptimizationBlock >::iterator i_ = splitPositions.begin();
      while (i_ != splitPositions.end()) {
        for (size_t i = 0; i < (*i_).buf.size(); i++)
          outBufTmp.push_back((*i_).buf[i]);
        i_ = splitPositions.erase(i_);
      }
      if (progressDisplayEnabled) {
        setProgressPercentage(100);
        progressMessage("");
      }
      // pack output data
      if (outBuf.size() == 0)
        outBuf.push_back((unsigned char) 0x00); // reserve space for CRC value
      for (size_t i = 0; i < outBufTmp.size(); i++) {
        unsigned int  c = outBufTmp[i];
        unsigned int  nBits = c >> 24;
        c = c & 0x00FFFFFFU;
        for (unsigned int j = nBits; j-- > 0U; ) {
          unsigned int  b = (unsigned int) (bool(c & (1U << j)));
          outputShiftReg = ((outputShiftReg & 0x7F) << 1) | (unsigned char) b;
          if (++outputBitCnt >= 8) {
            outBuf.push_back(outputShiftReg);
            outputShiftReg = 0x00;
            outputBitCnt = 0;
          }
        }
      }
      if (isLastBlock) {
        while (outputBitCnt != 0) {
          outputShiftReg = ((outputShiftReg & 0x7F) << 1);
          if (++outputBitCnt >= 8) {
            outBuf.push_back(outputShiftReg);
            outputShiftReg = 0x00;
            outputBitCnt = 0;
          }
        }
        // calculate CRC
        unsigned char crcVal = 0x00;
        for (size_t i = outBuf.size() - 1; i > 0; i--) {
          unsigned int  tmp = (unsigned int) crcVal ^ (unsigned int) outBuf[i];
          tmp = ((tmp << 1) + ((tmp & 0x80U) >> 7) + 0xC4U) & 0xFFU;
          crcVal = (unsigned char) tmp;
        }
        crcVal = (unsigned char) ((0x180 - 0xC4) >> 1) ^ crcVal;
        outBuf[0] = crcVal;
        outputBitCnt = -1;      // set output buffer closed flag
      }
    }
    catch (...) {
      if (searchTable) {
        delete searchTable;
        searchTable = (DSearchTable *) 0;
      }
      if (progressDisplayEnabled)
        progressMessage("");
      throw;
    }
    return true;
  }

}       // namespace Ep128Compress

