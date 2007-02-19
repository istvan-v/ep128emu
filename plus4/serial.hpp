
// plus4 -- portable Commodore Plus/4 emulator
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

#ifndef EP128EMU_PLUS4_SERIAL_HPP
#define EP128EMU_PLUS4_SERIAL_HPP

#include "ep128emu.hpp"

namespace Plus4 {

  class SerialBus {
   private:
    uint16_t  clkStateMask;
    uint16_t  dataStateMask;
    uint8_t   clkState;
    uint8_t   dataState;
    uint8_t   atnState;
   public:
    SerialBus()
      : clkStateMask(0xFFFF),
        dataStateMask(0xFFFF),
        clkState(0xFF),
        dataState(0xFF),
        atnState(0xFF)
    {
    }
    // returns the current state of the CLK line (0: low, 0xFF: high)
    inline uint8_t getCLK() const
    {
      return clkState;
    }
    // returns the current state of the DATA line (0: low, 0xFF: high)
    inline uint8_t getDATA() const
    {
      return dataState;
    }
    // returns the current state of the ATN line (0: low, 0xFF: high)
    inline uint8_t getATN() const
    {
      return atnState;
    }
    // set the CLK output (false: low, true: high) for device 'n' (0 to 15)
    inline void setCLK(int n, bool newState)
    {
      uint16_t  mask_ = uint16_t(1) << n;
      if (newState) {
        clkStateMask |= mask_;
        clkState = uint8_t(clkStateMask == 0xFFFF ? 0xFF : 0x00);
      }
      else {
        clkStateMask &= (mask_ ^ uint16_t(0xFFFF));
        clkState = uint8_t(0x00);
      }
    }
    // set the DATA output (false: low, true: high) for device 'n' (0 to 15)
    inline void setDATA(int n, bool newState)
    {
      uint16_t  mask_ = uint16_t(1) << n;
      if (newState) {
        dataStateMask |= mask_;
        dataState = uint8_t(dataStateMask == 0xFFFF ? 0xFF : 0x00);
      }
      else {
        dataStateMask &= (mask_ ^ uint16_t(0xFFFF));
        dataState = uint8_t(0x00);
      }
    }
    // set the state of the ATN line (false: low, true: high)
    inline void setATN(bool newState)
    {
      atnState = uint8_t(newState ? 0xFF : 0x00);
    }
    // remove device 'n' (0 to 15) from the bus, setting its outputs to high
    inline void removeDevice(int n)
    {
      uint16_t  mask_ = uint16_t(1) << n;
      clkStateMask |= mask_;
      dataStateMask |= mask_;
      clkState = uint8_t(clkStateMask == 0xFFFF ? 0xFF : 0x00);
      dataState = uint8_t(dataStateMask == 0xFFFF ? 0xFF : 0x00);
    }
    // remove devices defined by 'mask_' (bit N of 'mask_' corresponds to
    // device N) from the bus
    inline void removeDevices(uint16_t mask_)
    {
      clkStateMask |= mask_;
      dataStateMask |= mask_;
      clkState = uint8_t(clkStateMask == 0xFFFF ? 0xFF : 0x00);
      dataState = uint8_t(dataStateMask == 0xFFFF ? 0xFF : 0x00);
    }
  };

}       // namespace Plus4

#endif  // EP128EMU_PLUS4_SERIAL_HPP

