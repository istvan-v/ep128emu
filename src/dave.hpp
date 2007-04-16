
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

#ifndef EP128EMU_DAVE_HPP
#define EP128EMU_DAVE_HPP

#include "ep128emu.hpp"

namespace Ep128 {

  class DaveTables {
   public:
    // tables for polynomial counters
    // (note: the table data is stored in reverse order)
    uint8_t *polycnt4_table;        // length = 15,     poly = 0x00000008
    uint8_t *polycnt5_table;        // length = 31,     poly = 0x00000013
    uint8_t *polycnt7_table;        // length = 127,    poly = 0x00000040
    uint8_t *polycnt9_table;        // length = 511,    poly = 0x00000109
    uint8_t *polycnt11_table;       // length = 2047,   poly = 0x00000403
    uint8_t *polycnt15_table;       // length = 32767,  poly = 0x00004000
    uint8_t *polycnt17_table;       // length = 131071, poly = 0x00010005
    // ----------------
    DaveTables();
    ~DaveTables();
  };

  class Dave {
   private:
    static DaveTables t;
    int     clockDiv;               // 2 if bit 1 of port 0xBF is 0, 3 otherwise
    int     clockCnt;               // counts from 'clockDiv' towards zero
    // variable length counter uses one of the 9, 11, 15, and 17 bit tables
    uint8_t *polycntVL_table;
    // polynomial counters
    int     polycnt4_phase;         // 4-bit counter phase (14 -> 0)
    int     polycnt5_phase;         // 5-bit counter phase (30 -> 0)
    int     polycnt7_phase;         // 7-bit counter phase (126 -> 0)
    int     polycntVL_phase;        // variable length counter phase ...
    int     polycntVL_maxphase;     // ... counts from this value to zero
    int     polycnt4_state;         // 4-bit counter output
    int     polycnt5_state;         // 5-bit counter output
    int     polycnt7_state;         // 7-bit counter output
    int     polycntVL_state;        // variable length counter output
    // fixed frequency counters (f = 250000 / (n + 1))
    static const int clk_62500_frq = 3;
    static const int clk_1000_frq = 249;
    static const int clk_50_frq = 4999;
    static const int clk_1_frq = 249999;
    int     clk_62500_phase;
    int     clk_1000_phase;
    int     clk_50_phase;
    int     clk_1_phase;
    int     clk_62500_state;
    // channel 0 parameters
    int     chn0_state;             // current output state
    int     chn0_prv;               // previous output state
    int     chn0_state1;            // oscillator output
    int     chn0_phase;             // phase (frqcode -> 0)
    int     chn0_frqcode;           // frequency code (0 - 4095)
    int     *chn0_input_polycnt;    // polynomial counter
    int     chn0_hp_1;              // enable highpass filter
    int     chn0_rm_2;              // enable ring modulation
    int     chn0_run;               // 1: oscillator is running
    int     chn0_left;              // left volume (0 - 63)
    int     chn0_right;             // right volume (0 - 63)
    // channel 1 parameters
    int     chn1_state;             // current output state
    int     chn1_prv;               // previous output state
    int     chn1_state1;            // oscillator output
    int     chn1_phase;             // phase (frqcode -> 0)
    int     chn1_frqcode;           // frequency code (0 - 4095)
    int     *chn1_input_polycnt;    // polynomial counter
    int     chn1_hp_2;              // enable highpass filter
    int     chn1_rm_3;              // enable ring modulation
    int     chn1_run;               // 1: oscillator is running
    int     chn1_left;              // left volume (0 - 63)
    int     chn1_right;             // right volume (0 - 63)
    // channel 2 parameters
    int     chn2_state;             // current output state
    int     chn2_prv;               // previous output state
    int     chn2_state1;            // oscillator output
    int     chn2_phase;             // phase (frqcode -> 0)
    int     chn2_frqcode;           // frequency code (0 - 4095)
    int     *chn2_input_polycnt;    // polynomial counter
    int     chn2_hp_3;              // enable highpass filter
    int     chn2_rm_0;              // enable ring modulation
    int     chn2_run;               // 1: oscillator is running
    int     chn2_left;              // left volume (0 - 63)
    int     chn2_right;             // right volume (0 - 63)
    // channel 3 (noise) parameters
    int     chn3_state;             // current output state
    int     chn3_prv;               // previous output state
    int     chn3_state1;            // polynomial counter output
    int     chn3_state2;            // lowpass filter output
    int     *chn3_clk_source;       // clock input signal
    int     chn3_clk_source_prv;    // previous clock input
    int     noise_polycnt_is_7bit;  // 0xA6 port bit 4
    int     *chn3_input_polycnt;    // polynomial counter
    int     chn3_lp_2;              // enable lowpass filter
    int     chn3_hp_0;              // enable highpass filter
    int     chn3_rm_1;              // enable ring modulation
    int     chn3_left;              // left volume (0 - 63)
    int     chn3_right;             // right volume (0 - 63)
    // enable DAC mode for left/right channel
    int     dac_mode_left;
    int     dac_mode_right;
    // interrupts
    int     *int_snd_phase;
    int     enable_int_snd;
    int     enable_int_1hz;
    int     enable_int_1;
    int     enable_int_2;
    int     int_snd_state;
    int     int_1hz_state;
    int     int_1_state;
    int     int_2_state;
    int     int_snd_active;
    int     int_1hz_active;
    int     int_1_active;
    int     int_2_active;
    uint32_t  audioOutput;
    uint8_t page0Segment;
    uint8_t page1Segment;
    uint8_t page2Segment;
    uint8_t page3Segment;
    int     tape_feedback;
    int     tape_input;
    int     tape_input_level;
    int     keyboardRow;
    uint8_t keyboardState[16];
    // ----------------
    inline void triggerIntSnd();
    inline void triggerInt1Hz();
    int * findPolycntForToneChannel(int n);
    uint32_t runOneCycle_();
   public:
    Dave();
    virtual ~Dave();
   protected:
    virtual void setMemoryPage(uint8_t page, uint8_t segment);
    virtual void setMemoryWaitMode(int mode);
    virtual void setRemote1State(int state);
    virtual void setRemote2State(int state);
    virtual void interruptRequest();
    virtual void clearInterruptRequest();
   public:
    // Run DAVE emulation for 2 us (clock frequency = 500 kHz).
    // Return value is audio output in left_channel + (right_channel << 16)
    // format, where the range for a single channel is 0 to 40320 (sum of 4
    // sound generators and tape feedback, 0 to 8064 each).
    inline uint32_t runOneCycle()
    {
      if (--clockCnt > 0)
        return audioOutput;
      clockCnt = clockDiv;
      return runOneCycle_();
    }
    // Write to a DAVE register.
    void writePort(uint16_t addr, uint8_t value);
    // Read from a DAVE register.
    uint8_t readPort(uint16_t addr) const;
    // Set hardware interrupt 1 state, and (possibly) trigger interrupt.
    void setInt1State(int new_state);
    // Set hardware interrupt 2 state, and (possibly) trigger interrupt.
    void setInt2State(int new_state);
    // Set tape input state, and level (0: low, 1: high).
    void setTapeInput(int state, int level);
    // Set state of key 'keyCode' (0 to 127, see table below) to pressed
    // (state != 0) or released (state == 0).
    //        +-------+-------+-------+-------+-------+-------+-------+-------+
    //        |  0x00 |  0x01 |  0x02 |  0x03 |  0x04 |  0x05 |  0x06 |  0x07 |
    //        |  0x08 |  0x09 |  0x0A |  0x0B |  0x0C |  0x0D |  0x0E |  0x0F |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x00 |     N |     \ |     B |     C |     V |     X |     Z | SHF_L |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x08 |     H |  LOCK |     G |     D |     F |     S |     A |  CTRL |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x10 |     U |     Q |     Y |     R |     T |     E |     W |   TAB |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x18 |     7 |     1 |     6 |     4 |     5 |     3 |     2 |   ESC |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x20 |    F4 |    F8 |    F3 |    F6 |    F5 |    F7 |    F2 |    F1 |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x28 |     8 |       |     9 |     - |     0 |     ^ | ERASE |       |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x30 |     J |       |     K |     ; |     L |     : |     ] |       |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x38 |  STOP |  DOWN | RIGHT |    UP |  HOLD |  LEFT | ENTER |   ALT |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x40 |     M |   DEL |     , |     / |     . | SHF_R | SPACE |   INS |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x48 |     I |       |     O |     @ |     P |     [ |       |       |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x70 | JOY1R | JOY1L | JOY1D | JOY1U | JOY1F |       |       |       |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    // | 0x78 | JOY2R | JOY2L | JOY2D | JOY2U | JOY2F |       |       |       |
    // +------+-------+-------+-------+-------+-------+-------+-------+-------+
    void setKeyboardState(int keyCode, int state);
    // Reset DAVE.
    void reset(bool isColdReset = false);
    // save snapshot
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    // load snapshot
    void loadState(Ep128Emu::File::Buffer&);
    void registerChunkType(Ep128Emu::File&);
  };

}       // namespace Ep128

#endif  // EP128EMU_DAVE_HPP

