
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
    class ParametricEqualizer {
     private:
      int     mode; // -1: disabled, 0: peaking EQ, 1: low shelf, 2: high shelf
      double  xnm1, xnm2, ynm1, ynm2;
      double  a1da0, a2da0, b0da0, b1da0, b2da0;
     public:
      ParametricEqualizer();
      void setParameters(int mode_, float omega_, float level_, float q_);
      inline float process(float inputSignal);
    };
    float   inputSampleRate;
    float   outputSampleRate;
    DCBlockFilter dcBlock1L;
    DCBlockFilter dcBlock1R;
    DCBlockFilter dcBlock2L;
    DCBlockFilter dcBlock2R;
    ParametricEqualizer eqL;
    ParametricEqualizer eqR;
    float   ampScale;
   public:
    AudioConverter(float inputSampleRate_, float outputSampleRate_,
                   float dcBlockFreq1 = 10.0f, float dcBlockFreq2 = 10.0f,
                   float ampScale_ = 0.7071f);
    virtual ~AudioConverter();
    virtual void sendInputSignal(uint32_t audioInput) = 0;
    virtual void sendMonoInputSignal(int32_t audioInput) = 0;
    virtual void setInputSampleRate(float sampleRate_);
    virtual void setOutputSampleRate(float sampleRate_);
    void setDCBlockFilters(float frq1, float frq2);
    void setEqualizerParameters(int mode_, float freq_, float level_, float q_);
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
    AudioConverterLowQuality(float inputSampleRate_,
                             float outputSampleRate_,
                             float dcBlockFreq1 = 10.0f,
                             float dcBlockFreq2 = 10.0f,
                             float ampScale_ = 0.7071f);
    virtual ~AudioConverterLowQuality();
    virtual void sendInputSignal(uint32_t audioInput);
    virtual void sendMonoInputSignal(int32_t audioInput);
    virtual void setInputSampleRate(float sampleRate_);
    virtual void setOutputSampleRate(float sampleRate_);
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
      inline void processSample(float inL, float *outBufL,
                                int outBufSize, float bufPos);
    };
    static ResampleWindow window;
    static const int bufSize = 16;
    float   bufL[16];
    float   bufR[16];
    float   bufPos, nxtPos;
    float   resampleRatio;
    // ----------------
   public:
    AudioConverterHighQuality(float inputSampleRate_,
                              float outputSampleRate_,
                              float dcBlockFreq1 = 10.0f,
                              float dcBlockFreq2 = 10.0f,
                              float ampScale_ = 0.7071f);
    virtual ~AudioConverterHighQuality();
    virtual void sendInputSignal(uint32_t audioInput);
    virtual void sendMonoInputSignal(int32_t audioInput);
    virtual void setInputSampleRate(float sampleRate_);
    virtual void setOutputSampleRate(float sampleRate_);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_SND_CONV_HPP

