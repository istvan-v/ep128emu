
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
#include "fileio.hpp"
#include "cpu.hpp"

namespace Plus4 {

  class TED7360 : public M7501 {
   private:
    class ColorRegisters {
     private:
      uint64_t  colors_;
     public:
      ColorRegisters()
        : colors_(0U)
      {
      }
      ColorRegisters(const ColorRegisters& r)
        : colors_(r.colors_)
      {
      }
      inline ColorRegisters& operator=(const ColorRegisters& r)
      {
        colors_ = r.colors_;
        return (*this);
      }
      inline unsigned char& operator[](size_t n)
      {
        return ((unsigned char *) &colors_)[n];
      }
    };
    // memory images (64K RAM, 4*32K ROM)
    uint8_t     memory_ram[65536];
    uint8_t     memory_rom[4][32768];
    // memory and register read functions
    static uint8_t  read_ram_(void *userData, uint16_t addr);
    static uint8_t  read_memory_8000(void *userData, uint16_t addr);
    static uint8_t  read_memory_C000(void *userData, uint16_t addr);
    static uint8_t  read_memory_FC00(void *userData, uint16_t addr);
    static uint8_t  read_register_0000(void *userData, uint16_t addr);
    static uint8_t  read_register_0001(void *userData, uint16_t addr);
    static uint8_t  read_register_unimplemented(void *userData, uint16_t addr);
    static uint8_t  read_register_FD10(void *userData, uint16_t addr);
    static uint8_t  read_register_FD30(void *userData, uint16_t addr);
    static uint8_t  read_register_FF00(void *userData, uint16_t addr);
    static uint8_t  read_register_FF01(void *userData, uint16_t addr);
    static uint8_t  read_register_FF02(void *userData, uint16_t addr);
    static uint8_t  read_register_FF03(void *userData, uint16_t addr);
    static uint8_t  read_register_FF04(void *userData, uint16_t addr);
    static uint8_t  read_register_FF05(void *userData, uint16_t addr);
    static uint8_t  read_register_FF06(void *userData, uint16_t addr);
    static uint8_t  read_register_FF09(void *userData, uint16_t addr);
    static uint8_t  read_register_FF0A(void *userData, uint16_t addr);
    static uint8_t  read_register_FF0C(void *userData, uint16_t addr);
    static uint8_t  read_register_FF10(void *userData, uint16_t addr);
    static uint8_t  read_register_FF12(void *userData, uint16_t addr);
    static uint8_t  read_register_FF13(void *userData, uint16_t addr);
    static uint8_t  read_register_FF14(void *userData, uint16_t addr);
    static uint8_t  read_register_FF15_to_FF19(void *userData, uint16_t addr);
    static uint8_t  read_register_FF1A(void *userData, uint16_t addr);
    static uint8_t  read_register_FF1B(void *userData, uint16_t addr);
    static uint8_t  read_register_FF1C(void *userData, uint16_t addr);
    static uint8_t  read_register_FF1D(void *userData, uint16_t addr);
    static uint8_t  read_register_FF1E(void *userData, uint16_t addr);
    static uint8_t  read_register_FF1F(void *userData, uint16_t addr);
    static uint8_t  read_register_FF3E(void *userData, uint16_t addr);
    static uint8_t  read_register_FF3F(void *userData, uint16_t addr);
    // memory and register write functions
    static void     write_ram_(void *userData, uint16_t addr, uint8_t value);
    static void     write_register_0000(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_0001(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FD1x(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FD3x(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FDDx(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF00(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF01(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF02(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF03(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF04(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF05(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF06(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF07(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF08(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF09(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0A(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0B(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0C(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0D(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0E(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0F(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF10(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF12(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF13(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF14(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF15_to_FF19(void *userData,
                                                uint16_t addr, uint8_t value);
    static void     write_register_FF1A(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF1B(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF1C(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF1D(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF1E(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF1F(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF3E(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF3F(void *userData,
                                        uint16_t addr, uint8_t value);
    // render functions
    static void render_BMM_hires(void *ted_);
    static void render_BMM_multicolor(void *ted_);
    static void render_char_128(void *ted_);
    static void render_char_256(void *ted_);
    static void render_char_ECM(void *ted_);
    static void render_char_MCM_128(void *ted_);
    static void render_char_MCM_256(void *ted_);
    static void render_invalid_mode(void *ted_);
    // currently selected render function (depending on bits of FF06 and FF07)
    void        (*render_func)(void *ted_);
    // -----------------------------------------------------------------
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
    // current video column (0 to 113, = (FF1E) / 2)
    uint8_t     video_column;
    // current video line (0 to 311, = (FF1D, FF1C)
    int         video_line;
    // character sub-line (0 to 7, bits 0..2 of FF1F)
    int         character_line;
    // character line offset (FF1A, FF1B)
    int         character_position;
    int         character_position_reload;
    int         character_column;
    int         dma_position;
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
    // display state flags, set depending on video line and column
    bool        renderWindow;
    bool        dmaWindow;
    bool        bitmapFetchWindow;
    bool        displayWindow;
    bool        renderingDisplay;
    bool        displayActive;
    bool        horizontalBlanking;
    bool        verticalBlanking;
    bool        singleClockMode;
    bool        doubleClockModeEnabled;
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
    uint8_t     attr_buf_tmp[40];
    uint8_t     char_buf[40];
    uint8_t     pixel_buf[64];
    uint8_t     line_buf[414];
    int         line_buf_pos;
    ColorRegisters  oldColors;
    ColorRegisters  newColors;
    bool        bitmapMode;
    uint8_t     currentCharacter;
    uint8_t     characterMask;
    uint8_t     currentAttribute;
    uint8_t     currentBitmap;
    uint8_t     pixelBufReadPos;
    uint8_t     pixelBufWritePos;
    uint8_t     attributeDMACnt;
    uint8_t     characterDMACnt;
    uint8_t     savedCharacterLine;
    int         videoInterruptLine;
    bool        prvVideoInterruptState;
    // for fetching bitmap data from invalid memory address
    uint8_t     dataBusState;
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
    void selectRenderer();
    void initRegisters();
   protected:
    virtual void playSample(int16_t sampleValue)
    {
      (void) sampleValue;
    }
    virtual void drawLine(const uint8_t *buf, size_t nBytes)
    {
      (void) buf;
      (void) nBytes;
    }
    virtual void verticalSync(bool newState_, unsigned int currentSlot_)
    {
      (void) newState_;
      (void) currentSlot_;
    }
   public:
    TED7360();
    virtual ~TED7360();
    void loadROM(int bankNum, int offs, int cnt, const uint8_t *buf);
    void run(int nCycles = 1);
    virtual void reset(bool cold_reset = false);
    void setCPUClockMultiplier(int clk);
    void setKeyState(int keyNum, bool isPressed);
    // Returns memory segment at page 'n' (0 to 3). Segments 0x00 to 0x07 are
    // used for ROM, while segments 0xFC to 0xFF are RAM.
    uint8_t getMemoryPage(int n) const;
    // Read memory directly without paging. Valid address ranges are
    // 0x00000000 to 0x0001FFFF for ROM, and 0x003F0000 to 0x003FFFFF for RAM.
    uint8_t readMemoryRaw(uint32_t addr) const;
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
    void saveState(Ep128Emu::File::Buffer&);
    void saveState(Ep128Emu::File&);
    // load snapshot
    void loadState(Ep128Emu::File::Buffer&);
    // save program
    void saveProgram(Ep128Emu::File::Buffer&);
    void saveProgram(Ep128Emu::File&);
    void saveProgram(const char *fileName);
    // load program
    void loadProgram(Ep128Emu::File::Buffer&);
    void loadProgram(const char *fileName);
    void registerChunkTypes(Ep128Emu::File&);
  };

}       // namespace Plus4

