
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

#ifndef EP128EMU_TVC64VM_HPP
#define EP128EMU_TVC64VM_HPP

#include "ep128emu.hpp"
#include "z80/z80.hpp"
#include "tvcmem.hpp"
#include "ioports.hpp"
#include "crtc6845.hpp"
#include "tvcvideo.hpp"
#include "display.hpp"
#include "snd_conv.hpp"
#include "soundio.hpp"
#include "vm.hpp"
#include "ep_fdd.hpp"
#include "wd177x.hpp"
#ifdef ENABLE_SDEXT
#  include "sdext.hpp"
#endif

namespace Ep128Emu {
  class VideoCapture;
}

namespace TVC64 {

  class TVC64VM : public Ep128Emu::VirtualMachine {
   private:
    class Z80_ : public Ep128::Z80 {
     private:
      TVC64VM&  vm;
     public:
      Z80_(TVC64VM& vm_);
      virtual ~Z80_();
     protected:
      virtual EP128EMU_REGPARM1 void executeInterrupt();
      virtual EP128EMU_REGPARM2 uint8_t readMemory(uint16_t addr);
      virtual EP128EMU_REGPARM2 uint16_t readMemoryWord(uint16_t addr);
      virtual EP128EMU_REGPARM1 uint8_t readOpcodeFirstByte();
      virtual EP128EMU_REGPARM2
          uint8_t readOpcodeSecondByte(const bool *invalidOpcodeTable =
                                           (bool *) 0);
      virtual EP128EMU_REGPARM2 uint8_t readOpcodeByte(int offset);
      virtual EP128EMU_REGPARM2 uint16_t readOpcodeWord(int offset);
      virtual EP128EMU_REGPARM3 void writeMemory(uint16_t addr, uint8_t value);
      virtual EP128EMU_REGPARM3 void writeMemoryWord(uint16_t addr,
                                                     uint16_t value);
      virtual EP128EMU_REGPARM2 void pushWord(uint16_t value);
      virtual EP128EMU_REGPARM3 void doOut(uint16_t addr, uint8_t value);
      virtual EP128EMU_REGPARM2 uint8_t doIn(uint16_t addr);
      virtual EP128EMU_REGPARM1 void updateCycle();
      virtual EP128EMU_REGPARM2 void updateCycles(int cycles);
    };
    class Memory_ : public Memory {
     private:
      TVC64VM&  vm;
     public:
      Memory_(TVC64VM& vm_);
      virtual ~Memory_();
      void setEnableSDExt();
     protected:
      virtual void breakPointCallback(bool isWrite,
                                      uint16_t addr, uint8_t value);
      virtual EP128EMU_REGPARM2 uint8_t extensionRead(uint16_t addr);
      virtual EP128EMU_REGPARM2 uint8_t extensionReadNoDebug(
                                            uint16_t addr) const;
      virtual EP128EMU_REGPARM3 void extensionWrite(uint16_t addr,
                                                    uint8_t value);
    };
    class IOPorts_ : public Ep128::IOPorts {
     private:
      TVC64VM&  vm;
     public:
      IOPorts_(TVC64VM& vm_);
      virtual ~IOPorts_();
     protected:
      virtual void breakPointCallback(bool isWrite,
                                      uint16_t addr, uint8_t value);
    };
    class TVCVideo_ : public TVCVideo {
     private:
      TVC64VM&  vm;
     public:
      TVCVideo_(TVC64VM& vm_,
                const CPC464::CRTC6845& crtc_, const uint8_t *videoMemory_);
      virtual ~TVCVideo_();
     protected:
      virtual void drawLine(const uint8_t *buf, size_t nBytes);
      virtual void vsyncStateChange(bool newState, unsigned int currentSlot_);
    };
    // ----------------
    Z80_      z80;
    Memory_   memory;
    IOPorts_  ioPorts;
    CPC464::CRTC6845  crtc;
    TVCVideo_ videoRenderer;
    uint32_t  crtcCyclesRemainingL;     // in 2^-32 CRTC cycle units
    int32_t   crtcCyclesRemainingH;     // in CRTC cycle units
    uint8_t   z80HalfCycleCnt;          // time since the last CRTC cycle
    uint8_t   machineHalfCycleCnt;
    uint8_t   tapeInputSignal;
    uint8_t   tapeOutputSignal;
    uint8_t   crtcRegisterSelected;
    uint8_t   irqState;
    uint8_t   irqEnableMask;
    // 0: normal mode, 1: single step, 2: step over, 3: trace
    uint8_t   singleStepMode;
    int32_t   singleStepModeNextAddr;
    bool      tapeCallbackFlag;
    bool      prvTapeCallbackFlag;
    uint8_t   keyboardRow;
    bool      prvSndIntState;
    uint32_t  toneGenCnt1;
    uint32_t  toneGenFreq;
    uint8_t   toneGenCnt2;
    bool      toneGenEnabled;
    uint8_t   audioOutputLevel;
    uint32_t  soundOutputSignal;
    Ep128Emu::File  *demoFile;
    // contains demo data, which is the emulator version number as a 32-bit
    // integer ((MAJOR << 16) + (MINOR << 8) + PATCHLEVEL), followed by a
    // sequence of events in the following format:
    //   uint64_t   deltaTime   (in CRTC cycles, stored as MSB first dynamic
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
    // used for counting time between demo events (in CRTC cycles)
    uint64_t  demoTimeCnt;
    // floppy drives
    Ep128Emu::WD177x      wd177x;
    Ep128Emu::FloppyDrive floppyDrives[4];
    uint8_t   vtdosROMPage;             // 0 to 3
    uint8_t   breakPointPriorityThreshold;
    struct TVC64VMCallback {
      void      (*func)(void *);
      void      *userData;
      TVC64VMCallback  *nxt;
    };
    TVC64VMCallback  callbacks[16];
    TVC64VMCallback  *firstCallback;
    Ep128Emu::VideoCapture  *videoCapture;
    int64_t   tapeSamplesPerCRTCCycle;
    int64_t   tapeSamplesRemaining;
    size_t    crtcFrequency;            // defaults to 1562500 Hz
    uint8_t   keyboardState[16];
    uint8_t   tvcKeyboardState[16];
#ifdef ENABLE_SDEXT
    Ep128::SDExt  sdext;
#endif
    // ----------------
    EP128EMU_INLINE void updateCPUCycles(int cycles);
    EP128EMU_INLINE void updateCPUHalfCycles(int halfCycles);
    EP128EMU_INLINE void memoryWait(uint16_t addr);
    EP128EMU_INLINE void memoryWaitM1(uint16_t addr);
    EP128EMU_INLINE void ioPortWait(uint16_t addr);
    EP128EMU_INLINE void updateSndIntState(bool cursorState);
    EP128EMU_REGPARM1 void runDevices();
    static uint8_t ioPortReadCallback(void *userData, uint16_t addr);
    static void ioPortWriteCallback(void *userData,
                                    uint16_t addr, uint8_t value);
    static uint8_t ioPortDebugReadCallback(void *userData, uint16_t addr);
    static EP128EMU_REGPARM2 void hSyncStateChangeCallback(void *userData,
                                                           bool newState);
    static EP128EMU_REGPARM2 void vSyncStateChangeCallback(void *userData,
                                                           bool newState);
    static void tapeCallback(void *userData);
    static void demoPlayCallback(void *userData);
    static void demoRecordCallback(void *userData);
    static void videoCaptureCallback(void *userData);
    void stopDemoPlayback();
    void stopDemoRecording(bool writeFile_);
    uint8_t checkSingleStepModeBreak();
    void convertKeyboardState();
    void resetKeyboard();
    void resetFloppyDrives(bool isColdReset);
    // Set function to be called at every CRTC cycle. The functions are called
    // in the order of being registered; up to 16 callbacks can be set.
    void setCallback(void (*func)(void *userData), void *userData_,
                     bool isEnabled);
   public:
    TVC64VM(Ep128Emu::VideoDisplay&, Ep128Emu::AudioOutput&);
    virtual ~TVC64VM();
    /*!
     * Run emulation for the specified number of microseconds.
     */
    virtual void run(size_t microseconds);
    /*!
     * Reset emulated machine; if 'isColdReset' is true, RAM is cleared.
     */
    virtual void reset(bool isColdReset = false);
    /*!
     * Delete all ROM segments, and resize RAM to 'memSize' kilobytes;
     * implies calling reset(true).
     */
    virtual void resetMemoryConfiguration(size_t memSize);
    /*!
     * Load ROM segment 'n' from the specified file, skipping 'offs' bytes.
     */
    virtual void loadROMSegment(uint8_t n, const char *fileName, size_t offs);
#ifdef ENABLE_SDEXT
    /*!
     * Set if SD card emulation should be enabled on segment 0x01,
     * and the file name of the 64 KB flash ROM image.
     */
    virtual void configureSDCard(bool isEnabled,
                                 const std::string& romFileName);
#endif
    /*!
     * Set the number of video 'slots' per second (defaults to 1000000 Hz).
     */
    virtual void setVideoFrequency(size_t freq_);
    /*!
     * Set state of key 'keyCode' (0 to 127; see tvc64vm.cpp).
     */
    virtual void setKeyboardState(int keyCode, bool isPressed);
    /*!
     * Returns status information about the emulated machine (see also
     * struct VMStatus above, and the comments for functions that return
     * individual status values).
     */
    virtual void getVMStatus(VirtualMachine::VMStatus& vmStatus_);
    /*!
     * Create video capture object with the specified frame rate (24 to 60)
     * and format (768x576 RLE8 or 384x288 YV12) if it does not exist yet,
     * and optionally set callbacks for printing error messages and asking
     * for a new output file on reaching 2 GB file size.
     */
    virtual void openVideoCapture(
        int frameRate_ = 50,
        bool yuvFormat_ = false,
        void (*errorCallback_)(void *userData, const char *msg) =
            (void (*)(void *, const char *)) 0,
        void (*fileNameCallback_)(void *userData, std::string& fileName) =
            (void (*)(void *, std::string&)) 0,
        void *userData_ = (void *) 0);
    /*!
     * Set output file name for video capture (an empty file name means no
     * file is written). openVideoCapture() should be called first.
     */
    virtual void setVideoCaptureFile(const std::string& fileName_);
    /*!
     * Destroy video capture object, freeing all allocated memory and closing
     * the output file.
     */
    virtual void closeVideoCapture();
    // -------------------------- DISK AND FILE I/O ---------------------------
    /*!
     * Load disk image for drive 'n' (counting from zero; 0 to 3 are floppy
     * drives, 4 to 7 are IDE drives, and 8 is SD card); an empty file name
     * means no disk.
     */
    virtual void setDiskImageFile(int n, const std::string& fileName_,
                                  int nTracks_ = -1, int nSides_ = 2,
                                  int nSectorsPerTrack_ = 9);
    /*!
     * Returns the current state of the disk drive LEDs, which is the sum
     * of any of the following values:
     *   0x00000001: floppy drive 0 red LED is on
     *   0x00000002: floppy drive 0 green LED is on
     *   0x00000004: IDE drive 0 red LED is on (low priority)
     *   0x0000000C: IDE drive 0 red LED is on (high priority)
     *   0x00000100: floppy drive 1 red LED is on
     *   0x00000200: floppy drive 1 green LED is on
     *   0x00000400: IDE drive 1 red LED is on (low priority)
     *   0x00000C00: IDE drive 1 red LED is on (high priority)
     *   0x00010000: floppy drive 2 red LED is on
     *   0x00020000: floppy drive 2 green LED is on
     *   0x00040000: IDE drive 2 red LED is on (low priority)
     *   0x000C0000: IDE drive 2 red LED is on (high priority)
     *   0x00100000: SD card 1 blue LED is on (low priority)
     *   0x00200000: SD card 1 blue LED is on (high priority)
     *   0x00300000: SD card 1 cyan LED is on (high priority)
     *   0x01000000: floppy drive 3 red LED is on
     *   0x02000000: floppy drive 3 green LED is on
     *   0x04000000: IDE drive 3 red LED is on (low priority)
     *   0x0C000000: IDE drive 3 red LED is on (high priority)
     *   0x10000000: SD card 2 blue LED is on (low priority)
     *   0x20000000: SD card 2 blue LED is on (high priority)
     *   0x30000000: SD card 2 cyan LED is on (high priority)
     */
    virtual uint32_t getFloppyDriveLEDState();
    // ---------------------------- TAPE EMULATION ----------------------------
    /*!
     * Set tape image file name (if the file name is NULL or empty, tape
     * emulation is disabled).
     */
    virtual void setTapeFileName(const std::string& fileName);
    /*!
     * Start tape playback.
     */
    virtual void tapePlay();
    /*!
     * Start tape recording; if the tape file is read-only, this is
     * equivalent to calling tapePlay().
     */
    virtual void tapeRecord();
    /*!
     * Stop tape playback and recording.
     */
    virtual void tapeStop();
    /*!
     * Set tape position to the specified time (in seconds).
     */
    virtual void tapeSeek(double t);
    // ------------------------------ DEBUGGING -------------------------------
    /*!
     * Add or delete a single breakpoint (see also bplist.hpp).
     */
    virtual void setBreakPoint(const Ep128Emu::BreakPoint& bp,
                               bool isEnabled = true);
    /*!
     * Clear all breakpoints.
     */
    virtual void clearBreakPoints();
    /*!
     * Set breakpoint priority threshold (0 to 4); breakpoints with a
     * priority less than this value will not trigger a break.
     */
    virtual void setBreakPointPriorityThreshold(int n);
    /*!
     * Set if the breakpoint callback should be called whenever the first byte
     * of a CPU instruction is read from memory. 'mode_' should be one of the
     * following values:
     *   0: normal mode
     *   1: single step mode (break on every instruction, ignore breakpoints)
     *   2: step over mode
     *   3: trace (similar to mode 1, but does not ignore breakpoints)
     *   4: step into mode
     */
    virtual void setSingleStepMode(int mode_);
    /*!
     * Set the next address where single step mode will stop, ignoring any
     * other instructions. If 'addr' is negative, then a break is triggered
     * immediately at the next instruction.
     * Note: setSingleStepMode() must be called first with a mode parameter
     * of 2 or 4.
     */
    virtual void setSingleStepModeNextAddress(int32_t addr);
    /*!
     * Returns the segment at page 'n' (0 to 3).
     */
    virtual uint8_t getMemoryPage(int n) const;
    /*!
     * Read a byte from memory. If 'isCPUAddress' is false, bits 14 to 21 of
     * 'addr' define the segment number, while bits 0 to 13 are the offset
     * (0 to 0x3FFF) within the segment; otherwise, 'addr' is interpreted as
     * a 16-bit CPU address.
     */
    virtual uint8_t readMemory(uint32_t addr, bool isCPUAddress = false) const;
    /*!
     * Write a byte to memory. If 'isCPUAddress' is false, bits 14 to 21 of
     * 'addr' define the segment number, while bits 0 to 13 are the offset
     * (0 to 0x3FFF) within the segment; otherwise, 'addr' is interpreted as
     * a 16-bit CPU address.
     * NOTE: calling this function will stop any demo recording or playback.
     */
    virtual void writeMemory(uint32_t addr, uint8_t value,
                             bool isCPUAddress = false);
    /*!
     * Write a byte to any memory (RAM or ROM). Bits 14 to 21 of 'addr' define
     * the segment number, while bits 0 to 13 are the offset (0 to 0x3FFF)
     * within the segment.
     * NOTE: calling this function will stop any demo recording or playback.
     */
    virtual void writeROM(uint32_t addr, uint8_t value);
    /*!
     * Read a byte from I/O port 'addr'.
     */
    virtual uint8_t readIOPort(uint16_t addr) const;
    /*!
     * Write a byte to I/O port 'addr'.
     * NOTE: calling this function will stop any demo recording or playback.
     */
    virtual void writeIOPort(uint16_t addr, uint8_t value);
    /*!
     * Returns the current value of the Z80 program counter (PC).
     */
    virtual uint16_t getProgramCounter() const;
    /*!
     * Set the CPU program counter (PC) to a new address. The change will only
     * take effect after the completion of one instruction.
     * NOTE: calling this function may stop any demo recording or playback.
     */
    virtual void setProgramCounter(uint16_t addr);
    /*!
     * Returns the CPU address of the last byte pushed to the stack.
     */
    virtual uint16_t getStackPointer() const;
    /*!
     * Dumps the current values of all CPU registers to 'buf' in ASCII format.
     * The register list may be written as multiple lines separated by '\n'
     * characters, however, there is no newline character at the end of the
     * buffer. The maximum line width is 56 characters.
     */
    virtual void listCPURegisters(std::string& buf) const;
    /*!
     * Dumps the current values of all I/O registers to 'buf' in ASCII format.
     * The register list may be written as multiple lines separated by '\n'
     * characters, however, there is no newline character at the end of the
     * buffer. The maximum line width is 40 characters.
     */
    virtual void listIORegisters(std::string& buf) const;
    /*!
     * Disassemble one Z80 instruction, starting from memory address 'addr',
     * and write the result to 'buf' (not including a newline character).
     * 'offs' is added to the instruction address that is printed.
     * The maximum line width is 40 characters.
     * Returns the address of the next instruction. If 'isCPUAddress' is
     * true, 'addr' is interpreted as a 16-bit CPU address, otherwise it
     * is assumed to be a 22-bit physical address (8 bit segment + 14 bit
     * offset).
     */
    virtual uint32_t disassembleInstruction(std::string& buf, uint32_t addr,
                                            bool isCPUAddress = false,
                                            int32_t offs = 0) const;
    /*!
     * Returns the current horizontal (0 to 455) and vertical (0 to 311)
     * video position.
     */
    virtual void getVideoPosition(int& xPos, int& yPos) const;
    /*!
     * Returns a reference to a structure containing all Z80 registers;
     * see z80/z80.hpp for more information.
     * NOTE: getting a non-const reference will stop any demo recording or
     * playback.
     */
    Ep128::Z80_REGISTERS& getZ80Registers();
    inline const Ep128::Z80_REGISTERS& getZ80Registers() const
    {
      return z80.getReg();
    }
    // ------------------------------- FILE I/O -------------------------------
    /*!
     * Save snapshot of virtual machine state, including all ROM and RAM
     * segments, as well as all hardware registers. Note that the clock
     * frequency and timing settings, tape and disk state, and breakpoint list
     * are not saved.
     */
    virtual void saveState(Ep128Emu::File&);
    /*!
     * Save clock frequency and timing settings.
     */
    virtual void saveMachineConfiguration(Ep128Emu::File&);
    /*!
     * Register all types of file data supported by this class, for use by
     * Ep128Emu::File::processAllChunks(). Note that loading snapshot data
     * will clear all breakpoints.
     */
    virtual void registerChunkTypes(Ep128Emu::File&);
    /*!
     * Start recording a demo to the file object, which will be used until
     * the recording is stopped for some reason.
     * Implies calling saveMachineConfiguration() and saveState() first.
     */
    virtual void recordDemo(Ep128Emu::File&);
    /*!
     * Stop playing or recording demo.
     */
    virtual void stopDemo();
    /*!
     * Returns true if a demo is currently being recorded. The recording stops
     * when stopDemo() is called, any tape or disk I/O is attempted, clock
     * frequency and timing settings are changed, or a snapshot is loaded.
     * This function will also flush demo data to the associated file object
     * after recording is stopped for some reason other than calling
     * stopDemo().
     */
    virtual bool getIsRecordingDemo();
    /*!
     * Returns true if a demo is currently being played. The playback stops
     * when the end of the demo is reached, stopDemo() is called, any tape or
     * disk I/O is attempted, clock frequency and timing settings are changed,
     * or a snapshot is loaded. Note that keyboard events are ignored while
     * playing a demo.
     */
    virtual bool getIsPlayingDemo() const;
    // ----------------
    virtual void loadState(Ep128Emu::File::Buffer&);
    virtual void loadMachineConfiguration(Ep128Emu::File::Buffer&);
    virtual void loadDemo(Ep128Emu::File::Buffer&);
  };

}       // namespace TVC64

#endif  // EP128EMU_TVC64VM_HPP

