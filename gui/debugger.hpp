
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

#ifndef EP128EMU_GUI_DEBUGGER_HPP
#define EP128EMU_GUI_DEBUGGER_HPP

#include "ep128emu.hpp"
#include "gui.hpp"
#include "script.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Multiline_Output.H>

class Ep128EmuGUI_LuaScript : public Ep128Emu::LuaScript {
 private:
  Ep128EmuGUI_DebugWindow&  debugWindow;
 public:
  Ep128EmuGUI_LuaScript(Ep128EmuGUI_DebugWindow& debugWindow_,
                        Ep128Emu::VirtualMachine& vm_)
    : Ep128Emu::LuaScript(vm_),
      debugWindow(debugWindow_)
  {
  }
  virtual ~Ep128EmuGUI_LuaScript();
  virtual void errorCallback(const char *msg);
  virtual void messageCallback(const char *msg);
};

class Ep128EmuGUI_ScrollableOutput : public Fl_Multiline_Output {
 public:
  Fl_Widget *upWidget;
  Fl_Widget *downWidget;
  // --------
  Ep128EmuGUI_ScrollableOutput(int xx, int yy, int ww, int hh,
                               const char *lbl = 0)
    : Fl_Multiline_Output(xx, yy, ww, hh, lbl),
      upWidget((Fl_Widget *) 0),
      downWidget((Fl_Widget *) 0)
  {
  }
  virtual ~Ep128EmuGUI_ScrollableOutput();
  virtual int handle(int evt);
};

#endif  // EP128EMU_GUI_DEBUGGER_HPP

