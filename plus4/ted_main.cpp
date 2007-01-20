
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

  void TED7360::run(int nCycles)
  {
    for (int i = 0; i < nCycles; i++, cycle_count++) {
      bool  evenCycle = !(cycle_count & 1UL);
      if (ted_disabled) {
        tedRegisterWriteMask = 0U;
        if (!evenCycle) {
          M7501::run(cpu_clock_multiplier);
          video_column &= uint8_t(0x7E);
        }
        else {
          if (!(cycle_count & 7UL))
            playSample(0);
          video_column |= uint8_t(0x01);
        }
        continue;
      }
      switch (video_column) {
      case 1:                           // start display (38 column mode)
        if (displayWindow &&
            (tedRegisters[0x07] & uint8_t(0x08)) == uint8_t(0)) {
          displayActive = true;
          pixelBufReadPos = (pixelBufWritePos + 40) & 0x38;
        }
        break;
      case 75:                          // DRAM refresh start
        singleClockMode = true;
        dmaCycleCounter = 0;
        M7501::setIsCPURunning(true);
        if (bitmapFetchWindow && savedCharacterLine == uint8_t(7))
          character_position_reload = (character_position_reload + 40) & 0x03FF;
        break;
      case 76:                          // initialize character position
        character_position = character_position_reload;
                                        // update DMA read position
        if (dmaWindow && character_line == 6)
          dma_position = (dma_position & 0x0400)
                         | ((dma_position + 40) & 0x03FF);
        break;
      case 77:                          // end of display (38 column mode)
        if ((tedRegisters[0x07] & uint8_t(0x08)) == uint8_t(0))
          displayActive = false;
        break;
      case 79:                          // end of display (40 column mode)
        if ((tedRegisters[0x07] & uint8_t(0x08)) != uint8_t(0))
          displayActive = false;
        break;
      case 85:                          // DRAM refresh end
        singleClockMode = false;
        break;
      case 87:                          // horizontal blanking start
        horizontalBlanking = true;
        drawLine(&(line_buf[0]), 432);
        break;
      case 88:                          // increment flash counter
        if (video_line == 205) {
          tedRegisters[0x1F] =
              (tedRegisters[0x1F] & uint8_t(0x7F)) + uint8_t(0x08);
          if (tedRegisters[0x1F] & uint8_t(0x80))
            flash_state = !flash_state;
        }
        break;
      case 95:
        if (renderWindow) {
          // if done attribute DMA in this line, continue with character
          // DMA in next one
          dmaFlags = (dmaFlags & 1) << 1;
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
        break;
      case 98:
        if ((tedRegisters[0x07] & uint8_t(0x40)) == uint8_t(0)) {   // PAL
          switch (savedVideoLine) {
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
        else {                                                      // NTSC
          switch (savedVideoLine) {
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
        if ((tedRegisters[0x06] & uint8_t(0x10)) != uint8_t(0)) {
          if (savedVideoLine == 0)
            renderWindow = true;
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
        if (savedVideoLine == 0)        // end of screen
          character_position_reload = 0x0000;
        if (!(tedRegisterWriteMask & uint32_t(0x30000000)))
          video_line = savedVideoLine;
        break;
      case 99:
        if (savedVideoLine == 0) {
          dma_position = 0x0000;
          dmaFlags = 0;
        }
        if (dmaWindow) {                // increment character sub-line
          character_line = (character_line + 1) & 7;
          savedCharacterLine = uint8_t(character_line);
        }
        if (savedVideoLine == 204) {    // end of display
          renderWindow = false;
          dmaWindow = false;
          bitmapFetchWindow = false;
        }
        else if (renderWindow) {
          // check if DMA should be requested
          uint8_t tmp =
              (uint8_t(savedVideoLine) + (tedRegisters[0x06] ^ uint8_t(0x07)))
              & uint8_t(0x07);
          if (tmp == uint8_t(7)) {
            if (savedVideoLine != 203) {
              // start a new DMA at character line 7
              dmaWindow = true;
              dmaCycleCounter = 1;
              dmaFlags = dmaFlags | 1;
              dma_position = dma_position & 0x03FF;
            }
          }
          else if (dmaFlags & 2) {
            bitmapFetchWindow = true;
            // done reading attribute data in previous line,
            // now continue DMA to get character data
            dmaCycleCounter = 1;
            dma_position = dma_position | 0x0400;
          }
        }
        break;
      case 101:                         // external fetch single clock start
        if (renderWindow) {
          singleClockMode = true;
          if (savedVideoLine == 0) {    // initialize character sub-line
            character_line = 7;
            savedCharacterLine = uint8_t(7);
          }
        }
        break;
      case 105:                         // horizontal blanking end
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
            (tedRegisters[0x07] & uint8_t(0x08)) != uint8_t(0)) {
          displayActive = true;
          pixelBufReadPos = (pixelBufWritePos + 40) & 0x38;
        }
        break;
      }
      // initialize DMA if requested
      if (dmaCycleCounter != uint8_t(0) && !evenCycle) {
        dmaCycleCounter++;
        switch (dmaCycleCounter) {
        case 3:
          singleClockMode = true;
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
        if (!evenCycle || !(singleClockMode || !doubleClockModeEnabled)) {
          if (((tedRegisters[0x09] & tedRegisters[0x0A]) & uint8_t(0x5E))
              != uint8_t(0))
            M7501::interruptRequest();
          M7501::run(cpu_clock_multiplier);
        }
      }
      if (!evenCycle) {
        // perform DMA fetches on odd cycle counts
        if (renderingDisplay && dmaCycleCounter >= 4) {
          if (dmaCycleCounter >= 7) {
            memoryReadMap = tedDMAReadMap;
            (void) readMemory(uint16_t((attr_base_addr
                                        | (dma_position & 0x0400))
                                       | ((dma_position + character_column)
                                          & 0x03FF)));
            memoryReadMap = cpuMemoryReadMap;
          }
          if (dmaFlags & 1)
            attr_buf_tmp[character_column] = dataBusState;
          if (dmaFlags & 2)
            char_buf[character_column] = dataBusState;
        }
      }
      else {
        // bitmap fetches and rendering display are done on even cycle counts
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
            if (addr_ >= uint16_t(0x8000) || !(tedBitmapReadMap & 0x80U)) {
              memoryReadMap = tedBitmapReadMap;
              (void) readMemory(addr_);
            }
          }
          else {
            memoryReadMap = tedDMAReadMap;  // not sure if this is correct
            (void) readMemory(0xFFFF);
          }
          currentBitmap = dataBusState;
          memoryReadMap = cpuMemoryReadMap;
          render_func(this);
          pixelBufWritePos = (pixelBufWritePos + 8) & 0x38;
          if (++character_column >= 40) {
            character_column = 39;
            if (dmaCycleCounter) {
              dmaCycleCounter = 0;
              if (dmaFlags & 1) {
                for (int j = 0; j < 40; j++)
                  attr_buf[j] = attr_buf_tmp[j];
              }
            }
            M7501::setIsCPURunning(true);
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
      if (line_buf_pos < 381) {
        unsigned int  ndx = (unsigned int) line_buf_pos >> 3;
        ndx = ((ndx << 3) + ndx) + ((unsigned int) line_buf_pos & 7U) + 1U;
        line_buf_pos += 4;
        uint8_t *bufp = &(line_buf[ndx]);
        if (horizontalBlanking || verticalBlanking) {
          bufp[0] = uint8_t(0x00);
          bufp[1] = uint8_t(0x00);
          bufp[2] = uint8_t(0x00);
          bufp[3] = uint8_t(0x00);
        }
        else if (!displayActive) {
          uint8_t c = tedRegisters[0x19];
          if (tedRegisterWriteMask & 0x02000000U) {
            c = c | uint8_t(0xF0);
            c = (c != uint8_t(0xF0) ? c : uint8_t(0xF1));
          }
          bufp[0] = c;
          c = tedRegisters[0x19];
          bufp[1] = c;
          bufp[2] = c;
          bufp[3] = c;
        }
        else {
          size_t  ndx2 = size_t(pixelBufReadPos) + size_t(8 - horiz_scroll);
          pixelBufReadPos = (pixelBufReadPos + uint8_t(4)) & uint8_t(0x3C);
          uint8_t c = pixel_buf[ndx2 & 0x3F];
          if (c >= uint8_t(0x80)) {
            uint32_t  mask_ = uint32_t(0x00200000) << (c & uint8_t(0x1F));
            c = tedRegisters[c - uint8_t(0x6B)];
            if (tedRegisterWriteMask & mask_) {
              c = c | uint8_t(0xF0);
              c = (c != uint8_t(0xF0) ? c : uint8_t(0xF1));
            }
          }
          bufp[0] = c;
          c = pixel_buf[(++ndx2) & 0x3F];
          bufp[1] = (c < uint8_t(0x80) ? c : tedRegisters[c - uint8_t(0x6B)]);
          c = pixel_buf[(++ndx2) & 0x3F];
          bufp[2] = (c < uint8_t(0x80) ? c : tedRegisters[c - uint8_t(0x6B)]);
          c = pixel_buf[(++ndx2) & 0x3F];
          bufp[3] = (c < uint8_t(0x80) ? c : tedRegisters[c - uint8_t(0x6B)]);
        }
      }
      if (evenCycle) {
        // update timer 2 and 3 on even cycle count (884 kHz rate)
        if (timer2_run) {
          if (!(tedRegisterWriteMask & 0x00000008U))
            timer2_state = (timer2_state - 1) & 0xFFFF;
        }
        if (timer3_run)
          timer3_state = (timer3_state - 1) & 0xFFFF;
        // update sound generators on every 8th cycle (221 kHz)
        if (!(cycle_count & 7UL)) {
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
        video_column |= uint8_t(0x01);
      }
      else {
        // check timer interrupts
        if (!timer1_state) {
          if (timer1_run) {
            tedRegisters[0x09] |= uint8_t(0x08);
            // reload timer
            timer1_state = timer1_reload_value;
          }
        }
        if (!timer2_state) {
          if (timer2_run)
            tedRegisters[0x09] |= uint8_t(0x10);
        }
        if (!timer3_state) {
          if (timer3_run)
            tedRegisters[0x09] |= uint8_t(0x40);
        }
        // update timer 1 on odd cycle count (884 kHz rate)
        if (timer1_run)
          timer1_state = (timer1_state - 1) & 0xFFFF;
        // update horizontal position
        if (!(tedRegisterWriteMask & 0x40000000U))
          video_column = (video_column != 113 ? (video_column + 1) : 0);
        video_column &= uint8_t(0x7E);
      }
    }
  }

}       // namespace Plus4

