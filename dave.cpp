
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

#include "ep128.hpp"
#include "dave.hpp"

// calculate parity of 'n'
// returns 0 if parity is even, and 1 if parity is odd

static int parity_32(uint32_t n)
{
    n = n ^ (n >> 1);
    n = n ^ (n >> 2);
    n = n ^ (n >> 4);
    n = n ^ (n >> 8);
    n = n ^ (n >> 16);

    return int(n & 1);
}

// generate polynomial counter of 'nbits' length, and store
// ((2 ^ nbits) - 1) samples at 'tabptr'
// the table will be played back in reverse order, so the 'poly' constant
// should be set to a value that reproduces the output of a tone channel
// sampling the polynomial counter of 'nbits' length on a real DAVE chip,
// with a frequency code of (((2 ^ nbits) - 3) + (k * ((2 ^ nbits) - 1))),
// where k is any integer

static void calculate_polycnt(uint8_t *tabptr, int nbits, int poly)
{
    int i, j, k;

    j = 0x7FFFFFFF;
    k = 1;
    for (i = 0; i < ((1 << nbits) - 1); i++) {
      k = (k ^ (int) parity_32((uint32_t) (j & poly))) & 1;
      j = (j << 1) | k;
      tabptr[i] = (uint8_t) k;
    }
}

namespace Ep128 {

  DaveTables  Dave::t;

  DaveTables::DaveTables()
  {
    polycnt4_table = (uint8_t*) 0;
    polycnt5_table = (uint8_t*) 0;
    polycnt7_table = (uint8_t*) 0;
    polycnt9_table = (uint8_t*) 0;
    polycnt11_table = (uint8_t*) 0;
    polycnt15_table = (uint8_t*) 0;
    polycnt17_table = (uint8_t*) 0;
    try {
      polycnt4_table = new uint8_t[15];
      polycnt5_table = new uint8_t[31];
      polycnt7_table = new uint8_t[127];
      polycnt9_table = new uint8_t[511];
      polycnt11_table = new uint8_t[2047];
      polycnt15_table = new uint8_t[32767];
      polycnt17_table = new uint8_t[131071];
    }
    catch (...) {
      if (polycnt4_table)
        delete[] polycnt4_table;
      if (polycnt5_table)
        delete[] polycnt5_table;
      if (polycnt7_table)
        delete[] polycnt7_table;
      if (polycnt9_table)
        delete[] polycnt9_table;
      if (polycnt11_table)
        delete[] polycnt11_table;
      if (polycnt15_table)
        delete[] polycnt15_table;
      if (polycnt17_table)
        delete[] polycnt17_table;
      throw;
    }
    // 4-bit polynomial counter (poly = 8)
    calculate_polycnt(polycnt4_table, 4, 8);
    // 5-bit polynomial counter (poly = 19)
    calculate_polycnt(polycnt5_table, 5, 19);
    // 7-bit polynomial counter (poly = 64)
    calculate_polycnt(polycnt7_table, 7, 64);
    // 9-bit polynomial counter (poly = 265)
    calculate_polycnt(polycnt9_table, 9, 265);
    // 11-bit polynomial counter (poly = 1027)
    calculate_polycnt(polycnt11_table, 11, 1027);
    // 15-bit polynomial counter (poly = 16384)
    calculate_polycnt(polycnt15_table, 15, 16384);
    // 17-bit polynomial counter (poly = 65541)
    calculate_polycnt(polycnt17_table, 17, 65541);
  }

  DaveTables::~DaveTables()
  {
    if (polycnt4_table)
      delete[] polycnt4_table;
    if (polycnt5_table)
      delete[] polycnt5_table;
    if (polycnt7_table)
      delete[] polycnt7_table;
    if (polycnt9_table)
      delete[] polycnt9_table;
    if (polycnt11_table)
      delete[] polycnt11_table;
    if (polycnt15_table)
      delete[] polycnt15_table;
    if (polycnt17_table)
      delete[] polycnt17_table;
  }

  // handle timer interrupts

  inline void Dave::triggerIntSnd()
  {
    // trigger interrupt on edge only
    if (int_snd_active)
      return;
    // mark as active
    int_snd_active = 1;
    interruptRequest();
  }

  inline void Dave::triggerInt1Hz()
  {
    // trigger interrupt on edge only
    if (int_1hz_active)
      return;
    // mark as active
    int_1hz_active = 1;
    interruptRequest();
  }

  // run DAVE emulation, and also trigger any sound or timer interrupts

  uint32_t Dave::runOneCycle()
  {
    if (--clockCnt > 0)
      return audioOutput;
    clockCnt = clockDiv;

    unsigned int  lval, rval;

    // update polynomial counters
    if (--polycnt4_phase < 0)                   // 4-bit
      polycnt4_phase = 14;
    if (--polycnt5_phase < 0)                   // 5-bit
      polycnt5_phase = 30;
    if (!noise_polycnt_is_7bit) {
      // channel 3 uses the variable length polynomial counter
      if (--polycnt7_phase < 0)                 // 7-bit
        polycnt7_phase = 126;
      // channel 3 polynomial counter: updated on negative edge
      if (*chn3_clk_source < chn3_clk_source_prv) {
        if (--polycntVL_phase < 0)              // variable length
          polycntVL_phase = polycntVL_maxphase;
      }
      chn3_clk_source_prv = *chn3_clk_source;
    }
    else {
      // channel 3 uses the 7-bit polynomial counter
      if (*chn3_clk_source < chn3_clk_source_prv) {
        // update on negative edge
        if (--polycnt7_phase < 0)               // 7-bit
          polycnt7_phase = 126;
      }
      chn3_clk_source_prv = *chn3_clk_source;
      if (--polycntVL_phase < 0)                // variable length
        polycntVL_phase = polycntVL_maxphase;
    }
    // read polynomial counter tables
    polycnt4_state = (int) t.polycnt4_table[polycnt4_phase];
    polycnt5_state = (int) t.polycnt5_table[polycnt5_phase];
    polycnt7_state = (int) t.polycnt7_table[polycnt7_phase];
    polycntVL_state = (int) polycntVL_table[polycntVL_phase];

    // update the phase of all oscillators
    clk_62500_phase--;
    clk_1000_phase--;
    clk_50_phase--;
    clk_1_phase--;
    if (chn0_run)
      chn0_phase--;
    if (chn1_run)
      chn1_phase--;
    if (chn2_run)
      chn2_phase--;

    // trigger interrupts if enabled
    if ((*int_snd_phase) < 0) {
      // will reload counter later
      int_snd_state = (int_snd_state & 1) ^ 1;          // invert state
      if (enable_int_snd)
        triggerIntSnd();
    }
    if (clk_1_phase < 0) {
      clk_1_phase = clk_1_frq;                          // reload counter
      int_1hz_state = (int_1hz_state & 1) ^ 1;          // invert state
      if (enable_int_1hz)
        triggerInt1Hz();
    }

    // reload phase counters if necessary
    if (clk_1000_phase < 0)
      clk_1000_phase = clk_1000_frq;
    if (clk_50_phase < 0)
      clk_50_phase = clk_50_frq;

    // calculate oscillator outputs
    if (clk_62500_phase < 0) {
      // simple 31250 Hz oscillator
      clk_62500_phase = clk_62500_frq;                  // reload counter
      clk_62500_state = (clk_62500_state & 1) ^ 1;      // invert state
    }
    // ---- channel 3 ----
    chn3_prv = chn3_state;                      // save previous output
    chn3_state1 = *chn3_input_polycnt;          // get input signal
    if (!chn3_lp_2 || (chn2_state < chn2_prv)) {
      // lowpass filter holds signal until negative edge in channel 2
      chn3_state2 = chn3_state1;
    }
    if (chn3_hp_0 && (chn0_state < chn0_prv)) {
      // highpass filter: sets level to 0 on negative edge in channel 0
      chn3_state2 = 0;
    }
    // store final output signal in chn3_state
    chn3_state = chn3_state2;
    if (chn3_rm_1) {
      // ring modulation: XOR by channel 1
      chn3_state ^= chn1_state;
    }
    // ---- channel 2 ----
    chn2_prv = chn2_state;                      // save previous output
    if (chn2_phase < 0) {
      chn2_phase = chn2_frqcode;                // reload counter
      if (chn2_input_polycnt == NULL) {
        // square wave
        chn2_state1 = (chn2_state1 & 1) ^ 1;
      }
      else {
        // get input from polynomial counter
        chn2_state1 = *chn2_input_polycnt;
      }
    }
    if (chn2_hp_3 && (chn3_state < chn3_prv)) {
      // highpass filter: sets level to 0 on negative edge in channel 3
      chn2_state1 = 0;
    }
    // store final output signal in chn2_state
    chn2_state = chn2_state1;
    if (chn2_rm_0) {
      // ring modulation: XOR by channel 0
      chn2_state ^= chn0_state;
    }
    // ---- channel 1 ----
    chn1_prv = chn1_state;                      // save previous output
    if (chn1_phase < 0) {
      chn1_phase = chn1_frqcode;                // reload counter
      if (chn1_input_polycnt == NULL) {
        // square wave
        chn1_state1 = (chn1_state1 & 1) ^ 1;
      }
      else {
        // get input from polynomial counter
        chn1_state1 = *chn1_input_polycnt;
      }
    }
    if (chn1_hp_2 && (chn2_state < chn2_prv)) {
      // highpass filter: sets level to 0 on negative edge in channel 2
      chn1_state1 = 0;
    }
    // store final output signal in chn1_state
    chn1_state = chn1_state1;
    if (chn1_rm_3) {
      // ring modulation: XOR by channel 3
      chn1_state ^= chn3_state;
    }
    // ---- channel 0 ----
    chn0_prv = chn0_state;                      // save previous output
    if (chn0_phase < 0) {
      chn0_phase = chn0_frqcode;                // reload counter
      if (chn0_input_polycnt == NULL) {
        // square wave
        chn0_state1 = (chn0_state1 & 1) ^ 1;
      }
      else {
        // get input from polynomial counter
        chn0_state1 = *chn0_input_polycnt;
      }
    }
    if (chn0_hp_1 && (chn1_state < chn1_prv)) {
      // highpass filter: sets level to 0 on negative edge in channel 1
      chn0_state1 = 0;
    }
    // store final output signal in chn0_state
    chn0_state = chn0_state1;
    if (chn0_rm_2) {
      // ring modulation: XOR by channel 2
      chn0_state ^= chn2_state;
    }

    // and now the final DAC output (left/right) values
    // total output range (not including tape feedback): 0 to 252
    if (tape_feedback && tape_input) {
      // tape feedback (if enabled)
      lval = 0x3F;
      rval = 0x3F;
    }
    else {
      lval = 0;
      rval = 0;
    }
    if (dac_mode_left) {
      lval += (chn0_left << 2);
      if (dac_mode_right) {
        // simplest case: both channels in DAC mode
        rval += (chn0_right << 2);
      }
      else {
        // left channel is in DAC mode, but right is not
        if (chn0_state)
          rval += chn0_right;
        if (chn1_state)
          rval += chn1_right;
        if (chn2_state)
          rval += chn2_right;
        if (chn3_state)
          rval += chn3_right;
      }
    }
    else if (dac_mode_right) {
      // right channel is in DAC mode, but left is not
      rval += (chn0_right << 2);
      if (chn0_state)
        lval += chn0_left;
      if (chn1_state)
        lval += chn1_left;
      if (chn2_state)
        lval += chn2_left;
      if (chn3_state)
        lval += chn3_left;
    }
    else {
      // neither channel is in DAC mode
      if (chn0_state) {
        lval += chn0_left;
        rval += chn0_right;
      }
      if (chn1_state) {
        lval += chn1_left;
        rval += chn1_right;
      }
      if (chn2_state) {
        lval += chn2_left;
        rval += chn2_right;
      }
      if (chn3_state) {
        lval += chn3_left;
        rval += chn3_right;
      }
    }

    audioOutput = uint32_t(lval + (rval << 16));
    return audioOutput;
  }

  // returns pointer to the polynomial counter for channels 0, 1, and 2
  // selected by 'n' (allowed values for 'n' are 0x00, 0x10, 0x20, and 0x30)

  int * Dave::findPolycntForToneChannel(int n)
  {
    switch (n) {
      case 0x10:
        return (&polycnt4_state);       // 4-bit
      case 0x20:
        return (&polycnt5_state);       // 5-bit
      case 0x30:
        if (!noise_polycnt_is_7bit) {
          return (&polycnt7_state);     // 7-bit
        }
        else {
          return (&polycntVL_state);    // variable length
        }
    }
    // defaunt to square wave
    return (int*) NULL;
  }

  // write to DAVE registers

  void Dave::writePort(uint16_t addr, uint8_t value)
  {
    switch (uint8_t(addr & 0x1F)) {
    case 0x00:
      // channel 0 frequency
      chn0_frqcode = (chn0_frqcode & 0x0F00) | (int) value;
      break;
    case 0x01:
      // channel 0 frequency and mode
      chn0_frqcode = (chn0_frqcode & 0x00FF) | (((int) value & 0x0F) << 8);
      // select distortion mode
      chn0_input_polycnt = findPolycntForToneChannel((int) value & 0x30);
      chn0_hp_1 = ((int) value & 0x40 ? 1 : 0);         // highpass
      chn0_rm_2 = ((int) value & 0x80 ? 1 : 0);         // ringmod
      break;
    case 0x02:
      // channel 1 frequency
      chn1_frqcode = (chn1_frqcode & 0x0F00) | (int) value;
      break;
    case 0x03:
      // channel 1 frequency and mode
      chn1_frqcode = (chn1_frqcode & 0x00FF) | (((int) value & 0x0F) << 8);
      // select distortion mode
      chn1_input_polycnt = findPolycntForToneChannel((int) value & 0x30);
      chn1_hp_2 = ((int) value & 0x40 ? 1 : 0);         // highpass
      chn1_rm_3 = ((int) value & 0x80 ? 1 : 0);         // ringmod
      break;
    case 0x04:
      // channel 2 frequency
      chn2_frqcode = (chn2_frqcode & 0x0F00) | (int) value;
      break;
    case 0x05:
      // channel 2 frequency and mode
      chn2_frqcode = (chn2_frqcode & 0x00FF) | (((int) value & 0x0F) << 8);
      // select distortion mode
      chn2_input_polycnt = findPolycntForToneChannel((int) value & 0x30);
      chn2_hp_3 = ((int) value & 0x40 ? 1 : 0);         // highpass
      chn2_rm_0 = ((int) value & 0x80 ? 1 : 0);         // ringmod
      break;
    case 0x06:
      // channel 3 parameters
      switch ((int) value & 0x03) {
        // polynomial counter clock source
        case 0x00: chn3_clk_source = &clk_62500_state; break;
        case 0x01: chn3_clk_source = &chn0_state; break;
        case 0x02: chn3_clk_source = &chn1_state; break;
        case 0x03: chn3_clk_source = &chn2_state; break;
      }
      // select variable length polynomial counter
      switch ((int) value & 0x0C) {
        case 0x00:
          polycntVL_table = t.polycnt17_table;          // 17-bit
          polycntVL_maxphase = 131070;
          break;
        case 0x04:
          polycntVL_table = t.polycnt15_table;          // 15-bit
          polycntVL_maxphase = 32766;
          break;
        case 0x08:
          polycntVL_table = t.polycnt11_table;          // 11-bit
          polycntVL_maxphase = 2046;
          break;
        case 0x0C:
          polycntVL_table = t.polycnt9_table;           // 9-bit
          polycntVL_maxphase = 510;
          break;
      }
      // wrap the phase of variable length polynomial counter to table length
      polycntVL_phase = polycntVL_phase % (polycntVL_maxphase + 1);
      // bit 4: exchange 7-bit and variable length polynomial counters if set
      if ((int) value & 0x10) {
        noise_polycnt_is_7bit = 1;
        chn3_input_polycnt = &polycnt7_state;
        if (chn0_input_polycnt == &polycnt7_state)
          chn0_input_polycnt = &polycntVL_state;
        if (chn1_input_polycnt == &polycnt7_state)
          chn1_input_polycnt = &polycntVL_state;
        if (chn2_input_polycnt == &polycnt7_state)
          chn2_input_polycnt = &polycntVL_state;
      }
      else {
        noise_polycnt_is_7bit = 0;
        chn3_input_polycnt = &polycntVL_state;
        if (chn0_input_polycnt == &polycntVL_state)
          chn0_input_polycnt = &polycnt7_state;
        if (chn1_input_polycnt == &polycntVL_state)
          chn1_input_polycnt = &polycnt7_state;
        if (chn2_input_polycnt == &polycntVL_state)
          chn2_input_polycnt = &polycnt7_state;
      }
      chn3_lp_2 = ((int) value & 0x20 ? 1 : 0); // lowpass with channel 2
      chn3_hp_0 = ((int) value & 0x40 ? 1 : 0); // highpass with channel 0
      chn3_rm_1 = ((int) value & 0x80 ? 1 : 0); // ring mod. with channel 1
      break;
    case 0x07:
      // sound/interrupt control register
      if ((int) value & 0x01) {
        chn0_run = 0;                     // channel 0 sync
        chn0_phase = chn0_frqcode;        // reset phase
      }
      else
        chn0_run = 1;
      if ((int) value & 0x02) {
        chn1_run = 0;                     // channel 1 sync
        chn1_phase = chn1_frqcode;        // reset phase
      }
      else
        chn1_run = 1;
      if ((int) value & 0x04) {
        chn2_run = 0;                     // channel 2 sync
        chn2_phase = chn2_frqcode;        // reset phase
      }
      else
        chn2_run = 1;
      dac_mode_left  = ((int) value & 0x08 ? 1 : 0);    // analogue mode
      dac_mode_right = ((int) value & 0x10 ? 1 : 0);
      switch ((int) value & 0x60) {
      // sound interrupt mode
      case 0x00:
        int_snd_phase = &clk_1000_phase;
        break;
      case 0x20:
        int_snd_phase = &clk_50_phase;
        break;
      case 0x40:
        int_snd_phase = &chn0_phase;
        break;
      case 0x60:
        int_snd_phase = &chn1_phase;
        break;
      }
      break;
    case 0x08:
      // channel 0 left volume
      chn0_left = int(value & 0x3F);
      break;
    case 0x09:
      // channel 1 left volume
      chn1_left = int(value & 0x3F);
      break;
    case 0x0A:
      // channel 2 left volume
      chn2_left = int(value & 0x3F);
      break;
    case 0x0B:
      // channel 3 left volume
      chn3_left = int(value & 0x3F);
      break;
    case 0x0C:
      // channel 0 right volume
      chn0_right = int(value & 0x3F);
      break;
    case 0x0D:
      // channel 1 right volume
      chn1_right = int(value & 0x3F);
      break;
    case 0x0E:
      // channel 2 right volume
      chn2_right = int(value & 0x3F);
      break;
    case 0x0F:
      // channel 3 right volume
      chn3_right = int(value & 0x3F);
      break;
    case 0x10:
      // memory page 0
      page0Segment = value;
      setMemoryPage(0, value);
      break;
    case 0x11:
      // memory page 1
      page1Segment = value;
      setMemoryPage(1, value);
      break;
    case 0x12:
      // memory page 2
      page2Segment = value;
      setMemoryPage(2, value);
      break;
    case 0x13:
      // memory page 3
      page3Segment = value;
      setMemoryPage(3, value);
      break;
    case 0x14:
      // interrupt control register
      {
        int prv = (int_snd_active | int_1hz_active
                   | int_1_active | int_2_active);
        uint8_t tmp = (uint8_t) value ^ (uint8_t) 0x55;
        // sound/timer interrupt
        enable_int_snd = (tmp & (uint8_t) 0x01 ? 0 : 1);
        if (tmp & (uint8_t) 0x03)
          int_snd_active = 0;
        // 1 Hz interrupt
        enable_int_1hz = (tmp & (uint8_t) 0x04 ? 0 : 1);
        if (tmp & (uint8_t) 0x0C)
          int_1hz_active = 0;
        // INT 1 (video interrupt)
        enable_int_1 = (tmp & (uint8_t) 0x10 ? 0 : 1);
        if (tmp & (uint8_t) 0x30)
          int_1_active = 0;
        // INT 2
        enable_int_2 = (tmp & (uint8_t) 0x40 ? 0 : 1);
        if (tmp & (uint8_t) 0xC0)
          int_2_active = 0;
        if (prv && !(int_snd_active | int_1hz_active
                     | int_1_active | int_2_active)) {
          // no more active interrupts: clear request to CPU
          clearInterruptRequest();
        }
      }
      break;
    case 0x15:
      // select keyboard row
      keyboardRow = int(value & 0x0F);
      if (value & 0x20)                         // tape control
        tape_feedback = (tape_feedback & 1) ^ 1;
      setRemote1State(value & 0x40 ? 1 : 0);
      setRemote2State(value & 0x80 ? 1 : 0);
      break;
    case 0x1F:          // system configuration register
      {
        // CPU wait cycle control
        setMemoryWaitMode(int(value & 0x0C) >> 2);
        // input clock frequency (note: the frequency is always 8 MHz,
        // this bit sets the assumed value)
        //   0:  8 MHz, dave_clock_freq = input_freq / 32
        //   1: 12 MHz, dave_clock_freq = input_freq / 48
        clockDiv = ((value & 0x02) == 0 ? 2 : 3);
      }
      break;
    }
  }

  // read from DAVE registers

  uint8_t Dave::readPort(uint16_t addr)
  {
    switch (uint8_t(addr & 0x1F)) {
    case 0x10:
      return page0Segment;
    case 0x11:
      return page1Segment;
    case 0x12:
      return page2Segment;
    case 0x13:
      return page3Segment;
    case 0x14:
      {
        // interrupt state
        uint8_t n = 0;
        int_snd_state &= 1;
        int_1hz_state &= 1;
        if (int_snd_state)
          n |= 0x01;
        if (int_snd_active)
          n |= 0x02;
        if (int_1hz_state)
          n |= 0x04;
        if (int_1hz_active)
          n |= 0x08;
        if (int_1_state)
          n |= 0x10;
        if (int_1_active)
          n |= 0x20;
        if (int_2_state)
          n |= 0x40;
        if (int_2_active)
          n |= 0x80;
        return (uint8_t) n;
      }
      break;
    case 0x15:
      // read currently selected keyboard row
      return keyboardState[keyboardRow];
    case 0x16:
      // tape input
      return uint8_t(0x3F | (tape_input_level > 0 ? 0x40 : 0x00)
                          | (tape_input ? 0x80 : 0x00));
    }
    // anything else is either handled elsewhere, or is write-only
    return 0xFF;
  }

  // set hardware interrupt 1 state, and (possibly) trigger interrupt

  void Dave::setInt1State(int new_state)
  {
    int prv = int_1_state;
    // set new state
    int_1_state = (new_state ? 1 : 0);
    // on negative edge, trigger CPU interrupt
    // (assuming it is enabled, and not active already)
    if (!enable_int_1)
      return;           // disabled
    if (int_1_state || !prv)
      return;           // not on negative edge
    if (int_1_active)
      return;           // already active
    // now active
    int_1_active = 1;
    // send request to CPU
    interruptRequest();
  }

  // set hardware interrupt 2 state, and (possibly) trigger interrupt

  void Dave::setInt2State(int new_state)
  {
    int prv = int_2_state;
    // set new state
    int_2_state = (new_state ? 1 : 0);
    // on negative edge, trigger CPU interrupt
    // (assuming it is enabled, and not active already)
    if (!enable_int_2)
      return;           // disabled
    if (int_2_state || !prv)
      return;           // not on negative edge
    if (int_2_active)
      return;           // already active
    // now active
    int_2_active = 1;
    // send request to CPU
    interruptRequest();
  }

  Dave::Dave()
  {
    clockDiv = 2;
    clockCnt = 1;
    polycntVL_table = t.polycnt17_table;
    polycnt4_phase = 0;
    polycnt5_phase = 0;
    polycnt7_phase = 0;
    polycntVL_phase = 0;
    polycntVL_maxphase = 131070;
    polycnt4_state = 0;
    polycnt5_state = 0;
    polycnt7_state = 0;
    polycntVL_state = 0;
    clk_62500_phase = 0;
    clk_1000_phase = 0;
    clk_50_phase = 0;
    clk_1_phase = 0;
    clk_62500_state = 0;
    chn0_state = 0;
    chn0_prv = 0;
    chn0_state1 = 0;
    chn0_phase = 0;
    chn0_frqcode = 0;
    chn0_input_polycnt = (int*) 0;
    chn0_hp_1 = 0;
    chn0_rm_2 = 0;
    chn0_run = 1;
    chn0_left = 0;
    chn0_right = 0;
    chn1_state = 0;
    chn1_prv = 0;
    chn1_state1 = 0;
    chn1_phase = 0;
    chn1_frqcode = 0;
    chn1_input_polycnt = (int*) 0;
    chn1_hp_2 = 0;
    chn1_rm_3 = 0;
    chn1_run = 1;
    chn1_left = 0;
    chn1_right = 0;
    chn2_state = 0;
    chn2_prv = 0;
    chn2_state1 = 0;
    chn2_phase = 0;
    chn2_frqcode = 0;
    chn2_input_polycnt = (int*) 0;
    chn2_hp_3 = 0;
    chn2_rm_0 = 0;
    chn2_run = 1;
    chn2_left = 0;
    chn2_right = 0;
    chn3_state = 0;
    chn3_prv = 0;
    chn3_state1 = 0;
    chn3_state2 = 0;
    chn3_clk_source = &clk_62500_state;
    chn3_clk_source_prv = 0;
    noise_polycnt_is_7bit = 0;
    chn3_input_polycnt = &polycntVL_state;
    chn3_lp_2 = 0;
    chn3_hp_0 = 0;
    chn3_rm_1 = 0;
    chn3_left = 0;
    chn3_right = 0;
    dac_mode_left = 0;
    dac_mode_right = 0;
    int_snd_phase = &clk_1000_phase;
    enable_int_snd = 0;
    enable_int_1hz = 0;
    enable_int_1 = 0;
    enable_int_2 = 0;
    int_snd_state = 0;
    int_1hz_state = 0;
    int_1_state = 1;
    int_2_state = 1;
    int_snd_active = 0;
    int_1hz_active = 0;
    int_1_active = 0;
    int_2_active = 0;
    audioOutput = 0;
    page0Segment = 0;
    page1Segment = 0;
    page2Segment = 0;
    page3Segment = 0;
    tape_feedback = 0;
    tape_input = 0;
    tape_input_level = 0;
    keyboardRow = 0;
    for (int i = 0; i < 16; i++)
      keyboardState[i] = 0xFF;
  }

  Dave::~Dave()
  {
  }

  void Dave::reset()
  {
    polycnt4_phase = 0;
    polycnt5_phase = 0;
    polycnt7_phase = 0;
    polycntVL_phase = 0;
    chn0_phase = 0;
    chn1_phase = 0;
    chn2_phase = 0;
    clk_62500_phase = 0;
    clk_1000_phase = 0;
    clk_50_phase = 0;
    clk_1_phase = 0;
    // initialize registers
    // this will also reset many variables to the default value
    for (uint16_t i = 0x00; i < 0x20; i++)
      writePort(i, 0);
    // clear all interrupts
    writePort(0x14, 0xAA);
    // reset keyboard state
    for (int i = 0; i < 16; i++)
      keyboardState[i] = 0xFF;
  }

  void Dave::setTapeInput(int state, int level)
  {
    tape_input = (state ? 1 : 0);
    tape_input_level = (level > 0 ? 1 : 0);
  }

  void Dave::setKeyboardState(int keyCode, int state)
  {
    int     row = (keyCode & 0x78) >> 3;
    uint8_t mask = uint8_t(1 << (keyCode & 0x07));
    if (!state)
      keyboardState[row] |= mask;
    else
      keyboardState[row] &= (~mask);
  }

  // --------------------------------------------------------------------------

  void Dave::setMemoryPage(uint8_t page, uint8_t segment)
  {
    (void) page;
    (void) segment;
  }

  void Dave::setMemoryWaitMode(int mode)
  {
    (void) mode;
  }

  void Dave::setRemote1State(int state)
  {
    (void) state;
  }

  void Dave::setRemote2State(int state)
  {
    (void) state;
  }

  void Dave::interruptRequest()
  {
  }

  void Dave::clearInterruptRequest()
  {
  }

}       // namespace Ep128

