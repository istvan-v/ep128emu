
// plus4 -- portable Commodore PLUS/4 emulator
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
    // Add breakpoints from the specified breakpoint list (see also
    // bplist.hpp).
    virtual void setBreakPoints(const Ep128Emu::BreakPointList& bpList);
    // Returns the currently defined breakpoints.
    virtual Ep128Emu::BreakPointList getBreakPoints();
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
    // Read a byte from memory. If 'isCPUAddress' is false, bits 14 to 21 of
    // 'addr' define the segment number, while bits 0 to 13 are the offset
    // (0 to 0x3FFF) within the segment; otherwise, 'addr' is interpreted as
    // a 16-bit CPU address.
    virtual uint8_t readMemory(uint32_t addr, bool isCPUAddress = false) const;
    // Write a byte to memory. If 'isCPUAddress' is false, bits 14 to 21 of
    // 'addr' define the segment number, while bits 0 to 13 are the offset
    // (0 to 0x3FFF) within the segment; otherwise, 'addr' is interpreted as
    // a 16-bit CPU address.
    // NOTE: calling this function will stop any demo recording or playback.
    virtual void writeMemory(uint32_t addr, uint8_t value,
                             bool isCPUAddress = false);
    // Returns the current value of the CPU program counter (PC).
    virtual uint16_t getProgramCounter() const;
    // Returns the CPU address of the last byte pushed to the stack.
    virtual uint16_t getStackPointer() const;
    // Dumps the current values of all CPU registers to 'buf' in ASCII format.
    // The register list may be written as multiple lines separated by '\n'
    // characters, however, there is no newline character at the end of the
    // buffer. The maximum line width is 40 characters.
    virtual void listCPURegisters(std::string& buf) const;
    // Disassemble one CPU instruction, starting from memory address 'addr',
    // and write the result to 'buf' (not including a newline character).
    // 'offs' is added to the instruction address that is printed.
    // The maximum line width is 40 characters.
    // Returns the address of the next instruction. If 'isCPUAddress' is
    // true, 'addr' is interpreted as a 16-bit CPU address, otherwise it
    // is assumed to be a 22-bit physical address (8 bit segment + 14 bit
    // offset).
    virtual uint32_t disassembleInstruction(std::string& buf, uint32_t addr,
                                            bool isCPUAddress = false,
                                            int32_t offs = 0) const;
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

