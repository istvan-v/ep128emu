
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

#ifndef EP128EMU_VIDEOREC_HPP
#define EP128EMU_VIDEOREC_HPP

#include "ep128emu.hpp"
#include "display.hpp"
#include "snd_conv.hpp"

namespace Ep128Emu {

  class VideoCapture {
   public:
    static const int  sampleRate = 48000;
    static const int  audioBuffers = 8;
   protected:
    class AudioConverter_ : public AudioConverterHighQuality {
     private:
      VideoCapture& videoCapture;
     public:
      AudioConverter_(VideoCapture& videoCapture_,
                      float inputSampleRate_, float outputSampleRate_,
                      float dcBlockFreq1 = 10.0f, float dcBlockFreq2 = 10.0f,
                      float ampScale_ = 0.7071f);
      virtual ~AudioConverter_();
     protected:
      virtual void audioOutput(int16_t left, int16_t right);
    };
    // --------
    std::FILE   *aviFile;
    int16_t     *audioBuf;              // 8 * (sampleRate / frameRate) frames
    int         frameRate;              // video frames per second
    int         audioBufSize;           // = (sampleRate / frameRate)
    int         audioBufReadPos;
    int         audioBufWritePos;
    int         audioBufSamples;        // write position - read position
    size_t      clockFrequency;
    uint32_t    soundOutputAccumulatorL;
    uint32_t    soundOutputAccumulatorR;
    int         curLine;
    int         vsyncCnt;
    int         hsyncCnt;
    bool        vsyncState;
    bool        oddFrame;
    size_t      framesWritten;
    size_t      duplicateFrames;
    size_t      fileSize;
    AudioConverter  *audioConverter;
    size_t      aviHeaderSize;
    void        (*errorCallback)(void *userData, const char *msg);
    void        *errorCallbackUserData;
    void        (*fileNameCallback)(void *userData, std::string& fileName);
    void        *fileNameCallbackUserData;
    // ----------------
    static void aviHeader_writeFourCC(uint8_t*& bufp, const char *s);
    static void aviHeader_writeUInt16(uint8_t*& bufp, uint16_t n);
    static void aviHeader_writeUInt32(uint8_t*& bufp, uint32_t n);
    static void defaultErrorCallback(void *userData, const char *msg);
    static void defaultFileNameCallback(void *userData, std::string& fileName);
    virtual void writeAVIHeader() = 0;
    virtual void writeAVIIndex() = 0;
    void closeFile();
    void errorMessage(const char *msg);
   public:
    VideoCapture(int frameRate_ = 50);
    virtual ~VideoCapture();
    virtual void runOneCycle(const uint8_t *videoInput,
                             uint32_t audioInput) = 0;
    virtual void setClockFrequency(size_t freq_) = 0;
    void horizontalSync();
    void vsyncStateChange(bool newState, unsigned int currentSlot_);
    void openFile(const char *fileName);
    void setErrorCallback(void (*func)(void *userData, const char *msg),
                          void *userData_);
    void setFileNameCallback(void (*func)(void *userData,
                                          std::string& fileName),
                             void *userData_);
  };

  // --------------------------------------------------------------------------

  class VideoCapture_RLE8 : public VideoCapture {
   public:
    static const int  videoWidth = 768;
    static const int  videoHeight = 576;
   private:
    class VideoCaptureFrameBuffer {
     private:
      uint32_t  *buf;
      uint8_t   **linePtrs;
      uint32_t  *lineBytes_;
     public:
      VideoCaptureFrameBuffer(int w, int h);
      virtual ~VideoCaptureFrameBuffer();
      inline uint8_t * operator[](long n)
      {
        return linePtrs[n];
      }
      inline const uint8_t * operator[](long n) const
      {
        return linePtrs[n];
      }
      inline uint32_t& lineBytes(long n)
      {
        return lineBytes_[n];
      }
      inline const uint32_t& lineBytes(long n) const
      {
        return lineBytes_[n];
      }
      // returns true if the lines are identical
      bool compareLine(long dstLine,
                       const VideoCaptureFrameBuffer& src, long srcLine);
      void copyLine(long dstLine, long srcLine);
      void copyLine(long dstLine,
                    const VideoCaptureFrameBuffer& src, long srcLine);
      void clearLine(long n);
    };
    // --------
    VideoCaptureFrameBuffer tmpFrameBuf;    // 768x576
    VideoCaptureFrameBuffer outputFrameBuf; // 768x576
    uint32_t    *frameSizes;
    int         cycleCnt;
    bool        prvOddFrame;
    uint8_t     *colormap;
    // ----------------
    void lineDone();
    void frameDone();
    void decodeLine(uint8_t *outBuf, const uint8_t *inBuf);
    size_t rleCompressLine(uint8_t *outBuf, const uint8_t *inBuf);
    void writeFrame(bool frameChanged);
    virtual void writeAVIHeader();
    virtual void writeAVIIndex();
   public:
    VideoCapture_RLE8(void indexToRGBFunc(uint8_t color,
                                          float& r, float& g, float& b) =
                          (void (*)(uint8_t, float&, float&, float&)) 0,
                      int frameRate_ = 50);
    virtual ~VideoCapture_RLE8();
    virtual void runOneCycle(const uint8_t *videoInput, uint32_t audioInput);
    virtual void setClockFrequency(size_t freq_);
  };

  // --------------------------------------------------------------------------

  class VideoCapture_YV12 : public VideoCapture {
   public:
    static const int  videoWidth = 384;
    static const int  videoHeight = 288;
   private:
    uint8_t     *lineBuf;               // 1024 bytes
    uint8_t     *frameBuf0Y;            // 384x288
    uint8_t     *frameBuf0V;            // 192x144
    uint8_t     *frameBuf0U;            // 192x144
    uint8_t     *frameBuf1Y;            // 384x288
    uint8_t     *frameBuf1V;            // 192x144
    uint8_t     *frameBuf1U;            // 192x144
    int32_t     *interpBufY;            // 384x288
    int32_t     *interpBufV;            // 192x144
    int32_t     *interpBufU;            // 192x144
    uint8_t     *outBufY;               // 384x288
    uint8_t     *outBufV;               // 192x144
    uint8_t     *outBufU;               // 192x144
    uint8_t     *duplicateFrameBitmap;
    int64_t     timesliceLength;
    int64_t     curTime;
    int64_t     frame0Time;
    int64_t     frame1Time;
    int         cycleCnt;
    int32_t     interpTime;
    size_t      lineBufBytes;
    uint32_t    *colormap;
    // ----------------
    void lineDone();
    void decodeLine();
    void frameDone();
    void resampleFrame();
    void writeFrame(bool frameChanged);
    virtual void writeAVIHeader();
    virtual void writeAVIIndex();
   public:
    VideoCapture_YV12(void indexToRGBFunc(uint8_t color,
                                          float& r, float& g, float& b) =
                          (void (*)(uint8_t, float&, float&, float&)) 0,
                      int frameRate_ = 30);
    virtual ~VideoCapture_YV12();
    virtual void runOneCycle(const uint8_t *videoInput, uint32_t audioInput);
    virtual void setClockFrequency(size_t freq_);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_VIDEOREC_HPP

