
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
#include "decompress0.hpp"
#include <vector>

namespace Ep128Compress {

  unsigned int Decompressor_M0::readBits(size_t nBits)
  {
    unsigned int  retval = 0U;
    for (size_t i = 0; i < nBits; i++) {
      if (shiftRegisterCnt < 1) {
        if (inputBufferPosition >= inputBufferSize)
          throw Ep128Emu::Exception("unexpected end of compressed data");
        shiftRegister = inputBuffer[inputBufferPosition];
        shiftRegisterCnt = 8;
        inputBufferPosition++;
      }
      retval = (retval << 1) | (unsigned int) ((shiftRegister >> 7) & 0x01);
      shiftRegister = (shiftRegister & 0x7F) << 1;
      shiftRegisterCnt--;
    }
    return retval;
  }

  unsigned int Decompressor_M0::readLZMatchParameterBits(unsigned char n)
  {
    if (n < 0x08)
      return (unsigned int) n;
    if (n >= 0x3C)
      throw Ep128Emu::Exception("error in compressed data");
    unsigned int  retval = (unsigned int) ((n & 0x03) | 0x04);
    unsigned int  nBits = (unsigned int) (n >> 2) - 1U;
    retval = (retval << nBits) | readBits(nBits);
    return retval;
  }

  unsigned int Decompressor_M0::gammaDecode()
  {
    unsigned int  retval = 1U;
    while (readBits(1) != 0U) {
      retval = (retval << 1) | readBits(1);
      if (retval > 325U)
        throw Ep128Emu::Exception("error in compressed data");
    }
    return retval;
  }

  unsigned int Decompressor_M0::huffmanDecode(int huffTable)
  {
    if (huffTable == 0 && !usingHuffTable0)
      return readBits(9);
    if (huffTable != 0 && !usingHuffTable1)
      return readBits(5);
    int     tmp = 0;
    int     cnt = -1;
    const unsigned int  *symCntTable =
        (huffTable == 0 ? huffmanSymCntTable0 : huffmanSymCntTable1);
    const unsigned int  *offsetTable =
        (huffTable == 0 ? huffmanOffsetTable0 : huffmanOffsetTable1);
    const unsigned int  *decodeTable =
        (huffTable == 0 ? huffmanDecodeTable0 : huffmanDecodeTable1);
    do {
      if (++cnt >= 16)
        throw Ep128Emu::Exception("error in compressed data");
      tmp = ((tmp << 1) | int(readBits(1))) - int(symCntTable[cnt]);
    } while (tmp >= 0);
    tmp = tmp + int(offsetTable[cnt]);
    if (decodeTable[tmp] == 0xFFFFFFFFU)
      throw Ep128Emu::Exception("error in compressed data");
    return decodeTable[tmp];
  }

  unsigned char Decompressor_M0::readDeltaValue()
  {
    unsigned char retval = (unsigned char) readBits(7);
    if (retval < 0x40)
      retval = retval + 0xC0;
    else
      retval = retval - 0x3F;
    return retval;
  }

  unsigned int Decompressor_M0::readMatchLength()
  {
    return (readLZMatchParameterBits((unsigned char) huffmanDecode(1)) + 2U);
  }

  void Decompressor_M0::huffmanInit(int huffTable)
  {
    bool    symbolsUsed[324];
    unsigned int  *symCntTable =
        (huffTable == 0 ? huffmanSymCntTable0 : huffmanSymCntTable1);
    unsigned int  *offsetTable =
        (huffTable == 0 ? huffmanOffsetTable0 : huffmanOffsetTable1);
    unsigned int  *decodeTable =
        (huffTable == 0 ? huffmanDecodeTable0 : huffmanDecodeTable1);
    size_t  nSymbols = (huffTable == 0 ? 324 : 28);
    // clear tables
    for (size_t i = 0; i < 16; i++) {
      symCntTable[i] = 0U;
      offsetTable[i] = (unsigned int) nSymbols;
    }
    for (size_t i = 0; i < nSymbols; i++) {
      decodeTable[i] = 0xFFFFFFFFU;
      symbolsUsed[i] = false;
    }
    size_t  tablePos = 0;
    bool    huffmanEnabled = bool(readBits(1));
    if (huffTable == 0)
      usingHuffTable0 = huffmanEnabled;
    else
      usingHuffTable1 = huffmanEnabled;
    if (!huffmanEnabled)
      return;                   // Huffman coding is disabled, nothing to do
    for (size_t i = 0; i < 16; i++) {
      size_t  cnt = gammaDecode() - 1U;
      if (cnt > nSymbols)
        throw Ep128Emu::Exception("error in compressed data");
      unsigned int  decodedValue = 0U - 1U;
      for (size_t j = 0; j < cnt; j++) {
        decodedValue = decodedValue + gammaDecode();
        if (decodedValue >= (unsigned int) nSymbols || tablePos >= nSymbols ||
            symbolsUsed[decodedValue]) {
          throw Ep128Emu::Exception("error in compressed data");
        }
        decodeTable[tablePos] = decodedValue;
        tablePos++;
        symbolsUsed[decodedValue] = true;
      }
      symCntTable[i] = (unsigned int) cnt;
      offsetTable[i] = (unsigned int) tablePos;
    }
  }

  bool Decompressor_M0::decompressDataBlock(std::vector< unsigned char >& buf,
                                            std::vector< bool >& bytesUsed,
                                            unsigned int& startAddr)
  {
    bool    allowWrap = false;
    if (startAddr >= 0x80000000U)
      startAddr = readBits(16);
    else
      allowWrap = true;         // "raw" data can be larger than 64K
    unsigned int  nBytes = (readBits(16) ^ 0xFFFFU) + 1U;
    bool    isLastBlock = readBits(1);
    bool    compressionEnabled = readBits(1);
    if (!compressionEnabled) {
      // copy uncompressed data
      for (unsigned int i = 0U; i < nBytes; i++) {
        if (startAddr > 0xFFFFU || (bytesUsed[startAddr] && !allowWrap))
          throw Ep128Emu::Exception("error in compressed data");
        buf[startAddr] = (unsigned char) readBits(8);
        bytesUsed[startAddr] = true;
        startAddr = (startAddr + 1U) & (allowWrap ? 0x0000FFFFU : 0xFFFFFFFFU);
      }
      return isLastBlock;
    }
    huffmanInit(0);
    huffmanInit(1);
    prvMatchOffsetsPos = 0U;
    for (int i = 0; i < 4; i++)         // clear recent match offsets buffer
      prvMatchOffsets[i] = 0xFFFFFFFFU;
    for (unsigned int i = 0U; i < nBytes; ) {
      unsigned int  tmp = huffmanDecode(0);
      if (tmp >= 0x0144U)
        throw Ep128Emu::Exception("error in compressed data");
      if (tmp <= 0xFFU) {
        // literal character
        if (startAddr > 0xFFFFU || (bytesUsed[startAddr] && !allowWrap))
          throw Ep128Emu::Exception("error in compressed data");
        buf[startAddr] = (unsigned char) tmp;
        bytesUsed[startAddr] = true;
        startAddr = (startAddr + 1U) & (allowWrap ? 0x0000FFFFU : 0xFFFFFFFFU);
        i++;
        continue;
      }
      unsigned int  offs = 0U;
      unsigned char deltaValue = 0x00;
      if (tmp >= 0x013CU) {
        // check for special match codes:
        if (tmp < 0x0140U) {
          // match with delta value
          deltaValue = readDeltaValue();
          offs = tmp & 0x03U;
        }
        else {
          // use recent match offset
          offs = prvMatchOffsets[(prvMatchOffsetsPos + tmp) & 3U];
        }
      }
      else if (tmp < 0x0108) {
        // short match offset
        offs = tmp & 0x07U;
      }
      else {
        offs = readLZMatchParameterBits((unsigned char) (tmp & 0xFFU));
        // store in recent offsets buffer
        prvMatchOffsetsPos = (prvMatchOffsetsPos - 1U) & 3U;
        prvMatchOffsets[prvMatchOffsetsPos] = offs;
      }
      if (offs > 0xFFFFU)
        throw Ep128Emu::Exception("error in compressed data");
      offs++;
      unsigned int  lzMatchReadAddr = (startAddr - offs) & 0xFFFFU;
      unsigned int  matchLength = readMatchLength();
      for (unsigned int j = 0U; j < matchLength; j++) {
        if (i >= nBytes)                // block should not end within a match
          throw Ep128Emu::Exception("error in compressed data");
        if (!bytesUsed[lzMatchReadAddr])    // byte does not exist yet
          throw Ep128Emu::Exception("error in compressed data");
        if (startAddr > 0xFFFFU || (bytesUsed[startAddr] && !allowWrap))
          throw Ep128Emu::Exception("error in compressed data");
        buf[startAddr] = (buf[lzMatchReadAddr] + deltaValue) & 0xFF;
        bytesUsed[startAddr] = true;
        startAddr = (startAddr + 1U) & (allowWrap ? 0x0000FFFFU : 0xFFFFFFFFU);
        lzMatchReadAddr = (lzMatchReadAddr + 1U) & 0xFFFFU;
        i++;
      }
    }
    return isLastBlock;
  }

  // --------------------------------------------------------------------------

  Decompressor_M0::Decompressor_M0()
    : Decompressor(),
      huffmanSymCntTable0((unsigned int *) 0),
      huffmanOffsetTable0((unsigned int *) 0),
      huffmanDecodeTable0((unsigned int *) 0),
      huffmanSymCntTable1((unsigned int *) 0),
      huffmanOffsetTable1((unsigned int *) 0),
      huffmanDecodeTable1((unsigned int *) 0),
      usingHuffTable0(false),
      usingHuffTable1(false),
      shiftRegister(0x00),
      shiftRegisterCnt(0),
      inputBuffer((unsigned char *) 0),
      inputBufferSize(0),
      inputBufferPosition(0),
      prvMatchOffsetsPos(0U)
  {
    size_t  totalTableSize = 16 + 16 + 324 + 16 + 16 + 28;
    huffmanSymCntTable0 = new unsigned int[totalTableSize];
    for (size_t i = 0; i < totalTableSize; i++)
      huffmanSymCntTable0[i] = 0U;
    huffmanOffsetTable0 = &(huffmanSymCntTable0[16]);
    huffmanDecodeTable0 = &(huffmanOffsetTable0[16]);
    huffmanSymCntTable1 = &(huffmanDecodeTable0[324]);
    huffmanOffsetTable1 = &(huffmanSymCntTable1[16]);
    huffmanDecodeTable1 = &(huffmanOffsetTable1[16]);
    for (size_t i = 0; i < 4; i++)
      prvMatchOffsets[i] = 0xFFFFFFFFU;
  }

  Decompressor_M0::~Decompressor_M0()
  {
    delete[] huffmanSymCntTable0;
  }

  void Decompressor_M0::decompressData(
      std::vector< std::vector< unsigned char > >& outBuf,
      const std::vector< unsigned char >& inBuf)
  {
    outBuf.clear();
    if (inBuf.size() < 1)
      return;
    // verify checksum
    unsigned char chkSum = 0x00;
    for (size_t i = inBuf.size(); i-- > 0; ) {
      chkSum = chkSum ^ inBuf[i];
      chkSum = ((chkSum & 0x7F) << 1) | ((chkSum & 0x80) >> 7);
      chkSum = (chkSum + 0xC4) & 0xFF;
    }
    if (chkSum != 0x80)
      throw Ep128Emu::Exception("error in compressed data");
    // decompress all data blocks
    inputBuffer = &(inBuf.front());
    inputBufferSize = inBuf.size();
    inputBufferPosition = 1;
    shiftRegister = 0x00;
    shiftRegisterCnt = 0;
    std::vector< unsigned char >  tmpBuf(65536, 0x00);
    std::vector< bool > bytesUsed(65536, false);
    while (true) {
      unsigned int  startAddr = 0xFFFFFFFFU;
      if (decompressDataBlock(tmpBuf, bytesUsed, startAddr))
        break;
    }
    // on successful decompression, all input data must be consumed
    if (!(inputBufferPosition >= inputBufferSize && shiftRegister == 0x00))
      throw Ep128Emu::Exception("error in compressed data");
    unsigned int  startPos = 0U;
    size_t        nBytes = 0;
    // find all data blocks
    for (size_t i = 0; i <= 65536; i++) {
      if (i >= 65536 || !bytesUsed[i]) {
        if (nBytes > 0) {
          std::vector< unsigned char >  newBuf;
          newBuf.push_back((unsigned char) (startPos & 0xFFU));
          newBuf.push_back((unsigned char) ((startPos >> 8) & 0xFFU));
          for (size_t j = 0U; j < nBytes; j++)
            newBuf.push_back(tmpBuf[startPos + j]);
          outBuf.push_back(newBuf);
        }
        startPos = (unsigned int) (i + 1);
        nBytes = 0;
      }
      else {
        nBytes++;
      }
    }
  }

  void Decompressor_M0::decompressData(
      std::vector< unsigned char >& outBuf,
      const std::vector< unsigned char >& inBuf)
  {
    outBuf.clear();
    if (inBuf.size() < 1)
      return;
    // verify checksum
    unsigned char chkSum = 0x00;
    for (size_t i = inBuf.size(); i-- > 0; ) {
      chkSum = chkSum ^ inBuf[i];
      chkSum = ((chkSum & 0x7F) << 1) | ((chkSum & 0x80) >> 7);
      chkSum = (chkSum + 0xC4) & 0xFF;
    }
    if (chkSum != 0x80)
      throw Ep128Emu::Exception("error in compressed data");
    // decompress all data blocks
    inputBuffer = &(inBuf.front());
    inputBufferSize = inBuf.size();
    inputBufferPosition = 1;
    shiftRegister = 0x00;
    shiftRegisterCnt = 0;
    std::vector< unsigned char >  tmpBuf(65536, 0x00);
    std::vector< bool > bytesUsed(65536, false);
    unsigned int  startAddr = 0U;
    bool          isLastBlock = false;
    do {
      unsigned int  prvStartAddr = startAddr;
      isLastBlock = decompressDataBlock(tmpBuf, bytesUsed, startAddr);
      unsigned int  j = prvStartAddr;
      do {
        outBuf.push_back(tmpBuf[j]);
        j = (j + 1U) & 0xFFFFU;
      } while (j != startAddr);
    } while (!isLastBlock);
    // on successful decompression, all input data must be consumed
    if (!(inputBufferPosition >= inputBufferSize && shiftRegister == 0x00))
      throw Ep128Emu::Exception("error in compressed data");
  }

}       // namespace Ep128Compress

