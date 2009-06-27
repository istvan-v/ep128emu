
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

#ifndef EP128EMU_EMUCFG_HPP
#define EP128EMU_EMUCFG_HPP

#include "ep128emu.hpp"
#include "cfg_db.hpp"
#include "display.hpp"
#include "joystick.hpp"
#include "soundio.hpp"
#include "vm.hpp"

#include <map>

namespace Ep128Emu {

  class EmulatorConfiguration : public ConfigurationDB {
   private:
    VirtualMachine& vm_;
    VideoDisplay&   videoDisplay;
    AudioOutput&    audioOutput;
    std::map< int, int >  keyboardMap;
    void            (*errorCallback)(void *, const char *);
    void            *errorCallbackUserData;
   public:
    struct {
      unsigned int  cpuClockFrequency;
      unsigned int  videoClockFrequency;
      unsigned int  soundClockFrequency;
      unsigned int  speedPercentage;    // NOTE: this uses soundSettingsChanged
      int           processPriority;    // uses vmProcessPriorityChanged
      bool          enableMemoryTimingEmulation;
      bool          enableFileIO;
    } vm;
    bool          vmConfigurationChanged;
    bool          vmProcessPriorityChanged;
    // --------
    struct MemoryConfiguration_ {
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
    };
    MemoryConfiguration_  memory;
    bool          memoryConfigurationChanged;
    // --------
    struct {
      bool        enabled;
      int         bufferingMode;
      int         quality;
      double      brightness;
      double      contrast;
      double      gamma;
      double      hueShift;
      double      saturation;
      struct {
        double    brightness;
        double    contrast;
        double    gamma;
      } red;
      struct {
        double    brightness;
        double    contrast;
        double    gamma;
      } green;
      struct {
        double    brightness;
        double    contrast;
        double    gamma;
      } blue;
      double      lineShade;
      double      blendScale;
      double      motionBlur;
      int         width;
      int         height;
      double      pixelAspectRatio;
    } display;
    bool          displaySettingsChanged;
    // --------
    struct SoundConfiguration_ {
      bool        enabled;
      bool        highQuality;
      int         device;
      double      sampleRate;
      double      latency;
      int         hwPeriods;
      int         swPeriods;
      std::string file;
      double      volume;
      double      dcBlockFilter1Freq;
      double      dcBlockFilter2Freq;
      struct {
        int       mode;
        double    frequency;
        double    level;
        double    q;
      } equalizer;
    };
    SoundConfiguration_   sound;
    bool          soundSettingsChanged;
    // --------
    int           keyboard[128][2];
    bool          keyboardMapChanged;
    // --------
    JoystickInput::JoystickConfiguration  joystick;
    bool          joystickSettingsChanged;
    // --------
    struct FloppyDriveSettings {
      std::string imageFile;
      int         tracks;
      int         sides;
      int         sectorsPerTrack;
      FloppyDriveSettings()
        : imageFile(""),
          tracks(-1),
          sides(2),
          sectorsPerTrack(9)
      {
      }
    };
    struct FloppyConfiguration_ {
      FloppyDriveSettings a;
      FloppyDriveSettings b;
      FloppyDriveSettings c;
      FloppyDriveSettings d;
    };
    FloppyConfiguration_  floppy;
    bool          floppyAChanged;
    bool          floppyBChanged;
    bool          floppyCChanged;
    bool          floppyDChanged;
    // --------
    struct IDEConfiguration_ {
      std::string imageFile0;
      std::string imageFile1;
      std::string imageFile2;
      std::string imageFile3;
    };
    IDEConfiguration_     ide;
    bool          ideDisk0Changed;
    bool          ideDisk1Changed;
    bool          ideDisk2Changed;
    bool          ideDisk3Changed;
    // --------
    struct TapeConfiguration_ {
      std::string imageFile;
      int         defaultSampleRate;
      int         soundFileChannel;
      bool        enableSoundFileFilter;
      double      soundFileFilterMinFreq;
      double      soundFileFilterMaxFreq;
    };
    TapeConfiguration_    tape;
    bool          tapeFileChanged;
    bool          tapeSettingsChanged;
    bool          tapeSoundFileSettingsChanged;
    // --------
    struct FileIOConfiguration_ {
      std::string workingDirectory;
    };
    FileIOConfiguration_  fileio;
    bool          fileioSettingsChanged;
    // --------
    struct {
      int         bpPriorityThreshold;
    } debug;
    bool          debugSettingsChanged;
    // --------
    struct {
      int         frameRate;
      bool        yuvFormat;
    } videoCapture;
    bool          videoCaptureSettingsChanged;
    // ----------------
    EmulatorConfiguration(VirtualMachine& vm__,
                          VideoDisplay& videoDisplay_,
                          AudioOutput& audioOutput_);
    virtual ~EmulatorConfiguration();
    void applySettings();
    int convertKeyCode(int keyCode);
    void setErrorCallback(void (*func)(void *userData, const char *msg),
                          void *userData_);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_EMUCFG_HPP

