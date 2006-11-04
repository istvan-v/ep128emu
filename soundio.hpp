
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

#ifndef EP128EMU_SOUNDIO_HPP
#define EP128EMU_SOUNDIO_HPP

#include "ep128.hpp"
#include "system.hpp"
#include "dave.hpp"

#include <sndfile.h>
#include <portaudio.h>
#include <cstring>
#include <vector>

namespace Ep128Emu {

  template <typename T>
  class AudioOutput : public T {
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
    std::string outputFileName;
    SNDFILE *sf;
    bool    paInitialized;
    std::vector< Buffer >   buffers;
    size_t  writeBufIndex;
    size_t  readBufIndex;
    PaStream  *paStream;
   public:
    // write sound output to the specified file name, closing any
    // previously opened file with a different name
    // if the name is an empty string, no file is written
    void setOutputFile(const std::string& fileName)
    {
      if (fileName == outputFileName)
        return;
      if (sf != (SNDFILE *) 0) {
        sf_close(sf);
        sf = (SNDFILE *) 0;
      }
      if (fileName.length() != 0) {
        SF_INFO sfinfo;
        std::memset(&sfinfo, 0, sizeof(SF_INFO));
        sfinfo.frames = -1;
        sfinfo.samplerate = int(this->outputSampleRate + 0.5);
        sfinfo.channels = 2;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        sf = sf_open(fileName.c_str(), SFM_WRITE, &sfinfo);
        if (!sf)
          throw Ep128::Exception("error opening output sound file");
      }
    }
    static int portAudioCallback(const void *input, void *output,
                                 unsigned long frameCount,
                                 const PaStreamCallbackTimeInfo *timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void *userData)
    {
      AudioOutput *p = reinterpret_cast<AudioOutput *>(userData);
      int16_t *buf = reinterpret_cast<int16_t *>(output);
      size_t  i = 0, nFrames = frameCount;
      (void) input;
      (void) timeInfo;
      (void) statusFlags;
      if (nFrames > (p->buffers[p->readBufIndex].audioData.size() >> 1))
        nFrames = p->buffers[p->readBufIndex].audioData.size() >> 1;
      nFrames <<= 1;
      if (p->buffers[p->readBufIndex].paLock.wait(0)) {
        for ( ; i < nFrames; i++)
          buf[i] = p->buffers[p->readBufIndex].audioData[i];
      }
      p->buffers[p->readBufIndex].epLock.notify();
      if (++(p->readBufIndex) >= p->buffers.size())
        p->readBufIndex = 0;
      for ( ; i < (frameCount << 1); i++)
        buf[i] = 0;
      return int(paContinue);
    }
    AudioOutput(float inputSampleRate_, float outputSampleRate_,
                float dcBlockFreq1 = 10.0f, float dcBlockFreq2 = 10.0f,
                float ampScale_ = 0.7071f,
                int devNum = 0, float totalLatency = 0.1f,
                int nPeriodsHW = 4, int nPeriodsSW = 3,
                const char *outputFileName_ = (char*) 0)
      : T(inputSampleRate_, outputSampleRate_,
          dcBlockFreq1, dcBlockFreq2, ampScale_)
    {
      outputFileName = "";
      sf = (SNDFILE *) 0;
      paInitialized = false;
      writeBufIndex = 0;
      readBufIndex = 0;
      paStream = (PaStream *) 0;
      // calculate buffer size and number of buffers
      totalLatency = (totalLatency > 0.005f ?
                      (totalLatency < 0.5f ? totalLatency : 0.5f) : 0.005f);
      nPeriodsHW = (nPeriodsHW > 2 ? (nPeriodsHW < 16 ? nPeriodsHW : 16) : 2);
      nPeriodsSW = (nPeriodsSW > 2 ? (nPeriodsSW < 16 ? nPeriodsSW : 16) : 2);
      int     periodSize = int(totalLatency * outputSampleRate_ + 0.5)
                           / (nPeriodsHW + nPeriodsSW - 2);
      for (int i = 16; i < 16384; i <<= 1) {
        if (i >= periodSize) {
          periodSize = i;
          break;
        }
      }
      if (periodSize > 16384)
        periodSize = 16384;
      // initialize buffers
      buffers.resize(size_t(nPeriodsSW));
      for (int i = 0; i < nPeriodsSW; i++) {
        buffers[i].audioData.resize(size_t(periodSize) << 1);
        for (int j = 0; j < (periodSize << 1); j++)
          buffers[i].audioData[j] = 0;
      }
      // open output sound file (if requested)
      if (outputFileName_)
        setOutputFile(outputFileName_);
      try {
        // initialize PortAudio
        if (Pa_Initialize() != paNoError)
          throw Ep128::Exception("error initializing PortAudio");
        paInitialized = true;
        // find audio device
        int     devCnt = int(Pa_GetDeviceCount());
        if (devCnt < 1)
          throw Ep128::Exception("no audio device is available");
        int     devIndex;
        for (devIndex = 0; devIndex < devCnt; devIndex++) {
          const PaDeviceInfo  *devInfo;
          devInfo = Pa_GetDeviceInfo(PaDeviceIndex(devIndex));
          if (!devInfo)
            throw Ep128::Exception("error querying audio device information");
          if (devInfo->maxOutputChannels >= 2) {
            if (--devNum == -1) {
              devNum = devIndex;
              break;
            }
          }
        }
        if (devIndex >= devCnt)
          throw Ep128::Exception("device number is out of range");
        // open audio stream
        PaStreamParameters  streamParams;
        std::memset(&streamParams, 0, sizeof(PaStreamParameters));
        streamParams.device = PaDeviceIndex(devNum);
        streamParams.channelCount = 2;
        streamParams.sampleFormat = paInt16;
        streamParams.suggestedLatency = PaTime(double(periodSize) * nPeriodsHW
                                               / outputSampleRate_);
        streamParams.hostApiSpecificStreamInfo = (void *) 0;
        if (Pa_OpenStream(&paStream, (PaStreamParameters *) 0, &streamParams,
                          outputSampleRate_, unsigned(periodSize),
                          paNoFlag, &portAudioCallback, (void *) this)
            != paNoError)
          throw Ep128::Exception("error opening audio device");
        Pa_StartStream(paStream);
      }
      catch (...) {
        if (sf != (SNDFILE *) 0) {
          sf_close(sf);
          sf = (SNDFILE *) 0;
        }
        if (paInitialized) {
          Pa_Terminate();
          paInitialized = false;
        }
        throw;
      }
    }
    virtual ~AudioOutput()
    {
      if (sf != (SNDFILE *) 0) {
        sf_close(sf);
        sf = (SNDFILE *) 0;
      }
      if (paStream) {
        Pa_AbortStream(paStream);
        Pa_CloseStream(paStream);
        paStream = (PaStream *) 0;
      }
      if (paInitialized) {
        Pa_Terminate();
        paInitialized = false;
      }
    }
    virtual void audioOutput(int16_t left, int16_t right)
    {
      Buffer& buf = buffers[writeBufIndex];
      buf.audioData[buf.writePos++] = left;
      buf.audioData[buf.writePos++] = right;
      if (buf.writePos < buf.audioData.size())
        return;
      buf.writePos = 0;
      buf.paLock.notify();
      if (sf) {
        if (sf_write_short(sf, reinterpret_cast<short *>(&(buf.audioData[0])),
                           sf_count_t(buf.audioData.size()))
            != sf_count_t(buf.audioData.size())) {
          sf_close(sf);
          sf = (SNDFILE *) 0;
          outputFileName = "";
        }
      }
      if (!buf.epLock.wait(1000))
        return;
      if (++writeBufIndex >= buffers.size())
        writeBufIndex = 0;
    }
  };

  typedef AudioOutput<Ep128::DaveConverterLowQuality>   AudioOutput_LowQuality;
  typedef AudioOutput<Ep128::DaveConverterHighQuality>  AudioOutput_HighQuality;

}       // namespace Ep128Emu

#endif  // EP128EMU_SOUNDIO_HPP

