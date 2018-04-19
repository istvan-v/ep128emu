
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
#include "decompress1.hpp"
#include <vector>

namespace Ep128Compress {

  unsigned int Decompressor_M1::readBits(size_t nBits)
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

  unsigned char Decompressor_M1::readLiteralByte()
  {
    if (inputBufferPosition >= inputBufferSize)
      throw Ep128Emu::Exception("unexpected end of compressed data");
    unsigned char retval = inputBuffer[inputBufferPosition];
    inputBufferPosition++;
    return retval;
  }

  unsigned int Decompressor_M1::readMatchLength()
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
      return ((unsigned int) readLiteralByte() + 0x80000010U);
    return (readLZMatchParameter((unsigned char) (slotNum - 1U),
                                 &(lengthDecodeTable[0])) + 1U);
  }

  unsigned int Decompressor_M1::readLZMatchParameter(
      unsigned char slotNum, const unsigned int *decodeTable)
  {
    unsigned int  retval = decodeTable[int(slotNum) * 2 + 1];
    retval += readBits(size_t(decodeTable[int(slotNum) * 2 + 0]));
    return retval;
  }

  void Decompressor_M1::readDecodeTables()
  {
    unsigned int  tmp = 0U;
    unsigned int  *tablePtr = &(offs1DecodeTable[0]);
    offs3PrefixSize = size_t(readBits(2)) + 2;
    size_t  offs3NumSlots = (size_t(1) << offs3PrefixSize) - 1;
    for (size_t i = 0; i < (8 + 4 + 8 + offs3NumSlots); i++) {
      if (i == 4) {
        tmp = 0U;
        tablePtr = &(lengthDecodeTable[0]);
      }
      else if (i == (4 + 8)) {
        tmp = 0U;
        tablePtr = &(offs2DecodeTable[0]);
      }
      else if (i == (4 + 8 + 8)) {
        tmp = 0U;
        tablePtr = &(offs3DecodeTable[0]);
      }
      tablePtr[0] = readBits(4);
      tablePtr[1] = tmp;
      tmp = tmp + (1U << tablePtr[0]);
      tablePtr = tablePtr + 2;
    }
  }

  bool Decompressor_M1::decompressDataBlock(std::vector< unsigned char >& buf)
  {
    readDecodeTables();
    size_t  bufSize = buf.size();
    while (true) {
      if ((buf.size() - bufSize) > 65536)
        throw Ep128Emu::Exception("error in compressed data");
      unsigned int  matchLength = readMatchLength();
      if (matchLength == 0U) {
        // literal byte
        buf.push_back(readLiteralByte());
      }
      else if (matchLength >= 0x80000000U) {
        // literal sequence
        matchLength &= 0x7FFFFFFFU;
        if (matchLength < 18U)
          return bool(17U - matchLength);
        while (matchLength > 0U) {
          buf.push_back(readLiteralByte());
          matchLength--;
        }
      }
      else {
        if (matchLength > 65535U)
          throw Ep128Emu::Exception("error in compressed data");
        // get match offset:
        unsigned int  offs = 0U;
        unsigned char d = 0;
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
          if (!slotNum) {
            // 5-bit offset and delta value
            offs = 31U - readBits(5);
            d = readLiteralByte();
          }
          else {
            slotNum--;
            offs = readLZMatchParameter((unsigned char) slotNum,
                                        &(offs3DecodeTable[0]));
          }
        }
        if (offs >= buf.size())
          throw Ep128Emu::Exception("error in compressed data");
        offs++;
        for (unsigned int j = 0U; j < matchLength; j++)
          buf.push_back((buf[buf.size() - offs] + d) & 0xFF);
      }
    }
    return true;        // not reached
  }

  // --------------------------------------------------------------------------

  Decompressor_M1::Decompressor_M1()
    : Decompressor(),
      offs3PrefixSize(2),
      shiftRegister(0x00),
      shiftRegisterCnt(0),
      inputBuffer((unsigned char *) 0),
      inputBufferSize(0),
      inputBufferPosition(0)
  {
  }

  Decompressor_M1::~Decompressor_M1()
  {
  }

  void Decompressor_M1::decompressData(
      std::vector< std::vector< unsigned char > >& outBuf,
      const std::vector< unsigned char >& inBuf)
  {
    outBuf.clear();
    std::vector< unsigned char >  tmpOutBuf;
    decompressData(tmpOutBuf, inBuf);
    // NOTE: this format does not support start addresses,
    // so use a fixed value of 0100H (program with EXOS 5 header)
    tmpOutBuf.insert(tmpOutBuf.begin(), 2, (unsigned char) 0x00);
    tmpOutBuf[1] = 0x01;
    outBuf.push_back(tmpOutBuf);
  }

  void Decompressor_M1::decompressData(
      std::vector< unsigned char >& outBuf,
      const std::vector< unsigned char >& inBuf)
  {
    outBuf.clear();
    if (inBuf.size() < 1)
      return;
    // decompress all data blocks
    inputBuffer = &(inBuf.front());
    inputBufferSize = inBuf.size();
    inputBufferPosition = 0;
    shiftRegister = 0x00;
    shiftRegisterCnt = 0;
    while (!decompressDataBlock(outBuf))
      ;
    // on successful decompression, all input data must be consumed
    if (!(inputBufferPosition >= inputBufferSize && shiftRegister == 0x00))
      throw Ep128Emu::Exception("error in compressed data");
  }

}       // namespace Ep128Compress

