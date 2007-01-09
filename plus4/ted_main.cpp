
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

  void TED7360::run(int nCycles)
  {
    for (int i = 0; i < nCycles; i++, cycle_count++) {
      if (ted_disabled) {
        if (cycle_count & 1UL)
          M7501::run(cpu_clock_multiplier);
        if (!(cycle_count & 7UL))
          playSample(0);
        continue;
      }
      bool  evenCycle = !(video_column & uint8_t(0x01));
      switch (video_column) {
      case 1:                           // start display (38 column mode)
        if (displayWindow &&
            (memory_ram[0xFF07] & uint8_t(0x08)) == uint8_t(0)) {
          displayActive = true;
          pixelBufReadPos = (pixelBufWritePos + 40) & 0x38;
        }
        break;
      case 75:                          // DRAM refresh start
        singleClockMode = true;
        M7501::setIsCPURunning(true);
        if (bitmapFetchWindow && savedCharacterLine == uint8_t(7))
          character_position_reload = (character_position_reload + 40) & 0x03FF;
        break;
      case 76:                          // initialize character position
        character_position = character_position_reload;
                                        // update DMA read position
        if (dmaWindow && character_line == 6)
          dma_position = (dma_position + 40) & 0x03FF;
        break;
      case 77:                          // end of display (38 column mode)
        if ((memory_ram[0xFF07] & uint8_t(0x08)) == uint8_t(0))
          displayActive = false;
        break;
      case 79:                          // end of display (40 column mode)
        if ((memory_ram[0xFF07] & uint8_t(0x08)) != uint8_t(0))
          displayActive = false;
        break;
      case 85:                          // DRAM refresh end
        singleClockMode = false;
        drawLine(&(line_buf[0]), 414);
        break;
      case 88:                          // increment flash counter
        if (video_line == 205) {
          memory_ram[0xFF1F] =
              (memory_ram[0xFF1F] & uint8_t(0x7F)) + uint8_t(0x08);
          if (memory_ram[0xFF1F] & uint8_t(0x80))
            flash_state = !flash_state;
        }
        break;
      case 89:                          // horizontal blanking start
        horizontalBlanking = true;
        break;
      case 98:                          // increment line number
        if ((memory_ram[0xFF07] & uint8_t(0x40)) == uint8_t(0)) {       // PAL
          video_line = (video_line != 311 ? ((video_line + 1) & 0x01FF) : 0);
          switch (video_line) {
          case 251:
            verticalBlanking = true;
            break;
          case 254:
            verticalSync(true, 8);
            break;
          case 257:
            verticalSync(false, 28);
            break;
          case 269:
            verticalBlanking = false;
            break;
          }
        }
        else {                                                          // NTSC
          video_line = (video_line != 261 ? ((video_line + 1) & 0x01FF) : 0);
          switch (video_line) {
          case 226:
            verticalBlanking = true;
            break;
          case 229:
            verticalSync(true, 8);
            break;
          case 232:
            verticalSync(false, 28);
            break;
          case 244:
            verticalBlanking = false;
            break;
          }
        }
        if ((memory_ram[0xFF06] & uint8_t(0x10)) != uint8_t(0)) {
          if (video_line == 0)
            renderWindow = true;
          else if ((video_line == 4 &&
                    (memory_ram[0xFF06] & uint8_t(0x08)) != uint8_t(0)) ||
                   (video_line == 8 &&
                    (memory_ram[0xFF06] & uint8_t(0x08)) == uint8_t(0)))
            displayWindow = true;
        }
        if ((video_line == 200 &&
             (memory_ram[0xFF06] & uint8_t(0x08)) == uint8_t(0)) ||
            (video_line == 204 &&
             (memory_ram[0xFF06] & uint8_t(0x08)) != uint8_t(0)))
          displayWindow = false;
        if (video_line == 0)            // end of screen
          character_position_reload = 0x0000;
        break;
      case 99:
        if (video_line == 0)
          dma_position = 0x0000;
                                        // increment character sub-line
        if (dmaWindow || attributeDMACnt != uint8_t(0)) {
          character_line = (character_line + 1) & 7;
          savedCharacterLine = uint8_t(character_line);
          bitmapFetchWindow = true;
        }
        if (video_line == 204) {        // end of display
          renderWindow = false;
          dmaWindow = false;
          bitmapFetchWindow = false;
        }
        break;
      case 101:                         // external fetch single clock start
        if (renderWindow) {
          singleClockMode = true;
          // check if DMA should be requested
          uint8_t tmp = (uint8_t(video_line)
                         + (memory_ram[0xFF06] ^ uint8_t(0xFF)))
                        & uint8_t(0x07);
          if (tmp == uint8_t(0)) {
            if (dmaWindow)
              characterDMACnt = 1;
          }
          else if (tmp == uint8_t(7)) {
            if (video_line != 203)
              attributeDMACnt = 1;
          }
          if (video_line == 0) {        // initialize character sub-line
            character_line = 7;
            savedCharacterLine = uint8_t(7);
          }
        }
        break;
      case 107:                         // horizontal blanking end
        horizontalBlanking = false;
        line_buf_pos = 0;
        break;
      case 109:                         // start DMA and/or bitmap fetch
        if (renderWindow) {
          renderingDisplay = true;
          singleClockMode = true;
          character_column = 0;
        }
        break;
      case 113:                         // start display (40 column mode)
        if (displayWindow &&
            (memory_ram[0xFF07] & uint8_t(0x08)) != uint8_t(0)) {
          displayActive = true;
          pixelBufReadPos = (pixelBufWritePos + 40) & 0x38;
        }
        break;
      }
      // initialize DMA if requested
      if ((attributeDMACnt | characterDMACnt) != uint8_t(0) && !evenCycle) {
        if (attributeDMACnt) {
          if (attributeDMACnt == 1) {
            singleClockMode = true;
          }
          else if (attributeDMACnt == 2) {
            M7501::setIsCPURunning(false);
          }
          else if (attributeDMACnt == 5) {
            dmaWindow = true;
            if (video_column >= 109 || video_column < 75) {
              M7501::haltFlag = true;
            }
            else {
              M7501::setIsCPURunning(true);
              attributeDMACnt = uint8_t(0);
              attributeDMACnt--;
            }
          }
          attributeDMACnt++;
        }
        else {
          if (characterDMACnt == 1) {
            singleClockMode = true;
          }
          else if (characterDMACnt == 2) {
            M7501::setIsCPURunning(false);
          }
          else if (characterDMACnt == 5) {
            M7501::haltFlag = true;
            for (int j = 0; j < 40; j++)
              attr_buf[j] = attr_buf_tmp[j];
          }
          characterDMACnt++;
        }
      }
      // check for video interrupt
      if (video_line == videoInterruptLine) {
        if (!prvVideoInterruptState) {
          prvVideoInterruptState = true;
          memory_ram[0xFF09] |= uint8_t(0x02);
        }
      }
      else
        prvVideoInterruptState = false;
      // run CPU and render display
      if (!M7501::haltFlag) {
        if (!evenCycle || !(singleClockMode || !doubleClockModeEnabled)) {
          if (((memory_ram[0xFF09] & memory_ram[0xFF0A]) & uint8_t(0x5E))
              != uint8_t(0))
            M7501::interruptRequest();
          M7501::run(cpu_clock_multiplier);
        }
      }
      else if (!evenCycle) {
        if (attributeDMACnt >= 5) {
          dataBusState =
              readMemory(uint16_t(((dma_position + character_column) & 0x03FF)
                                  | (attr_base_addr & 0xF800)));
          attr_buf_tmp[character_column] = dataBusState;
        }
        else if (characterDMACnt >= 5) {
          dataBusState =
              readMemory(uint16_t(((dma_position + character_column) & 0x03FF)
                                  | (attr_base_addr | 0x0400)));
          char_buf[character_column] = dataBusState;
        }
      }
      if (evenCycle) {
        if (renderingDisplay) {
          currentCharacter = char_buf[character_column];
          currentAttribute = attr_buf[character_column];
          // read bitmap data from memory
          if (bitmapFetchWindow) {
            uint16_t  addr_ = uint16_t(character_line);
            if (bitmapMode)
              addr_ |= uint16_t(bitmap_base_addr
                                | (character_position << 3));
            else
              addr_ |= uint16_t(charset_base_addr
                                | (int(currentCharacter
                                       & characterMask) << 3));
            if (rom_enabled_for_video) {
              if (addr_ >= 0x8000) {
                if (addr_ < 0xC000)
                  dataBusState = memory_rom[rom_bank_low][addr_ & 0x7FFF];
                else
                  dataBusState = memory_rom[rom_bank_high][addr_ & 0x7FFF];
              }
            }
            else
              dataBusState = memory_ram[addr_];
          }
          else
            dataBusState = readMemory(0xFFFF);
          currentBitmap = dataBusState;
          render_func(this);
          pixelBufWritePos = (pixelBufWritePos + 8) & 0x38;
          if (++character_column >= 40) {
            character_column = 39;
            attributeDMACnt = (attributeDMACnt < 5 ? attributeDMACnt : 0);
            characterDMACnt = 0;
            currentBitmap = uint8_t(0x00);
            render_func(this);
            pixelBufWritePos = (pixelBufWritePos + 8) & 0x38;
            renderingDisplay = false;
          }
          if (bitmapFetchWindow)
            character_position = (character_position + 1) & 0x03FF;
          else
            character_position = 0x03FF;
        }
      }
      if (line_buf_pos < 365) {
        unsigned int  ndx = (unsigned int) line_buf_pos >> 3;
        ndx = ((ndx << 3) + ndx) + ((unsigned int) line_buf_pos & 7U) + 1U;
        line_buf_pos += 4;
        uint8_t *bufp = &(line_buf[ndx]);
        if (horizontalBlanking || verticalBlanking) {
          *(bufp++) = uint8_t(0x00);
          *(bufp++) = uint8_t(0x00);
          *(bufp++) = uint8_t(0x00);
          *(bufp++) = uint8_t(0x00);
        }
        else if (!displayActive) {
          *(bufp++) = oldColors[4];
          oldColors = newColors;
          *(bufp++) = oldColors[4];
          *(bufp++) = oldColors[4];
          *(bufp++) = oldColors[4];
        }
        else {
          size_t  ndx2 = size_t(pixelBufReadPos) + size_t(8 - horiz_scroll);
          pixelBufReadPos = (pixelBufReadPos + uint8_t(4)) & uint8_t(0x3C);
          uint8_t c = pixel_buf[ndx2 & 0x3F];
          ndx2++;
          *(bufp++) = (c < uint8_t(0x80) ? c : oldColors[c & uint8_t(0x07)]);
          oldColors = newColors;
          c = pixel_buf[ndx2 & 0x3F];
          ndx2++;
          *(bufp++) = (c < uint8_t(0x80) ? c : oldColors[c & uint8_t(0x07)]);
          c = pixel_buf[ndx2 & 0x3F];
          ndx2++;
          *(bufp++) = (c < uint8_t(0x80) ? c : oldColors[c & uint8_t(0x07)]);
          c = pixel_buf[ndx2 & 0x3F];
          *(bufp++) = (c < uint8_t(0x80) ? c : oldColors[c & uint8_t(0x07)]);
        }
      }
      // update timers on even cycle count (884 kHz rate)
      if (!(cycle_count & 1UL)) {
        if (timer1_run) {
          timer1_state = (timer1_state - 1) & 0xFFFF;
          if (!timer1_state) {
            // reload timer
            timer1_state = timer1_reload_value;
            // trigger interrupt if enabled
            memory_ram[0xFF09] |= uint8_t(0x08);
          }
        }
        if (timer2_run) {
          timer2_state = (timer2_state - 1) & 0xFFFF;
          if (!timer2_state) {
            // trigger interrupt if enabled
            memory_ram[0xFF09] |= uint8_t(0x10);
          }
        }
        if (timer3_run) {
          timer3_state = (timer3_state - 1) & 0xFFFF;
          if (!timer3_state) {
            // trigger interrupt if enabled
            memory_ram[0xFF09] |= uint8_t(0x40);
          }
        }
      }
      // update sound generators on every 8th cycle (221 kHz)
      if (!(cycle_count & 7UL)) {
        int     sound_output = 0;
        int     sound_volume;
        uint8_t sound_register = memory_ram[0xFF11];
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
      video_column = (video_column == 113 ? 0 : ((video_column + 1) & 0x7F));
    }
  }

}       // namespace Plus4

