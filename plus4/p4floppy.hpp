
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

#ifndef EP128EMU_P4FLOPPY_HPP
#define EP128EMU_P4FLOPPY_HPP

#include "ep128emu.hpp"
#include "serial.hpp"

namespace Plus4 {

  class FloppyDrive {
   public:
    FloppyDrive(int driveNum_ = 8)
    {
      (void) driveNum_;
    }
    virtual ~FloppyDrive()
    {
    }
    // use 'romData_' (should point to 16384 bytes of data which is expected
    // to remain valid until either a new address is set or the object is
    // destroyed, or can be NULL for no ROM data) for ROM bank 'n'; allowed
    // values for 'n' are:
    //   0: 1581 low
    //   1: 1581 high
    //   2: 1541
    // if this drive type does not use the selected ROM bank, the function call
    // is ignored
    virtual void setROMImage(int n, const uint8_t *romData_) = 0;
    // open disk image file 'fileName_' (an empty file name means no disk)
    virtual void setDiskImageFile(const std::string& fileName_) = 0;
    // returns true if there is a disk image file opened
    virtual bool haveDisk() const = 0;
    // run floppy emulation for 't' microseconds
    virtual void run(SerialBus& serialBus_, size_t t = 1) = 0;
    // reset floppy drive
    virtual void reset() = 0;
  };

}       // namespace Plus4

#endif  // EP128EMU_P4FLOPPY_HPP

