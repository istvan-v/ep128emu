
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

#ifndef EP128EMU_EP_FDD_HPP
#define EP128EMU_EP_FDD_HPP

#include "ep128emu.hpp"
#include <vector>

namespace Ep128Emu {

  /*!
   * Check if the disk image specified in 'fileName' (should not be NULL) is
   * a real floppy disk, and set (if zero or negative) or verify (if positive)
   * the geometry parameters.
   * Returns one of the following values:
   *   -2: the specified geometry does not match the actual disk parameters
   *   -1: error opening file or device
   *    0: the file is not a disk device
   *    1: no error, disk is write protected
   *    2: no error, disk is not write protected
   */
  extern int checkFloppyDisk(const char *fileName,
                             int& nTracks, int& nSides, int& nSectorsPerTrack);

  class FloppyDrive {
   private:
    static const uint32_t ledStateCount1 = 60U;         // 120 ms
    static const uint32_t ledStateCount2 = 500U;        // 1000 ms
    std::string imageFileName;
    std::FILE   *imageFile;
    uint8_t     nTracks;
    uint8_t     nSides;
    uint8_t     nSectorsPerTrack;
    uint8_t     currentTrack;
    uint8_t     currentSide;
    bool        writeProtectFlag;
    bool        diskChangeFlag;
    uint8_t     bufferedTrack;          // track stored in buffer, 0xFF: none
    uint8_t     bufferedSide;           // side stored in buffer, 0xFF: none
    bool        motorOnInput;
    bool        isMotorOn;
    uint32_t    ledStateCounter;
    std::vector< uint32_t > buf_;
    uint8_t     *trackBuffer;
    // for each sector of the buffered track:
    // bit 0: sector data is valid, bit 7: sector is "dirty" (not flushed)
    uint8_t     *flagsBuffer;
    uint8_t     *tmpBuffer;
    long        bufPos;                 // position in track buffer, -1: none
    bool        trackDirtyFlag;
    // ----------------
    void closeDiskImage();
    uint8_t getLEDState_();
   public:
    FloppyDrive();
    virtual ~FloppyDrive();
    virtual void setDiskImageFile(const std::string& fileName_,
                                  int nTracks_ = -1,
                                  int nSides_ = 2,
                                  int nSectorsPerTrack_ = 9);
    inline void setDiskChangeFlag(bool isChanged)
    {
      diskChangeFlag = isChanged;
    }
    inline bool getDiskChangeFlag() const
    {
      return diskChangeFlag;
    }
    inline void setSide(int n)
    {
      currentSide = uint8_t(n & 1);
    }
    inline bool haveDisk() const
    {
      return (imageFile != (std::FILE *) 0);
    }
    inline bool getIsWriteProtected() const
    {
      return writeProtectFlag;
    }
    inline void motorOn()
    {
      motorOnInput = true;
      isMotorOn = true;
      ledStateCounter = ledStateCount1;
    }
    inline void motorOff()
    {
      motorOnInput = false;
    }
    inline bool getIsMotorOn() const
    {
      return isMotorOn;
    }
    // returns 0: black (off), 1: red, 2: green, 3: yellow-green
    // should be called at a rate of 500 Hz
    inline uint8_t getLEDState()
    {
      if (ledStateCounter)
        return getLEDState_();
      return 0x00;
    }
    virtual void reset();
    inline uint8_t getSectorsPerTrack() const
    {
      return nSectorsPerTrack;
    }
    inline uint8_t getCurrentTrack() const
    {
      return currentTrack;
    }
    inline uint8_t getCurrentSide() const
    {
      return currentSide;
    }
    inline bool checkCurrentTrack(uint8_t trackNum) const
    {
      return (imageFile && currentTrack < nTracks && currentSide < nSides &&
              currentTrack == trackNum);
    }
    // n > 0: step in, n < 0: step out
    void doStep(int n);
    inline bool getIsTrack0() const
    {
      return (currentTrack == 0);
    }
    // returns true on success
    bool startDataTransfer(uint8_t trackNum, uint8_t sectorNum);
    inline void endDataTransfer()
    {
      bufPos = -1L;
    }
    inline bool isDataTransfer() const
    {
      return (bufPos >= 0L);
    }
    inline long getSectorBufferPosition() const
    {
      return (bufPos >= 0L ? (bufPos & 511L) : bufPos);
    }
    // returns true at end of sector
    inline bool readByte(uint8_t& b)
    {
      b = trackBuffer[bufPos];
      bufPos++;
      return (!(bufPos & 511L));
    }
    inline bool writeByte(uint8_t b)
    {
      trackDirtyFlag = true;
      trackBuffer[bufPos] = b;
      flagsBuffer[bufPos >> 9] = 0x81;
      bufPos++;
      return (!(bufPos & 511L));
    }
    // pad the rest of the current sector with zero bytes
    // called when a write command is aborted
    void padSector();
   private:
    void copySector(int n);
    void clearDirtyFlag();
    void clearFlagsBuffer();
    void clearBuffer(uint8_t *buf);
    bool updateBufferedTrack_();
    bool readTrack();
   public:
    bool flushTrack();
    inline bool updateBufferedTrack()
    {
      if (currentTrack != bufferedTrack || currentSide != bufferedSide)
        return updateBufferedTrack_();
      return true;
    }
    inline bool getSectorData()
    {
      if (!flagsBuffer[bufPos >> 9])
        return readTrack();
      return true;
    }
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_EP_FDD_HPP

