
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

#ifndef EP128EMU_SCRIPT_HPP
#define EP128EMU_SCRIPT_HPP

#include "ep128emu.hpp"
#include "vm.hpp"
#include "ep128vm.hpp"

#ifdef HAVE_LUA_H
extern "C" {
#  include "lua.h"
#  include "lauxlib.h"
#  include "lualib.h"
}
#else
struct  lua_State;      // dummy declaration
#endif  // HAVE_LUA_H

namespace Ep128Emu {

  class LuaScript {
   protected:
    VirtualMachine& vm;
    lua_State   *luaState;
    const char  *errorMessage;
    bool        haveBreakPointCallback;
    // --------
#ifdef HAVE_LUA_H
    static void *allocFunc(void *userData,
                           void *ptr, size_t oldSize, size_t newSize);
    static int luaFunc_AND(lua_State *lst);
    static int luaFunc_OR(lua_State *lst);
    static int luaFunc_XOR(lua_State *lst);
    static int luaFunc_SHL(lua_State *lst);
    static int luaFunc_SHR(lua_State *lst);
    static int luaFunc_setBreakPoint(lua_State *lst);
    static int luaFunc_clearBreakPoints(lua_State *lst);
    static int luaFunc_getMemoryPage(lua_State *lst);
    static int luaFunc_readMemory(lua_State *lst);
    static int luaFunc_writeMemory(lua_State *lst);
    static int luaFunc_readMemoryRaw(lua_State *lst);
    static int luaFunc_writeMemoryRaw(lua_State *lst);
    static int luaFunc_readIOPort(lua_State *lst);
    static int luaFunc_writeIOPort(lua_State *lst);
    static int luaFunc_getPC(lua_State *lst);
    static int luaFunc_getA(lua_State *lst);
    static int luaFunc_getF(lua_State *lst);
    static int luaFunc_getAF(lua_State *lst);
    static int luaFunc_getB(lua_State *lst);
    static int luaFunc_getC(lua_State *lst);
    static int luaFunc_getBC(lua_State *lst);
    static int luaFunc_getD(lua_State *lst);
    static int luaFunc_getE(lua_State *lst);
    static int luaFunc_getDE(lua_State *lst);
    static int luaFunc_getH(lua_State *lst);
    static int luaFunc_getL(lua_State *lst);
    static int luaFunc_getHL(lua_State *lst);
    static int luaFunc_getAF_(lua_State *lst);
    static int luaFunc_getBC_(lua_State *lst);
    static int luaFunc_getDE_(lua_State *lst);
    static int luaFunc_getHL_(lua_State *lst);
    static int luaFunc_getSP(lua_State *lst);
    static int luaFunc_getIX(lua_State *lst);
    static int luaFunc_getIY(lua_State *lst);
    static int luaFunc_getIM(lua_State *lst);
    static int luaFunc_getI(lua_State *lst);
    static int luaFunc_getR(lua_State *lst);
    static int luaFunc_setPC(lua_State *lst);
    static int luaFunc_setA(lua_State *lst);
    static int luaFunc_setF(lua_State *lst);
    static int luaFunc_setAF(lua_State *lst);
    static int luaFunc_setB(lua_State *lst);
    static int luaFunc_setC(lua_State *lst);
    static int luaFunc_setBC(lua_State *lst);
    static int luaFunc_setD(lua_State *lst);
    static int luaFunc_setE(lua_State *lst);
    static int luaFunc_setDE(lua_State *lst);
    static int luaFunc_setH(lua_State *lst);
    static int luaFunc_setL(lua_State *lst);
    static int luaFunc_setHL(lua_State *lst);
    static int luaFunc_setAF_(lua_State *lst);
    static int luaFunc_setBC_(lua_State *lst);
    static int luaFunc_setDE_(lua_State *lst);
    static int luaFunc_setHL_(lua_State *lst);
    static int luaFunc_setSP(lua_State *lst);
    static int luaFunc_setIX(lua_State *lst);
    static int luaFunc_setIY(lua_State *lst);
    static int luaFunc_setIM(lua_State *lst);
    static int luaFunc_setI(lua_State *lst);
    static int luaFunc_setR(lua_State *lst);
    static int luaFunc_getNextOpcodeAddr(lua_State *lst);
    static int luaFunc_loadMemory(lua_State *lst);
    static int luaFunc_saveMemory(lua_State *lst);
    static int luaFunc_mprint(lua_State *lst);
    void registerLuaFunction(lua_CFunction f, const char *name);
    bool runBreakPointCallback_(int type, uint16_t addr, uint8_t value);
    void luaError(const char *msg);
#endif  // HAVE_LUA_H
   public:
    LuaScript(VirtualMachine& vm_);
    virtual ~LuaScript();
    virtual void loadScript(const char *s);
    virtual void closeScript();
    inline bool runBreakPointCallback(int type, uint16_t addr, uint8_t value)
    {
#ifdef HAVE_LUA_H
      if (haveBreakPointCallback)
        return runBreakPointCallback_(type, addr, value);
#else
      (void) type;
      (void) addr;
      (void) value;
#endif  // HAVE_LUA_H
      return true;
    }
    virtual void errorCallback(const char *msg);
    virtual void messageCallback(const char *msg);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_SCRIPT_HPP

