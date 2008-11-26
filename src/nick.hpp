
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2008 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EP128EMU_NICK_HPP
#define EP128EMU_NICK_HPP

#include "ep128emu.hpp"

namespace Ep128 {

  class Memory;
  class Nick;

  struct NickLPB {
    int       nLines;           // total number of lines in this LPB (1..256)
    bool      interruptFlag;    // true: trigger interrupt
    bool      vresMode;         // true: enable full vertical resolution
    bool      reloadFlag;       // true: reload LPT address
    uint8_t   colorMode;        // 0: 2 colors
                                // 1: 4 colors
                                // 2: 16 colors
                                // 3: 256 colors
    uint8_t   videoMode;        // 0: VSYNC
                                // 1: PIXEL
                                // 2: ATTRIBUTE
                                // 3: CH256
                                // 4: CH128
                                // 5: CH64
                                // 6: invalid mode
                                // 7: LPIXEL
    bool      altInd0;          // +4 to palette index if b6 of charcode is set
    bool      altInd1;          // +2 to palette index if b7 of charcode is set
    bool      lsbAlt;           // +4 to palette index if b0 of bitmap is set
    bool      msbAlt;           // +2 to palette index if b7 of bitmap is set
    uint8_t   leftMargin;
    uint8_t   rightMargin;
    uint16_t  ld1Base;          // LD1 base address
    uint16_t  ld2Base;          // LD2 base address
    uint8_t   palette[16];      // 0..7 from LPB, 8..15 from FIXBIAS
    // ----------------
    uint16_t  ld1Addr;          // LD1 current address
    uint16_t  ld2Addr;          // LD2 current address
  };

  // --------------------------------------------------------------------------

  class NickRenderer {
   protected:
    struct NickTables {
      uint8_t fourColors[1024];
      uint8_t sixteenColors[512];
      NickTables();
    };
    static NickTables t;
    NickLPB&  lpb;
    const uint8_t *videoMemory;
    inline void renderByte2ColorsL(uint8_t*& buf_, uint8_t b1,
                                   uint8_t paletteOffset);
    inline void renderByte4ColorsL(uint8_t*& buf_, uint8_t b1,
                                   uint8_t paletteOffset);
    inline void renderByte16ColorsL(uint8_t*& buf_, uint8_t b1);
    inline void renderByte256ColorsL(uint8_t*& buf_, uint8_t b1);
    inline void renderBytes2Colors(uint8_t*& buf_,
                                   uint8_t b1, uint8_t b2,
                                   uint8_t paletteOffset1,
                                   uint8_t paletteOffset2);
    inline void renderBytes4Colors(uint8_t*& buf_,
                                   uint8_t b1, uint8_t b2,
                                   uint8_t paletteOffset1,
                                   uint8_t paletteOffset2);
    inline void renderBytes16Colors(uint8_t*& buf_, uint8_t b1, uint8_t b2);
    inline void renderBytes256Colors(uint8_t*& buf_, uint8_t b1, uint8_t b2);
    inline void renderBytesAttribute(uint8_t*& buf_, uint8_t b1, uint8_t attr);
   public:
    NickRenderer(NickLPB& lpb_, const uint8_t *videoMemory_)
      : lpb(lpb_), videoMemory(videoMemory_)
    {
    }
    virtual ~NickRenderer()
    {
    }
    // render 16 pixels to 'buf'; called 46 times per line
    virtual EP128EMU_REGPARM3 void doRender(uint8_t*& buf) = 0;
  };

#ifdef DECLARE_RENDERER
#  undef DECLARE_RENDERER
#endif
#define DECLARE_RENDERER(x)                     \
class x : public NickRenderer {                 \
 public:                                        \
  x(NickLPB& lpb_, const uint8_t *videoMemory_) \
    : NickRenderer(lpb_, videoMemory_) { }      \
  virtual ~x() { }                              \
  virtual EP128EMU_REGPARM3 void doRender(uint8_t*& buf);   \
}

  DECLARE_RENDERER(NickRenderer_Blank);
  DECLARE_RENDERER(NickRenderer_PIXEL_2);
  DECLARE_RENDERER(NickRenderer_PIXEL_2_LSBALT);
  DECLARE_RENDERER(NickRenderer_PIXEL_2_MSBALT);
  DECLARE_RENDERER(NickRenderer_PIXEL_2_LSBALT_MSBALT);
  DECLARE_RENDERER(NickRenderer_PIXEL_4);
  DECLARE_RENDERER(NickRenderer_PIXEL_4_LSBALT);
  DECLARE_RENDERER(NickRenderer_PIXEL_16);
  DECLARE_RENDERER(NickRenderer_PIXEL_256);
  DECLARE_RENDERER(NickRenderer_ATTRIBUTE);
  DECLARE_RENDERER(NickRenderer_CH256_2);
  DECLARE_RENDERER(NickRenderer_CH256_4);
  DECLARE_RENDERER(NickRenderer_CH256_16);
  DECLARE_RENDERER(NickRenderer_CH256_256);
  DECLARE_RENDERER(NickRenderer_CH128_2);
  DECLARE_RENDERER(NickRenderer_CH128_2_ALTIND1);
  DECLARE_RENDERER(NickRenderer_CH128_4);
  DECLARE_RENDERER(NickRenderer_CH128_16);
  DECLARE_RENDERER(NickRenderer_CH128_256);
  DECLARE_RENDERER(NickRenderer_CH64_2);
  DECLARE_RENDERER(NickRenderer_CH64_2_ALTIND0);
  DECLARE_RENDERER(NickRenderer_CH64_2_ALTIND1);
  DECLARE_RENDERER(NickRenderer_CH64_2_ALTIND0_ALTIND1);
  DECLARE_RENDERER(NickRenderer_CH64_4);
  DECLARE_RENDERER(NickRenderer_CH64_4_ALTIND0);
  DECLARE_RENDERER(NickRenderer_CH64_16);
  DECLARE_RENDERER(NickRenderer_CH64_256);
  DECLARE_RENDERER(NickRenderer_INVALID_MODE);
  DECLARE_RENDERER(NickRenderer_LPIXEL_2);
  DECLARE_RENDERER(NickRenderer_LPIXEL_2_LSBALT);
  DECLARE_RENDERER(NickRenderer_LPIXEL_2_MSBALT);
  DECLARE_RENDERER(NickRenderer_LPIXEL_2_LSBALT_MSBALT);
  DECLARE_RENDERER(NickRenderer_LPIXEL_4);
  DECLARE_RENDERER(NickRenderer_LPIXEL_4_LSBALT);
  DECLARE_RENDERER(NickRenderer_LPIXEL_16);
  DECLARE_RENDERER(NickRenderer_LPIXEL_256);

#undef DECLARE_RENDERER

  class NickRendererTable {
   private:
    NickRenderer  **t;
   public:
    NickRendererTable(NickLPB& lpb, const uint8_t *videoMemory_);
    virtual ~NickRendererTable();
    NickRenderer& getRenderer(const NickLPB& lpb);
  };

  // --------------------------------------------------------------------------

  class Nick {
   private:
    NickLPB   lpb;              // current LPB
    uint16_t  lptBaseAddr;      // LPT base address
    uint16_t  lptCurrentAddr;   // current LPT address
    int       linesRemaining;   // lines remaining until loading next LPB
    const uint8_t *videoMemory;
    NickRenderer  *currentRenderer;     // NULL if border only
    NickRendererTable   renderers;
    uint8_t   currentSlot;      // 0 to 56
    uint8_t   *lineBuf;         // 57 slots = 912 pixels
    uint8_t   *lineBufPtr;
    uint8_t   *oldLineBufPtr;
    uint8_t   borderColor;
    uint8_t   dataBusState;
    bool      lptClockEnabled;
    uint8_t   savedBorderColor;
    bool      vsyncFlag;
    // --------
    void clearLineBuffer();
   protected:
    /*!
     * Called when the IRQ state changes, with a true parameter when the
     * IRQ is active (low).
     */
    virtual void irqStateChange(bool newState);
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
     *   0x10: sixteen 8-bit color indices (pixel width = 1)
     * The buffer is aligned to 4 bytes, and contains 'nBytes' (in the range
     * of 96 to 816) bytes of data.
     */
    virtual void drawLine(const uint8_t *buf, size_t nBytes);
    /*!
     * Called at the beginning (newState = true) and end (newState = false)
     * of VSYNC. 'currentSlot_' is the position within the current line
     * (0 to 56).
     */
    virtual void vsyncStateChange(bool newState, unsigned int currentSlot_);
   public:
    Nick(Memory& m_);
    virtual ~Nick();
    static void convertPixelToRGB(uint8_t pixelValue,
                                  float& r, float& g, float& b);
    uint8_t readPort(uint16_t portNum);
    void writePort(uint16_t portNum, uint8_t value);
    uint8_t readPortDebug(uint16_t portNum) const;
    void randomizeRegisters();
    // returns a pointer to the video output generated in the last cycle
    // (16 pixels); the format is the same as in the case of drawLine()
    inline const uint8_t * getVideoOutput() const
    {
      return oldLineBufPtr;
    }
    void runOneSlot();
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
  };

}       // namespace Ep128

#endif  // EP128EMU_NICK_HPP

