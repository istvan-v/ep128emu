
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

#include "ep128emu.hpp"
#include "cpu.hpp"
#include "ted.hpp"

namespace Plus4 {

  void TED7360::set_video_line(int lineNum, bool isHorizontalRefresh)
  {
    int   verticalScroll;
    bool  PAL_mode = ((memory_ram[0xFF07] & (uint8_t) 0x40) ? false : true);

    lineNum &= 0x01FF;
    if (PAL_mode) {
      // PAL mode (312 lines)
      if (lineNum == 312 && isHorizontalRefresh)
        lineNum = 0;
      if (lineNum == video_line)
        return;
      video_line = lineNum;
      if (lineNum >= 254 && lineNum < 258)
        verticalSync();
    }
    else {
      // NTSC mode (262 lines)
      if (lineNum == 262 && isHorizontalRefresh)
        lineNum = 0;
      if (lineNum == video_line)
        return;
      video_line = lineNum;
      if (lineNum >= 229 && lineNum < 233)
        verticalSync();
    }
    // update character sub-line
    if (isHorizontalRefresh) {
      character_position = character_position_reload;
      if (DMA_enabled) {
        character_line = (character_line + 1) & 7;
        if (character_line == 7)
          character_position_reload = (character_position_reload + 40) & 0x03FF;
      }
    }
    verticalScroll = (int) (memory_ram[0xFF06] & (uint8_t) 0x07);
    if (memory_ram[0xFF06] & (uint8_t) 0x10) {
      // if display is enabled:
      if (!lineNum) {
        // at line 0: start reading pixel data, but still display border only
        display_enabled = false;
        render_enabled = true;
        DMA_enabled = false;
        vertical_blanking = false;
        // initialize registers
        character_line = 7;
        for (int j = 0; j < 40; j++)
          char_buf[j] = (uint8_t) 0x00;
        character_position = 0;
        character_position_reload = 0;
      }
      else {
        if (memory_ram[0xFF06] & (uint8_t) 0x08) {
          // 25 line mode:
          if (lineNum == 4)
            display_enabled = true;
          else if (lineNum == 204)
            display_enabled = false;
        }
        else {
          // 24 line mode:
          if (lineNum == 8)
            display_enabled = true;
          else if (lineNum == 200)
            display_enabled = false;
        }
      }
      // start DMA fetches, delayed by vertical scroll
      if (render_enabled && (lineNum & 7) == verticalScroll)
        DMA_enabled = true;
    }
    if (lineNum == 204) {
      // at end of display area, update flash counter and state
      memory_ram[0xFF1F] = (memory_ram[0xFF1F] & (uint8_t) 0x7F) + (uint8_t) 8;
      if (memory_ram[0xFF1F] & (uint8_t) 0x80)
        flash_state = (flash_state ? false : true);
      // bottom border start
      display_enabled = false;
      render_enabled = false;
      DMA_enabled = false;
      vertical_blanking = false;
    }
    else if ((PAL_mode && lineNum == 251) || (!PAL_mode && lineNum == 226)) {
      // bottom border end, vertical blanking start
      display_enabled = false;
      render_enabled = false;
      DMA_enabled = false;
      vertical_blanking = true;
    }
    else if ((PAL_mode && lineNum == 270) || (!PAL_mode && lineNum == 245)) {
      // vertical blanking end, top border start
      display_enabled = false;
      render_enabled = false;
      DMA_enabled = false;
      vertical_blanking = false;
    }
    // select method table for rendering display
    //   0: vertical blanking
    //   1: border, no data fetch
    //   2: border, but render display
    //   3: border, DMA (character line 0)
    //   4: border, DMA (character line 7)
    //   5: render display (character lines 1 to 6)
    //   6: display, DMA (character line 0)
    //   7: display, DMA (character line 7)
    //  +8: 38 column mode
    // +16: single clock mode
    if (vertical_blanking)
      display_mode = 0;
    else if (!display_enabled && !render_enabled)
      display_mode = 1;
    else {
      display_mode = 2;
      if (DMA_enabled) {
        if (!character_line)
          display_mode = 3;
        else if (character_line == 7 && lineNum != 203)
          display_mode = 4;     // NOTE: need not start DMA at last line
      }
      if (display_enabled)
        display_mode += 3;
    }
    if (!(memory_ram[0xFF07] & (uint8_t) 0x08))     // 38 column mode
      display_mode += 8;
    if (memory_ram[0xFF13] & (uint8_t) 0x02)        // single clock mode
      display_mode += 16;
    // check for video interrupt
    if ((uint8_t) (lineNum & 0xFF) == memory_ram[0xFF0B] &&
        (uint8_t) (lineNum >> 8) == (memory_ram[0xFF0A] & (uint8_t) 0x01)) {
      video_interrupt_shift_register |= 0x080;
    }
  }

  // horizontal blanking (88..113)

  void TED7360::run_blank_CPU()
  {
    int     i = ((int) video_column << 2) - 388;
    i = (i < 0 ? i + 456 : i);
    line_buf[i]     = (uint8_t) 0;
    line_buf[i + 1] = (uint8_t) 0;
    line_buf[i + 2] = (uint8_t) 0;
    line_buf[i + 3] = (uint8_t) 0;
    runCPU();
  }

  void TED7360::run_blank_no_CPU()
  {
    int     i = ((int) video_column << 2) - 388;
    i = (i < 0 ? i + 456 : i);
    line_buf[i]     = (uint8_t) 0;
    line_buf[i + 1] = (uint8_t) 0;
    line_buf[i + 2] = (uint8_t) 0;
    line_buf[i + 3] = (uint8_t) 0;
  }

  // border areas with no data fetch

  void TED7360::run_border_CPU()
  {
    int     i = ((int) video_column << 2) - 388;
    uint8_t c = memory_ram[0xFF19];
    i = (i < 0 ? i + 456 : i);
    line_buf[i]     = c;
    line_buf[i + 1] = c;
    line_buf[i + 2] = c;
    line_buf[i + 3] = c;
    runCPU();
  }

  void TED7360::run_border_no_CPU()
  {
    int     i = ((int) video_column << 2) - 388;
    uint8_t c = memory_ram[0xFF19];
    i = (i < 0 ? i + 456 : i);
    line_buf[i]     = c;
    line_buf[i + 1] = c;
    line_buf[i + 2] = c;
    line_buf[i + 3] = c;
  }

  // border areas, reading attribute data (character line 0)

  void TED7360::run_border_DMA_0()
  {
    int     ndx = ((int) video_column >> 1) - 55;
    ndx = (ndx < 0 ? ndx + 57 : ndx);
    // fetch attribute data
    attr_buf[ndx] = readMemory((uint16_t) (((ndx + character_position_reload)
                                            & 0x03FF) + attr_base_addr));
    // copy character data from temporary buffer
    char_buf[ndx] = char_buf_tmp[ndx];
    int     i = ((int) video_column << 2) - 388;
    uint8_t c = memory_ram[0xFF19];
    i = (i < 0 ? i + 456 : i);
    line_buf[i]     = c;
    line_buf[i + 1] = c;
    line_buf[i + 2] = c;
    line_buf[i + 3] = c;
  }

  // border areas, reading character data (character line 7)

  void TED7360::run_border_DMA_7()
  {
    int     ndx = ((int) video_column >> 1) - 55;
    ndx = (ndx < 0 ? ndx + 57 : ndx);
    // fetch character data
    char_buf_tmp[ndx] =
        readMemory((uint16_t) (((ndx + character_position_reload)
                                | 0x0400) + attr_base_addr));
    int     i = ((int) video_column << 2) - 388;
    uint8_t c = memory_ram[0xFF19];
    i = (i < 0 ? i + 456 : i);
    line_buf[i]     = c;
    line_buf[i + 1] = c;
    line_buf[i + 2] = c;
    line_buf[i + 3] = c;
  }

  // border areas, starting display

  void TED7360::run_border_render_init()
  {
    int     i = ((int) video_column << 2) - 388;
    uint8_t c = memory_ram[0xFF19];
    i = (i < 0 ? i + 456 : i);
    line_buf[i]     = c;
    line_buf[i + 1] = c;
    line_buf[i + 2] = c;
    line_buf[i + 3] = c;
    render_init();
  }

  // border areas, rendering display

  void TED7360::run_border_render()
  {
    int     i = ((int) video_column << 2) - 388;
    uint8_t c = memory_ram[0xFF19];
    i = (i < 0 ? i + 456 : i);
    line_buf[i]     = c;
    line_buf[i + 1] = c;
    line_buf[i + 2] = c;
    line_buf[i + 3] = c;
    (this->*render_func)();
    character_position = (character_position + 1) & 0x03FF;
    // shift contents of pixel buffer for next 8 pixels
    pixel_buf[0] = pixel_buf[8];
    pixel_buf[1] = pixel_buf[9];
    pixel_buf[2] = pixel_buf[10];
    pixel_buf[3] = pixel_buf[11];
    pixel_buf[4] = pixel_buf[12];
    pixel_buf[5] = pixel_buf[13];
    pixel_buf[6] = pixel_buf[14];
    pixel_buf[7] = pixel_buf[15];
  }

  // display area, reading attribute data (character line 0)

  void TED7360::run_display_DMA_0()
  {
    int     ndx = ((int) video_column >> 1) - 55;
    ndx = (ndx < 0 ? ndx + 57 : ndx);
    // fetch attribute data
    attr_buf[ndx] = readMemory((uint16_t) (((ndx + character_position_reload)
                                            & 0x03FF) + attr_base_addr));
    // copy character data from temporary buffer
    char_buf[ndx] = char_buf_tmp[ndx];
    // read from pixel buffer with horizontal scroll
    int     i = ((int) video_column << 2) - 388;
    int     j;
    i = (i < 0 ? i + 456 : i);
    j = 12 - (int) horiz_scroll;
    line_buf[i]     = pixel_buf[j];
    line_buf[i + 1] = pixel_buf[j + 1];
    line_buf[i + 2] = pixel_buf[j + 2];
    line_buf[i + 3] = pixel_buf[j + 3];
    // shift contents of pixel buffer for next 8 pixels
    pixel_buf[0] = pixel_buf[8];
    pixel_buf[1] = pixel_buf[9];
    pixel_buf[2] = pixel_buf[10];
    pixel_buf[3] = pixel_buf[11];
    pixel_buf[4] = pixel_buf[12];
    pixel_buf[5] = pixel_buf[13];
    pixel_buf[6] = pixel_buf[14];
    pixel_buf[7] = pixel_buf[15];
  }

  // display area, reading character data (character line 7)

  void TED7360::run_display_DMA_7()
  {
    int     ndx = ((int) video_column >> 1) - 55;
    ndx = (ndx < 0 ? ndx + 57 : ndx);
    // fetch character data
    char_buf_tmp[ndx] =
        readMemory((uint16_t) (((ndx + character_position_reload)
                                | 0x0400) + attr_base_addr));
    // read from pixel buffer with horizontal scroll
    int     i = ((int) video_column << 2) - 388;
    int     j;
    i = (i < 0 ? i + 456 : i);
    j = 12 - (int) horiz_scroll;
    line_buf[i]     = pixel_buf[j];
    line_buf[i + 1] = pixel_buf[j + 1];
    line_buf[i + 2] = pixel_buf[j + 2];
    line_buf[i + 3] = pixel_buf[j + 3];
    // shift contents of pixel buffer for next 8 pixels
    pixel_buf[0] = pixel_buf[8];
    pixel_buf[1] = pixel_buf[9];
    pixel_buf[2] = pixel_buf[10];
    pixel_buf[3] = pixel_buf[11];
    pixel_buf[4] = pixel_buf[12];
    pixel_buf[5] = pixel_buf[13];
    pixel_buf[6] = pixel_buf[14];
    pixel_buf[7] = pixel_buf[15];
  }

  // display area, rendering display

  void TED7360::run_display_render()
  {
    (this->*render_func)();
    character_position = (character_position + 1) & 0x03FF;
    // read from pixel buffer with horizontal scroll
    int     i = ((int) video_column << 2) - 388;
    int     j;
    i = (i < 0 ? i + 456 : i);
    j = 8 - (int) horiz_scroll;
    line_buf[i]     = pixel_buf[j];
    line_buf[i + 1] = pixel_buf[j + 1];
    line_buf[i + 2] = pixel_buf[j + 2];
    line_buf[i + 3] = pixel_buf[j + 3];
  }

  // display area

  void TED7360::run_display_CPU()
  {
    // read from pixel buffer with horizontal scroll
    int     i = ((int) video_column << 2) - 388;
    int     j;
    i = (i < 0 ? i + 456 : i);
    j = 12 - (int) horiz_scroll;
    line_buf[i]     = pixel_buf[j];
    line_buf[i + 1] = pixel_buf[j + 1];
    line_buf[i + 2] = pixel_buf[j + 2];
    line_buf[i + 3] = pixel_buf[j + 3];
    // shift contents of pixel buffer for next 8 pixels
    pixel_buf[0] = pixel_buf[8];
    pixel_buf[1] = pixel_buf[9];
    pixel_buf[2] = pixel_buf[10];
    pixel_buf[3] = pixel_buf[11];
    pixel_buf[4] = pixel_buf[12];
    pixel_buf[5] = pixel_buf[13];
    pixel_buf[6] = pixel_buf[14];
    pixel_buf[7] = pixel_buf[15];
    runCPU();
  }

  void TED7360::run_display_no_CPU()
  {
    // read from pixel buffer with horizontal scroll
    int     i = ((int) video_column << 2) - 388;
    int     j;
    i = (i < 0 ? i + 456 : i);
    j = 12 - (int) horiz_scroll;
    line_buf[i]     = pixel_buf[j];
    line_buf[i + 1] = pixel_buf[j + 1];
    line_buf[i + 2] = pixel_buf[j + 2];
    line_buf[i + 3] = pixel_buf[j + 3];
    // shift contents of pixel buffer for next 8 pixels
    pixel_buf[0] = pixel_buf[8];
    pixel_buf[1] = pixel_buf[9];
    pixel_buf[2] = pixel_buf[10];
    pixel_buf[3] = pixel_buf[11];
    pixel_buf[4] = pixel_buf[12];
    pixel_buf[5] = pixel_buf[13];
    pixel_buf[6] = pixel_buf[14];
    pixel_buf[7] = pixel_buf[15];
  }

  void TED7360::run(int nCycles)
  {
    for (int i = 0; i < nCycles; i++, cycle_count++) {
      if (ted_disabled) {
        if (cycle_count & 1UL) {
          int j = cpu_clock_multiplier;
          do {
            M7501::runOneCycle();
          } while (--j);
        }
        continue;
      }
      // update display and run CPU
      (this->*(display_func_table[display_mode][video_column]))();
      // check for video interrupt
      video_interrupt_shift_register >>= 1;
      if ((video_interrupt_shift_register & 1U) != 0U) {
        if ((memory_ram[0xFF0A] & (uint8_t) 0x02) != (uint8_t) 0)
          memory_ram[0xFF09] |= (uint8_t) 0x82;
      }
      // update timers on even cycle count (884 kHz rate)
      if (!(cycle_count & 1UL)) {
        if (timer1_run) {
          timer1_state = (timer1_state - 1) & 0xFFFF;
          if (!timer1_state) {
            // reload timer
            timer1_state = timer1_reload_value;
            // trigger interrupt if enabled
            if (memory_ram[0xFF0A] & (uint8_t) 0x08)
              memory_ram[0xFF09] |= (uint8_t) 0x88;
          }
        }
        if (timer2_run) {
          timer2_state = (timer2_state - 1) & 0xFFFF;
          if (!timer2_state) {
            // trigger interrupt if enabled
            if (memory_ram[0xFF0A] & (uint8_t) 0x10)
              memory_ram[0xFF09] |= (uint8_t) 0x90;
          }
        }
        if (timer3_run) {
          timer3_state = (timer3_state - 1) & 0xFFFF;
          if (!timer3_state) {
            // trigger interrupt if enabled
            if (memory_ram[0xFF0A] & (uint8_t) 0x40)
              memory_ram[0xFF09] |= (uint8_t) 0xC0;
          }
        }
      }
      // update sound generators on every 8th cycle (221 kHz)
      if (!(cycle_count & 7UL)) {
        int     sound_output = 0;
        int     sound_volume;
        uint8_t sound_register = memory_ram[0xFF11];
        sound_volume = (int) (sound_register & (uint8_t) 0x0F) << 10;
        sound_volume = (sound_volume < 8192 ? sound_volume : 8192);
        if (sound_register & (uint8_t) 0x80) {
          // DAC mode
          sound_channel_1_cnt = sound_channel_1_reload;
          sound_channel_2_cnt = sound_channel_2_reload;
          sound_channel_1_state = (uint8_t) 1;
          sound_channel_2_state = (uint8_t) 1;
          sound_channel_2_noise_state = (uint8_t) 0xFF;
          sound_channel_2_noise_output = (uint8_t) 1;
        }
        else {
          if (++sound_channel_1_cnt >= 1024) {
            // channel 1 square wave
            sound_channel_1_cnt = sound_channel_1_reload;
            sound_channel_1_state ^= (uint8_t) 1;
            sound_channel_1_state &= (uint8_t) 1;
          }
          if (++sound_channel_2_cnt >= 1024) {
            // channel 2 square wave
            sound_channel_2_cnt = sound_channel_2_reload;
            sound_channel_2_state ^= (uint8_t) 1;
            sound_channel_2_state &= (uint8_t) 1;
            // channel 2 noise, 8 bit polycnt (10110011)
            uint8_t tmp = sound_channel_2_noise_state & (uint8_t) 0xB3;
            tmp = tmp ^ (tmp >> 1);
            tmp = tmp ^ (tmp >> 2);
            tmp = (tmp ^ (tmp >> 4)) & (uint8_t) 1;
            sound_channel_2_noise_output ^= tmp;
            sound_channel_2_noise_output &= (uint8_t) 1;
            sound_channel_2_noise_state <<= 1;
            sound_channel_2_noise_state |= sound_channel_2_noise_output;
          }
        }
        // mix sound outputs
        if (sound_register & (uint8_t) 0x10)
          sound_output += (sound_channel_1_state ? sound_volume : 0);
        if (sound_register & (uint8_t) 0x20)
          sound_output += (sound_channel_2_state ? sound_volume : 0);
        else if (sound_register & (uint8_t) 0x40)
          sound_output += (sound_channel_2_noise_output ? 0 : sound_volume);
        // send sound output signal (sample rate = 221 kHz)
        playSample((int16_t) sound_output);
      }
      // update horizontal refresh of display
      video_column = (video_column < 113 ? video_column + 1 : 0);
      if (video_column == 97) {
        // at end of line:
        // flush line buffer
        for (int j = 0; j < 456; j++)
          line_buf[j] &= (uint8_t) 0x7F;
        drawLine(&(line_buf[0]), 456);
        for (int j = 0; j < 456; j++)
          line_buf[j] = (uint8_t) 0;
        // update line counter
        set_video_line(video_line + 1, true);
      }
    }
  }

}       // namespace Plus4

