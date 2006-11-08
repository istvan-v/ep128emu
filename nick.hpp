
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

#ifndef EP128EMU_NICK_HPP
#define EP128EMU_NICK_HPP

#include "ep128.hpp"

namespace Ep128 {

  enum {
    VIDEO_MEMORY_BASE_ADDR = 0x003F0000
  };

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

#ifdef REGPARM
#  undef REGPARM
#endif
#if defined(__GNUC__) && (__GNUC__ >= 3) && defined(__i386__) && !defined(__ICC)
#  define REGPARM __attribute__ ((__regparm__ (3)))
#else
#  define REGPARM
#endif

  class NickRenderer {
   protected:
    struct NickTables {
      uint8_t twoColors[2048];
      uint8_t fourColors[1024];
      uint8_t sixteenColors[512];
      NickTables();
    };
    static NickTables t;
    NickLPB&  lpb;
    Memory&   m;
    inline void renderByte2ColorsL(uint8_t *buf, uint8_t b1,
                                   uint8_t paletteOffset);
    inline void renderByte4ColorsL(uint8_t *buf, uint8_t b1,
                                   uint8_t paletteOffset);
    inline void renderByte16ColorsL(uint8_t *buf, uint8_t b1);
    inline void renderByte256ColorsL(uint8_t *buf, uint8_t b1);
    inline void renderByte2Colors(uint8_t *buf, uint8_t b1,
                                  uint8_t paletteOffset);
    inline void renderByte4Colors(uint8_t *buf, uint8_t b1,
                                  uint8_t paletteOffset);
    inline void renderByte16Colors(uint8_t *buf, uint8_t b1);
    inline void renderByte256Colors(uint8_t *buf, uint8_t b1);
    inline void renderByteAttribute(uint8_t *buf, uint8_t b1, uint8_t attr);
    inline uint8_t readVideoMemory(uint16_t addr);
   public:
    NickRenderer(NickLPB& lpb_, Memory& m_)
      : lpb(lpb_), m(m_)
    {
    }
    virtual ~NickRenderer()
    {
    }
    // render 16 pixels to 'buf'; called 57 times per line
    virtual REGPARM void doRender(uint8_t *buf) = 0;
  };

#ifdef DECLARE_RENDERER
#  undef DECLARE_RENDERER
#endif
#define DECLARE_RENDERER(x)                     \
class x : public NickRenderer {                 \
 public:                                        \
  x(NickLPB& lpb_, Memory& m_)                  \
    : NickRenderer(lpb_, m_) { }                \
  virtual ~x() { }                              \
  virtual REGPARM void doRender(uint8_t *buf);  \
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

#ifdef REGPARM
#  undef REGPARM
#endif

  class NickRendererTable {
   private:
    NickRenderer  **t;
   public:
    NickRendererTable(NickLPB& lpb, Memory& m);
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
    Memory&   m;
    NickRenderer  *currentRenderer;     // NULL if border only
    NickRendererTable   renderers;
    uint8_t   currentSlot;      // 0 to 56
    uint8_t   *lineBuf;         // 46 slots = 736 pixels
    uint8_t   borderColor;
    bool      lptClockEnabled;
    // ----------------
    inline uint8_t readVideoMemory(uint16_t addr);
   protected:
    // called when the IRQ state changes, with a true parameter when the
    // IRQ is active (low)
    virtual void irqStateChange(bool newState);
    // called after rendering each line; 'buf' contains 736 bytes of pixel data
    virtual void drawLine(const uint8_t *buf);
    // called at the beginning and end of VSYNC
    virtual void vsyncStateChange(bool newState);
   public:
    Nick(Memory& m_);
    virtual ~Nick();
    static void convertPixelToRGB(uint8_t pixelValue,
                                  float& r, float& g, float& b);
    void writePort(uint16_t portNum, uint8_t value);
    void runOneSlot();
    void saveState(File::Buffer&);
    void saveState(File&);
    void loadState(File::Buffer&);
    void registerChunkType(File&);
  };

}

#endif  // EP128EMU_NICK_HPP

