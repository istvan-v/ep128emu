
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
    File      *demoFile;
    // contains demo data, which is the emulator version number as a 32-bit
    // integer ((MAJOR << 16) + (MINOR << 8) + PATCHLEVEL), followed by a
    // sequence of events in the following format:
    //   uint64_t   deltaTime   (in NICK cycles, stored as MSB first dynamic
    //                          length (1 to 8 bytes) value)
    //   uint8_t    eventType   (currently allowed values are 0 for end of
    //                          demo (zero data bytes), 1 for key press, and
    //                          2 for key release)
    //   uint8_t    dataLength  number of event data bytes
    //   ...        eventData   (dataLength bytes)
    // the event data for event types 1 and 2 is the key code with a length of
    // one byte:
    //   uint8_t    keyCode     key code in the range 0 to 127
    File::Buffer  demoBuffer;
    // true while recording a demo
    bool      isRecordingDemo;
    // true while playing a demo
    bool      isPlayingDemo;
    // true after loading a snapshot; if not playing a demo as well, the
    // keyboard state will be cleared
    bool      snapshotLoadFlag;
    // used for counting time between demo events (in NICK cycles)
    uint64_t  demoTimeCnt;
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
    void stopDemoPlayback();
    void stopDemoRecording(bool writeFile_);
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
    // ---------------------------- TAPE EMULATION ----------------------------
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
    // ------------------------------ DEBUGGING -------------------------------
    // Add breakpoints from the specified ASCII format breakpoint list.
    // The breakpoint list is a sequence of breakpoint definitions, separated
    // by any whitespace characters (space, tab, or newline). A breakpoint
    // definition consists of an address or address range in one of the
    // following formats (each 'n' is a hexadecimal digit):
    //   nn             a single I/O port address
    //   nn-nn          all I/O port addresses in the specified range
    //   nnnn           a single Z80 memory address
    //   nnnn-nnnn      all Z80 memory addresses in the specified range
    //   nn:nnnn        a single raw memory address, as segment:offset
    //   nn:nnnn-nnnn   range of raw memory addresses
    //                  (segment:first_offset-last_offset)
    // and these optional modifiers:
    //   r              the breakpoint is triggered on reads
    //   w              the breakpoint is triggered on writes
    //   p0             the breakpoint has a priority of 0
    //   p1             the breakpoint has a priority of 1
    //   p2             the breakpoint has a priority of 2
    //   p3             the breakpoint has a priority of 3
    // by default, the breakpoint is triggered on both reads and writes if
    // 'r' or 'w' is not used, and has a priority of 2.
    // Example: 8000-8003rp1 means break on reading Z80 addresses 0x8000,
    // 0x8001, 0x8002, and 0x8003, if the breakpoint priority threshold is
    // less than or equal to 1.
    // If there are any syntax errors in the list, Ep128::Exception is
    // thrown, and no breakpoints are added.
    virtual void setBreakPoints(const std::string& bpList);
    // Returns currently defined breakpoints in the same format as described
    // above.
    virtual std::string getBreakPoints();
    // Clear all breakpoints.
    virtual void clearBreakPoints();
    // Set breakpoint priority threshold (0 to 4); breakpoints with a
    // priority less than this value will not trigger a break.
    virtual void setBreakPointPriorityThreshold(int n);
    // Returns the segment at page 'n' (0 to 3).
    virtual uint8_t getMemoryPage(int n) const;
    // Read a byte from memory; bits 14 to 21 of 'addr' define the segment
    // number, while bits 0 to 13 are the offset (0 to 0x3FFF) within the
    // segment.
    virtual uint8_t readMemory(uint32_t addr) const;
    // Returns read-only reference to a structure containing all Z80
    // registers; see z80/z80.hpp for more information.
    virtual const Z80_REGISTERS& getCPURegisters() const;
    // ------------------------------- FILE I/O -------------------------------
    // Save snapshot of virtual machine state, including all ROM and RAM
    // segments, as well as Z80, NICK, and DAVE registers. Note that the clock
    // frequency and timing settings, tape and disk state, and breakpoint list
    // are not saved.
    virtual void saveState(File&);
    // Save clock frequency and timing settings.
    virtual void saveMachineConfiguration(File&);
    // Register all types of file data supported by this class, for use by
    // File::processAllChunks(). Note that loading snapshot data will clear
    // all breakpoints.
    virtual void registerChunkTypes(File&);
    // Start recording a demo to the file object, which will be used until
    // the recording is stopped for some reason.
    // Implies calling saveMachineConfiguration() and saveState() first.
    virtual void recordDemo(File&);
    // Stop playing or recording demo.
    virtual void stopDemo();
    // Returns true if a demo is currently being recorded. The recording stops
    // when stopDemo() is called, any tape or disk I/O is attempted, clock
    // frequency and timing settings are changed, or a snapshot is loaded.
    // This function will also flush demo data to the associated file object
    // after recording is stopped for some reason other than calling
    // stopDemo().
    virtual bool getIsRecordingDemo();
    // Returns true if a demo is currently being played. The playback stops
    // when the end of the demo is reached, stopDemo() is called, any tape or
    // disk I/O is attempted, clock frequency and timing settings are changed,
    // or a snapshot is loaded. Note that keyboard events are ignored while
    // playing a demo.
    virtual bool getIsPlayingDemo() const;
    // ----------------
    virtual void loadState(File::Buffer&);
    virtual void loadMachineConfiguration(File::Buffer&);
    virtual void loadDemo(File::Buffer&);
   protected:
    virtual void breakPointCallback(bool isIO, bool isWrite,
                                    uint16_t addr, uint8_t value);
  };

}       // namespace Ep128

#endif  // EP128EMU_EP128VM_HPP

