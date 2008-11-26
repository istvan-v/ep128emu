
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2008 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifdef WIN32
#  undef WIN32
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  include <direct.h>
#  define WIN32 1
#else
#  include <sys/types.h>
#  include <sys/stat.h>
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
  0xFF13, 0xFF50, 0xFF51,     -1, 0xFF0D,     -1, 0xFFEA, 0x00E1,
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
  0xFF13, 0xFF50, 0xFF51,     -1, 0xFF0D,     -1, 0xFFEA, 0x00E1,
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
    // 0x20 to 0x23, and 0x30 to 0x33
    ROMSegmentConfig  rom[52];
  } memory;
 public:
  Ep128EmuMachineConfiguration(Ep128Emu::ConfigurationDB& config, int n,
                               const std::string& romDirectory);
  ~Ep128EmuMachineConfiguration()
  {
  }
};

static const char *machineConfigFileNames[] = {
  "EP_64k_Tape.cfg",                            // 0
  "EP_64k_Tape_NoCartridge.cfg",                // 1
  "EP_64k_Tape_NoCartridge_FileIO.cfg",         // 2
  "EP_64k_Tape_FileIO.cfg",                     // 3
  "EP_64k_Tape_FileIO_TASMON.cfg",              // 4
  "EP_64k_Tape_TASMON.cfg",                     // 5
  "EP_64k_EXDOS.cfg",                           // 6
  "EP_64k_EXDOS_NoCartridge.cfg",               // 7
  "EP_64k_EXDOS_NoCartridge_FileIO.cfg",        // 8
  "EP_64k_EXDOS_FileIO.cfg",                    // 9
  "EP_64k_EXDOS_FileIO_TASMON.cfg",             // 10
  "EP_64k_EXDOS_TASMON.cfg",                    // 11
  "EP_128k_Tape.cfg",                           // 12
  "EP_128k_Tape_NoCartridge.cfg",               // 13
  "EP_128k_Tape_NoCartridge_FileIO.cfg",        // 14
  "EP_128k_Tape_FileIO.cfg",                    // 15
  "EP_128k_Tape_FileIO_TASMON.cfg",             // 16
  "EP_128k_Tape_TASMON.cfg",                    // 17
  "EP_128k_EXDOS.cfg",                          // 18
  "EP_128k_EXDOS_NoCartridge.cfg",              // 19
  "EP_128k_EXDOS_NoCartridge_FileIO.cfg",       // 20
  "EP_128k_EXDOS_FileIO.cfg",                   // 21
  "EP_128k_EXDOS_FileIO_TASMON.cfg",            // 22
  "EP_128k_EXDOS_TASMON.cfg",                   // 23
  "EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg",  // 24
  "EP_320k_Tape.cfg",                           // 25
  "EP_320k_Tape_FileIO.cfg",                    // 26
  "EP_320k_Tape_FileIO_TASMON.cfg",             // 27
  "EP_320k_Tape_TASMON.cfg",                    // 28
  "EP_320k_EXDOS.cfg",                          // 29
  "EP_320k_EXDOS_FileIO.cfg",                   // 30
  "EP_320k_EXDOS_FileIO_TASMON.cfg",            // 31
  "EP_320k_EXDOS_TASMON.cfg",                   // 32
  "EP_640k_EXOS23_EXDOS_utils.cfg"              // 33
};

// bits 0..7: number of RAM segments
// bit 8:     -
// bit 9:     -
// bit 10:    EXOS 2.0 (exos20.rom at segments 0x00, 0x01)
// bit 11:    EXOS 2.1 (exos21.rom at segments 0x00, 0x01)
// bit 12:    EXOS 2.2 (exos22.rom at segments 0x00, 0x01, 0x02, 0x03)
// bit 13:    EXOS 2.31 (exos231.rom at segments 0x00, 0x01, 0x02, 0x03)
// bit 14:    BASIC 2.0 (basic20.rom at segment 0x04)
// bit 15:    BASIC 2.0 (basic20.rom at segment 0x06)
// bit 16:    BASIC 2.1 (basic21.rom at segment 0x04)
// bit 17:    BASIC 2.1 (basic21.rom at segment 0x06)
// bit 18:    EXDOS 1.0 (exdos10.rom at segment 0x20)
// bit 19:    EXDOS 1.3 (exdos13.rom at segments 0x20, 0x21)
// bit 20:    EPDOS (epdos_z.rom at segments 0x10, 0x11)
// bit 21:    ASMEN (asmen15.rom at segments 0x04, 0x05)
// bit 22:    FENAS (fenas12.rom at segments 0x06, 0x07)
// bit 23:    HEASS (heass10.rom at segments 0x04, 0x05)
// bit 24:    TASMON (tasmon15.rom at segments 0x04, 0x05)
// bit 25:    ZOZOTOOLS (zt18.rom at segments 0x30, 0x31)
// bit 26:    ZX (zx41.rom at segments 0x30, 0x31)
// bit 27:    FILEIO (epfileio.rom at segment 0x10)
// bit 28:    -
// bit 29:    -
// bit 30:    TASMON (tasmon15.rom at segments 0x05, 0x06)
// bit 31:    -

static const unsigned long machineConfigs[] = {
  0x00004404UL,         // EP_64k_Tape.cfg
  0x00000404UL,         // EP_64k_Tape_NoCartridge.cfg
  0x08000404UL,         // EP_64k_Tape_NoCartridge_FileIO.cfg
  0x08004404UL,         // EP_64k_Tape_FileIO.cfg
  0x48004404UL,         // EP_64k_Tape_FileIO_TASMON.cfg
  0x40004404UL,         // EP_64k_Tape_TASMON.cfg
  0x00044404UL,         // EP_64k_EXDOS.cfg
  0x00040404UL,         // EP_64k_EXDOS_NoCartridge.cfg
  0x08040404UL,         // EP_64k_EXDOS_NoCartridge_FileIO.cfg
  0x08044404UL,         // EP_64k_EXDOS_FileIO.cfg
  0x48044404UL,         // EP_64k_EXDOS_FileIO_TASMON.cfg
  0x40044404UL,         // EP_64k_EXDOS_TASMON.cfg
  0x00010808UL,         // EP_128k_Tape.cfg
  0x00000808UL,         // EP_128k_Tape_NoCartridge.cfg
  0x08000808UL,         // EP_128k_Tape_NoCartridge_FileIO.cfg
  0x08010808UL,         // EP_128k_Tape_FileIO.cfg
  0x09020808UL,         // EP_128k_Tape_FileIO_TASMON.cfg
  0x01020808UL,         // EP_128k_Tape_TASMON.cfg
  0x00090808UL,         // EP_128k_EXDOS.cfg
  0x00080808UL,         // EP_128k_EXDOS_NoCartridge.cfg
  0x08080808UL,         // EP_128k_EXDOS_NoCartridge_FileIO.cfg
  0x08090808UL,         // EP_128k_EXDOS_FileIO.cfg
  0x090A0808UL,         // EP_128k_EXDOS_FileIO_TASMON.cfg
  0x010A0808UL,         // EP_128k_EXDOS_TASMON.cfg
  0x0C090808UL,         // EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg
  0x00010814UL,         // EP_320k_Tape.cfg
  0x08010814UL,         // EP_320k_Tape_FileIO.cfg
  0x09020814UL,         // EP_320k_Tape_FileIO_TASMON.cfg
  0x01020814UL,         // EP_320k_Tape_TASMON.cfg
  0x00090814UL,         // EP_320k_EXDOS.cfg
  0x08090814UL,         // EP_320k_EXDOS_FileIO.cfg
  0x090A0814UL,         // EP_320k_EXDOS_FileIO_TASMON.cfg
  0x010A0814UL,         // EP_320k_EXDOS_TASMON.cfg
  0x02782028UL          // EP_640k_EXOS23_EXDOS_utils.cfg
};

static const char *romFileNames[24] = {
  (char *) 0,
  (char *) 0,
  "exos20.rom",
  "exos21.rom",
  "exos22.rom",
  "exos231.rom",
  "basic20.rom",
  "basic20.rom",
  "basic21.rom",
  "basic21.rom",
  "exdos10.rom",
  "exdos13.rom",
  "epdos_z.rom",
  "asmen15.rom",
  "fenas12.rom",
  "heass10.rom",
  "tasmon15.rom",
  "zt18.rom",
  "zx41.rom",
  "epfileio.rom",
  (char *) 0,
  (char *) 0,
  "tasmon15.rom",
  (char *) 0
};

static const unsigned long romFileSegments[24] = {
  0xFFFFFFFFUL,         // -
  0xFFFFFFFFUL,         // -
  0xFFFF0100UL,         // exos20.rom
  0xFFFF0100UL,         // exos21.rom
  0x03020100UL,         // exos22.rom
  0x03020100UL,         // exos231.rom
  0xFFFFFF04UL,         // basic20.rom
  0xFFFFFF06UL,         // basic20.rom
  0xFFFFFF04UL,         // basic21.rom
  0xFFFFFF06UL,         // basic21.rom
  0xFFFFFF20UL,         // exdos10.rom
  0xFFFF2120UL,         // exdos13.rom
  0xFFFF1110UL,         // epdos_z.rom
  0xFFFF0504UL,         // asmen15.rom
  0xFFFF0706UL,         // fenas12.rom
  0xFFFF0504UL,         // heass10.rom
  0xFFFF0504UL,         // tasmon15.rom
  0xFFFF3130UL,         // zt18.rom
  0xFFFF3130UL,         // zx41.rom
  0xFFFFFF10UL,         // epfileio.rom
  0xFFFFFFFFUL,         // -
  0xFFFFFFFFUL,         // -
  0xFFFF0605UL,         // tasmon15.rom
  0xFFFFFFFFUL          // -
};

Ep128EmuMachineConfiguration::Ep128EmuMachineConfiguration(
    Ep128Emu::ConfigurationDB& config, int n, const std::string& romDirectory)
{
  vm.cpuClockFrequency = 4000000U;
  vm.videoClockFrequency = 890625U;
  vm.soundClockFrequency = 500000U;
  vm.enableMemoryTimingEmulation = true;
  memory.ram.size = int((machineConfigs[n] & 0xFFUL) << 4);
  for (int i = 0; i < 24; i++) {
    if (machineConfigs[n] & (1UL << (i + 8))) {
      unsigned long tmp = romFileSegments[i];
      for (int j = 0; j < 4 && (tmp & 0xFFUL) < 0x80UL; j++) {
        memory.rom[tmp & 0xFFUL].file = romDirectory + romFileNames[i];
        memory.rom[tmp & 0xFFUL].offset = j << 14;
        tmp = tmp >> 8;
      }
    }
  }
  config.createKey("vm.cpuClockFrequency", vm.cpuClockFrequency);
  config.createKey("vm.videoClockFrequency", vm.videoClockFrequency);
  config.createKey("vm.soundClockFrequency", vm.soundClockFrequency);
  config.createKey("vm.enableMemoryTimingEmulation",
                   vm.enableMemoryTimingEmulation);
  config.createKey("memory.ram.size", memory.ram.size);
  config.createKey("memory.rom.00.file", memory.rom[0x00].file);
  config.createKey("memory.rom.00.offset", memory.rom[0x00].offset);
  config.createKey("memory.rom.01.file", memory.rom[0x01].file);
  config.createKey("memory.rom.01.offset", memory.rom[0x01].offset);
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
}

class Ep128EmuDisplaySndConfiguration {
 private:
    struct {
      int         quality;
    } display;
    struct {
      double      latency;
      int         hwPeriods;
    } sound;
 public:
  Ep128EmuDisplaySndConfiguration(Ep128Emu::ConfigurationDB& config)
  {
    display.quality = 2;
#ifndef WIN32
    sound.latency = 0.05;
#else
    sound.latency = 0.1;
#endif
    sound.hwPeriods = 16;
    config.createKey("display.quality", display.quality);
    config.createKey("sound.latency", sound.latency);
    config.createKey("sound.hwPeriods", sound.hwPeriods);
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

int main(int argc, char **argv)
{
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
    tmp2 = tmp + "demo";
    mkdir(tmp2.c_str(), 0755);
    tmp2 = tmp + "disk";
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
    tmp2 = tmp + "demo";
    _mkdir(tmp2.c_str());
    tmp2 = tmp + "disk";
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
      config = new Ep128Emu::ConfigurationDB();
      mCfg = new Ep128EmuMachineConfiguration(*config, 18, romDirectory);
      dsCfg = new Ep128EmuDisplaySndConfiguration(*config);
      setKeyboardConfiguration(*config, (gui->keyboardMapHU ? 1 : 0));
      try {
        Ep128Emu::File  f;
        config->saveState(f);
        f.writeFile("ep128cfg.dat", true);
      }
      catch (std::exception& e) {
        gui->errorMessage(e.what());
      }
      delete config;
      delete mCfg;
      delete dsCfg;
      config = (Ep128Emu::ConfigurationDB *) 0;
      mCfg = (Ep128EmuMachineConfiguration *) 0;
    }
    for (int i = 0;
         i < int(sizeof(machineConfigs) / sizeof(unsigned long));
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

