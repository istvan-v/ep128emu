
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
#include "fileio.hpp"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <map>

#ifdef WIN32
#  undef WIN32
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  define WIN32 1
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#  include <direct.h>
#  include <windows.h>
#endif

static const unsigned char ep128EmuFile_Magic[16] = {
  0x5D, 0x12, 0xE4, 0xF4, 0xC9, 0xDA, 0xB6, 0x42,
  0x01, 0x33, 0xDE, 0x07, 0xD2, 0x34, 0xF2, 0x22
};

static std::map< int, Ep128::ChunkTypeHandler * >   chunkTypeDB;

static uint32_t hash_32(const unsigned char *buf, size_t nBytes)
{
  size_t        n = nBytes >> 2;
  unsigned int  h = 1U;

  for (size_t i = 0; i < n; i++) {
    h ^=  ((unsigned int) buf[0] & 0xFFU);
    h ^= (((unsigned int) buf[1] & 0xFFU) << 8);
    h ^= (((unsigned int) buf[2] & 0xFFU) << 16);
    h ^= (((unsigned int) buf[3] & 0xFFU) << 24);
    buf += 4;
    uint64_t  tmp = (uint32_t) h * (uint64_t) 0xC2B0C3CCU;
    h = ((unsigned int) tmp ^ (unsigned int) (tmp >> 32)) & 0xFFFFFFFFU;
  }
  switch (uint8_t(nBytes) & 3) {
  case 3:
    h ^= (((unsigned int) buf[2] & 0xFFU) << 16);
  case 2:
    h ^= (((unsigned int) buf[1] & 0xFFU) << 8);
  case 1:
    h ^=  ((unsigned int) buf[0] & 0xFFU);
    {
      uint64_t  tmp = (uint32_t) h * (uint64_t) 0xC2B0C3CCU;
      h = ((unsigned int) tmp ^ (unsigned int) (tmp >> 32)) & 0xFFFFFFFFU;
    }
    break;
  default:
    break;
  }
  return uint32_t(h);
}

static void getFullPathFileName(const char *fileName, std::string& fullName)
{
  std::string dirName;

  dirName = "";
#ifndef WIN32
  if (std::getenv("HOME") != (char*) 0)
    dirName = std::getenv("HOME");
  if ((int) dirName.size() == 0)
    dirName = ".";
  mkdir(dirName.c_str(), 0700);
  if (dirName[dirName.size() - 1] != '/')
    dirName += '/';
  dirName += ".ep128emu";
  mkdir(dirName.c_str(), 0700);
#else
  CsoundGUIMain::stripString(dirName, std::getenv("USERPROFILE"));
  if ((int) dirName.size() != 0) {
    struct _stat tmp;
    std::memset(&tmp, 0, sizeof(struct _stat));
    if (dirName[dirName.size() - 1] != '\\')
      dirName += '\\';
    dirName += "Application Data";
    if (_stat(dirName.c_str(), &tmp) != 0 ||
        !(tmp.st_mode & _S_IFDIR))
      dirName = "";
  }
  if ((int) dirName.size() == 0) {
    CsoundGUIMain::stripString(dirName, std::getenv("HOME"));
    if ((int) dirName.size() != 0) {
      struct _stat tmp;
      std::memset(&tmp, 0, sizeof(struct _stat));
      if (_stat(dirName.c_str(), &tmp) != 0 ||
          !(tmp.st_mode & _S_IFDIR))
        dirName = "";
    }
  }
  if ((int) dirName.size() == 0) {
    char  buf[512];
    int   len;
    len = (int) GetModuleFileName((HMODULE) 0, &(buf[0]), (DWORD) 512);
    if (len >= 512)
      len = 0;
    while (len > 0) {
      len--;
      if (buf[len] == '\\') {
        buf[len] = (char) 0;
        break;
      }
    }
    if (len > 0)
      dirName = &(buf[0]);
    else
      dirName = ".";
  }
  if (dirName[dirName.size() - 1] != '\\')
    dirName += '\\';
  dirName += ".ep128emu";
  _mkdir(dirName.c_str());
#endif
  fullName = dirName;
#ifndef WIN32
  fullName += '/';
#else
  fullName += '\\';
#endif
  fullName += fileName;
}

// ----------------------------------------------------------------------------

namespace Ep128 {

  File::Buffer::Buffer()
  {
    buf = (unsigned char *) 0;
    this->clear();
  }

  File::Buffer::Buffer(const unsigned char *buf_, size_t nBytes)
  {
    buf = (unsigned char *) 0;
    this->clear();
    writeData(buf_, nBytes);
  }

  File::Buffer::~Buffer()
  {
    this->clear();
  }

  unsigned char File::Buffer::readByte()
  {
    if (curPos >= dataSize)
      throw Exception("unexpected end of data chunk");
    unsigned char c = buf[curPos++] & 0xFF;
    return c;
  }

  bool File::Buffer::readBoolean()
  {
    unsigned char c = readByte();
    return (c == 0 ? false : true);
  }

  int32_t File::Buffer::readInt32()
  {
    uint32_t  n = readByte();
    n = (n << 8) | readByte();
    n = (n << 8) | readByte();
    n = (n << 8) | readByte();
    return int32_t(n);
  }

  uint32_t File::Buffer::readUInt32()
  {
    uint32_t  n = readByte();
    n = (n << 8) | readByte();
    n = (n << 8) | readByte();
    n = (n << 8) | readByte();
    return n;
  }

  double File::Buffer::readFloat()
  {
    int32_t   i = readInt32();
    uint32_t  f = readUInt32();
    return (double(i) + (double(f) * (1.0 / 4294967296.0)));
  }

  std::string File::Buffer::readString()
  {
    size_t  i, j;

    for (i = curPos; i < dataSize; i++) {
      if (buf[i] == 0)
        break;
    }
    if (i >= dataSize)
      throw Exception("unexpected end of data chunk while reading string");
    j = curPos;
    curPos = i + 1;
    return std::string(reinterpret_cast<char *>(&buf[j]));
  }

  void File::Buffer::writeByte(unsigned char n)
  {
    if (curPos >= allocSize) {
      size_t        newSize = ((allocSize + (allocSize >> 3)) | 255) + 1;
      unsigned char *newBuf = new unsigned char[newSize];
      if (buf) {
        for (size_t i = 0; i < dataSize; i++)
          newBuf[i] = buf[i];
        delete[] buf;
      }
      buf = newBuf;
      allocSize = newSize;
    }
    buf[curPos++] = n & 0xFF;
    if (curPos > dataSize)
      dataSize = curPos;
  }

  void File::Buffer::writeBoolean(bool n)
  {
    writeByte(n ? 1 : 0);
  }

  void File::Buffer::writeInt32(int32_t n)
  {
    writeByte(uint8_t(uint32_t(n) >> 24));
    writeByte(uint8_t(uint32_t(n) >> 16));
    writeByte(uint8_t(uint32_t(n) >> 8));
    writeByte(uint8_t(uint32_t(n)));
  }

  void File::Buffer::writeUInt32(uint32_t n)
  {
    writeByte(uint8_t(n >> 24));
    writeByte(uint8_t(n >> 16));
    writeByte(uint8_t(n >> 8));
    writeByte(uint8_t(n));
  }

  void File::Buffer::writeFloat(double n)
  {
    int32_t   i = 0;
    uint32_t  f = 0;
    if (n > -2147483648.0 && n < 2147483648.0) {
      double  tmp_i, tmp_f;
      tmp_f = std::modf(n, &tmp_i) * 4294967296.0;
      i = int32_t(tmp_i);
      if (n >= 0.0) {
        tmp_f += 0.5;
        if (tmp_f >= 4294967296.0) {
          f = 0;
          if (i < 2147483647)
            i++;
          else
            f--;
        }
        else
          f = uint32_t(tmp_f);
      }
      else {
        i--;
        tmp_f += 4294967296.5;
        if (tmp_f >= 4294967296.0) {
          f = 0;
          i++;
        }
        else
          f = uint32_t(tmp_f);
      }
    }
    else if (n <= -2147483648.0) {
      i = (-2147483647 - 1);
    }
    else if (n >= 2147483648.0) {
      i = 2147483647;
      f--;
    }
    writeInt32(i);
    writeUInt32(f);
  }

  void File::Buffer::writeString(const std::string& n)
  {
    writeData(reinterpret_cast<const unsigned char *>(n.c_str()),
              n.length() + 1);
  }

  void File::Buffer::writeData(const unsigned char *buf_, size_t nBytes)
  {
    if ((curPos + nBytes) > allocSize) {
      size_t  newSize = allocSize;
      do {
        newSize = ((newSize + (newSize >> 3)) | 255) + 1;
      } while (newSize < (curPos + nBytes));
      unsigned char *newBuf = new unsigned char[newSize];
      if (buf) {
        for (size_t i = 0; i < dataSize; i++)
          newBuf[i] = buf[i];
        delete[] buf;
      }
      buf = newBuf;
      allocSize = newSize;
    }
    for (size_t i = 0; i < nBytes; i++)
      buf[curPos++] = buf_[i] & 0xFF;
    if (curPos > dataSize)
      dataSize = curPos;
  }

  void File::Buffer::setPosition(size_t pos)
  {
    if (pos > dataSize) {
      if (pos > allocSize) {
        size_t  newSize = allocSize;
        do {
          newSize = ((newSize + (newSize >> 3)) | 255) + 1;
        } while (newSize < pos);
        unsigned char *newBuf = new unsigned char[newSize];
        if (buf) {
          for (size_t i = 0; i < dataSize; i++)
            newBuf[i] = buf[i];
          delete[] buf;
        }
        buf = newBuf;
        allocSize = newSize;
      }
      for (size_t i = dataSize; i < pos; i++)
        buf[i] = 0;
      dataSize = pos;
    }
    curPos = pos;
  }

  void File::Buffer::clear()
  {
    if (buf)
      delete[] buf;
    buf = (unsigned char *) 0;
    curPos = 0;
    dataSize = 0;
    allocSize = 0;
  }

  // --------------------------------------------------------------------------

  File::File()
  {
  }

  File::File(const char *fileName, bool useHomeDirectory)
  {
    bool    err = false;

    if (fileName != (char*) 0 && fileName[0] != '\0') {
      std::string fullName;
      if (useHomeDirectory)
        getFullPathFileName(fileName, fullName);
      else
        fullName = fileName;
      std::FILE *f = std::fopen(fullName.c_str(), "rb");
      if (f) {
        try {
          int     c;
          for (int i = 0; i < 16; i++) {
            c = std::fgetc(f);
            if (c == EOF || (unsigned char) (c & 0xFF) != ep128EmuFile_Magic[i])
              throw Exception("invalid file header");
          }
          while ((c = std::fgetc(f)) != EOF)
            buf.writeByte((unsigned char) (c & 0xFF));
        }
        catch (...) {
          buf.clear();
          std::fclose(f);
          throw;
        }
        buf.setPosition(0);
        if (std::ferror(f))
          err = true;
        if (std::fclose(f) != 0)
          err = true;
      }
      else
        err = true;
    }
    else
      err = true;
    if (err) {
      buf.clear();
      throw Exception("error opening or reading file");
    }
  }

  File::~File()
  {
  }

  void File::addChunk(ChunkType type, const Buffer& buf_)
  {
    if (type == EP128EMU_CHUNKTYPE_END_OF_FILE)
      throw Exception("internal error: invalid chunk type");
    size_t  startPos = buf.getPosition();
    buf.setPosition(startPos + buf_.getDataSize() + 12);
    buf.setPosition(startPos);
    buf.writeUInt32(uint32_t(type));
    buf.writeUInt32(uint32_t(buf_.getDataSize()));
    buf.writeData(buf_.getData(), buf_.getDataSize());
    buf.writeUInt32(hash_32(buf.getData() + startPos, buf_.getDataSize() + 8));
  }

  void File::processAllChunks()
  {
    if (buf.getDataSize() < 12)
      throw Exception("file is too short (no data)");
    buf.setPosition(0);
    while (buf.getPosition() < (buf.getDataSize() - 12)) {
      size_t  startPos = buf.getPosition();
      int     type = int(buf.readInt32());
      size_t  len = buf.readUInt32();
      if (len > (buf.getDataSize() - (startPos + 12)))
        throw Exception("unexpected end of file");
      buf.setPosition(startPos + len + 8);
      if (buf.readUInt32() != hash_32(buf.getData() + startPos, len + 8))
        throw Exception("CRC error in file data");
      if (ChunkType(type) == EP128EMU_CHUNKTYPE_END_OF_FILE)
        throw Exception("unexpected 'end of file' chunk");
      if (chunkTypeDB.find(type) != chunkTypeDB.end()) {
        Buffer  tmpBuf(buf.getData() + (startPos + 8), len);
        tmpBuf.setPosition(0);
        chunkTypeDB[type]->processChunk(tmpBuf);
      }
    }
    if (buf.getPosition() != (buf.getDataSize() - 12))
      throw Exception("file is truncated (missing 'end of file' chunk)");
    if (ChunkType(buf.readUInt32()) != EP128EMU_CHUNKTYPE_END_OF_FILE)
      throw Exception("file is truncated (missing 'end of file' chunk)");
    if (buf.readUInt32() != 0)
      throw Exception("invalid length for 'end of file' chunk (must be zero)");
    if (buf.readUInt32()
        != hash_32(buf.getData() + (buf.getDataSize() - 12), 8))
      throw Exception("CRC error in file data");
  }

  void File::writeFile(const char *fileName, bool useHomeDirectory)
  {
    size_t  startPos = buf.getPosition();
    bool    err = false;

    buf.setPosition(startPos + 12);
    buf.setPosition(startPos);
    buf.writeUInt32(uint32_t(EP128EMU_CHUNKTYPE_END_OF_FILE));
    buf.writeUInt32(0);
    buf.writeUInt32(hash_32(buf.getData() + startPos, 8));
    if (fileName != (char*) 0 && fileName[0] != '\0') {
      std::string fullName;
      if (useHomeDirectory)
        getFullPathFileName(fileName, fullName);
      else
        fullName = fileName;
      std::FILE *f = std::fopen(fullName.c_str(), "wb");
      if (f) {
        if (std::fwrite(&(ep128EmuFile_Magic[0]), 1, 16, f) != 16)
          err = true;
        if (std::fwrite(buf.getData(), 1, buf.getDataSize(), f)
            != buf.getDataSize())
          err = true;
        if (std::fclose(f) != 0)
          err = true;
        if (err)
          std::remove(fullName.c_str());
      }
      else
        err = true;
    }
    else
      err = true;
    buf.clear();
    if (err)
      throw Exception("error opening or writing file");
  }

  // --------------------------------------------------------------------------

  ChunkTypeHandler::~ChunkTypeHandler()
  {
  }

  void registerChunkType(ChunkTypeHandler *p)
  {
    if (!p)
      throw Exception("internal error: NULL chunk type handler");

    int     type = int(p->getChunkType());

    if (chunkTypeDB.find(type) != chunkTypeDB.end()) {
      delete chunkTypeDB[type];
      chunkTypeDB.erase(type);
    }
    chunkTypeDB[type] = p;
  }

  void unregisterAllChunkTypes()
  {
    std::map< int, ChunkTypeHandler * >::iterator   i;

    for (i = chunkTypeDB.begin(); i != chunkTypeDB.end(); i++)
      delete (*i).second;
    chunkTypeDB.clear();
  }

}       // namespace Ep128

