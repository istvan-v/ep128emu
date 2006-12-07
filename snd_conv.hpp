
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

#ifndef EP128EMU_SND_CONV_HPP
#define EP128EMU_SND_CONV_HPP

#include "ep128emu.hpp"

namespace Ep128Emu {

  class AudioConverter {
   protected:
    class DCBlockFilter {
     private:
      float   sampleRate;
      float   c, xnm1, ynm1;
     public:
      inline float process(float inputSignal);
      void setCutoffFrequency(float frq);
      DCBlockFilter(float sampleRate_, float cutoffFreq = 10.0f);
    };
    float   inputSampleRate;
    float   outputSampleRate;
    DCBlockFilter dcBlock1L;
    DCBlockFilter dcBlock1R;
    DCBlockFilter dcBlock2L;
    DCBlockFilter dcBlock2R;
    float   ampScale;
   public:
    AudioConverter(float inputSampleRate_, float outputSampleRate_,
                   float dcBlockFreq1 = 10.0f, float dcBlockFreq2 = 10.0f,
                   float ampScale_ = 0.7071f);
    virtual ~AudioConverter();
    virtual void sendInputSignal(uint32_t audioInput) = 0;
    void setDCBlockFilters(float frq1, float frq2);
    void setOutputVolume(float ampScale_);
   protected:
    virtual void audioOutput(int16_t left, int16_t right) = 0;
    inline void sendOutputSignal(float left, float right);
  };

  class AudioConverterLowQuality : public AudioConverter {
   private:
    float   prvInputL, prvInputR;
    float   phs, nxtPhs;
    float   downsampleRatio;
    float   outLeft, outRight;
   public:
    virtual void sendInputSignal(uint32_t audioInput);
    AudioConverterLowQuality(float inputSampleRate_,
                             float outputSampleRate_,
                             float dcBlockFreq1 = 10.0f,
                             float dcBlockFreq2 = 10.0f,
                             float ampScale_ = 0.7071f);
    virtual ~AudioConverterLowQuality();
  };

  class AudioConverterHighQuality : public AudioConverter {
   private:
    class ResampleWindow {
     private:
      static const int windowSize = 12 * 128;
      float   windowTable[12 * 128 + 1];
     public:
      ResampleWindow();
      inline void processSample(float inL, float inR,
                                float *outBufL, float *outBufR,
                                int outBufSize, float bufPos);
    };
    static ResampleWindow window;
    float   prvInputL, prvInputR;
    int     sampleCnt;
    static const int bufSize = 16;
    float   bufL[16];
    float   bufR[16];
    float   bufPos, nxtPos;
    float   resampleRatio;
    // ----------------
   public:
    virtual void sendInputSignal(uint32_t audioInput);
    AudioConverterHighQuality(float inputSampleRate_,
                              float outputSampleRate_,
                              float dcBlockFreq1 = 10.0f,
                              float dcBlockFreq2 = 10.0f,
                              float ampScale_ = 0.7071f);
    virtual ~AudioConverterHighQuality();
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_SND_CONV_HPP

