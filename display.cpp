
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

#include "ep128emu.hpp"
#include "display.hpp"

#include <cmath>

namespace Ep128Emu {

  void VideoDisplay::DisplayParameters::defaultIndexToRGBFunc(uint8_t color,
                                                              float& red,
                                                              float& green,
                                                              float& blue)
  {
    blue = green = red = (float(color) / 255.0f);
  }

  void VideoDisplay::DisplayParameters::copyDisplayParameters(
      const DisplayParameters& src)
  {
    displayQuality = (src.displayQuality > 0 ?
                      (src.displayQuality < 3 ? src.displayQuality : 3)
                      : 0);
    if (src.indexToRGBFunc)
      indexToRGBFunc = src.indexToRGBFunc;
    else
      indexToRGBFunc = &defaultIndexToRGBFunc;
    brightness = (src.brightness > -0.5 ?
                  (src.brightness < 0.5 ? src.brightness : 0.5)
                  : -0.5);
    contrast = (src.contrast > 0.5 ?
                (src.contrast < 2.0 ? src.contrast : 2.0)
                : 0.5);
    gamma = (src.gamma > 0.25 ?
             (src.gamma < 4.0 ? src.gamma : 4.0)
             : 0.25);
    saturation = (src.saturation > 0.0 ?
                  (src.saturation < 2.0 ? src.saturation : 2.0)
                  : 0.0);
    redBrightness = (src.redBrightness > -0.5 ?
                     (src.redBrightness < 0.5 ? src.redBrightness : 0.5)
                     : -0.5);
    redContrast = (src.redContrast > 0.5 ?
                   (src.redContrast < 2.0 ? src.redContrast : 2.0)
                   : 0.5);
    redGamma = (src.redGamma > 0.25 ?
                (src.redGamma < 4.0 ? src.redGamma : 4.0)
                : 0.25);
    greenBrightness = (src.greenBrightness > -0.5 ?
                       (src.greenBrightness < 0.5 ? src.greenBrightness : 0.5)
                       : -0.5);
    greenContrast = (src.greenContrast > 0.5 ?
                     (src.greenContrast < 2.0 ? src.greenContrast : 2.0)
                     : 0.5);
    greenGamma = (src.greenGamma > 0.25 ?
                  (src.greenGamma < 4.0 ? src.greenGamma : 4.0)
                  : 0.25);
    blueBrightness = (src.blueBrightness > -0.5 ?
                      (src.blueBrightness < 0.5 ? src.blueBrightness : 0.5)
                      : -0.5);
    blueContrast = (src.blueContrast > 0.5 ?
                    (src.blueContrast < 2.0 ? src.blueContrast : 2.0)
                    : 0.5);
    blueGamma = (src.blueGamma > 0.25 ?
                 (src.blueGamma < 4.0 ? src.blueGamma : 4.0)
                 : 0.25);
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

  VideoDisplay::DisplayParameters::DisplayParameters()
    : displayQuality(2),
      indexToRGBFunc(&defaultIndexToRGBFunc),
      brightness(0.0), contrast(1.0), gamma(1.0), saturation(1.0),
      redBrightness(0.0), redContrast(1.0), redGamma(1.0),
      greenBrightness(0.0), greenContrast(1.0), greenGamma(1.0),
      blueBrightness(0.0), blueContrast(1.0), blueGamma(1.0),
      blendScale1(0.5),
      blendScale2(0.7),
      blendScale3(0.3),
      pixelAspectRatio(1.0)
  {
  }

  VideoDisplay::DisplayParameters::DisplayParameters(
      const DisplayParameters& dp)
  {
    copyDisplayParameters(dp);
  }

  VideoDisplay::DisplayParameters& VideoDisplay::DisplayParameters::operator=(
      const DisplayParameters& dp)
  {
    copyDisplayParameters(dp);
    return (*this);
  }

  void VideoDisplay::DisplayParameters::applyColorCorrection(float& red,
                                                             float& green,
                                                             float& blue) const
  {
    double  r = red;
    double  g = green;
    double  b = blue;
    // Y = (0.299 * R) + (0.587 * G) + (0.114 * B)
    // U = 0.492 * (B - Y)
    // V = 0.877 * (R - Y)
    double  y = (0.299 * r) + (0.587 * g) + (0.114 * b);
    double  u = 0.492 * (b - y);
    double  v = 0.877 * (r - y);
    u = u * saturation;
    v = v * saturation;
    // R = (V / 0.877) + Y
    // B = (U / 0.492) + Y
    // G = (Y - ((R * 0.299) + (B * 0.114))) / 0.587
    r = (v / 0.877) + y;
    b = (u / 0.492) + y;
    g = (y - ((r * 0.299) + (b * 0.114))) / 0.587;
    r = std::pow((r > 0.0 ? r : 0.0), 1.0 / gamma);
    g = std::pow((g > 0.0 ? g : 0.0), 1.0 / gamma);
    b = std::pow((b > 0.0 ? b : 0.0), 1.0 / gamma);
    r = (r - 0.5) * contrast + 0.5 + brightness;
    g = (g - 0.5) * contrast + 0.5 + brightness;
    b = (b - 0.5) * contrast + 0.5 + brightness;
    r = std::pow((r > 0.0 ? r : 0.0), 1.0 / redGamma);
    g = std::pow((g > 0.0 ? g : 0.0), 1.0 / greenGamma);
    b = std::pow((b > 0.0 ? b : 0.0), 1.0 / blueGamma);
    r = (r - 0.5) * redContrast + 0.5 + redBrightness;
    g = (g - 0.5) * greenContrast + 0.5 + greenBrightness;
    b = (b - 0.5) * blueContrast + 0.5 + blueBrightness;
    red = float(r > 0.0 ? (r < 1.0 ? r : 1.0) : 0.0);
    green = float(g > 0.0 ? (g < 1.0 ? g : 1.0) : 0.0);
    blue = float(b > 0.0 ? (b < 1.0 ? b : 1.0) : 0.0);
  }

  VideoDisplay::~VideoDisplay()
  {
  }

}       // namespace Ep128Emu

