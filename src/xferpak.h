#ifndef _xferpak_h__
#define _xferpak_h__

#include <stdint.h>
#include "gcn64.h"
#include "gbcart.h"
#include "uiio.h"

#define XFERPAK_OK 0
#define XFERPAK_BAD_PARAM -1
#define XFERPAK_COULD_NOT_READ_HEADER -2
#define XFERPAK_BAD_CHECKSUM -3
#define XFERPAK_IO_ERROR -4
#define XFERPAK_UNSUPPORTED -5
#define XFERPAK_NO_RAM -6
#define XFERPAK_USER_CANCELLED -7
#define XFERPAK_OUT_OF_MEMORY	-8

typedef struct _xferpak xferpak;

/** \brief Allocate and initialize a transfer pak
 * \param hdl Handle to adapter
 * \param channel Adapter channel (port)
 * \return The xferpak object, or NULL if not detected or initialization failed.
 */
xferpak *gcn64lib_xferpak_init(gcn64_hdl_t hdl, int channel, uiio *uiio);
void xferpak_free(xferpak *xpak);
void xferpak_setUIIO(xferpak *pak, uiio *uiio);

/* Transfer Pak low level IO (N64 pak address space) */
int xferpak_writeBlock(xferpak *xpak, unsigned int addr, const unsigned char data[32]);
int xferpak_readBlock(xferpak *xpak, unsigned int addr, unsigned char data[32]);
int xferpak_setBank(xferpak *xpak, int bank);
int xferpak_enableCartridge(xferpak *xpak, int enable);

/* Gameboy cartridge IO in gameboy cartridge address space (uses above low level IO) */
int xferpak_writeCart(xferpak *xpak, unsigned int start_addr, unsigned int len, const unsigned char *data);
int xferpak_readCart(xferpak *xpak, unsigned int start_addr, unsigned int len, unsigned char *dst);

/* Gamecube MBC chips operations */
int xferpak_gb_mbc5_select_rom_bank(xferpak *xpak, int bank);
int xferpak_gb_mbc1235_enable_ram(xferpak *xpak, int enable);
int xferpak_gb_mbc1_select_rom_mode(xferpak *xpak);
int xferpak_gb_mbc1_select_ram_mode(xferpak *xpak);
int xferpak_gb_mbc1_select_rom_bank(xferpak *xpak, int bank);
int xferpak_gb_mbc2_select_rom_bank(xferpak *xpak, int bank);
int xferpak_gb_mbc3_select_rom_bank(xferpak *xpak, int bank);
int xferpak_gb_mbc135_select_ram_bank(xferpak *xpak, int bank);

/* RAM and ROM IO for various MBC chips */
int xferpak_gb_mbc2_readROM(xferpak *xpak, unsigned int rom_size, unsigned char *dstbuf);
int xferpak_gb_mbc3_readROM(xferpak *xpak, unsigned int rom_size, unsigned char *dstbuf);
int xferpak_gb_mbc35_readRAM(xferpak *xpak, unsigned int ram_size, unsigned char *dstbuf);
int xferpak_gb_mbc5_readROM(xferpak *xpak, unsigned int rom_size, unsigned char *dstbuf);
int xferpak_gb_mbc1_writeRAM(xferpak *xpak, unsigned int ram_size, const unsigned char *data);
int xferpak_gb_mbc35_writeRAM(xferpak *xpak, unsigned int ram_size, const unsigned char *data);

/* * * High level cartridge operations * * */

int xferpak_gb_readInfo(xferpak *xpak, struct gbcart_info *inf);
/** \brief Read the ROM from a cartridge.
 * \param xpak The transfer pak to use
 * \param inf Optional pointer to store a copy of the cartridge info.
 * \param rambuffer Pointer to pointer to rom buffer. (Must be freed using free() later)
 * \return The size of the ROM. Will be 0 if no RAM and rombuffer will not be allocated. Negative values on error.
 **/
int xferpak_gb_readROM(xferpak *xpak, struct gbcart_info *inf, unsigned char **rombuffer);
int xferpak_gb_readRAM(xferpak *xpak, struct gbcart_info *inf, unsigned char **rombuffer);
int xferpak_gb_writeRAM(xferpak *xpak, unsigned int mem_size, const unsigned char *mem);

const char *xferpak_errStr(int error);

#endif // _xferpak_h__
