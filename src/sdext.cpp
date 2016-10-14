
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/ep128emu/
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

// Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
// Copyright (C)2015 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
// http://xep128.lgb.hu/
//
// http://elm-chan.org/docs/mmc/mmc_e.html
// http://www.mikroe.com/downloads/get/1624/microsd_card_spec.pdf
// http://users.ece.utexas.edu/~valvano/EE345M/SD_Physical_Layer_Spec.pdf

#include "ep128emu.hpp"

#include <cstring>
#include <limits.h>
#include <unistd.h>
#include <cerrno>

#include "sdext.hpp"
#include "ide.hpp"

namespace Ep128 {

  /* ID files:
   * C0 71 00 00 │ 00 5D 01 32 │ 13 59 80 E3 │ 76 D9 CF FF │ 16 40 00 4F
   * 01 50 41 53 │ 30 31 36 42 │ 41 35 E4 39 │ 06 00 35 03 │ 80 FF 80 00
   * 00 00 00 00 │ 00 00 00 00
   * 4 bytes: size in sectors:   C0 71 00 00
   * CSD register  00 5D 01 32 │ 13 59 80 E3 │ 76 D9 CF FF │ 16 40 00 4F
   * CID register  01 50 41 53 | 30 31 36 42 │ 41 35 E4 39 │ 06 00 35 03
   * OCR register  80 FF 80 00
   */

  static const uint8_t _stop_transmission_answer[] = {
    0, 0, 0, 0, // "stuff byte" and some of it is the R1 answer anyway
    0xFF        // SD card is ready again
  };
  static const uint8_t _read_csd_answer[] = {
    0xFF,       // waiting a bit
    0xFE,       // data token
    // the CSD itself
    0x00, 0x5D, 0x01, 0x32, 0x13, 0x59, 0x80, 0xE3,
    0x76, 0xD9, 0xCF, 0xFF, 0x16, 0x40, 0x00, 0x4F,
    0, 0        // CRC bytes
  };
  static const uint8_t _read_cid_answer[] = {
    0xFF,       // waiting a bit
    0xFE,       // data token
    // the CID itself
    0x01, 0x50, 0x41, 0x53, 0x30, 0x31, 0x36, 0x42,
    0x41, 0x35, 0xE4, 0x39, 0x06, 0x00, 0x35, 0x03,
    0, 0        // CRC bytes
  };
  // no data token, nor CRC!
  // (technically this is the R3 answer minus the R1 part at the beginning...)
  static const uint8_t _read_ocr_answer[] = {
    // the OCR itself
    0x80, 0xFF, 0x80, 0x00
  };

#define ADD_ANS(ans) { ans_p = (ans); ans_index = 0; ans_size = sizeof(ans); }

  SDExt::SDExt()
    : sdext_enabled(false),
      status(0x00),
      _spi_last_w(0xFF),
      is_hs_read(false),
      rom_page_ofs(0x0000),
      cs0(false),
      cs1(false),
      sdextSegment(0xFFFFFFFFU),
      sdextAddress(0xFFFFFFFFU),
      cmd_index(0),
      _read_b(0x00),
      _write_b(0xFF),
      _write_specified(0x00),
      ans_p((uint8_t *) 0),
      ans_index(0),
      ans_size(0),
      writePos(0),
      writeState(0x00),
      ans_callback(false),
      delayCnt(0),
      writeProtectFlag(true),
      sdf((std::FILE *) 0),
      sdfno(-1),
      sd_card_size(0U),
      sd_card_pos(0U)
  {
    sd_ram_ext.resize(0x1C00, 0xFF);
    _buffer.resize(1024, 0x00);
    this->reset(false);
  }

  SDExt::~SDExt()
  {
    openImage((char *) 0);
  }

  void SDExt::reset(bool clear_ram)
  {
    if (clear_ram)
      std::memset(&(sd_ram_ext.front()), 0xFF, sd_ram_ext.size());
    rom_page_ofs = 0x0000;
    is_hs_read = false;
    cmd_index = 0;
    ans_size = 0;
    ans_index = 0;
    writePos = 0;
    writeState = 0x00;
    ans_callback = false;
    delayCnt = 0;
    status = 0;
    _read_b = 0;
    _write_b = 0xFF;
    _spi_last_w = 0xFF;
    sd_card_pos = 0U;
  }

  /* SDEXT emulation currently expects the cartridge area (segments 4-7) to be
   * filled with the FLASH ROM content. Even segment 7, which will be copied to
   * the second 64K "hidden" and pagable flash area of the SD cartridge.
   * Currently, there is no support for the full sized SDEXT flash image
   */
  void SDExt::openImage(const char *sdimg_path)
  {
    disableSDExt();
    this->reset(false);
    if (sdf)
      std::fclose(sdf);
    writeProtectFlag = true;
    sdf = NULL;
    sdfno = -1;
    sd_card_size = 0U;
    sd_card_pos = 0U;
    if (!sdimg_path || sdimg_path[0] == '\0')
      return;
    sdf = std::fopen(sdimg_path, "r+b");
    if (!sdf) {
      sdf = std::fopen(sdimg_path, "rb");
      if (!sdf)
        throw Ep128Emu::Exception("error opening SD card image file");
    }
    else {
      writeProtectFlag = false;
    }
    std::setvbuf(sdf, (char *) 0, _IONBF, 0);
    sdfno = fileno(sdf);
    {
      uint16_t  c = 0;
      uint16_t  h = 0;
      uint16_t  s = 0;
      try {
        checkVHDImage(sdf, sdimg_path, c, h, s);
      }
      catch (...) {
        openImage((char *) 0);
        throw;
      }
      sd_card_size = (uint32_t(c) * uint32_t(h) * uint32_t(s)) << 9;
    }
    // turn emulation on
    enableSDExt();
  }

  static int safe_read(int fd, uint8_t *buffer, int size)
  {
    int all = 0;
    while (size) {
      int ret = read(fd, buffer, size);
      if (ret <= 0)
        break;
      all += ret;
      size -= ret;
      buffer += ret;
    }
    return all;
  }

  void SDExt::_block_read()
  {
    uint8_t *bufp = &(_buffer.front());
    ans_p = bufp;
    ans_index = 0;
    bufp[0] = 0xFF;             // wait a bit
    if (sd_card_pos > (sd_card_size - 512U)) {
      bufp[1] = 0x09;           // error, out of range
      ans_size = 2;
      ans_callback = false;
      return;
    }
    if (safe_read(sdfno, bufp + 2, 512) != 512) {
      bufp[1] = 0x03;           // CRC error
      ans_size = 2;
      ans_callback = false;
      return;
    }
    bufp[1] = 0xFE;             // data token
    bufp[512 + 2] = 0;          // CRC
    bufp[512 + 3] = 0;          // CRC
    ans_size = 512 + 4;
    sd_card_pos = sd_card_pos + 512U;
  }

  /* SPI is a read/write in once stuff. We have only a single function ...
   * _write_b is the data value to put on MOSI
   * _read_b is the data read from MISO without spending _ANY_ SPI time to do
   * shifting!
   * This is not a real thing, but easier to code this way.
   * The implementation of the real behaviour is up to the caller of this
   * function.
   */
  void SDExt::_spi_shifting_with_sd_card()
  {
    if (!cs0) {
      // Currently, we only emulate one SD card,
      // and it must be selected for any answer
      _read_b = 0xFF;
      return;
    }
    if (delayCnt > 0) {
      delayCnt--;
      return;
    }
    if (writeState) {
      _read_b = 0xFF;
      if (writeState == 0x01) {
        if (_write_b == 0xFD) { // stop token
          _read_b = 0;          // wait a tiny time...
          writeState = 0x00;    // ...but otherwise, end of write session
          return;
        }
        if (_write_b != 0xFE && _write_b != 0xFC)
          return;
        writeState++;
        return;
      }
      _buffer[writePos] = _write_b;     // store written byte
      if (++writePos >= (512 + 2)) {    // if one block (+ 2byte CRC)
        writePos = 0;                   // is written by host...
        if (sd_card_size > 0U && !writeProtectFlag &&
            sd_card_pos <= (sd_card_size - 512U) &&
            lseek(sdfno, off_t(sd_card_pos), SEEK_SET) == off_t(sd_card_pos) &&
            write(sdfno, &(_buffer.front()), 512) == 512) {
          _read_b = 5;          // data accepted
          // if multiple blocks: write mode back to the token waiting phase
          writeState = uint8_t(cmd[0] == 25);
          sd_card_pos = sd_card_pos + 512U;
        }
        else {
          _read_b = 13;         // write error
          writeState = 0x00;
        }
        delayCnt = 1;
      }
      return;
    }
    if (cmd_index == 0 && (_write_b & 0xC0) != 0x40) {
      if (ans_index < ans_size) {
        _read_b = ans_p[ans_index++];
      }
      else {
        if (ans_callback) {
          _block_read();
        }
        else {
          ans_index = 0;
          ans_size = 0;
        }
        _read_b = 0xFF;
      }
      return;
    }
    if (cmd_index < 6) {
      cmd[cmd_index++] = _write_b;
      _read_b = 0xFF;
      return;
    }
    cmd[0] &= 63;
    cmd_index = 0;
    ans_callback = false;
    switch (cmd[0]) {
      case 0:           // CMD 0
        _read_b = 1;    // IDLE state R1 answer
        break;
      case 1:           // CMD 1 - init
        _read_b = 0;    // non-IDLE now (?) R1 answer
        break;
      case 16:          // CMD16 - set blocklen (?!):
        // we only handle that as dummy command oh-oh ...
        _read_b = 0;    // R1 answer
        break;
      case 9:           // CMD9: read CSD register
        ADD_ANS(_read_csd_answer);
        _read_b = 0;    // R1
        break;
      case 10:          // CMD10: read CID register
        ADD_ANS(_read_cid_answer);
        _read_b = 0;    // R1
        break;
      case 58:          // CMD58: read OCR
        ADD_ANS(_read_ocr_answer);
        // R1 (R3 is sent as data in the emulation without the data token)
        _read_b = 0;
        break;
      case 12:          // CMD12: stop transmission (reading multiple)
        // actually we don't do too much, as on receiving a new command
        // callback will be deleted before this switch-case block
        ADD_ANS(_stop_transmission_answer);
        _read_b = 0;
        break;
      case 17:          // CMD17: read a single block, babe
      case 18:          // CMD18: read multiple blocks
        sd_card_pos = (uint32_t(cmd[1]) << 24) | (uint32_t(cmd[2]) << 16)
                      | (uint32_t(cmd[3]) << 8) | uint32_t(cmd[4]);
        if (sd_card_size > 0U && sd_card_pos <= (sd_card_size - 512U) &&
            lseek(sdfno, off_t(sd_card_pos), SEEK_SET) == off_t(sd_card_pos)) {
          _block_read();
          // in case of CMD18, continue multiple sectors,
          // register callback for that!
          ans_callback = (cmd[0] == 18);
          _read_b = 0;  // R1
        }
        else {
          // address error, if no SD card image or access beyond card size...
          // [this is bad, TODO: better error handling]
          _read_b = 32;
        }
        break;
      case 24:          // CMD24: write block
      case 25:          // CMD25: write multiple blocks
        writePos = 0;
        writeState = 0x01;
        sd_card_pos = (uint32_t(cmd[1]) << 24) | (uint32_t(cmd[2]) << 16)
                      | (uint32_t(cmd[3]) << 8) | uint32_t(cmd[4]);
        _read_b = 0;    // R1 answer, OK
        break;
      default:          // unimplemented command, heh!
        _read_b = 4;    // illegal command :-/
        break;
    }
  }

  /* Warning:
   * Some resources mention addresses like 0xFC00 for the I/O area.
   * Here, I mean addresses within segment 7 only, so it becomes 0x3C00 ...
   */

  uint8_t SDExt::readCartP3(uint32_t addr, const uint8_t *romPtr)
  {
    addr = addr & 0x3FFFU;
    if (addr < 0x2000) {
      uint32_t  addr_ = (uint32_t(rom_page_ofs) + addr) & 0xFFFFU;
      uint8_t   byte = 0xFF;
      if (addr_ < 0x4000U && romPtr)
        byte = romPtr[addr_];
      return byte;
    }
    if (addr < 0x3C00) {
      return sd_ram_ext[addr - 0x2000];
    }
    if (is_hs_read) {
      // in HS-read (high speed read) mode, all the 0x3C00-0x3FFF
      // acts as data _read_ register (but not for _write_!!!)
      // also, there is a fundamental difference compared to "normal" read:
      // each reads triggers SPI shifting in HS mode, but not in regular mode,
      // there only write does that!
      // HS-read initiates an SPI shift, but the result (AFAIK)
      // is the previous state, as shifting needs time!
      uint8_t old = _read_b;
      _spi_shifting_with_sd_card();
      return old;
    }
    switch (addr & 3) {
      case 0:
        // regular read (not HS) only gives the last shifted-in data,
        // that's all!
        return _read_b;
      case 1:
        // status reg: bit7=wp1, bit6=insert, bit5=changed
        // (insert/changed=1: some of the cards not inserted or changed)
        return status;
      case 2:           // ROM pager [hmm not readable?!]
        return 0xFF;
      case 3:           // HS read config is not readable?!]
        return 0xFF;
    }
    return 0xFF;        // make GCC happy :)
  }

  void SDExt::writeCartP3(uint32_t addr, uint8_t data)
  {
    addr = addr & 0x3FFFU;
    if (addr < 0x2000)                  // pageable ROM (8K), do not overwrite
      return;                           // [reflash is currently not supported]
    if (addr < 0x3C00) {                // SDEXT's RAM (7K), writable
      sd_ram_ext[addr - 0x2000] = data;
      return;
    }
    // rest 1K is the (memory mapped) I/O area
    switch (addr & 3) {
      case 0:           // data register
        if (!is_hs_read)
          _write_b = data;
        _write_specified = data;
        _spi_shifting_with_sd_card();
        break;
      case 1:
        // control register (bit7=CS0, bit6=CS1, bit5=clear change card signal)
        if (data & 32)  // clear change signal
          status &= 255 - 32;
        cs0 = bool(data & 128);
        cs1 = bool(data & 64);
        break;
      case 2:           // ROM pager register
        rom_page_ofs = uint16_t(data & 0xE0) << 8;  // only high 3 bits count
        break;
      case 3:           // HS (high speed) read mode to set: bit7=1
        is_hs_read = bool(data & 128);
        _write_b = is_hs_read ? 0xFF : _write_specified;
        break;
    }
  }

}       // namespace Ep128

