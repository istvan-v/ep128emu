
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

// simple emulator frontend for testing only

#include "ep128emu.hpp"
#include "system.hpp"
#include "gldisp.hpp"
#include "soundio.hpp"
#include "ep128vm.hpp"
#include "plus4/plus4vm.hpp"
#include "emucfg.hpp"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <list>
#include <map>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>
#include <unistd.h>

struct KeyboardEvent {
  bool    isKeyPress;
  int     keyCode;
};

class Display : public Ep128Emu::OpenGLDisplay {
 private:
  std::list<KeyboardEvent>  keyboardQueue;
  Ep128Emu::Mutex keyboardQueueMutex;
  std::map< int, bool > keyboardState;
 public:
  Display(int xx, int yy, int ww, int hh, const char *lbl = 0,
          bool isDoubleBuffered = false)
    : Ep128Emu::OpenGLDisplay(xx, yy, ww, hh, lbl, isDoubleBuffered)
  {
  }
  virtual ~Display()
  {
  }
  virtual int handle(int event)
  {
    if (event == FL_FOCUS || event == FL_UNFOCUS)
      return 1;
    if (event == FL_KEYUP || event == FL_KEYDOWN) {
      KeyboardEvent evt;
      evt.isKeyPress = (event == FL_KEYDOWN);
      evt.keyCode = Fl::event_key();
      if (keyboardState[evt.keyCode] != evt.isKeyPress) {
        keyboardState[evt.keyCode] = evt.isKeyPress;
        keyboardQueueMutex.lock();
        keyboardQueue.push_back(evt);
        keyboardQueueMutex.unlock();
      }
      return 1;
    }
    return 0;
  }
  KeyboardEvent getKeyboardEvent()
  {
    KeyboardEvent evt;
    evt.isKeyPress = false;
    evt.keyCode = 0;
    keyboardQueueMutex.lock();
    if (keyboardQueue.begin() != keyboardQueue.end()) {
      evt = keyboardQueue.front();
      keyboardQueue.pop_front();
    }
    keyboardQueueMutex.unlock();
    return evt;
  }
};

class VMThread : public Ep128Emu::Thread {
 private:
  Ep128Emu::VirtualMachine& vm;
  Display&                  display;
  Ep128Emu::AudioOutput&    audioOutput;
  Ep128Emu::EmulatorConfiguration&  config;
 public:
  volatile bool   stopFlag;
  VMThread(Ep128Emu::VirtualMachine& vm_,
           Display& display_, Ep128Emu::AudioOutput& audioOutput_,
           Ep128Emu::EmulatorConfiguration& config_)
    : Thread(),
      vm(vm_),
      display(display_),
      audioOutput(audioOutput_),
      config(config_),
      stopFlag(false)
  {
  }
  virtual ~VMThread()
  {
  }
  virtual void run()
  {
    try {
      Ep128Emu::File  f;
      bool    recordingDemo = false;
      while (!stopFlag) {
        while (true) {
          if (recordingDemo && !vm.getIsRecordingDemo()) {
            f.writeFile("demotest.dat");
            recordingDemo = false;
            std::cout << "demo recording stopped" << std::endl;
          }
          KeyboardEvent evt = display.getKeyboardEvent();
          if (evt.keyCode == 0)
            break;
          if (evt.isKeyPress &&
              evt.keyCode >= (FL_F + 9) && evt.keyCode <= (FL_F + 12)) {
            int   n = evt.keyCode - (FL_F + 9);
            if (Fl::event_shift() != 0)
              n += 4;
            if (Fl::event_ctrl() != 0)
              n += 8;
            switch (n) {
            case 0:                             // F9: tape play
              vm.tapePlay();
              std::cout << "tape play on (position = "
                        << vm.getTapePosition() << " seconds)" << std::endl;
              break;
            case 1:                             // F10: tape stop
              vm.tapeStop();
              std::cout << "tape stopped (position = "
                        << vm.getTapePosition() << " seconds)" << std::endl;
              break;
            case 2:                             // F11: tape seek
              {
                double  oldTapePos = vm.getTapePosition();
                vm.tapeSeekToCuePoint(true, 60.0);
                if (vm.getTapePosition() == oldTapePos)
                  vm.tapeSeek(0.0);
                std::cout << "tape position set to "
                          << vm.getTapePosition() << " seconds)" << std::endl;
              }
              break;
            case 3:                             // F12: soft reset
              vm.reset(false);
              break;
            case 4:                             // Shift + F9: record demo
              if (vm.getIsPlayingDemo() || vm.getIsRecordingDemo()) {
                if (vm.getIsPlayingDemo())
                  std::cout << "demo playback stopped" << std::endl;
                vm.stopDemo();
              }
              else {
                vm.recordDemo(f);
                recordingDemo = true;
                std::cout << "recording demo..." << std::endl;
              }
              break;
            case 5:                             // Shift + F10: play demo
              if (vm.getIsPlayingDemo() || vm.getIsRecordingDemo()) {
                if (vm.getIsPlayingDemo())
                  std::cout << "demo playback stopped" << std::endl;
                vm.stopDemo();
              }
              else {
                Ep128Emu::File  f_("demotest.dat");
                vm.registerChunkTypes(f_);
                f_.processAllChunks();
                std::cout << "playing demo..." << std::endl;
              }
              break;
            case 6:                             // Shift + F11: tape record
              vm.tapeRecord();
              std::cout << "tape record on (position = "
                        << vm.getTapePosition() << " seconds)" << std::endl;
              break;
            case 7:                             // Shift + F12: hard reset
              vm.reset(true);
              break;
            case 10:                            // Ctrl + F11: rewind tape
              vm.tapeSeek(0.0);
              std::cout << "tape position set to "
                        << vm.getTapePosition() << " seconds)" << std::endl;
              break;
            case 11:                            // Ctrl + F12: reload config
              try {
                if (typeid(vm) == typeid(Ep128::Ep128VM))
                  config.loadState("ep128.cfg", true);
                else if (typeid(vm) == typeid(Plus4::Plus4VM))
                  config.loadState("plus4.cfg", true);
                config.applySettings();
              }
              catch (Ep128Emu::Exception& e) {
                std::cerr << "WARNING: " << e.what() << std::endl;
              }
              break;
            }
          }
          else if (config.convertKeyCode(evt.keyCode) >= 0)
            vm.setKeyboardState(config.convertKeyCode(evt.keyCode),
                                evt.isKeyPress);
        }
        vm.run(2000);
      }
      if (vm.getIsRecordingDemo()) {
        vm.stopDemo();
        f.writeFile("demotest.dat");
        std::cout << "demo recording stopped" << std::endl;
      }
      vm.stopDemo();
    }
    catch (std::exception& e) {
      stopFlag = true;
      std::cerr << " *** error: " << e.what() << std::endl;
    }
  }
};

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
  Fl_Window *basew = (Fl_Window *) 0;
  Display   *w = (Display *) 0;
  Ep128Emu::VirtualMachine  *vm = (Ep128Emu::VirtualMachine *) 0;
  Ep128Emu::AudioOutput     *audioOutput = (Ep128Emu::AudioOutput *) 0;
  Ep128Emu::EmulatorConfiguration *config =
      (Ep128Emu::EmulatorConfiguration *) 0;
  VMThread  *vmThread = (VMThread *) 0;
  bool      isPlus4 = false;
  const char  *cfgFileName = "ep128.cfg";

  try {
    // find out machine type to be emulated
    for (int i = 1; i < argc; i++) {
      if (std::strcmp(argv[i], "-cfg") == 0 && i < (argc - 1)) {
        i++;
      }
      else if (std::strcmp(argv[i], "-ep128") == 0) {
        isPlus4 = false;
        cfgFileName = "ep128.cfg";
      }
      else if (std::strcmp(argv[i], "-plus4") == 0) {
        isPlus4 = true;
        cfgFileName = "plus4.cfg";
      }
    }

    Fl::lock();
    audioOutput = new Ep128Emu::AudioOutput_PortAudio();
    {
      std::vector< std::string >  devList = audioOutput->getDeviceList();
      std::cout << "The available sound devices are:" << std::endl;
      for (size_t i = 0; i < devList.size(); i++)
        std::cout << "  " << i << ": " << devList[i] << std::endl;
    }
    basew = new Fl_Window(50, 50, 352, 288, "ep128emu test");
    w = new Display(0, 0, 352, 288, "OpenGL test window");
    w->end();
    basew->end();
    if (!isPlus4)
      vm = new Ep128::Ep128VM(*w, *audioOutput);
    else
      vm = new Plus4::Plus4VM(*w, *audioOutput);
    config = new Ep128Emu::EmulatorConfiguration(*vm, *w, *audioOutput);
    config->setErrorCallback(&cfgErrorFunc, (void *) 0);
    // set machine specific defaults and limits
    if (isPlus4) {
      Ep128Emu::ConfigurationDB::ConfigurationVariable  *cv;
      cv = &((*config)["vm.cpuClockFrequency"]);
      (*cv).setRange(1500000.0, 150000000.0, 0.0);
      (*cv) = (unsigned int) 1773448;
      cv = &((*config)["vm.videoClockFrequency"]);
      (*cv).setRange(768000.0, 1024000.0, 4.0);
      (*cv) = (unsigned int) 886724;
      (*cv).setCallback(&plus4ClockFreqChangeCallback, config, true);
      cv = &((*config)["vm.soundClockFrequency"]);
      (*cv).setRange(192000.0, 256000.0, 0.0);
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
    {
      std::string fname = Ep128Emu::getEp128EmuHomeDirectory();
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
      fname += '\\';
#else
      fname += '/';
#endif
      fname += cfgFileName;
      std::FILE *f = std::fopen(fname.c_str(), "r");
      if (f) {
        std::fclose(f);
        config->loadState(fname.c_str(), false);
      }
    }
    // check command line for any additional configuration
    for (int i = 1; i < argc; i++) {
      if (std::strcmp(argv[i], "-ep128") == 0 ||
          std::strcmp(argv[i], "-plus4") == 0)
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
    basew->size(config->display.width, config->display.height);
    w->size(config->display.width, config->display.height);
    basew->size_range(352, 288, 1408, 1152);
    basew->show();
    w->setIsDoubleBuffered(config->display.doubleBuffered);
    w->show();
    w->cursor(FL_CURSOR_NONE);
    vmThread = new VMThread(*vm, *w, *audioOutput, *config);
    vmThread->start();
    do {
      config->display.width = basew->w();
      config->display.height = basew->h();
      w->size(basew->w(), basew->h());
      w->redraw();
      Fl::wait(0.05);
    } while (basew->shown() && !vmThread->stopFlag);
    vmThread->stopFlag = true;
    vmThread->join();
  }
  catch (std::exception& e) {
    if (vmThread) {
      vmThread->stopFlag = true;
      vmThread->join();
      delete vmThread;
    }
    if (config) {
#if 0
      try {
        config->saveState(cfgFileName, true);
      }
      catch (...) {
      }
#endif
      delete config;
    }
    if (vm)
      delete vm;
    if (basew)
      delete basew;
    if (audioOutput)
      delete audioOutput;
    std::cerr << " *** error: " << e.what() << std::endl;
    return -1;
  }
  delete vmThread;
  try {
    config->saveState(cfgFileName, true);
  }
  catch (...) {
  }
  delete config;
  delete vm;
  delete basew;
  delete audioOutput;
  return 0;
}

