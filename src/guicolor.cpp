
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

#include "ep128emu.hpp"
#include "guicolor.hpp"

#include <FL/Fl.H>

static const unsigned char colorTable[24] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x08, 0x0C, 0x10, 0x18, 0x20, 0x28, 0x30,
  0x3C, 0x48, 0x60, 0x80, 0xA0, 0xC0, 0xE0, 0xFF
};

namespace Ep128Emu {

  void setGUIColorScheme()
  {
    Fl::set_color(FL_FOREGROUND_COLOR, 224, 224, 224);
    Fl::set_color(FL_BACKGROUND2_COLOR, 0, 0, 0);
    Fl::set_color(FL_INACTIVE_COLOR, 184, 184, 184);
    Fl::set_color(FL_SELECTION_COLOR, 127, 218, 255);
    for (int i = 0; i < 24; i++) {
      unsigned char c = colorTable[i];
      Fl::set_color(Fl_Color(int(FL_GRAY_RAMP) + i), c, c, c);
    }
  }

}       // namespace Ep128Emu

