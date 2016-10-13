
#ifndef EP128EMU_SDEXT_HPP
#define EP128EMU_SDEXT_HPP

extern unsigned int sdext_cp3m_usability;

uint8_t sdext_read_cart_p3 ( uint32_t addr );
void sdext_write_cart_p3 ( uint32_t addr, uint8_t data );
void sdext_init ( void );

#endif

