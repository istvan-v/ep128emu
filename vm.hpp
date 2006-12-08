
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

#ifndef EP128EMU_VM_HPP
#define EP128EMU_VM_HPP

#include "ep128emu.hpp"
#include "fileio.hpp"
#include "display.hpp"
#include "snd_conv.hpp"
#include "soundio.hpp"
#include "tape.hpp"

namespace Ep128Emu {

  class VirtualMachine {
   protected:
    VideoDisplay&   display;
   private:
    AudioOutput&    audioOutput;
    AudioConverter  *audioConverter;
    bool            writingAudioOutput;
    bool            audioOutputEnabled;
    bool            audioOutputHighQuality;
    bool            displayEnabled;
    float           audioConverterSampleRate;
    float           audioOutputSampleRate;
    float           audioOutputVolume;
    float           audioOutputFilter1Freq;
    float           audioOutputFilter2Freq;
    bool            tapePlaybackOn;
    bool            tapeRecordOn;
    bool            tapeMotorOn;
    bool            fastTapeModeEnabled;
    Tape            *tape;
    std::string     tapeFileName;
   protected:
    void            (*breakPointCallback)(void *userData,
                                          bool isIO, bool isWrite,
                                          uint16_t addr, uint8_t value);
    void            *breakPointCallbackUserData;
   public:
    VirtualMachine(VideoDisplay& display_, AudioOutput& audioOutput_);
    virtual ~VirtualMachine();
    // run emulation for the specified number of microseconds
    virtual void run(size_t microseconds);
    // reset emulated machine; if 'isColdReset' is true, RAM is cleared
    virtual void reset(bool isColdReset = false);
    // delete all ROM segments, and resize RAM to 'memSize' kilobytes
    // implies calling reset(true)
    virtual void resetMemoryConfiguration(size_t memSize);
    // load ROM segment 'n' from the specified file, skipping 'offs' bytes
    virtual void loadROMSegment(uint8_t n, const char *fileName, size_t offs);
    // set audio output quality
    virtual void setAudioOutputQuality(bool useHighQualityResample);
    // set cutoff frequencies of highpass filters used on audio output to
    // remove DC offset
    virtual void setAudioOutputFilters(float dcBlockFreq1_,
                                       float dcBlockFreq2_);
    // set amplitude scale for audio output (defaults to 0.7071)
    virtual void setAudioOutputVolume(float ampScale_);
    // set if audio data is sent to sound card and output file (disabling
    // this also makes the emulation run faster than real time)
    virtual void setEnableAudioOutput(bool isEnabled);
    // set if video data is sent to the associated VideoDisplay object
    virtual void setEnableDisplay(bool isEnabled);
    // set CPU clock frequency (in Hz)
    virtual void setCPUFrequency(size_t freq_);
    // set the number of video 'slots' per second
    virtual void setVideoFrequency(size_t freq_);
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
    //   nnnn           a single CPU memory address
    //   nnnn-nnnn      all CPU memory addresses in the specified range
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
    // Example: 8000-8003rp1 means break on reading CPU addresses 0x8000,
    // 0x8001, 0x8002, and 0x8003, if the breakpoint priority threshold is
    // less than or equal to 1.
    // If there are any syntax errors in the list, Ep128Emu::Exception is
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
    // Set function to be called when a breakpoint is triggered.
    virtual void setBreakPointCallback(void (*breakPointCallback_)(
                                           void *userData,
                                           bool isIO, bool isWrite,
                                           uint16_t addr, uint8_t value),
                                       void *userData_);
    // Returns the segment at page 'n' (0 to 3).
    virtual uint8_t getMemoryPage(int n) const;
    // Read a byte from memory; bits 14 to 21 of 'addr' define the segment
    // number, while bits 0 to 13 are the offset (0 to 0x3FFF) within the
    // segment.
    virtual uint8_t readMemory(uint32_t addr) const;
    // ------------------------------- FILE I/O -------------------------------
    // Save snapshot of virtual machine state, including all ROM and RAM
    // segments, as well as all hardware registers. Note that the clock
    // frequency and timing settings, tape and disk state, and breakpoint list
    // are not saved.
    virtual void saveState(File& f);
    // Save clock frequency and timing settings.
    virtual void saveMachineConfiguration(File& f);
    // Register all types of file data supported by this class, for use by
    // File::processAllChunks(). Note that loading snapshot data will clear
    // all breakpoints.
    virtual void registerChunkTypes(File& f);
    // Start recording a demo to the file object, which will be used until
    // the recording is stopped for some reason.
    // Implies calling saveMachineConfiguration() and saveState() first.
    virtual void recordDemo(File& f);
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
    virtual void loadState(File::Buffer& buf);
    virtual void loadMachineConfiguration(File::Buffer& buf);
    virtual void loadDemo(File::Buffer& buf);
   protected:
    inline void sendAudioOutput(uint32_t audioData)
    {
      if (this->writingAudioOutput)
        this->audioConverter->sendInputSignal(audioData);
    }
    inline void sendAudioOutput(uint16_t left, uint16_t right)
    {
      if (this->writingAudioOutput)
        this->audioConverter->sendInputSignal(uint32_t(left)
                                              | (uint32_t(right) << 16));
    }
   private:
    void setTapeMotorState_(bool newState);
   protected:
    inline void setTapeMotorState(bool newState)
    {
      if (newState != this->tapeMotorOn)
        this->setTapeMotorState_(newState);
    }
    inline bool getIsTapeMotorOn() const
    {
      return this->tapeMotorOn;
    }
    inline int getTapeButtonState() const
    {
      if (this->tapePlaybackOn)
        return (this->tapeRecordOn ? 2 : 1);
      return 0;
    }
    inline bool haveTape() const
    {
      return (this->tape != (Tape *) 0);
    }
    inline long getTapeSampleRate() const
    {
      if (this->tape != (Tape *) 0)
        return this->tape->getSampleRate();
      return 0L;
    }
    inline int runTape(int tapeInput)
    {
      if (this->tape != (Tape *) 0 &&
          this->tapeMotorOn && this->tapePlaybackOn) {
        if (this->tapeRecordOn) {
          this->tape->setInputSignal(tapeInput);
          this->tape->runOneSample();
          return 0;
        }
        else {
          this->tape->runOneSample();
          return (this->tape->getOutputSignal());
        }
      }
      return 0;
    }
    inline bool getIsDisplayEnabled() const
    {
      return this->displayEnabled;
    }
    void setAudioConverterSampleRate(float sampleRate_);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_VM_HPP

