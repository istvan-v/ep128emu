
// compressor utility for Enterprise 128 programs
// Copyright (C) 2007-2019 Istvan Varga <istvanv@users.sourceforge.net>
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

#include <vector>

static const size_t       extLoaderSize = 42;
static const unsigned int extDecompressorAddr_M3 = 0xFF84U;

#include "sfxcode.cpp"

namespace Ep128Compress {

  void addSFXModule(std::vector< unsigned char >& outBuf,
                    const std::vector< unsigned char >& inBuf,
                    int compressionType,
                    bool noBorderFX, bool noCleanup, bool noCRCCheck,
                    bool isExtension)
  {
    unsigned int  uncompressedDataEndAddr = 0U;
    // decompress data to a temporary buffer, and calculate uncompressed size
    if (isExtension) {
      std::vector< unsigned char >  tmpBuf;
      compressionType = decompressData(tmpBuf, inBuf, compressionType);
      uncompressedDataEndAddr = 0xC00AU + (unsigned int) tmpBuf.size();
    }
    else {
      std::vector< std::vector< unsigned char > > tmpBuf;
      compressionType = decompressData(tmpBuf, inBuf, compressionType);
      for (size_t i = 0; i < tmpBuf.size(); i++) {
        if (tmpBuf[i].size() < 3)
          continue;
        unsigned int  endAddr =
            (unsigned int) ((size_t(tmpBuf[i][0]) | (size_t(tmpBuf[i][1]) << 8))
                            + (tmpBuf[i].size() - 2));
        if (endAddr > uncompressedDataEndAddr)
          uncompressedDataEndAddr = endAddr;
      }
    }
    const unsigned char *sfxCodePtr = (unsigned char *) 0;
    size_t  sfxCodeSize = 0;
    size_t  sfxLoaderSize = extLoaderSize;
    if (isExtension) {
      if (compressionType == 3) {
        sfxCodePtr = &(epEXTSFXModule_M3[0]);
        sfxCodeSize = sizeof(epEXTSFXModule_M3) / sizeof(unsigned char);
      }
    }
    else if (compressionType == 3) {
      if (!noCleanup) {
        if (!noBorderFX) {
          sfxCodePtr = &(epSFXModule_M3_0[0]);
          sfxCodeSize = sizeof(epSFXModule_M3_0) / sizeof(unsigned char);
        }
        else {
          sfxCodePtr = &(epSFXModule_M3_1[0]);
          sfxCodeSize = sizeof(epSFXModule_M3_1) / sizeof(unsigned char);
        }
      }
      else {
        if (!noBorderFX) {
          sfxCodePtr = &(epSFXModule_M3_2[0]);
          sfxCodeSize = sizeof(epSFXModule_M3_2) / sizeof(unsigned char);
        }
        else {
          sfxCodePtr = &(epSFXModule_M3_3[0]);
          sfxCodeSize = sizeof(epSFXModule_M3_3) / sizeof(unsigned char);
        }
      }
    }
    if (!sfxCodePtr)
      throw Ep128Emu::Exception("invalid compression type or SFX parameters");
    if (isExtension) {
      if ((sfxCodeSize + inBuf.size()) > 0x3FF6)
        throw Ep128Emu::Exception("SFX program compressed size is too large");
      if (uncompressedDataEndAddr >= (extDecompressorAddr_M3 + 1))
        throw Ep128Emu::Exception("SFX program uncompressed size is too large");
    }
    else {
      sfxLoaderSize = size_t(sfxCodePtr[0]) | (size_t(sfxCodePtr[1]) << 8);
      sfxCodePtr = sfxCodePtr + 2;
      sfxCodeSize = sfxCodeSize - 2;
      if ((sfxCodeSize + inBuf.size()) > 0xBF00)
        throw Ep128Emu::Exception("SFX program compressed size is too large");
      if (uncompressedDataEndAddr >= 0x00010001U)
        throw Ep128Emu::Exception("SFX program uncompressed size is too large");
    }
    // write EXOS header
    outBuf.clear();
    outBuf.resize(16, 0x00);
    outBuf[1] = (unsigned char) (int(isExtension) + 0x05);
    outBuf[2] = (unsigned char) ((sfxCodeSize + inBuf.size()) & 0xFF);
    outBuf[3] = (unsigned char) ((sfxCodeSize + inBuf.size()) >> 8);
    // write loader code
    for (size_t i = 0; i < sfxLoaderSize; i++)
      outBuf.push_back(sfxCodePtr[i]);
    if (!isExtension) {
      outBuf[outBuf.size() - 4] =
          (unsigned char) (uncompressedDataEndAddr & 0xFFU);
      outBuf[outBuf.size() - 3] =
          (unsigned char) (uncompressedDataEndAddr >> 8);
    }
    outBuf[outBuf.size() - 2] = (unsigned char) (inBuf.size() & 0xFF);
    outBuf[outBuf.size() - 1] = (unsigned char) (inBuf.size() >> 8);
    // write compressed data
    outBuf.insert(outBuf.end(), inBuf.begin(), inBuf.end());
    // write main decompressor code
    for (size_t i = sfxLoaderSize; i < sfxCodeSize; i++)
      outBuf.push_back(sfxCodePtr[i]);
  }

  int decompressSFXProgram(std::vector< unsigned char >& outBuf,
                           const std::vector< unsigned char >& inBuf,
                           int compressionType, bool isExtension)
  {
    outBuf.clear();
    size_t  offs = 0;
    size_t  nBytes = inBuf.size();
    if (nBytes >= 16) {
      if (inBuf[0] == 0x00 &&
          inBuf[1] == (unsigned char) (int(isExtension) + 0x05)) {
        offs = 16;
        nBytes = nBytes - 16;
        size_t  tmp = size_t(inBuf[2]) | (size_t(inBuf[3]) << 8);
        if (nBytes > tmp)
          nBytes = tmp;
      }
    }
    size_t  sfxLoaderSize = extLoaderSize;
    if (!isExtension) {
      sfxLoaderSize = 0x7FFFFFFF;
      for (size_t i = 80; (i + 8) <= inBuf.size(); i++) {
        if (inBuf[i] == 0x0E && inBuf[i + 1] == 0x80 &&         // LD C, 80H
            inBuf[i + 2] == 0x18 && inBuf[i + 3] == 0xFA) {     // JR -6
          sfxLoaderSize = i + 8;
          break;
        }
      }
    }
    if (nBytes <= sfxLoaderSize) {
      throw Ep128Emu::Exception("decompressSFXProgram(): input data is not "
                                "an SFX program");
    }
    if (isExtension) {
      // decompress self-extracting extension module (EXOS file type 6)
      size_t  compressedSize = size_t(inBuf[offs + extLoaderSize - 2])
                               | (size_t(inBuf[offs + extLoaderSize - 1]) << 8);
      if (nBytes < (compressedSize + 160) ||
          nBytes > (compressedSize + 1024)) {
        throw Ep128Emu::Exception("decompressSFXProgram(): input data is not "
                                  "an SFX program");
      }
      offs = offs + extLoaderSize;
      nBytes = compressedSize;
      std::vector< unsigned char >  tmpBuf;
      {
        std::vector< unsigned char >  tmpBuf2;
        tmpBuf2.insert(tmpBuf2.end(),
                       inBuf.begin() + offs, inBuf.begin() + (offs + nBytes));
        compressionType = decompressData(tmpBuf, tmpBuf2, compressionType);
      }
      size_t  uncompressedSize = tmpBuf.size();
      if (uncompressedSize < 1 || uncompressedSize > 0x3FF6) {
        throw Ep128Emu::Exception("decompressSFXProgram(): input data is not "
                                  "an SFX program");
      }
      outBuf.resize(uncompressedSize + 16, 0x00);
      outBuf[1] = 0x06;
      outBuf[2] = (unsigned char) (uncompressedSize & 0xFF);
      outBuf[3] = (unsigned char) (uncompressedSize >> 8);
      for (size_t i = 0; i < uncompressedSize; i++)
        outBuf[i + 16] = tmpBuf[i];
      return compressionType;
    }
    // decompress self-extracting program (EXOS file type 5)
    size_t  compressedSize = size_t(inBuf[offs + sfxLoaderSize - 2])
                             | (size_t(inBuf[offs + sfxLoaderSize - 1]) << 8);
    size_t  uncompressedSize = size_t(inBuf[offs + sfxLoaderSize - 4])
                               | (size_t(inBuf[offs + sfxLoaderSize - 3]) << 8);
    if (nBytes < (compressedSize + 208) || nBytes > (compressedSize + 1024) ||
        uncompressedSize <= 256) {
      throw Ep128Emu::Exception("decompressSFXProgram(): input data is not "
                                "an SFX program");
    }
    uncompressedSize = uncompressedSize - 256;
    offs = offs + sfxLoaderSize;
    nBytes = compressedSize;
    std::vector< std::vector< unsigned char > > tmpBuf;
    {
      std::vector< unsigned char >  tmpBuf2;
      tmpBuf2.insert(tmpBuf2.end(),
                     inBuf.begin() + offs, inBuf.begin() + (offs + nBytes));
      compressionType = decompressData(tmpBuf, tmpBuf2, compressionType);
    }
    outBuf.resize(uncompressedSize + 16, 0x00);
    outBuf[1] = 0x05;
    outBuf[2] = (unsigned char) (uncompressedSize & 0xFF);
    outBuf[3] = (unsigned char) (uncompressedSize >> 8);
    for (size_t i = 0; i < tmpBuf.size(); i++) {
      if (tmpBuf[i].size() < 2) {
        throw Ep128Emu::Exception("decompressSFXProgram(): input data is not "
                                  "an SFX program");
      }
      size_t  startAddr = size_t(tmpBuf[i][0]) | (size_t(tmpBuf[i][1]) << 8);
      for (size_t j = 2; j < tmpBuf[i].size(); j++) {
        size_t  addr = startAddr + (j - 2);
        if (addr < 256 || addr >= (uncompressedSize + 256)) {
          throw Ep128Emu::Exception("decompressSFXProgram(): input data is "
                                    "not an SFX program");
        }
        outBuf[(addr - 256) + 16] = tmpBuf[i][j];
      }
    }
    return compressionType;
  }

}       // namespace Ep128Compress

