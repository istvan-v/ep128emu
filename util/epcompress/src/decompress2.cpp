
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
#include "decompress2.hpp"
#include "decompm2.hpp"
#include <vector>

namespace Ep128Compress {

  unsigned int Decompressor_M2::readBits(size_t nBits)
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

  unsigned char Decompressor_M2::readLiteralByte()
  {
    if (inputBufferPosition >= inputBufferSize)
      throw Ep128Emu::Exception("unexpected end of compressed data");
    unsigned char retval = inputBuffer[inputBufferPosition];
    inputBufferPosition++;
    return retval;
  }

  unsigned int Decompressor_M2::readMatchLength()
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

  unsigned int Decompressor_M2::readLZMatchParameter(
      unsigned char slotNum, const unsigned int *decodeTable)
  {
    unsigned int  retval = decodeTable[int(slotNum) * 2 + 1];
    retval += readBits(size_t(decodeTable[int(slotNum) * 2 + 0]));
    return retval;
  }

  void Decompressor_M2::readDecodeTables()
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

  bool Decompressor_M2::decompressDataBlock(std::vector< unsigned char >& buf,
                                            std::vector< bool >& bytesUsed,
                                            unsigned int& startAddr)
  {
    startAddr = readBits(16);
    unsigned int  nSymbols = readBits(16) + 1U;
    bool    isLastBlock = readBits(1);
    bool    compressionEnabled = readBits(1);
    if (!compressionEnabled) {
      // copy uncompressed data
      for (unsigned int i = 0U; i < nSymbols; i++) {
        if (startAddr > 0xFFFFU || bytesUsed[startAddr])
          throw Ep128Emu::Exception("error in compressed data");
        buf[startAddr] = readLiteralByte();
        bytesUsed[startAddr] = true;
        startAddr++;
      }
      return isLastBlock;
    }
    readDecodeTables();
    for (unsigned int i = 0U; i < nSymbols; i++) {
      unsigned int  matchLength = readMatchLength();
      if (matchLength == 0U) {
        // literal byte
        if (startAddr > 0xFFFFU || bytesUsed[startAddr])
          throw Ep128Emu::Exception("error in compressed data");
        buf[startAddr] = readLiteralByte();
        bytesUsed[startAddr] = true;
        startAddr++;
      }
      else if (matchLength >= 0x80000000U) {
        // literal sequence
        matchLength &= 0x7FFFFFFFU;
        while (matchLength > 0U) {
          if (startAddr > 0xFFFFU || bytesUsed[startAddr])
            throw Ep128Emu::Exception("error in compressed data");
          buf[startAddr] = readLiteralByte();
          bytesUsed[startAddr] = true;
          startAddr++;
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
        if (offs >= 0xFFFFU)
          throw Ep128Emu::Exception("error in compressed data");
        offs++;
        unsigned int  lzMatchReadAddr = (startAddr - offs) & 0xFFFFU;
        for (unsigned int j = 0U; j < matchLength; j++) {
          if (!bytesUsed[lzMatchReadAddr])  // byte does not exist yet
            throw Ep128Emu::Exception("error in compressed data");
          if (startAddr > 0xFFFFU || bytesUsed[startAddr])
            throw Ep128Emu::Exception("error in compressed data");
          buf[startAddr] = buf[lzMatchReadAddr];
          bytesUsed[startAddr] = true;
          startAddr++;
          lzMatchReadAddr = (lzMatchReadAddr + 1U) & 0xFFFFU;
        }
      }
    }
    return isLastBlock;
  }

  // --------------------------------------------------------------------------

  Decompressor_M2::Decompressor_M2()
    : Decompressor(),
      offs3PrefixSize(2),
      shiftRegister(0x00),
      shiftRegisterCnt(0),
      inputBuffer((unsigned char *) 0),
      inputBufferSize(0),
      inputBufferPosition(0)
  {
  }

  Decompressor_M2::~Decompressor_M2()
  {
  }

  void Decompressor_M2::decompressData(
      std::vector< std::vector< unsigned char > >& outBuf,
      const std::vector< unsigned char >& inBuf)
  {
    outBuf.clear();
    if (inBuf.size() < 1)
      return;
    // verify checksum
    unsigned char crcValue = 0xFF;
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

  void Decompressor_M2::decompressData(
      std::vector< unsigned char >& outBuf,
      const std::vector< unsigned char >& inBuf)
  {
    Ep128Emu::decompressData(outBuf, &(inBuf.front()), inBuf.size());
  }

}       // namespace Ep128Compress

