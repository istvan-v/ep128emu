/*
 *  ENTER emulator (c) Copyright, Kevin Thacker 1999-2001
 *
 *  This file is part of the ENTER emulator source code distribution.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Istvan Varga, 2004, 2007, 2009: fixed opcode cycle counts */

#include "z80.hpp"

#include "z80macros.hpp"
#include "z80funcs.hpp"

/***************************************************************************/

namespace Ep128 {

  EP128EMU_INLINE void Z80::Index_CB_ExecuteInstruction()
  {
    uint8_t Opcode = readOpcodeByte(3);
    updateCycles(2);
    switch (Opcode) {
    case 0x000:
      {
        INDEX_CB_RLC_REG(R.BC.B.h);
      }
      break;
    case 0x001:
      {
        INDEX_CB_RLC_REG(R.BC.B.l);
      }
      break;
    case 0x002:
      {
        INDEX_CB_RLC_REG(R.DE.B.h);
      }
      break;
    case 0x003:
      {
        INDEX_CB_RLC_REG(R.DE.B.l);
      }
      break;
    case 0x004:
      {
        INDEX_CB_RLC_REG(R.HL.B.h);
      }
      break;
    case 0x005:
      {
        INDEX_CB_RLC_REG(R.HL.B.l);
      }
      break;
    case 0x006:
      {
        RLC_INDEX();
      }
      break;
    case 0x007:
      {
        INDEX_CB_RLC_REG(R.AF.B.h);
      }
      break;
    case 0x008:
      {
        INDEX_CB_RRC_REG(R.BC.B.h);
      }
      break;
    case 0x009:
      {
        INDEX_CB_RRC_REG(R.BC.B.l);
      }
      break;
    case 0x00a:
      {
        INDEX_CB_RRC_REG(R.DE.B.h);
      }
      break;
    case 0x00b:
      {
        INDEX_CB_RRC_REG(R.DE.B.l);
      }
      break;
    case 0x00c:
      {
        INDEX_CB_RRC_REG(R.HL.B.h);
      }
      break;
    case 0x00d:
      {
        INDEX_CB_RRC_REG(R.HL.B.l);
      }
      break;
    case 0x00e:
      {
        RRC_INDEX();
      }
      break;
    case 0x00f:
      {
        INDEX_CB_RRC_REG(R.AF.B.h);
      }
      break;
    case 0x010:
      {
        INDEX_CB_RL_REG(R.BC.B.h);
      }
      break;
    case 0x011:
      {
        INDEX_CB_RL_REG(R.BC.B.l);
      }
      break;
    case 0x012:
      {
        INDEX_CB_RL_REG(R.DE.B.h);
      }
      break;
    case 0x013:
      {
        INDEX_CB_RL_REG(R.DE.B.l);
      }
      break;
    case 0x014:
      {
        INDEX_CB_RL_REG(R.HL.B.h);
      }
      break;
    case 0x015:
      {
        INDEX_CB_RL_REG(R.HL.B.l);
      }
      break;
    case 0x016:
      {
        RL_INDEX();
      }
      break;
    case 0x017:
      {
        INDEX_CB_RL_REG(R.AF.B.h);
      }
      break;
    case 0x018:
      {
        INDEX_CB_RR_REG(R.BC.B.h);
      }
      break;
    case 0x019:
      {
        INDEX_CB_RR_REG(R.BC.B.l);
      }
      break;
    case 0x01a:
      {
        INDEX_CB_RR_REG(R.DE.B.h);
      }
      break;
    case 0x01b:
      {
        INDEX_CB_RR_REG(R.DE.B.l);
      }
      break;
    case 0x01c:
      {
        INDEX_CB_RR_REG(R.HL.B.h);
      }
      break;
    case 0x01d:
      {
        INDEX_CB_RR_REG(R.HL.B.l);
      }
      break;
    case 0x01e:
      {
        RR_INDEX();
      }
      break;
    case 0x01f:
      {
        INDEX_CB_RR_REG(R.AF.B.h);
      }
      break;
    case 0x020:
      {
        INDEX_CB_SLA_REG(R.BC.B.h);
      }
      break;
    case 0x021:
      {
        INDEX_CB_SLA_REG(R.BC.B.l);
      }
      break;
    case 0x022:
      {
        INDEX_CB_SLA_REG(R.DE.B.h);
      }
      break;
    case 0x023:
      {
        INDEX_CB_SLA_REG(R.DE.B.l);
      }
      break;
    case 0x024:
      {
        INDEX_CB_SLA_REG(R.HL.B.h);
      }
      break;
    case 0x025:
      {
        INDEX_CB_SLA_REG(R.HL.B.l);
      }
      break;
    case 0x026:
      {
        SLA_INDEX();
      }
      break;
    case 0x027:
      {
        INDEX_CB_SLA_REG(R.AF.B.h);
      }
      break;
    case 0x028:
      {
        INDEX_CB_SRA_REG(R.BC.B.h);
      }
      break;
    case 0x029:
      {
        INDEX_CB_SRA_REG(R.BC.B.l);
      }
      break;
    case 0x02a:
      {
        INDEX_CB_SRA_REG(R.DE.B.h);
      }
      break;
    case 0x02b:
      {
        INDEX_CB_SRA_REG(R.DE.B.l);
      }
      break;
    case 0x02c:
      {
        INDEX_CB_SRA_REG(R.HL.B.h);
      }
      break;
    case 0x02d:
      {
        INDEX_CB_SRA_REG(R.HL.B.l);
      }
      break;
    case 0x02e:
      {
        SRA_INDEX();
      }
      break;
    case 0x02f:
      {
        INDEX_CB_SRA_REG(R.AF.B.h);
      }
      break;
    case 0x030:
      {
        INDEX_CB_SLL_REG(R.BC.B.h);
      }
      break;
    case 0x031:
      {
        INDEX_CB_SLL_REG(R.BC.B.l);
      }
      break;
    case 0x032:
      {
        INDEX_CB_SLL_REG(R.DE.B.h);
      }
      break;
    case 0x033:
      {
        INDEX_CB_SLL_REG(R.DE.B.l);
      }
      break;
    case 0x034:
      {
        INDEX_CB_SLL_REG(R.HL.B.h);
      }
      break;
    case 0x035:
      {
        INDEX_CB_SLL_REG(R.HL.B.l);
      }
      break;
    case 0x036:
      {
        SLL_INDEX();
      }
      break;
    case 0x037:
      {
        INDEX_CB_SLL_REG(R.AF.B.h);
      }
      break;
    case 0x038:
      {
        INDEX_CB_SRL_REG(R.BC.B.h);
      }
      break;
    case 0x039:
      {
        INDEX_CB_SRL_REG(R.BC.B.l);
      }
      break;
    case 0x03a:
      {
        INDEX_CB_SRL_REG(R.DE.B.h);
      }
      break;
    case 0x03b:
      {
        INDEX_CB_SRL_REG(R.DE.B.l);
      }
      break;
    case 0x03c:
      {
        INDEX_CB_SRL_REG(R.HL.B.h);
      }
      break;
    case 0x03d:
      {
        INDEX_CB_SRL_REG(R.HL.B.l);
      }
      break;
    case 0x03e:
      {
        SRL_INDEX();
      }
      break;
    case 0x03f:
      {
        INDEX_CB_SRL_REG(R.AF.B.h);
      }
      break;
    case 0x040:
      {
        BIT_INDEX(0);
      }
      break;
    case 0x041:
      {
        BIT_INDEX(0);
      }
      break;
    case 0x042:
      {
        BIT_INDEX(0);
      }
      break;
    case 0x043:
      {
        BIT_INDEX(0);
      }
      break;
    case 0x044:
      {
        BIT_INDEX(0);
      }
      break;
    case 0x045:
      {
        BIT_INDEX(0);
      }
      break;
    case 0x046:
      {
        BIT_INDEX(0);
      }
      break;
    case 0x047:
      {
        BIT_INDEX(0);
      }
      break;
    case 0x048:
      {
        BIT_INDEX(1);
      }
      break;
    case 0x049:
      {
        BIT_INDEX(1);
      }
      break;
    case 0x04a:
      {
        BIT_INDEX(1);
      }
      break;
    case 0x04b:
      {
        BIT_INDEX(1);
      }
      break;
    case 0x04c:
      {
        BIT_INDEX(1);
      }
      break;
    case 0x04d:
      {
        BIT_INDEX(1);
      }
      break;
    case 0x04e:
      {
        BIT_INDEX(1);
      }
      break;
    case 0x04f:
      {
        BIT_INDEX(1);
      }
      break;
    case 0x050:
      {
        BIT_INDEX(2);
      }
      break;
    case 0x051:
      {
        BIT_INDEX(2);
      }
      break;
    case 0x052:
      {
        BIT_INDEX(2);
      }
      break;
    case 0x053:
      {
        BIT_INDEX(2);
      }
      break;
    case 0x054:
      {
        BIT_INDEX(2);
      }
      break;
    case 0x055:
      {
        BIT_INDEX(2);
      }
      break;
    case 0x056:
      {
        BIT_INDEX(2);
      }
      break;
    case 0x057:
      {
        BIT_INDEX(2);
      }
      break;
    case 0x058:
      {
        BIT_INDEX(3);
      }
      break;
    case 0x059:
      {
        BIT_INDEX(3);
      }
      break;
    case 0x05a:
      {
        BIT_INDEX(3);
      }
      break;
    case 0x05b:
      {
        BIT_INDEX(3);
      }
      break;
    case 0x05c:
      {
        BIT_INDEX(3);
      }
      break;
    case 0x05d:
      {
        BIT_INDEX(3);
      }
      break;
    case 0x05e:
      {
        BIT_INDEX(3);
      }
      break;
    case 0x05f:
      {
        BIT_INDEX(3);
      }
      break;
    case 0x060:
      {
        BIT_INDEX(4);
      }
      break;
    case 0x061:
      {
        BIT_INDEX(4);
      }
      break;
    case 0x062:
      {
        BIT_INDEX(4);
      }
      break;
    case 0x063:
      {
        BIT_INDEX(4);
      }
      break;
    case 0x064:
      {
        BIT_INDEX(4);
      }
      break;
    case 0x065:
      {
        BIT_INDEX(4);
      }
      break;
    case 0x066:
      {
        BIT_INDEX(4);
      }
      break;
    case 0x067:
      {
        BIT_INDEX(4);
      }
      break;
    case 0x068:
      {
        BIT_INDEX(5);
      }
      break;
    case 0x069:
      {
        BIT_INDEX(5);
      }
      break;
    case 0x06a:
      {
        BIT_INDEX(5);
      }
      break;
    case 0x06b:
      {
        BIT_INDEX(5);
      }
      break;
    case 0x06c:
      {
        BIT_INDEX(5);
      }
      break;
    case 0x06d:
      {
        BIT_INDEX(5);
      }
      break;
    case 0x06e:
      {
        BIT_INDEX(5);
      }
      break;
    case 0x06f:
      {
        BIT_INDEX(5);
      }
      break;
    case 0x070:
      {
        BIT_INDEX(6);
      }
      break;
    case 0x071:
      {
        BIT_INDEX(6);
      }
      break;
    case 0x072:
      {
        BIT_INDEX(6);
      }
      break;
    case 0x073:
      {
        BIT_INDEX(6);
      }
      break;
    case 0x074:
      {
        BIT_INDEX(6);
      }
      break;
    case 0x075:
      {
        BIT_INDEX(6);
      }
      break;
    case 0x076:
      {
        BIT_INDEX(6);
      }
      break;
    case 0x077:
      {
        BIT_INDEX(6);
      }
      break;
    case 0x078:
      {
        BIT_INDEX(7);
      }
      break;
    case 0x079:
      {
        BIT_INDEX(7);
      }
      break;
    case 0x07a:
      {
        BIT_INDEX(7);
      }
      break;
    case 0x07b:
      {
        BIT_INDEX(7);
      }
      break;
    case 0x07c:
      {
        BIT_INDEX(7);
      }
      break;
    case 0x07d:
      {
        BIT_INDEX(7);
      }
      break;
    case 0x07e:
      {
        BIT_INDEX(7);
      }
      break;
    case 0x07f:
      {
        BIT_INDEX(7);
      }
      break;
    case 0x080:
      {
        INDEX_CB_RES_REG(0x001, R.BC.B.h);
      }
      break;
    case 0x081:
      {
        INDEX_CB_RES_REG(0x001, R.BC.B.l);
      }
      break;
    case 0x082:
      {
        INDEX_CB_RES_REG(0x001, R.DE.B.h);
      }
      break;
    case 0x083:
      {
        INDEX_CB_RES_REG(0x001, R.DE.B.l);
      }
      break;
    case 0x084:
      {
        INDEX_CB_RES_REG(0x001, R.HL.B.h);
      }
      break;
    case 0x085:
      {
        INDEX_CB_RES_REG(0x001, R.HL.B.l);
      }
      break;
    case 0x086:
      {
        RES_INDEX(0x01);
      }
      break;
    case 0x087:
      {
        INDEX_CB_RES_REG(0x001, R.AF.B.h);
      }
      break;
    case 0x088:
      {
        INDEX_CB_RES_REG(0x002, R.BC.B.h);
      }
      break;
    case 0x089:
      {
        INDEX_CB_RES_REG(0x002, R.BC.B.l);
      }
      break;
    case 0x08a:
      {
        INDEX_CB_RES_REG(0x002, R.DE.B.h);
      }
      break;
    case 0x08b:
      {
        INDEX_CB_RES_REG(0x002, R.DE.B.l);
      }
      break;
    case 0x08c:
      {
        INDEX_CB_RES_REG(0x002, R.HL.B.h);
      }
      break;
    case 0x08d:
      {
        INDEX_CB_RES_REG(0x002, R.HL.B.l);
      }
      break;
    case 0x08e:
      {
        RES_INDEX(0x02);
      }
      break;
    case 0x08f:
      {
        INDEX_CB_RES_REG(0x002, R.AF.B.h);
      }
      break;
    case 0x090:
      {
        INDEX_CB_RES_REG(0x004, R.BC.B.h);
      }
      break;
    case 0x091:
      {
        INDEX_CB_RES_REG(0x004, R.BC.B.l);
      }
      break;
    case 0x092:
      {
        INDEX_CB_RES_REG(0x004, R.DE.B.h);
      }
      break;
    case 0x093:
      {
        INDEX_CB_RES_REG(0x004, R.DE.B.l);
      }
      break;
    case 0x094:
      {
        INDEX_CB_RES_REG(0x004, R.HL.B.h);
      }
      break;
    case 0x095:
      {
        INDEX_CB_RES_REG(0x004, R.HL.B.l);
      }
      break;
    case 0x096:
      {
        RES_INDEX(0x04);
      }
      break;
    case 0x097:
      {
        INDEX_CB_RES_REG(0x004, R.AF.B.h);
      }
      break;
    case 0x098:
      {
        INDEX_CB_RES_REG(0x008, R.BC.B.h);
      }
      break;
    case 0x099:
      {
        INDEX_CB_RES_REG(0x008, R.BC.B.l);
      }
      break;
    case 0x09a:
      {
        INDEX_CB_RES_REG(0x008, R.DE.B.h);
      }
      break;
    case 0x09b:
      {
        INDEX_CB_RES_REG(0x008, R.DE.B.l);
      }
      break;
    case 0x09c:
      {
        INDEX_CB_RES_REG(0x008, R.HL.B.h);
      }
      break;
    case 0x09d:
      {
        INDEX_CB_RES_REG(0x008, R.HL.B.l);
      }
      break;
    case 0x09e:
      {
        RES_INDEX(0x08);
      }
      break;
    case 0x09f:
      {
        INDEX_CB_RES_REG(0x008, R.AF.B.h);
      }
      break;
    case 0x0a0:
      {
        INDEX_CB_RES_REG(0x010, R.BC.B.h);
      }
      break;
    case 0x0a1:
      {
        INDEX_CB_RES_REG(0x010, R.BC.B.l);
      }
      break;
    case 0x0a2:
      {
        INDEX_CB_RES_REG(0x010, R.DE.B.h);
      }
      break;
    case 0x0a3:
      {
        INDEX_CB_RES_REG(0x010, R.DE.B.l);
      }
      break;
    case 0x0a4:
      {
        INDEX_CB_RES_REG(0x010, R.HL.B.h);
      }
      break;
    case 0x0a5:
      {
        INDEX_CB_RES_REG(0x010, R.HL.B.l);
      }
      break;
    case 0x0a6:
      {
        RES_INDEX(0x10);
      }
      break;
    case 0x0a7:
      {
        INDEX_CB_RES_REG(0x010, R.AF.B.h);
      }
      break;
    case 0x0a8:
      {
        INDEX_CB_RES_REG(0x020, R.BC.B.h);
      }
      break;
    case 0x0a9:
      {
        INDEX_CB_RES_REG(0x020, R.BC.B.l);
      }
      break;
    case 0x0aa:
      {
        INDEX_CB_RES_REG(0x020, R.DE.B.h);
      }
      break;
    case 0x0ab:
      {
        INDEX_CB_RES_REG(0x020, R.DE.B.l);
      }
      break;
    case 0x0ac:
      {
        INDEX_CB_RES_REG(0x020, R.HL.B.h);
      }
      break;
    case 0x0ad:
      {
        INDEX_CB_RES_REG(0x020, R.HL.B.l);
      }
      break;
    case 0x0ae:
      {
        RES_INDEX(0x20);
      }
      break;
    case 0x0af:
      {
        INDEX_CB_RES_REG(0x020, R.AF.B.h);
      }
      break;
    case 0x0b0:
      {
        INDEX_CB_RES_REG(0x040, R.BC.B.h);
      }
      break;
    case 0x0b1:
      {
        INDEX_CB_RES_REG(0x040, R.BC.B.l);
      }
      break;
    case 0x0b2:
      {
        INDEX_CB_RES_REG(0x040, R.DE.B.h);
      }
      break;
    case 0x0b3:
      {
        INDEX_CB_RES_REG(0x040, R.DE.B.l);
      }
      break;
    case 0x0b4:
      {
        INDEX_CB_RES_REG(0x040, R.HL.B.h);
      }
      break;
    case 0x0b5:
      {
        INDEX_CB_RES_REG(0x040, R.HL.B.l);
      }
      break;
    case 0x0b6:
      {
        RES_INDEX(0x40);
      }
      break;
    case 0x0b7:
      {
        INDEX_CB_RES_REG(0x040, R.AF.B.h);
      }
      break;
    case 0x0b8:
      {
        INDEX_CB_RES_REG(0x080, R.BC.B.h);
      }
      break;
    case 0x0b9:
      {
        INDEX_CB_RES_REG(0x080, R.BC.B.l);
      }
      break;
    case 0x0ba:
      {
        INDEX_CB_RES_REG(0x080, R.DE.B.h);
      }
      break;
    case 0x0bb:
      {
        INDEX_CB_RES_REG(0x080, R.DE.B.l);
      }
      break;
    case 0x0bc:
      {
        INDEX_CB_RES_REG(0x080, R.HL.B.h);
      }
      break;
    case 0x0bd:
      {
        INDEX_CB_RES_REG(0x080, R.HL.B.l);
      }
      break;
    case 0x0be:
      {
        RES_INDEX(0x80);
      }
      break;
    case 0x0bf:
      {
        INDEX_CB_RES_REG(0x080, R.AF.B.h);
      }
      break;
    case 0x0c0:
      {
        INDEX_CB_SET_REG(0x001, R.BC.B.h);
      }
      break;
    case 0x0c1:
      {
        INDEX_CB_SET_REG(0x001, R.BC.B.l);
      }
      break;
    case 0x0c2:
      {
        INDEX_CB_SET_REG(0x001, R.DE.B.h);
      }
      break;
    case 0x0c3:
      {
        INDEX_CB_SET_REG(0x001, R.DE.B.l);
      }
      break;
    case 0x0c4:
      {
        INDEX_CB_SET_REG(0x001, R.HL.B.h);
      }
      break;
    case 0x0c5:
      {
        INDEX_CB_SET_REG(0x001, R.HL.B.l);
      }
      break;
    case 0x0c6:
      {
        SET_INDEX(0x001);
      }
      break;
    case 0x0c7:
      {
        INDEX_CB_SET_REG(0x001, R.AF.B.h);
      }
      break;
    case 0x0c8:
      {
        INDEX_CB_SET_REG(0x002, R.BC.B.h);
      }
      break;
    case 0x0c9:
      {
        INDEX_CB_SET_REG(0x002, R.BC.B.l);
      }
      break;
    case 0x0ca:
      {
        INDEX_CB_SET_REG(0x002, R.DE.B.h);
      }
      break;
    case 0x0cb:
      {
        INDEX_CB_SET_REG(0x002, R.DE.B.l);
      }
      break;
    case 0x0cc:
      {
        INDEX_CB_SET_REG(0x002, R.HL.B.h);
      }
      break;
    case 0x0cd:
      {
        INDEX_CB_SET_REG(0x002, R.HL.B.l);
      }
      break;
    case 0x0ce:
      {
        SET_INDEX(0x002);
      }
      break;
    case 0x0cf:
      {
        INDEX_CB_SET_REG(0x002, R.AF.B.h);
      }
      break;
    case 0x0d0:
      {
        INDEX_CB_SET_REG(0x004, R.BC.B.h);
      }
      break;
    case 0x0d1:
      {
        INDEX_CB_SET_REG(0x004, R.BC.B.l);
      }
      break;
    case 0x0d2:
      {
        INDEX_CB_SET_REG(0x004, R.DE.B.h);
      }
      break;
    case 0x0d3:
      {
        INDEX_CB_SET_REG(0x004, R.DE.B.l);
      }
      break;
    case 0x0d4:
      {
        INDEX_CB_SET_REG(0x004, R.HL.B.h);
      }
      break;
    case 0x0d5:
      {
        INDEX_CB_SET_REG(0x004, R.HL.B.l);
      }
      break;
    case 0x0d6:
      {
        SET_INDEX(0x004);
      }
      break;
    case 0x0d7:
      {
        INDEX_CB_SET_REG(0x004, R.AF.B.h);
      }
      break;
    case 0x0d8:
      {
        INDEX_CB_SET_REG(0x008, R.BC.B.h);
      }
      break;
    case 0x0d9:
      {
        INDEX_CB_SET_REG(0x008, R.BC.B.l);
      }
      break;
    case 0x0da:
      {
        INDEX_CB_SET_REG(0x008, R.DE.B.h);
      }
      break;
    case 0x0db:
      {
        INDEX_CB_SET_REG(0x008, R.DE.B.l);
      }
      break;
    case 0x0dc:
      {
        INDEX_CB_SET_REG(0x008, R.HL.B.h);
      }
      break;
    case 0x0dd:
      {
        INDEX_CB_SET_REG(0x008, R.HL.B.l);
      }
      break;
    case 0x0de:
      {
        SET_INDEX(0x008);
      }
      break;
    case 0x0df:
      {
        INDEX_CB_SET_REG(0x008, R.AF.B.h);
      }
      break;
    case 0x0e0:
      {
        INDEX_CB_SET_REG(0x010, R.BC.B.h);
      }
      break;
    case 0x0e1:
      {
        INDEX_CB_SET_REG(0x010, R.BC.B.l);
      }
      break;
    case 0x0e2:
      {
        INDEX_CB_SET_REG(0x010, R.DE.B.h);
      }
      break;
    case 0x0e3:
      {
        INDEX_CB_SET_REG(0x010, R.DE.B.l);
      }
      break;
    case 0x0e4:
      {
        INDEX_CB_SET_REG(0x010, R.HL.B.h);
      }
      break;
    case 0x0e5:
      {
        INDEX_CB_SET_REG(0x010, R.HL.B.l);
      }
      break;
    case 0x0e6:
      {
        SET_INDEX(0x010);
      }
      break;
    case 0x0e7:
      {
        INDEX_CB_SET_REG(0x010, R.AF.B.h);
      }
      break;
    case 0x0e8:
      {
        INDEX_CB_SET_REG(0x020, R.BC.B.h);
      }
      break;
    case 0x0e9:
      {
        INDEX_CB_SET_REG(0x020, R.BC.B.l);
      }
      break;
    case 0x0ea:
      {
        INDEX_CB_SET_REG(0x020, R.DE.B.h);
      }
      break;
    case 0x0eb:
      {
        INDEX_CB_SET_REG(0x020, R.DE.B.l);
      }
      break;
    case 0x0ec:
      {
        INDEX_CB_SET_REG(0x020, R.HL.B.h);
      }
      break;
    case 0x0ed:
      {
        INDEX_CB_SET_REG(0x020, R.HL.B.l);
      }
      break;
    case 0x0ee:
      {
        SET_INDEX(0x020);
      }
      break;
    case 0x0ef:
      {
        INDEX_CB_SET_REG(0x020, R.AF.B.h);
      }
      break;
    case 0x0f0:
      {
        INDEX_CB_SET_REG(0x040, R.BC.B.h);
      }
      break;
    case 0x0f1:
      {
        INDEX_CB_SET_REG(0x040, R.BC.B.l);
      }
      break;
    case 0x0f2:
      {
        INDEX_CB_SET_REG(0x040, R.DE.B.h);
      }
      break;
    case 0x0f3:
      {
        INDEX_CB_SET_REG(0x040, R.DE.B.l);
      }
      break;
    case 0x0f4:
      {
        INDEX_CB_SET_REG(0x040, R.HL.B.h);
      }
      break;
    case 0x0f5:
      {
        INDEX_CB_SET_REG(0x040, R.HL.B.l);
      }
      break;
    case 0x0f6:
      {
        SET_INDEX(0x040);
      }
      break;
    case 0x0f7:
      {
        INDEX_CB_SET_REG(0x040, R.AF.B.h);
      }
      break;
    case 0x0f8:
      {
        INDEX_CB_SET_REG(0x080, R.BC.B.h);
      }
      break;
    case 0x0f9:
      {
        INDEX_CB_SET_REG(0x080, R.BC.B.l);
      }
      break;
    case 0x0fa:
      {
        INDEX_CB_SET_REG(0x080, R.DE.B.h);
      }
      break;
    case 0x0fb:
      {
        INDEX_CB_SET_REG(0x080, R.DE.B.l);
      }
      break;
    case 0x0fc:
      {
        INDEX_CB_SET_REG(0x080, R.HL.B.h);
      }
      break;
    case 0x0fd:
      {
        INDEX_CB_SET_REG(0x080, R.HL.B.l);
      }
      break;
    case 0x0fe:
      {
        SET_INDEX(0x080);
      }
      break;
    case 0x0ff:
      {
        INDEX_CB_SET_REG(0x080, R.AF.B.h);
      }
      break;
    default:
      /* the following tells MSDEV 6 to not generate */
      /* code which checks if a input value to the */
      /* switch is not valid. */
#ifdef _MSC_VER
#  if (_MSC_VER >= 1200)
      __assume(0);
#  endif
#endif
      break;
    }
    INC_REFRESH(2);
    ADD_PC(4);
    R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
  }

  /***************************************************************************/
  EP128EMU_INLINE void Z80::FD_ExecuteInstruction()
  {
    uint8_t Opcode;
    Opcode = readOpcodeSecondByte();
    switch (Opcode) {
    case 0x000:
    case 0x001:
    case 0x002:
    case 0x003:
    case 0x004:
    case 0x005:
    case 0x006:
    case 0x007:
    case 0x008:
    case 0x00a:
    case 0x00b:
    case 0x00c:
    case 0x00d:
    case 0x00e:
    case 0x00f:
    case 0x010:
    case 0x011:
    case 0x012:
    case 0x013:
    case 0x014:
    case 0x015:
    case 0x016:
    case 0x017:
    case 0x018:
    case 0x01a:
    case 0x01b:
    case 0x01c:
    case 0x01d:
    case 0x01e:
    case 0x01f:
    case 0x020:
    case 0x027:
    case 0x028:
    case 0x02f:
    case 0x030:
    case 0x031:
    case 0x032:
    case 0x033:
    case 0x037:
    case 0x038:
    case 0x03a:
    case 0x03b:
    case 0x03c:
    case 0x03d:
    case 0x03e:
    case 0x03f:
    case 0x040:
    case 0x041:
    case 0x042:
    case 0x043:
    case 0x047:
    case 0x048:
    case 0x049:
    case 0x04a:
    case 0x04b:
    case 0x04f:
    case 0x050:
    case 0x051:
    case 0x052:
    case 0x053:
    case 0x057:
    case 0x058:
    case 0x059:
    case 0x05a:
    case 0x05b:
    case 0x05f:
    case 0x076:
    case 0x078:
    case 0x079:
    case 0x07a:
    case 0x07b:
    case 0x07f:
    case 0x080:
    case 0x081:
    case 0x082:
    case 0x083:
    case 0x087:
    case 0x088:
    case 0x089:
    case 0x08a:
    case 0x08b:
    case 0x08f:
    case 0x090:
    case 0x091:
    case 0x092:
    case 0x093:
    case 0x097:
    case 0x098:
    case 0x099:
    case 0x09a:
    case 0x09b:
    case 0x09f:
    case 0x0a0:
    case 0x0a1:
    case 0x0a2:
    case 0x0a3:
    case 0x0a7:
    case 0x0a8:
    case 0x0a9:
    case 0x0aa:
    case 0x0ab:
    case 0x0af:
    case 0x0b0:
    case 0x0b1:
    case 0x0b2:
    case 0x0b3:
    case 0x0b7:
    case 0x0b8:
    case 0x0b9:
    case 0x0ba:
    case 0x0bb:
    case 0x0bf:
    case 0x0c0:
    case 0x0c1:
    case 0x0c2:
    case 0x0c3:
    case 0x0c4:
    case 0x0c5:
    case 0x0c6:
    case 0x0c7:
    case 0x0c8:
    case 0x0c9:
    case 0x0ca:
    case 0x0cc:
    case 0x0cd:
    case 0x0ce:
    case 0x0cf:
    case 0x0d0:
    case 0x0d1:
    case 0x0d2:
    case 0x0d3:
    case 0x0d4:
    case 0x0d5:
    case 0x0d6:
    case 0x0d7:
    case 0x0d8:
    case 0x0d9:
    case 0x0da:
    case 0x0db:
    case 0x0dc:
    case 0x0dd:
    case 0x0de:
    case 0x0df:
    case 0x0e0:
    case 0x0e2:
    case 0x0e4:
    case 0x0e6:
    case 0x0e7:
    case 0x0e8:
    case 0x0ea:
    case 0x0eb:
    case 0x0ec:
    case 0x0ed:
    case 0x0ee:
    case 0x0ef:
    case 0x0f0:
    case 0x0f1:
    case 0x0f2:
    case 0x0f3:
    case 0x0f4:
    case 0x0f5:
    case 0x0f6:
    case 0x0f7:
    case 0x0f8:
    case 0x0fa:
    case 0x0fb:
    case 0x0fc:
    case 0x0fd:
    case 0x0fe:
    case 0x0ff:
      {
        PrefixIgnore();
      }
      break;
    case 0x009:
      {
        ADD_RR_rr(R.IY.W, R.BC.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x019:
      {
        ADD_RR_rr(R.IY.W, R.DE.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x021:
      {
        LD_INDEXRR_nn(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x022:
      {
        LD_nnnn_INDEXRR(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x023:
      {
        INC_rp(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x024:
      {
        INC_R(R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x025:
      {
        DEC_R(R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x026:
      {
        LD_RI_n(R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x029:
      {
        ADD_RR_rr(R.IY.W, R.IY.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02a:
      {
        LD_INDEXRR_nnnn(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02b:
      {
        DEC_rp(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02c:
      {
        INC_R(R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02d:
      {
        DEC_R(R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02e:
      {
        LD_RI_n(R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x034:
      {
        _INC_INDEX_(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x035:
      {
        _DEC_INDEX_(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x036:
      {
        LD_INDEX_n(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x039:
      {
        ADD_RR_rr(R.IY.W, R.SP.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x044:
      {
        LD_R_R(R.BC.B.h, R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x045:
      {
        LD_R_R(R.BC.B.h, R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x046:
      {
        LD_R_INDEX(R.IY.W, R.BC.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04c:
      {
        LD_R_R(R.BC.B.l, R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04d:
      {
        LD_R_R(R.BC.B.l, R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04e:
      {
        LD_R_INDEX(R.IY.W, R.BC.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x054:
      {
        LD_R_R(R.DE.B.h, R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x055:
      {
        LD_R_R(R.DE.B.h, R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x056:
      {
        LD_R_INDEX(R.IY.W, R.DE.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05c:
      {
        LD_R_R(R.DE.B.l, R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05d:
      {
        LD_R_R(R.DE.B.l, R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05e:
      {
        LD_R_INDEX(R.IY.W, R.DE.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x060:
      {
        LD_R_R(R.IY.B.h, R.BC.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x061:
      {
        LD_R_R(R.IY.B.h, R.BC.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x062:
      {
        LD_R_R(R.IY.B.h, R.DE.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x063:
      {
        LD_R_R(R.IY.B.h, R.DE.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x064:
      {
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x065:
      {
        LD_R_R(R.IY.B.h, R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x066:
      {
        LD_R_INDEX(R.IY.W, R.HL.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x067:
      {
        LD_R_R(R.IY.B.h, R.AF.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x068:
      {
        LD_R_R(R.IY.B.l, R.BC.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x069:
      {
        LD_R_R(R.IY.B.l, R.BC.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06a:
      {
        LD_R_R(R.IY.B.l, R.DE.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06b:
      {
        LD_R_R(R.IY.B.l, R.DE.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06c:
      {
        LD_R_R(R.IY.B.l, R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06d:
      {
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06e:
      {
        LD_R_INDEX(R.IY.W, R.HL.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06f:
      {
        LD_R_R(R.IY.B.l, R.AF.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x070:
      {
        LD_INDEX_R(R.IY.W, R.BC.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x071:
      {
        LD_INDEX_R(R.IY.W, R.BC.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x072:
      {
        LD_INDEX_R(R.IY.W, R.DE.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x073:
      {
        LD_INDEX_R(R.IY.W, R.DE.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x074:
      {
        LD_INDEX_R(R.IY.W, R.HL.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x075:
      {
        LD_INDEX_R(R.IY.W, R.HL.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x077:
      {
        LD_INDEX_R(R.IY.W, R.AF.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07c:
      {
        LD_R_R(R.AF.B.h, R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07d:
      {
        LD_R_R(R.AF.B.h, R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07e:
      {
        LD_R_INDEX(R.IY.W, R.AF.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x084:
      {
        ADD_A_R(R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x085:
      {
        ADD_A_R(R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x086:
      {
        ADD_A_INDEX(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08c:
      {
        ADC_A_R(R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08d:
      {
        ADC_A_R(R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08e:
      {
        ADC_A_INDEX(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x094:
      {
        SUB_A_R(R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x095:
      {
        SUB_A_R(R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x096:
      {
        SUB_A_INDEX(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09c:
      {
        SBC_A_R(R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09d:
      {
        SBC_A_R(R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09e:
      {
        SBC_A_INDEX(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a4:
      {
        AND_A_R(R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a5:
      {
        AND_A_R(R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a6:
      {
        AND_A_INDEX(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ac:
      {
        XOR_A_R(R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ad:
      {
        XOR_A_R(R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ae:
      {
        XOR_A_INDEX(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b4:
      {
        OR_A_R(R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b5:
      {
        OR_A_R(R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b6:
      {
        OR_A_INDEX(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0bc:
      {
        CP_A_R(R.IY.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0bd:
      {
        CP_A_R(R.IY.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0be:
      {
        CP_A_INDEX(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0cb:
      {
        SETUP_INDEXED_ADDRESS(R.IY.W);
        Index_CB_ExecuteInstruction();
      }
      break;
    case 0x0e1:
      {
        R.IY.W = POP();
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e3:
      {
        EX_SP_rr(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e5:
      {
        PUSH(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e9:
      {
        JP_rp(R.IY.W);
        INC_REFRESH(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0f9:
      {
        LD_SP_rp(R.IY.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    default:
      /* the following tells MSDEV 6 to not generate */
      /* code which checks if a input value to the */
      /* switch is not valid. */
#ifdef _MSC_VER
#  if (_MSC_VER >= 1200)
      __assume(0);
#  endif
#endif
      break;
    }
  }

  /***************************************************************************/
  EP128EMU_INLINE void Z80::DD_ExecuteInstruction()
  {
    uint8_t Opcode;
    Opcode = readOpcodeSecondByte();
    switch (Opcode) {
    case 0x000:
    case 0x001:
    case 0x002:
    case 0x003:
    case 0x004:
    case 0x005:
    case 0x006:
    case 0x007:
    case 0x008:
    case 0x00a:
    case 0x00b:
    case 0x00c:
    case 0x00d:
    case 0x00e:
    case 0x00f:
    case 0x010:
    case 0x011:
    case 0x012:
    case 0x013:
    case 0x014:
    case 0x015:
    case 0x016:
    case 0x017:
    case 0x018:
    case 0x01a:
    case 0x01b:
    case 0x01c:
    case 0x01d:
    case 0x01e:
    case 0x01f:
    case 0x020:
    case 0x027:
    case 0x028:
    case 0x02f:
    case 0x030:
    case 0x031:
    case 0x032:
    case 0x033:
    case 0x037:
    case 0x038:
    case 0x03a:
    case 0x03b:
    case 0x03c:
    case 0x03d:
    case 0x03e:
    case 0x03f:
    case 0x040:
    case 0x041:
    case 0x042:
    case 0x043:
    case 0x047:
    case 0x048:
    case 0x049:
    case 0x04a:
    case 0x04b:
    case 0x04f:
    case 0x050:
    case 0x051:
    case 0x052:
    case 0x053:
    case 0x057:
    case 0x058:
    case 0x059:
    case 0x05a:
    case 0x05b:
    case 0x05f:
    case 0x076:
    case 0x078:
    case 0x079:
    case 0x07a:
    case 0x07b:
    case 0x07f:
    case 0x080:
    case 0x081:
    case 0x082:
    case 0x083:
    case 0x087:
    case 0x088:
    case 0x089:
    case 0x08a:
    case 0x08b:
    case 0x08f:
    case 0x090:
    case 0x091:
    case 0x092:
    case 0x093:
    case 0x097:
    case 0x098:
    case 0x099:
    case 0x09a:
    case 0x09b:
    case 0x09f:
    case 0x0a0:
    case 0x0a1:
    case 0x0a2:
    case 0x0a3:
    case 0x0a7:
    case 0x0a8:
    case 0x0a9:
    case 0x0aa:
    case 0x0ab:
    case 0x0af:
    case 0x0b0:
    case 0x0b1:
    case 0x0b2:
    case 0x0b3:
    case 0x0b7:
    case 0x0b8:
    case 0x0b9:
    case 0x0ba:
    case 0x0bb:
    case 0x0bf:
    case 0x0c0:
    case 0x0c1:
    case 0x0c2:
    case 0x0c3:
    case 0x0c4:
    case 0x0c5:
    case 0x0c6:
    case 0x0c7:
    case 0x0c8:
    case 0x0c9:
    case 0x0ca:
    case 0x0cc:
    case 0x0cd:
    case 0x0ce:
    case 0x0cf:
    case 0x0d0:
    case 0x0d1:
    case 0x0d2:
    case 0x0d3:
    case 0x0d4:
    case 0x0d5:
    case 0x0d6:
    case 0x0d7:
    case 0x0d8:
    case 0x0d9:
    case 0x0da:
    case 0x0db:
    case 0x0dc:
    case 0x0dd:
    case 0x0de:
    case 0x0df:
    case 0x0e0:
    case 0x0e2:
    case 0x0e4:
    case 0x0e6:
    case 0x0e7:
    case 0x0e8:
    case 0x0ea:
    case 0x0eb:
    case 0x0ec:
    case 0x0ed:
    case 0x0ee:
    case 0x0ef:
    case 0x0f0:
    case 0x0f1:
    case 0x0f2:
    case 0x0f3:
    case 0x0f4:
    case 0x0f5:
    case 0x0f6:
    case 0x0f7:
    case 0x0f8:
    case 0x0fa:
    case 0x0fb:
    case 0x0fc:
    case 0x0fd:
    case 0x0fe:
    case 0x0ff:
      {
        PrefixIgnore();
      }
      break;
    case 0x009:
      {
        ADD_RR_rr(R.IX.W, R.BC.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x019:
      {
        ADD_RR_rr(R.IX.W, R.DE.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x021:
      {
        LD_INDEXRR_nn(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x022:
      {
        LD_nnnn_INDEXRR(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x023:
      {
        INC_rp(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x024:
      {
        INC_R(R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x025:
      {
        DEC_R(R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x026:
      {
        LD_RI_n(R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x029:
      {
        ADD_RR_rr(R.IX.W, R.IX.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02a:
      {
        LD_INDEXRR_nnnn(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02b:
      {
        DEC_rp(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02c:
      {
        INC_R(R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02d:
      {
        DEC_R(R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02e:
      {
        LD_RI_n(R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x034:
      {
        _INC_INDEX_(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x035:
      {
        _DEC_INDEX_(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x036:
      {
        LD_INDEX_n(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x039:
      {
        ADD_RR_rr(R.IX.W, R.SP.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x044:
      {
        LD_R_R(R.BC.B.h, R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x045:
      {
        LD_R_R(R.BC.B.h, R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x046:
      {
        LD_R_INDEX(R.IX.W, R.BC.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04c:
      {
        LD_R_R(R.BC.B.l, R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04d:
      {
        LD_R_R(R.BC.B.l, R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04e:
      {
        LD_R_INDEX(R.IX.W, R.BC.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x054:
      {
        LD_R_R(R.DE.B.h, R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x055:
      {
        LD_R_R(R.DE.B.h, R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x056:
      {
        LD_R_INDEX(R.IX.W, R.DE.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05c:
      {
        LD_R_R(R.DE.B.l, R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05d:
      {
        LD_R_R(R.DE.B.l, R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05e:
      {
        LD_R_INDEX(R.IX.W, R.DE.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x060:
      {
        LD_R_R(R.IX.B.h, R.BC.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x061:
      {
        LD_R_R(R.IX.B.h, R.BC.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x062:
      {
        LD_R_R(R.IX.B.h, R.DE.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x063:
      {
        LD_R_R(R.IX.B.h, R.DE.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x064:
      {
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x065:
      {
        LD_R_R(R.IX.B.h, R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x066:
      {
        LD_R_INDEX(R.IX.W, R.HL.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x067:
      {
        LD_R_R(R.IX.B.h, R.AF.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x068:
      {
        LD_R_R(R.IX.B.l, R.BC.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x069:
      {
        LD_R_R(R.IX.B.l, R.BC.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06a:
      {
        LD_R_R(R.IX.B.l, R.DE.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06b:
      {
        LD_R_R(R.IX.B.l, R.DE.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06c:
      {
        LD_R_R(R.IX.B.l, R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06d:
      {
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06e:
      {
        LD_R_INDEX(R.IX.W, R.HL.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06f:
      {
        LD_R_R(R.IX.B.l, R.AF.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x070:
      {
        LD_INDEX_R(R.IX.W, R.BC.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x071:
      {
        LD_INDEX_R(R.IX.W, R.BC.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x072:
      {
        LD_INDEX_R(R.IX.W, R.DE.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x073:
      {
        LD_INDEX_R(R.IX.W, R.DE.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x074:
      {
        LD_INDEX_R(R.IX.W, R.HL.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x075:
      {
        LD_INDEX_R(R.IX.W, R.HL.B.l);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x077:
      {
        LD_INDEX_R(R.IX.W, R.AF.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07c:
      {
        LD_R_R(R.AF.B.h, R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07d:
      {
        LD_R_R(R.AF.B.h, R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07e:
      {
        LD_R_INDEX(R.IX.W, R.AF.B.h);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x084:
      {
        ADD_A_R(R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x085:
      {
        ADD_A_R(R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x086:
      {
        ADD_A_INDEX(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08c:
      {
        ADC_A_R(R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08d:
      {
        ADC_A_R(R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08e:
      {
        ADC_A_INDEX(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x094:
      {
        SUB_A_R(R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x095:
      {
        SUB_A_R(R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x096:
      {
        SUB_A_INDEX(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09c:
      {
        SBC_A_R(R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09d:
      {
        SBC_A_R(R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09e:
      {
        SBC_A_INDEX(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a4:
      {
        AND_A_R(R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a5:
      {
        AND_A_R(R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a6:
      {
        AND_A_INDEX(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ac:
      {
        XOR_A_R(R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ad:
      {
        XOR_A_R(R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ae:
      {
        XOR_A_INDEX(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b4:
      {
        OR_A_R(R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b5:
      {
        OR_A_R(R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b6:
      {
        OR_A_INDEX(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0bc:
      {
        CP_A_R(R.IX.B.h);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0bd:
      {
        CP_A_R(R.IX.B.l);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0be:
      {
        CP_A_INDEX(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0cb:
      {
        SETUP_INDEXED_ADDRESS(R.IX.W);
        Index_CB_ExecuteInstruction();
      }
      break;
    case 0x0e1:
      {
        R.IX.W = POP();
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e3:
      {
        EX_SP_rr(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e5:
      {
        PUSH(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e9:
      {
        JP_rp(R.IX.W);
        INC_REFRESH(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0f9:
      {
        LD_SP_rp(R.IX.W);
        INC_REFRESH(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    default:
      /* the following tells MSDEV 6 to not generate */
      /* code which checks if a input value to the */
      /* switch is not valid. */
#ifdef _MSC_VER
#  if (_MSC_VER >= 1200)
      __assume(0);
#  endif
#endif
      break;
    }
  }

  /***************************************************************************/
  EP128EMU_INLINE void Z80::ED_ExecuteInstruction()
  {
    INC_REFRESH(2);
    uint8_t Opcode;
    Opcode = readOpcodeSecondByte();
    switch (Opcode) {
    case 0x000:
    case 0x001:
    case 0x002:
    case 0x003:
    case 0x004:
    case 0x005:
    case 0x006:
    case 0x007:
    case 0x008:
    case 0x009:
    case 0x00a:
    case 0x00b:
    case 0x00c:
    case 0x00d:
    case 0x00e:
    case 0x00f:
    case 0x010:
    case 0x011:
    case 0x012:
    case 0x013:
    case 0x014:
    case 0x015:
    case 0x016:
    case 0x017:
    case 0x018:
    case 0x019:
    case 0x01a:
    case 0x01b:
    case 0x01c:
    case 0x01d:
    case 0x01e:
    case 0x01f:
    case 0x020:
    case 0x021:
    case 0x022:
    case 0x023:
    case 0x024:
    case 0x025:
    case 0x026:
    case 0x027:
    case 0x028:
    case 0x029:
    case 0x02a:
    case 0x02b:
    case 0x02c:
    case 0x02d:
    case 0x02e:
    case 0x02f:
    case 0x030:
    case 0x031:
    case 0x032:
    case 0x033:
    case 0x034:
    case 0x035:
    case 0x036:
    case 0x037:
    case 0x038:
    case 0x039:
    case 0x03a:
    case 0x03b:
    case 0x03c:
    case 0x03d:
    case 0x03e:
    case 0x03f:
    case 0x080:
    case 0x081:
    case 0x082:
    case 0x083:
    case 0x084:
    case 0x085:
    case 0x086:
    case 0x087:
    case 0x088:
    case 0x089:
    case 0x08a:
    case 0x08b:
    case 0x08c:
    case 0x08d:
    case 0x08e:
    case 0x08f:
    case 0x090:
    case 0x091:
    case 0x092:
    case 0x093:
    case 0x094:
    case 0x095:
    case 0x096:
    case 0x097:
    case 0x098:
    case 0x099:
    case 0x09a:
    case 0x09b:
    case 0x09c:
    case 0x09d:
    case 0x09e:
    case 0x09f:
    case 0x0a4:
    case 0x0a5:
    case 0x0a6:
    case 0x0a7:
    case 0x0ac:
    case 0x0ad:
    case 0x0ae:
    case 0x0af:
    case 0x0b4:
    case 0x0b5:
    case 0x0b6:
    case 0x0b7:
    case 0x0bc:
    case 0x0bd:
    case 0x0be:
    case 0x0bf:
    case 0x0c0:
    case 0x0c1:
    case 0x0c2:
    case 0x0c3:
    case 0x0c4:
    case 0x0c5:
    case 0x0c6:
    case 0x0c7:
    case 0x0c8:
    case 0x0c9:
    case 0x0ca:
    case 0x0cb:
    case 0x0cc:
    case 0x0cd:
    case 0x0ce:
    case 0x0cf:
    case 0x0d0:
    case 0x0d1:
    case 0x0d2:
    case 0x0d3:
    case 0x0d4:
    case 0x0d5:
    case 0x0d6:
    case 0x0d7:
    case 0x0d8:
    case 0x0d9:
    case 0x0da:
    case 0x0db:
    case 0x0dc:
    case 0x0dd:
    case 0x0de:
    case 0x0df:
    case 0x0e0:
    case 0x0e1:
    case 0x0e2:
    case 0x0e3:
    case 0x0e4:
    case 0x0e5:
    case 0x0e6:
    case 0x0e7:
    case 0x0e8:
    case 0x0e9:
    case 0x0ea:
    case 0x0eb:
    case 0x0ec:
    case 0x0ed:
    case 0x0ee:
    case 0x0ef:
    case 0x0f0:
    case 0x0f1:
    case 0x0f2:
    case 0x0f3:
    case 0x0f4:
    case 0x0f5:
    case 0x0f6:
    case 0x0f7:
    case 0x0f8:
    case 0x0f9:
    case 0x0fa:
    case 0x0fb:
    case 0x0fc:
    case 0x0fd:
    case 0x0ff:
      {
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;

    case 0x0fe:
      {
        tapePatch();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;

    case 0x040:
      {
        R.BC.B.h = doIn(R.BC.W);
        R.AF.B.l =
            (R.AF.B.l & Z80_CARRY_FLAG) | t.zeroSignParityTable[R.BC.B.h];
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x041:
      {
        doOut(R.BC.W, R.BC.B.h);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x042:
      {
        SBC_HL_rr(R.BC.W);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x043:
      {
        LD_nnnn_RR(R.BC.W);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x044:
      {
        NEG();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x045:
      {
        RETN();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x046:
      {
        SET_IM(0);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x047:
      {
        LD_I_A();
        ADD_PC(2);
        updateCycle();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x048:
      {
        R.BC.B.l = doIn(R.BC.W);
        R.AF.B.l =
            (R.AF.B.l & Z80_CARRY_FLAG) | t.zeroSignParityTable[R.BC.B.l];
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x049:
      {
        doOut(R.BC.W, R.BC.B.l);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04a:
      {
        ADC_HL_rr(R.BC.W);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04b:
      {
        LD_RR_nnnn(R.BC.W);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04c:
      {
        NEG();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04d:
      {
        RETI();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04e:
      {
        SET_IM(0);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04f:
      {
        LD_R_A();
        ADD_PC(2);
        updateCycle();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x050:
      {
        R.DE.B.h = doIn(R.BC.W);
        R.AF.B.l =
            (R.AF.B.l & Z80_CARRY_FLAG) | t.zeroSignParityTable[R.DE.B.h];
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x051:
      {
        doOut(R.BC.W, R.DE.B.h);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x052:
      {
        SBC_HL_rr(R.DE.W);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x053:
      {
        LD_nnnn_RR(R.DE.W);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x054:
      {
        NEG();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x055:
      {
        RETN();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x056:
      {
        SET_IM(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x057:
      {
        LD_A_I();
        ADD_PC(2);
        updateCycle();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
        checkNMOSBug();
      }
      break;
    case 0x058:
      {
        R.DE.B.l = doIn(R.BC.W);
        R.AF.B.l =
            (R.AF.B.l & Z80_CARRY_FLAG) | t.zeroSignParityTable[R.DE.B.l];
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x059:
      {
        doOut(R.BC.W, R.DE.B.l);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05a:
      {
        ADC_HL_rr(R.DE.W);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05b:
      {
        LD_RR_nnnn(R.DE.W);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05c:
      {
        NEG();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05d:
      {
        RETI();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05e:
      {
        SET_IM(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05f:
      {
        LD_A_R();
        ADD_PC(2);
        updateCycle();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
        checkNMOSBug();
      }
      break;
    case 0x060:
      {
        R.HL.B.h = doIn(R.BC.W);
        R.AF.B.l =
            (R.AF.B.l & Z80_CARRY_FLAG) | t.zeroSignParityTable[R.HL.B.h];
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x061:
      {
        doOut(R.BC.W, R.HL.B.h);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x062:
      {
        SBC_HL_rr(R.HL.W);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x063:
      {
        LD_nnnn_RR(R.HL.W);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x064:
      {
        NEG();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x065:
      {
        RETN();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x066:
      {
        SET_IM(0);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x067:
      {
        RRD();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x068:
      {
        R.HL.B.l = doIn(R.BC.W);
        R.AF.B.l =
            (R.AF.B.l & Z80_CARRY_FLAG) | t.zeroSignParityTable[R.HL.B.l];
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x069:
      {
        doOut(R.BC.W, R.HL.B.l);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06a:
      {
        ADC_HL_rr(R.HL.W);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06b:
      {
        LD_RR_nnnn(R.HL.W);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06c:
      {
        NEG();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06d:
      {
        RETI();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06e:
      {
        SET_IM(0);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06f:
      {
        RLD();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x070:
      {
        Z80_BYTE  tempByte = doIn(R.BC.W);
        R.AF.B.l =
            (R.AF.B.l & Z80_CARRY_FLAG) | t.zeroSignParityTable[tempByte];
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x071:
      {
        // 0 = NMOS Z80, 0xFF = CMOS
        doOut(R.BC.W, 0);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x072:
      {
        SBC_HL_rr(R.SP.W);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x073:
      {
        LD_nnnn_RR(R.SP.W);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x074:
      {
        NEG();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x075:
      {
        RETN();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x076:
      {
        SET_IM(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x077:
      {
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x078:
      {
        R.AF.B.h = doIn(R.BC.W);
        R.AF.B.l =
            (R.AF.B.l & Z80_CARRY_FLAG) | t.zeroSignParityTable[R.AF.B.h];
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x079:
      {
        doOut(R.BC.W, R.AF.B.h);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07a:
      {
        ADC_HL_rr(R.SP.W);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07b:
      {
        LD_RR_nnnn(R.SP.W);
        ADD_PC(4);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07c:
      {
        NEG();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07d:
      {
        RETI();
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07e:
      {
        SET_IM(2);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07f:
      {
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a0:
      {
        LDI();
        ADD_PC(2);
        updateCycles(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a1:
      {
        CPI();
        ADD_PC(2);
        updateCycles(5);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a2:
      {
        INI();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a3:
      {
        OUTI();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a8:
      {
        LDD();
        ADD_PC(2);
        updateCycles(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a9:
      {
        CPD();
        ADD_PC(2);
        updateCycles(5);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0aa:
      {
        IND();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ab:
      {
        OUTD();
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b0:
      {
        LDI();
        if (Z80_TEST_PARITY_EVEN) {
          updateCycles(7);
        }
        else {
          ADD_PC(2);
          updateCycles(2);
        }
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b1:
      {
        CPI();
        if ((Z80_FLAGS_REG & (Z80_PARITY_FLAG | Z80_ZERO_FLAG))
            == Z80_PARITY_FLAG) {
          updateCycles(10);
        }
        else {
          ADD_PC(2);
          updateCycles(5);
        }
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b2:
      {
        INI();
        if (Z80_FLAGS_REG & Z80_ZERO_FLAG)
          ADD_PC(2);
        else
          updateCycles(5);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b3:
      {
        OUTI();
        if (Z80_FLAGS_REG & Z80_ZERO_FLAG)
          ADD_PC(2);
        else
          updateCycles(5);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b8:
      {
        LDD();
        if (Z80_FLAGS_REG & Z80_PARITY_FLAG) {
          updateCycles(7);
        }
        else {
          ADD_PC(2);
          updateCycles(2);
        }
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b9:
      {
        CPD();
        if ((Z80_FLAGS_REG & (Z80_PARITY_FLAG | Z80_ZERO_FLAG))
            == Z80_PARITY_FLAG) {
          updateCycles(10);
        }
        else {
          ADD_PC(2);
          updateCycles(5);
        }
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ba:
      {
        IND();
        if (Z80_FLAGS_REG & Z80_ZERO_FLAG)
          ADD_PC(2);
        else
          updateCycles(5);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0bb:
      {
        OUTD();
        if (Z80_FLAGS_REG & Z80_ZERO_FLAG)
          ADD_PC(2);
        else
          updateCycles(5);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    default:
      /* the following tells MSDEV 6 to not generate */
      /* code which checks if a input value to the */
      /* switch is not valid. */
#ifdef _MSC_VER
#  if (_MSC_VER >= 1200)
      __assume(0);
#  endif
#endif
      break;
    }
  }

  /***************************************************************************/
  EP128EMU_INLINE void Z80::CB_ExecuteInstruction()
  {
    uint8_t Opcode;
    Opcode = readOpcodeSecondByte();
    switch (Opcode) {
    case 0x000:
      {
        RLC_REG(R.BC.B.h);
      }
      break;
    case 0x001:
      {
        RLC_REG(R.BC.B.l);
      }
      break;
    case 0x002:
      {
        RLC_REG(R.DE.B.h);
      }
      break;
    case 0x003:
      {
        RLC_REG(R.DE.B.l);
      }
      break;
    case 0x004:
      {
        RLC_REG(R.HL.B.h);
      }
      break;
    case 0x005:
      {
        RLC_REG(R.HL.B.l);
      }
      break;
    case 0x006:
      {
        RLC_HL();
      }
      break;
    case 0x007:
      {
        RLC_REG(R.AF.B.h);
      }
      break;
    case 0x008:
      {
        RRC_REG(R.BC.B.h);
      }
      break;
    case 0x009:
      {
        RRC_REG(R.BC.B.l);
      }
      break;
    case 0x00a:
      {
        RRC_REG(R.DE.B.h);
      }
      break;
    case 0x00b:
      {
        RRC_REG(R.DE.B.l);
      }
      break;
    case 0x00c:
      {
        RRC_REG(R.HL.B.h);
      }
      break;
    case 0x00d:
      {
        RRC_REG(R.HL.B.l);
      }
      break;
    case 0x00e:
      {
        RRC_HL();
      }
      break;
    case 0x00f:
      {
        RRC_REG(R.AF.B.h);
      }
      break;
    case 0x010:
      {
        RL_REG(R.BC.B.h);
      }
      break;
    case 0x011:
      {
        RL_REG(R.BC.B.l);
      }
      break;
    case 0x012:
      {
        RL_REG(R.DE.B.h);
      }
      break;
    case 0x013:
      {
        RL_REG(R.DE.B.l);
      }
      break;
    case 0x014:
      {
        RL_REG(R.HL.B.h);
      }
      break;
    case 0x015:
      {
        RL_REG(R.HL.B.l);
      }
      break;
    case 0x016:
      {
        RL_HL();
      }
      break;
    case 0x017:
      {
        RL_REG(R.AF.B.h);
      }
      break;
    case 0x018:
      {
        RR_REG(R.BC.B.h);
      }
      break;
    case 0x019:
      {
        RR_REG(R.BC.B.l);
      }
      break;
    case 0x01a:
      {
        RR_REG(R.DE.B.h);
      }
      break;
    case 0x01b:
      {
        RR_REG(R.DE.B.l);
      }
      break;
    case 0x01c:
      {
        RR_REG(R.HL.B.h);
      }
      break;
    case 0x01d:
      {
        RR_REG(R.HL.B.l);
      }
      break;
    case 0x01e:
      {
        RR_HL();
      }
      break;
    case 0x01f:
      {
        RR_REG(R.AF.B.h);
      }
      break;
    case 0x020:
      {
        SLA_REG(R.BC.B.h);
      }
      break;
    case 0x021:
      {
        SLA_REG(R.BC.B.l);
      }
      break;
    case 0x022:
      {
        SLA_REG(R.DE.B.h);
      }
      break;
    case 0x023:
      {
        SLA_REG(R.DE.B.l);
      }
      break;
    case 0x024:
      {
        SLA_REG(R.HL.B.h);
      }
      break;
    case 0x025:
      {
        SLA_REG(R.HL.B.l);
      }
      break;
    case 0x026:
      {
        SLA_HL();
      }
      break;
    case 0x027:
      {
        SLA_REG(R.AF.B.h);
      }
      break;
    case 0x028:
      {
        SRA_REG(R.BC.B.h);
      }
      break;
    case 0x029:
      {
        SRA_REG(R.BC.B.l);
      }
      break;
    case 0x02a:
      {
        SRA_REG(R.DE.B.h);
      }
      break;
    case 0x02b:
      {
        SRA_REG(R.DE.B.l);
      }
      break;
    case 0x02c:
      {
        SRA_REG(R.HL.B.h);
      }
      break;
    case 0x02d:
      {
        SRA_REG(R.HL.B.l);
      }
      break;
    case 0x02e:
      {
        SRA_HL();
      }
      break;
    case 0x02f:
      {
        SRA_REG(R.AF.B.h);
      }
      break;
    case 0x030:
      {
        SLL_REG(R.BC.B.h);
      }
      break;
    case 0x031:
      {
        SLL_REG(R.BC.B.l);
      }
      break;
    case 0x032:
      {
        SLL_REG(R.DE.B.h);
      }
      break;
    case 0x033:
      {
        SLL_REG(R.DE.B.l);
      }
      break;
    case 0x034:
      {
        SLL_REG(R.HL.B.h);
      }
      break;
    case 0x035:
      {
        SLL_REG(R.HL.B.l);
      }
      break;
    case 0x036:
      {
        SLL_HL();
      }
      break;
    case 0x037:
      {
        SLL_REG(R.AF.B.h);
      }
      break;
    case 0x038:
      {
        SRL_REG(R.BC.B.h);
      }
      break;
    case 0x039:
      {
        SRL_REG(R.BC.B.l);
      }
      break;
    case 0x03a:
      {
        SRL_REG(R.DE.B.h);
      }
      break;
    case 0x03b:
      {
        SRL_REG(R.DE.B.l);
      }
      break;
    case 0x03c:
      {
        SRL_REG(R.HL.B.h);
      }
      break;
    case 0x03d:
      {
        SRL_REG(R.HL.B.l);
      }
      break;
    case 0x03e:
      {
        SRL_HL();
      }
      break;
    case 0x03f:
      {
        SRL_REG(R.AF.B.h);
      }
      break;
    case 0x040:
      {
        BIT_REG(0, R.BC.B.h);
      }
      break;
    case 0x041:
      {
        BIT_REG(0, R.BC.B.l);
      }
      break;
    case 0x042:
      {
        BIT_REG(0, R.DE.B.h);
      }
      break;
    case 0x043:
      {
        BIT_REG(0, R.DE.B.l);
      }
      break;
    case 0x044:
      {
        BIT_REG(0, R.HL.B.h);
      }
      break;
    case 0x045:
      {
        BIT_REG(0, R.HL.B.l);
      }
      break;
    case 0x046:
      {
        BIT_HL(0);
      }
      break;
    case 0x047:
      {
        BIT_REG(0, R.AF.B.h);
      }
      break;
    case 0x048:
      {
        BIT_REG(1, R.BC.B.h);
      }
      break;
    case 0x049:
      {
        BIT_REG(1, R.BC.B.l);
      }
      break;
    case 0x04a:
      {
        BIT_REG(1, R.DE.B.h);
      }
      break;
    case 0x04b:
      {
        BIT_REG(1, R.DE.B.l);
      }
      break;
    case 0x04c:
      {
        BIT_REG(1, R.HL.B.h);
      }
      break;
    case 0x04d:
      {
        BIT_REG(1, R.HL.B.l);
      }
      break;
    case 0x04e:
      {
        BIT_HL(1);
      }
      break;
    case 0x04f:
      {
        BIT_REG(1, R.AF.B.h);
      }
      break;
    case 0x050:
      {
        BIT_REG(2, R.BC.B.h);
      }
      break;
    case 0x051:
      {
        BIT_REG(2, R.BC.B.l);
      }
      break;
    case 0x052:
      {
        BIT_REG(2, R.DE.B.h);
      }
      break;
    case 0x053:
      {
        BIT_REG(2, R.DE.B.l);
      }
      break;
    case 0x054:
      {
        BIT_REG(2, R.HL.B.h);
      }
      break;
    case 0x055:
      {
        BIT_REG(2, R.HL.B.l);
      }
      break;
    case 0x056:
      {
        BIT_HL(2);
      }
      break;
    case 0x057:
      {
        BIT_REG(2, R.AF.B.h);
      }
      break;
    case 0x058:
      {
        BIT_REG(3, R.BC.B.h);
      }
      break;
    case 0x059:
      {
        BIT_REG(3, R.BC.B.l);
      }
      break;
    case 0x05a:
      {
        BIT_REG(3, R.DE.B.h);
      }
      break;
    case 0x05b:
      {
        BIT_REG(3, R.DE.B.l);
      }
      break;
    case 0x05c:
      {
        BIT_REG(3, R.HL.B.h);
      }
      break;
    case 0x05d:
      {
        BIT_REG(3, R.HL.B.l);
      }
      break;
    case 0x05e:
      {
        BIT_HL(3);
      }
      break;
    case 0x05f:
      {
        BIT_REG(3, R.AF.B.h);
      }
      break;
    case 0x060:
      {
        BIT_REG(4, R.BC.B.h);
      }
      break;
    case 0x061:
      {
        BIT_REG(4, R.BC.B.l);
      }
      break;
    case 0x062:
      {
        BIT_REG(4, R.DE.B.h);
      }
      break;
    case 0x063:
      {
        BIT_REG(4, R.DE.B.l);
      }
      break;
    case 0x064:
      {
        BIT_REG(4, R.HL.B.h);
      }
      break;
    case 0x065:
      {
        BIT_REG(4, R.HL.B.l);
      }
      break;
    case 0x066:
      {
        BIT_HL(4);
      }
      break;
    case 0x067:
      {
        BIT_REG(4, R.AF.B.h);
      }
      break;
    case 0x068:
      {
        BIT_REG(5, R.BC.B.h);
      }
      break;
    case 0x069:
      {
        BIT_REG(5, R.BC.B.l);
      }
      break;
    case 0x06a:
      {
        BIT_REG(5, R.DE.B.h);
      }
      break;
    case 0x06b:
      {
        BIT_REG(5, R.DE.B.l);
      }
      break;
    case 0x06c:
      {
        BIT_REG(5, R.HL.B.h);
      }
      break;
    case 0x06d:
      {
        BIT_REG(5, R.HL.B.l);
      }
      break;
    case 0x06e:
      {
        BIT_HL(5);
      }
      break;
    case 0x06f:
      {
        BIT_REG(5, R.AF.B.h);
      }
      break;
    case 0x070:
      {
        BIT_REG(6, R.BC.B.h);
      }
      break;
    case 0x071:
      {
        BIT_REG(6, R.BC.B.l);
      }
      break;
    case 0x072:
      {
        BIT_REG(6, R.DE.B.h);
      }
      break;
    case 0x073:
      {
        BIT_REG(6, R.DE.B.l);
      }
      break;
    case 0x074:
      {
        BIT_REG(6, R.HL.B.h);
      }
      break;
    case 0x075:
      {
        BIT_REG(6, R.HL.B.l);
      }
      break;
    case 0x076:
      {
        BIT_HL(6);
      }
      break;
    case 0x077:
      {
        BIT_REG(6, R.AF.B.h);
      }
      break;
    case 0x078:
      {
        BIT_REG(7, R.BC.B.h);
      }
      break;
    case 0x079:
      {
        BIT_REG(7, R.BC.B.l);
      }
      break;
    case 0x07a:
      {
        BIT_REG(7, R.DE.B.h);
      }
      break;
    case 0x07b:
      {
        BIT_REG(7, R.DE.B.l);
      }
      break;
    case 0x07c:
      {
        BIT_REG(7, R.HL.B.h);
      }
      break;
    case 0x07d:
      {
        BIT_REG(7, R.HL.B.l);
      }
      break;
    case 0x07e:
      {
        BIT_HL(7);
      }
      break;
    case 0x07f:
      {
        BIT_REG(7, R.AF.B.h);
      }
      break;
    case 0x080:
      {
        RES_REG(0x01, R.BC.B.h);
      }
      break;
    case 0x081:
      {
        RES_REG(0x01, R.BC.B.l);
      }
      break;
    case 0x082:
      {
        RES_REG(0x01, R.DE.B.h);
      }
      break;
    case 0x083:
      {
        RES_REG(0x01, R.DE.B.l);
      }
      break;
    case 0x084:
      {
        RES_REG(0x01, R.HL.B.h);
      }
      break;
    case 0x085:
      {
        RES_REG(0x01, R.HL.B.l);
      }
      break;
    case 0x086:
      {
        RES_HL(0x01);
      }
      break;
    case 0x087:
      {
        RES_REG(0x01, R.AF.B.h);
      }
      break;
    case 0x088:
      {
        RES_REG(0x02, R.BC.B.h);
      }
      break;
    case 0x089:
      {
        RES_REG(0x02, R.BC.B.l);
      }
      break;
    case 0x08a:
      {
        RES_REG(0x02, R.DE.B.h);
      }
      break;
    case 0x08b:
      {
        RES_REG(0x02, R.DE.B.l);
      }
      break;
    case 0x08c:
      {
        RES_REG(0x02, R.HL.B.h);
      }
      break;
    case 0x08d:
      {
        RES_REG(0x02, R.HL.B.l);
      }
      break;
    case 0x08e:
      {
        RES_HL(0x02);
      }
      break;
    case 0x08f:
      {
        RES_REG(0x02, R.AF.B.h);
      }
      break;
    case 0x090:
      {
        RES_REG(0x04, R.BC.B.h);
      }
      break;
    case 0x091:
      {
        RES_REG(0x04, R.BC.B.l);
      }
      break;
    case 0x092:
      {
        RES_REG(0x04, R.DE.B.h);
      }
      break;
    case 0x093:
      {
        RES_REG(0x04, R.DE.B.l);
      }
      break;
    case 0x094:
      {
        RES_REG(0x04, R.HL.B.h);
      }
      break;
    case 0x095:
      {
        RES_REG(0x04, R.HL.B.l);
      }
      break;
    case 0x096:
      {
        RES_HL(0x04);
      }
      break;
    case 0x097:
      {
        RES_REG(0x04, R.AF.B.h);
      }
      break;
    case 0x098:
      {
        RES_REG(0x08, R.BC.B.h);
      }
      break;
    case 0x099:
      {
        RES_REG(0x08, R.BC.B.l);
      }
      break;
    case 0x09a:
      {
        RES_REG(0x08, R.DE.B.h);
      }
      break;
    case 0x09b:
      {
        RES_REG(0x08, R.DE.B.l);
      }
      break;
    case 0x09c:
      {
        RES_REG(0x08, R.HL.B.h);
      }
      break;
    case 0x09d:
      {
        RES_REG(0x08, R.HL.B.l);
      }
      break;
    case 0x09e:
      {
        RES_HL(0x08);
      }
      break;
    case 0x09f:
      {
        RES_REG(0x08, R.AF.B.h);
      }
      break;
    case 0x0a0:
      {
        RES_REG(0x10, R.BC.B.h);
      }
      break;
    case 0x0a1:
      {
        RES_REG(0x10, R.BC.B.l);
      }
      break;
    case 0x0a2:
      {
        RES_REG(0x10, R.DE.B.h);
      }
      break;
    case 0x0a3:
      {
        RES_REG(0x10, R.DE.B.l);
      }
      break;
    case 0x0a4:
      {
        RES_REG(0x10, R.HL.B.h);
      }
      break;
    case 0x0a5:
      {
        RES_REG(0x10, R.HL.B.l);
      }
      break;
    case 0x0a6:
      {
        RES_HL(0x10);
      }
      break;
    case 0x0a7:
      {
        RES_REG(0x10, R.AF.B.h);
      }
      break;
    case 0x0a8:
      {
        RES_REG(0x20, R.BC.B.h);
      }
      break;
    case 0x0a9:
      {
        RES_REG(0x20, R.BC.B.l);
      }
      break;
    case 0x0aa:
      {
        RES_REG(0x20, R.DE.B.h);
      }
      break;
    case 0x0ab:
      {
        RES_REG(0x20, R.DE.B.l);
      }
      break;
    case 0x0ac:
      {
        RES_REG(0x20, R.HL.B.h);
      }
      break;
    case 0x0ad:
      {
        RES_REG(0x20, R.HL.B.l);
      }
      break;
    case 0x0ae:
      {
        RES_HL(0x20);
      }
      break;
    case 0x0af:
      {
        RES_REG(0x20, R.AF.B.h);
      }
      break;
    case 0x0b0:
      {
        RES_REG(0x40, R.BC.B.h);
      }
      break;
    case 0x0b1:
      {
        RES_REG(0x40, R.BC.B.l);
      }
      break;
    case 0x0b2:
      {
        RES_REG(0x40, R.DE.B.h);
      }
      break;
    case 0x0b3:
      {
        RES_REG(0x40, R.DE.B.l);
      }
      break;
    case 0x0b4:
      {
        RES_REG(0x40, R.HL.B.h);
      }
      break;
    case 0x0b5:
      {
        RES_REG(0x40, R.HL.B.l);
      }
      break;
    case 0x0b6:
      {
        RES_HL(0x40);
      }
      break;
    case 0x0b7:
      {
        RES_REG(0x40, R.AF.B.h);
      }
      break;
    case 0x0b8:
      {
        RES_REG(0x80, R.BC.B.h);
      }
      break;
    case 0x0b9:
      {
        RES_REG(0x80, R.BC.B.l);
      }
      break;
    case 0x0ba:
      {
        RES_REG(0x80, R.DE.B.h);
      }
      break;
    case 0x0bb:
      {
        RES_REG(0x80, R.DE.B.l);
      }
      break;
    case 0x0bc:
      {
        RES_REG(0x80, R.HL.B.h);
      }
      break;
    case 0x0bd:
      {
        RES_REG(0x80, R.HL.B.l);
      }
      break;
    case 0x0be:
      {
        RES_HL(0x80);
      }
      break;
    case 0x0bf:
      {
        RES_REG(0x80, R.AF.B.h);
      }
      break;
    case 0x0c0:
      {
        SET_REG(0x01, R.BC.B.h);
      }
      break;
    case 0x0c1:
      {
        SET_REG(0x01, R.BC.B.l);
      }
      break;
    case 0x0c2:
      {
        SET_REG(0x01, R.DE.B.h);
      }
      break;
    case 0x0c3:
      {
        SET_REG(0x01, R.DE.B.l);
      }
      break;
    case 0x0c4:
      {
        SET_REG(0x01, R.HL.B.h);
      }
      break;
    case 0x0c5:
      {
        SET_REG(0x01, R.HL.B.l);
      }
      break;
    case 0x0c6:
      {
        SET_HL(0x01);
      }
      break;
    case 0x0c7:
      {
        SET_REG(0x01, R.AF.B.h);
      }
      break;
    case 0x0c8:
      {
        SET_REG(0x02, R.BC.B.h);
      }
      break;
    case 0x0c9:
      {
        SET_REG(0x02, R.BC.B.l);
      }
      break;
    case 0x0ca:
      {
        SET_REG(0x02, R.DE.B.h);
      }
      break;
    case 0x0cb:
      {
        SET_REG(0x02, R.DE.B.l);
      }
      break;
    case 0x0cc:
      {
        SET_REG(0x02, R.HL.B.h);
      }
      break;
    case 0x0cd:
      {
        SET_REG(0x02, R.HL.B.l);
      }
      break;
    case 0x0ce:
      {
        SET_HL(0x02);
      }
      break;
    case 0x0cf:
      {
        SET_REG(0x02, R.AF.B.h);
      }
      break;
    case 0x0d0:
      {
        SET_REG(0x04, R.BC.B.h);
      }
      break;
    case 0x0d1:
      {
        SET_REG(0x04, R.BC.B.l);
      }
      break;
    case 0x0d2:
      {
        SET_REG(0x04, R.DE.B.h);
      }
      break;
    case 0x0d3:
      {
        SET_REG(0x04, R.DE.B.l);
      }
      break;
    case 0x0d4:
      {
        SET_REG(0x04, R.HL.B.h);
      }
      break;
    case 0x0d5:
      {
        SET_REG(0x04, R.HL.B.l);
      }
      break;
    case 0x0d6:
      {
        SET_HL(0x04);
      }
      break;
    case 0x0d7:
      {
        SET_REG(0x04, R.AF.B.h);
      }
      break;
    case 0x0d8:
      {
        SET_REG(0x08, R.BC.B.h);
      }
      break;
    case 0x0d9:
      {
        SET_REG(0x08, R.BC.B.l);
      }
      break;
    case 0x0da:
      {
        SET_REG(0x08, R.DE.B.h);
      }
      break;
    case 0x0db:
      {
        SET_REG(0x08, R.DE.B.l);
      }
      break;
    case 0x0dc:
      {
        SET_REG(0x08, R.HL.B.h);
      }
      break;
    case 0x0dd:
      {
        SET_REG(0x08, R.HL.B.l);
      }
      break;
    case 0x0de:
      {
        SET_HL(0x08);
      }
      break;
    case 0x0df:
      {
        SET_REG(0x08, R.AF.B.h);
      }
      break;
    case 0x0e0:
      {
        SET_REG(0x10, R.BC.B.h);
      }
      break;
    case 0x0e1:
      {
        SET_REG(0x10, R.BC.B.l);
      }
      break;
    case 0x0e2:
      {
        SET_REG(0x10, R.DE.B.h);
      }
      break;
    case 0x0e3:
      {
        SET_REG(0x10, R.DE.B.l);
      }
      break;
    case 0x0e4:
      {
        SET_REG(0x10, R.HL.B.h);
      }
      break;
    case 0x0e5:
      {
        SET_REG(0x10, R.HL.B.l);
      }
      break;
    case 0x0e6:
      {
        SET_HL(0x10);
      }
      break;
    case 0x0e7:
      {
        SET_REG(0x10, R.AF.B.h);
      }
      break;
    case 0x0e8:
      {
        SET_REG(0x20, R.BC.B.h);
      }
      break;
    case 0x0e9:
      {
        SET_REG(0x20, R.BC.B.l);
      }
      break;
    case 0x0ea:
      {
        SET_REG(0x20, R.DE.B.h);
      }
      break;
    case 0x0eb:
      {
        SET_REG(0x20, R.DE.B.l);
      }
      break;
    case 0x0ec:
      {
        SET_REG(0x20, R.HL.B.h);
      }
      break;
    case 0x0ed:
      {
        SET_REG(0x20, R.HL.B.l);
      }
      break;
    case 0x0ee:
      {
        SET_HL(0x20);
      }
      break;
    case 0x0ef:
      {
        SET_REG(0x20, R.AF.B.h);
      }
      break;
    case 0x0f0:
      {
        SET_REG(0x40, R.BC.B.h);
      }
      break;
    case 0x0f1:
      {
        SET_REG(0x40, R.BC.B.l);
      }
      break;
    case 0x0f2:
      {
        SET_REG(0x40, R.DE.B.h);
      }
      break;
    case 0x0f3:
      {
        SET_REG(0x40, R.DE.B.l);
      }
      break;
    case 0x0f4:
      {
        SET_REG(0x40, R.HL.B.h);
      }
      break;
    case 0x0f5:
      {
        SET_REG(0x40, R.HL.B.l);
      }
      break;
    case 0x0f6:
      {
        SET_HL(0x40);
      }
      break;
    case 0x0f7:
      {
        SET_REG(0x40, R.AF.B.h);
      }
      break;
    case 0x0f8:
      {
        SET_REG(0x80, R.BC.B.h);
      }
      break;
    case 0x0f9:
      {
        SET_REG(0x80, R.BC.B.l);
      }
      break;
    case 0x0fa:
      {
        SET_REG(0x80, R.DE.B.h);
      }
      break;
    case 0x0fb:
      {
        SET_REG(0x80, R.DE.B.l);
      }
      break;
    case 0x0fc:
      {
        SET_REG(0x80, R.HL.B.h);
      }
      break;
    case 0x0fd:
      {
        SET_REG(0x80, R.HL.B.l);
      }
      break;
    case 0x0fe:
      {
        SET_HL(0x80);
      }
      break;
    case 0x0ff:
      {
        SET_REG(0x80, R.AF.B.h);
      }
      break;
    default:
      /* the following tells MSDEV 6 to not generate */
      /* code which checks if a input value to the */
      /* switch is not valid. */
#ifdef _MSC_VER
#  if (_MSC_VER >= 1200)
      __assume(0);
#  endif
#endif
      break;
    }
    INC_REFRESH(2);
    ADD_PC(2);
    R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
  }

  /***************************************************************************/
  void Z80::executeInstruction()
  {
    uint8_t Opcode;
    Opcode = readOpcodeFirstByte();
    switch (Opcode) {
    case 0x000:
      {
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x001:
      {
        LD_RR_nn(R.BC.W);
        INC_REFRESH(1);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x002:
      {
        LD_RR_A(R.BC.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x003:
      {
        INC_rp(R.BC.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x004:
      {
        INC_R(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x005:
      {
        DEC_R(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x006:
      {
        LD_R_n(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x007:
      {
        RLCA();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x008:
      {
        SWAP(R.AF.W, R.altAF.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x009:
      {
        ADD_RR_rr(R.HL.W, R.BC.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x00a:
      {
        LD_A_RR(R.BC.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x00b:
      {
        DEC_rp(R.BC.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x00c:
      {
        INC_R(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x00d:
      {
        DEC_R(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x00e:
      {
        LD_R_n(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x00f:
      {
        RRCA();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x010:
      {
        DJNZ_dd();
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x011:
      {
        LD_RR_nn(R.DE.W);
        INC_REFRESH(1);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x012:
      {
        LD_RR_A(R.DE.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x013:
      {
        INC_rp(R.DE.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x014:
      {
        INC_R(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x015:
      {
        DEC_R(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x016:
      {
        LD_R_n(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x017:
      {
        RLA();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x018:
      {
        JR();
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x019:
      {
        ADD_RR_rr(R.HL.W, R.DE.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x01a:
      {
        LD_A_RR(R.DE.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x01b:
      {
        DEC_rp(R.DE.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x01c:
      {
        INC_R(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x01d:
      {
        DEC_R(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x01e:
      {
        LD_R_n(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x01f:
      {
        RRA();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x020:
      {
        if (Z80_TEST_ZERO_NOT_SET) {
          JR();
        }
        else {
          (void) readOpcodeByte(1);
          ADD_PC(2);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x021:
      {
        LD_RR_nn(R.HL.W);
        INC_REFRESH(1);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x022:
      {
        LD_nnnn_HL();
        INC_REFRESH(1);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x023:
      {
        INC_rp(R.HL.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x024:
      {
        INC_R(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x025:
      {
        DEC_R(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x026:
      {
        LD_R_n(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x027:
      {
        DAA();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x028:
      {
        if (Z80_TEST_ZERO_SET) {
          JR();
        }
        else {
          (void) readOpcodeByte(1);
          ADD_PC(2);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x029:
      {
        ADD_RR_rr(R.HL.W, R.HL.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02a:
      {
        LD_HL_nnnn();
        INC_REFRESH(1);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02b:
      {
        DEC_rp(R.HL.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02c:
      {
        INC_R(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02d:
      {
        DEC_R(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02e:
      {
        LD_R_n(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x02f:
      {
        CPL();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x030:
      {
        if (Z80_TEST_CARRY_NOT_SET) {
          JR();
        }
        else {
          (void) readOpcodeByte(1);
          ADD_PC(2);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x031:
      {
        LD_RR_nn(R.SP.W);
        INC_REFRESH(1);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x032:
      {
        LD_nnnn_A();
        INC_REFRESH(1);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x033:
      {
        INC_rp(R.SP.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x034:
      {
        INC_HL_();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x035:
      {
        DEC_HL_();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x036:
      {
        LD_HL_n();
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x037:
      {
        SCF();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x038:
      {
        if (Z80_TEST_CARRY_SET) {
          JR();
        }
        else {
          (void) readOpcodeByte(1);
          ADD_PC(2);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x039:
      {
        ADD_RR_rr(R.HL.W, R.SP.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x03a:
      {
        LD_A_nnnn();
        INC_REFRESH(1);
        ADD_PC(3);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x03b:
      {
        DEC_rp(R.SP.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x03c:
      {
        INC_R(R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x03d:
      {
        DEC_R(R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x03e:
      {
        LD_R_n(R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x03f:
      {
        CCF();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x040:
      {
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x041:
      {
        LD_R_R(R.BC.B.h, R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x042:
      {
        LD_R_R(R.BC.B.h, R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x043:
      {
        LD_R_R(R.BC.B.h, R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x044:
      {
        LD_R_R(R.BC.B.h, R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x045:
      {
        LD_R_R(R.BC.B.h, R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x046:
      {
        LD_R_HL(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x047:
      {
        LD_R_R(R.BC.B.h, R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x048:
      {
        LD_R_R(R.BC.B.l, R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x049:
      {
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04a:
      {
        LD_R_R(R.BC.B.l, R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04b:
      {
        LD_R_R(R.BC.B.l, R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04c:
      {
        LD_R_R(R.BC.B.l, R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04d:
      {
        LD_R_R(R.BC.B.l, R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04e:
      {
        LD_R_HL(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x04f:
      {
        LD_R_R(R.BC.B.l, R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x050:
      {
        LD_R_R(R.DE.B.h, R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x051:
      {
        LD_R_R(R.DE.B.h, R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x052:
      {
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x053:
      {
        LD_R_R(R.DE.B.h, R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x054:
      {
        LD_R_R(R.DE.B.h, R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x055:
      {
        LD_R_R(R.DE.B.h, R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x056:
      {
        LD_R_HL(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x057:
      {
        LD_R_R(R.DE.B.h, R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x058:
      {
        LD_R_R(R.DE.B.l, R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x059:
      {
        LD_R_R(R.DE.B.l, R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05a:
      {
        LD_R_R(R.DE.B.l, R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05b:
      {
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05c:
      {
        LD_R_R(R.DE.B.l, R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05d:
      {
        LD_R_R(R.DE.B.l, R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05e:
      {
        LD_R_HL(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x05f:
      {
        LD_R_R(R.DE.B.l, R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x060:
      {
        LD_R_R(R.HL.B.h, R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x061:
      {
        LD_R_R(R.HL.B.h, R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x062:
      {
        LD_R_R(R.HL.B.h, R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x063:
      {
        LD_R_R(R.HL.B.h, R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x064:
      {
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x065:
      {
        LD_R_R(R.HL.B.h, R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x066:
      {
        LD_R_HL(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x067:
      {
        LD_R_R(R.HL.B.h, R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x068:
      {
        LD_R_R(R.HL.B.l, R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x069:
      {
        LD_R_R(R.HL.B.l, R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06a:
      {
        LD_R_R(R.HL.B.l, R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06b:
      {
        LD_R_R(R.HL.B.l, R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06c:
      {
        LD_R_R(R.HL.B.l, R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06d:
      {
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06e:
      {
        LD_R_HL(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x06f:
      {
        LD_R_R(R.HL.B.l, R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x070:
      {
        LD_HL_R(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x071:
      {
        LD_HL_R(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x072:
      {
        LD_HL_R(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x073:
      {
        LD_HL_R(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x074:
      {
        LD_HL_R(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x075:
      {
        LD_HL_R(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x076:
      {
        HALT();
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x077:
      {
        LD_HL_R(R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x078:
      {
        LD_R_R(R.AF.B.h, R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x079:
      {
        LD_R_R(R.AF.B.h, R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07a:
      {
        LD_R_R(R.AF.B.h, R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07b:
      {
        LD_R_R(R.AF.B.h, R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07c:
      {
        LD_R_R(R.AF.B.h, R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07d:
      {
        LD_R_R(R.AF.B.h, R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07e:
      {
        LD_R_HL(R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x07f:
      {
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x080:
      {
        ADD_A_R(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x081:
      {
        ADD_A_R(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x082:
      {
        ADD_A_R(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x083:
      {
        ADD_A_R(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x084:
      {
        ADD_A_R(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x085:
      {
        ADD_A_R(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x086:
      {
        ADD_A_HL();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x087:
      {
        ADD_A_R(R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x088:
      {
        ADC_A_R(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x089:
      {
        ADC_A_R(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08a:
      {
        ADC_A_R(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08b:
      {
        ADC_A_R(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08c:
      {
        ADC_A_R(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08d:
      {
        ADC_A_R(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08e:
      {
        ADC_A_HL();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x08f:
      {
        ADC_A_R(R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x090:
      {
        SUB_A_R(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x091:
      {
        SUB_A_R(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x092:
      {
        SUB_A_R(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x093:
      {
        SUB_A_R(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x094:
      {
        SUB_A_R(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x095:
      {
        SUB_A_R(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x096:
      {
        SUB_A_HL();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x097:
      {
        Z80_BYTE Flags;
        R.AF.B.h = 0;
        Flags = Z80_ZERO_FLAG | Z80_SUBTRACT_FLAG;
        Z80_FLAGS_REG = Flags;
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x098:
      {
        SBC_A_R(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x099:
      {
        SBC_A_R(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09a:
      {
        SBC_A_R(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09b:
      {
        SBC_A_R(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09c:
      {
        SBC_A_R(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09d:
      {
        SBC_A_R(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09e:
      {
        SBC_A_HL();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x09f:
      {
        SBC_A_R(R.AF.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a0:
      {
        AND_A_R(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a1:
      {
        AND_A_R(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a2:
      {
        AND_A_R(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a3:
      {
        AND_A_R(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a4:
      {
        AND_A_R(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a5:
      {
        AND_A_R(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a6:
      {
        AND_A_HL();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a7:
      {
        Z80_BYTE Flags;
        Flags = R.AF.B.h & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2);
        Flags |= Z80_HALFCARRY_FLAG;
        Flags |= t.zeroSignParityTable[R.AF.B.h];
        Z80_FLAGS_REG = Flags;
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a8:
      {
        XOR_A_R(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0a9:
      {
        XOR_A_R(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0aa:
      {
        XOR_A_R(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ab:
      {
        XOR_A_R(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ac:
      {
        XOR_A_R(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ad:
      {
        XOR_A_R(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ae:
      {
        XOR_A_HL();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0af:
      {
        Z80_BYTE Flags;
        R.AF.B.h = 0;
        Flags = Z80_ZERO_FLAG | Z80_PARITY_FLAG;
        Z80_FLAGS_REG = Flags;
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b0:
      {
        OR_A_R(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b1:
      {
        OR_A_R(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b2:
      {
        OR_A_R(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b3:
      {
        OR_A_R(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b4:
      {
        OR_A_R(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b5:
      {
        OR_A_R(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b6:
      {
        OR_A_HL();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b7:
      {
        Z80_BYTE Flags;
        Flags = R.AF.B.h & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2);
        Flags |= t.zeroSignParityTable[R.AF.B.h];
        Z80_FLAGS_REG = Flags;
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b8:
      {
        CP_A_R(R.BC.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0b9:
      {
        CP_A_R(R.BC.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ba:
      {
        CP_A_R(R.DE.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0bb:
      {
        CP_A_R(R.DE.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0bc:
      {
        CP_A_R(R.HL.B.h);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0bd:
      {
        CP_A_R(R.HL.B.l);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0be:
      {
        CP_A_HL();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0bf:
      {
        Z80_BYTE Flags;
        Flags = R.AF.B.h & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2);
        Flags |= Z80_ZERO_FLAG | Z80_SUBTRACT_FLAG;
        Z80_FLAGS_REG = Flags;
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0c0:
      {
        updateCycle();
        if (Z80_TEST_ZERO_NOT_SET) {
          RETURN();
        }
        else {
          ADD_PC(1);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0c1:
      {
        R.BC.W = POP();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0c2:
      {
        if (Z80_TEST_ZERO_NOT_SET) {
          JP();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0c3:
      {
        JP();
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0c4:
      {
        if (Z80_TEST_ZERO_NOT_SET) {
          CALL();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0c5:
      {
        PUSH(R.BC.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0c6:
      {
        ADD_A_n();
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0c7:
      {
        RST(0x00000);
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0c8:
      {
        updateCycle();
        if (Z80_TEST_ZERO_SET) {
          RETURN();
        }
        else {
          ADD_PC(1);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0c9:
      {
        RETURN();
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ca:
      {
        if (Z80_TEST_ZERO_SET) {
          JP();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0cb:
      {
        CB_ExecuteInstruction();
      }
      break;
    case 0x0cc:
      {
        if (Z80_TEST_ZERO_SET) {
          CALL();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0cd:
      {
        CALL();
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ce:
      {
        ADC_A_n();
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0cf:
      {
        RST(0x00008);
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0d0:
      {
        updateCycle();
        if (Z80_TEST_CARRY_NOT_SET) {
          RETURN();
        }
        else {
          ADD_PC(1);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0d1:
      {
        R.DE.W = POP();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0d2:
      {
        if (Z80_TEST_CARRY_NOT_SET) {
          JP();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0d3:
      {
        OUT_n_A();
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0d4:
      {
        if (Z80_TEST_CARRY_NOT_SET) {
          CALL();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0d5:
      {
        PUSH(R.DE.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0d6:
      {
        SUB_A_n();
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0d7:
      {
        RST(0x00010);
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0d8:
      {
        updateCycle();
        if (Z80_TEST_CARRY_SET) {
          RETURN();
        }
        else {
          ADD_PC(1);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0d9:
      {
        SWAP(R.DE.W, R.altDE.W);
        SWAP(R.HL.W, R.altHL.W);
        SWAP(R.BC.W, R.altBC.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0da:
      {
        if (Z80_TEST_CARRY_SET) {
          JP();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0db:
      {
        IN_A_n();
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0dc:
      {
        if (Z80_TEST_CARRY_SET) {
          CALL();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0dd:
      {
        DD_ExecuteInstruction();
      }
      break;
    case 0x0de:
      {
        SBC_A_n();
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0df:
      {
        RST(0x00018);
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e0:
      {
        updateCycle();
        if (Z80_TEST_PARITY_ODD) {
          RETURN();
        }
        else {
          ADD_PC(1);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e1:
      {
        R.HL.W = POP();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e2:
      {
        if (Z80_TEST_PARITY_ODD) {
          JP();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e3:
      {
        EX_SP_rr(R.HL.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e4:
      {
        if (Z80_TEST_PARITY_ODD) {
          CALL();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e5:
      {
        PUSH(R.HL.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e6:
      {
        AND_A_n();
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e7:
      {
        RST(0x00020);
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e8:
      {
        updateCycle();
        if (Z80_TEST_PARITY_EVEN) {
          RETURN();
        }
        else {
          ADD_PC(1);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0e9:
      {
        JP_rp(R.HL.W);
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ea:
      {
        if (Z80_TEST_PARITY_EVEN) {
          JP();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0eb:
      {
        SWAP(R.HL.W, R.DE.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ec:
      {
        if (Z80_TEST_PARITY_EVEN) {
          CALL();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ed:
      {
        ED_ExecuteInstruction();
      }
      break;
    case 0x0ee:
      {
        XOR_A_n();
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ef:
      {
        RST(0x00028);
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0f0:
      {
        updateCycle();
        if (Z80_TEST_POSITIVE) {
          RETURN();
        }
        else {
          ADD_PC(1);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0f1:
      {
        R.AF.W = POP();
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0f2:
      {
        if (Z80_TEST_POSITIVE) {
          JP();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0f3:
      {
        DI();
        INC_REFRESH(1);
        ADD_PC(1);
      }
      break;
    case 0x0f4:
      {
        if (Z80_TEST_POSITIVE) {
          CALL();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0f5:
      {
        PUSH(R.AF.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0f6:
      {
        OR_A_n();
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0f7:
      {
        RST(0x00030);
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0f8:
      {
        updateCycle();
        if (Z80_TEST_MINUS) {
          RETURN();
        }
        else {
          ADD_PC(1);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0f9:
      {
        LD_SP_rp(R.HL.W);
        INC_REFRESH(1);
        ADD_PC(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0fa:
      {
        if (Z80_TEST_MINUS) {
          JP();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0fb:
      {
        EI();
        INC_REFRESH(1);
        ADD_PC(1);
      }
      break;
    case 0x0fc:
      {
        if (Z80_TEST_MINUS) {
          CALL();
        }
        else {
          (void) readOpcodeWord(1);
          ADD_PC(3);
        }
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0fd:
      {
        FD_ExecuteInstruction();
      }
      break;
    case 0x0fe:
      {
        CP_A_n();
        INC_REFRESH(1);
        ADD_PC(2);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    case 0x0ff:
      {
        RST(0x00038);
        INC_REFRESH(1);
        R.Flags |= Z80_CHECK_INTERRUPT_FLAG;
      }
      break;
    default:
      /* the following tells MSDEV 6 to not generate */
      /* code which checks if a input value to the */
      /* switch is not valid. */
#ifdef _MSC_VER
#  if (_MSC_VER >= 1200)
      __assume(0);
#  endif
#endif
      break;
    }
    /* check interrupts? */
    if (EP128EMU_UNLIKELY(R.Flags & (Z80_EXECUTE_INTERRUPT_HANDLER_FLAG
                                     | Z80_NMI_FLAG | Z80_SET_PC_FLAG))) {
      if (EP128EMU_EXPECT(!(R.Flags & (Z80_NMI_FLAG | Z80_SET_PC_FLAG)))) {
        if (R.IFF1) {
          if (R.Flags & Z80_CHECK_INTERRUPT_FLAG)
            executeInterrupt();
        }
      }
      else {
        this->NMI();
      }
    }
  }

}       // namespace Ep128

