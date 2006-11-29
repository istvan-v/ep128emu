
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2006 Istvan Varga <istvanv@users.sourceforge.net>
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

#include "ep128.hpp"
#include "gldisp.hpp"
#include "ep128vm.hpp"

#include <iostream>
#include <cmath>
#include <map>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>
#include <unistd.h>

// #define RECORDING_DEMO 1
// #define PLAYING_DEMO 1

class Display : public Ep128Emu::OpenGLDisplay {
 private:
  Ep128::Ep128VM  *vm;
  std::map<int, int> keyboardMap;
 public:
  Display(int xx, int yy, int ww, int hh, const char *lbl = 0)
    : Ep128Emu::OpenGLDisplay(xx, yy, ww, hh, lbl)
  {
    vm = 0;
    keyboardMap['a'] = 0x0E;
    keyboardMap['b'] = 0x02;
    keyboardMap['c'] = 0x03;
    keyboardMap['d'] = 0x0B;
    keyboardMap['e'] = 0x15;
    keyboardMap['f'] = 0x0C;
    keyboardMap['g'] = 0x0A;
    keyboardMap['h'] = 0x08;
    keyboardMap['i'] = 0x48;
    keyboardMap['j'] = 0x30;
    keyboardMap['k'] = 0x32;
    keyboardMap['l'] = 0x34;
    keyboardMap['m'] = 0x40;
    keyboardMap['n'] = 0x00;
    keyboardMap['o'] = 0x4A;
    keyboardMap['p'] = 0x4C;
    keyboardMap['q'] = 0x11;
    keyboardMap['r'] = 0x13;
    keyboardMap['s'] = 0x0D;
    keyboardMap['t'] = 0x14;
    keyboardMap['u'] = 0x10;
    keyboardMap['v'] = 0x04;
    keyboardMap['w'] = 0x16;
    keyboardMap['x'] = 0x05;
    keyboardMap['y'] = 0x12;
    keyboardMap['z'] = 0x06;
    keyboardMap['0'] = 0x2C;
    keyboardMap['1'] = 0x19;
    keyboardMap['2'] = 0x1E;
    keyboardMap['3'] = 0x1D;
    keyboardMap['4'] = 0x1B;
    keyboardMap['5'] = 0x1C;
    keyboardMap['6'] = 0x1A;
    keyboardMap['7'] = 0x18;
    keyboardMap['8'] = 0x28;
    keyboardMap['9'] = 0x2A;
    keyboardMap[' '] = 0x46;
    keyboardMap['\n'] = 0x3E;
    keyboardMap['\r'] = 0x3E;
    keyboardMap[FL_Enter] = 0x3E;
    keyboardMap[','] = 0x42;
    keyboardMap['.'] = 0x44;
    keyboardMap[FL_Down] = 0x39;
    keyboardMap[FL_Right] = 0x3A;
    keyboardMap[FL_Up] = 0x3B;
    keyboardMap[FL_Left] = 0x3D;
    keyboardMap[FL_Shift_L] = 0x07;
    keyboardMap[FL_Shift_R] = 0x45;
    keyboardMap['-'] = 0x2B;
    keyboardMap['/'] = 0x43;
    keyboardMap[FL_BackSpace] = 0x2E;
    keyboardMap[';'] = 0x33;
    keyboardMap['\''] = 0x35;
    keyboardMap[']'] = 0x36;
    keyboardMap['['] = 0x4D;
  }
  virtual ~Display()
  {
  }
  void setVM(Ep128::Ep128VM& vm_)
  {
    vm = &vm_;
  }
  virtual int handle(int event)
  {
    if (event == FL_FOCUS || event == FL_UNFOCUS)
      return 1;
    if ((event == FL_KEYUP || event == FL_KEYDOWN) && vm) {
      int   keyCode = Fl::event_key();
      if (event == FL_KEYDOWN && keyCode == (FL_F + 12))
        vm->reset();
      else
        vm->setKeyboardState(keyboardMap[keyCode], (event == FL_KEYDOWN));
      return 1;
    }
    return 0;
  }
};

class VMThread : public Ep128Emu::Thread {
 private:
  Ep128::Ep128VM  vm;
  Display&        display;
 public:
  volatile bool stopFlag;
  VMThread(Display& display_)
    : Thread(),
      vm(display_),
      display(display_)
  {
    stopFlag = false;
  }
  virtual ~VMThread()
  {
  }
  virtual void run()
  {
    display.setVM(vm);
    vm.resetMemoryConfiguration(128);
    vm.loadROMSegment(0, "./roms/exos0.rom", 0);
    vm.loadROMSegment(1, "./roms/exos1.rom", 0);
    vm.loadROMSegment(4, "./roms/basic.rom", 0);
    vm.setAudioOutputQuality(true, 48000.0f);
    vm.setAudioOutputDeviceParameters(11, 0.02, 4, 4);
    vm.setCPUFrequency(4000000);
 // vm.setAudioOutputFileName("/tmp/ep.wav");
    vm.setTapeFileName("./tape/tape0.tap");
    vm.tapePlay();
    vm.setEnableFastTapeMode(true);
#ifdef RECORDING_DEMO
    Ep128::File f;
    vm.recordDemo(f);
#endif
#ifdef PLAYING_DEMO
    Ep128::File f("demotest.dat");
    vm.registerChunkTypes(f);
    f.processAllChunks();
#endif
    while (!stopFlag) {
      vm.run(100000);
    }
    vm.stopDemo();
#ifdef RECORDING_DEMO
    f.writeFile("demotest.dat");
#endif
  }
};

int main()
{
  Fl::lock();
  Fl_Window *basew = new Fl_Window(50, 50, 1056, 864, "ep128emu test");
  basew->size_range(352, 288, 1408, 1152);
  Display *w = new Display(0, 0, 1056, 864, "OpenGL test window");
  w->end();
  basew->end();
  basew->show();
  w->show();
  w->cursor(FL_CURSOR_NONE);
  Ep128Emu::OpenGLDisplay::DisplayParameters dp(w->getDisplayParameters());
  dp.displayQuality = 3;
  dp.blendScale1 = 0.37;
  dp.blendScale2 = 0.72;
  dp.blendScale3 = 0.30;
  dp.g = 1.25;
  dp.b = 0.025;
  dp.pixelAspectRatio = 1.0;
  w->setDisplayParameters(dp);
  {
    VMThread  vm(*w);
    vm.start();
    do {
      w->size(basew->w(), basew->h());
      w->redraw();
      Fl::wait(0.15);
    } while (basew->shown());
    vm.stopFlag = true;
    vm.join();
  }
  delete basew;
  return 0;
}

