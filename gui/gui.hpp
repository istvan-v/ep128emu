
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

#ifndef EP128EMU_GUI_HPP
#define EP128EMU_GUI_HPP

#include "ep128emu.hpp"
#include "fileio.hpp"
#include "system.hpp"
#include "display.hpp"
#include "fldisp.hpp"
#include "gldisp.hpp"
#include "soundio.hpp"
#include "vm.hpp"
#include "vmthread.hpp"
#include "cfg_db.hpp"
#include "emucfg.hpp"

#include "ep128vm.hpp"
#include "plus4/plus4vm.hpp"

#include <FL/Fl_File_Chooser.H>

class Ep128EmuGUI_DisplayConfigWindow;
class Ep128EmuGUI_SoundConfigWindow;
class Ep128EmuGUI_MachineConfigWindow;

#include "gui_fl.hpp"
#include "disp_cfg.hpp"
#include "snd_cfg.hpp"
#include "vm_cfg.hpp"

#endif  // EP128EMU_GUI_HPP

