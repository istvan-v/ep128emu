
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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>

#include "fldisp.hpp"

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

namespace Ep128Emu {

  FLTKDisplay::Colormap::Colormap()
  {
    palette = new uint32_t[256];
    try {
      palette2 = new uint32_t[65536];
    }
    catch (...) {
      delete[] palette;
      throw;
    }
    DisplayParameters dp;
    setParams(dp);
  }

  FLTKDisplay::Colormap::~Colormap()
  {
    delete[] palette;
    delete[] palette2;
  }

  void FLTKDisplay::Colormap::setParams(const DisplayParameters& dp)
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

  uint32_t FLTKDisplay::Colormap::pixelConv(double r, double g, double b)
  {
    unsigned int  ri, gi, bi;
    ri = (r > 0.0 ? (r < 1.0 ? (unsigned int) (r * 255.0 + 0.5) : 255U) : 0U);
    gi = (g > 0.0 ? (g < 1.0 ? (unsigned int) (g * 255.0 + 0.5) : 255U) : 0U);
    bi = (b > 0.0 ? (b < 1.0 ? (unsigned int) (b * 255.0 + 0.5) : 255U) : 0U);
    return ((uint32_t(ri) << 16) + (uint32_t(gi) << 8) + uint32_t(bi));
  }

  // --------------------------------------------------------------------------

  FLTKDisplay::Message::~Message()
  {
  }

  FLTKDisplay::Message_LineData::~Message_LineData()
  {
  }

  void FLTKDisplay::Message_LineData::copyLine(const uint8_t *buf,
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

  FLTKDisplay::Message_FrameDone::~Message_FrameDone()
  {
  }

  FLTKDisplay::Message_SetParameters::~Message_SetParameters()
  {
  }

  void FLTKDisplay::deleteMessage(Message *m)
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

  void FLTKDisplay::queueMessage(Message *m)
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

  FLTKDisplay::FLTKDisplay(int xx, int yy, int ww, int hh, const char *lbl)
    : Fl_Window(xx, yy, ww, hh, lbl),
      messageQueue((Message *) 0),
      lastMessage((Message *) 0),
      freeMessageStack((Message *) 0),
      messageQueueMutex(),
      colormap(),
      lineBuffers((Message_LineData **) 0),
      linesChanged((bool *) 0),
      curLine(0),
      lineCnt(0),
      prvLineCnt(0),
      framesPending(0),
      skippingFrame(false),
      displayParameters(),
      savedDisplayParameters(),
      exitFlag(false),
      forceUpdateLineCnt(0),
      forceUpdateLineMask(0),
      redrawFlag(false)
  {
    try {
      lineBuffers = new Message_LineData*[576];
      for (size_t n = 0; n < 576; n++)
        lineBuffers[n] = (Message_LineData *) 0;
      linesChanged = new bool[576];
      for (size_t n = 0; n < 576; n++)
        linesChanged[n] = false;
    }
    catch (...) {
      if (lineBuffers)
        delete[] lineBuffers;
      if (linesChanged)
        delete[] linesChanged;
      throw;
    }
  }

  FLTKDisplay::~FLTKDisplay()
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
    delete[] linesChanged;
    for (size_t n = 0; n < 576; n++) {
      Message *m = lineBuffers[n];
      if (m) {
        lineBuffers[n] = (Message_LineData *) 0;
        m->~Message();
        std::free(m);
      }
    }
    delete[] lineBuffers;
  }

  void FLTKDisplay::displayFrame()
  {
    int     windowWidth_ = this->w();
    int     windowHeight_ = this->h();
    int     displayWidth_ = windowWidth_;
    int     displayHeight_ = windowHeight_;
    bool    halfResolutionX_ = false;
    bool    halfResolutionY_ = false;
    int     x0 = 0;
    int     y0 = 0;
    int     x1 = displayWidth_;
    int     y1 = displayHeight_;
    double  aspectScale_ = (768.0 / 576.0)
                           / ((double(windowWidth_) / double(windowHeight_))
                              * displayParameters.pixelAspectRatio);
    if (aspectScale_ > 1.0001) {
      displayHeight_ = int((double(windowHeight_) / aspectScale_) + 0.5);
      y0 = (windowHeight_ - displayHeight_) >> 1;
      y1 = y0 + displayHeight_;
    }
    else if (aspectScale_ < 0.9999) {
      displayWidth_ = int((double(windowWidth_) * aspectScale_) + 0.5);
      x0 = (windowWidth_ - displayWidth_) >> 1;
      x1 = x0 + displayWidth_;
    }
    if (displayWidth_ < 576)
      halfResolutionX_ = true;
    if (displayHeight_ < 432)
      halfResolutionY_ = true;

    if (x0 > 0) {
      fl_color(FL_BLACK);
      fl_rectf(0, 0, x0, windowHeight_);
    }
    if (x1 < windowWidth_) {
      fl_color(FL_BLACK);
      fl_rectf(x1, 0, (windowWidth_ - x1), windowHeight_);
    }
    if (y0 > 0) {
      fl_color(FL_BLACK);
      fl_rectf(0, 0, windowWidth_, y0);
    }
    if (y1 < windowHeight_) {
      fl_color(FL_BLACK);
      fl_rectf(0, y1, windowWidth_, (windowHeight_ - y1));
    }

    if (displayWidth_ <= 0 || displayHeight_ <= 0)
      return;

    unsigned char lineBuf_[768];
    unsigned char *pixelBuf_ =
        (unsigned char *) std::calloc(size_t(displayWidth_ * 3),
                                      sizeof(unsigned char));
    if (pixelBuf_) {
      int   prvLine_ = -1;
      int   curLine_ = 0;
      int   fracX_ = 0;
      int   fracY_ = 0;
      bool  skippingLine_ = true;
      Message_LineData  *prvLineRendered_ = (Message_LineData *) 0;
      for (int yc = y0; yc < y1; yc++) {
        if (curLine_ != prvLine_) {
          prvLine_ = curLine_;
          Message_LineData  *lBuf_ = lineBuffers[curLine_];
          if (linesChanged[curLine_] || linesChanged[curLine_ ^ 1])
            skippingLine_ = false;
          if (!skippingLine_) {
            if (!lBuf_)
              lBuf_ = lineBuffers[curLine_ ^ 1];
            if (halfResolutionY_ || curLine_ == 0 ||
                !((prvLineRendered_ == (Message_LineData *) 0 &&
                   lBuf_ == (Message_LineData *) 0) ||
                  (prvLineRendered_ != (Message_LineData *) 0 &&
                   lBuf_ != (Message_LineData *) 0 &&
                   (*lBuf_) == (*prvLineRendered_)))) {
              prvLineRendered_ = lBuf_;
              if (lBuf_) {
                // decode video data
                const unsigned char *bufp = (unsigned char *) 0;
                size_t  nBytes = 0;
                lBuf_->getLineData(bufp, nBytes);
                decodeLine(&(lineBuf_[0]), bufp, nBytes);
                // convert to RGB
                bufp = &(lineBuf_[0]);
                unsigned char *p = pixelBuf_;
                uint32_t      c = 0U;
                fracX_ = displayWidth_;
                if (!halfResolutionX_) {
                  while (true) {
                    if (fracX_ >= displayWidth_) {
                      if (bufp >= &(lineBuf_[768]))
                        break;
                      do {
                        c = colormap(*bufp);
                        fracX_ -= displayWidth_;
                        bufp++;
                      } while (fracX_ >= displayWidth_);
                    }
                    {
                      uint32_t  tmp = c;
                      p[2] = (unsigned char) tmp & (unsigned char) 0xFF;
                      tmp = tmp >> 8;
                      p[1] = (unsigned char) tmp & (unsigned char) 0xFF;
                      tmp = tmp >> 8;
                      p[0] = (unsigned char) tmp & (unsigned char) 0xFF;
                    }
                    fracX_ += 768;
                    p += 3;
                  }
                }
                else {
                  while (true) {
                    if (fracX_ >= displayWidth_) {
                      if (bufp >= &(lineBuf_[768]))
                        break;
                      do {
                        c = colormap(bufp[0], bufp[1]);
                        fracX_ -= displayWidth_;
                        bufp += 2;
                      } while (fracX_ >= displayWidth_);
                    }
                    {
                      uint32_t  tmp = c;
                      p[2] = (unsigned char) tmp & (unsigned char) 0xFF;
                      tmp = tmp >> 8;
                      p[1] = (unsigned char) tmp & (unsigned char) 0xFF;
                      tmp = tmp >> 8;
                      p[0] = (unsigned char) tmp & (unsigned char) 0xFF;
                    }
                    fracX_ += 384;
                    p += 3;
                  }
                }
              }
              else
                std::memset(pixelBuf_, 0, size_t(displayWidth_ * 3));
            }
          }
        }
        if (!skippingLine_)
          fl_draw_image(pixelBuf_, x0, yc, displayWidth_, 1);
        if (!halfResolutionY_) {
          fracY_ += 576;
          while (fracY_ >= displayHeight_) {
            fracY_ -= displayHeight_;
            if (curLine_ & 1) {
              linesChanged[curLine_ ^ 1] = false;
              linesChanged[curLine_] = false;
              skippingLine_ = true;
            }
            curLine_ = (curLine_ < 575 ? (curLine_ + 1) : curLine_);
          }
        }
        else {
          fracY_ += 288;
          while (fracY_ >= displayHeight_) {
            fracY_ -= displayHeight_;
            linesChanged[curLine_] = false;
            linesChanged[curLine_ | 1] = false;
            skippingLine_ = true;
            curLine_ = (curLine_ < 573 ? (curLine_ + 2) : curLine_);
          }
        }
      }
      std::free(pixelBuf_);
    }

    // make sure that all lines are updated at a slow rate
    if (forceUpdateLineMask) {
      for (size_t yc = 0; yc < 576; yc++) {
        if (!(forceUpdateLineMask & (uint8_t(1) << uint8_t((yc >> 1) & 7))))
          continue;
        if (lineBuffers[yc] != (Message_LineData *) 0) {
          Message *m = lineBuffers[yc];
          lineBuffers[yc] = (Message_LineData *) 0;
          deleteMessage(m);
        }
        linesChanged[yc] = true;
      }
      forceUpdateLineMask = 0;
    }

    messageQueueMutex.lock();
    framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
    messageQueueMutex.unlock();
  }

  void FLTKDisplay::draw()
  {
    if (redrawFlag) {
      redrawFlag = false;
      displayFrame();
      noInputTimer.reset();
    }
  }

  bool FLTKDisplay::checkEvents()
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
        if (msg->lineNum >= 0 && msg->lineNum < 576) {
          // check if this line has changed
          if (lineBuffers[msg->lineNum] != (Message_LineData *) 0) {
            if (*(lineBuffers[msg->lineNum]) == *msg) {
              deleteMessage(m);
              continue;
            }
          }
          linesChanged[msg->lineNum] = true;
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
        displayParameters = msg->dp;
        DisplayParameters tmp_dp(displayParameters);
        tmp_dp.blendScale1 = 0.5;
        colormap.setParams(tmp_dp);
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

  int FLTKDisplay::handle(int event)
  {
    (void) event;
    return 0;
  }

  void FLTKDisplay::setDisplayParameters(const DisplayParameters& dp)
  {
    vsyncStateChange(true, 8);
    vsyncStateChange(false, 28);
    Message_SetParameters *m = allocateMessage<Message_SetParameters>();
    m->dp = dp;
    savedDisplayParameters = dp;
    queueMessage(m);
  }

  const VideoDisplay::DisplayParameters&
      FLTKDisplay::getDisplayParameters() const
  {
    return savedDisplayParameters;
  }

  void FLTKDisplay::drawLine(const uint8_t *buf, size_t nBytes)
  {
    if (!skippingFrame) {
      if (curLine >= 0 && curLine < 576) {
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

  void FLTKDisplay::vsyncStateChange(bool newState, unsigned int currentSlot_)
  {
    (void) currentSlot_;
    if (newState) {
      curLine = 272 - prvLineCnt;
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

