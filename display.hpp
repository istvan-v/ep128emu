
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

#ifndef EP128EMU_DISPLAY_HPP
#define EP128EMU_DISPLAY_HPP

#include "ep128.hpp"

namespace Ep128Emu {

  class VideoDisplay {
   public:
    class DisplayParameters {
     public:
      // 0: half horizontal resolution, no interlace (352x288),
      //    no texture filtering, no blend effects
      // 1: half horizontal resolution, no interlace (352x288)
      // 2: full horizontal resolution, no interlace (704x288)
      // 3: full horizontal resolution, interlace (704x576)
      int     displayQuality;
      // function to convert 8-bit color indices to red, green, and blue
      // levels (in the range 0.0 to 1.0); if NULL, greyscale is assumed
      void    (*indexToRGBFunc)(uint8_t color,
                                float& red, float& green, float& blue);
      // brightness (default: 0.0)
      double  b;
      // contrast (default: 1.0)
      double  c;
      // gamma (default: 1.0, higher values result in a brighter display)
      double  g;
      // brightness for red channel
      double  rb;
      // contrast for red channel
      double  rc;
      // gamma for red channel
      double  rg;
      // brightness for green channel
      double  gb;
      // contrast for green channel
      double  gc;
      // gamma for green channel
      double  gg;
      // brightness for blue channel
      double  bb;
      // contrast for blue channel
      double  bc;
      // gamma for blue channel
      double  bg;
      // controls vertical filtering of textures (0 to 0.5)
      double  blendScale1;
      // scale applied to new pixels written to frame buffer
      double  blendScale2;
      // scale applied to old pixels in frame buffer
      double  blendScale3;
      // pixel aspect ratio to assume
      // (calculated as (screen_width / screen_height) / (X_res / Y_res))
      double  pixelAspectRatio;
     private:
      static void defaultIndexToRGBFunc(uint8_t color,
                                        float& red, float& green, float& blue)
      {
        blue = green = red = (float(color) / 255.0f);
      }
      void copyDisplayParameters(const DisplayParameters& src)
      {
        displayQuality = (src.displayQuality > 0 ?
                          (src.displayQuality < 3 ? src.displayQuality : 3)
                          : 0);
        if (src.indexToRGBFunc)
          indexToRGBFunc = src.indexToRGBFunc;
        else
          indexToRGBFunc = &defaultIndexToRGBFunc;
        b = (src.b > -0.5 ? (src.b < 0.5 ? src.b : 0.5) : -0.5);
        c = (src.c > 0.5 ? (src.c < 2.0 ? src.c : 2.0) : 0.5);
        g = (src.g > 0.25 ? (src.g < 4.0 ? src.g : 4.0) : 0.25);
        rb = (src.rb > -0.5 ? (src.rb < 0.5 ? src.rb : 0.5) : -0.5);
        rc = (src.rc > 0.5 ? (src.rc < 2.0 ? src.rc : 2.0) : 0.5);
        rg = (src.rg > 0.25 ? (src.rg < 4.0 ? src.rg : 4.0) : 0.25);
        gb = (src.gb > -0.5 ? (src.gb < 0.5 ? src.gb : 0.5) : -0.5);
        gc = (src.gc > 0.5 ? (src.gc < 2.0 ? src.gc : 2.0) : 0.5);
        gg = (src.gg > 0.25 ? (src.gg < 4.0 ? src.gg : 4.0) : 0.25);
        bb = (src.bb > -0.5 ? (src.bb < 0.5 ? src.bb : 0.5) : -0.5);
        bc = (src.bc > 0.5 ? (src.bc < 2.0 ? src.bc : 2.0) : 0.5);
        bg = (src.bg > 0.25 ? (src.bg < 4.0 ? src.bg : 4.0) : 0.25);
        blendScale1 = (src.blendScale1 > 0.0 ?
                       (src.blendScale1 < 0.5 ? src.blendScale1 : 0.5) : 0.0);
        blendScale2 = (src.blendScale2 > 0.0 ?
                       (src.blendScale2 < 1.0 ? src.blendScale2 : 1.0) : 0.0);
        blendScale3 = (src.blendScale3 > 0.0 ?
                       (src.blendScale3 < 1.0 ? src.blendScale3 : 1.0) : 0.0);
        pixelAspectRatio = (src.pixelAspectRatio > 0.5 ?
                            (src.pixelAspectRatio < 2.0 ?
                             src.pixelAspectRatio : 2.0) : 0.5);
      }
     public:
      DisplayParameters()
        : displayQuality(2),
          indexToRGBFunc(&defaultIndexToRGBFunc),
          b(0.0), c(1.0), g(1.0),
          rb(0.0), rc(1.0), rg(1.0),
          gb(0.0), gc(1.0), gg(1.0),
          bb(0.0), bc(1.0), bg(1.0),
          blendScale1(0.5),
          blendScale2(0.7),
          blendScale3(0.3),
          pixelAspectRatio(1.0)
      {
      }
      DisplayParameters(const DisplayParameters& dp)
      {
        copyDisplayParameters(dp);
      }
      DisplayParameters& operator=(const DisplayParameters& dp)
      {
        copyDisplayParameters(dp);
        return (*this);
      }
    };
    // ----------------
    VideoDisplay()
    {
    }
    virtual ~VideoDisplay()
    {
    }
    // set color correction and other display parameters
    // (see 'struct DisplayParameters' above for more information)
    virtual void setDisplayParameters(const DisplayParameters& dp) = 0;
    virtual const DisplayParameters& getDisplayParameters() const = 0;
    // Draw next line of display.
    // 'buf' defines a line of 736 pixels, as 46 groups of 16 pixels each,
    // in the following format: the first byte defines the number of
    // additional bytes that encode the 16 pixels to be displayed. The data
    // length also determines the pixel format, and can have the following
    // values:
    //   0x01: one 8-bit color index (pixel width = 16)
    //   0x02: two 8-bit color indices (pixel width = 8)
    //   0x03: two 8-bit color indices for background (bit value = 0) and
    //         foreground (bit value = 1) color, followed by a 8-bit bitmap
    //         (msb first, pixel width = 2)
    //   0x04: four 8-bit color indices (pixel width = 4)
    //   0x06: similar to 0x03, but there are two sets of colors/bitmap
    //         (c0a, c1a, bitmap_a, c0b, c1b, bitmap_b) and the pixel width
    //         is 1
    //   0x08: eight 8-bit color indices (pixel width = 2)
    //   0x10: sixteen 8-bit color indices (pixel width = 1)
    // The buffer contains 'nBytes' (in the range of 92 to 782) bytes of data.
    virtual void drawLine(const uint8_t *buf, size_t nBytes) = 0;
    // Should be called at the beginning (newState = true) and end
    // (newState = false) of VSYNC. 'currentSlot_' is the position within
    // the current line (0 to 56).
    virtual void vsyncStateChange(bool newState, unsigned int currentSlot_) = 0;
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_DISPLAY_HPP

