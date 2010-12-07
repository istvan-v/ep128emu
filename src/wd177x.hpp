
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2010 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EP128EMU_WD177X_HPP
#define EP128EMU_WD177X_HPP

#include "ep128emu.hpp"
#include <vector>

namespace Ep128Emu {

  class WD177x {
   private:
    static const uint32_t ledStateCount1 = 60U;         // 120 ms
    static const uint32_t ledStateCount2 = 500U;        // 1000 ms
    std::string imageFileName;
    std::FILE   *imageFile;
    uint8_t     nTracks;
    uint8_t     nSides;
    uint8_t     nSectorsPerTrack;
    uint8_t     commandRegister;
    uint8_t     statusRegister;
    uint8_t     trackRegister;
    uint8_t     sectorRegister;
    uint8_t     dataRegister;
    uint8_t     currentTrack;
    uint8_t     currentSide;
    bool        writeProtectFlag;
    bool        diskChangeFlag;
    bool        interruptRequestFlag;
    bool        dataRequestFlag;
    bool        isWD1773;
    bool        steppingIn;
    bool        busyFlagHackEnabled;
    bool        busyFlagHack;
    uint8_t     bufferedTrack;          // track stored in buffer, 0xFF: none
    uint8_t     bufferedSide;           // side stored in buffer, 0xFF: none
    uint32_t    ledStateCounter;
    std::vector< uint32_t > buf_;
    uint8_t     *trackBuffer;
    // for each sector of the buffered track:
    // bit 0: sector data is valid, bit 7: sector is "dirty" (not flushed)
    uint8_t     *flagsBuffer;
    uint8_t     *tmpBuffer;
    long        bufPos;                 // position in track buffer, -1: none
    bool        trackDirtyFlag;
    uint8_t     writeTrackSectorsRemaining;
    //  0: gap (60 * 0x4E)
    //  1: sync (12 * 0x00)
    //  2: index address mark (3 * 0xF6)
    //  3: 0xFC
    // ---- sector begin ----
    //  4: gap (60 * 0x4E)
    //  5: sync (12 * 0x00)
    //  6: sector ID address mark (3 * 0xF5)
    //  7: 0xFE
    //  8: track number (0..nTracks-1)
    //  9: side number (0..nSides-1)
    // 10: sector number (1..nSectorsPerTrack)
    // 11: sector length code = log2(sectorLength) - 7
    // 12: CRC (0xF7)
    // 13: gap (22 * 0x4E)
    // 14: sync (12 * 0x00)
    // 15: sector data address mark (3 * 0xF5)
    // 16: 0xFB: normal sector, 0xF8: deleted sector
    // 17: sector data (2 ^ (sectorLengthCode + 7) bytes)
    // 18: CRC (0xF7)
    // 19: gap (24 * 0x4E)
    // ---- sector end ----
    // 20: gap (520 * 0x4E)
    // +(0x20 * N): N bytes of data was received
    uint8_t     writeTrackState;
    // ----------------
    bool checkDiskPosition(bool ignoreSectorRegister = false);
    void setError(uint8_t n = 0);
    void doStep(bool updateFlag);
    void closeDiskImage();
    void copySector(int n);
    void clearDirtyFlag();
    void clearFlagsBuffer();
    void clearBuffer(uint8_t *buf);
    bool updateBufferedTrack();
    bool readTrack();
    bool flushTrack();
    uint8_t getLEDState_();
    static uint16_t calculateCRC(const uint8_t *buf, size_t nBytes,
                                 uint16_t n = 0xFFFF);
   public:
    WD177x();
    virtual ~WD177x();
    virtual void setDiskImageFile(const std::string& fileName_,
                                  int nTracks_ = -1,
                                  int nSides_ = 2,
                                  int nSectorsPerTrack_ = 9);
    virtual void writeCommandRegister(uint8_t n);
    virtual uint8_t readStatusRegister();
    virtual void writeTrackRegister(uint8_t n);
    virtual uint8_t readTrackRegister() const;
    virtual void writeSectorRegister(uint8_t n);
    virtual uint8_t readSectorRegister() const;
    virtual void writeDataRegister(uint8_t n);
    virtual uint8_t readDataRegister();
    virtual uint8_t readStatusRegisterDebug() const;
    virtual uint8_t readDataRegisterDebug() const;
    inline bool getDiskChangeFlag() const
    {
      return diskChangeFlag;
    }
    virtual void clearDiskChangeFlag();
    virtual void setIsWD1773(bool isEnabled);
    inline void setSide(int n)
    {
      currentSide = uint8_t(n & 1);
    }
    inline bool getInterruptRequestFlag() const
    {
      return interruptRequestFlag;
    }
    inline bool getDataRequestFlag() const
    {
      return dataRequestFlag;
    }
    inline bool haveDisk() const
    {
      return (imageFile != (std::FILE *) 0);
    }
    inline bool getIsWriteProtected() const
    {
      return writeProtectFlag;
    }
    void setEnableBusyFlagHack(bool isEnabled);
    // returns 0: black (off), 1: red, 2: green, 3: yellow-green
    // should be called at a rate of 500 Hz
    inline uint8_t getLEDState()
    {
      if (ledStateCounter)
        return getLEDState_();
      return 0x00;
    }
    virtual void reset();
   protected:
    virtual void interruptRequest();
    virtual void clearInterruptRequest();
  };

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

}       // namespace Ep128Emu

#endif  // EP128EMU_WD177X_HPP

