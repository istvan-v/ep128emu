
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

#include "ep128.hpp"
#include "system.hpp"

#include <cmath>
#include <cstring>
#include <typeinfo>

#define GL_GLEXT_PROTOTYPES 1

#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <GL/glext.h>

#include "gldisp.hpp"

static void decodeLine(unsigned char *outBuf,
                       const unsigned char *inBuf, size_t nBytes)
{
  const unsigned char *bufp = inBuf;

  for (size_t i = 0; i < 704; i += 16) {
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
    double  b = dp.b + ((1.0 - dp.c) * 0.5);
    double  rb = dp.rb + ((1.0 - dp.rc) * 0.5);
    double  gb = dp.gb + ((1.0 - dp.gc) * 0.5);
    double  bb = dp.bb + ((1.0 - dp.bc) * 0.5);
    for (size_t i = 0; i < 256; i++) {
      float   r1, g1, b1;
      dp.indexToRGBFunc(uint8_t(i), r1, g1, b1);
      palette[i] =
          pixelConv(std::pow(double(r1), 1.0 / (dp.rg * dp.g))
                    * (dp.rc * dp.c) + (rb + b),
                    std::pow(double(g1), 1.0 / (dp.gg * dp.g))
                    * (dp.gc * dp.c) + (gb + b),
                    std::pow(double(b1), 1.0 / (dp.bg * dp.g))
                    * (dp.bc * dp.c) + (bb + b));
    }
    for (size_t i = 0; i < 256; i++) {
      float   r1, g1, b1;
      dp.indexToRGBFunc(uint8_t(i), r1, g1, b1);
      for (size_t j = 0; j < 256; j++) {
        float   r2, g2, b2;
        dp.indexToRGBFunc(uint8_t(j), r2, g2, b2);
        double  rx = (r1 + r2) * dp.blendScale1;
        double  gx = (g1 + g2) * dp.blendScale1;
        double  bx = (b1 + b2) * dp.blendScale1;
        palette2[(i << 8) + j] =
            pixelConv(std::pow(double(rx), 1.0 / (dp.rg * dp.g))
                      * (dp.rc * dp.c) + (rb + b),
                      std::pow(double(gx), 1.0 / (dp.gg * dp.g))
                      * (dp.gc * dp.c) + (gb + b),
                      std::pow(double(bx), 1.0 / (dp.bg * dp.g))
                      * (dp.bc * dp.c) + (bb + b));
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
    bufp += (int(*bufp) + 1);
    unsigned char *p = reinterpret_cast<unsigned char *>(&(buf_[0]));
    for (size_t i = 0; i < 44; i++) {
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
    m->prv = (Message *) 0;
    messageQueueMutex.lock();
    m->nxt = freeMessageStack;
    if (freeMessageStack)
      freeMessageStack->prv = m;
    freeMessageStack = m;
    messageQueueMutex.unlock();
    m->~Message();
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
    if (typeid(*m) == typeid(Message_FrameDone))
      Fl::awake();
  }

  // --------------------------------------------------------------------------

  OpenGLDisplay::OpenGLDisplay(int xx, int yy, int ww, int hh,
                               const char *lbl)
    : Fl_Gl_Window(xx, yy, ww, hh, lbl),
      messageQueue((Message *) 0),
      lastMessage((Message *) 0),
      freeMessageStack((Message *) 0),
      messageQueueMutex(),
      colormap(),
      lineBuffers((Message_LineData **) 0),
      textureBuffer((uint16_t *) 0),
      textureID(0UL),
      curLine(0),
      lineCnt(0),
      framesPending(0),
      skippingFrame(false),
      displayParameters(),
      savedDisplayParameters(),
      exitFlag(false)
  {
    try {
      lineBuffers = new Message_LineData*[578];
      for (size_t n = 0; n < 578; n++)
        lineBuffers[n] = (Message_LineData *) 0;
      textureBuffer = new uint16_t[1024 * 1024];    // texture size = 1024x1024
      std::memset(textureBuffer, 0, sizeof(uint16_t) * 1024 * 1024);
    }
    catch (...) {
      if (lineBuffers)
        delete[] lineBuffers;
      if (textureBuffer)
        delete[] textureBuffer;
      throw;
    }
    this->mode(FL_RGB | FL_SINGLE);
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
    double  aspectScale = (704.0 / 576.0)
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

    unsigned char lineBuf1[704];
    unsigned char *curLine_ = &(lineBuf1[0]);
    if (displayParameters.displayQuality > 2) {
      // full horizontal resolution, interlace (704x576)
      unsigned char lineBuf2[704];
      unsigned char *prvLine_ = &(lineBuf2[0]);
      for (size_t yc = 0; yc < 578; yc++) {
        if ((yc == 0 && lineBuffers[yc] != (Message_LineData *) 0) ||
            (yc != 577 && (lineBuffers[yc] == (Message_LineData *) 0 &&
                           lineBuffers[yc + 1] != (Message_LineData *) 0))) {
          unsigned char *tmp = curLine_;
          curLine_ = prvLine_;
          prvLine_ = tmp;
          // decode video data
          const unsigned char *bufp = (unsigned char *) 0;
          size_t  nBytes = 0;
          if (yc == 0 && lineBuffers[yc] != (Message_LineData *) 0)
            lineBuffers[yc]->getLineData(bufp, nBytes);
          else
            lineBuffers[yc + 1]->getLineData(bufp, nBytes);
          decodeLine(curLine_, bufp, nBytes);
        }
        // build 16-bit texture
        uint16_t  *txtp = &(textureBuffer[yc * 704]);
        if (lineBuffers[yc] != (Message_LineData *) 0) {
          for (size_t xc = 0; xc < 704; xc++)
            txtp[xc] = colormap(curLine_[xc]);
        }
        else if ((yc != 0 && yc != 577) &&
                 (lineBuffers[yc - 1] != (Message_LineData *) 0 &&
                  lineBuffers[yc + 1] != (Message_LineData *) 0)) {
          for (size_t xc = 0; xc < 704; xc++)
            txtp[xc] = colormap(prvLine_[xc], curLine_[xc]);
        }
        else if (yc != 0 && lineBuffers[yc - 1] != (Message_LineData *) 0) {
          for (size_t xc = 0; xc < 704; xc++)
            txtp[xc] = colormap(curLine_[xc], 0);
        }
        else if (yc != 577 && lineBuffers[yc + 1] != (Message_LineData *) 0) {
          for (size_t xc = 0; xc < 704; xc++)
            txtp[xc] = colormap(0, curLine_[xc]);
        }
        else {
          for (size_t xc = 0; xc < 704; xc++)
            txtp[xc] = colormap(0);
        }
      }
    }
    else {
      for (size_t yc = 0; yc < 289; yc++) {
        // decode video data
        const unsigned char *bufp = (unsigned char *) 0;
        size_t  nBytes = 0;
        if (lineBuffers[(yc << 1) + 0] != (Message_LineData *) 0) {
          lineBuffers[(yc << 1) + 0]->getLineData(bufp, nBytes);
          decodeLine(curLine_, bufp, nBytes);
        }
        else if (lineBuffers[(yc << 1) + 1] != (Message_LineData *) 0) {
          lineBuffers[(yc << 1) + 1]->getLineData(bufp, nBytes);
          decodeLine(curLine_, bufp, nBytes);
        }
        else
          std::memset(curLine_, 0, 704);
        // build 16-bit texture
        if (displayParameters.displayQuality == 2) {
          // full horizontal resolution, no interlace (704x288)
          uint16_t  *txtp = &(textureBuffer[yc * 704]);
          for (size_t xc = 0; xc < 704; xc++)
            txtp[xc] = colormap(curLine_[xc]);
        }
        else {
          // half horizontal resolution, no interlace (352x288)
          uint16_t  *txtp = &(textureBuffer[yc * 352]);
          for (size_t xc = 0; xc < 704; xc += 2)
            txtp[xc >> 1] = colormap(curLine_[xc], curLine_[xc + 1]);
        }
      }
    }

    // load texture
    GLuint  textureID_ = GLuint(textureID);
    GLint   savedTextureID = 0;
    glEnable(GL_TEXTURE_2D);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTextureID);
    glBindTexture(GL_TEXTURE_2D, textureID_);
    setTextureParameters(displayParameters.displayQuality);
    if (displayParameters.displayQuality > 2)
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 704, 578,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                      (GLvoid *) textureBuffer);
    else if (displayParameters.displayQuality < 2)
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 352, 289,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                      (GLvoid *) textureBuffer);
    else
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 704, 289,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                      (GLvoid *) textureBuffer);

    // update display
    glEnable(GL_TEXTURE_2D);
    if (displayParameters.displayQuality > 0) {
      glEnable(GL_BLEND);
      glBlendColor(GLclampf(displayParameters.blendScale2),
                   GLclampf(displayParameters.blendScale2),
                   GLclampf(displayParameters.blendScale2),
                   GLclampf(1.0 - displayParameters.blendScale3));
      glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_ALPHA);
    }
    else
      glDisable(GL_BLEND);
    glBegin(GL_QUADS);
    if (displayParameters.displayQuality > 2) {
      glTexCoord2f(GLfloat(0.0), GLfloat(1.0 / 1024.0));
      glVertex2f(GLfloat(x0), GLfloat(y0));
      glTexCoord2f(GLfloat(704.0 / 1024.0), GLfloat(1.0 / 1024));
      glVertex2f(GLfloat(x1), GLfloat(y0));
      glTexCoord2f(GLfloat(704.0 / 1024.0), GLfloat(577.0 / 1024.0));
      glVertex2f(GLfloat(x1), GLfloat(y1));
      glTexCoord2f(GLfloat(0.0), GLfloat(577.0 / 1024.0));
      glVertex2f(GLfloat(x0), GLfloat(y1));
    }
    else if (displayParameters.displayQuality < 2) {
      glTexCoord2f(GLfloat(0.0), GLfloat(0.0));
      glVertex2f(GLfloat(x0), GLfloat(y0));
      glTexCoord2f(GLfloat(352.0 / 1024.0), GLfloat(0.0));
      glVertex2f(GLfloat(x1), GLfloat(y0));
      glTexCoord2f(GLfloat(352.0 / 1024.0), GLfloat(288.0 / 1024.0));
      glVertex2f(GLfloat(x1), GLfloat(y1));
      glTexCoord2f(GLfloat(0.0), GLfloat(288.0 / 1024.0));
      glVertex2f(GLfloat(x0), GLfloat(y1));
    }
    else {
      glTexCoord2f(GLfloat(0.0), GLfloat(0.0));
      glVertex2f(GLfloat(x0), GLfloat(y0));
      glTexCoord2f(GLfloat(704.0 / 1024.0), GLfloat(0.0));
      glVertex2f(GLfloat(x1), GLfloat(y0));
      glTexCoord2f(GLfloat(704.0 / 1024.0), GLfloat(288.0 / 1024.0));
      glVertex2f(GLfloat(x1), GLfloat(y1));
      glTexCoord2f(GLfloat(0.0), GLfloat(288.0 / 1024.0));
      glVertex2f(GLfloat(x0), GLfloat(y1));
    }
    glEnd();

    // clean up
    glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
    glPopMatrix();
    glFinish();
    for (size_t n = 0; n < 578; n++) {
      if (lineBuffers[n] != (Message_LineData *) 0) {
        Message *m = lineBuffers[n];
        lineBuffers[n] = (Message_LineData *) 0;
        deleteMessage(m);
      }
    }
    messageQueueMutex.lock();
    framesPending--;
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
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0,
                   GL_RGB, GL_UNSIGNED_SHORT_5_6_5, (GLvoid *) textureBuffer);
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
        msg = dynamic_cast<Message_LineData *>(m);
        if (msg->lineNum >= 0 && msg->lineNum < 578) {
          if (lineBuffers[msg->lineNum] != (Message_LineData *) 0)
            deleteMessage(lineBuffers[msg->lineNum]);
          lineBuffers[msg->lineNum] = msg;
          continue;
        }
      }
      else if (typeid(*m) == typeid(Message_FrameDone)) {
        displayFrame();
        // do not display more than one frame per draw() call
        deleteMessage(m);
        break;
      }
      else if (typeid(*m) == typeid(Message_SetParameters)) {
        Message_SetParameters *msg;
        msg = dynamic_cast<Message_SetParameters *>(m);
        if (displayParameters.displayQuality != msg->dp.displayQuality) {
          // reset texture
          std::memset(textureBuffer, 0, sizeof(uint16_t) * 1024 * 1024);
          glEnable(GL_TEXTURE_2D);
          GLint   savedTextureID;
          glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTextureID);
          glBindTexture(GL_TEXTURE_2D, GLuint(textureID));
          setTextureParameters(msg->dp.displayQuality);
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0,
                       GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                       (GLvoid *) textureBuffer);
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
      }
      deleteMessage(m);
    }
  }

  int OpenGLDisplay::handle(int event)
  {
    (void) event;
    return 0;
  }

  void OpenGLDisplay::setDisplayParameters(const DisplayParameters& dp)
  {
    vsyncStateChange(false, 28);
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
    if (newState)
      return;
    messageQueueMutex.lock();
    bool    skippedFrame = skippingFrame;
    if (!skippedFrame)
      framesPending++;
    skippingFrame = (framesPending > 2);
    messageQueueMutex.unlock();
    curLine = 288 - (lineCnt ^ 1);
    lineCnt = 0;
    if (skippedFrame)
      return;
    Message *m = allocateMessage<Message_FrameDone>();
    queueMessage(m);
  }

}       // namespace Ep128Emu

