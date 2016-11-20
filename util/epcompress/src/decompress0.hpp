
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

#ifndef EPCOMPRESS_DECOMPRESS0_HPP
#define EPCOMPRESS_DECOMPRESS0_HPP

#include "ep128emu.hpp"
#include "compress.hpp"
#include <vector>

namespace Ep128Compress {

  class Decompressor_M0 : public Decompressor {
   private:
    // main Huffman table (literal characters + match offset codes)
    unsigned int  *huffmanSymCntTable0;
    unsigned int  *huffmanOffsetTable0;
    unsigned int  *huffmanDecodeTable0;
    // match length Huffman table
    unsigned int  *huffmanSymCntTable1;
    unsigned int  *huffmanOffsetTable1;
    unsigned int  *huffmanDecodeTable1;
    bool          usingHuffTable0;
    bool          usingHuffTable1;
    unsigned char shiftRegister;
    int           shiftRegisterCnt;
    const unsigned char *inputBuffer;
    size_t        inputBufferSize;
    size_t        inputBufferPosition;
    unsigned int  prvMatchOffsets[4];
    unsigned int  prvMatchOffsetsPos;
    // --------
    unsigned int readBits(size_t nBits);
    unsigned int readLZMatchParameterBits(unsigned char n);
    unsigned int gammaDecode();
    unsigned int huffmanDecode(int huffTable);
    unsigned char readDeltaValue();
    unsigned int readMatchLength();
    void huffmanInit(int huffTable);
    // if 'startAddr' is 0xFFFFFFFF, it is read from the compressed data
    bool decompressDataBlock(std::vector< unsigned char >& buf,
                             std::vector< bool >& bytesUsed,
                             unsigned int& startAddr);
   public:
    Decompressor_M0();
    virtual ~Decompressor_M0();
    virtual void decompressData(
        std::vector< std::vector< unsigned char > >& outBuf,
        const std::vector< unsigned char >& inBuf);
    // this function assumes "raw" compressed data blocks with no start address
    virtual void decompressData(
        std::vector< unsigned char >& outBuf,
        const std::vector< unsigned char >& inBuf);
  };

}       // namespace Ep128Compress

#endif  // EPCOMPRESS_DECOMPRESS0_HPP

