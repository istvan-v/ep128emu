
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

#include "ep128.hpp"
#include "tapeio.hpp"

#include <iostream>
#include <string>
#include <exception>
#include <vector>
#include <cmath>
#include <cstdio>

#include <sndfile.h>
#include <FL/Fl.H>

namespace Ep128Emu {

  TapeFilter::TapeFilter(size_t irSamples)
  {
    if (irSamples < 16 || irSamples > 32768 ||
        (irSamples & (irSamples - 1)) != 0)
      throw std::exception();
    irFFT.resize(irSamples << 2);
    fftBuf.resize(irSamples << 2);
    outBuf.resize(irSamples);
    sampleCnt = 0;
    for (size_t i = 0; i < (irSamples << 2); i++)
      irFFT[i] = 0.0f;
    irFFT[irSamples >> 1] = 1.0f;
    this->fft(&(irFFT.front()), (irSamples << 1), false);
    for (size_t i = 0; i < (irSamples << 2); i++)
      fftBuf[i] = 0.0f;
    for (size_t i = 0; i < irSamples; i++)
      outBuf[i] = 0.0f;
  }

  TapeFilter::~TapeFilter()
  {
  }

  void TapeFilter::setFilterParameters(float sampleRate,
                                       float minFreq, float maxFreq)
  {
    size_t  n = outBuf.size();
    // calculate linear phase filter
    size_t  fhpp, fhps, flpp, flps;
    fhpp = size_t(long(float(long(n)) * (minFreq / sampleRate) + 0.5f));
    fhps = size_t(long(float(long(n)) * (0.25f * minFreq / sampleRate) + 0.5f));
    flpp = size_t(long(float(long(n)) * (maxFreq / sampleRate) + 0.5f));
    flps = size_t(long(float(long(n)) * (4.0f * maxFreq / sampleRate) + 0.5f));
    for (size_t i = 0; i <= (n >> 1); i++) {
      double  a = 1.0 / double(long(n));
      if (i <= fhps || i >= flps)
        a = 0.0;
      else {
        if (i > fhps && i < fhpp) {
          double  w = std::sin(2.0 * std::atan(1.0)
                               * double(long(i - fhps))
                               / double(long(fhpp - fhps)));
          a *= (w * w);
        }
        if (i > flpp && i < flps) {
          double  w = std::cos(2.0 * std::atan(1.0)
                               * double(long(i - flpp))
                               / double(long(flps - flpp)));
          a *= (w * w);
        }
      }
      irFFT[(i << 1) + 0] = float((i & 1) == 0 ? a : -a);
      irFFT[(i << 1) + 1] = 0.0f;
    }
    // apply von Hann window to impulse response
    this->fft(&(irFFT.front()), n, true);
    for (size_t i = 0; i < n; i++) {
      double  w = std::sin(4.0 * std::atan(1.0) * double(long(i))
                                                / double(long(n)));
      irFFT[i] *= float(w * w);
    }
    // pad to double length
    for (size_t i = n; i < (n << 1); i++)
      irFFT[i] = 0.0f;
    this->fft(&(irFFT.front()), (n << 1), false);
  }

  float TapeFilter::processSample(float inputSignal)
  {
    size_t  n = outBuf.size();
    if (sampleCnt >= n) {
      sampleCnt = 0;
      // copy remaining samples from previous buffer
      for (size_t i = 0; i < n; i++)
        outBuf[i] = fftBuf[i + n];
      // pad input to double length
      for (size_t i = n; i < (n << 1); i++)
        fftBuf[i] = 0.0f;
      // convolve
      this->fft(&(fftBuf.front()), (n << 1), false);
      for (size_t i = 0; i <= n; i++) {
        double  re1 = fftBuf[(i << 1) + 0];
        double  im1 = fftBuf[(i << 1) + 1];
        double  re2 = irFFT[(i << 1) + 0];
        double  im2 = irFFT[(i << 1) + 1];
        fftBuf[(i << 1) + 0] = re1 * re2 - im1 * im2;
        fftBuf[(i << 1) + 1] = re1 * im2 + im1 * re2;
      }
      this->fft(&(fftBuf.front()), (n << 1), true);
      // mix new buffer to output
      for (size_t i = 0; i < n; i++)
        outBuf[i] += fftBuf[i];
    }
    fftBuf[sampleCnt] = inputSignal;
    float   outputSignal = outBuf[sampleCnt] / float(long(n));
    sampleCnt++;
    return outputSignal;
  }

  void TapeFilter::fft(float *buf, size_t n, bool isInverse)
  {
    // check FFT size
    if (n < 16 || n > 65536 || (n & (n - 1)) != 0)
      throw std::exception();
    if (!isInverse) {
      // convert real data to interleaved real/imaginary format
      size_t  i = n;
      do {
        i--;
        buf[(i << 1) + 0] = buf[i];
        buf[(i << 1) + 1] = 0.0f;
      } while (i);
    }
    else {
      buf[1] = 0.0f;
      buf[n + 1] = 0.0f;
      for (size_t i = 1; i < (n >> 1); i++) {
        buf[((n - i) << 1) + 0] = buf[(i << 1) + 0];
        buf[((n - i) << 1) + 1] = -(buf[(i << 1) + 1]);
      }
    }
    // pack data in reverse bit order
    size_t  i, j;
    for (i = 0, j = 0; i < n; i++) {
      if (i < j) {
        float   tmp1 = buf[(i << 1) + 0];
        float   tmp2 = buf[(i << 1) + 1];
        buf[(i << 1) + 0] = buf[(j << 1) + 0];
        buf[(i << 1) + 1] = buf[(j << 1) + 1];
        buf[(j << 1) + 0] = tmp1;
        buf[(j << 1) + 1] = tmp2;
      }
      for (size_t k = (n >> 1); k > 0; k >>= 1) {
        j ^= k;
        if ((j & k) != 0)
          break;
      }
    }
    // calculate FFT
    for (size_t k = 1; k < n; k <<= 1) {
      double  ph_inc_re = std::cos(4.0 * std::atan(1.0) / double(long(k)));
      double  ph_inc_im = std::sin((isInverse ? 4.0 : -4.0)
                                   * std::atan(1.0) / double(long(k)));
      for (j = 0; j < n; j += (k << 1)) {
        double  ph_re = 1.0;
        double  ph_im = 0.0;
        for (i = j; i < (j + k); i++) {
          double  re1 = buf[(i << 1) + 0];
          double  im1 = buf[(i << 1) + 1];
          double  re2 = buf[((i + k) << 1) + 0];
          double  im2 = buf[((i + k) << 1) + 1];
          buf[(i << 1) + 0] = float(re1 + re2 * ph_re - im2 * ph_im);
          buf[(i << 1) + 1] = float(im1 + re2 * ph_im + im2 * ph_re);
          buf[((i + k) << 1) + 0] = float(re1 - re2 * ph_re + im2 * ph_im);
          buf[((i + k) << 1) + 1] = float(im1 - re2 * ph_im - im2 * ph_re);
          double  tmp = ph_re * ph_inc_re - ph_im * ph_inc_im;
          ph_im = ph_re * ph_inc_im + ph_im * ph_inc_re;
          ph_re = tmp;
        }
      }
    }
    if (!isInverse) {
      buf[1] = 0.0f;
      buf[n + 1] = 0.0f;
    }
    else {
      // convert from interleaved real/imaginary format to pure real data
      for (i = 0; i < n; i++)
        buf[i] = buf[(i << 1) + 0];
      for (i = n; i < (n << 1); i++)
        buf[i] = 0.0f;
    }
  }

  // --------------------------------------------------------------------------

  TapeFile::TapeFile()
    : fileName(""),
      hasErrors(false),
      isComplete(true),
      isCopyProtected(false)
  {
  }

  void TapeFile::setFileName(const std::string& fname)
  {
    fileName = "";
    size_t  i = 0;
    for ( ; i < fname.length(); i++) {
      if (fname[i] != ' ' && fname[i] != '\t' &&
          fname[i] != '\r' && fname[i] != '\n')
        break;
    }
    size_t  j = fname.length();
    while (j > i) {
      j--;
      if (fname[j] != ' ' && fname[j] != '\t' &&
          fname[j] != '\r' && fname[j] != '\n') {
        j++;
        break;
      }
    }
    if (j > (i + 28))
      j = i + 28;
    for ( ; i < j; i++) {
      if ((fname[i] >= 'A' && fname[i] <= 'Z') ||
          (fname[i] >= '0' && fname[i] <= '9') ||
          fname[i] == '.' || fname[i] == '-' || fname[i] == '_')
        fileName += fname[i];
      else if (fname[i] >= 'a' && fname[i] <= 'z')
        fileName += ((fname[i] - 'a') + 'A');   // convert to upper case
      else
        fileName += '_';        // replace invalid characters with underscores
    }
    if (fileName == "__unnamed_file__")
      fileName = "";
  }

  std::string TapeFile::getFileName() const
  {
    std::string s("");
    if (fileName.length() == 0)
      s = "__unnamed_file__";
    else {
      for (size_t i = 0; i < fileName.length(); i++) {
        if (fileName[i] >= 'A' && fileName[i] <= 'Z')
          s += ((fileName[i] - 'A') + 'a');     // convert to lower case
        else
          s += fileName[i];
      }
    }
    return s;
  }

  // --------------------------------------------------------------------------

  TapeInput::TapeInput(Fl_Progress *disp)
    : sampleCnt(0),
      percentsDone(0),
      progressDisplay(disp),
      prvState(-1),
      crcValue(0),
      periodLength(1.0),
      totalSamples(0)
  {
    if (disp) {
      disp->minimum(0.0f);
      disp->maximum(100.0f);
      disp->value(0.0f);
      Fl::wait(0.0);
    }
  }

  TapeInput::~TapeInput()
  {
  }

  int TapeInput::getSample()
  {
    long    percentsDone_ = 0L;
    if (totalSamples > 0) {
      percentsDone_ = long(100.0 * double(long(sampleCnt))
                                 / double(long(totalSamples))
                           + 0.5);
      percentsDone_ = (percentsDone_ > 0L ?
                       (percentsDone_ < 100L ? percentsDone_ : 100L) : 0L);
    }
    if (size_t(percentsDone_) != percentsDone) {
      percentsDone = size_t(percentsDone_);
      if (progressDisplay) {
        progressDisplay->value(float(percentsDone_));
        Fl::wait(0.0);
      }
    }
    sampleCnt++;
    return getSample_();
  }

  long TapeInput::getHalfPeriod()
  {
    long    cnt = 1L;
    while (cnt < 0x3FFFFFFFL) {
      int   tmp = getSample();
      if (tmp == prvState && tmp >= 0) {
        cnt++;
        continue;
      }
      prvState = tmp;
      if (tmp < 0)
        return -1L;
      break;
    }
    return cnt;
  }

  long TapeInput::getPeriod()
  {
    long    p1 = getHalfPeriod();
    long    p2 = getHalfPeriod();
    return (p1 >= 0L && p2 >= 0L ? (p1 + p2) : -1L);
  }

  bool TapeInput::search()
  {
    size_t  cnt = 0;
    long    curHalfPeriod = 1L;
    long    prvHalfPeriod = 1L;
    periodLength = 1.0;
    while (true) {
      prvHalfPeriod = curHalfPeriod;
      curHalfPeriod = getHalfPeriod();
      if (curHalfPeriod < 0L)
        return false;
      periodLength = (periodLength * 0.975) + (double(curHalfPeriod) * 0.05);
      if (cnt < 100) {
        cnt++;
        continue;
      }
      if ((double(prvHalfPeriod + curHalfPeriod) / periodLength) > 1.45) {
        if (((unsigned int) readByte() & 0x100U) != 0U)
          continue;
        if (readByte() == 0x6A)
          return true;
      }
    }
  }

  void TapeInput::resetCRC()
  {
    crcValue = 0;
  }

  int TapeInput::readByte()
  {
    bool    err = false;
    int     retval = 0;
    for (int i = 0; i < 8; i++) {
      long    n = getPeriod();
      if (n < 0L)
        return -1;
      double  t = double(n) / periodLength;
      if (t > 1.0) {
        retval = (retval >> 1);
        if (t < 1.05 || t > 1.6)
          err = true;
      }
      else {
        retval = (retval >> 1) | 0x80;
        if (t < 0.6 || t > 0.95)
          err = true;
      }
      crcValue ^= uint16_t((retval & 0x80) << 8);
      if ((crcValue & 0x8000) != 0)
        crcValue ^= uint16_t(0x0810);
      crcValue = ((crcValue & 0x7FFF) << 1) | ((crcValue & 0x8000) >> 15);
    }
    return (err ? 256 : retval);
  }

  uint16_t TapeInput::getCRC() const
  {
    return crcValue;
  }

  int TapeInput::readBlock(uint8_t *buf, size_t& nBytes)
  {
    for (size_t i = 0; i < 256; i++)
      buf[i] = 0;
    nBytes = 0;
    int   n = readByte();
    if (n < 0)
      return -1;
    if (n > 255)
      return 1;
    if (!n)
      n = 256;
    resetCRC();
    do {
      int   c = readByte();
      if (c < 0)
        return -1;
      if (c > 255)
        return 1;
      buf[nBytes++] = uint8_t(c);
    } while (--n);
    uint16_t  crcCalculated = getCRC();
    uint16_t  crcStored = 0;
    int   c = readByte();
    if (c < 0)
      return -1;
    if (c > 255)
      return 1;
    crcStored = uint16_t(c);
    c = readByte();
    if (c < 0)
      return -1;
    if (c > 255)
      return 1;
    crcStored |= (uint16_t(c) << 8);
    return (crcCalculated == crcStored ? 0 : 1);
  }

  int TapeInput::readChunk(uint8_t *buf, size_t& nBytes, bool& isHeader)
  {
    for (size_t i = 0; i < 4096; i++)
      buf[i] = 0;
    nBytes = 0;
    isHeader = false;
    if (!this->search())
      return -1;
    int   n = readByte();
    if (n < 0)
      return -1;
    if (!(n <= 16 || n == 255))
      return 1;
    if (n == 255) {
      isHeader = true;
      int   retval = readBlock(buf, nBytes);
      if (!retval) {
        if (nBytes < 2 || nBytes > 30 || nBytes != (size_t(buf[1]) + 2))
          retval = 1;
      }
      if (buf[1] > 28)
        buf[1] = 28;
      buf[buf[1] + 2] = 0;
      return retval;
    }
    else {
      while (n--) {
        size_t  tmp = 0;
        int     retval = readBlock(&(buf[nBytes]), tmp);
        nBytes += tmp;
        if (retval != 0)
          return retval;
        if (n != 0 && tmp != 256)
          return 1;
      }
    }
    return 0;
  }

  TapeInput_TapeImage::TapeInput_TapeImage(std::FILE *f_,
                                           Fl_Progress *disp)
    : TapeInput(disp),
      f(f_),
      bitBuffer(0),
      bitsRemaining(0)
  {
    if (f) {
      if (std::fseek(f, 0L, SEEK_END) == 0) {
        long  fileSize = std::ftell(f);
        if (fileSize > 0L)
          totalSamples = size_t(fileSize) << 3;
      }
      std::fseek(f, 0L, SEEK_SET);
    }
  }

  TapeInput_TapeImage::~TapeInput_TapeImage()
  {
    if (f)
      std::fclose(f);
  }

  int TapeInput_TapeImage::getSample_()
  {
    if (!bitsRemaining) {
      if (f) {
        int   c = std::fgetc(f);
        if (c != EOF) {
          bitBuffer = uint8_t(c);
          bitsRemaining = 8;
        }
        else {
          std::fclose(f);
          f = (std::FILE *) 0;
        }
      }
      if (!bitsRemaining)
        return -1;
    }
    int   retval = ((bitBuffer & 0x80) == 0 ? 0 : 1);
    bitBuffer <<= 1;
    bitsRemaining--;
    return retval;
  }

  TapeInput_SndFile::TapeInput_SndFile(SNDFILE *f_, SF_INFO& sfinfo,
                                       Fl_Progress *disp,
                                       int channel_,
                                       float minFreq, float maxFreq)
    : TapeInput(disp),
      f(f_),
      bufPos(256),
      channel(0),
      nChannels(0),
      filter(2048),
      prvSample(0.0f),
      interpFlag(false)
  {
    if (f_) {
      nChannels = size_t(sfinfo.channels);
      channel = size_t(channel_ > 0 ?
                       (channel_ < (sfinfo.channels - 1) ?
                        channel_ : (sfinfo.channels - 1))
                       : 0);
      totalSamples = size_t(sfinfo.frames) << 1;
      filter.setFilterParameters(float(sfinfo.samplerate), minFreq, maxFreq);
      buf.resize(256 * nChannels);
    }
  }

  TapeInput_SndFile::~TapeInput_SndFile()
  {
    if (f)
      sf_close(f);
  }

  int TapeInput_SndFile::getSample_()
  {
    if (!interpFlag) {
      interpFlag = true;
      return (prvSample > 0.0f ? 1 : 0);
    }
    float   nxtSample = 0.0f;
    if (bufPos < 256) {
      nxtSample = buf[bufPos * nChannels + channel];
      bufPos++;
    }
    else if (f != (SNDFILE *) 0) {
      bufPos = 0;
      sf_count_t  n;
      n = sf_read_float(f, &(buf.front()), sf_count_t(256 * nChannels));
      n = (n > sf_count_t(0) ? n : sf_count_t(0));
      if (n < sf_count_t(256 * nChannels)) {
        sf_close(f);
        f = (SNDFILE *) 0;
        do {
          buf[n] = 0.0f;
        } while (++n < sf_count_t(256 * nChannels));
      }
    }
    else if (bufPos < 4096) {
      bufPos++;
    }
    else
      return -1;
    nxtSample = filter.processSample(nxtSample);
    int     retval = ((prvSample + nxtSample) > 0.0f ? 1 : 0);
    prvSample = nxtSample;
    interpFlag = false;
    return retval;
  }

  // --------------------------------------------------------------------------

  void TapeOutput::writeSample(int n)
  {
    isFlushed = false;
    bitBuffer = ((bitBuffer & 0x7F) << 1) | uint8_t(n > 0 ? 1 : 0);
    if (++bitCnt < 8)
      return;
    bitCnt = 0;
    if (f) {
      if (std::fputc(bitBuffer, f) == EOF)
        throw Ep128::Exception("error writing file - is the disk full ?");
      fileSize++;
    }
    bitBuffer = 0;
  }

  void TapeOutput::writePeriod(size_t periodLength)
  {
    size_t  i = 0;
    for ( ; i < (periodLength >> 1); i++)
      writeSample(1);
    for ( ; i < periodLength; i++)
      writeSample(0);
  }

  void TapeOutput::resetCRC()
  {
    crcValue = 0;
  }

  void TapeOutput::writeByte(uint8_t n)
  {
    uint8_t b = n;
    for (int i = 0; i < 8; i++) {
      crcValue ^= (uint16_t(b & 1) << 15);
      if ((crcValue & 0x8000) != 0)
        crcValue ^= uint16_t(0x0810);
      crcValue = ((crcValue & 0x7FFF) << 1) | ((crcValue & 0x8000) >> 15);
      if ((b & 1) != 0)
        writePeriod(8);
      else
        writePeriod(12);
      b >>= 1;
    }
  }

  void TapeOutput::writeCRC()
  {
    uint8_t l = uint8_t(crcValue & 0xFF);
    uint8_t h = uint8_t((crcValue >> 8) & 0xFF);
    writeByte(l);
    writeByte(h);
  }

  TapeOutput::TapeOutput(const char *fileName)
    : f((std::FILE *) 0),
      bitBuffer(0),
      bitCnt(0),
      crcValue(0),
      isFlushed(false),
      fileSize(0)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Ep128::Exception("invalid file name");
    f = std::fopen(fileName, "wb");
    if (!f)
      throw Ep128::Exception("error opening file");
    try {
      // empty space for cue point table (will be written later by flush())
      for (size_t i = 0; i < 32768; i++)
        writeSample(0);
    }
    catch (...) {
      std::fclose(f);
      f = (std::FILE *) 0;
      throw;
    }
  }

  TapeOutput::~TapeOutput()
  {
    try {
      // FIXME: cannot handle errors here
      this->flush();
      if (f)
        std::fclose(f);
    }
    catch (...) {
    }
  }

  void TapeOutput::writeFile(const TapeFile& file_)
  {
    // add cue point at the beginning of the file
    if (cuePoints.size() < 1021) {
      if (fileSize >= 4096 && fileSize <= 0x20000000)
        cuePoints.push_back((fileSize - 4096) << 3);
    }
    // write lead-in, sync bit, dummy byte, and 0x6A
    for (size_t i = 0; i < 6000; i++)       // 0.25 seconds
      writeSample(0);
    for (size_t i = 0; i < 6000; i++)       // 2.5 seconds
      writePeriod(10);
    writePeriod(16);
    writeByte(0x00);
    writeByte(0x6A);
    // write header
    std::string fname = file_.getFileName();
    writeByte(0xFF);
    writeByte(uint8_t(fname.length() + 2));
    resetCRC();
    writeByte(file_.isCopyProtected ? 0x00 : 0xFF);
    writeByte(uint8_t(fname.length()));
    for (size_t i = 0; i < fname.length(); i++)
      writeByte(uint8_t(fname[i] >= 'a' && fname[i] <= 'z' ?
                        (fname[i] - 'a') + 'A' : fname[i]));
    writeCRC();
    for (size_t i = 0; i < 600; i++)        // 0.25 seconds
      writePeriod(10);
    for (size_t i = 0; i < 6000; i++)       // 0.25 seconds
      writeSample(0);
    // write file data
    for (size_t i = 0; i <= (file_.fileData.size() >> 12); i++) {
      size_t  nBlocks = ((file_.fileData.size() - (i << 12)) + 255) >> 8;
      nBlocks = (nBlocks < 16 ? nBlocks : 16);
      // write lead-in, sync bit, dummy byte, and 0x6A
      for (size_t l = 0; l < 6000; l++)     // 0.25 seconds
        writeSample(0);
      for (size_t l = 0; l < 4800; l++)     // 2 seconds
        writePeriod(10);
      writePeriod(16);
      writeByte(0x00);
      writeByte(0x6A);
      // write number of blocks
      writeByte(uint8_t(nBlocks));
      for (size_t j = 0; j < nBlocks; j++) {
        size_t  nBytes = file_.fileData.size() - (((i << 4) + j) << 8);
        nBytes = (nBytes < 256 ? nBytes : 256);
        // write number of bytes in block (0 for 256)
        writeByte(uint8_t(nBytes & 255));
        resetCRC();
        // write block data
        for (size_t k = (((i << 4) + j) << 8);
             k < ((((i << 4) + j) << 8) + nBytes);
             k++)
          writeByte(file_.fileData[k]);
        // write CRC
        writeCRC();
      }
      for (size_t l = 0; l < 600; l++)      // 0.25 seconds
        writePeriod(10);
      for (size_t l = 0; l < 6000; l++)     // 0.25 seconds
        writeSample(0);
    }
    for (size_t i = 0; i < 24000; i++)      // 1 second
      writeSample(0);
    this->flush();
  }

  void TapeOutput::flush()
  {
    if (f == (std::FILE *) 0 || isFlushed)
      return;
    while (bitCnt != 0)
      writeSample(0);
    // write cue point table
    if (std::fseek(f, 0L, SEEK_SET) < 0)
      throw Ep128::Exception("error setting file position in tape image");
    bool    err = false;
    if (std::fputc(0x7B, f) == EOF)
      err = true;
    if (std::fputc(0x6C, f) == EOF)
      err = true;
    if (std::fputc(0xDE, f) == EOF)
      err = true;
    if (std::fputc(0x49, f) == EOF)
      err = true;
    uint32_t  cuePointChecksum = 1U;
    for (size_t i = 0; i < 1023; i++) {
      uint32_t  n = uint32_t(0xFFFFFFFFUL);
      if (i < cuePoints.size())
        n = uint32_t(cuePoints[i]);
      else if (i == 1022)
        n = cuePointChecksum;
      if (std::fputc(int((n >> 24) & 0xFF), f) == EOF)
        err = true;
      if (std::fputc(int((n >> 16) & 0xFF), f) == EOF)
        err = true;
      if (std::fputc(int((n >> 8) & 0xFF), f) == EOF)
        err = true;
      if (std::fputc(int(n & 0xFF), f) == EOF)
        err = true;
      cuePointChecksum ^= n;
      uint64_t  tmp = cuePointChecksum * uint64_t(0xC2B0C3CCUL);
      cuePointChecksum = (uint32_t(tmp) ^ uint32_t(tmp >> 32))
                         & uint32_t(0xFFFFFFFFUL);
    }
    if (err)
      throw Ep128::Exception("error writing file - is the disk full ?");
    if (std::fseek(f, long(fileSize), SEEK_SET) < 0)
      throw Ep128::Exception("error setting file position in tape image");
    while (fileSize < 8192)
      writeSample(0);
    isFlushed = true;
  }

  // --------------------------------------------------------------------------

  TapeFiles::TapeFiles()
  {
  }

  TapeFiles::~TapeFiles()
  {
    for (size_t i = 0; i < tapeFiles_.size(); i++) {
      if (tapeFiles_[i] != (TapeFile *) 0) {
        delete tapeFiles_[i];
        tapeFiles_[i] = (TapeFile *) 0;
      }
    }
    tapeFiles_.clear();
  }

  void TapeFiles::readTapeImage(const char *fileName_, Fl_Progress *disp,
                                int channel_, float minFreq_, float maxFreq_)
  {
    if (fileName_ == (char *) 0 || fileName_[0] == '\0')
      throw Ep128::Exception("invalid file name");
    SF_INFO   sfinfo;
    std::memset(&sfinfo, 0, sizeof(SF_INFO));
    SNDFILE   *sf = sf_open(fileName_, SFM_READ, &sfinfo);
    std::FILE *f = (std::FILE *) 0;
    if (!sf) {
      f = std::fopen(fileName_, "rb");
      if (!f)
        throw Ep128::Exception("error opening file");
    }
    TapeInput *t = (TapeInput *) 0;
    try {
      if (sf)
        t = new TapeInput_SndFile(sf, sfinfo, disp,
                                  channel_, minFreq_, maxFreq_);
      else
        t = new TapeInput_TapeImage(f, disp);
    }
    catch (...) {
      if (sf)
        sf_close(sf);
      if (f)
        std::fclose(f);
      throw;
    }
    TapeFile  *tf = (TapeFile *) 0;
    try {
      bool    readingFile = false;
      bool    isCompleteFile = false;
      std::vector<uint8_t>  buf;
      buf.resize(4096);
      size_t  nBytes = 0;
      bool    isHeader = false;
      int     retval;
      tf = new TapeFile;
      tf->hasErrors = false;
      tf->isComplete = false;
      tf->isCopyProtected = false;
      while (true) {
        retval = t->readChunk(&(buf.front()), nBytes, isHeader);
        if (!isHeader) {
          for (size_t i = 0; i < nBytes; i++)
            tf->fileData.push_back(buf[i]);
        }
        if (readingFile && retval == 0 && nBytes == 4096 && !isHeader)
          continue;
        if (nBytes > 0 && !isHeader)
          readingFile = true;
        if (retval == 0 && nBytes == 4096 && !isHeader)
          continue;
        if (readingFile) {
          if (tf->fileData.size() > 0 && tf->getFileName().length() > 0) {
            if (retval != 0 || isHeader) {
              tf->hasErrors = true;
              tf->isComplete = false;
            }
            if (isCompleteFile && !tf->hasErrors)
              tf->isComplete = true;
            tapeFiles_.push_back(tf);
            tf = (TapeFile *) 0;
            tf = new TapeFile;
          }
        }
        tf->fileData.clear();
        tf->hasErrors = false;
        tf->isComplete = false;
        tf->isCopyProtected = false;
        readingFile = false;
        isCompleteFile = false;
        if (retval < 0)
          break;
        if (retval == 0 && isHeader) {
          readingFile = true;
          isCompleteFile = true;
          tf->setFileName(reinterpret_cast<char *>(&(buf.front()) + 2));
          tf->isCopyProtected = (buf[0] == 0x00);
        }
      }
    }
    catch (...) {
      if (tf)
        delete tf;
      delete t;
      throw;
    }
    if (tf)
      delete tf;
    delete t;
  }

  bool TapeFiles::writeTapeImage(const char *fileName_, bool allowOverwrite_)
  {
    if (fileName_ == (char *) 0 || fileName_[0] == '\0')
      throw Ep128::Exception("invalid file name");
    if (!allowOverwrite_) {
      std::FILE *f = std::fopen(fileName_, "rb");
      if (f) {
        std::fclose(f);
        return false;
      }
    }
    {
      TapeOutput  t(fileName_);
      for (size_t i = 0; i < tapeFiles_.size(); i++)
        t.writeFile(*(tapeFiles_[i]));
      t.flush();
    }
    return true;
  }

  void TapeFiles::importFile(const char *fileName_)
  {
    TapeFile  *tf = new TapeFile;
    std::FILE *f = (std::FILE *) 0;
    try {
      if (fileName_ == (char *) 0 || fileName_[0] == '\0')
        throw Ep128::Exception("invalid file name");
      f = std::fopen(fileName_, "rb");
      if (!f)
        throw Ep128::Exception("error opening file");
      tf->setFileName(fileName_);
      int   c;
      while (true) {
        c = std::fgetc(f);
        if (c == EOF)
          break;
        tf->fileData.push_back(uint8_t(c));
      }
      std::fclose(f);
      tapeFiles_.push_back(tf);
    }
    catch (...) {
      if (f)
        std::fclose(f);
      delete tf;
      throw;
    }
  }

  bool TapeFiles::exportFile(int n, const char *fileName_, bool allowOverwrite_)
  {
    if (n < 0 || n >= int(tapeFiles_.size()))
      return true;
    if (fileName_ == (char *) 0 || fileName_[0] == '\0')
      throw Ep128::Exception("invalid file name");
    std::FILE *f = (std::FILE *) 0;
    try {
      if (!allowOverwrite_) {
        f = std::fopen(fileName_, "rb");
        if (f) {
          std::fclose(f);
          return false;
        }
      }
      f = std::fopen(fileName_, "wb");
      if (!f)
        throw Ep128::Exception("error opening file");
      for (size_t i = 0; i < tapeFiles_[n]->fileData.size(); i++) {
        if (std::fputc(tapeFiles_[n]->fileData[i], f) == EOF)
          throw Ep128::Exception("error writing file - is the disk full ?");
      }
      if (std::fclose(f) != 0) {
        f = (std::FILE *) 0;
        throw Ep128::Exception("error writing file - is the disk full ?");
      }
    }
    catch (...) {
      if (f)
        std::fclose(f);
      throw;
    }
    return true;
  }

  void TapeFiles::duplicateFile(int n)
  {
    if (n < 0 || n >= int(tapeFiles_.size()))
      return;
    TapeFile  *tf = new TapeFile;
    try {
      tf->setFileName(tapeFiles_[n]->getFileName());
      tf->hasErrors = tapeFiles_[n]->hasErrors;
      tf->isComplete = tapeFiles_[n]->isComplete;
      tf->isCopyProtected = tapeFiles_[n]->isCopyProtected;
      for (size_t i = 0; i < tapeFiles_[n]->fileData.size(); i++)
        tf->fileData.push_back(tapeFiles_[n]->fileData[i]);
      tapeFiles_.push_back(tf);
    }
    catch (...) {
      delete tf;
      throw;
    }
  }

  void TapeFiles::moveFile(int n, bool isForward)
  {
    if (n < 0 || n >= int(tapeFiles_.size()))
      return;
    if (n == 0 && !isForward)
      return;
    if (n == (int(tapeFiles_.size()) - 1) && isForward)
      return;
    TapeFile  *tmp = tapeFiles_[n];
    if (isForward) {
      tapeFiles_[n] = tapeFiles_[n + 1];
      tapeFiles_[n + 1] = tmp;
    }
    else {
      tapeFiles_[n] = tapeFiles_[n - 1];
      tapeFiles_[n - 1] = tmp;
    }
  }

  void TapeFiles::removeFile(int n)
  {
    if (n < 0 || n >= int(tapeFiles_.size()))
      return;
    if (tapeFiles_[n] != (TapeFile *) 0) {
      delete tapeFiles_[n];
      tapeFiles_[n] = (TapeFile *) 0;
    }
    tapeFiles_.erase(tapeFiles_.begin() + n);
  }

}       // namespace Ep128Emu

