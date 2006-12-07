
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
#include "snd_conv.hpp"
#include <cmath>

namespace Ep128Emu {

  inline float AudioConverter::DCBlockFilter::process(float inputSignal)
  {
    // y[n] = x[n] - x[n - 1] + (c * y[n - 1])
    float   outputSignal = (inputSignal - this->xnm1) + (this->c * this->ynm1);
    // avoid denormals
#if defined(__i386__) || defined(__x86_64__)
    unsigned char e = ((unsigned char *) &outputSignal)[3] & 0x7F;
    if (e < 0x08 || e >= 0x78)
      outputSignal = 0.0f;
#else
    outputSignal = (outputSignal < -1.0e-20f || outputSignal > 1.0e-20f ?
                    outputSignal : 0.0f);
#endif
    this->xnm1 = inputSignal;
    this->ynm1 = outputSignal;
    return outputSignal;
  }

  void AudioConverter::DCBlockFilter::setCutoffFrequency(float frq)
  {
    float   tpfdsr = (2.0f * 3.14159265f * frq / sampleRate);
    c = 1.0f - (tpfdsr > 0.0003f ?
                (tpfdsr < 0.125f ? tpfdsr : 0.125f) : 0.0003f);
  }

  AudioConverter::DCBlockFilter::DCBlockFilter(float sampleRate_,
                                               float cutoffFreq)
  {
    sampleRate = sampleRate_;
    c = 1.0f;
    xnm1 = 0.0f;
    ynm1 = 0.0f;
    setCutoffFrequency(cutoffFreq);
  }

  inline void AudioConverter::sendOutputSignal(float left, float right)
  {
    // scale, clip, and convert to 16 bit integer format
    float   outL = left * ampScale;
    float   outR = right * ampScale;
    if (outL < 0.0f)
      outL = (outL > -32767.0f ? outL - 0.5f : -32767.5f);
    else
      outL = (outL < 32767.0f ? outL + 0.5f : 32767.5f);
    if (outR < 0.0f)
      outR = (outR > -32767.0f ? outR - 0.5f : -32767.5f);
    else
      outR = (outR < 32767.0f ? outR + 0.5f : 32767.5f);
#if defined(__linux) || defined(__linux__)
    int16_t outL_i = int16_t(outL);
    int16_t outR_i = int16_t(outR);
    // hack to work around clicks in ALSA sound output
    outL_i = (outL_i != 0 ? outL_i : 1);
    outR_i = (outR_i != 0 ? outR_i : 1);
    this->audioOutput(int16_t(outL_i), int16_t(outR_i));
#else
    this->audioOutput(int16_t(outL), int16_t(outR));
#endif
  }

  AudioConverter::AudioConverter(float inputSampleRate_,
                                 float outputSampleRate_,
                                 float dcBlockFreq1, float dcBlockFreq2,
                                 float ampScale_)
    : inputSampleRate(inputSampleRate_),
      outputSampleRate(outputSampleRate_),
      dcBlock1L(outputSampleRate_, dcBlockFreq1),
      dcBlock1R(outputSampleRate_, dcBlockFreq1),
      dcBlock2L(outputSampleRate_, dcBlockFreq2),
      dcBlock2R(outputSampleRate_, dcBlockFreq2)
  {
    setOutputVolume(ampScale_);
  }

  AudioConverter::~AudioConverter()
  {
  }

  void AudioConverter::setDCBlockFilters(float frq1, float frq2)
  {
    dcBlock1L.setCutoffFrequency(frq1);
    dcBlock1R.setCutoffFrequency(frq1);
    dcBlock2L.setCutoffFrequency(frq2);
    dcBlock2R.setCutoffFrequency(frq2);
  }

  void AudioConverter::setOutputVolume(float ampScale_)
  {
    if (ampScale_ > 0.01f && ampScale_ < 1.0f)
      ampScale = 1.17f * ampScale_;
    else if (ampScale_ > 0.99f)
      ampScale = 1.17f;
    else
      ampScale = 0.017f;
  }

  void AudioConverterLowQuality::sendInputSignal(uint32_t audioInput)
  {
    float   left = float(int(audioInput & 0xFFFF));
    float   right = float(int(audioInput >> 16));
    phs += 1.0f;
    if (phs < nxtPhs) {
      outLeft += (prvInputL + left);
      outRight += (prvInputR + right);
    }
    else {
      float   phsFrac = nxtPhs - (phs - 1.0f);
      float   left2 = prvInputL + ((left - prvInputL) * phsFrac);
      float   right2 = prvInputR + ((right - prvInputR) * phsFrac);
      outLeft += ((prvInputL + left2) * phsFrac);
      outRight += ((prvInputR + right2) * phsFrac);
      outLeft /= (downsampleRatio * 2.0f);
      outRight /= (downsampleRatio * 2.0f);
      sendOutputSignal(dcBlock2L.process(dcBlock1L.process(outLeft)),
                       dcBlock2R.process(dcBlock1R.process(outRight)));
      outLeft = (left2 + left) * (1.0f - phsFrac);
      outRight = (right2 + right) * (1.0f - phsFrac);
      nxtPhs = (nxtPhs + downsampleRatio) - phs;
      phs = 0.0f;
    }
    prvInputL = left;
    prvInputR = right;
  }

  AudioConverterLowQuality::AudioConverterLowQuality(float inputSampleRate_,
                                                     float outputSampleRate_,
                                                     float dcBlockFreq1,
                                                     float dcBlockFreq2,
                                                     float ampScale_)
    : AudioConverter(inputSampleRate_, outputSampleRate_,
                     dcBlockFreq1, dcBlockFreq2, ampScale_)
  {
    prvInputL = 0.0f;
    prvInputR = 0.0f;
    phs = 0.0f;
    downsampleRatio = inputSampleRate_ / outputSampleRate_;
    nxtPhs = downsampleRatio;
    outLeft = 0.0f;
    outRight = 0.0f;
  }

  AudioConverterLowQuality::~AudioConverterLowQuality()
  {
  }

  inline void AudioConverterHighQuality::ResampleWindow::processSample(
      float inL, float inR, float *outBufL, float *outBufR,
      int outBufSize, float bufPos)
  {
    int      writePos = int(bufPos);
    float    posFrac = bufPos - writePos;
    float    winPos = (1.0f - posFrac) * float(windowSize / 12);
    int      winPosInt = int(winPos);
    float    winPosFrac = winPos - winPosInt;
    writePos -= 5;
    while (writePos < 0)
      writePos += outBufSize;
    do {
      float   w = windowTable[winPosInt]
                  + ((windowTable[winPosInt + 1] - windowTable[winPosInt])
                     * winPosFrac);
      outBufL[writePos] += inL * w;
      outBufR[writePos] += inR * w;
      if (++writePos >= outBufSize)
        writePos = 0;
      winPosInt += (windowSize / 12);
    } while (winPosInt < windowSize);
  }

  AudioConverterHighQuality::ResampleWindow::ResampleWindow()
  {
    double  pi = std::atan(1.0) * 4.0;
    double  phs = -(pi * 6.0);
    double  phsInc = 12.0 * pi / windowSize;
    for (int i = 0; i <= windowSize; i++) {
      if (i == (windowSize / 2))
        windowTable[i] = 1.0f;
      else
        windowTable[i] = float((std::cos(phs / 6.0) * 0.5 + 0.5)  // von Hann
                               * (std::sin(phs) / phs));
      phs += phsInc;
    }
  }

  AudioConverterHighQuality::ResampleWindow AudioConverterHighQuality::window;

  void AudioConverterHighQuality::sendInputSignal(uint32_t audioInput)
  {
    float   left = float(int(audioInput & 0xFFFF));
    float   right = float(int(audioInput >> 16));
    if (!sampleCnt) {
      prvInputL = left;
      prvInputR = right;
      sampleCnt++;
      return;
    }
    sampleCnt = 0;
    left += prvInputL;
    right += prvInputR;
    window.processSample(left, right, bufL, bufR, bufSize, bufPos);
    bufPos += resampleRatio;
    if (bufPos >= nxtPos) {
      if (bufPos >= float(bufSize))
        bufPos -= float(bufSize);
      nxtPos = float(int(bufPos) + 1);
      int     readPos = int(bufPos) - 6;
      while (readPos < 0)
        readPos += bufSize;
      left = bufL[readPos] * (0.5f * resampleRatio);
      bufL[readPos] = 0.0f;
      right = bufR[readPos] * (0.5f * resampleRatio);
      bufR[readPos] = 0.0f;
      sendOutputSignal(dcBlock2L.process(dcBlock1L.process(left)),
                       dcBlock2R.process(dcBlock1R.process(right)));
    }
  }

  AudioConverterHighQuality::AudioConverterHighQuality(float inputSampleRate_,
                                                       float outputSampleRate_,
                                                       float dcBlockFreq1,
                                                       float dcBlockFreq2,
                                                       float ampScale_)
    : AudioConverter(inputSampleRate_, outputSampleRate_,
                     dcBlockFreq1, dcBlockFreq2, ampScale_)
  {
    prvInputL = 0.0f;
    prvInputR = 0.0f;
    sampleCnt = 0;
    for (int i = 0; i < bufSize; i++) {
      bufL[i] = 0.0f;
      bufR[i] = 0.0f;
    }
    bufPos = 0.0f;
    nxtPos = 1.0f;
    resampleRatio = outputSampleRate_ / (inputSampleRate_ * 0.5f);
  }

  AudioConverterHighQuality::~AudioConverterHighQuality()
  {
  }

}       // namespace Ep128Emu

