
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

#include "gldisp.hpp"

#ifdef WIN32
#  undef WIN32
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  define WIN32 1
#endif

#ifdef WIN32
#  include <wingdi.h>
#endif

static void decodeLine(unsigned char *outBuf,
                       const unsigned char *inBuf, size_t nBytes)
{
  const unsigned char *bufp = inBuf;

  for (size_t i = 0; i < 768; i += 16) {
    unsigned char c = *(bufp++);
    switch (c) {
    case 0x01:
      outBuf[i + 15] = outBuf[i + 14] =
      outBuf[i + 13] = outBuf[i + 12] =
      outBuf[i + 11] = outBuf[i + 10] =
      outBuf[i +  9] = outBuf[i +  8] =
      outBuf[i +  7] = outBuf[i +  6] =
      outBuf[i +  5] = outBuf[i +  4] =
      outBuf[i +  3] = outBuf[i +  2] =
      outBuf[i +  1] = outBuf[i +  0] = *(bufp++);
      break;
    case 0x02:
      outBuf[i +  7] = outBuf[i +  6] =
      outBuf[i +  5] = outBuf[i +  4] =
      outBuf[i +  3] = outBuf[i +  2] =
      outBuf[i +  1] = outBuf[i +  0] = *(bufp++);
      outBuf[i + 15] = outBuf[i + 14] =
      outBuf[i + 13] = outBuf[i + 12] =
      outBuf[i + 11] = outBuf[i + 10] =
      outBuf[i +  9] = outBuf[i +  8] = *(bufp++);
      break;
    case 0x03:
      {
        unsigned char c0 = *(bufp++);
        unsigned char c1 = *(bufp++);
        unsigned char b = *(bufp++);
        outBuf[i +  1] = outBuf[i +  0] = ((b & 128) ? c1 : c0);
        outBuf[i +  3] = outBuf[i +  2] = ((b &  64) ? c1 : c0);
        outBuf[i +  5] = outBuf[i +  4] = ((b &  32) ? c1 : c0);
        outBuf[i +  7] = outBuf[i +  6] = ((b &  16) ? c1 : c0);
        outBuf[i +  9] = outBuf[i +  8] = ((b &   8) ? c1 : c0);
        outBuf[i + 11] = outBuf[i + 10] = ((b &   4) ? c1 : c0);
        outBuf[i + 13] = outBuf[i + 12] = ((b &   2) ? c1 : c0);
        outBuf[i + 15] = outBuf[i + 14] = ((b &   1) ? c1 : c0);
      }
      break;
    case 0x04:
      outBuf[i +  3] = outBuf[i +  2] =
      outBuf[i +  1] = outBuf[i +  0] = *(bufp++);
      outBuf[i +  7] = outBuf[i +  6] =
      outBuf[i +  5] = outBuf[i +  4] = *(bufp++);
      outBuf[i + 11] = outBuf[i + 10] =
      outBuf[i +  9] = outBuf[i +  8] = *(bufp++);
      outBuf[i + 15] = outBuf[i + 14] =
      outBuf[i + 13] = outBuf[i + 12] = *(bufp++);
      break;
    case 0x06:
      {
        unsigned char c0 = *(bufp++);
        unsigned char c1 = *(bufp++);
        unsigned char b = *(bufp++);
        outBuf[i +  0] = ((b & 128) ? c1 : c0);
        outBuf[i +  1] = ((b &  64) ? c1 : c0);
        outBuf[i +  2] = ((b &  32) ? c1 : c0);
        outBuf[i +  3] = ((b &  16) ? c1 : c0);
        outBuf[i +  4] = ((b &   8) ? c1 : c0);
        outBuf[i +  5] = ((b &   4) ? c1 : c0);
        outBuf[i +  6] = ((b &   2) ? c1 : c0);
        outBuf[i +  7] = ((b &   1) ? c1 : c0);
        c0 = *(bufp++);
        c1 = *(bufp++);
        b = *(bufp++);
        outBuf[i +  8] = ((b & 128) ? c1 : c0);
        outBuf[i +  9] = ((b &  64) ? c1 : c0);
        outBuf[i + 10] = ((b &  32) ? c1 : c0);
        outBuf[i + 11] = ((b &  16) ? c1 : c0);
        outBuf[i + 12] = ((b &   8) ? c1 : c0);
        outBuf[i + 13] = ((b &   4) ? c1 : c0);
        outBuf[i + 14] = ((b &   2) ? c1 : c0);
        outBuf[i + 15] = ((b &   1) ? c1 : c0);
      }
      break;
    case 0x08:
      outBuf[i +  1] = outBuf[i +  0] = *(bufp++);
      outBuf[i +  3] = outBuf[i +  2] = *(bufp++);
      outBuf[i +  5] = outBuf[i +  4] = *(bufp++);
      outBuf[i +  7] = outBuf[i +  6] = *(bufp++);
      outBuf[i +  9] = outBuf[i +  8] = *(bufp++);
      outBuf[i + 11] = outBuf[i + 10] = *(bufp++);
      outBuf[i + 13] = outBuf[i + 12] = *(bufp++);
      outBuf[i + 15] = outBuf[i + 14] = *(bufp++);
      break;
    case 0x10:
      outBuf[i +  0] = *(bufp++);
      outBuf[i +  1] = *(bufp++);
      outBuf[i +  2] = *(bufp++);
      outBuf[i +  3] = *(bufp++);
      outBuf[i +  4] = *(bufp++);
      outBuf[i +  5] = *(bufp++);
      outBuf[i +  6] = *(bufp++);
      outBuf[i +  7] = *(bufp++);
      outBuf[i +  8] = *(bufp++);
      outBuf[i +  9] = *(bufp++);
      outBuf[i + 10] = *(bufp++);
      outBuf[i + 11] = *(bufp++);
      outBuf[i + 12] = *(bufp++);
      outBuf[i + 13] = *(bufp++);
      outBuf[i + 14] = *(bufp++);
      outBuf[i + 15] = *(bufp++);
      break;
    default:
      outBuf[i + 15] = outBuf[i + 14] =
      outBuf[i + 13] = outBuf[i + 12] =
      outBuf[i + 11] = outBuf[i + 10] =
      outBuf[i +  9] = outBuf[i +  8] =
      outBuf[i +  7] = outBuf[i +  6] =
      outBuf[i +  5] = outBuf[i +  4] =
      outBuf[i +  3] = outBuf[i +  2] =
      outBuf[i +  1] = outBuf[i +  0] = 0;
      break;
    }
  }

  (void) nBytes;
#if 0
  if (size_t(bufp - inBuf) != nBytes)
    throw std::exception();
#endif
}

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
    for (size_t i = 0; i < 256; i++) {
      for (size_t j = 0; j < 256; j++) {
        double  r = (rTbl[i] + rTbl[j]) * dp.blendScale1;
        double  g = (gTbl[i] + gTbl[j]) * dp.blendScale1;
        double  b = (bTbl[i] + bTbl[j]) * dp.blendScale1;
        palette2[(i << 8) + j] = pixelConv(r, g, b);
      }
    }
  }

  uint16_t OpenGLDisplay::Colormap::pixelConv(double r, double g, double b)
  {
    unsigned int  ri, gi, bi;
    ri = (r > 0.0 ? (r < 1.0 ? (unsigned int) (r * 31.0 + 0.5) : 31U) : 0U);
    gi = (g > 0.0 ? (g < 1.0 ? (unsigned int) (g * 63.0 + 0.5) : 63U) : 0U);
    bi = (b > 0.0 ? (b < 1.0 ? (unsigned int) (b * 31.0 + 0.5) : 31U) : 0U);
    return uint16_t((ri << 11) + (gi << 5) + bi);
  }

  // --------------------------------------------------------------------------

  OpenGLDisplay::Message::~Message()
  {
  }

  OpenGLDisplay::Message_LineData::~Message_LineData()
  {
  }

  void OpenGLDisplay::Message_LineData::copyLine(const uint8_t *buf,
                                                 size_t nBytes)
  {
    (void) nBytes;
    const uint8_t *bufp = buf;
    unsigned char *p = reinterpret_cast<unsigned char *>(&(buf_[0]));
    for (size_t i = 0; i < 48; i++) {
      unsigned char n = *(bufp++);
      *(p++) = n;
      do {
        *(p++) = *(bufp++);
      } while (--n);
    }
    nBytes_ = size_t(p - reinterpret_cast<unsigned char *>(&(buf_[0])));
    for (size_t i = nBytes_; (i & 3) != 0; i++)
      *(p++) = 0;
  }

  OpenGLDisplay::Message_FrameDone::~Message_FrameDone()
  {
  }

  OpenGLDisplay::Message_SetParameters::~Message_SetParameters()
  {
  }

  void OpenGLDisplay::deleteMessage(Message *m)
  {
    m->~Message();
    m->prv = (Message *) 0;
    messageQueueMutex.lock();
    m->nxt = freeMessageStack;
    if (freeMessageStack)
      freeMessageStack->prv = m;
    freeMessageStack = m;
    messageQueueMutex.unlock();
  }

  void OpenGLDisplay::queueMessage(Message *m)
  {
    messageQueueMutex.lock();
    if (exitFlag) {
      messageQueueMutex.unlock();
      m->~Message();
      std::free(m);
      return;
    }
    m->prv = lastMessage;
    m->nxt = (Message *) 0;
    if (lastMessage)
      lastMessage->nxt = m;
    else
      messageQueue = m;
    lastMessage = m;
    messageQueueMutex.unlock();
    if (typeid(*m) == typeid(Message_FrameDone)) {
      Fl::awake();
      threadLock.wait(1);
    }
  }

  // --------------------------------------------------------------------------

  OpenGLDisplay::OpenGLDisplay(int xx, int yy, int ww, int hh,
                               const char *lbl, bool isDoubleBuffered)
    : Fl_Gl_Window(xx, yy, ww, hh, lbl),
      messageQueue((Message *) 0),
      lastMessage((Message *) 0),
      freeMessageStack((Message *) 0),
      messageQueueMutex(),
      colormap(),
      lineBuffers((Message_LineData **) 0),
      linesChanged((bool *) 0),
      textureBuffer((uint16_t *) 0),
      textureID(0UL),
      curLine(0),
      lineCnt(0),
      prvLineCnt(0),
      framesPending(0),
      skippingFrame(false),
      vsyncState(false),
      displayParameters(),
      savedDisplayParameters(),
      exitFlag(false),
      forceUpdateLineCnt(0),
      forceUpdateLineMask(0),
      redrawFlag(false)
  {
    try {
      lineBuffers = new Message_LineData*[578];
      for (size_t n = 0; n < 578; n++)
        lineBuffers[n] = (Message_LineData *) 0;
      linesChanged = new bool[289];
      for (size_t n = 0; n < 289; n++)
        linesChanged[n] = false;
      textureBuffer = new uint16_t[1024 * 16];  // max. texture size = 1024x16
      std::memset(textureBuffer, 0, sizeof(uint16_t) * 1024 * 16);
    }
    catch (...) {
      if (lineBuffers)
        delete[] lineBuffers;
      if (linesChanged)
        delete[] linesChanged;
      if (textureBuffer)
        delete[] textureBuffer;
      throw;
    }
    displayParameters.useDoubleBuffering = isDoubleBuffered;
    savedDisplayParameters.useDoubleBuffering = isDoubleBuffered;
    this->mode(FL_RGB | (isDoubleBuffered ? FL_DOUBLE : FL_SINGLE));
  }

  OpenGLDisplay::~OpenGLDisplay()
  {
    messageQueueMutex.lock();
    exitFlag = true;
    while (freeMessageStack) {
      Message *m = freeMessageStack;
      freeMessageStack = m->nxt;
      if (freeMessageStack)
        freeMessageStack->prv = (Message *) 0;
      std::free(m);
    }
    while (messageQueue) {
      Message *m = messageQueue;
      messageQueue = m->nxt;
      if (messageQueue)
        messageQueue->prv = (Message *) 0;
      m->~Message();
      std::free(m);
    }
    lastMessage = (Message *) 0;
    messageQueueMutex.unlock();
    if (textureID) {
      GLuint  tmp = GLuint(textureID);
      textureID = 0UL;
      glDeleteTextures(1, &tmp);
    }
    delete[] textureBuffer;
    delete[] linesChanged;
    for (size_t n = 0; n < 578; n++) {
      Message *m = lineBuffers[n];
      if (m) {
        lineBuffers[n] = (Message_LineData *) 0;
        m->~Message();
        std::free(m);
      }
    }
    delete[] lineBuffers;
  }

  void OpenGLDisplay::displayFrame()
  {
    glViewport(0, 0, GLsizei(this->w()), GLsizei(this->h()));
    glPushMatrix();
    glOrtho(0.0, 1.0, 1.0, 0.0, 0.0, 1.0);

    double  x0, y0, x1, y1;
    double  aspectScale = (768.0 / 576.0)
                          / ((double(this->w()) / double(this->h()))
                             * displayParameters.pixelAspectRatio);
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

    unsigned char lineBuf1[768];
    unsigned char *curLine_ = &(lineBuf1[0]);
    GLuint  textureID_ = GLuint(textureID);
    GLint   savedTextureID = 0;
    glEnable(GL_TEXTURE_2D);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTextureID);
    glBindTexture(GL_TEXTURE_2D, textureID_);
    setTextureParameters(displayParameters.displayQuality);
    if (displayParameters.displayQuality == 0) {
      // full horizontal resolution, no interlace (768x288)
      // no texture filtering or effects
      glDisable(GL_BLEND);
      for (size_t yc = 0; yc < 288; yc += 8) {
        size_t  offs;
        if (!displayParameters.useDoubleBuffering) {
          // quality=0 with single buffered display is special case: only those
          // lines are updated that have changed since the last frame
          for (offs = 0; offs < 8; offs++) {
            if (linesChanged[yc + offs])
              break;
          }
          if (offs == 8)
            continue;
        }
        for (offs = 0; offs < 8; offs++) {
          linesChanged[yc + offs] = false;
          // decode video data
          const unsigned char *bufp = (unsigned char *) 0;
          size_t  nBytes = 0;
          size_t  lineNum = (yc + offs) << 1;
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
      if (!displayParameters.useDoubleBuffering) {
        if (forceUpdateLineMask) {
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
    else {
      if (!(displayParameters.useDoubleBuffering ||
            (displayParameters.blendScale2 > 0.99 &&
             displayParameters.blendScale3 < 0.01))) {
        glEnable(GL_BLEND);
#ifndef WIN32
        glBlendColor(GLclampf(displayParameters.blendScale2),
                     GLclampf(displayParameters.blendScale2),
                     GLclampf(displayParameters.blendScale2),
                     GLclampf(1.0 - displayParameters.blendScale3));
#else
        void  (*glBlendColor_)(GLclampf, GLclampf, GLclampf, GLclampf) =
            (void (*)(GLclampf, GLclampf, GLclampf, GLclampf))
                wglGetProcAddress("glBlendColor");
        glBlendColor_(GLclampf(displayParameters.blendScale2),
                      GLclampf(displayParameters.blendScale2),
                      GLclampf(displayParameters.blendScale2),
                      GLclampf(1.0 - displayParameters.blendScale3));
#endif
        glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_ALPHA);
      }
      else
        glDisable(GL_BLEND);
      switch (displayParameters.displayQuality) {
      case 1:
        // half horizontal resolution, no interlace (384x288)
        for (size_t yc = 0; yc < 588; yc += 28) {
          for (size_t offs = 0; offs < 32; offs += 2) {
            // decode video data
            const unsigned char *bufp = (unsigned char *) 0;
            size_t  nBytes = 0;
            bool    haveLineData = false;
            if ((yc + offs) < 578) {
              if (lineBuffers[yc + offs] != (Message_LineData *) 0) {
                lineBuffers[yc + offs]->getLineData(bufp, nBytes);
                haveLineData = true;
              }
              else if (lineBuffers[yc + offs + 1] != (Message_LineData *) 0) {
                lineBuffers[yc + offs + 1]->getLineData(bufp, nBytes);
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
        break;
      case 2:
        // full horizontal resolution, no interlace (768x288)
        for (size_t yc = 0; yc < 588; yc += 28) {
          for (size_t offs = 0; offs < 32; offs += 2) {
            // decode video data
            const unsigned char *bufp = (unsigned char *) 0;
            size_t  nBytes = 0;
            bool    haveLineData = false;
            if ((yc + offs) < 578) {
              if (lineBuffers[yc + offs] != (Message_LineData *) 0) {
                lineBuffers[yc + offs]->getLineData(bufp, nBytes);
                haveLineData = true;
              }
              else if (lineBuffers[yc + offs + 1] != (Message_LineData *) 0) {
                lineBuffers[yc + offs + 1]->getLineData(bufp, nBytes);
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
        break;
      case 3:
        // full horizontal resolution, interlace (768x576)
        {
          unsigned char lineBuf2[768];
          unsigned char *prvLine_ = &(lineBuf2[0]);
          int     cnt = 0;
          for (size_t yc = 0; yc < 590; yc++) {
            bool    haveLineDataInPrvLine = false;
            bool    haveLineDataInCurLine = false;
            bool    haveLineDataInNxtLine = false;
            if (yc > 0 && yc < 579)
              haveLineDataInPrvLine = !!(lineBuffers[yc - 1]);
            if (yc < 578)
              haveLineDataInCurLine = !!(lineBuffers[yc]);
            if (yc < 577)
              haveLineDataInNxtLine = !!(lineBuffers[yc + 1]);
            if (haveLineDataInCurLine | haveLineDataInNxtLine) {
              unsigned char *tmp = curLine_;
              curLine_ = prvLine_;
              prvLine_ = tmp;
              // decode video data
              const unsigned char *bufp = (unsigned char *) 0;
              size_t  nBytes = 0;
              if (haveLineDataInCurLine)
                lineBuffers[yc]->getLineData(bufp, nBytes);
              else
                lineBuffers[yc + 1]->getLineData(bufp, nBytes);
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
        break;
      }
      // clean up
      glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
      glPopMatrix();
      glFlush();
      for (size_t n = 0; n < 578; n++) {
        if (lineBuffers[n] != (Message_LineData *) 0) {
          Message *m = lineBuffers[n];
          lineBuffers[n] = (Message_LineData *) 0;
          deleteMessage(m);
        }
      }
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
    if (redrawFlag) {
      redrawFlag = false;
      displayFrame();
      noInputTimer.reset();
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
            if (!displayParameters.useDoubleBuffering) {
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
        break;
      }
      else if (typeid(*m) == typeid(Message_SetParameters)) {
        Message_SetParameters *msg;
        msg = static_cast<Message_SetParameters *>(m);
        if (displayParameters.displayQuality != msg->dp.displayQuality ||
            displayParameters.useDoubleBuffering
            != msg->dp.useDoubleBuffering) {
          if (displayParameters.useDoubleBuffering
              != msg->dp.useDoubleBuffering) {
            // if double buffering mode has changed, also need to generate
            // a new texture ID
            GLuint  oldTextureID = GLuint(textureID);
            textureID = 0UL;
            if (oldTextureID)
              glDeleteTextures(1, &oldTextureID);
            this->mode(FL_RGB
                       | (msg->dp.useDoubleBuffering ? FL_DOUBLE : FL_SINGLE));
            if (oldTextureID) {
              oldTextureID = 0U;
              glGenTextures(1, &oldTextureID);
              textureID = (unsigned long) oldTextureID;
            }
            Fl::focus(this);
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
          tmp_dp.blendScale1 = 0.5;
          colormap.setParams(tmp_dp);
        }
        for (size_t yc = 0; yc < 289; yc++)
          linesChanged[yc] = true;
      }
      deleteMessage(m);
    }
    if (noInputTimer.getRealTime() > 0.33) {
      if (redrawFlag) {
        // lost a frame
        messageQueueMutex.lock();
        framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
        messageQueueMutex.unlock();
      }
      redrawFlag = true;
    }
    if (forceUpdateTimer.getRealTime() >= 0.125) {
      forceUpdateLineMask |= (uint8_t(1) << forceUpdateLineCnt);
      forceUpdateLineCnt++;
      forceUpdateLineCnt &= uint8_t(7);
      forceUpdateTimer.reset();
    }
    return redrawFlag;
  }

  int OpenGLDisplay::handle(int event)
  {
    (void) event;
    return 0;
  }

  void OpenGLDisplay::setDisplayParameters(const DisplayParameters& dp)
  {
    if (dp.displayQuality != savedDisplayParameters.displayQuality ||
        dp.useDoubleBuffering != savedDisplayParameters.useDoubleBuffering) {
      vsyncStateChange(true, 8);
      vsyncStateChange(false, 28);
    }
    Message_SetParameters *m = allocateMessage<Message_SetParameters>();
    m->dp = dp;
    savedDisplayParameters = dp;
    queueMessage(m);
  }

  const VideoDisplay::DisplayParameters&
      OpenGLDisplay::getDisplayParameters() const
  {
    return savedDisplayParameters;
  }

  void OpenGLDisplay::drawLine(const uint8_t *buf, size_t nBytes)
  {
    if (!skippingFrame) {
      if (curLine >= 0 && curLine < 578) {
        Message_LineData  *m = allocateMessage<Message_LineData>();
        m->lineNum = curLine;
        m->copyLine(buf, nBytes);
        queueMessage(m);
      }
    }
    if (lineCnt < 500) {
      curLine += 2;
      lineCnt++;
    }
  }

  void OpenGLDisplay::vsyncStateChange(bool newState,
                                       unsigned int currentSlot_)
  {
    (void) currentSlot_;
    if (newState == vsyncState)
      return;
    vsyncState = newState;
    if (newState) {
      curLine = (savedDisplayParameters.displayQuality == 0 ? 272 : 274)
                - prvLineCnt;
      prvLineCnt = lineCnt;
      lineCnt = 0;
      return;
    }
    messageQueueMutex.lock();
    bool    skippedFrame = skippingFrame;
    if (!skippedFrame)
      framesPending++;
    skippingFrame = (framesPending > 3);    // should this be configurable ?
    messageQueueMutex.unlock();
    if (skippedFrame) {
      Fl::awake();
      threadLock.wait(1);
      return;
    }
    Message *m = allocateMessage<Message_FrameDone>();
    queueMessage(m);
  }

}       // namespace Ep128Emu

