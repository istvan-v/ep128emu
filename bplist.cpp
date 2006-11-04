
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
#include "bplist.hpp"
#include "ioports.hpp"
#include "memory.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Ep128 {

  BreakPointList::BreakPointList(const std::string& lst)
  {
    std::vector<std::string>  tokens;
    std::string curToken = "";
    char        ch = '\0';
    bool        wasSpace = true;

    for (size_t i = 0; i < lst.length(); i++) {
      ch = lst[i];
      bool  isSpace = (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
      if (isSpace && wasSpace)
        continue;
      if (isSpace) {
        tokens.push_back(curToken);
        curToken = "";
      }
      else
        curToken += ch;
      wasSpace = isSpace;
    }
    if (!wasSpace) {
      curToken += ch;
      tokens.push_back(curToken);
    }

    IOPorts p;
    Memory  m;

    for (size_t i = 0; i < tokens.size(); i++) {
      size_t    j;
      uint16_t  addr = 0, lastAddr;
      uint8_t   segment = 0;
      bool      isIO = false, haveSegment = false;
      bool      isRead = false, isWrite = false;
      int       priority = 2;
      uint32_t  n;

      curToken = tokens[i];
      n = 0;
      for (j = 0; j < curToken.length(); j++) {
        char    c = curToken[j];
        if (c >= '0' && c <= '9')
          n = (n << 4) + uint32_t(c - '0');
        else if (c >= 'A' && c <= 'F')
          n = (n << 4) + uint32_t(c - 'A') + 10;
        else if (c >= 'a' && c <= 'f')
          n = (n << 4) + uint32_t(c - 'a') + 10;
        else
          break;
      }
      if (j != 2 && j != 4)
        throw Exception("syntax error in breakpoint list");
      if (j == 2 && j < curToken.length() && curToken[j] == ':') {
        j++;
        haveSegment = true;
        segment = uint8_t(n);
        n = 0;
        for ( ; j < curToken.length(); j++) {
          char    c = curToken[j];
          if (c >= '0' && c <= '9')
            n = (n << 4) + uint32_t(c - '0');
          else if (c >= 'A' && c <= 'F')
            n = (n << 4) + uint32_t(c - 'A') + 10;
          else if (c >= 'a' && c <= 'f')
            n = (n << 4) + uint32_t(c - 'a') + 10;
          else
            break;
        }
        if (j != 7 || n >= 0x4000)
          throw Exception("syntax error in breakpoint list");
      }
      else if (j == 2)
        isIO = true;
      addr = uint16_t(n);
      lastAddr = addr;
      if (j < curToken.length() && curToken[j] == '-') {
        size_t  len = 0;
        n = 0;
        j++;
        for ( ; j < curToken.length(); j++, len++) {
          char    c = curToken[j];
          if (c >= '0' && c <= '9')
            n = (n << 4) + uint32_t(c - '0');
          else if (c >= 'A' && c <= 'F')
            n = (n << 4) + uint32_t(c - 'A') + 10;
          else if (c >= 'a' && c <= 'f')
            n = (n << 4) + uint32_t(c - 'a') + 10;
          else
            break;
        }
        if (isIO) {
          if (len != 2)
            throw Exception("syntax error in breakpoint list");
        }
        else if (len != 4 || (haveSegment && n >= 0x4000))
          throw Exception("syntax error in breakpoint list");
        if (n < addr)
          throw Exception("syntax error in breakpoint list");
        lastAddr = n;
      }
      for ( ; j < curToken.length(); j++) {
        switch (curToken[j]) {
        case 'r':
          isRead = true;
          break;
        case 'w':
          isWrite = true;
          break;
        case 'p':
          if (++j >= curToken.length())
            throw Exception("syntax error in breakpoint list");
          if (curToken[j] < '0' || curToken[j] > '3')
            throw Exception("syntax error in breakpoint list");
          priority = int(curToken[j] - '0');
          break;
        default:
          throw Exception("syntax error in breakpoint list");
        }
      }
      if (!isRead && !isWrite) {
        isRead = true;
        isWrite = true;
      }
      while (true) {
        if (isIO)
          p.setBreakPoint(addr, priority, isRead, isWrite);
        else if (haveSegment)
          m.setBreakPoint(segment, addr, priority, isRead, isWrite);
        else
          m.setBreakPoint(addr, priority, isRead, isWrite);
        if (addr == lastAddr)
          break;
        addr++;
      }
    }

    lst_ = p.getBreakPointList().lst_;
    BreakPointList  tmp = m.getBreakPointList();
    lst_.insert(lst_.end(), tmp.lst_.begin(), tmp.lst_.end());
  }

  void BreakPointList::addMemoryBreakPoint(uint8_t segment, uint16_t addr,
                                           bool r, bool w, int priority)
  {
    lst_.push_back(BreakPoint(false, true, r, w, segment, addr, priority));
  }

  void BreakPointList::addMemoryBreakPoint(uint16_t addr,
                                           bool r, bool w, int priority)
  {
    lst_.push_back(BreakPoint(false, false, r, w, 0, addr, priority));
  }

  void BreakPointList::addIOBreakPoint(uint16_t addr,
                                       bool r, bool w, int priority)
  {
    lst_.push_back(BreakPoint(true, false, r, w, 0, addr, priority));
  }

  std::string BreakPointList::getBreakPointList()
  {
    std::ostringstream  lst;
    BreakPoint          prv_bp(false, false, false, false, 0, 0, 0);
    size_t              i;
    uint16_t            firstAddr = 0;

    lst << std::hex << std::uppercase << std::right << std::setfill('0');
    std::stable_sort(lst_.begin(), lst_.end());
    for (i = 0; i < lst_.size(); i++) {
      BreakPoint  bp(lst_[i]);
      if (!i) {
        prv_bp = bp;
        firstAddr = bp.addr();
        continue;
      }
      if ((bp.addr() == (prv_bp.addr() + 1) || bp.addr() == prv_bp.addr()) &&
          bp.isIO() == prv_bp.isIO() &&
          bp.haveSegment() == prv_bp.haveSegment() &&
          bp.isRead() == prv_bp.isRead() &&
          bp.isWrite() == prv_bp.isWrite() &&
          bp.priority() == prv_bp.priority() &&
          bp.segment() == prv_bp.segment()) {
        prv_bp = bp;
        continue;
      }
      uint16_t  lastAddr = lst_[i - 1].addr();
      if (prv_bp.haveSegment())
        lst << std::setw(2) << uint32_t(prv_bp.segment()) << ":";
      if (prv_bp.isIO()) {
        lst << std::setw(2) << (firstAddr & 0xFF);
        if (lastAddr != firstAddr)
          lst << "-" << std::setw(2) << (lastAddr & 0xFF);
      }
      else {
        lst << std::setw(4) << firstAddr;
        if (lastAddr != firstAddr)
          lst << "-" << std::setw(4) << lastAddr;
      }
      if (prv_bp.isRead() != prv_bp.isWrite())
        lst << (prv_bp.isRead() ? "r" : "w");
      if (prv_bp.priority() != 2)
        lst << "p" << std::setw(1) << prv_bp.priority();
      lst << "\n";
      prv_bp = bp;
      firstAddr = bp.addr();
    }
    if (i) {
      uint16_t  lastAddr = lst_[i - 1].addr();
      if (prv_bp.haveSegment())
        lst << std::setw(2) << uint32_t(prv_bp.segment()) << ":";
      if (prv_bp.isIO()) {
        lst << std::setw(2) << (firstAddr & 0xFF);
        if (lastAddr != firstAddr)
          lst << "-" << std::setw(2) << (lastAddr & 0xFF);
      }
      else {
        lst << std::setw(4) << firstAddr;
        if (lastAddr != firstAddr)
          lst << "-" << std::setw(4) << lastAddr;
      }
      if (prv_bp.isRead() != prv_bp.isWrite())
        lst << (prv_bp.isRead() ? "r" : "w");
      if (prv_bp.priority() != 2)
        lst << "p" << std::setw(1) << prv_bp.priority();
      lst << "\n";
    }
    lst << "\n";
    return lst.str();
  }

}

