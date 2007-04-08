
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

#ifndef EP128EMU_IOPORTS_HPP
#define EP128EMU_IOPORTS_HPP

#include "ep128emu.hpp"
#include "bplist.hpp"

namespace Ep128 {

  class IOPorts {
   private:
    struct ReadCallback {
      uint8_t   (*func)(void *userData, uint16_t addr);
      void      *userData_;
      uint16_t  addr_;
    };
    struct WriteCallback {
      void      (*func)(void *userData, uint16_t addr, uint8_t value);
      void      *userData_;
      uint16_t  addr_;
    };
    uint8_t *portValues;
    ReadCallback *readCallbacks;
    WriteCallback *writeCallbacks;
    ReadCallback *debugReadCallbacks;
    uint8_t *breakPointTable;
    size_t breakPointCnt;
    uint8_t breakPointPriorityThreshold;
   public:
    IOPorts();
    virtual ~IOPorts();
    void setBreakPoint(uint16_t addr, int priority, bool r, bool w);
    void clearBreakPoints();
    void setBreakPointPriorityThreshold(int n);
    int getBreakPointPriorityThreshold();
    inline uint8_t read(uint16_t addr);
    inline void write(uint16_t addr, uint8_t value);
    uint8_t readDebug(uint16_t addr) const;
    void setReadCallback(uint16_t firstAddr, uint16_t lastAddr,
                         uint8_t (*func)(void *p, uint16_t addr),
                         void *userData, uint16_t baseAddr);
    void setDebugReadCallback(uint16_t firstAddr, uint16_t lastAddr,
                              uint8_t (*func)(void *p, uint16_t addr),
                              void *userData, uint16_t baseAddr);
    void setWriteCallback(uint16_t firstAddr, uint16_t lastAddr,
                          void (*func)(void *p, uint16_t addr, uint8_t value),
                          void *userData, uint16_t baseAddr);
    Ep128Emu::BreakPointList getBreakPointList();
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
   protected:
    virtual void breakPointCallback(bool isWrite, uint16_t addr, uint8_t value);
  };

  // --------------------------------------------------------------------------

  inline uint8_t IOPorts::read(uint16_t addr)
  {
    uint8_t       offs = (uint8_t) addr & 0xFF, value;
    ReadCallback& cb = readCallbacks[offs];

    value = cb.func(cb.userData_, cb.addr_);
    if (breakPointTable) {
      if (breakPointTable[offs] >= breakPointPriorityThreshold &&
          (breakPointTable[offs] & 1) != 0)
        breakPointCallback(false, addr, value);
    }
    return value;
  }

  inline void IOPorts::write(uint16_t addr, uint8_t value)
  {
    uint8_t         offs = (uint8_t) addr & 0xFF;
    WriteCallback&  cb = writeCallbacks[offs];

    if (breakPointTable) {
      if (breakPointTable[offs] >= breakPointPriorityThreshold &&
          (breakPointTable[offs] & 2) != 0)
        breakPointCallback(true, addr, value);
    }
    portValues[offs] = value;
    cb.func(cb.userData_, cb.addr_, value);
  }

}

#endif  // EP128EMU_IOPORTS_HPP

