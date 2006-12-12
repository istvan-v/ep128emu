
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
#include "fileio.hpp"
#include "display.hpp"
#include "snd_conv.hpp"
#include "soundio.hpp"
#include "tape.hpp"
#include "vm.hpp"

static void defaultBreakPointCallback(void *userData,
                                      bool isIO, bool isWrite,
                                      uint16_t addr, uint8_t value)
{
  (void) userData;
  (void) isIO;
  (void) isWrite;
  (void) addr;
  (void) value;
}

namespace Ep128Emu {

  template <typename T>
  class AudioConverter_ : public T {
   private:
    AudioOutput&  audioOutput_;
    int16_t       buf[16];
    size_t        bufPos;
   public:
    AudioConverter_(AudioOutput& audioOutput__,
                    float inputSampleRate_,
                    float outputSampleRate_,
                    float dcBlockFreq1 = 10.0f,
                    float dcBlockFreq2 = 10.0f,
                    float ampScale_ = 0.7071f)
      : T(inputSampleRate_, outputSampleRate_,
          dcBlockFreq1, dcBlockFreq2, ampScale_),
        audioOutput_(audioOutput__),
        bufPos(0)
    {
    }
    virtual ~AudioConverter_()
    {
    }
    virtual void audioOutput(int16_t left, int16_t right)
    {
      buf[bufPos++] = left;
      buf[bufPos++] = right;
      if (bufPos >= 16) {
        bufPos = 0;
        audioOutput_.sendAudioData(&(buf[0]), 8);
      }
    }
  };

  VirtualMachine::VirtualMachine(VideoDisplay& display_,
                                 AudioOutput& audioOutput_)
    : display(display_),
      audioOutput(audioOutput_),
      audioConverter((AudioConverter *) 0),
      writingAudioOutput(false),
      audioOutputEnabled(true),
      audioOutputHighQuality(false),
      displayEnabled(true),
      audioConverterSampleRate(0.0f),
      audioOutputSampleRate(0.0f),
      audioOutputVolume(0.7071f),
      audioOutputFilter1Freq(10.0f),
      audioOutputFilter2Freq(10.0f),
      tapePlaybackOn(false),
      tapeRecordOn(false),
      tapeMotorOn(false),
      fastTapeModeEnabled(false),
      tape((Tape *) 0),
      tapeFileName(""),
      breakPointCallback(&defaultBreakPointCallback),
      breakPointCallbackUserData((void *) 0)
  {
  }

  VirtualMachine::~VirtualMachine()
  {
    if (tape) {
      delete tape;
      tape = (Tape *) 0;
    }
    if (audioConverter) {
      delete audioConverter;
      audioConverter = (AudioConverter *) 0;
    }
  }

  void VirtualMachine::run(size_t microseconds)
  {
    (void) microseconds;
    if ((audioConverter == (AudioConverter *) 0 && audioOutputEnabled) ||
        (audioConverter != (AudioConverter *) 0 &&
         audioOutput.getSampleRate() != audioOutputSampleRate)) {
      // open or re-open audio converter if needed
      if (audioConverter) {
        delete audioConverter;
        audioConverter = (AudioConverter *) 0;
      }
      audioOutputSampleRate = audioOutput.getSampleRate();
      if (audioOutputEnabled &&
          audioConverterSampleRate > 0.0f && audioOutputSampleRate > 0.0f) {
        if (audioOutputHighQuality)
          audioConverter = new AudioConverter_<AudioConverterHighQuality>(
              audioOutput,
              audioConverterSampleRate, audioOutputSampleRate,
              audioOutputFilter1Freq, audioOutputFilter2Freq,
              audioOutputVolume);
        else
          audioConverter = new AudioConverter_<AudioConverterLowQuality>(
              audioOutput,
              audioConverterSampleRate, audioOutputSampleRate,
              audioOutputFilter1Freq, audioOutputFilter2Freq,
              audioOutputVolume);
      }
    }
    writingAudioOutput =
        (audioConverter != (AudioConverter *) 0 && audioOutputEnabled &&
         !(tape != (Tape *) 0 && fastTapeModeEnabled &&
           tapeMotorOn && tapePlaybackOn));
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void VirtualMachine::reset(bool isColdReset)
  {
    (void) isColdReset;
  }

  void VirtualMachine::resetMemoryConfiguration(size_t memSize)
  {
    (void) memSize;
  }

  void VirtualMachine::loadROMSegment(uint8_t n,
                                      const char *fileName, size_t offs)
  {
    (void) n;
    (void) fileName;
    (void) offs;
  }

  void VirtualMachine::setAudioOutputQuality(bool useHighQualityResample)
  {
    if (useHighQualityResample != audioOutputHighQuality) {
      audioOutputHighQuality = useHighQualityResample;
      if (audioConverter) {
        delete audioConverter;
        audioConverter = (AudioConverter *) 0;
      }
      audioOutputSampleRate = audioOutput.getSampleRate();
      if (audioOutputEnabled &&
          audioConverterSampleRate > 0.0f && audioOutputSampleRate > 0.0f) {
        if (audioOutputHighQuality)
          audioConverter = new AudioConverter_<AudioConverterHighQuality>(
              audioOutput,
              audioConverterSampleRate, audioOutputSampleRate,
              audioOutputFilter1Freq, audioOutputFilter2Freq,
              audioOutputVolume);
        else
          audioConverter = new AudioConverter_<AudioConverterLowQuality>(
              audioOutput,
              audioConverterSampleRate, audioOutputSampleRate,
              audioOutputFilter1Freq, audioOutputFilter2Freq,
              audioOutputVolume);
      }
      writingAudioOutput =
          (audioConverter != (AudioConverter *) 0 && audioOutputEnabled &&
           !(tape != (Tape *) 0 && fastTapeModeEnabled &&
             tapeMotorOn && tapePlaybackOn));
    }
  }

  void VirtualMachine::setAudioOutputFilters(float dcBlockFreq1_,
                                             float dcBlockFreq2_)
  {
    audioOutputFilter1Freq =
        (dcBlockFreq1_ > 1.0f ?
         (dcBlockFreq1_ < 1000.0f ? dcBlockFreq1_ : 1000.0f) : 1.0f);
    audioOutputFilter2Freq =
        (dcBlockFreq2_ > 1.0f ?
         (dcBlockFreq2_ < 1000.0f ? dcBlockFreq2_ : 1000.0f) : 1.0f);
    if (audioConverter)
      audioConverter->setDCBlockFilters(audioOutputFilter1Freq,
                                        audioOutputFilter2Freq);
  }

  void VirtualMachine::setAudioOutputVolume(float ampScale_)
  {
    audioOutputVolume =
        (ampScale_ > 0.01f ? (ampScale_ < 1.0f ? ampScale_ : 1.0f) : 0.01f);
    if (audioConverter)
      audioConverter->setOutputVolume(audioOutputVolume);
  }

  void VirtualMachine::setEnableAudioOutput(bool isEnabled)
  {
    audioOutputEnabled = isEnabled;
    writingAudioOutput =
        (audioConverter != (AudioConverter *) 0 && audioOutputEnabled &&
         !(tape != (Tape *) 0 && fastTapeModeEnabled &&
           tapeMotorOn && tapePlaybackOn));
  }

  void VirtualMachine::setEnableDisplay(bool isEnabled)
  {
    displayEnabled = isEnabled;
  }

  void VirtualMachine::setCPUFrequency(size_t freq_)
  {
    (void) freq_;
  }

  void VirtualMachine::setVideoFrequency(size_t freq_)
  {
    (void) freq_;
  }

  void VirtualMachine::setVideoMemoryLatency(size_t t_)
  {
    (void) t_;
  }

  void VirtualMachine::setEnableMemoryTimingEmulation(bool isEnabled)
  {
    (void) isEnabled;
  }

  void VirtualMachine::setKeyboardState(int keyCode, bool isPressed)
  {
    (void) keyCode;
    (void) isPressed;
  }

  void VirtualMachine::setDiskImageFile(int n, const std::string& fileName_,
                                        int nTracks_, int nSides_,
                                        int nSectorsPerTrack_)
  {
    (void) n;
    (void) fileName_;
    (void) nTracks_;
    (void) nSides_;
    (void) nSectorsPerTrack_;
  }

  void VirtualMachine::setTapeFileName(const char *fileName)
  {
    std::string fname("");
    if (fileName)
      fname = fileName;
    if (tape) {
      if (fname == tapeFileName) {
        tape->seek(0.0);
        return;
      }
      delete tape;
      tape = (Tape *) 0;
    }
    if (fname.length() == 0)
      return;
    tape = new Tape(fname.c_str());
    if (tapeRecordOn)
      tape->record();
    else if (tapePlaybackOn)
      tape->play();
    tape->setIsMotorOn(tapeMotorOn);
  }

  void VirtualMachine::tapePlay()
  {
    tapeRecordOn = false;
    if (tape) {
      tapePlaybackOn = true;
      tape->play();
    }
    else
      tapePlaybackOn = false;
  }

  void VirtualMachine::tapeRecord()
  {
    if (tape) {
      tapePlaybackOn = true;
      tapeRecordOn = true;
      tape->record();
    }
    else {
      tapePlaybackOn = false;
      tapeRecordOn = false;
    }
  }

  void VirtualMachine::tapeStop()
  {
    tapePlaybackOn = false;
    tapeRecordOn = false;
    if (tape)
      tape->stop();
  }

  void VirtualMachine::tapeSeek(double t)
  {
    if (tape)
      tape->seek(t);
  }

  double VirtualMachine::getTapePosition() const
  {
    if (tape)
      return tape->getPosition();
    return -1.0;
  }

  void VirtualMachine::tapeSeekToCuePoint(bool isForward, double t)
  {
    if (tape)
      tape->seekToCuePoint(isForward, t);
  }

  void VirtualMachine::tapeAddCuePoint()
  {
    if (tape)
      tape->addCuePoint();
  }

  void VirtualMachine::tapeDeleteNearestCuePoint()
  {
    if (tape)
      tape->deleteNearestCuePoint();
  }

  void VirtualMachine::tapeDeleteAllCuePoints()
  {
    if (tape)
      tape->deleteAllCuePoints();
  }

  void VirtualMachine::setEnableFastTapeMode(bool isEnabled)
  {
    fastTapeModeEnabled = isEnabled;
  }

  void VirtualMachine::setBreakPoints(const std::string& bpList)
  {
    (void) bpList;
  }

  std::string VirtualMachine::getBreakPoints()
  {
    return std::string("");
  }

  void VirtualMachine::clearBreakPoints()
  {
  }

  void VirtualMachine::setBreakPointPriorityThreshold(int n)
  {
    (void) n;
  }

  void VirtualMachine::setBreakPointCallback(void (*breakPointCallback_)(
                                                 void *userData,
                                                 bool isIO, bool isWrite,
                                                 uint16_t addr, uint8_t value),
                                             void *userData_)
  {
    if (breakPointCallback_)
      breakPointCallback = breakPointCallback_;
    else
      breakPointCallback = &defaultBreakPointCallback;
    breakPointCallbackUserData = userData_;
  }

  uint8_t VirtualMachine::getMemoryPage(int n) const
  {
    (void) n;
    return 0x00;
  }

  uint8_t VirtualMachine::readMemory(uint32_t addr) const
  {
    (void) addr;
    return 0xFF;
  }

  void VirtualMachine::saveState(File& f)
  {
    (void) f;
  }

  void VirtualMachine::saveMachineConfiguration(File& f)
  {
    (void) f;
  }

  void VirtualMachine::registerChunkTypes(File& f)
  {
    (void) f;
  }

  void VirtualMachine::recordDemo(File& f)
  {
    (void) f;
  }

  void VirtualMachine::stopDemo()
  {
  }

  bool VirtualMachine::getIsRecordingDemo()
  {
    return false;
  }

  bool VirtualMachine::getIsPlayingDemo() const
  {
    return false;
  }

  void VirtualMachine::loadState(File::Buffer& buf)
  {
    (void) buf;
  }

  void VirtualMachine::loadMachineConfiguration(File::Buffer& buf)
  {
    (void) buf;
  }

  void VirtualMachine::loadDemo(File::Buffer& buf)
  {
    (void) buf;
  }

  void VirtualMachine::setTapeMotorState_(bool newState)
  {
    tapeMotorOn = newState;
    writingAudioOutput =
        (audioConverter != (AudioConverter *) 0 && audioOutputEnabled &&
         !(tape != (Tape *) 0 && fastTapeModeEnabled &&
           tapeMotorOn && tapePlaybackOn));
    if (tape)
      tape->setIsMotorOn(newState);
  }

  void VirtualMachine::setAudioConverterSampleRate(float sampleRate_)
  {
    if (sampleRate_ != audioConverterSampleRate) {
      if (audioConverter) {
        delete audioConverter;
        audioConverter = (AudioConverter *) 0;
      }
      audioConverterSampleRate = sampleRate_;
      audioOutputSampleRate = audioOutput.getSampleRate();
      if (audioOutputEnabled &&
          audioConverterSampleRate > 0.0f && audioOutputSampleRate > 0.0f) {
        if (audioOutputHighQuality)
          audioConverter = new AudioConverter_<AudioConverterHighQuality>(
              audioOutput,
              audioConverterSampleRate, audioOutputSampleRate,
              audioOutputFilter1Freq, audioOutputFilter2Freq,
              audioOutputVolume);
        else
          audioConverter = new AudioConverter_<AudioConverterLowQuality>(
              audioOutput,
              audioConverterSampleRate, audioOutputSampleRate,
              audioOutputFilter1Freq, audioOutputFilter2Freq,
              audioOutputVolume);
      }
      writingAudioOutput =
          (audioConverter != (AudioConverter *) 0 && audioOutputEnabled &&
           !(tape != (Tape *) 0 && fastTapeModeEnabled &&
             tapeMotorOn && tapePlaybackOn));
    }
  }

}       // namespace Ep128Emu

