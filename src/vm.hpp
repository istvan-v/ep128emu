
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2009 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "bplist.hpp"
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
    int             audioOutputEQMode;
    float           audioOutputEQFrequency;
    float           audioOutputEQLevel;
    float           audioOutputEQ_Q;
    bool            tapePlaybackOn;
    bool            tapeRecordOn;
    bool            tapeMotorOn;
    Tape            *tape;
    std::string     tapeFileName;
    long            defaultTapeSampleRate;
    int             tapeSoundFileChannel;
    bool            tapeEnableSoundFileFilter;
    float           tapeSoundFileFilterMinFreq;
    float           tapeSoundFileFilterMaxFreq;
   protected:
    void            (*breakPointCallback)(void *userData, int type,
                                          uint16_t addr, uint8_t value);
    void            *breakPointCallbackUserData;
    bool            fileIOEnabled;
   private:
    std::string     fileIOWorkingDirectory;
    void            (*fileNameCallback)(void *userData, std::string& fileName);
    void            *fileNameCallbackUserData;
   public:
    struct VMStatus {
      bool      isRecordingDemo;
      bool      isPlayingDemo;
      bool      tapeReadOnly;
      double    tapePosition;
      double    tapeLength;
      long      tapeSampleRate;
      int       tapeSampleSize;
      // floppy drive LED state is the sum of any of the following values:
      //   0x00000001: drive 0 red LED is on
      //   0x00000002: drive 0 green LED is on
      //   0x00000100: drive 1 red LED is on
      //   0x00000200: drive 1 green LED is on
      //   0x00010000: drive 2 red LED is on
      //   0x00020000: drive 2 green LED is on
      //   0x01000000: drive 3 red LED is on
      //   0x02000000: drive 3 green LED is on
      uint32_t  floppyDriveLEDState;
    };
    // --------
    VirtualMachine(VideoDisplay& display_, AudioOutput& audioOutput_);
    virtual ~VirtualMachine();
    /*!
     * Returns reference to the associated video display object.
     */
    inline VideoDisplay& getVideoDisplay()
    {
      return this->display;
    }
    /*!
     * Returns reference to the associated audio output object.
     */
    inline AudioOutput& getAudioOutput()
    {
      return this->audioOutput;
    }
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
    /*!
     * Load epmemcfg format memory configuration file.
     */
    virtual void loadMemoryConfiguration(const std::string& fileName_);
    /*!
     * Set audio output quality.
     */
    virtual void setAudioOutputHighQuality(bool useHighQualityResample);
    /*!
     * Set cutoff frequencies of highpass filters used on audio output to
     * remove DC offset.
     */
    virtual void setAudioOutputFilters(float dcBlockFreq1_,
                                       float dcBlockFreq2_);
    /*!
     * Set parameters of audio output equalizer.
     * 'mode_' can be one of the following values: -1: disable equalizer,
     * 0: peaking EQ, 1: low shelf, 2: high shelf.
     */
    virtual void setAudioOutputEqualizer(int mode_,
                                         float freq_, float level_, float q_);
    /*!
     * Set amplitude scale for audio output (defaults to 0.7071).
     */
    virtual void setAudioOutputVolume(float ampScale_);
    /*!
     * Set if audio data is sent to sound card and output file (disabling
     * this also makes the emulation run faster than real time).
     */
    virtual void setEnableAudioOutput(bool isEnabled);
    /*!
     * Set if video data is sent to the associated VideoDisplay object.
     */
    virtual void setEnableDisplay(bool isEnabled);
    /*!
     * Set CPU clock frequency (in Hz).
     */
    virtual void setCPUFrequency(size_t freq_);
    /*!
     * Set the number of video 'slots' per second.
     */
    virtual void setVideoFrequency(size_t freq_);
    /*!
     * Set internal sample rate of sound emulation.
     */
    virtual void setSoundClockFrequency(size_t freq_);
    /*!
     * Set if emulation of memory timing is enabled.
     */
    virtual void setEnableMemoryTimingEmulation(bool isEnabled);
    /*!
     * Set state of key 'keyCode' (0 to 127).
     */
    virtual void setKeyboardState(int keyCode, bool isPressed);
    /*!
     * Returns status information about the emulated machine (see also
     * struct VMStatus above, and the comments for functions that return
     * individual status values).
     */
    virtual void getVMStatus(VMStatus& vmStatus_);
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
     * Load disk image for drive 'n' (counting from zero); an empty file
     * name means no disk.
     */
    virtual void setDiskImageFile(int n, const std::string& fileName_,
                                  int nTracks_ = -1, int nSides_ = 2,
                                  int nSectorsPerTrack_ = 9);
    /*!
     * Returns the current state of the floppy drive LEDs, which is the sum
     * of any of the following values:
     *   0x00000001: drive 0 red LED is on
     *   0x00000002: drive 0 green LED is on
     *   0x00000100: drive 1 red LED is on
     *   0x00000200: drive 1 green LED is on
     *   0x00010000: drive 2 red LED is on
     *   0x00020000: drive 2 green LED is on
     *   0x01000000: drive 3 red LED is on
     *   0x02000000: drive 3 green LED is on
     */
    virtual uint32_t getFloppyDriveLEDState();
    /*!
     * Set if the emulated machine should be allowed to access files in the
     * working directory.
     */
    virtual void setEnableFileIO(bool isEnabled);
    /*!
     * Set directory for files to be saved and loaded by the emulated machine.
     */
    virtual void setWorkingDirectory(const std::string& dirName_);
    /*!
     * Set function to be called when the emulated machine tries to open a
     * file with unspecified name. 'fileName' should be set to the name of the
     * file to be opened.
     */
    virtual void setFileNameCallback(void (*fileNameCallback_)(
                                         void *userData,
                                         std::string& fileName),
                                     void *userData_);
    // ---------------------------- TAPE EMULATION ----------------------------
    /*!
     * Set tape image file name (if the file name is NULL or empty, tape
     * emulation is disabled).
     */
    virtual void setTapeFileName(const std::string& fileName);
    /*!
     * Set sample rate (in Hz) to be used when creating a new tape image file.
     * If the file already exists, this setting is ignored, and the value
     * stored in the file header is used instead.
     */
    virtual void setDefaultTapeSampleRate(long sampleRate_ = 24000L);
    /*!
     * Returns the actual sample rate of the tape file, or zero if there is no
     * tape image file opened.
     */
    virtual long getTapeSampleRate() const;
    /*!
     * Returns the number of bits per sample in the tape file, or zero if there
     * is no tape image file opened.
     */
    virtual int getTapeSampleSize() const;
    /*!
     * Returns true if the tape is opened in read-only mode.
     */
    virtual bool getIsTapeReadOnly() const;
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
    /*!
     * Returns the current tape position in seconds, or -1.0 if there is
     * no tape image file opened.
     */
    virtual double getTapePosition() const;
    /*!
     * Returns the current length of the tape file in seconds, or -1.0 if
     * there is no tape image file opened.
     */
    virtual double getTapeLength() const;
    /*!
     * Seek forward (if isForward = true) or backward (if isForward = false)
     * to the nearest cue point, or by 't' seconds if no cue point is found.
     */
    virtual void tapeSeekToCuePoint(bool isForward = true, double t = 10.0);
    /*!
     * Create a new cue point at the current tape position.
     * Has no effect if the file does not have a cue point table, or it
     * is read-only.
     */
    virtual void tapeAddCuePoint();
    /*!
     * Delete the cue point nearest to the current tape position.
     * Has no effect if the file is read-only.
     */
    virtual void tapeDeleteNearestCuePoint();
    /*!
     * Delete all cue points. Has no effect if the file is read-only.
     */
    virtual void tapeDeleteAllCuePoints();
    /*!
     * Set parameters for tape sound file I/O.
     */
    virtual void setTapeSoundFileParameters(int requestedChannel_,
                                            bool enableFilter_,
                                            float filterMinFreq_,
                                            float filterMaxFreq_);
    // ------------------------------ DEBUGGING -------------------------------
    /*!
     * Add breakpoints from the specified breakpoint list (see also
     * bplist.hpp).
     */
    virtual void setBreakPoints(const BreakPointList& bpList);
    /*!
     * Returns the currently defined breakpoints.
     */
    virtual BreakPointList getBreakPoints();
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
     * Set function to be called when a breakpoint is triggered.
     * 'type' can be one of the following values:
     *   0: breakpoint at opcode read
     *   1: memory read
     *   2: memory write
     *   3: opcode read in single step mode
     *   5: I/O port read
     *   6: I/O port write
     */
    virtual void setBreakPointCallback(void (*breakPointCallback_)(
                                           void *userData, int type,
                                           uint16_t addr, uint8_t value),
                                       void *userData_);
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
     * Read a byte from I/O port 'addr'.
     */
    virtual uint8_t readIOPort(uint16_t addr) const;
    /*!
     * Write a byte to I/O port 'addr'.
     * NOTE: calling this function will stop any demo recording or playback.
     */
    virtual void writeIOPort(uint16_t addr, uint8_t value);
    /*!
     * Returns the current value of the CPU program counter (PC).
     */
    virtual uint16_t getProgramCounter() const;
    /*!
     * Set the CPU program counter (PC) to a new address. The change may only
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
     * Disassemble one CPU instruction, starting from memory address 'addr',
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
     * Open 'fileName' with openFileInWorkingDirectory(), and load it to the
     * memory area defined by 'startAddr' and 'endAddr', which may be 16-bit
     * CPU or 22-bit physical addresses, depending on 'cpuAddressMode'.
     * 'endAddr' is still loaded, and may be 0xFFFFFFFF to read all data from
     * the file.
     * The return value is the number of bytes read, which may be less than
     * 'endAddr' + 1 - 'startAddr' if the file is too short. On error, an
     * exception is thrown.
     * If 'verifyMode' is true, then memory is not written, and the number
     * of bytes that differ from the file data is returned.
     */
    virtual size_t loadMemory(const char *fileName, bool verifyMode,
                              bool asciiMode, bool cpuAddressMode,
                              uint32_t startAddr,
                              uint32_t endAddr = 0xFFFFFFFFU);
    /*!
     * Save the memory area defined by 'startAddr' and 'endAddr', which may be
     * 16-bit CPU or 22-bit physical addresses depending on 'cpuAddressMode',
     * to 'fileName'. The byte at 'endAddr' is still written to the file.
     * If 'asciiMode' is true, then the data is saved in a hexadecimal memory
     * dump format, which can be read by loadMemory().
     * On error, an exception is thrown.
     */
    virtual void saveMemory(const char *fileName,
                            bool asciiMode, bool cpuAddressMode,
                            uint32_t startAddr, uint32_t endAddr);
    // ------------------------------- FILE I/O -------------------------------
    /*!
     * Save snapshot of virtual machine state, including all ROM and RAM
     * segments, as well as all hardware registers. Note that the clock
     * frequency and timing settings, tape and disk state, and breakpoint list
     * are not saved.
     */
    virtual void saveState(File& f);
    /*!
     * Save clock frequency and timing settings.
     */
    virtual void saveMachineConfiguration(File& f);
    /*!
     * Register all types of file data supported by this class, for use by
     * File::processAllChunks(). Note that loading snapshot data will clear
     * all breakpoints.
     */
    virtual void registerChunkTypes(File& f);
    /*!
     * Start recording a demo to the file object, which will be used until
     * the recording is stopped for some reason.
     * Implies calling saveMachineConfiguration() and saveState() first.
     */
    virtual void recordDemo(File& f);
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
    inline void sendMonoAudioOutput(int32_t audioData)
    {
      if (this->writingAudioOutput)
        this->audioConverter->sendMonoInputSignal(audioData);
    }
    /*!
     * This function is similar to the public setTapeFileName(), but allows
     * derived classes to use a different sample size than the default of
     * 1 bit.
     */
    void setTapeFileName(const std::string& fileName, int bitsPerSample);
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
   public:
    /*!
     * Open a file in the user specified working directory. 'fileName_' is the
     * file name without any leading directory components; it is converted to
     * lower case, invalid characters are replaced with underscores, and the
     * file is searched case-insensitively. If 'fileName_' is empty, the file
     * name callback (if any) is called, which should return either a full path
     * file name that will be stored in 'fileName_', or an empty string in
     * which case this function fails and returns -2 (invalid file name).
     * 'mode' is the mode parameter to be passed to std::fopen().
     * On success, the file handle is stored in 'f', and zero is returned.
     * Otherwise, 'f' is set to NULL, and the return value is one of the
     * following error codes:
     *   -1: unknown error
     *   -2: invalid (empty) file name
     *   -3: the file is not found
     *   -4: the file is not a regular file
     *   -5: the file is found but cannot be opened (e.g. permission is
     *       denied), or a new file cannot be created; 'errno' is set
     *       according to the reason for the failure
     *   -6: the file already exists (if 'createOnly_' is true)
     */
    int openFileInWorkingDirectory(std::FILE*& f, std::string& fileName_,
                                   const char *mode, bool createOnly_ = false);
    /*!
     * Returns the error message for error code 'n' returned by
     * openFileInWorkingDirectory().
     */
    static const char * getFileOpenErrorMessage(int n);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_VM_HPP

