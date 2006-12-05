
// plus4 -- portable Commodore PLUS/4 emulator
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

#ifndef PLUS4_CPU_HPP
#define PLUS4_CPU_HPP

#include "ep128.hpp"

namespace Plus4 {

  class M7501 {
   protected:
    uint16_t    reg_PC;
    uint8_t     reg_SR;
    uint8_t     reg_AC;
    uint8_t     reg_XR;
    uint8_t     reg_YR;
    uint8_t     reg_SP;
    // -----------------------------------------------------------------
    virtual uint8_t readMemory(uint16_t addr) const
    {
      (void) addr;
      return (uint8_t) 0;
    }
    virtual void writeMemory(uint16_t addr, uint8_t value)
    {
      (void) addr;
      (void) value;
    }
   private:
    static const unsigned char cpu_opcode_length[256];
    void interruptRequest_();
    void runOneOpcode_();
    int   cyclesRemaining;
    int   currentOpcode;
    int   currentOperand;
    inline void op_setnz(uint8_t value)
    {
      reg_SR = (reg_SR & (uint8_t) 0x7D) | (value & (uint8_t) 0x80);
      reg_SR |= (value ? (uint8_t) 0 : (uint8_t) 2);
    }
    inline uint16_t op_addr_nn_X()
    {
      return (uint16_t) ((currentOperand + (int) reg_XR) & (int) 0xFF);
    }
    inline uint16_t op_addr_nn_Y()
    {
      return (uint16_t) ((currentOperand + (int) reg_YR) & (int) 0xFF);
    }
    inline uint16_t op_addr_nnnn_X()
    {
      return (uint16_t) ((unsigned int) currentOperand + (unsigned int) reg_XR);
    }
    inline uint16_t op_addr_nnnn_X_wait()
    {
      unsigned int  addr;
      addr = (unsigned int) currentOperand + (unsigned int) reg_XR;
      if ((addr ^ (unsigned int) currentOperand) & 0xFF00U)
        cyclesRemaining++;
      return (uint16_t) addr;
    }
    inline uint16_t op_addr_nnnn_Y()
    {
      return (uint16_t) ((unsigned int) currentOperand + (unsigned int) reg_YR);
    }
    inline uint16_t op_addr_nnnn_Y_wait()
    {
      unsigned int  addr;
      addr = (unsigned int) currentOperand + (unsigned int) reg_YR;
      if ((addr ^ (unsigned int) currentOperand) & 0xFF00U)
        cyclesRemaining++;
      return (uint16_t) addr;
    }
    inline uint16_t op_addr_nn_X_indirect()
    {
      uint16_t  addr;
      uint8_t   ptr;
      ptr = (uint8_t) ((unsigned int) currentOperand + (unsigned int) reg_XR);
      addr = (uint16_t) readMemory((uint16_t) ptr);
      ptr++;
      addr += ((uint16_t) readMemory((uint16_t) ptr) << 8);
      return addr;
    }
    inline uint16_t op_addr_nn_Y_indirect()
    {
      uint16_t  addr;
      uint8_t   ptr;
      ptr = (uint8_t) ((unsigned int) currentOperand);
      addr = (uint16_t) readMemory((uint16_t) ptr);
      ptr++;
      addr += ((uint16_t) readMemory((uint16_t) ptr) << 8);
      addr += (uint16_t) reg_YR;
      return addr;
    }
    inline uint16_t op_addr_nn_Y_indirect_wait()
    {
      uint16_t  addr, addr_Y;
      uint8_t   ptr;
      ptr = (uint8_t) ((unsigned int) currentOperand);
      addr = (uint16_t) readMemory((uint16_t) ptr);
      ptr++;
      addr += ((uint16_t) readMemory((uint16_t) ptr) << 8);
      addr_Y = addr + (uint16_t) reg_YR;
      if ((addr ^ addr_Y) & (uint16_t) 0xFF00)
        cyclesRemaining++;
      return addr_Y;
    }
    inline void op_adc(uint8_t &value, uint8_t arg)
    {
      unsigned int  result = (unsigned int) (reg_SR & (uint8_t) 0x01);
      if (reg_SR & (uint8_t) 0x08) {
        // add in BCD mode
        result += (unsigned int) (value & (uint8_t) 0x0F);
        result += (unsigned int) (arg & (uint8_t) 0x0F);
        result += (result < 0x0AU ? 0x00U : 0x06U);
        result += (unsigned int) (value & (uint8_t) 0xF0);
        result += (unsigned int) (arg & (uint8_t) 0xF0);
        result += (result < 0xA0U ? 0x00U : 0x60U);
      }
      else {
        // add in binary mode
        result += ((unsigned int) value + (unsigned int) arg);
      }
      reg_SR &= (uint8_t) 0x3C;
      // carry
      reg_SR |= (result >= (unsigned int) 0x0100 ? (uint8_t) 1 : (uint8_t) 0);
      // overflow
      reg_SR |= ((((value ^ ~arg) & (value ^ (uint8_t) result))
                  & (uint8_t) 0x80) >> 1);
      value = (uint8_t) result;
      op_setnz(value);
    }
    inline void op_and(uint8_t &value, uint8_t arg)
    {
      value &= arg;
      op_setnz(value);
    }
    inline void op_asl(uint8_t &value)
    {
      reg_SR = (reg_SR & (uint8_t) 0xFE) | ((value & (uint8_t) 0x80) >> 7);
      value <<= 1;
      op_setnz(value);
    }
    inline void op_bit(uint8_t value, uint8_t arg)
    {
      reg_SR &= (uint8_t) 0x3D;
      reg_SR |= (arg & (uint8_t) 0xC0);
      reg_SR |= ((value & arg) ? (uint8_t) 0x00 : (uint8_t) 0x02);
    }
    inline void op_branch(bool cond)
    {
      if (cond) {
        uint16_t  old_PC = reg_PC;
        reg_PC += (uint16_t) currentOperand;
        reg_PC += (uint16_t) ((currentOperand & (int) 0x80) ? 0xFF00 : 0x0000);
        cyclesRemaining = (((old_PC ^ reg_PC) & (uint16_t) 0xFF00) ? 2 : 1);
      }
    }
    inline void op_cmp(uint8_t value, uint8_t arg)
    {
      unsigned int  tmp;
      tmp = (unsigned int) value + (0x0100U - (unsigned int) arg);
      reg_SR &= (uint8_t) 0x7C;
      reg_SR |= ((uint8_t) tmp & (uint8_t) 0x80);
      reg_SR |= (uint8_t) (tmp >> 8);
      reg_SR |= ((uint8_t) tmp == (uint8_t) 0 ? (uint8_t) 2 : (uint8_t) 0);
    }
    inline void op_dec(uint8_t &value)
    {
      value--;
      op_setnz(value);
    }
    inline void op_eor(uint8_t &value, uint8_t arg)
    {
      value ^= arg;
      op_setnz(value);
    }
    inline void op_inc(uint8_t &value)
    {
      value++;
      op_setnz(value);
    }
    inline void op_lsr(uint8_t &value)
    {
      reg_SR = (reg_SR & (uint8_t) 0xFE) | (value & (uint8_t) 0x01);
      value >>= 1;
      op_setnz(value);
    }
    inline void op_ora(uint8_t &value, uint8_t arg)
    {
      value |= arg;
      op_setnz(value);
    }
    inline uint8_t op_pop()
    {
      reg_SP++;
      return readMemory((uint16_t) 0x0100 + (uint16_t) reg_SP);
    }
    inline void op_push(uint8_t value)
    {
      writeMemory((uint16_t) 0x0100 + (uint16_t) reg_SP, value);
      reg_SP--;
    }
    inline void op_rol(uint8_t &value)
    {
      uint8_t carryBit = ((value & (uint8_t) 0x80) >> 7);
      value = (value << 1) | (reg_SR & (uint8_t) 0x01);
      reg_SR = (reg_SR & (uint8_t) 0xFE) | carryBit;
      op_setnz(value);
    }
    inline void op_ror(uint8_t &value)
    {
      uint8_t carryBit = (value & (uint8_t) 0x01);
      value = (value >> 1) | ((reg_SR & (uint8_t) 0x01) << 7);
      reg_SR = (reg_SR & (uint8_t) 0xFE) | carryBit;
      op_setnz(value);
    }
    inline void op_sbc(uint8_t &value, uint8_t arg)
    {
      unsigned int  result = (unsigned int) (reg_SR & (uint8_t) 0x01);
      arg ^= (uint8_t) 0xFF;
      if (reg_SR & (uint8_t) 0x08) {
        // subtract in BCD mode
        result += (unsigned int) (value & (uint8_t) 0x0F);
        result += (unsigned int) (arg & (uint8_t) 0x0F);
        if (result < 0x10U)
          result = (result + 0x0AU) & 0x0FU;
        result += (unsigned int) (value & (uint8_t) 0xF0);
        result += (unsigned int) (arg & (uint8_t) 0xF0);
        if (result < 0x0100U)
          result = (result + 0xA0U) & 0xFFU;
      }
      else {
        // subtract in binary mode
        result += ((unsigned int) value + (unsigned int) arg);
      }
      reg_SR &= (uint8_t) 0x3C;
      // carry
      reg_SR |= (result >= (unsigned int) 0x0100 ? (uint8_t) 1 : (uint8_t) 0);
      // overflow
      reg_SR |= ((((value ^ ~arg) & (value ^ (uint8_t) result))
                  & (uint8_t) 0x80) >> 1);
      value = (uint8_t) result;
      op_setnz(value);
    }
    // -----------------------------------------------------------------
   public:
    inline void runOneCycle()
    {
      if (!(--cyclesRemaining))
        runOneOpcode_();
    }
    void interruptRequest()
    {
      if (!(reg_SR & (uint8_t) 0x04))
        interruptRequest_();
    }
    virtual void reset(bool cold_reset);
    M7501();
    virtual ~M7501()
    {
    }
  };

}       // namespace Plus4

#endif  // PLUS4_CPU_HPP

