
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

#ifndef EP128EMU_EP128VM_HPP
#define EP128EMU_EP128VM_HPP

#include "ep128emu.hpp"
#include "z80/z80.hpp"
#include "memory.hpp"
#include "ioports.hpp"
#include "dave.hpp"
#include "nick.hpp"
#include "display.hpp"
#include "snd_conv.hpp"
#include "soundio.hpp"
#include "vm.hpp"
#include "wd177x.hpp"

#include <map>

namespace Ep128 {

  class Ep128VM : public Ep128Emu::VirtualMachine {
   private:
    class Z80_ : public Z80 {
     private:
      Ep128VM&  vm;
      std::map< uint8_t, std::FILE * >  fileChannels;
      bool      defaultDeviceIsFILE;
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
      virtual void tapePatch();
     private:
      uint8_t readUserMemory(uint16_t addr);
      void writeUserMemory(uint16_t addr, uint8_t value);
     public:
      void closeAllFiles();
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
    Z80_      z80;
    Memory_   memory;
    IOPorts_  ioPorts;
    Dave_     dave;
    Nick_     nick;
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
    bool      singleStepModeEnabled;
    int64_t   tapeSamplesPerDaveCycle;
    int64_t   tapeSamplesRemaining;
    bool      isRemote1On;
    bool      isRemote2On;
    Ep128Emu::File  *demoFile;
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
    Ep128Emu::File::Buffer  demoBuffer;
    // true while recording a demo
    bool      isRecordingDemo;
    // true while playing a demo
    bool      isPlayingDemo;
    // true after loading a snapshot; if not playing a demo as well, the
    // keyboard state will be cleared
    bool      snapshotLoadFlag;
    // used for counting time between demo events (in NICK cycles)
    uint64_t  demoTimeCnt;
    // floppy drives
    Ep128Emu::WD177x  floppyDrives[4];
    uint8_t   currentFloppyDrive;
    uint8_t   breakPointPriorityThreshold;
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
    static uint8_t nickPortReadCallback(void *userData, uint16_t addr);
    static void nickPortWriteCallback(void *userData,
                                      uint16_t addr, uint8_t value);
    static uint8_t exdosPortReadCallback(void *userData, uint16_t addr);
    static void exdosPortWriteCallback(void *userData,
                                       uint16_t addr, uint8_t value);
    void stopDemoPlayback();
    void stopDemoRecording(bool writeFile_);
   public:
    Ep128VM(Ep128Emu::VideoDisplay&, Ep128Emu::AudioOutput&);
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
    // set CPU clock frequency (in Hz); defaults to 4000000 Hz
    virtual void setCPUFrequency(size_t freq_);
    // set the number of video 'slots' per second (defaults to 890625 Hz)
    virtual void setVideoFrequency(size_t freq_);
    // set parameter used for tuning video memory timing (defaults to 62 ns)
    virtual void setVideoMemoryLatency(size_t t_);
    // set if emulation of memory timing is enabled
    virtual void setEnableMemoryTimingEmulation(bool isEnabled);
    // Set state of key 'keyCode' (0 to 127; see dave.hpp).
    virtual void setKeyboardState(int keyCode, bool isPressed);
    // -------------------------- DISK AND FILE I/O ---------------------------
    // Load disk image for drive 'n' (counting from zero); an empty file
    // name means no disk.
    virtual void setDiskImageFile(int n, const std::string& fileName_,
                                  int nTracks_ = -1, int nSides_ = 2,
                                  int nSectorsPerTrack_ = 9);
    // ---------------------------- TAPE EMULATION ----------------------------
    // Set tape image file name (if the file name is NULL or empty, tape
    // emulation is disabled).
    virtual void setTapeFileName(const std::string& fileName);
    // start tape playback
    virtual void tapePlay();
    // start tape recording; if the tape file is read-only, this is
    // equivalent to calling tapePlay()
    virtual void tapeRecord();
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
    // Returns the current value of the Z80 program counter (PC).
    virtual uint16_t getProgramCounter() const;
    // Returns the CPU address of the last byte pushed to the stack.
    virtual uint16_t getStackPointer() const;
    // Dumps the current values of all CPU registers to 'buf' in ASCII format.
    // The register list may be written as multiple lines separated by '\n'
    // characters, however, there is no newline character at the end of the
    // buffer. The maximum line width is 40 characters.
    virtual void listCPURegisters(std::string& buf) const;
    // Disassemble one Z80 instruction, starting from memory address 'addr',
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
    // Returns read-only reference to a structure containing all Z80
    // registers; see z80/z80.hpp for more information.
    virtual const Z80_REGISTERS& getZ80Registers() const;
    // ------------------------------- FILE I/O -------------------------------
    // Save snapshot of virtual machine state, including all ROM and RAM
    // segments, as well as all hardware registers. Note that the clock
    // frequency and timing settings, tape and disk state, and breakpoint list
    // are not saved.
    virtual void saveState(Ep128Emu::File&);
    // Save clock frequency and timing settings.
    virtual void saveMachineConfiguration(Ep128Emu::File&);
    // Register all types of file data supported by this class, for use by
    // File::processAllChunks(). Note that loading snapshot data will clear
    // all breakpoints.
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

}       // namespace Ep128

#endif  // EP128EMU_EP128VM_HPP

