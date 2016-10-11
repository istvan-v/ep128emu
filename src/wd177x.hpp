
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

#ifndef EP128EMU_WD177X_HPP
#define EP128EMU_WD177X_HPP

#include "ep128emu.hpp"
#include "ep_fdd.hpp"

namespace Ep128Emu {

  class WD177x {
   private:
    FloppyDrive *floppyDrive;
    uint8_t     commandRegister;
    uint8_t     statusRegister;
    uint8_t     trackRegister;
    uint8_t     sectorRegister;
    uint8_t     dataRegister;
    bool        interruptRequestFlag;
    bool        dataRequestFlag;
    bool        isWD1773;
    bool        steppingIn;
    bool        busyFlagHackEnabled;
    bool        busyFlagHack;
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
    FloppyDrive dummyFloppyDrive;
    // ----------------
    void setError(uint8_t n = 0);
    // n > 0: step in, n < 0: step out
    void doStep(int n, bool updateFlag);
    static uint16_t calculateCRC(const uint8_t *buf, size_t nBytes,
                                 uint16_t n = 0xFFFF);
   public:
    WD177x();
    virtual ~WD177x();
    void setFloppyDrive(FloppyDrive *d);
    inline FloppyDrive& getFloppyDrive()
    {
      return (*floppyDrive);
    }
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
    virtual void setIsWD1773(bool isEnabled);
    inline bool getInterruptRequestFlag() const
    {
      return interruptRequestFlag;
    }
    inline bool getDataRequestFlag() const
    {
      return dataRequestFlag;
    }
    void setEnableBusyFlagHack(bool isEnabled);
    virtual void reset(bool isColdReset);
   protected:
    virtual void interruptRequest();
    virtual void clearInterruptRequest();
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_WD177X_HPP

