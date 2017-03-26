#ifndef _xferpak_h__
#define _xferpak_h__

#include <stdint.h>
#include "gcn64.h"
#include "gbcart.h"

typedef struct _xferpak xferpak;

typedef struct _xferpak_progress {
	const char *caption;
	uint32_t cur_addr;
	uint32_t max_addr;
	int (*update)(struct _xferpak_progress *progress);
} xferpak_progress;

/** \brief Allocate and initialize a transfer pak
 * \param hdl Handle to adapter
 * \param channel Adapter channel (port)
 * \return The xferpak object, or NULL if not detected or initialization failed.
 */
xferpak *gcn64lib_xferpak_init(gcn64_hdl_t hdl, int channel);
void xferpak_free(xferpak *xpak);
void xferpak_setProgressStruct(xferpak *pak, xferpak_progress *progress);

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
int xferpak_gb_mbc5_select_ram_bank(xferpak *xpak, int bank);

/* RAM and ROM IO for various MBC chips */
int xferpak_gb_mbc5_readRAM(xferpak *xpak, unsigned int ram_size, unsigned char *dstbuf);
int xferpak_gb_mbc5_readROM(xferpak *xpak, unsigned int rom_size, unsigned char *dstbuf);
int xferpak_gb_mbc5_writeRAM(xferpak *xpak, unsigned int ram_size, const unsigned char *data);

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


#endif // _xferpak_h__
