
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/ep128emu/
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
#include "debuglib.hpp"

#include <typeinfo>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#  include <unistd.h>
#  include <dirent.h>
#endif

static const char *fileOpenErrorMessages[7] = {
  "No error",
  "Error opening file",
  "Invalid file name",
  "File not found",
  "File is not a regular file",
  "Permission denied",
  "File already exists"
};

static void defaultBreakPointCallback(void *userData,
                                      int type, uint16_t addr, uint8_t value)
{
  (void) userData;
  (void) type;
  (void) addr;
  (void) value;
}

static void defaultFileNameCallback(void *userData, std::string& fileName)
{
  (void) userData;
  fileName.clear();
}

static void writeHexValueToFile(std::FILE *f, uint32_t n, int nDigits,
                                int prefixChar = -1)
{
  if (prefixChar >= 0) {
    if (std::fputc(prefixChar, f) == EOF)
      throw Ep128Emu::Exception("error writing file - is the disk full ?");
  }
  while (--nDigits >= 0) {
    char    c = char((n >> (nDigits << 2)) & 0x0FU) + '0';
    if (c > '9')
      c = c + ('A' - ('0' + char(10)));
    if (std::fputc(c, f) == EOF)
      throw Ep128Emu::Exception("error writing file - is the disk full ?");
  }
}

namespace Ep128Emu {

  template <typename T>
  class AudioConverter_ : public T {
   private:
    AudioOutput&  audioOutput_;
    int16_t       buf[32];
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
      if (bufPos >= 32) {
        bufPos = 0;
        audioOutput_.sendAudioData(&(buf[0]), 16);
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
      audioOutputEQMode(-1),
      audioOutputEQFrequency(1000.0f),
      audioOutputEQLevel(1.0f),
      audioOutputEQ_Q(0.7071f),
      tapePlaybackOn(false),
      tapeRecordOn(false),
      tapeMotorOn(false),
      tapeMotorState(0x00),
      tape((Tape *) 0),
      defaultTapeSampleRate(24000L),
      tapeSoundFileChannel(0),
      tapeEnableSoundFileFilter(false),
      tapeSoundFileFilterMinFreq(500.0f),
      tapeSoundFileFilterMaxFreq(5000.0f),
      breakPointCallback(&defaultBreakPointCallback),
      breakPointCallbackUserData((void *) 0),
      fileIOEnabled(false),
#ifndef WIN32
      fileIOWorkingDirectory("./"),
#else
      fileIOWorkingDirectory(".\\"),
#endif
      fileNameCallback(&defaultFileNameCallback),
      fileNameCallbackUserData((void *) 0)
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
    if (audioConverter == (AudioConverter *) 0) {
      if (audioOutputEnabled) {
        // open audio converter if needed
        audioOutputSampleRate = audioOutput.getSampleRate();
        if (audioConverterSampleRate > 0.0f && audioOutputSampleRate > 0.0f) {
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
          audioConverter->setEqualizerParameters(audioOutputEQMode,
                                                 audioOutputEQFrequency,
                                                 audioOutputEQLevel,
                                                 audioOutputEQ_Q);
        }
      }
    }
    else if (audioOutput.getSampleRate() != audioOutputSampleRate) {
      audioOutputSampleRate = audioOutput.getSampleRate();
      audioConverter->setOutputSampleRate(audioOutputSampleRate);
    }
    writingAudioOutput =
        (audioConverter != (AudioConverter *) 0 && audioOutputEnabled);
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

  void VirtualMachine::loadMemoryConfiguration(const std::string& fileName_)
  {
    (void) fileName_;
  }

  void VirtualMachine::setAudioOutputHighQuality(bool useHighQualityResample)
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
        audioConverter->setEqualizerParameters(audioOutputEQMode,
                                               audioOutputEQFrequency,
                                               audioOutputEQLevel,
                                               audioOutputEQ_Q);
      }
      writingAudioOutput =
          (audioConverter != (AudioConverter *) 0 && audioOutputEnabled);
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

  void VirtualMachine::setAudioOutputEqualizer(int mode_, float freq_,
                                               float level_, float q_)
  {
    mode_ = ((mode_ >= 0 && mode_ <= 2) ? mode_ : -1);
    freq_ = (freq_ > 1.0f ? (freq_ < 100000.0f ? freq_ : 100000.0f) : 1.0f);
    level_ = (level_ > 0.0001f ? (level_ < 100.0f ? level_ : 100.0f) : 0.0001f);
    q_ = (q_ > 0.001f ? (q_ < 100.0f ? q_ : 100.0f) : 0.001f);
    if (mode_ != audioOutputEQMode ||
        freq_ != audioOutputEQFrequency ||
        level_ != audioOutputEQLevel ||
        q_ != audioOutputEQ_Q) {
      audioOutputEQMode = mode_;
      audioOutputEQFrequency = freq_;
      audioOutputEQLevel = level_;
      audioOutputEQ_Q = q_;
      if (audioConverter) {
        audioConverter->setEqualizerParameters(audioOutputEQMode,
                                               audioOutputEQFrequency,
                                               audioOutputEQLevel,
                                               audioOutputEQ_Q);
      }
    }
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
        (audioConverter != (AudioConverter *) 0 && audioOutputEnabled);
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

  void VirtualMachine::setSoundClockFrequency(size_t freq_)
  {
    (void) freq_;
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

  void VirtualMachine::getVMStatus(VMStatus& vmStatus_)
  {
    vmStatus_.tapeReadOnly = getIsTapeReadOnly();
    vmStatus_.tapePosition = getTapePosition();
    vmStatus_.tapeLength = getTapeLength();
    vmStatus_.tapeSampleRate = getTapeSampleRate();
    vmStatus_.tapeSampleSize = getTapeSampleSize();
    vmStatus_.floppyDriveLEDState = getFloppyDriveLEDState();
    vmStatus_.isPlayingDemo = getIsPlayingDemo();
    vmStatus_.isRecordingDemo = getIsRecordingDemo();
  }

  void VirtualMachine::openVideoCapture(
      int frameRate_,
      bool yuvFormat_,
      void (*errorCallback_)(void *userData, const char *msg),
      void (*fileNameCallback_)(void *userData, std::string& fileName),
      void *userData_)
  {
    (void) frameRate_;
    (void) yuvFormat_;
    (void) errorCallback_;
    (void) fileNameCallback_;
    (void) userData_;
  }

  void VirtualMachine::setVideoCaptureFile(const std::string& fileName_)
  {
    (void) fileName_;
  }

  void VirtualMachine::closeVideoCapture()
  {
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

  uint32_t VirtualMachine::getFloppyDriveLEDState()
  {
    return 0U;
  }

  void VirtualMachine::setEnableFileIO(bool isEnabled)
  {
    fileIOEnabled = isEnabled;
  }

  void VirtualMachine::setWorkingDirectory(const std::string& dirName_)
  {
#ifndef WIN32
    if (dirName_.length() == 0)
      fileIOWorkingDirectory = "./";
    else {
      fileIOWorkingDirectory = dirName_;
      const std::string&  s = fileIOWorkingDirectory;
      for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == '\\')
          fileIOWorkingDirectory[i] = '/';
      }
      if (s[s.length() - 1] != '/')
        fileIOWorkingDirectory += '/';
    }
#else
    if (dirName_.length() == 0)
      fileIOWorkingDirectory = ".\\";
    else {
      fileIOWorkingDirectory = dirName_;
      const std::string&  s = fileIOWorkingDirectory;
      for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == '/')
          fileIOWorkingDirectory[i] = '\\';
      }
      if (s[s.length() - 1] != '\\')
        fileIOWorkingDirectory += '\\';
    }
#endif
  }

  void VirtualMachine::setFileNameCallback(void (*fileNameCallback_)(
                                               void *userData,
                                               std::string& fileName),
                                           void *userData_)
  {
    fileNameCallback = fileNameCallback_;
    fileNameCallbackUserData = userData_;
  }

  void VirtualMachine::setTapeFileName(const std::string& fileName)
  {
    setTapeFileName(fileName, 1);
  }

  void VirtualMachine::setDefaultTapeSampleRate(long sampleRate_)
  {
    defaultTapeSampleRate = (sampleRate_ > 10000L ?
                             (sampleRate_ < 120000L ? sampleRate_ : 120000L)
                             : 10000L);
  }

  long VirtualMachine::getTapeSampleRate() const
  {
    if (tape)
      return tape->getSampleRate();
    return 0L;
  }

  int VirtualMachine::getTapeSampleSize() const
  {
    if (tape)
      return tape->getSampleSize();
    return 0;
  }

  bool VirtualMachine::getIsTapeReadOnly() const
  {
    if (tape)
      return tape->getIsReadOnly();
    return true;
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

  double VirtualMachine::getTapeLength() const
  {
    if (tape)
      return tape->getLength();
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

  void VirtualMachine::setTapeSoundFileParameters(int requestedChannel_,
                                                  bool enableFilter_,
                                                  float filterMinFreq_,
                                                  float filterMaxFreq_)
  {
    if (requestedChannel_ == tapeSoundFileChannel &&
        enableFilter_ == tapeEnableSoundFileFilter &&
        filterMinFreq_ == tapeSoundFileFilterMinFreq &&
        filterMaxFreq_ == tapeSoundFileFilterMaxFreq)
      return;
    tapeSoundFileChannel = requestedChannel_;
    tapeEnableSoundFileFilter = enableFilter_;
    tapeSoundFileFilterMinFreq = filterMinFreq_;
    tapeSoundFileFilterMaxFreq = filterMaxFreq_;
    if (tape) {
      if (typeid(*tape) == typeid(Tape_SoundFile)) {
        Tape_SoundFile& tape_ = *(dynamic_cast<Tape_SoundFile *>(tape));
        tape_.setParameters(tapeSoundFileChannel,
                            tapeEnableSoundFileFilter,
                            tapeSoundFileFilterMinFreq,
                            tapeSoundFileFilterMaxFreq);
      }
    }
  }

  void VirtualMachine::setForceTapeMotorOn(bool isEnabled)
  {
    tapeMotorState = (tapeMotorState & 0x01) | (uint8_t(isEnabled) << 1);
    tapeMotorOn = bool(tapeMotorState);
    if (tape)
      tape->setIsMotorOn(tapeMotorOn);
  }

  void VirtualMachine::setBreakPoints(const BreakPointList& bpList)
  {
    for (size_t i = 0; i < bpList.getBreakPointCnt(); i++)
      setBreakPoint(bpList.getBreakPoint(i), true);
  }

  void VirtualMachine::setBreakPoint(const BreakPoint& bp, bool isEnabled)
  {
    (void) bp;
    (void) isEnabled;
  }

  void VirtualMachine::clearBreakPoints()
  {
  }

  void VirtualMachine::setBreakPointPriorityThreshold(int n)
  {
    (void) n;
  }

  void VirtualMachine::setSingleStepMode(int mode_)
  {
    (void) mode_;
  }

  void VirtualMachine::setSingleStepModeNextAddress(int32_t addr)
  {
    (void) addr;
  }

  void VirtualMachine::setBreakPointCallback(void (*breakPointCallback_)(
                                                 void *userData, int type,
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

  uint8_t VirtualMachine::readMemory(uint32_t addr, bool isCPUAddress) const
  {
    (void) addr;
    (void) isCPUAddress;
    return 0xFF;
  }

  void VirtualMachine::writeMemory(uint32_t addr, uint8_t value,
                                   bool isCPUAddress)
  {
    (void) addr;
    (void) value;
    (void) isCPUAddress;
  }

  void VirtualMachine::writeROM(uint32_t addr, uint8_t value)
  {
    (void) addr;
    (void) value;
  }

  uint8_t VirtualMachine::readIOPort(uint16_t addr) const
  {
    (void) addr;
    return 0xFF;
  }

  void VirtualMachine::writeIOPort(uint16_t addr, uint8_t value)
  {
    (void) addr;
    (void) value;
  }

  uint16_t VirtualMachine::getProgramCounter() const
  {
    return uint16_t(0x0000);
  }

  void VirtualMachine::setProgramCounter(uint16_t addr)
  {
    (void) addr;
  }

  uint16_t VirtualMachine::getStackPointer() const
  {
    return uint16_t(0x0000);
  }

  void VirtualMachine::listCPURegisters(std::string& buf) const
  {
    buf = "";
  }

  void VirtualMachine::listIORegisters(std::string& buf) const
  {
    buf = "";
  }

  uint32_t VirtualMachine::disassembleInstruction(std::string& buf,
                                                  uint32_t addr,
                                                  bool isCPUAddress,
                                                  int32_t offs) const
  {
    uint32_t  addrMask = (isCPUAddress ? 0x0000FFFFU : 0x003FFFFFU);
    addr &= addrMask;
    uint32_t  baseAddr = (addr + uint32_t(offs)) & addrMask;
    uint8_t   opNum = readMemory(addr, isCPUAddress);
    char      tmpBuf[40];
    if (isCPUAddress)
      std::sprintf(&(tmpBuf[0]), "  %04X  %02X            ???",
                   (unsigned int) baseAddr, (unsigned int) opNum);
    else
      std::sprintf(&(tmpBuf[0]), "%06X  %02X            ???",
                   (unsigned int) baseAddr, (unsigned int) opNum);
    buf = &(tmpBuf[0]);
    addr = (addr + 1U) & addrMask;
    return addr;
  }

  Ep128::Z80_REGISTERS& VirtualMachine::getZ80Registers()
  {
    return *((Ep128::Z80_REGISTERS *) 0);
  }

  const Ep128::Z80_REGISTERS& VirtualMachine::getZ80Registers() const
  {
    return *((Ep128::Z80_REGISTERS *) 0);
  }

  void VirtualMachine::getVideoPosition(int& xPos, int& yPos) const
  {
    xPos = 0;
    yPos = 0;
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

  void VirtualMachine::setTapeFileName(const std::string& fileName,
                                       int bitsPerSample)
  {
    if (tape) {
      delete tape;
      tape = (Tape *) 0;
    }
    if (fileName.length() == 0)
      return;
    tape = openTapeFile(fileName.c_str(), 0,
                        defaultTapeSampleRate, bitsPerSample);
    if (typeid(*tape) == typeid(Tape_SoundFile)) {
      Tape_SoundFile& tape_ = *(dynamic_cast<Tape_SoundFile *>(tape));
      tape_.setParameters(tapeSoundFileChannel,
                          tapeEnableSoundFileFilter,
                          tapeSoundFileFilterMinFreq,
                          tapeSoundFileFilterMaxFreq);
    }
    if (tapeRecordOn)
      tape->record();
    else if (tapePlaybackOn)
      tape->play();
    tape->setIsMotorOn(tapeMotorOn);
  }

  void VirtualMachine::setTapeMotorState_(bool newState)
  {
    tapeMotorState = (tapeMotorState & 0x02) | uint8_t(newState);
    tapeMotorOn = bool(tapeMotorState);
    if (tape)
      tape->setIsMotorOn(tapeMotorOn);
  }

  void VirtualMachine::setAudioConverterSampleRate(float sampleRate_)
  {
    if (sampleRate_ != audioConverterSampleRate) {
      audioConverterSampleRate = sampleRate_;
      if (audioConverter) {
        audioConverter->setInputSampleRate(audioConverterSampleRate);
        return;
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
        audioConverter->setEqualizerParameters(audioOutputEQMode,
                                               audioOutputEQFrequency,
                                               audioOutputEQLevel,
                                               audioOutputEQ_Q);
      }
      writingAudioOutput =
          (audioConverter != (AudioConverter *) 0 && audioOutputEnabled);
    }
  }

  int VirtualMachine::openFileInWorkingDirectory(std::FILE*& f,
                                                 std::string& fileName_,
                                                 const char *mode,
                                                 bool createOnly_)
  {
    f = (std::FILE *) 0;
    try {
      std::string fullName;
      if (fileName_.length() > 0) {
        // convert file name to lower case, replace invalid characters with '_'
        std::string baseName(fileName_);
        stringToLowerCase(baseName);
        for (size_t i = 0; i < baseName.length(); i++) {
          const std::string&  s = baseName;
          if (!((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= '0' && s[i] <= '9') ||
                s[i] == '.' || s[i] == '+' || s[i] == '-' || s[i] == '_'))
            baseName[i] = '_';
        }
        fullName = fileIOWorkingDirectory + baseName;
      }
      else {
        if (fileNameCallback)
          fileNameCallback(fileNameCallbackUserData, fullName);
        if (fullName.length() == 0)
          return -2;                    // error: invalid file name
#ifdef WIN32
        fileName_ = fullName;
#endif
      }
      // attempt to stat() file
#ifndef WIN32
      struct stat   st;
      std::memset(&st, 0, sizeof(struct stat));
      int   err = stat(fullName.c_str(), &st);
      if (fileName_.empty()) {
        fileName_ = fullName;
      }
      else if (err != 0) {
        // not found, try case insensitive file search
        std::string tmpName(fullName);
        tmpName[0] = tmpName.c_str()[0];    // unshare string
        DIR   *dir_;
        dir_ = opendir(fileIOWorkingDirectory.c_str());
        if (dir_) {
          do {
            struct dirent *ent_ = readdir(dir_);
            if (!ent_)
              break;
            bool        foundMatch = true;
            size_t      offs = fileIOWorkingDirectory.length();
            const char  *s1 = fullName.c_str() + offs;
            const char  *s2 = &(ent_->d_name[0]);
            size_t      i = 0;
            while (!(s1[i] == '\0' && s2[i] == '\0')) {
              if (s1[i] != s2[i]) {
                if (!(s2[i] >= 'A' && s2[i] <= 'Z' &&
                      s1[i] == (s2[i] + ('a' - 'A')))) {
                  foundMatch = false;
                  break;
                }
              }
              tmpName[offs + i] = s2[i];
              i++;
            }
            if (foundMatch) {
              std::memset(&st, 0, sizeof(struct stat));
              err = stat(tmpName.c_str(), &st);
            }
          } while (err != 0);
          closedir(dir_);
        }
        if (err == 0)
          fullName = tmpName;
      }
#else
      struct _stat  st;
      std::memset(&st, 0, sizeof(struct _stat));
      int   err = _stat(fullName.c_str(), &st);
#endif
      if (err != 0) {
        if (mode == (char *) 0 || mode[0] != 'w')
          return -3;                    // error: cannot find file
      }
      else {
#ifndef WIN32
        if (!(S_ISREG(st.st_mode)))
          return -4;                    // error: not a regular file
#else
        if (!(st.st_mode & _S_IFREG))
          return -4;                    // error: not a regular file
#endif
        if (createOnly_)
          return -6;                    // error: the file already exists
      }
      // FIXME: the file may possibly be created, changed, or removed between
      // calling stat() and fopen()
      f = std::fopen(fullName.c_str(), mode);
      if (!f)
        return -5;                      // error: cannot open file
    }
    catch (...) {
      if (f) {
        std::fclose(f);
        f = (std::FILE *) 0;
      }
      return -1;
    }
    return 0;
  }

  const char * VirtualMachine::getFileOpenErrorMessage(int n)
  {
    if (n >= -6 && n <= 0)
      return fileOpenErrorMessages[-n];
    return fileOpenErrorMessages[1];
  }

  size_t VirtualMachine::loadMemory(const char *fileName, bool verifyMode,
                                    bool asciiMode, bool cpuAddressMode,
                                    uint32_t startAddr, uint32_t endAddr)
  {
    std::FILE *f = (std::FILE *) 0;
    size_t    byteCnt = 0;
    try {
      if (!fileName)
        fileName = "";
      std::string fileName_(fileName);
      int       err = openFileInWorkingDirectory(f, fileName_, "rb");
      if (err)
        throw Exception(getFileOpenErrorMessage(err));
      uint32_t  addrMask = (cpuAddressMode ? 0x0000FFFFU : 0x003FFFFFU);
      uint32_t  addr = startAddr & addrMask;
      uint32_t  nBytes = 0U;
      // if the end address is 0xFFFFFFFF, then read all data
      if (endAddr != 0xFFFFFFFFU)
        nBytes = ((endAddr + 1U) - addr) & addrMask;
      if (!nBytes)
        nBytes = addrMask + 1U;
      if (!asciiMode) {
        while (nBytes) {
          int     c = std::fgetc(f);
          if (c == EOF)
            break;
          if (!verifyMode) {
            writeMemory(addr, uint8_t(c & 0xFF), cpuAddressMode);
            byteCnt++;
          }
          else if (readMemory(addr, cpuAddressMode) != uint8_t(c & 0xFF))
            byteCnt++;
          addr = (addr + 1U) & addrMask;
          nBytes--;
        }
      }
      else {
        std::string tmpBuf;
        std::string s;
        bool        eofFlag = false;
        while (nBytes > 0 && !eofFlag) {
          {
            int     c = std::fgetc(f);
            if (c == EOF) {
              eofFlag = true;
              c = '\n';
            }
            if (c != '\n' && c != '\r') {
              tmpBuf += char(c & 0xFF);
              continue;
            }
          }
          bool    firstArg = true;
          bool    haveAddr = false;
          bool    skippingSpace = true;
          for (size_t i = 0; i < tmpBuf.length(); i++) {
            char    c = tmpBuf[i];
            if (skippingSpace) {
              if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                continue;
              s = "";
              skippingSpace = false;
            }
            if (!(c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
              s += c;
              if (!((i + 1) >= tmpBuf.length() || c == '>' || c == ':'))
                continue;
            }
            skippingSpace = true;
            if (firstArg && s == ">") {
              firstArg = false;
              continue;
            }
            firstArg = false;
            if (haveAddr && s == ":")
              continue;
            uint32_t  n = parseHexNumberEx(s.c_str());
            if (!haveAddr) {
              haveAddr = true;
              continue;
            }
            if (n > 0xFFU)
              throw Exception("invalid byte value");
            if (!verifyMode) {
              writeMemory(addr, uint8_t(n), cpuAddressMode);
              byteCnt++;
            }
            else if (readMemory(addr, cpuAddressMode) != uint8_t(n))
              byteCnt++;
            addr = (addr + 1U) & addrMask;
            if (--nBytes == 0)
              break;
          }
          tmpBuf = "";
        }
      }
      std::fclose(f);
      f = (std::FILE *) 0;
    }
    catch (...) {
      if (f)
        std::fclose(f);
      throw;
    }
    // return the number of bytes read in load mode,
    // and the number of bytes that differ in verify mode
    return byteCnt;
  }

  void VirtualMachine::saveMemory(const char *fileName,
                                  bool asciiMode, bool cpuAddressMode,
                                  uint32_t startAddr, uint32_t endAddr)
  {
    std::FILE *f = (std::FILE *) 0;
    try {
      if (!fileName)
        fileName = "";
      std::string fileName_(fileName);
      int       err =
          openFileInWorkingDirectory(f, fileName_, (asciiMode ? "w" : "wb"));
      if (err)
        throw Exception(getFileOpenErrorMessage(err));
      uint32_t  addrMask = (cpuAddressMode ? 0x0000FFFFU : 0x003FFFFFU);
      uint32_t  addr = startAddr & addrMask;
      endAddr = (endAddr + 1U) & addrMask;
      bool      errorFlag = false;
      do {
        uint8_t c = readMemory(addr, cpuAddressMode);
        if (!asciiMode) {
          if (std::fputc(c, f) == EOF) {
            errorFlag = true;
            break;
          }
          addr = (addr + 1U) & addrMask;
        }
        else {
          if (((addr - startAddr) & 7U) == 0U) {
            writeHexValueToFile(f, addr, (cpuAddressMode ? 4 : 6), '>');
            if (std::fputc(' ', f) == EOF) {
              errorFlag = true;
              break;
            }
          }
          writeHexValueToFile(f, uint32_t(c), 2, ' ');
          addr = (addr + 1U) & addrMask;
          if (((addr - startAddr) & 7U) == 0U || addr == endAddr) {
            if (std::fputc('\n', f) == EOF) {
              errorFlag = true;
              break;
            }
          }
        }
      } while (addr != endAddr);
      if (errorFlag || std::fflush(f) != 0)
        throw Exception("error writing file - is the disk full ?");
      std::fclose(f);
      f = (std::FILE *) 0;
    }
    catch (...) {
      if (f)
        std::fclose(f);
      throw;
    }
  }

}       // namespace Ep128Emu

