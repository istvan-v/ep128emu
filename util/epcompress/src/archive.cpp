
// compressor utility for Enterprise 128 programs
// Copyright (C) 2007-2009 Istvan Varga <istvanv@users.sourceforge.net>
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

namespace Ep128Compress {

  void CompressedFileVolume::getNextVolumeName(std::string& s)
  {
    size_t  i = s.length();
    while (i > 0) {
      i--;
      char    c = s[i];
      if (c == '.') {
        if (s.length() == (i + 4)) {
          if (s[i + 1] >= '0' && s[i + 1] <= '9' &&
              s[i + 2] >= '0' && s[i + 2] <= '9' &&
              s[i + 3] >= '0' && s[i + 3] <= '9') {
            for (size_t j = i + 3; j > i; j--) {
              if (s[j] < '9') {
                s[j] = s[j] + char(1);
                break;
              }
              s[j] = '0';
            }
            return;
          }
        }
        if (s.length() > (i + 1))
          s.resize(i + 1);
        s += "001";
        return;
      }
      if (c == '/' || c == '\\' || c == ':')
        break;
    }
    s += ".001";
  }

  int CompressedFileVolume::openNextVolume()
  {
    closeFile(false);
    getNextVolumeName(volumeName);
    if (isOutput) {
      f = std::fopen(volumeName.c_str(), "wb");
      if (!f)
        throw Ep128Emu::Exception("error opening output file");
      volumeBytesRemaining = volumeSize - 1L;
    }
    else {
      long    tmp = volumeBytesRemaining;
      volumeBytesRemaining = 0L;
      if (tmp < volumeSize || (tmp & 0x0FFFL) != 0L)
        return EOF;
      f = std::fopen(volumeName.c_str(), "rb");
      if (!f)
        return EOF;
      int     c = std::fgetc(f);
      volumeBytesRemaining += long(c != EOF);
      return c;
    }
    return 0;
  }

  void CompressedFileVolume::outputFileError()
  {
    closeFile(true);
    throw Ep128Emu::Exception("error writing output file - is the disk full ?");
  }

  CompressedFileVolume::CompressedFileVolume(const char *fileName,
                                             bool isOutput_,
                                             size_t volumeSize_)
    : f((std::FILE *) 0),
      volumeSize(long(volumeSize_)),
      volumeBytesRemaining(0L),
      isOutput(isOutput_)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Ep128Emu::Exception("CompressedFileVolume: invalid file name");
    volumeName = fileName;
    if (volumeSize_ < 1) {
      volumeSize = 0x40000000L;
    }
    else if (volumeSize_ > 0x01000000) {
      volumeSize = 0x01000000L;
    }
    else {
      volumeSize = (volumeSize + 0x0800L) & 0x7FFFF000L;
      if (volumeSize < 0x1000L)
        volumeSize = 0x1000L;
    }
    if (isOutput) {
      f = std::fopen(volumeName.c_str(), "wb");
      if (!f)
        throw Ep128Emu::Exception("error opening output file");
      volumeBytesRemaining = volumeSize;
    }
    else {
      f = std::fopen(volumeName.c_str(), "rb");
      if (!f)
        throw Ep128Emu::Exception("error opening input file");
    }
  }

  CompressedFileVolume::~CompressedFileVolume()
  {
    try {
      closeFile(false);         // FIXME: errors are ignored here
    }
    catch (...) {
    }
  }

  void CompressedFileVolume::closeFile(bool deleteOutputFile)
  {
    if (!f)
      return;
    if (!isOutput) {
      std::fclose(f);
      f = (std::FILE *) 0;
    }
    else {
      if (std::fflush(f) != 0) {
        std::fclose(f);
        f = (std::FILE *) 0;
        std::remove(volumeName.c_str());
        throw Ep128Emu::Exception("error writing output file "
                                  "- is the disk full ?");
      }
      int     err = std::fclose(f);
      f = (std::FILE *) 0;
      if (err != 0) {
        std::remove(volumeName.c_str());
        throw Ep128Emu::Exception("error closing output file");
      }
      if (deleteOutputFile)
        std::remove(volumeName.c_str());
    }
  }

  // --------------------------------------------------------------------------

  void createArchive(std::vector< unsigned char >& outBuf,
                     const std::vector< std::string >& fileNames)
  {
    // get input file names, expanding any list files specified
    std::vector< std::string >  fileNames_;
    for (size_t i = 0; i < fileNames.size(); i++) {
      if (fileNames[i].length() < 1)
        throw Ep128Emu::Exception("createArchive(): invalid input file name");
      if (fileNames[i][0] == '@') {
        if (fileNames[i].length() < 2) {
          throw Ep128Emu::Exception("createArchive(): "
                                    "invalid input file list file name");
        }
        std::FILE *f = std::fopen(fileNames[i].c_str() + 1, "rb");
        if (!f) {
          throw Ep128Emu::Exception("createArchive(): "
                                    "error opening input file list file");
        }
        try {
          std::string s;
          while (true) {
            int     c = std::fgetc(f);
            if (c == EOF) {
              if (s.length() > 0)
                fileNames_.push_back(s);
              break;
            }
            c = c & 0xFF;
            if (c == 0x00 || c == 0x09 || c == 0x0A || c == 0x0D) {
              if (s.length() > 0) {
                fileNames_.push_back(s);
                s.clear();
              }
            }
            else {
              s += char(c);
            }
          }
        }
        catch (...) {
          std::fclose(f);
          throw;
        }
        std::fclose(f);
      }
      else {
        fileNames_.push_back(fileNames[i]);
      }
    }
    // read all input files
    if (fileNames_.size() < 1)
      throw Ep128Emu::Exception("createArchive(): no input file");
    if (fileNames_.size() > 16384)
      throw Ep128Emu::Exception("createArchive(): too many input files");
    outBuf.resize((fileNames_.size() * 32) + 4, 0x00);
    outBuf[2] = (unsigned char) (fileNames_.size() >> 8);
    outBuf[3] = (unsigned char) (fileNames_.size() & 0xFF);
    for (size_t i = 0; i < fileNames_.size(); i++) {
      const char  *s = fileNames_[i].c_str();
      const char  *t = s + fileNames_[i].length();
      while (t > s) {
        // remove directory name
        t--;
        char    c = *t;
        if (c == '/' || c == '\\' || c == ':') {
          t++;
          break;
        }
      }
      if ((*t) == '\0')
        throw Ep128Emu::Exception("createArchive(): invalid input file name");
      std::printf("  %s\n", s);
      for (size_t j = 0; j < 28; j++) {
        char    c = *(t++);
        if (c == '\0')
          break;
        if (c >= 'a' && c <= 'z')
          c = (c - 'a') + 'A';          // convert to upper case
        if ((c == '.' && j == 0) ||
            !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
              c == '+' || c == '-' || c == '.' || c == '_')) {
          // replace invalid characters with underscores
          c = '_';
        }
        outBuf[(i * 32) + j + 8] = (unsigned char) c;
      }
      std::FILE *f = std::fopen(s, "rb");
      size_t    fileSize = 0;
      if (!f)
        throw Ep128Emu::Exception("createArchive(): error opening input file");
      try {
        while (true) {
          int     c = std::fgetc(f);
          if (c == EOF)
            break;
          c = c & 0xFF;
          if (++fileSize >= 0x40000000) {
            throw Ep128Emu::Exception("createArchive(): "
                                      "input file is too large");
          }
          outBuf.push_back((unsigned char) c);
        }
      }
      catch (...) {
        std::fclose(f);
        throw;
      }
      std::fclose(f);
      outBuf[(i * 32) + 4] = (unsigned char) (fileSize >> 24);
      outBuf[(i * 32) + 5] = (unsigned char) ((fileSize >> 16) & 0xFF);
      outBuf[(i * 32) + 6] = (unsigned char) ((fileSize >> 8) & 0xFF);
      outBuf[(i * 32) + 7] = (unsigned char) (fileSize & 0xFF);
    }
    if (fileNames_.size() > 128) {
      std::fprintf(stderr,
                   "WARNING: archive contains more than 128 files (%lu)\n",
                   (unsigned long) fileNames_.size());
    }
  }

  // --------------------------------------------------------------------------

  void extractArchive(const std::vector< unsigned char >& inBuf,
                      bool writeFiles)
  {
    // check archive index, and get file names and file sizes
    if (inBuf.size() < 36) {
      throw Ep128Emu::Exception("insufficient input data size "
                                "for archive format");
    }
    size_t  nFiles = 0;
    for (size_t i = 0; i < 4; i++)
      nFiles = (nFiles << 8) | size_t(inBuf[i]);
    if (nFiles < 1 || nFiles > 16384 || inBuf.size() < ((nFiles * 32) + 4))
      throw Ep128Emu::Exception("invalid archive header");
    std::vector< std::string >  fileNames;
    std::vector< size_t >       fileSizes;
    size_t  totalDataSize = 0;
    for (size_t i = 0; i < nFiles; i++) {
      const unsigned char *bufp = &(inBuf.front()) + ((i * 32) + 4);
      if (bufp[0] > 0x3F)
        throw Ep128Emu::Exception("invalid file size in archive index");
      std::string fileName;
      size_t      fileSize = 0;
      for (size_t j = 0; j < 4; j++)
        fileSize = (fileSize << 8) | size_t(bufp[j]);
      for (size_t j = 0; j < 28; j++) {
        if (bufp[j + 4] >= 0x80)
          throw Ep128Emu::Exception("invalid file name in archive index");
        char    c = char(bufp[j + 4]);
        if (c >= 'A' && c <= 'Z')
          c = (c - 'A') + 'a';          // convert to lower case
        if (((c == '\0' || c == '.') && j == 0) ||
            !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
              c == '+' || c == '-' || c == '.' || c == '_' || c == '\0')) {
          throw Ep128Emu::Exception("invalid file name in archive index");
        }
        if (c == '\0')
          break;
        fileName += c;
      }
      totalDataSize += fileSize;
      if (inBuf.size() < (totalDataSize + (nFiles * 32) + 4))
        throw Ep128Emu::Exception("invalid file size in archive index");
      fileNames.push_back(fileName);
      fileSizes.push_back(fileSize);
      if (!writeFiles) {
        std::printf("  %-28s  (%6lu bytes)\n",
                    fileName.c_str(), (unsigned long) fileSize);
      }
    }
    if (inBuf.size() != (totalDataSize + (nFiles * 32) + 4))
      throw Ep128Emu::Exception("archive size does not match calculated value");
    if (!writeFiles)
      return;
    // extract all files
    const unsigned char *bufp = &(inBuf.front()) + ((nFiles * 32) + 4);
    for (size_t i = 0; i < nFiles; i++) {
      std::printf("  %s\n", fileNames[i].c_str());
      std::FILE *f = std::fopen(fileNames[i].c_str(), "wb");
      if (!f)
        throw Ep128Emu::Exception("error opening output file");
      for (size_t j = 0; j < fileSizes[i]; j++) {
        int     c = *(bufp++);
        if (std::fputc(c, f) == EOF) {
          std::fclose(f);
          std::remove(fileNames[i].c_str());
          throw Ep128Emu::Exception("error writing output file "
                                    "- is the disk full ?");
        }
      }
      if (std::fflush(f) != 0) {
        std::fclose(f);
        std::remove(fileNames[i].c_str());
        throw Ep128Emu::Exception("error writing output file "
                                  "- is the disk full ?");
      }
      if (std::fclose(f) != 0) {
        std::remove(fileNames[i].c_str());
        throw Ep128Emu::Exception("error closing output file");
      }
    }
  }

}       // namespace Ep128Compress

