
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

#include "ep128emu.hpp"
#include "tape.hpp"
#include <cstdio>
#include <cstdlib>

static size_t findCuePoint(size_t pos_,
                           uint32_t *cuePointTable, size_t cuePointCnt)
{
  uint32_t  pos = (pos_ < 0xFFFFFFFEUL ?
                   uint32_t(pos_) : uint32_t(0xFFFFFFFEUL));
  size_t    min_ = 1;
  size_t    max_ = cuePointCnt + 1;
  while ((min_ + 1) < max_) {
    size_t  tmp = (min_ + max_) >> 1;
    if (pos > cuePointTable[tmp])
      min_ = tmp;
    else if (pos < cuePointTable[tmp])
      max_ = tmp;
    else
      return tmp;
  }
  return min_;
}

static uint32_t calculateCuePointChecksum(uint32_t *cuePointTable)
{
  unsigned int  h = 1U;

  for (size_t i = 1; i < 1023; i++) {
    uint64_t  tmp;
    h ^= cuePointTable[i];
    tmp = (uint32_t) h * (uint64_t) 0xC2B0C3CCU;
    h = ((unsigned int) tmp ^ (unsigned int) (tmp >> 32)) & 0xFFFFFFFFU;
  }
  return uint32_t(h);
}

static int cuePointCmpFunc(const void *p1, const void *p2)
{
  if (*((uint32_t *) p1) < *((uint32_t *) p2))
    return -1;
  if (*((uint32_t *) p1) > *((uint32_t *) p2))
    return 1;
  return 0;
}

namespace Ep128Emu {

  void Tape::seek_(size_t pos_, bool recording)
  {
    // unless recording, clamp position to tape length
    size_t  pos = ((pos_ < tapeLength || recording) ? pos_ : tapeLength);
    size_t  oldBlockNum = (tapePosition >> 15);
    size_t  newBlockNum = (pos >> 15);

    if (recording && cuePointCnt > 0) {
      size_t  ndx = findCuePoint(pos, cuePointTable, cuePointCnt);
      // if recording over a cue point, delete it
      if (cuePointTable[ndx] == pos) {
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
    if (((newBlockNum + 1) << 15) <= tapeLength) {
      std::fseek(f, blockNumToFilePos_(newBlockNum), SEEK_SET);
      std::fread(buf, 1, 4096, f);
    }
    else if (!recording) {
      // if not recording, the position is clamped to the tape length,
      // and the file is not extended
      std::fseek(f, blockNumToFilePos_(newBlockNum), SEEK_SET);
      long    n = 0L;
      size_t  nBytes = (tapeLength >> 3) - (newBlockNum << 12);
      if (nBytes > 0)
        n = long(std::fread(buf, 1, nBytes, f));
      n = (n > 0L ? n : 0L);
      // pad buffer with zero bytes
      for ( ; n < 4096; n++)
        buf[n] = 0;
    }
    else {
      // extend file length
      std::fseek(f, 0L, SEEK_END);
      long  nBytes = blockNumToFilePos_(newBlockNum + 1) - std::ftell(f);
      long  n = 0L;
      while (n < nBytes) {
        if (std::fputc(0, f) == EOF) {
          err = true;
          break;
        }
        n++;
      }
      tapeLength += (size_t(n) << 3);
      size_t  tmp = tapePosition;
      tapePosition = size_t(0) - 1;
      this->seek_(tmp, false);
    }
    if (err)
      throw Exception("error writing tape file - is the disk full ?");
  }

  void Tape::flushBuffer_()
  {
    if (isBufferDirty) {
      size_t  blockNum = (tapePosition >> 15);
      bool    err = false;
      std::fseek(f, blockNumToFilePos_(blockNum), SEEK_SET);
      long    n = long(std::fwrite(buf, 1, 4096, f));
      isBufferDirty = false;
      n = (n > 0L ? n : 0L);
      if ((((blockNum << 12) + size_t(n)) << 3) > tapeLength)
        tapeLength = ((blockNum << 12) + size_t(n)) << 3;
      if (n != 4096L)
        err = true;
      if (std::fflush(f) != 0)
        err = true;
      if (err)
        throw Exception("error writing tape file - is the disk full ?");
    }
  }

  Tape::Tape(const char *fileName)
    : f((std::FILE *) 0),
      buf((uint8_t *) 0),
      cuePointTable((uint32_t *) 0),
      cuePointCnt(0),
      isReadOnly(true),
      isBufferDirty(false),
      isPlaybackOn(false),
      isRecordOn(false),
      isMotorOn(false),
      haveCuePoints(false),
      tapeLength(0),
      tapePosition(0),
      inputState(0),
      outputState(0)
  {
    try {
      if (fileName == (char *) 0 || fileName[0] == '\0')
        throw Exception("invalid tape file name");
      buf = new uint8_t[4096];
      for (size_t i = 0; i < 4096; i++)
        buf[i] = 0;
      cuePointTable = new uint32_t[1024];
      f = std::fopen(fileName, "r+b");
      if (!f)
        f = std::fopen(fileName, "rb");
      else
        isReadOnly = false;
      if (!f)
        throw Exception("error opening tape file");
      if (std::fseek(f, 0L, SEEK_END) < 0)
        throw Exception("error setting tape file position");
      long  fSize = std::ftell(f);
      if (fSize < 0L)
        throw Exception("cannot find out length of tape file");
      tapeLength = size_t(fSize) << 3;
      std::fseek(f, 0L, SEEK_SET);
      // check for a cue point table
      if (tapeLength >= 32768) {
        std::fread(buf, 1, 4096, f);
        for (size_t i = 0; i < 4096; i += 4) {
          uint32_t  tmp;
          tmp =   (uint32_t(buf[i + 0]) << 24) | (uint32_t(buf[i + 1]) << 16)
                | (uint32_t(buf[i + 2]) << 8)  |  uint32_t(buf[i + 3]);
          cuePointTable[i >> 2] = tmp;
        }
        if (cuePointTable[0] == 0x7B6CDE49 &&
            cuePointTable[1023] == calculateCuePointChecksum(cuePointTable)) {
          std::qsort(&(cuePointTable[1]), 1022, sizeof(uint32_t),
                     &cuePointCmpFunc);
          cuePointTable[1022] = uint32_t(0xFFFFFFFFUL);
          cuePointTable[1023] = calculateCuePointChecksum(cuePointTable);
          while (cuePointTable[cuePointCnt + 1] != uint32_t(0xFFFFFFFFUL))
            cuePointCnt++;
          haveCuePoints = true;
          tapeLength -= 32768;
        }
      }
      if (!haveCuePoints) {
        cuePointTable[0] = 0x7B6CDE49;
        for (size_t i = 1; i < 1023; i++)
          cuePointTable[i] = uint32_t(0xFFFFFFFFUL);
        cuePointTable[1023] = calculateCuePointChecksum(cuePointTable);
      }
      // fill buffer
      std::fseek(f, (haveCuePoints ? 4096L : 0L), SEEK_SET);
      size_t  n = tapeLength >> 3;
      std::fread(buf, 1, (n < 4096 ? n : 4096), f);
    }
    catch (...) {
      if (f)
        std::fclose(f);
      if (cuePointTable)
        delete[] cuePointTable;
      if (buf)
        delete[] buf;
      throw;
    }
  }

  Tape::~Tape()
  {
    // flush any pending file changes, and close file
    // FIXME: errors are not handled here
    try {
      flushBuffer_();
      std::fclose(f);
    }
    catch (...) {
    }
    delete[] cuePointTable;
    delete[] buf;
  }

  void Tape::setIsMotorOn(bool newState)
  {
    isMotorOn = newState;
    if (!isMotorOn)
      flushBuffer_();
  }

  void Tape::stop()
  {
    isPlaybackOn = false;
    isRecordOn = false;
    flushBuffer_();
  }

  void Tape::seek(double t)
  {
    this->seek_(size_t(long(t > 0.0 ? (t * double(sampleRate) + 0.5) : 0.0)),
                false);
  }

  void Tape::seekToCuePoint(bool isForward, double t)
  {
    if (cuePointCnt > 0) {
      size_t  ndx = findCuePoint(tapePosition, cuePointTable, cuePointCnt);
      if ((cuePointTable[ndx] < tapePosition && !isForward) ||
          (cuePointTable[ndx] > tapePosition && isForward)) {
        this->seek_(cuePointTable[ndx], false);
        return;
      }
      else if (ndx > 1 && !isForward) {
        this->seek_(cuePointTable[ndx - 1], false);
        return;
      }
      else if (ndx < cuePointCnt && isForward) {
        this->seek_(cuePointTable[ndx + 1], false);
        return;
      }
    }
    if (isForward)
      this->seek(getPosition() + (t > 0.0 ? t : 0.0));
    else
      this->seek(getPosition() - (t < 0.0 ? t : 0.0));
  }

  void Tape::addCuePoint()
  {
    if (isReadOnly || cuePointCnt >= 1021 || !haveCuePoints)
      return;
    uint32_t  pos = (tapePosition < 0xFFFFFFFEUL ?
                     uint32_t(tapePosition) : uint32_t(0xFFFFFFFEUL));
    size_t    ndx = findCuePoint(pos, cuePointTable, cuePointCnt);
    if (cuePointTable[ndx] == pos)
      return;           // there is already a cue point at this position
    cuePointTable[++cuePointCnt] = pos;
    // sort table
    for (size_t i = cuePointCnt - 1; i >= 1; i--) {
      uint32_t  tmp = cuePointTable[i];
      if (tmp < cuePointTable[i + 1])
        break;
      cuePointTable[i] = cuePointTable[i + 1];
      cuePointTable[i + 1] = tmp;
    }
    cuePointTable[1023] = calculateCuePointChecksum(cuePointTable);
    std::fseek(f, 0L, SEEK_SET);
    for (size_t i = 0; i < 1024; i++) {
      if (std::fputc(int((cuePointTable[i] >> 24) & 0xFF), f) == EOF ||
          std::fputc(int((cuePointTable[i] >> 16) & 0xFF), f) == EOF ||
          std::fputc(int((cuePointTable[i] >> 8) & 0xFF), f) == EOF ||
          std::fputc(int(cuePointTable[i] & 0xFF), f) == EOF)
        throw Exception("error updating cue point table");
    }
  }

  void Tape::deleteNearestCuePoint()
  {
    if (isReadOnly || cuePointCnt < 1)
      return;
    uint32_t  pos = (tapePosition < 0xFFFFFFFEUL ?
                     uint32_t(tapePosition) : uint32_t(0xFFFFFFFEUL));
    size_t    ndx = 0;
    long      nearestDiff = 0x7FFFFFFFL;
    for (size_t i = 1; i <= cuePointCnt; i++) {
      long  tmp = long(cuePointTable[i]) - long(pos);
      tmp = (tmp >= 0L ? tmp : -tmp);
      if (tmp >= nearestDiff)
        break;
      nearestDiff = tmp;
      ndx = i;
    }
    for (size_t i = ndx; i < cuePointCnt; i++)
      cuePointTable[i] = cuePointTable[i + 1];
    cuePointTable[cuePointCnt--] = uint32_t(0xFFFFFFFEUL);
    cuePointTable[1023] = calculateCuePointChecksum(cuePointTable);
    std::fseek(f, 0L, SEEK_SET);
    for (size_t i = 0; i < 1024; i++) {
      if (std::fputc(int((cuePointTable[i] >> 24) & 0xFF), f) == EOF ||
          std::fputc(int((cuePointTable[i] >> 16) & 0xFF), f) == EOF ||
          std::fputc(int((cuePointTable[i] >> 8) & 0xFF), f) == EOF ||
          std::fputc(int(cuePointTable[i] & 0xFF), f) == EOF)
        throw Exception("error updating cue point table");
    }
  }

  void Tape::deleteAllCuePoints()
  {
    if (isReadOnly || cuePointCnt < 1)
      return;
    cuePointTable[0] = 0x7B6CDE49;
    for (size_t i = 1; i < 1023; i++)
      cuePointTable[i] = uint32_t(0xFFFFFFFFUL);
    cuePointTable[1023] = calculateCuePointChecksum(cuePointTable);
    std::fseek(f, 0L, SEEK_SET);
    for (size_t i = 0; i < 1024; i++) {
      if (std::fputc(int((cuePointTable[i] >> 24) & 0xFF), f) == EOF ||
          std::fputc(int((cuePointTable[i] >> 16) & 0xFF), f) == EOF ||
          std::fputc(int((cuePointTable[i] >> 8) & 0xFF), f) == EOF ||
          std::fputc(int(cuePointTable[i] & 0xFF), f) == EOF)
        throw Exception("error updating cue point table");
    }
  }

}       // namespace Ep128Emu

