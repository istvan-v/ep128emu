
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2010 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "fileio.hpp"
#include "system.hpp"
#include "cfg_db.hpp"
#include "mkcfg_fl.hpp"
#include "guicolor.hpp"

#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>

#ifndef WIN32
#  include <sys/types.h>
#  include <sys/stat.h>
#else
#  include <direct.h>
#endif

#include <vector>

static int keyboardMap_EP[256] = {
  0x006E,     -1, 0x005C,     -1, 0x0062,     -1, 0x0063,     -1,
  0x0076,     -1, 0x0078,     -1, 0x007A,     -1, 0xFFE1,     -1,
  0x0068,     -1, 0xFFE5,     -1, 0x0067,     -1, 0x0064,     -1,
  0x0066,     -1, 0x0073,     -1, 0x0061,     -1, 0xFFE3, 0xFFE4,
  0x0075,     -1, 0x0071,     -1, 0x0079,     -1, 0x0072,     -1,
  0x0074,     -1, 0x0065,     -1, 0x0077,     -1, 0xFF09,     -1,
  0x0037,     -1, 0x0031,     -1, 0x0036,     -1, 0x0034,     -1,
  0x0035,     -1, 0x0033,     -1, 0x0032,     -1, 0xFF1B,     -1,
  0xFFC1,     -1, 0xFFC5,     -1, 0xFFC0,     -1, 0xFFC3,     -1,
  0xFFC2,     -1, 0xFFC4,     -1, 0xFFBF,     -1, 0xFFBE,     -1,
  0x0038,     -1,     -1,     -1, 0x0039,     -1, 0x002D,     -1,
  0x0030,     -1, 0x003D,     -1, 0xFF08,     -1,     -1,     -1,
  0x006A,     -1,     -1,     -1, 0x006B,     -1, 0x003B,     -1,
  0x006C,     -1, 0x0027,     -1, 0x005D,     -1,     -1,     -1,
  0xFF61, 0xFF57, 0xFF54,     -1, 0xFF53,     -1, 0xFF52,     -1,
#ifndef WIN32
  0xFF13, 0xFF50, 0xFF51,     -1, 0xFF0D,     -1, 0xFFEA, 0xFE03,
#else
  0xFF13, 0xFF50, 0xFF51,     -1, 0xFF0D,     -1, 0xFFEA, 0xFF67,
#endif
  0x006D,     -1, 0xFFFF,     -1, 0x002C,     -1, 0x002F,     -1,
  0x002E,     -1, 0xFFE2,     -1, 0x0020,     -1, 0xFF63,     -1,
  0x0069,     -1,     -1,     -1, 0x006F,     -1, 0x0060,     -1,
  0x0070,     -1, 0x005B,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  0xFFB9, 0xC001, 0xFFB7, 0xC000, 0xFFB5, 0xC003, 0xFFAF, 0xC002,
  0xFFAB, 0xC010,     -1,     -1,     -1,     -1,     -1,     -1,
  0xFFB6, 0xC005, 0xFFB4, 0xC004, 0xFFB2, 0xC007, 0xFFB8, 0xC006,
  0xFFB0, 0xC011,     -1,     -1,     -1,     -1,     -1,     -1
};

#ifndef WIN32

static int keyboardMap_EP_HU[256] = {
  0x006E,     -1, 0x01FB,     -1, 0x0062,     -1, 0x0063,     -1,
  0x0076,     -1, 0x0078,     -1, 0x0079,     -1, 0xFFE1,     -1,
  0x0068,     -1, 0xFFE5,     -1, 0x0067,     -1, 0x0064,     -1,
  0x0066,     -1, 0x0073,     -1, 0x0061,     -1, 0xFFE3, 0xFFE4,
  0x0075,     -1, 0x0071,     -1, 0x007A,     -1, 0x0072,     -1,
  0x0074,     -1, 0x0065,     -1, 0x0077,     -1, 0xFF09,     -1,
  0x0037,     -1, 0x0031,     -1, 0x0036,     -1, 0x0034,     -1,
  0x0035,     -1, 0x0033,     -1, 0x0032,     -1, 0xFF1B,     -1,
  0xFFC1,     -1, 0xFFC5,     -1, 0xFFC0,     -1, 0xFFC3,     -1,
  0xFFC2,     -1, 0xFFC4,     -1, 0xFFBF,     -1, 0xFFBE,     -1,
  0x0038,     -1,     -1,     -1, 0x0039,     -1, 0x00FC,     -1,
  0x00F6,     -1, 0x00F3,     -1, 0xFF08,     -1,     -1,     -1,
  0x006A,     -1,     -1,     -1, 0x006B,     -1, 0x00E9,     -1,
  0x006C,     -1, 0x00E1,     -1, 0x00FA,     -1,     -1,     -1,
  0xFF61, 0xFF57, 0xFF54,     -1, 0xFF53,     -1, 0xFF52,     -1,
  0xFF13, 0xFF50, 0xFF51,     -1, 0xFF0D,     -1, 0xFFEA, 0xFE03,
  0x006D,     -1, 0xFFFF,     -1, 0x002C,     -1, 0x002D,     -1,
  0x002E,     -1, 0xFFE2,     -1, 0x0020,     -1, 0xFF63,     -1,
  0x0069,     -1,     -1,     -1, 0x006F,     -1, 0x0030,     -1,
  0x0070,     -1, 0x01F5,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  0xFFB9, 0xC001, 0xFFB7, 0xC000, 0xFFB5, 0xC003, 0xFFAF, 0xC002,
  0xFFAB, 0xC010,     -1,     -1,     -1,     -1,     -1,     -1,
  0xFFB6, 0xC005, 0xFFB4, 0xC004, 0xFFB2, 0xC007, 0xFFB8, 0xC006,
  0xFFB0, 0xC011,     -1,     -1,     -1,     -1,     -1,     -1
};

#else   // WIN32

static int keyboardMap_EP_HU[256] = {
  0x006E,     -1, 0x005C,     -1, 0x0062,     -1, 0x0063,     -1,
  0x0076,     -1, 0x0078,     -1, 0x0079,     -1, 0xFFE1,     -1,
  0x0068,     -1, 0xFFE5,     -1, 0x0067,     -1, 0x0064,     -1,
  0x0066,     -1, 0x0073,     -1, 0x0061,     -1, 0xFFE3, 0xFFE4,
  0x0075,     -1, 0x0071,     -1, 0x007A,     -1, 0x0072,     -1,
  0x0074,     -1, 0x0065,     -1, 0x0077,     -1, 0xFF09,     -1,
  0x0037,     -1, 0x0031,     -1, 0x0036,     -1, 0x0034,     -1,
  0x0035,     -1, 0x0033,     -1, 0x0032,     -1, 0xFF1B,     -1,
  0xFFC1,     -1, 0xFFC5,     -1, 0xFFC0,     -1, 0xFFC3,     -1,
  0xFFC2,     -1, 0xFFC4,     -1, 0xFFBF,     -1, 0xFFBE,     -1,
  0x0038,     -1,     -1,     -1, 0x0039,     -1, 0x002F,     -1,
  0x0060,     -1, 0x003D,     -1, 0xFF08,     -1,     -1,     -1,
  0x006A,     -1,     -1,     -1, 0x006B,     -1, 0x003B,     -1,
  0x006C,     -1, 0x0027,     -1, 0x005D,     -1,     -1,     -1,
  0xFF61, 0xFF57, 0xFF54,     -1, 0xFF53,     -1, 0xFF52,     -1,
  0xFF13, 0xFF50, 0xFF51,     -1, 0xFF0D,     -1, 0xFFEA, 0xFF67,
  0x006D,     -1, 0xFFFF,     -1, 0x002C,     -1, 0x002D,     -1,
  0x002E,     -1, 0xFFE2,     -1, 0x0020,     -1, 0xFF63,     -1,
  0x0069,     -1,     -1,     -1, 0x006F,     -1, 0x0030,     -1,
  0x0070,     -1, 0x005B,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  0xFFB9, 0xC001, 0xFFB7, 0xC000, 0xFFB5, 0xC003, 0xFFAF, 0xC002,
  0xFFAB, 0xC010,     -1,     -1,     -1,     -1,     -1,     -1,
  0xFFB6, 0xC005, 0xFFB4, 0xC004, 0xFFB2, 0xC007, 0xFFB8, 0xC006,
  0xFFB0, 0xC011,     -1,     -1,     -1,     -1,     -1,     -1
};

#endif  // WIN32

static const char *keyboardConfigFileNames[8] = {
  "EP_Keyboard_US.cfg",         // 0
  "EP_Keyboard_HU.cfg",         // 1
  (char *) 0,                   // 2
  (char *) 0,                   // 3
  (char *) 0,                   // 4
  (char *) 0,                   // 5
  (char *) 0,                   // 6
  (char *) 0                    // 7
};

static void setKeyboardConfiguration(Ep128Emu::ConfigurationDB& config, int n)
{
  int     *keyboardMapPtr = &(keyboardMap_EP[0]);
  switch (n) {
  case 0:                               // Enterprise
    keyboardMapPtr = &(keyboardMap_EP[0]);
    break;
  case 1:                               // Enterprise (HU)
    keyboardMapPtr = &(keyboardMap_EP_HU[0]);
    break;
  }
  for (unsigned int i = 0U; i < 256U; i++) {
    char    tmpBuf[16];
    std::sprintf(&(tmpBuf[0]), "keyboard.%02X.%X", (i >> 1), (i & 1U));
    config.createKey(std::string(&(tmpBuf[0])), keyboardMapPtr[i]);
  }
}

class Ep128EmuMachineConfiguration {
 private:
  struct {
    unsigned int  cpuClockFrequency;
    unsigned int  videoClockFrequency;
    unsigned int  soundClockFrequency;
    bool          enableMemoryTimingEmulation;
    bool          enableFileIO;
  } vm;
  struct {
    struct {
      int       size;
    } ram;
    struct ROMSegmentConfig {
      std::string file;
      int         offset;
      ROMSegmentConfig()
        : file(""),
          offset(0)
      {
      }
    };
    // ROM files can be loaded to segments 0x00 to 0x07, 0x10 to 0x13,
    // 0x20 to 0x23, 0x30 to 0x33, and 0x40 to 0x43
    ROMSegmentConfig  rom[68];
    // epmemcfg format configuration file (if specified, the RAM/ROM settings
    // above are ignored)
    std::string       configFile;
  } memory;
 public:
  Ep128EmuMachineConfiguration(Ep128Emu::ConfigurationDB& config, int n,
                               const std::string& romDirectory);
  ~Ep128EmuMachineConfiguration()
  {
  }
};

// bits 0..5: RAM size / 64K
// bit  6:    exos20.rom at segments 00h..01h
// bit  7:    exos21.rom at segments 00h..01h
// bit  8:    exos22.rom at segments 00h..03h
// bit  9:    exos23.rom at segments 00h..03h
// bit 10:    exos231esp.rom at segments 00h..03h
// bit 11:    exos231hun.rom at segments 00h..03h
// bit 12:    exos231uk.rom at segments 00h..03h
// bit 13:    basic20.rom at segment 04h
// bit 14:    basic21.rom at segment 04h
// bit 15:    basic21.rom at segment 05h
// bit 16:    basic21.rom at segment 06h
// bit 17:    exdos10.rom at segment 20h
// bit 18:    exdos13.rom at segments 20h..21h
// bit 19:    exdos13isdos10esp.rom at segments 20h..21h
// bit 20:    exdos13isdos10hun.rom at segments 20h..21h
// bit 21:    epfileio.rom at segment 10h
// bit 22:    brd.rom at segment 04h
// bit 23:    brd.rom at segment 07h
// bit 24:    brd.rom at segment 32h
// bit 25:    esp.rom at segment 04h
// bit 26:    esp.rom at segment 07h
// bit 27:    esp.rom at segment 32h
// bit 28:    hun.rom at segment 04h
// bit 29:    hun.rom at segment 07h
// bit 30:    hun.rom at segment 32h
// bit 31:    -
// bit 32:    asmon15.rom at segments 04h..05h
// bit 33:    asmon15.rom at segments 05h..06h
// bit 34:    cyrus.rom at segment 40h
// bit 35:    ep-plus.rom at segment 05h
// bit 36:    ep-plus.rom at segment 07h
// bit 37:    epdos_z.rom at segments 06h..07h
// bit 38:    epdos_z.rom at segments 10h..11h
// bit 39:    epd19hft.rom at segments 06h..07h
// bit 40:    epd19hft.rom at segments 10h..11h
// bit 41:    epd19uk.rom at segments 06h..07h
// bit 42:    epd19uk.rom at segments 10h..11h
// bit 43:    fenas12.rom at segments 06h..07h
// bit 44:    fenas12.rom at segments 12h..13h
// bit 45:    forth.rom at segment 33h
// bit 46:    genmon.rom at segments 42h..43h
// bit 47:    heassekn.rom at segments 12h..13h
// bit 48:    heass10uk.rom at segments 12h..13h
// bit 49:    ide.rom at segment 30h
// bit 50:    ide.rom at segment 40h
// bit 51:    ide.rom at segment 42h
// bit 52:    iview.rom at segments 30h..31h
// bit 53:    iview.rom at segments 40h..41h
// bit 54:    lisp.rom at segment 32h
// bit 55:    pascal11.rom at segments 22h..23h
// bit 56:    pasians.rom at segments 40h..43h
// bit 57:    tpt.rom at segment 43h
// bit 58:    zt18hun.rom at segments 40h..41h
// bit 59:    zt18uk.rom at segments 40h..41h
// bit 60:    zx41.rom at segments 30h..31h
// bit 61:    -
// bit 62:    -
// bit 63:    -

#define EP_RAM_64K              uint64_t(0x00000001UL)
#define EP_RAM_128K             uint64_t(0x00000002UL)
#define EP_RAM_256K             uint64_t(0x00000004UL)
#define EP_RAM_320K             uint64_t(0x00000005UL)
#define EP_RAM_384K             uint64_t(0x00000006UL)
#define EP_RAM_640K             uint64_t(0x0000000AUL)
#define EP_RAM_1024K            uint64_t(0x00000010UL)
#define EP_RAM_2048K            uint64_t(0x00000020UL)

#define EP_ROM_EXOS20           uint64_t(0x00000040UL)
#define EP_ROM_EXOS21           uint64_t(0x00000080UL)
#define EP_ROM_EXOS22           uint64_t(0x00000100UL)
#define EP_ROM_EXOS23           uint64_t(0x00000200UL)
#define EP_ROM_EXOS231_ES       uint64_t(0x00000400UL)
#define EP_ROM_EXOS231_HU       uint64_t(0x00000800UL)
#define EP_ROM_EXOS231_UK       uint64_t(0x00001000UL)
#define EP_ROM_BASIC20          uint64_t(0x00002000UL)
#define EP_ROM_BASIC21_04       uint64_t(0x00004000UL)
#define EP_ROM_BASIC21_05       uint64_t(0x00008000UL)
#define EP_ROM_BASIC21_06       uint64_t(0x00010000UL)
#define EP_ROM_EXDOS10          uint64_t(0x00020000UL)
#define EP_ROM_EXDOS13          uint64_t(0x00040000UL)
#define EP_ROM_EXDOS13I_ES      uint64_t(0x00080000UL)
#define EP_ROM_EXDOS13I_HU      uint64_t(0x00100000UL)
#define EP_ROM_EPFILEIO         uint64_t(0x00200000UL)
#define EP_ROM_BRD_04           uint64_t(0x00400000UL)
#define EP_ROM_BRD_07           uint64_t(0x00800000UL)
#define EP_ROM_BRD_32           uint64_t(0x01000000UL)
#define EP_ROM_ESP_04           uint64_t(0x02000000UL)
#define EP_ROM_ESP_07           uint64_t(0x04000000UL)
#define EP_ROM_ESP_32           uint64_t(0x08000000UL)
#define EP_ROM_HUN_04           uint64_t(0x10000000UL)
#define EP_ROM_HUN_07           uint64_t(0x20000000UL)
#define EP_ROM_HUN_32           uint64_t(0x40000000UL)

#define EP_ROM_ASMON15_04       (uint64_t(0x00000001UL) << 32)
#define EP_ROM_ASMON15_05       (uint64_t(0x00000002UL) << 32)
#define EP_ROM_CYRUS            (uint64_t(0x00000004UL) << 32)
#define EP_ROM_EP_PLUS_05       (uint64_t(0x00000008UL) << 32)
#define EP_ROM_EP_PLUS_07       (uint64_t(0x00000010UL) << 32)
#define EP_ROM_EPDOS_Z_06       (uint64_t(0x00000020UL) << 32)
#define EP_ROM_EPDOS_Z_10       (uint64_t(0x00000040UL) << 32)
#define EP_ROM_EPD19HU_06       (uint64_t(0x00000080UL) << 32)
#define EP_ROM_EPD19HU_10       (uint64_t(0x00000100UL) << 32)
#define EP_ROM_EPD19UK_06       (uint64_t(0x00000200UL) << 32)
#define EP_ROM_EPD19UK_10       (uint64_t(0x00000400UL) << 32)
#define EP_ROM_FENAS12_06       (uint64_t(0x00000800UL) << 32)
#define EP_ROM_FENAS12_12       (uint64_t(0x00001000UL) << 32)
#define EP_ROM_FORTH            (uint64_t(0x00002000UL) << 32)
#define EP_ROM_GENMON           (uint64_t(0x00004000UL) << 32)
#define EP_ROM_HEASS10_HU       (uint64_t(0x00008000UL) << 32)
#define EP_ROM_HEASS10_UK       (uint64_t(0x00010000UL) << 32)
#define EP_ROM_IDE_30           (uint64_t(0x00020000UL) << 32)
#define EP_ROM_IDE_40           (uint64_t(0x00040000UL) << 32)
#define EP_ROM_IDE_42           (uint64_t(0x00080000UL) << 32)
#define EP_ROM_IVIEW_30         (uint64_t(0x00100000UL) << 32)
#define EP_ROM_IVIEW_40         (uint64_t(0x00200000UL) << 32)
#define EP_ROM_LISP             (uint64_t(0x00400000UL) << 32)
#define EP_ROM_PASCAL11         (uint64_t(0x00800000UL) << 32)
#define EP_ROM_PASIANS          (uint64_t(0x01000000UL) << 32)
#define EP_ROM_TPT_43           (uint64_t(0x02000000UL) << 32)
#define EP_ROM_ZT18_HU          (uint64_t(0x04000000UL) << 32)
#define EP_ROM_ZT18_UK          (uint64_t(0x08000000UL) << 32)
#define EP_ROM_ZX41             (uint64_t(0x10000000UL) << 32)

static const char *romFileNames[58] = {
  "exos20.rom",
  "exos21.rom",
  "exos22.rom",
  "exos23.rom",
  "exos231esp.rom",
  "exos231hun.rom",
  "exos231uk.rom",
  "basic20.rom",
  "basic21.rom",
  "basic21.rom",
  "basic21.rom",
  "exdos10.rom",
  "exdos13.rom",
  "exdos13isdos10esp.rom",
  "exdos13isdos10hun.rom",
  "epfileio.rom",
  "brd.rom",
  "brd.rom",
  "brd.rom",
  "esp.rom",
  "esp.rom",
  "esp.rom",
  "hun.rom",
  "hun.rom",
  "hun.rom",
  (char *) 0,
  "asmon15.rom",
  "asmon15.rom",
  "cyrus.rom",
  "ep-plus.rom",
  "ep-plus.rom",
  "epdos_z.rom",
  "epdos_z.rom",
  "epd19hft.rom",
  "epd19hft.rom",
  "epd19uk.rom",
  "epd19uk.rom",
  "fenas12.rom",
  "fenas12.rom",
  "forth.rom",
  "genmon.rom",
  "heassekn.rom",
  "heass10uk.rom",
  "ide.rom",
  "ide.rom",
  "ide.rom",
  "iview.rom",
  "iview.rom",
  "lisp.rom",
  "pascal11.rom",
  "pasians.rom",
  "tpt.rom",
  "zt18hun.rom",
  "zt18uk.rom",
  "zx41.rom",
  (char *) 0,
  (char *) 0,
  (char *) 0
};

static const unsigned long romFileSegments[58] = {
  0x0001FFFFUL,         // exos20.rom
  0x0001FFFFUL,         // exos21.rom
  0x00010203UL,         // exos22.rom
  0x00010203UL,         // exos23.rom
  0x00010203UL,         // exos231esp.rom
  0x00010203UL,         // exos231hun.rom
  0x00010203UL,         // exos231uk.rom
  0x04FFFFFFUL,         // basic20.rom
  0x04FFFFFFUL,         // basic21.rom
  0x05FFFFFFUL,         // basic21.rom
  0x06FFFFFFUL,         // basic21.rom
  0x20FFFFFFUL,         // exdos10.rom
  0x2021FFFFUL,         // exdos13.rom
  0x2021FFFFUL,         // exdos13isdos10esp.rom
  0x2021FFFFUL,         // exdos13isdos10hun.rom
  0x10FFFFFFUL,         // epfileio.rom
  0x04FFFFFFUL,         // brd.rom
  0x07FFFFFFUL,         // brd.rom
  0x32FFFFFFUL,         // brd.rom
  0x04FFFFFFUL,         // esp.rom
  0x07FFFFFFUL,         // esp.rom
  0x32FFFFFFUL,         // esp.rom
  0x04FFFFFFUL,         // hun.rom
  0x07FFFFFFUL,         // hun.rom
  0x32FFFFFFUL,         // hun.rom
  0xFFFFFFFFUL,         // -
  0x0405FFFFUL,         // asmon15.rom
  0x0506FFFFUL,         // asmon15.rom
  0x40FFFFFFUL,         // cyrus.rom
  0x05FFFFFFUL,         // ep-plus.rom
  0x07FFFFFFUL,         // ep-plus.rom
  0x0607FFFFUL,         // epdos_z.rom
  0x1011FFFFUL,         // epdos_z.rom
  0x0607FFFFUL,         // epd19hft.rom
  0x1011FFFFUL,         // epd19hft.rom
  0x0607FFFFUL,         // epd19uk.rom
  0x1011FFFFUL,         // epd19uk.rom
  0x0607FFFFUL,         // fenas12.rom
  0x1213FFFFUL,         // fenas12.rom
  0x33FFFFFFUL,         // forth.rom
  0x4243FFFFUL,         // genmon.rom
  0x1213FFFFUL,         // heassekn.rom
  0x1213FFFFUL,         // heass10uk.rom
  0x30FFFFFFUL,         // ide.rom
  0x40FFFFFFUL,         // ide.rom
  0x42FFFFFFUL,         // ide.rom
  0x3031FFFFUL,         // iview.rom
  0x4041FFFFUL,         // iview.rom
  0x32FFFFFFUL,         // lisp.rom
  0x2223FFFFUL,         // pascal11.rom
  0x40414243UL,         // pasians.rom
  0x43FFFFFFUL,         // tpt.rom
  0x4041FFFFUL,         // zt18hun.rom
  0x4041FFFFUL,         // zt18uk.rom
  0x3031FFFFUL,         // zx41.rom
  0xFFFFFFFFUL,         // -
  0xFFFFFFFFUL,         // -
  0xFFFFFFFFUL          // -
};

static const char *machineConfigFileNames[] = {
  "ep128brd/EP2048k_EXOS231_EXDOS_utils.cfg",           //  0
  "ep128brd/EP_128k_EXDOS.cfg",                         //  1
  "ep128brd/EP_128k_EXDOS_FileIO.cfg",                  //  2
  "ep128brd/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg", //  3
  "ep128brd/EP_128k_EXDOS_TASMON.cfg",                  //  4
  "ep128brd/EP_128k_Tape.cfg",                          //  5
  "ep128brd/EP_128k_Tape_FileIO.cfg",                   //  6
  "ep128brd/EP_128k_Tape_FileIO_TASMON.cfg",            //  7
  "ep128brd/EP_128k_Tape_TASMON.cfg",                   //  8
  "ep128brd/EP_640k_EXOS231_EXDOS.cfg",                 //  9
  "ep128brd/EP_640k_EXOS231_EXDOS_utils.cfg",           // 10
  "ep128brd/EP_640k_EXOS231_IDE_utils.cfg",             // 11
  "ep128esp/EP2048k_EXOS231_EXDOS_utils.cfg",           // 12
  "ep128esp/EP_128k_EXDOS.cfg",                         // 13
  "ep128esp/EP_128k_EXDOS_FileIO.cfg",                  // 14
  "ep128esp/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg", // 15
  "ep128esp/EP_128k_EXDOS_TASMON.cfg",                  // 16
  "ep128esp/EP_128k_Tape.cfg",                          // 17
  "ep128esp/EP_128k_Tape_FileIO.cfg",                   // 18
  "ep128esp/EP_128k_Tape_FileIO_TASMON.cfg",            // 19
  "ep128esp/EP_128k_Tape_TASMON.cfg",                   // 20
  "ep128esp/EP_640k_EXOS231_EXDOS.cfg",                 // 21
  "ep128esp/EP_640k_EXOS231_EXDOS_utils.cfg",           // 22
  "ep128esp/EP_640k_EXOS231_IDE_utils.cfg",             // 23
  "ep128hun/EP2048k_EXOS231_EXDOS_utils.cfg",           // 24
  "ep128hun/EP_128k_EXDOS.cfg",                         // 25
  "ep128hun/EP_128k_EXDOS_FileIO.cfg",                  // 26
  "ep128hun/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg", // 27
  "ep128hun/EP_128k_EXDOS_TASMON.cfg",                  // 28
  "ep128hun/EP_128k_Tape.cfg",                          // 29
  "ep128hun/EP_128k_Tape_FileIO.cfg",                   // 30
  "ep128hun/EP_128k_Tape_FileIO_TASMON.cfg",            // 31
  "ep128hun/EP_128k_Tape_TASMON.cfg",                   // 32
  "ep128hun/EP_640k_EXOS22_EXDOS.cfg",                  // 33
  "ep128hun/EP_640k_EXOS23_EXDOS.cfg",                  // 34
  "ep128hun/EP_640k_EXOS231_EXDOS.cfg",                 // 35
  "ep128hun/EP_640k_EXOS231_EXDOS_utils.cfg",           // 36
  "ep128hun/EP_640k_EXOS231_IDE_utils.cfg",             // 37
  "ep128uk/EP2048k_EXOS231_EXDOS_utils.cfg",            // 38
  "ep128uk/EP_128k_EXDOS.cfg",                          // 39
  "ep128uk/EP_128k_EXDOS_EP-PLUS.cfg",                  // 40
  "ep128uk/EP_128k_EXDOS_EP-PLUS_TASMON.cfg",           // 41
  "ep128uk/EP_128k_EXDOS_FileIO.cfg",                   // 42
  "ep128uk/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg",  // 43
  "ep128uk/EP_128k_EXDOS_NoCartridge.cfg",              // 44
  "ep128uk/EP_128k_EXDOS_TASMON.cfg",                   // 45
  "ep128uk/EP_128k_Tape.cfg",                           // 46
  "ep128uk/EP_128k_Tape_EP-PLUS.cfg",                   // 47
  "ep128uk/EP_128k_Tape_FileIO.cfg",                    // 48
  "ep128uk/EP_128k_Tape_FileIO_TASMON.cfg",             // 49
  "ep128uk/EP_128k_Tape_NoCartridge.cfg",               // 50
  "ep128uk/EP_128k_Tape_NoCartridge_FileIO.cfg",        // 51
  "ep128uk/EP_128k_Tape_TASMON.cfg",                    // 52
  "ep128uk/EP_640k_EXOS231_EXDOS.cfg",                  // 53
  "ep128uk/EP_640k_EXOS231_EXDOS_utils.cfg",            // 54
  "ep128uk/EP_640k_EXOS231_IDE_utils.cfg",              // 55
  "ep64/EP_64k_EXDOS.cfg",                              // 56
  "ep64/EP_64k_EXDOS_FileIO.cfg",                       // 57
  "ep64/EP_64k_EXDOS_NoCartridge.cfg",                  // 58
  "ep64/EP_64k_EXDOS_TASMON.cfg",                       // 59
  "ep64/EP_64k_Tape.cfg",                               // 60
  "ep64/EP_64k_Tape_FileIO.cfg",                        // 61
  "ep64/EP_64k_Tape_NoCartridge.cfg",                   // 62
  "ep64/EP_64k_Tape_TASMON.cfg",                        // 63
  "zx/ZX_16k.cfg",                                      // +0
  "zx/ZX_16k_FileIO.cfg",                               // +1
  "zx/ZX_48k.cfg",                                      // +2
  "zx/ZX_48k_FileIO.cfg",                               // +3
  "zx/ZX_128k.cfg",                                     // +4
  "zx/ZX_128k_FileIO.cfg",                              // +5
  "cpc/CPC_64k.cfg",                                    // +6
  "cpc/CPC_128k.cfg",                                   // +7
  "cpc/CPC_576k.cfg"                                    // +8
};

static const uint64_t machineConfigs[] = {
  // ep128brd/EP2048k_EXOS231_EXDOS_utils.cfg
  EP_RAM_2048K | EP_ROM_EXOS231_UK | EP_ROM_ASMON15_04 | EP_ROM_FENAS12_06
  | EP_ROM_EPDOS_Z_10 | EP_ROM_HEASS10_UK | EP_ROM_EXDOS13 | EP_ROM_PASCAL11
  | EP_ROM_ZX41 | EP_ROM_BRD_32 | EP_ROM_FORTH | EP_ROM_ZT18_UK
  | EP_ROM_GENMON,
  // ep128brd/EP_128k_EXDOS.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BRD_04 | EP_ROM_BASIC21_05
  | EP_ROM_EXDOS13,
  // ep128brd/EP_128k_EXDOS_FileIO.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BRD_04 | EP_ROM_BASIC21_05
  | EP_ROM_EPFILEIO | EP_ROM_EXDOS13,
  // ep128brd/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BRD_04 | EP_ROM_BASIC21_05
  | EP_ROM_EPFILEIO | EP_ROM_EXDOS13 | EP_ROM_ZX41,
  // ep128brd/EP_128k_EXDOS_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_BRD_07 | EP_ROM_EXDOS13,
  // ep128brd/EP_128k_Tape.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BRD_04 | EP_ROM_BASIC21_05,
  // ep128brd/EP_128k_Tape_FileIO.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BRD_04 | EP_ROM_BASIC21_05
  | EP_ROM_EPFILEIO,
  // ep128brd/EP_128k_Tape_FileIO_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_BRD_07 | EP_ROM_EPFILEIO,
  // ep128brd/EP_128k_Tape_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_BRD_07,
  // ep128brd/EP_640k_EXOS231_EXDOS.cfg
  EP_RAM_640K | EP_ROM_EXOS231_UK | EP_ROM_BRD_04 | EP_ROM_EXDOS13,
  // ep128brd/EP_640k_EXOS231_EXDOS_utils.cfg
  EP_RAM_640K | EP_ROM_EXOS231_UK | EP_ROM_ASMON15_04 | EP_ROM_EPDOS_Z_06
  | EP_ROM_EPFILEIO | EP_ROM_FENAS12_12 | EP_ROM_EXDOS13 | EP_ROM_IVIEW_30
  | EP_ROM_BRD_32 | EP_ROM_ZT18_UK | EP_ROM_TPT_43,
  // ep128brd/EP_640k_EXOS231_IDE_utils.cfg
  EP_RAM_640K | EP_ROM_EXOS231_UK | EP_ROM_ASMON15_04 | EP_ROM_EPD19UK_06
  | EP_ROM_EPFILEIO | EP_ROM_HEASS10_UK | EP_ROM_EXDOS13 | EP_ROM_IDE_42
  | EP_ROM_IVIEW_30 | EP_ROM_BRD_32 | EP_ROM_ZT18_UK | EP_ROM_TPT_43,
  // ep128esp/EP2048k_EXOS231_EXDOS_utils.cfg
  EP_RAM_2048K | EP_ROM_EXOS231_ES | EP_ROM_ASMON15_04 | EP_ROM_FENAS12_06
  | EP_ROM_EPDOS_Z_10 | EP_ROM_HEASS10_UK | EP_ROM_EXDOS13I_ES
  | EP_ROM_PASCAL11 | EP_ROM_ZX41 | EP_ROM_ESP_32 | EP_ROM_FORTH
  | EP_ROM_ZT18_UK | EP_ROM_GENMON,
  // ep128esp/EP_128k_EXDOS.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ESP_04 | EP_ROM_BASIC21_05
  | EP_ROM_EXDOS13I_ES,
  // ep128esp/EP_128k_EXDOS_FileIO.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ESP_04 | EP_ROM_BASIC21_05
  | EP_ROM_EPFILEIO | EP_ROM_EXDOS13I_ES,
  // ep128esp/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ESP_04 | EP_ROM_BASIC21_05
  | EP_ROM_EPFILEIO | EP_ROM_EXDOS13I_ES | EP_ROM_ZX41,
  // ep128esp/EP_128k_EXDOS_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_ESP_07 | EP_ROM_EXDOS13I_ES,
  // ep128esp/EP_128k_Tape.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ESP_04 | EP_ROM_BASIC21_05,
  // ep128esp/EP_128k_Tape_FileIO.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ESP_04 | EP_ROM_BASIC21_05
  | EP_ROM_EPFILEIO,
  // ep128esp/EP_128k_Tape_FileIO_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_ESP_07 | EP_ROM_EPFILEIO,
  // ep128esp/EP_128k_Tape_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_ESP_07,
  // ep128esp/EP_640k_EXOS231_EXDOS.cfg
  EP_RAM_640K | EP_ROM_EXOS231_ES | EP_ROM_ESP_04 | EP_ROM_EXDOS13I_ES,
  // ep128esp/EP_640k_EXOS231_EXDOS_utils.cfg
  EP_RAM_640K | EP_ROM_EXOS231_ES | EP_ROM_ASMON15_04 | EP_ROM_EPDOS_Z_06
  | EP_ROM_EPFILEIO | EP_ROM_FENAS12_12 | EP_ROM_EXDOS13I_ES | EP_ROM_IVIEW_30
  | EP_ROM_ESP_32 | EP_ROM_ZT18_UK | EP_ROM_TPT_43,
  // ep128esp/EP_640k_EXOS231_IDE_utils.cfg
  EP_RAM_640K | EP_ROM_EXOS231_ES | EP_ROM_ASMON15_04 | EP_ROM_EPD19UK_06
  | EP_ROM_EPFILEIO | EP_ROM_HEASS10_UK | EP_ROM_EXDOS13I_ES | EP_ROM_IDE_42
  | EP_ROM_IVIEW_30 | EP_ROM_ESP_32 | EP_ROM_ZT18_UK | EP_ROM_TPT_43,
  // ep128hun/EP2048k_EXOS231_EXDOS_utils.cfg
  EP_RAM_2048K | EP_ROM_EXOS231_HU | EP_ROM_ASMON15_04 | EP_ROM_FENAS12_06
  | EP_ROM_EPDOS_Z_10 | EP_ROM_HEASS10_HU | EP_ROM_EXDOS13I_HU
  | EP_ROM_PASCAL11 | EP_ROM_ZX41 | EP_ROM_HUN_32 | EP_ROM_FORTH
  | EP_ROM_ZT18_HU | EP_ROM_GENMON,
  // ep128hun/EP_128k_EXDOS.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_HUN_04 | EP_ROM_BASIC21_05
  | EP_ROM_EXDOS13I_HU,
  // ep128hun/EP_128k_EXDOS_FileIO.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_HUN_04 | EP_ROM_BASIC21_05
  | EP_ROM_EPFILEIO | EP_ROM_EXDOS13I_HU,
  // ep128hun/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_HUN_04 | EP_ROM_BASIC21_05
  | EP_ROM_EPFILEIO | EP_ROM_EXDOS13I_HU | EP_ROM_ZX41,
  // ep128hun/EP_128k_EXDOS_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_HUN_07 | EP_ROM_EXDOS13I_HU,
  // ep128hun/EP_128k_Tape.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_HUN_04 | EP_ROM_BASIC21_05,
  // ep128hun/EP_128k_Tape_FileIO.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_HUN_04 | EP_ROM_BASIC21_05
  | EP_ROM_EPFILEIO,
  // ep128hun/EP_128k_Tape_FileIO_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_HUN_07 | EP_ROM_EPFILEIO,
  // ep128hun/EP_128k_Tape_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_HUN_07,
  // ep128hun/EP_640k_EXOS22_EXDOS.cfg
  EP_RAM_640K | EP_ROM_EXOS22 | EP_ROM_HUN_04 | EP_ROM_EXDOS13I_HU,
  // ep128hun/EP_640k_EXOS23_EXDOS.cfg
  EP_RAM_640K | EP_ROM_EXOS23 | EP_ROM_HUN_04 | EP_ROM_EXDOS13I_HU,
  // ep128hun/EP_640k_EXOS231_EXDOS.cfg
  EP_RAM_640K | EP_ROM_EXOS231_HU | EP_ROM_HUN_04 | EP_ROM_EXDOS13I_HU,
  // ep128hun/EP_640k_EXOS231_EXDOS_utils.cfg
  EP_RAM_640K | EP_ROM_EXOS231_HU | EP_ROM_ASMON15_04 | EP_ROM_EPDOS_Z_06
  | EP_ROM_EPFILEIO | EP_ROM_FENAS12_12 | EP_ROM_EXDOS13I_HU | EP_ROM_IVIEW_30
  | EP_ROM_HUN_32 | EP_ROM_ZT18_HU | EP_ROM_TPT_43,
  // ep128hun/EP_640k_EXOS231_IDE_utils.cfg
  EP_RAM_640K | EP_ROM_EXOS231_HU | EP_ROM_ASMON15_04 | EP_ROM_EPD19HU_06
  | EP_ROM_EPFILEIO | EP_ROM_HEASS10_HU | EP_ROM_EXDOS13I_HU | EP_ROM_IDE_42
  | EP_ROM_IVIEW_30 | EP_ROM_HUN_32 | EP_ROM_ZT18_HU | EP_ROM_TPT_43,
  // ep128uk/EP2048k_EXOS231_EXDOS_utils.cfg
  EP_RAM_2048K | EP_ROM_EXOS231_UK | EP_ROM_ASMON15_04 | EP_ROM_FENAS12_06
  | EP_ROM_EPDOS_Z_10 | EP_ROM_HEASS10_UK | EP_ROM_EXDOS13 | EP_ROM_PASCAL11
  | EP_ROM_ZX41 | EP_ROM_LISP | EP_ROM_FORTH | EP_ROM_ZT18_UK | EP_ROM_GENMON,
  // ep128uk/EP_128k_EXDOS.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EXDOS13,
  // ep128uk/EP_128k_EXDOS_EP-PLUS.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EP_PLUS_05
  | EP_ROM_EXDOS13,
  // ep128uk/EP_128k_EXDOS_EP-PLUS_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_EP_PLUS_07 | EP_ROM_EXDOS13,
  // ep128uk/EP_128k_EXDOS_FileIO.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EPFILEIO
  | EP_ROM_EXDOS13,
  // ep128uk/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EPFILEIO
  | EP_ROM_EXDOS13 | EP_ROM_ZX41,
  // ep128uk/EP_128k_EXDOS_NoCartridge.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_EXDOS13,
  // ep128uk/EP_128k_EXDOS_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_EXDOS13,
  // ep128uk/EP_128k_Tape.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04,
  // ep128uk/EP_128k_Tape_EP-PLUS.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EP_PLUS_05,
  // ep128uk/EP_128k_Tape_FileIO.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EPFILEIO,
  // ep128uk/EP_128k_Tape_FileIO_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  | EP_ROM_EPFILEIO,
  // ep128uk/EP_128k_Tape_NoCartridge.cfg
  EP_RAM_128K | EP_ROM_EXOS21,
  // ep128uk/EP_128k_Tape_NoCartridge_FileIO.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_EPFILEIO,
  // ep128uk/EP_128k_Tape_TASMON.cfg
  EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06,
  // ep128uk/EP_640k_EXOS231_EXDOS.cfg
  EP_RAM_640K | EP_ROM_EXOS231_UK | EP_ROM_EXDOS13,
  // ep128uk/EP_640k_EXOS231_EXDOS_utils.cfg
  EP_RAM_640K | EP_ROM_EXOS231_UK | EP_ROM_ASMON15_04 | EP_ROM_EPDOS_Z_06
  | EP_ROM_EPFILEIO | EP_ROM_FENAS12_12 | EP_ROM_EXDOS13 | EP_ROM_IVIEW_30
  | EP_ROM_ZT18_UK | EP_ROM_TPT_43,
  // ep128uk/EP_640k_EXOS231_IDE_utils.cfg
  EP_RAM_640K | EP_ROM_EXOS231_UK | EP_ROM_ASMON15_04 | EP_ROM_EPD19UK_06
  | EP_ROM_EPFILEIO | EP_ROM_HEASS10_UK | EP_ROM_EXDOS13 | EP_ROM_IDE_42
  | EP_ROM_IVIEW_30 | EP_ROM_ZT18_UK | EP_ROM_TPT_43,
  // ep64/EP_64k_EXDOS.cfg
  EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_BASIC20 | EP_ROM_EXDOS10,
  // ep64/EP_64k_EXDOS_FileIO.cfg
  EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_BASIC20 | EP_ROM_EPFILEIO
  | EP_ROM_EXDOS10,
  // ep64/EP_64k_EXDOS_NoCartridge.cfg
  EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_EXDOS10,
  // ep64/EP_64k_EXDOS_TASMON.cfg
  EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_BASIC20 | EP_ROM_ASMON15_05
  | EP_ROM_EXDOS10,
  // ep64/EP_64k_Tape.cfg
  EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_BASIC20,
  // ep64/EP_64k_Tape_FileIO.cfg
  EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_BASIC20 | EP_ROM_EPFILEIO,
  // ep64/EP_64k_Tape_NoCartridge.cfg
  EP_RAM_64K | EP_ROM_EXOS20,
  // ep64/EP_64k_Tape_TASMON.cfg
  EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_BASIC20 | EP_ROM_ASMON15_05
};

Ep128EmuMachineConfiguration::Ep128EmuMachineConfiguration(
    Ep128Emu::ConfigurationDB& config, int n, const std::string& romDirectory)
{
  int     machineType = 0;              // 0: EP, 1: ZX, 2: CPC
  if (n >= int(sizeof(machineConfigs) / sizeof(uint64_t))) {
    n = n - int(sizeof(machineConfigs) / sizeof(uint64_t));
    if (n < 6) {
      machineType = 1;
      try {
        config["display.quality"] = 1;
      }
      catch (Ep128Emu::Exception) {
      }
      if (n < 4) {                      // Spectrum 16 or 48
        vm.cpuClockFrequency = 3500000U;
        vm.videoClockFrequency = 875000U;
        vm.soundClockFrequency = 218750U;
        memory.ram.size = (n < 2 ? 16 : 48);
        memory.rom[0].file = romDirectory + "zx48.rom";
      }
      else {                            // Spectrum 128
        vm.cpuClockFrequency = 3546896U;
        vm.videoClockFrequency = 886724U;
        vm.soundClockFrequency = 221681U;
        memory.ram.size = 128;
        memory.rom[0].file = romDirectory + "zx128.rom";
        memory.rom[1].file = romDirectory + "zx128.rom";
        memory.rom[1].offset = 16384;
      }
      vm.enableMemoryTimingEmulation = true;
      vm.enableFileIO = bool(n & 1);
    }
    else {
      n = n - 6;
      machineType = 2;
      try {
        config["sound.volume"] = 0.5;
      }
      catch (Ep128Emu::Exception) {
      }
      vm.cpuClockFrequency = 4000000U;
      vm.videoClockFrequency = 1000000U;
      vm.soundClockFrequency = 125000U;
      memory.ram.size = (n == 0 ? 64 : (n == 1 ? 128 : 576));
      if (n == 0) {                     // CPC 64K
        memory.rom[0].file = romDirectory + "cpc464.rom";
        memory.rom[0].offset = 16384;
        memory.rom[16].file = romDirectory + "cpc464.rom";
      }
      else {                            // CPC 128K or 576K
        memory.rom[0].file = romDirectory + "cpc6128.rom";
        memory.rom[0].offset = 16384;
        memory.rom[16].file = romDirectory + "cpc6128.rom";
      }
      vm.enableMemoryTimingEmulation = true;
      vm.enableFileIO = false;
    }
  }
  else {                                // Enterprise
    vm.cpuClockFrequency = 4000000U;
    vm.videoClockFrequency = 889846U;
    vm.soundClockFrequency = 500000U;
    vm.enableMemoryTimingEmulation = true;
    vm.enableFileIO = bool(machineConfigs[n] & EP_ROM_EPFILEIO);
    memory.ram.size = int(machineConfigs[n] & uint64_t(0x3F)) << 6;
    for (int i = 0; i < 58; i++) {
      if (machineConfigs[n] & (uint64_t(1) << (i + 6))) {
        unsigned long tmp = romFileSegments[i];
        for (int j = 0; j < 4 && tmp < 0x80000000UL; j++) {
          memory.rom[(tmp >> 24) & 0xFFUL].file =
              romDirectory + romFileNames[i];
          memory.rom[(tmp >> 24) & 0xFFUL].offset = j << 14;
          tmp = (tmp & 0x00FFFFFFUL) << 8;
        }
      }
    }
  }
  memory.configFile = "";
  config.createKey("vm.cpuClockFrequency", vm.cpuClockFrequency);
  config.createKey("vm.videoClockFrequency", vm.videoClockFrequency);
  config.createKey("vm.soundClockFrequency", vm.soundClockFrequency);
  config.createKey("vm.enableMemoryTimingEmulation",
                   vm.enableMemoryTimingEmulation);
  config.createKey("vm.enableFileIO", vm.enableFileIO);
  config.createKey("memory.ram.size", memory.ram.size);
  config.createKey("memory.rom.00.file", memory.rom[0x00].file);
  config.createKey("memory.rom.00.offset", memory.rom[0x00].offset);
  config.createKey("memory.rom.01.file", memory.rom[0x01].file);
  config.createKey("memory.rom.01.offset", memory.rom[0x01].offset);
  if (machineType != 1) {
    config.createKey("memory.rom.02.file", memory.rom[0x02].file);
    config.createKey("memory.rom.02.offset", memory.rom[0x02].offset);
    config.createKey("memory.rom.03.file", memory.rom[0x03].file);
    config.createKey("memory.rom.03.offset", memory.rom[0x03].offset);
    config.createKey("memory.rom.04.file", memory.rom[0x04].file);
    config.createKey("memory.rom.04.offset", memory.rom[0x04].offset);
    config.createKey("memory.rom.05.file", memory.rom[0x05].file);
    config.createKey("memory.rom.05.offset", memory.rom[0x05].offset);
    config.createKey("memory.rom.06.file", memory.rom[0x06].file);
    config.createKey("memory.rom.06.offset", memory.rom[0x06].offset);
    config.createKey("memory.rom.07.file", memory.rom[0x07].file);
    config.createKey("memory.rom.07.offset", memory.rom[0x07].offset);
    config.createKey("memory.rom.10.file", memory.rom[0x10].file);
    config.createKey("memory.rom.10.offset", memory.rom[0x10].offset);
  }
  if (machineType == 0) {
    config.createKey("memory.rom.11.file", memory.rom[0x11].file);
    config.createKey("memory.rom.11.offset", memory.rom[0x11].offset);
    config.createKey("memory.rom.12.file", memory.rom[0x12].file);
    config.createKey("memory.rom.12.offset", memory.rom[0x12].offset);
    config.createKey("memory.rom.13.file", memory.rom[0x13].file);
    config.createKey("memory.rom.13.offset", memory.rom[0x13].offset);
    config.createKey("memory.rom.20.file", memory.rom[0x20].file);
    config.createKey("memory.rom.20.offset", memory.rom[0x20].offset);
    config.createKey("memory.rom.21.file", memory.rom[0x21].file);
    config.createKey("memory.rom.21.offset", memory.rom[0x21].offset);
    config.createKey("memory.rom.22.file", memory.rom[0x22].file);
    config.createKey("memory.rom.22.offset", memory.rom[0x22].offset);
    config.createKey("memory.rom.23.file", memory.rom[0x23].file);
    config.createKey("memory.rom.23.offset", memory.rom[0x23].offset);
    config.createKey("memory.rom.30.file", memory.rom[0x30].file);
    config.createKey("memory.rom.30.offset", memory.rom[0x30].offset);
    config.createKey("memory.rom.31.file", memory.rom[0x31].file);
    config.createKey("memory.rom.31.offset", memory.rom[0x31].offset);
    config.createKey("memory.rom.32.file", memory.rom[0x32].file);
    config.createKey("memory.rom.32.offset", memory.rom[0x32].offset);
    config.createKey("memory.rom.33.file", memory.rom[0x33].file);
    config.createKey("memory.rom.33.offset", memory.rom[0x33].offset);
    config.createKey("memory.rom.40.file", memory.rom[0x40].file);
    config.createKey("memory.rom.40.offset", memory.rom[0x40].offset);
    config.createKey("memory.rom.41.file", memory.rom[0x41].file);
    config.createKey("memory.rom.41.offset", memory.rom[0x41].offset);
    config.createKey("memory.rom.42.file", memory.rom[0x42].file);
    config.createKey("memory.rom.42.offset", memory.rom[0x42].offset);
    config.createKey("memory.rom.43.file", memory.rom[0x43].file);
    config.createKey("memory.rom.43.offset", memory.rom[0x43].offset);
    config.createKey("memory.configFile", memory.configFile);
  }
}

class Ep128EmuDisplaySndConfiguration {
 private:
    struct {
      int         quality;
    } display;
    struct {
      bool        highQuality;
      double      latency;
      int         hwPeriods;
      double      volume;
    } sound;
 public:
  Ep128EmuDisplaySndConfiguration(Ep128Emu::ConfigurationDB& config)
  {
    display.quality = 2;
    sound.highQuality = true;
    sound.latency = 0.07;
    sound.hwPeriods = 16;
    sound.volume = 0.7071;
    config.createKey("display.quality", display.quality);
    config.createKey("sound.highQuality", sound.highQuality);
    config.createKey("sound.latency", sound.latency);
    config.createKey("sound.hwPeriods", sound.hwPeriods);
    config.createKey("sound.volume", sound.volume);
  }
};

class Ep128EmuGUIConfiguration {
 private:
  struct {
    std::string snapshotDirectory;
    std::string demoDirectory;
    std::string soundFileDirectory;
    std::string configDirectory;
    std::string loadFileDirectory;
    std::string tapeImageDirectory;
    std::string diskImageDirectory;
    std::string romImageDirectory;
    std::string debuggerDirectory;
    std::string screenshotDirectory;
  } gui;
 public:
  Ep128EmuGUIConfiguration(Ep128Emu::ConfigurationDB& config,
                           const std::string& installDirectory)
  {
    gui.snapshotDirectory = installDirectory + "snapshot";
    gui.demoDirectory = installDirectory + "demo";
    gui.soundFileDirectory = ".";
    gui.configDirectory = installDirectory + "config";
    gui.loadFileDirectory = ".";
    gui.tapeImageDirectory = installDirectory + "tape";
    gui.diskImageDirectory = installDirectory + "disk";
    gui.romImageDirectory = installDirectory + "roms";
    gui.debuggerDirectory = ".";
    gui.screenshotDirectory = ".";
    config.createKey("gui.snapshotDirectory", gui.snapshotDirectory);
    config.createKey("gui.demoDirectory", gui.demoDirectory);
    config.createKey("gui.soundFileDirectory", gui.soundFileDirectory);
    config.createKey("gui.configDirectory", gui.configDirectory);
    config.createKey("gui.loadFileDirectory", gui.loadFileDirectory);
    config.createKey("gui.tapeImageDirectory", gui.tapeImageDirectory);
    config.createKey("gui.diskImageDirectory", gui.diskImageDirectory);
    config.createKey("gui.romImageDirectory", gui.romImageDirectory);
    config.createKey("gui.debuggerDirectory", gui.debuggerDirectory);
    config.createKey("gui.screenshotDirectory", gui.screenshotDirectory);
  }
};

// ----------------------------------------------------------------------------

class Decompressor {
 private:
  unsigned int  lengthDecodeTable[8 * 2];
  unsigned int  offs1DecodeTable[4 * 2];
  unsigned int  offs2DecodeTable[8 * 2];
  unsigned int  offs3DecodeTable[32 * 2];
  size_t        offs3PrefixSize;
  unsigned char shiftRegister;
  int           shiftRegisterCnt;
  const unsigned char *inputBuffer;
  size_t        inputBufferSize;
  size_t        inputBufferPosition;
  // --------
  unsigned int readBits(size_t nBits);
  unsigned char readLiteralByte();
  // returns LZ match length (1..65535), or zero for literal byte,
  // or length + 0x80000000 for literal sequence
  unsigned int readMatchLength();
  unsigned int readLZMatchParameter(unsigned char slotNum,
                                    const unsigned int *decodeTable);
  void readDecodeTables();
  bool decompressDataBlock(std::vector< unsigned char >& buf,
                           std::vector< bool >& bytesUsed,
                           unsigned int& startAddr);
 public:
  Decompressor();
  virtual ~Decompressor();
  virtual void decompressData(
      std::vector< unsigned char >& outBuf,
      const std::vector< unsigned char >& inBuf);
};

unsigned int Decompressor::readBits(size_t nBits)
{
  unsigned int  retval = 0U;
  for (size_t i = 0; i < nBits; i++) {
    if (shiftRegisterCnt < 1) {
      if (inputBufferPosition >= inputBufferSize)
        throw Ep128Emu::Exception("unexpected end of compressed data");
      shiftRegister = inputBuffer[inputBufferPosition];
      shiftRegisterCnt = 8;
      inputBufferPosition++;
    }
    retval = (retval << 1) | (unsigned int) ((shiftRegister >> 7) & 0x01);
    shiftRegister = (shiftRegister & 0x7F) << 1;
    shiftRegisterCnt--;
  }
  return retval;
}

unsigned char Decompressor::readLiteralByte()
{
  if (inputBufferPosition >= inputBufferSize)
    throw Ep128Emu::Exception("unexpected end of compressed data");
  unsigned char retval = inputBuffer[inputBufferPosition];
  inputBufferPosition++;
  return retval;
}

unsigned int Decompressor::readMatchLength()
{
  unsigned int  slotNum = 0U;
  do {
    if (readBits(1) == 0U)
      break;
    slotNum++;
  } while (slotNum < 9U);
  if (slotNum == 0U)                  // literal byte
    return 0U;
  if (slotNum == 9U)                  // literal sequence
    return (readBits(8) + 0x80000011U);
  return (readLZMatchParameter((unsigned char) (slotNum - 1U),
                               &(lengthDecodeTable[0])) + 1U);
}

unsigned int Decompressor::readLZMatchParameter(
    unsigned char slotNum, const unsigned int *decodeTable)
{
  unsigned int  retval = decodeTable[int(slotNum) * 2 + 1];
  retval += readBits(size_t(decodeTable[int(slotNum) * 2 + 0]));
  return retval;
}

void Decompressor::readDecodeTables()
{
  unsigned int  tmp = 0U;
  unsigned int  *tablePtr = &(lengthDecodeTable[0]);
  offs3PrefixSize = size_t(readBits(2)) + 2;
  size_t  offs3NumSlots = size_t(1) << offs3PrefixSize;
  for (size_t i = 0; i < (8 + 4 + 8 + offs3NumSlots); i++) {
    if (i == 8) {
      tmp = 0U;
      tablePtr = &(offs1DecodeTable[0]);
    }
    else if (i == (8 + 4)) {
      tmp = 0U;
      tablePtr = &(offs2DecodeTable[0]);
    }
    else if (i == (8 + 4 + 8)) {
      tmp = 0U;
      tablePtr = &(offs3DecodeTable[0]);
    }
    tablePtr[0] = readBits(4);
    tablePtr[1] = tmp;
    tmp = tmp + (1U << tablePtr[0]);
    tablePtr = tablePtr + 2;
  }
}

bool Decompressor::decompressDataBlock(std::vector< unsigned char >& buf,
                                       std::vector< bool >& bytesUsed,
                                       unsigned int& startAddr)
{
  unsigned int  nSymbols = readBits(16) + 1U;
  bool    isLastBlock = readBits(1);
  bool    compressionEnabled = readBits(1);
  if (!compressionEnabled) {
    // copy uncompressed data
    for (unsigned int i = 0U; i < nSymbols; i++) {
      buf[startAddr] = readLiteralByte();
      bytesUsed[startAddr] = true;
      startAddr = (startAddr + 1U) & 0xFFFFU;
    }
    return isLastBlock;
  }
  readDecodeTables();
  for (unsigned int i = 0U; i < nSymbols; i++) {
    unsigned int  matchLength = readMatchLength();
    if (matchLength == 0U) {
      // literal byte
      buf[startAddr] = readLiteralByte();
      bytesUsed[startAddr] = true;
      startAddr = (startAddr + 1U) & 0xFFFFU;
    }
    else if (matchLength >= 0x80000000U) {
      // literal sequence
      matchLength &= 0x7FFFFFFFU;
      while (matchLength > 0U) {
        buf[startAddr] = readLiteralByte();
        bytesUsed[startAddr] = true;
        startAddr = (startAddr + 1U) & 0xFFFFU;
        matchLength--;
      }
    }
    else {
      if (matchLength > 65535U)
        throw Ep128Emu::Exception("error in compressed data");
      // get match offset:
      unsigned int  offs = 0U;
      if (matchLength == 1U) {
        unsigned int  slotNum = readBits(2);
        offs = readLZMatchParameter((unsigned char) slotNum,
                                    &(offs1DecodeTable[0]));
      }
      else if (matchLength == 2U) {
        unsigned int  slotNum = readBits(3);
        offs = readLZMatchParameter((unsigned char) slotNum,
                                    &(offs2DecodeTable[0]));
      }
      else {
        unsigned int  slotNum = readBits(offs3PrefixSize);
        offs = readLZMatchParameter((unsigned char) slotNum,
                                    &(offs3DecodeTable[0]));
      }
      if (offs >= 0xFFFFU)
        throw Ep128Emu::Exception("error in compressed data");
      offs++;
      unsigned int  lzMatchReadAddr = (startAddr - offs) & 0xFFFFU;
      for (unsigned int j = 0U; j < matchLength; j++) {
        if (!bytesUsed[lzMatchReadAddr])  // byte does not exist yet
          throw Ep128Emu::Exception("error in compressed data");
        buf[startAddr] = buf[lzMatchReadAddr];
        bytesUsed[startAddr] = true;
        startAddr = (startAddr + 1U) & 0xFFFFU;
        lzMatchReadAddr = (lzMatchReadAddr + 1U) & 0xFFFFU;
      }
    }
  }
  return isLastBlock;
}

Decompressor::Decompressor()
  : offs3PrefixSize(2),
    shiftRegister(0x00),
    shiftRegisterCnt(0),
    inputBuffer((unsigned char *) 0),
    inputBufferSize(0),
    inputBufferPosition(0)
{
}

Decompressor::~Decompressor()
{
}

void Decompressor::decompressData(
    std::vector< unsigned char >& outBuf,
    const std::vector< unsigned char >& inBuf)
{
  outBuf.clear();
  if (inBuf.size() < 1)
    return;
  unsigned char crcValue = 0xFF;
  // verify checksum
  for (size_t i = inBuf.size(); i-- > 0; ) {
    crcValue = crcValue ^ inBuf[i];
    crcValue = ((crcValue & 0x7F) << 1) | ((crcValue & 0x80) >> 7);
    crcValue = (crcValue + 0xAC) & 0xFF;
  }
  if (crcValue != 0x80)
    throw Ep128Emu::Exception("error in compressed data");
  // decompress all data blocks
  inputBuffer = &(inBuf.front());
  inputBufferSize = inBuf.size();
  inputBufferPosition = 1;
  shiftRegister = 0x00;
  shiftRegisterCnt = 0;
  std::vector< unsigned char >  tmpBuf;
  std::vector< bool > bytesUsed;
  tmpBuf.resize(65536);
  bytesUsed.resize(65536);
  for (size_t j = 0; j < 65536; j++) {
    tmpBuf[j] = 0x00;
    bytesUsed[j] = false;
  }
  unsigned int  startAddr = 0U;
  bool          isLastBlock = false;
  do {
    unsigned int  prvStartAddr = startAddr;
    isLastBlock = decompressDataBlock(tmpBuf, bytesUsed, startAddr);
    unsigned int  j = prvStartAddr;
    do {
      outBuf.push_back(tmpBuf[j]);
      j = (j + 1U) & 0xFFFFU;
    } while (j != startAddr);
    if (outBuf.size() > 16777216)
      throw Ep128Emu::Exception("error in compressed data");
  } while (!isLastBlock);
  // on successful decompression, all input data must be consumed
  if (!(inputBufferPosition >= inputBufferSize && shiftRegister == 0x00))
    throw Ep128Emu::Exception("error in compressed data");
}

// ----------------------------------------------------------------------------

static bool unpackROMFiles(const std::string& romDir)
{
  std::string fName(romDir);
  // assume there is a path delimiter character at the end of 'romDir'
  fName += "ep128emu_roms.bin";
  Decompressor  *decompressor = (Decompressor *) 0;
  std::FILE     *f = std::fopen(fName.c_str(), "rb");
  if (!f)
    return false;
  try {
    std::vector< unsigned char >  buf;
    // read and decompress input file
    {
      std::vector< unsigned char >  inBuf;
      while (true) {
        int     c = std::fgetc(f);
        if (c == EOF)
          break;
        if (inBuf.size() >= 1000000)
          throw Ep128Emu::Exception("invalid compressed ROM data size");
        inBuf.push_back((unsigned char) (c & 0xFF));
      }
      std::fclose(f);
      f = (std::FILE *) 0;
      if (inBuf.size() < 4)
        throw Ep128Emu::Exception("invalid compressed ROM data size");
      decompressor = new Decompressor();
      decompressor->decompressData(buf, inBuf);
      delete decompressor;
      decompressor = (Decompressor *) 0;
      if (buf.size() < 4 || buf.size() > 2000000)
        throw Ep128Emu::Exception("invalid packed ROM data size");
    }
    // get the number of ROM files in the package
    size_t  nFiles = (size_t(buf[0]) << 24) | (size_t(buf[1]) << 16)
                     | (size_t(buf[2]) << 8) | size_t(buf[3]);
    if (nFiles < 1 || nFiles > 50)
      throw Ep128Emu::Exception("invalid number of files in packed ROM data");
    size_t  offs = nFiles * 32 + 4;
    if (offs > buf.size())
      throw Ep128Emu::Exception("unexpected end of packed ROM data");
    // extract and write all ROM files
    bool    err = false;        // true if there are any write errors
    for (size_t i = 0; i < nFiles; i++) {
      // get file size:
      size_t  fileSize =
          (size_t(buf[i * 32 + 4]) << 24) | (size_t(buf[i * 32 + 5]) << 16)
          | (size_t(buf[i * 32 + 6]) << 8) | size_t(buf[i * 32 + 7]);
      // must be an integer multiple of 16384 in the range 16384 to 65536
      if (fileSize < 16384 || fileSize > 65536 || (fileSize & 0x3FFF) != 0)
        throw Ep128Emu::Exception("invalid ROM file size");
      // get file name
      fName = romDir;
      for (size_t j = 0; j < 28; j++) {
        unsigned char c = buf[i * 32 + 8 + j];
        if (c == 0x00) {
          if (j == 0)
            throw Ep128Emu::Exception("invalid ROM file name");
          break;
        }
        // convert to lower case
        if (c >= 0x41 && c <= 0x5A)
          c = c | 0x20;
        // replace invalid characters with underscores
        if (!((c >= 0x30 && c <= 0x39) || (c >= 0x61 && c <= 0x7A) ||
              c == 0x2B || c == 0x2D || (c == 0x2E && j != 0))) {
          c = 0x5F;
        }
        fName += char(c);
      }
      f = std::fopen(fName.c_str(), "wb");
      if (!f) {
        err = true;
      }
      else {
        // write ROM file
        for (size_t j = 0; j < fileSize; j++) {
          if ((offs + j) >= buf.size())
            throw Ep128Emu::Exception("unexpected end of packed ROM data");
          if (std::fputc(buf[offs + j], f) == EOF) {
            err = true;
            break;
          }
        }
        if (std::fclose(f) != 0)
          err = true;
        f = (std::FILE *) 0;
      }
      offs += fileSize;
    }
    if (err)
      throw Ep128Emu::Exception("error writing ROM file");
  }
  catch (...) {
    if (decompressor)
      delete decompressor;
    if (f)
      std::fclose(f);
    throw;
  }
  return true;
}

// ----------------------------------------------------------------------------

int main(int argc, char **argv)
{
  if ((sizeof(machineConfigs) / sizeof(uint64_t))
      > (sizeof(machineConfigFileNames) / sizeof(char *))) {
    std::abort();
  }
  Fl::lock();
#ifndef WIN32
  Ep128Emu::setGUIColorScheme(0);
#else
  Ep128Emu::setGUIColorScheme(1);
#endif
  bool    forceInstallFlag = false;
  std::string installDirectory = "";
  {
    int     i = 0;
    while (++i < argc) {
      if (argv[i][0] == '-') {
        if (argv[i][1] == '-' && argv[i][2] == '\0')
          break;
        if (argv[i][1] == 'f' && argv[i][2] == '\0') {
          forceInstallFlag = true;
          continue;
        }
      }
      installDirectory = argv[i];
    }
    if (i < (argc - 1))
      installDirectory = argv[argc - 1];
  }
#ifndef WIN32
  if (installDirectory.length() == 0 && forceInstallFlag)
    installDirectory = Ep128Emu::getEp128EmuHomeDirectory();
#endif
  if (installDirectory.length() == 0) {
    std::string tmp = "";
#ifndef WIN32
    tmp = Ep128Emu::getEp128EmuHomeDirectory();
#endif
    Fl_Native_File_Chooser  *w = new Fl_Native_File_Chooser();
    w->type(Fl_Native_File_Chooser::BROWSE_SAVE_DIRECTORY);
    w->title("Select installation directory for ep128emu data files");
    w->filter("*");
    w->directory(tmp.c_str());
    if (w->show() == 0) {
      if (w->filename() != (char *) 0)
        installDirectory = w->filename();
    }
    delete w;
  }
  Ep128Emu::stripString(installDirectory);
  if (installDirectory.length() == 0)
    return -1;
#ifndef WIN32
  while (installDirectory[installDirectory.length() - 1] == '/' &&
         installDirectory.length() > 1) {
    installDirectory.resize(installDirectory.length() - 1);
  }
  {
    mkdir(installDirectory.c_str(), 0755);
    std::string tmp = installDirectory;
    if (tmp[tmp.length() - 1] != '/')
      tmp += '/';
    std::string tmp2 = tmp + "config";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "config/cpc";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "config/ep128brd";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "config/ep128esp";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "config/ep128hun";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "config/ep128uk";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "config/ep64";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "config/zx";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "demo";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "disk";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "files";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "roms";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "snapshot";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "tape";
    mkdir(tmp2.c_str(), 0755);
  }
#else
  while ((installDirectory[installDirectory.length() - 1] == '/' ||
          installDirectory[installDirectory.length() - 1] == '\\') &&
         !(installDirectory.length() <= 1 ||
           (installDirectory.length() == 3 && installDirectory[1] == ':'))) {
    installDirectory.resize(installDirectory.length() - 1);
  }
  {
    _mkdir(installDirectory.c_str());
    std::string tmp = installDirectory;
    if (tmp[tmp.length() - 1] != '/' && tmp[tmp.length() - 1] != '\\')
      tmp += '\\';
    std::string tmp2 = tmp + "config";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "config\\cpc";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "config\\ep128brd";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "config\\ep128esp";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "config\\ep128hun";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "config\\ep128uk";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "config\\ep64";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "config\\zx";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "demo";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "disk";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "files";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "roms";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "snapshot";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "tape";
    _mkdir(tmp2.c_str());
  }
#endif
#ifdef WIN32
  uint8_t c = '\\';
#else
  uint8_t c = '/';
#endif
  if (installDirectory[installDirectory.length() - 1] != c)
    installDirectory += c;
  std::string configDirectory = installDirectory + "config";
  configDirectory += c;
  std::string romDirectory = installDirectory + "roms";
  romDirectory += c;
  Ep128EmuConfigInstallerGUI  *gui = new Ep128EmuConfigInstallerGUI();
  Ep128Emu::setWindowIcon(gui->mainWindow, 11);
  Ep128Emu::setWindowIcon(gui->errorWindow, 12);
  if (!forceInstallFlag) {
    gui->mainWindow->show();
    do {
      Fl::wait(0.05);
    } while (gui->mainWindow->shown());
  }
  else
    gui->enableCfgInstall = true;
  try {
    if (unpackROMFiles(romDirectory)) {
      // if successfully extracted, delete compressed ROM package
      std::string tmp(romDirectory);
      tmp += "ep128emu_roms.bin";
      std::FILE *f = std::fopen(tmp.c_str(), "r+b");
      if (f) {
        std::fclose(f);
        std::remove(tmp.c_str());
      }
    }
  }
  catch (std::exception& e) {
    gui->errorMessage(e.what());
  }
  try {
    Ep128Emu::ConfigurationDB     *config = (Ep128Emu::ConfigurationDB *) 0;
    Ep128EmuMachineConfiguration  *mCfg = (Ep128EmuMachineConfiguration *) 0;
    if (gui->enableCfgInstall) {
      Ep128EmuDisplaySndConfiguration   *dsCfg =
          (Ep128EmuDisplaySndConfiguration *) 0;
      config = new Ep128Emu::ConfigurationDB();
      {
        Ep128EmuGUIConfiguration  *gCfg =
            new Ep128EmuGUIConfiguration(*config, installDirectory);
        try {
          Ep128Emu::File  f;
          config->saveState(f);
          f.writeFile("gui_cfg.dat", true);
        }
        catch (std::exception& e) {
          gui->errorMessage(e.what());
        }
        delete gCfg;
      }
      delete config;
      for (int i = 0; i < 3; i++) {
        const char  *fName = "ep128cfg.dat";
        int         configNum = 39;
        if (i == 1) {
          fName = "zx128cfg.dat";
          configNum = int(sizeof(machineConfigs) / sizeof(uint64_t)) + 4;
        }
        else if (i == 2) {
          fName = "cpc_cfg.dat";
          configNum = int(sizeof(machineConfigs) / sizeof(uint64_t)) + 7;
        }
        config = new Ep128Emu::ConfigurationDB();
        dsCfg = new Ep128EmuDisplaySndConfiguration(*config);
        mCfg = new Ep128EmuMachineConfiguration(*config, configNum,
                                                romDirectory);
        setKeyboardConfiguration(*config, (gui->keyboardMapHU ? 1 : 0));
        std::string fileIODir = installDirectory + "files";
        config->createKey("fileio.workingDirectory", fileIODir);
        try {
          Ep128Emu::File  f;
          config->saveState(f);
          f.writeFile(fName, true);
        }
        catch (std::exception& e) {
          gui->errorMessage(e.what());
        }
        delete config;
        delete dsCfg;
        delete mCfg;
        config = (Ep128Emu::ConfigurationDB *) 0;
        dsCfg = (Ep128EmuDisplaySndConfiguration *) 0;
        mCfg = (Ep128EmuMachineConfiguration *) 0;
      }
    }
    for (int i = 0;
         i < int(sizeof(machineConfigFileNames) / sizeof(char *));
         i++) {
      config = new Ep128Emu::ConfigurationDB();
      mCfg = new Ep128EmuMachineConfiguration(*config, i, romDirectory);
      try {
        std::string fileName = configDirectory;
        fileName += machineConfigFileNames[i];
        config->saveState(fileName.c_str(), false);
      }
      catch (std::exception& e) {
        gui->errorMessage(e.what());
      }
      delete config;
      delete mCfg;
      config = (Ep128Emu::ConfigurationDB *) 0;
      mCfg = (Ep128EmuMachineConfiguration *) 0;
    }
    for (int i = 0; i < 8; i++) {
      if (keyboardConfigFileNames[i] != (char *) 0) {
        config = new Ep128Emu::ConfigurationDB();
        try {
          setKeyboardConfiguration(*config, i);
          std::string fileName = configDirectory;
          fileName += keyboardConfigFileNames[i];
          config->saveState(fileName.c_str(), false);
        }
        catch (std::exception& e) {
          gui->errorMessage(e.what());
        }
        delete config;
        config = (Ep128Emu::ConfigurationDB *) 0;
      }
    }
  }
  catch (std::exception& e) {
    gui->errorMessage(e.what());
    delete gui;
    return -1;
  }
  delete gui;
  return 0;
}

