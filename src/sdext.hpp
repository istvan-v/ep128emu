
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

#ifndef EP128EMU_SDEXT_HPP
#define EP128EMU_SDEXT_HPP

#include "ep128emu.hpp"
#include <vector>

namespace Ep128 {

  class SDExt {
   protected:
    bool      sdext_enabled;    // only used in temporaryDisable()
    uint8_t   status;
    uint8_t   _spi_last_w;
    bool      is_hs_read;
    uint16_t  rom_page_ofs;
    bool      cs0;
    bool      cs1;
    uint32_t  sdextSegment;
    uint32_t  sdextAddress;
    // 7K of useful SRAM
    std::vector< uint8_t >  sd_ram_ext;
    // 64K flash ROM
    std::vector< uint8_t >  sd_rom_ext;
    uint8_t   cmd[6];
    uint8_t   cmd_index;
    uint8_t   _read_b;
    uint8_t   _write_b;
    uint8_t   _write_specified;
    const uint8_t *ans_p;
    int       ans_index;
    int       ans_size;
    int       writePos;
    uint8_t   writeState;       // 0 = not writing, 1 = waiting, 2 = writing
    bool      ans_callback;
    uint8_t   delayCnt;
    bool      writeProtectFlag;
    std::FILE *sdf;
    int       sdfno;
    std::vector< uint8_t >  _buffer;
    uint32_t  sd_card_size;
    uint32_t  sd_card_pos;
    std::string romFileName;
    bool      romFileWriteProtected;
    bool      romDataChanged;
    bool      flashErased;      // true if sd_rom_ext is filled with 0xFF bytes
    uint8_t   flashCommand;     // the lower nibble is the bus cycle (0 to 5)
    // ----------------
    void _block_read();
    void _spi_shifting_with_sd_card();
    uint8_t flashRead(uint32_t addr);
    void flashWrite(uint32_t addr, uint8_t data);
    uint8_t flashReadDebug(uint32_t addr) const;
   public:
    SDExt();
    virtual ~SDExt();
    void setEnabled(bool isEnabled);
    // for demo recording/playback, calling with isDisabled == false
    // restores the state previously set with setEnabled()
    void temporaryDisable(bool isDisabled);
    void reset(bool clear_ram);
    void openImage(const char *sdimg_path);
    void openROMFile(const char *fileName);
    uint8_t readCartP3(uint32_t addr);
    void writeCartP3(uint32_t addr, uint8_t data);
    uint8_t readCartP3Debug(uint32_t addr) const;
    EP128EMU_INLINE bool isSDExtSegment(uint8_t segment) const
    {
      return (segment == sdextSegment);
    }
    EP128EMU_INLINE bool isSDExtAddress(uint32_t addr) const
    {
      return ((addr & 0x003FC000U) == sdextAddress);
    }
  };

}       // namespace Ep128

#endif  // EP128EMU_SDEXT_HPP

