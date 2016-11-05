
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

#include "ep128emu.hpp"
#include "fileio.hpp"
#include "system.hpp"
#include "cfg_db.hpp"
#include "mkcfg_fl.hpp"
#include "guicolor.hpp"
#include "decompm2.hpp"

#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>

#ifndef WIN32
#  include <sys/types.h>
#  include <sys/stat.h>
#else
#  include <windows.h>
#  include <direct.h>
#endif

#include <vector>

#ifndef MAKECFG_ROM_PKG_NAME
#  define MAKECFG_ROM_PKG_NAME  "ep128emu_roms-2.0.10.bin"
#endif

#ifdef MAKECFG_USE_CURL
#  ifndef MAKECFG_ROM_URL_1
#    define MAKECFG_ROM_URL_1   "http://ep128.hu/Emu/" MAKECFG_ROM_PKG_NAME
#  endif
#  ifndef MAKECFG_ROM_URL_2
#    define MAKECFG_ROM_URL_2   "https://enterpriseforever.com/"        \
                                "letoltesek-downloads/egyeb-misc/"      \
                                "?action=dlattach;attach=16486"
#  endif
#  include <curl/curl.h>
#endif

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
  struct {
    std::string       romFile;
    bool              enabled;
  } sdext;
 public:
  Ep128EmuMachineConfiguration(Ep128Emu::ConfigurationDB& config,
                               uint64_t machineConfig,
                               const std::string& romDirectory);
  ~Ep128EmuMachineConfiguration()
  {
  }
};

// ----------------------------------------------------------------------------

static const int epRAMSizeTable[8] = {
    64,  128,  256,  320,  640, 1024, 2048, 3712
};

#define EP_RAM_64K              uint64_t(0)
#define EP_RAM_128K             uint64_t(1)
#define EP_RAM_256K             uint64_t(2)
#define EP_RAM_320K             uint64_t(3)
#define EP_RAM_640K             uint64_t(4)
#define EP_RAM_1024K            uint64_t(5)
#define EP_RAM_2048K            uint64_t(6)
#define EP_RAM_3712K            uint64_t(7)

struct EP_ROM_File {
  const char  *fileName;
  uint32_t    segments;
};

static const EP_ROM_File epROMFiles[61] = {
#define EP_ROM_EXOS20           (uint64_t(1) << 3)
  { "exos20.rom",               0x0001FFFFU },
#define EP_ROM_EXOS21           (uint64_t(1) << 4)
  { "exos21.rom",               0x0001FFFFU },
#define EP_ROM_EXOS22           (uint64_t(1) << 5)
  { "exos22.rom",               0x00010203U },
#define EP_ROM_EXOS23           (uint64_t(1) << 6)
  { "exos23.rom",               0x00010203U },
#define EP_ROM_EXOS232_ES       (uint64_t(1) << 7)
  { "exos232esp.rom",           0x00010203U },
#define EP_ROM_EXOS232_HU       (uint64_t(1) << 8)
  { "exos232hun.rom",           0x00010203U },
#define EP_ROM_EXOS232_UK       (uint64_t(1) << 9)
  { "exos232uk.rom",            0x00010203U },
#define EP_ROM_EXOS24_BRD       (uint64_t(1) << 10)
  { "exos24brd.rom",            0x00010203U },
#define EP_ROM_EXOS24_ES        (uint64_t(1) << 11)
  { "exos24es.rom",             0x00010203U },
#define EP_ROM_EXOS24_HU        (uint64_t(1) << 12)
  { "exos24hu.rom",             0x00010203U },
#define EP_ROM_EXOS24_UK        (uint64_t(1) << 13)
  { "exos24uk.rom",             0x00010203U },
#define EP_ROM_BASIC20_04       (uint64_t(1) << 14)
  { "basic20.rom",              0x04FFFFFFU },
#define EP_ROM_BASIC20_06       (uint64_t(1) << 15)
  { "basic20.rom",              0x06FFFFFFU },
#define EP_ROM_BASIC21_04       (uint64_t(1) << 16)
  { "basic21.rom",              0x04FFFFFFU },
#define EP_ROM_BASIC21_05       (uint64_t(1) << 17)
  { "basic21.rom",              0x05FFFFFFU },
#define EP_ROM_BASIC21_06       (uint64_t(1) << 18)
  { "basic21.rom",              0x06FFFFFFU },
#define EP_ROM_EXDOS10          (uint64_t(1) << 19)
  { "exdos10.rom",              0x20FFFFFFU },
#define EP_ROM_EXDOS14I_BRD     (uint64_t(1) << 20)
  { "exdos14isdos10uk-brd.rom", 0x2021FFFFU },
#define EP_ROM_EXDOS14I_ES      (uint64_t(1) << 21)
  { "exdos14isdos10uk-esp.rom", 0x2021FFFFU },
#define EP_ROM_EXDOS14I_HU      (uint64_t(1) << 22)
  { "exdos14isdos10uk-hfont.rom",       0x2021FFFFU },
#define EP_ROM_EXDOS14I_UK      (uint64_t(1) << 23)
  { "exdos14isdos10uk.rom",     0x2021FFFFU },
#define EP_ROM_EPFILEIO         (uint64_t(1) << 24)
  { "epfileio.rom",             0x10FFFFFFU },
#define EP_ROM_BRD_04           (uint64_t(1) << 25)
  { "brd.rom",                  0x04FFFFFFU },
#define EP_ROM_BRD_07           (uint64_t(1) << 26)
  { "brd.rom",                  0x07FFFFFFU },
#define EP_ROM_BRD_43           (uint64_t(1) << 27)
  { "brd.rom",                  0x43FFFFFFU },
#define EP_ROM_ESP_04           (uint64_t(1) << 28)
  { "esp.rom",                  0x04FFFFFFU },
#define EP_ROM_ESP_07           (uint64_t(1) << 29)
  { "esp.rom",                  0x07FFFFFFU },
#define EP_ROM_ESP_43           (uint64_t(1) << 30)
  { "esp.rom",                  0x43FFFFFFU },
#define EP_ROM_HUN_04           (uint64_t(1) << 31)
  { "hun.rom",                  0x04FFFFFFU },
#define EP_ROM_HUN_07           (uint64_t(1) << 32)
  { "hun.rom",                  0x07FFFFFFU },
#define EP_ROM_HUN_43           (uint64_t(1) << 33)
  { "hun.rom",                  0x43FFFFFFU },
#define EP_ROM_ASMON15_04       (uint64_t(1) << 34)
  { "asmon15.rom",              0x0405FFFFU },
#define EP_ROM_CYRUS            (uint64_t(1) << 35)
  { "cyrus.rom",                0x40FFFFFFU },
#define EP_ROM_EP_PLUS_05       (uint64_t(1) << 36)
  { "ep-plus.rom",              0x05FFFFFFU },
#define EP_ROM_EP_PLUS_07       (uint64_t(1) << 37)
  { "ep-plus.rom",              0x07FFFFFFU },
#define EP_ROM_EPD17Z12_06      (uint64_t(1) << 38)
  { "epd17z12.rom",             0x0607FFFFU },
#define EP_ROM_EPD19HU_04       (uint64_t(1) << 39)
  { "epd19hft.rom",             0x0405FFFFU },
#define EP_ROM_EPD19UK_04       (uint64_t(1) << 40)
  { "epd19uk.rom",              0x0405FFFFU },
#define EP_ROM_EPD19HU_06       (uint64_t(1) << 41)
  { "epd19hft.rom",             0x0607FFFFU },
#define EP_ROM_EPD19UK_06       (uint64_t(1) << 42)
  { "epd19uk.rom",              0x0607FFFFU },
#define EP_ROM_FENAS12_12       (uint64_t(1) << 43)
  { "fenas12.rom",              0x1213FFFFU },
#define EP_ROM_FENAS12_22       (uint64_t(1) << 44)
  { "fenas12.rom",              0x2223FFFFU },
#define EP_ROM_FORTH            (uint64_t(1) << 45)
  { "forth.rom",                0x42FFFFFFU },
#define EP_ROM_GENMON           (uint64_t(1) << 46)
  { "genmon.rom",               0x3233FFFFU },
#define EP_ROM_HEASS10_HU       (uint64_t(1) << 47)
  { "heassekn.rom",             0x1213FFFFU },
#define EP_ROM_HEASS10_UK       (uint64_t(1) << 48)
  { "heass10uk.rom",            0x1213FFFFU },
#define EP_ROM_IDE12_42         (uint64_t(1) << 49)
  { "ide12.rom",                0x42FFFFFFU },
#define EP_ROM_IVIEW_30         (uint64_t(1) << 50)
  { "iview.rom",                0x3031FFFFU },
#define EP_ROM_LISP             (uint64_t(1) << 51)
  { "lisp.rom",                 0x11FFFFFFU },
#define EP_ROM_PASCAL12         (uint64_t(1) << 52)
  { "pascal12.rom",             0x43FFFFFFU },
#define EP_ROM_TPT_32           (uint64_t(1) << 53)
  { "tpt.rom",                  0x32FFFFFFU },
#define EP_ROM_ZT19_HU          (uint64_t(1) << 54)
  { "zt19hun.rom",              0x4041FFFFU },
#define EP_ROM_ZT19_UK          (uint64_t(1) << 55)
  { "zt19uk.rom",               0x4041FFFFU },
#define EP_ROM_ZX41_HU          (uint64_t(1) << 56)
  { "zx41.rom",                 0x3031FFFFU },
#define EP_ROM_ZX41_UK          (uint64_t(1) << 57)
  { "zx41uk.rom",               0x3031FFFFU },
#define EP_ROM_EDCW             (uint64_t(1) << 58)
  { "edcw.rom",                 0x33FFFFFFU },
#define EP_ROM_PASZIANS         (uint64_t(1) << 59)
  { "paszians.rom",             0x2223FFFFU },
  // sdext05.rom is a special case, needs to be loaded with sdext.romFile
#define EP_ROM_SDEXT05_07       (uint64_t(1) << 60)
  { "sdext05.rom",              0x87FFFFFFU },
  { (char *) 0,                 0xFFFFFFFFU },
  { (char *) 0,                 0xFFFFFFFFU },
  { (char *) 0,                 0xFFFFFFFFU }
};

#define ZX_RAM_16K              (16UL << 12)
#define ZX_RAM_48K              (48UL << 12)
#define ZX_RAM_128K             (128UL << 12)
#define ZX_ROM_ZX48             (uint64_t(1) << 22)
#define ZX_ROM_ZX128            (uint64_t(1) << 23)
#define ZX_ROM_ZX48GW03         (uint64_t(1) << 24)
#define ZX_ENABLE_FILEIO        (uint64_t(1) << 25)
#define CPC_RAM_64K             (64UL << 12)
#define CPC_RAM_128K            (128UL << 12)
#define CPC_RAM_576K            (576UL << 12)
#define CPC_ROM_464             (uint64_t(1) << 26)
#define CPC_ROM_664             (uint64_t(1) << 27)
#define CPC_ROM_6128            (uint64_t(1) << 28)
#define CPC_ROM_AMSDOS          (uint64_t(1) << 29)

#define IS_EP_CONFIG(x)         bool((x) & 0x0000FFFFUL)
#define EP_RAM_SIZE(x)          epRAMSizeTable[(x) & 7UL]
#define ZX_CPC_RAM_SIZE(x)      int(((x) & 0x003F0000UL) >> 12)
#define IS_ZX_CONFIG(x)         (((x) & 0x03C00000UL) && !IS_EP_CONFIG(x))
#define IS_CPC_CONFIG(x)        (((x) & 0x3C000000UL) && !IS_EP_CONFIG(x))

// ----------------------------------------------------------------------------

struct EPMachineConfig {
  const char  *fileName;
  uint64_t    config;
};

static const EPMachineConfig machineConfigs[] = {
  { "ep128brd/EP2048k_EXOS24_EXDOS_utils.cfg",
    EP_RAM_2048K | EP_ROM_EXOS24_BRD | EP_ROM_ASMON15_04 | EP_ROM_EPD17Z12_06
    | EP_ROM_EPFILEIO | EP_ROM_LISP | EP_ROM_HEASS10_UK | EP_ROM_EXDOS14I_BRD
    | EP_ROM_FENAS12_22 | EP_ROM_ZX41_UK | EP_ROM_GENMON | EP_ROM_ZT19_UK
    | EP_ROM_FORTH | EP_ROM_BRD_43
  },
  { "ep128brd/EP_128k_EXDOS.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BRD_04 | EP_ROM_BASIC21_05
    | EP_ROM_EXDOS14I_BRD
  },
  { "ep128brd/EP_128k_EXDOS_FileIO.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BRD_04 | EP_ROM_BASIC21_05
    | EP_ROM_EPFILEIO | EP_ROM_EXDOS14I_BRD
  },
  { "ep128brd/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BRD_04 | EP_ROM_BASIC21_05
    | EP_ROM_EPFILEIO | EP_ROM_EXDOS14I_BRD | EP_ROM_ZX41_UK
  },
  { "ep128brd/EP_128k_EXDOS_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_BRD_07 | EP_ROM_EXDOS14I_BRD
  },
  { "ep128brd/EP_128k_Tape.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BRD_04 | EP_ROM_BASIC21_05
  },
  { "ep128brd/EP_128k_Tape_FileIO.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BRD_04 | EP_ROM_BASIC21_05
    | EP_ROM_EPFILEIO
  },
  { "ep128brd/EP_128k_Tape_FileIO_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_BRD_07 | EP_ROM_EPFILEIO
  },
  { "ep128brd/EP_128k_Tape_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_BRD_07
  },
  { "ep128brd/EP_640k_EXOS232_EXDOS.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_UK | EP_ROM_BRD_04 | EP_ROM_EXDOS14I_BRD
  },
  { "ep128brd/EP_640k_EXOS232_EXDOS_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_UK | EP_ROM_ASMON15_04 | EP_ROM_EPD17Z12_06
    | EP_ROM_EPFILEIO | EP_ROM_FENAS12_12 | EP_ROM_EXDOS14I_BRD
    | EP_ROM_PASZIANS | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW
    | EP_ROM_ZT19_UK | EP_ROM_BRD_43
  },
  { "ep128brd/EP_640k_EXOS232_IDE_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_UK | EP_ROM_ASMON15_04 | EP_ROM_EPD19UK_06
    | EP_ROM_EPFILEIO | EP_ROM_HEASS10_UK | EP_ROM_EXDOS14I_BRD
    | EP_ROM_PASZIANS | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW
    | EP_ROM_ZT19_UK | EP_ROM_IDE12_42 | EP_ROM_BRD_43
  },
  { "ep128brd/EP_640k_EXOS24_SDEXT_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS24_BRD | EP_ROM_EPD19UK_04 | EP_ROM_SDEXT05_07
    | EP_ROM_EPFILEIO | EP_ROM_HEASS10_UK | EP_ROM_EXDOS14I_BRD
    | EP_ROM_PASZIANS | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW
    | EP_ROM_ZT19_UK | EP_ROM_BRD_43
  },
  { "ep128esp/EP2048k_EXOS24_EXDOS_utils.cfg",
    EP_RAM_2048K | EP_ROM_EXOS24_ES | EP_ROM_ASMON15_04 | EP_ROM_EPD17Z12_06
    | EP_ROM_EPFILEIO | EP_ROM_LISP | EP_ROM_HEASS10_UK | EP_ROM_EXDOS14I_ES
    | EP_ROM_FENAS12_22 | EP_ROM_ZX41_UK | EP_ROM_GENMON | EP_ROM_ZT19_UK
    | EP_ROM_FORTH | EP_ROM_ESP_43
  },
  { "ep128esp/EP_128k_EXDOS.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ESP_04 | EP_ROM_BASIC21_05
    | EP_ROM_EXDOS14I_ES
  },
  { "ep128esp/EP_128k_EXDOS_FileIO.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ESP_04 | EP_ROM_BASIC21_05
    | EP_ROM_EPFILEIO | EP_ROM_EXDOS14I_ES
  },
  { "ep128esp/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ESP_04 | EP_ROM_BASIC21_05
    | EP_ROM_EPFILEIO | EP_ROM_EXDOS14I_ES | EP_ROM_ZX41_UK
  },
  { "ep128esp/EP_128k_EXDOS_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_ESP_07 | EP_ROM_EXDOS14I_ES
  },
  { "ep128esp/EP_128k_Tape.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ESP_04 | EP_ROM_BASIC21_05
  },
  { "ep128esp/EP_128k_Tape_FileIO.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ESP_04 | EP_ROM_BASIC21_05
    | EP_ROM_EPFILEIO
  },
  { "ep128esp/EP_128k_Tape_FileIO_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_ESP_07 | EP_ROM_EPFILEIO
  },
  { "ep128esp/EP_128k_Tape_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_ESP_07
  },
  { "ep128esp/EP_640k_EXOS232_EXDOS.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_ES | EP_ROM_ESP_04 | EP_ROM_EXDOS14I_ES
  },
  { "ep128esp/EP_640k_EXOS232_EXDOS_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_ES | EP_ROM_ASMON15_04 | EP_ROM_EPD17Z12_06
    | EP_ROM_EPFILEIO | EP_ROM_FENAS12_12 | EP_ROM_EXDOS14I_ES | EP_ROM_PASZIANS
    | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW | EP_ROM_ZT19_UK
    | EP_ROM_ESP_43
  },
  { "ep128esp/EP_640k_EXOS232_IDE_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_ES | EP_ROM_ASMON15_04 | EP_ROM_EPD19UK_06
    | EP_ROM_EPFILEIO | EP_ROM_HEASS10_UK | EP_ROM_EXDOS14I_ES | EP_ROM_PASZIANS
    | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW | EP_ROM_ZT19_UK
    | EP_ROM_IDE12_42 | EP_ROM_ESP_43
  },
  { "ep128esp/EP_640k_EXOS24_SDEXT_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS24_ES | EP_ROM_EPD19UK_04 | EP_ROM_SDEXT05_07
    | EP_ROM_EPFILEIO | EP_ROM_HEASS10_UK | EP_ROM_EXDOS14I_ES | EP_ROM_PASZIANS
    | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW | EP_ROM_ZT19_UK
    | EP_ROM_ESP_43
  },
  { "ep128hun/EP2048k_EXOS24_EXDOS_utils.cfg",
    EP_RAM_2048K | EP_ROM_EXOS24_HU | EP_ROM_ASMON15_04 | EP_ROM_EPD17Z12_06
    | EP_ROM_EPFILEIO | EP_ROM_LISP | EP_ROM_HEASS10_HU | EP_ROM_EXDOS14I_HU
    | EP_ROM_FENAS12_22 | EP_ROM_ZX41_HU | EP_ROM_GENMON | EP_ROM_ZT19_HU
    | EP_ROM_FORTH | EP_ROM_HUN_43
  },
  { "ep128hun/EP_128k_EXDOS.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_HUN_04 | EP_ROM_BASIC21_05
    | EP_ROM_EXDOS14I_HU
  },
  { "ep128hun/EP_128k_EXDOS_FileIO.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_HUN_04 | EP_ROM_BASIC21_05
    | EP_ROM_EPFILEIO | EP_ROM_EXDOS14I_HU
  },
  { "ep128hun/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_HUN_04 | EP_ROM_BASIC21_05
    | EP_ROM_EPFILEIO | EP_ROM_EXDOS14I_HU | EP_ROM_ZX41_HU
  },
  { "ep128hun/EP_128k_EXDOS_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_HUN_07 | EP_ROM_EXDOS14I_HU
  },
  { "ep128hun/EP_128k_Tape.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_HUN_04 | EP_ROM_BASIC21_05
  },
  { "ep128hun/EP_128k_Tape_FileIO.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_HUN_04 | EP_ROM_BASIC21_05
    | EP_ROM_EPFILEIO
  },
  { "ep128hun/EP_128k_Tape_FileIO_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_HUN_07 | EP_ROM_EPFILEIO
  },
  { "ep128hun/EP_128k_Tape_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_HUN_07
  },
  { "ep128hun/EP_640k_EXOS22_EXDOS.cfg",
    EP_RAM_640K | EP_ROM_EXOS22 | EP_ROM_HUN_04 | EP_ROM_EXDOS14I_HU
  },
  { "ep128hun/EP_640k_EXOS23_EXDOS.cfg",
    EP_RAM_640K | EP_ROM_EXOS23 | EP_ROM_HUN_04 | EP_ROM_EXDOS14I_HU
  },
  { "ep128hun/EP_640k_EXOS232_EXDOS.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_HU | EP_ROM_HUN_04 | EP_ROM_EXDOS14I_HU
  },
  { "ep128hun/EP_640k_EXOS232_EXDOS_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_HU | EP_ROM_ASMON15_04 | EP_ROM_EPD17Z12_06
    | EP_ROM_EPFILEIO | EP_ROM_FENAS12_12 | EP_ROM_EXDOS14I_HU | EP_ROM_PASZIANS
    | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW | EP_ROM_ZT19_HU
    | EP_ROM_HUN_43
  },
  { "ep128hun/EP_640k_EXOS232_IDE_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_HU | EP_ROM_ASMON15_04 | EP_ROM_EPD19HU_06
    | EP_ROM_EPFILEIO | EP_ROM_HEASS10_HU | EP_ROM_EXDOS14I_HU | EP_ROM_PASZIANS
    | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW | EP_ROM_ZT19_HU
    | EP_ROM_IDE12_42 | EP_ROM_HUN_43
  },
  { "ep128hun/EP_640k_EXOS24_SDEXT_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS24_HU | EP_ROM_EPD19HU_04 | EP_ROM_SDEXT05_07
    | EP_ROM_EPFILEIO | EP_ROM_HEASS10_HU | EP_ROM_EXDOS14I_HU | EP_ROM_PASZIANS
    | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW | EP_ROM_ZT19_HU
    | EP_ROM_HUN_43
  },
  { "ep128uk/EP2048k_EXOS24_EXDOS_utils.cfg",
    EP_RAM_2048K | EP_ROM_EXOS24_UK | EP_ROM_ASMON15_04 | EP_ROM_EPD17Z12_06
    | EP_ROM_EPFILEIO | EP_ROM_LISP | EP_ROM_HEASS10_UK | EP_ROM_EXDOS14I_UK
    | EP_ROM_FENAS12_22 | EP_ROM_ZX41_UK | EP_ROM_GENMON | EP_ROM_ZT19_UK
    | EP_ROM_FORTH | EP_ROM_PASCAL12
  },
  { "ep128uk/EP_128k_EXDOS.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EXDOS14I_UK
  },
  { "ep128uk/EP_128k_EXDOS_EP-PLUS.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EP_PLUS_05
    | EP_ROM_EXDOS14I_UK
  },
  { "ep128uk/EP_128k_EXDOS_EP-PLUS_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_EP_PLUS_07 | EP_ROM_EXDOS14I_UK
  },
  { "ep128uk/EP_128k_EXDOS_FileIO.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EPFILEIO
    | EP_ROM_EXDOS14I_UK
  },
  { "ep128uk/EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EPFILEIO
    | EP_ROM_EXDOS14I_UK | EP_ROM_ZX41_UK
  },
  { "ep128uk/EP_128k_EXDOS_NoCartridge.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_EXDOS14I_UK
  },
  { "ep128uk/EP_128k_EXDOS_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_EXDOS14I_UK
  },
  { "ep128uk/EP_128k_Tape.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04
  },
  { "ep128uk/EP_128k_Tape_EP-PLUS.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EP_PLUS_05
  },
  { "ep128uk/EP_128k_Tape_FileIO.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_BASIC21_04 | EP_ROM_EPFILEIO
  },
  { "ep128uk/EP_128k_Tape_FileIO_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
    | EP_ROM_EPFILEIO
  },
  { "ep128uk/EP_128k_Tape_NoCartridge.cfg",
    EP_RAM_128K | EP_ROM_EXOS21
  },
  { "ep128uk/EP_128k_Tape_NoCartridge_FileIO.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_EPFILEIO
  },
  { "ep128uk/EP_128k_Tape_TASMON.cfg",
    EP_RAM_128K | EP_ROM_EXOS21 | EP_ROM_ASMON15_04 | EP_ROM_BASIC21_06
  },
  { "ep128uk/EP_640k_EXOS232_EXDOS.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_UK | EP_ROM_EXDOS14I_UK
  },
  { "ep128uk/EP_640k_EXOS232_EXDOS_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_UK | EP_ROM_ASMON15_04 | EP_ROM_EPD17Z12_06
    | EP_ROM_EPFILEIO | EP_ROM_FENAS12_12 | EP_ROM_EXDOS14I_UK | EP_ROM_PASZIANS
    | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW | EP_ROM_ZT19_UK
  },
  { "ep128uk/EP_640k_EXOS232_IDE_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS232_UK | EP_ROM_ASMON15_04 | EP_ROM_EPD19UK_06
    | EP_ROM_EPFILEIO | EP_ROM_HEASS10_UK | EP_ROM_EXDOS14I_UK | EP_ROM_PASZIANS
    | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW | EP_ROM_ZT19_UK
    | EP_ROM_IDE12_42
  },
  { "ep128uk/EP_640k_EXOS24_SDEXT_utils.cfg",
    EP_RAM_640K | EP_ROM_EXOS24_UK | EP_ROM_EPD19UK_04 | EP_ROM_SDEXT05_07
    | EP_ROM_EPFILEIO | EP_ROM_HEASS10_UK | EP_ROM_EXDOS14I_UK | EP_ROM_PASZIANS
    | EP_ROM_IVIEW_30 | EP_ROM_TPT_32 | EP_ROM_EDCW | EP_ROM_ZT19_UK
  },
  { "ep64/EP_64k_EXDOS.cfg",
    EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_BASIC20_04 | EP_ROM_EXDOS10
  },
  { "ep64/EP_64k_EXDOS_FileIO.cfg",
    EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_BASIC20_04 | EP_ROM_EPFILEIO
    | EP_ROM_EXDOS10
  },
  { "ep64/EP_64k_EXDOS_NoCartridge.cfg",
    EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_EXDOS10
  },
  { "ep64/EP_64k_EXDOS_TASMON.cfg",
    EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_ASMON15_04 | EP_ROM_BASIC20_06
    | EP_ROM_EXDOS10
  },
  { "ep64/EP_64k_Tape.cfg",
    EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_BASIC20_04
  },
  { "ep64/EP_64k_Tape_FileIO.cfg",
    EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_BASIC20_04 | EP_ROM_EPFILEIO
  },
  { "ep64/EP_64k_Tape_NoCartridge.cfg",
    EP_RAM_64K | EP_ROM_EXOS20
  },
  { "ep64/EP_64k_Tape_TASMON.cfg",
    EP_RAM_64K | EP_ROM_EXOS20 | EP_ROM_ASMON15_04 | EP_ROM_BASIC20_06
  },
  { "zx/ZX_16k.cfg",
    ZX_RAM_16K | ZX_ROM_ZX48
  },
  { "zx/ZX_16k_FileIO.cfg",
    ZX_RAM_16K | ZX_ROM_ZX48 | ZX_ENABLE_FILEIO
  },
  { "zx/ZX_48k.cfg",
    ZX_RAM_48K | ZX_ROM_ZX48
  },
  { "zx/ZX_48k_FileIO.cfg",
    ZX_RAM_48K | ZX_ROM_ZX48 | ZX_ENABLE_FILEIO
  },
  { "zx/ZX_48k_GW03.cfg",
    ZX_RAM_48K | ZX_ROM_ZX48GW03
  },
  { "zx/ZX_128k.cfg",
    ZX_RAM_128K | ZX_ROM_ZX128
  },
  { "zx/ZX_128k_FileIO.cfg",
    ZX_RAM_128K | ZX_ROM_ZX128 | ZX_ENABLE_FILEIO
  },
  { "cpc/CPC_64k.cfg",
    CPC_RAM_64K | CPC_ROM_464
  },
  { "cpc/CPC_64k_AMSDOS.cfg",
    CPC_RAM_64K | CPC_ROM_664 | CPC_ROM_AMSDOS
  },
  { "cpc/CPC_128k.cfg",
    CPC_RAM_128K | CPC_ROM_6128
  },
  { "cpc/CPC_128k_AMSDOS.cfg",
    CPC_RAM_128K | CPC_ROM_6128 | CPC_ROM_AMSDOS
  },
  { "cpc/CPC_576k.cfg",
    CPC_RAM_576K | CPC_ROM_6128
  },
  { "cpc/CPC_576k_AMSDOS.cfg",
    CPC_RAM_576K | CPC_ROM_6128 | CPC_ROM_AMSDOS
  },
  { (char *) 0,
    0UL
  }
};

// ----------------------------------------------------------------------------

Ep128EmuMachineConfiguration::Ep128EmuMachineConfiguration(
    Ep128Emu::ConfigurationDB& config, uint64_t machineConfig,
    const std::string& romDirectory)
{
  sdext.romFile.clear();
  sdext.enabled = false;
  if (!IS_EP_CONFIG(machineConfig)) {
    memory.ram.size = ZX_CPC_RAM_SIZE(machineConfig);
    if (IS_ZX_CONFIG(machineConfig)) {
      try {
        config["display.quality"] = 1;
      }
      catch (Ep128Emu::Exception) {
      }
      if (!(machineConfig & ZX_ROM_ZX128)) {    // Spectrum 16 or 48
        vm.videoClockFrequency = 875000U;
        memory.rom[0].file = romDirectory
                             + ((machineConfig & ZX_ROM_ZX48GW03) ?
                                "zx48gw03.rom" : "zx48.rom");
      }
      else {                                    // Spectrum 128
        vm.videoClockFrequency = 886724U;
        memory.rom[0].file = romDirectory + "zx128.rom";
        memory.rom[1].file = romDirectory + "zx128.rom";
        memory.rom[1].offset = 16384;
      }
      vm.soundClockFrequency = vm.videoClockFrequency >> 2;
      vm.enableMemoryTimingEmulation = true;
      vm.enableFileIO = bool(machineConfig & ZX_ENABLE_FILEIO);
    }
    else {
      try {
        config["display.quality"] = 3;
        config["display.lineShade"] = 0.5;
        config["sound.volume"] = 0.5;
      }
      catch (Ep128Emu::Exception) {
      }
      vm.videoClockFrequency = 1000000U;
      if (machineConfig & CPC_ROM_464)          // CPC 64K
        memory.rom[0].file = romDirectory + "cpc464.rom";
      else if (machineConfig & CPC_ROM_664)     // CPC 64K (AMSDOS)
        memory.rom[0].file = romDirectory + "cpc664.rom";
      else if (machineConfig & CPC_ROM_6128)    // CPC 128K or 576K
        memory.rom[0].file = romDirectory + "cpc6128.rom";
      if (machineConfig & CPC_ROM_AMSDOS)
        memory.rom[7].file = romDirectory + "cpc_amsdos.rom";
      memory.rom[0].offset = 16384;
      memory.rom[16].file = memory.rom[0].file;
      vm.soundClockFrequency = vm.videoClockFrequency >> 3;
      vm.enableMemoryTimingEmulation = true;
      vm.enableFileIO = false;
    }
    vm.cpuClockFrequency = vm.videoClockFrequency << 2;
  }
  else {                                // Enterprise
    vm.cpuClockFrequency = 4000000U;
    vm.videoClockFrequency = 889846U;
    memory.ram.size = EP_RAM_SIZE(machineConfig);
    for (int i = 0; i < 61; i++) {
      if (machineConfig & (uint64_t(1) << (i + 3))) {
        unsigned long tmp = epROMFiles[i].segments;
        for (int j = 0; j < 4 && tmp < 0xFF000000UL; j++) {
          unsigned int  segment = (unsigned int) ((tmp >> 24) & 0x7FUL);
          memory.rom[segment].file = romDirectory + epROMFiles[i].fileName;
          memory.rom[segment].offset = j << 14;
          if (tmp >= 0x80000000UL) {
            sdext.enabled = true;
            sdext.romFile = memory.rom[segment].file;
          }
          tmp = (tmp & 0x00FFFFFFUL) << 8;
        }
      }
    }
    vm.soundClockFrequency = vm.cpuClockFrequency >> 3;
    vm.enableMemoryTimingEmulation = true;
    vm.enableFileIO =
        ((machineConfig & (EP_ROM_EPFILEIO | EP_ROM_IDE12_42
                           | EP_ROM_SDEXT05_07)) == EP_ROM_EPFILEIO);
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
  if (!IS_ZX_CONFIG(machineConfig)) {
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
  if (IS_EP_CONFIG(machineConfig)) {
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
    config.createKey("sdext.romFile", sdext.romFile);
    config.createKey("sdext.enabled", sdext.enabled);
  }
}

class Ep128EmuDisplaySndConfiguration {
 private:
    struct {
      int         quality;
      double      lineShade;
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
    display.lineShade = 0.75;
    sound.highQuality = true;
    sound.latency = 0.07;
    sound.hwPeriods = 16;
    sound.volume = 0.7071;
    config.createKey("display.quality", display.quality);
    config.createKey("display.lineShade", display.lineShade);
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

struct CurlRecvUserData {
  Ep128EmuConfigInstallerGUI    *gui;
  std::vector< unsigned char >  *inBuf;
  bool    errorFlag;
  char    windowTitleBuf[64];
};

size_t Ep128EmuConfigInstallerGUI::curlRecvCallback(void *buf, size_t size,
                                                    size_t n, void *userData)
{
#ifdef MAKECFG_USE_CURL
  CurlRecvUserData&             userData_ =
      *(reinterpret_cast< CurlRecvUserData * >(userData));
  Ep128EmuConfigInstallerGUI&   gui = *(userData_.gui);
  std::vector< unsigned char >& inBuf = *(userData_.inBuf);
  size_t  nBytes = n * size;
  if ((inBuf.size() + nBytes) > 0x00100000)
    userData_.errorFlag = true;
  if (nBytes < 1 || userData_.errorFlag)
    return 0;
  try {
    std::sprintf(userData_.windowTitleBuf, "Downloaded %lu bytes\n",
                 (unsigned long) (inBuf.size() + nBytes));
    gui.mainWindow->label(userData_.windowTitleBuf);
    Fl::wait(0.0);
    if (gui.cancelButtonPressed) {
      userData_.errorFlag = true;
      inBuf.clear();
      return 0;
    }
    inBuf.resize(inBuf.size() + nBytes);
    std::memcpy(&(inBuf.front()) + (inBuf.size() - nBytes), buf, nBytes);
    return nBytes;
  }
  catch (std::exception) {
    userData_.errorFlag = true;
    inBuf.clear();
    return 0;
  }
#else
  (void) buf;
  (void) size;
  (void) n;
  (void) userData;
  return 0;
#endif
}

// returns true if the compressed ROM package file can be deleted

bool Ep128EmuConfigInstallerGUI::unpackROMFiles(const std::string& romDir,
                                                double& decompressTime)
{
  std::vector< unsigned char >  inBuf;
  std::string fName;
  std::FILE   *f = (std::FILE *) 0;
  bool        usingLocalFile = true;
  {
    fName = romDir;
    // assume there is a path delimiter character at the end of 'romDir'
    fName += MAKECFG_ROM_PKG_NAME;
    f = std::fopen(fName.c_str(), "rb");
#if defined(__linux) || defined(__linux__)
    if (!f && !enableROMDownload) {
      usingLocalFile = false;
      f = std::fopen("/usr/share/ep128emu/roms/" MAKECFG_ROM_PKG_NAME, "rb");
    }
#endif
  }
  if (f) {
    // read input file
    try {
      while (true) {
        int     c = std::fgetc(f);
        if (c == EOF)
          break;
        if (inBuf.size() >= 0x00100000)
          throw Ep128Emu::Exception("invalid compressed ROM data size");
        inBuf.push_back((unsigned char) (c & 0xFF));
      }
    }
    catch (...) {
      std::fclose(f);
      throw;
    }
    std::fclose(f);
    f = (std::FILE *) 0;
  }
#ifdef MAKECFG_USE_CURL
  else if (enableROMDownload) {
    // use cURL to download the ROM package if enabled
    usingLocalFile = false;
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
      throw Ep128Emu::Exception("Error initializing libcurl");
    CURL    *curl_ = (CURL *) 0;
    for (int i = 0; i < 2; i++) {
      const char  *romURL = (i == 0 ? MAKECFG_ROM_URL_1 : MAKECFG_ROM_URL_2);
      try {
        CurlRecvUserData  curlCallbackUserData;
        curlCallbackUserData.gui = this;
        curlCallbackUserData.inBuf = &inBuf;
        curlCallbackUserData.errorFlag = false;
        curlCallbackUserData.windowTitleBuf[0] = '\0';
        curl_ = curl_easy_init();
        if (!curl_)
          throw Ep128Emu::Exception("Error initializing cURL download");
        if (curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION,
                             &curlRecvCallback) != CURLE_OK ||
            curl_easy_setopt(curl_, CURLOPT_WRITEDATA,
                             &curlCallbackUserData) != CURLE_OK ||
            curl_easy_setopt(curl_, CURLOPT_URL, romURL) != CURLE_OK) {
          throw Ep128Emu::Exception("Error configuring cURL download");
        }
        inBuf.clear();
        CURLcode err = curl_easy_perform(curl_);
        mainWindow->label("Install ep128emu configuration files");
        if (err != CURLE_OK || curlCallbackUserData.errorFlag ||
            inBuf.size() < 100000) {
          if (cancelButtonPressed) {
            i = 1;
            throw Ep128Emu::Exception("Download cancelled");
          }
          throw Ep128Emu::Exception(i == 0 ?
                                    "Error downloading " MAKECFG_ROM_URL_1
                                    : "Error downloading " MAKECFG_ROM_URL_2);
        }
        curl_easy_cleanup(curl_);
        curl_ = (CURL *) 0;
        break;
      }
      catch (...) {
        if (curl_) {
          curl_easy_cleanup(curl_);
          curl_ = (CURL *) 0;
          if (i == 0) {
            errorMessage("Error downloading from http://ep128.hu, "
                         "trying https://enterpriseforever.com instead");
            continue;
          }
        }
        curl_global_cleanup();
        throw;
      }
    }
    curl_global_cleanup();
  }
#endif
  else {
    return false;
  }
  try {
    std::vector< unsigned char >  buf;
    // read and decompress input file
    {
      if (inBuf.size() < 4)
        throw Ep128Emu::Exception("invalid compressed ROM data size");
      Ep128Emu::Timer   tt;
      double  t0 = tt.getRealTime();
      double  t1, t2;
      do {
        buf.clear();
        t1 = tt.getRealTime();
        Ep128Emu::decompressData(buf, &(inBuf.front()), inBuf.size());
        t2 = tt.getRealTime();
        t1 = t2 - t1;
        if (t1 > 0.0 && t1 < decompressTime)
          decompressTime = t1;
      } while ((t2 - t0) < 0.35);
      if (buf.size() < 0x4000 || buf.size() > 0x00300000)
        throw Ep128Emu::Exception("invalid packed ROM data size");
    }
    // get the number of ROM files in the package
    size_t  nFiles = (size_t(buf[0]) << 24) | (size_t(buf[1]) << 16)
                     | (size_t(buf[2]) << 8) | size_t(buf[3]);
    if (nFiles < 1 || nFiles > 128)
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
    if (f)
      std::fclose(f);
    throw;
  }
  return usingLocalFile;
}

// ----------------------------------------------------------------------------

static void saveConfigurationFile(Ep128Emu::ConfigurationDB& config,
                                  const std::string& dirName,
                                  const char *fileName,
                                  Ep128EmuConfigInstallerGUI& gui)
{
  try {
    std::string fullName = dirName;
    fullName += fileName;
#ifdef WIN32
    try
#endif
    {
      config.saveState(fullName.c_str(), false);
    }
#ifdef WIN32
    catch (Ep128Emu::Exception& e) {
      if (std::strncmp(e.what(), "error opening ", 14) == 0) {
        // hack to work around errors due to lack of write access to
        // Program Files if makecfg is run as a normal user; if the
        // file already exists, then the error is ignored
        std::FILE *f = std::fopen(fullName.c_str(), "rb");
        if (!f)
          throw;
        else
          std::fclose(f);
      }
      else {
        throw;
      }
    }
#endif
  }
  catch (std::exception& e) {
    gui.errorMessage(e.what());
  }
}

int main(int argc, char **argv)
{
  Fl::lock();
#ifndef WIN32
  Ep128Emu::setGUIColorScheme(0);
#else
  Ep128Emu::setGUIColorScheme(1);
#endif
  bool    forceInstallFlag = false;
  bool    defaultCfgInstall = false;
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
        if (argv[i][1] == 'c' && argv[i][2] == '\0') {
          defaultCfgInstall = true;
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
#else
    {
      // try to get installation directory from registry
      char    installDir[256];
      HKEY    regKey = 0;
      DWORD   regType = 0;
      DWORD   bufSize = 256;
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       "Software\\ep128emu2\\InstallDirectory", 0,
                       KEY_QUERY_VALUE | KEY_WOW64_32KEY, &regKey)
          == ERROR_SUCCESS) {
        if (RegQueryValueEx(regKey, "", (LPDWORD) 0, &regType,
                            (LPBYTE) installDir, &bufSize)
            == ERROR_SUCCESS && regType == REG_SZ && bufSize < 256) {
          installDir[bufSize] = '\0';
          tmp = installDir;
        }
        RegCloseKey(regKey);
      }
    }
#endif
#if defined(__linux) || defined(__linux__)
    // use FLTK file chooser to work around bugs in the new 1.3.3 GTK chooser
    Fl_File_Chooser *w =
        new Fl_File_Chooser(
                tmp.c_str(), "*",
                Fl_File_Chooser::CREATE | Fl_File_Chooser::DIRECTORY,
                "Select installation directory for ep128emu data files");
    w->show();
    do {
      Fl::wait(0.05);
    } while (w->shown());
    if (w->value() && w->value()[0] != '\0')
      installDirectory = w->value();
    delete w;
#else
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
#endif
  }
  Ep128Emu::stripString(installDirectory);
  if (installDirectory.length() == 0)
    return -1;
  Ep128EmuConfigInstallerGUI  *gui = new Ep128EmuConfigInstallerGUI();
  Ep128Emu::setWindowIcon(gui->mainWindow, 11);
  Ep128Emu::setWindowIcon(gui->errorWindow, 12);
  if (!forceInstallFlag) {
    try {
      Ep128Emu::File  f("ep128cfg.dat", true);
    }
    catch (...) {
      defaultCfgInstall = true;
    }
    gui->enableCfgInstall = defaultCfgInstall;
    gui->cfgInstallValuator->value(int(defaultCfgInstall));
#ifdef MAKECFG_USE_CURL
    gui->romDownloadValuator->activate();
    gui->romDownloadLabel->activate();
#endif
    gui->mainWindow->show();
    do {
      Fl::wait(0.05);
    } while (gui->mainWindow->shown() &&
             !(gui->okButtonPressed || gui->cancelButtonPressed));
    if (gui->cancelButtonPressed) {
      delete gui;
      return 0;
    }
  }
  else {
    gui->enablePresetCfgInstall = true;
    gui->enableCfgInstall = true;
  }
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
  bool    compressFiles = false;
  if (gui->enablePresetCfgInstall | gui->enableROMDownload) {
    try {
      double  decompressTime = 1.0;
      if (gui->unpackROMFiles(romDirectory, decompressTime)) {
        // if successfully extracted, delete compressed ROM package
        std::string tmp(romDirectory);
        tmp += MAKECFG_ROM_PKG_NAME;
        std::FILE *f = std::fopen(tmp.c_str(), "r+b");
        if (f) {
          std::fclose(f);
          std::remove(tmp.c_str());
        }
      }
      // enable snapshot compression by default on fast machines
      compressFiles = (decompressTime < 0.023);
    }
    catch (std::exception& e) {
      gui->errorMessage(e.what());
    }
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
        // EP_128k_EXDOS
        uint64_t    machineConfig = EP_RAM_128K | EP_ROM_EXOS21
                                    | EP_ROM_BASIC21_04 | EP_ROM_EXDOS14I_UK;
        if (i == 1) {
          fName = "zx128cfg.dat";
          // ZX_128k
          machineConfig = ZX_RAM_128K | ZX_ROM_ZX128;
        }
        else if (i == 2) {
          fName = "cpc_cfg.dat";
          // CPC_128k_AMSDOS.cfg
          machineConfig = CPC_RAM_128K | CPC_ROM_6128 | CPC_ROM_AMSDOS;
        }
        config = new Ep128Emu::ConfigurationDB();
        dsCfg = new Ep128EmuDisplaySndConfiguration(*config);
        mCfg = new Ep128EmuMachineConfiguration(*config, machineConfig,
                                                romDirectory);
        setKeyboardConfiguration(*config, (gui->keyboardMapHU ? 1 : 0));
        std::string fileIODir = installDirectory + "files";
        config->createKey("fileio.workingDirectory", fileIODir);
        if (compressFiles) {
          config->createKey("compressFiles", compressFiles);
          (*config)["display.quality"] = 3;
        }
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
    if (!(gui->enablePresetCfgInstall)) {
      delete gui;
      return 0;
    }
    for (int i = 0; machineConfigs[i].fileName; i++) {
      config = new Ep128Emu::ConfigurationDB();
      mCfg = new Ep128EmuMachineConfiguration(*config, machineConfigs[i].config,
                                              romDirectory);
      saveConfigurationFile(*config,
                            configDirectory, machineConfigs[i].fileName, *gui);
      delete config;
      delete mCfg;
      config = (Ep128Emu::ConfigurationDB *) 0;
      mCfg = (Ep128EmuMachineConfiguration *) 0;
    }
    for (int i = 0; i < 8; i++) {
      if (keyboardConfigFileNames[i] != (char *) 0) {
        config = new Ep128Emu::ConfigurationDB();
        setKeyboardConfiguration(*config, i);
        saveConfigurationFile(*config,
                              configDirectory, keyboardConfigFileNames[i],
                              *gui);
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

