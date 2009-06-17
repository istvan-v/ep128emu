
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2009 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "ep128vm.hpp"

#include <vector>

namespace Ep128 {

  void Ep128VM::resetMemoryConfiguration(size_t memSize)
  {
    stopDemo();
    // calculate new number of RAM segments
    size_t  nSegments = (memSize + 15) >> 4;
    nSegments = (nSegments > 4 ? (nSegments < 232 ? nSegments : 232) : 4);
    // delete all ROM segments
    for (int i = 0; i < 256; i++) {
      if (memory.isSegmentROM(uint8_t(i)))
        memory.deleteSegment(uint8_t(i));
    }
    // resize RAM
    for (int i = 0; i <= (0xFF - int(nSegments)); i++) {
      if (memory.isSegmentRAM(uint8_t(i)))
        memory.deleteSegment(uint8_t(i));
    }
    for (int i = 0xFF; i > (0xFF - int(nSegments)); i--) {
      if (!memory.isSegmentRAM(uint8_t(i)))
        memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
    }
    // cold reset
    this->reset(true);
  }

  void Ep128VM::loadROMSegment(uint8_t n, const char *fileName, size_t offs)
  {
    stopDemo();
    if (fileName == (char *) 0 || fileName[0] == '\0') {
      // empty file name: delete segment
      if (memory.isSegmentROM(n))
        memory.deleteSegment(n);
      else if (memory.isSegmentRAM(n)) {
        memory.deleteSegment(n);
        // if there was RAM at the specified segment, relocate it
        for (int i = 0xFF; i >= 0x08; i--) {
          if (!(memory.isSegmentROM(uint8_t(i)) ||
                memory.isSegmentRAM(uint8_t(i)))) {
            memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
            break;
          }
        }
      }
      return;
    }
    // load file into memory
    std::vector<uint8_t>  buf;
    buf.resize(0x4000);
    std::FILE   *f = std::fopen(fileName, "rb");
    if (!f)
      throw Ep128Emu::Exception("cannot open ROM file");
    std::fseek(f, 0L, SEEK_END);
    if (ftell(f) < long(offs + 0x4000)) {
      std::fclose(f);
      throw Ep128Emu::Exception("ROM file is shorter than expected");
    }
    std::fseek(f, long(offs), SEEK_SET);
    std::fread(&(buf.front()), 1, 0x4000, f);
    std::fclose(f);
    if (memory.isSegmentRAM(n)) {
      memory.loadSegment(n, true, &(buf.front()), 0x4000);
      // if there was RAM at the specified segment, relocate it
      for (int i = 0xFF; i >= 0x08; i--) {
        if (!(memory.isSegmentROM(uint8_t(i)) ||
              memory.isSegmentRAM(uint8_t(i)))) {
          memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
          break;
        }
      }
    }
    else {
      // otherwise just load new segment, or replace existing ROM
      memory.loadSegment(n, true, &(buf.front()), 0x4000);
    }
  }

  // --------------------------------------------------------------------------

  void Ep128VM::loadMemoryConfiguration(const std::string& fileName_)
  {
    resetMemoryConfiguration(64);
    std::FILE *f = (std::FILE *) 0;
    std::FILE *romFile = (std::FILE *) 0;
    try {
      if (fileName_.length() < 1)
        throw Ep128Emu::Exception("invalid memory configuration file name");
      f = std::fopen(fileName_.c_str(), "rb");
      if (!f)
        throw Ep128Emu::Exception("error opening memory configuration file");
      std::vector< std::string >  args;
      bool    isEOF = false;
      do {
        args.resize(0);
        bool    readingArg = false;
        bool    readingStr = false;
        bool    backSlashFlag = false;
        std::string s;
        while (true) {
          int     n = std::fgetc(f);
          if (n == EOF) {                       // end of file
            isEOF = true;
            break;
          }
          char    c = char(n & 0xFF);
          if (backSlashFlag) {                  // escaped character
            backSlashFlag = false;              // in quoted string:
            if (c != '\r' && c != '\n') {
              if (c == 't')
                c = '\t';
              else if (c == 'n')
                c = '\n';
              else if (c == 'r')
                c = '\r';
              s += c;
              continue;
            }
          }
          if (c == '\\') {                      // backslash:
            if (readingStr) {                   // if quoted string, then
              backSlashFlag = true;             // next character is escaped
              continue;
            }
          }
          if (c == '\r' || c == '\n') {         // end of line
            break;
          }
          else if (c == ';' || c == '#') {      // comment:
            if (readingStr) {                   // if quoted string,
              s += c;                           // just copy the character
            }
            else {                              // skip the rest of the line
              do {
                n = std::fgetc(f);
                if (n == EOF) {
                  isEOF = true;
                  break;
                }
                c = char(n & 0xFF);
              } while (!(c == '\r' || c == '\n'));
              break;
            }
          }
          else if (c == ' ' || c == '\t') {     // whitespace
            if (readingStr) {                   // if quoted string,
              s += c;                           // just copy the character
            }
            else if (readingArg) {              // terminate current token
              readingArg = false;
              args.push_back(s);
              s.clear();
            }
          }
          else if (c == '"') {                  // quote character:
            if (readingStr) {                   // terminate quoted string
              readingArg = false;
              readingStr = false;
              args.push_back(s);
              s.clear();
            }
            else {
              if (readingArg) {                 // terminate any previous token
                args.push_back(s);
                s.clear();
              }
              readingArg = true;                // and begin quoted string
              readingStr = true;
              s.clear();
            }
          }
          else {                                // collect any other character
            if (!(readingArg || readingStr)) {  // begin new token
              readingArg = true;
              s.clear();
            }
            s += c;
          }
        }
        if (readingArg || readingStr) {         // terminate token
          readingArg = false;
          readingStr = false;
          args.push_back(s);
          s.clear();
        }
        if (args.size() < 1)
          continue;
        if (args.size() < 2) {
          throw Ep128Emu::Exception("syntax error in memory configuration "
                                    "file: too few arguments");
        }
        char    *endPtr = (char *) 0;
        long    n = std::strtol(args[0].c_str(), &endPtr, 0);
        if (args[0].length() < 1 ||
            endPtr == (char *) 0 || endPtr[0] != '\0') {
          throw Ep128Emu::Exception("syntax error in memory configuration "
                                    "file: error parsing segment number");
        }
        if (n < 0L || n > 0xFFL) {
          throw Ep128Emu::Exception("segment number is out of range "
                                    "in memory configuration file");
        }
        uint8_t firstSegment = uint8_t(n);
        if (args[1].length() != 3) {
          throw Ep128Emu::Exception("syntax error in memory configuration "
                                    "file: segment type must be 'ROM' or "
                                    "'RAM'");
        }
        if ((args[1][0] != 'R' && args[1][0] != 'r') ||
            (args[1][1] != 'O' && args[1][1] != 'o' &&
             args[1][1] != 'A' && args[1][1] != 'a') ||
            (args[1][2] != 'M' && args[1][2] != 'm')) {
          throw Ep128Emu::Exception("syntax error in memory configuration "
                                    "file: segment type must be 'ROM' or "
                                    "'RAM'");
        }
        bool    romSegment = (args[1][1] == 'O' || args[1][1] == 'o');
        std::string fileName("");
        if (args.size() > 2)
          fileName = args[2];
        size_t  nSegments = size_t(fileName.length() < 1);
        if (args.size() > 3) {
          n = std::strtol(args[3].c_str(), &endPtr, 0);
          if (args[3].length() < 1 ||
              endPtr == (char *) 0 || endPtr[0] != '\0') {
            throw Ep128Emu::Exception("syntax error in memory configuration "
                                      "file: error parsing the number of "
                                      "segments");
          }
          if (n < 1L ||
              (long(firstSegment) + n) > (romSegment ? 0x00FCL : 0x0100L)) {
            throw Ep128Emu::Exception("invalid segment range "
                                      "in memory configuration file");
          }
          nSegments = size_t(n);
        }
        if (args.size() > 4) {
          throw Ep128Emu::Exception("syntax error in memory configuration "
                                    "file: too many arguments");
        }
        romFile = (std::FILE *) 0;
        if (fileName.length() > 0) {
          romFile = std::fopen(fileName.c_str(), "rb");
          if (!romFile)
            throw Ep128Emu::Exception("error opening ROM file");
        }
        std::vector< uint8_t >  buf(16384);
        uint8_t curSegment = firstSegment;
        while (true) {
          size_t  nBytesRead = 0;
          if (romFile) {
            nBytesRead =
                std::fread(&(buf.front()), sizeof(uint8_t), 16384, romFile);
            if (!(nBytesRead >= 1 && nBytesRead <= 16384)) {
              if (nSegments == 0)
                break;
              if (curSegment == firstSegment) {
                // empty file
                std::fclose(romFile);
                romFile = (std::FILE *) 0;
              }
              else if (std::fseek(romFile, 0L, SEEK_SET) < 0) {
                throw Ep128Emu::Exception("error seeking ROM file");
              }
              firstSegment = curSegment;
              continue;
            }
            if (nBytesRead < 16384) {
              if (nSegments == 0)
                nSegments = 1;
              else if (std::fseek(romFile, 0L, SEEK_SET) < 0)
                throw Ep128Emu::Exception("error seeking ROM file");
            }
          }
          if (curSegment >= 0xFC && romSegment)
            throw Ep128Emu::Exception("segment number is out of range");
          memory.loadSegment(curSegment, romSegment, &(buf.front()),
                             nBytesRead);
          if (nSegments > 0) {
            if (--nSegments < 1)
              break;
          }
          if (curSegment >= 0xFF)
            throw Ep128Emu::Exception("segment number is out of range");
          curSegment = curSegment + 1;
        }
        if (romFile) {
          std::fclose(romFile);
          romFile = (std::FILE *) 0;
        }
      } while (!isEOF);
      std::fclose(f);
      f = (std::FILE *) 0;
    }
    catch (...) {
      if (romFile)
        std::fclose(romFile);
      if (f)
        std::fclose(f);
      throw;
    }
  }

}       // namespace Ep128

