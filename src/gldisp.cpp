
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

#include "ep128emu.hpp"
#include "system.hpp"

#include <cstring>
#include <typeinfo>

#define GL_GLEXT_PROTOTYPES 1

#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <GL/glext.h>

#include "fldisp.hpp"
#include "gldisp.hpp"

#ifdef WIN32
#  undef WIN32
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  define WIN32 1
#endif

#ifdef WIN32
#  include <wingdi.h>
#  if defined(_MSC_VER) && !defined(__GNUC__)
typedef void (APIENTRY *PFNGLBLENDCOLORPROC)(GLclampf, GLclampf, GLclampf,
                                             GLclampf);
#  endif
#endif

#ifndef WIN32
#  define   glBlendColor_         glBlendColor
#else
static PFNGLBLENDCOLORPROC      glBlendColor__ = (PFNGLBLENDCOLORPROC) 0;
static inline void glBlendColor_(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
  if (glBlendColor__)
    glBlendColor__(r, g, b, a);
  else
    glDisable(GL_BLEND);
}
#endif

static void setTextureParameters(int displayQuality)
{
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  if (displayQuality > 0) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glPixelTransferf(GL_RED_SCALE, GLfloat(1));
  glPixelTransferf(GL_RED_BIAS, GLfloat(0));
  glPixelTransferf(GL_GREEN_SCALE, GLfloat(1));
  glPixelTransferf(GL_GREEN_BIAS, GLfloat(0));
  glPixelTransferf(GL_BLUE_SCALE, GLfloat(1));
  glPixelTransferf(GL_BLUE_BIAS, GLfloat(0));
  glPixelTransferf(GL_ALPHA_SCALE, GLfloat(1));
  glPixelTransferf(GL_ALPHA_BIAS, GLfloat(0));
}

static void initializeTexture(const Ep128Emu::VideoDisplay::DisplayParameters&
                                  dp,
                              const uint16_t *textureBuffer)
{
  GLsizei txtWidth = 1024;
  GLsizei txtHeight = 16;
  switch (dp.displayQuality) {
  case 0:
    txtHeight = 8;
    break;
  case 1:
    txtWidth = 512;
    break;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, txtWidth, txtHeight, 0,
               GL_RGB, GL_UNSIGNED_SHORT_5_6_5, (const GLvoid *) textureBuffer);
}

namespace Ep128Emu {

  OpenGLDisplay::Colormap::Colormap()
  {
    palette = new uint16_t[256];
    try {
      palette2 = new uint16_t[65536];
    }
    catch (...) {
      delete[] palette;
      throw;
    }
    DisplayParameters dp;
    setParams(dp);
  }

  OpenGLDisplay::Colormap::~Colormap()
  {
    delete[] palette;
    delete[] palette2;
  }

  void OpenGLDisplay::Colormap::setParams(const DisplayParameters& dp)
  {
    float   rTbl[256];
    float   gTbl[256];
    float   bTbl[256];
    for (size_t i = 0; i < 256; i++) {
      float   r = float(uint8_t(i)) / 255.0f;
      float   g = float(uint8_t(i)) / 255.0f;
      float   b = float(uint8_t(i)) / 255.0f;
      if (dp.indexToRGBFunc)
        dp.indexToRGBFunc(uint8_t(i), r, g, b);
      dp.applyColorCorrection(r, g, b);
      rTbl[i] = r;
      gTbl[i] = g;
      bTbl[i] = b;
    }
    for (size_t i = 0; i < 256; i++) {
      palette[i] = pixelConv(rTbl[i], gTbl[i], bTbl[i]);
    }
    double  lineShade_ = double(dp.lineShade * 0.5f);
    for (size_t i = 0; i < 256; i++) {
      for (size_t j = 0; j < 256; j++) {
        double  r = (rTbl[i] + rTbl[j]) * lineShade_;
        double  g = (gTbl[i] + gTbl[j]) * lineShade_;
        double  b = (bTbl[i] + bTbl[j]) * lineShade_;
        palette2[(i << 8) + j] = pixelConv(r, g, b);
      }
    }
  }

  uint16_t OpenGLDisplay::Colormap::pixelConv(double r, double g, double b)
  {
    int ri = int(r >= 0.0f ? (r < 1.0f ? (r * 248.0f + 4.0f) : 252.0f) : 4.0f);
    int gi = int(g >= 0.0f ? (g < 1.0f ? (g * 504.0f + 4.0f) : 508.0f) : 4.0f);
    int bi = int(b >= 0.0f ? (b < 1.0f ? (b * 248.0f + 4.0f) : 252.0f) : 4.0f);
    if (((ri | bi) & 7) < 2 && (gi & 7) >= 6)
      gi = gi + 4;
    if (((ri & bi) & 7) >= 6 && (gi & 7) < 2)
      gi = gi - 4;
    return uint16_t(((ri & 0x00F8) << 8) | ((gi & 0x01F8) << 2) | (bi >> 3));
  }

  // --------------------------------------------------------------------------

  OpenGLDisplay::OpenGLDisplay(int xx, int yy, int ww, int hh,
                               const char *lbl, bool isDoubleBuffered)
    : Fl_Gl_Window(xx, yy, ww, hh, lbl),
      FLTKDisplay_(),
      colormap(),
      linesChanged((bool *) 0),
      textureBuffer((uint16_t *) 0),
      textureID(0UL),
      forceUpdateLineCnt(0),
      forceUpdateLineMask(0),
      redrawFlag(false),
      displayFrameRate(60.0),
      inputFrameRate(50.0),
      ringBufferReadPos(0.0),
      ringBufferWritePos(2)
  {
    try {
      for (size_t n = 0; n < 4; n++)
        frameRingBuffer[n] = (Message_LineData **) 0;
      linesChanged = new bool[289];
      for (size_t n = 0; n < 289; n++)
        linesChanged[n] = false;
      textureBuffer = new uint16_t[1024 * 16];  // max. texture size = 1024x16
      std::memset(textureBuffer, 0, sizeof(uint16_t) * 1024 * 16);
      for (size_t n = 0; n < 4; n++) {
        frameRingBuffer[n] = new Message_LineData*[578];
        for (size_t yc = 0; yc < 578; yc++)
          frameRingBuffer[n][yc] = (Message_LineData *) 0;
      }
    }
    catch (...) {
      if (linesChanged)
        delete[] linesChanged;
      if (textureBuffer)
        delete[] textureBuffer;
      for (size_t n = 0; n < 4; n++) {
        if (frameRingBuffer[n])
          delete[] frameRingBuffer[n];
      }
      throw;
    }
    displayParameters.bufferingMode = (isDoubleBuffered ? 1 : 0);
    savedDisplayParameters.bufferingMode = (isDoubleBuffered ? 1 : 0);
    this->mode(FL_RGB | (isDoubleBuffered ? FL_DOUBLE : FL_SINGLE));
  }

  OpenGLDisplay::~OpenGLDisplay()
  {
    Fl::remove_idle(&fltkIdleCallback, (void *) this);
    if (textureID) {
      GLuint  tmp = GLuint(textureID);
      textureID = 0UL;
      glDeleteTextures(1, &tmp);
    }
    delete[] textureBuffer;
    delete[] linesChanged;
    for (size_t n = 0; n < 4; n++) {
      for (size_t yc = 0; yc < 578; yc++) {
        Message *m = frameRingBuffer[n][yc];
        if (m) {
          frameRingBuffer[n][yc] = (Message_LineData *) 0;
          m->~Message();
          std::free(m);
        }
      }
      delete[] frameRingBuffer[n];
    }
  }

  void OpenGLDisplay::drawFrame_quality0(Message_LineData **lineBuffers_,
                                         double x0, double y0,
                                         double x1, double y1)
  {
    unsigned char lineBuf1[768];
    unsigned char *curLine_ = &(lineBuf1[0]);
    // full horizontal resolution, no interlace (768x288)
    // no texture filtering or effects
    for (size_t yc = 0; yc < 288; yc += 8) {
      for (size_t offs = 0; offs < 8; offs++) {
        linesChanged[yc + offs + 1] = false;
        // decode video data
        const unsigned char *bufp = (unsigned char *) 0;
        size_t  nBytes = 0;
        size_t  lineNum = (yc + offs + 1) << 1;
        if (lineBuffers_[lineNum + 0] != (Message_LineData *) 0) {
          lineBuffers_[lineNum + 0]->getLineData(bufp, nBytes);
          decodeLine(curLine_, bufp, nBytes);
        }
        else if (lineBuffers_[lineNum + 1] != (Message_LineData *) 0) {
          lineBuffers_[lineNum + 1]->getLineData(bufp, nBytes);
          decodeLine(curLine_, bufp, nBytes);
        }
        else
          std::memset(curLine_, 0, 768);
        // build 16-bit texture:
        // full horizontal resolution, no interlace (768x8)
        uint16_t  *txtp = &(textureBuffer[offs * 768]);
        for (size_t xc = 0; xc < 768; xc++)
          txtp[xc] = colormap(curLine_[xc]);
      }
      // load texture
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 768, 8,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                      (GLvoid *) textureBuffer);
      // update display
      double  ycf0 = y0 + ((double(int(yc << 1)) * (1.0 / 576.0))
                           * (y1 - y0));
      double  ycf1 = y0 + ((double(int(yc << 1) + 16) * (1.0 / 576.0))
                           * (y1 - y0));
      glBegin(GL_QUADS);
      glTexCoord2f(GLfloat(0.0), GLfloat(0.001 / 8.0));
      glVertex2f(GLfloat(x0), GLfloat(ycf0));
      glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(0.001 / 8.0));
      glVertex2f(GLfloat(x1), GLfloat(ycf0));
      glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(7.999 / 8.0));
      glVertex2f(GLfloat(x1), GLfloat(ycf1));
      glTexCoord2f(GLfloat(0.0), GLfloat(7.999 / 8.0));
      glVertex2f(GLfloat(x0), GLfloat(ycf1));
      glEnd();
    }
  }

  void OpenGLDisplay::drawFrame_quality1(Message_LineData **lineBuffers_,
                                         double x0, double y0,
                                         double x1, double y1)
  {
    unsigned char lineBuf1[768];
    unsigned char *curLine_ = &(lineBuf1[0]);
    // half horizontal resolution, no interlace (384x288)
    for (size_t yc = 0; yc < 588; yc += 28) {
      for (size_t offs = 0; offs < 32; offs += 2) {
        // decode video data
        const unsigned char *bufp = (unsigned char *) 0;
        size_t  nBytes = 0;
        bool    haveLineData = false;
        if ((yc + offs) < 578) {
          if (lineBuffers_[yc + offs] != (Message_LineData *) 0) {
            lineBuffers_[yc + offs]->getLineData(bufp, nBytes);
            haveLineData = true;
          }
          else if (lineBuffers_[yc + offs + 1] != (Message_LineData *) 0) {
            lineBuffers_[yc + offs + 1]->getLineData(bufp, nBytes);
            haveLineData = true;
          }
        }
        if (haveLineData)
          decodeLine(curLine_, bufp, nBytes);
        else
          std::memset(curLine_, 0, 768);
        // build 16-bit texture
        uint16_t  *txtp = &(textureBuffer[(offs >> 1) * 384]);
        for (size_t xc = 0; xc < 768; xc += 2)
          txtp[xc >> 1] = colormap(curLine_[xc], curLine_[xc + 1]);
      }
      // load texture
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 384, 16,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                      (GLvoid *) textureBuffer);
      // update display
      double  ycf0 = y0 + ((double(int(yc)) * (1.0 / 576.0))
                           * (y1 - y0));
      double  ycf1 = y0 + ((double(int(yc + 28)) * (1.0 / 576.0))
                           * (y1 - y0));
      double  txtycf1 = 15.0 / 16.0;
      if (yc == 560) {
        ycf1 -= ((y1 - y0) * (12.0 / 576.0));
        txtycf1 -= (6.0 / 16.0);
      }
      glBegin(GL_QUADS);
      glTexCoord2f(GLfloat(0.0), GLfloat(1.0 / 16.0));
      glVertex2f(GLfloat(x0), GLfloat(ycf0));
      glTexCoord2f(GLfloat(384.0 / 512.0), GLfloat(1.0 / 16.0));
      glVertex2f(GLfloat(x1), GLfloat(ycf0));
      glTexCoord2f(GLfloat(384.0 / 512.0), GLfloat(txtycf1));
      glVertex2f(GLfloat(x1), GLfloat(ycf1));
      glTexCoord2f(GLfloat(0.0), GLfloat(txtycf1));
      glVertex2f(GLfloat(x0), GLfloat(ycf1));
      glEnd();
    }
  }

  void OpenGLDisplay::drawFrame_quality2(Message_LineData **lineBuffers_,
                                         double x0, double y0,
                                         double x1, double y1)
  {
    unsigned char lineBuf1[768];
    unsigned char *curLine_ = &(lineBuf1[0]);
    // full horizontal resolution, no interlace (768x288)
    for (size_t yc = 0; yc < 588; yc += 28) {
      for (size_t offs = 0; offs < 32; offs += 2) {
        // decode video data
        const unsigned char *bufp = (unsigned char *) 0;
        size_t  nBytes = 0;
        bool    haveLineData = false;
        if ((yc + offs) < 578) {
          if (lineBuffers_[yc + offs] != (Message_LineData *) 0) {
            lineBuffers_[yc + offs]->getLineData(bufp, nBytes);
            haveLineData = true;
          }
          else if (lineBuffers_[yc + offs + 1] != (Message_LineData *) 0) {
            lineBuffers_[yc + offs + 1]->getLineData(bufp, nBytes);
            haveLineData = true;
          }
        }
        if (haveLineData)
          decodeLine(curLine_, bufp, nBytes);
        else
          std::memset(curLine_, 0, 768);
        // build 16-bit texture
        uint16_t  *txtp = &(textureBuffer[(offs >> 1) * 768]);
        for (size_t xc = 0; xc < 768; xc++)
          txtp[xc] = colormap(curLine_[xc]);
      }
      // load texture
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 768, 16,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                      (GLvoid *) textureBuffer);
      // update display
      double  ycf0 = y0 + ((double(int(yc)) * (1.0 / 576.0))
                           * (y1 - y0));
      double  ycf1 = y0 + ((double(int(yc + 28)) * (1.0 / 576.0))
                           * (y1 - y0));
      double  txtycf1 = 15.0 / 16.0;
      if (yc == 560) {
        ycf1 -= ((y1 - y0) * (12.0 / 576.0));
        txtycf1 -= (6.0 / 16.0);
      }
      glBegin(GL_QUADS);
      glTexCoord2f(GLfloat(0.0), GLfloat(1.0 / 16.0));
      glVertex2f(GLfloat(x0), GLfloat(ycf0));
      glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(1.0 / 16.0));
      glVertex2f(GLfloat(x1), GLfloat(ycf0));
      glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(txtycf1));
      glVertex2f(GLfloat(x1), GLfloat(ycf1));
      glTexCoord2f(GLfloat(0.0), GLfloat(txtycf1));
      glVertex2f(GLfloat(x0), GLfloat(ycf1));
      glEnd();
    }
  }

  void OpenGLDisplay::drawFrame_quality3(Message_LineData **lineBuffers_,
                                         double x0, double y0,
                                         double x1, double y1)
  {
    if (displayParameters.lineShade >= 0.995f) {
      drawFrame_quality2(lineBuffers_, x0, y0, x1, y1);
      return;
    }
    unsigned char lineBuf1[768];
    unsigned char lineBuf2[768];
    unsigned char *curLine_ = &(lineBuf1[0]);
    unsigned char *prvLine_ = &(lineBuf2[0]);
    // full horizontal resolution, interlace (768x576)
    int     cnt = 0;
    for (size_t yc = 0; yc < 590; yc++) {
      bool    haveLineDataInPrvLine = false;
      bool    haveLineDataInCurLine = false;
      bool    haveLineDataInNxtLine = false;
      if (yc > 0 && yc < 579)
        haveLineDataInPrvLine = !!(lineBuffers_[yc - 1]);
      if (yc < 578)
        haveLineDataInCurLine = !!(lineBuffers_[yc]);
      if (yc < 577)
        haveLineDataInNxtLine = !!(lineBuffers_[yc + 1]);
      if (haveLineDataInCurLine | haveLineDataInNxtLine) {
        unsigned char *tmp = curLine_;
        curLine_ = prvLine_;
        prvLine_ = tmp;
        // decode video data
        const unsigned char *bufp = (unsigned char *) 0;
        size_t  nBytes = 0;
        if (haveLineDataInCurLine)
          lineBuffers_[yc]->getLineData(bufp, nBytes);
        else
          lineBuffers_[yc + 1]->getLineData(bufp, nBytes);
        decodeLine(curLine_, bufp, nBytes);
      }
      // build 16-bit texture
      uint16_t  *txtp = &(textureBuffer[(yc & 15) * 768]);
      if (haveLineDataInCurLine) {
        for (size_t xc = 0; xc < 768; xc++)
          txtp[xc] = colormap(curLine_[xc]);
      }
      else if (haveLineDataInPrvLine && haveLineDataInNxtLine) {
        for (size_t xc = 0; xc < 768; xc++)
          txtp[xc] = colormap(prvLine_[xc], curLine_[xc]);
      }
      else if (haveLineDataInPrvLine) {
        for (size_t xc = 0; xc < 768; xc++)
          txtp[xc] = colormap(curLine_[xc], 0);
      }
      else if (haveLineDataInNxtLine) {
        for (size_t xc = 0; xc < 768; xc++)
          txtp[xc] = colormap(0, curLine_[xc]);
      }
      else {
        for (size_t xc = 0; xc < 768; xc++)
          txtp[xc] = colormap(0);
      }
      if (++cnt == 16) {
        cnt = 2;
        // load texture
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 768, 16,
                        GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                        (GLvoid *) textureBuffer);
        // update display
        double  ycf0 = y0 + ((double(int(yc - 15)) * (1.0 / 576.0))
                             * (y1 - y0));
        double  ycf1 = y0 + ((double(int(yc - 1)) * (1.0 / 576.0))
                             * (y1 - y0));
        double  txtycf0 = double(int((yc - 14) & 15)) * (1.0 / 16.0);
        double  txtycf1 = txtycf0 + (14.0 / 16.0);
        if (yc == 589) {
          ycf1 -= ((y1 - y0) * (12.0 / 576.0));
          txtycf1 -= (12.0 / 16.0);
        }
        glBegin(GL_QUADS);
        glTexCoord2f(GLfloat(0.0), GLfloat(txtycf0));
        glVertex2f(GLfloat(x0), GLfloat(ycf0));
        glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(txtycf0));
        glVertex2f(GLfloat(x1), GLfloat(ycf0));
        glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(txtycf1));
        glVertex2f(GLfloat(x1), GLfloat(ycf1));
        glTexCoord2f(GLfloat(0.0), GLfloat(txtycf1));
        glVertex2f(GLfloat(x0), GLfloat(ycf1));
        glEnd();
      }
    }
  }

  void OpenGLDisplay::fltkIdleCallback(void *userData_)
  {
    (void) userData_;
    Fl::unlock();
    Timer::wait(0.000001);
    Fl::lock();
  }

  void OpenGLDisplay::displayFrame()
  {
    glViewport(0, 0, GLsizei(this->w()), GLsizei(this->h()));
    glPushMatrix();
    glOrtho(0.0, 1.0, 1.0, 0.0, 0.0, 1.0);

    double  x0, y0, x1, y1;
    double  aspectScale = (768.0 / 576.0)
                          / ((double(this->w()) / double(this->h()))
                             * double(displayParameters.pixelAspectRatio));
    x0 = 0.0;
    y0 = 0.0;
    x1 = 1.0;
    y1 = 1.0;
    if (aspectScale > 1.0001) {
      y0 = 0.5 - (0.5 / aspectScale);
      y1 = 1.0 - y0;
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_BLEND);
      glColor4f(GLfloat(0), GLfloat(0), GLfloat(0), GLfloat(1));
      glBegin(GL_QUADS);
      glVertex2f(GLfloat(0.0), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(y0));
      glVertex2f(GLfloat(0.0), GLfloat(y0));
      glVertex2f(GLfloat(0.0), GLfloat(y1));
      glVertex2f(GLfloat(1.0), GLfloat(y1));
      glVertex2f(GLfloat(1.0), GLfloat(1.0));
      glVertex2f(GLfloat(0.0), GLfloat(1.0));
      glEnd();
    }
    else if (aspectScale < 0.9999) {
      x0 = 0.5 - (0.5 * aspectScale);
      x1 = 1.0 - x0;
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_BLEND);
      glColor4f(GLfloat(0), GLfloat(0), GLfloat(0), GLfloat(1));
      glBegin(GL_QUADS);
      glVertex2f(GLfloat(0.0), GLfloat(0.0));
      glVertex2f(GLfloat(x0), GLfloat(0.0));
      glVertex2f(GLfloat(x0), GLfloat(1.0));
      glVertex2f(GLfloat(0.0), GLfloat(1.0));
      glVertex2f(GLfloat(x1), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(1.0));
      glVertex2f(GLfloat(x1), GLfloat(1.0));
      glEnd();
    }
    if (x0 >= x1 || y0 >= y1)
      return;

    GLuint  textureID_ = GLuint(textureID);
    GLint   savedTextureID = 0;
    glEnable(GL_TEXTURE_2D);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTextureID);
    glBindTexture(GL_TEXTURE_2D, textureID_);
    setTextureParameters(displayParameters.displayQuality);
#ifdef WIN32
    if (!glBlendColor__)
      glBlendColor__ = (PFNGLBLENDCOLORPROC) wglGetProcAddress("glBlendColor");
#endif

    if (displayParameters.displayQuality == 0 &&
        displayParameters.bufferingMode == 0) {
      // full horizontal resolution, no interlace (768x288)
      // no texture filtering or effects
      glDisable(GL_BLEND);
      unsigned char lineBuf1[768];
      unsigned char *curLine_ = &(lineBuf1[0]);
      for (size_t yc = 0; yc < 288; yc += 8) {
        size_t  offs;
        // quality=0 with single buffered display is special case: only those
        // lines are updated that have changed since the last frame
        for (offs = 0; offs < 8; offs++) {
          if (linesChanged[yc + offs + 1])
            break;
        }
        if (offs == 8)
          continue;
        for (offs = 0; offs < 8; offs++) {
          linesChanged[yc + offs + 1] = false;
          // decode video data
          const unsigned char *bufp = (unsigned char *) 0;
          size_t  nBytes = 0;
          size_t  lineNum = (yc + offs + 1) << 1;
          if (lineBuffers[lineNum + 0] != (Message_LineData *) 0) {
            lineBuffers[lineNum + 0]->getLineData(bufp, nBytes);
            decodeLine(curLine_, bufp, nBytes);
          }
          else if (lineBuffers[lineNum + 1] != (Message_LineData *) 0) {
            lineBuffers[lineNum + 1]->getLineData(bufp, nBytes);
            decodeLine(curLine_, bufp, nBytes);
          }
          else
            std::memset(curLine_, 0, 768);
          // build 16-bit texture:
          // full horizontal resolution, no interlace (768x8)
          uint16_t  *txtp = &(textureBuffer[offs * 768]);
          for (size_t xc = 0; xc < 768; xc++)
            txtp[xc] = colormap(curLine_[xc]);
        }
        // load texture
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 768, 8,
                        GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                        (GLvoid *) textureBuffer);
        // update display
        double  ycf0 = y0 + ((double(int(yc << 1)) * (1.0 / 576.0))
                             * (y1 - y0));
        double  ycf1 = y0 + ((double(int(yc << 1) + 16) * (1.0 / 576.0))
                             * (y1 - y0));
        glBegin(GL_QUADS);
        glTexCoord2f(GLfloat(0.0), GLfloat(0.001 / 8.0));
        glVertex2f(GLfloat(x0), GLfloat(ycf0));
        glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(0.001 / 8.0));
        glVertex2f(GLfloat(x1), GLfloat(ycf0));
        glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(7.999 / 8.0));
        glVertex2f(GLfloat(x1), GLfloat(ycf1));
        glTexCoord2f(GLfloat(0.0), GLfloat(7.999 / 8.0));
        glVertex2f(GLfloat(x0), GLfloat(ycf1));
        glEnd();
      }
      // clean up
      glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
      glPopMatrix();
      glFlush();
      // make sure that all lines are updated at a slow rate
      if (forceUpdateLineMask) {
        if (!screenshotCallbackCnt) {
          for (size_t yc = 0; yc < 289; yc++) {
            if (!(forceUpdateLineMask & (uint8_t(1) << uint8_t((yc >> 3) & 7))))
              continue;
            for (size_t tmp = (yc << 1); tmp < ((yc + 1) << 1); tmp++) {
              if (lineBuffers[tmp] != (Message_LineData *) 0) {
                Message *m = lineBuffers[tmp];
                lineBuffers[tmp] = (Message_LineData *) 0;
                deleteMessage(m);
              }
            }
            linesChanged[yc] = true;
          }
          forceUpdateLineMask = 0;
        }
      }
    }
    else if (displayParameters.bufferingMode != 2) {
      float   blendScale3 = displayParameters.motionBlur;
      float   blendScale2 = (1.0f - blendScale3) * displayParameters.blendScale;
      if (!(displayParameters.bufferingMode != 0 ||
            (blendScale2 > 0.99f && blendScale2 < 1.01f &&
             blendScale3 < 0.01f))) {
        glEnable(GL_BLEND);
        glBlendColor_(GLclampf(blendScale2),
                      GLclampf(blendScale2),
                      GLclampf(blendScale2),
                      GLclampf(1.0f - blendScale3));
        glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_ALPHA);
      }
      else
        glDisable(GL_BLEND);
      switch (displayParameters.displayQuality) {
      case 0:
        // full horizontal resolution, no interlace (768x288)
        // no texture filtering or effects
        drawFrame_quality0(lineBuffers, x0, y0, x1, y1);
        break;
      case 1:
        // half horizontal resolution, no interlace (384x288)
        drawFrame_quality1(lineBuffers, x0, y0, x1, y1);
        break;
      case 2:
        // full horizontal resolution, no interlace (768x288)
        drawFrame_quality2(lineBuffers, x0, y0, x1, y1);
        break;
      case 3:
        // full horizontal resolution, interlace (768x576)
        drawFrame_quality3(lineBuffers, x0, y0, x1, y1);
        break;
      }
      // clean up
      glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
      glPopMatrix();
      glFlush();
      if (!screenshotCallbackCnt) {
        for (size_t n = 0; n < 578; n++) {
          if (lineBuffers[n] != (Message_LineData *) 0) {
            Message *m = lineBuffers[n];
            lineBuffers[n] = (Message_LineData *) 0;
            deleteMessage(m);
          }
        }
      }
    }
    else {
      // resample video input to monitor refresh rate
      int     readPosInt = int(ringBufferReadPos);
      double  readPosFrac = ringBufferReadPos - double(readPosInt);
      double  d = inputFrameRate / displayFrameRate;
      d = (d > 0.01 ? (d < 1.75 ? d : 1.75) : 0.01);
      switch ((ringBufferWritePos - readPosInt) & 3) {
      case 1:
        d = 0.0;
        readPosFrac = 0.0;
        break;
      case 2:
        d = d * 0.97;
        break;
      case 3:
        d = d * 1.04;
        break;
      case 0:
        d = d * 1.25;
        break;
      }
      ringBufferReadPos = ringBufferReadPos + d;
      if (ringBufferReadPos >= 4.0)
        ringBufferReadPos -= 4.0;
      if (readPosFrac <= 0.998) {
        glDisable(GL_BLEND);
        switch (displayParameters.displayQuality) {
        case 0:
          // full horizontal resolution, no interlace (768x288)
          // no texture filtering or effects
          drawFrame_quality0(frameRingBuffer[readPosInt & 3], x0, y0, x1, y1);
          break;
        case 1:
          // half horizontal resolution, no interlace (384x288)
          drawFrame_quality1(frameRingBuffer[readPosInt & 3], x0, y0, x1, y1);
          break;
        case 2:
          // full horizontal resolution, no interlace (768x288)
          drawFrame_quality2(frameRingBuffer[readPosInt & 3], x0, y0, x1, y1);
          break;
        case 3:
          // full horizontal resolution, interlace (768x576)
          drawFrame_quality3(frameRingBuffer[readPosInt & 3], x0, y0, x1, y1);
          break;
        }
      }
      if (readPosFrac >= 0.002) {
        if (readPosFrac <= 0.9975) {
          glEnable(GL_BLEND);
          glBlendColor_(GLclampf(1.0), GLclampf(1.0), GLclampf(1.0),
                        GLclampf(readPosFrac));
          glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
        }
        else
          glDisable(GL_BLEND);
        switch (displayParameters.displayQuality) {
        case 0:
          // full horizontal resolution, no interlace (768x288)
          // no texture filtering or effects
          drawFrame_quality0(frameRingBuffer[(readPosInt + 1) & 3],
                             x0, y0, x1, y1);
          break;
        case 1:
          // half horizontal resolution, no interlace (384x288)
          drawFrame_quality1(frameRingBuffer[(readPosInt + 1) & 3],
                             x0, y0, x1, y1);
          break;
        case 2:
          // full horizontal resolution, no interlace (768x288)
          drawFrame_quality2(frameRingBuffer[(readPosInt + 1) & 3],
                             x0, y0, x1, y1);
          break;
        case 3:
          // full horizontal resolution, interlace (768x576)
          drawFrame_quality3(frameRingBuffer[(readPosInt + 1) & 3],
                             x0, y0, x1, y1);
          break;
        }
      }
      // clean up
      glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
      glPopMatrix();
      glFlush();
    }

    messageQueueMutex.lock();
    framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
    messageQueueMutex.unlock();
  }

  void OpenGLDisplay::draw()
  {
    if (!textureID) {
      glViewport(0, 0, GLsizei(this->w()), GLsizei(this->h()));
      glPushMatrix();
      glOrtho(0.0, 1.0, 1.0, 0.0, 0.0, 1.0);
      // on first call: initialize texture
      glEnable(GL_TEXTURE_2D);
      GLuint  tmp = 0;
      glGenTextures(1, &tmp);
      textureID = (unsigned long) tmp;
      GLint   savedTextureID;
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTextureID);
      glBindTexture(GL_TEXTURE_2D, tmp);
      setTextureParameters(displayParameters.displayQuality);
      initializeTexture(displayParameters, textureBuffer);
      glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
      // clear display
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_BLEND);
      glColor4f(GLfloat(0), GLfloat(0), GLfloat(0), GLfloat(1));
      glBegin(GL_QUADS);
      glVertex2f(GLfloat(0.0), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(1.0));
      glVertex2f(GLfloat(0.0), GLfloat(1.0));
      glEnd();
      glPopMatrix();
    }
    if (redrawFlag || videoResampleEnabled) {
      redrawFlag = false;
      displayFrame();
      if (videoResampleEnabled) {
        double  t = displayFrameRateTimer.getRealTime();
        displayFrameRateTimer.reset();
        t = (t > 0.002 ? (t < 0.25 ? t : 0.25) : 0.002);
        displayFrameRate = 1.0 / ((0.97 / displayFrameRate) + (0.03 * t));
      }
    }
  }

  bool OpenGLDisplay::checkEvents()
  {
    threadLock.notify();
    while (true) {
      messageQueueMutex.lock();
      Message *m = messageQueue;
      if (m) {
        messageQueue = m->nxt;
        if (messageQueue) {
          messageQueue->prv = (Message *) 0;
          if (!messageQueue->nxt)
            lastMessage = messageQueue;
        }
        else
          lastMessage = (Message *) 0;
      }
      messageQueueMutex.unlock();
      if (!m)
        break;
      if (typeid(*m) == typeid(Message_LineData)) {
        Message_LineData  *msg;
        msg = static_cast<Message_LineData *>(m);
        if (msg->lineNum >= 0 && msg->lineNum < 578) {
          if (displayParameters.displayQuality == 0) {
            msg->lineNum = msg->lineNum & (~(int(1)));
            if (!displayParameters.bufferingMode) {
              // check if this line has changed
              if (lineBuffers[msg->lineNum] != (Message_LineData *) 0) {
                if (*(lineBuffers[msg->lineNum]) == *msg) {
                  deleteMessage(m);
                  continue;
                }
              }
              linesChanged[msg->lineNum >> 1] = true;
            }
          }
          if (lineBuffers[msg->lineNum] != (Message_LineData *) 0)
            deleteMessage(lineBuffers[msg->lineNum]);
          lineBuffers[msg->lineNum] = msg;
          continue;
        }
      }
      else if (typeid(*m) == typeid(Message_FrameDone)) {
        // need to update display
        if (redrawFlag) {
          // lost a frame
          messageQueueMutex.lock();
          framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
          messageQueueMutex.unlock();
        }
        redrawFlag = true;
        deleteMessage(m);
        noInputTimer.reset();
        if (screenshotCallbackCnt) {
          checkScreenshotCallback();
        }
        else if (videoResampleEnabled) {
          double  t = inputFrameRateTimer.getRealTime();
          inputFrameRateTimer.reset();
          t = (t > 0.002 ? (t < 0.25 ? t : 0.25) : 0.002);
          inputFrameRate = 1.0 / ((0.97 / inputFrameRate) + (0.03 * t));
          if (ringBufferWritePos != int(ringBufferReadPos)) {
            // if buffer is not already full, copy current frame
            for (size_t yc = 0; yc < 578; yc++) {
              if (frameRingBuffer[ringBufferWritePos][yc])
                deleteMessage(frameRingBuffer[ringBufferWritePos][yc]);
              frameRingBuffer[ringBufferWritePos][yc] = lineBuffers[yc];
              lineBuffers[yc] = (Message_LineData *) 0;
            }
            ringBufferWritePos = (ringBufferWritePos + 1) & 3;
            continue;
          }
        }
        break;
      }
      else if (typeid(*m) == typeid(Message_SetParameters)) {
        Message_SetParameters *msg;
        msg = static_cast<Message_SetParameters *>(m);
        if (displayParameters.displayQuality != msg->dp.displayQuality ||
            displayParameters.bufferingMode != msg->dp.bufferingMode) {
          Fl::remove_idle(&fltkIdleCallback, (void *) this);
          if (displayParameters.bufferingMode != msg->dp.bufferingMode) {
#ifdef WIN32
            glBlendColor__ = (PFNGLBLENDCOLORPROC) 0;
#endif
            // if double buffering mode has changed, also need to generate
            // a new texture ID
            GLuint  oldTextureID = GLuint(textureID);
            textureID = 0UL;
            if (oldTextureID)
              glDeleteTextures(1, &oldTextureID);
            this->mode(FL_RGB
                       | (msg->dp.bufferingMode != 0 ? FL_DOUBLE : FL_SINGLE));
            if (oldTextureID) {
              oldTextureID = 0U;
              glGenTextures(1, &oldTextureID);
              textureID = (unsigned long) oldTextureID;
            }
            Fl::focus(this);
          }
          if (msg->dp.bufferingMode == 2) {
            videoResampleEnabled = true;
            Fl::add_idle(&fltkIdleCallback, (void *) this);
            displayFrameRateTimer.reset();
            inputFrameRateTimer.reset();
          }
          else {
            videoResampleEnabled = false;
            for (size_t n = 0; n < 4; n++) {
              for (size_t yc = 0; yc < 578; yc++) {
                Message *m_ = frameRingBuffer[n][yc];
                if (m_) {
                  frameRingBuffer[n][yc] = (Message_LineData *) 0;
                  m_->~Message();
                  std::free(m_);
                }
              }
            }
          }
          // reset texture
          std::memset(textureBuffer, 0, sizeof(uint16_t) * 1024 * 16);
          glEnable(GL_TEXTURE_2D);
          GLint   savedTextureID;
          glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTextureID);
          glBindTexture(GL_TEXTURE_2D, GLuint(textureID));
          setTextureParameters(msg->dp.displayQuality);
          initializeTexture(msg->dp, textureBuffer);
          glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
        }
        displayParameters = msg->dp;
        if (displayParameters.displayQuality > 1)
          colormap.setParams(displayParameters);
        else {
          DisplayParameters tmp_dp(displayParameters);
          tmp_dp.lineShade = 1.0f;
          colormap.setParams(tmp_dp);
        }
        for (size_t yc = 0; yc < 289; yc++)
          linesChanged[yc] = true;
      }
      deleteMessage(m);
    }
    if (noInputTimer.getRealTime() > 1.0) {
      noInputTimer.reset(0.45);
      if (redrawFlag) {
        // lost a frame
        messageQueueMutex.lock();
        framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
        messageQueueMutex.unlock();
      }
      if (videoResampleEnabled) {
        for (size_t yc = 0; yc < 578; yc++) {
          Message *m = frameRingBuffer[ringBufferWritePos][yc];
          if (m) {
            frameRingBuffer[ringBufferWritePos][yc] = (Message_LineData *) 0;
            deleteMessage(m);
          }
        }
        ringBufferWritePos = (ringBufferWritePos + 1) & 3;
      }
      redrawFlag = true;
      if (screenshotCallbackCnt)
        checkScreenshotCallback();
    }
    if (this->damage() & FL_DAMAGE_EXPOSE) {
      forceUpdateLineMask = 0xFF;
      forceUpdateLineCnt = 0;
      forceUpdateTimer.reset();
    }
    else if (forceUpdateTimer.getRealTime() >= 0.085) {
      forceUpdateLineMask |= (uint8_t(1) << forceUpdateLineCnt);
      forceUpdateLineCnt++;
      forceUpdateLineCnt &= uint8_t(7);
      forceUpdateTimer.reset();
    }
    return (redrawFlag | videoResampleEnabled);
  }

  int OpenGLDisplay::handle(int event)
  {
    return fltkEventCallback(fltkEventCallbackUserData, event);
  }

}       // namespace Ep128Emu

