
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/ep128emu/
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

#include "ep128emu.hpp"
#include "dave.hpp"
#include <cmath>

// Generate polynomial counter of log2(m0) + 1 bits length, and store
// (m0 * 2 - 1) samples at 'tabptr' in reverse order.
// log2(m1) is the second bit to be used in the XOR operation when
// calculating the next bit of output.

static void calculate_polycnt(uint8_t *tabptr, uint32_t m0, uint32_t m1)
{
  uint32_t  sr = 0xFFFFFFFFU;
  int       n = int(m0 << 1) - 1;
  while (--n >= 0) {
    uint8_t b0 = uint8_t(bool(sr & m0));
    uint8_t b1 = uint8_t(bool(sr & m1));
    tabptr[n] = b0;
    sr = (sr << 1) | uint32_t(b0 ^ b1);
  }
}

namespace Ep128 {

  DaveTables  Dave::t;

  DaveTables::DaveTables()
  {
    static const uint32_t polycnt_params[14] = {
      0x00000008U, 0x00000004U,         // 4-bit: poly = 11001
      0x00000010U, 0x00000004U,         // 5-bit: poly = 101001
      0x00000040U, 0x00000020U,         // 7-bit: poly = 11000001
      0x00000100U, 0x00000010U,         // 9-bit: poly = 1000100001
      0x00000400U, 0x00000100U,         // 11-bit: poly = 101000000001
      0x00004000U, 0x00002000U,         // 15-bit: poly = 1100000000000001
      0x00010000U, 0x00002000U          // 17-bit: poly = 100100000000000001
    };
    polycnt4_table = new uint8_t[15 + 31 + 127 + 511 + 2047 + 32767 + 131071];
    polycnt5_table = &(polycnt4_table[15]);
    polycnt7_table = &(polycnt5_table[31]);
    polycnt9_table = &(polycnt7_table[127]);
    polycnt11_table = &(polycnt9_table[511]);
    polycnt15_table = &(polycnt11_table[2047]);
    polycnt17_table = &(polycnt15_table[32767]);
    uint8_t *bufp = polycnt4_table;
    for (int i = 0; i < 14; i += 2) {
      calculate_polycnt(bufp, polycnt_params[i], polycnt_params[i + 1]);
      bufp = bufp + ((polycnt_params[i] << 1) - 1U);
    }
  }

  DaveTables::~DaveTables()
  {
    delete[] polycnt4_table;
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

  uint32_t Dave::runOneCycle_()
  {
    // update polynomial counters
    if (--polycnt4_phase < 0)                   // 4-bit
      polycnt4_phase = 14;
    polycnt4_state = (int) t.polycnt4_table[polycnt4_phase];
    if (--polycnt5_phase < 0)                   // 5-bit
      polycnt5_phase = 30;
    polycnt5_state = (int) t.polycnt5_table[polycnt5_phase];
    if (!noise_polycnt_is_7bit) {
      // channel 3 uses the variable length polynomial counter
      if (--polycnt7_phase < 0)                 // 7-bit
        polycnt7_phase = 126;
      polycnt7_state = (int) t.polycnt7_table[polycnt7_phase];
      // channel 3 polynomial counter: updated on negative edge
      if (*chn3_clk_source < chn3_clk_source_prv) {
        if (--polycntVL_phase < 0)              // variable length
          polycntVL_phase = polycntVL_maxphase;
        polycntVL_state = (int) polycntVL_table[polycntVL_phase];
        chn3_state1 = polycntVL_state;          // input signal to channel 3
        if (!chn3_lp_2)
          chn3_state2 = polycntVL_state;
      }
      chn3_clk_source_prv = *chn3_clk_source;
    }
    else {
      // channel 3 uses the 7-bit polynomial counter
      if (*chn3_clk_source < chn3_clk_source_prv) {
        // update on negative edge
        if (--polycnt7_phase < 0)               // 7-bit
          polycnt7_phase = 126;
        polycnt7_state = (int) t.polycnt7_table[polycnt7_phase];
        chn3_state1 = polycnt7_state;           // input signal to channel 3
        if (!chn3_lp_2)
          chn3_state2 = polycnt7_state;
      }
      chn3_clk_source_prv = *chn3_clk_source;
      if (--polycntVL_phase < 0)                // variable length
        polycntVL_phase = polycntVL_maxphase;
      polycntVL_state = (int) polycntVL_table[polycntVL_phase];
    }

    // update the phase of all oscillators
    clk_62500_phase--;
    clk_1000_phase--;
    chn0_phase -= chn0_run;
    chn1_phase -= chn1_run;
    chn2_phase -= chn2_run;

    // reload phase counters if necessary
    if (EP128EMU_UNLIKELY(clk_1000_phase < 0)) {
      clk_50_phase--;
      // trigger interrupts if enabled
      if ((*int_snd_phase) < 0) {
        // will reload counter later
        int_snd_state = (int_snd_state & 1) ^ 1;        // invert state
        if (enable_int_snd)
          triggerIntSnd();
      }
      clk_1000_phase = clk_1000_frq;
      if (EP128EMU_UNLIKELY(clk_50_phase < 0)) {
        clk_50_phase = clk_50_frq;
        clk_1_phase--;
        if (EP128EMU_UNLIKELY(clk_1_phase < 0)) {
          clk_1_phase = clk_1_frq;                      // reload counter
          int_1hz_state = (int_1hz_state & 1) ^ 1;      // invert state
          if (enable_int_1hz)
            triggerInt1Hz();
        }
      }
    }
    else if (EP128EMU_UNLIKELY((*int_snd_phase) < 0)) {
      // trigger interrupt if enabled
      int_snd_state = (int_snd_state & 1) ^ 1;          // invert state
      if (enable_int_snd)
        triggerIntSnd();
    }

    // calculate oscillator outputs
    if (clk_62500_phase < 0) {
      // simple 31250 Hz oscillator
      clk_62500_phase = clk_62500_frq;                  // reload counter
      clk_62500_state = (clk_62500_state & 1) ^ 1;      // invert state
    }
    // ---- channel 3 ----
    chn3_prv = chn3_state;                      // save previous output
    if (chn3_lp_2 && (chn2_state < chn2_prv)) {
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
      // ring modulation: XNOR by channel 1
      chn3_state ^= (chn1_state ^ 1);
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
      // ring modulation: XNOR by channel 0
      chn2_state ^= (chn0_state ^ 1);
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
      // ring modulation: XNOR by channel 3
      chn1_state ^= (chn3_state ^ 1);
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
      // ring modulation: XNOR by channel 2
      chn0_state ^= (chn2_state ^ 1);
    }

    // and now the final DAC output (left/right) values
    // total output range (not including tape feedback): 0 to 252
    unsigned int  lval =
        ((tape_feedback & tape_input) == 0 ? 0U : 0x3FU);   // tape feedback
    unsigned int  rval = lval;
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

    audioOutput = uint32_t(lval + (rval << 16)) << 7;
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
    // default to square wave
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
      // bit 4: swap 7-bit and variable length polynomial counters if set
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
        chn0_state1 = 0;
      }
      else if (!chn0_run) {
        chn0_phase = chn0_frqcode;        // reset phase
        chn0_run = 1;
      }
      if ((int) value & 0x02) {
        chn1_run = 0;                     // channel 1 sync
        chn1_state1 = 0;
      }
      else if (!chn1_run) {
        chn1_phase = chn1_frqcode;        // reset phase
        chn1_run = 1;
      }
      if ((int) value & 0x04) {
        chn2_run = 0;                     // channel 2 sync
        chn2_state1 = 0;
      }
      else if (!chn2_run) {
        chn2_phase = chn2_frqcode;        // reset phase
        chn2_run = 1;
      }
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
      tape_feedback = int(!(value & 0x20));     // tape control
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

  uint8_t Dave::readPort(uint16_t addr) const
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
        return uint8_t((int_snd_state | (int_snd_active << 1))
                       | ((int_1hz_state | (int_1hz_active << 1)) << 2)
                       | ((int_1_state | (int_1_active << 1)) << 4)
                       | ((int_2_state | (int_2_active << 1)) << 6));
      }
      break;
    case 0x15:
      // read currently selected keyboard row
      return (keyboardRow < 10 ? keyboardState[keyboardRow] : 0xFF);
    case 0x16:
      {
        // tape input
        uint8_t n =
            uint8_t(((tape_input_level - 1) & 0x40) | ((tape_input - 1) & 0x80)
                    | 0x0F);
        if (keyboardRow < 5) {
          if (keyboardRow == 0) {
            // EnterMice buttons (left and right)
            n &= uint8_t(0xF9 | (mouseInput >> 3));
            // EXT1 joystick fire buttons
            n &= uint8_t(0xF8 | (keyboardState[14] >> 4));
            // EXT3/5 fire buttons actually overlap with EXT1 fire2 and 3
            n &= uint8_t(0xFD | (keyboardState[12] >> 3));
            n &= uint8_t(0xFB | (keyboardState[10] >> 2));
          }
          else {
            if (!(mouseInput & 0x80))   // EnterMice data input on column K
              n &= uint8_t(0xFD | ((mouseInput << 1) >> (keyboardRow - 1)));
            // EXT1 joystick (mapped to row 14)
            n &= uint8_t(0xFE | (keyboardState[14] >> (4 - keyboardRow)));
            n &= uint8_t(0xFD | ((keyboardState[12] << 1) >> (4 - keyboardRow)));
            n &= uint8_t(0xFB | ((keyboardState[10] << 2) >> (4 - keyboardRow)));
          }
        }
        else if (keyboardRow < 10) {
          // external joystick 2 (mapped to keyboard row 15)
          if (keyboardRow == 5) {         // fire buttons
            n &= uint8_t(0xF8 | (keyboardState[15] >> 4));
            // EXT4/6 fire buttons actually overlap with EXT2 fire2 and 3
            n &= uint8_t(0xFD | (keyboardState[13] >> 3));
            n &= uint8_t(0xFB | (keyboardState[11] >> 2));
          }
          else {
            n &= uint8_t(0xFE | (keyboardState[15] >> (9 - keyboardRow)));
            n &= uint8_t(0xFD | ((keyboardState[13] << 1 ) >> (9 - keyboardRow)));
            n &= uint8_t(0xFB | ((keyboardState[11] << 2 ) >> (9 - keyboardRow)));
            }
        }
        return n;
      }
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
    polycnt4_state = 0;
    polycnt5_state = 0;
    polycnt7_state = 0;
    polycntVL_state = 0;
    clk_62500_state = 0;
    chn0_state = 0;
    chn0_prv = 0;
    chn0_state1 = 0;
    chn0_frqcode = 0;
    chn0_run = 1;
    chn1_state = 0;
    chn1_prv = 0;
    chn1_state1 = 0;
    chn1_frqcode = 0;
    chn1_run = 1;
    chn2_state = 0;
    chn2_prv = 0;
    chn2_state1 = 0;
    chn2_frqcode = 0;
    chn2_run = 1;
    chn3_state = 0;
    chn3_prv = 0;
    chn3_state1 = 0;
    chn3_state2 = 0;
    chn3_clk_source_prv = 0;
    int_snd_state = 0;
    int_1hz_state = 0;
    int_1_state = 1;
    int_2_state = 1;
    int_snd_active = 0;
    int_1hz_active = 0;
    int_1_active = 0;
    int_2_active = 0;
    audioOutput = 0;
    tape_input = 0;
    tape_input_level = 0;
    this->reset(true);
  }

  Dave::~Dave()
  {
  }

  void Dave::reset(bool isColdReset)
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
    if (isColdReset) {
      // reset keyboard state
      for (int i = 0; i < 16; i++)
        keyboardState[i] = 0xFF;
    }
    mouseInput = 0xFF;
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

  class ChunkType_DaveSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Dave&   ref;
   public:
    ChunkType_DaveSnapshot(Dave& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_DaveSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_DAVE_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void Dave::saveState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000003);        // version number
    buf.writeByte(uint8_t(clockDiv));
    buf.writeByte(uint8_t(clockCnt));
    if (polycntVL_table == t.polycnt9_table)
      buf.writeByte(9);
    else if (polycntVL_table == t.polycnt11_table)
      buf.writeByte(11);
    else if (polycntVL_table == t.polycnt15_table)
      buf.writeByte(15);
    else
      buf.writeByte(17);
    buf.writeUInt32(uint32_t(polycnt4_phase));
    buf.writeUInt32(uint32_t(polycnt5_phase));
    buf.writeUInt32(uint32_t(polycnt7_phase));
    buf.writeUInt32(uint32_t(polycntVL_phase));
    buf.writeUInt32(uint32_t(polycntVL_maxphase));
    buf.writeByte(uint8_t(polycnt4_state));
    buf.writeByte(uint8_t(polycnt5_state));
    buf.writeByte(uint8_t(polycnt7_state));
    buf.writeByte(uint8_t(polycntVL_state));
    buf.writeUInt32(uint32_t(clk_62500_phase));
    buf.writeUInt32(uint32_t(clk_1000_phase));
    buf.writeUInt32(uint32_t(clk_50_phase));
    buf.writeUInt32(uint32_t(clk_1_phase));
    buf.writeByte(uint8_t(clk_62500_state));
    buf.writeByte(uint8_t(chn0_state));
    buf.writeByte(uint8_t(chn0_prv));
    buf.writeByte(uint8_t(chn0_state1));
    buf.writeUInt32(uint32_t(chn0_phase));
    buf.writeUInt32(uint32_t(chn0_frqcode));
    if (chn0_input_polycnt == &polycnt4_state)
      buf.writeByte(4);
    else if (chn0_input_polycnt == &polycnt5_state)
      buf.writeByte(5);
    else if (chn0_input_polycnt == &polycnt7_state)
      buf.writeByte(7);
    else if (chn0_input_polycnt == &polycntVL_state)
      buf.writeByte(17);
    else
      buf.writeByte(0);
    buf.writeBoolean(bool(chn0_hp_1));
    buf.writeBoolean(bool(chn0_rm_2));
    buf.writeBoolean(bool(chn0_run));
    buf.writeByte(uint8_t(chn0_left));
    buf.writeByte(uint8_t(chn0_right));
    buf.writeByte(uint8_t(chn1_state));
    buf.writeByte(uint8_t(chn1_prv));
    buf.writeByte(uint8_t(chn1_state1));
    buf.writeUInt32(uint32_t(chn1_phase));
    buf.writeUInt32(uint32_t(chn1_frqcode));
    if (chn1_input_polycnt == &polycnt4_state)
      buf.writeByte(4);
    else if (chn1_input_polycnt == &polycnt5_state)
      buf.writeByte(5);
    else if (chn1_input_polycnt == &polycnt7_state)
      buf.writeByte(7);
    else if (chn1_input_polycnt == &polycntVL_state)
      buf.writeByte(17);
    else
      buf.writeByte(0);
    buf.writeBoolean(bool(chn1_hp_2));
    buf.writeBoolean(bool(chn1_rm_3));
    buf.writeBoolean(bool(chn1_run));
    buf.writeByte(uint8_t(chn1_left));
    buf.writeByte(uint8_t(chn1_right));
    buf.writeByte(uint8_t(chn2_state));
    buf.writeByte(uint8_t(chn2_prv));
    buf.writeByte(uint8_t(chn2_state1));
    buf.writeUInt32(uint32_t(chn2_phase));
    buf.writeUInt32(uint32_t(chn2_frqcode));
    if (chn2_input_polycnt == &polycnt4_state)
      buf.writeByte(4);
    else if (chn2_input_polycnt == &polycnt5_state)
      buf.writeByte(5);
    else if (chn2_input_polycnt == &polycnt7_state)
      buf.writeByte(7);
    else if (chn2_input_polycnt == &polycntVL_state)
      buf.writeByte(17);
    else
      buf.writeByte(0);
    buf.writeBoolean(bool(chn2_hp_3));
    buf.writeBoolean(bool(chn2_rm_0));
    buf.writeBoolean(bool(chn2_run));
    buf.writeByte(uint8_t(chn2_left));
    buf.writeByte(uint8_t(chn2_right));
    buf.writeByte(uint8_t(chn3_state));
    buf.writeByte(uint8_t(chn3_prv));
    buf.writeByte(uint8_t(chn3_state1));
    buf.writeByte(uint8_t(chn3_state2));
    if (chn3_clk_source == &chn0_state)
      buf.writeByte(0);
    else if (chn3_clk_source == &chn1_state)
      buf.writeByte(1);
    else if (chn3_clk_source == &chn2_state)
      buf.writeByte(2);
    else
      buf.writeByte(3);
    buf.writeByte(uint8_t(chn3_clk_source_prv));
    buf.writeBoolean(bool(noise_polycnt_is_7bit));
    if (chn3_input_polycnt == &polycnt7_state)
      buf.writeByte(7);
    else
      buf.writeByte(17);
    buf.writeBoolean(bool(chn3_lp_2));
    buf.writeBoolean(bool(chn3_hp_0));
    buf.writeBoolean(bool(chn3_rm_1));
    buf.writeByte(uint8_t(chn3_left));
    buf.writeByte(uint8_t(chn3_right));
    buf.writeBoolean(bool(dac_mode_left));
    buf.writeBoolean(bool(dac_mode_right));
    if (int_snd_phase == &chn0_phase)
      buf.writeByte(0);
    else if (int_snd_phase == &chn1_phase)
      buf.writeByte(1);
    else if (int_snd_phase == &clk_50_phase)
      buf.writeByte(2);
    else
      buf.writeByte(3);
    buf.writeBoolean(bool(enable_int_snd));
    buf.writeBoolean(bool(enable_int_1hz));
    buf.writeBoolean(bool(enable_int_1));
    buf.writeBoolean(bool(enable_int_2));
    buf.writeByte(uint8_t(int_snd_state));
    buf.writeByte(uint8_t(int_1hz_state));
    buf.writeByte(uint8_t(int_1_state));
    buf.writeByte(uint8_t(int_2_state));
    buf.writeBoolean(bool(int_snd_active));
    buf.writeBoolean(bool(int_1hz_active));
    buf.writeBoolean(bool(int_1_active));
    buf.writeBoolean(bool(int_2_active));
    buf.writeUInt32(audioOutput);
    buf.writeByte(page0Segment);
    buf.writeByte(page1Segment);
    buf.writeByte(page2Segment);
    buf.writeByte(page3Segment);
    buf.writeBoolean(bool(tape_feedback));
    buf.writeByte(uint8_t(tape_input));
    buf.writeByte(uint8_t(tape_input_level));
    buf.writeByte(uint8_t(keyboardRow));
    for (size_t i = 0; i < 16; i++)
      buf.writeByte(keyboardState[i]);
    buf.writeByte(mouseInput);
  }

  void Dave::saveState(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_DAVE_STATE, buf);
  }

  void Dave::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (!(version >= 0x01000000 && version <= 0x01000003)) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible Dave snapshot format");
    }
    // reset DAVE (FIXME: needed ?)
    this->reset(true);
    // load saved state
    clockDiv = (buf.readByte() & 1) | 2;
    clockCnt = buf.readByte() & 3;
    {
      uint32_t  len = 131071;
      switch (buf.readByte()) {
      case 9:
        polycntVL_table = t.polycnt9_table;
        len = 511;
        break;
      case 11:
        polycntVL_table = t.polycnt11_table;
        len = 2047;
        break;
      case 15:
        polycntVL_table = t.polycnt15_table;
        len = 32767;
        break;
      default:
        polycntVL_table = t.polycnt17_table;
      }
      polycnt4_phase = int(buf.readUInt32() % 15U);
      polycnt5_phase = int(buf.readUInt32() % 31U);
      polycnt7_phase = int(buf.readUInt32() % 127U);
      polycntVL_phase = int(buf.readUInt32() % len);
      polycntVL_maxphase = int(len - 1);
      if (buf.readUInt32() != (len - 1))
        throw Ep128Emu::Exception("inconsistent Dave snapshot data");
    }
    polycnt4_state = (buf.readByte() == 0 ? 0 : 1);
    polycnt5_state = (buf.readByte() == 0 ? 0 : 1);
    polycnt7_state = (buf.readByte() == 0 ? 0 : 1);
    polycntVL_state = (buf.readByte() == 0 ? 0 : 1);
    clk_62500_phase = int(buf.readUInt32() % uint32_t(clk_62500_frq + 1));
    clk_1000_phase = int(buf.readUInt32() % uint32_t(clk_1000_frq + 1));
    {
      uint32_t  clk50Phase_ = buf.readUInt32();
      uint32_t  clk1Phase_ = buf.readUInt32();
      if (version < 0x01000003) {
        clk50Phase_ = clk50Phase_ / 250U;
        clk1Phase_ = clk1Phase_ / 5000U;
      }
      clk_50_phase = int(clk50Phase_ % uint32_t(clk_50_frq + 1));
      clk_1_phase = int(clk1Phase_ % uint32_t(clk_1_frq + 1));
    }
    clk_62500_state = (buf.readByte() == 0 ? 0 : 1);
    chn0_state = (buf.readByte() == 0 ? 0 : 1);
    chn0_prv = (buf.readByte() == 0 ? 0 : 1);
    chn0_state1 = (buf.readByte() == 0 ? 0 : 1);
    chn0_phase = int(buf.readUInt32() & 0x0FFF);
    chn0_frqcode = int(buf.readUInt32() & 0x0FFF);
    switch (buf.readByte()) {
    case 0:
      chn0_input_polycnt = (int *) 0;
      break;
    case 4:
      chn0_input_polycnt = &polycnt4_state;
      break;
    case 5:
      chn0_input_polycnt = &polycnt5_state;
      break;
    case 7:
      chn0_input_polycnt = &polycnt7_state;
      break;
    default:
      chn0_input_polycnt = &polycntVL_state;
    }
    chn0_hp_1 = (buf.readBoolean() ? 1 : 0);
    chn0_rm_2 = (buf.readBoolean() ? 1 : 0);
    chn0_run = (buf.readBoolean() ? 1 : 0);
    chn0_left = buf.readByte() & 0x3F;
    chn0_right = buf.readByte() & 0x3F;
    chn1_state = (buf.readByte() == 0 ? 0 : 1);
    chn1_prv = (buf.readByte() == 0 ? 0 : 1);
    chn1_state1 = (buf.readByte() == 0 ? 0 : 1);
    chn1_phase = int(buf.readUInt32() & 0x0FFF);
    chn1_frqcode = int(buf.readUInt32() & 0x0FFF);
    switch (buf.readByte()) {
    case 0:
      chn1_input_polycnt = (int *) 0;
      break;
    case 4:
      chn1_input_polycnt = &polycnt4_state;
      break;
    case 5:
      chn1_input_polycnt = &polycnt5_state;
      break;
    case 7:
      chn1_input_polycnt = &polycnt7_state;
      break;
    default:
      chn1_input_polycnt = &polycntVL_state;
    }
    chn1_hp_2 = (buf.readBoolean() ? 1 : 0);
    chn1_rm_3 = (buf.readBoolean() ? 1 : 0);
    chn1_run = (buf.readBoolean() ? 1 : 0);
    chn1_left = buf.readByte() & 0x3F;
    chn1_right = buf.readByte() & 0x3F;
    chn2_state = (buf.readByte() == 0 ? 0 : 1);
    chn2_prv = (buf.readByte() == 0 ? 0 : 1);
    chn2_state1 = (buf.readByte() == 0 ? 0 : 1);
    chn2_phase = int(buf.readUInt32() & 0x0FFF);
    chn2_frqcode = int(buf.readUInt32() & 0x0FFF);
    switch (buf.readByte()) {
    case 0:
      chn2_input_polycnt = (int *) 0;
      break;
    case 4:
      chn2_input_polycnt = &polycnt4_state;
      break;
    case 5:
      chn2_input_polycnt = &polycnt5_state;
      break;
    case 7:
      chn2_input_polycnt = &polycnt7_state;
      break;
    default:
      chn2_input_polycnt = &polycntVL_state;
    }
    chn2_hp_3 = (buf.readBoolean() ? 1 : 0);
    chn2_rm_0 = (buf.readBoolean() ? 1 : 0);
    chn2_run = (buf.readBoolean() ? 1 : 0);
    chn2_left = buf.readByte() & 0x3F;
    chn2_right = buf.readByte() & 0x3F;
    chn3_state = (buf.readByte() == 0 ? 0 : 1);
    chn3_prv = (buf.readByte() == 0 ? 0 : 1);
    chn3_state1 = (buf.readByte() == 0 ? 0 : 1);
    chn3_state2 = (buf.readByte() == 0 ? 0 : 1);
    switch (buf.readByte()) {
    case 0:
      chn3_clk_source = &chn0_state;
      break;
    case 1:
      chn3_clk_source = &chn1_state;
      break;
    case 2:
      chn3_clk_source = &chn2_state;
      break;
    default:
      chn3_clk_source = &clk_62500_state;
    }
    chn3_clk_source_prv = (buf.readByte() == 0 ? 0 : 1);
    noise_polycnt_is_7bit = (buf.readBoolean() ? 1 : 0);
    if (buf.readByte() == 7)
      chn3_input_polycnt = &polycnt7_state;
    else
      chn3_input_polycnt = &polycntVL_state;
    chn3_lp_2 = (buf.readBoolean() ? 1 : 0);
    chn3_hp_0 = (buf.readBoolean() ? 1 : 0);
    chn3_rm_1 = (buf.readBoolean() ? 1 : 0);
    chn3_left = buf.readByte() & 0x3F;
    chn3_right = buf.readByte() & 0x3F;
    dac_mode_left = (buf.readBoolean() ? 1 : 0);
    dac_mode_right = (buf.readBoolean() ? 1 : 0);
    switch (buf.readByte()) {
    case 0:
      int_snd_phase = &chn0_phase;
      break;
    case 1:
      int_snd_phase = &chn1_phase;
      break;
    case 2:
      int_snd_phase = &clk_50_phase;
      break;
    default:
      int_snd_phase = &clk_1000_phase;
    }
    enable_int_snd = (buf.readBoolean() ? 1 : 0);
    enable_int_1hz = (buf.readBoolean() ? 1 : 0);
    enable_int_1 = (buf.readBoolean() ? 1 : 0);
    enable_int_2 = (buf.readBoolean() ? 1 : 0);
    int_snd_state = (buf.readByte() == 0 ? 0 : 1);
    int_1hz_state = (buf.readByte() == 0 ? 0 : 1);
    int_1_state = (buf.readByte() == 0 ? 0 : 1);
    int_2_state = (buf.readByte() == 0 ? 0 : 1);
    int_snd_active = (buf.readBoolean() ? 1 : 0);
    int_1hz_active = (buf.readBoolean() ? 1 : 0);
    int_1_active = (buf.readBoolean() ? 1 : 0);
    int_2_active = (buf.readBoolean() ? 1 : 0);
    audioOutput = buf.readUInt32() & 0x01FF01FF;
    writePort(0x10, buf.readByte());
    writePort(0x11, buf.readByte());
    writePort(0x12, buf.readByte());
    writePort(0x13, buf.readByte());
    tape_feedback = (buf.readBoolean() ? 1 : 0);
    tape_input = (buf.readByte() == 0 ? 0 : 1);
    tape_input_level = (buf.readByte() == 0 ? 0 : 1);
    keyboardRow = buf.readByte() & 0x0F;
    for (size_t i = 0; i < 16; i++)
      keyboardState[i] = buf.readByte();
    if (version >= 0x01000001)
      mouseInput = buf.readByte() ^ (version >= 0x01000002 ? 0x00 : 0x30);
    else
      mouseInput = 0xFF;
    if (buf.getPosition() != buf.getDataSize())
      throw Ep128Emu::Exception("trailing garbage at end of "
                                "Dave snapshot data");
  }

  void Dave::registerChunkType(Ep128Emu::File& f)
  {
    ChunkType_DaveSnapshot  *p;
    p = new ChunkType_DaveSnapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
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

