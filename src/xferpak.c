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
	xferpak_progress *progress;
};

static int xferpak_testPresence(xferpak *xpak);

void xferpak_setProgressStruct(xferpak *pak, xferpak_progress *progress)
{
	if (pak && progress && progress->caption && progress->update) {
		pak->progress = progress;
	}
}

xferpak *gcn64lib_xferpak_init(gcn64_hdl_t hdl, int channel)
{
	unsigned char cmd[64] = { };
	xferpak *pak;
	int res, n;

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
		return -1;
	}

	res = xferpak_readBlock(xpak, 0x8000, buf);
	if (res != 32) {
		fprintf(stderr, "transfer pak io error\n");
		return -1;
	}
	if (buf[0] != 0x00) {
		fprintf(stderr, "transfer pak error 1\n");
		return -1;
	}

	memset(buf, 0x84, sizeof(buf));
	res = xferpak_writeBlock(xpak, 0x8000, buf);
	if (res < 0) {
		fprintf(stderr, "transfer pak io error (%d)\n", res);
		return -1;
	}

	res = xferpak_readBlock(xpak, 0x8000, buf);
	if (res != 32) {
		fprintf(stderr, "transfer pak io error\n");
		return -1;
	}
	if (buf[0] != 0x84) {
		fprintf(stderr, "transfer pak error 2\n");
		return -1;
	}

	res = xferpak_readBlock(xpak, 0x8000, buf);
	if (res != 32) {
		fprintf(stderr, "transfer pak io error\n");
		return -1;
	}
	if (buf[0] != 0x84) {
		fprintf(stderr, "transfer pak error 2\n");
		return -1;
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
		return -1;
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
		return -1;
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

		if (xpak->progress) {
			xpak->progress->cur_addr += 32;
			// Don't call update too often for performance reason
			if (!(xpak->progress->cur_addr & 0x1FF)) {
				xpak->progress->update(xpak->progress);
			}
		}

		res = xferpak_writeBlock(xpak, 0xC000 + ((addr+start_addr) & 0x3FFF), data);
		if (res < 0) {
			fprintf(stderr, "Could not write to cartridge\n");
			return -1;
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

		if (xpak->progress) {
			xpak->progress->cur_addr += 32;
			// Don't call update too often for performance reason
			if (!(xpak->progress->cur_addr & 0x1FF)) {
				xpak->progress->update(xpak->progress);
			}
		}

		res = xferpak_readBlock(xpak, 0xC000 + ((addr+start_addr) & 0x3FFF), buf);
		if (res != 32) {
			fprintf(stderr, "Could not read cartridge header\n");
			return -1;
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

int xferpak_gb_mbc1_select_rom_bank(xferpak *xpak, int bank)
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

int xferpak_gb_mbc35_writeRAM(xferpak *xpak, unsigned int ram_size, const unsigned char *data)
{
	int i, res;
	unsigned char bankbuf[0x2000]; // 8K banks
	int cur_bank = -1;

	if (ram_size & 0x1FFF) {
		fprintf(stderr, "ram size must be a multiple of 8K\n");
		return -1;
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
				return -1;
			}
		}

		memcpy(bankbuf, data, sizeof(bankbuf));
		data += sizeof(bankbuf);

		res = xferpak_writeCart(xpak, 0xA000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			fprintf(stderr, "transfer pak io error (%d)\n", res);
			xferpak_gb_mbc1235_enable_ram(xpak, 0);
			return -1;
		}
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
		return -1;
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
				return -1;
			}
		}

		res = xferpak_readCart(xpak, 0xA000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			fprintf(stderr, "transfer pak io error (%d)\n", res);
			xferpak_gb_mbc1235_enable_ram(xpak, 0);
			return -1;
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
				return -1;
			}
		}

		res = xferpak_readCart(xpak, 0x4000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			fprintf(stderr, "transfer pak io error (%d)\n", res);
			return -1;
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
		return -1;
	}
	memcpy(dstbuf, bankbuf, sizeof(bankbuf));
	dstbuf += sizeof(bankbuf);

	/* Now read all other banks */
	for (i=sizeof(bankbuf); i<rom_size; i+= sizeof(bankbuf))
	{
		if ((i/sizeof(bankbuf)) != cur_bank) {
			cur_bank = i/sizeof(bankbuf);
			//printf("Selecting MBC5 ROM bank 0x%x\n", cur_bank);
			res = xferpak_gb_mbc1_select_rom_bank(xpak, cur_bank);
			if (res < 0) {
				fprintf(stderr, "failed to set mbc5 bank\n");
				return -1;
			}
		}

		res = xferpak_readCart(xpak, 0x4000, sizeof(bankbuf), bankbuf);
		if (res < 0) {
			fprintf(stderr, "transfer pak io error (%d)\n", res);
			return -1;
		}

		memcpy(dstbuf, bankbuf, sizeof(bankbuf));
		dstbuf += sizeof(bankbuf);
	}

	return 0;
}


int xferpak_gb_readInfo(xferpak *xpak, struct gbcart_info *inf)
{
	unsigned char header[0x200];
	unsigned char chksum;
	int res, i;

	if (!inf)
		return -1;

	/* Read the header */
	res = xferpak_readCart(xpak, 0, sizeof(header), header);
	if (res < 0) {
		return res;
	}

	/* Verify checksum */
	for (chksum=0,i=0x134; i<=0x14C; i++) {
		chksum -= header[i]+1;
	}
	if (header[0x14D] != chksum) {
		fprintf(stderr, "Bad header checksum!\n");
		return -1;
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
	unsigned char *mem;
	struct gbcart_info cartinfo;
	int res;
	unsigned int memory_size;

	if (!xpak)
		return -1;

	if (!membuffer)
		return -1;

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
	if (memory_size <= 0) {
		fprintf(stderr, "Error: Memory size is 0.\n");
		return -1;
	}
	mem = calloc(1, memory_size);
	if (!mem) {
		perror("calloc");
		return -1;
	}

	/* Prepare progress structure */
	if (xpak->progress) {
		xpak->progress->cur_addr = 0;
		xpak->progress->max_addr = memory_size;
	}

	/* Do it */
	if (type == MEMORY_TYPE_ROM)
	{
		switch(GB_MBC_MASK(cartinfo.flags))
		{
			case GB_FLAG_MBC5:
				res = xferpak_gb_mbc5_readROM(xpak, memory_size, mem);
				break;
			case GB_FLAG_MBC3:
				res = xferpak_gb_mbc3_readROM(xpak, memory_size, mem);
				break;
			default:
				fprintf(stderr, "Cartridge type not yet supported\n");
				free(mem);
				return -1;
		}
	}
	else {
		switch(GB_MBC_MASK(cartinfo.flags))
		{
			case GB_FLAG_MBC3:
			case GB_FLAG_MBC5:
				res = xferpak_gb_mbc35_readRAM(xpak, memory_size, mem);
				break;
			default:
				fprintf(stderr, "Cartridge type not yet supported\n");
				free(mem);
				return -1;
		}

	}

	if (res < 0) {
		free(mem);
		return res;
	}

	*membuffer = mem;

	return memory_size;
}

int xferpak_gb_writeRAM(xferpak *xpak, unsigned int mem_size, const unsigned char *mem)
{
	struct gbcart_info cartinfo;
	int res;

	if (!xpak)
		return -1;
	if (!mem)
		return -1;

	/* Read and validate the header */
	res = xferpak_gb_readInfo(xpak, &cartinfo);
	if (res < 0) {
		return res;
	}

	/* Prepare progress structure */
	if (xpak->progress) {
		xpak->progress->cur_addr = 0;
		xpak->progress->max_addr = cartinfo.ram_size;
	}

	switch(GB_MBC_MASK(cartinfo.flags))
	{
		case GB_FLAG_MBC3:
		case GB_FLAG_MBC5:
			res = xferpak_gb_mbc35_writeRAM(xpak, mem_size, mem);
			break;
		default:
			fprintf(stderr, "Cartridge type not yet supported\n");
			return -1;
	}

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

