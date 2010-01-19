
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

#ifndef EP128EMU_CPCVIDEO_HPP
#define EP128EMU_CPCVIDEO_HPP

#include "ep128emu.hpp"
#include "crtc6845.hpp"

namespace CPC464 {

  class CPCVideo {
   protected:
    const CRTC6845& crtc;
    uint8_t   *lineBufPtr;
    int       hSyncCnt;
    uint8_t   crtcHSyncCnt;
    uint8_t   vSyncCnt;
    uint8_t   videoDelayBufPos;         // 0 or 4
    uint8_t   videoModeLatched;
    // sync (delay=1), displayEnabled (d=2), videoByte0 (d=2), videoByte1 (d=2)
    uint8_t   videoDelayBuf[8];
    const uint8_t *videoMemory;
    uint8_t   *lineBuf;     // 48 * 9 = 432 bytes allocated as 108 uint32_t's
    uint8_t   palette[16];
    uint8_t   borderColor;
    uint8_t   videoMode;
    // --------
    /*!
     * drawLine() is called after rendering each line.
     * 'buf' defines a line of 768 pixels, as 48 groups of 16 pixels each,
     * in the following format: the first byte defines the number of
     * additional bytes that encode the 16 pixels to be displayed. The data
     * length also determines the pixel format, and can have the following
     * values:
     *   0x01: one 8-bit color index (pixel width = 16)
     *   0x02: two 8-bit color indices (pixel width = 8)
     *   0x03: two 8-bit color indices for background (bit value = 0) and
     *         foreground (bit value = 1) color, followed by a 8-bit bitmap
     *         (msb first, pixel width = 2)
     *   0x04: four 8-bit color indices (pixel width = 4)
     *   0x06: similar to 0x03, but there are two sets of colors/bitmap
     *         (c0a, c1a, bitmap_a, c0b, c1b, bitmap_b) and the pixel width
     *         is 1
     *   0x08: eight 8-bit color indices (pixel width = 2)
     * The buffer is aligned to 4 bytes, and contains 'nBytes' (in the range
     * of 96 to 432) bytes of data.
     */
    virtual void drawLine(const uint8_t *buf, size_t nBytes);
    /*!
     * Called at the beginning (newState = true) and end (newState = false)
     * of VSYNC. 'currentSlot_' is the position within the current line
     * (0 to 56).
     */
    virtual void vsyncStateChange(bool newState, unsigned int currentSlot_);
   public:
    CPCVideo(const CRTC6845& crtc_, const uint8_t *videoMemory_);
    virtual ~CPCVideo();
    static void convertPixelToRGB(uint8_t pixelValue,
                                  float& r, float& g, float& b);
    inline void setVideoMode(uint8_t n)
    {
      videoMode = n & 0x03;
    }
    inline uint8_t getVideoMode() const
    {
      return videoMode;
    }
    void setColor(uint8_t penNum, uint8_t c);   // penNum >= 16 is border
    uint8_t getColor(uint8_t penNum) const;
    EP128EMU_REGPARM1 void runOneCycle();
    void reset();
  };

}       // namespace CPC464

#endif  // EP128EMU_CPCVIDEO_HPP

