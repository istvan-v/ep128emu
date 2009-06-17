
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2009 Istvan Varga <istvanv@users.sourceforge.net>
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
                      (src.displayQuality < 4 ? src.displayQuality : 4)
                      : 0);
    bufferingMode = (src.bufferingMode > 0 ?
                     (src.bufferingMode < 2 ? src.bufferingMode : 2) : 0);
    if (src.indexToRGBFunc)
      indexToRGBFunc = src.indexToRGBFunc;
    else
      indexToRGBFunc = &defaultIndexToRGBFunc;
    brightness = (src.brightness > -0.5f ?
                  (src.brightness < 0.5f ? src.brightness : 0.5f)
                  : -0.5f);
    contrast = (src.contrast > 0.5f ?
                (src.contrast < 2.0f ? src.contrast : 2.0f)
                : 0.5f);
    gamma = (src.gamma > 0.25f ?
             (src.gamma < 4.0f ? src.gamma : 4.0f)
             : 0.25f);
    hueShift = (src.hueShift > -180.0f ?
                (src.hueShift < 180.0f ? src.hueShift : 180.0f)
                : -180.0f);
    saturation = (src.saturation > 0.0f ?
                  (src.saturation < 2.0f ? src.saturation : 2.0f)
                  : 0.0f);
    redBrightness = (src.redBrightness > -0.5f ?
                     (src.redBrightness < 0.5f ? src.redBrightness : 0.5f)
                     : -0.5f);
    redContrast = (src.redContrast > 0.5f ?
                   (src.redContrast < 2.0f ? src.redContrast : 2.0f)
                   : 0.5f);
    redGamma = (src.redGamma > 0.25f ?
                (src.redGamma < 4.0f ? src.redGamma : 4.0f)
                : 0.25f);
    greenBrightness = (src.greenBrightness > -0.5f ?
                       (src.greenBrightness < 0.5f ? src.greenBrightness : 0.5f)
                       : -0.5f);
    greenContrast = (src.greenContrast > 0.5f ?
                     (src.greenContrast < 2.0f ? src.greenContrast : 2.0f)
                     : 0.5f);
    greenGamma = (src.greenGamma > 0.25f ?
                  (src.greenGamma < 4.0f ? src.greenGamma : 4.0f)
                  : 0.25f);
    blueBrightness = (src.blueBrightness > -0.5f ?
                      (src.blueBrightness < 0.5f ? src.blueBrightness : 0.5f)
                      : -0.5f);
    blueContrast = (src.blueContrast > 0.5f ?
                    (src.blueContrast < 2.0f ? src.blueContrast : 2.0f)
                    : 0.5f);
    blueGamma = (src.blueGamma > 0.25f ?
                 (src.blueGamma < 4.0f ? src.blueGamma : 4.0f)
                 : 0.25f);
    lineShade = (src.lineShade > 0.0f ?
                 (src.lineShade < 1.0f ? src.lineShade : 1.0f) : 0.0f);
    blendScale = (src.blendScale > 0.5f ?
                  (src.blendScale < 2.0f ? src.blendScale : 2.0f) : 0.5f);
    motionBlur = (src.motionBlur > 0.0f ?
                  (src.motionBlur < 0.95f ? src.motionBlur : 0.95f) : 0.0f);
    pixelAspectRatio = (src.pixelAspectRatio > 0.5f ?
                        (src.pixelAspectRatio < 2.0f ?
                         src.pixelAspectRatio : 2.0f) : 0.5f);
  }

  VideoDisplay::DisplayParameters::DisplayParameters()
    : displayQuality(2),
      bufferingMode(0),
      indexToRGBFunc(&defaultIndexToRGBFunc),
      brightness(0.0f), contrast(1.0f), gamma(1.0f),
      hueShift(0.0f), saturation(1.0f),
      redBrightness(0.0f), redContrast(1.0f), redGamma(1.0f),
      greenBrightness(0.0f), greenContrast(1.0f), greenGamma(1.0f),
      blueBrightness(0.0f), blueContrast(1.0f), blueGamma(1.0f),
      lineShade(0.75f),
      blendScale(1.0f),
      motionBlur(0.25f),
      pixelAspectRatio(1.0f)
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
    // Y = (0.299 * R) + (0.587 * G) + (0.114 * B)
    // U = 0.492 * (B - Y)
    // V = 0.877 * (R - Y)
    float   y = (0.299f * red) + (0.587f * green) + (0.114f * blue);
    float   u = 0.492f * (blue - y);
    float   v = 0.877f * (red - y);
    float   hueShiftU = float(std::cos(double(hueShift) * 0.01745329252));
    float   hueShiftV = float(std::sin(double(hueShift) * 0.01745329252));
    float   tmpU = ((u * hueShiftU) - (v * hueShiftV)) * saturation;
    float   tmpV = ((v * hueShiftU) + (u * hueShiftV)) * saturation;
    // R = (V / 0.877) + Y
    // B = (U / 0.492) + Y
    // G = (Y - ((R * 0.299) + (B * 0.114))) / 0.587
    float   r = y + (tmpV * float(1.0 / 0.877));
    float   g = y + (tmpU * float(-0.114 / (0.492 * 0.587)))
                  + (tmpV * float(-0.299 / (0.877 * 0.587)));
    float   b = y + (tmpU * float(1.0 / 0.492));
    r = (r - 0.5f) * (contrast * redContrast) + 0.5f;
    g = (g - 0.5f) * (contrast * greenContrast) + 0.5f;
    b = (b - 0.5f) * (contrast * blueContrast) + 0.5f;
    r = r + (brightness + redBrightness);
    g = g + (brightness + greenBrightness);
    b = b + (brightness + blueBrightness);
    if (std::fabs(double((gamma * redGamma) - 1.0f)) > 0.01) {
      r = (r > 0.0f ? r : 0.0f);
      r = float(std::pow(double(r), double(1.0f / (gamma * redGamma))));
    }
    if (std::fabs(double((gamma * greenGamma) - 1.0f)) > 0.01) {
      g = (g > 0.0f ? g : 0.0f);
      g = float(std::pow(double(g), double(1.0f / (gamma * greenGamma))));
    }
    if (std::fabs(double((gamma * blueGamma) - 1.0f)) > 0.01) {
      b = (b > 0.0f ? b : 0.0f);
      b = float(std::pow(double(b), double(1.0f / (gamma * blueGamma))));
    }
    red = r;
    green = g;
    blue = b;
  }

  VideoDisplay::~VideoDisplay()
  {
  }

  void VideoDisplay::limitFrameRate(bool isEnabled)
  {
    (void) isEnabled;
  }

}       // namespace Ep128Emu

