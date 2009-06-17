
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

#ifndef EP128EMU_GLDISP_HPP
#define EP128EMU_GLDISP_HPP

#include "ep128emu.hpp"
#include "system.hpp"
#include "display.hpp"
#include "fldisp.hpp"

#include <FL/Fl_Gl_Window.H>

namespace Ep128Emu {

  class OpenGLDisplay : public Fl_Gl_Window, public FLTKDisplay_ {
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
    class Colormap_YUV {
     private:
      uint32_t  *palette0;
      uint32_t  *palette1;
      static uint32_t pixelConv(double r, double g, double b, double p = 0.0);
     public:
      Colormap_YUV();
      ~Colormap_YUV();
      void setParams(const DisplayParameters& dp);
      inline uint32_t getColor_0(uint8_t c) const
      {
        return palette0[c];
      }
      inline uint32_t getColor_1(uint8_t c) const
      {
        return palette1[c];
      }
    };
    // ----------------
    bool compileShader(int shaderMode_);
    void deleteShader();
    bool enableShader();
    void disableShader();
    void displayFrame();
    void initializeGLDisplay();
    void drawFrame_quality0(Message_LineData **lineBuffers_,
                            double x0, double y0, double x1, double y1,
                            bool oddFrame_);
    void drawFrame_quality1(Message_LineData **lineBuffers_,
                            double x0, double y0, double x1, double y1,
                            bool oddFrame_);
    void drawFrame_quality2(Message_LineData **lineBuffers_,
                            double x0, double y0, double x1, double y1,
                            bool oddFrame_);
    void drawFrame_quality3(Message_LineData **lineBuffers_,
                            double x0, double y0, double x1, double y1,
                            bool oddFrame_);
    void drawFrame_quality4(Message_LineData **lineBuffers_,
                            double x0, double y0, double x1, double y1,
                            bool oddFrame_);
    void copyFrameToRingBuffer();
    static void fltkIdleCallback(void *userData_);
    // ----------------
    Colormap      colormap;
    bool          *linesChanged;
    // 1024x16 texture in 16-bit (R5G6B5) format
    uint16_t      *textureBuffer16;
    unsigned long textureID;
    uint8_t       forceUpdateLineCnt;
    uint8_t       forceUpdateLineMask;
    bool          redrawFlag;
    bool          prvFrameWasOdd;
    int           lastLineNum;
    Timer         noInputTimer;
    Timer         forceUpdateTimer;
    Timer         displayFrameRateTimer;
    Timer         inputFrameRateTimer;
    double        displayFrameRate;
    double        inputFrameRate;
    Message_LineData  **frameRingBuffer[4];
    double        ringBufferReadPos;
    int           ringBufferWritePos;
    int           shaderMode;   // 0: no shader, 1: PAL
    unsigned long shaderHandle;
    unsigned long programHandle;
    // space allocated for textureBuffer16 and textureBuffer32
    unsigned char *textureSpace;
    // 1024x16 YUV texture in 32-bit (A2U10Y10V10) format
    uint32_t      *textureBuffer32;
    Colormap_YUV  colormap32;
   public:
    OpenGLDisplay(int xx = 0, int yy = 0, int ww = 768, int hh = 576,
                  const char *lbl = (char *) 0, bool isDoubleBuffered = false);
    virtual ~OpenGLDisplay();
#ifndef ENABLE_GL_SHADERS
    virtual void setDisplayParameters(const DisplayParameters& dp);
#endif
    /*!
     * Read and process messages sent by the child thread. Returns true if
     * redraw() needs to be called to update the display.
     */
    virtual bool checkEvents();
   protected:
    virtual void draw();
   public:
    virtual int handle(int event);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_GLDISP_HPP

