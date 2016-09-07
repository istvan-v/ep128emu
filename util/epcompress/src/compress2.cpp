
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
#include "compress2.hpp"

#include <list>
#include <map>

namespace Ep128Compress {

  const size_t Compressor_M2::lengthPrefixSizeTable[lengthNumSlots] = {
    1, 2, 3, 4, 5, 6, 7, 8
  };

  const size_t Compressor_M2::offs3SlotCntTable[4] = {
    4, 8, 16, 32
  };

  Compressor_M2::EncodeTable::EncodeTable(size_t nSlots_, size_t nSymbols_,
                                          const size_t *slotPrefixSizeTable_,
                                          size_t minPrefixSize_,
                                          size_t maxPrefixSize_,
                                          const size_t *prefixSlotCntTable_)
    : nSlots(nSlots_),
      nSymbols(nSymbols_),
      nSymbolsUsed(nSymbols_),
      nSymbolsEncoded(nSymbols_),
      totalSlotWeight(0),
      unusedSymbolSize(8192),
      minPrefixSize(minPrefixSize_),
      maxPrefixSize(maxPrefixSize_),
      prefixOnlySymbolCnt(0)
  {
    if (nSymbols < 1)
      throw Ep128Emu::Exception("EncodeTable::EncodeTable(): nSymbols < 1");
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
      if (nSlots < 1)
        throw Ep128Emu::Exception("EncodeTable::EncodeTable(): nSlots < 1");
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

  Compressor_M2::EncodeTable::~EncodeTable()
  {
  }

  void Compressor_M2::EncodeTable::setPrefixSize(size_t n)
  {
    if (n < 1) {
      throw Ep128Emu::Exception("EncodeTable::setPrefixSize(): "
                                "prefix size < 1");
    }
    if (n < minPrefixSize || n > maxPrefixSize) {
      throw Ep128Emu::Exception("EncodeTable::setPrefixSize(): "
                                "prefix size is out of range");
    }
    nSlots = prefixSlotCntTable[n - minPrefixSize];
    if (nSlots < 1)
      throw Ep128Emu::Exception("EncodeTable::setPrefixSize(): nSlots < 1");
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

  inline size_t Compressor_M2::EncodeTable::calculateEncodedSize() const
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

  inline size_t Compressor_M2::EncodeTable::calculateEncodedSize(
      size_t firstSlot, unsigned int firstSymbol, size_t baseSize) const
  {
    size_t  totalSize = baseSize;
    size_t  p = size_t(firstSymbol);
    for (size_t i = firstSlot; i < nSlots; i++) {
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

  size_t Compressor_M2::EncodeTable::optimizeSlotBitsTable_fast()
  {
    size_t  totalSymbolCnt = symbolCntTable[nSymbolsUsed];
    size_t  slotWeightSum = totalSlotWeight;
    size_t  slotBegin = 0;
    size_t  slotEnd = 0;
    for (size_t i = 0; i < nSlots; i++) {
      slotBegin = slotEnd;
      if (totalSymbolCnt < 1) {
        slotBitsTable[i] = 0;
        continue;
      }
      if ((i + 1) < nSlots) {
        size_t  bestSize = 0;
        long    bestDiff = 0x7FFFFFFFL;
        for (size_t j = 0; j <= 15; j++) {
          slotEnd = slotBegin + (size_t(1) << j);
          if (slotEnd > nSymbolsUsed)
            slotEnd = nSymbolsUsed;
          if ((i + 2) >= nSlots && (slotEnd + 32768) < nSymbolsUsed)
            continue;
          size_t  tmp = symbolCntTable[slotEnd] - symbolCntTable[slotBegin];
          tmp = size_t(tmp * uint64_t(0x01000000U) / totalSymbolCnt);
          size_t  tmp2 = size_t(slotWeightTable[i] * uint64_t(0x01000000U)
                                / slotWeightSum);
          long    d = long(tmp) - long(tmp2);
          if (d < 0L)
            d = (-d);
          if (d < bestDiff || (tmp == 0 && d == bestDiff)) {
            bestSize = j;
            bestDiff = d;
          }
        }
        slotBitsTable[i] = bestSize;
      }
      else {
        for (size_t j = 0; true; j++) {
          slotEnd = slotBegin + (size_t(1) << j);
          if (slotEnd > nSymbolsUsed)
            slotEnd = nSymbolsUsed;
          size_t  tmp = symbolCntTable[slotEnd] - symbolCntTable[slotBegin];
          if (tmp >= totalSymbolCnt) {
            slotBitsTable[i] = j;
            break;
          }
          if (j >= 15) {
            throw Ep128Emu::Exception("internal error in "
                                      "EncodeTable::optimizeSlotBitsTable()");
          }
        }
      }
      slotEnd = slotBegin + (size_t(1) << slotBitsTable[i]);
      if (slotEnd > nSymbolsUsed)
        slotEnd = nSymbolsUsed;
      size_t  symbolsUsed =
          symbolCntTable[slotEnd] - symbolCntTable[slotBegin];
      totalSymbolCnt = totalSymbolCnt - symbolsUsed;
      slotWeightSum = slotWeightSum - slotWeightTable[i];
    }
    size_t  bestSize = calculateEncodedSize();
    for (int l = 0; l < 4; l++) {
      int     offs = ((l & 1) == 0 ? 1 : -1);
      while (true) {
        size_t  bestSlot = 0;
        bool    doneFlag = true;
        for (size_t i = 0; i < nSlots; i++) {
          if (!((slotBitsTable[i] >= 15 && offs > 0) ||
                (slotBitsTable[i] < 1 && offs < 0))) {
            slotBitsTable[i] = size_t(int(slotBitsTable[i]) + offs);
            size_t  newSize = calculateEncodedSize();
            slotBitsTable[i] = size_t(int(slotBitsTable[i]) - offs);
            if (newSize < bestSize) {
              bestSize = newSize;
              bestSlot = i;
              doneFlag = false;
            }
          }
        }
        if (doneFlag)
          break;
        slotBitsTable[bestSlot] = size_t(int(slotBitsTable[bestSlot]) + offs);
      }
    }
    std::vector< size_t > bestEncodeTable(nSlots);
    for (size_t i = 0; i < nSlots; i++)
      bestEncodeTable[i] = slotBitsTable[i];
    bool    doneFlag = false;
    do {
      doneFlag = true;
      for (size_t i = 0; (i + 1) < nSlots; i++) {
        size_t  firstSlot = i;
        size_t  baseSize = 0;
        unsigned int  firstSymbol = 0U;
        if (firstSlot > 0) {
          for (size_t j = 0; j < firstSlot; j++) {
            size_t  symbolCnt = size_t(symbolCntTable[firstSymbol]);
            firstSymbol = firstSymbol + (1U << (unsigned int) slotBitsTable[j]);
            size_t  symbolSize = slotPrefixSizeTable[j] + slotBitsTable[j];
            if (size_t(firstSymbol) >= nSymbolsUsed)
              break;
            baseSize += ((size_t(symbolCntTable[firstSymbol]) - symbolCnt)
                         * symbolSize);
          }
          if (size_t(firstSymbol) >= nSymbolsUsed)
            continue;
        }
        for (size_t j = i + 1; j < nSlots; j++) {
          if (bestEncodeTable[i] == bestEncodeTable[j])
            continue;
          slotBitsTable[i] = bestEncodeTable[j];
          slotBitsTable[j] = bestEncodeTable[i];
          size_t  newSize =
              calculateEncodedSize(firstSlot, firstSymbol, baseSize);
          if (newSize < bestSize) {
            bestSize = newSize;
            doneFlag = false;
            bestEncodeTable[i] = slotBitsTable[i];
            bestEncodeTable[j] = slotBitsTable[j];
          }
          else {
            slotBitsTable[i] = bestEncodeTable[i];
            slotBitsTable[j] = bestEncodeTable[j];
          }
        }
      }
      for (size_t i = nSlots; i > 0; ) {
        i--;
        int     bestOffsets[3];
        bestOffsets[0] = 0;
        bestOffsets[1] = 0;
        bestOffsets[2] = 0;
        size_t  firstSlot = (i > 2 ? (i - 2) : 0);
        size_t  baseSize = 0;
        unsigned int  firstSymbol = 0U;
        if (firstSlot > 0) {
          for (size_t j = 0; j < firstSlot; j++) {
            size_t  symbolCnt = size_t(symbolCntTable[firstSymbol]);
            firstSymbol = firstSymbol + (1U << (unsigned int) slotBitsTable[j]);
            size_t  symbolSize = slotPrefixSizeTable[j] + slotBitsTable[j];
            if (size_t(firstSymbol) >= nSymbolsUsed)
              break;
            baseSize += ((size_t(symbolCntTable[firstSymbol]) - symbolCnt)
                         * symbolSize);
          }
          if (size_t(firstSymbol) >= nSymbolsUsed)
            continue;
        }
        for (int offs2 = (i >= 2 ? -1 : 1); offs2 <= 1; offs2++) {
          if (i >= 2) {
            int     tmp = int(bestEncodeTable[i - 2]) + offs2;
            if (tmp < 0 || tmp > 15)
              continue;
            slotBitsTable[i - 2] = size_t(tmp);
          }
          for (int offs1 = (i >= 1 ? -1 : 1); offs1 <= 1; offs1++) {
            if (i >= 1) {
              int     tmp = int(bestEncodeTable[i - 1]) + offs1;
              if (tmp < 0 || tmp > 15)
                continue;
              slotBitsTable[i - 1] = size_t(tmp);
            }
            for (int offs0 = -1; offs0 <= 1; offs0++) {
              int     tmp = int(bestEncodeTable[i]) + offs0;
              if (tmp < 0 || tmp > 15)
                continue;
              slotBitsTable[i] = size_t(tmp);
              size_t  newSize =
                  calculateEncodedSize(firstSlot, firstSymbol, baseSize);
              if (newSize < bestSize) {
                bestSize = newSize;
                doneFlag = false;
                bestOffsets[0] = offs0;
                bestOffsets[1] = offs1;
                bestOffsets[2] = offs2;
              }
            }
          }
        }
        int     tmp = int(bestEncodeTable[i]) + bestOffsets[0];
        slotBitsTable[i] = size_t(tmp);
        bestEncodeTable[i] = size_t(tmp);
        if (i >= 1) {
          tmp = int(bestEncodeTable[i - 1]) + bestOffsets[1];
          slotBitsTable[i - 1] = size_t(tmp);
          bestEncodeTable[i - 1] = size_t(tmp);
        }
        if (i >= 2) {
          tmp = int(bestEncodeTable[i - 2]) + bestOffsets[2];
          slotBitsTable[i - 2] = size_t(tmp);
          bestEncodeTable[i - 2] = size_t(tmp);
        }
      }
    } while (!doneFlag);
    for (size_t i = nSlots; i-- > 0; ) {
      while (true) {
        if (slotBitsTable[i] < 1)
          break;
        slotBitsTable[i] = slotBitsTable[i] - 1;
        size_t  newSize = calculateEncodedSize();
        if (newSize > bestSize) {
          slotBitsTable[i] = slotBitsTable[i] + 1;
          break;
        }
        else {
          bestSize = newSize;
        }
      }
    }
    return bestSize;
  }

  size_t Compressor_M2::EncodeTable::optimizeSlotBitsTable()
  {
    for (size_t i = 0; i < nSlots; i++)
      slotBitsTable[i] = 0;
    if (nSymbolsUsed < 1)
      return 0;
    size_t  nSlotsD2 = (nSlots + 1) >> 1;
    std::vector< uint8_t >  slotBitsBuffer(nSlotsD2 * nSymbolsUsed, 0x00);
    std::vector< uint32_t > encodedSizeBuffer(nSymbolsUsed + 1);
    std::vector< uint8_t >  minSlotSizeTable(nSymbolsUsed, 0x00);
    for (size_t i = 0; i <= nSymbolsUsed; i++) {
      encodedSizeBuffer[i] = uint32_t(unencodedSymbolCostTable[nSymbolsUsed]
                                      - unencodedSymbolCostTable[i]);
    }
    for (size_t i = 0; i < nSymbolsUsed; i++) {
      size_t  j = 0;
      size_t  nxtSlotEnd = i + 2;
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
        case 0:
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
        if (slotBitsTable[i] < 1)
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

  void Compressor_M2::EncodeTable::updateTables(bool fastMode)
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
      symbolCntTable[nSymbolsUsed] = totalSymbolCnt;
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
        size_t  encodedSize = 0;
        if (fastMode)
          encodedSize = optimizeSlotBitsTable_fast();
        else
          encodedSize = optimizeSlotBitsTable();
        if (maxPrefixSize > minPrefixSize) {
          encodedSize += (nSlots * 4);
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
      for (size_t i = 0; i < nSlots; i++)
        slotBitsTable[i] = bestSlotBitsTable[i];
      unsigned int  baseSymbol = 0U;
      for (size_t i = 0; i < nSlots; i++) {
        slotBaseSymbolTable[i] = baseSymbol;
        unsigned int  prvBaseSymbol = baseSymbol;
        baseSymbol = prvBaseSymbol + (1U << (unsigned int) slotBitsTable[i]);
        if (baseSymbol > nSymbols)
          baseSymbol = nSymbols;
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
      prefixOnlySymbolCnt = 0;
    }
    catch (...) {
      this->clear();
      throw;
    }
  }

  void Compressor_M2::EncodeTable::clear()
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
    prefixOnlySymbolCnt = 0;
  }

  // --------------------------------------------------------------------------

  void Compressor_M2::SearchTable::sortFunc(
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
        if (len1 > Compressor_M2::maxRepeatLen)
          len1 = Compressor_M2::maxRepeatLen;
        size_t  len2 = bufSize - pos2;
        if (len2 > Compressor_M2::maxRepeatLen)
          len2 = Compressor_M2::maxRepeatLen;
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

  void Compressor_M2::SearchTable::addMatch(size_t bufPos,
                                            size_t matchPos, size_t matchLen)
  {
    size_t  offs = 0;
    if (matchLen > 0) {
      if (matchPos >= bufPos)
        return;
      offs = bufPos - matchPos;
      if (offs > Compressor_M2::maxRepeatDist)
        return;
      if (matchLen > Compressor_M2::maxRepeatLen)
        matchLen = Compressor_M2::maxRepeatLen;
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

  Compressor_M2::SearchTable::SearchTable(
      const std::vector< unsigned char >& inBuf)
    : buf(inBuf)
  {
    if (buf.size() < 1) {
      throw Ep128Emu::Exception("Compressor_M2::SearchTable::SearchTable(): "
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
        if (rleLength >= Compressor_M2::minRepeatLen)
          rleLengthTable[i + 1] = (unsigned short) rleLength;
        if (rleLength < size_t(Compressor_M2::lengthMaxValue))
          rleLength++;
      }
    }
    std::vector< unsigned short > offs1Table(
        buf.size(), (unsigned short) Compressor_M2::maxRepeatDist);
    std::vector< unsigned short > offs2Table(
        buf.size(), (unsigned short) Compressor_M2::maxRepeatDist);
    {
      std::vector< size_t > lastPosTable(65536, size_t(0x7FFFFFFF));
      // find 1-byte matches
      for (size_t i = 0; i < buf.size(); i++) {
        unsigned int  c = (unsigned int) buf[i];
        if (lastPosTable[c] < i) {
          size_t  d = i - (lastPosTable[c] + 1);
          if (d < Compressor_M2::maxRepeatDist)
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
          if (d < Compressor_M2::maxRepeatDist)
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
      if (startPos >= Compressor_M2::maxRepeatDist)
        startPos_ = startPos - Compressor_M2::maxRepeatDist;
      size_t  endPos = startPos + Compressor_M2::maxRepeatDist;
      if (endPos > buf.size() ||
          buf.size() <= (Compressor_M2::maxRepeatDist * 3)) {
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
          maxLen = (maxLen < Compressor_M2::maxRepeatLen ?
                    maxLen : Compressor_M2::maxRepeatLen);
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
          Compressor_M2::maxRepeatLen + 1,
          (unsigned long) Compressor_M2::maxRepeatDist);
      for (size_t i_ = 0; i_ < nBytes; i_++) {
        size_t  i = suffixArray[i_];
        if (i < startPos)
          continue;
        size_t  rleLen = rleLengthTable[i];
        if (rleLen > Compressor_M2::maxRepeatLen)
          rleLen = Compressor_M2::maxRepeatLen;
        size_t  minLen = Compressor_M2::minRepeatLen;
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
        unsigned long prvDist = Compressor_M2::maxRepeatDist;
        for (size_t k = maxLen; k >= Compressor_M2::minRepeatLen; k--) {
          unsigned long d = offsTable[k];
          offsTable[k] = Compressor_M2::maxRepeatDist;
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
      if (size_t(matchTableBuf[matchTable[i]]) == Compressor_M2::maxRepeatLen) {
        size_t          len = Compressor_M2::maxRepeatLen;
        unsigned short  offs = matchTableBuf[matchTable[i] + 1];
        while (--i > 0) {
          // NOTE: the offset always changes when the match length decreases
          if (matchTableBuf[matchTable[i] + 1] != offs)
            break;
          len = (len < size_t(Compressor_M2::lengthMaxValue) ? (len + 1) : len);
          matchTableBuf[matchTable[i]] = (unsigned short) len;
        }
        continue;
      }
      i--;
    }
  }

  Compressor_M2::SearchTable::~SearchTable()
  {
  }

  // --------------------------------------------------------------------------

  void Compressor_M2::writeRepeatCode(std::vector< unsigned int >& buf,
                                      size_t d, size_t n)
  {
    n = n - minRepeatLen;
    unsigned int  slotNum =
        (unsigned int) lengthEncodeTable.getSymbolSlotIndex((unsigned int) n);
    buf.push_back(((slotNum + 2U) << 24U) | ((1U << (slotNum + 2U)) - 2U));
    if (lengthEncodeTable.getSlotSize(slotNum) > 0)
      buf.push_back(lengthEncodeTable.encodeSymbol((unsigned int) n));
    d = d - minRepeatDist;
    if ((n + minRepeatLen) > 2) {
      slotNum =
          (unsigned int) offs3EncodeTable.getSymbolSlotIndex((unsigned int) d);
      buf.push_back((unsigned int) (offs3PrefixSize << 24) | slotNum);
      if (offs3EncodeTable.getSlotSize(slotNum) > 0)
        buf.push_back(offs3EncodeTable.encodeSymbol((unsigned int) d));
    }
    else if ((n + minRepeatLen) > 1) {
      slotNum =
          (unsigned int) offs2EncodeTable.getSymbolSlotIndex((unsigned int) d);
      buf.push_back((unsigned int) (offs2PrefixSize << 24) | slotNum);
      if (offs2EncodeTable.getSlotSize(slotNum) > 0)
        buf.push_back(offs2EncodeTable.encodeSymbol((unsigned int) d));
    }
    else {
      slotNum =
          (unsigned int) offs1EncodeTable.getSymbolSlotIndex((unsigned int) d);
      buf.push_back((unsigned int) (offs1PrefixSize << 24) | slotNum);
      if (offs1EncodeTable.getSlotSize(slotNum) > 0)
        buf.push_back(offs1EncodeTable.encodeSymbol((unsigned int) d));
    }
  }

  inline size_t Compressor_M2::getRepeatCodeLength(size_t d, size_t n) const
  {
    n = n - minRepeatLen;
    size_t  nBits = lengthEncodeTable.getSymbolSize(n) + 1;
    d = d - minRepeatDist;
    if ((n + minRepeatLen) > 2)
      nBits += offs3EncodeTable.getSymbolSize(d);
    else if ((n + minRepeatLen) > 1)
      nBits += offs2EncodeTable.getSymbolSize(d);
    else
      nBits += offs1EncodeTable.getSymbolSize(d);
    return nBits;
  }

  void Compressor_M2::optimizeMatches_noStats(LZMatchParameters *matchTable,
                                              size_t *bitCountTable,
                                              size_t offs, size_t nBytes)
  {
    // simplified optimal parsing code for the first optimization pass
    // when no symbol size information is available from the statistical
    // compression
    static const size_t matchSize1 = 4; // 6 if offset > 16, 8 if offset > 64
    static const size_t matchSize2 = 6; // 8 if offset > 1024
    static const size_t matchSize3 = 8;
    for (size_t i = nBytes; i-- > 0; ) {
      size_t  bestSize = 0x7FFFFFFF;
      size_t  bestLen = 1;
      size_t  bestOffs = 0;
      const unsigned short  *matchPtr = searchTable->getMatches(offs + i);
      size_t  len = matchPtr[0];        // match length
      if (len > (nBytes - i))
        len = nBytes - i;
      if (len > maxRepeatLen) {
        if (matchPtr[1] > 1) {
          // long LZ77 match
          if (size_t(matchPtr[1]) <= config.maxOffset) {
            bestSize = bitCountTable[i + len] + matchSize3;
            bestOffs = matchPtr[1];
            bestLen = len;
            len = maxRepeatLen;
          }
        }
        else {
          // if a long RLE match is possible, use that
          matchTable[i].d = 1;
          matchTable[i].len = (unsigned short) len;
          bitCountTable[i] = bitCountTable[i + len] + matchSize3;
          continue;
        }
      }
      // otherwise check all possible LZ77 match lengths,
      for ( ; len > 0; matchPtr = matchPtr + 2, len = matchPtr[0]) {
        if (len > (nBytes - i))
          len = nBytes - i;
        unsigned int  d = matchPtr[1];
        size_t        nxtLen = matchPtr[2];
        nxtLen = (nxtLen >= config.minLength ? nxtLen : (config.minLength - 1));
        if (len <= nxtLen || size_t(d) > config.maxOffset)
          continue;                     // offset is out of range, ignore match
        if (len >= 3) {
          size_t  minLenM1 = (nxtLen > 2 ? nxtLen : 2);
          do {
            size_t  nBits = bitCountTable[i + len] + matchSize3;
            if (nBits <= bestSize) {
              bestSize = nBits;
              bestOffs = d;
              bestLen = len;
            }
          } while (--len > minLenM1);
          if (nxtLen >= 2)
            continue;
          len = 2;
        }
        // check short match lengths:
        if (len == 2) {                 // 2 bytes
          if (d <= offs2MaxValue) {
            size_t  nBits = bitCountTable[i + 2] + matchSize2
                            + (size_t(d > 1024U) << 1);
            if (nBits <= bestSize) {
              bestSize = nBits;
              bestOffs = d;
              bestLen = 2;
            }
          }
          if (nxtLen >= 1)
            continue;
        }
        if (d <= offs1MaxValue) {       // 1 byte
          size_t  nBits = bitCountTable[i + 1] + matchSize1
                          + ((size_t(d > 16U) + size_t(d > 64U)) << 1);
          if (nBits <= bestSize) {
            bestSize = nBits;
            bestOffs = d;
            bestLen = 1;
          }
        }
      }
      if (bestSize >= (bitCountTable[i + 1] + 8)) {
        // literal byte,
        size_t  nBits = bitCountTable[i + 1] + 9;
        if (nBits <= bestSize) {
          bestSize = nBits;
          bestOffs = 0;
          bestLen = 1;
        }
        for (size_t k = literalSequenceMinLength;
             k <= (literalSequenceMinLength + 255) && (i + k) <= nBytes;
             k++) {
          // and all possible literal sequence lengths
          nBits = bitCountTable[i + k] + (k * 8 + literalSequenceMinLength);
          if (nBits > (bestSize + literalSequenceMinLength))
            break;      // quit the loop earlier if the data can be compressed
          if (nBits <= bestSize) {
            bestSize = nBits;
            bestOffs = 0;
            bestLen = k;
          }
        }
      }
      matchTable[i].d = (unsigned short) bestOffs;
      matchTable[i].len = (unsigned short) bestLen;
      bitCountTable[i] = bestSize;
    }
  }

  void Compressor_M2::optimizeMatches(LZMatchParameters *matchTable,
                                      size_t *bitCountTable,
                                      size_t *offsSumTable,
                                      size_t offs, size_t nBytes)
  {
    size_t  len1BitsP1 = lengthEncodeTable.getSymbolSize(1U - minRepeatLen) + 1;
    size_t  len2BitsP1 = lengthEncodeTable.getSymbolSize(2U - minRepeatLen) + 1;
    for (size_t i = nBytes; i-- > 0; ) {
      size_t  bestSize = 0x7FFFFFFF;
      size_t  bestLen = 1;
      size_t  bestOffs = 0;
      const unsigned short  *matchPtr = searchTable->getMatches(offs + i);
      size_t  len = matchPtr[0];        // match length
      if (len > (nBytes - i))
        len = nBytes - i;
      if (len > maxRepeatLen) {
        if (matchPtr[1] > 1) {
          // long LZ77 match
          if (size_t(matchPtr[1]) <= config.maxOffset) {
            bestOffs = matchPtr[1];
            bestLen = len;
            bestSize = getRepeatCodeLength(bestOffs, len)
                       + bitCountTable[i + len];
            len = maxRepeatLen;
          }
        }
        else {
          // if a long RLE match is possible, use that
          matchTable[i].d = 1;
          matchTable[i].len = (unsigned short) len;
          bitCountTable[i] = bitCountTable[i + len]
                             + getRepeatCodeLength(1, len);
          offsSumTable[i] = offsSumTable[i + len] + 1;
          continue;
        }
      }
      // otherwise check all possible LZ77 match lengths,
      for ( ; len > 0; matchPtr = matchPtr + 2, len = matchPtr[0]) {
        if (len > (nBytes - i))
          len = nBytes - i;
        unsigned int  d = matchPtr[1];
        if (size_t(d) > config.maxOffset)
          continue;                     // offset is out of range, ignore match
        if (len >= 3) {
          // flag bit + offset bits
          size_t  nBitsBase = offs3EncodeTable.getSymbolSize(
                                  d - (unsigned int) minRepeatDist) + 1;
          do {
            size_t  nBits = lengthEncodeTable.getSymbolSize(
                                (unsigned int) (len - minRepeatLen))
                            + nBitsBase + bitCountTable[i + len];
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
          } while (--len >= 3);
        }
        // check short match lengths:
        if (len == 2) {                                         // 2 bytes
          if (config.minLength <= 2 &&
              d <= offs2EncodeTable.getSymbolsEncoded()) {
            size_t  nBits = len2BitsP1 + offs2EncodeTable.getSymbolSize(
                                             d - (unsigned int) minRepeatDist)
                            + bitCountTable[i + 2];
            do {
              if (nBits > bestSize)
                break;
              if (nBits == bestSize) {
                if ((offsSumTable[i + 2] + d)
                    > (offsSumTable[i + bestLen] + bestOffs)) {
                  break;
                }
              }
              bestSize = nBits;
              bestOffs = d;
              bestLen = 2;
            } while (false);
          }
        }
        if (config.minLength <= 1 &&
            d <= offs1EncodeTable.getSymbolsEncoded()) {        // 1 byte
          size_t  nBits = len1BitsP1 + offs1EncodeTable.getSymbolSize(
                                           d - (unsigned int) minRepeatDist)
                          + bitCountTable[i + 1];
          if (nBits > bestSize)
            continue;
          if (nBits == bestSize) {
            if ((offsSumTable[i + 1] + d)
                > (offsSumTable[i + bestLen] + bestOffs)) {
              continue;
            }
          }
          bestSize = nBits;
          bestOffs = d;
          bestLen = 1;
        }
      }
      if (bestSize >= (bitCountTable[i + 1] + 8)) {
        // literal byte,
        size_t  nBits = bitCountTable[i + 1] + 9;
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
        if ((i + literalSequenceMinLength) <= nBytes &&
            (bitCountTable[i + literalSequenceMinLength]
             + (literalSequenceMinLength * 8)) <= bestSize) {
          for (size_t k = literalSequenceMinLength;
               k <= (literalSequenceMinLength + 255) && (i + k) <= nBytes;
               k++) {
            // and all possible literal sequence lengths
            nBits = bitCountTable[i + k] + (k * 8 + literalSequenceMinLength);
            if (nBits > bestSize) {
              if (nBits > (bestSize + literalSequenceMinLength))
                break;  // quit the loop earlier if the data can be compressed
              continue;
            }
            if (nBits == bestSize) {
              if (offsSumTable[i + k] > (offsSumTable[i + bestLen] + bestOffs))
                continue;
            }
            bestSize = nBits;
            bestOffs = 0;
            bestLen = k;
          }
        }
      }
      matchTable[i].d = (unsigned short) bestOffs;
      matchTable[i].len = (unsigned short) bestLen;
      bitCountTable[i] = bestSize;
      offsSumTable[i] = offsSumTable[i + bestLen] + bestOffs;
    }
  }

  size_t Compressor_M2::compressData_(std::vector< unsigned int >& tmpOutBuf,
                                      const std::vector< unsigned char >& inBuf,
                                      size_t offs, size_t nBytes,
                                      bool firstPass, bool fastMode)
  {
    size_t  endPos = offs + nBytes;
    size_t  nSymbols = 0;
    tmpOutBuf.clear();
    if (!firstPass) {
      // generate optimal encode tables for offset values
      offs1EncodeTable.updateTables(false);
      offs2EncodeTable.updateTables(false);
      offs3EncodeTable.updateTables(fastMode);
      offs3NumSlots = offs3EncodeTable.getSlotCnt();
      offs3PrefixSize = offs3EncodeTable.getSlotPrefixSize(0);
    }
    // compress data by searching for repeated byte sequences,
    // and replacing them with length/distance codes
    std::vector< LZMatchParameters >  matchTable(nBytes);
    {
      std::vector< size_t > bitCountTable(nBytes + 1, 0);
      if (!firstPass) {
        std::vector< size_t > offsSumTable(nBytes + 1, 0);
        lengthEncodeTable.setUnencodedSymbolSize(lengthNumSlots + 15);
        optimizeMatches(&(matchTable.front()), &(bitCountTable.front()),
                        &(offsSumTable.front()), offs, nBytes);
      }
      else {
        // first pass: no symbol size information is available yet
        optimizeMatches_noStats(&(matchTable.front()), &(bitCountTable.front()),
                                offs, nBytes);
      }
    }
    lengthEncodeTable.setUnencodedSymbolSize(8192);
    // generate optimal encode table for length values
    for (size_t i = offs; i < endPos; ) {
      LZMatchParameters&  tmp = matchTable[i - offs];
      if (tmp.d > 0) {
        long    unencodedCost = long(tmp.len) * 9L - 1L;
        unencodedCost -=
            (tmp.len > 1 ? long(offs2PrefixSize) : long(offs1PrefixSize));
        unencodedCost = (unencodedCost > 0L ? unencodedCost : 0L);
        lengthEncodeTable.addSymbol(tmp.len - minRepeatLen,
                                    size_t(unencodedCost));
      }
      i += size_t(tmp.len);
    }
    lengthEncodeTable.updateTables(false);
    // update LZ77 offset statistics for calculating encode tables later
    for (size_t i = offs; i < endPos; ) {
      LZMatchParameters&  tmp = matchTable[i - offs];
      if (tmp.d > 0) {
        if (lengthEncodeTable.getSymbolSize(tmp.len - minRepeatLen) <= 64) {
          long    unencodedCost = long(tmp.len) * 9L - 1L;
          unencodedCost -=
              long(lengthEncodeTable.getSymbolSize(tmp.len - minRepeatLen));
          unencodedCost = (unencodedCost > 0L ? unencodedCost : 0L);
          if (tmp.len > 2) {
            offs3EncodeTable.addSymbol(tmp.d - minRepeatDist,
                                       size_t(unencodedCost));
          }
          else if (tmp.len > 1) {
            offs2EncodeTable.addSymbol(tmp.d - minRepeatDist,
                                       size_t(unencodedCost));
          }
          else {
            offs1EncodeTable.addSymbol(tmp.d - minRepeatDist,
                                       size_t(unencodedCost));
          }
        }
      }
      i += size_t(tmp.len);
    }
    // first pass: there are no offset encode tables yet, so no data is written
    if (firstPass)
      return 0;
    // write encode tables
    tmpOutBuf.push_back(0x02000000U | (unsigned int) (offs3PrefixSize - 2));
    for (size_t i = 0; i < lengthNumSlots; i++) {
      unsigned int  c = (unsigned int) lengthEncodeTable.getSlotSize(i);
      tmpOutBuf.push_back(0x04000000U | c);
    }
    for (size_t i = 0; i < offs1NumSlots; i++) {
      unsigned int  c = (unsigned int) offs1EncodeTable.getSlotSize(i);
      tmpOutBuf.push_back(0x04000000U | c);
    }
    for (size_t i = 0; i < offs2NumSlots; i++) {
      unsigned int  c = (unsigned int) offs2EncodeTable.getSlotSize(i);
      tmpOutBuf.push_back(0x04000000U | c);
    }
    for (size_t i = 0; i < offs3NumSlots; i++) {
      unsigned int  c = (unsigned int) offs3EncodeTable.getSlotSize(i);
      tmpOutBuf.push_back(0x04000000U | c);
    }
    // write compressed data
    for (size_t i = offs; i < endPos; ) {
      LZMatchParameters&  tmp = matchTable[i - offs];
      if (tmp.d > 0) {
        // check if this match needs to be replaced with literals:
        size_t  nBits = getRepeatCodeLength(tmp.d, tmp.len);
        if (nBits > 64) {
          // if the match cannot be encoded, assume "infinite" size
          nBits = 0x7FFFFFFF;
        }
        if ((size_t(tmp.len) >= literalSequenceMinLength &&
             nBits > (literalSequenceMinLength + (size_t(tmp.len) * 8))) ||
            nBits >= (size_t(tmp.len) * 9)) {
          tmp.d = 0;
        }
      }
      if (tmp.d > 0) {
        // write LZ77 match
        writeRepeatCode(tmpOutBuf, tmp.d, tmp.len);
        i = i + tmp.len;
        nSymbols++;
      }
      else {
        while (size_t(tmp.len) >= literalSequenceMinLength) {
          // write literal sequence
          size_t  len = tmp.len;
          len = (len < (literalSequenceMinLength + 255) ?
                 len : (literalSequenceMinLength + 255));
          tmpOutBuf.push_back((unsigned int) ((lengthNumSlots + 1) << 24)
                              | ((1U << (unsigned int) (lengthNumSlots + 1))
                                 - 1U));
          tmpOutBuf.push_back(0x08000000U
                              | (unsigned int) (len
                                                - literalSequenceMinLength));
          for (size_t j = 0; j < len; j++) {
            tmpOutBuf.push_back(0x88000000U | (unsigned int) inBuf[i]);
            i++;
          }
          nSymbols++;
          tmp.len -= (unsigned short) len;
        }
        while (tmp.len > 0) {
          // write literal byte(s)
          tmpOutBuf.push_back(0x01000000U);
          tmpOutBuf.push_back(0x88000000U | (unsigned int) inBuf[i]);
          i++;
          nSymbols++;
          tmp.len--;
        }
      }
    }
    return nSymbols;
  }

  bool Compressor_M2::compressData(std::vector< unsigned int >& tmpOutBuf,
                                   const std::vector< unsigned char >& inBuf,
                                   unsigned int startAddr, bool isLastBlock,
                                   size_t offs, size_t nBytes, bool fastMode)
  {
    // the 'offs' and 'nBytes' parameters allow compressing a buffer
    // as multiple chunks for possibly improved statistical compression
    if (nBytes < 1 || offs >= inBuf.size())
      return true;
    if (nBytes > (inBuf.size() - offs))
      nBytes = inBuf.size() - offs;
    size_t  endPos = offs + nBytes;
    lengthEncodeTable.clear();
    offs1EncodeTable.clear();
    offs2EncodeTable.clear();
    offs3EncodeTable.clear();
    std::vector< uint64_t >     hashTable;
    std::vector< unsigned int > bestBuf;
    std::vector< unsigned int > tmpBuf;
    const size_t  headerSize = (startAddr < 0x80000000U ? 34 : 18);
    size_t  bestSize = 0x7FFFFFFF;
    size_t  nSymbols = 0;
    bool    doneFlag = false;
    for (size_t i = 0; i < config.optimizeIterations; i++) {
      if (progressDisplayEnabled) {
        if (!setProgressPercentage(int(progressCnt * 100 / progressMax)))
          return false;
        progressCnt++;
      }
      if (doneFlag)     // if the compression cannot be optimized further,
        continue;       // quit the loop earlier
      tmpBuf.clear();
      size_t  tmp =
          compressData_(tmpBuf, inBuf, offs, nBytes, (i == 0), fastMode);
      if (i == 0)       // the first optimization pass writes no data
        continue;
      // calculate compressed size and hash value
      size_t    compressedSize = headerSize;
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
        nSymbols = tmp;
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
    size_t  uncompressedSize = headerSize + (nBytes * 8);
    if (startAddr < 0x80000000U)
      tmpOutBuf.push_back(0x10000000U | (startAddr + (unsigned int) offs));
    if (bestSize >= uncompressedSize) {
      // if cannot reduce the data size, store without compression
      tmpOutBuf.push_back(0x10000000U | (unsigned int) (nBytes - 1));
      tmpOutBuf.push_back(0x01000000U | (unsigned int) isLastBlock);
      tmpOutBuf.push_back(0x01000000U);
      for (size_t i = offs; i < endPos; i++)
        tmpOutBuf.push_back(0x88000000U | (unsigned int) inBuf[i]);
    }
    else {
      tmpOutBuf.push_back(0x10000000U | (unsigned int) (nSymbols - 1));
      tmpOutBuf.push_back(0x01000000U | (unsigned int) isLastBlock);
      tmpOutBuf.push_back(0x01000001U);
      // append compressed data to output buffer
      for (size_t i = 0; i < bestBuf.size(); i++)
        tmpOutBuf.push_back(bestBuf[i]);
    }
    return true;
  }

  // --------------------------------------------------------------------------

  Compressor_M2::Compressor_M2(std::vector< unsigned char >& outBuf_)
    : Compressor(outBuf_),
      lengthEncodeTable(lengthNumSlots, lengthMaxValue,
                        &(lengthPrefixSizeTable[0])),
      offs1EncodeTable(offs1NumSlots, offs1MaxValue, (size_t *) 0,
                       offs1PrefixSize),
      offs2EncodeTable(offs2NumSlots, offs2MaxValue, (size_t *) 0,
                       offs2PrefixSize),
      offs3EncodeTable(0, offs3MaxValue, (size_t *) 0,
                       2, 5, &(offs3SlotCntTable[0])),
      offs3NumSlots(4),
      offs3PrefixSize(2),
      searchTable((SearchTable *) 0),
      savedOutBufPos(0x7FFFFFFF),
      outputShiftReg(0x00),
      outputBitCnt(0)
  {
  }

  Compressor_M2::~Compressor_M2()
  {
    if (searchTable)
      delete searchTable;
  }

  bool Compressor_M2::compressData(const std::vector< unsigned char >& inBuf,
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
        searchTable = (SearchTable *) 0;
      }
      searchTable = new SearchTable(inBuf);
      // split large files to improve statistical compression
      std::list< SplitOptimizationBlock >   splitPositions;
      std::map< uint64_t, size_t >          splitOptimizationCache;
      size_t  splitDepth = config.splitOptimizationDepth - 1;
      {
        while (inBuf.size() > (size_t(1) << (splitDepth + 16)))
          splitDepth++;         // limit block size to 64K
        size_t  splitCnt = size_t(1) << splitDepth;
        if (splitCnt > inBuf.size())
          splitCnt = inBuf.size();
        progressCnt = 0;
        progressMax = splitCnt
                      + (splitCnt > 1 ? (splitCnt - 1) : 0)
                      + (splitCnt > 2 ? (splitCnt - 2) : 0)
                      + (splitCnt > 3 ? (splitCnt - 3) : 0);
        progressMax = progressMax * config.optimizeIterations;
        progressMax =
            progressMax * ((splitDepth / 2) + 2) / ((splitDepth / 2) + 1);
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
          splitPositions.push_back(tmpBlock);
        }
      }
      while (config.blockSize < 1) {
        size_t  bestMergePos = 0;
        long    bestMergeBits = 0x7FFFFFFFL;
        // find the pair of blocks that reduce the total compressed size
        // the most when merged
        std::list< SplitOptimizationBlock >::iterator curBlock =
            splitPositions.begin();
        while (curBlock != splitPositions.end()) {
          std::list< SplitOptimizationBlock >::iterator nxtBlock = curBlock;
          nxtBlock++;
          if (nxtBlock == splitPositions.end())
            break;
          if (((*curBlock).nBytes + (*nxtBlock).nBytes) > 65536) {
            curBlock++;
            continue;                   // limit block size to <= 64K
          }
          size_t  nBitsSplit = 0;
          size_t  nBitsMerged = 0;
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
              std::vector< unsigned int > tmpBuf;
              if (!compressData(tmpBuf, inBuf, startAddr, false,
                                startPos, endPos - startPos, true)) {
                delete searchTable;
                searchTable = (SearchTable *) 0;
                if (progressDisplayEnabled)
                  progressMessage("");
                return false;
              }
              // calculate compressed size
              size_t  nBits = 0;
              for (size_t j = 0; j < tmpBuf.size(); j++)
                nBits += size_t((tmpBuf[j] & 0x7F000000U) >> 24);
              splitOptimizationCache[cacheKey] = nBits;
            }
            size_t  nBits = splitOptimizationCache[cacheKey];
            switch (i) {
            case 0:
              nBitsMerged = nBits;
              break;
            default:
              nBitsSplit += nBits;
              break;
            }
          }
          // calculate size change when merging blocks
          long    sizeDiff = long(nBitsMerged) - long(nBitsSplit);
          if (sizeDiff < bestMergeBits) {
            bestMergePos = (*curBlock).startPos;
            bestMergeBits = sizeDiff;
          }
          curBlock++;
        }
        if (bestMergeBits > 0L)         // no more blocks can be merged
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
      // compress all blocks again with full optimization
      {
        size_t  progressPercentage = 0;
        if (progressCnt > 0 && progressMax > 0) {
          progressPercentage = (progressCnt * 100) / progressMax;
          if (progressPercentage > 85)
            progressPercentage = 85;
        }
        size_t  tmp = config.optimizeIterations * splitPositions.size();
        progressCnt = (tmp * progressPercentage) / (100 - progressPercentage);
        progressMax = progressCnt + tmp;
      }
      std::vector< unsigned int >   outBufTmp;
      std::list< SplitOptimizationBlock >::iterator i_ = splitPositions.begin();
      while (i_ != splitPositions.end()) {
        std::vector< unsigned int > tmpBuf;
        if (!compressData(tmpBuf, inBuf, startAddr,
                          (isLastBlock &&
                           ((*i_).startPos + (*i_).nBytes) >= inBuf.size()),
                          (*i_).startPos, (*i_).nBytes, false)) {
          delete searchTable;
          searchTable = (SearchTable *) 0;
          if (progressDisplayEnabled)
            progressMessage("");
          return false;
        }
        for (size_t i = 0; i < tmpBuf.size(); i++)
          outBufTmp.push_back(tmpBuf[i]);
        i_++;
      }
      delete searchTable;
      searchTable = (SearchTable *) 0;
      if (progressDisplayEnabled) {
        setProgressPercentage(100);
        progressMessage("");
      }
      // pack output data
      if (outBuf.size() == 0)
        outBuf.push_back((unsigned char) 0x00); // reserve space for CRC value
      for (size_t i = 0; i < outBufTmp.size(); i++) {
        unsigned int  c = outBufTmp[i];
        if (c >= 0x80000000U) {
          // special case for literal bytes, which are stored byte-aligned
          if (outputBitCnt > 0 && savedOutBufPos >= outBuf.size()) {
            // reserve space for the shift register to be stored later when
            // it is full, and save the write position
            savedOutBufPos = outBuf.size();
            outBuf.push_back((unsigned char) 0x00);
          }
          unsigned int  nBytes = ((c & 0x7F000000U) + 0x07000000U) >> 27;
          while (nBytes > 0U) {
            nBytes--;
            outBuf.push_back((unsigned char) ((c >> (nBytes * 8U)) & 0xFFU));
          }
        }
        else {
          unsigned int  nBits = c >> 24;
          c = c & 0x00FFFFFFU;
          for (unsigned int j = nBits; j > 0U; ) {
            j--;
            unsigned int  b = (unsigned int) (bool(c & (1U << j)));
            outputShiftReg = ((outputShiftReg & 0x7F) << 1) | (unsigned char) b;
            if (++outputBitCnt >= 8) {
              if (savedOutBufPos >= outBuf.size()) {
                outBuf.push_back(outputShiftReg);
              }
              else {
                // store at saved position if any literal bytes were inserted
                outBuf[savedOutBufPos] = outputShiftReg;
                savedOutBufPos = 0x7FFFFFFF;
              }
              outputShiftReg = 0x00;
              outputBitCnt = 0;
            }
          }
        }
      }
      if (isLastBlock) {
        while (outputBitCnt != 0) {
          outputShiftReg = ((outputShiftReg & 0x7F) << 1);
          if (++outputBitCnt >= 8) {
            if (savedOutBufPos >= outBuf.size()) {
              outBuf.push_back(outputShiftReg);
            }
            else {
              // store at saved position if any literal bytes were inserted
              outBuf[savedOutBufPos] = outputShiftReg;
              savedOutBufPos = 0x7FFFFFFF;
            }
            outputShiftReg = 0x00;
            outputBitCnt = 0;
          }
        }
        // calculate CRC
        unsigned char crcVal = 0xFF;
        for (size_t i = outBuf.size() - 1; i > 0; i--) {
          unsigned int  tmp = (unsigned int) crcVal ^ (unsigned int) outBuf[i];
          tmp = ((tmp << 1) + ((tmp & 0x80U) >> 7) + 0xACU) & 0xFFU;
          crcVal = (unsigned char) tmp;
        }
        crcVal = (unsigned char) ((0x0180 - 0xAC) >> 1) ^ crcVal;
        outBuf[0] = crcVal;
        outputBitCnt = -1;              // set output buffer closed flag
      }
    }
    catch (...) {
      if (searchTable) {
        delete searchTable;
        searchTable = (SearchTable *) 0;
      }
      if (progressDisplayEnabled)
        progressMessage("");
      throw;
    }
    return true;
  }

}       // namespace Ep128Compress

