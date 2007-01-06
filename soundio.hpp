
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

#ifndef EP128EMU_SOUNDIO_HPP
#define EP128EMU_SOUNDIO_HPP

#include "ep128emu.hpp"
#include "system.hpp"

#include <sndfile.h>
#include <portaudio.h>
#include <cstring>
#include <vector>

namespace Ep128Emu {

  class AudioOutput {
   private:
    std::string outputFileName;
    SNDFILE *soundFile;
   protected:
    int     deviceNumber;
    float   sampleRate;
    float   totalLatency;
    int     nPeriodsHW;
    int     nPeriodsSW;
   public:
    AudioOutput();
    virtual ~AudioOutput();
    // set audio output parameters (changing these settings implies
    // restarting the audio output stream if it is already open)
    void setParameters(int deviceNumber_, float sampleRate_,
                       float totalLatency_ = 0.1f,
                       int nPeriodsHW_ = 4, int nPeriodsSW_ = 4);
    inline float getSampleRate() const
    {
      return this->sampleRate;
    }
    // write sound output to the specified file name, closing any
    // previously opened file with a different name
    // if the name is an empty string, no file is written
    void setOutputFile(const std::string& fileName);
    // write 'nFrames' interleaved stereo sample frames from 'buf'
    // (in 16 bit signed PCM format) to the audio output device and file
    virtual void sendAudioData(const int16_t *buf, size_t nFrames);
    // close the audio device
    virtual void closeDevice();
    // returns an array of the available audio device names,
    // indexed by the device number (starting from zero)
    virtual std::vector< std::string > getDeviceList();
   protected:
    virtual void openDevice();
  };

  class AudioOutput_PortAudio : public AudioOutput {
   private:
    struct Buffer {
      ThreadLock  paLock;
      ThreadLock  epLock;
      std::vector< int16_t >    audioData;
      size_t      writePos;
      Buffer()
        : paLock(true), epLock(false), writePos(0)
      {
      }
      ~Buffer()
      {
      }
    };
    bool    paInitialized;
    std::vector< Buffer >   buffers;
    size_t  writeBufIndex;
    size_t  readBufIndex;
    PaStream  *paStream;
#ifndef USING_OLD_PORTAUDIO_API
    static int portAudioCallback(const void *input, void *output,
                                 unsigned long frameCount,
                                 const PaStreamCallbackTimeInfo *timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void *userData);
#else
    static int portAudioCallback(void *input, void *output,
                                 unsigned long frameCount,
                                 PaTimestamp outTime, void *userData);
#endif
   public:
    AudioOutput_PortAudio();
    virtual ~AudioOutput_PortAudio();
    virtual void sendAudioData(const int16_t *buf, size_t nFrames);
    virtual void closeDevice();
    virtual std::vector< std::string > getDeviceList();
   protected:
    virtual void openDevice();
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_SOUNDIO_HPP

