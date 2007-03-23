
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2007 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifdef WIN32
#  undef WIN32
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  define WIN32 1
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
  0xFF61,     -1, 0xFF54,     -1, 0xFF53,     -1, 0xFF52,     -1,
  0xFF13,     -1, 0xFF51,     -1, 0xFF0D,     -1, 0xFFE9, 0xFFEA,
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
  0xFFB9,     -1, 0xFFB7,     -1, 0xFFB5,     -1, 0xFFAF,     -1,
  0xFFAB,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  0xFFB6,     -1, 0xFFB4,     -1, 0xFFB2,     -1, 0xFFB8,     -1,
  0xFF8D,     -1, 0xFFB0,     -1,     -1,     -1,     -1,     -1
};

static const char *keyboardConfigFileNames[8] = {
  "EP_Keyboard_US.cfg",         // 0
  (char *) 0,                   // 1
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
  case 1:                               // TODO: Enterprise (HU)
    keyboardMapPtr = &(keyboardMap_EP[0]);
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
    unsigned int  videoMemoryLatency;
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

static const char *machineConfigFileNames[40] = {
  "EP_64k_Tape.cfg",                    //  0
  "EP_128k_Tape.cfg",                   //  1
  "EP_256k_Tape.cfg",                   //  2
  "EP_64k_EXDOS.cfg",                   //  3
  "EP_128k_EXDOS.cfg",                  //  4
  "EP_256k_EXDOS.cfg",                  //  5
  "EP_64k_Tape_FileIO.cfg",             //  6
  "EP_128k_Tape_FileIO.cfg",            //  7
  "EP_256k_Tape_FileIO.cfg",            //  8
  "EP_64k_EXDOS_FileIO.cfg",            //  9
  "EP_128k_EXDOS_FileIO.cfg",           // 10
  "EP_256k_EXDOS_FileIO.cfg",           // 11
  "EP_64k_Tape_TASMON.cfg",             // 12
  "EP_128k_Tape_TASMON.cfg",            // 13
  "EP_256k_Tape_TASMON.cfg",            // 14
  "EP_64k_EXDOS_TASMON.cfg",            // 15
  "EP_128k_EXDOS_TASMON.cfg",           // 16
  "EP_256k_EXDOS_TASMON.cfg",           // 17
  "EP_64k_Tape_FileIO_TASMON.cfg",      // 18
  "EP_128k_Tape_FileIO_TASMON.cfg",     // 19
  "EP_256k_Tape_FileIO_TASMON.cfg",     // 20
  "EP_64k_EXDOS_FileIO_TASMON.cfg",     // 21
  "EP_128k_EXDOS_FileIO_TASMON.cfg",    // 22
  "EP_256k_EXDOS_FileIO_TASMON.cfg"     // 23
};

Ep128EmuMachineConfiguration::Ep128EmuMachineConfiguration(
    Ep128Emu::ConfigurationDB& config, int n, const std::string& romDirectory)
{
  if (n < 24) {
    vm.cpuClockFrequency = 4000000U;
    vm.videoClockFrequency = 890625U;
    vm.soundClockFrequency = 500000U;
    vm.videoMemoryLatency = 62U;
    vm.enableMemoryTimingEmulation = true;
    memory.ram.size = ((n % 3) == 0 ? 64 : ((n % 3) == 1 ? 128 : 256));
    memory.rom[0x00].file = romDirectory + "exos0.rom";
    memory.rom[0x01].file = romDirectory + "exos1.rom";
    memory.rom[0x04].file = romDirectory + "ep_basic.rom";
    if (n >= 12) {
      memory.rom[0x05].file = romDirectory + "tasmon0.rom";
      memory.rom[0x06].file = romDirectory + "tasmon1.rom";
    }
    if ((n % 12) >= 6) {
      memory.rom[0x10].file = romDirectory + "epfileio.rom";
    }
    if ((n % 6) >= 3) {
      memory.rom[0x20].file = romDirectory + "exdos0.rom";
      memory.rom[0x21].file = romDirectory + "exdos1.rom";
    }
  }
  config.createKey("vm.cpuClockFrequency", vm.cpuClockFrequency);
  config.createKey("vm.videoClockFrequency", vm.videoClockFrequency);
  config.createKey("vm.soundClockFrequency", vm.soundClockFrequency);
  config.createKey("vm.videoMemoryLatency", vm.videoMemoryLatency);
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
      double      dcBlockFilter1Freq;
      double      dcBlockFilter2Freq;
      struct {
        int       mode;
        double    frequency;
        double    level;
        double    q;
      } equalizer;
    } sound;
 public:
  Ep128EmuDisplaySndConfiguration(Ep128Emu::ConfigurationDB& config)
  {
    display.quality = 2;
    sound.latency = 0.05;
    sound.hwPeriods = 8;
    sound.dcBlockFilter1Freq = 10.0;
    sound.dcBlockFilter2Freq = 10.0;
    sound.equalizer.mode = -1;
    sound.equalizer.frequency = 1000.0;
    sound.equalizer.level = 1.0;
    sound.equalizer.q = 0.7071;
    config.createKey("display.quality", display.quality);
    config.createKey("sound.latency", sound.latency);
    config.createKey("sound.hwPeriods", sound.hwPeriods);
    config.createKey("sound.dcBlockFilter1Freq", sound.dcBlockFilter1Freq);
    config.createKey("sound.dcBlockFilter2Freq", sound.dcBlockFilter2Freq);
    config.createKey("sound.equalizer.mode", sound.equalizer.mode);
    config.createKey("sound.equalizer.frequency", sound.equalizer.frequency);
    config.createKey("sound.equalizer.level", sound.equalizer.level);
    config.createKey("sound.equalizer.q", sound.equalizer.q);
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
    std::string prgFileDirectory;
    std::string debuggerDirectory;
    std::string screenshotDirectory;
  } gui;
 public:
  Ep128EmuGUIConfiguration(Ep128Emu::ConfigurationDB& config,
                           const std::string& installDirectory)
  {
    gui.snapshotDirectory = ".";
    gui.demoDirectory = installDirectory + "demo";
    gui.soundFileDirectory = ".";
    gui.configDirectory = installDirectory + "config";
    gui.loadFileDirectory = ".";
    gui.tapeImageDirectory = installDirectory + "tape";
    gui.diskImageDirectory = installDirectory + "disk";
    gui.romImageDirectory = installDirectory + "roms";
    gui.prgFileDirectory = installDirectory + "progs";
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
    config.createKey("gui.prgFileDirectory", gui.prgFileDirectory);
    config.createKey("gui.debuggerDirectory", gui.debuggerDirectory);
    config.createKey("gui.screenshotDirectory", gui.screenshotDirectory);
  }
};

int main(int argc, char **argv)
{
  Fl::lock();
  std::string installDirectory = "";
  if (argc > 1)
    installDirectory = argv[argc - 1];
  if (installDirectory.length() == 0)
    return -1;
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
  gui->mainWindow->show();
  do {
    Fl::wait(0.05);
  } while (gui->mainWindow->shown());
  try {
    Ep128Emu::ConfigurationDB     *config = (Ep128Emu::ConfigurationDB *) 0;
    Ep128EmuMachineConfiguration  *mCfg = (Ep128EmuMachineConfiguration *) 0;
    if (gui->enableCfgInstall) {
      Ep128EmuDisplaySndConfiguration *dsCfg =
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
      mCfg = new Ep128EmuMachineConfiguration(*config, 4, romDirectory);
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
    for (int i = 0; i < 24; i++) {
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

