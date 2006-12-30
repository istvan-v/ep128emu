
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
#include <cstdio>

#ifndef EP128EMU_TAPE_HPP
#define EP128EMU_TAPE_HPP

namespace Ep128Emu {

  class Tape {
   private:
    static const long sampleRate = 24000L;
    std::FILE *f;               // tape image file
    uint8_t   *buf;             // 4096 bytes ( = 32768 samples)
    uint32_t  *cuePointTable;   // the table has 1024 elements, and contains:
                                //         0: magic number (0x7B6CDE49,
                                //            big-endian)
                                // 1 to 1022: the sample positions where cue
                                //            points are defined, in sorted
                                //            order, padded with 0xFFFFFFFF
                                //            values
                                //      1023: 32 bit checksum (big-endian)
    size_t    cuePointCnt;
    bool      isReadOnly;
    bool      isBufferDirty;    // true if 'buf' has been changed,
                                // and not written to file yet
    bool      isPlaybackOn;
    bool      isRecordOn;
    bool      isMotorOn;
    bool      haveCuePoints;
    size_t    tapeLength;       // tape length (in samples)
    size_t    tapePosition;     // current read/write position (in samples)
    int       inputState;
    int       outputState;
    // ----------------
    void seek_(size_t pos, bool recording);
    inline void setSample_(int newState)
    {
      uint8_t mask = uint8_t(128 >> (tapePosition & 7));
      buf[(tapePosition & 0x7FF8) >> 3] =
          (buf[(tapePosition & 0x7FF8) >> 3] & (mask ^ 0xFF))
          | (newState > 0 ? mask : 0);
    }
    inline int getSample_() const
    {
      return ((buf[(tapePosition & 0x7FF8) >> 3]
               & uint8_t(128 >> (tapePosition & 7))) == 0 ? 0 : 1);
    }
    inline long blockNumToFilePos_(size_t blockNum_)
    {
      return long((haveCuePoints ? (blockNum_ + 1) : blockNum_) << 12);
    }
    void flushBuffer_();
   public:
    Tape(const char *fileName);
    virtual ~Tape();
    // get sample rate of tape emulation
    inline long getSampleRate() const
    {
      return sampleRate;
    }
    // run tape emulation for a period of 1.0 / getSampleRate() seconds
    inline void runOneSample()
    {
      if (isPlaybackOn && isMotorOn) {
        outputState = getSample_();
        if (isRecordOn) {
          setSample_(inputState);
          isBufferDirty = true;
        }
        seek_(tapePosition + 1, isRecordOn);
      }
    }
    // turn motor on (newState = true) or off (newState = false)
    void setIsMotorOn(bool newState);
    // returns true if the motor is currently on
    inline bool getIsMotorOn() const
    {
      return isMotorOn;
    }
    // start playback
    // (note: the motor should also be turned on to actually play)
    inline void play()
    {
      isPlaybackOn = true;
      isRecordOn = false;
    }
    // start recording; if the tape file is read-only, this is equivalent
    // to calling play()
    // (note: the motor should also be turned on to actually record)
    inline void record()
    {
      isPlaybackOn = true;
      isRecordOn = !isReadOnly;
    }
    // stop playback and recording
    void stop();
    // set input signal for recording
    inline void setInputSignal(int newState)
    {
      inputState = (newState > 0 ? 1 : 0);
    }
    // get output signal
    inline int getOutputSignal() const
    {
      return (outputState > 0 ? 1 : 0);
    }
    // Seek to the specified time (in seconds).
    void seek(double t);
    // Returns the current tape position in seconds.
    inline double getPosition() const
    {
      return (double(long(tapePosition)) / double(sampleRate));
    }
    // Seek forward (if isForward = true) or backward (if isForward = false)
    // to the nearest cue point, or by 't' seconds if no cue point is found.
    void seekToCuePoint(bool isForward = true, double t = 10.0);
    // Create a new cue point at the current tape position.
    // Has no effect if the file does not have a cue point table, or it
    // is read-only.
    void addCuePoint();
    // Delete the cue point nearest to the current tape position.
    // Has no effect if the file is read-only.
    void deleteNearestCuePoint();
    // Delete all cue points. Has no effect if the file is read-only.
    void deleteAllCuePoints();
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_TAPE_HPP

