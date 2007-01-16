//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2004  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#ifndef __SID_HPP__
#define __SID_HPP__

#include "ep128emu.hpp"
#include "siddefs.hpp"
#include "voice.hpp"
#include "filter.hpp"
#include "extfilt.hpp"
#include "pot.hpp"
#include "fileio.hpp"

namespace Plus4 {

  class SID
  {
  public:
    SID();
    ~SID();

    void set_chip_model(chip_model model);
    void enable_filter(bool enable);
    void enable_external_filter(bool enable);

    void fc_default(const fc_point*& points, int& count);
    PointPlotter<sound_sample> fc_plotter();

    void clock();
    void clock(cycle_count delta_t);
    void reset();

    // Read/write registers.
    reg8 read(reg8 offset);
    void write(reg8 offset, reg8 value);

    // Read/write state.
    class State
    {
    public:
      State();

      char sid_register[0x20];

      reg8 bus_value;
      cycle_count bus_value_ttl;

      reg24 accumulator[3];
      reg24 shift_register[3];
      reg16 rate_counter[3];
      reg16 rate_counter_period[3];
      reg16 exponential_counter[3];
      reg16 exponential_counter_period[3];
      reg8 envelope_counter[3];
      EnvelopeGenerator::State envelope_state[3];
      bool hold_zero[3];
    };

    State read_state();
    void write_state(const State& state);

    // 16-bit input (EXT IN).
    void input(int sample);

    // 16-bit output (AUDIO OUT).
    int output();
    // n-bit output.
    int output(int bits);

  protected:
    Voice voice[3];
    Filter filter;
    ExternalFilter extfilt;
    Potentiometer potx;
    Potentiometer poty;

    reg8 bus_value;
    cycle_count bus_value_ttl;

    // External audio input.
    int ext_in;

  public:
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
  };

}       // namespace Plus4

#endif  // not __SID_HPP__

