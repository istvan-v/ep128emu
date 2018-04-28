
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
#include "decompress6.hpp"
#include <vector>

namespace Ep128Compress {

  M6Encoder::M6Encoder()
  {
    parseEncoding((char *) 0);
  }

  M6Encoder::~M6Encoder()
  {
  }

  const char * M6Encoder::parseEncoding(
      std::vector< unsigned int >& encodeTable,
      std::vector< unsigned int >& decodeTable,
      size_t& prefixBits, const char *s)
  {
    prefixBits = 0;
    encodeTable.clear();
    decodeTable.clear();
    bool    isLength = (&encodeTable == &lengthEncodeTable);
    if (s[0] >= 'I' && s[0] <= 'X' && (s[1] == ',' || s[1] == '\0')) {
      // flat encoding ('I' = 1 bit .. 'X' = 16 bits)
      if (isLength)
        throw Ep128Emu::Exception("invalid encoding definition");
      prefixBits = size_t(s[0] - 'H');
      size_t  n = size_t(1) << prefixBits;
      encodeTable.resize((n + 1) << 1, 0U);
      for (size_t i = 1; i <= n; i++) {
        encodeTable[i << 1] =
            0x80000000U | (unsigned int) ((prefixBits << 24) | (i - 1));
      }
      return (s + 1);
    }

    size_t  nSlots = 0;
    for ( ; s[nSlots] != ',' && s[nSlots] != '\0'; nSlots++) {
      if (!((s[nSlots] >= '0' && s[nSlots] <= '9') ||
            (s[nSlots] >= 'A' && s[nSlots] <= 'F'))) {
        throw Ep128Emu::Exception("invalid encoding definition");
      }
    }
    if (nSlots < 2 || nSlots > (isLength ? 23 : 64))
      throw Ep128Emu::Exception("invalid encoding definition");
    while ((size_t(1) << prefixBits) < nSlots)
      prefixBits++;
    encodeTable.resize(2, 0U);
    decodeTable.resize(nSlots << 1, 0U);

    unsigned int  n = 1U;
    for (size_t i = 0; i < nSlots; i++) {
      unsigned char nBits = (unsigned char) s[i];
      if (nBits >= '0' && nBits <= '9')
        nBits = nBits - '0';
      else
        nBits = (nBits - 'A') + 10;
      decodeTable[i << 1] = nBits;
      decodeTable[(i << 1) + 1] = n;
      for (unsigned int j = 0U; j < (1U << nBits); j++) {
        if (!isLength)
          encodeTable.push_back((unsigned int) ((prefixBits << 24) | i));
        else if (!flagBitsInverted)
          encodeTable.push_back((unsigned int) ((i + 1) << 24) | 0x00000001U);
        else
          encodeTable.push_back((unsigned int) ((i + 1) << 24) | 0x007FFFFEU);
        encodeTable.push_back(((unsigned int) nBits << 24) | j);
      }
      n = n + (1U << nBits);
    }

    return (s + nSlots);
  }

  void M6Encoder::parseEncoding(const char *s)
  {
    std::vector< char > t;
    if (!s)
      s = "";
    for ( ; *s != '\0'; s++) {
      char    c = *s;
      if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\032')
        continue;
      if (c >= 'a' && c <= 'z')
        c = c - ('a' - 'A');
      t.push_back(c);
    }
    if (t.size() < 1) {
      parseEncoding(",G,,12345678,0123456789ABCDEF");
      return;
    }
    reverseDirection = false;
    rightShifting = false;
    literals9Bit = false;
    flagBitsInverted = false;
    shiftReg = 0x00;
    shiftRegBits = 0;
    prvLZ77Flag = false;
    bufPos = 0;
    minLength = 4;
    maxLength = 0;
    maxOffs1 = 0;
    maxOffs2 = 0;
    maxOffs3 = 0;
    t.push_back('\0');

    for (s = &(t.front()); *s != ','; s++) {
      char    c = *s;
      switch (c) {
      case '0':
        flagBitsInverted = false;
        break;
      case '1':
        flagBitsInverted = true;
        break;
      case '9':
        literals9Bit = true;
        break;
      case 'B':
        reverseDirection = true;
        break;
      case 'F':
        reverseDirection = false;
        break;
      case 'G':
        literals9Bit = false;
        break;
      case 'L':
        rightShifting = false;
        break;
      case 'R':
        rightShifting = true;
        break;
      default:
        throw Ep128Emu::Exception("invalid encoding definition");
      }
    }
    s++;

    if (s[0] == 'G' && s[1] == ',') {
      parseEncoding(lengthEncodeTable, lengthDecodeTable, lengthBits,
                    "0123456789ABCDEF");
      s = s + 2;
    }
    else {
      s = parseEncoding(lengthEncodeTable, lengthDecodeTable, lengthBits, s);
      if (*s != ',')
        throw Ep128Emu::Exception("invalid encoding definition");
      s++;
    }

    if (*s != ',')
      s = parseEncoding(offs1EncodeTable, offs1DecodeTable, offs1Bits, s);
    if (*s != ',')
      throw Ep128Emu::Exception("invalid encoding definition");
    s++;

    if (*s != ',')
      s = parseEncoding(offs2EncodeTable, offs2DecodeTable, offs2Bits, s);
    if (*s != ',')
      throw Ep128Emu::Exception("invalid encoding definition");
    s++;

    s = parseEncoding(offs3EncodeTable, offs3DecodeTable, offs3Bits, s);

    if (*s != '\0' ||
        lengthEncodeTable.size() < 8 || lengthDecodeTable.size() > 46 ||
        offs3EncodeTable.size() < 4) {
      throw Ep128Emu::Exception("invalid encoding definition");
    }

    minLength = 3;
    if (offs2EncodeTable.size() >= 4) {
      maxOffs2 = (offs2EncodeTable.size() >> 1) - 1;
      minLength = 2;
    }
    if (offs1EncodeTable.size() >= 4) {
      if (minLength > 2)
        throw Ep128Emu::Exception("invalid encoding definition");
      maxOffs1 = (offs1EncodeTable.size() >> 1) - 1;
      minLength = 1;
    }
    maxLength = (lengthEncodeTable.size() >> 1) + minLength - 2;
    maxOffs3 = (offs3EncodeTable.size() >> 1) - 1;
  }

  void M6Encoder::reverseBuffer(std::vector< unsigned char >& tmpBuf)
  {
    if (reverseDirection && tmpBuf.size() > 1) {
      size_t  i = 0;
      size_t  j = tmpBuf.size() - 1;
      for ( ; i < j; i++, j--) {
        unsigned char tmp = tmpBuf[i];
        tmpBuf[i] = tmpBuf[j];
        tmpBuf[j] = tmp;
      }
    }
  }

  unsigned char M6Encoder::readByte()
  {
    if (bufPos >= buf.size())
      throw Ep128Emu::Exception("unexpected end of compressed data");
    unsigned char b = buf[bufPos];
    bufPos++;
    return b;
  }

  unsigned int M6Encoder::readBits(size_t nBits)
  {
    unsigned int b = 0U;
    while (nBits-- > 0) {
      if (!shiftRegBits) {
        shiftReg = readByte();
        shiftRegBits = 8;
      }
      if (!rightShifting) {
        b = (b << 1) | (unsigned int) ((shiftReg >> 7) & 1);
        shiftReg = shiftReg << 1;
      }
      else {
        b = (b << 1) | (unsigned int) (shiftReg & 1);
        shiftReg = shiftReg >> 1;
      }
      shiftRegBits--;
    }
    return b;
  }

  int M6Encoder::readLength()
  {
    if (literals9Bit) {
      if (bufPos > 0)
        prvLZ77Flag = (readBits(1) == (unsigned int) flagBitsInverted);
      if (!prvLZ77Flag)
        return -1;
    }
    else {
      if (!prvLZ77Flag)
        prvLZ77Flag = (bufPos > 0);
      else
        prvLZ77Flag = (readBits(1) == (unsigned int) flagBitsInverted);
    }
    if (!prvLZ77Flag) {
      // literal sequence with gamma encoded length
      size_t  n = 0;
      while (readBits(1) == (unsigned int) flagBitsInverted) {
        if (++n >= 28)
          throw Ep128Emu::Exception("error in compressed data");
      }
      unsigned int  l = (1U << (unsigned char) n) | readBits(n);
      return -(int(l));
    }
    size_t  n = 0;
    while (readBits(1) == (unsigned int) flagBitsInverted) {
      if (++n >= (lengthDecodeTable.size() >> 1))
        return 0;                       // end of data
    }
    // decode length
    unsigned int  l = lengthDecodeTable[(n << 1) + 1]
                      + readBits(lengthDecodeTable[n << 1])
                      + (unsigned int) (minLength - 1);
    return int(l);
  }

  size_t M6Encoder::decodeOffset(const std::vector< unsigned int >& decodeTable,
                                 size_t prefixBits)
  {
    if (decodeTable.size() < 1) {
      // flat encoding
      unsigned int  n = 0U;
      if (prefixBits & 7)
        n = readBits(prefixBits & 7);
      for ( ; prefixBits >= 8; prefixBits = prefixBits - 8)
        n = (n << 8) | readByte();
      return size_t(n + 1U);
    }
    size_t  n = readBits(prefixBits);
    if (n >= (decodeTable.size() >> 1))
      throw Ep128Emu::Exception("error in compressed data");
    return size_t(decodeTable[(n << 1) + 1] + readBits(decodeTable[n << 1]));
  }

  size_t M6Encoder::readOffset(size_t len)
  {
    if (len < 3) {
      if (len < 2)
        return decodeOffset(offs1DecodeTable, offs1Bits);
      return decodeOffset(offs2DecodeTable, offs2Bits);
    }
    return decodeOffset(offs3DecodeTable, offs3Bits);
  }

  bool M6Encoder::isEOF() const
  {
    if (bufPos < buf.size())
      return false;
    unsigned char b = shiftReg;
    for (unsigned char i = shiftRegBits; i > 0; i--) {
      if (!rightShifting) {
        if (bool(b & 0x80) != flagBitsInverted)
          return false;
        b = b << 1;
      }
      else {
        if (bool(b & 0x01) != flagBitsInverted)
          return false;
        b = b >> 1;
      }
    }
    return true;
  }

  void M6Encoder::writeByte(unsigned char b)
  {
    buf.push_back(b);
    if (shiftRegBits < 1)
      bufPos = buf.size();
  }

  void M6Encoder::writeBits(unsigned int b)
  {
    unsigned char nBits = (unsigned char) ((b & 0x7F000000U) >> 24);
    if (b & 0x80000000U) {
      // literal byte(s)
      if (nBits & 7) {
        writeBits(((unsigned int) (nBits & 7) << 24)
                  | ((b & 0x00FFFFFFU) >> (nBits & 0x78)));
      }
      nBits = nBits & 0x78;
      while (nBits >= 8) {
        nBits = nBits - 8;
        writeByte((unsigned char) ((b >> nBits) & 0xFFU));
      }
      return;
    }
    while (nBits-- > 0) {
      if (shiftRegBits < 1 && bufPos >= buf.size())
        buf.push_back(0x00);
      if (!rightShifting)
        shiftReg = (shiftReg << 1) | (unsigned char) ((b >> nBits) & 1U);
      else
        shiftReg = (shiftReg >> 1) | (unsigned char) (((b >> nBits) & 1U) << 7);
      if (++shiftRegBits >= 8) {
        if (bufPos >= buf.size())
          writeByte(shiftReg);
        else
          buf[bufPos] = shiftReg;
        bufPos = buf.size();
        shiftReg = 0x00;
        shiftRegBits = 0;
      }
    }
  }

  void M6Encoder::writeLiteralSequence(const unsigned char *tmpBuf,
                                       size_t nBytes)
  {
    if (nBytes < 1 || !tmpBuf)
      return;
    if (prvLZ77Flag && !literals9Bit)
      writeBits(0x01000000U | (unsigned int) (!flagBitsInverted));
    prvLZ77Flag = false;
    if (literals9Bit) {
      for (size_t i = 0; i < nBytes; i++) {
        if (bufPos > 0)
          writeBits(0x01000000U | (unsigned int) (!flagBitsInverted));
        writeByte(tmpBuf[i]);
      }
    }
    else {
      unsigned char nBits = 0;
      for ( ; nBytes >= (size_t(1) << (nBits + 1)); nBits++)
        ;
      if (!flagBitsInverted)
        writeBits(((unsigned int) (nBits + 1) << 24) | 0x00000001U);
      else
        writeBits(((unsigned int) (nBits + 1) << 24) | 0x007FFFFEU);
      writeBits(((unsigned int) nBits << 24) | nBytes);
      for (size_t i = 0; i < nBytes; i++)
        writeByte(tmpBuf[i]);
    }
  }

  void M6Encoder::writeLZ77Sequence(size_t len, size_t offs)
  {
    if (literals9Bit || prvLZ77Flag)
      writeBits(0x01000000U | (unsigned int) flagBitsInverted);
    prvLZ77Flag = true;
    if (len < 1) {
      // end of data
      unsigned int  b = (unsigned int) (lengthDecodeTable.size() >> 1) << 24;
      if (!flagBitsInverted)
        writeBits(b);
      else
        writeBits(b | 0x00FFFFFFU);
      return;
    }
    // write length
    writeBits(lengthEncodeTable[(len - (minLength - 1)) << 1]);
    writeBits(lengthEncodeTable[((len - (minLength - 1)) << 1) + 1]);
    // write offset
    if (len < 2) {
      writeBits(offs1EncodeTable[offs << 1]);
      writeBits(offs1EncodeTable[(offs << 1) + 1]);
    }
    else if (len < 3) {
      writeBits(offs2EncodeTable[offs << 1]);
      writeBits(offs2EncodeTable[(offs << 1) + 1]);
    }
    else {
      writeBits(offs3EncodeTable[offs << 1]);
      writeBits(offs3EncodeTable[(offs << 1) + 1]);
    }
  }

  void M6Encoder::flush()
  {
    while (shiftRegBits > 0)
      writeBits(0x01000000U | (unsigned int) flagBitsInverted);
  }

  void M6Encoder::clear()
  {
    buf.clear();
    bufPos = 0;
    shiftReg = 0x00;
    shiftRegBits = 0x00;
    prvLZ77Flag = false;
  }

  // --------------------------------------------------------------------------

  Decompressor_M6::Decompressor_M6()
    : Decompressor()
  {
  }

  Decompressor_M6::~Decompressor_M6()
  {
  }

  void Decompressor_M6::decompressData(
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

  void Decompressor_M6::decompressData(
      std::vector< unsigned char >& outBuf,
      const std::vector< unsigned char >& inBuf)
  {
    outBuf.clear();
    if (inBuf.size() < 1) {
      throw Ep128Emu::Exception("Decompressor_M6::decompressData(): "
                                "insufficient input data size");
    }
    decoder.clear();
    decoder.buf.insert(decoder.buf.end(), inBuf.begin(), inBuf.end());
    decoder.reverseBuffer();
    while (true) {
      int     len = decoder.readLength();
      if (!len)
        break;
      if (outBuf.size() >= 0x00100000) {
        throw Ep128Emu::Exception("Decompressor_M6::decompressData(): "
                                  "invalid uncompressed data size");
      }
      if (len < 0) {
        // literal sequence
        do {
          outBuf.push_back(decoder.readByte());
        } while (++len < 0);
        continue;
      }
      size_t  offs = decoder.readOffset(size_t(len));
      if (!(offs >= 1 && offs <= outBuf.size())) {
        throw Ep128Emu::Exception("Decompressor_M6::decompressData(): "
                                  "error in compressed data");
      }
      do {
        outBuf.push_back(outBuf[outBuf.size() - offs]);
      } while (--len);
    }
    if (!decoder.isEOF()) {
      throw Ep128Emu::Exception("Decompressor_M6::decompressData(): "
                                "more input data than expected");
    }
    decoder.reverseBuffer(outBuf);
  }

  void Decompressor_M6::setEncoding(const char *s)
  {
    decoder.parseEncoding(s);
  }

}       // namespace Ep128Compress

