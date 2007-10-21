
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

#ifndef EP128EMU_WD177X_HPP
#define EP128EMU_WD177X_HPP

#include "ep128emu.hpp"

#include <cstdio>
#include <string>
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
    std::vector< uint8_t >  buf;
    size_t      bufPos;
    bool setFilePosition();
    void doStep(bool updateFlag);
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

