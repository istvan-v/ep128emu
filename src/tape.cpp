
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2007 Istvan Varga <istvanv@users.sourceforge.net>
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

#include "ep128emu.hpp"
#include "tape.hpp"
#include <cstdio>
#include <cstdlib>

static const char *epteFileMagic = "ENTERPRISE 128K TAPE FILE       ";

static int cuePointCmpFunc(const void *p1, const void *p2)
{
  if (*((uint32_t *) p1) < *((uint32_t *) p2))
    return -1;
  if (*((uint32_t *) p1) > *((uint32_t *) p2))
    return 1;
  return 0;
}

namespace Ep128Emu {

  Tape::Tape(int bitsPerSample)
    : sampleRate(24000L),
      fileBitsPerSample(1),
      requestedBitsPerSample(bitsPerSample),
      isReadOnly(true),
      isPlaybackOn(false),
      isRecordOn(false),
      isMotorOn(false),
      tapeLength(0),
      tapePosition(0),
      inputState(0),
      outputState(0)
  {
    if (!(requestedBitsPerSample == 1 || requestedBitsPerSample == 2 ||
          requestedBitsPerSample == 4 || requestedBitsPerSample == 8))
      throw Exception("invalid tape sample size");
  }

  Tape::~Tape()
  {
  }

  void Tape::runOneSample_()
  {
  }

  void Tape::setIsMotorOn(bool newState)
  {
    isMotorOn = newState;
  }

  void Tape::stop()
  {
    isPlaybackOn = false;
    isRecordOn = false;
  }

  void Tape::seek(double t)
  {
    (void) t;
  }

  void Tape::seekToCuePoint(bool isForward, double t)
  {
    (void) isForward;
    (void) t;
  }

  void Tape::addCuePoint()
  {
  }

  void Tape::deleteNearestCuePoint()
  {
  }

  void Tape::deleteAllCuePoints()
  {
  }

  // --------------------------------------------------------------------------

  bool Tape_Ep128Emu::findCuePoint_(size_t& ndx_, size_t pos_)
  {
    uint32_t  *cuePointTable = &(fileHeader[4]);
    uint32_t  pos = (pos_ < 0xFFFFFFFEUL ?
                     uint32_t(pos_) : uint32_t(0xFFFFFFFEUL));
    size_t    min_ = 0;
    size_t    max_ = cuePointCnt;
    while ((min_ + 1) < max_) {
      size_t  tmp = (min_ + max_) >> 1;
      if (pos > cuePointTable[tmp])
        min_ = tmp;
      else if (pos < cuePointTable[tmp])
        max_ = tmp;
      else {
        ndx_ = tmp;
        return true;
      }
    }
    ndx_ = min_;
    return (cuePointTable[min_] == pos);
  }

  void Tape_Ep128Emu::packSamples_()
  {
    uint8_t maxValue = uint8_t((1 << requestedBitsPerSample) - 1);
    uint8_t *buf_ = buf;
    if (fileBitsPerSample == requestedBitsPerSample) {
      for (size_t i = 0; i < 4096; i++) {
        uint8_t c = buf_[i];
        buf_[i] = (c < maxValue ? c : maxValue);
      }
    }
    else if (fileBitsPerSample < requestedBitsPerSample) {
      uint8_t rShift = uint8_t(requestedBitsPerSample - fileBitsPerSample);
      for (size_t i = 0; i < 4096; i++) {
        uint8_t c = buf_[i];
        buf_[i] = (c < maxValue ? c : maxValue) >> rShift;
      }
    }
    else {
      uint8_t lShift = uint8_t(fileBitsPerSample - requestedBitsPerSample);
      for (size_t i = 0; i < 4096; i++) {
        uint8_t c = buf_[i];
        buf_[i] = (c < maxValue ? c : maxValue) << lShift;
      }
    }
    if (fileBitsPerSample != 8) {
      int     nBits = fileBitsPerSample;
      int     byteBuf = 0;
      int     bitCnt = 8;
      size_t  writePos = 0;
      for (size_t i = 0; i < 4096; i++) {
        byteBuf = (byteBuf << nBits) | int(buf_[i]);
        bitCnt = bitCnt - nBits;
        if (!bitCnt) {
          buf_[writePos++] = uint8_t(byteBuf);
          byteBuf = 0;
          bitCnt = 8;
        }
      }
    }
  }

  bool Tape_Ep128Emu::writeBuffer_()
  {
    // calculate file position
    size_t  blockSize = 512U * (unsigned int) fileBitsPerSample;
    size_t  filePos = (tapePosition >> 12) * blockSize;
    if (usingNewFormat)
      filePos = filePos + 4096;
    if (std::fseek(f, long(filePos), SEEK_SET) < 0)
      return false;
    // write data
    long    n = long(std::fwrite(buf, sizeof(uint8_t), blockSize, f));
    n = (n >= 0L ? n : 0L);
    size_t  endPos = filePos + size_t(n);
    if (usingNewFormat)
      endPos = endPos - 4096;
    endPos = endPos * (8U / (unsigned int) fileBitsPerSample);
    if (endPos > tapeLength)
      tapeLength = endPos;
    bool    err = (n != long(blockSize));
    if (std::fflush(f) != 0)
      err = true;
    return (!err);
  }

  bool Tape_Ep128Emu::readBuffer_()
  {
    bool    err = false;
    long    n = 0L;
    size_t  blockSize = 512U * (unsigned int) fileBitsPerSample;
    if ((tapePosition & 0xFFFFF000UL) < tapeLength) {
      // calculate file position
      size_t  filePos = (tapePosition >> 12) * blockSize;
      if (usingNewFormat)
        filePos = filePos + 4096;
      if (std::fseek(f, long(filePos), SEEK_SET) >= 0) {
        // read data
        n = long(std::fread(buf, sizeof(uint8_t), blockSize, f));
        n = (n >= 0L ? n : 0L);
      }
    }
    if (n < long(blockSize)) {
      err = true;
      // pad with zero bytes
      do {
        buf[n] = 0;
      } while (++n < long(blockSize));
    }
    return (!err);
  }

  void Tape_Ep128Emu::unpackSamples_()
  {
    uint8_t *buf_ = buf;
    if (fileBitsPerSample != 8) {
      int     nBits = fileBitsPerSample;
      int     byteBuf = 0;
      int     bitCnt = 0;
      int     bitMask = (1 << fileBitsPerSample) - 1;
      size_t  readPos = 512U * (unsigned int) fileBitsPerSample;
      for (size_t i = 4096; i != 0; ) {
        if (!bitCnt) {
          byteBuf = int(buf_[--readPos]);
          bitCnt = 8;
        }
        buf_[--i] = uint8_t(byteBuf & bitMask);
        byteBuf = byteBuf >> nBits;
        bitCnt = bitCnt - nBits;
      }
    }
    if (fileBitsPerSample < requestedBitsPerSample) {
      uint8_t lShift = uint8_t(requestedBitsPerSample - fileBitsPerSample);
      for (size_t i = 0; i < 4096; i++)
        buf_[i] = buf_[i] << lShift;
    }
    else if (fileBitsPerSample > requestedBitsPerSample) {
      uint8_t rShift = uint8_t(fileBitsPerSample - requestedBitsPerSample);
      for (size_t i = 0; i < 4096; i++)
        buf_[i] = buf_[i] >> rShift;
    }
  }

  void Tape_Ep128Emu::seek_(size_t pos_)
  {
    // clamp position to tape length
    size_t  pos = (pos_ < tapeLength ? pos_ : tapeLength);
    size_t  oldBlockNum = (tapePosition >> 12);
    size_t  newBlockNum = (pos >> 12);

    if (newBlockNum == oldBlockNum) {
      tapePosition = pos;
      return;
    }
    bool    err = false;
    try {
      // flush any pending file changes
      flushBuffer_();
    }
    catch (...) {
      err = true;
    }
    tapePosition = pos;
    readBuffer_();
    unpackSamples_();
    if (err)
      throw Exception("error writing tape file - is the disk full ?");
  }

  bool Tape_Ep128Emu::writeHeader_()
  {
    if (!usingNewFormat)
      return true;
    if (std::fseek(f, 0L, SEEK_SET) < 0)
      return false;
    bool      err = false;
    uint32_t  tmp = 0U;
    for (size_t i = 0; i < 4096; i++) {
      if (!(i & 3)) {
        tmp = fileHeader[i >> 2];
        tmp = ((tmp & 0xFF000000U) >> 24) | ((tmp & 0x00FF0000U) >> 8)
              | ((tmp & 0x0000FF00U) << 8) | ((tmp & 0x000000FFU) << 24);
      }
      if (std::fputc(int(tmp & 0xFFU), f) == EOF) {
        err = true;
        break;
      }
      tmp = tmp >> 8;
    }
    if (std::fflush(f) != 0)
      err = true;
    return (!err);
  }

  void Tape_Ep128Emu::flushBuffer_()
  {
    if (isBufferDirty) {
      packSamples_();
      bool    err = !(writeBuffer_());
      unpackSamples_();
      isBufferDirty = false;
      if (err)
        throw Exception("error writing tape file - is the disk full ?");
    }
  }

  Tape_Ep128Emu::Tape_Ep128Emu(const char *fileName, int mode,
                               long sampleRate_, int bitsPerSample)
    : Tape(bitsPerSample),
      f((std::FILE *) 0),
      buf((uint8_t *) 0),
      fileHeader((uint32_t *) 0),
      cuePointCnt(0),
      isBufferDirty(false),
      usingNewFormat(false)
  {
    isReadOnly = false;
    try {
      if (fileName == (char *) 0 || fileName[0] == '\0')
        throw Exception("invalid tape file name");
      if (sampleRate_ < 10000L || sampleRate_ > 120000L)
        throw Exception("invalid tape sample rate");
      if (!(mode >= 0 && mode <= 3))
        throw Exception("invalid tape open mode parameter");
      buf = new uint8_t[4096];
      for (size_t i = 0; i < 4096; i++)
        buf[i] = 0;
      fileHeader = new uint32_t[1024];
      if (mode == 0 || mode == 1)
        f = std::fopen(fileName, "r+b");
      if (f == (std::FILE *) 0 && mode != 3) {
        f = std::fopen(fileName, "rb");
        if (f)
          isReadOnly = true;
      }
      if (f == (std::FILE *) 0 && (mode == 0 || mode == 3)) {
        // create new tape file
        f = std::fopen(fileName, "w+b");
        if (f) {
          usingNewFormat = true;
          sampleRate = sampleRate_;
          fileBitsPerSample = requestedBitsPerSample;
          fileHeader[0] = 0x0275CD72U;
          fileHeader[1] = 0x1C445126U;
          fileHeader[2] = uint32_t(fileBitsPerSample);
          fileHeader[3] = uint32_t(sampleRate);
          for (size_t i = 4; i < 1024; i++)
            fileHeader[i] = 0xFFFFFFFFU;
          if (!writeHeader_()) {
            std::fclose(f);
            std::remove(fileName);
            f = (std::FILE *) 0;
          }
        }
      }
      if (!f)
        throw Exception("error opening tape file");
      if (!usingNewFormat) {            // if opening an already existing file:
        if (std::fseek(f, 0L, SEEK_END) < 0)
          throw Exception("error setting tape file position");
        long  fSize = std::ftell(f);
        if (fSize < 0L)
          throw Exception("cannot find out length of tape file");
        std::fseek(f, 0L, SEEK_SET);
        // check file header
        if (fSize >= 4096L) {
          std::fread(buf, 1, 4096, f);
          for (size_t i = 0; i < 4096; i += 4) {
            uint32_t  tmp;
            tmp =   (uint32_t(buf[i + 0]) << 24) | (uint32_t(buf[i + 1]) << 16)
                  | (uint32_t(buf[i + 2]) << 8)  |  uint32_t(buf[i + 3]);
            fileHeader[i >> 2] = tmp;
          }
          if (fileHeader[0] == 0x0275CD72U && fileHeader[1] == 0x1C445126U &&
              (fileHeader[2] == 1U || fileHeader[2] == 2U ||
               fileHeader[2] == 4U || fileHeader[2] == 8U) &&
              (fileHeader[3] >= 10000U && fileHeader[3] <= 120000U) &&
              fileHeader[1023] == 0xFFFFFFFFU) {
            usingNewFormat = true;
            sampleRate = long(fileHeader[3]);
            fileBitsPerSample = int(fileHeader[2]);
            std::qsort(&(fileHeader[4]), 1020, sizeof(uint32_t),
                       &cuePointCmpFunc);
            while (fileHeader[cuePointCnt + 4] != uint32_t(0xFFFFFFFFUL))
              cuePointCnt++;
          }
        }
        tapeLength = size_t(fSize);
        if (usingNewFormat)
          tapeLength = tapeLength - 4096;
        tapeLength = (tapeLength << 3) / (unsigned int) fileBitsPerSample;
        if (!usingNewFormat) {
          fileHeader[0] = 0x0275CD72U;
          fileHeader[1] = 0x1C445126U;
          fileHeader[2] = uint32_t(fileBitsPerSample);
          fileHeader[3] = uint32_t(sampleRate);
          for (size_t i = 4; i < 1024; i++)
            fileHeader[i] = 0xFFFFFFFFU;
        }
        // fill buffer
        readBuffer_();
        unpackSamples_();
      }
    }
    catch (...) {
      if (f)
        std::fclose(f);
      if (fileHeader)
        delete[] fileHeader;
      if (buf)
        delete[] buf;
      throw;
    }
  }

  Tape_Ep128Emu::~Tape_Ep128Emu()
  {
    // flush any pending file changes, and close file
    // FIXME: errors are not handled here
    try {
      flushBuffer_();
    }
    catch (...) {
    }
    std::fclose(f);
    delete[] fileHeader;
    delete[] buf;
  }

  void Tape_Ep128Emu::runOneSample_()
  {
    outputState = buf[tapePosition & 0x0FFF];
    if (isRecordOn) {
      buf[tapePosition & 0x0FFF] =
          uint8_t(inputState > 0 ? (inputState < 255 ? inputState : 255) : 0);
      isBufferDirty = true;
    }
    size_t  pos = tapePosition + 1;
    // unless recording, clamp position to tape length
    pos = ((pos < tapeLength || isRecordOn) ? pos : tapeLength);
    size_t  oldBlockNum = (tapePosition >> 12);
    size_t  newBlockNum = (pos >> 12);

    if (isRecordOn && cuePointCnt > 0) {
      size_t  ndx = 0;
      if (findCuePoint_(ndx, pos)) {
        // if recording over a cue point, delete it
        size_t  savedPos = tapePosition;
        tapePosition = pos;
        deleteNearestCuePoint();
        tapePosition = savedPos;
      }
    }
    if (newBlockNum == oldBlockNum) {
      tapePosition = pos;
      return;
    }
    bool    err = false;
    try {
      // flush any pending file changes
      flushBuffer_();
    }
    catch (...) {
      err = true;
    }
    tapePosition = pos;
    readBuffer_();
    unpackSamples_();
    if (err)
      throw Exception("error writing tape file - is the disk full ?");
  }

  void Tape_Ep128Emu::setIsMotorOn(bool newState)
  {
    isMotorOn = newState;
    if (!isMotorOn)
      flushBuffer_();
  }

  void Tape_Ep128Emu::stop()
  {
    isPlaybackOn = false;
    isRecordOn = false;
    flushBuffer_();
  }

  void Tape_Ep128Emu::seek(double t)
  {
    this->seek_(size_t(long(t > 0.0 ? (t * double(sampleRate) + 0.5) : 0.0)));
  }

  void Tape_Ep128Emu::seekToCuePoint(bool isForward, double t)
  {
    if (cuePointCnt > 0) {
      size_t    ndx = 0;
      findCuePoint_(ndx, tapePosition);
      uint32_t  *cuePointTable = &(fileHeader[4]);
      if ((cuePointTable[ndx] < tapePosition && !isForward) ||
          (cuePointTable[ndx] > tapePosition && isForward)) {
        this->seek_(cuePointTable[ndx]);
        return;
      }
      else if (ndx > 0 && !isForward) {
        this->seek_(cuePointTable[ndx - 1]);
        return;
      }
      else if ((ndx + 1) < cuePointCnt && isForward) {
        this->seek_(cuePointTable[ndx + 1]);
        return;
      }
    }
    if (isForward)
      this->seek(getPosition() + (t > 0.0 ? t : 0.0));
    else
      this->seek(getPosition() - (t < 0.0 ? t : 0.0));
  }

  void Tape_Ep128Emu::addCuePoint()
  {
    if (isReadOnly || cuePointCnt >= 1019 || !usingNewFormat)
      return;
    uint32_t  *cuePointTable = &(fileHeader[4]);
    uint32_t  pos = (tapePosition < 0xFFFFFFFEUL ?
                     uint32_t(tapePosition) : uint32_t(0xFFFFFFFEUL));
    size_t    ndx = 0;
    if (findCuePoint_(ndx, pos))
      return;           // there is already a cue point at this position
    cuePointTable[cuePointCnt++] = pos;
    // sort table
    for (size_t i = cuePointCnt - 1; i != 0; ) {
      uint32_t  tmp = cuePointTable[--i];
      if (tmp < cuePointTable[i + 1])
        break;
      cuePointTable[i] = cuePointTable[i + 1];
      cuePointTable[i + 1] = tmp;
    }
    if (!writeHeader_())
      throw Exception("error updating cue point table");
  }

  void Tape_Ep128Emu::deleteNearestCuePoint()
  {
    if (isReadOnly || cuePointCnt < 1)
      return;
    uint32_t  *cuePointTable = &(fileHeader[4]);
    uint32_t  pos = (tapePosition < 0xFFFFFFFEUL ?
                     uint32_t(tapePosition) : uint32_t(0xFFFFFFFEUL));
    size_t    ndx = 0;
    uint32_t  nearestDiff = uint32_t(0xFFFFFFFFUL);
    for (size_t i = 0; i < cuePointCnt; i++) {
      uint32_t  tmp = (cuePointTable[i] >= pos ?
                       (cuePointTable[i] - pos) : (pos - cuePointTable[i]));
      if (tmp >= nearestDiff)
        break;
      nearestDiff = tmp;
      ndx = i;
    }
    for (size_t i = (ndx + 1); i < cuePointCnt; i++)
      cuePointTable[i - 1] = cuePointTable[i];
    cuePointTable[cuePointCnt--] = uint32_t(0xFFFFFFFFUL);
    if (!writeHeader_())
      throw Exception("error updating cue point table");
  }

  void Tape_Ep128Emu::deleteAllCuePoints()
  {
    if (isReadOnly || cuePointCnt < 1)
      return;
    for (size_t i = 4; i < 1024; i++)
      fileHeader[i] = uint32_t(0xFFFFFFFFUL);
    cuePointCnt = 0;
    if (!writeHeader_())
      throw Exception("error updating cue point table");
  }

  // --------------------------------------------------------------------------

  Tape_EPTE::Tape_EPTE(const char *fileName, int bitsPerSample)
    : Tape(bitsPerSample),
      f((std::FILE *) 0),
      bytesRemaining(0),
      endOfTape(false),
      shiftReg(0x00),
      bitsRemaining(0),
      halfPeriodSamples(5),
      samplesRemaining(0),
      leaderSampleCnt(0),
      chunkBytesRemaining(0),
      chunkCnt(0)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Exception("invalid tape file name");
    f = std::fopen(fileName, "rb");
    if (!f)
      throw Exception("error opening tape file");
    bool    isEPTEFile = false;
    if (std::fseek(f, 0L, SEEK_END) >= 0) {
      if (std::ftell(f) >= 512L) {
        std::fseek(f, 128L, SEEK_SET);
        for (int i = 0; i < 32; i++) {
          if (std::fgetc(f) != int(epteFileMagic[i]))
            break;
          if (i == 31)
            isEPTEFile = true;
        }
      }
    }
    if (!isEPTEFile) {
      std::fclose(f);
      throw Exception("invalid tape file header");
    }
    this->seek(0.0);
  }

  Tape_EPTE::~Tape_EPTE()
  {
    if (f)
      std::fclose(f);
  }

  void Tape_EPTE::runOneSample_()
  {
    if (endOfTape) {
      outputState = 0;
      return;
    }
    if (!samplesRemaining) {
      outputState = (outputState == 0 ?
                     (1 << (requestedBitsPerSample - 1)) : 0);
      if (halfPeriodSamples == 8) {
        // sync bit
        if (outputState == 0)
          halfPeriodSamples = 6;
        samplesRemaining = 8;
      }
      else {
        if (outputState != 0 && leaderSampleCnt == 0) {
          if (!bitsRemaining) {
            if (chunkBytesRemaining) {
              chunkBytesRemaining--;
              shiftReg = uint8_t(std::fgetc(f));
              bytesRemaining--;
              bitsRemaining = 8;
            }
            else {
              // end of chunk, start leader
              halfPeriodSamples = 5;
              leaderSampleCnt = 120000; // 5 seconds
            }
          }
          if (bitsRemaining) {
            bitsRemaining--;
            if (!(shiftReg & 0x01))
              halfPeriodSamples = 6;    // bit = 0
            else
              halfPeriodSamples = 4;    // bit = 1
            shiftReg = shiftReg >> 1;
          }
        }
        samplesRemaining = halfPeriodSamples;
      }
    }
    if (leaderSampleCnt) {
      leaderSampleCnt--;
      if (!leaderSampleCnt) {
        if (!bytesRemaining) {
          endOfTape = true;             // end of tape
          outputState = 0;
          samplesRemaining = 0;
          tapePosition = tapeLength;
          return;
        }
        // sync bit
        halfPeriodSamples = 8;
        // next chunk
        size_t  startPos = 0;
        size_t  endPos = 0;
        if (chunkCnt < 128) {
          std::fseek(f, long(chunkCnt << 2), SEEK_SET);
          startPos = size_t(std::fgetc(f) & 0xFF);
          startPos |= (size_t(std::fgetc(f) & 0xFF) << 8);
          startPos |= (size_t(std::fgetc(f) & 0xFF) << 16);
          startPos |= (size_t(std::fgetc(f) & 0xFF) << 24);
          if (++chunkCnt < 128) {
            endPos = size_t(std::fgetc(f) & 0xFF);
            endPos |= (size_t(std::fgetc(f) & 0xFF) << 8);
            endPos |= (size_t(std::fgetc(f) & 0xFF) << 16);
            endPos |= (size_t(std::fgetc(f) & 0xFF) << 24);
          }
          std::fseek(f, long(startPos + 512), SEEK_SET);
        }
        if (endPos > startPos)
          chunkBytesRemaining = endPos - startPos;
        else
          chunkBytesRemaining = bytesRemaining;
      }
    }
    if (samplesRemaining > 0)
      samplesRemaining--;
    tapePosition++;
    tapeLength = tapePosition + (bytesRemaining * 80) + leaderSampleCnt + 1;
  }

  void Tape_EPTE::setIsMotorOn(bool newState)
  {
    isMotorOn = newState;
  }

  void Tape_EPTE::stop()
  {
    isPlaybackOn = false;
    isRecordOn = false;
  }

  void Tape_EPTE::seek(double t)
  {
    if (t <= 0.0) {
      tapeLength = 0;
      tapePosition = 0;
      outputState = 0;
      bytesRemaining = 0;
      endOfTape = false;
      shiftReg = 0x00;
      bitsRemaining = 0;
      halfPeriodSamples = 5;
      samplesRemaining = 0;
      leaderSampleCnt = 0;
      chunkBytesRemaining = 0;
      chunkCnt = 0;
      if (std::fseek(f, 0L, SEEK_END) >= 0) {
        long    n = std::ftell(f);
        if (n > 512L) {
          bytesRemaining = size_t(n - 512L);
          tapeLength = bytesRemaining * 80;
          std::fseek(f, 512L, SEEK_SET);
        }
      }
    }
  }

  void Tape_EPTE::seekToCuePoint(bool isForward, double t)
  {
    (void) t;
    if (!isForward)
      this->seek(0.0);
  }

  void Tape_EPTE::addCuePoint()
  {
  }

  void Tape_EPTE::deleteNearestCuePoint()
  {
  }

  void Tape_EPTE::deleteAllCuePoints()
  {
  }

  // --------------------------------------------------------------------------

  Tape *openTapeFile(const char *fileName, int mode,
                     long sampleRate_, int bitsPerSample)
  {
    Tape    *t = (Tape *) 0;
    if (mode != 3) {
      try {
        t = new Tape_EPTE(fileName, bitsPerSample);
      }
      catch (...) {
      }
    }
    if (!t)
      t = new Tape_Ep128Emu(fileName, mode, sampleRate_, bitsPerSample);
    return t;
  }

}       // namespace Ep128Emu

