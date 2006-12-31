
// plus4 -- portable Commodore PLUS/4 emulator
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

#ifndef EP128EMU_PLUS4VM_HPP
#define EP128EMU_PLUS4VM_HPP

#include "ep128emu.hpp"
#include "ted.hpp"
#include "display.hpp"
#include "soundio.hpp"
#include "vm.hpp"

namespace Plus4 {

  class Plus4VM : public Ep128Emu::VirtualMachine {
   private:
    class TED7360_ : public TED7360 {
     private:
      Plus4VM&  vm;
      int       lineCnt_;
     public:
      TED7360_(Plus4VM& vm_);
      virtual ~TED7360_();
     protected:
      virtual void playSample(int16_t sampleValue);
      virtual void drawLine(const uint8_t *buf, size_t nBytes);
      virtual void verticalSync(bool newState_, unsigned int currentSlot_);
      virtual bool systemCallback(uint8_t n);
      virtual void breakPointCallback(bool isWrite,
                                      uint16_t addr, uint8_t value);
    };
    // ----------------
    TED7360_                *ted;
    size_t    cpuFrequencyMultiplier;   // defaults to 1
    size_t    tedFrequency;             // fixed at 886724 Hz
    size_t    soundClockFrequency;      // fixed at 221681 Hz
    int64_t   tedCyclesRemaining;       // in 2^-32 TED cycle units
    int64_t   tapeSamplesPerTEDCycle;
    int64_t   tapeSamplesRemaining;
    Ep128Emu::File  *demoFile;
    // contains demo data, which is the emulator version number as a 32-bit
    // integer ((MAJOR << 16) + (MINOR << 8) + PATCHLEVEL), followed by a
    // sequence of events in the following format:
    //   uint64_t   deltaTime   (in TED cycles, stored as MSB first dynamic
    //                          length (1 to 8 bytes) value)
    //   uint8_t    eventType   (currently allowed values are 0 for end of
    //                          demo (zero data bytes), 1 for key press, and
    //                          2 for key release)
    //   uint8_t    dataLength  number of event data bytes
    //   ...        eventData   (dataLength bytes)
    // the event data for event types 1 and 2 is the key code with a length of
    // one byte:
    //   uint8_t    keyCode     key code in the range 0 to 127
    Ep128Emu::File::Buffer  demoBuffer;
    // true while recording a demo
    bool      isRecordingDemo;
    // true while playing a demo
    bool      isPlayingDemo;
    // true after loading a snapshot; if not playing a demo as well, the
    // keyboard state will be cleared
    bool      snapshotLoadFlag;
    // used for counting time between demo events (in TED cycles)
    uint64_t  demoTimeCnt;
    // ----------------
    void stopDemoPlayback();
    void stopDemoRecording(bool writeFile_);
   public:
    Plus4VM(Ep128Emu::VideoDisplay&, Ep128Emu::AudioOutput&);
    virtual ~Plus4VM();
    // run emulation for the specified number of microseconds
    virtual void run(size_t microseconds);
    // reset emulated machine; if 'isColdReset' is true, RAM is cleared
    virtual void reset(bool isColdReset = false);
    // delete all ROM segments, and resize RAM to 'memSize' kilobytes
    // implies calling reset(true)
    virtual void resetMemoryConfiguration(size_t memSize);
    // load ROM segment 'n' from the specified file, skipping 'offs' bytes
    virtual void loadROMSegment(uint8_t n, const char *fileName, size_t offs);
    // set CPU clock frequency (in Hz); defaults to 1773448 Hz
    virtual void setCPUFrequency(size_t freq_);
    // set the number of video 'slots' per second (defaults to 886724 Hz)
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
    virtual void setTapeFileName(const std::string& fileName);
    // start tape playback
    virtual void tapePlay();
    // start tape recording; if the tape file is read-only, this is
    // equivalent to calling tapePlay()
    virtual void tapeRecord();
    // stop tape playback and recording
    virtual void tapeStop();
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
    // Set if the breakpoint callback should be called whenever the first byte
    // of a CPU instruction is read from memory. Breakpoints are ignored in
    // this mode.
    virtual void setSingleStepMode(bool isEnabled);
    // Returns the segment at page 'n' (0 to 3).
    virtual uint8_t getMemoryPage(int n) const;
    // Read a byte from memory; bits 14 to 21 of 'addr' define the segment
    // number, while bits 0 to 13 are the offset (0 to 0x3FFF) within the
    // segment.
    virtual uint8_t readMemory(uint32_t addr) const;
    // Returns read-only reference to a structure containing all CPU
    // registers; see plus4/cpu.hpp for more information.
    virtual const M7501Registers& getCPURegisters() const;
    // ------------------------------- FILE I/O -------------------------------
    // Save snapshot of virtual machine state, including all ROM and RAM
    // segments, as well as all hardware registers. Note that the clock
    // frequency and timing settings, tape and disk state, and breakpoint list
    // are not saved.
    virtual void saveState(Ep128Emu::File&);
    // Save clock frequency and timing settings.
    virtual void saveMachineConfiguration(Ep128Emu::File&);
    // save program
    virtual void saveProgram(Ep128Emu::File&);
    virtual void saveProgram(const char *fileName);
    // load program
    virtual void loadProgram(const char *fileName);
    // Register all types of file data supported by this class, for use by
    // Ep128Emu::File::processAllChunks(). Note that loading snapshot data will
    // clear all breakpoints.
    virtual void registerChunkTypes(Ep128Emu::File&);
    // Start recording a demo to the file object, which will be used until
    // the recording is stopped for some reason.
    // Implies calling saveMachineConfiguration() and saveState() first.
    virtual void recordDemo(Ep128Emu::File&);
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
    virtual void loadState(Ep128Emu::File::Buffer&);
    virtual void loadMachineConfiguration(Ep128Emu::File::Buffer&);
    virtual void loadDemo(Ep128Emu::File::Buffer&);
  };

}       // namespace Plus4

#endif  // EP128EMU_PLUS4VM_HPP

