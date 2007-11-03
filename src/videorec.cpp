
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
#include "display.hpp"
#include "snd_conv.hpp"
#include "videorec.hpp"

#include <cmath>
#include <cstring>

static const size_t aviHeaderSize_RLE8 = 0x0546;
static const size_t aviHeaderSize_YV12 = 0x0146;

namespace Ep128Emu {

  VideoCapture::AudioConverter_::AudioConverter_(VideoCapture& videoCapture_,
                                                 float inputSampleRate_,
                                                 float outputSampleRate_,
                                                 float dcBlockFreq1,
                                                 float dcBlockFreq2,
                                                 float ampScale_)
    : AudioConverterHighQuality(inputSampleRate_, outputSampleRate_,
                                dcBlockFreq1, dcBlockFreq2, ampScale_),
      videoCapture(videoCapture_)
  {
  }

  VideoCapture::AudioConverter_::~AudioConverter_()
  {
  }

  void VideoCapture::AudioConverter_::audioOutput(int16_t left, int16_t right)
  {
    const int   totalBufSize =
        videoCapture.audioBufSize * videoCapture.audioBuffers * 2;
    if (videoCapture.audioBufSamples < totalBufSize) {
      videoCapture.audioBuf[videoCapture.audioBufWritePos++] = left;
      if (videoCapture.audioBufWritePos >= totalBufSize)
        videoCapture.audioBufWritePos = 0;
      videoCapture.audioBufSamples++;
    }
    if (videoCapture.audioBufSamples < totalBufSize) {
      videoCapture.audioBuf[videoCapture.audioBufWritePos++] = right;
      if (videoCapture.audioBufWritePos >= totalBufSize)
        videoCapture.audioBufWritePos = 0;
      videoCapture.audioBufSamples++;
    }
  }

  // --------------------------------------------------------------------------

  void VideoCapture::aviHeader_writeFourCC(uint8_t*& bufp, const char *s)
  {
    bufp[0] = uint8_t(s[0]);
    bufp[1] = uint8_t(s[1]);
    bufp[2] = uint8_t(s[2]);
    bufp[3] = uint8_t(s[3]);
    bufp = bufp + 4;
  }

  void VideoCapture::aviHeader_writeUInt16(uint8_t*& bufp, uint16_t n)
  {
    bufp[0] = uint8_t(n & 0x00FF);
    bufp[1] = uint8_t((n & 0xFF00) >> 8);
    bufp = bufp + 2;
  }

  void VideoCapture::aviHeader_writeUInt32(uint8_t*& bufp, uint32_t n)
  {
    bufp[0] = uint8_t(n & 0x000000FFU);
    bufp[1] = uint8_t((n & 0x0000FF00U) >> 8);
    bufp[2] = uint8_t((n & 0x00FF0000U) >> 16);
    bufp[3] = uint8_t((n & 0xFF000000U) >> 24);
    bufp = bufp + 4;
  }

  void VideoCapture::defaultErrorCallback(void *userData, const char *msg)
  {
    (void) userData;
    throw Exception(msg);
  }

  void VideoCapture::defaultFileNameCallback(void *userData,
                                             std::string& fileName)
  {
    (void) userData;
    fileName.clear();
  }

  VideoCapture::VideoCapture(int frameRate_)
    : aviFile((std::FILE *) 0),
      audioBuf((int16_t *) 0),
      frameRate(frameRate_),
      audioBufSize(0),
      audioBufReadPos(0),
      audioBufWritePos(0),
      audioBufSamples(0),
      clockFrequency(0),
      soundOutputAccumulatorL(0U),
      soundOutputAccumulatorR(0U),
      curLine(0),
      vsyncCnt(0),
      hsyncCnt(0),
      vsyncState(false),
      oddFrame(false),
      framesWritten(0),
      duplicateFrames(0),
      fileSize(0),
      audioConverter((AudioConverter *) 0),
      aviHeaderSize(0),
      errorCallback(&defaultErrorCallback),
      errorCallbackUserData((void *) this),
      fileNameCallback(&defaultFileNameCallback),
      fileNameCallbackUserData((void *) this)
  {
    try {
      frameRate = (frameRate > 24 ? (frameRate < 60 ? frameRate : 60) : 24);
      while (((sampleRate / frameRate) * frameRate) != sampleRate)
        frameRate++;
      audioBufSize = sampleRate / frameRate;
      audioBuf = new int16_t[audioBufSize * audioBuffers * 2];
      for (int i = 0; i < (audioBufSize * audioBuffers * 2); i++)
        audioBuf[i] = int16_t(0);
      audioConverter =
          new AudioConverter_(*this, 222656.25f, float(sampleRate));
    }
    catch (...) {
      if (audioBuf)
        delete[] audioBuf;
      if (audioConverter)
        delete audioConverter;
      throw;
    }
  }

  VideoCapture::~VideoCapture()
  {
    delete[] audioBuf;
    delete audioConverter;
  }

  void VideoCapture::horizontalSync()
  {
    if (hsyncCnt >= 41)
      hsyncCnt = 51;
  }

  void VideoCapture::vsyncStateChange(bool newState, unsigned int currentSlot_)
  {
    vsyncState = newState;
    if (newState && (vsyncCnt + int(hsyncCnt >= 51)) >= 261) {
      vsyncCnt = -19 - int(hsyncCnt >= 51);
      oddFrame = (currentSlot_ >= 20U && currentSlot_ < 48U);
    }
  }

  void VideoCapture::openFile(const char *fileName)
  {
    closeFile();
    if (fileName == (char *) 0 || fileName[0] == '\0')
      return;
    aviFile = std::fopen(fileName, "wb");
    if (!aviFile)
      throw Exception("error opening AVI file");
    framesWritten = 0;
    duplicateFrames = 0;
    fileSize = aviHeaderSize;
    writeAVIHeader();
  }

  void VideoCapture::closeFile()
  {
    if (aviFile) {
      // FIXME: file I/O errors are ignored here
      try {
        writeAVIHeader();
        writeAVIIndex();
      }
      catch (...) {
      }
      if (aviFile)
        std::fclose(aviFile);
      aviFile = (std::FILE *) 0;
      framesWritten = 0;
      duplicateFrames = 0;
      fileSize = 0;
    }
  }

  void VideoCapture::errorMessage(const char *msg)
  {
    if (msg == (char *) 0 || msg[0] == '\0')
      msg = "unknown video capture error";
    errorCallback(errorCallbackUserData, msg);
  }

  void VideoCapture::setErrorCallback(void (*func)(void *userData,
                                                   const char *msg),
                                      void *userData_)
  {
    if (func) {
      errorCallback = func;
      errorCallbackUserData = userData_;
    }
    else {
      errorCallback = &defaultErrorCallback;
      errorCallbackUserData = (void *) this;
    }
  }

  void VideoCapture::setFileNameCallback(void (*func)(void *userData,
                                                      std::string& fileName),
                                         void *userData_)
  {
    if (func) {
      fileNameCallback = func;
      fileNameCallbackUserData = userData_;
    }
    else {
      fileNameCallback = &defaultFileNameCallback;
      fileNameCallbackUserData = (void *) this;
    }
  }

  // --------------------------------------------------------------------------

  VideoCapture_RLE8::VideoCaptureFrameBuffer::VideoCaptureFrameBuffer(int w,
                                                                      int h)
    : buf((uint32_t *) 0),
      linePtrs((uint8_t **) 0),
      lineBytes_((uint32_t *) 0)
  {
    try {
      size_t  nBytes = (((((size_t(w) * 17) + 15) >> 4) + 3) >> 2) << 2;
      buf = new uint32_t[(nBytes >> 2) * size_t(h)];
      std::memset(buf, 0x00, nBytes * size_t(h));
      linePtrs = new uint8_t*[h];
      uint8_t *p = reinterpret_cast<uint8_t *>(buf);
      for (int i = 0; i < h; i++) {
        linePtrs[i] = p;
        p = p + nBytes;
      }
      lineBytes_ = new uint32_t[h];
      for (int i = 0; i < h; i++)
        lineBytes_[i] = 0U;
    }
    catch (...) {
      if (buf)
        delete[] buf;
      if (linePtrs)
        delete[] linePtrs;
      if (lineBytes_)
        delete[] lineBytes_;
      throw;
    }
  }

  VideoCapture_RLE8::VideoCaptureFrameBuffer::~VideoCaptureFrameBuffer()
  {
    delete[] buf;
    delete[] linePtrs;
    delete[] lineBytes_;
  }

  inline bool VideoCapture_RLE8::VideoCaptureFrameBuffer::compareLine(
      long dstLine, const VideoCaptureFrameBuffer& src, long srcLine)
  {
    if (lineBytes_[dstLine] != src.lineBytes_[srcLine])
      return false;
    if (src.lineBytes_[srcLine] == 0U)
      return true;
    return (std::memcmp(linePtrs[dstLine], src.linePtrs[srcLine],
                        src.lineBytes_[srcLine]) == 0);
  }

  inline void VideoCapture_RLE8::VideoCaptureFrameBuffer::copyLine(long dstLine,
                                                                   long srcLine)
  {
    std::memcpy(linePtrs[dstLine], linePtrs[srcLine], lineBytes_[srcLine]);
    lineBytes_[dstLine] = lineBytes_[srcLine];
  }

  inline void VideoCapture_RLE8::VideoCaptureFrameBuffer::copyLine(
      long dstLine, const VideoCaptureFrameBuffer& src, long srcLine)
  {
    std::memcpy(linePtrs[dstLine], src.linePtrs[srcLine],
                src.lineBytes_[srcLine]);
    lineBytes_[dstLine] = src.lineBytes_[srcLine];
  }

  void VideoCapture_RLE8::VideoCaptureFrameBuffer::clearLine(long n)
  {
    uint8_t *p = linePtrs[n];
    for (int i = 0; i < 48; i++) {      // FIXME: assumes videoWidth == 768
      *(p++) = 0x01;
      *(p++) = 0x00;
    }
    lineBytes_[n] = 96U;
  }

  // --------------------------------------------------------------------------

  VideoCapture_RLE8::VideoCapture_RLE8(
      void (*indexToRGBFunc)(uint8_t color, float& r, float& g, float& b),
      int frameRate_)
    : VideoCapture(frameRate_),
      tmpFrameBuf(videoWidth, videoHeight),
      outputFrameBuf(videoWidth, videoHeight),
      frameSizes((uint32_t *) 0),
      cycleCnt(0),
      prvOddFrame(false),
      colormap((uint8_t *) 0)
  {
    try {
      aviHeaderSize = aviHeaderSize_RLE8;
      size_t  maxFrames = 0x20000000 / size_t(audioBufSize);
      frameSizes = new uint32_t[maxFrames];
      std::memset(frameSizes, 0x00, maxFrames * sizeof(uint32_t));
      // initialize colormap
      colormap = new uint8_t[256 * 4];
      for (int i = 0; i < 256; i++) {
        float   r = float(i) / 255.0f;
        float   g = r;
        float   b = r;
        if (indexToRGBFunc)
          indexToRGBFunc(uint8_t(i), r, g, b);
        int ri = int((r > 0.0f ? (r < 1.0f ? r : 1.0f) : 0.0f) * 255.0f + 0.5f);
        int gi = int((g > 0.0f ? (g < 1.0f ? g : 1.0f) : 0.0f) * 255.0f + 0.5f);
        int bi = int((b > 0.0f ? (b < 1.0f ? b : 1.0f) : 0.0f) * 255.0f + 0.5f);
        colormap[(i * 4) + 0] = uint8_t(bi);
        colormap[(i * 4) + 1] = uint8_t(gi);
        colormap[(i * 4) + 2] = uint8_t(ri);
        colormap[(i * 4) + 3] = 0x00;
      }
    }
    catch (...) {
      if (frameSizes)
        delete[] frameSizes;
      if (colormap)
        delete[] colormap;
      throw;
    }
    setClockFrequency(890625);
  }

  VideoCapture_RLE8::~VideoCapture_RLE8()
  {
    closeFile();
    delete[] frameSizes;
    delete[] colormap;
  }

  void VideoCapture_RLE8::runOneCycle(const uint8_t *videoInput,
                                      uint32_t audioInput)
  {
    soundOutputAccumulatorL += uint32_t(audioInput & 0xFFFFU);
    soundOutputAccumulatorR += uint32_t(audioInput >> 16);
    if (++cycleCnt >= 4) {
      cycleCnt = 0;
      uint32_t  tmpL = (soundOutputAccumulatorL + 2U) >> 2;
      uint32_t  tmpR = (soundOutputAccumulatorR + 2U) >> 2;
      soundOutputAccumulatorL = 0U;
      soundOutputAccumulatorR = 0U;
      audioConverter->sendInputSignal(tmpL | (tmpR << 16));
    }
    if (hsyncCnt < (videoWidth / 16) && curLine >= 0 && curLine < videoHeight) {
      uint8_t *p = &(tmpFrameBuf[curLine][tmpFrameBuf.lineBytes(curLine)]);
      uint8_t c = videoInput[0];
      tmpFrameBuf.lineBytes(curLine) += (uint32_t(c) + uint32_t(1));
      *(p++) = c;
      for (uint8_t i = 0; i < c; i++)
        *(p++) = videoInput[i + 1];
    }
    if (++hsyncCnt >= 60)
      lineDone();
  }

  void VideoCapture_RLE8::setClockFrequency(size_t freq_)
  {
    if (freq_ == clockFrequency)
      return;
    clockFrequency = freq_;
    audioConverter->setInputSampleRate(float(long(freq_)) * 0.25f);
  }

  void VideoCapture_RLE8::lineDone()
  {
    if (curLine >= 0 && curLine < videoHeight &&
        bool(curLine & 1) == prvOddFrame) {
      // no interlace, need to duplicate line
      tmpFrameBuf.copyLine(curLine ^ 1, curLine);
    }
    hsyncCnt = 0;
    if (vsyncCnt != 0) {
      curLine += 2;
      if (curLine < videoHeight)
        tmpFrameBuf.lineBytes(curLine) = 0U;
      if (vsyncCnt >= 261 && (vsyncState || vsyncCnt >= 335))
        vsyncCnt = -19;
      vsyncCnt++;
    }
    else {
      for (int i = (curLine + 1); i < videoHeight; i++)
        tmpFrameBuf.clearLine(i);
      frameDone();
      prvOddFrame = bool(curLine & 1);
      curLine = (oddFrame ? -1 : 0);
      if (curLine >= 0)
        tmpFrameBuf.lineBytes(curLine) = 0U;
      vsyncCnt++;
      oddFrame = false;
    }
  }

  void VideoCapture_RLE8::frameDone()
  {
    if (audioBufSamples >= (audioBufSize * 2)) {
      bool    frameChanged = false;
      for (int i = 0; i < videoHeight; i++) {
        if (!tmpFrameBuf.compareLine(i, outputFrameBuf, i)) {
          frameChanged = true;
          outputFrameBuf.copyLine(i, tmpFrameBuf, i);
        }
      }
      do {
        audioBufSamples -= (audioBufSize * 2);
        writeFrame(frameChanged);
        frameChanged = false;
        audioBufReadPos += (audioBufSize * 2);
        while (audioBufReadPos >= (audioBufSize * audioBuffers * 2))
          audioBufReadPos -= (audioBufSize * audioBuffers * 2);
      } while (audioBufSamples >= (audioBufSize * 2));
    }
  }

  void VideoCapture_RLE8::decodeLine(uint8_t *outBuf, const uint8_t *inBuf)
  {
    for (size_t i = 0; i < (videoWidth / 16); i++) {
      uint8_t c = *(inBuf++);
      switch (c) {
      case 0x01:
        outBuf[15] = outBuf[14] = outBuf[13] = outBuf[12] =
        outBuf[11] = outBuf[10] = outBuf[9]  = outBuf[8]  =
        outBuf[7]  = outBuf[6]  = outBuf[5]  = outBuf[4]  =
        outBuf[3]  = outBuf[2]  = outBuf[1]  = outBuf[0]  = *(inBuf++);
        break;
      case 0x02:
        outBuf[7]  = outBuf[6]  = outBuf[5]  = outBuf[4]  =
        outBuf[3]  = outBuf[2]  = outBuf[1]  = outBuf[0]  = *(inBuf++);
        outBuf[15] = outBuf[14] = outBuf[13] = outBuf[12] =
        outBuf[11] = outBuf[10] = outBuf[9]  = outBuf[8]  = *(inBuf++);
        break;
      case 0x03:
        {
          uint8_t c0 = *(inBuf++);
          uint8_t c1 = *(inBuf++);
          uint8_t b = *(inBuf++);
          outBuf[1]  = outBuf[0]  = ((b & 128) ? c1 : c0);
          outBuf[3]  = outBuf[2]  = ((b &  64) ? c1 : c0);
          outBuf[5]  = outBuf[4]  = ((b &  32) ? c1 : c0);
          outBuf[7]  = outBuf[6]  = ((b &  16) ? c1 : c0);
          outBuf[9]  = outBuf[8]  = ((b &   8) ? c1 : c0);
          outBuf[11] = outBuf[10] = ((b &   4) ? c1 : c0);
          outBuf[13] = outBuf[12] = ((b &   2) ? c1 : c0);
          outBuf[15] = outBuf[14] = ((b &   1) ? c1 : c0);
        }
        break;
      case 0x04:
        outBuf[3]  = outBuf[2]  = outBuf[1]  = outBuf[0]  = *(inBuf++);
        outBuf[7]  = outBuf[6]  = outBuf[5]  = outBuf[4]  = *(inBuf++);
        outBuf[11] = outBuf[10] = outBuf[9]  = outBuf[8]  = *(inBuf++);
        outBuf[15] = outBuf[14] = outBuf[13] = outBuf[12] = *(inBuf++);
        break;
      case 0x06:
        {
          uint8_t c0 = *(inBuf++);
          uint8_t c1 = *(inBuf++);
          uint8_t b = *(inBuf++);
          outBuf[0]  = ((b & 128) ? c1 : c0);
          outBuf[1]  = ((b &  64) ? c1 : c0);
          outBuf[2]  = ((b &  32) ? c1 : c0);
          outBuf[3]  = ((b &  16) ? c1 : c0);
          outBuf[4]  = ((b &   8) ? c1 : c0);
          outBuf[5]  = ((b &   4) ? c1 : c0);
          outBuf[6]  = ((b &   2) ? c1 : c0);
          outBuf[7]  = ((b &   1) ? c1 : c0);
          c0 = *(inBuf++);
          c1 = *(inBuf++);
          b = *(inBuf++);
          outBuf[8]  = ((b & 128) ? c1 : c0);
          outBuf[9]  = ((b &  64) ? c1 : c0);
          outBuf[10] = ((b &  32) ? c1 : c0);
          outBuf[11] = ((b &  16) ? c1 : c0);
          outBuf[12] = ((b &   8) ? c1 : c0);
          outBuf[13] = ((b &   4) ? c1 : c0);
          outBuf[14] = ((b &   2) ? c1 : c0);
          outBuf[15] = ((b &   1) ? c1 : c0);
        }
        break;
      case 0x08:
        outBuf[1]  = outBuf[0]  = *(inBuf++);
        outBuf[3]  = outBuf[2]  = *(inBuf++);
        outBuf[5]  = outBuf[4]  = *(inBuf++);
        outBuf[7]  = outBuf[6]  = *(inBuf++);
        outBuf[9]  = outBuf[8]  = *(inBuf++);
        outBuf[11] = outBuf[10] = *(inBuf++);
        outBuf[13] = outBuf[12] = *(inBuf++);
        outBuf[15] = outBuf[14] = *(inBuf++);
        break;
      case 0x10:
        outBuf[0]  = *(inBuf++);
        outBuf[1]  = *(inBuf++);
        outBuf[2]  = *(inBuf++);
        outBuf[3]  = *(inBuf++);
        outBuf[4]  = *(inBuf++);
        outBuf[5]  = *(inBuf++);
        outBuf[6]  = *(inBuf++);
        outBuf[7]  = *(inBuf++);
        outBuf[8]  = *(inBuf++);
        outBuf[9]  = *(inBuf++);
        outBuf[10] = *(inBuf++);
        outBuf[11] = *(inBuf++);
        outBuf[12] = *(inBuf++);
        outBuf[13] = *(inBuf++);
        outBuf[14] = *(inBuf++);
        outBuf[15] = *(inBuf++);
        break;
      default:
        outBuf[15] = outBuf[14] = outBuf[13] = outBuf[12] =
        outBuf[11] = outBuf[10] = outBuf[9]  = outBuf[8]  =
        outBuf[7]  = outBuf[6]  = outBuf[5]  = outBuf[4]  =
        outBuf[3]  = outBuf[2]  = outBuf[1]  = outBuf[0]  = 0;
        break;
      }
      outBuf = outBuf + 16;
    }
  }

  size_t VideoCapture_RLE8::rleCompressLine(uint8_t *outBuf,
                                            const uint8_t *inBuf)
  {
    uint8_t runLengths[1024];
    uint8_t byteValues[1024];
    size_t  nBytes = 0;
    int     n = 0;
    int     runLength = 1;
    for (int i = 1; i < videoWidth; i++) {
      if (inBuf[i] != inBuf[i - 1]) {
        runLengths[n] = uint8_t(runLength);
        byteValues[n++] = inBuf[i - 1];
        runLength = 1;
      }
      else {
        if (++runLength >= 256) {
          runLengths[n] = 255;
          byteValues[n++] = inBuf[i - 1];
          runLength = 1;
        }
      }
    }
    runLengths[n] = uint8_t(runLength);
    byteValues[n++] = inBuf[videoWidth - 1];
    for (int i = 0; i < n; ) {
      if (runLengths[i] >= 2 || (i + 1) >= n) {
        outBuf[nBytes++] = runLengths[i];
        outBuf[nBytes++] = byteValues[i];
        i++;
      }
      else {
        int     j = i + 1;
        int     bytesToCopy = int(runLengths[i]);
        int     minLength = 3;
        do {
          int     tmp = int(runLengths[j]);
          if (tmp >= minLength || (bytesToCopy + tmp) > 255)
            break;
          bytesToCopy += tmp;
          minLength = 4 | (bytesToCopy & 1);
        } while (++j < n);
        if (bytesToCopy >= 3) {
          outBuf[nBytes++] = 0x00;
          outBuf[nBytes++] = uint8_t(bytesToCopy);
          do {
            int     k = int(runLengths[i]);
            uint8_t tmp = byteValues[i];
            do {
              outBuf[nBytes++] = tmp;
            } while (--k != 0);
          } while (++i < j);
          if (bytesToCopy & 1)
            outBuf[nBytes++] = 0x00;
        }
        else {
          do {
            outBuf[nBytes++] = runLengths[i];
            outBuf[nBytes++] = byteValues[i];
          } while (++i < j);
        }
      }
    }
    outBuf[nBytes++] = 0x00;    // end of line
    outBuf[nBytes++] = 0x00;
    return nBytes;
  }

  void VideoCapture_RLE8::writeFrame(bool frameChanged)
  {
    if (!aviFile)
      return;
    if (!frameChanged) {
      if (framesWritten == 0 || duplicateFrames >= size_t(frameRate))
        frameChanged = true;
      else
        duplicateFrames++;
    }
    if (frameChanged)
      duplicateFrames = 0;
    try {
      if (fileSize >= 0x7F800000) {
        closeFile();
        try {
          errorMessage("AVI file is too large, starting new output file");
        }
        catch (...) {
        }
        std::string fileName = "";
        fileNameCallback(fileNameCallbackUserData, fileName);
        if (fileName.length() < 1)
          return;
        openFile(fileName.c_str());
      }
      if (std::fseek(aviFile, 0L, SEEK_END) < 0)
        throw Exception("error seeking AVI file");
      uint8_t headerBuf[8];
      uint8_t *bufp = &(headerBuf[0]);
      size_t  nBytes = 0;
      frameSizes[framesWritten] = 0U;
      aviHeader_writeFourCC(bufp, "00dc");
      aviHeader_writeUInt32(bufp, 0x00000000U);
      fileSize = fileSize + 8;
      if (std::fwrite(&(headerBuf[0]), 1, 8, aviFile) != 8)
        throw Exception("error writing AVI file");
      if (frameChanged) {
        long    savedFilePos = std::ftell(aviFile);
        if (savedFilePos < 4L)
          throw Exception("error seeking AVI file");
        savedFilePos = savedFilePos - 4L;
        uint8_t lineBuf[1024];
        uint8_t rleBuf[1024];
        size_t  n = 0;
        for (int i = (videoHeight - 1); i >= 0; i--) {
          if (i == (videoHeight - 1) ||
              !outputFrameBuf.compareLine(i, outputFrameBuf, i + 1)) {
            decodeLine(&(lineBuf[0]), outputFrameBuf[i]);
            n = rleCompressLine(&(rleBuf[0]), &(lineBuf[0]));
          }
          nBytes += n;
          fileSize += n;
          if (std::fwrite(&(rleBuf[0]), 1, n, aviFile) != n)
            throw Exception("error writing AVI file");
        }
        if (std::fseek(aviFile, savedFilePos, SEEK_SET) < 0)
          throw Exception("error seeking AVI file");
        bufp = &(headerBuf[4]);
        frameSizes[framesWritten] = uint32_t(nBytes);
        aviHeader_writeUInt32(bufp, uint32_t(nBytes));
        if (std::fwrite(&(headerBuf[4]), 1, 4, aviFile) != 4)
          throw Exception("error writing AVI file");
        if (std::fseek(aviFile, 0L, SEEK_END) < 0)
          throw Exception("error seeking AVI file");
      }
      bufp = &(headerBuf[0]);
      nBytes = size_t(audioBufSize * 4);
      aviHeader_writeFourCC(bufp, "01wb");
      aviHeader_writeUInt32(bufp, uint32_t(nBytes));
      fileSize = fileSize + 8;
      if (std::fwrite(&(headerBuf[0]), 1, 8, aviFile) != 8)
        throw Exception("error writing AVI file");
      int     bufPos = audioBufReadPos;
      for (int i = 0; i < (audioBufSize * 2); i++) {
        if (bufPos >= (audioBufSize * audioBuffers * 2))
          bufPos = 0;
        int16_t tmp = audioBuf[bufPos++];
        fileSize++;
        if (std::fputc(int(uint16_t(tmp) & 0xFF), aviFile) == EOF)
          throw Exception("error writing AVI file");
        fileSize++;
        if (std::fputc(int((uint16_t(tmp) >> 8) & 0xFF), aviFile) == EOF)
          throw Exception("error writing AVI file");
      }
    }
    catch (std::exception& e) {
      closeFile();
      errorMessage(e.what());
      return;
    }
    framesWritten++;
    if (!(framesWritten & 31)) {
      try {
        writeAVIHeader();
      }
      catch (std::exception& e) {
        errorMessage(e.what());
      }
    }
  }

  void VideoCapture_RLE8::writeAVIHeader()
  {
    if (!aviFile)
      return;
    try {
      if (std::fseek(aviFile, 0L, SEEK_SET) < 0)
        throw Exception("error seeking AVI file");
      uint8_t   headerBuf[1536];
      uint8_t   *bufp = &(headerBuf[0]);
      size_t    maxVideoFrameSize = size_t((videoWidth + 16) * videoHeight);
      size_t    maxFrameSize = size_t(maxVideoFrameSize + (audioBufSize * 4)
                                      + 16);
      aviHeader_writeFourCC(bufp, "RIFF");
      aviHeader_writeUInt32(bufp, uint32_t(fileSize - 8));
      aviHeader_writeFourCC(bufp, "AVI ");
      aviHeader_writeFourCC(bufp, "LIST");
      aviHeader_writeUInt32(bufp, 0x00000526U);
      aviHeader_writeFourCC(bufp, "hdrl");
      aviHeader_writeFourCC(bufp, "avih");
      aviHeader_writeUInt32(bufp, 0x00000038U);
      // microseconds per frame
      aviHeader_writeUInt32(bufp,
                            uint32_t((1000000 + (frameRate >> 1)) / frameRate));
      // max. bytes per second
      aviHeader_writeUInt32(bufp, uint32_t(maxFrameSize * size_t(frameRate)));
      // padding
      aviHeader_writeUInt32(bufp, 0x00000001U);
      // flags (AVIF_HASINDEX | AVIF_ISINTERLEAVED | AVIF_TRUSTCKTYPE)
      aviHeader_writeUInt32(bufp, 0x00000910U);
      // total frames
      aviHeader_writeUInt32(bufp, uint32_t(framesWritten));
      // initial frames
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // number of streams
      aviHeader_writeUInt32(bufp, 0x00000002U);
      // suggested buffer size
      aviHeader_writeUInt32(bufp, uint32_t(maxFrameSize));
      // width
      aviHeader_writeUInt32(bufp, uint32_t(videoWidth));
      // height
      aviHeader_writeUInt32(bufp, uint32_t(videoHeight));
      // reserved
      aviHeader_writeUInt32(bufp, 0x00000000U);
      aviHeader_writeUInt32(bufp, 0x00000000U);
      aviHeader_writeUInt32(bufp, 0x00000000U);
      aviHeader_writeUInt32(bufp, 0x00000000U);
      aviHeader_writeFourCC(bufp, "LIST");
      aviHeader_writeUInt32(bufp, 0x00000474U);
      aviHeader_writeFourCC(bufp, "strl");
      aviHeader_writeFourCC(bufp, "strh");
      aviHeader_writeUInt32(bufp, 0x00000038U);
      aviHeader_writeFourCC(bufp, "vids");
      // video codec
      aviHeader_writeUInt32(bufp, 0x00000001U); // BI_RLE8
      // flags
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // priority
      aviHeader_writeUInt16(bufp, 0x0000);
      // language
      aviHeader_writeUInt16(bufp, 0x0000);
      // initial frames
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // scale
      aviHeader_writeUInt32(bufp, 0x00000001U);
      // rate
      aviHeader_writeUInt32(bufp, uint32_t(frameRate));
      // start time
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // length
      aviHeader_writeUInt32(bufp, uint32_t(framesWritten));
      // suggested buffer size
      aviHeader_writeUInt32(bufp, uint32_t(maxVideoFrameSize));
      // quality
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // sample size
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // left
      aviHeader_writeUInt16(bufp, 0x0000);
      // top
      aviHeader_writeUInt16(bufp, 0x0000);
      // right
      aviHeader_writeUInt16(bufp, uint16_t(videoWidth));
      // bottom
      aviHeader_writeUInt16(bufp, uint16_t(videoHeight));
      aviHeader_writeFourCC(bufp, "strf");
      aviHeader_writeUInt32(bufp, 0x00000428U);
      aviHeader_writeUInt32(bufp, 0x00000028U);
      // width
      aviHeader_writeUInt32(bufp, uint32_t(videoWidth));
      // height
      aviHeader_writeUInt32(bufp, uint32_t(videoHeight));
      // planes
      aviHeader_writeUInt16(bufp, 0x0001);
      // bits per pixel
      aviHeader_writeUInt16(bufp, 0x0008);
      // compression
      aviHeader_writeUInt32(bufp, 0x00000001U); // BI_RLE8
      // image size in bytes
      aviHeader_writeUInt32(bufp, uint32_t(videoWidth * videoHeight));
      // X resolution
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // Y resolution
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // color indexes used
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // color indexes required
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // colormap (256 entries)
      std::memcpy(bufp, colormap, 1024);
      bufp = bufp + 1024;
      aviHeader_writeFourCC(bufp, "LIST");
      aviHeader_writeUInt32(bufp, 0x0000005EU);
      aviHeader_writeFourCC(bufp, "strl");
      aviHeader_writeFourCC(bufp, "strh");
      aviHeader_writeUInt32(bufp, 0x00000038U);
      aviHeader_writeFourCC(bufp, "auds");
      // audio codec (WAVE_FORMAT_PCM)
      aviHeader_writeUInt32(bufp, 0x00000001U);
      // flags
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // priority
      aviHeader_writeUInt16(bufp, 0x0000);
      // language
      aviHeader_writeUInt16(bufp, 0x0000);
      // initial frames
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // scale
      aviHeader_writeUInt32(bufp, 0x00000001U);
      // rate
      aviHeader_writeUInt32(bufp, uint32_t(sampleRate));
      // start time
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // length
      aviHeader_writeUInt32(bufp, uint32_t(framesWritten
                                           * size_t(audioBufSize)));
      // suggested buffer size
      aviHeader_writeUInt32(bufp, uint32_t(audioBufSize * 4));
      // quality
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // sample size
      aviHeader_writeUInt32(bufp, 0x00000004U);
      // left
      aviHeader_writeUInt16(bufp, 0x0000);
      // top
      aviHeader_writeUInt16(bufp, 0x0000);
      // right
      aviHeader_writeUInt16(bufp, 0x0000);
      // bottom
      aviHeader_writeUInt16(bufp, 0x0000);
      aviHeader_writeFourCC(bufp, "strf");
      aviHeader_writeUInt32(bufp, 0x00000012U);
      // audio format (WAVE_FORMAT_PCM)
      aviHeader_writeUInt16(bufp, 0x0001);
      // audio channels
      aviHeader_writeUInt16(bufp, 0x0002);
      // samples per second
      aviHeader_writeUInt32(bufp, uint32_t(sampleRate));
      // bytes per second
      aviHeader_writeUInt32(bufp, uint32_t(sampleRate * 4));
      // block alignment
      aviHeader_writeUInt16(bufp, 0x0004);
      // bits per sample
      aviHeader_writeUInt16(bufp, 0x0010);
      // additional format information size
      aviHeader_writeUInt16(bufp, 0x0000);
      aviHeader_writeFourCC(bufp, "LIST");
      aviHeader_writeUInt32(bufp, uint32_t((fileSize - aviHeaderSize) + 4));
      aviHeader_writeFourCC(bufp, "movi");
      size_t  nBytes = size_t(bufp - (&(headerBuf[0])));
      if (std::fwrite(&(headerBuf[0]), 1, nBytes, aviFile) != nBytes)
        throw Exception("error writing AVI file header");
      if (std::fflush(aviFile) != 0)
        throw Exception("error writing AVI file header");
    }
    catch (...) {
      std::fclose(aviFile);
      aviFile = (std::FILE *) 0;
      framesWritten = 0;
      duplicateFrames = 0;
      fileSize = 0;
      throw;
    }
  }

  void VideoCapture_RLE8::writeAVIIndex()
  {
    if (!aviFile)
      return;
    try {
      if (std::fseek(aviFile, 0L, SEEK_END) < 0)
        throw Exception("error seeking AVI file");
      uint8_t   tmpBuf[32];
      uint8_t   *bufp = &(tmpBuf[0]);
      aviHeader_writeFourCC(bufp, "idx1");
      aviHeader_writeUInt32(bufp, uint32_t(framesWritten << 5));
      fileSize = fileSize + 8;
      if (std::fwrite(&(tmpBuf[0]), 1, 8, aviFile) != 8)
        throw Exception("error writing AVI file index");
      size_t    filePos = 4;
      for (size_t i = 0; i < framesWritten; i++) {
        bufp = &(tmpBuf[0]);
        aviHeader_writeFourCC(bufp, "00dc");
        size_t    frameBytes = size_t(frameSizes[i]);
        if (frameBytes > 0)
          aviHeader_writeUInt32(bufp, 0x00000010U);     // AVIIF_KEYFRAME
        else
          aviHeader_writeUInt32(bufp, 0x00000000U);
        aviHeader_writeUInt32(bufp, uint32_t(filePos));
        filePos = filePos + frameBytes + 8;
        aviHeader_writeUInt32(bufp, uint32_t(frameBytes));
        aviHeader_writeFourCC(bufp, "01wb");
        aviHeader_writeUInt32(bufp, 0x00000010U);       // AVIIF_KEYFRAME
        aviHeader_writeUInt32(bufp, uint32_t(filePos));
        frameBytes = size_t(audioBufSize) << 2;
        filePos = filePos + frameBytes + 8;
        aviHeader_writeUInt32(bufp, uint32_t(frameBytes));
        fileSize = fileSize + 32;
        if (std::fwrite(&(tmpBuf[0]), 1, 32, aviFile) != 32)
          throw Exception("error writing AVI file index");
      }
      if (std::fseek(aviFile, 0L, SEEK_SET) < 0)
        throw Exception("error seeking AVI file");
      bufp = &(tmpBuf[0]);
      aviHeader_writeFourCC(bufp, "RIFF");
      aviHeader_writeUInt32(bufp, uint32_t(fileSize - 8));
      if (std::fwrite(&(tmpBuf[0]), 1, 8, aviFile) != 8)
        throw Exception("error writing AVI file index");
      if (std::fflush(aviFile) != 0)
        throw Exception("error writing AVI file index");
    }
    catch (...) {
      std::fclose(aviFile);
      aviFile = (std::FILE *) 0;
      framesWritten = 0;
      duplicateFrames = 0;
      fileSize = 0;
      throw;
    }
  }

  // --------------------------------------------------------------------------

  VideoCapture_YV12::VideoCapture_YV12(
      void (*indexToRGBFunc)(uint8_t color, float& r, float& g, float& b),
      int frameRate_)
    : VideoCapture(frameRate_),
      lineBuf((uint8_t *) 0),
      frameBuf0Y((uint8_t *) 0),
      frameBuf0V((uint8_t *) 0),
      frameBuf0U((uint8_t *) 0),
      frameBuf1Y((uint8_t *) 0),
      frameBuf1V((uint8_t *) 0),
      frameBuf1U((uint8_t *) 0),
      interpBufY((int32_t *) 0),
      interpBufV((int32_t *) 0),
      interpBufU((int32_t *) 0),
      outBufY((uint8_t *) 0),
      outBufV((uint8_t *) 0),
      outBufU((uint8_t *) 0),
      duplicateFrameBitmap((uint8_t *) 0),
      timesliceLength(0L),
      curTime(0L),
      frame0Time(-1L),
      frame1Time(0L),
      cycleCnt(0),
      interpTime(0),
      lineBufBytes(0),
      colormap((uint32_t *) 0)
  {
    try {
      aviHeaderSize = aviHeaderSize_YV12;
      size_t    bufSize1 = size_t(videoWidth * videoHeight);
      size_t    bufSize2 = 1024 / 4;
      size_t    bufSize3 = (bufSize1 + 3) >> 2;
      size_t    bufSize4 = (bufSize3 + 3) >> 2;
      size_t    totalSize = bufSize2;
      totalSize += (3 * (bufSize3 + bufSize4 + bufSize4));
      totalSize += (bufSize1 + bufSize3 + bufSize3);
      uint32_t  *videoBuf = new uint32_t[totalSize];
      totalSize = 0;
      lineBuf = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(lineBuf, 0x00, 1024);
      totalSize += bufSize2;
      frameBuf0Y = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(frameBuf0Y, 0x10, bufSize1);
      totalSize += bufSize3;
      frameBuf0V = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(frameBuf0V, 0x80, bufSize3);
      totalSize += bufSize4;
      frameBuf0U = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(frameBuf0U, 0x80, bufSize3);
      totalSize += bufSize4;
      frameBuf1Y = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(frameBuf1Y, 0x10, bufSize1);
      totalSize += bufSize3;
      frameBuf1V = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(frameBuf1V, 0x80, bufSize3);
      totalSize += bufSize4;
      frameBuf1U = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(frameBuf1U, 0x80, bufSize3);
      totalSize += bufSize4;
      interpBufY = reinterpret_cast<int32_t *>(&(videoBuf[totalSize]));
      for (size_t i = 0; i < bufSize1; i++)
        interpBufY[i] = 0;
      totalSize += bufSize1;
      interpBufV = reinterpret_cast<int32_t *>(&(videoBuf[totalSize]));
      for (size_t i = 0; i < bufSize3; i++)
        interpBufV[i] = 0;
      totalSize += bufSize3;
      interpBufU = reinterpret_cast<int32_t *>(&(videoBuf[totalSize]));
      for (size_t i = 0; i < bufSize3; i++)
        interpBufU[i] = 0;
      totalSize += bufSize3;
      outBufY = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(outBufY, 0x10, bufSize1);
      totalSize += bufSize3;
      outBufV = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(outBufV, 0x80, bufSize3);
      totalSize += bufSize4;
      outBufU = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(outBufU, 0x80, bufSize3);
      size_t  nBytes = 0x04000000 / size_t(audioBufSize);
      duplicateFrameBitmap = new uint8_t[nBytes];
      std::memset(duplicateFrameBitmap, 0x00, nBytes);
      // initialize colormap
      colormap = new uint32_t[256];
      for (int i = 0; i < 256; i++) {
        float   r = float(i) / 255.0f;
        float   g = r;
        float   b = r;
        if (indexToRGBFunc)
          indexToRGBFunc(uint8_t(i), r, g, b);
        float   y = (0.299f * r) + (0.587f * g) + (0.114f * b);
        float   u = 0.492f * (b - y);
        float   v = 0.877f * (r - y);
        // scale video signal to YCrCb range
        y = 16.0f + (y * 219.5f);
        u = 128.0f + (u * (111.5f / (0.886f * 0.492f)));
        v = 128.0f + (v * (111.5f / (0.701f * 0.877f)));
        y = (y > 16.0f ? (y < 235.0f ? y : 235.0f) : 16.0f);
        u = (u > 16.0f ? (u < 239.0f ? u : 239.0f) : 16.0f);
        v = (v > 16.0f ? (v < 239.0f ? v : 239.0f) : 16.0f);
        // change pixel format for more efficient processing
        colormap[i] = uint32_t(int32_t(y + 0.5f)
                               | (int32_t(u + 0.5f) << 10)
                               | (int32_t(v + 0.5f) << 20));
      }
    }
    catch (...) {
      if (lineBuf)
        delete[] reinterpret_cast<uint32_t *>(lineBuf);
      if (duplicateFrameBitmap)
        delete[] duplicateFrameBitmap;
      if (colormap)
        delete[] colormap;
      throw;
    }
    setClockFrequency(890625);
  }

  VideoCapture_YV12::~VideoCapture_YV12()
  {
    closeFile();
    delete[] reinterpret_cast<uint32_t *>(lineBuf);
    delete[] duplicateFrameBitmap;
    delete[] colormap;
  }

  void VideoCapture_YV12::runOneCycle(const uint8_t *videoInput,
                                      uint32_t audioInput)
  {
    soundOutputAccumulatorL += uint32_t(audioInput & 0xFFFFU);
    soundOutputAccumulatorR += uint32_t(audioInput >> 16);
    if (++cycleCnt >= 4) {
      cycleCnt = 0;
      uint32_t  tmpL = (soundOutputAccumulatorL + 2U) >> 2;
      uint32_t  tmpR = (soundOutputAccumulatorR + 2U) >> 2;
      soundOutputAccumulatorL = 0U;
      soundOutputAccumulatorR = 0U;
      audioConverter->sendInputSignal(tmpL | (tmpR << 16));
    }
    uint8_t   c = videoInput[0];
    lineBuf[lineBufBytes++] = c;
    for (uint8_t i = 0; i < c; i++)
      lineBuf[lineBufBytes++] = videoInput[i + 1];
    if (++hsyncCnt >= 60)
      lineDone();
    curTime += timesliceLength;
  }

  void VideoCapture_YV12::setClockFrequency(size_t freq_)
  {
    if (freq_ == clockFrequency)
      return;
    clockFrequency = freq_;
    timesliceLength = (int64_t(1000000) << 32) / int64_t(freq_);
    audioConverter->setInputSampleRate(float(long(freq_)) * 0.25f);
  }

  void VideoCapture_YV12::lineDone()
  {
    if (curLine >= 0 && curLine < (videoHeight * 2))
      decodeLine();
    hsyncCnt = 0;
    lineBufBytes = 0;
    if (vsyncCnt != 0) {
      curLine += 2;
      if (vsyncCnt >= 261 && (vsyncState || vsyncCnt >= 335))
        vsyncCnt = -19;
      vsyncCnt++;
    }
    else {
      curLine = (oddFrame ? -1 : 0);
      vsyncCnt++;
      oddFrame = false;
      frameDone();
    }
  }

  void VideoCapture_YV12::decodeLine()
  {
    int       lineNum = curLine >> 1;
    int       offs = lineNum * videoWidth;
    uint8_t   *yPtr = &(frameBuf1Y[offs]);
    offs = (lineNum >> 1) * (videoWidth >> 1);
    uint8_t   *vPtr = &(frameBuf1V[offs]);
    uint8_t   *uPtr = &(frameBuf1U[offs]);
    const uint8_t   *bufp = lineBuf;

    if (!(lineNum & 1)) {
      for (size_t i = 0; i < 48; i++) {
        uint8_t c = *(bufp++);
        switch (c) {
        case 0x01:
          {
            uint32_t  tmp = colormap[*(bufp++)];
            yPtr[7] = yPtr[6] = yPtr[5] = yPtr[4] =
            yPtr[3] = yPtr[2] = yPtr[1] = yPtr[0] = uint8_t(tmp & 0xFFU);
            vPtr[3] = vPtr[2] = vPtr[1] = vPtr[0] =
                uint8_t((tmp >> 20) & 0xFFU);
            uPtr[3] = uPtr[2] = uPtr[1] = uPtr[0] =
                uint8_t((tmp >> 10) & 0xFFU);
          }
          break;
        case 0x02:
          {
            uint32_t  tmp = colormap[*(bufp++)];
            yPtr[3] = yPtr[2] = yPtr[1] = yPtr[0] = uint8_t(tmp & 0xFFU);
            vPtr[1] = vPtr[0] = uint8_t((tmp >> 20) & 0xFFU);
            uPtr[1] = uPtr[0] = uint8_t((tmp >> 10) & 0xFFU);
            tmp = colormap[*(bufp++)];
            yPtr[7] = yPtr[6] = yPtr[5] = yPtr[4] = uint8_t(tmp & 0xFFU);
            vPtr[3] = vPtr[2] = uint8_t((tmp >> 20) & 0xFFU);
            uPtr[3] = uPtr[2] = uint8_t((tmp >> 10) & 0xFFU);
          }
          break;
        case 0x03:
          {
            unsigned char c0 = *(bufp++);
            unsigned char c1 = *(bufp++);
            unsigned char b = *(bufp++);
            uint32_t  p0 = colormap[((b & 128) ? c1 : c0)];
            uint32_t  p1 = colormap[((b &  64) ? c1 : c0)];
            yPtr[0] = uint8_t(p0 & 0xFFU);
            yPtr[1] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[0] = uint8_t((p0 >> 21) & 0xFFU);
            uPtr[0] = uint8_t((p0 >> 11) & 0xFFU);
            p0 = colormap[((b &  32) ? c1 : c0)];
            p1 = colormap[((b &  16) ? c1 : c0)];
            yPtr[2] = uint8_t(p0 & 0xFFU);
            yPtr[3] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[1] = uint8_t((p0 >> 21) & 0xFFU);
            uPtr[1] = uint8_t((p0 >> 11) & 0xFFU);
            p0 = colormap[((b &   8) ? c1 : c0)];
            p1 = colormap[((b &   4) ? c1 : c0)];
            yPtr[4] = uint8_t(p0 & 0xFFU);
            yPtr[5] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[2] = uint8_t((p0 >> 21) & 0xFFU);
            uPtr[2] = uint8_t((p0 >> 11) & 0xFFU);
            p0 = colormap[((b &   2) ? c1 : c0)];
            p1 = colormap[((b &   1) ? c1 : c0)];
            yPtr[6] = uint8_t(p0 & 0xFFU);
            yPtr[7] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[3] = uint8_t((p0 >> 21) & 0xFFU);
            uPtr[3] = uint8_t((p0 >> 11) & 0xFFU);
          }
          break;
        case 0x04:
          {
            uint32_t  tmp = colormap[*(bufp++)];
            yPtr[1] = yPtr[0] = uint8_t(tmp & 0xFFU);
            vPtr[0] = uint8_t((tmp >> 20) & 0xFFU);
            uPtr[0] = uint8_t((tmp >> 10) & 0xFFU);
            tmp = colormap[*(bufp++)];
            yPtr[3] = yPtr[2] = uint8_t(tmp & 0xFFU);
            vPtr[1] = uint8_t((tmp >> 20) & 0xFFU);
            uPtr[1] = uint8_t((tmp >> 10) & 0xFFU);
            tmp = colormap[*(bufp++)];
            yPtr[5] = yPtr[4] = uint8_t(tmp & 0xFFU);
            vPtr[2] = uint8_t((tmp >> 20) & 0xFFU);
            uPtr[2] = uint8_t((tmp >> 10) & 0xFFU);
            tmp = colormap[*(bufp++)];
            yPtr[7] = yPtr[6] = uint8_t(tmp & 0xFFU);
            vPtr[3] = uint8_t((tmp >> 20) & 0xFFU);
            uPtr[3] = uint8_t((tmp >> 10) & 0xFFU);
          }
          break;
        case 0x06:
          {
            unsigned char c0 = *(bufp++);
            unsigned char c1 = *(bufp++);
            unsigned char b = *(bufp++);
            uint32_t  p0 = colormap[((b & 128) ? c1 : c0)];
            uint32_t  p1 = colormap[((b &  64) ? c1 : c0)];
            uint32_t  p2 = colormap[((b &  32) ? c1 : c0)];
            uint32_t  p3 = colormap[((b &  16) ? c1 : c0)];
            yPtr[0] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[1] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[0] = uint8_t((p0 >> 22) & 0xFFU);
            uPtr[0] = uint8_t((p0 >> 12) & 0xFFU);
            p0 = colormap[((b &   8) ? c1 : c0)];
            p1 = colormap[((b &   4) ? c1 : c0)];
            p2 = colormap[((b &   2) ? c1 : c0)];
            p3 = colormap[((b &   1) ? c1 : c0)];
            yPtr[2] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[3] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[1] = uint8_t((p0 >> 22) & 0xFFU);
            uPtr[1] = uint8_t((p0 >> 12) & 0xFFU);
            c0 = *(bufp++);
            c1 = *(bufp++);
            b = *(bufp++);
            p0 = colormap[((b & 128) ? c1 : c0)];
            p1 = colormap[((b &  64) ? c1 : c0)];
            p2 = colormap[((b &  32) ? c1 : c0)];
            p3 = colormap[((b &  16) ? c1 : c0)];
            yPtr[4] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[5] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[2] = uint8_t((p0 >> 22) & 0xFFU);
            uPtr[2] = uint8_t((p0 >> 12) & 0xFFU);
            p0 = colormap[((b &   8) ? c1 : c0)];
            p1 = colormap[((b &   4) ? c1 : c0)];
            p2 = colormap[((b &   2) ? c1 : c0)];
            p3 = colormap[((b &   1) ? c1 : c0)];
            yPtr[6] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[7] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[3] = uint8_t((p0 >> 22) & 0xFFU);
            uPtr[3] = uint8_t((p0 >> 12) & 0xFFU);
          }
          break;
        case 0x08:
          {
            uint32_t  p0 = colormap[*(bufp++)];
            uint32_t  p1 = colormap[*(bufp++)];
            yPtr[0] = uint8_t(p0 & 0xFFU);
            yPtr[1] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[0] = uint8_t((p0 >> 21) & 0xFFU);
            uPtr[0] = uint8_t((p0 >> 11) & 0xFFU);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            yPtr[2] = uint8_t(p0 & 0xFFU);
            yPtr[3] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[1] = uint8_t((p0 >> 21) & 0xFFU);
            uPtr[1] = uint8_t((p0 >> 11) & 0xFFU);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            yPtr[4] = uint8_t(p0 & 0xFFU);
            yPtr[5] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[2] = uint8_t((p0 >> 21) & 0xFFU);
            uPtr[2] = uint8_t((p0 >> 11) & 0xFFU);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            yPtr[6] = uint8_t(p0 & 0xFFU);
            yPtr[7] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[3] = uint8_t((p0 >> 21) & 0xFFU);
            uPtr[3] = uint8_t((p0 >> 11) & 0xFFU);
          }
          break;
        case 0x10:
          {
            uint32_t  p0 = colormap[*(bufp++)];
            uint32_t  p1 = colormap[*(bufp++)];
            uint32_t  p2 = colormap[*(bufp++)];
            uint32_t  p3 = colormap[*(bufp++)];
            yPtr[0] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[1] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[0] = uint8_t((p0 >> 22) & 0xFFU);
            uPtr[0] = uint8_t((p0 >> 12) & 0xFFU);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            p2 = colormap[*(bufp++)];
            p3 = colormap[*(bufp++)];
            yPtr[2] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[3] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[1] = uint8_t((p0 >> 22) & 0xFFU);
            uPtr[1] = uint8_t((p0 >> 12) & 0xFFU);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            p2 = colormap[*(bufp++)];
            p3 = colormap[*(bufp++)];
            yPtr[4] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[5] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[2] = uint8_t((p0 >> 22) & 0xFFU);
            uPtr[2] = uint8_t((p0 >> 12) & 0xFFU);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            p2 = colormap[*(bufp++)];
            p3 = colormap[*(bufp++)];
            yPtr[6] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[7] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[3] = uint8_t((p0 >> 22) & 0xFFU);
            uPtr[3] = uint8_t((p0 >> 12) & 0xFFU);
          }
          break;
        default:
          yPtr[7] = yPtr[6] = yPtr[5] = yPtr[4] =
          yPtr[3] = yPtr[2] = yPtr[1] = yPtr[0] = 0x00;
          vPtr[3] = vPtr[2] = vPtr[1] = vPtr[0] = 0x00;
          uPtr[3] = uPtr[2] = uPtr[1] = uPtr[0] = 0x00;
          break;
        }
        yPtr = yPtr + 8;
        vPtr = vPtr + 4;
        uPtr = uPtr + 4;
      }
    }
    else {
      for (size_t i = 0; i < 48; i++) {
        uint8_t c = *(bufp++);
        switch (c) {
        case 0x01:
          {
            uint32_t  tmp = colormap[*(bufp++)];
            yPtr[7] = yPtr[6] = yPtr[5] = yPtr[4] =
            yPtr[3] = yPtr[2] = yPtr[1] = yPtr[0] = uint8_t(tmp & 0xFFU);
            uint32_t  v = (tmp >> 20) & 0xFFU;
            uint32_t  u = (tmp >> 10) & 0xFFU;
            vPtr[0] = uint8_t((uint32_t(vPtr[0]) + v + 1U) >> 1);
            vPtr[1] = uint8_t((uint32_t(vPtr[1]) + v + 1U) >> 1);
            vPtr[2] = uint8_t((uint32_t(vPtr[2]) + v + 1U) >> 1);
            vPtr[3] = uint8_t((uint32_t(vPtr[3]) + v + 1U) >> 1);
            uPtr[0] = uint8_t((uint32_t(uPtr[0]) + u + 1U) >> 1);
            uPtr[1] = uint8_t((uint32_t(uPtr[1]) + u + 1U) >> 1);
            uPtr[2] = uint8_t((uint32_t(uPtr[2]) + u + 1U) >> 1);
            uPtr[3] = uint8_t((uint32_t(uPtr[3]) + u + 1U) >> 1);
          }
          break;
        case 0x02:
          {
            uint32_t  tmp = colormap[*(bufp++)];
            yPtr[3] = yPtr[2] = yPtr[1] = yPtr[0] = uint8_t(tmp & 0xFFU);
            uint32_t  v = (tmp >> 20) & 0xFFU;
            uint32_t  u = (tmp >> 10) & 0xFFU;
            vPtr[0] = uint8_t((uint32_t(vPtr[0]) + v + 1U) >> 1);
            vPtr[1] = uint8_t((uint32_t(vPtr[1]) + v + 1U) >> 1);
            uPtr[0] = uint8_t((uint32_t(uPtr[0]) + u + 1U) >> 1);
            uPtr[1] = uint8_t((uint32_t(uPtr[1]) + u + 1U) >> 1);
            tmp = colormap[*(bufp++)];
            yPtr[7] = yPtr[6] = yPtr[5] = yPtr[4] = uint8_t(tmp & 0xFFU);
            v = (tmp >> 20) & 0xFFU;
            u = (tmp >> 10) & 0xFFU;
            vPtr[2] = uint8_t((uint32_t(vPtr[2]) + v + 1U) >> 1);
            vPtr[3] = uint8_t((uint32_t(vPtr[3]) + v + 1U) >> 1);
            uPtr[2] = uint8_t((uint32_t(uPtr[2]) + u + 1U) >> 1);
            uPtr[3] = uint8_t((uint32_t(uPtr[3]) + u + 1U) >> 1);
          }
          break;
        case 0x03:
          {
            unsigned char c0 = *(bufp++);
            unsigned char c1 = *(bufp++);
            unsigned char b = *(bufp++);
            uint32_t  p0 = colormap[((b & 128) ? c1 : c0)];
            uint32_t  p1 = colormap[((b &  64) ? c1 : c0)];
            yPtr[0] = uint8_t(p0 & 0xFFU);
            yPtr[1] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[0] =
                uint8_t((uint32_t(vPtr[0]) + ((p0 >> 21) & 0xFFU) + 1U) >> 1);
            uPtr[0] =
                uint8_t((uint32_t(uPtr[0]) + ((p0 >> 11) & 0xFFU) + 1U) >> 1);
            p0 = colormap[((b &  32) ? c1 : c0)];
            p1 = colormap[((b &  16) ? c1 : c0)];
            yPtr[2] = uint8_t(p0 & 0xFFU);
            yPtr[3] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[1] =
                uint8_t((uint32_t(vPtr[1]) + ((p0 >> 21) & 0xFFU) + 1U) >> 1);
            uPtr[1] =
                uint8_t((uint32_t(uPtr[1]) + ((p0 >> 11) & 0xFFU) + 1U) >> 1);
            p0 = colormap[((b &   8) ? c1 : c0)];
            p1 = colormap[((b &   4) ? c1 : c0)];
            yPtr[4] = uint8_t(p0 & 0xFFU);
            yPtr[5] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[2] =
                uint8_t((uint32_t(vPtr[2]) + ((p0 >> 21) & 0xFFU) + 1U) >> 1);
            uPtr[2] =
                uint8_t((uint32_t(uPtr[2]) + ((p0 >> 11) & 0xFFU) + 1U) >> 1);
            p0 = colormap[((b &   2) ? c1 : c0)];
            p1 = colormap[((b &   1) ? c1 : c0)];
            yPtr[6] = uint8_t(p0 & 0xFFU);
            yPtr[7] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[3] =
                uint8_t((uint32_t(vPtr[3]) + ((p0 >> 21) & 0xFFU) + 1U) >> 1);
            uPtr[3] =
                uint8_t((uint32_t(uPtr[3]) + ((p0 >> 11) & 0xFFU) + 1U) >> 1);
          }
          break;
        case 0x04:
          {
            uint32_t  tmp = colormap[*(bufp++)];
            yPtr[1] = yPtr[0] = uint8_t(tmp & 0xFFU);
            uint32_t  v = (tmp >> 20) & 0xFFU;
            uint32_t  u = (tmp >> 10) & 0xFFU;
            vPtr[0] = uint8_t((uint32_t(vPtr[0]) + v + 1U) >> 1);
            uPtr[0] = uint8_t((uint32_t(uPtr[0]) + u + 1U) >> 1);
            tmp = colormap[*(bufp++)];
            yPtr[3] = yPtr[2] = uint8_t(tmp & 0xFFU);
            v = (tmp >> 20) & 0xFFU;
            u = (tmp >> 10) & 0xFFU;
            vPtr[1] = uint8_t((uint32_t(vPtr[1]) + v + 1U) >> 1);
            uPtr[1] = uint8_t((uint32_t(uPtr[1]) + u + 1U) >> 1);
            tmp = colormap[*(bufp++)];
            yPtr[5] = yPtr[4] = uint8_t(tmp & 0xFFU);
            v = (tmp >> 20) & 0xFFU;
            u = (tmp >> 10) & 0xFFU;
            vPtr[2] = uint8_t((uint32_t(vPtr[2]) + v + 1U) >> 1);
            uPtr[2] = uint8_t((uint32_t(uPtr[2]) + u + 1U) >> 1);
            tmp = colormap[*(bufp++)];
            yPtr[7] = yPtr[6] = uint8_t(tmp & 0xFFU);
            v = (tmp >> 20) & 0xFFU;
            u = (tmp >> 10) & 0xFFU;
            vPtr[3] = uint8_t((uint32_t(vPtr[3]) + v + 1U) >> 1);
            uPtr[3] = uint8_t((uint32_t(uPtr[3]) + u + 1U) >> 1);
          }
          break;
        case 0x06:
          {
            unsigned char c0 = *(bufp++);
            unsigned char c1 = *(bufp++);
            unsigned char b = *(bufp++);
            uint32_t  p0 = colormap[((b & 128) ? c1 : c0)];
            uint32_t  p1 = colormap[((b &  64) ? c1 : c0)];
            uint32_t  p2 = colormap[((b &  32) ? c1 : c0)];
            uint32_t  p3 = colormap[((b &  16) ? c1 : c0)];
            yPtr[0] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[1] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[0] =
                uint8_t((uint32_t(vPtr[0]) + ((p0 >> 22) & 0xFFU) + 1U) >> 1);
            uPtr[0] =
                uint8_t((uint32_t(uPtr[0]) + ((p0 >> 12) & 0xFFU) + 1U) >> 1);
            p0 = colormap[((b &   8) ? c1 : c0)];
            p1 = colormap[((b &   4) ? c1 : c0)];
            p2 = colormap[((b &   2) ? c1 : c0)];
            p3 = colormap[((b &   1) ? c1 : c0)];
            yPtr[2] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[3] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[1] =
                uint8_t((uint32_t(vPtr[1]) + ((p0 >> 22) & 0xFFU) + 1U) >> 1);
            uPtr[1] =
                uint8_t((uint32_t(uPtr[1]) + ((p0 >> 12) & 0xFFU) + 1U) >> 1);
            c0 = *(bufp++);
            c1 = *(bufp++);
            b = *(bufp++);
            p0 = colormap[((b & 128) ? c1 : c0)];
            p1 = colormap[((b &  64) ? c1 : c0)];
            p2 = colormap[((b &  32) ? c1 : c0)];
            p3 = colormap[((b &  16) ? c1 : c0)];
            yPtr[4] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[5] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[2] =
                uint8_t((uint32_t(vPtr[2]) + ((p0 >> 22) & 0xFFU) + 1U) >> 1);
            uPtr[2] =
                uint8_t((uint32_t(uPtr[2]) + ((p0 >> 12) & 0xFFU) + 1U) >> 1);
            p0 = colormap[((b &   8) ? c1 : c0)];
            p1 = colormap[((b &   4) ? c1 : c0)];
            p2 = colormap[((b &   2) ? c1 : c0)];
            p3 = colormap[((b &   1) ? c1 : c0)];
            yPtr[6] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[7] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[3] =
                uint8_t((uint32_t(vPtr[3]) + ((p0 >> 22) & 0xFFU) + 1U) >> 1);
            uPtr[3] =
                uint8_t((uint32_t(uPtr[3]) + ((p0 >> 12) & 0xFFU) + 1U) >> 1);
          }
          break;
        case 0x08:
          {
            uint32_t  p0 = colormap[*(bufp++)];
            uint32_t  p1 = colormap[*(bufp++)];
            yPtr[0] = uint8_t(p0 & 0xFFU);
            yPtr[1] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[0] =
                uint8_t((uint32_t(vPtr[0]) + ((p0 >> 21) & 0xFFU) + 1U) >> 1);
            uPtr[0] =
                uint8_t((uint32_t(uPtr[0]) + ((p0 >> 11) & 0xFFU) + 1U) >> 1);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            yPtr[2] = uint8_t(p0 & 0xFFU);
            yPtr[3] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[1] =
                uint8_t((uint32_t(vPtr[1]) + ((p0 >> 21) & 0xFFU) + 1U) >> 1);
            uPtr[1] =
                uint8_t((uint32_t(uPtr[1]) + ((p0 >> 11) & 0xFFU) + 1U) >> 1);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            yPtr[4] = uint8_t(p0 & 0xFFU);
            yPtr[5] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[2] =
                uint8_t((uint32_t(vPtr[2]) + ((p0 >> 21) & 0xFFU) + 1U) >> 1);
            uPtr[2] =
                uint8_t((uint32_t(uPtr[2]) + ((p0 >> 11) & 0xFFU) + 1U) >> 1);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            yPtr[6] = uint8_t(p0 & 0xFFU);
            yPtr[7] = uint8_t(p1 & 0xFFU);
            p0 = (p0 + p1) + 0x00100400U;
            vPtr[3] =
                uint8_t((uint32_t(vPtr[3]) + ((p0 >> 21) & 0xFFU) + 1U) >> 1);
            uPtr[3] =
                uint8_t((uint32_t(uPtr[3]) + ((p0 >> 11) & 0xFFU) + 1U) >> 1);
          }
          break;
        case 0x10:
          {
            uint32_t  p0 = colormap[*(bufp++)];
            uint32_t  p1 = colormap[*(bufp++)];
            uint32_t  p2 = colormap[*(bufp++)];
            uint32_t  p3 = colormap[*(bufp++)];
            yPtr[0] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[1] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[0] =
                uint8_t((uint32_t(vPtr[0]) + ((p0 >> 22) & 0xFFU) + 1U) >> 1);
            uPtr[0] =
                uint8_t((uint32_t(uPtr[0]) + ((p0 >> 12) & 0xFFU) + 1U) >> 1);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            p2 = colormap[*(bufp++)];
            p3 = colormap[*(bufp++)];
            yPtr[2] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[3] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[1] =
                uint8_t((uint32_t(vPtr[1]) + ((p0 >> 22) & 0xFFU) + 1U) >> 1);
            uPtr[1] =
                uint8_t((uint32_t(uPtr[1]) + ((p0 >> 12) & 0xFFU) + 1U) >> 1);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            p2 = colormap[*(bufp++)];
            p3 = colormap[*(bufp++)];
            yPtr[4] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[5] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[2] =
                uint8_t((uint32_t(vPtr[2]) + ((p0 >> 22) & 0xFFU) + 1U) >> 1);
            uPtr[2] =
                uint8_t((uint32_t(uPtr[2]) + ((p0 >> 12) & 0xFFU) + 1U) >> 1);
            p0 = colormap[*(bufp++)];
            p1 = colormap[*(bufp++)];
            p2 = colormap[*(bufp++)];
            p3 = colormap[*(bufp++)];
            yPtr[6] = uint8_t(((p0 + p1 + 1U) >> 1) & 0xFFU);
            yPtr[7] = uint8_t(((p2 + p3 + 1U) >> 1) & 0xFFU);
            p0 = (p0 + p1 + p2 + p3) + 0x00200800U;
            vPtr[3] =
                uint8_t((uint32_t(vPtr[3]) + ((p0 >> 22) & 0xFFU) + 1U) >> 1);
            uPtr[3] =
                uint8_t((uint32_t(uPtr[3]) + ((p0 >> 12) & 0xFFU) + 1U) >> 1);
          }
          break;
        default:
          yPtr[7] = yPtr[6] = yPtr[5] = yPtr[4] =
          yPtr[3] = yPtr[2] = yPtr[1] = yPtr[0] = 0x00;
          vPtr[0] = uint8_t((uint32_t(vPtr[0]) + 1U) >> 1);
          vPtr[1] = uint8_t((uint32_t(vPtr[1]) + 1U) >> 1);
          vPtr[2] = uint8_t((uint32_t(vPtr[2]) + 1U) >> 1);
          vPtr[3] = uint8_t((uint32_t(vPtr[3]) + 1U) >> 1);
          uPtr[0] = uint8_t((uint32_t(uPtr[0]) + 1U) >> 1);
          uPtr[1] = uint8_t((uint32_t(uPtr[1]) + 1U) >> 1);
          uPtr[2] = uint8_t((uint32_t(uPtr[2]) + 1U) >> 1);
          uPtr[3] = uint8_t((uint32_t(uPtr[3]) + 1U) >> 1);
          break;
        }
        yPtr = yPtr + 8;
        vPtr = vPtr + 4;
        uPtr = uPtr + 4;
      }
    }
  }

  void VideoCapture_YV12::frameDone()
  {
    resampleFrame();
    while (audioBufSamples >= (audioBufSize * 2)) {
      audioBufSamples -= (audioBufSize * 2);
      int64_t   frameTime =
          int64_t((4294967296000000.0 / double(frameRate)) + 0.5);
      if (frameTime > frame1Time)
        frameTime = frame1Time;
      int32_t   t0 =
          int32_t(((frameTime - frame0Time) + int64_t(0x80000000UL)) >> 32);
      int32_t   t1 =
          int32_t(((frame1Time - frameTime) + int64_t(0x80000000UL)) >> 32);
      double    tt = 3.1415926535898 * (double(t1) / (double(t0) + double(t1)));
      tt = 0.3183098861838 * (tt - std::sin(tt));
      int32_t   scaleFac0 = int32_t(double(t1) * tt + 0.5);
      int32_t   scaleFac1 = int32_t(double(t1) * (2.0 - tt) + 0.5);
      int32_t   outScale = int32_t(0x20000000) / (interpTime - t1);
      interpTime = t1;
      int       n = (videoWidth * videoHeight * 3) / 2;
      int       i = 0;
      uint8_t   frameChanged = 0x00;
      do {
        int32_t   tmp;
        uint8_t   tmp2;
        tmp = (int32_t(frameBuf0Y[i]) * scaleFac0)
              + (int32_t(frameBuf1Y[i]) * scaleFac1);
        tmp2 = uint8_t(((((interpBufY[i] - tmp) >> 8) * outScale)
                        + 0x00200000) >> 22);
        interpBufY[i] = tmp;
        frameChanged |= (tmp2 ^ outBufY[i]);
        outBufY[i] = tmp2;
        i++;
        tmp = (int32_t(frameBuf0Y[i]) * scaleFac0)
              + (int32_t(frameBuf1Y[i]) * scaleFac1);
        tmp2 = uint8_t(((((interpBufY[i] - tmp) >> 8) * outScale)
                        + 0x00200000) >> 22);
        interpBufY[i] = tmp;
        frameChanged |= (tmp2 ^ outBufY[i]);
        outBufY[i] = tmp2;
      } while (++i < n);
      writeFrame(bool(frameChanged));
      audioBufReadPos += (audioBufSize * 2);
      while (audioBufReadPos >= (audioBufSize * audioBuffers * 2))
        audioBufReadPos -= (audioBufSize * audioBuffers * 2);
      frame0Time -= frameTime;
      frame1Time -= frameTime;
      curTime -= frameTime;
    }
    int64_t   frameTime =
        ((int64_t(audioBufSamples * 5000) << 32) + int64_t(sampleRate / 200))
        / int64_t(sampleRate / 100);
    curTime += (frameTime - frame1Time);
    frame0Time += (frameTime - frame1Time);
    frame1Time = frameTime;
    uint8_t   *tmp = frameBuf0Y;
    frameBuf0Y = frameBuf1Y;
    frameBuf1Y = tmp;
    tmp = frameBuf0V;
    frameBuf0V = frameBuf1V;
    frameBuf1V = tmp;
    tmp = frameBuf0U;
    frameBuf0U = frameBuf1U;
    frameBuf1U = tmp;
    std::memset(frameBuf1Y, 0x10, size_t(videoWidth * videoHeight));
    std::memset(frameBuf1V, 0x80,
                size_t((videoWidth >> 1) * (videoHeight >> 1)));
    std::memset(frameBuf1U, 0x80,
                size_t((videoWidth >> 1) * (videoHeight >> 1)));
  }

  void VideoCapture_YV12::resampleFrame()
  {
    frame0Time = frame1Time;
    frame1Time = curTime;
    int32_t   scaleFac =
        int32_t(((frame1Time - frame0Time) + int64_t(0x80000000UL)) >> 32);
    interpTime += scaleFac;
    int       n = (videoWidth * videoHeight * 3) / 2;
    int       i = 0;
    do {
      interpBufY[i] +=
          ((int32_t(frameBuf0Y[i]) + int32_t(frameBuf1Y[i])) * scaleFac);
      i++;
      interpBufY[i] +=
          ((int32_t(frameBuf0Y[i]) + int32_t(frameBuf1Y[i])) * scaleFac);
    } while (++i < n);
  }

  void VideoCapture_YV12::writeFrame(bool frameChanged)
  {
    if (!aviFile)
      return;
    if (!frameChanged) {
      if (framesWritten == 0 || duplicateFrames >= size_t(frameRate))
        frameChanged = true;
      else
        duplicateFrames++;
    }
    if (frameChanged) {
      duplicateFrames = 0;
      duplicateFrameBitmap[framesWritten >> 3] &=
          uint8_t((1 << (framesWritten & 7)) ^ 0xFF);
    }
    else {
      duplicateFrameBitmap[framesWritten >> 3] |=
          uint8_t(1 << (framesWritten & 7));
    }
    try {
      if (fileSize >= 0x7F800000) {
        closeFile();
        try {
          errorMessage("AVI file is too large, starting new output file");
        }
        catch (...) {
        }
        std::string fileName = "";
        fileNameCallback(fileNameCallbackUserData, fileName);
        if (fileName.length() < 1)
          return;
        openFile(fileName.c_str());
      }
      if (std::fseek(aviFile, 0L, SEEK_END) < 0)
        throw Exception("error seeking AVI file");
      uint8_t headerBuf[8];
      uint8_t *bufp = &(headerBuf[0]);
      size_t  nBytes = 0;
      if (frameChanged)
        nBytes = size_t((videoWidth * videoHeight * 3) / 2);
      aviHeader_writeFourCC(bufp, "00dc");
      aviHeader_writeUInt32(bufp, uint32_t(nBytes));
      fileSize = fileSize + 8;
      if (std::fwrite(&(headerBuf[0]), 1, 8, aviFile) != 8)
        throw Exception("error writing AVI file");
      if (nBytes > 0) {
        fileSize = fileSize + nBytes;
        if (std::fwrite(&(outBufY[0]), 1, nBytes, aviFile) != nBytes)
          throw Exception("error writing AVI file");
      }
      bufp = &(headerBuf[0]);
      nBytes = size_t(audioBufSize * 4);
      aviHeader_writeFourCC(bufp, "01wb");
      aviHeader_writeUInt32(bufp, uint32_t(nBytes));
      fileSize = fileSize + 8;
      if (std::fwrite(&(headerBuf[0]), 1, 8, aviFile) != 8)
        throw Exception("error writing AVI file");
      int     bufPos = audioBufReadPos;
      for (int i = 0; i < (audioBufSize * 2); i++) {
        if (bufPos >= (audioBufSize * audioBuffers * 2))
          bufPos = 0;
        int16_t tmp = audioBuf[bufPos++];
        fileSize++;
        if (std::fputc(int(uint16_t(tmp) & 0xFF), aviFile) == EOF)
          throw Exception("error writing AVI file");
        fileSize++;
        if (std::fputc(int((uint16_t(tmp) >> 8) & 0xFF), aviFile) == EOF)
          throw Exception("error writing AVI file");
      }
    }
    catch (std::exception& e) {
      closeFile();
      errorMessage(e.what());
      return;
    }
    framesWritten++;
    if (!(framesWritten & 31)) {
      try {
        writeAVIHeader();
      }
      catch (std::exception& e) {
        errorMessage(e.what());
      }
    }
  }

  void VideoCapture_YV12::writeAVIHeader()
  {
    if (!aviFile)
      return;
    try {
      if (std::fseek(aviFile, 0L, SEEK_SET) < 0)
        throw Exception("error seeking AVI file");
      uint8_t   headerBuf[512];
      uint8_t   *bufp = &(headerBuf[0]);
      size_t    frameSize = size_t(((videoWidth * videoHeight * 3) / 2)
                                   + (audioBufSize * 4) + 16);
      aviHeader_writeFourCC(bufp, "RIFF");
      aviHeader_writeUInt32(bufp, uint32_t(fileSize - 8));
      aviHeader_writeFourCC(bufp, "AVI ");
      aviHeader_writeFourCC(bufp, "LIST");
      aviHeader_writeUInt32(bufp, 0x00000126U);
      aviHeader_writeFourCC(bufp, "hdrl");
      aviHeader_writeFourCC(bufp, "avih");
      aviHeader_writeUInt32(bufp, 0x00000038U);
      // microseconds per frame
      aviHeader_writeUInt32(bufp,
                            uint32_t((1000000 + (frameRate >> 1)) / frameRate));
      // max. bytes per second
      aviHeader_writeUInt32(bufp, uint32_t(frameSize * size_t(frameRate)));
      // padding
      aviHeader_writeUInt32(bufp, 0x00000001U);
      // flags (AVIF_HASINDEX | AVIF_ISINTERLEAVED | AVIF_TRUSTCKTYPE)
      aviHeader_writeUInt32(bufp, 0x00000910U);
      // total frames
      aviHeader_writeUInt32(bufp, uint32_t(framesWritten));
      // initial frames
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // number of streams
      aviHeader_writeUInt32(bufp, 0x00000002U);
      // suggested buffer size
      aviHeader_writeUInt32(bufp, uint32_t(frameSize));
      // width
      aviHeader_writeUInt32(bufp, uint32_t(videoWidth));
      // height
      aviHeader_writeUInt32(bufp, uint32_t(videoHeight));
      // reserved
      aviHeader_writeUInt32(bufp, 0x00000000U);
      aviHeader_writeUInt32(bufp, 0x00000000U);
      aviHeader_writeUInt32(bufp, 0x00000000U);
      aviHeader_writeUInt32(bufp, 0x00000000U);
      aviHeader_writeFourCC(bufp, "LIST");
      aviHeader_writeUInt32(bufp, 0x00000074U);
      aviHeader_writeFourCC(bufp, "strl");
      aviHeader_writeFourCC(bufp, "strh");
      aviHeader_writeUInt32(bufp, 0x00000038U);
      aviHeader_writeFourCC(bufp, "vids");
      // video codec
      aviHeader_writeFourCC(bufp, "YV12");
      // flags
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // priority
      aviHeader_writeUInt16(bufp, 0x0000);
      // language
      aviHeader_writeUInt16(bufp, 0x0000);
      // initial frames
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // scale
      aviHeader_writeUInt32(bufp, 0x00000001U);
      // rate
      aviHeader_writeUInt32(bufp, uint32_t(frameRate));
      // start time
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // length
      aviHeader_writeUInt32(bufp, uint32_t(framesWritten));
      // suggested buffer size
      aviHeader_writeUInt32(bufp, uint32_t((videoWidth * videoHeight * 3) / 2));
      // quality
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // sample size
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // left
      aviHeader_writeUInt16(bufp, 0x0000);
      // top
      aviHeader_writeUInt16(bufp, 0x0000);
      // right
      aviHeader_writeUInt16(bufp, uint16_t(videoWidth));
      // bottom
      aviHeader_writeUInt16(bufp, uint16_t(videoHeight));
      aviHeader_writeFourCC(bufp, "strf");
      aviHeader_writeUInt32(bufp, 0x00000028U);
      aviHeader_writeUInt32(bufp, 0x00000028U);
      // width
      aviHeader_writeUInt32(bufp, uint32_t(videoWidth));
      // height
      aviHeader_writeUInt32(bufp, uint32_t(videoHeight));
      // planes
      aviHeader_writeUInt16(bufp, 0x0001);
      // bits per pixel
      aviHeader_writeUInt16(bufp, 0x0018);
      // compression
      aviHeader_writeFourCC(bufp, "YV12");
      // image size in bytes
      aviHeader_writeUInt32(bufp, uint32_t(videoWidth * videoHeight * 3));
      // X resolution
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // Y resolution
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // color indexes used
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // color indexes required
      aviHeader_writeUInt32(bufp, 0x00000000U);
      aviHeader_writeFourCC(bufp, "LIST");
      aviHeader_writeUInt32(bufp, 0x0000005EU);
      aviHeader_writeFourCC(bufp, "strl");
      aviHeader_writeFourCC(bufp, "strh");
      aviHeader_writeUInt32(bufp, 0x00000038U);
      aviHeader_writeFourCC(bufp, "auds");
      // audio codec (WAVE_FORMAT_PCM)
      aviHeader_writeUInt32(bufp, 0x00000001U);
      // flags
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // priority
      aviHeader_writeUInt16(bufp, 0x0000);
      // language
      aviHeader_writeUInt16(bufp, 0x0000);
      // initial frames
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // scale
      aviHeader_writeUInt32(bufp, 0x00000001U);
      // rate
      aviHeader_writeUInt32(bufp, uint32_t(sampleRate));
      // start time
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // length
      aviHeader_writeUInt32(bufp, uint32_t(framesWritten
                                           * size_t(audioBufSize)));
      // suggested buffer size
      aviHeader_writeUInt32(bufp, uint32_t(audioBufSize * 4));
      // quality
      aviHeader_writeUInt32(bufp, 0x00000000U);
      // sample size
      aviHeader_writeUInt32(bufp, 0x00000004U);
      // left
      aviHeader_writeUInt16(bufp, 0x0000);
      // top
      aviHeader_writeUInt16(bufp, 0x0000);
      // right
      aviHeader_writeUInt16(bufp, 0x0000);
      // bottom
      aviHeader_writeUInt16(bufp, 0x0000);
      aviHeader_writeFourCC(bufp, "strf");
      aviHeader_writeUInt32(bufp, 0x00000012U);
      // audio format (WAVE_FORMAT_PCM)
      aviHeader_writeUInt16(bufp, 0x0001);
      // audio channels
      aviHeader_writeUInt16(bufp, 0x0002);
      // samples per second
      aviHeader_writeUInt32(bufp, uint32_t(sampleRate));
      // bytes per second
      aviHeader_writeUInt32(bufp, uint32_t(sampleRate * 4));
      // block alignment
      aviHeader_writeUInt16(bufp, 0x0004);
      // bits per sample
      aviHeader_writeUInt16(bufp, 0x0010);
      // additional format information size
      aviHeader_writeUInt16(bufp, 0x0000);
      aviHeader_writeFourCC(bufp, "LIST");
      aviHeader_writeUInt32(bufp, uint32_t((fileSize - aviHeaderSize) + 4));
      aviHeader_writeFourCC(bufp, "movi");
      size_t  nBytes = size_t(bufp - (&(headerBuf[0])));
      if (std::fwrite(&(headerBuf[0]), 1, nBytes, aviFile) != nBytes)
        throw Exception("error writing AVI file header");
      if (std::fflush(aviFile) != 0)
        throw Exception("error writing AVI file header");
    }
    catch (...) {
      std::fclose(aviFile);
      aviFile = (std::FILE *) 0;
      framesWritten = 0;
      duplicateFrames = 0;
      fileSize = 0;
      throw;
    }
  }

  void VideoCapture_YV12::writeAVIIndex()
  {
    if (!aviFile)
      return;
    try {
      if (std::fseek(aviFile, 0L, SEEK_END) < 0)
        throw Exception("error seeking AVI file");
      uint8_t   tmpBuf[32];
      uint8_t   *bufp = &(tmpBuf[0]);
      aviHeader_writeFourCC(bufp, "idx1");
      aviHeader_writeUInt32(bufp, uint32_t(framesWritten << 5));
      fileSize = fileSize + 8;
      if (std::fwrite(&(tmpBuf[0]), 1, 8, aviFile) != 8)
        throw Exception("error writing AVI file index");
      size_t    filePos = 4;
      for (size_t i = 0; i < framesWritten; i++) {
        bufp = &(tmpBuf[0]);
        aviHeader_writeFourCC(bufp, "00dc");
        size_t    frameBytes = 0;
        if (!(duplicateFrameBitmap[i >> 3] & uint8_t(1 << (i & 7)))) {
          aviHeader_writeUInt32(bufp, 0x00000010U);     // AVIIF_KEYFRAME
          frameBytes = size_t((videoWidth * videoHeight * 3) / 2);
        }
        else {
          aviHeader_writeUInt32(bufp, 0x00000000U);
        }
        aviHeader_writeUInt32(bufp, uint32_t(filePos));
        filePos = filePos + frameBytes + 8;
        aviHeader_writeUInt32(bufp, uint32_t(frameBytes));
        aviHeader_writeFourCC(bufp, "01wb");
        aviHeader_writeUInt32(bufp, 0x00000010U);       // AVIIF_KEYFRAME
        aviHeader_writeUInt32(bufp, uint32_t(filePos));
        frameBytes = size_t(audioBufSize) << 2;
        filePos = filePos + frameBytes + 8;
        aviHeader_writeUInt32(bufp, uint32_t(frameBytes));
        fileSize = fileSize + 32;
        if (std::fwrite(&(tmpBuf[0]), 1, 32, aviFile) != 32)
          throw Exception("error writing AVI file index");
      }
      if (std::fseek(aviFile, 0L, SEEK_SET) < 0)
        throw Exception("error seeking AVI file");
      bufp = &(tmpBuf[0]);
      aviHeader_writeFourCC(bufp, "RIFF");
      aviHeader_writeUInt32(bufp, uint32_t(fileSize - 8));
      if (std::fwrite(&(tmpBuf[0]), 1, 8, aviFile) != 8)
        throw Exception("error writing AVI file index");
      if (std::fflush(aviFile) != 0)
        throw Exception("error writing AVI file index");
    }
    catch (...) {
      std::fclose(aviFile);
      aviFile = (std::FILE *) 0;
      framesWritten = 0;
      duplicateFrames = 0;
      fileSize = 0;
      throw;
    }
  }

}       // namespace Ep128Emu

