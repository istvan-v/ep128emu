
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2006 Istvan Varga <istvanv@users.sourceforge.net>
// http://sourceforge.net/projects/ep128emu/
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

#ifndef EP128EMU_TAPEIO_HPP
#define EP128EMU_TAPEIO_HPP

#include "ep128.hpp"

#include <cstdio>
#include <string>
#include <vector>

#include <sndfile.h>
#include <FL/Fl_Progress.H>

namespace Ep128Emu {

  class TapeFilter {
   private:
    std::vector<float> irFFT;
    std::vector<float> fftBuf;
    std::vector<float> outBuf;
    size_t  sampleCnt;
   public:
    TapeFilter(size_t irSamples = 1024);
    virtual ~TapeFilter();
    void setFilterParameters(float sampleRate, float minFreq, float maxFreq);
    float processSample(float inputSignal);
    static void fft(float *buf, size_t n, bool isInverse);
  };

  class TapeInput {
   private:
    size_t    sampleCnt;
    size_t    percentsDone;
    Fl_Progress *progressDisplay;
    int       prvState;
    uint16_t  crcValue;
    double    periodLength;
   protected:
    size_t    totalSamples;
   public:
    TapeInput(Fl_Progress *);
    virtual ~TapeInput();
    // returns tape signal (0 or 1), or -1 on end of file
    virtual int getSample();
    // returns the length of the next half-period in samples,
    // or -1 on end of file
    long getHalfPeriod();
    // returns the length of the next full period in samples,
    // or -1 on end of file
    long getPeriod();
    // search for lead-in signal, sync bit, dummy byte, and 0x6A byte
    // return value is true if found, or false if end of file is reached
    bool search();
    // reset CRC register to zero
    void resetCRC();
    // read a single byte (0 to 255); return value is 256 on error (i.e.
    // invalid period time), and -1 on end of file
    int readByte();
    // returns the current CRC value
    uint16_t getCRC() const;
    // read a block (up to 256 bytes) into 'buf'
    // returns 0 on success, -1 on end of tape, and 1 on CRC error
    int readBlock(uint8_t *buf, size_t& nBytes);
    // read a chunk (up to 4096 bytes) into 'buf'
    // returns 0 on success, -1 on end of tape, and 1 on CRC error
    int readChunk(uint8_t *buf, size_t& nBytes, bool& isHeader);
   protected:
    virtual int getSample_() = 0;
  };

  class TapeInput_TapeImage : public TapeInput {
   private:
    std::FILE *f;
    uint8_t bitBuffer;
    uint8_t bitsRemaining;
   public:
    TapeInput_TapeImage(std::FILE *f_, Fl_Progress *disp);
    virtual ~TapeInput_TapeImage();
    // returns tape signal (0 or 1), or -1 on end of file
    virtual int getSample_();
  };

  class TapeInput_SndFile : public TapeInput {
   private:
    SNDFILE *f;
    std::vector<float>  buf;
    size_t  bufPos;
    size_t  channel;
    size_t  nChannels;
    TapeFilter  filter;
    float   prvSample;
    bool    interpFlag;
   public:
    TapeInput_SndFile(SNDFILE *f_, SF_INFO& sfinfo, Fl_Progress *disp,
                      int channel_ = 0,
                      float minFreq = 600.0f, float maxFreq = 3000.0f);
    virtual ~TapeInput_SndFile();
    // returns tape signal (0 or 1), or -1 on end of file
    virtual int getSample_();
  };

  class TapeFile {
   private:
    std::string fileName;
   public:
    std::vector<uint8_t>  fileData;
    bool    hasErrors;
    bool    isComplete;
    bool    isCopyProtected;
    TapeFile();
    void setFileName(const std::string&);
    std::string getFileName() const;
  };

  class TapeOutput {
   private:
    std::FILE *f;
    std::vector<size_t> cuePoints;
    uint8_t   bitBuffer;
    uint8_t   bitCnt;
    uint16_t  crcValue;
    bool      isFlushed;
    size_t    fileSize;
    void writeSample(int n);
    void writePeriod(size_t periodLength);
    void resetCRC();
    void writeByte(uint8_t n);
    void writeCRC();
   public:
    TapeOutput(const char *fileName);
    virtual ~TapeOutput();
    void writeFile(const TapeFile& file_);
    void flush();
  };

  class TapeFiles {
   private:
    std::vector<TapeFile *> tapeFiles_;
   public:
    TapeFiles();
    virtual ~TapeFiles();
    TapeFile * operator[](size_t n)
    {
      if ((unsigned long) n >= tapeFiles_.size())
        return (TapeFile *) 0;
      return tapeFiles_[n];
    }
    size_t getFileCnt() const
    {
      return tapeFiles_.size();
    }
    void readTapeImage(const char *fileName_, Fl_Progress *disp,
                       int channel_ = 0,
                       float minFreq_ = 600.0f, float maxFreq_ = 3000.0f);
    bool writeTapeImage(const char *fileName_, bool allowOverwrite_ = false);
    void importFile(const char *fileName_);
    bool exportFile(int n, const char *fileName_, bool allowOverwrite_ = false);
    void duplicateFile(int n);
    void moveFile(int n, bool isForward);
    void removeFile(int n);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_TAPEIO_HPP

