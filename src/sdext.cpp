
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
    0x00, 0x5D, 0x01, 0x32, 0x13, 0x50, 0x80, 0x00,
    0x36, 0xD8, 0x4F, 0xFF, 0x16, 0x40, 0x00, 0x00,
    0, 0        // CRC bytes
  };
  static const uint8_t _read_cid_answer[] = {
    0xFF,       // waiting a bit
    0xFE,       // data token
    // the CID itself
    0x01, 0x45, 0x50, 0x31, 0x32, 0x38, 0x53, 0x44,     // "EP128SD"
    0x10, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0A, 0x00,     // Rev 1.0, Oct 2016
    0, 0        // CRC bytes
  };
  // no data token, nor CRC!
  // (technically this is the R3 answer minus the R1 part at the beginning...)
  static const uint8_t _read_ocr_answer[] = {
    // the OCR itself
    0x80, 0xFF, 0x80, 0x00
  };

#define ADD_ANS(ans) { ans_p = (ans); ans_bytes_left = uint32_t(sizeof(ans)); }

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
      ans_bytes_left(0U),
      serialNum(0U),
      writePos(0),
      writeState(0x00),
      ans_callback(false),
      delayCnt(0),
      writeProtectFlag(true),
      sdf((std::FILE *) 0),
      sdfno(-1),
      sd_card_size(0U),
      sd_card_pos(0U),
      romFileName(""),
      romFileWriteProtected(false),
      romDataChanged(false),
      flashErased(true),
      flashCommand(0x00)
  {
    sd_ram_ext.resize(0x00001C00, 0xFF);
    sd_rom_ext.resize(0x00010000, 0xFF);
    _buffer.resize(1024, 0x00);
    this->reset(1);
  }

  SDExt::~SDExt()
  {
    openImage((char *) 0);
    try {
      openROMFile((char *) 0);
    }
    catch (...) {
      // FIXME: errors are ignored here
    }
  }

  void SDExt::setEnabled(bool isEnabled)
  {
    sdext_enabled = isEnabled;
    sdextSegment = 0x07U | (uint32_t(isEnabled) - 1U);
    sdextAddress = (0x07U << 14) | (uint32_t(isEnabled) - 1U);
  }

  void SDExt::temporaryDisable(bool isDisabled)
  {
    if (isDisabled) {
      sdextSegment = 0xFFFFFFFFU;
      sdextAddress = 0xFFFFFFFFU;
    }
    else {
      setEnabled(sdext_enabled);
    }
  }

  void SDExt::reset(int reset_level)
  {
    if (reset_level >= 2)
      std::memset(&(sd_ram_ext.front()), 0xFF, sd_ram_ext.size());
    rom_page_ofs = 0x0000;
    is_hs_read = false;
    cmd_index = 0;
    ans_p = (uint8_t *) 0;
    ans_bytes_left = 0U;
    writePos = 0;
    writeState = 0x00;
    ans_callback = false;
    delayCnt = 0;
    if (reset_level >= 1) {
      // card change, not initialized
      status = (uint8_t(writeProtectFlag) << 7) | (uint8_t(!sdf) << 6) | 0x21;
    }
    _read_b = 0;
    _write_b = 0xFF;
    _spi_last_w = 0xFF;
    sd_card_pos = 0U;
    flashCommand = 0x00;
  }

  void SDExt::openImage(const char *sdimg_path)
  {
    serialNum = 0U;
    writeProtectFlag = true;
    if (sdf)
      std::fclose(sdf);
    sdf = NULL;
    sdfno = -1;
    sd_card_size = 0U;
    this->reset(1);
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
      status = status & 0x7F;           // not write protected
    }
    status = status & 0xBF;             // card inserted
    std::setvbuf(sdf, (char *) 0, _IONBF, 0);
    sdfno = fileno(sdf);
    {
      try {
        uint16_t  c = 0;
        uint16_t  h = 0;
        uint16_t  s = 0;
        uint32_t  tmp = checkVHDImage(sdf, sdimg_path, c, h, s);
        sd_card_size = tmp << 9;
        int       n = 0;
        while ((tmp > 4096U || n < 2) && !(tmp & 1U)) {
          tmp = tmp >> 1;
          n++;
        }
        if (!(tmp > 0U && tmp <= 4096U && n >= 2 && n <= 10))
          throw Ep128Emu::Exception("invalid disk image geometry for SD card");
      }
      catch (...) {
        openImage((char *) 0);
        throw;
      }
    }
    serialNum =
        Ep128Emu::File::hash_32(reinterpret_cast< const unsigned char * >(
                                    sdimg_path), std::strlen(sdimg_path));
  }

  void SDExt::openROMFile(const char *fileName)
  {
    const char  *errMsg = (char *) 0;
    if (romDataChanged) {
      romDataChanged = false;
      if (!(romFileName.empty() || romFileWriteProtected)) {
        // write old ROM to disk
        std::FILE *f = std::fopen(romFileName.c_str(), "wb");
        if (!f) {
          errMsg = "SDExt: error saving flash ROM";
        }
        else {
          if (std::fwrite(&(sd_rom_ext.front()),
                          sizeof(uint8_t), sd_rom_ext.size(), f)
              != sd_rom_ext.size() || std::fflush(f) != 0) {
            errMsg = "SDExt: error saving flash ROM";
          }
          std::fclose(f);
        }
      }
    }
    romFileName.clear();
    romFileWriteProtected = false;
    flashErased = true;
    flashCommand = 0x00;
    std::memset(&(sd_rom_ext.front()), 0xFF, sd_rom_ext.size());
    if (errMsg)
      throw Ep128Emu::Exception(errMsg);
    if (!fileName || fileName[0] == '\0')
      return;
    // load ROM image from disk
    std::FILE *f = std::fopen(fileName, "r+b");
    if (!f) {
      f = std::fopen(fileName, "rb");
      if (!f)
        throw Ep128Emu::Exception("SDExt: error opening ROM file");
      romFileWriteProtected = true;
    }
    size_t  bytesRead = std::fread(&(sd_rom_ext.front()),
                                   sizeof(uint8_t), sd_rom_ext.size(), f);
    if (!(bytesRead >= 0x1000 && bytesRead <= sd_rom_ext.size())) {
      std::fclose(f);
      romFileWriteProtected = false;
      std::memset(&(sd_rom_ext.front()), 0xFF, sd_rom_ext.size());
      throw Ep128Emu::Exception("SDExt: error loading ROM file");
    }
    std::fclose(f);
    for (size_t i = 0; flashErased && i < sd_rom_ext.size(); i++)
      flashErased = (sd_rom_ext[i] == 0xFF);
    romFileName = fileName;
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
    bufp[0] = 0xFF;             // wait a bit
    if (sd_card_pos > (sd_card_size - 512U)) {
      bufp[1] = 0x09;           // error, out of range
      ans_bytes_left = 2U;
      ans_callback = false;
      return;
    }
    if (safe_read(sdfno, bufp + 2, 512) != 512) {
      bufp[1] = 0x03;           // CC error
      ans_bytes_left = 2U;
      ans_callback = false;
      return;
    }
    bufp[1] = 0xFE;             // data token
    bufp[512 + 2] = 0;          // CRC
    bufp[512 + 3] = 0;          // CRC
    ans_bytes_left = 516U;
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
      if (ans_bytes_left) {
        _read_b = *(ans_p++);
        ans_bytes_left--;
      }
      else {
        if (ans_callback)
          _block_read();
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
    if (status & 0x01) {
      // in idle state, only CMD0, CMD1, ACMD41, CMD58 and CMD59 are valid
      if (!(cmd[0] == 1 || cmd[0] == 55 || cmd[0] == 58 || cmd[0] == 59)) {
        _read_b = (cmd[0] == 8 ? 0x05 : 0x01);  // IDLE state R1 answer
        return;
      }
    }
    switch (cmd[0]) {
      case 0:                   // CMD 0
        status = status | 0x01;
        _read_b = 0x01;         // IDLE state R1 answer
        break;
      case 1:                   // CMD 1 - init
        // fail if there is no card inserted
        status = (status & 0xFE) | uint8_t(!sdf);
        _read_b = status & 0x01;        // non-IDLE now (?) R1 answer
        break;
      case 9:                   // CMD9: read CSD register
      case 10:                  // CMD10: read CID register
        if (cmd[0] == 9) {
          ans_p = _read_csd_answer;
          ans_bytes_left = uint32_t(sizeof(_read_csd_answer));
        }
        else {
          ans_p = _read_cid_answer;
          ans_bytes_left = uint32_t(sizeof(_read_cid_answer));
        }
        std::memcpy(&(_buffer.front()), ans_p, ans_bytes_left);
        ans_p = &(_buffer.front());
        if (cmd[0] == 9) {
          size_t  n = sd_card_size >> 9;
          uint8_t m = 0;
          while (n > 4096 && m < 10 && !(n & 1)) {
            n = n >> 1;
            m++;
          }
          if (m < 10) {
            _buffer[7] = _buffer[7] | 0x09;     // block size = 2^9 = 512 bytes
          }
          else {                                // card size > 1 GB:
            _buffer[7] = _buffer[7] | 0x0A;     // need 1024 bytes block size
            m--;
          }
          n = (n - 1) & 0x0FFF;
          m = (m - 2) & 0x07;
          _buffer[8] = _buffer[8] | uint8_t(n >> 10);
          _buffer[9] = uint8_t((n >> 2) & 0xFF);
          _buffer[10] = _buffer[10] | uint8_t((n << 6) & 0xC0);
          _buffer[11] = _buffer[11] | (m >> 1);
          _buffer[12] = _buffer[12] | ((m << 7) & 0x80);
        }
        else {
          _buffer[11] = uint8_t((serialNum >> 24) & 0xFFU);
          _buffer[12] = uint8_t((serialNum >> 16) & 0xFFU);
          _buffer[13] = uint8_t((serialNum >> 8) & 0xFFU);
          _buffer[14] = uint8_t(serialNum & 0xFFU);
        }
        {
          unsigned char crc7 = 0x00;
          for (size_t i = 0; i < ((ans_bytes_left - 5U) << 3); i++) {
            crc7 = crc7 ^ (((ans_p[(i >> 3) + 2] << (i & 7)) >> 1) & 0x40);
            crc7 = ((crc7 << 1) ^ ((crc7 & 0x40) >> 3)) | ((crc7 & 0x40) >> 6);
          }
          _buffer[ans_bytes_left - 3U] = ((crc7 & 0x7F) << 1) | 0x01;
        }
        _read_b = 0x00;         // R1
        break;
      case 12:                  // CMD12: stop transmission (reading multiple)
        // actually we don't do too much, as on receiving a new command
        // callback will be deleted before this switch-case block
        ADD_ANS(_stop_transmission_answer);
        _read_b = 0x00;
        break;
      case 16:                  // CMD16 - set blocklen (?!):
        // we only handle that as dummy command oh-oh...
        if ((cmd[1] | cmd[2] | (cmd[3] ^ 0x02) | cmd[4]) != 0) {
          // TODO: implement support for block sizes other than 512 bytes
          _read_b = 0x40;       // parameter error
          return;
        }
        _read_b = 0x00;         // R1 answer
        break;
      case 17:                  // CMD17: read a single block, babe
      case 18:                  // CMD18: read multiple blocks
        sd_card_pos = (uint32_t(cmd[1]) << 24) | (uint32_t(cmd[2]) << 16)
                      | (uint32_t(cmd[3]) << 8) | uint32_t(cmd[4]);
        if (sd_card_size > 0U && sd_card_pos <= (sd_card_size - 512U) &&
            lseek(sdfno, off_t(sd_card_pos), SEEK_SET) == off_t(sd_card_pos)) {
          _block_read();
          // in case of CMD18, continue multiple sectors,
          // register callback for that!
          ans_callback = (cmd[0] == 18);
          _read_b = 0x00;       // R1
        }
        else {
          // address error, if no SD card image or access beyond card size...
          // [this is bad, TODO: better error handling]
          _read_b = 0x20;
        }
        break;
      case 24:                  // CMD24: write block
      case 25:                  // CMD25: write multiple blocks
        writePos = 0;
        writeState = 0x01;
        sd_card_pos = (uint32_t(cmd[1]) << 24) | (uint32_t(cmd[2]) << 16)
                      | (uint32_t(cmd[3]) << 8) | uint32_t(cmd[4]);
        _read_b = 0x00;         // R1 answer, OK
        break;
      case 58:                  // CMD58: read OCR
        ADD_ANS(_read_ocr_answer);
        // R1 (R3 is sent as data in the emulation without the data token)
        _read_b = 0x00;
        break;
      default:                  // unimplemented command, heh!
        _read_b = 0x04;         // illegal command :-/
        break;
    }
  }

  /* Warning:
   * Some resources mention addresses like 0xFC00 for the I/O area.
   * Here, I mean addresses within segment 7 only, so it becomes 0x3C00 ...
   */

  uint8_t SDExt::readCartP3(uint32_t addr)
  {
    addr = addr & 0x3FFFU;
    if (addr < 0x2000) {
      uint32_t  addr_ = (uint32_t(rom_page_ofs) + addr) & 0xFFFFU;
      if (EP128EMU_UNLIKELY(flashCommand != 0x00))
        return flashRead(addr_);
      uint8_t   byte = 0xFF;
      if (addr_ < sd_rom_ext.size())
        byte = sd_rom_ext[addr_];
      return byte;
    }
    if (addr < 0x3C00)
      return sd_ram_ext[addr - 0x2000];
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
        return (status | 0x1F);
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
    if (EP128EMU_UNLIKELY(addr < 0x2000)) {     // pageable ROM (8K)
      flashWrite((uint32_t(rom_page_ofs) + addr) & 0xFFFFU, data);
      return;
    }
    if (addr < 0x3C00) {                // SDEXT's RAM (7K), writable
      sd_ram_ext[addr - 0x2000] = data;
      return;
    }
    // rest 1K is the (memory mapped) I/O area
    switch (addr & 3) {
      case 0:                   // data register
        if (!is_hs_read)
          _write_b = data;
        _write_specified = data;
        _spi_shifting_with_sd_card();
        break;
      case 1:
        // control register (bit7=CS0, bit6=CS1, bit5=clear change card signal)
        if (data & 0x20)        // clear change signal
          status = status & 0xDF;
        cs0 = bool(data & 128);
        cs1 = bool(data & 64);
        break;
      case 2:                   // ROM pager register
        rom_page_ofs = uint16_t(data & 0xE0) << 8;  // only high 3 bits count
        break;
      case 3:                   // HS (high speed) read mode to set: bit7=1
        is_hs_read = bool(data & 128);
        _write_b = is_hs_read ? 0xFF : _write_specified;
        break;
    }
  }

  uint8_t SDExt::readCartP3Debug(uint32_t addr) const
  {
    addr = addr & 0x3FFFU;
    if (addr < 0x2000) {
      uint32_t  addr_ = (uint32_t(rom_page_ofs) + addr) & 0xFFFFU;
      if (EP128EMU_UNLIKELY(flashCommand != 0x00))
        return flashReadDebug(addr_);
      uint8_t   byte = 0xFF;
      if (addr_ < sd_rom_ext.size())
        byte = sd_rom_ext[addr_];
      return byte;
    }
    if (addr < 0x3C00)
      return sd_ram_ext[addr - 0x2000];
    switch (addr & 3) {
      case 0:
        // regular read (not HS) only gives the last shifted-in data,
        // that's all!
        return _read_b;
      case 1:
        // status reg: bit7=wp1, bit6=insert, bit5=changed
        // (insert/changed=1: some of the cards not inserted or changed)
        // NOTE: for debugging, some of the unused bits are also set:
        //   bit2 = idle state
        //   bit1 = cs1
        //   bit0 = cs0
        return ((status & 0xE0) | 0x18 | ((status & 0x01) << 2)
                | uint8_t(cs0) | (uint8_t(cs1) << 1));
      case 2:           // ROM pager
        return uint8_t(rom_page_ofs >> 8);
      case 3:           // HS read config
        return (uint8_t(is_hs_read) << 7);
    }
    return 0xFF;        // make GCC happy :)
  }

  // --------------------------------------------------------------------------

  uint8_t SDExt::flashRead(uint32_t addr)
  {
    uint32_t  addr_ = (uint32_t(flashCommand & 0xF0) << 8) | (addr & 0xFFU);
    flashCommand = 0x00;
    switch (addr_) {
    case 0x9000U:               // manufacturer ID
      return 0x01;
    case 0x9002U:               // device ID, top boot block
      return 0x23;
    case 0x9004U:               // sector protect status, etc?
      return 0x00;
    }
    if (addr < sd_rom_ext.size())
      return sd_rom_ext[addr];
    return 0xFF;
  }

  void SDExt::flashWrite(uint32_t addr, uint8_t data)
  {
    if ((flashCommand & 0xF0) == 0x90) {
      // autoselect mode does not have wr cycles more (only rd)
      flashCommand = 0x90;
    }
    switch (flashCommand & 0x0F) {
    case 0:
      flashCommand = 0x00;      // invalidate command
      if (data == 0xB0 || data == 0x30) {
        // well, erase suspend/resume is currently not supported :-(
        return;
      }
      if (data == 0xF0) {       // reset command
        return;
      }
      if ((addr & 0x3FFFU) != 0x0AAAU || data != 0xAA) {
        return;                 // invalid cmd seq
      }
      flashCommand++;           // next cycle
      return;
    case 1:
      if ((addr & 0x3FFFU) != 0x0555U || data != 0x55) {
        flashCommand = 0x00;    // invalid cmd seq
        return;
      }
      flashCommand++;           // next cycle
      return;
    case 2:
      if ((addr & 0x3FFFU) != 0x0AAAU ||
          (data != 0x80 && data != 0x90 && data != 0xA0)) {
        flashCommand = 0x00;    // invalid cmd seq
        return;
      }
      flashCommand = data | 0x03;
      return;
    case 3:
      if ((flashCommand & 0xF0) == 0xA0) {      // program command!!!!
        if (!romFileWriteProtected && addr < sd_rom_ext.size()) {
          // flash programming allows only 1->0 on data bits,
          // erase must be executed for 0->1
          data = data & sd_rom_ext[addr];
          if (data != sd_rom_ext[addr]) {
            romDataChanged = true;
            flashErased = false;
            sd_rom_ext[addr] = data;
          }
        }
        flashCommand = 0x00;    // end of command
        return;
      }
      // only flash command 0x80 can be left,
      // 0x90 handled before "switch", 0xA0 just before...
      if ((addr & 0x3FFFU) != 0x0AAAU || data != 0xAA) {
        flashCommand = 0x00;    // invalid cmd seq
        return;
      }
      flashCommand++;           // next cycle
      return;
    case 4:                     // only flash command 0x80 can get this far...
      if ((addr & 0x3FFFU) != 0x0555U || data != 0x55) {
        flashCommand = 0x00;    // invalid cmd seq
        return;
      }
      flashCommand++;           // next cycle
      return;
    case 5:                     // only flash command 0x80 can get this far...
      if (((addr & 0x3FFFU) == 0x0AAAU && data == 0x10) || data == 0x30) {
        if (!(romFileWriteProtected || flashErased)) {
          romDataChanged = true;
          flashErased = true;
          std::memset(&(sd_rom_ext.front()), 0xFF, sd_rom_ext.size());
        }
      }
      flashCommand = 0x00;      // end of erase command?
      return;
    }
    flashCommand = 0x00;        // should not be reached
  }

  uint8_t SDExt::flashReadDebug(uint32_t addr) const
  {
    uint32_t  addr_ = (uint32_t(flashCommand & 0xF0) << 8) | (addr & 0xFFU);
    switch (addr_) {
    case 0x9000U:               // manufacturer ID
      return 0x01;
    case 0x9002U:               // device ID, top boot block
      return 0x23;
    case 0x9004U:               // sector protect status, etc?
      return 0x00;
    }
    if (addr < sd_rom_ext.size())
      return sd_rom_ext[addr];
    return 0xFF;
  }

  // --------------------------------------------------------------------------

  class ChunkType_SDExtSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    SDExt&  ref;
   public:
    ChunkType_SDExtSnapshot(SDExt& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_SDExtSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_SDEXT_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void SDExt::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000001U);       // version number
    buf.writeBoolean(sdext_enabled);
    buf.writeBoolean(cs0);
    buf.writeBoolean(cs1);
    buf.writeBoolean(is_hs_read);
    buf.writeUInt16(rom_page_ofs);
    // save 7K SRAM if not empty
    bool    sramEmpty = true;
    for (size_t i = 0; sramEmpty && i < sd_ram_ext.size(); i++)
      sramEmpty = (sd_ram_ext[i] == 0xFF);
    buf.writeBoolean(!sramEmpty);
    if (!sramEmpty) {
      for (size_t i = 0; i < sd_ram_ext.size(); i++)
        buf.writeByte(sd_ram_ext[i]);
    }
    // save 64K flash ROM if not empty
    if (flashErased) {
      buf.writeUInt16(0);
      buf.writeByte(0xFF);
    }
    else {
      size_t  lastPos = sd_rom_ext.size() - 1;
      while (lastPos > 0 && sd_rom_ext[lastPos] == 0xFF)
        lastPos--;
      buf.writeUInt16(uint16_t(lastPos));
      for (size_t i = 0; i <= lastPos; i++)
        buf.writeByte(sd_rom_ext[i]);
    }
  }

  void SDExt::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_SDEXT_STATE, buf);
  }

  void SDExt::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000001U) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible SDExt snapshot format");
    }
    try {
      // save flash ROM first if changed, reset it to erased state
      openROMFile((char *) 0);
      romFileWriteProtected = true;
      // reset the interface as most registers are not saved in the snapshot
      this->reset(1);
      // load saved state
      setEnabled(buf.readBoolean());
      cs0 = buf.readBoolean();
      cs1 = buf.readBoolean();
      (void) buf.readBoolean();         // is_hs_read is ignored
      rom_page_ofs = buf.readUInt16() & 0xE000;
      // 7K SRAM
      if (buf.readBoolean()) {
        for (size_t i = 0; i < sd_ram_ext.size(); i++)
          sd_ram_ext[i] = buf.readByte();
      }
      else {
        std::memset(&(sd_ram_ext.front()), 0xFF, sd_ram_ext.size());
      }
      // 64K flash ROM
      size_t  romSize = size_t(buf.readUInt16()) + 1;
      if (romSize > sd_rom_ext.size())
        romSize = sd_rom_ext.size();
      for (size_t i = 0; i < romSize; i++) {
        sd_rom_ext[i] = buf.readByte();
        flashErased = (flashErased && (sd_rom_ext[i] == 0xFF));
      }
      if (buf.getPosition() != buf.getDataSize()) {
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "SDExt snapshot data");
      }
    }
    catch (...) {
      // reset SDExt
      this->reset(2);
      openROMFile((char *) 0);
      romFileWriteProtected = true;
      throw;
    }
  }

  void SDExt::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_SDExtSnapshot *p;
    p = new ChunkType_SDExtSnapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace Ep128

