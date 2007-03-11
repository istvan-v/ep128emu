
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

#ifndef EP128EMU_PLUS4_TED_HPP
#define EP128EMU_PLUS4_TED_HPP

#include "ep128emu.hpp"
#include "fileio.hpp"
#include "cpu.hpp"
#include "serial.hpp"

#ifdef REGPARM
#  undef REGPARM
#endif
#if defined(__GNUC__) && (__GNUC__ >= 3) && defined(__i386__) && !defined(__ICC)
#  define REGPARM __attribute__ ((__regparm__ (3)))
#else
#  define REGPARM
#endif

namespace Plus4 {

  class TED7360 : public M7501 {
   private:
    // memory and register read functions
    static uint8_t  read_memory_0000_to_0FFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_1000_to_3FFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_4000_to_7FFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_8000_to_BFFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_C000_to_FBFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_FC00_to_FCFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_FD00_to_FEFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_FF00_to_FFFF(void *userData, uint16_t addr);
    static uint8_t  read_register_0000(void *userData, uint16_t addr);
    static uint8_t  read_register_0001(void *userData, uint16_t addr);
    static uint8_t  read_register_FD0x(void *userData, uint16_t addr);
    static uint8_t  read_register_FD1x(void *userData, uint16_t addr);
    static uint8_t  read_register_FD16(void *userData, uint16_t addr);
    static uint8_t  read_register_FD3x(void *userData, uint16_t addr);
    static uint8_t  read_register_FFxx(void *userData, uint16_t addr);
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
    static uint8_t  read_register_FF3E_FF3F(void *userData, uint16_t addr);
    // memory and register write functions
    static void     write_memory_0000_to_0FFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_1000_to_3FFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_4000_to_7FFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_8000_to_BFFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_C000_to_FBFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_FC00_to_FCFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_FD00_to_FEFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_FF00_to_FFFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_register_0000(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_0001(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FD1x(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FD16(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FD3x(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FDDx(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FFxx(void *userData,
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
    static REGPARM void render_BMM_hires(TED7360& ted, uint8_t *bufp, int offs);
    static REGPARM void render_BMM_multicolor(TED7360& ted,
                                              uint8_t *bufp, int offs);
    static REGPARM void render_char_128(TED7360& ted, uint8_t *bufp, int offs);
    static REGPARM void render_char_256(TED7360& ted, uint8_t *bufp, int offs);
    static REGPARM void render_char_ECM(TED7360& ted, uint8_t *bufp, int offs);
    static REGPARM void render_char_MCM(TED7360& ted, uint8_t *bufp, int offs);
    static REGPARM void render_invalid_mode(TED7360& ted,
                                            uint8_t *bufp, int offs);
    // -----------------------------------------------------------------
    // CPU I/O registers
    uint8_t     ioRegister_0000;
    uint8_t     ioRegister_0001;
    // number of RAM segments; can be one of the following values:
    //   1: 16K (segment FF)
    //   2: 32K (segments FE, FF)
    //   4: 64K (segments FC to FF)
    //  16: 256K Hannes (segments F0 to FF)
    //  64: 1024K Hannes (segments C0 to FF)
    uint8_t     ramSegments;
    // value written to FD16:
    //   bit 7:       enable memory expansion at 1000-FFFF if set to 0, or
    //                at 4000-FFFF if set to 1
    //   bit 6:       if set to 1, allow access to memory expansion by TED
    //   bits 0 to 3: RAM bank selected
    uint8_t     hannesRegister;
    // base index to memoryMapTable[] (see below) to be used by readMemory()
    unsigned int  memoryReadMap;
    // base index to memoryMapTable[] to be used by writeMemory()
    unsigned int  memoryWriteMap;
    // memoryReadMap is set to one of these before calling readMemory()
    unsigned int  cpuMemoryReadMap;
    unsigned int  tedDMAReadMap;
    unsigned int  tedBitmapReadMap;
    // copy of TED registers at FF00 to FF1F
    uint32_t    tedRegisterWriteMask;
    uint8_t     tedRegisters[32];
    // currently selected render function (depending on bits of FF06 and FF07)
    REGPARM void  (*render_func)(TED7360& ted, uint8_t *bufp, int offs);
    // delay mode changes by one cycle
    REGPARM void  (*prv_render_func)(TED7360& ted, uint8_t *bufp, int offs);
    // CPU clock multiplier
    int         cpu_clock_multiplier;
    // TED cycle counter (0 to 3)
    uint8_t     cycle_count;
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
    int         dma_position_reload;
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
    // flash state (0x00 or 0xFF) for cursor etc.,
    // updated on every 16th video frame
    uint8_t     flashState;
    // display state flags, set depending on video line and column
    bool        renderWindow;
    bool        dmaWindow;
    bool        bitmapFetchWindow;
    bool        displayWindow;
    bool        renderingDisplay;
    bool        displayActive;
    // bit 0: horizontal blanking
    // bit 1: vertical blanking
    uint8_t     displayBlankingFlags;
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
    uint8_t     attr_buf[64];
    uint8_t     attr_buf_tmp[64];
    uint8_t     char_buf[64];
    uint32_t    attributeShiftRegister;
    uint32_t    characterShiftRegister;
    uint32_t    cursorShiftRegister;
    uint32_t    bitmapHShiftRegister;
    uint32_t    bitmapM0ShiftRegister;
    uint32_t    bitmapM1ShiftRegister;
    uint8_t     line_buf[432];
    uint8_t     line_buf_tmp[4];
    int         line_buf_pos;
    bool        bitmapMode;
    uint8_t     characterMask;
    // bit 0: single clock mode controlled by TED
    // bit 1: copied from FF13 bit 1
    uint8_t     singleClockModeFlags;
    uint8_t     dmaCycleCounter;
    uint8_t     dmaFlags;       // sum of: 1: attribute DMA; 2: character DMA
    uint8_t     prvCharacterLine;
    uint8_t     renderCnt;
    bool        incrementingDMAPosition;
    int         savedVideoLine;
    int         videoInterruptLine;
    bool        prvVideoInterruptState;
   protected:
    // for reading data from invalid memory address
    uint8_t     dataBusState;
   private:
    // keyboard matrix
    int         keyboard_row_select_mask;
    uint8_t     keyboard_matrix[16];
    // external ports
    uint8_t     user_port_state;
    bool        tape_motor_state;
    bool        tape_read_state;
    bool        tape_write_state;
    bool        tape_button_state;
    SerialBus   serialPort;
    // --------
    uint8_t     *segmentTable[256];
    // table of 4096 memory maps, indexed by a 12-bit value multiplied by 8:
    //   bits 8 to 11: ROM banks selected by writing to FDD0 + n
    //   bit 7:        1: do not allow RAM expansion at 1000-3FFF
    //   bit 6:        1: allow use of RAM expansion by TED
    //   bit 5:        memory access by CPU (0) or TED (1)
    //   bit 4:        RAM (0) or ROM (1) selected
    //   bits 0 to 3:  Hannes memory bank
    // each memory map consists of 8 bytes selecting segments for the
    // following address ranges:
    //   0: 1000-3FFF
    //   1: 4000-7FFF
    //   2: 8000-BFFF
    //   3: C000-FBFF
    //   4: 0000-0FFF
    //   5: FC00-FCFF
    //   6: FD00-FEFF
    //   7: FF00-FFFF
    uint8_t     memoryMapTable[32768];
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
    virtual void ntscModeChangeCallback(bool isNTSC_)
    {
      (void) isNTSC_;
    }
   public:
    TED7360();
    virtual ~TED7360();
    // Load 'cnt' bytes of ROM data from 'buf' to bank 'bankNum' (0 to 3),
    // starting the write from byte 'offs' (0 to 32767). If 'buf' is NULL,
    // the ROM segment at the starting position is deleted, and 'cnt' is
    // ignored.
    void loadROM(int bankNum, int offs, int cnt, const uint8_t *buf);
    // Resize RAM to 'n' kilobytes (16, 32, 64, 256, or 1024), and clear all
    // RAM data.
    void setRAMSize(size_t n);
    void runOneCycle();
    virtual void reset(bool cold_reset = false);
    void setCPUClockMultiplier(int clk);
    void setKeyState(int keyNum, bool isPressed);
    // Returns memory segment at page 'n' (0 to 3). Segments 0x00 to 0x07 are
    // used for ROM, while segments 0xFC to 0xFF are RAM.
    uint8_t getMemoryPage(int n) const;
    // Read memory directly without paging. Valid address ranges are
    // 0x00000000 to 0x0001FFFF for ROM, and 0x003F0000 to 0x003FFFFF for RAM
    // (assuming 64K size).
    uint8_t readMemoryRaw(uint32_t addr) const;
    // Write memory directly without paging. Valid addresses are in the
    // range 0x003F0000 to 0x003FFFFF (i.e. 64K RAM).
    void writeMemoryRaw(uint32_t addr, uint8_t value);
    // Read memory at 16-bit CPU address with paging (this also allows access
    // to I/O and TED registers). If 'forceRAM_' is true, data is always read
    // from RAM.
    uint8_t readMemoryCPU(uint16_t addr, bool forceRAM_ = false) const;
    // Write memory at 16-bit CPU address with paging (this also allows access
    // to I/O and TED registers).
    void writeMemoryCPU(uint16_t addr, uint8_t value);
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
      ioRegister_0001 = (ioRegister_0001 & 0xF7) | (state ? 0x00 : 0x08);
    }
    inline bool getTapeMotorState() const
    {
      return tape_motor_state;
    }
    inline bool getIsNTSCMode() const
    {
      return !!(tedRegisters[0x07] & 0x40);
    }
    inline SerialBus& getSerialPort()
    {
      return serialPort;
    }
    inline const SerialBus& getSerialPort() const
    {
      return serialPort;
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

#ifdef REGPARM
#  undef REGPARM
#endif

#endif  // EP128EMU_PLUS4_TED_HPP

