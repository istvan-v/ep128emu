
// iview2png: convert IVIEW and TVC KEP images to PNG format
// Copyright (C) 2017 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "compress.hpp"
#include "pngwrite.hpp"
#include "system.hpp"

#include <vector>

static void uncompressIViewImage(std::vector< unsigned char >& outBuf,
                                 const std::vector< unsigned char >& inBuf,
                                 int compressionType)
{
  outBuf.clear();
  std::vector< unsigned char >  tmpBuf1;
  std::vector< unsigned char >  tmpBuf2;
  const unsigned char *p = &(inBuf.front()) + 16;
  size_t  nBytes = inBuf.size() - 16;
  for (int i = 0; i < 2; i++) {
    tmpBuf1.clear();
    tmpBuf2.clear();
    if (nBytes < 4)
      throw Ep128Emu::Exception("unexpected end of IVIEW image file data");
    size_t  uncompressedSize = size_t(p[0]) | (size_t(p[1]) << 8);
    size_t  compressedSize = size_t(p[2]) | (size_t(p[3]) << 8);
    if (!uncompressedSize || !compressedSize)
      throw Ep128Emu::Exception("invalid IVIEW image compressed data size");
    if ((compressedSize + 4) > nBytes)
      throw Ep128Emu::Exception("unexpected end of IVIEW image file data");
    nBytes = nBytes - (compressedSize + 4);
    p = p + 4;
    do {
      tmpBuf1.push_back(*p);
      p++;
    } while (--compressedSize);
    Ep128Compress::decompressData(tmpBuf2, tmpBuf1, compressionType);
    if (tmpBuf2.size() != uncompressedSize)
      throw Ep128Emu::Exception("invalid IVIEW image compressed data header");
    outBuf.insert(outBuf.end(), tmpBuf2.begin(), tmpBuf2.end());
  }
}

static void loadIViewImage(std::vector< unsigned char >& outBuf,
                           int& w, int& h,
                           const std::vector< unsigned char >& inBuf)
{
  outBuf.clear();
  w = 0;
  h = 0;
  if (inBuf.size() < 17 || !(inBuf[0] == 0x00 && inBuf[1] == 0x49))
    throw Ep128Emu::Exception("invalid IVIEW image file header");
  if ((inBuf[2] | (inBuf[5] & 0x01)) != 0)
    throw Ep128Emu::Exception("unsupported IVIEW image video mode resolution");

  int     compressionType = inBuf[10];
  if (compressionType > 4)
    throw Ep128Emu::Exception("invalid IVIEW image compression type");
  compressionType = ((0x00064130 >> (compressionType << 2)) & 0x0F) - 1;
  std::vector< unsigned char >  tmpBuf;
  const unsigned char *p = &(inBuf.front()) + 16;
  size_t  nBytes = inBuf.size() - 16;
  if (compressionType >= 0) {
    // uncompress image data to a temporary buffer
    uncompressIViewImage(tmpBuf, inBuf, compressionType);
    p = &(tmpBuf.front());
    nBytes = tmpBuf.size();
  }

  unsigned char videoMode = *p & 0x6E;
  p++;
  nBytes--;
  int     paletteColors = 0;
  int     attrBytesPerChar = 0;
  int     pixelBytesPerChar = 1;
  switch (videoMode & 0x0E) {
  case 0x02:                            // PIXEL
    pixelBytesPerChar++;
  case 0x0E:                            // LPIXEL
    if ((videoMode & 0x60) != 0x60)
      paletteColors = 1 << (((videoMode & 0x60) >> 5) + 1);
    break;
  case 0x04:                            // ATTRIBUTE
    attrBytesPerChar++;
    if ((videoMode & 0x60) != 0x60)
      paletteColors = 8;
    break;
  default:
    throw Ep128Emu::Exception("invalid or unsupported IVIEW image video mode");
  }
  int     biasResolution = inBuf[3];
  int     paletteResolution = inBuf[4];
  unsigned char interlaceFlags = inBuf[5];
  if ((interlaceFlags & 0x60) != 0 ||
      bool(interlaceFlags & 0x80) != bool(interlaceFlags & 0x1F)) {
    throw Ep128Emu::Exception("invalid IVIEW image interlace flags");
  }
#if 0
  unsigned char borderColor = inBuf[9];
#endif
  int     nFields = inBuf[11];
  int     attributeResolution = inBuf[13];
  if (!nFields)
    nFields = 1;
  else if (nFields > 2)
    throw Ep128Emu::Exception("animated IVIEW images are not supported");
  if (nFields != (int(bool(interlaceFlags)) + 1)) {
    throw Ep128Emu::Exception("IVIEW image field count "
                              "does not match interlace flags");
  }
  w = inBuf[8];
  h = int(inBuf[6]) | (int(inBuf[7]) << 8);
  if ((w * h) == 0 || h > 16384) {
    w = 0;
    h = 0;
    throw Ep128Emu::Exception("invalid IVIEW image dimensions");
  }
  if (biasResolution < 1 || biasResolution > h)
    biasResolution = h;
  if (paletteResolution < 1 || paletteResolution > h)
    paletteResolution = h;
  if (attributeResolution < 1)
    attributeResolution = 1;
  else if (attributeResolution > h)
    attributeResolution = h;

  outBuf.resize(size_t(h * 2 * (w << 4) + (256 * 3)), 0x00);
  std::vector< unsigned char >  paletteData(size_t(h * 2 * 16));
  std::vector< unsigned char >  attributeData(size_t(h * 2 * w
                                                     * attrBytesPerChar));
  std::vector< unsigned char >  pixelData(size_t(h * 2 * w
                                                 * pixelBytesPerChar));

  // read FIXBIAS data
  if (paletteColors == 8) {
    int     biasFields = int(bool(interlaceFlags & 0x02)) + 1;
    size_t  biasDataSize =
        size_t(((h + biasResolution - 1) / biasResolution) * biasFields);
    if (nBytes < biasDataSize)
      throw Ep128Emu::Exception("unexpected end of IVIEW image file data");
    nBytes -= biasDataSize;
    for (int i = 0; i < biasFields; i++) {
      for (int y = 0; y < h; y++) {
        unsigned char c = (p[y / biasResolution] & 0x1F) << 3;
        for (int n = 8; n < 16; n++) {
          paletteData[((y * 2 + i) << 4) + n] = c;
          if (biasFields < 2)
            paletteData[((y * 2 + 1) << 4) + n] = c;
          c++;
        }
      }
      p = p + (biasDataSize / size_t(biasFields));
    }
  }

  // read palette data
  if (paletteColors > 0) {
    int     paletteFields = int(bool(interlaceFlags & 0x04)) + 1;
    size_t  paletteDataSize =
        size_t(((h + paletteResolution - 1) / paletteResolution)
               * paletteColors * paletteFields);
    if (nBytes < paletteDataSize)
      throw Ep128Emu::Exception("unexpected end of IVIEW image file data");
    nBytes -= paletteDataSize;
    for (int i = 0; i < paletteFields; i++) {
      for (int y = 0; y < h; y++) {
        int     offs = (y / paletteResolution) * paletteColors;
        for (int n = 0; n < paletteColors; n++) {
          unsigned char c = p[offs + n];
          paletteData[((y * 2 + i) << 4) + n] = c;
          if (paletteFields < 2)
            paletteData[((y * 2 + 1) << 4) + n] = c;
        }
      }
      p = p + (paletteDataSize / size_t(paletteFields));
    }
  }

  // read attribute data
  if (attrBytesPerChar > 0) {
    int     attrFields = int(bool(interlaceFlags & 0x08)) + 1;
    size_t  attrDataSize =
        size_t(((h + attributeResolution - 1) / attributeResolution)
               * w * attrFields);
    if (nBytes < attrDataSize)
      throw Ep128Emu::Exception("unexpected end of IVIEW image file data");
    nBytes -= attrDataSize;
    for (int i = 0; i < attrFields; i++) {
      for (int y = 0; y < h; y++) {
        int     offs = (y / attributeResolution) * w;
        for (int x = 0; x < w; x++) {
          unsigned char a = p[offs + x];
          attributeData[((y * 2 + i) * w) + x] = a;
          if (attrFields < 2)
            attributeData[((y * 2 + 1) * w) + x] = a;
        }
      }
      p = p + (attrDataSize / size_t(attrFields));
    }
  }

  // read pixel data
  {
    int     pixelFields = int(bool(interlaceFlags & 0x10)) + 1;
    int     bytesPerLine = w * pixelBytesPerChar;
    size_t  pixelDataSize = size_t(h * bytesPerLine * pixelFields);
    if (nBytes < pixelDataSize)
      throw Ep128Emu::Exception("unexpected end of IVIEW image file data");
    nBytes -= pixelDataSize;
    for (int i = 0; i < pixelFields; i++) {
      for (int y = 0; y < h; y++) {
        int     offs = y * bytesPerLine;
        for (int x = 0; x < bytesPerLine; x++) {
          unsigned char b = p[offs + x];
          pixelData[((y * 2 + i) * bytesPerLine) + x] = b;
          if (pixelFields < 2)
            pixelData[((y * 2 + 1) * bytesPerLine) + x] = b;
        }
      }
      p = p + (pixelDataSize / size_t(pixelFields));
    }
  }

  h = h * 2;

  // convert image
  for (int y = 0; y < h; y++) {
    p = &(pixelData.front()) + size_t(y * w * pixelBytesPerChar);
    const unsigned char *palette = &(paletteData.front()) + (y << 4);
    for (int x = 0; x < w; x++) {
      int     offs = ((y * w + x) << 4) + (256 * 3);
      for (int i = 0; i < 16; i++) {
        unsigned char c =
            p[(pixelBytesPerChar != 2 ? x : ((x << 1) | (i >> 3)))];
        if ((videoMode & 0x60) != 0x60) {
          // if not 256 color mode:
          switch (videoMode & 0x60) {
          case 0x00:                    // 2 colors
            c = c >> (~(i >> (2 - pixelBytesPerChar)) & 7);
            c = c & 0x01;
            break;
          case 0x20:                    // 4 colors
            c = c >> (~(i >> (3 - pixelBytesPerChar)) & 3);
            c = ((c & 0x10) >> 4) | ((c & 0x01) << 1);
            break;
          case 0x40:                    // 16 colors
            c = c >> (~(i >> (4 - pixelBytesPerChar)) & 1);
            c = ((c & 0x40) >> 6) | ((c & 0x04) >> 1)
                | ((c & 0x10) >> 2) | ((c & 0x01) << 3);
            break;
          }
          if (attrBytesPerChar > 0) {
            // ATTRIBUTE mode:
            unsigned char a = attributeData[y * w + x];
            c = (a >> ((~c & 1) << 2)) & 0x0F;
          }
          c = palette[c];
        }
        outBuf[offs + i] = c;
      }
    }
  }

  w = w * 16;

  // write Enterprise palette
  for (int i = 0; i < 256; i++) {
    int     r = ((i & 0x01) << 2) | ((i & 0x08) >> 2) | ((i & 0x40) >> 6);
    int     g = ((i & 0x02) << 1) | ((i & 0x10) >> 3) | ((i & 0x80) >> 7);
    int     b = ((i & 0x04) >> 1) | ((i & 0x20) >> 5);
    outBuf[i * 3] = (unsigned char) ((r * 510 + 7) / 14);
    outBuf[i * 3 + 1] = (unsigned char) ((g * 510 + 7) / 14);
    outBuf[i * 3 + 2] = (unsigned char) ((b * 510 + 3) / 6);
  }
}

// ----------------------------------------------------------------------------

static void uncompressTVCKEPImage(std::vector< unsigned char >& outBuf,
                                  const std::vector< unsigned char >& inBuf)
{
  outBuf.clear();
  std::vector< unsigned char >  tmpBuf1;
  std::vector< unsigned char >  tmpBuf2;
  const unsigned char *p = &(inBuf.front()) + 14;
  size_t  nBytes = inBuf.size() - 14;
  for (int i = 0; i < 2; i++) {
    if (i == 0 && (inBuf[3] & 0x22) != 0x20) {
      // no palette data block
      p = p + 2;
      nBytes = nBytes - 2;
      continue;
    }
    if (i == 0 && (inBuf[3] & 0x14) == 0x04) {
      // palette data is not compressed in RLE format
      p = p + 2;
      nBytes = nBytes - 2;
      size_t  paletteDataSize = size_t(inBuf[10]) | (size_t(inBuf[11]) << 8);
      paletteDataSize = paletteDataSize << ((inBuf[3] & 1) + 1);
      if (nBytes < paletteDataSize)
        throw Ep128Emu::Exception("unexpected end of TVC KEP image file data");
      nBytes = nBytes - paletteDataSize;
      do {
        outBuf.push_back(*p);
        p++;
      } while (--paletteDataSize);
      continue;
    }
    tmpBuf1.clear();
    tmpBuf2.clear();
    size_t  compressedSize = nBytes;
    if (i == 0) {
      compressedSize = size_t(p[0]) | (size_t(p[1]) << 8);
      p = p + 2;
      nBytes = nBytes - 2;
    }
    if (!compressedSize)
      throw Ep128Emu::Exception("invalid TVC KEP image compressed data size");
    if (compressedSize > nBytes)
      throw Ep128Emu::Exception("unexpected end of TVC KEP image file data");
    nBytes = nBytes - compressedSize;
    if ((inBuf[3] & 0x14) == 0x04) {
      // uncompress RLE format
      unsigned char flagBits = 0x00;
      do {
        unsigned char c = *(p++);
        if (!((c ^ flagBits) & 0xC0)) {
          // RLE sequence
          unsigned char len = c & 0x3F;
          if (!len)
            throw Ep128Emu::Exception("error in TVC KEP RLE compressed data");
          if (len == 1)
            flagBits = (flagBits + 0x40) & 0xC0;
          if (!(--compressedSize)) {
            throw Ep128Emu::Exception("unexpected end of "
                                      "TVC KEP image file data");
          }
          c = *(p++);
          do {
            outBuf.push_back(c);
          } while (--len);
        }
        else {
          // literal byte
          outBuf.push_back(c);
        }
        if (outBuf.size() > 0x02000000)
          throw Ep128Emu::Exception("error in TVC KEP RLE compressed data");
      } while (--compressedSize);
    }
    else {
      // uncompress epcompress or zlib format
      do {
        tmpBuf1.push_back(*p);
        p++;
      } while (--compressedSize);
      int     compressionType = (0x5302 >> (inBuf[3] & 0x0C)) & 0x0F;
      Ep128Compress::decompressData(tmpBuf2, tmpBuf1, compressionType);
      outBuf.insert(outBuf.end(), tmpBuf2.begin(), tmpBuf2.end());
    }
  }
}

static void loadTVCKEPImage(std::vector< unsigned char >& outBuf,
                            int& w, int& h,
                            const std::vector< unsigned char >& inBuf)
{
  outBuf.clear();
  w = 0;
  h = 0;
  if (inBuf.size() < 8 ||
      !(inBuf[0] == 0x4B && inBuf[1] == 0x45 && inBuf[2] == 0x50) ||
      ((inBuf[3] & 0x1C) != 0 && inBuf.size() < 16)) {
    throw Ep128Emu::Exception("invalid TVC KEP image file header");
  }
  if ((inBuf[3] & 0x03) == 3)
    throw Ep128Emu::Exception("invalid TVC KEP image video mode");
  std::vector< unsigned char >  tmpBuf;
  const unsigned char *p = &(inBuf.front()) + 8;
  size_t  nBytes = inBuf.size() - 8;
  if ((inBuf[3] & 0x1C) != 0) {
    p = p + 8;
    nBytes = nBytes - 8;
    if ((inBuf[3] & 0x14) != 0) {
      // uncompress image data to a temporary buffer
      uncompressTVCKEPImage(tmpBuf, inBuf);
      p = &(tmpBuf.front());
      nBytes = tmpBuf.size();
    }
    w = int(inBuf[8]) | (int(inBuf[9]) << 8);
    h = int(inBuf[10]) | (int(inBuf[11]) << 8);
    if (w < 1 || w > 4096 || h < 1 || h > 4096 ||
        ((inBuf[3] & 0x40) | (h & 1)) == 0x41) {
      w = 0;
      h = 0;
      throw Ep128Emu::Exception("invalid TVC KEP image dimensions");
    }
    if (inBuf[3] & 0x40)
      h = h >> 1;                       // field height in interlace mode
  }
  else {
    w = 512 >> (inBuf[3] & 0x03);
    h = 240;
  }

  w = w >> (~(inBuf[3]) & 3);           // width in bytes
  int     paletteColors = 1 << ((inBuf[3] & 0x03) + 1);
  if (paletteColors == 8)
    paletteColors = 0;
#if 0
  unsigned char borderColor = 0x00;
  if ((inBuf[3] & 0x1C) != 0)
    borderColor = inBuf[12];
#endif

  outBuf.resize(size_t(h * 2 * (w << 3) + (256 * 3)), 0x00);
  std::vector< unsigned char >  paletteData(size_t(h * 2 * 4));
  std::vector< unsigned char >  pixelData(size_t(h * 2 * w * 2));

  // read palette data
  if (paletteColors > 0 && (inBuf[3] & 0x22) != 0x20) {
    // fixed palette
    for (size_t i = 0; i < paletteData.size(); i++) {
      unsigned char c = inBuf[(i & 3) | 4];
      c = (c | (c >> 1)) & 0x55;
      paletteData[i] = c;
    }
  }
  else if (paletteColors > 0) {
    int     paletteFields = int(bool(inBuf[3] & 0x40)) + 1;
    size_t  paletteDataSize = size_t(h * paletteColors * paletteFields);
    if (nBytes < paletteDataSize)
      throw Ep128Emu::Exception("unexpected end of TVC KEP image file data");
    nBytes -= paletteDataSize;
    for (int i = 0; i < paletteFields; i++) {
      for (int y = 0; y < h; y++) {
        int     offs = y * paletteColors;
        for (int n = 0; n < paletteColors; n++) {
          unsigned char c = p[offs + n];
          c = (c | (c >> 1)) & 0x55;
          paletteData[((y * 2 + i) << 2) + n] = c;
          if (paletteFields < 2)
            paletteData[((y * 2 + 1) << 2) + n] = c;
        }
      }
      p = p + (paletteDataSize / size_t(paletteFields));
    }
  }

  // read pixel data
  {
    int     pixelFields = int(bool(inBuf[3] & 0x40)) + 1;
    int     bytesPerLine = w;
    size_t  pixelDataSize = size_t(h * bytesPerLine * pixelFields);
    if (nBytes < pixelDataSize)
      throw Ep128Emu::Exception("unexpected end of TVC KEP image file data");
    nBytes -= pixelDataSize;
    for (int i = 0; i < pixelFields; i++) {
      for (int y = 0; y < h; y++) {
        int     offs = y * bytesPerLine;
        for (int x = 0; x < bytesPerLine; x++) {
          unsigned char b = p[offs + x];
          pixelData[((y * 2 + i) * bytesPerLine) + x] = b;
          if (pixelFields < 2)
            pixelData[((y * 2 + 1) * bytesPerLine) + x] = b;
        }
      }
      p = p + (pixelDataSize / size_t(pixelFields));
    }
  }

  h = h * 2;

  // convert image
  for (int y = 0; y < h; y++) {
    p = &(pixelData.front()) + size_t(y * w);
    const unsigned char *palette = &(paletteData.front()) + (y << 2);
    for (int x = 0; x < w; x++) {
      int     offs = ((y * w + x) << 3) + (256 * 3);
      for (int i = 0; i < 8; i++) {
        unsigned char c = p[x];
        switch (inBuf[3] & 0x03) {
        case 0x00:                      // 2 colors
          c = c >> (~i & 7);
          c = palette[c & 0x01];
          break;
        case 0x01:                      // 4 colors
          c = c >> (~(i >> 1) & 3);
          c = palette[((c & 0x10) >> 4) | ((c & 0x01) << 1)];
          break;
        case 0x02:                      // 16 colors
          c = c >> (~(i >> 2) & 1);
          c = c & 0x55;
          break;
        }
        outBuf[offs + i] = c;
      }
    }
  }

  w = w * 8;

  // write TVC palette
  for (int c = 0; c < 256; c++) {
    int     i = (!(c & 0xC0) ? 4 : 7);
    int     r = int(bool(c & 0x0C)) * i;
    int     g = int(bool(c & 0x30)) * i;
    int     b = int(bool(c & 0x03)) * i;
    outBuf[c * 3] = (unsigned char) ((r * 510 + 7) / 14);
    outBuf[c * 3 + 1] = (unsigned char) ((g * 510 + 7) / 14);
    outBuf[c * 3 + 2] = (unsigned char) ((b * 510 + 7) / 14);
  }
}

// ----------------------------------------------------------------------------

int main(int argc, char **argv)
{
  if (argc != 3 && argc != 4) {
    std::printf("Usage: %s [-n] INFILE.PIC OUTFILE.PNG\n", argv[0]);
    return -1;
  }
  std::FILE *f = (std::FILE *) 0;
  try {
    bool    optimizePalette = true;
    if (argc == 4) {
      if (std::strcmp(argv[1], "-n") != 0)
        throw Ep128Emu::Exception("invalid command line option");
      optimizePalette = false;
    }
    f = Ep128Emu::fileOpen(argv[argc - 2], "rb");
    if (!f)
      throw Ep128Emu::Exception("error opening input file");
    std::vector< unsigned char >  inBuf;
    {
      int     c;
      while ((c = std::fgetc(f)) != EOF)
        inBuf.push_back((unsigned char) (c & 0xFF));
    }
    std::fclose(f);
    f = (std::FILE *) 0;
    if (inBuf.size() < 8)
      throw Ep128Emu::Exception("invalid input file header");
    std::vector< unsigned char >  outBuf;
    int     w = 0;
    int     h = 0;
    if (inBuf[0] == 0x00 && inBuf[1] == 0x49) {
      std::printf("Converting IVIEW image...\n");
      loadIViewImage(outBuf, w, h, inBuf);
    }
    else if (inBuf[0] == 0x4B && inBuf[1] == 0x45 && inBuf[2] == 0x50) {
      std::printf("Converting TVC KEP image...\n");
      loadTVCKEPImage(outBuf, w, h, inBuf);
    }
    else {
      throw Ep128Emu::Exception("invalid input file header");
    }
    std::printf("Compressing PNG file...\n");
    Ep128Emu::writePNGImage(argv[argc - 1], &(outBuf.front()), w, h, 256,
                            optimizePalette, 32768);
    std::printf("Done.\n");
  }
  catch (std::exception& e) {
    if (f)
      std::fclose(f);
    std::fprintf(stderr, " *** %s: %s\n", argv[0], e.what());
    return -1;
  }
  return 0;
}

