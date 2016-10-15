
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

#include "ep128emu.hpp"
#include "decompm2.hpp"

namespace Ep128Emu {

  unsigned int Decompressor::readBits(size_t nBits)
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

  unsigned char Decompressor::readLiteralByte()
  {
    if (inputBufferPosition >= inputBufferSize)
      throw Ep128Emu::Exception("unexpected end of compressed data");
    unsigned char retval = inputBuffer[inputBufferPosition];
    inputBufferPosition++;
    return retval;
  }

  unsigned int Decompressor::readMatchLength()
  {
    unsigned int  slotNum = 0U;
    do {
      if (readBits(1) == 0U)
        break;
      slotNum++;
    } while (slotNum < 9U);
    if (slotNum == 0U)                  // literal byte
      return 0U;
    if (slotNum == 9U)                  // literal sequence
      return (readBits(8) + 0x80000011U);
    return (readLZMatchParameter((unsigned char) (slotNum - 1U),
                                 &(lengthDecodeTable[0])) + 1U);
  }

  unsigned int Decompressor::readLZMatchParameter(
      unsigned char slotNum, const unsigned int *decodeTable)
  {
    unsigned int  retval = decodeTable[int(slotNum) * 2 + 1];
    retval += readBits(size_t(decodeTable[int(slotNum) * 2 + 0]));
    return retval;
  }

  void Decompressor::readDecodeTables()
  {
    unsigned int  tmp = 0U;
    unsigned int  *tablePtr = &(lengthDecodeTable[0]);
    offs3PrefixSize = size_t(readBits(2)) + 2;
    size_t  offs3NumSlots = size_t(1) << offs3PrefixSize;
    for (size_t i = 0; i < (8 + 4 + 8 + offs3NumSlots); i++) {
      if (i == 8) {
        tmp = 0U;
        tablePtr = &(offs1DecodeTable[0]);
      }
      else if (i == (8 + 4)) {
        tmp = 0U;
        tablePtr = &(offs2DecodeTable[0]);
      }
      else if (i == (8 + 4 + 8)) {
        tmp = 0U;
        tablePtr = &(offs3DecodeTable[0]);
      }
      tablePtr[0] = readBits(4);
      tablePtr[1] = tmp;
      tmp = tmp + (1U << tablePtr[0]);
      tablePtr = tablePtr + 2;
    }
  }

  bool Decompressor::decompressDataBlock(std::vector< unsigned char >& buf,
                                         std::vector< bool >& bytesUsed,
                                         unsigned int& startAddr)
  {
    unsigned int  nSymbols = readBits(16) + 1U;
    bool    isLastBlock = readBits(1);
    bool    compressionEnabled = readBits(1);
    if (!compressionEnabled) {
      // copy uncompressed data
      for (unsigned int i = 0U; i < nSymbols; i++) {
        buf[startAddr] = readLiteralByte();
        bytesUsed[startAddr] = true;
        startAddr = (startAddr + 1U) & 0x0003FFFFU;
      }
      return isLastBlock;
    }
    readDecodeTables();
    for (unsigned int i = 0U; i < nSymbols; i++) {
      unsigned int  matchLength = readMatchLength();
      if (matchLength == 0U) {
        // literal byte
        buf[startAddr] = readLiteralByte();
        bytesUsed[startAddr] = true;
        startAddr = (startAddr + 1U) & 0x0003FFFFU;
      }
      else if (matchLength >= 0x80000000U) {
        // literal sequence
        matchLength &= 0x7FFFFFFFU;
        while (matchLength > 0U) {
          buf[startAddr] = readLiteralByte();
          bytesUsed[startAddr] = true;
          startAddr = (startAddr + 1U) & 0x0003FFFFU;
          matchLength--;
        }
      }
      else {
        if (matchLength > 65535U)
          throw Ep128Emu::Exception("error in compressed data");
        // get match offset:
        unsigned int  offs = 0U;
        if (matchLength == 1U) {
          unsigned int  slotNum = readBits(2);
          offs = readLZMatchParameter((unsigned char) slotNum,
                                      &(offs1DecodeTable[0]));
        }
        else if (matchLength == 2U) {
          unsigned int  slotNum = readBits(3);
          offs = readLZMatchParameter((unsigned char) slotNum,
                                      &(offs2DecodeTable[0]));
        }
        else {
          unsigned int  slotNum = readBits(offs3PrefixSize);
          offs = readLZMatchParameter((unsigned char) slotNum,
                                      &(offs3DecodeTable[0]));
        }
        if (offs >= 0x00040000U)
          throw Ep128Emu::Exception("error in compressed data");
        offs++;
        unsigned int  lzMatchReadAddr = (startAddr - offs) & 0x0003FFFFU;
        for (unsigned int j = 0U; j < matchLength; j++) {
          if (!bytesUsed[lzMatchReadAddr])      // byte does not exist yet
            throw Ep128Emu::Exception("error in compressed data");
          buf[startAddr] = buf[lzMatchReadAddr];
          bytesUsed[startAddr] = true;
          startAddr = (startAddr + 1U) & 0x0003FFFFU;
          lzMatchReadAddr = (lzMatchReadAddr + 1U) & 0x0003FFFFU;
        }
      }
    }
    return isLastBlock;
  }

  Decompressor::Decompressor()
    : offs3PrefixSize(2),
      shiftRegister(0x00),
      shiftRegisterCnt(0),
      inputBuffer((unsigned char *) 0),
      inputBufferSize(0),
      inputBufferPosition(0)
  {
  }

  Decompressor::~Decompressor()
  {
  }

  void Decompressor::decompressData(
      std::vector< unsigned char >& outBuf,
      const std::vector< unsigned char >& inBuf)
  {
    outBuf.clear();
    if (inBuf.size() < 1)
      return;
    unsigned char crcValue = 0xFF;
    // verify checksum
    for (size_t i = inBuf.size(); i-- > 0; ) {
      crcValue = crcValue ^ inBuf[i];
      crcValue = ((crcValue & 0x7F) << 1) | ((crcValue & 0x80) >> 7);
      crcValue = (crcValue + 0xAC) & 0xFF;
    }
    if (crcValue != 0x80)
      throw Ep128Emu::Exception("error in compressed data");
    // decompress all data blocks
    inputBuffer = &(inBuf.front());
    inputBufferSize = inBuf.size();
    inputBufferPosition = 1;
    shiftRegister = 0x00;
    shiftRegisterCnt = 0;
    std::vector< unsigned char >  tmpBuf(262144, 0x00);
    std::vector< bool > bytesUsed(262144, false);
    unsigned int  startAddr = 0U;
    bool          isLastBlock = false;
    do {
      unsigned int  prvStartAddr = startAddr;
      isLastBlock = decompressDataBlock(tmpBuf, bytesUsed, startAddr);
      unsigned int  j = prvStartAddr;
      do {
        outBuf.push_back(tmpBuf[j]);
        j = (j + 1U) & 0x0003FFFFU;
      } while (j != startAddr);
      if (outBuf.size() > 16777216)
        throw Ep128Emu::Exception("error in compressed data");
    } while (!isLastBlock);
    // on successful decompression, all input data must be consumed
    if (!(inputBufferPosition >= inputBufferSize && shiftRegister == 0x00))
      throw Ep128Emu::Exception("error in compressed data");
  }

}       // namespace Ep128Emu

