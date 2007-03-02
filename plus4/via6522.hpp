
// plus4 -- portable Commodore Plus/4 emulator
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

#ifndef EP128EMU_VIA6522_HPP
#define EP128EMU_VIA6522_HPP

#include "ep128emu.hpp"

namespace Plus4 {

  class VIA6522 {
   private:
    uint8_t   viaRegisters[16];
    uint8_t   portADataDirection;
    uint8_t   portARegister;
    uint8_t   portAInput;
    uint8_t   portALatch;
    uint8_t   portBDataDirection;
    uint8_t   portBRegister;
    uint8_t   portBInput;
    uint8_t   portBLatch;
    uint8_t   portBTimerOutputMask;
    uint8_t   portBTimerOutput;
    uint16_t  timer1Counter;
    uint16_t  timer1Latch;
    bool      timer1SingleShotMode;
    bool      timer1SingleShotModeDone;
    uint16_t  timer2Counter;
    uint8_t   timer2LatchL;
    bool      timer2PulseCountingMode;
    bool      timer2SingleShotModeDone;
    bool      ca1Input;
    bool      ca1Output;
    bool      ca1IsOutput;
    bool      ca1PositiveEdge;
    bool      ca2Input;
    bool      ca2Output;
    bool      ca2IsOutput;
    bool      cb1Input;
    bool      cb1Output;
    bool      cb1IsOutput;
    bool      cb1PositiveEdge;
    bool      cb2Input;
    bool      cb2Output;
    bool      cb2IsOutput;
    uint8_t   shiftRegister;
    uint8_t   shiftCounter;
    bool      prvIRQState;
    inline void updateInterruptFlags()
    {
      viaRegisters[0x0D] = viaRegisters[0x0D] & 0x7F;
      if (viaRegisters[0x0D] & viaRegisters[0x0E])
        viaRegisters[0x0D] = viaRegisters[0x0D] | 0x80;
    }
    inline void updateTimer1()
    {
      timer1Counter = (timer1Counter - 1) & 0xFFFF;
      if (!timer1Counter) {
        if (!timer1SingleShotMode || !timer1SingleShotModeDone) {
          if (timer1SingleShotMode)
            portBTimerOutput = 0x80;
          else {
            timer1Counter = timer1Latch;
            portBTimerOutput = portBTimerOutput ^ 0x80;
          }
          viaRegisters[0x0D] = viaRegisters[0x0D] | 0x40;
          updateInterruptFlags();
        }
        timer1SingleShotModeDone = true;
      }
    }
    inline void updateTimer2()
    {
      timer2Counter = (timer2Counter - 1) & 0xFFFF;
      if (!timer2Counter) {
        if (!timer2SingleShotModeDone) {
          timer2SingleShotModeDone = true;
          viaRegisters[0x0D] = viaRegisters[0x0D] | 0x20;
          updateInterruptFlags();
        }
      }
    }
   public:
    VIA6522();
    virtual ~VIA6522();
    void reset();
    void run(size_t nCycles = 1);
    uint8_t readRegister(uint16_t addr);
    void writeRegister(uint16_t addr, uint8_t value);
    inline uint8_t getPortA() const
    {
      return uint8_t(portAInput
                     & (portARegister | (portADataDirection ^ 0xFF)));
    }
    inline void setPortA(uint8_t value)
    {
      portAInput = value & 0xFF;
    }
    inline uint8_t getPortB() const
    {
      return uint8_t(portBInput
                     & ((portBRegister | (portBDataDirection ^ 0xFF))
                        & (portBTimerOutput | (portBTimerOutputMask ^ 0xFF))));
    }
    inline void setPortB(uint8_t value)
    {
      if (!timer2PulseCountingMode) {
        portBInput = value & 0xFF;
      }
      else {
        uint8_t prvState = getPortB();
        portBInput = value & 0xFF;
        uint8_t newState = getPortB();
        if ((newState & 0x40) < (prvState & 0x40))
          updateTimer2();
      }
    }
    inline bool getCA1() const
    {
      return (ca1Input && (ca1Output || !ca1IsOutput));
    }
    inline void setCA1(bool value)
    {
      bool    prvState = getCA1();
      ca1Input = value;
      bool    newState = getCA1();
      if (newState != prvState) {
        if (newState == ca1PositiveEdge) {
          viaRegisters[0x0D] = viaRegisters[0x0D] | 0x02;
          updateInterruptFlags();
          portALatch = getPortA();
        }
      }
    }
    inline bool getCA2() const
    {
      return (ca2Input && (ca2Output || !ca2IsOutput));
    }
    inline void setCA2(bool value)
    {
      bool    prvState = getCA2();
      ca2Input = value;
      bool    newState = getCA2();
      if (newState != prvState) {
        if (!ca2IsOutput) {
          if (newState == !!(viaRegisters[0x0C] & 0x40)) {
            viaRegisters[0x0D] = viaRegisters[0x0D] | 0x01;
            updateInterruptFlags();
          }
        }
      }
    }
    inline bool getCB1() const
    {
      return (cb1Input && (cb1Output || !cb1IsOutput));
    }
    inline void setCB1(bool value)
    {
      bool    prvState = getCB1();
      cb1Input = value;
      bool    newState = getCB1();
      if (newState != prvState) {
        if (newState == cb1PositiveEdge) {
          viaRegisters[0x0D] = viaRegisters[0x0D] | 0x10;
          updateInterruptFlags();
          portBLatch = getPortB();
        }
      }
    }
    inline bool getCB2() const
    {
      return (cb2Input && (cb2Output || !cb2IsOutput));
    }
    inline void setCB2(bool value)
    {
      bool    prvState = getCB2();
      cb2Input = value;
      bool    newState = getCB2();
      if (newState != prvState) {
        if (!cb2IsOutput) {
          if (newState == !!(viaRegisters[0x0C] & 0x04)) {
            viaRegisters[0x0D] = viaRegisters[0x0D] | 0x08;
            updateInterruptFlags();
          }
        }
      }
    }
   protected:
    // called when the state of the IRQ line changes
    // 'newState' is true if the IRQ line is low, and false if it is high
    virtual void irqStateChangeCallback(bool newState);
  };

}       // namespace Plus4

#endif  // EP128EMU_VIA6522_HPP

