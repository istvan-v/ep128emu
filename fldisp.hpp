
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

#ifndef EP128EMU_FLDISP_HPP
#define EP128EMU_FLDISP_HPP

#include "ep128emu.hpp"
#include "system.hpp"
#include "display.hpp"

#include <FL/Fl_Window.H>

namespace Ep128Emu {

  class FLTKDisplay : public Fl_Window, public VideoDisplay {
   private:
    class Colormap {
     private:
      uint32_t  *palette;
      uint32_t  *palette2;
      static uint32_t pixelConv(double r, double g, double b);
     public:
      Colormap();
      ~Colormap();
      void setParams(const DisplayParameters& dp);
      inline uint32_t operator()(uint8_t c) const
      {
        return palette[c];
      }
      inline uint32_t operator()(uint8_t c1, uint8_t c2) const
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
      // a line of 768 pixels needs a maximum space of 768 * (17 / 16) = 816
      // ( = 204 * 4) bytes in compressed format
      uint32_t  buf_[204];
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
      // copy a line (768 pixels in compressed format) to the buffer
      void copyLine(const uint8_t *buf, size_t nBytes);
      inline void getLineData(const unsigned char*& buf, size_t& nBytes)
      {
        buf = reinterpret_cast<unsigned char *>(&(buf_[0]));
        nBytes = nBytes_;
      }
      bool operator==(const Message_LineData& r) const
      {
        if (r.nBytes_ != nBytes_)
          return false;
        size_t  n = (nBytes_ + 3) >> 2;
        for (size_t i = 0; i < n; i++) {
          if (r.buf_[i] != buf_[i])
            return false;
        }
        return true;
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
    // for 576 lines
    Message_LineData  **lineBuffers;
    bool          *linesChanged;
    int           curLine;
    int           lineCnt;      // nr. of lines received so far in this frame
    int           prvLineCnt;
    int           framesPending;
    bool          skippingFrame;
    bool          vsyncState;
    DisplayParameters   displayParameters;
    DisplayParameters   savedDisplayParameters;
    volatile bool exitFlag;
    uint8_t       forceUpdateLineCnt;
    uint8_t       forceUpdateLineMask;
    bool          redrawFlag;
    Timer         noInputTimer;
    Timer         forceUpdateTimer;
    ThreadLock    threadLock;
   public:
    FLTKDisplay(int xx = 0, int yy = 0, int ww = 768, int hh = 576,
                const char *lbl = (char *) 0);
    virtual ~FLTKDisplay();
    // set color correction and other display parameters
    // (see 'struct DisplayParameters' above for more information)
    virtual void setDisplayParameters(const DisplayParameters& dp);
    virtual const DisplayParameters& getDisplayParameters() const;
    // Draw next line of display.
    // 'buf' defines a line of 768 pixels, as 48 groups of 16 pixels each,
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
    // The buffer contains 'nBytes' (in the range of 96 to 816) bytes of data.
    virtual void drawLine(const uint8_t *buf, size_t nBytes);
    // Should be called at the beginning (newState = true) and end
    // (newState = false) of VSYNC. 'currentSlot_' is the position within
    // the current line (0 to 56).
    virtual void vsyncStateChange(bool newState, unsigned int currentSlot_);
    // Read and process messages sent by the child thread. Returns true if
    // redraw() needs to be called to update the display.
    bool checkEvents();
   protected:
    virtual void draw();
   public:
    virtual int handle(int event);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_FLDISP_HPP

