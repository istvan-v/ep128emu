
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

#include "gui.hpp"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cmath>

static void cfgErrorFunc(void *userData, const char *msg)
{
  (void) userData;
  std::cerr << "WARNING: " << msg << std::endl;
}

static void plus4ClockFreqChangeCallback(void *userData,
                                         const std::string& name,
                                         unsigned int value)
{
  (void) name;
  Ep128Emu::EmulatorConfiguration&  cfg =
      *(reinterpret_cast<Ep128Emu::EmulatorConfiguration *>(userData));
  if (value == cfg.vm.videoClockFrequency)
    cfg.vmConfigurationChanged = true;
  cfg.vm.soundClockFrequency = (cfg.vm.videoClockFrequency + 2U) >> 2;
}

int main(int argc, char **argv)
{
  Fl_Window *w = (Fl_Window *) 0;
  Ep128Emu::VirtualMachine  *vm = (Ep128Emu::VirtualMachine *) 0;
  Ep128Emu::AudioOutput     *audioOutput = (Ep128Emu::AudioOutput *) 0;
  Ep128Emu::EmulatorConfiguration *config =
      (Ep128Emu::EmulatorConfiguration *) 0;
  Ep128Emu::VMThread        *vmThread = (Ep128Emu::VMThread *) 0;
  Ep128EmuGUI               *gui_ = (Ep128EmuGUI *) 0;
  bool      isPlus4 = false;
  bool      glEnabled = true;
  const char  *cfgFileName = "ep128cfg.dat";
  int       retval = 0;
  bool      configLoaded = false;

  try {
    // find out machine type to be emulated
    for (int i = 1; i < argc; i++) {
      if (std::strcmp(argv[i], "-cfg") == 0 && i < (argc - 1)) {
        i++;
      }
      else if (std::strcmp(argv[i], "-ep128") == 0) {
        isPlus4 = false;
        cfgFileName = "ep128cfg.dat";
      }
      else if (std::strcmp(argv[i], "-plus4") == 0) {
        isPlus4 = true;
        cfgFileName = "plus4cfg.dat";
      }
      else if (std::strcmp(argv[i], "-opengl") == 0) {
        glEnabled = true;
      }
      else if (std::strcmp(argv[i], "-no-opengl") == 0) {
        glEnabled = false;
      }
      else if (std::strcmp(argv[i], "-h") == 0 ||
               std::strcmp(argv[i], "-help") == 0 ||
               std::strcmp(argv[i], "--help") == 0) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS...]" << std::endl;
        std::cerr << "The allowed options are:" << std::endl;
        std::cerr << "    -h | -help | --help "
                     "print this message" << std::endl;
        std::cerr << "    -ep128 | -plus4     "
                     "select machine to be emulated (default: ep128)"
                  << std::endl;
        std::cerr << "    -cfg <FILENAME>     "
                     "load ASCII format configuration file" << std::endl;
        std::cerr << "    -opengl             "
                     "use OpenGL video driver (this is the default)"
                  << std::endl;
        std::cerr << "    -no-opengl          "
                     "use software video driver" << std::endl;
        std::cerr << "    OPTION=VALUE        "
                     "set configuration variable 'OPTION' to 'VALUE'"
                  << std::endl;
        std::cerr << "    OPTION              "
                     "set boolean configuration variable 'OPTION' to true"
                  << std::endl;
        return 0;
      }
    }

    Fl::lock();
    audioOutput = new Ep128Emu::AudioOutput_PortAudio();
    if (glEnabled)
      w = new Ep128Emu::OpenGLDisplay(32, 32, 384, 288, "");
    else
      w = new Ep128Emu::FLTKDisplay(32, 32, 384, 288, "");
    w->end();
    if (!isPlus4)
      vm = new Ep128::Ep128VM(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                              *audioOutput);
    else
      vm = new Plus4::Plus4VM(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                              *audioOutput);
    config = new Ep128Emu::EmulatorConfiguration(
        *vm, *(dynamic_cast<Ep128Emu::VideoDisplay *>(w)), *audioOutput);
    config->setErrorCallback(&cfgErrorFunc, (void *) 0);
    // set machine specific defaults and limits
    if (isPlus4) {
      Ep128Emu::ConfigurationDB::ConfigurationVariable  *cv;
      cv = &((*config)["vm.cpuClockFrequency"]);
      (*cv).setRange(886724.0, 150000000.0, 0.0);
      (*cv) = (unsigned int) 1773448;
      cv = &((*config)["vm.videoClockFrequency"]);
      (*cv).setRange(443364.0, 1773448.0, 4.0);
      (*cv) = (unsigned int) 886724;
      (*cv).setCallback(&plus4ClockFreqChangeCallback, config, true);
      cv = &((*config)["vm.soundClockFrequency"]);
      (*cv).setRange(110841.0, 443362.0, 0.0);
      (*cv) = (unsigned int) 221681;
      (*cv).setCallback(&plus4ClockFreqChangeCallback, config, true);
      cv = &((*config)["vm.videoMemoryLatency"]);
      (*cv).setRange(0.0, 0.0, 0.0);
      (*cv) = (unsigned int) 0;
      cv = &((*config)["memory.ram.size"]);
      (*cv).setRange(16.0, 1024.0, 16.0);
      (*cv) = int(64);
      cv = &((*config)["sound.sampleRate"]);
      (*cv).setRange(11025.0, 96000.0, 0.0);
    }
    // load base configuration (if available)
    try {
      Ep128Emu::File  f(cfgFileName, true);
      config->registerChunkType(f);
      f.processAllChunks();
    }
    catch (...) {
    }
    configLoaded = true;
    // check command line for any additional configuration
    for (int i = 1; i < argc; i++) {
      if (std::strcmp(argv[i], "-ep128") == 0 ||
          std::strcmp(argv[i], "-plus4") == 0 ||
          std::strcmp(argv[i], "-opengl") == 0 ||
          std::strcmp(argv[i], "-no-opengl") == 0)
        continue;
      if (std::strcmp(argv[i], "-cfg") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing configuration file name");
        config->loadState(argv[i], false);
      }
      else {
        const char  *s = argv[i];
        if (*s == '-')
          s++;
        if (*s == '-')
          s++;
        const char  *p = std::strchr(s, '=');
        if (!p)
          (*config)[s] = bool(true);
        else {
          std::string optName;
          while (s != p) {
            optName += (*s);
            s++;
          }
          p++;
          (*config)[optName] = p;
        }
      }
    }
    config->applySettings();
    vmThread = new Ep128Emu::VMThread(*vm);
    gui_ = new Ep128EmuGUI(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                           *audioOutput, *vm, *vmThread, *config);
    gui_->run();
  }
  catch (std::exception& e) {
    if (gui_)
      gui_->errorMessage(e.what());
    else
      std::cerr << " *** error: " << e.what() << std::endl;
    retval = -1;
  }
  if (gui_)
    delete gui_;
  if (vmThread)
    delete vmThread;
  if (config) {
    if (configLoaded) {
      try {
        Ep128Emu::File  f;
        config->saveState(f);
        f.writeFile(cfgFileName, true);
      }
      catch (...) {
      }
    }
    delete config;
  }
  if (vm)
    delete vm;
  if (w)
    delete w;
  if (audioOutput)
    delete audioOutput;
  return retval;
}
