
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
      lineBuf((uint8_t *) 0),
      tmpFrameBuf((uint8_t *) 0),
      outputFrameBuf((uint8_t *) 0),
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
      lineBufBytes(0),
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
      size_t    bufSize1 = (size_t(videoWidth * videoHeight) + 3) >> 2;
      size_t    bufSize2 = 1024 / 4;
      size_t    totalSize = bufSize2 + bufSize1 + bufSize1;
      uint32_t  *videoBuf = new uint32_t[totalSize];
      totalSize = 0;
      lineBuf = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(lineBuf, 0x00, 1024);
      totalSize += bufSize2;
      tmpFrameBuf = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(tmpFrameBuf, 0x00, bufSize1 * sizeof(uint32_t));
      totalSize += bufSize1;
      outputFrameBuf = reinterpret_cast<uint8_t *>(&(videoBuf[totalSize]));
      std::memset(outputFrameBuf, 0x00, bufSize1 * sizeof(uint32_t));
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
      if (lineBuf)
        delete[] reinterpret_cast<uint32_t *>(lineBuf);
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
    delete[] reinterpret_cast<uint32_t *>(lineBuf);
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
    uint8_t   c = videoInput[0];
    lineBuf[lineBufBytes++] = c;
    for (uint8_t i = 0; i < c; i++)
      lineBuf[lineBufBytes++] = videoInput[i + 1];
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
      vsyncCnt = (hsyncCnt < 51 ? -18 : -19);
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
    if (curLine >= 0 && curLine < 578)
      decodeLine();
    hsyncCnt = 0;
    lineBufBytes = 0;
    if (vsyncCnt != 0) {
      curLine += 2;
      if (vsyncCnt >= 262 && (vsyncState || vsyncCnt >= 336))
        vsyncCnt = -18;
      vsyncCnt++;
    }
    else {
      prvOddFrame = bool(curLine & 1);
      for (int i = (curLine + 1); i < videoHeight; i++)
        std::memset(&(tmpFrameBuf[i * videoWidth]), 0x00, size_t(videoWidth));
      curLine = (oddFrame ? -1 : 0);
      vsyncCnt++;
      oddFrame = false;
      frameDone();
    }
  }

  void VideoCapture::decodeLine()
  {
    int       lineNum = curLine - 2;
    if (lineNum < 0 || lineNum >= 576)
      return;

    uint8_t       *outBuf = &(tmpFrameBuf[lineNum * videoWidth]);
    const uint8_t *bufp = lineBuf;

    for (size_t i = 0; i < 48; i++) {
      uint8_t c = *(bufp++);
      switch (c) {
      case 0x01:
        outBuf[15] = outBuf[14] = outBuf[13] = outBuf[12] =
        outBuf[11] = outBuf[10] = outBuf[9]  = outBuf[8]  =
        outBuf[7]  = outBuf[6]  = outBuf[5]  = outBuf[4]  =
        outBuf[3]  = outBuf[2]  = outBuf[1]  = outBuf[0]  = *(bufp++);
        break;
      case 0x02:
        outBuf[7]  = outBuf[6]  = outBuf[5]  = outBuf[4]  =
        outBuf[3]  = outBuf[2]  = outBuf[1]  = outBuf[0]  = *(bufp++);
        outBuf[15] = outBuf[14] = outBuf[13] = outBuf[12] =
        outBuf[11] = outBuf[10] = outBuf[9]  = outBuf[8]  = *(bufp++);
        break;
      case 0x03:
        {
          uint8_t c0 = *(bufp++);
          uint8_t c1 = *(bufp++);
          uint8_t b = *(bufp++);
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
        outBuf[3]  = outBuf[2]  = outBuf[1]  = outBuf[0]  = *(bufp++);
        outBuf[7]  = outBuf[6]  = outBuf[5]  = outBuf[4]  = *(bufp++);
        outBuf[11] = outBuf[10] = outBuf[9]  = outBuf[8]  = *(bufp++);
        outBuf[15] = outBuf[14] = outBuf[13] = outBuf[12] = *(bufp++);
        break;
      case 0x06:
        {
          uint8_t c0 = *(bufp++);
          uint8_t c1 = *(bufp++);
          uint8_t b = *(bufp++);
          outBuf[0]  = ((b & 128) ? c1 : c0);
          outBuf[1]  = ((b &  64) ? c1 : c0);
          outBuf[2]  = ((b &  32) ? c1 : c0);
          outBuf[3]  = ((b &  16) ? c1 : c0);
          outBuf[4]  = ((b &   8) ? c1 : c0);
          outBuf[5]  = ((b &   4) ? c1 : c0);
          outBuf[6]  = ((b &   2) ? c1 : c0);
          outBuf[7]  = ((b &   1) ? c1 : c0);
          c0 = *(bufp++);
          c1 = *(bufp++);
          b = *(bufp++);
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
        outBuf[1]  = outBuf[0]  = *(bufp++);
        outBuf[3]  = outBuf[2]  = *(bufp++);
        outBuf[5]  = outBuf[4]  = *(bufp++);
        outBuf[7]  = outBuf[6]  = *(bufp++);
        outBuf[9]  = outBuf[8]  = *(bufp++);
        outBuf[11] = outBuf[10] = *(bufp++);
        outBuf[13] = outBuf[12] = *(bufp++);
        outBuf[15] = outBuf[14] = *(bufp++);
        break;
      case 0x10:
        outBuf[0]  = *(bufp++);
        outBuf[1]  = *(bufp++);
        outBuf[2]  = *(bufp++);
        outBuf[3]  = *(bufp++);
        outBuf[4]  = *(bufp++);
        outBuf[5]  = *(bufp++);
        outBuf[6]  = *(bufp++);
        outBuf[7]  = *(bufp++);
        outBuf[8]  = *(bufp++);
        outBuf[9]  = *(bufp++);
        outBuf[10] = *(bufp++);
        outBuf[11] = *(bufp++);
        outBuf[12] = *(bufp++);
        outBuf[13] = *(bufp++);
        outBuf[14] = *(bufp++);
        outBuf[15] = *(bufp++);
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

    if (bool(lineNum & 1) == prvOddFrame) {
      // no interlace, need to duplicate line
      std::memcpy(&(tmpFrameBuf[(lineNum ^ 1) * videoWidth]),
                  &(tmpFrameBuf[lineNum * videoWidth]), size_t(videoWidth));
    }
  }

  void VideoCapture::frameDone()
  {
    if (audioBufSamples >= (audioBufSize * 2)) {
      bool    frameChanged = false;
      for (int i = 0; i < videoHeight; i++) {
        if (std::memcmp(&(tmpFrameBuf[i * videoWidth]),
                        &(outputFrameBuf[i * videoWidth]),
                        size_t(videoWidth)) != 0) {
          frameChanged = true;
          std::memcpy(&(outputFrameBuf[i * videoWidth]),
                      &(tmpFrameBuf[i * videoWidth]),
                      size_t(videoWidth));
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
        for ( ; j < n; j++) {
          int     tmp = int(runLengths[j]);
          if (tmp >= 3 || (bytesToCopy + tmp) > 255)
            break;
          bytesToCopy += tmp;
        }
        if (bytesToCopy >= 3) {
          outBuf[nBytes++] = 0x00;
          outBuf[nBytes++] = uint8_t(bytesToCopy);
          for (int k = i; k < j; k++) {
            int     m = int(runLengths[k]);
            uint8_t tmp = byteValues[k];
            for (int l = 0; l < m; l++)
              outBuf[nBytes++] = tmp;
          }
          if (bytesToCopy & 1)
            outBuf[nBytes++] = 0x00;
        }
        else {
          for (int k = i; k < j; k++) {
            outBuf[nBytes++] = runLengths[k];
            outBuf[nBytes++] = byteValues[k];
          }
        }
        i = j;
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
        uint8_t rleBuf[1024];
        for (int i = (videoHeight - 1); i >= 0; i--) {
          size_t  n = rleCompressLine(&(rleBuf[0]),
                                      &(outputFrameBuf[i * videoWidth]));
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

