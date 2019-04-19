
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
#include "decompm2.hpp"
#include "pngwrite.hpp"

#include <vector>

// extract compressed file
static bool   extractMode = false;
// test compressed file(s)
static bool   testMode = false;
// assume archive format (implies forceRawMode)
static bool   archiveFormat = false;
// compression type (2 or 3; default: 3, or auto-detect when decompressing)
static int    compressionType = -1;
// compression level (1: fast, low compression ... 9: slow, high compression)
static int    compressionLevel = 10;
// disable decompressor border effects
static bool   noBorderFX = false;
// do not reset memory paging and stack pointer after decompression
static bool   noCleanup = true;
// ignore EXOS file headers
static bool   forceRawMode = false;
// volume size for compressed files (0: no volumes)
static size_t volumeSize = 0;

static int readInputFile(std::vector< unsigned char >& inBuf,
                         const char *fileName)
{
  inBuf.clear();
  if (fileName == (char *) 0 || fileName[0] == '\0')
    throw Ep128Emu::Exception("invalid input file name");
  Ep128Compress::CompressedFileVolume f(fileName, false, volumeSize);
  int     exosFileType = 0;
  while (true) {
    int     c = f.readByte();
    if (c == EOF)
      break;
    inBuf.push_back((unsigned char) (c & 0xFF));
  }
  if (inBuf.size() > 16 && !forceRawMode) {
    size_t  len = size_t(inBuf[2]) | (size_t(inBuf[3]) << 8);
    if (inBuf[0] == 0x00 && (inBuf[1] == 0x05 || inBuf[1] == 0x06) &&
        len > 0 && len <= (inBuf.size() - 16)) {
      exosFileType = inBuf[1];
      inBuf.erase(inBuf.begin(), inBuf.begin() + 16);
      inBuf.resize(len);
    }
  }
  return exosFileType;
}

static bool isEPImageFile(const std::vector< unsigned char >& buf,
                          size_t *dataSize1 = (size_t *) 0,
                          size_t *dataSize2 = (size_t *) 0,
                          bool *isCompressed = (bool *) 0)
{
  if (dataSize1)
    (*dataSize1) = 0;
  if (dataSize2)
    (*dataSize2) = 0;
  if (isCompressed)
    (*isCompressed) = false;
  if (buf.size() < 18 || forceRawMode)
    return false;
  if (!(buf[0] == 0x00 && buf[1] == 0x49 && buf[15] == 0x00))
    return false;
  int     width = int(buf[8]);
  int     height = int(buf[6]) | (int(buf[7]) << 8);
  if (width < 1 || height < 1)
    return false;
  if (buf[10] > 0x02)
    return false;
  if (buf[10] != 0x00) {
    if (buf.size() < 20)
      return false;
    size_t  compressedSize1 = size_t(buf[18]) | (size_t(buf[19]) << 8);
    if (compressedSize1 < 1 || buf.size() < (compressedSize1 + 24))
      return false;
    size_t  compressedSize2 = size_t(buf[compressedSize1 + 22])
                              | (size_t(buf[compressedSize1 + 23]) << 8);
    if (compressedSize2 < 1 ||
        buf.size() != (compressedSize1 + compressedSize2 + 24)) {
      return false;
    }
    if (dataSize1)
      (*dataSize1) = compressedSize1 + 4;
    if (dataSize2)
      (*dataSize2) = compressedSize2 + 4;
    if (isCompressed)
      (*isCompressed) = true;
    return true;
  }
  if (buf[2] != 0x00 || (buf[5] & 0x61) != 0)
    return false;
  if (buf[11] > 2 || buf[11] == (buf[5] == 0x00 ? 2 : 1))
    return false;
  if ((buf[16] & 0x81) != 0 ||
      !((buf[16] & 0x0E) == 0x02 || (buf[16] & 0x0E) == 0x04 ||
        (buf[16] & 0x0E) == 0x0E)) {
    return false;
  }
  if ((buf[16] & 0x0E) == 0x04 && (buf[16] & 0x60) != 0)
    return false;
  size_t  videoModeDataSize = 1;
  size_t  biasDataSize =
      size_t((buf[16] & 0x60) == 0x40 || (buf[16] & 0x0E) == 0x04);
  size_t  paletteDataSize = size_t(2) << ((buf[16] & 0x60) >> 5);
  if (biasDataSize > 0)
    paletteDataSize = 8;
  else if (paletteDataSize > 8)
    paletteDataSize = 0;
  size_t  attrDataSize = size_t((buf[16] & 0x0E) == 0x04) * size_t(width);
  size_t  pixelDataSize =
      ((buf[16] & 0x0E) == 0x02 ? 2 : 1) * size_t(width) * size_t(height);
  if (buf[3] > 0)
    biasDataSize *= size_t((height + int(buf[3]) - 1) / int(buf[3]));
  if (buf[4] > 0)
    paletteDataSize *= size_t((height + int(buf[4]) - 1) / int(buf[4]));
  if (buf[13] > 0)
    attrDataSize *= size_t((height + int(buf[13]) - 1) / int(buf[13]));
  else
    attrDataSize *= size_t(height);
  if (buf[5] & 0x02)
    biasDataSize = biasDataSize * 2;
  if (buf[5] & 0x04)
    paletteDataSize = paletteDataSize * 2;
  if (buf[5] & 0x08)
    attrDataSize = attrDataSize * 2;
  if (buf[5] & 0x10)
    pixelDataSize = pixelDataSize * 2;
  if (buf.size() != (videoModeDataSize + biasDataSize + paletteDataSize
                     + attrDataSize + pixelDataSize + 16)) {
    return false;
  }
  if ((videoModeDataSize + biasDataSize + paletteDataSize) >= 65536)
    return false;
  if ((attrDataSize + pixelDataSize) >= 65536)
    return false;
  if (dataSize1)
    (*dataSize1) = videoModeDataSize + biasDataSize + paletteDataSize;
  if (dataSize2)
    (*dataSize2) = attrDataSize + pixelDataSize;
  return true;
}

static void decompressRawData(std::vector< unsigned char >& outBuf,
                              const std::vector< unsigned char >& inBuf)
{
  Ep128Compress::decompressData(outBuf, inBuf, compressionType);
}

static void decompressFile(std::vector< unsigned char >& outBuf,
                           const std::vector< unsigned char >& inBuf,
                           int exosFileType = 0)
{
  outBuf.clear();
  if (inBuf.size() < 1)
    throw Ep128Emu::Exception("empty compressed data buffer");
  if (exosFileType != 0) {
    Ep128Compress::decompressSFXProgram(outBuf, inBuf, compressionType,
                                        (exosFileType == 6));
    return;
  }
  std::vector< unsigned char >  tmpBuf;
  size_t  imageDataSize1 = 0;
  size_t  imageDataSize2 = 0;
  bool    imageCompressedFlag = false;
  if (!isEPImageFile(inBuf, &imageDataSize1, &imageDataSize2,
                     &imageCompressedFlag)) {
    // raw compressed data
    try {
      // try new format first (no start address in block headers)
      decompressRawData(tmpBuf, inBuf);
      outBuf.insert(outBuf.end(), tmpBuf.begin(), tmpBuf.end());
      return;
    }
    catch (Ep128Emu::Exception) {
    }
    // if that fails, try old <64K format
    std::vector< std::vector< unsigned char > > tmpBuf2;
    Ep128Compress::decompressData(tmpBuf2, inBuf, compressionType);
    for (size_t i = 0; i < tmpBuf2.size(); i++)
      outBuf.insert(outBuf.end(), tmpBuf2[i].begin() + 2, tmpBuf2[i].end());
    return;
  }
  if (!imageCompressedFlag) {
    // uncompressed image: simply copy it to the output buffer
    outBuf.insert(outBuf.end(), inBuf.begin(), inBuf.end());
    return;
  }
  // compressed image:
  // copy header, change compression type to 0 (uncompressed)
  outBuf.insert(outBuf.end(), inBuf.begin(), inBuf.begin() + 16);
  outBuf[10] = 0x00;
  // decompress video mode, bias, and palette data
  size_t  uncompressedSize1 = size_t(inBuf[16]) | (size_t(inBuf[17]) << 8);
  tmpBuf.insert(tmpBuf.end(),
                inBuf.begin() + 20, inBuf.begin() + 16 + imageDataSize1);
  std::vector< unsigned char >  tmpBuf2;
  Ep128Compress::decompressData(tmpBuf2, tmpBuf, (inBuf[10] == 0x02 ? 0 : 2));
  tmpBuf.clear();
  if (tmpBuf2.size() != uncompressedSize1)
    throw Ep128Emu::Exception("error in compressed image data");
  outBuf.insert(outBuf.end(), tmpBuf2.begin(), tmpBuf2.end());
  tmpBuf2.clear();
  // decompress attribute and pixel data
  size_t  uncompressedSize2 = size_t(inBuf[imageDataSize1 + 16])
                              | (size_t(inBuf[imageDataSize1 + 17]) << 8);
  tmpBuf.insert(tmpBuf.end(),
                inBuf.begin() + 20 + imageDataSize1, inBuf.end());
  Ep128Compress::decompressData(tmpBuf2, tmpBuf, (inBuf[10] == 0x02 ? 0 : 2));
  tmpBuf.clear();
  if (tmpBuf2.size() != uncompressedSize2)
    throw Ep128Emu::Exception("error in compressed image data");
  outBuf.insert(outBuf.end(), tmpBuf2.begin(), tmpBuf2.end());
}

static size_t compressFile(std::vector< unsigned char >& outBuf,
                           const std::vector< unsigned char >& inBuf,
                           unsigned int startAddr = 0xFFFFFFFFU,
                           size_t skipBytes = 0, size_t length = 0x7FFFFFFF)
{
  size_t  startPos = outBuf.size();
  Ep128Compress::Compressor *compress = (Ep128Compress::Compressor *) 0;
  try {
    std::vector< unsigned char >  tmpBuf;
    std::vector< unsigned char >  tmpBuf2;
    if (skipBytes >= inBuf.size() || length < 1)
      return 0;
    if (length > (inBuf.size() - skipBytes))
      length = inBuf.size() - skipBytes;
    if (compressionType == 2) {         // fast mode
      Ep128Emu::compressData(tmpBuf2, &(inBuf.front()) + skipBytes, length);
    }
    else {
      tmpBuf.insert(tmpBuf.end(),
                    inBuf.begin() + skipBytes,
                    inBuf.begin() + skipBytes + length);
      compress = Ep128Compress::createCompressor(compressionType, tmpBuf2);
      Ep128Compress::Compressor::CompressionParameters  config;
      compress->getCompressionParameters(config);
      config.setCompressionLevel(compressionLevel);
      config.minLength = 2;
      config.maxOffset = 65535;
      config.blockSize = 65536;
      compress->setCompressionParameters(config);
      compress->compressData(tmpBuf, startAddr, true, true);
      delete compress;
      compress = (Ep128Compress::Compressor *) 0;
    }
    tmpBuf.clear();
    // verify compressed data
    if (startAddr < 0x80000000U) {
      std::vector< std::vector< unsigned char > > tmpBufs;
      Ep128Compress::decompressData(tmpBufs, tmpBuf2, compressionType);
      for (size_t i = 0; i < tmpBufs.size(); i++)
        tmpBuf.insert(tmpBuf.end(), tmpBufs[i].begin() + 2, tmpBufs[i].end());
    }
    else {
      decompressRawData(tmpBuf, tmpBuf2);
    }
    if (tmpBuf.size() != length)
      throw Ep128Emu::Exception("internal error compressing data");
    for (size_t i = 0; i < length; i++) {
      if (tmpBuf[i] != inBuf[skipBytes + i])
        throw Ep128Emu::Exception("internal error compressing data");
    }
    outBuf.insert(outBuf.end(), tmpBuf2.begin(), tmpBuf2.end());
  }
  catch (...) {
    if (compress)
      delete compress;
    throw;
  }
  return (outBuf.size() - startPos);
}

int main(int argc, char **argv)
{
  const char  *programName = argv[0];
  if (programName == (char *) 0)
    programName = "";
  for (size_t i = std::strlen(programName); i > 0; ) {
    i--;
    if (programName[i] == '/' || programName[i] == '\\' ||
        programName[i] == ':') {
      programName = programName + (i + 1);
      break;
    }
  }
  if (programName[0] == '\0')
    programName = "epcompress";
  std::vector< std::string >  fileNames;
  bool    printUsageFlag = false;
  bool    helpFlag = false;
  bool    endOfOptions = false;
  try {
    for (int i = 1; i < argc; i++) {
      std::string tmp = argv[i];
      if (tmp.length() < 1)
        continue;
      if (endOfOptions || tmp[0] != '-') {
        fileNames.push_back(tmp);
        continue;
      }
      if (tmp == "--") {
        endOfOptions = true;
        continue;
      }
      if (tmp.length() >= 4) {
        // allow GNU-style long options
        if (tmp[1] == '-')
          tmp.erase(0, 1);
      }
      if (tmp == "-h" || tmp == "-help") {
        printUsageFlag = true;
        helpFlag = true;
        throw Ep128Emu::Exception("");
      }
      else if (tmp == "-x") {
        extractMode = true;
        testMode = false;
      }
      else if (tmp == "-t") {
        testMode = true;
        extractMode = false;
      }
      else if (tmp == "-a") {
        archiveFormat = true;
      }
      else if (tmp == "-n") {
        archiveFormat = false;
      }
      else if (tmp == "-m") {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for -m");
        compressionType = int(std::atoi(argv[i]));
        if (!(compressionType == -1 ||
              (compressionType >= 2 && compressionType <= 3))) {
          throw Ep128Emu::Exception("invalid compression type");
        }
      }
      else if (tmp.length() == 3 && tmp[0] == '-' && tmp[1] == 'm' &&
               tmp[2] >= '2' && tmp[2] <= '3') {
        compressionType = int(tmp[2] - '0');
      }
      else if (tmp == "-borderfx") {
        noBorderFX = false;
      }
      else if (tmp == "-noborderfx") {
        noBorderFX = true;
      }
      else if (tmp == "-cleanup") {
        noCleanup = false;
      }
      else if (tmp == "-nocleanup") {
        noCleanup = true;
      }
      else if (tmp == "-raw") {
        forceRawMode = true;
      }
      else if (tmp == "-noraw") {
        forceRawMode = false;
      }
      else if (tmp == "-V") {
        if (extractMode || testMode) {
          volumeSize = 4096;
        }
        else {
          if (++i >= argc)
            throw Ep128Emu::Exception("missing argument for -V");
          volumeSize = size_t(std::atoi(argv[i]));
          if (volumeSize < 1)
            volumeSize = 0;
          else if (volumeSize > 16384)
            volumeSize = 16384;
          else
            volumeSize = (volumeSize + 3) & 0xFFFC;
          volumeSize = volumeSize * 1024;
        }
      }
      else {
        printUsageFlag = true;
        throw Ep128Emu::Exception("invalid command line option");
      }
    }
    if (fileNames.size()
        < ((testMode || (archiveFormat && extractMode)) ? 1 : 2)) {
      printUsageFlag = true;
      throw Ep128Emu::Exception("missing file name");
    }
    if (fileNames.size() > ((archiveFormat && extractMode) ? 1 : 2) &&
        !(testMode || (archiveFormat && !extractMode))) {
      printUsageFlag = true;
      throw Ep128Emu::Exception("too many file names");
    }
    if (archiveFormat)
      forceRawMode = true;
    if (testMode) {
      // test compressed file(s)
      std::vector< unsigned char >  inBuf;
      bool    errorFlag = false;
      for (size_t i = 0; i < fileNames.size(); i++) {
        std::printf("%s:%c",
                    fileNames[i].c_str(), (archiveFormat ? '\n' : ' '));
        inBuf.clear();
        int     exosFileType = readInputFile(inBuf, fileNames[i].c_str());
        if (inBuf.size() < 1) {
          errorFlag = true;
          std::printf("FAILED (empty file)\n");
        }
        else {
          std::vector< unsigned char >  tmpBuf;
          try {
            decompressFile(tmpBuf, inBuf, exosFileType);
            if (archiveFormat) {
              Ep128Compress::extractArchive(tmpBuf, false);
              std::printf("%s: ", fileNames[i].c_str());
            }
            std::printf("OK\n");
          }
          catch (Ep128Emu::Exception) {
            errorFlag = true;
            if (archiveFormat)
              std::printf("\n%s: ", fileNames[i].c_str());
            std::printf("FAILED\n");
          }
        }
      }
      return (errorFlag ? -1 : 0);
    }
    if (extractMode) {
      // extract compressed file
      std::vector< unsigned char >  inBuf;
      std::vector< unsigned char >  outBuf;
      int     exosFileType = readInputFile(inBuf, fileNames[0].c_str());
      if (inBuf.size() < 1)
        throw Ep128Emu::Exception("empty input file");
      decompressFile(outBuf, inBuf, exosFileType);
      if (outBuf.size() < 1)
        throw Ep128Emu::Exception("empty input file");
      if (archiveFormat) {
        Ep128Compress::extractArchive(outBuf, true);
      }
      else {
        Ep128Compress::CompressedFileVolume f(fileNames[1].c_str(), true, 0);
        for (size_t j = 0; j < outBuf.size(); j++)
          f.writeByte(int(outBuf[j]));
        f.closeFile();
      }
      return 0;
    }
    // compress file
    if (compressionType < 0)
      compressionType = 3;              // set default compression type
    std::vector< unsigned char >  outBuf;
    std::vector< unsigned char >  inBuf;
    // read input file
    int     exosFileType = 0;
    if (archiveFormat) {
      std::string tmp(fileNames[fileNames.size() - 1]);
      fileNames.resize(fileNames.size() - 1);
      Ep128Compress::createArchive(inBuf, fileNames);
      fileNames.push_back(tmp);
    }
    else {
      exosFileType = readInputFile(inBuf, fileNames[0].c_str());
    }
    bool    isImageFile = false;
    if (exosFileType == 0) {
      size_t  imageDataSize1 = 0;
      size_t  imageDataSize2 = 0;
      bool    imageCompressedFlag = false;
      if (isEPImageFile(inBuf, &imageDataSize1, &imageDataSize2,
                        &imageCompressedFlag)) {
        if (imageCompressedFlag) {
          // compressed image file: decompress it first
          std::vector< unsigned char >  tmpBuf;
          tmpBuf.insert(tmpBuf.end(), inBuf.begin(), inBuf.end());
          inBuf.clear();
          decompressFile(inBuf, tmpBuf, false);
          if (!isEPImageFile(inBuf, &imageDataSize1, &imageDataSize2,
                             &imageCompressedFlag)) {
            throw Ep128Emu::Exception("internal error loading "
                                      "compressed image file");
          }
        }
        // copy image header, and set compression type
        outBuf.insert(outBuf.end(), inBuf.begin(), inBuf.begin() + 16);
        outBuf[10] = (unsigned char) (compressionType == 2 ? 0x01 : 0x03);
        // compress video mode, bias, and palette data
        outBuf.push_back((unsigned char) (imageDataSize1 & 0xFF));
        outBuf.push_back((unsigned char) (imageDataSize1 >> 8));
        outBuf.push_back(0x00); // reserve space for compressed data size
        outBuf.push_back(0x00);
        size_t  compressedSize1 =
            compressFile(outBuf, inBuf, 0xFFFFFFFFU, 16, imageDataSize1);
        outBuf[18] = (unsigned char) (compressedSize1 & 0xFF);
        outBuf[19] = (unsigned char) (compressedSize1 >> 8);
        // compress attribute and pixel data
        outBuf.push_back((unsigned char) (imageDataSize2 & 0xFF));
        outBuf.push_back((unsigned char) (imageDataSize2 >> 8));
        outBuf.push_back(0x00); // reserve space for compressed data size
        outBuf.push_back(0x00);
        size_t  compressedSize2 =
            compressFile(outBuf, inBuf, 0xFFFFFFFFU,
                         imageDataSize1 + 16, imageDataSize2);
        outBuf[compressedSize1 + 22] = (unsigned char) (compressedSize2 & 0xFF);
        outBuf[compressedSize1 + 23] = (unsigned char) (compressedSize2 >> 8);
        isImageFile = true;
      }
    }
    unsigned int  startAddr = (exosFileType == 5 ? 0x0100U : 0xFFFFFFFFU);
    // compress data
    if (!isImageFile) {
      if (compressionType != 3 && exosFileType != 0 && !forceRawMode)
        throw Ep128Emu::Exception("output format requires -a or -raw");
      compressFile(outBuf, inBuf, startAddr);
      if (exosFileType != 0) {
        std::vector< unsigned char >  sfxBuf;
        Ep128Compress::addSFXModule(sfxBuf, outBuf, compressionType,
                                    noBorderFX, noCleanup, noCleanup,
                                    (exosFileType == 6));
        outBuf = sfxBuf;
      }
    }
    // write output file
    Ep128Compress::CompressedFileVolume
        f(fileNames[fileNames.size() - 1].c_str(), true, volumeSize);
    for (size_t i = 0; i < outBuf.size(); i++)
      f.writeByte(int(outBuf[i]));
    f.closeFile();
  }
  catch (std::exception& e) {
    if (printUsageFlag || helpFlag) {
      std::printf("Usage:\n");
      std::printf("    %s [OPTIONS...] <infile> [OPTIONS...] <outfile>\n",
                  programName);
      std::printf("        compress file\n");
      std::printf("    %s -x [OPTIONS...] <infile> [OPTIONS...] <outfile>\n",
                  programName);
      std::printf("        extract compressed file\n");
      std::printf("    %s -t [OPTIONS...] <infile...>\n", programName);
      std::printf("        test compressed file(s)\n");
      std::printf("Options:\n");
      std::printf("    --\n");
      std::printf("        interpret all remaining arguments as file names\n");
      std::printf("    -h | -help | --help\n");
      std::printf("        print usage information\n");
      std::printf("    -m2 | -m3\n");
      std::printf("        select compression type (default: 3, or "
                  "automatically detected\n"
                  "        when decompressing)\n");
      std::printf("        the output file smaller)\n");
      std::printf("    -raw | -noraw\n");
      std::printf("        ignore EXOS file headers if -raw (default: no)\n");
      std::printf("    -a | -n\n");
      std::printf("        enable or disable archive format; if enabled, "
                  "multiple input\n"
                  "        files can be specified when compressing (use @F to "
                  "read the list\n"
                  "        of file names from 'F'), and output file names are "
                  "read from\n"
                  "        the archive when decompressing; the default is -n, "
                  "and using -a\n"
                  "        implies -raw\n");
      std::printf("    -V\n");
      std::printf("        enable reading split files (extract and test mode "
                  "only)\n");
      std::printf("    -V <N>\n");
      std::printf("        split compressed output file to N kilobyte volumes "
                  "(compress\n"
                  "        mode only; N should be an integer multiple of 4)\n");
      std::printf("    -borderfx | -noborderfx\n");
      std::printf("        enable or disable decompressor border effects "
                  "(default: enabled)\n");
      std::printf("    -cleanup | -nocleanup\n");
      std::printf("        enable or disable restoring memory paging and "
                  "stack pointer\n"
                  "        after decompression (default: disabled)\n");
      if (helpFlag)
        return 0;
    }
    std::fprintf(stderr, " *** %s: %s\n", programName, e.what());
    return -1;
  }
  return 0;
}

