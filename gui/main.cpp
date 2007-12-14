
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
#include "system.hpp"
#include "guicolor.hpp"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cmath>

static void cfgErrorFunc(void *userData, const char *msg)
{
  (void) userData;
  std::cerr << "WARNING: " << msg << std::endl;
}

int main(int argc, char **argv)
{
  Fl_Window *w = (Fl_Window *) 0;
  Ep128Emu::VirtualMachine  *vm = (Ep128Emu::VirtualMachine *) 0;
  Ep128Emu::AudioOutput     *audioOutput = (Ep128Emu::AudioOutput *) 0;
  Ep128Emu::EmulatorConfiguration   *config =
      (Ep128Emu::EmulatorConfiguration *) 0;
  Ep128Emu::VMThread        *vmThread = (Ep128Emu::VMThread *) 0;
  Ep128EmuGUI               *gui_ = (Ep128EmuGUI *) 0;
  bool      glEnabled = true;
  const char  *cfgFileName = "ep128cfg.dat";
  int       snapshotNameIndex = 0;
  int       colorScheme = 0;
  int       retval = 0;
  bool      configLoaded = false;

  try {
    // find out machine type to be emulated
    for (int i = 1; i < argc; i++) {
      if (std::strcmp(argv[i], "-cfg") == 0 && i < (argc - 1)) {
        i++;
      }
      else if (std::strcmp(argv[i], "-snapshot") == 0 && i < (argc - 1)) {
        i++;
      }
      else if (std::strcmp(argv[i], "-ep128") == 0) {
        cfgFileName = "ep128cfg.dat";
      }
      else if (std::strcmp(argv[i], "-opengl") == 0) {
        glEnabled = true;
      }
      else if (std::strcmp(argv[i], "-no-opengl") == 0) {
        glEnabled = false;
      }
      else if (std::strcmp(argv[i], "-colorscheme") == 0 && i < (argc - 1)) {
        i++;
        colorScheme = int(std::atoi(argv[i]));
        colorScheme = (colorScheme >= 0 && colorScheme <= 2 ? colorScheme : 0);
      }
      else if (std::strcmp(argv[i], "-h") == 0 ||
               std::strcmp(argv[i], "-help") == 0 ||
               std::strcmp(argv[i], "--help") == 0) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS...]" << std::endl;
        std::cerr << "The allowed options are:" << std::endl;
        std::cerr << "    -h | -help | --help "
                     "print this message" << std::endl;
        std::cerr << "    -cfg <FILENAME>     "
                     "load ASCII format configuration file" << std::endl;
        std::cerr << "    -snapshot <FNAME>   "
                     "load snapshot or demo file on startup" << std::endl;
        std::cerr << "    -opengl             "
                     "use OpenGL video driver (this is the default)"
                  << std::endl;
        std::cerr << "    -no-opengl          "
                     "use software video driver" << std::endl;
        std::cerr << "    -colorscheme <N>    "
                     "use GUI color scheme N (0, 1, or 2)" << std::endl;
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
    Ep128Emu::setGUIColorScheme(colorScheme);
    audioOutput = new Ep128Emu::AudioOutput_PortAudio();
    if (glEnabled)
      w = new Ep128Emu::OpenGLDisplay(32, 32, 384, 288, "");
    else
      w = new Ep128Emu::FLTKDisplay(32, 32, 384, 288, "");
    w->end();
    vm = new Ep128::Ep128VM(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                            *audioOutput);
    config = new Ep128Emu::EmulatorConfiguration(
        *vm, *(dynamic_cast<Ep128Emu::VideoDisplay *>(w)), *audioOutput);
    config->setErrorCallback(&cfgErrorFunc, (void *) 0);
    // load base configuration (if available)
    {
      Ep128Emu::File  *f = (Ep128Emu::File *) 0;
      try {
        try {
          f = new Ep128Emu::File(cfgFileName, true);
        }
        catch (Ep128Emu::Exception& e) {
          std::string cmdLine = "\"";
          cmdLine += argv[0];
          size_t  i = cmdLine.length();
          while (i > 0) {
            i--;
            if (cmdLine[i] == '/' || cmdLine[i] == '\\') {
              i++;
              break;
            }
          }
          cmdLine.resize(i);
          cmdLine += "makecfg\"";
#ifdef __APPLE__
          cmdLine += " -f";
#endif
          std::system(cmdLine.c_str());
          f = new Ep128Emu::File(cfgFileName, true);
        }
        config->registerChunkType(*f);
        f->processAllChunks();
        delete f;
      }
      catch (...) {
        if (f)
          delete f;
      }
    }
    configLoaded = true;
    // check command line for any additional configuration
    for (int i = 1; i < argc; i++) {
      if (std::strcmp(argv[i], "-ep128") == 0 ||
          std::strcmp(argv[i], "-opengl") == 0 ||
          std::strcmp(argv[i], "-no-opengl") == 0)
        continue;
      if (std::strcmp(argv[i], "-cfg") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing configuration file name");
        config->loadState(argv[i], false);
      }
      else if (std::strcmp(argv[i], "-snapshot") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing snapshot file name");
        snapshotNameIndex = i;
      }
      else if (std::strcmp(argv[i], "-colorscheme") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing color scheme number");
      }
      else {
        const char  *s = argv[i];
#ifdef __APPLE__
        if (std::strncmp(s, "-psn_", 5) == 0)
          continue;
#endif
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
    if (snapshotNameIndex >= 1) {
      Ep128Emu::File  f(argv[snapshotNameIndex], false);
      vm->registerChunkTypes(f);
      f.processAllChunks();
    }
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

