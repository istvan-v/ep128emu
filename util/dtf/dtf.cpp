
// dtf.cpp
// Copyright (C) 2008-2010 Istvan Varga <istvanv@users.sourceforge.net>
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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <exception>
#include <new>
#include <string>
#include <vector>
#include <list>
#include <map>

#include "epcompress/ep128emu.hpp"
#include "epcompress/compress.hpp"
#include "epcompress/compress0.hpp"
#include "epcompress/compress0.cpp"
#include "epcompress/compress2.hpp"
#include "epcompress/compress2.cpp"
#include "epcompress/compress3.hpp"
#include "epcompress/compress3.cpp"
#include "epcompress/decompress0.hpp"
#include "epcompress/decompress0.cpp"
#include "epcompress/decompress2.hpp"
#include "epcompress/decompress2.cpp"
#include "epcompress/decompress3.hpp"
#include "epcompress/decompress3.cpp"
#include "epcompress/compress.cpp"

using Ep128Emu::Exception;

struct DTFCompressionParameters {
  size_t  blockSize;
  int     minPrefixSize;
  int     maxPrefixSize;
  bool    disableStatisticalCompression;
  // 0: DTF
  // 1: LZ
  // 2: LZ2 (epcompress -m2)
  // 3: LZ0 (epcompress -m0)
  uint8_t compressionType;
  uint8_t compressionLevel;
  // --------
  DTFCompressionParameters()
    : blockSize(16384),
      minPrefixSize(2),
      maxPrefixSize(2),
      disableStatisticalCompression(false),
      compressionType(0),
      compressionLevel(5)
  {
  }
};

// ----------------------------------------------------------------------------

static void fixDTFFileNameCharacter(char& c, bool toUpperCase = false)
{
  if (toUpperCase) {
    if (c >= 'a' && c <= 'z') {
      c = (c - 'a') + 'A';
      return;
    }
  }
  else {
    if (c >= 'A' && c <= 'Z') {
      c = (c - 'A') + 'a';
      return;
    }
  }
  if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '.' || c == '+' || c == '-' || c == '_')) {
    c = '_';
  }
}

static void removeFileNamePath(std::string& s)
{
  if (s.length() > 0) {
    size_t  i = s.length();
    while (i > 0) {
      i--;
      if (s[i] == '/' || s[i] == '\\') {
        i++;
        break;
      }
#ifdef WIN32
      if (i == 1) {
        if (((s[0] >= 'A' && s[0] <= 'Z') || (s[0] >= 'a' && s[0] <= 'z')) &&
            s[i] == ':') {
          i++;
          break;
        }
      }
#endif
    }
    if (i > 0 && i < s.length())
      s.erase(0, i);
  }
}

static void fixDTFFileName(std::string& s, bool toUpperCase = false,
                           size_t maxLen = 13)
{
  removeFileNamePath(s);
  if (s.length() < 1)
    throw Exception("fixDTFFileName(): empty or invalid file name");
  if (s.length() > maxLen)
    s.resize(maxLen);
  for (size_t j = 0; j < s.length(); j++)
    fixDTFFileNameCharacter(s[j], toUpperCase);
}

static void setFileNameExtension(std::string& s, const char *e)
{
  size_t  i = s.length();
  if (i < 1)
    return;
  while (true) {
    i--;
    if (s[i] == '.') {
      if (i > 0) {
        if (!(s[i - 1] == '/' || s[i - 1] == '\\' || s[i - 1] == ':'))
          break;
      }
      i = s.length();
      break;
    }
    if (i < 1 || s[i] == '/' || s[i] == '\\' || s[i] == ':') {
      i = s.length();
      break;
    }
  }
  s.resize(i);
  if (e != (char *) 0 && e[0] != '\0')
    s += e;
}

// ----------------------------------------------------------------------------

class StatisticalCompressor {
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
  size_t optimizeSlotBitsTable(bool disableZeroSlotSize = false);
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
  StatisticalCompressor(size_t nSlots_, size_t nSymbols_,
                        const size_t *slotPrefixSizeTable_ = (size_t *) 0,
                        size_t minPrefixSize_ = 4,
                        size_t maxPrefixSize_ = 0,
                        const size_t *prefixSlotCntTable_ = (size_t *) 0);
  virtual ~StatisticalCompressor();
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
  inline size_t getSymbolSize(unsigned int n) const
  {
    if (size_t(n) >= nSymbolsEncoded)
      return unusedSymbolSize;
    return symbolSizeTable[n];
  }
  inline size_t getSymbolSlotIndex(unsigned int n) const
  {
    if (size_t(n) >= nSymbolsEncoded)
      throw Exception("internal error: encoding invalid symbol");
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
  void updateTables(bool disableZeroSlotSize = false);
  void clear();
};

StatisticalCompressor::StatisticalCompressor(
    size_t nSlots_, size_t nSymbols_,
    const size_t *slotPrefixSizeTable_,
    size_t minPrefixSize_, size_t maxPrefixSize_,
    const size_t *prefixSlotCntTable_)
  : nSlots(nSlots_),
    nSymbols(nSymbols_),
    nSymbolsUsed(nSymbols_),
    nSymbolsEncoded(nSymbols_),
    totalSlotWeight(0),
    unusedSymbolSize(1),
    minPrefixSize(minPrefixSize_),
    maxPrefixSize(maxPrefixSize_),
    prefixOnlySymbolCnt(0)
{
  if (nSymbols < 1) {
    throw Exception("StatisticalCompressor::StatisticalCompressor(): "
                    "nSymbols < 1");
  }
  if (slotPrefixSizeTable_ != (size_t *) 0) {
    minPrefixSize = 0;
    maxPrefixSize = 0;
  }
  if (maxPrefixSize <= minPrefixSize) {
    maxPrefixSize = minPrefixSize;
    prefixSlotCntTable_ = (size_t *) 0;
  }
  prefixSlotCntTable.resize((maxPrefixSize - minPrefixSize) + 1);
  symbolCntTable.resize(nSymbols + 1);
  unencodedSymbolCostTable.resize(nSymbols + 1);
  symbolSlotNumTable.resize(nSymbols);
  symbolSizeTable.resize(nSymbols);
  for (size_t i = minPrefixSize; i <= maxPrefixSize; i++) {
    if (prefixSlotCntTable_ != (size_t *) 0) {
      prefixSlotCntTable[i - minPrefixSize] =
          prefixSlotCntTable_[i - minPrefixSize];
    }
    else if (maxPrefixSize > minPrefixSize) {
      prefixSlotCntTable[i - minPrefixSize] = size_t(1) << i;
    }
    else {
      prefixSlotCntTable[i - minPrefixSize] = nSlots;
    }
  }
  if (slotPrefixSizeTable_ != (size_t *) 0) {
    if (nSlots < 1) {
      throw Exception("StatisticalCompressor::StatisticalCompressor(): "
                      "nSlots < 1");
    }
    slotPrefixSizeTable.resize(nSlots);
    slotWeightTable.resize(nSlots);
    slotBitsTable.resize(nSlots);
    slotBaseSymbolTable.resize(nSlots);
    maxPrefixSize = 0;
    for (size_t i = 0; i < nSlots; i++) {
      slotPrefixSizeTable[i] = slotPrefixSizeTable_[i];
      if (slotPrefixSizeTable[i] > maxPrefixSize)
        maxPrefixSize = slotPrefixSizeTable[i];
    }
    for (size_t i = 0; i < nSlots; i++) {
      slotWeightTable[i] =
          size_t(1) << (maxPrefixSize - slotPrefixSizeTable[i]);
    }
    minPrefixSize = 0;
    maxPrefixSize = 0;
    totalSlotWeight = 0;
    for (size_t i = 0; i < nSlots; i++)
      totalSlotWeight = totalSlotWeight + slotWeightTable[i];
  }
  else {
    setPrefixSize(minPrefixSize);
  }
  this->clear();
}

StatisticalCompressor::~StatisticalCompressor()
{
}

void StatisticalCompressor::setPrefixSize(size_t n)
{
  if (n < 1) {
    throw Exception("StatisticalCompressor::setPrefixSize(): prefix size < 1");
  }
  if (n < minPrefixSize || n > maxPrefixSize) {
    throw Exception("StatisticalCompressor::setPrefixSize(): "
                    "prefix size is out of range");
  }
  nSlots = prefixSlotCntTable[n - minPrefixSize];
  if (nSlots < 1)
    throw Exception("StatisticalCompressor::setPrefixSize(): nSlots < 1");
  slotPrefixSizeTable.resize(nSlots);
  slotWeightTable.resize(nSlots);
  slotBitsTable.resize(nSlots);
  slotBaseSymbolTable.resize(nSlots);
  for (size_t i = 0; i < nSlots; i++) {
    slotPrefixSizeTable[i] = n;
    slotWeightTable[i] = 1;
  }
  totalSlotWeight = nSlots;
}

inline size_t StatisticalCompressor::calculateEncodedSize() const
{
  size_t  totalSize = 0;
  size_t  p = 0;
  for (size_t i = 0; i < nSlots; i++) {
    size_t  symbolCnt = size_t(symbolCntTable[p]);
    p = p + (size_t(1) << slotBitsTable[i]);
    size_t  symbolSize = slotPrefixSizeTable[i] + slotBitsTable[i];
    if (p >= nSymbolsUsed) {
      return (totalSize + ((size_t(symbolCntTable[nSymbolsUsed]) - symbolCnt)
                           * symbolSize));
    }
    totalSize += ((size_t(symbolCntTable[p]) - symbolCnt) * symbolSize);
  }
  // add the cost of any symbols that could not be encoded
  totalSize += (size_t(unencodedSymbolCostTable[nSymbolsUsed])
                - size_t(unencodedSymbolCostTable[p]));
  return totalSize;
}

size_t StatisticalCompressor::optimizeSlotBitsTable(bool disableZeroSlotSize)
{
  for (size_t i = 0; i < nSlots; i++)
    slotBitsTable[i] = size_t(disableZeroSlotSize);
  if (nSymbolsUsed <= (nSlots * (size_t(disableZeroSlotSize) + 1)))
    return calculateEncodedSize();
  size_t  nSlotsD2 = (nSlots + 1) >> 1;
  std::vector< uint8_t >  slotBitsBuffer(nSlotsD2 * nSymbolsUsed, 0x00);
  std::vector< uint32_t > encodedSizeBuffer(nSymbolsUsed + 1);
  std::vector< uint8_t >  minSlotSizeTable(nSymbolsUsed, 0x00);
  for (size_t i = 0; i <= nSymbolsUsed; i++) {
    encodedSizeBuffer[i] = uint32_t(unencodedSymbolCostTable[nSymbolsUsed]
                                    - unencodedSymbolCostTable[i]);
  }
  for (size_t i = 0; i < nSymbolsUsed; i++) {
    size_t  j = size_t(disableZeroSlotSize);
    size_t  nxtSlotEnd = i + (size_t(2) << uint8_t(disableZeroSlotSize));
    while (j < 15 && nxtSlotEnd < nSymbolsUsed) {
      if (symbolCntTable[nxtSlotEnd] != symbolCntTable[i])
        break;
      nxtSlotEnd = nxtSlotEnd * 2 - i;
      j++;
    }
    minSlotSizeTable[i] = uint8_t(j);
  }
  for (size_t slotNum = (nSlots < nSymbolsUsed ? nSlots : nSymbolsUsed);
       slotNum-- > 0; ) {
    size_t  maxSlotSize = 0;
    while ((size_t(1) << maxSlotSize) < nSymbolsUsed && maxSlotSize < 15)
      maxSlotSize++;
    size_t  maxPos = nSymbolsUsed - ((size_t(1) << maxSlotSize) >> 1);
    while (slotNum >= maxPos) {
      maxSlotSize--;
      maxPos = nSymbolsUsed - ((nSymbolsUsed - maxPos) >> 1);
    }
    size_t  endPos = (slotNum > 0 ? nSymbolsUsed : 1);
    for (size_t i = slotNum; true; i++) {
      if (i >= maxPos) {
        if (i >= endPos)
          break;
        maxSlotSize--;
        maxPos = nSymbolsUsed - ((nSymbolsUsed - maxPos) >> 1);
      }
      {
        size_t  bitCnt = (i & 0x55555555) + ((i >> 1) & 0x55555555);
        bitCnt = (bitCnt & 0x33333333) + ((bitCnt >> 2) & 0x33333333);
        bitCnt = (bitCnt & 0x07070707) + ((bitCnt >> 4) & 0x07070707);
        bitCnt = bitCnt + (bitCnt >> 8);
        bitCnt = (bitCnt + (bitCnt >> 16)) & 0xFF;
        if (bitCnt > slotNum)
          continue;
      }
      size_t  baseSymbolCnt = size_t(symbolCntTable[i]);
      size_t  slotEnd = i + (size_t(1) << minSlotSizeTable[i]);
      size_t  maxSymbolSize = slotPrefixSizeTable[slotNum] + maxSlotSize;
      size_t  bestSize = 0x7FFFFFFF;
      size_t  bestSlotSize = 0;
      size_t  nBits;
      switch (maxSlotSize - size_t(minSlotSizeTable[i])) {
      case 15:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 15))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 15 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 14:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 14))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 14 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 13:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 13))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 13 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 12:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 12))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 12 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 11:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 11))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 11 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 10:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 10))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 10 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 9:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 9))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 9 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 8:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 8))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 8 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 7:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 7))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 7 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 6:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 6))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 6 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 5:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 5))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 5 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 4:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 4))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 4 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 3:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 3))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 3 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 2:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 2))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 2 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      case 1:
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * (maxSymbolSize - 1))
                + size_t(encodedSizeBuffer[slotEnd]);
        slotEnd = slotEnd * 2 - i;
        bestSlotSize = (nBits < bestSize ? 1 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      default:
        slotEnd = (slotEnd < nSymbolsUsed ? slotEnd : nSymbolsUsed);
        nBits = ((size_t(symbolCntTable[slotEnd]) - baseSymbolCnt)
                 * maxSymbolSize)
                + size_t(encodedSizeBuffer[slotEnd]);
        bestSlotSize = (nBits < bestSize ? 0 : bestSlotSize);
        bestSize = (nBits < bestSize ? nBits : bestSize);
      }
      slotBitsBuffer[(slotNum >> 1) * nSymbolsUsed + i] |=
          uint8_t((maxSlotSize - bestSlotSize) << ((slotNum & 1) << 2));
      encodedSizeBuffer[i] = uint32_t(bestSize);
    }
  }
  size_t  slotBegin = 0;
  for (size_t i = 0; i < nSlots; i++) {
    slotBitsTable[i] =
        size_t((slotBitsBuffer[(i >> 1) * nSymbolsUsed + slotBegin]
               >> ((i & 1) << 2)) & 0x0F);
    slotBegin += (size_t(1) << slotBitsTable[i]);
    if (slotBegin >= nSymbolsUsed)
      break;
  }
  for (size_t i = nSlots; i-- > 0; ) {
    while (true) {
      if (slotBitsTable[i] <= size_t(disableZeroSlotSize))
        break;
      slotBitsTable[i] = slotBitsTable[i] - 1;
      if (calculateEncodedSize() != size_t(encodedSizeBuffer[0])) {
        slotBitsTable[i] = slotBitsTable[i] + 1;
        break;
      }
    }
  }
  return size_t(encodedSizeBuffer[0]);
}

void StatisticalCompressor::updateTables(bool disableZeroSlotSize)
{
  try {
    size_t  totalSymbolCnt = 0;
    size_t  totalUnencodedSymbolCost = 0;
    for (size_t i = 0; i < nSymbolsUsed; i++) {
      size_t  tmp = size_t(symbolCntTable[i]);
      symbolCntTable[i] = (unsigned int) totalSymbolCnt;
      totalSymbolCnt += tmp;
      tmp = size_t(unencodedSymbolCostTable[i]);
      unencodedSymbolCostTable[i] = (unsigned int) totalUnencodedSymbolCost;
      totalUnencodedSymbolCost += tmp;
    }
    symbolCntTable[nSymbolsUsed] = (unsigned int) totalSymbolCnt;
    unencodedSymbolCostTable[nSymbolsUsed] =
        (unsigned int) totalUnencodedSymbolCost;
    size_t  bestSize = 0x7FFFFFFF;
    size_t  bestPrefixSize = minPrefixSize;
    std::vector< size_t > bestSlotBitsTable;
    for (size_t prefixSize = minPrefixSize;
         prefixSize <= maxPrefixSize;
         prefixSize++) {
      if (maxPrefixSize > minPrefixSize)
        setPrefixSize(prefixSize);
      size_t  encodedSize = optimizeSlotBitsTable(disableZeroSlotSize);
      if (maxPrefixSize > minPrefixSize) {
        encodedSize += (nSlots * 8);
        encodedSize += (prefixOnlySymbolCnt * prefixSize);
      }
      if (encodedSize < bestSize) {
        bestSize = encodedSize;
        bestPrefixSize = prefixSize;
        bestSlotBitsTable.resize(nSlots);
        for (size_t i = 0; i < nSlots; i++)
          bestSlotBitsTable[i] = slotBitsTable[i];
      }
    }
    if (maxPrefixSize > minPrefixSize)
      setPrefixSize(bestPrefixSize);
    for (size_t i = 0; i < nSlots; i++) {
      slotBitsTable[i] = bestSlotBitsTable[i];
      if (slotBitsTable[i] < size_t(disableZeroSlotSize)) {
        throw Exception("StatisticalCompressor::updateTables(): "
                        "internal error: zero slot size");
      }
    }
    unsigned int  baseSymbol = 0U;
    for (size_t i = 0; i < nSlots; i++) {
      slotBaseSymbolTable[i] = baseSymbol;
      unsigned int  prvBaseSymbol = baseSymbol;
      baseSymbol = prvBaseSymbol + (1U << (unsigned int) slotBitsTable[i]);
      if (baseSymbol > nSymbols)
        baseSymbol = (unsigned int) nSymbols;
      size_t  symbolSize = slotPrefixSizeTable[i] + slotBitsTable[i];
      for (unsigned int j = prvBaseSymbol; j < baseSymbol; j++) {
        symbolSlotNumTable[j] = (unsigned short) i;
        symbolSizeTable[j] = (unsigned short) symbolSize;
      }
    }
    for (size_t i = 0; i <= nSymbolsUsed; i++) {
      symbolCntTable[i] = 0U;
      unencodedSymbolCostTable[i] = 0U;
    }
    nSymbolsUsed = 0;
    nSymbolsEncoded = baseSymbol;
    unusedSymbolSize = 8192;
    prefixOnlySymbolCnt = 0;
  }
  catch (...) {
    this->clear();
    throw;
  }
}

void StatisticalCompressor::clear()
{
  for (size_t i = 0; i < nSlots; i++) {
    slotBitsTable[i] = 0;
    slotBaseSymbolTable[i] = 0U;
  }
  for (size_t i = 0; true; i++) {
    symbolCntTable[i] = 0U;
    unencodedSymbolCostTable[i] = 0U;
    if (i >= nSymbolsUsed && i >= nSymbolsEncoded)
      break;
    symbolSlotNumTable[i] = 0;
    symbolSizeTable[i] = 1;
  }
  nSymbolsUsed = 0;
  nSymbolsEncoded = 0;
  unusedSymbolSize = 1;
  prefixOnlySymbolCnt = 0;
}

// ----------------------------------------------------------------------------

static void findRLELengths(std::vector< unsigned int >& rleLengths,
                           const std::vector< uint8_t >& inBuf)
{
  unsigned int  rleLength = 0U;
  uint8_t       rleByte = 0x00;
  for (size_t i = inBuf.size(); i-- > 0; ) {
    if (inBuf[i] != rleByte) {
      rleLength = 0U;
      rleByte = inBuf[i];
    }
    if (rleLength < 256U)
      rleLength++;
    rleLengths[i] = rleLength;
  }
}

static uint8_t tomCompressData(std::vector< uint8_t >& outBuf,
                               const std::vector< uint8_t >& inBuf,
                               const uint8_t *symbolSizes = (uint8_t *) 0)
{
  if (inBuf.size() < 1)
    throw Exception("tomCompressData(): empty input buffer");
  std::vector< unsigned int > rleLengths(inBuf.size());
  findRLELengths(rleLengths, inBuf);
  // find the best flag byte
  uint8_t flagByte = 0x00;
  {
    std::vector< size_t > flagByteCosts(256, 0);
    for (size_t i = 0; i < inBuf.size(); ) {
      size_t  len = size_t(rleLengths[i]);
      uint8_t c = inBuf[i];
      size_t  literalSize = 8;
      size_t  rleSize = 16;
      if (symbolSizes) {
        literalSize = symbolSizes[c];
        rleSize = size_t(symbolSizes[c]) + size_t(symbolSizes[len & 0xFF]);
      }
      rleSize += literalSize;
      literalSize *= len;
      if (rleSize > literalSize)
        flagByteCosts[c] = flagByteCosts[c] + (rleSize - literalSize);
      i += len;
    }
    size_t  bestFlagByteCost = 0x7FFFFFFF;
    for (size_t i = 0; i < 256; i++) {
      if (flagByteCosts[i] < bestFlagByteCost ||
          (flagByteCosts[i] == bestFlagByteCost &&
           (symbolSizes != (uint8_t *) 0 &&
            symbolSizes[i] < symbolSizes[flagByte]))) {
        flagByte = uint8_t(i);
        bestFlagByteCost = flagByteCosts[i];
      }
    }
  }
  // compress data
  if (symbolSizes) {
    std::vector< size_t > bitCountTable(256, 0);
    for (size_t i = inBuf.size(); i-- > 0; ) {
      size_t  bestSize = 0x7FFFFFFF;
      size_t  bestLen = rleLengths[i];
      uint8_t c = inBuf[i];
      size_t  nBitsBase =
          size_t(symbolSizes[flagByte]) + size_t(symbolSizes[c]);
      for (size_t j = rleLengths[i]; j >= 1; j--) {
        size_t  nBits = nBitsBase + size_t(symbolSizes[j & 0xFF]);
        nBits += bitCountTable[(i + j) & 0xFF];
        if (nBits < bestSize) {
          bestSize = nBits;
          bestLen = j;
        }
        break;                  // TODO: improve optimization
      }
      size_t  nBits = size_t(symbolSizes[c]) + bitCountTable[(i + 1) & 0xFF];
      if ((nBits < bestSize ||
           (nBits == bestSize &&
            (symbolSizes[flagByte] > symbolSizes[c] ||
             symbolSizes[bestLen & 0xFF] > symbolSizes[c]))) &&
          c != flagByte) {
        rleLengths[i] = 1U;
        bitCountTable[i & 0xFF] = nBits;
      }
      else {
        rleLengths[i] = (unsigned int) bestLen;
        bitCountTable[i & 0xFF] = bestSize;
      }
    }
  }
  else {
    for (size_t i = 0; i < inBuf.size(); i++) {
      if (rleLengths[i] <= 3U && inBuf[i] != flagByte)
        rleLengths[i] = 1U;
    }
  }
  for (size_t i = 0; i < inBuf.size(); i++) {
    size_t  len = size_t(rleLengths[i]);
    uint8_t c = inBuf[i];
    if (len > 1 || c == flagByte) {
      outBuf.push_back(flagByte);
      outBuf.push_back(uint8_t(len & 0xFF));
      i = i + (len - 1);
    }
    outBuf.push_back(c);
  }
  return flagByte;
}

// ============================================================================

class DTFCompressor {
 private:
  std::vector< uint8_t >  symbolSizeTable;
  // ----------------
  static void statSortFunc(std::vector< size_t >& symbolCntTable,
                           std::vector< uint8_t >& symbolValueTable,
                           std::vector< size_t >& tmpBuf1,
                           std::vector< uint8_t >& tmpBuf2,
                           size_t startPos, size_t endPos);
  static void statisticalCompressData(std::vector< uint8_t >& outBuf,
                                      const std::vector< uint8_t >& inBuf,
                                      int minPrefixSize = 2,
                                      int maxPrefixSize = 2,
                                      bool disableCompression = false,
                                      uint8_t *symbolSizes = (uint8_t *) 0);
 public:
  DTFCompressor();
  virtual ~DTFCompressor();
 private:
  void lzCompressBlock(std::vector< uint8_t >& outBuf,
                       const std::vector< uint8_t >& inBuf,
                       const DTFCompressionParameters& cfg,
                       size_t offs, size_t nBytes);
 public:
  void compressDataBlock(std::vector< uint8_t >& outBuf,
                         const std::vector< uint8_t >& inBuf,
                         const DTFCompressionParameters& cfg,
                         bool decompPrgInput = false,
                         const char *fileName = (char *) 0);
  void compressData(std::vector< uint8_t >& outBuf,
                    const std::vector< uint8_t >& inBuf,
                    const DTFCompressionParameters& cfg);
};

void DTFCompressor::statSortFunc(std::vector< size_t >& symbolCntTable,
                                 std::vector< uint8_t >& symbolValueTable,
                                 std::vector< size_t >& tmpBuf1,
                                 std::vector< uint8_t >& tmpBuf2,
                                 size_t startPos, size_t endPos)
{
  if ((startPos + 1) >= endPos)
    return;
  size_t  midPos = (startPos + endPos) / 2;
  if ((startPos + 2) < endPos) {
    statSortFunc(symbolCntTable, symbolValueTable, tmpBuf1, tmpBuf2,
                 startPos, midPos);
    statSortFunc(symbolCntTable, symbolValueTable, tmpBuf1, tmpBuf2,
                 midPos, endPos);
  }
  size_t  i = startPos;
  size_t  j = midPos;
  for (size_t k = 0; k < (endPos - startPos); k++) {
    size_t  p = 0;
    if (i >= midPos)
      p = j++;
    else if (j >= endPos)
      p = i++;
    else if (symbolCntTable[j] > symbolCntTable[i])
      p = j++;
    else
      p = i++;
    tmpBuf1[k] = symbolCntTable[p];
    tmpBuf2[k] = symbolValueTable[p];
  }
  for (size_t k = startPos; k < endPos; k++) {
    symbolCntTable[k] = tmpBuf1[k - startPos];
    symbolValueTable[k] = tmpBuf2[k - startPos];
  }
}

void DTFCompressor::statisticalCompressData(
    std::vector< uint8_t >& outBuf, const std::vector< uint8_t >& inBuf,
    int minPrefixSize, int maxPrefixSize,
    bool disableCompression, uint8_t *symbolSizes)
{
  if (inBuf.size() < 1) {
    throw Exception("DTFCompressor::statisticalCompressData(): "
                    "empty input buffer");
  }
  if (disableCompression) {
    // compression is disabled: copy uncompressed data
    outBuf.push_back(0x88);
    outBuf.insert(outBuf.end(), inBuf.begin(), inBuf.end());
    if (symbolSizes) {
      for (size_t i = 0; i < 256; i++)
        symbolSizes[i] = 8;
    }
    return;
  }
  // count the number of occurences of each symbol
  std::vector< size_t >   symbolCntTable(256, 0);
  std::vector< uint8_t >  symbolValueTable(256);
  for (size_t i = 0; i < 256; i++)
    symbolValueTable[i] = uint8_t(i);
  for (size_t i = 0; i < inBuf.size(); i++) {
    size_t  c = inBuf[i];
    symbolCntTable[c] = symbolCntTable[c] + 1;
  }
  // sort symbols by frequency in descending order
  {
    std::vector< size_t >   tmpBuf1(256);
    std::vector< uint8_t >  tmpBuf2(256);
    statSortFunc(symbolCntTable, symbolValueTable, tmpBuf1, tmpBuf2, 0, 256);
  }
  // find optimal encoding
  StatisticalCompressor statisticalCompressor(
      size_t(1 << minPrefixSize), 256,
      (size_t *) 0, size_t(minPrefixSize), size_t(maxPrefixSize), (size_t *) 0);
  size_t  nSymbolsUsed = 256;
  for (size_t i = 0; i < 256; i++) {
    size_t  n = symbolCntTable[i];
    if (n < 1) {
      nSymbolsUsed = i;
      break;
    }
    size_t  unencodedCost = size_t(0x00100000UL) / n;
    for (size_t j = 0; j < n; j++)
      statisticalCompressor.addSymbol((unsigned int) i, unencodedCost);
  }
  statisticalCompressor.updateTables(minPrefixSize == 2 && maxPrefixSize == 2);
  if (!(minPrefixSize == 2 && maxPrefixSize == 2)) {
    // if the data size cannot be reduced, and DTF compatibility is not
    // required, then store data without statistical compression
    size_t  uncompressedSize = (inBuf.size() + 1) * 8;
    size_t  compressedSize =
        (statisticalCompressor.getSlotCnt() + 2 + nSymbolsUsed) * 8;
    for (size_t i = 0; i < nSymbolsUsed; i++) {
      compressedSize += (statisticalCompressor.getSymbolSize((unsigned int) i)
                         * symbolCntTable[i]);
    }
    if (compressedSize >= uncompressedSize) {
      statisticalCompressData(outBuf, inBuf, 0, 0, true, symbolSizes);
      return;
    }
  }
  // write decode tables
  std::vector< uint8_t >  symbolValueMap(256);
  for (size_t i = 0; i < 256; i++)
    symbolValueMap[symbolValueTable[i]] = uint8_t(i);
  size_t  prefixSize = statisticalCompressor.getSlotPrefixSize(0);
  size_t  nSlots = statisticalCompressor.getSlotCnt();
  for (size_t i = 0; i < nSlots; i++) {
    uint8_t c = uint8_t(statisticalCompressor.getSlotSize(i));
    if (i == 0 && (minPrefixSize != 2 || maxPrefixSize != 2))
      c |= uint8_t((prefixSize << 4) | 0x80);
    outBuf.push_back(c);
  }
  outBuf.push_back(uint8_t(nSymbolsUsed & 0xFF));
  outBuf.push_back(0x00);       // must be 0 for compatibility with ATTUS.LDR
  for (size_t i = 0; i < nSymbolsUsed; i++)
    outBuf.push_back(symbolValueTable[i]);
  // write encoded data
  unsigned int  shiftRegister = 0U;
  uint8_t       shiftRegisterBitCnt = 0;
  for (size_t i = 0; i < inBuf.size(); i++) {
    unsigned int  c = symbolValueMap[inBuf[i]];
    shiftRegisterBitCnt += uint8_t(prefixSize);
    shiftRegister =
        (shiftRegister << uint8_t(prefixSize))
        | (unsigned int) statisticalCompressor.getSymbolSlotIndex(c);
    c = statisticalCompressor.encodeSymbol(c);
    uint8_t nBits = uint8_t((c >> 24) & 0x7FU);
    shiftRegisterBitCnt += nBits;
    shiftRegister = (shiftRegister << nBits) | (c & ((1U << nBits) - 1U));
    if (shiftRegisterBitCnt >= 8) {
      do {
        shiftRegisterBitCnt = shiftRegisterBitCnt - 8;
        outBuf.push_back(uint8_t((shiftRegister >> shiftRegisterBitCnt)
                                 & 0xFFU));
      } while (shiftRegisterBitCnt >= 8);
      shiftRegister = shiftRegister & ((1U << shiftRegisterBitCnt) - 1U);
    }
  }
  if (shiftRegisterBitCnt > 0) {
    outBuf.push_back(uint8_t((shiftRegister << (8 - shiftRegisterBitCnt))
                             & 0xFFU));
  }
  // update symbol size statistics
  if (symbolSizes) {
    for (unsigned int i = 0U; i < 256U; i++) {
      size_t  symbolSize = statisticalCompressor.getSymbolSize(i);
      if (symbolSize > (prefixSize + 8))
        symbolSize = prefixSize + 9;    // default size for unencoded symbols
      symbolSizes[symbolValueTable[i]] = uint8_t(symbolSize);
    }
  }
}

DTFCompressor::DTFCompressor()
  : symbolSizeTable(256, 0x08)
{
}

DTFCompressor::~DTFCompressor()
{
}

void DTFCompressor::lzCompressBlock(std::vector< uint8_t >& outBuf,
                                    const std::vector< uint8_t >& inBuf,
                                    const DTFCompressionParameters& cfg,
                                    size_t offs, size_t nBytes)
{
  if (offs >= inBuf.size() || nBytes < 1)
    return;
  if ((offs + nBytes) > inBuf.size())
    nBytes = inBuf.size() - offs;
  std::vector< unsigned char >  tmpBuf;
  std::vector< unsigned char >  tmpBuf2;
  Ep128Compress::Compressor     *compressor = (Ep128Compress::Compressor *) 0;
  if (cfg.compressionType == 2)
    compressor = new Ep128Compress::Compressor_M2(tmpBuf2);
  else if (cfg.compressionType == 3)
    compressor = new Ep128Compress::Compressor_M0(tmpBuf2);
  else
    compressor = new Ep128Compress::Compressor_M3(tmpBuf2);
  try {
    for (size_t i = 0; i < nBytes; i++)
      tmpBuf.push_back(inBuf[offs + i]);
    compressor->setCompressionLevel(cfg.compressionLevel);
    compressor->compressData(tmpBuf, 0xFFFFFFFFU, true, true);
    if (cfg.compressionType == 2 || cfg.compressionType == 3) {
      outBuf.push_back(uint8_t(tmpBuf2.size() & 0xFF));
      outBuf.push_back(uint8_t(tmpBuf2.size() >> 8));
    }
    for (size_t i = 0; i < tmpBuf2.size(); i++)
      outBuf.push_back(tmpBuf2[i]);
  }
  catch (...) {
    delete compressor;
    throw;
  }
  delete compressor;
}

void DTFCompressor::compressDataBlock(std::vector< uint8_t >& outBuf,
                                      const std::vector< uint8_t >& inBuf,
                                      const DTFCompressionParameters& cfg,
                                      bool decompPrgInput, const char *fileName)
{
  if ((fileName != (char *) 0 && fileName[0] != '\0') || decompPrgInput) {
    const char  *s = (char *) 0;
    if (!decompPrgInput) {
      // parse file name for block size list
      size_t  nameLen = std::strlen(fileName);
      for (size_t i = 0; (i + 3) <= nameLen; i++) {
        if (fileName[i] == ':' && fileName[i + 1] == ':' &&
            fileName[i + 2] >= '0' && fileName[i + 2] <= '9') {
          s = fileName + (i + 2);
          break;
        }
      }
    }
    if (s != (char *) 0 || decompPrgInput) {
      size_t  inBufPos = 0;
      while (inBufPos < inBuf.size()) {
        size_t  nBytes = inBuf.size() - inBufPos;
        if (decompPrgInput) {
          if (nBytes < 3)
            throw Exception("error in decompressed DTF program data");
          size_t  tmp =
              size_t(inBuf[inBufPos]) | (size_t(inBuf[inBufPos + 1]) << 8);
          if (tmp < 1 || tmp > (nBytes - 2))
            throw Exception("error in decompressed DTF program data");
          nBytes = tmp;
          inBufPos = inBufPos + 2;
        }
        else if (s != (char *) 0 && s[0] != '\0') {
          char    *endPtr = (char *) 0;
          nBytes = size_t(std::strtol(s, &endPtr, 0));
          if (endPtr == (char *) 0 || endPtr == s ||
              (endPtr[0] != '\0' && endPtr[0] != ',')) {
            throw Exception("syntax error in block size list");
          }
          if (nBytes < 1)
            throw Exception("invalid block size");
          s = endPtr;
          if (s[0] == ',')
            s++;
          if (nBytes > (inBuf.size() - inBufPos))
            nBytes = inBuf.size() - inBufPos;
        }
        else {
          break;
        }
        if (nBytes > 0) {
          std::vector< uint8_t >  tmpBuf;
          tmpBuf.resize(nBytes);
          for (size_t i = 0; i < nBytes; i++)
            tmpBuf[i] = inBuf[inBufPos + i];
          if (decompPrgInput && cfg.compressionType == 0 && inBufPos == 2) {
            if (nBytes > 0x3F00)
              throw Exception("invalid DTF loader code size");
            outBuf.resize(3);
            uint8_t rleFlagByte = tomCompressData(outBuf, tmpBuf);
            outBuf[2] = rleFlagByte;
            outBuf[0] = uint8_t((outBuf.size() - 2) & 0xFF);
            outBuf[1] = uint8_t((outBuf.size() - 2) >> 8);
          }
          else {
            compressDataBlock(outBuf, tmpBuf, cfg, false, (char *) 0);
          }
          inBufPos = inBufPos + nBytes;
        }
      }
      return;
    }
  }
  if (cfg.compressionType > 0) {
    lzCompressBlock(outBuf, inBuf, cfg, 0, inBuf.size());
    return;
  }
  if (inBuf.size() < 1)
    return;
  if (inBuf.size() > 65535) {
    throw Exception("DTFCompressor::compressDataBlock(): "
                    "uncompressed size > 65535 bytes");
  }
  for (size_t i = 0; i < 256; i++)
    symbolSizeTable[i] = 8;
  std::vector< uint64_t > hashTable;
  std::vector< uint8_t >  bestBuf;
  std::vector< uint8_t >  tmpBuf1;
  std::vector< uint8_t >  tmpBuf2;
  const uint8_t *symbolSizes = (uint8_t *) 0;
  size_t  bestSize = 0x7FFFFFFF;
  uint8_t flagByte = 0x00;
  for (size_t i = 40; i > 0; i--) {
    tmpBuf1.resize(0);
    tmpBuf2.resize(0);
    uint8_t tmpFlagByte = tomCompressData(tmpBuf1, inBuf, symbolSizes);
    statisticalCompressData(tmpBuf2, tmpBuf1,
                            cfg.minPrefixSize, cfg.maxPrefixSize,
                            cfg.disableStatisticalCompression,
                            &(symbolSizeTable.front()));
    symbolSizes = &(symbolSizeTable.front());
    if (tmpBuf2.size() < bestSize) {
      bestSize = tmpBuf2.size();
      flagByte = tmpFlagByte;
      bestBuf.resize(tmpBuf2.size());
      for (size_t j = 0; j < tmpBuf2.size(); j++)
        bestBuf[j] = tmpBuf2[j];
    }
    if (cfg.disableStatisticalCompression)
      break;
    uint64_t  h = 1UL;
    for (size_t j = 0; j < tmpBuf2.size(); j++) {
      h = h ^ uint64_t(tmpBuf2[j]);
      h = uint32_t(h) * uint64_t(0xC2B0C3CCUL);
      h = (h ^ (h >> 32)) & 0xFFFFFFFFUL;
    }
    h = h | (uint64_t(tmpBuf2.size()) << 32);
    for (size_t j = 0; j < hashTable.size(); j++) {
      if (hashTable[j] == h) {
        i = 1;
        break;
      }
    }
    hashTable.push_back(h);
  }
  // calculate compressed data size without header and decode tables
  size_t  compressedSize = bestBuf.size();
  if (bestBuf[0] != 0x88) {
    size_t  nSlots = 4;
    if (bestBuf[0] >= 0x80)
      nSlots = size_t(1) << ((bestBuf[0] & 0x70) >> 4);
    compressedSize = compressedSize - (nSlots + 2);
    size_t  nSymbolsUsed =
        size_t(bestBuf[nSlots]) | (size_t(bestBuf[nSlots + 1]) << 8);
    if (nSymbolsUsed == 0)
      nSymbolsUsed = 256;
    compressedSize = compressedSize - nSymbolsUsed;
  }
  else {
    compressedSize--;
  }
  if (compressedSize > 65535) {
    throw Exception("DTFCompressor::compressDataBlock(): "
                    "compressed size > 65535 bytes");
  }
  // write DTF data block header
  outBuf.push_back(uint8_t(compressedSize & 0xFF));
  outBuf.push_back(uint8_t(compressedSize >> 8));
  outBuf.push_back(uint8_t(inBuf.size() & 0xFF));
  outBuf.push_back(uint8_t(inBuf.size() >> 8));
  outBuf.push_back(flagByte);
  // append compressed data to output buffer
  outBuf.insert(outBuf.end(), bestBuf.begin(), bestBuf.end());
}

struct SplitOptimizationBlock {
  size_t  startPos;
  size_t  nBytes;
};

void DTFCompressor::compressData(std::vector< uint8_t >& outBuf,
                                 const std::vector< uint8_t >& inBuf,
                                 const DTFCompressionParameters& cfg)
{
  if (cfg.compressionType > 0) {
    // use a fixed block size of 32768 bytes
    for (size_t i = 0; i < inBuf.size(); i += 32768) {
      outBuf.push_back(uint8_t((i + 32768) >= inBuf.size() ? 0xE4 : 0x00));
      lzCompressBlock(outBuf, inBuf, cfg, i, 32768);
    }
    return;
  }
  if (inBuf.size() < 1)
    return;
  // split large files to improve statistical compression
  std::list< SplitOptimizationBlock > splitPositions;
  if (cfg.blockSize > 0) {
    for (size_t i = 0; i < inBuf.size(); ) {
      SplitOptimizationBlock  tmp;
      tmp.startPos = i;
      tmp.nBytes = (cfg.blockSize < 0xFC00 ? cfg.blockSize : 0xFC00);
      if ((i + tmp.nBytes) > inBuf.size())
        tmp.nBytes = inBuf.size() - i;
      i += tmp.nBytes;
      splitPositions.push_back(tmp);
    }
  }
  else {
    std::map< uint64_t, size_t >  splitOptimizationCache;
    // create initial block list
    for (size_t i = 0; i < inBuf.size(); ) {
      SplitOptimizationBlock  tmp;
      tmp.startPos = i;
      tmp.nBytes = 256;
      if ((i + tmp.nBytes) > inBuf.size())
        tmp.nBytes = inBuf.size() - i;
      i += tmp.nBytes;
      splitPositions.push_back(tmp);
    }
    while (true) {
      size_t  bestMergePos = 0;
      long    bestMergeBytes = 0x7FFFFFFFL;
      // find the pair of blocks that reduce the total compressed size
      // the most when merged
      std::list< SplitOptimizationBlock >::iterator curBlock =
          splitPositions.begin();
      while (curBlock != splitPositions.end()) {
        std::list< SplitOptimizationBlock >::iterator nxtBlock = curBlock;
        nxtBlock++;
        if (nxtBlock == splitPositions.end())
          break;
        if (((*curBlock).nBytes + (*nxtBlock).nBytes) > 0xFC00) {
          curBlock++;
          continue;                     // limit block size to <= 63K
        }
        size_t  nBytesSplit = 0;
        size_t  nBytesMerged = 0;
        for (size_t i = 0; i < 3; i++) {
          // i = 0: merged block, i = 1: first block, i = 2: second block
          size_t  startPos = 0;
          size_t  endPos = 0;
          switch (i) {
          case 0:
            startPos = (*curBlock).startPos;
            endPos = startPos + (*curBlock).nBytes + (*nxtBlock).nBytes;
            break;
          case 1:
            startPos = (*curBlock).startPos;
            endPos = startPos + (*curBlock).nBytes;
            break;
          case 2:
            startPos = (*nxtBlock).startPos;
            endPos = startPos + (*nxtBlock).nBytes;
            break;
          }
          uint64_t  cacheKey = (uint64_t(startPos) << 32) | uint64_t(endPos);
          if (splitOptimizationCache.find(cacheKey)
              == splitOptimizationCache.end()) {
            // if this block is not in the cache yet, compress it,
            // and store the compressed size in the cache
            std::vector< uint8_t >  tmpOutBuf;
            std::vector< uint8_t >  tmpBuf;
            tmpBuf.insert(tmpBuf.end(),
                          inBuf.begin() + startPos, inBuf.begin() + endPos);
            compressDataBlock(tmpOutBuf, tmpBuf, cfg);
            // calculate compressed size
            splitOptimizationCache[cacheKey] = tmpOutBuf.size() + 1;
          }
          size_t  nBytes = splitOptimizationCache[cacheKey];
          switch (i) {
          case 0:
            nBytesMerged = nBytes;
            break;
          default:
            nBytesSplit += nBytes;
            break;
          }
        }
        // calculate size change when merging blocks
        long    sizeDiff = long(nBytesMerged) - long(nBytesSplit);
        if (sizeDiff < bestMergeBytes) {
          bestMergePos = (*curBlock).startPos;
          bestMergeBytes = sizeDiff;
        }
        curBlock++;
      }
      if (bestMergeBytes > 0L)          // no more blocks can be merged
        break;
      // merge the best pair of blocks and continue
      curBlock = splitPositions.begin();
      while ((*curBlock).startPos != bestMergePos)
        curBlock++;
      std::list< SplitOptimizationBlock >::iterator nxtBlock = curBlock;
      nxtBlock++;
      (*curBlock).nBytes = (*curBlock).nBytes + (*nxtBlock).nBytes;
      splitPositions.erase(nxtBlock);
    }
  }
  // compress all blocks again:
  std::list< SplitOptimizationBlock >::iterator i_ = splitPositions.begin();
  while (i_ != splitPositions.end()) {
    std::vector< uint8_t >  tmpBuf;
    tmpBuf.insert(tmpBuf.end(),
                  inBuf.begin() + (*i_).startPos,
                  inBuf.begin() + ((*i_).startPos + (*i_).nBytes));
    if (((*i_).startPos + (*i_).nBytes) < inBuf.size())
      outBuf.push_back(0x00);
    else
      outBuf.push_back(0xE4);
    compressDataBlock(outBuf, tmpBuf, cfg);
    i_++;
  }
}

// ----------------------------------------------------------------------------

class DTFDecompressor {
 private:
  const std::vector< uint8_t >& inBuf;
  size_t  readPos;
  size_t  endPos;
  unsigned long shiftRegister;
  // --------
  uint8_t readByte();
  uint8_t readBits(size_t n);
 public:
  DTFDecompressor(const std::vector< uint8_t >& inBuf_);
  virtual ~DTFDecompressor();
  void decompressTOMDataBlock(std::vector< uint8_t >& outBuf,
                              uint8_t compressionType = 0);
  void decompressDataBlock(std::vector< uint8_t >& outBuf,
                           uint8_t compressionType = 0);
  void setReadPosition(long n);
  size_t getReadPosition() const;
};

uint8_t DTFDecompressor::readByte()
{
  if (readPos >= inBuf.size()) {
    throw Exception("DTFDecompressor::readByte(): "
                    "unexpected end of input data");
  }
  if (readPos >= endPos) {
    throw Exception("DTFDecompressor::readByte(): "
                    "unexpected end of compressed data block");
  }
  uint8_t c = inBuf[readPos];
  readPos++;
  return c;
}

uint8_t DTFDecompressor::readBits(size_t n)
{
  uint8_t c = 0;
  while (n-- > 0) {
    if (shiftRegister > 0xFFFFUL)
      shiftRegister = (unsigned long) readByte() | 0x0100UL;
    shiftRegister = shiftRegister << 1;
    c = (c << 1) | uint8_t(bool(shiftRegister & 0x0100UL));
  }
  return c;
}

DTFDecompressor::DTFDecompressor(const std::vector< uint8_t >& inBuf_)
  : inBuf(inBuf_)
{
}

DTFDecompressor::~DTFDecompressor()
{
}

void DTFDecompressor::decompressTOMDataBlock(std::vector< uint8_t >& outBuf,
                                             uint8_t compressionType)
{
  if (compressionType > 0) {
    decompressDataBlock(outBuf, compressionType);
    return;
  }
  endPos = inBuf.size();
  size_t  compressedSize = readByte();
  compressedSize = compressedSize | (size_t(readByte()) << 8);
  if (compressedSize < 1) {
    throw Exception("DTFDecompressor::decompressTOMDataBlock(): "
                    "invalid compressed data size");
  }
  endPos = readPos + compressedSize;
  uint8_t flagByte = readByte();
  while (readPos < endPos) {
    uint8_t c = readByte();
    if (c == flagByte) {
      uint8_t len = readByte();
      c = readByte();
      do {
        outBuf.push_back(c);
        len = (len - 1) & 0xFF;
      } while (len != 0x00);
    }
    else {
      outBuf.push_back(c);
    }
  }
}

void DTFDecompressor::decompressDataBlock(std::vector< uint8_t >& outBuf,
                                          uint8_t compressionType)
{
  endPos = inBuf.size();
  if (compressionType > 0) {
    std::vector< unsigned char >  tmpBuf;
    std::vector< unsigned char >  tmpBuf2;
    if (compressionType == 2 || compressionType == 3) {
      size_t  compressedSize = size_t(readByte());
      compressedSize = compressedSize | (size_t(readByte()) << 8);
      for (size_t i = 0; i < compressedSize; i++)
        tmpBuf.push_back(readByte());
      Ep128Compress::decompressData(tmpBuf2, tmpBuf,
                                    (compressionType == 3 ? 0 : 2));
    }
    else {
      tmpBuf.push_back(readByte());
      tmpBuf.push_back(readByte());
      size_t  compressedSize = size_t(tmpBuf[0]) | (size_t(tmpBuf[1]) << 8);
      for (size_t i = 0; i < (compressedSize + 2); i++)
        tmpBuf.push_back(readByte());
      Ep128Compress::Decompressor_M3  decompressor;
      decompressor.decompressData(tmpBuf2, tmpBuf);
    }
    for (size_t i = 0; i < tmpBuf2.size(); i++)
      outBuf.push_back(tmpBuf2[i]);
    return;
  }
  size_t  compressedSize = readByte();
  compressedSize = compressedSize | (size_t(readByte()) << 8);
  if (compressedSize < 1) {
    throw Exception("DTFDecompressor::decompressDataBlock(): "
                    "invalid compressed data size");
  }
  size_t  uncompressedSize = readByte();
  uncompressedSize = uncompressedSize | (size_t(readByte()) << 8);
  if (uncompressedSize < 1) {
    throw Exception("DTFDecompressor::decompressDataBlock(): "
                    "invalid uncompressed data size");
  }
  std::vector< uint8_t >  slotBitsTable(1, 0x08);
  std::vector< uint16_t > slotBaseTable(1, 0x0000);
  std::vector< uint8_t >  symbolValueTable(256);
  size_t  nSymbolsUsed = 256;
  for (size_t i = 0; i < 256; i++)
    symbolValueTable[i] = uint8_t(i);
  uint8_t flagByte = readByte();
  uint8_t prefixSize = readByte();
  if (prefixSize != 0x88) {
    if (prefixSize >= 0x80)
      prefixSize = (prefixSize & 0x70) >> 4;
    else
      prefixSize = 2;
    if (prefixSize == 0) {
      throw Exception("DTFDecompressor::decompressDataBlock(): "
                      "error in compressed data");
    }
    readPos--;
    slotBitsTable.resize(size_t(1) << prefixSize);
    slotBaseTable.resize(size_t(1) << prefixSize, 0x0000);
    for (size_t i = 0; i < slotBitsTable.size(); i++) {
      uint8_t c = readByte();
      if (i == 0 && c >= 0x80)
        c = c & 0x0F;
      if (c > 0x08) {
        throw Exception("DTFDecompressor::decompressDataBlock(): "
                        "error in compressed data");
      }
      slotBitsTable[i] = c;
      if ((i + 1) < slotBitsTable.size())
        slotBaseTable[i + 1] = slotBaseTable[i] + uint16_t(1 << c);
    }
    nSymbolsUsed = readByte();
    nSymbolsUsed = nSymbolsUsed | (size_t(readByte()) << 8);
    if (nSymbolsUsed == 0)
      nSymbolsUsed = 256;
    if (nSymbolsUsed > 256) {
      throw Exception("DTFDecompressor::decompressDataBlock(): "
                      "error in compressed data");
    }
    for (size_t i = 0; i < nSymbolsUsed; i++)
      symbolValueTable[i] = readByte();
  }
  else {
    prefixSize = 0;             // no statistical compression
  }
  endPos = readPos + compressedSize;
  shiftRegister = 0x00010000UL;
  while (uncompressedSize > 0) {
    uint8_t   prefixValue = readBits(prefixSize);
    uint16_t  n = slotBaseTable[prefixValue]
                  + uint16_t(readBits(slotBitsTable[prefixValue]));
    if (size_t(n) >= nSymbolsUsed) {
      throw Exception("DTFDecompressor::decompressDataBlock(): "
                      "error in compressed data");
    }
    uint8_t   c = symbolValueTable[n];
    if (c == flagByte) {
      prefixValue = readBits(prefixSize);
      n = slotBaseTable[prefixValue]
          + uint16_t(readBits(slotBitsTable[prefixValue]));
      if (size_t(n) >= nSymbolsUsed) {
        throw Exception("DTFDecompressor::decompressDataBlock(): "
                        "error in compressed data");
      }
      uint8_t len = symbolValueTable[n];
      prefixValue = readBits(prefixSize);
      n = slotBaseTable[prefixValue]
          + uint16_t(readBits(slotBitsTable[prefixValue]));
      if (size_t(n) >= nSymbolsUsed) {
        throw Exception("DTFDecompressor::decompressDataBlock(): "
                        "error in compressed data");
      }
      c = symbolValueTable[n];
      do {
        if (uncompressedSize < 1) {
          throw Exception("DTFDecompressor::decompressDataBlock(): "
                          "error in compressed data");
        }
        uncompressedSize--;
        outBuf.push_back(c);
        len = (len - 1) & 0xFF;
      } while (len != 0x00);
    }
    else {
      uncompressedSize--;
      outBuf.push_back(c);
    }
  }
  if (readPos != endPos) {
    throw Exception("DTFDecompressor::decompressDataBlock(): "
                    "more compressed data than expected");
  }
}

void DTFDecompressor::setReadPosition(long n)
{
  if (n < 0L || size_t(n) > inBuf.size()) {
    throw Exception("DTFDecompressor::setReadPosition(): "
                    "position is out of range");
  }
  readPos = size_t(n);
}

size_t DTFDecompressor::getReadPosition() const
{
  return readPos;
}

// ----------------------------------------------------------------------------

static const uint8_t  prgLoaderCode[] = {
  0x00, 0x05, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xF3, 0x31, 0x00, 0x01, 0x0E, 0x40, 0xF7, 0x00,
  0x3E, 0xC3, 0x32, 0x28, 0x00, 0x21, 0x69, 0x00, 0x22, 0x29, 0x00, 0x21,
  0x35, 0x01, 0x11, 0x60, 0x00, 0xD5, 0x01, 0x2D, 0x00, 0xED, 0xB0, 0xFB,
  0x3E, 0x01, 0x11, 0x62, 0x01, 0xF7, 0x01, 0xB7, 0x20, 0x2C, 0x3E, 0xFD,
  0xD3, 0xB1, 0x87, 0xD3, 0xB2, 0x3C, 0xD3, 0xB3, 0xC9, 0xCD, 0x66, 0x00,
  0xC3, 0x00, 0x01, 0x11, 0x00, 0x01, 0xD5, 0x11, 0x74, 0x00, 0x01, 0x02,
  0x00, 0xCD, 0x77, 0x00, 0x01, 0x00, 0x00, 0xD1, 0x3E, 0x01, 0xF7, 0x06,
  0x6B, 0x62, 0xB7, 0xDB, 0xB2, 0xC8, 0xF3, 0x31, 0x00, 0x01, 0x0E, 0x60,
  0xF7, 0x00, 0x0E, 0x80, 0x18, 0xFA, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

static const uint8_t  dtfLoaderCode[] = {
  0x00, 0x05, 0x4B, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xF3, 0x31, 0x00, 0x38, 0x0E, 0x40, 0xF7, 0x00,
  0x3E, 0x01, 0x11, 0x4A, 0x03, 0xF7, 0x01, 0xB7, 0x20, 0x49, 0x3C, 0x01,
  0x02, 0x00, 0x11, 0x60, 0x00, 0xF7, 0x06, 0xB7, 0x20, 0x3D, 0xED, 0x4B,
  0x60, 0x00, 0x78, 0xFE, 0x40, 0x30, 0x34, 0xB1, 0x28, 0x31, 0xC5, 0x21,
  0x7D, 0x01, 0x11, 0x20, 0x00, 0x01, 0x10, 0x00, 0xED, 0xB0, 0x1E, 0x60,
  0x0E, 0x58, 0xED, 0xB0, 0x11, 0x00, 0xAF, 0x01, 0x65, 0x01, 0xED, 0xB0,
  0xC1, 0xED, 0x43, 0x06, 0xAF, 0x3E, 0xFD, 0xD3, 0xB1, 0x87, 0xD3, 0xB2,
  0x3C, 0x31, 0x00, 0x01, 0x11, 0x00, 0x01, 0xD5, 0xC3, 0x20, 0x00, 0x3E,
  0x01, 0xF7, 0x03, 0xF3, 0x21, 0x08, 0x00, 0x3E, 0x30, 0x74, 0x2C, 0x28,
  0x06, 0xBD, 0x20, 0xF9, 0x29, 0x18, 0xF6, 0xFB, 0x3E, 0xFF, 0xD3, 0xB2,
  0x3E, 0x01, 0xD3, 0xB3, 0x3E, 0x06, 0xC3, 0x0D, 0xC0, 0xD3, 0xB3, 0x18,
  0x42, 0xFF, 0xC3, 0x39, 0xB0, 0x18, 0x3F, 0x31, 0x00, 0x00, 0x62, 0xFB,
  0xC9, 0xCD, 0x66, 0x00, 0xC3, 0x00, 0x01, 0x3E, 0xFF, 0xFE, 0xAF, 0xF3,
  0x21, 0x20, 0x00, 0x01, 0xAF, 0x04, 0x0C, 0xED, 0xA2, 0x20, 0xFB, 0x0D,
  0xED, 0xA3, 0xED, 0x73, 0x2B, 0x00, 0x31, 0xDC, 0x00, 0xCD, 0x00, 0xAF,
  0xD3, 0xB2, 0x18, 0xA3, 0xAF, 0x18, 0x08, 0x8F, 0xD8, 0xCB, 0x21, 0xC2,
  0x8A, 0x00, 0x67, 0xD9, 0x2C, 0x28, 0x8F, 0x7E, 0xD9, 0xD3, 0x81, 0xD0,
  0x8F, 0x4F, 0x7C, 0x18, 0xEA, 0x3E, 0x40, 0xCD, 0x8C, 0x00, 0x21, 0xC0,
  0xAC, 0x87, 0x85, 0x6F, 0x7E, 0x2C, 0x6E, 0xB7, 0xC4, 0x8C, 0x00, 0x85,
  0x6F, 0x26, 0xAD, 0x7E, 0xC9, 0x67, 0x7A, 0xCD, 0x11, 0xB0, 0x01, 0x00,
  0x00, 0x3A, 0x27, 0xB0, 0x3E, 0xCD, 0x32, 0x08, 0xAF, 0x29, 0x30, 0x01,
  0x0B, 0xED, 0x43, 0x3E, 0xB0, 0xD4, 0x27, 0xB0, 0xC5, 0xD9, 0xC1, 0x21,
  0xFF, 0xAE, 0xD9, 0xCD, 0x2B, 0xB0, 0x32, 0x4B, 0xAF, 0x32, 0xD1, 0xAF,
  0x29, 0xCD, 0x6A, 0xAF, 0x3A, 0x21, 0x00, 0xD3, 0xB1, 0x3A, 0x0E, 0xB0,
  0x82, 0xD6, 0x40, 0x57, 0xAF, 0x4F, 0x47, 0xD3, 0x81, 0x6B, 0x3A, 0x22,
  0x00, 0xC9, 0xCD, 0x60, 0xAF, 0x06, 0x01, 0xFE, 0x00, 0x20, 0x07, 0xCD,
  0x60, 0xAF, 0x47, 0xCD, 0x60, 0xAF, 0xCB, 0x7A, 0xC2, 0xFC, 0xAF, 0x12,
  0x13, 0x10, 0xF7, 0x18, 0xE5, 0xD9, 0x79, 0xB0, 0x0B, 0xC2, 0x93, 0x00,
  0xD9, 0xE1, 0xC9, 0x38, 0xD9, 0xCD, 0x2B, 0xB0, 0x21, 0xA1, 0x00, 0x36,
  0xE5, 0x2B, 0x36, 0x18, 0xFE, 0x88, 0x28, 0x4E, 0x0E, 0x02, 0xFE, 0x10,
  0x38, 0x11, 0xFE, 0x90, 0x38, 0x77, 0x47, 0x0F, 0x0F, 0x0F, 0x0F, 0xE6,
  0x07, 0x4F, 0xFE, 0x06, 0x30, 0x6B, 0x78, 0xE6, 0x0F, 0xF5, 0x79, 0xCD,
  0xEF, 0xAF, 0x36, 0x3E, 0x23, 0x77, 0xF1, 0xD5, 0x21, 0xC0, 0xAC, 0x58,
  0x51, 0xFE, 0x09, 0x30, 0x54, 0xCD, 0xEF, 0xAF, 0x77, 0x2C, 0x73, 0x7B,
  0x81, 0x5F, 0x23, 0xCD, 0x2B, 0xB0, 0x15, 0x20, 0xEC, 0xCD, 0x2A, 0xB0,
  0xAF, 0x47, 0x0D, 0x3C, 0x03, 0x11, 0x00, 0xAD, 0xF7, 0x06, 0xD1, 0xB7,
  0x20, 0x33, 0x01, 0x80, 0x00, 0xCD, 0xA0, 0x00, 0x04, 0xFE, 0x00, 0x20,
  0x09, 0xCD, 0xA0, 0x00, 0x47, 0xCD, 0xA0, 0x00, 0x6F, 0x7D, 0x12, 0x1C,
  0x20, 0x04, 0x14, 0xFC, 0x0D, 0xB0, 0xD9, 0x0B, 0x79, 0xB0, 0xD9, 0xC8,
  0x10, 0xEF, 0x18, 0xDD, 0x0E, 0x01, 0x47, 0xB7, 0xC8, 0xAF, 0x37, 0x1F,
  0xCB, 0x21, 0x10, 0xFB, 0xC9, 0xF3, 0x31, 0x00, 0x01, 0x0E, 0x40, 0xF7,
  0x00, 0x3E, 0x01, 0xD3, 0xB3, 0x3E, 0x06, 0xC3, 0x0D, 0xC0, 0x3E, 0x00,
  0xC6, 0x40, 0xCB, 0xBA, 0xCB, 0xF2, 0xE6, 0xC0, 0x32, 0x0E, 0xB0, 0x07,
  0x07, 0xC6, 0x20, 0x32, 0x22, 0xB0, 0x3A, 0x20, 0x00, 0xD3, 0xB1, 0xC9,
  0xCD, 0x2B, 0xB0, 0x4F, 0xC5, 0xD5, 0x3E, 0x01, 0xF7, 0x05, 0xD1, 0xB7,
  0x20, 0xC7, 0x78, 0xC1, 0x47, 0xC9, 0xD9, 0xF5, 0xC5, 0xD5, 0x01, 0x00,
  0x00, 0x78, 0xB7, 0x28, 0x08, 0x3D, 0x32, 0x3F, 0xB0, 0x79, 0x01, 0x00,
  0x01, 0x32, 0x3E, 0xB0, 0x79, 0xB0, 0x28, 0xA9, 0x3E, 0x01, 0x11, 0x00,
  0xAE, 0xF7, 0x06, 0xF3, 0xD1, 0xC1, 0xB7, 0x20, 0x9C, 0xF1, 0xD9, 0xC3,
  0x96, 0x00, 0x00
};

static const uint8_t  lzLoaderCode[] = {
  0x00, 0x05, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xF3, 0x31, 0x00, 0x38, 0x0E, 0x40, 0xF7, 0x00,
  0x3E, 0x01, 0x11, 0x0F, 0x02, 0xF7, 0x01, 0xB7, 0x20, 0x49, 0x3C, 0x01,
  0x02, 0x00, 0x11, 0x60, 0x00, 0xF7, 0x06, 0xB7, 0x20, 0x3D, 0xED, 0x4B,
  0x60, 0x00, 0x78, 0xEE, 0x64, 0xB1, 0x20, 0x33, 0x3C, 0x01, 0x0E, 0x00,
  0xF7, 0x06, 0xB7, 0x20, 0x2A, 0x2A, 0x6E, 0x00, 0x7D, 0xB4, 0x20, 0x23,
  0xDB, 0xB0, 0xD3, 0xB1, 0x3E, 0xFF, 0xD3, 0xB2, 0x21, 0x7D, 0x01, 0x11,
  0x08, 0x00, 0x01, 0x28, 0x00, 0xED, 0xB0, 0x1E, 0x60, 0x0E, 0x6A, 0xED,
  0xB0, 0x31, 0x00, 0x01, 0x11, 0x00, 0x01, 0xD5, 0xC3, 0x28, 0x00, 0x3E,
  0x01, 0xF7, 0x03, 0xF3, 0x21, 0x08, 0x00, 0x3E, 0x30, 0x74, 0x2C, 0x28,
  0x06, 0xBD, 0x20, 0xF9, 0x29, 0x18, 0xF6, 0xFB, 0x3E, 0xFF, 0xD3, 0xB2,
  0x3E, 0x01, 0xD3, 0xB3, 0x3E, 0x06, 0xC3, 0x0D, 0xC0, 0x21, 0x00, 0x00,
  0x19, 0x38, 0x03, 0x18, 0x50, 0x04, 0xCB, 0x39, 0xCC, 0x86, 0x00, 0x38,
  0xF8, 0x04, 0x21, 0x01, 0x00, 0x05, 0xC8, 0xCB, 0x39, 0xCC, 0x86, 0x00,
  0xED, 0x6A, 0x10, 0xF7, 0xC9, 0xCD, 0xBE, 0x00, 0xAF, 0x6F, 0x67, 0x18,
  0x38, 0xD1, 0xD1, 0x6B, 0x62, 0xAF, 0x4F, 0xFB, 0xC9, 0xED, 0x52, 0x22,
  0x09, 0x00, 0x3C, 0xF7, 0x06, 0xCD, 0xBE, 0x00, 0x79, 0xB0, 0x28, 0xEB,
  0xF3, 0xD5, 0xD9, 0xE1, 0xD9, 0xEB, 0x09, 0xE5, 0x2B, 0xEB, 0x01, 0x01,
  0x10, 0xDF, 0xCF, 0xD9, 0x78, 0x41, 0x2B, 0x4E, 0xD9, 0x30, 0x03, 0x1F,
  0x4F, 0xC9, 0x12, 0x1B, 0x2B, 0x7D, 0xB4, 0x20, 0xEE, 0xCF, 0xE5, 0x68,
  0x06, 0x04, 0x28, 0x16, 0xCD, 0x1E, 0x00, 0x45, 0xDF, 0x19, 0x79, 0xED,
  0xA8, 0xC1, 0xED, 0xB8, 0x4F, 0xCB, 0x39, 0xCC, 0x86, 0x00, 0x30, 0xD2,
  0x18, 0xE3, 0xCD, 0x25, 0x00, 0x2C, 0x45, 0xDF, 0x2B, 0x18, 0xE6, 0xCD,
  0xC2, 0x00, 0x68, 0xD5, 0x3E, 0x01, 0xF7, 0x05, 0xD1, 0x4D, 0xC9, 0x00
};

static const uint8_t  compatLoaderCode[] = {
  0x3E, 0xFD, 0xD3, 0xB1, 0x87, 0xD3, 0xB2, 0x3C, 0xD3, 0xB3, 0x2A, 0x60,
  0x00, 0x01, 0xCC, 0x66, 0xED, 0x42, 0x28, 0x11, 0x21, 0x2C, 0x01, 0x11,
  0x20, 0x00, 0x01, 0x10, 0x00, 0xED, 0xB0, 0x1E, 0x60, 0x0E, 0x83, 0xED,
  0xB0, 0x11, 0x00, 0x01, 0xD5, 0xC3, 0x28, 0x00, 0xCB, 0x39, 0xC0, 0x18,
  0x5E, 0x1F, 0x4F, 0xC9, 0x26, 0x01, 0xCD, 0xD8, 0x00, 0x7C, 0x18, 0x30,
  0xED, 0x53, 0xB9, 0x00, 0xF7, 0x06, 0xD5, 0xD9, 0x21, 0x18, 0xBD, 0xE1,
  0xD9, 0xCD, 0xD8, 0x00, 0x79, 0xB0, 0x28, 0x4E, 0xF3, 0xEB, 0x09, 0xE5,
  0x2B, 0xEB, 0x01, 0x01, 0x10, 0xE7, 0x10, 0xFD, 0xCD, 0xB8, 0x00, 0xD9,
  0x78, 0x41, 0x2B, 0x4E, 0xD9, 0x38, 0x9A, 0x12, 0x1B, 0x2B, 0x7D, 0xB4,
  0x20, 0xF1, 0xCD, 0xB8, 0x00, 0xE5, 0x68, 0x06, 0x04, 0x28, 0x13, 0xCD,
  0xD2, 0x00, 0x45, 0xCD, 0xCC, 0x00, 0x19, 0x79, 0xC1, 0x03, 0xED, 0xB8,
  0x4F, 0xE7, 0x30, 0xD4, 0x18, 0xE4, 0xCD, 0xD5, 0x00, 0x45, 0xCD, 0xCD,
  0x00, 0x2B, 0x18, 0xEA, 0x21, 0x00, 0x00, 0x37, 0xED, 0x52, 0x38, 0x09,
  0xD1, 0xD1, 0x6B, 0x62, 0xAF, 0x4F, 0xFB, 0xC9, 0x04, 0xE7, 0x38, 0xFC,
  0x05, 0x21, 0x01, 0x00, 0x04, 0xC8, 0xE7, 0xED, 0x6A, 0x10, 0xFB, 0xC9,
  0xCD, 0xDC, 0x00, 0x68, 0xD5, 0x7C, 0xF7, 0x05, 0xD1, 0x4D, 0xC9
};

static const uint8_t  lzM2LoaderCode[] = {
  0x00, 0x05, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xF3, 0x31, 0x00, 0x01, 0x0E, 0x40, 0xF7, 0x00,
  0x3E, 0xFF, 0xD3, 0xB2, 0x21, 0x53, 0x01, 0x11, 0x20, 0x00, 0x01, 0x10,
  0x00, 0xED, 0xB0, 0x1E, 0x60, 0x0E, 0x52, 0xED, 0xB0, 0x11, 0x80, 0xAC,
  0x01, 0x6D, 0x01, 0xED, 0xB0, 0x3E, 0x01, 0x11, 0x22, 0x03, 0xF7, 0x01,
  0xB7, 0xC2, 0xC5, 0x02, 0x3C, 0x11, 0x10, 0x00, 0x4B, 0x42, 0xF7, 0x06,
  0xB7, 0x20, 0xF2, 0x2A, 0x10, 0x00, 0x7C, 0xEE, 0x64, 0xB5, 0x20, 0xE9,
  0x2A, 0x1E, 0x00, 0x7D, 0x3D, 0xB4, 0x20, 0xE1, 0x11, 0x00, 0x01, 0xD5,
  0xC3, 0x28, 0x00, 0xD3, 0xB3, 0xDA, 0x80, 0xEC, 0xAF, 0xFB, 0xC9, 0xF3,
  0xDB, 0xB3, 0x47, 0x37, 0x9F, 0x18, 0xF0, 0xCD, 0x66, 0x00, 0xC3, 0x00,
  0x01, 0xC3, 0x69, 0x00, 0x18, 0xBD, 0x9C, 0x67, 0x3E, 0x00, 0xD3, 0xB3,
  0x79, 0xD3, 0x81, 0xC1, 0xED, 0xB0, 0x4F, 0x3E, 0xFF, 0xD3, 0xB3, 0xFD,
  0x25, 0xC8, 0x3E, 0x80, 0xF7, 0x28, 0x12, 0x06, 0x08, 0x1F, 0xF7, 0xCA,
  0x44, 0xED, 0x10, 0xF9, 0x68, 0xF7, 0xC6, 0x10, 0x47, 0xCB, 0x15, 0x04,
  0x2C, 0xDD, 0x2C, 0xCC, 0x63, 0xED, 0xDD, 0x66, 0x00, 0x3E, 0x00, 0xD3,
  0xB3, 0x7C, 0x12, 0x13, 0x3E, 0xFF, 0xD3, 0xB3, 0x38, 0xD1, 0x10, 0xE9,
  0x2D, 0x20, 0xE6, 0x18, 0xCA, 0x78, 0x32, 0x9F, 0x00, 0x32, 0x6E, 0x00,
  0xED, 0x73, 0x3D, 0xED, 0x31, 0xDC, 0x00, 0x21, 0x30, 0x00, 0xD5, 0x11,
  0xED, 0xED, 0x01, 0x30, 0x00, 0xED, 0xB0, 0xD1, 0x2E, 0x02, 0x22, 0x6A,
  0xED, 0xCD, 0x63, 0xED, 0x2A, 0x00, 0xEF, 0x22, 0x6A, 0xED, 0xDD, 0xE5,
  0xFD, 0xE5, 0xDD, 0x21, 0xFF, 0xEF, 0x01, 0x80, 0x03, 0x3E, 0x01, 0xF7,
  0x3C, 0x6C, 0x67, 0x10, 0xF8, 0x3E, 0x61, 0xF7, 0x1F, 0x3C, 0x32, 0x2A,
  0xED, 0x38, 0x09, 0xFD, 0x26, 0x01, 0x44, 0xCD, 0x96, 0x00, 0x18, 0x59,
  0xE5, 0xFD, 0xE1, 0x3E, 0x40, 0xF7, 0x47, 0x04, 0x2E, 0x02, 0x3E, 0x90,
  0x29, 0x0F, 0x10, 0xFC, 0x32, 0x56, 0xED, 0x7D, 0xC6, 0x11, 0x47, 0xD5,
  0x11, 0x30, 0xEE, 0x21, 0x01, 0x00, 0x3E, 0x10, 0xF7, 0xC5, 0xE5, 0x47,
  0x21, 0x01, 0x00, 0x4C, 0x7C, 0x28, 0x08, 0x1F, 0xCB, 0x19, 0x29, 0x10,
  0xFA, 0xB9, 0x88, 0xEB, 0x71, 0x2C, 0x77, 0x2C, 0x4B, 0x42, 0xD1, 0x73,
  0x2C, 0x72, 0x2C, 0xEB, 0x09, 0xC1, 0x7B, 0xFE, 0x50, 0x28, 0xD4, 0xFE,
  0x60, 0x28, 0xD0, 0xFE, 0x80, 0x28, 0xCC, 0x10, 0xCD, 0xD1, 0xCD, 0x7F,
  0x00, 0xFD, 0x2D, 0x20, 0xF9, 0x06, 0x02, 0xC3, 0xB4, 0xEC, 0xFD, 0xE1,
  0xDD, 0xE1, 0xCD, 0xA8, 0xED, 0xAF, 0x4F, 0x47, 0xD3, 0x81, 0x3A, 0x9F,
  0x00, 0x31, 0x00, 0x00, 0x6B, 0x62, 0xC3, 0x20, 0x00, 0x3E, 0x14, 0x90,
  0xFF, 0xE5, 0x20, 0x0A, 0x2D, 0x28, 0x0A, 0x2D, 0x20, 0x04, 0x3E, 0x23,
  0x18, 0x05, 0x3E, 0x12, 0x21, 0x3E, 0x45, 0xF7, 0xFF, 0x7B, 0x95, 0x6F,
  0x7A, 0xC3, 0x6B, 0x00, 0xF5, 0xC5, 0xD5, 0xCD, 0xA8, 0xED, 0x01, 0x00,
  0x00, 0x78, 0xB7, 0x28, 0x08, 0x3D, 0x32, 0x6B, 0xED, 0x79, 0x01, 0x00,
  0x01, 0x32, 0x6A, 0xED, 0x79, 0xB0, 0x28, 0x11, 0x3E, 0x01, 0x11, 0x00,
  0xEF, 0xF7, 0x06, 0xB7, 0x20, 0x07, 0xCD, 0xAE, 0xED, 0xD1, 0xC1, 0xF1,
  0xC9, 0xF3, 0x31, 0x00, 0x01, 0x3E, 0xFF, 0xD3, 0xB2, 0xC3, 0x9B, 0xAD,
  0x0E, 0x40, 0xF7, 0x00, 0x3E, 0x01, 0xD3, 0xB3, 0x3E, 0x06, 0xC3, 0x0D,
  0xC0, 0xE5, 0x21, 0xED, 0xED, 0x18, 0x05, 0xF3, 0xE5, 0x21, 0xBD, 0xED,
  0x11, 0x30, 0x00, 0x01, 0x30, 0x00, 0xED, 0xB0, 0xE1, 0xC9, 0xCB, 0x21,
  0x28, 0x1E, 0x8F, 0x30, 0xF9, 0xC9, 0x87, 0x87, 0x6F, 0x26, 0xEE, 0x7E,
  0xB7, 0xC4, 0x30, 0x00, 0x47, 0x2C, 0x7E, 0xB7, 0xC4, 0x30, 0x00, 0x2C,
  0x86, 0x2C, 0x66, 0x6F, 0x78, 0x8C, 0x67, 0xC9, 0xDD, 0x2C, 0xCC, 0x63,
  0xED, 0xDD, 0x4E, 0x00, 0xCB, 0x11, 0x8F, 0x30, 0xFB, 0xC9, 0x00
};

static void printUsage()
{
  std::printf("Usage:\n");
  std::printf("    dtf [OPTIONS...] <DTFFILE.DTF> <FILE1> [FILE2 "
              "[FILE3...]]\n");
  std::printf("    dtf -c [OPTIONS...] <DTFFILE.DTF> <FILE1> [FILE2 "
              "[FILE3...]]\n");
  std::printf("        create a DTF archive for ATTUS.LDR from the specified "
              "list of\n"
              "        files\n");
  std::printf("    dtf -cp [OPTIONS...] <DTFFILE.DTF> <FILE1.COM> [FILE2 "
              "[FILE3...]]\n");
  std::printf("        create a DTF program for the DTF&TOM loader from the "
              "given list\n"
              "        of files; the first file is expected to be the loader "
              "as EXOS\n"
              "        file type 5, or raw Z80 code; the remaining files are "
              "the\n"
              "        program data (.SCR, .PRG, etc.), and should be "
              "specified in the\n"
              "        correct order;\n");
  std::printf("        it is possible to split input files by appending a "
              "\"::\" followed\n"
              "        by a comma separated list of block sizes (which can be "
              "decimal,\n"
              "        or hexadecimal with a \"0x\" prefix) to the file "
              "name\n");
  std::printf("    dtf -cr [OPTIONS...] <DTFFILE.BIN> <FILE1> [FILE2 "
              "[FILE3...]]\n");
  std::printf("        create raw DTF compressed data; splitting input files "
              "is\n"
              "        supported, similarly to -cp\n");
  std::printf("    dtf -x <DTFFILE.DTF> [FILE1 [FILE2 [FILE3...]]]\n");
  std::printf("        extract all files from a DTF archive or program; file "
              "names\n"
              "        specified on the command line override file names "
              "stored in a\n"
              "        DTF archive; in the case of DTF programs, the file "
              "names default\n"
              "        to 'DTFFILE.com' (which is saved with EXOS header type "
              "5),\n"
              "        'DTFFILE-01.prg', 'DTFFILE-02.prg', and so on\n");
  std::printf("    dtf -xr [-lz|-lz2|-lz0] <DTFFILE.BIN> "
              "[FILE1 [FILE2 [FILE3...]]]\n");
  std::printf("        extract all data blocks from raw DTF compressed data; "
              "file names\n"
              "        default to 'DTFFILE-01.bin', 'DTFFILE-02.bin', and so "
              "on, but\n"
              "        can be overridden on the command line\n");
  std::printf("    dtf -d <DTFFILE.DTF> [LOADER.COM [PROGRAM.PRG]]\n");
  std::printf("        decompress a DTF program to a single file, and create "
              "a loader\n"
              "        with EXOS header type 5 that can load the decompressed "
              "program\n"
              "        file; the output file names default to 'DTFFILE.com' "
              "and\n"
              "        'DTFFILE.prg', but these can be optionally "
              "overridden\n");
  std::printf("    dtf -cl [-lz|-lz2] <LOADER.COM> <DTFFILE.DTF>\n");
  std::printf("        create loader for the specified DTF program name\n");
  std::printf("Options:\n");
  std::printf("    --\n");
  std::printf("        interpret all remaining arguments as file names\n");
  std::printf("    -h | -help | --help\n");
  std::printf("        print this message\n");
  std::printf("    -a | -ca | --create-archive\n");
  std::printf("        same as '-c'\n");
  std::printf("    --create-program\n");
  std::printf("        same as '-cp'\n");
  std::printf("    --create-raw\n");
  std::printf("        same as '-cr'\n");
  std::printf("    --extract\n");
  std::printf("        same as '-x'\n");
  std::printf("    --extract-raw\n");
  std::printf("        same as '-xr'\n");
  std::printf("    --decompress-program\n");
  std::printf("        same as '-d'\n");
  std::printf("    --create-loader\n");
  std::printf("        same as '-cl'\n");
  std::printf("    -idp | --input-decompressed-program\n");
  std::printf("        assume that the input file is a DTF program "
              "previously\n"
              "        decompressed with the 'dtf -d' command or Anti-DTF; it "
              "is used\n"
              "        in -cp and -cr mode only, requires a single input "
              "file, and\n"
              "        ignores the block size list\n");
  std::printf("    -b | --block-size <N>           (0, or 16 to 0xFC00; "
              "default: 16384)\n");
  std::printf("        set the block size when creating a DTF archive; a "
              "value of zero\n"
              "        means finding optimal block sizes. Note that any "
              "setting other\n"
              "        than the default 16384 is incompatible with "
              "ATTUS.LDR\n");
  std::printf("    -p | --prefix-bits <MIN> <MAX>  (1 to 7; default: MIN=2, "
              "MAX=2)\n");
  std::printf("        set the minimum and maximum number of prefix bits to "
              "be used by\n"
              "        the statistical compression; if the two values are not "
              "the same,\n"
              "        then the optimal prefix size is searched in the "
              "specified range.\n");
  std::printf("        Note that prefix sizes greater than 5 bits are "
              "incompatible with\n"
              "        the currently available DTF decompressors on the "
              "Enterprise, and\n"
              "        any setting other than the default is only supported "
              "by DL2\n");
  std::printf("    -n | --nostat\n");
  std::printf("    --stat\n");
  std::printf("        disable or enable statistical compression; it is "
              "enabled by\n"
              "        default, and is required for compatibility with older "
              "Enterprise\n"
              "        DTF decompressors\n");
  std::printf("    -1..-9\n");
  std::printf("        set compression level for -lz2 and -lz0 (default: 5)\n");
  std::printf("    -lz\n");
  std::printf("    -lz2\n");
  std::printf("    -lz0\n");
  std::printf("        use alternate compression algorithms (requires DL2 on "
              "the\n"
              "        Enterprise, but -lz0 is not supported by it yet); "
              "-b, -p, and -n\n"
              "        are ignored in these modes\n");
  std::printf("    -C\n");
  std::printf("        include compatibility loader code (for -cp -lz only)\n");
}

static void readInputFile(const char *fileName,
                          std::vector< uint8_t >& buf,
                          bool ignoreBlockSizeList = false)
{
  buf.resize(0);
  if (fileName == (char *) 0)
    throw Exception("invalid input file name");
  std::string fName(fileName);
  if (ignoreBlockSizeList) {
    // parse file name for block size list
    for (size_t i = 0; (i + 3) <= fName.length(); i++) {
      if (fName[i] == ':' && fName[i + 1] == ':' &&
          fName[i + 2] >= '0' && fName[i + 2] <= '9') {
        fName.resize(i);
        break;
      }
    }
  }
  fileName = fName.c_str();
  if (fileName[0] == '\0')
    throw Exception("invalid input file name");
  std::FILE *f = (std::FILE *) 0;
  try {
    f = std::fopen(fileName, "rb");
    if (!f)
      throw Exception("error opening input file");
    while (true) {
      int     c = std::fgetc(f);
      if (c == EOF)
        break;
      buf.push_back(uint8_t(c & 0xFF));
    }
    std::fclose(f);
    f = (std::FILE *) 0;
    if (buf.size() < 1)
      throw Exception("empty input file");
  }
  catch (...) {
    if (f)
      std::fclose(f);
    throw;
  }
}

static void writeOutputFile(const char *fileName,
                            const std::vector< uint8_t >& buf)
{
  if (fileName == (char *) 0 || fileName[0] == '\0')
    throw Exception("invalid output file name");
  std::FILE *f = (std::FILE *) 0;
  try {
    f = std::fopen(fileName, "wb");
    if (!f)
      throw Exception("error opening output file");
    for (size_t i = 0; i < buf.size(); i++) {
      if (std::fputc(buf[i], f) == EOF)
        throw Exception("error writing output file - is the disk full ?");
    }
    if (std::fflush(f) != 0)
      throw Exception("error writing output file - is the disk full ?");
    int     err = std::fclose(f);
    f = (std::FILE *) 0;
    if (err != 0) {
      std::remove(fileName);
      throw Exception("error closing output file");
    }
  }
  catch (...) {
    if (f) {
      std::fclose(f);
      std::remove(fileName);
    }
    throw;
  }
}

static void writeLoaderProgram(const std::string& fileName,
                               const std::string& prgName, int prgType)
{
  // prgType = 0: PRG (uncompressed)
  // prgType = 1: DTF
  // prgType = 2: LZ
  // prgType = 3: LZ2
  // prgType = 4: LZ0
  std::vector< uint8_t >  outBuf;
  if (prgType == 1) {
    for (size_t i = 0; i < (sizeof(dtfLoaderCode) / sizeof(uint8_t)); i++)
      outBuf.push_back(dtfLoaderCode[i]);
  }
  else if (prgType == 2) {
    for (size_t i = 0; i < (sizeof(lzLoaderCode) / sizeof(uint8_t)); i++)
      outBuf.push_back(lzLoaderCode[i]);
  }
  else if (prgType == 3) {
    for (size_t i = 0; i < (sizeof(lzM2LoaderCode) / sizeof(uint8_t)); i++)
      outBuf.push_back(lzM2LoaderCode[i]);
  }
  else if (prgType == 4) {
    throw Exception("program loader code for -lz0 is not implemented yet");
  }
  else {
    prgType = 0;
    for (size_t i = 0; i < (sizeof(prgLoaderCode) / sizeof(uint8_t)); i++)
      outBuf.push_back(prgLoaderCode[i]);
  }
  std::string outFileName(prgName);
  fixDTFFileName(outFileName, true, 27);
  if (prgType == 0) {
    outBuf[outBuf.size() - 28] = uint8_t(outFileName.length());
    for (size_t i = 0; i < outFileName.length(); i++)
      outBuf[(outBuf.size() - 27) + i] = uint8_t(outFileName[i]);
  }
  else {
    outBuf[outBuf.size() - 1] = uint8_t(outFileName.length());
    for (size_t i = 0; i < outFileName.length(); i++)
      outBuf.push_back(uint8_t(outFileName[i]));
    outBuf[2] = uint8_t((outBuf.size() - 16) & 0xFF);
    outBuf[3] = uint8_t((outBuf.size() - 16) >> 8);
  }
  std::printf("  %s\n", fileName.c_str());
  writeOutputFile(fileName.c_str(), outBuf);
}

int main(int argc, char **argv)
{
  // 0: create DTF archive
  // 1: create DTF program
  // 2: create raw DTF data
  // 3: extract DTF file
  // 4: extract raw DTF data
  // 5: decompress DTF program
  // 6: create loader for DTF program
  int     mode = 0;
  bool    decompPrgInput = false;
  bool    useCompatCode = false;
  DTFCompressionParameters  cfg;
  try {
    std::vector< std::string >  fileNames;
    bool    endOfOptions = false;
    for (int i = 1; i < argc; i++) {
      if (argv[i] == (char *) 0 || argv[i][0] == '\0')
        continue;
      if (endOfOptions || argv[i][0] != '-') {
        fileNames.push_back(std::string(argv[i]));
        continue;
      }
      std::string s;
      if (argv[i][1] == '-') {
        if (argv[i][2] == '\0') {
          endOfOptions = true;
          continue;
        }
        s = &(argv[i][1]);
      }
      else {
        s = argv[i];
      }
      if (s == "-h" || s == "-help" || s == "--help") {
        printUsage();
        return 0;
      }
      else if (s == "-a" || s == "-c" || s == "-ca" || s == "-create-archive") {
        mode = 0;
      }
      else if (s == "-cp" || s == "-create-program") {
        mode = 1;
      }
      else if (s == "-cr" || s == "-create-raw") {
        mode = 2;
      }
      else if (s == "-x" || s == "-extract") {
        mode = 3;
      }
      else if (s == "-xr" || s == "-extract-raw") {
        mode = 4;
      }
      else if (s == "-d" || s == "-decompress-program") {
        mode = 5;
      }
      else if (s == "-cl" || s == "-create-loader") {
        mode = 6;
      }
      else if (s == "-idp" || s == "-input-decompressed-program") {
        decompPrgInput = true;
      }
      else if (s == "-b" || s == "-block-size") {
        if (++i >= argc) {
          printUsage();
          throw Exception("missing argument for --block-size");
        }
        char    *endPtr = (char *) 0;
        long    n = std::strtol(argv[i], &endPtr, 0);
        if (argv[i][0] == '\0' || endPtr == (char *) 0 || endPtr[0] != '\0') {
          printUsage();
          throw Exception("invalid argument for --block-size: "
                          "must be an integer");
        }
        if (n < 0L || n > 0xFC00L || (n >= 1L && n < 16L)) {
          printUsage();
          throw Exception("--block-size parameter is out of range");
        }
        cfg.blockSize = size_t(n);
      }
      else if (s == "-p" || s == "-prefix-bits") {
        if (++i >= argc) {
          printUsage();
          throw Exception("missing arguments for --prefix-bits");
        }
        char    *endPtr = (char *) 0;
        long    n = std::strtol(argv[i], &endPtr, 0);
        if (argv[i][0] == '\0' || endPtr == (char *) 0 || endPtr[0] != '\0') {
          printUsage();
          throw Exception("invalid argument for --prefix-bits: "
                          "must be an integer");
        }
        if (n < 1L || n > 7L) {
          printUsage();
          throw Exception("prefix size is out of range");
        }
        cfg.minPrefixSize = int(n);
        if (++i >= argc) {
          printUsage();
          throw Exception("missing argument for --prefix-bits");
        }
        endPtr = (char *) 0;
        n = std::strtol(argv[i], &endPtr, 0);
        if (argv[i][0] == '\0' || endPtr == (char *) 0 || endPtr[0] != '\0') {
          printUsage();
          throw Exception("invalid argument for --prefix-bits: "
                          "must be an integer");
        }
        if (n < 1L || n > 7L) {
          printUsage();
          throw Exception("prefix size is out of range");
        }
        if (n < long(cfg.minPrefixSize)) {
          cfg.maxPrefixSize = cfg.minPrefixSize;
          cfg.minPrefixSize = int(n);
        }
        else {
          cfg.maxPrefixSize = int(n);
        }
      }
      else if (s == "-n" || s == "-nostat") {
        cfg.disableStatisticalCompression = true;
      }
      else if (s == "-stat") {
        cfg.disableStatisticalCompression = false;
      }
      else if (s == "-dtf") {
        cfg.compressionType = 0;
      }
      else if (s == "-lz") {
        cfg.compressionType = 1;
      }
      else if (s == "-lz2") {
        cfg.compressionType = 2;
      }
      else if (s == "-lz0") {
        cfg.compressionType = 3;
      }
      else if (s.length() == 2 &&
               (s[0] == '-' && s[1] >= '1' && s[1] <= '9')) {
        cfg.compressionLevel = uint8_t(s[1] - '0');
      }
      else if (s == "-C") {
        useCompatCode = true;
      }
      else {
        printUsage();
        throw Exception("invalid command line option");
      }
    }
    if (fileNames.size() < 1 || (mode < 3 && fileNames.size() < 2)) {
      printUsage();
      throw Exception("missing file name");
    }
    if (mode >= 1 && mode <= 2 && decompPrgInput && fileNames.size() > 2) {
      printUsage();
      throw Exception("too many input files");
    }
    if (mode == 6 && fileNames.size() != 2) {
      printUsage();
      throw Exception("invalid number of file names");
    }
    std::vector< uint8_t >  inBuf;
    std::vector< uint8_t >  outBuf;
    if (mode >= 3 && mode <= 5)         // if extracting, read input file
      readInputFile(fileNames[0].c_str(), inBuf);
    switch (mode) {
    case 0:                             // create DTF archive
      {
        outBuf.resize(6);
        outBuf[0] = 0x41;       // 'A'
        outBuf[1] = 0x54;       // 'T'
        outBuf[2] = 0x54;       // 'T'
        outBuf[3] = 0x55;       // 'U'
        outBuf[4] = 0x53;       // 'S'
        outBuf[5] = 0x20;       // ' '
        for (size_t i = 1; i < fileNames.size(); i++) {
          std::string s(fileNames[i]);
          fixDTFFileName(s, true);
          std::printf("  %s\n", s.c_str());
          readInputFile(fileNames[i].c_str(), inBuf);
          outBuf.push_back(uint8_t(s.length()));
          for (size_t j = 0; j < 13; j++) {
            if (j < s.length())
              outBuf.push_back(uint8_t(s[j]));
            else
              outBuf.push_back(0x20);
          }
          DTFCompressor dtfCompressor;
          dtfCompressor.compressData(outBuf, inBuf, cfg);
        }
      }
      break;
    case 1:                             // create DTF program
      {
        DTFCompressor dtfCompressor;
        if (cfg.compressionType > 0) {
          outBuf.resize(16, 0x00);
          outBuf[1] = 0x64;
          outBuf[14] = uint8_t(cfg.compressionType - 1);
          if (useCompatCode && cfg.compressionType == 1) {
            inBuf.resize(sizeof(compatLoaderCode) / sizeof(uint8_t));
            for (size_t i = 0;
                 i < (sizeof(compatLoaderCode) / sizeof(uint8_t));
                 i++) {
              inBuf[i] = compatLoaderCode[i];
            }
            dtfCompressor.compressDataBlock(outBuf, inBuf, cfg, false,
                                            (char *) 0);
          }
        }
        if (!decompPrgInput) {
          // compress loader code
          inBuf.resize(0);
          std::printf("  %s\n", fileNames[1].c_str());
          readInputFile(fileNames[1].c_str(), inBuf, true);
          if (inBuf.size() >= 4) {
            if (inBuf[0] == 0x00 && inBuf[1] == 0x05) {
              size_t  nBytes = size_t(inBuf[2]) | (size_t(inBuf[3]) << 8);
              if (inBuf.size() < (nBytes + 16))
                throw Exception("unexpected end of DTF loader file");
              inBuf.erase(inBuf.begin(), inBuf.begin() + 16);
              inBuf.resize(nBytes);
            }
          }
          if (inBuf.size() < 1 || inBuf.size() > 0x3F00)
            throw Exception("invalid DTF loader code size");
          if (cfg.compressionType == 0) {
            outBuf.resize(3);
            uint8_t rleFlagByte = tomCompressData(outBuf, inBuf);
            outBuf[2] = rleFlagByte;
            outBuf[0] = uint8_t((outBuf.size() - 2) & 0xFF);
            outBuf[1] = uint8_t((outBuf.size() - 2) >> 8);
          }
          else {
            dtfCompressor.compressDataBlock(outBuf, inBuf, cfg, decompPrgInput,
                                            fileNames[1].c_str());
          }
        }
        // compress all data files
        for (size_t i = 2 - size_t(decompPrgInput);
             i < fileNames.size();
             i++) {
          inBuf.resize(0);
          std::printf("  %s\n", fileNames[i].c_str());
          readInputFile(fileNames[i].c_str(), inBuf, true);
          dtfCompressor.compressDataBlock(outBuf, inBuf, cfg, decompPrgInput,
                                          fileNames[i].c_str());
        }
      }
      break;
    case 2:                             // create raw DTF data
      {
        // compress all data files
        for (size_t i = 1; i < fileNames.size(); i++) {
          inBuf.resize(0);
          std::printf("  %s\n", fileNames[i].c_str());
          readInputFile(fileNames[i].c_str(), inBuf, true);
          DTFCompressor dtfCompressor;
          dtfCompressor.compressDataBlock(outBuf, inBuf, cfg, decompPrgInput,
                                          fileNames[i].c_str());
        }
      }
      break;
    case 3:                             // extract DTF file
      {
        cfg.compressionType = 0;
        if (inBuf.size() > 16) {
          if (inBuf[0] == 0x00 && inBuf[1] == 0x64 && inBuf[14] <= 0x02 &&
              inBuf[15] == 0x00) {
            cfg.compressionType = inBuf[14] + 1;
            inBuf.erase(inBuf.begin(), inBuf.begin() + 16);
          }
        }
        DTFDecompressor dtfDecompressor(inBuf);
        bool    isArchive = false;
        if (inBuf.size() >= 6) {
          if (inBuf[0] == 0x41 && inBuf[1] == 0x54 &&   // "AT"
              inBuf[2] == 0x54 && inBuf[3] == 0x55 &&   // "TU"
              inBuf[4] == 0x53 && inBuf[5] == 0x20) {   // "S "
            isArchive = true;
          }
        }
        if (isArchive) {                // extract DTF archive
          size_t  readPosition = 6;
          size_t  fileCnt = 1;
          while (readPosition < inBuf.size()) {
            if ((readPosition + 14) > inBuf.size())
              throw Exception("unexpected end of DTF archive");
            size_t  nameLen = inBuf[readPosition];
            if (nameLen < 1 || nameLen > 13)
              throw Exception("error in DTF archive header");
            std::string outFileName;
            if (fileCnt < fileNames.size()) {
              outFileName = fileNames[fileCnt];
            }
            else {
              for (size_t i = 0; i < nameLen; i++) {
                char    c = inBuf[readPosition + 1 + i];
                fixDTFFileNameCharacter(c);
                if (i == 0 && c == '.')
                  c = '_';
                outFileName += c;
              }
            }
            std::printf("  %s\n", outFileName.c_str());
            readPosition = readPosition + 14;
            outBuf.resize(0);
            bool    lastBlockFlag = false;
            do {
              if (readPosition >= inBuf.size())
                throw Exception("unexpected end of DTF archive data");
              if (inBuf[readPosition] == 0xE4)
                lastBlockFlag = true;
              else if (inBuf[readPosition] != 0x00)
                throw Exception("error in DTF archive data block header");
              readPosition++;
              dtfDecompressor.setReadPosition(readPosition);
              dtfDecompressor.decompressDataBlock(outBuf, cfg.compressionType);
              readPosition = dtfDecompressor.getReadPosition();
            } while (!lastBlockFlag);
            writeOutputFile(outFileName.c_str(), outBuf);
            fileCnt++;
          }
        }
        else {                          // extract DTF program
          dtfDecompressor.setReadPosition(0);
          size_t  fileCnt = 1;
          while (dtfDecompressor.getReadPosition() < inBuf.size()) {
            std::string outFileName;
            if (fileCnt < fileNames.size()) {
              outFileName = fileNames[fileCnt];
            }
            else {
              outFileName = fileNames[0];
              removeFileNamePath(outFileName);
              setFileNameExtension(outFileName, "");
              fixDTFFileName(outFileName, false, 248);
              if (fileCnt == 1) {
                outFileName += ".com";
              }
              else {
                outFileName += '-';
                outFileName += char((((fileCnt - 1) / 10) % 10) + 0x30);
                outFileName += char(((fileCnt - 1) % 10) + 0x30);
                outFileName += ".prg";
              }
            }
            std::printf("  %s\n", outFileName.c_str());
            outBuf.resize(0);
            if (fileCnt == 1) {
              outBuf.resize(16, 0x00);
              dtfDecompressor.decompressTOMDataBlock(outBuf,
                                                     cfg.compressionType);
              if (outBuf.size() < 17 || outBuf.size() > 0x3F10)
                throw Exception("invalid DTF loader code size");
              outBuf[1] = 0x05;
              outBuf[2] = uint8_t((outBuf.size() - 16) & 0xFF);
              outBuf[3] = uint8_t((outBuf.size() - 16) >> 8);
            }
            else {
              dtfDecompressor.decompressDataBlock(outBuf, cfg.compressionType);
            }
            writeOutputFile(outFileName.c_str(), outBuf);
            fileCnt++;
          }
        }
      }
      break;
    case 4:                             // extract raw DTF data
      {
        DTFDecompressor dtfDecompressor(inBuf);
        dtfDecompressor.setReadPosition(0);
        size_t  fileCnt = 1;
        while (dtfDecompressor.getReadPosition() < inBuf.size()) {
          std::string outFileName;
          if (fileCnt < fileNames.size()) {
            outFileName = fileNames[fileCnt];
          }
          else {
            outFileName = fileNames[0];
            removeFileNamePath(outFileName);
            setFileNameExtension(outFileName, "");
            fixDTFFileName(outFileName, false, 248);
            outFileName += '-';
            outFileName += char(((fileCnt / 10) % 10) + 0x30);
            outFileName += char((fileCnt % 10) + 0x30);
            outFileName += ".bin";
          }
          std::printf("  %s\n", outFileName.c_str());
          outBuf.resize(0);
          dtfDecompressor.decompressDataBlock(outBuf, cfg.compressionType);
          writeOutputFile(outFileName.c_str(), outBuf);
          fileCnt++;
        }
      }
      break;
    case 5:                             // decompress DTF program
      {
        if (fileNames.size() > 3) {
          printUsage();
          throw Exception("too many file names");
        }
        while (fileNames.size() < 3) {
          std::string s(fileNames[0]);
          removeFileNamePath(s);
          setFileNameExtension(s, "");
          fixDTFFileName(s, false, 23);
          if (fileNames.size() == 1)
            s += ".com";
          else
            s += ".prg";
          fileNames.push_back(s);
        }
        if (inBuf.size() >= 6) {
          if (inBuf[0] == 0x41 && inBuf[1] == 0x54 &&   // "AT"
              inBuf[2] == 0x54 && inBuf[3] == 0x55 &&   // "TU"
              inBuf[4] == 0x53 && inBuf[5] == 0x20) {   // "S "
            throw Exception("cannot decompress DTF archive as program");
          }
        }
        cfg.compressionType = 0;
        if (inBuf.size() > 16) {
          if (inBuf[0] == 0x00 && inBuf[1] == 0x64 && inBuf[14] <= 0x02 &&
              inBuf[15] == 0x00) {
            cfg.compressionType = inBuf[14] + 1;
            inBuf.erase(inBuf.begin(), inBuf.begin() + 16);
          }
        }
        DTFDecompressor dtfDecompressor(inBuf);
        dtfDecompressor.setReadPosition(0);
        // extract loader
        outBuf.resize(2);
        dtfDecompressor.decompressTOMDataBlock(outBuf, cfg.compressionType);
        if (outBuf.size() > 0x3F02)
          throw Exception("error in DTF program: loader code is too large");
        outBuf[0] = uint8_t((outBuf.size() - 2) & 0xFF);
        outBuf[1] = uint8_t((outBuf.size() - 2) >> 8);
        // extract all program data blocks
        while (dtfDecompressor.getReadPosition() < inBuf.size()) {
          outBuf.push_back(0x00);
          outBuf.push_back(0x00);
          size_t  startPos = outBuf.size();
          dtfDecompressor.decompressDataBlock(outBuf, cfg.compressionType);
          outBuf[startPos - 2] = uint8_t((outBuf.size() - startPos) & 0xFF);
          outBuf[startPos - 1] = uint8_t((outBuf.size() - startPos) >> 8);
        }
        // write program data
        std::printf("  %s\n", fileNames[2].c_str());
        writeOutputFile(fileNames[2].c_str(), outBuf);
        // write loader
        writeLoaderProgram(fileNames[1], fileNames[2], 0);
      }
      break;
    case 6:                             // create loader for DTF program
      writeLoaderProgram(fileNames[0], fileNames[1], cfg.compressionType + 1);
      break;
    default:
      throw Exception("internal error: invalid mode");
    }
    if (mode < 3) {
      // if creating an archive: write output file
      writeOutputFile(fileNames[0].c_str(), outBuf);
    }
  }
  catch (std::exception& e) {
    std::fprintf(stderr, " *** dtf: %s\n", e.what());
    return -1;
  }
  return 0;
}

