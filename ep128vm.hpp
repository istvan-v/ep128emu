
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

#ifndef EP128EMU_EP128VM_HPP
#define EP128EMU_EP128VM_HPP

#include "ep128.hpp"
#include "z80/z80.hpp"
#include "memory.hpp"
#include "ioports.hpp"
#include "dave.hpp"
#include "nick.hpp"
#include "tape.hpp"
#include "gldisp.hpp"

namespace Ep128 {

  class Ep128VM {
   private:
    class Z80_ : public Z80 {
     private:
      Ep128VM&  vm;
     public:
      Z80_(Ep128VM& vm_);
      virtual ~Z80_();
     protected:
      virtual uint8_t readMemory(uint16_t addr);
      virtual uint16_t readMemoryWord(uint16_t addr);
      virtual uint8_t readOpcodeFirstByte();
      virtual uint8_t readOpcodeByte(int offset);
      virtual uint16_t readOpcodeWord(int offset);
      virtual void writeMemory(uint16_t addr, uint8_t value);
      virtual void writeMemoryWord(uint16_t addr, uint16_t value);
      virtual void doOut(uint16_t addr, uint8_t value);
      virtual uint8_t doIn(uint16_t addr);
      virtual void updateCycles(int cycles);
    };
    class Memory_ : public Memory {
     private:
      Ep128VM&  vm;
     public:
      Memory_(Ep128VM& vm_);
      virtual ~Memory_();
     protected:
      virtual void breakPointCallback(bool isWrite,
                                      uint16_t addr, uint8_t value);
    };
    class IOPorts_ : public IOPorts {
     private:
      Ep128VM&  vm;
     public:
      IOPorts_(Ep128VM& vm_);
      virtual ~IOPorts_();
     protected:
      virtual void breakPointCallback(bool isWrite,
                                      uint16_t addr, uint8_t value);
    };
    class Dave_ : public Dave {
     private:
      Ep128VM&  vm;
     public:
      Dave_(Ep128VM& vm_);
      virtual ~Dave_();
     protected:
      virtual void setMemoryPage(uint8_t page, uint8_t segment);
      virtual void setMemoryWaitMode(int mode);
      virtual void setRemote1State(int state);
      virtual void setRemote2State(int state);
      virtual void interruptRequest();
      virtual void clearInterruptRequest();
    };
    class Nick_ : public Nick {
     private:
      Ep128VM&  vm;
     public:
      Nick_(Ep128VM& vm_);
      virtual ~Nick_();
     protected:
      virtual void irqStateChange(bool newState);
      virtual void drawLine(const uint8_t *buf, size_t nBytes);
      virtual void vsyncStateChange(bool newState, unsigned int currentSlot_);
    };
    // ----------------
    Ep128Emu::OpenGLDisplay&  display;
    Z80_      z80;
    Memory_   memory;
    IOPorts_  ioPorts;
    Dave_     dave;
    Nick_     nick;
    DaveConverter   *audioOutput;
    uint8_t   pageTable[4];
    uint8_t   romBitmap[32];
    uint8_t   ramBitmap[32];
    size_t    cpuFrequency;             // defaults to 4000000 Hz
    size_t    daveFrequency;            // for now, this is always 500000 Hz
    size_t    nickFrequency;            // defaults to 15625 * 57 = 890625 Hz
    size_t    videoMemoryLatency;       // defaults to 62 ns
    int64_t   nickCyclesRemaining;      // in 2^-32 NICK cycle units
    int64_t   cpuCyclesPerNickCycle;    // in 2^-32 Z80 cycle units
    int64_t   cpuCyclesRemaining;       // in 2^-32 Z80 cycle units
    int64_t   cpuSyncToNickCnt;         // in 2^-32 Z80 cycle units
    int64_t   videoMemoryLatencyCycles; // in 2^-32 Z80 cycle units
    int64_t   daveCyclesPerNickCycle;   // in 2^-32 DAVE cycle units
    int64_t   daveCyclesRemaining;      // in 2^-32 DAVE cycle units
    int       memoryWaitMode;           // set on write to port 0xBF
    bool      memoryTimingEnabled;
    bool      displayEnabled;
    bool      audioOutputEnabled;
    bool      audioOutputHighQuality;
    float     audioOutputSampleRate;
    int       audioOutputDevice;
    float     audioOutputLatency;
    int       audioOutputPeriodsHW;
    int       audioOutputPeriodsSW;
    float     audioOutputVolume;
    float     audioOutputFilter1Freq;
    float     audioOutputFilter2Freq;
    std::string audioOutputFileName;
    std::string tapeFileName;
    Tape      *tape;
    int64_t   tapeSamplesPerDaveCycle;
    int64_t   tapeSamplesRemaining;
    bool      isRemote1On;
    bool      isRemote2On;
    bool      tapePlaybackOn;
    bool      tapeRecordOn;
    bool      fastTapeModeEnabled;
    bool      writingAudioOutput;
    // ----------------
    void updateTimingParameters();
    inline void updateCPUCycles(int cycles)
    {
      cpuCyclesRemaining -= (int64_t(cycles) << 32);
      if (memoryTimingEnabled) {
        while (cpuSyncToNickCnt >= cpuCyclesRemaining)
          cpuSyncToNickCnt -= cpuCyclesPerNickCycle;
      }
    }
    inline void memoryWaitCycle()
    {
      cpuCyclesRemaining -= (int64_t(1) << 32);
      // assume cpuFrequency > nickFrequency
      if (cpuSyncToNickCnt >= cpuCyclesRemaining)
        cpuSyncToNickCnt -= cpuCyclesPerNickCycle;
    }
    inline void videoMemoryWait()
    {
      // NOTE: videoMemoryLatencyCycles also includes the +0xFFFFFFFF
      // needed for ceil rounding
      cpuCyclesRemaining -= (((cpuCyclesRemaining - cpuSyncToNickCnt)
                              + videoMemoryLatencyCycles)
                             & (int64_t(-1) - int64_t(0xFFFFFFFFUL)));
      cpuSyncToNickCnt -= cpuCyclesPerNickCycle;
    }
    static uint8_t davePortReadCallback(void *userData, uint16_t addr);
    static void davePortWriteCallback(void *userData,
                                      uint16_t addr, uint8_t value);
    static void nickPortWriteCallback(void *userData,
                                      uint16_t addr, uint8_t value);
   public:
    Ep128VM(Ep128Emu::OpenGLDisplay&);
    virtual ~Ep128VM();
    // run emulation for the specified number of microseconds
    virtual void run(size_t microseconds);
    // reset emulated machine; if 'isColdReset' is true, RAM is cleared
    virtual void reset(bool isColdReset = false);
    // delete all ROM segments, and resize RAM to 'memSize' kilobytes
    // implies calling reset(true)
    virtual void resetMemoryConfiguration(size_t memSize);
    // load ROM segment 'n' from the specified file, skipping 'offs' bytes
    virtual void loadROMSegment(uint8_t n, const char *fileName, size_t offs);
    // set audio output quality parameters (changing these settings implies
    // restarting the audio output stream if it is already open)
    virtual void setAudioOutputQuality(bool useHighQualityResample,
                                       float sampleRate_);
    // select and open audio output device, and set buffering parameters
    // (changing these settings implies restarting the audio output stream)
    virtual void setAudioOutputDeviceParameters(int devNum,
                                                float totalLatency_ = 0.1f,
                                                int nPeriodsHW_ = 4,
                                                int nPeriodsSW_ = 3);
    // set cutoff frequencies of highpass filters used on audio output to
    // remove DC offset
    virtual void setAudioOutputFilters(float dcBlockFreq1_,
                                       float dcBlockFreq2_);
    // set amplitude scale for audio output (defaults to 0.7071)
    virtual void setAudioOutputVolume(float ampScale_);
    // set audio output file name (if a file is already opened, and the new
    // name is different, the old file is closed first); a NULL or empty name
    // means not writing sound to a file
    virtual void setAudioOutputFileName(const char *fileName);
    // set if audio data is sent to sound card and output file (disabling
    // this also makes the emulation run faster than real time)
    virtual void setEnableAudioOutput(bool isEnabled);
    // set if video data is sent to the associated OpenGLDisplay object
    virtual void setEnableDisplay(bool isEnabled);
    // set Z80 clock frequency (in Hz); defaults to 4000000 Hz
    virtual void setCPUFrequency(size_t freq_);
    // set the number of NICK 'slots' per second (defaults to 890625 Hz)
    virtual void setNickFrequency(size_t freq_);
    // set parameter used for tuning video memory timing (defaults to 62 ns)
    virtual void setVideoMemoryLatency(size_t t_);
    // set if emulation of memory timing is enabled
    virtual void setEnableMemoryTimingEmulation(bool isEnabled);
    // Set state of key 'keyCode' (0 to 127).
    virtual void setKeyboardState(int keyCode, bool isPressed);
    // Set tape image file name (if the file name is NULL or empty, tape
    // emulation is disabled).
    virtual void setTapeFileName(const char *fileName);
    // start tape playback
    virtual void tapePlay();
    // start tape recording; if the tape file is read-only, this is
    // equivalent to calling tapePlay()
    virtual void tapeRecord();
    // stop tape playback and recording
    virtual void tapeStop();
    // Set tape position to the specified time (in seconds).
    virtual void tapeSeek(double t);
    // Returns the current tape position in seconds, or -1.0 if there is
    // no tape image file opened.
    virtual double getTapePosition() const;
    // Seek forward (if isForward = true) or backward (if isForward = false)
    // to the nearest cue point, or by 't' seconds if no cue point is found.
    virtual void tapeSeekToCuePoint(bool isForward = true, double t = 10.0);
    // Create a new cue point at the current tape position.
    // Has no effect if the file does not have a cue point table, or it
    // is read-only.
    virtual void tapeAddCuePoint();
    // Delete the cue point nearest to the current tape position.
    // Has no effect if the file is read-only.
    virtual void tapeDeleteNearestCuePoint();
    // Delete all cue points. Has no effect if the file is read-only.
    virtual void tapeDeleteAllCuePoints();
    // Set if audio output is turned off on tape I/O, allowing the emulation
    // to run faster than real time.
    virtual void setEnableFastTapeMode(bool isEnabled);
   protected:
    virtual void breakPointCallback(bool isIO, bool isWrite,
                                    uint16_t addr, uint8_t value);
  };

}       // namespace Ep128

#endif  // EP128EMU_EP128VM_HPP

