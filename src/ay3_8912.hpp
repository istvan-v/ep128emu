
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2010 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EP128EMU_AY3_8912_HPP
#define EP128EMU_AY3_8912_HPP

#include "ep128emu.hpp"

namespace ZX128 {

  class AY3_8912 {
   protected:
    static const uint8_t  registerMaskTable[16];
    static const uint16_t amplitudeTable[16];
    uint8_t   registers[16];
    int       tgFreqA;                  // tone generator A frequency code
    int       tgCntA;                   // tone generator A counter
    int       tgFreqB;                  // tone generator B frequency code
    int       tgCntB;                   // tone generator B counter
    int       tgFreqC;                  // tone generator C frequency code
    int       tgCntC;                   // tone generator C counter
    int       ngFreq;                   // noise generator frequency code
    int       ngCnt;                    // noise gen. counter | (clkState << 7)
    uint32_t  ngShiftReg;               // noise generator shift register
    bool      tgStateA;                 // tone generator A state
    bool      tgStateB;                 // tone generator B state
    bool      tgStateC;                 // tone generator C state
    bool      ngState;                  // noise generator output (bit 16)
    uint32_t  envFreq;                  // envelope generator frequency code
    uint32_t  envCnt;                   // envelope generator counter
    int       envState;                 // envelope generator state (0 to 31)
    int       envDir;                   // env. gen. direction (-1, 0, or 1)
    bool      envHold;                  // envelope generator HOLD bit
    bool      envAlternate;             // envelope generator ALTERNATE bit
    bool      envAttack;                // envelope generator ATTACK bit
    bool      envContinue;              // envelope generator CONTINUE bit
    bool      tgDisabledA;              // tone generator A output enable flag
    bool      tgDisabledB;              // tone generator B output enable flag
    bool      tgDisabledC;              // tone generator C output enable flag
    bool      ngDisabledA;              // noise generator output A enable flag
    bool      ngDisabledB;              // noise generator output B enable flag
    bool      ngDisabledC;              // noise generator output C enable flag
    uint16_t  amplitudeA;               // channel A current amplitude
    uint16_t  amplitudeB;               // channel B current amplitude
    uint16_t  amplitudeC;               // channel C current amplitude
    bool      envEnabledA;              // envelope to channel A enable flag
    bool      envEnabledB;              // envelope to channel B enable flag
    bool      envEnabledC;              // envelope to channel C enable flag
    uint8_t   portAInput;               // port A input byte (defaults to 0xFF)
    // --------
    void resetRegisters();
   public:
    AY3_8912();
    virtual ~AY3_8912();
    void reset();
    uint8_t readRegister(uint16_t addr) const;
    void writeRegister(uint16_t addr, uint8_t value);
    void runOneCycle(uint16_t& outA, uint16_t& outB, uint16_t& outC);
    inline void setPortAInput(uint8_t value)
    {
      portAInput = value;
    }
    inline uint8_t getPortAState() const
    {
      return ((this->registers[7] & 0x40) == 0 ?
              portAInput : (portAInput & this->registers[14]));
    }
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
  };

}       // namespace ZX128

#endif  // EP128EMU_AY3_8912_HPP

