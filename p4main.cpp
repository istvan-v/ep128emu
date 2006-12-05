
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
#include "plus4/plus4vm.hpp"

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
  Plus4::Plus4VM  *vm;
  std::map<int, int> keyboardMap;
 public:
  Display(int xx, int yy, int ww, int hh, const char *lbl = 0)
    : Ep128Emu::OpenGLDisplay(xx, yy, ww, hh, lbl)
  {
    vm = 0;
    keyboardMap['a'] = 0x0A;
    keyboardMap['b'] = 0x1C;
    keyboardMap['c'] = 0x14;
    keyboardMap['d'] = 0x12;
    keyboardMap['e'] = 0x0E;
    keyboardMap['f'] = 0x15;
    keyboardMap['g'] = 0x1A;
    keyboardMap['h'] = 0x1D;
    keyboardMap['i'] = 0x21;
    keyboardMap['j'] = 0x22;
    keyboardMap['k'] = 0x25;
    keyboardMap['l'] = 0x2A;
    keyboardMap['m'] = 0x24;
    keyboardMap['n'] = 0x27;
    keyboardMap['o'] = 0x26;
    keyboardMap['p'] = 0x29;
    keyboardMap['q'] = 0x3E;
    keyboardMap['r'] = 0x11;
    keyboardMap['s'] = 0x0D;
    keyboardMap['t'] = 0x16;
    keyboardMap['u'] = 0x1E;
    keyboardMap['v'] = 0x1F;
    keyboardMap['w'] = 0x09;
    keyboardMap['x'] = 0x17;
    keyboardMap['y'] = 0x19;
    keyboardMap['z'] = 0x0C;
    keyboardMap['0'] = 0x23;
    keyboardMap['1'] = 0x38;
    keyboardMap['2'] = 0x3B;
    keyboardMap['3'] = 0x08;
    keyboardMap['4'] = 0x0B;
    keyboardMap['5'] = 0x10;
    keyboardMap['6'] = 0x13;
    keyboardMap['7'] = 0x18;
    keyboardMap['8'] = 0x1B;
    keyboardMap['9'] = 0x20;
    keyboardMap[' '] = 0x3C;
    keyboardMap['\n'] = 0x01;
    keyboardMap['\r'] = 0x01;
    keyboardMap[FL_Enter] = 0x01;
    keyboardMap[','] = 0x2F;
    keyboardMap['.'] = 0x2C;
    keyboardMap[FL_Down] = 0x28;
    keyboardMap[FL_Right] = 0x33;
    keyboardMap[FL_Up] = 0x2B;
    keyboardMap[FL_Left] = 0x30;
    keyboardMap[FL_Shift_L] = 0x0F;
    keyboardMap[FL_Shift_R] = 0x0F;
    keyboardMap['-'] = 0x2E;
    keyboardMap['/'] = 0x37;
    keyboardMap[FL_BackSpace] = 0x00;
    keyboardMap[';'] = 0x32;
    keyboardMap['\''] = 0x2D;
    keyboardMap['+'] = 0x36;
    keyboardMap['='] = 0x35;
    keyboardMap[FL_KP + '/'] = 0x50;
    keyboardMap[FL_KP + '9'] = 0x53;
    keyboardMap[FL_KP + '8'] = 0x51;
    keyboardMap[FL_KP + '7'] = 0x52;
    keyboardMap[FL_KP + '+'] = 0x56;
    keyboardMap[FL_KP + '5'] = 0x48;
    keyboardMap[FL_KP + '3'] = 0x4B;
    keyboardMap[FL_KP + '2'] = 0x49;
    keyboardMap[FL_KP + '1'] = 0x4A;
    keyboardMap[FL_KP + '\r'] = 0x4F;
  }
  virtual ~Display()
  {
  }
  void setVM(Plus4::Plus4VM& vm_)
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
      else if (event == FL_KEYDOWN && keyCode == (FL_F + 11))
        vm->loadProgram("./progs/test.prg");
      else if (event == FL_KEYDOWN && keyCode == (FL_F + 10))
        vm->saveProgram("./progs/test2.prg");
      else
        vm->setKeyboardState(keyboardMap[keyCode], (event == FL_KEYDOWN));
      return 1;
    }
    return 0;
  }
};

class VMThread : public Ep128Emu::Thread {
 private:
  Plus4::Plus4VM  vm;
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
    vm.resetMemoryConfiguration(64);
    vm.loadROMSegment(0, "./roms/plus4.rom", 0);
    vm.loadROMSegment(1, "./roms/plus4.rom", 16384);
    vm.setAudioOutputQuality(true, 48000.0f);
    vm.setAudioOutputDeviceParameters(11, 0.02, 4, 4);
    vm.setAudioOutputVolume(0.5f);
    vm.setCPUFrequency(1775000);
 // vm.setTapeFileName("./tape/tape1.tap");
 // vm.tapePlay();
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
      vm.run(50000);
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
  Fl_Window *basew = new Fl_Window(50, 50, 1056, 864, "plus4 test");
  basew->size_range(352, 288, 1408, 1152);
  Display *w = new Display(0, 0, 1056, 864, "OpenGL test window");
  w->end();
  basew->end();
  basew->show();
  w->show();
  w->cursor(FL_CURSOR_NONE);
  Ep128Emu::VideoDisplay::DisplayParameters dp(w->getDisplayParameters());
  dp.displayQuality = 1;
  dp.blendScale1 = 0.37;
  dp.blendScale2 = 0.72;
  dp.blendScale3 = 0.30;
  dp.g = 1.0;
  dp.c = 1.1;
  dp.b = 0.05;
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

