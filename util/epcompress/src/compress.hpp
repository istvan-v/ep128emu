
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

#ifndef EPCOMPRESS_COMPRESS_HPP
#define EPCOMPRESS_COMPRESS_HPP

#include "ep128emu.hpp"
#include <vector>

namespace Ep128Compress {

  class Compressor {
   public:
    class CompressionParameters {
     private:
      void limitParameters();
     public:
      size_t  optimizeIterations;
      size_t  splitOptimizationDepth;
      size_t  minLength;
      size_t  maxOffset;
      size_t  blockSize;
      CompressionParameters();
      CompressionParameters(const CompressionParameters& r);
      ~CompressionParameters()
      {
      }
      CompressionParameters& operator=(const CompressionParameters& r);
      void setCompressionLevel(int n);
    };
   protected:
    std::vector< unsigned char >& outBuf;
    size_t  progressCnt;
    size_t  progressMax;
    bool    progressDisplayEnabled;
    int     prvProgressPercentage;
    void    (*progressMessageCallback)(void *userData, const char *msg);
    void    *progressMessageUserData;
    bool    (*progressPercentageCallback)(void *userData, int n);
    void    *progressPercentageUserData;
    CompressionParameters   config;
    // --------
    void progressMessage(const char *msg);
    bool setProgressPercentage(int n);
   public:
    Compressor(std::vector< unsigned char >& outBuf_);
    virtual ~Compressor();
    virtual void getCompressionParameters(CompressionParameters& cfg) const;
    virtual void setCompressionParameters(const CompressionParameters& cfg);
    virtual void setCompressionLevel(int n);
    // if 'startAddr' is 0xFFFFFFFF, it is not stored in the compressed data
    virtual bool compressData(
        const std::vector< unsigned char >& inBuf, unsigned int startAddr,
        bool isLastBlock, bool enableProgressDisplay = false) = 0;
    virtual void setProgressMessageCallback(
        void (*func)(void *userData, const char *msg), void *userData_);
    virtual void setProgressPercentageCallback(
        bool (*func)(void *userData, int n), void *userData_);
  };

  // --------------------------------------------------------------------------

  class Decompressor {
   public:
    Decompressor();
    virtual ~Decompressor();
    virtual void decompressData(
        std::vector< std::vector< unsigned char > >& outBuf,
        const std::vector< unsigned char >& inBuf) = 0;
    // this function assumes "raw" compressed data blocks with no start address
    virtual void decompressData(
        std::vector< unsigned char >& outBuf,
        const std::vector< unsigned char >& inBuf) = 0;
  };

  // --------------------------------------------------------------------------

  Compressor * createCompressor(int compressionType,
                                std::vector< unsigned char >& outBuf);

  Decompressor * createDecompressor(int compressionType);

  // 'compressionType' can be negative to automatically detect compression type
  // returns the actual compression type used in 'inBuf'
  int decompressData(std::vector< std::vector< unsigned char > >& outBuf,
                     const std::vector< unsigned char >& inBuf,
                     int compressionType);

  // this function assumes that the compressed data includes no start addresses
  int decompressData(std::vector< unsigned char >& outBuf,
                     const std::vector< unsigned char >& inBuf,
                     int compressionType);

  void addSFXModule(std::vector< unsigned char >& outBuf,
                    const std::vector< unsigned char >& inBuf,
                    int compressionType = -1, bool noBorderFX = false,
                    bool noCleanup = false, bool noCRCCheck = false,
                    bool isExtension = false);

  // the return value is the actual compression type used in 'inBuf'
  int decompressSFXProgram(std::vector< unsigned char >& outBuf,
                           const std::vector< unsigned char >& inBuf,
                           int compressionType = -1, bool isExtension = false);

  // --------------------------------------------------------------------------

  class CompressedFileVolume {
   private:
    std::string volumeName;
    std::FILE   *f;
    long        volumeSize;
    long        volumeBytesRemaining;
    bool        isOutput;
    // --------
    static void getNextVolumeName(std::string& s);
    int openNextVolume();
    void outputFileError();
   public:
    CompressedFileVolume(const char *fileName, bool isOutput_,
                         size_t volumeSize_ = 0);
    virtual ~CompressedFileVolume();
    EP128EMU_INLINE int readByte()
    {
      int     c = std::fgetc(f);
      if (c == EOF)
        return openNextVolume();
      volumeBytesRemaining++;
      return c;
    }
    EP128EMU_INLINE void writeByte(int c)
    {
      if (--volumeBytesRemaining < 0L)
        (void) openNextVolume();
      if (std::fputc(c, f) == EOF)
        outputFileError();
    }
    void closeFile(bool deleteOutputFile = false);
  };

  void createArchive(std::vector< unsigned char >& outBuf,
                     const std::vector< std::string >& fileNames);

  void extractArchive(const std::vector< unsigned char >& inBuf,
                      bool writeFiles);

}       // namespace Ep128Compress

#endif  // EPCOMPRESS_COMPRESS_HPP

