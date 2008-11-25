
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

#ifndef EP128EMU_GUI_HPP
#define EP128EMU_GUI_HPP

#include "ep128emu.hpp"
#include "fileio.hpp"
#include "system.hpp"
#include "display.hpp"
#include "fldisp.hpp"
#include "gldisp.hpp"
#include "joystick.hpp"
#include "soundio.hpp"
#include "vm.hpp"
#include "vmthread.hpp"
#include "cfg_db.hpp"
#include "emucfg.hpp"
#include "script.hpp"
#include "ep128vm.hpp"

#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>

class Ep128EmuGUI;
class Ep128EmuGUI_DiskConfigWindow;
class Ep128EmuGUI_DisplayConfigWindow;
class Ep128EmuGUI_KbdConfigWindow;
class Ep128EmuGUI_SoundConfigWindow;
class Ep128EmuGUI_MachineConfigWindow;
class Ep128EmuGUI_DebugWindow;
class Ep128EmuGUI_AboutWindow;
class Ep128EmuGUIMonitor;
class Ep128EmuGUI_LuaScript;
class Ep128EmuGUI_ScrollableOutput;

#include "debugger.hpp"
#include "gui_fl.hpp"
#include "disk_cfg_fl.hpp"
#include "disp_cfg_fl.hpp"
#include "kbd_cfg_fl.hpp"
#include "snd_cfg_fl.hpp"
#include "vm_cfg_fl.hpp"
#include "debug_fl.hpp"
#include "about_fl.hpp"
#include "monitor.hpp"

#endif  // EP128EMU_GUI_HPP

