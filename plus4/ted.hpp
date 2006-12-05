
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

#include "ep128.hpp"
#include "fileio.hpp"
#include "cpu.hpp"

namespace Plus4 {

  class TED7360 : public M7501 {
   private:
    typedef uint8_t (Plus4::TED7360::*readMemoryFunc)(uint16_t) const;
    typedef void    (Plus4::TED7360::*writeMemoryFunc)(uint16_t, uint8_t);
    typedef void    (Plus4::TED7360::*renderFuncType)();
    typedef void    (Plus4::TED7360::*displayFuncType)();
    // memory images (64K RAM, 4*32K ROM)
    uint8_t     memory_ram[65536];
    uint8_t     memory_rom[4][32768];
    // table of functions for reading memory when ROM is enabled
    readMemoryFunc  read_memory_rom[65536];
    // table of functions for reading memory when ROM is disabled
    readMemoryFunc  read_memory_ram[65536];
    // table of functions for writing memory
    writeMemoryFunc write_memory[65536];
    // table of functions for updating display and running the CPU,
    // indexed by display_mode and video_column
    displayFuncType display_func_table[32][114];
    // memory and register read functions
    uint8_t     read_ram_(uint16_t addr) const;
    uint8_t     read_rom_low(uint16_t addr) const;
    uint8_t     read_rom_high(uint16_t addr) const;
    uint8_t     read_rom_FCxx(uint16_t addr) const;
    uint8_t     read_register_0000(uint16_t addr) const;
    uint8_t     read_register_0001(uint16_t addr) const;
    uint8_t     read_register_unimplemented(uint16_t addr) const;
    uint8_t     read_register_FD10(uint16_t addr) const;
    uint8_t     read_register_FD30(uint16_t addr) const;
    uint8_t     read_register_FF00(uint16_t addr) const;
    uint8_t     read_register_FF01(uint16_t addr) const;
    uint8_t     read_register_FF02(uint16_t addr) const;
    uint8_t     read_register_FF03(uint16_t addr) const;
    uint8_t     read_register_FF04(uint16_t addr) const;
    uint8_t     read_register_FF05(uint16_t addr) const;
    uint8_t     read_register_FF06(uint16_t addr) const;
    uint8_t     read_register_FF09(uint16_t addr) const;
    uint8_t     read_register_FF0A(uint16_t addr) const;
    uint8_t     read_register_FF0C(uint16_t addr) const;
    uint8_t     read_register_FF10(uint16_t addr) const;
    uint8_t     read_register_FF12(uint16_t addr) const;
    uint8_t     read_register_FF13(uint16_t addr) const;
    uint8_t     read_register_FF14(uint16_t addr) const;
    uint8_t     read_register_FF15_to_FF19(uint16_t addr) const;
    uint8_t     read_register_FF1A(uint16_t addr) const;
    uint8_t     read_register_FF1B(uint16_t addr) const;
    uint8_t     read_register_FF1C(uint16_t addr) const;
    uint8_t     read_register_FF1D(uint16_t addr) const;
    uint8_t     read_register_FF1E(uint16_t addr) const;
    uint8_t     read_register_FF1F(uint16_t addr) const;
    uint8_t     read_register_FF3E(uint16_t addr) const;
    uint8_t     read_register_FF3F(uint16_t addr) const;
    // memory and register write functions
    void        write_ram_(uint16_t addr, uint8_t value);
    void        write_register_0000(uint16_t addr, uint8_t value);
    void        write_register_0001(uint16_t addr, uint8_t value);
    void        write_register_FD1x(uint16_t addr, uint8_t value);
    void        write_register_FD3x(uint16_t addr, uint8_t value);
    void        write_register_FDDx(uint16_t addr, uint8_t value);
    void        write_register_FF00(uint16_t addr, uint8_t value);
    void        write_register_FF01(uint16_t addr, uint8_t value);
    void        write_register_FF02(uint16_t addr, uint8_t value);
    void        write_register_FF03(uint16_t addr, uint8_t value);
    void        write_register_FF04(uint16_t addr, uint8_t value);
    void        write_register_FF05(uint16_t addr, uint8_t value);
    void        write_register_FF06(uint16_t addr, uint8_t value);
    void        write_register_FF07(uint16_t addr, uint8_t value);
    void        write_register_FF08(uint16_t addr, uint8_t value);
    void        write_register_FF09(uint16_t addr, uint8_t value);
    void        write_register_FF0C(uint16_t addr, uint8_t value);
    void        write_register_FF0D(uint16_t addr, uint8_t value);
    void        write_register_FF0E(uint16_t addr, uint8_t value);
    void        write_register_FF0F(uint16_t addr, uint8_t value);
    void        write_register_FF10(uint16_t addr, uint8_t value);
    void        write_register_FF12(uint16_t addr, uint8_t value);
    void        write_register_FF13(uint16_t addr, uint8_t value);
    void        write_register_FF14(uint16_t addr, uint8_t value);
    void        write_register_FF1A(uint16_t addr, uint8_t value);
    void        write_register_FF1B(uint16_t addr, uint8_t value);
    void        write_register_FF1C(uint16_t addr, uint8_t value);
    void        write_register_FF1D(uint16_t addr, uint8_t value);
    void        write_register_FF1E(uint16_t addr, uint8_t value);
    void        write_register_FF1F(uint16_t addr, uint8_t value);
    void        write_register_FF3E(uint16_t addr, uint8_t value);
    void        write_register_FF3F(uint16_t addr, uint8_t value);
    // functions for updating the display and running the CPU
    void        run_blank_CPU();
    void        run_blank_no_CPU();
    void        run_border_CPU();
    void        run_border_no_CPU();
    void        run_border_DMA_0();
    void        run_border_DMA_7();
    void        run_border_render_init();
    void        run_border_render();
    void        run_display_DMA_0();
    void        run_display_DMA_7();
    void        run_display_render();
    void        run_display_CPU();
    void        run_display_no_CPU();
    // render functions (called by run_*() above)
    void        render_BMM_hires();
    void        render_BMM_multicolor();
    void        render_char_128();
    void        render_char_256();
    void        render_char_ECM();
    void        render_char_MCM_128();
    void        render_char_MCM_256();
    void        render_invalid_mode();
    void        render_init();
    // currently selected render function (depending on bits of FF06 and FF07)
    renderFuncType  render_func;
    // -----------------------------------------------------------------
    void set_video_line(int lineNum, bool isHorizontalRefresh);
    // this flag is true if ROM is enabled (FF3E was written)
    bool        rom_enabled;
    // this flag is true if ROM is enabled for video data (FF12 bit 2 is set)
    bool        rom_enabled_for_video;
    // ROM bank selected for 8000..BFFF (0 to 3)
    unsigned char rom_bank_low;
    // ROM bank selected for C000..FFFF (0 to 3)
    unsigned char rom_bank_high;
    // TED cycle counter
    unsigned long cycle_count;
    // CPU clock multiplier
    int         cpu_clock_multiplier;
    // display mode, depending on current video line and TED registers
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
    int         display_mode;
    // current video column (0 to 113, = (FF1E) / 2)
    int         video_column;
    // delay video interrupts by 8 cycles
    unsigned int  video_interrupt_shift_register;
    // current video line (0 to 311, = (FF1D, FF1C)
    int         video_line;
    // character sub-line (0 to 7, bits 0..2 of FF1F)
    int         character_line;
    // character line offset (FF1A, FF1B)
    int         character_position;
    int         character_position_reload;
    // base address for attribute data (FF14 bits 3..7)
    int         attr_base_addr;
    // base address for bitmap data (FF12 bits 3..5)
    int         bitmap_base_addr;
    // base address for character set (FF13 bits 2..7)
    int         charset_base_addr;
    // horizontal scroll (0 to 7)
    int         horiz_scroll;
    // cursor position (FF0C, FF0D)
    int         cursor_position;
    // FF07 bit 5
    bool        ted_disabled;
    // flash state for cursor etc., updated on every 16th video frame
    bool        flash_state;
    // display state flags, set depending on video line
    bool        display_enabled;
    bool        render_enabled;
    bool        DMA_enabled;
    bool        vertical_blanking;
    // timers (FF00 to FF05)
    bool        timer1_run;
    bool        timer2_run;
    bool        timer3_run;
    int         timer1_state;
    int         timer1_reload_value;
    int         timer2_state;
    int         timer3_state;
    // sound generators
    int         sound_channel_1_cnt;
    int         sound_channel_1_reload;
    int         sound_channel_2_cnt;
    int         sound_channel_2_reload;
    uint8_t     sound_channel_1_state;
    uint8_t     sound_channel_2_state;
    uint8_t     sound_channel_2_noise_state;
    uint8_t     sound_channel_2_noise_output;
    // video buffers
    uint8_t     attr_buf[40];
    uint8_t     char_buf_tmp[40];
    uint8_t     char_buf[40];
    uint8_t     pixel_buf[16];
    uint8_t     line_buf[456];
    // keyboard matrix
    int         keyboard_row_select_mask;
    uint8_t     keyboard_matrix[16];
    // external ports
    uint8_t     user_port_state;
    bool        tape_motor_state;
    bool        tape_read_state;
    bool        tape_write_state;
    bool        tape_button_state;
    // -----------------------------------------------------------------
    // read a byte from video memory
    inline uint8_t readVideoMemory(uint16_t addr) const
    {
      if (video_line > int(memory_ram[0xFF06] & 0x07)) {
        if (rom_enabled_for_video)
          return (this->*(read_memory_rom[addr]))(addr);
        return (this->*(read_memory_ram[addr]))(addr);
      }
      else
        return 0xFC;
    }
    inline void runCPU()
    {
      int i = cpu_clock_multiplier;
      do {
        if (memory_ram[0xFF09] & (uint8_t) 0x5E)
          M7501::interruptRequest();
        M7501::runOneCycle();
      } while (--i);
    }
   protected:
    virtual void playSample(int16_t sampleValue)
    {
      (void) sampleValue;
    }
    virtual void drawLine(const uint8_t *buf, int nPixels)
    {
      (void) buf;
      (void) nPixels;
    }
    virtual void verticalSync()
    {
    }
   public:
    TED7360();
    virtual ~TED7360();
    void loadROM(int bankNum, int offs, int cnt, const uint8_t *buf);
    void run(int nCycles);
    void reset(bool cold_reset);
    void setCPUClockMultiplier(int clk);
    void setKeyState(int keyNum, bool isPressed);
    // Returns memory segment at page 'n' (0 to 3). Segments 0x00 to 0x07 are
    // used for ROM, while segments 0xFC to 0xFF are RAM.
    uint8_t getMemoryPage(int n) const;
    uint8_t readMemory(uint16_t addr) const;
    // Read memory directly without paging. Valid address ranges are
    // 0x00000000 to 0x0001FFFF for ROM, and 0x003F0000 to 0x003FFFFF for RAM.
    uint8_t readMemoryRaw(uint32_t addr) const;
    void writeMemory(uint16_t addr, uint8_t value);
    inline void setUserPort(uint8_t value)
    {
      user_port_state = value;
    }
    inline uint8_t getUserPort() const
    {
      return user_port_state;
    }
    inline void setTapeInput(bool state)
    {
      tape_read_state = state;
    }
    inline bool getTapeOutput() const
    {
      return tape_write_state;
    }
    inline void setTapeButtonState(bool state)
    {
      tape_button_state = state;
    }
    inline void setTapeMotorState(bool state)
    {
      tape_motor_state = state;
      memory_ram[0x0001] = (memory_ram[0x0001] & 0xF7) | (state ? 0x00 : 0x08);
    }
    inline bool getTapeMotorState() const
    {
      return tape_motor_state;
    }
    static void convertPixelToRGB(uint8_t color,
                                  float& red, float& green, float& blue);
    // save snapshot
    void saveState(Ep128::File::Buffer&);
    void saveState(Ep128::File&);
    // load snapshot
    void loadState(Ep128::File::Buffer&);
    // save program
    void saveProgram(Ep128::File::Buffer&);
    void saveProgram(Ep128::File&);
    void saveProgram(const char *fileName);
    // load program
    void loadProgram(Ep128::File::Buffer&);
    void loadProgram(const char *fileName);
    void registerChunkTypes(Ep128::File&);
  };

}       // namespace Plus4

