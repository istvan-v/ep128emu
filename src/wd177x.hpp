
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2008 Istvan Varga <istvanv@users.sourceforge.net>
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
    std::vector< uint8_t >  trackBuffer;
    size_t      bufPos;
    bool        trackNotReadFlag;
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
    bool readTrack();
    bool flushTrack();
    static uint16_t calculateCRC(const uint8_t *buf_, size_t nBytes,
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
    virtual bool getDiskChangeFlag() const;
    virtual void clearDiskChangeFlag();
    virtual void setIsWD1773(bool isEnabled);
    inline void setSide(int n)
    {
      currentSide = uint8_t(n) & 1;
    }
    virtual bool getInterruptRequestFlag() const;
    virtual bool getDataRequestFlag() const;
    virtual bool haveDisk() const;
    virtual bool getIsWriteProtected() const;
    void setEnableBusyFlagHack(bool isEnabled);
    inline bool getLEDState() const
    {
      return bool(statusRegister & 0x01);
    }
    virtual void reset();
   protected:
    virtual void interruptRequest();
    virtual void clearInterruptRequest();
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_WD177X_HPP

