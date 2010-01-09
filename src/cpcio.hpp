
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

#ifndef EP128EMU_CPCIO_HPP
#define EP128EMU_CPCIO_HPP

#include "ep128emu.hpp"
#include "bplist.hpp"

namespace CPC464 {

  class IOPorts {
   private:
    void    *callbackUserData;
    uint8_t (*readCallback)(void *userData, uint16_t addr);
    void    (*writeCallback)(void *userData, uint16_t addr, uint8_t value);
    uint8_t (*debugReadCallback)(void *userData, uint16_t addr);
    uint8_t *breakPointTable;
    size_t  breakPointCnt;
    uint8_t breakPointPriorityThreshold;
   public:
    IOPorts();
    virtual ~IOPorts();
    void setBreakPoint(uint16_t addr, int priority, bool r, bool w);
    void clearBreakPoints();
    void setBreakPointPriorityThreshold(int n);
    int getBreakPointPriorityThreshold();
    Ep128Emu::BreakPointList getBreakPointList();
    inline uint8_t read(uint16_t addr);
    inline void write(uint16_t addr, uint8_t value);
    uint8_t readDebug(uint16_t addr) const;
    void writeDebug(uint16_t addr, uint8_t value);
    void setCallbackUserData(void *userData);
    void setReadCallback(uint8_t (*func)(void *userData, uint16_t addr));
    void setDebugReadCallback(uint8_t (*func)(void *userData, uint16_t addr));
    void setWriteCallback(void (*func)(void *userData,
                                       uint16_t addr, uint8_t value));
   protected:
    virtual void breakPointCallback(bool isWrite, uint16_t addr, uint8_t value);
  };

  // --------------------------------------------------------------------------

  inline uint8_t IOPorts::read(uint16_t addr)
  {
    uint8_t value = readCallback(callbackUserData, addr);
    if (breakPointTable) {
      uint8_t offs = uint8_t(addr >> 8);
      if (breakPointTable[offs] >= breakPointPriorityThreshold &&
          (breakPointTable[offs] & 1) != 0)
        breakPointCallback(false, addr, value);
    }
    return value;
  }

  inline void IOPorts::write(uint16_t addr, uint8_t value)
  {
    if (breakPointTable) {
      uint8_t offs = uint8_t(addr >> 8);
      if (breakPointTable[offs] >= breakPointPriorityThreshold &&
          (breakPointTable[offs] & 2) != 0)
        breakPointCallback(true, addr, value);
    }
    writeCallback(callbackUserData, addr, value);
  }

}       // namespace CPC464

#endif  // EP128EMU_CPCIO_HPP

