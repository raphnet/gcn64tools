#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "gcn64lib.h"
#include "gcn64.h"
#include "mempak.h"
#include "mempak_gcn64usb.h"
#include "hexdump.h"
#include "gcn64_protocol.h"
#include "requests.h"
#include "xferpak.h"
#include "mempak_gcn64usb.h"

struct _xferpak {
	gcn64_hdl_t hdl;
	int channel;
	int cur_bank;
	uiio *u;
};

static int xferpak_testPresence(xferpak *xpak);

void xferpak_setUIIO(xferpak *pak, uiio *u)
{
	if (pak) {
		pak->u = getUIIO(u);
	}
}

xferpak *gcn64lib_xferpak_init(gcn64_hdl_t hdl, int channel, uiio *u)
{
	unsigned char cmd[64] = { };
	xferpak *pak;
	int res, n;
	u = getUIIO(u);

	/* Check for accessory presence */
	n = gcn64lib_rawSiCommand(hdl, channel, cmd, 1, cmd, sizeof(cmd));
	if (n != 3) {
		fprintf(stderr, "Failed to read N64 controller 'capabilities'\n");
		return NULL;
	}
	if (!(cmd[2] & 0x01)) {
		fprintf(stderr, "No accessory connected\n");
		return NULL;
	}

	/* Allocate pak structure */
	pak = calloc(1, sizeof(xferpak));
	if (!pak) {
		perror("could not allocate xferpak structure");
		return NULL;
	}
	pak->hdl = hdl;
	pak->channel = channel;
	pak->cur_bank = -1;
	pak->u = u;

	/* Check for presence */
	res = xferpak_testPresence(pak);
	if (res < 0) {
		fprintf(stderr, "Transfer pak not detected\n");
		free(pak);
		return NULL;
	}

	/* Enable cartridge? */
	res = xferpak_enableCartridge(pak, 1);
	if (res < 0) {
		fprintf(stderr, "Could not enable cartridge\n");
		free(pak);
		return NULL;
	}

	return pak;
}

void xferpak_free(xferpak *xpak)
{
	if (xpak) {
		xferpak_enableCartridge(xpak, 0);
		free(xpak);
	}
}

int xferpak_writeBlock(xferpak *xpak, unsigned int addr, const unsigned char data[32])
{
	return gcn64lib_mempak_writeBlock(xpak->hdl, addr, data);
}

int xferpak_readBlock(xferpak *xpak, unsigned int addr, unsigned char data[32])
{
	return gcn64lib_mempak_readBlock(xpak->hdl, addr, data);
}

static int xferpak_testPresence(xferpak *xpak)
{
	unsigned char buf[32];
	int res;

	// Disable?
	memset(buf, 0xFE, sizeof(buf));
	res = xferpak_writeBlock(xpak, 0x8000, buf);
	if (res < 0) {
		fprintf(stderr, "transfer pak io error (%d)\n", res);
		return XFERPAK_IO_ERROR;
	}

	res = xferpak_readBlock(xpak, 0x8000, buf);
	if (res != 32) {
		fprintf(stderr, "transfer pak io error\n");
		return XFERPAK_IO_ERROR;
	}
	if (buf[0] != 0x00) {
		fprintf(stderr, "transfer pak error 1\n");
		return XFERPAK_IO_ERROR;
	}

	memset(buf, 0x84, sizeof(buf));
	res = xferpak_writeBlock(xpak, 0x8000, buf);
	if (res < 0) {
		fprintf(stderr, "transfer pak io error (%d)\n", res);
		return XFERPAK_IO_ERROR;
	}

	res = xferpak_readBlock(xpak, 0x8000, buf);
	if (res != 32) {
		fprintf(stderr, "transfer pak io error\n");
		return XFERPAK_IO_ERROR;
	}
	if (buf[0] != 0x84) {
		fprintf(stderr, "transfer pak error 2\n");
		return XFERPAK_IO_ERROR;
	}

	res = xferpak_readBlock(xpak, 0x8000, buf);
	if (res != 32) {
		fprintf(stderr, "transfer pak io error\n");
		return XFERPAK_IO_ERROR;
	}
	if (buf[0] != 0x84) {
		fprintf(stderr, "transfer pak error 2\n");
		return XFERPAK_IO_ERROR;
	}

//	printf("Enable state (0x8000): "); printHexBuf(buf, 32);

	return 0;
}

/**
 * \param bank 0-7 (0x4000 per bank)
 */
int xferpak_setBank(xferpak *xpak, int bank)
{
	unsigned char buf[32];
	int res;

	if (xpak->cur_bank == bank)
		return 0;

	memset(buf, bank, sizeof(buf));
	res = xferpak_writeBlock(xpak, 0xA000, buf);
	if (res < 0) {
		fprintf(stderr, "transfer pak io error (%d)\n", res);
		return XFERPAK_IO_ERROR;
	}

	return 0;
}

int xferpak_enableCartridge(xferpak *xpak, int enable)
{
	unsigned char buf[32];
	int res;

	// FIXME: I'm not really sure what this does... Well writing 0x01 seems to
	// be required or things do not work. But is the 'enable cartridge' terminology
	// correct?

	memset(buf, enable ? 0x01 : 0x00, sizeof(buf));
	res = xferpak_writeBlock(xpak, 0xB000, buf);
	if (res < 0) {
		fprintf(stderr, "transfer pak io error (%d)\n", res);
		return XFERPAK_IO_ERROR;
	}

	return 0;
}

int xferpak_writeCart(xferpak *xpak, unsigned int start_addr, unsigned int len, const unsigned char *data)
{
	int addr;
	int res;

//	printf("Writing to gb cartridge: 0x%04x [%d bytes] : ", start_addr, len);
//	printHexBuf(data, len);

	for (addr = 0; addr < len; addr += 32)
	{
		xferpak_setBank(xpak, (addr + start_addr) >> 14);

		if (xpak->u) {
			xpak->u->cur_progress += 32;
			// Don't call update too often for performance reason
			if (!(xpak->u->cur_progress & 0x1FF)) {
				if (xpak->u->update(xpak->u)) {
					return XFERPAK_USER_CANCELLED;
				}
			}
		}

		res = xferpak_writeBlock(xpak, 0xC000 + ((addr+start_addr) & 0x3FFF), data);
		if (res < 0) {
			fprintf(stderr, "Could not write to cartridge\n");
			return XFERPAK_IO_ERROR;
		}

		data += 32;
	}

	return 0;
}

int xferpak_readCart(xferpak *xpak, unsigned int start_addr, unsigned int len, unsigned char *dst)
{
	unsigned char buf[32];
	int addr;
	int res;

	//printf("Reading gb cartridge address: 0x%04x [%d bytes]...\n", start_addr, len);
	for (addr = 0; addr < len; addr += 32)
	{
		xferpak_setBank(xpak, (addr + start_addr) >> 14);

		if (xpak->u) {
			xpak->u->cur_progress += 32;
			// Don't call update too often for performance reason
			if (!(xpak->u->cur_progress & 0x1FF)) {
				if (xpak->u->update(xpak->u)) {
					return XFERPAK_USER_CANCELLED;
				}
			}
		}

		res = xferpak_readBlock(xpak, 0xC000 + ((addr+start_addr) & 0x3FFF), buf);
		if (res != 32) {
			fprintf(stderr, "Could not read cartridge header\n");
			return XFERPAK_IO_ERROR;
		}
		memcpy(dst, buf, 32);
		dst+=32;
	}

	return 0;
}

int xferpak_gb_mbc5_select_rom_bank(xferpak *xpak, int bank)
{
	unsigned char buf[32];
	int res;

	// 0x2000 Lower 8 bits for ROM bank number
	memset(buf, bank & 0xff, sizeof(buf));
	res = xferpak_writeCart(xpak, 0x2000, sizeof(buf), buf);
	if (res<0) {
		return res;
	}

	// 0x3000 High bit of ROM bank number
	memset(buf, (bank >> 8) & 0xff, sizeof(buf));
	res = xferpak_writeCart(xpak, 0x3000, sizeof(buf), buf);
	if (res<0) {
		return res;
	}

	return 0;
}

int xferpak_gb_mbc1_select_rom_mode(xferpak *xpak)
{
	unsigned char buf[32];
	int res;

	// 0x6000 ROM/RAM Mode Select
	//
	// 0x00: ROM banking mode
	// 0x01: RAM banking mode
	memset(buf, 0x00, sizeof(buf));
	res = xferpak_writeCart(xpak, 0x6000, sizeof(buf), buf);
	if (res<0) {
		return res;
	}

	return 0;
}

int xferpak_gb_mbc1_select_ram_mode(xferpak *xpak)
{
	unsigned char buf[32];
	int res;

	// 0x6000 ROM/RAM Mode Select
	//
	// 0x00: ROM banking mode
	// 0x01: RAM banking mode
	memset(buf, 0x00, sizeof(buf));
	res = xferpak_writeCart(xpak, 0x6000, sizeof(buf), buf);
	if (res<0) {
		return res;
	}

	return 0;
}

int xferpak_gb_mbc3_select_rom_bank(xferpak *xpak, int bank)
{
	unsigned char buf[32];
	int res;

	// 0x2000 Lower 8 bits for ROM bank number
	memset(buf, bank & 0xff, sizeof(buf));
	res = xferpak_writeCart(xpak, 0x2000, sizeof(buf), buf);
	if (res<0) {
		return res;
	}

	return 0;
}

int xferpak_gb_mbc2_select_rom_bank(xferpak *xpak, int bank)
{
	unsigned char buf[32];
	int res;

	res = xferpak_gb_mbc1_select_rom_mode(xpak);
	if (res < 0) {
		return res;
	}

	// 0x2000 Lower 4 bits for ROM bank number
	memset(buf, bank & 0x0f, sizeof(buf));
	res = xferpak_writeCart(xpak, 0x2100, sizeof(buf), buf);
	if (res<0) {
		return res;
	}

	return 0;
}

int xferpak_gb_mbc1_select_rom_bank(xferpak *xpak, int bank)
{
	unsigned char buf[32];
	int res;

	res = xferpak_gb_mbc1_select_rom_mode(xpak);
	if (res < 0) {
		return res;
	}

	// 0x2000 Lower 5 bits for ROM bank number
	memset(buf, bank & 0x1f, sizeof(buf));
	res = xferpak_writeCart(xpak, 0x2000, sizeof(buf), buf);
	if (res<0) {
		return res;
	}

	// 0x4000 Bits 5-6 for ROM bank number
	memset(buf, bank >> 5, sizeof(buf));
	res = xferpak_writeCart(xpak, 0x4000, sizeof(buf), buf);
	if (res<0) {
		return res;
	}

	return 0;
}

int xferpak_gb_mbc1235_enable_ram(xferpak *xpak, int enable)
{
	unsigned char buf[32];
	int res;

	// 0x0000 Lower 8 bits for ROM bank number. Writing 0x0A enables.
	memset(buf, enable ? 0x0A : 0x00, sizeof(buf));
	res = xferpak_writeCart(xpak, 0x0000, sizeof(buf), buf);
	if (res<0) {
		return res;
	}

	return 0;
}

int xferpak_gb_mbc135_select_ram_bank(xferpak *xpak, int bank)
{
	unsigned char buf[32];
	int res;

	// 0x4000 RAM Bank Number
	memset(buf, bank, sizeof(buf));
	res = xferpak_writeCart(xpak, 0x4000, sizeof(buf), buf);
	if (res<0) {
		return res;
	}

	return 0;
}

int xferpak_gb_mbc1_writeRAM(xferpak *xpak, unsigned int ram_size, const unsigned char *data)
{
	int i, res;
	unsigned char bankbuf[0x2000]; // 8K banks
	int cur_bank = -1;

	if (ram_size & 0x1FFF) {
		fprintf(stderr, "ram size must be a multiple of 8K\n");
		return XFERPAK_BAD_PARAM;
	}

	res = xferpak_gb_mbc1235_enable_ram(xpak, 1);
	if (res < 0) {
		return res;
	}

	res = xferpak_gb_mbc1_select_ram_mode(xpak);
	if (res < 0) {
		return res;
	}

	for (i=0; i<ram_size; i+= sizeof(bankbuf))
	{
		if ((i/sizeof(bankbuf)) != cur_bank) {
			cur_bank = i/sizeof(bankbuf);
			res = xferpak_gb_mbc135_select_ram_bank(xpak, cur_bank);
			if (res < 0) {
				fprintf(stderr, "failed to set mbc5 ram bank\n");
				xferpak_gb_mbc1235_enable_ram(xpak, 0);
				return XFERPAK_IO_ERROR;
			}
		}

		memcpy(bankbuf, data, sizeof(bankbuf));
		data += sizeof(bankbuf);

		res = xferpak_writeCart(xpak, 0xA000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			fprintf(stderr, "transfer pak io error (%d)\n", res);
			xferpak_gb_mbc1235_enable_ram(xpak, 0);
			return XFERPAK_IO_ERROR;
		}
	}

	xferpak_gb_mbc1235_enable_ram(xpak, 0);

	return 0;
}

int xferpak_gb_mbc35_writeRAM(xferpak *xpak, unsigned int ram_size, const unsigned char *data)
{
	int i, res;
	unsigned char bankbuf[0x2000]; // 8K banks
	int cur_bank = -1;

	if (ram_size & 0x1FFF) {
		fprintf(stderr, "ram size must be a multiple of 8K\n");
		return XFERPAK_BAD_PARAM;
	}

	res = xferpak_gb_mbc1235_enable_ram(xpak, 1);
	if (res < 0) {
		return res;
	}

//	printf("Reading MBC5 RAM (size=0x%06x)...\n", ram_size);
	for (i=0; i<ram_size; i+= sizeof(bankbuf))
	{
		if ((i/sizeof(bankbuf)) != cur_bank) {
			cur_bank = i/sizeof(bankbuf);
//			printf("Selecting MBC5 RAM bank 0x%x\n", cur_bank);
			res = xferpak_gb_mbc135_select_ram_bank(xpak, cur_bank);
			if (res < 0) {
				fprintf(stderr, "failed to set mbc5 ram bank\n");
				xferpak_gb_mbc1235_enable_ram(xpak, 0);
				return XFERPAK_IO_ERROR;
			}
		}

		memcpy(bankbuf, data, sizeof(bankbuf));
		data += sizeof(bankbuf);

		res = xferpak_writeCart(xpak, 0xA000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			fprintf(stderr, "transfer pak io error (%d)\n", res);
			xferpak_gb_mbc1235_enable_ram(xpak, 0);
			return XFERPAK_IO_ERROR;
		}
	}

	xferpak_gb_mbc1235_enable_ram(xpak, 0);

	return 0;
}

int xferpak_gb_mbc2_readRAM(xferpak *xpak, unsigned int ram_size, unsigned char *dstbuf)
{
	int res;

	res = xferpak_gb_mbc1235_enable_ram(xpak, 1);
	if (res < 0) {
		return res;
	}

	res = xferpak_readCart(xpak, 0xA000, ram_size, dstbuf);
	if (res < 0) {
		xferpak_gb_mbc1235_enable_ram(xpak, 0);
		return res;
	}

	xferpak_gb_mbc1235_enable_ram(xpak, 0);

	return 0;
}

int xferpak_gb_mbc1_readRAM(xferpak *xpak, unsigned int ram_size, unsigned char *dstbuf)
{
	int i, res;
	unsigned char bankbuf[0x2000]; // 8K banks
	int cur_bank = -1;

	if (ram_size & 0x1FFF) {
		fprintf(stderr, "ram size must be a multiple of 8K\n");
		return XFERPAK_BAD_PARAM;
	}

	res = xferpak_gb_mbc1235_enable_ram(xpak, 1);
	if (res < 0) {
		return res;
	}

	res = xferpak_gb_mbc1_select_ram_mode(xpak);
	if (res < 0) {
		return res;
	}

	for (i=0; i<ram_size; i+= sizeof(bankbuf))
	{
		if ((i/sizeof(bankbuf)) != cur_bank) {
			cur_bank = i/sizeof(bankbuf);
			res = xferpak_gb_mbc135_select_ram_bank(xpak, cur_bank);
			if (res < 0) {
				fprintf(stderr, "failed to set mbc1 ram bank\n");
				xferpak_gb_mbc1235_enable_ram(xpak, 0);
				return XFERPAK_IO_ERROR;
			}
		}

		res = xferpak_readCart(xpak, 0xA000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			xferpak_gb_mbc1235_enable_ram(xpak, 0);
			return res;
		}

		memcpy(dstbuf, bankbuf, sizeof(bankbuf));
		dstbuf += sizeof(bankbuf);
	}

	xferpak_gb_mbc1235_enable_ram(xpak, 0);

	return 0;
}


int xferpak_gb_mbc35_readRAM(xferpak *xpak, unsigned int ram_size, unsigned char *dstbuf)
{
	int i, res;
	unsigned char bankbuf[0x2000]; // 8K banks
	int cur_bank = -1;

	if (ram_size & 0x1FFF) {
		fprintf(stderr, "ram size must be a multiple of 8K\n");
		return XFERPAK_BAD_PARAM;
	}

	res = xferpak_gb_mbc1235_enable_ram(xpak, 1);
	if (res < 0) {
		return res;
	}

//	printf("Reading MBC5 RAM (size=0x%06x)...\n", ram_size);
	for (i=0; i<ram_size; i+= sizeof(bankbuf))
	{
		if ((i/sizeof(bankbuf)) != cur_bank) {
			cur_bank = i/sizeof(bankbuf);
//			printf("Selecting MBC5 RAM bank 0x%x\n", cur_bank);
			res = xferpak_gb_mbc135_select_ram_bank(xpak, cur_bank);
			if (res < 0) {
				fprintf(stderr, "failed to set mbc5 ram bank\n");
				xferpak_gb_mbc1235_enable_ram(xpak, 0);
				return XFERPAK_IO_ERROR;
			}
		}

		res = xferpak_readCart(xpak, 0xA000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			xferpak_gb_mbc1235_enable_ram(xpak, 0);
			return res;
		}

		memcpy(dstbuf, bankbuf, sizeof(bankbuf));
		dstbuf += sizeof(bankbuf);
	}

	xferpak_gb_mbc1235_enable_ram(xpak, 0);

	return 0;
}

int xferpak_gb_mbc5_readROM(xferpak *xpak, unsigned int rom_size, unsigned char *dstbuf)
{
	int i, res;
	unsigned char bankbuf[0x4000];
	int cur_bank = -1;

	//printf("Reading MBC5 rom (size=0x%06x)...\n", rom_size);
	for (i=0; i<rom_size; i+= sizeof(bankbuf))
	{
		if ((i/sizeof(bankbuf)) != cur_bank) {
			cur_bank = i/sizeof(bankbuf);
			//printf("Selecting MBC5 ROM bank 0x%x\n", cur_bank);
			res = xferpak_gb_mbc5_select_rom_bank(xpak, cur_bank);
			if (res < 0) {
				fprintf(stderr, "failed to set mbc5 bank\n");
				return XFERPAK_IO_ERROR;
			}
		}

		res = xferpak_readCart(xpak, 0x4000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			return res;
		}

		memcpy(dstbuf, bankbuf, sizeof(bankbuf));
		dstbuf += sizeof(bankbuf);
	}

	return 0;
}

int xferpak_gb_mbc3_readROM(xferpak *xpak, unsigned int rom_size, unsigned char *dstbuf)
{
	int i, res;
	unsigned char bankbuf[0x4000];
	int cur_bank = -1;

	/* First read bank 00 at its fixed address. */
	res = xferpak_readCart(xpak, 0x0000, sizeof(bankbuf), bankbuf);
	if (res < 0) {
		fprintf(stderr, "transfer pak io error (%d)\n", res);
		return res;
	}
	memcpy(dstbuf, bankbuf, sizeof(bankbuf));
	dstbuf += sizeof(bankbuf);

	/* Now read all other banks */
	for (i=sizeof(bankbuf); i<rom_size; i+= sizeof(bankbuf))
	{
		if ((i/sizeof(bankbuf)) != cur_bank) {
			cur_bank = i/sizeof(bankbuf);
			//printf("Selecting MBC5 ROM bank 0x%x\n", cur_bank);
			res = xferpak_gb_mbc3_select_rom_bank(xpak, cur_bank);
			if (res < 0) {
				fprintf(stderr, "failed to set mbc5 bank\n");
				return XFERPAK_IO_ERROR;
			}
		}

		res = xferpak_readCart(xpak, 0x4000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			return res;
		}

		memcpy(dstbuf, bankbuf, sizeof(bankbuf));
		dstbuf += sizeof(bankbuf);
	}

	return 0;
}

int xferpak_gb_mbc2_readROM(xferpak *xpak, unsigned int rom_size, unsigned char *dstbuf)
{
	int i, res;
	unsigned char bankbuf[0x4000];
	int cur_bank = -1;

	/* First read bank 00 at its fixed address. */
	res = xferpak_readCart(xpak, 0x0000, sizeof(bankbuf), bankbuf);
	if (res < 0) {
		fprintf(stderr, "transfer pak io error (%d)\n", res);
		return res;
	}
	memcpy(dstbuf, bankbuf, sizeof(bankbuf));
	dstbuf += sizeof(bankbuf);

	/* Now read all other banks */
	for (i=sizeof(bankbuf); i<rom_size; i+= sizeof(bankbuf))
	{
		if ((i/sizeof(bankbuf)) != cur_bank) {
			cur_bank = i/sizeof(bankbuf);
			res = xferpak_gb_mbc2_select_rom_bank(xpak, cur_bank);
			if (res < 0) {
				fprintf(stderr, "failed to set mbc1 bank\n");
				return XFERPAK_IO_ERROR;
			}
		}

		res = xferpak_readCart(xpak, 0x4000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			return res;
		}

		memcpy(dstbuf, bankbuf, sizeof(bankbuf));
		dstbuf += sizeof(bankbuf);
	}

	return 0;
}

int xferpak_gb_mbc1_readROM(xferpak *xpak, unsigned int rom_size, unsigned char *dstbuf)
{
	int i, res;
	unsigned char bankbuf[0x4000];
	int cur_bank = -1;

	/* First read bank 00 at its fixed address. */
	res = xferpak_readCart(xpak, 0x0000, sizeof(bankbuf), bankbuf);
	if (res < 0) {
		fprintf(stderr, "transfer pak io error (%d)\n", res);
		return res;
	}
	memcpy(dstbuf, bankbuf, sizeof(bankbuf));
	dstbuf += sizeof(bankbuf);

	/* Now read all other banks */
	for (i=sizeof(bankbuf); i<rom_size; i+= sizeof(bankbuf))
	{
		if ((i/sizeof(bankbuf)) != cur_bank) {
			cur_bank = i/sizeof(bankbuf);
			res = xferpak_gb_mbc1_select_rom_bank(xpak, cur_bank);
			if (res < 0) {
				fprintf(stderr, "failed to set mbc1 bank\n");
				return XFERPAK_IO_ERROR;
			}
		}

		res = xferpak_readCart(xpak, 0x4000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			return res;
		}

		memcpy(dstbuf, bankbuf, sizeof(bankbuf));
		dstbuf += sizeof(bankbuf);
	}

	return 0;
}

int xferpak_gb_32k_read(xferpak *xpak, unsigned char *dstbuf)
{
	unsigned int rom_size = 0x8000;
	int res;

	res = xferpak_readCart(xpak, 0x0000, rom_size, dstbuf);
	if (res < 0) {
		return res;
	}

	return 0;
}

int xferpak_gb_readInfo(xferpak *xpak, struct gbcart_info *inf)
{
	unsigned char header[0x200];
	unsigned char chksum;
	int res, i;

	if (!inf)
		return XFERPAK_BAD_PARAM;

	/* Read the header */
	res = xferpak_readCart(xpak, 0, sizeof(header), header);
	if (res < 0) {
		return XFERPAK_COULD_NOT_READ_HEADER;
	}

	/* Verify checksum */
	for (chksum=0,i=0x134; i<=0x14C; i++) {
		chksum -= header[i]+1;
	}
	if (header[0x14D] != chksum) {
		fprintf(stderr, "Bad header checksum!\n");
		return XFERPAK_BAD_CHECKSUM;
	}

	/* Title at 0x134 */
	memset(inf->title, 0, sizeof(inf->title));
	memcpy(inf->title, header + 0x134, sizeof(inf->title)-1);

	/* Cartridge type at 0x147 */
	inf->type = header[0x147];
	inf->rom_size = getGBCartROMSize(header[0x148]);
	inf->ram_size = getGBCartRAMSize(header[0x149]);

	/* Build flags */
	inf->flags = getGBCartTypeFlags(inf->type);
	if (header[0x14A] == 0) {
		inf->flags |= GB_FLAG_JAPANESE_MARKET;
	}

	return 0;
}

#define MEMORY_TYPE_ROM	0
#define MEMORY_TYPE_RAM	1
static int xferpak_gb_readMEMORY(xferpak *xpak, struct gbcart_info *inf, int type, unsigned char **membuffer)
{
	unsigned char *mem = NULL;
	struct gbcart_info cartinfo;
	int res;
	unsigned int memory_size;

	if (!xpak)
		return XFERPAK_BAD_PARAM;
	if (!membuffer)
		return XFERPAK_BAD_PARAM;

	/* Read and validate the header */
	res = xferpak_gb_readInfo(xpak, &cartinfo);
	if (res < 0) {
		return res;
	}

	/* Copy the header to user-supplied pointer */
	if (inf) {
		memcpy(inf, &cartinfo, sizeof(cartinfo));
	}

	/* Allocate buffer for memory */
	if (type == MEMORY_TYPE_RAM) {
		memory_size = cartinfo.ram_size;
	} else {
		memory_size = cartinfo.rom_size;
	}
	if (GB_MBC_MASK(cartinfo.flags) == GB_FLAG_MBC2) {
		if (cartinfo.flags & (GB_FLAG_RAM|GB_FLAG_BATTERY)) {
			memory_size = 0x200;
		}
	}

	if (memory_size <= 0) {
		fprintf(stderr, "Error: Memory size is 0.\n");
		return XFERPAK_NO_RAM;
	}
	mem = calloc(1, memory_size);
	if (!mem) {
		perror("calloc");
		return XFERPAK_OUT_OF_MEMORY;
	}

	/* Prepare the progress */
	if (xpak->u) {
		xpak->u->cur_progress = 0;
		xpak->u->max_progress = memory_size;
		xpak->u->progressStart(xpak->u);
	}

	/* Do it */
	if (type == MEMORY_TYPE_ROM)
	{
		switch(GB_MBC_MASK(cartinfo.flags))
		{
			case 0: /* ROM ONLY */
				if (memory_size != 0x8000) {
					res = XFERPAK_UNSUPPORTED;
					break;
				}
				res = xferpak_gb_32k_read(xpak, mem);
				break;
			case GB_FLAG_MBC5:
				res = xferpak_gb_mbc5_readROM(xpak, memory_size, mem);
				break;
			case GB_FLAG_MBC3:
				res = xferpak_gb_mbc3_readROM(xpak, memory_size, mem);
				break;
			case GB_FLAG_MBC2:
				res = xferpak_gb_mbc2_readROM(xpak, memory_size, mem);
				break;
			case GB_FLAG_MBC1:
				res = xferpak_gb_mbc1_readROM(xpak, memory_size, mem);
				break;
			default:
				fprintf(stderr, "Cartridge type not yet supported\n");
				res = XFERPAK_UNSUPPORTED;
		}
	}
	else {
		switch(GB_MBC_MASK(cartinfo.flags))
		{
			case GB_FLAG_MBC3:
			case GB_FLAG_MBC5:
				res = xferpak_gb_mbc35_readRAM(xpak, memory_size, mem);
				break;
			case GB_FLAG_MBC2:
				res = xferpak_gb_mbc2_readRAM(xpak, memory_size, mem);
				break;
			case GB_FLAG_MBC1:
				res = xferpak_gb_mbc1_readRAM(xpak, memory_size, mem);
				break;
			default:
				fprintf(stderr, "Cartridge type not yet supported\n");
				res = XFERPAK_UNSUPPORTED;
		}

	}

	/* End progress (UI) */
	if (res)
	{
		/* error return */
		if (xpak->u) {
			xpak->u->progressEnd(xpak->u, "Aborted");
		}

		free(mem);
		return res;
	}

	if (xpak->u) {
		xpak->u->progressEnd(xpak->u, type == MEMORY_TYPE_ROM ? "Done reading ROM":"Done reading RAM");
	}
	*membuffer = mem;

	return memory_size;
}

int xferpak_gb_writeRAM(xferpak *xpak, unsigned int mem_size, const unsigned char *mem)
{
	struct gbcart_info cartinfo;
	int res;

	if (!xpak)
		return XFERPAK_BAD_PARAM;
	if (!mem)
		return XFERPAK_BAD_PARAM;

	/* Read and validate the header */
	res = xferpak_gb_readInfo(xpak, &cartinfo);
	if (res < 0) {
		return XFERPAK_COULD_NOT_READ_HEADER;
	}

	/* Prepare the progress */
	if (xpak->u) {
		xpak->u->cur_progress = 0;
		xpak->u->max_progress = cartinfo.ram_size;
		xpak->u->progressStart(xpak->u);
	}

	switch(GB_MBC_MASK(cartinfo.flags))
	{
		case GB_FLAG_MBC3:
		case GB_FLAG_MBC5:
			res = xferpak_gb_mbc35_writeRAM(xpak, mem_size, mem);
			break;
		case GB_FLAG_MBC1:
			res = xferpak_gb_mbc1_writeRAM(xpak, mem_size, mem);
			break;
		default:
			fprintf(stderr, "Cartridge type not yet supported\n");

			if (xpak->u)
				xpak->u->progressEnd(xpak->u, "Aborted");

			return XFERPAK_UNSUPPORTED;
	}

	if (xpak->u)
		xpak->u->progressEnd(xpak->u, "Done writing RAM");

	return res;
}

int xferpak_gb_readRAM(xferpak *xpak, struct gbcart_info *inf, unsigned char **rombuffer)
{
	return xferpak_gb_readMEMORY(xpak, inf, MEMORY_TYPE_RAM, rombuffer);
}

int xferpak_gb_readROM(xferpak *xpak, struct gbcart_info *inf, unsigned char **rombuffer)
{
	return xferpak_gb_readMEMORY(xpak, inf, MEMORY_TYPE_ROM, rombuffer);
}

const char *xferpak_errStr(int error)
{
	switch (error)
	{
		case XFERPAK_OK: return "Ok";
		case XFERPAK_BAD_PARAM: return "Bad parameter";
		case XFERPAK_COULD_NOT_READ_HEADER: return "Could not read header";
		case XFERPAK_BAD_CHECKSUM: return "Bad header checksum";
		case XFERPAK_IO_ERROR: return "IO error";
		case XFERPAK_UNSUPPORTED: return "Cartridge type not supported yet";
		case XFERPAK_NO_RAM: return "Cartridge does not contain RAM";
		case XFERPAK_USER_CANCELLED: return "Manually cancelled.";
		case XFERPAK_OUT_OF_MEMORY: return "Out of memory";
	}
	return "Undefined error";
}
