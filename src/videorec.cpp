
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

static const size_t aviHeaderSize = 0x0546;

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

  VideoCapture::VideoCaptureFrameBuffer::VideoCaptureFrameBuffer(int w, int h)
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

  VideoCapture::VideoCaptureFrameBuffer::~VideoCaptureFrameBuffer()
  {
    delete[] buf;
    delete[] linePtrs;
    delete[] lineBytes_;
  }

  bool VideoCapture::VideoCaptureFrameBuffer::compareLine(
      long dstLine, const VideoCaptureFrameBuffer& src, long srcLine)
  {
    if (lineBytes_[dstLine] != src.lineBytes_[srcLine])
      return false;
    if (src.lineBytes_[srcLine] == 0U)
      return true;
    return (std::memcmp(linePtrs[dstLine], src.linePtrs[srcLine],
                        src.lineBytes_[srcLine]) == 0);
  }

  void VideoCapture::VideoCaptureFrameBuffer::copyLine(long dstLine,
                                                       long srcLine)
  {
    std::memcpy(linePtrs[dstLine], linePtrs[srcLine], lineBytes_[srcLine]);
    lineBytes_[dstLine] = lineBytes_[srcLine];
  }

  void VideoCapture::VideoCaptureFrameBuffer::copyLine(
      long dstLine, const VideoCaptureFrameBuffer& src, long srcLine)
  {
    std::memcpy(linePtrs[dstLine], src.linePtrs[srcLine],
                src.lineBytes_[srcLine]);
    lineBytes_[dstLine] = src.lineBytes_[srcLine];
  }

  void VideoCapture::VideoCaptureFrameBuffer::clearLine(long n)
  {
    uint8_t *p = linePtrs[n];
    for (int i = 0; i < 48; i++) {      // FIXME: assumes videoWidth == 768
      *(p++) = 0x01;
      *(p++) = 0x00;
    }
    lineBytes_[n] = 96U;
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

  // --------------------------------------------------------------------------

  VideoCapture::VideoCapture(
      void (*indexToRGBFunc)(uint8_t color, float& r, float& g, float& b),
      int frameRate_)
    : aviFile((std::FILE *) 0),
      tmpFrameBuf(videoWidth, videoHeight),
      outputFrameBuf(videoWidth, videoHeight),
      audioBuf((int16_t *) 0),
      frameSizes((uint32_t *) 0),
      frameRate(frameRate_),
      audioBufSize(0),
      audioBufReadPos(0),
      audioBufWritePos(0),
      audioBufSamples(0),
      clockFrequency(0),
      soundOutputAccumulatorL(0U),
      soundOutputAccumulatorR(0U),
      cycleCnt(0),
      curLine(0),
      vsyncCnt(0),
      hsyncCnt(0),
      vsyncState(false),
      oddFrame(false),
      prvOddFrame(false),
      framesWritten(0),
      duplicateFrames(0),
      fileSize(0),
      audioConverter((AudioConverter *) 0),
      colormap((uint8_t *) 0),
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
      size_t  maxFrames = 0x20000000 / size_t(audioBufSize);
      frameSizes = new uint32_t[maxFrames];
      std::memset(frameSizes, 0x00, maxFrames * sizeof(uint32_t));
      audioConverter =
          new AudioConverter_(*this, 222656.25f, float(sampleRate));
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
      if (audioBuf)
        delete[] audioBuf;
      if (frameSizes)
        delete[] frameSizes;
      if (audioConverter)
        delete audioConverter;
      if (colormap)
        delete[] colormap;
      throw;
    }
    setClockFrequency(890625);
  }

  VideoCapture::~VideoCapture()
  {
    closeFile();
    delete[] audioBuf;
    delete[] frameSizes;
    delete audioConverter;
    delete[] colormap;
  }

  void VideoCapture::runOneCycle(const uint8_t *videoInput, uint32_t audioInput)
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

  void VideoCapture::horizontalSync()
  {
    if (hsyncCnt >= 41)
      hsyncCnt = 51;
  }

  void VideoCapture::vsyncStateChange(bool newState, unsigned int currentSlot_)
  {
    vsyncState = newState;
    if (newState && vsyncCnt >= 262) {
      vsyncCnt = (hsyncCnt < 51 ? -19 : -20);
      oddFrame = (currentSlot_ >= 20U && currentSlot_ < 48U);
    }
  }

  void VideoCapture::setClockFrequency(size_t freq_)
  {
    if (freq_ == clockFrequency)
      return;
    clockFrequency = freq_;
    audioConverter->setInputSampleRate(float(long(freq_)) * 0.25f);
  }

  void VideoCapture::lineDone()
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
      if (vsyncCnt >= 262 && (vsyncState || vsyncCnt >= 336))
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

  void VideoCapture::frameDone()
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

  void VideoCapture::decodeLine(uint8_t *outBuf, const uint8_t *inBuf)
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

  size_t VideoCapture::rleCompressLine(uint8_t *outBuf, const uint8_t *inBuf)
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

  void VideoCapture::writeFrame(bool frameChanged)
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
        for (int i = (videoHeight - 1); i >= 0; i--) {
          decodeLine(&(lineBuf[0]), outputFrameBuf[i]);
          size_t  n = rleCompressLine(&(rleBuf[0]), &(lineBuf[0]));
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
    if (!(framesWritten & 15)) {
      try {
        writeAVIHeader();
      }
      catch (std::exception& e) {
        errorMessage(e.what());
      }
    }
  }

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

  void VideoCapture::writeAVIHeader()
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

  void VideoCapture::writeAVIIndex()
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

}       // namespace Ep128Emu

