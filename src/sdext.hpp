
#ifndef EP128EMU_SDEXT_HPP
#define EP128EMU_SDEXT_HPP

extern unsigned int sdext_cp3m_usability;

void sdext_init ( bool clear_ram );
void sdext_open_image ( const char *sdimg_path );
uint8_t sdext_read_cart_p3 ( uint32_t addr, const uint8_t *sd_rom_ext );
void sdext_write_cart_p3 ( uint32_t addr, uint8_t data );

#endif

