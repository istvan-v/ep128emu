
// plus4 -- portable Commodore PLUS/4 emulator
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

#include "ep128emu.hpp"
#include "cpu.hpp"
#include "ted.hpp"

namespace Plus4 {

  void TED7360::runOneCycle()
  {
    if (ted_disabled) {
      tedRegisterWriteMask = 0U;
      dmaCycleCounter = 0;
      M7501::setIsCPURunning(true);
      M7501::run(cpu_clock_multiplier);
      if (!cycle_count)
        playSample(0);
      if (line_buf_pos < 533) {
        if (line_buf_pos >= 0) {
          uint8_t *bufp = &(line_buf[line_buf_pos]);
          bufp[3] = bufp[2] = bufp[1] = bufp[0] = uint8_t(0x00);
          bufp[7] = bufp[6] = bufp[5] = bufp[4] = uint8_t(0x00);
        }
        line_buf_pos += 9;
      }
      if (!ted_disabled)
        video_column |= uint8_t(0x01);
      cycle_count = (cycle_count + 1) & 3;
      return;
    }

    if (video_column & uint8_t(0x01)) {
      // -------- ODD HALF-CYCLE (FF1E bit 1 == 1) --------
      switch (video_column) {
      case 1:                           // start display (38 column mode)
        if (displayWindow &&
            (tedRegisters[0x07] & uint8_t(0x08)) == uint8_t(0)) {
          displayActive = true;
        }
        break;
      case 75:                          // DRAM refresh start
        singleClockModeFlags |= uint8_t(0x01);
        bitmapAddressDisableFlags = bitmapAddressDisableFlags | 0x01;
        renderingDisplay = false;
        // terminate DMA transfer
        dmaCycleCounter = 0;
        M7501::setIsCPURunning(true);
        break;
      case 77:                          // end of display (38 column mode)
        if ((tedRegisters[0x07] & uint8_t(0x08)) == uint8_t(0))
          displayActive = false;
        break;
      case 79:                          // end of display (40 column mode)
        if ((tedRegisters[0x07] & uint8_t(0x08)) != uint8_t(0))
          displayActive = false;
        videoShiftRegisterEnabled = displayActive;
        break;
      case 85:                          // DRAM refresh end
        singleClockModeFlags &= uint8_t(0x02);
        break;
      case 87:                          // horizontal blanking start
        displayBlankingFlags = displayBlankingFlags | 0x01;
        if (line_buf_pos >= 9 && line_buf_pos <= 541) {
          bool  invColors = (invertColorPhaseFlag != bool(savedVideoLine & 1));
          if (invColors)
            invColors = !(tedRegisters[0x07] & 0x40);
          if (line_buf_pos <= 442 && !invColors)
            drawLine(&(line_buf[0]), 432);
          else
            resampleAndDrawLine(invColors);
          invertColorPhaseFlag = !invertColorPhaseFlag;
        }
        line_buf_pos = 1000;
        break;
      case 95:
        if (renderWindow) {
          // if done attribute DMA in this line, continue with character
          // DMA in next one
          if ((uint8_t(savedVideoLine) ^ tedRegisters[0x06]) & 0x07)
            dmaFlags = 0;
          else if (dmaEnabled)
            dmaFlags = 2;
        }
        break;
      case 97:                          // increment line number
        if ((tedRegisters[0x07] & uint8_t(0x40)) == uint8_t(0)) {   // PAL
          savedVideoLine =
              (savedVideoLine != 311 ? ((video_line + 1) & 0x01FF) : 0);
        }
        else {                                                      // NTSC
          savedVideoLine =
              (savedVideoLine != 261 ? ((video_line + 1) & 0x01FF) : 0);
        }
        if (savedVideoLine == 0) {      // end of screen
          character_position = 0x0000;
          character_position_reload = 0x0000;
        }
        line_buf_pos = -35;
        prvCharacterLine = uint8_t(character_line);
        break;
      case 99:
        if (dmaWindow)                  // increment character sub-line
          character_line = (character_line + 1) & 7;
        if (video_line == 205) {
          dma_position = 0x03FF;
          dma_position_reload = 0x03FF;
          dmaFlags = 0;
        }
        if (savedVideoLine == 203)
          dmaEnabled = false;
        if (savedVideoLine == 204) {    // end of display
          renderWindow = false;
          dmaWindow = false;
          bitmapAddressDisableFlags = bitmapAddressDisableFlags | 0x02;
        }
        else if (renderWindow) {
          if (dmaFlags & 2)
            bitmapAddressDisableFlags = bitmapAddressDisableFlags & 0x01;
          // check if DMA should be requested
          if (!((uint8_t(savedVideoLine) ^ tedRegisters[0x06]) & 0x07) &&
              dmaEnabled) {
            // start a new DMA at character line 7
            dmaWindow = true;
            dmaCycleCounter = 1;
            dmaFlags = dmaFlags | 1;
            dma_position = dma_position & 0x03FF;
          }
          else if (dmaFlags & 2) {
            // done reading attribute data in previous line,
            // now continue DMA to get character data
            dmaCycleCounter = 1;
            dma_position = dma_position | 0x0400;
          }
        }
        break;
      case 101:                         // external fetch single clock start
        if (renderWindow) {
          singleClockModeFlags |= uint8_t(0x01);
          if (savedVideoLine == 0) {    // initialize character sub-line
            // FIXME: this check is a hack
            if (bitmapAddressDisableFlags & 0x02) {
              character_line = 7;
              prvCharacterLine = uint8_t(7);
            }
          }
        }
        character_column = 0x3C;
        break;
      case 105:                         // horizontal blanking end
        displayBlankingFlags = displayBlankingFlags & 0x02;
        break;
      case 107:
        dma_position = (dma_position & 0x0400) | dma_position_reload;
        incrementingDMAPosition = renderWindow;
        break;
      case 109:                         // start DMA and/or bitmap fetch
        if (renderWindow | displayWindow | displayActive) {
          bitmapAddressDisableFlags = bitmapAddressDisableFlags & 0x02;
          renderingDisplay = true;
        }                               // initialize character position
        character_position = character_position_reload;
        break;
      case 111:
        if (renderWindow | displayWindow | displayActive) {
          renderingDisplay = true;
          videoShiftRegisterEnabled = true;
        }
        break;
      case 113:                         // start display (40 column mode)
        if (displayWindow &&
            (tedRegisters[0x07] & uint8_t(0x08)) != uint8_t(0)) {
          displayActive = true;
        }
        break;
      }
      // initialize DMA if requested
      if (dmaCycleCounter != uint8_t(0)) {
        dmaCycleCounter++;
        switch (dmaCycleCounter) {
        case 3:
          singleClockModeFlags |= uint8_t(0x01);
          break;
        case 4:
          M7501::setIsCPURunning(false);
          break;
        }
      }
      // check for video interrupt
      if (video_line == videoInterruptLine) {
        if (!prvVideoInterruptState) {
          prvVideoInterruptState = true;
          tedRegisters[0x09] |= uint8_t(0x02);
        }
      }
      else
        prvVideoInterruptState = false;
      // run CPU and render display
      tedRegisterWriteMask = 0U;
      if (dmaCycleCounter < 7) {
        if (((tedRegisters[0x09] & tedRegisters[0x0A]) & uint8_t(0x5E))
            != uint8_t(0))
          M7501::interruptRequest();
        M7501::run(cpu_clock_multiplier);
      }
      // perform DMA fetches on odd cycle counts
      if (dmaCycleCounter >= 4) {
        if (renderingDisplay) {
          if (dmaCycleCounter >= 7) {
            memoryReadMap = tedDMAReadMap;
            (void) readMemory(uint16_t(attr_base_addr | dma_position));
            memoryReadMap = cpuMemoryReadMap;
          }
          if (dmaFlags & 1)
            attr_buf_tmp[character_column] = dataBusState;
          if (dmaFlags & 2)
            char_buf[character_column] = dataBusState;
        }
      }
      if (incrementingDMAPosition)
        dma_position = (dma_position & 0x0400) | ((dma_position + 1) & 0x03FF);
      // calculate video output
      {
        bool    tmpFlag = videoShiftRegisterEnabled;
        if ((unsigned int) line_buf_pos < 533U) {
          uint8_t *bufp = &(line_buf[line_buf_pos]);
          if (displayBlankingFlags) {
            bufp[0] = uint8_t(0x00);
            bufp[1] = uint8_t(0x00);
            bufp[2] = uint8_t(0x00);
            bufp[3] = uint8_t(0x00);
          }
          else if (!displayActive) {
            uint8_t c = tedRegisters[0x19];
            bufp[0] = (!(tedRegisterWriteMask & 0x02000000U) ?
                       c : uint8_t(0xFF));
            bufp[1] = c;
            bufp[2] = c;
            bufp[3] = c;
          }
          else {
            prv_render_func(*this, bufp, 0);
            tmpFlag = false;
          }
        }
        if (tmpFlag)
          render_invalid_mode(*this, (uint8_t *) 0, 0);
      }
      // delay video mode changes by one cycle
      prv_render_func = render_func;
      // check timer interrupts
      if (timer1_run) {
        if (!timer1_state) {
          tedRegisters[0x09] |= uint8_t(0x08);
          // reload timer
          timer1_state = timer1_reload_value;
        }
        // update timer 1 on odd cycle count (886 kHz rate)
        timer1_state = (timer1_state - 1) & 0xFFFF;
      }
      if (!timer2_state) {
        if (timer2_run)
          tedRegisters[0x09] |= uint8_t(0x10);
      }
      if (!timer3_state) {
        if (timer3_run)
          tedRegisters[0x09] |= uint8_t(0x40);
      }
      // update horizontal position
      video_column =
          uint8_t(!(tedRegisterWriteMask & 0x40000000U) ?
                  (video_column != 113 ? ((video_column + 1) & 0x7F) : 0)
                  : (video_column & 0x7E));
    }

    // -------- EVEN HALF-CYCLE (FF1E bit 1 == 0) --------
    switch (video_column) {
    case 70:                            // update DMA read position
      if (renderWindow && character_line == 6)
        dma_position_reload =
            (dma_position + (incrementingDMAPosition ? 1 : 0)) & 0x03FF;
      break;
    case 72:
      incrementingDMAPosition = false;
      break;
    case 74:
      // update character position reload (FF1A, FF1B)
      if (!bitmapAddressDisableFlags && prvCharacterLine == uint8_t(6)) {
        if (!(tedRegisterWriteMask & 0x0C000000U))
          character_position_reload = (character_position + 1) & 0x03FF;
      }
      break;
    case 88:                            // increment flash counter
      if (video_line == 205) {
        tedRegisters[0x1F] =
            (tedRegisters[0x1F] & uint8_t(0x7F)) + uint8_t(0x08);
        if (tedRegisters[0x1F] & uint8_t(0x80))
          flashState = uint8_t(flashState == 0x00 ? 0xFF : 0x00);
      }
      break;
    case 98:
      if ((tedRegisters[0x07] & uint8_t(0x40)) == uint8_t(0)) {     // PAL
        switch (savedVideoLine) {
        case 251:
          displayBlankingFlags = displayBlankingFlags | 0x02;
          break;
        case 254:
          verticalSync(true, 8);
          break;
        case 257:
          verticalSync(false, 28);
          invertColorPhaseFlag = true;
          break;
        case 269:
          displayBlankingFlags = displayBlankingFlags & 0x01;
          break;
        }
      }
      else {                                                        // NTSC
        switch (savedVideoLine) {
        case 226:
          displayBlankingFlags = displayBlankingFlags | 0x02;
          break;
        case 229:
          verticalSync(true, 8);
          break;
        case 232:
          verticalSync(false, 28);
          invertColorPhaseFlag = false;
          break;
        case 244:
          displayBlankingFlags = displayBlankingFlags & 0x01;
          break;
        }
      }
      if ((tedRegisters[0x06] & uint8_t(0x10)) != uint8_t(0)) {
        if (savedVideoLine == 0) {
          renderWindow = true;
          dmaEnabled = true;
        }
        else if ((savedVideoLine == 4 &&
                  (tedRegisters[0x06] & uint8_t(0x08)) != uint8_t(0)) ||
                 (savedVideoLine == 8 &&
                  (tedRegisters[0x06] & uint8_t(0x08)) == uint8_t(0)))
          displayWindow = true;
      }
      if ((savedVideoLine == 200 &&
           (tedRegisters[0x06] & uint8_t(0x08)) == uint8_t(0)) ||
          (savedVideoLine == 204 &&
           (tedRegisters[0x06] & uint8_t(0x08)) != uint8_t(0)))
        displayWindow = false;
      if (!(tedRegisterWriteMask & uint32_t(0x30000000)))
        video_line = savedVideoLine;
      break;
    }
    // check for video interrupt
    if (video_line == videoInterruptLine) {
      if (!prvVideoInterruptState) {
        prvVideoInterruptState = true;
        tedRegisters[0x09] |= uint8_t(0x02);
      }
    }
    else
      prvVideoInterruptState = false;
    // run CPU
    tedRegisterWriteMask = 0U;
    if (dmaCycleCounter < 7 && !singleClockModeFlags) {
      if (((tedRegisters[0x09] & tedRegisters[0x0A]) & uint8_t(0x5E))
          != uint8_t(0))
        M7501::interruptRequest();
      M7501::run(cpu_clock_multiplier);
    }
    // calculate video output
    {
      bool    tmpFlag = videoShiftRegisterEnabled;
      if (line_buf_pos < 533) {
        if (line_buf_pos >= 0) {
          uint8_t *bufp = &(line_buf[line_buf_pos + 4]);
          if (displayBlankingFlags) {
            bufp[0] = uint8_t(0x00);
            bufp[1] = uint8_t(0x00);
            bufp[2] = uint8_t(0x00);
            bufp[3] = uint8_t(0x00);
          }
          else if (!displayActive) {
            uint8_t c = tedRegisters[0x19];
            bufp[0] = (!(tedRegisterWriteMask & 0x02000000U) ?
                       c : uint8_t(0xFF));
            bufp[1] = c;
            bufp[2] = c;
            bufp[3] = c;
          }
          else {
            prv_render_func(*this, bufp, 4);
            tmpFlag = false;
          }
        }
        line_buf_pos += 9;
      }
      if (tmpFlag)
        render_invalid_mode(*this, (uint8_t *) 0, 4);
    }
    // delay video mode changes by one cycle
    prv_render_func = render_func;
    // delay horizontal scroll changes
    horiz_scroll = tedRegisters[0x07] & 0x07;
    // bitmap fetches and rendering display are done on even cycle counts
    if (videoShiftRegisterEnabled) {
      currentAttribute = nextAttribute;
      currentCharacter = nextCharacter;
      currentBitmap = nextBitmap;
      nextBitmap = 0x00;
      cursorFlag = nextCursorFlag;
    }
    if (renderingDisplay) {
      nextAttribute = attr_buf[character_column];
      nextCharacter = char_buf[character_column];
      // read bitmap data from memory
      if (!bitmapAddressDisableFlags) {
        uint16_t  addr_ = uint16_t(character_line);
        if (!(tedRegisters[0x06] & uint8_t(0x80))) {
          if (bitmapMode)
            addr_ |= uint16_t(bitmap_base_addr
                              | (character_position << 3));
          else
            addr_ |= uint16_t(charset_base_addr
                              | (int(nextCharacter & characterMask) << 3));
        }
        else {
          // IC test mode (FF06 bit 7 set)
          int     tmp = (int(attr_buf_tmp[character_column]) << 3) | 0xF800;
          if (bitmapMode)
            addr_ |= uint16_t(bitmap_base_addr
                              | ((character_position << 3) & tmp));
          else
            addr_ |= uint16_t(tmp);
        }
        if (addr_ >= uint16_t(0x8000) || !(tedBitmapReadMap & 0x80U)) {
          memoryReadMap = tedBitmapReadMap;
          (void) readMemory(addr_);
        }
      }
      else if (singleClockModeFlags) {
        memoryReadMap = tedDMAReadMap;  // not sure if this is correct
        (void) readMemory(0xFFFF);
      }
      memoryReadMap = cpuMemoryReadMap;
      nextBitmap = dataBusState;
      nextCursorFlag = (character_position == cursor_position);
      attr_buf[character_column] = attr_buf_tmp[character_column];
      if (!bitmapAddressDisableFlags)
        character_position = (character_position + 1) & 0x03FF;
      else
        character_position = 0x03FF;
    }
    character_column = (character_column + 1) & 0x3F;
    // update timer 2 and 3 on even cycle count (886 kHz rate)
    if (timer2_run) {
      if (!(tedRegisterWriteMask & 0x00000008U))
        timer2_state = (timer2_state - 1) & 0xFFFF;
    }
    if (timer3_run)
      timer3_state = (timer3_state - 1) & 0xFFFF;
    // update sound generators on every 8th cycle (221 kHz)
    if (!cycle_count) {
      int     sound_output = 0;
      int     sound_volume;
      uint8_t sound_register = tedRegisters[0x11];
      sound_volume = int(sound_register & uint8_t(0x0F)) << 10;
      sound_volume = (sound_volume < 8192 ? sound_volume : 8192);
      if (sound_register & uint8_t(0x80)) {
        // DAC mode
        sound_channel_1_cnt = sound_channel_1_reload;
        sound_channel_2_cnt = sound_channel_2_reload;
        sound_channel_1_state = uint8_t(1);
        sound_channel_2_state = uint8_t(1);
        sound_channel_2_noise_state = uint8_t(0xFF);
        sound_channel_2_noise_output = uint8_t(1);
      }
      else {
        sound_channel_1_cnt = (sound_channel_1_cnt + 1) & 1023;
        if (sound_channel_1_cnt == 1023) {
          // channel 1 square wave
          sound_channel_1_cnt = sound_channel_1_reload;
          if (sound_channel_1_reload != 1022) {
            sound_channel_1_state ^= uint8_t(1);
            sound_channel_1_state &= uint8_t(1);
          }
        }
        sound_channel_2_cnt = (sound_channel_2_cnt + 1) & 1023;
        if (sound_channel_2_cnt == 1023) {
          // channel 2 square wave
          sound_channel_2_cnt = sound_channel_2_reload;
          if (sound_channel_2_reload != 1022) {
            sound_channel_2_state ^= uint8_t(1);
            sound_channel_2_state &= uint8_t(1);
            // channel 2 noise, 8 bit polycnt (10110011)
            uint8_t tmp = sound_channel_2_noise_state & uint8_t(0xB3);
            tmp = tmp ^ (tmp >> 1);
            tmp = tmp ^ (tmp >> 2);
            tmp = (tmp ^ (tmp >> 4)) & uint8_t(1);
            sound_channel_2_noise_output ^= tmp;
            sound_channel_2_noise_output &= uint8_t(1);
            sound_channel_2_noise_state <<= 1;
            sound_channel_2_noise_state |= sound_channel_2_noise_output;
          }
        }
      }
      // mix sound outputs
      if (sound_register & uint8_t(0x10))
        sound_output += (sound_channel_1_state ? sound_volume : 0);
      if (sound_register & uint8_t(0x20))
        sound_output += (sound_channel_2_state ? sound_volume : 0);
      else if (sound_register & uint8_t(0x40))
        sound_output += (sound_channel_2_noise_output ? 0 : sound_volume);
      // send sound output signal (sample rate = 221 kHz)
      playSample(int16_t(sound_output));
    }
    // update horizontal position
    if (!ted_disabled)
      video_column |= uint8_t(0x01);
    cycle_count = (cycle_count + 1) & 3;
  }

}       // namespace Plus4

