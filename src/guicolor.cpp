
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

#include "ep128emu.hpp"
#include "guicolor.hpp"

#include <cmath>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>

#ifdef WIN32

#include <windows.h>
#include <FL/x.H>

static const char *windowClassTable[14] = {
  "EPMainWindow",
  (char *) 0,
  (char *) 0,
  (char *) 0,
  (char *) 0,
  (char *) 0,
  (char *) 0,
  (char *) 0,
  (char *) 0,
  (char *) 0,
  "EPInformation",
  "EPQuestion",
  "EPWarning",
  "EPError"
};

#endif  // WIN32

static const unsigned char colorTable[24] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x08, 0x0C, 0x10, 0x18, 0x20, 0x28, 0x30,
  0x3C, 0x48, 0x60, 0x7C, 0xA0, 0xC0, 0xE0, 0xFF
};

namespace Ep128Emu {

  void setGUIColorScheme(int colorScheme)
  {
    double  rgamma = 1.0;
    double  ggamma = 1.0;
    double  bgamma = 1.0;
    switch (colorScheme) {
    case 1:
      {
        Fl::scheme("none");
        Fl::set_color(FL_FOREGROUND_COLOR, 0, 0, 0);
        Fl::set_color(FL_BACKGROUND2_COLOR, 255, 255, 255);
        Fl::set_color(FL_INACTIVE_COLOR, 150, 148, 144);
        Fl::set_color(FL_SELECTION_COLOR, 0, 0, 128);
        Fl::set_color(Fl_Color(1), 255, 0, 0);
        Fl::set_color(Fl_Color(2), 0, 0, 0);
        Fl::set_color(Fl_Color(3), 0, 224, 0);
        Fl::set_color(Fl_Color(6), 191, 255, 255);
        rgamma = std::log(212.0 / 255.0) / std::log(16.0 / 23.0);
        ggamma = std::log(208.0 / 255.0) / std::log(16.0 / 23.0);
        bgamma = std::log(200.0 / 255.0) / std::log(16.0 / 23.0);
      }
      break;
    case 2:
      {
        Fl::scheme("plastic");
        Fl::set_color(FL_FOREGROUND_COLOR, 0, 0, 0);
        Fl::set_color(FL_BACKGROUND2_COLOR, 255, 255, 255);
        Fl::set_color(FL_INACTIVE_COLOR, 148, 148, 148);
        Fl::set_color(FL_SELECTION_COLOR, 0, 0, 128);
        Fl::set_color(Fl_Color(1), 192, 0, 0);
        Fl::set_color(Fl_Color(2), 0, 0, 0);
        Fl::set_color(Fl_Color(3), 255, 0, 0);
        Fl::set_color(Fl_Color(6), 0, 0, 0);
        rgamma = std::log(208.0 / 255.0) / std::log(16.0 / 23.0);
        ggamma = std::log(208.0 / 255.0) / std::log(16.0 / 23.0);
        bgamma = std::log(208.0 / 255.0) / std::log(16.0 / 23.0);
      }
      break;
    case 3:
      {
        Fl::scheme("gtk+");
        Fl::set_color(FL_FOREGROUND_COLOR, 0, 0, 0);
        Fl::set_color(FL_BACKGROUND2_COLOR, 255, 255, 255);
        Fl::set_color(FL_INACTIVE_COLOR, 195, 194, 193);
        Fl::set_color(FL_SELECTION_COLOR, 169, 209, 255);
        Fl::set_color(Fl_Color(1), 255, 0, 0);
        Fl::set_color(Fl_Color(2), 0, 0, 0);
        Fl::set_color(Fl_Color(3), 0, 224, 255);
        Fl::set_color(Fl_Color(6), 191, 255, 255);
        rgamma = std::log(235.0 / 255.0) / std::log(17.0 / 23.0);
        ggamma = std::log(233.0 / 255.0) / std::log(17.0 / 23.0);
        bgamma = std::log(232.0 / 255.0) / std::log(17.0 / 23.0);
      }
      break;
    default:
      Fl::scheme("none");
      Fl::set_color(FL_FOREGROUND_COLOR, 224, 224, 224);
      Fl::set_color(FL_BACKGROUND2_COLOR, 0, 0, 0);
      Fl::set_color(FL_INACTIVE_COLOR, 184, 184, 184);
      Fl::set_color(FL_SELECTION_COLOR, 127, 218, 255);
      Fl::set_color(Fl_Color(1), 255, 0, 0);
      Fl::set_color(Fl_Color(2), 128, 255, 128);
      Fl::set_color(Fl_Color(3), 255, 255, 0);
      Fl::set_color(Fl_Color(6), 191, 255, 255);
      for (int i = 0; i < 24; i++) {
        unsigned char c = colorTable[i];
        Fl::set_color(Fl_Color(int(FL_GRAY_RAMP) + i), c, c, c);
      }
      return;
    }
    for (int i = 0; i < 24; i++) {
      int     r = int(std::pow(double(i) / 23.0, rgamma) * 255.0 + 0.5);
      int     g = int(std::pow(double(i) / 23.0, ggamma) * 255.0 + 0.5);
      int     b = int(std::pow(double(i) / 23.0, bgamma) * 255.0 + 0.5);
      Fl::set_color(Fl_Color(int(FL_GRAY_RAMP) + i),
                    (unsigned char) r, (unsigned char) g, (unsigned char) b);
    }
  }

  void setWindowIcon(Fl_Window *w, int iconNum)
  {
#ifdef WIN32
    if (!((iconNum >= 0 && iconNum <= 2) || (iconNum >= 10 && iconNum <= 13)))
      return;                   // ignore invalid icon numbers
    // FIXME: according to the FLTK documentation, Fl_Window::show(int, char**)
    // should be used for the icon to be displayed, but Fl_Window::show() seems
    // to work anyway on Windows
    w->xclass(windowClassTable[iconNum]);
    HANDLE  iconHandle = (HANDLE) 0;
    INT     xSize = GetSystemMetrics(SM_CXICON);
    INT     ySize = GetSystemMetrics(SM_CYICON);
    UINT    imageFlags = LR_DEFAULTCOLOR | LR_SHARED;
    if (iconNum < 10) {
      // ep128emu specific icons
      iconHandle = LoadImage(fl_display, MAKEINTRESOURCE(iconNum + 101),
                             IMAGE_ICON, xSize, ySize, imageFlags);
    }
    else {
      // Windows system icons
      switch (iconNum) {
      case 10:
        iconHandle = LoadImage((HINSTANCE) 0, IDI_INFORMATION,
                               IMAGE_ICON, xSize, ySize, imageFlags);
        break;
      case 11:
        iconHandle = LoadImage((HINSTANCE) 0, IDI_QUESTION,
                               IMAGE_ICON, xSize, ySize, imageFlags);
        break;
      case 12:
        iconHandle = LoadImage((HINSTANCE) 0, IDI_WARNING,
                               IMAGE_ICON, xSize, ySize, imageFlags);
        break;
      case 13:
        iconHandle = LoadImage((HINSTANCE) 0, IDI_ERROR,
                               IMAGE_ICON, xSize, ySize, imageFlags);
        break;
      }
    }
    w->icon(reinterpret_cast<char *>(iconHandle));
#else
    // TODO: implement window icons for non-Windows platforms
    (void) w;
    (void) iconNum;
#endif
  }

}       // namespace Ep128Emu

