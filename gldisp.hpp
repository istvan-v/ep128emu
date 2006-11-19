
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

#ifndef EP128EMU_GLDISP_HPP
#define EP128EMU_GLDISP_HPP

#include "ep128.hpp"
#include "system.hpp"

#include <FL/Fl_Gl_Window.H>

namespace Ep128Emu {

  class OpenGLDisplay : public Fl_Gl_Window {
   public:
    struct DisplayParameters {
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
      // ----------------
      DisplayParameters();
      DisplayParameters(const DisplayParameters&);
      DisplayParameters& operator=(const DisplayParameters&);
    };
   private:
    class Colormap {
     private:
      uint16_t  *palette;
      uint16_t  *palette2;
      static uint16_t pixelConv(double r, double g, double b);
     public:
      Colormap();
      ~Colormap();
      void setParams(const DisplayParameters& dp);
      inline uint16_t operator()(uint8_t c) const
      {
        return palette[c];
      }
      inline uint16_t operator()(uint8_t c1, uint8_t c2) const
      {
        return palette2[(size_t(c1) << 8) + c2];
      }
    };
    // ----------------
    class Message {
     public:
      Message *prv;
      Message *nxt;
      // --------
      Message()
      {
        prv = (Message *) 0;
        nxt = (Message *) 0;
      }
      virtual ~Message();
    };
    class Message_LineData : public Message {
     private:
      // a line of 704 pixels needs a maximum space of 704 * (17 / 16) = 748
      // ( = 187 * 4) bytes in compressed format
      uint32_t  buf_[187];
      // number of bytes in buffer
      size_t    nBytes_;
     public:
      // line number
      int       lineNum;
      Message_LineData()
        : Message()
      {
        nBytes_ = 0;
        lineNum = 0;
      }
      virtual ~Message_LineData();
      // copy a line (736 pixels in compressed format) to the buffer
      void copyLine(const uint8_t *buf, size_t nBytes);
      inline void getLineData(const unsigned char*& buf, size_t& nBytes)
      {
        buf = reinterpret_cast<unsigned char *>(&(buf_[0]));
        nBytes = nBytes_;
      }
    };
    class Message_FrameDone : public Message {
     public:
      Message_FrameDone()
        : Message()
      {
      }
      virtual ~Message_FrameDone();
    };
    class Message_SetParameters : public Message {
     public:
      DisplayParameters dp;
      Message_SetParameters()
        : Message(),
          dp()
      {
      }
      virtual ~Message_SetParameters();
    };
    template <typename T>
    T * allocateMessage()
    {
      void  *m_ = (void *) 0;
      messageQueueMutex.lock();
      if (freeMessageStack) {
        Message *m = freeMessageStack;
        freeMessageStack = m->nxt;
        if (freeMessageStack)
          freeMessageStack->prv = (Message *) 0;
        m_ = m;
      }
      messageQueueMutex.unlock();
      if (!m_) {
        // allocate space that is enough for the largest message type
        m_ = std::malloc((sizeof(Message_LineData) | 15) + 1);
        if (!m_)
          throw std::bad_alloc();
      }
      T *m;
      try {
        m = new(m_) T();
      }
      catch (...) {
        std::free(m_);
        throw;
      }
      return m;
    }
    void deleteMessage(Message *m);
    void queueMessage(Message *m);
    void displayFrame();
    // ----------------
    Message       *messageQueue;
    Message       *lastMessage;
    Message       *freeMessageStack;
    Mutex         messageQueueMutex;
    Colormap      colormap;
    // for 578 lines (576 + 2 border)
    Message_LineData  **lineBuffers;
    // 1024x1024 texture in 16-bit (R5G6B5) format
    uint16_t      *textureBuffer;
    unsigned long textureID;
    int           curLine;
    int           lineCnt;      // nr. of lines received so far in this frame
    int           framesPending;
    bool          skippingFrame;
    DisplayParameters   displayParameters;
    DisplayParameters   savedDisplayParameters;
    volatile bool exitFlag;
   public:
    OpenGLDisplay(int xx = 0, int yy = 0, int ww = 704, int hh = 576,
                  const char *lbl = (char *) 0);
    virtual ~OpenGLDisplay();
    // set color correction and other display parameters
    // (see 'struct DisplayParameters' above for more information)
    void setDisplayParameters(const DisplayParameters& dp);
    const DisplayParameters& getDisplayParameters() const;
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
    void drawLine(const uint8_t *buf, size_t nBytes);
    // Should be called at the beginning (newState = true) and end
    // (newState = false) of VSYNC. 'currentSlot_' is the position within
    // the current line (0 to 56).
    void vsyncStateChange(bool newState, unsigned int currentSlot_);
   protected:
    virtual void draw();
   public:
    virtual int handle(int event);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_GLDISP_HPP

