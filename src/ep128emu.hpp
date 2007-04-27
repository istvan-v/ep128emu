
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2006 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EP128EMU_EP128EMU_HPP
#define EP128EMU_EP128EMU_HPP

#include <exception>
#include <new>
#include <string>
#include <iostream>

#if defined(HAVE_STDINT_H) || defined(__GNUC__)
#  include <stdint.h>
#else
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef short               int16_t;
typedef unsigned short      uint16_t;
typedef int                 int32_t;
typedef unsigned int        uint32_t;
#  ifndef _WIN32
typedef long long           int64_t;
typedef unsigned long long  uint64_t;
#  else
typedef __int64             int64_t;
typedef unsigned __int64    uint64_t;
#  endif
#endif

namespace Ep128Emu {

  class Exception : public std::exception {
   private:
    const char  *msg;
   public:
    Exception() throw()
      : std::exception()
    {
      msg = (char *) 0;
    }
    Exception(const char *msg_) throw()
      : std::exception()
    {
      msg = msg_;
    }
    Exception(const Exception& e) throw()
      : std::exception()
    {
      msg = e.msg;
    }
    virtual ~Exception() throw()
    {
    }
    void operator=(const Exception& e) throw()
    {
      msg = e.msg;
    }
    virtual const char * what() const throw()
    {
      return (msg == (char *) 0 ? "unknown error" : msg);
    }
  };

}       // namespace Ep128Emu

#include "fileio.hpp"

#endif  // EP128EMU_EP128EMU_HPP

