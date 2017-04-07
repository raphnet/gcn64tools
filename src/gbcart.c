#include <stdio.h>
#include <string.h>
#include "gbcart.h"

// Based on pandocs.txt, "The Cartridge Header" section

const char *getCartTypeString(unsigned char type)
{
	// Longest possible string today:
	//
	// "ROM+MBCx+RUMBLE+TIMER+RAM+BATTERY"
	// (33 bytes)
	static char cbuf[64];
	int flags;

	cbuf[0] = 0;

	switch(type)
	{
		case 0xFC: strcat(cbuf, "POCKET CAMERA"); break;
		case 0xFD: strcat(cbuf, "BANDAI TAMA5"); break;
	}

	flags = getGBCartTypeFlags(type);
	if (!GB_MBC_MASK(flags)) {
		strcat(cbuf, "ROM ONLY");
	} else {
		strcat(cbuf, "ROM");

		switch(flags & 0xFF)
		{
			case GB_FLAG_MBC1: strcat(cbuf, "+MBC1"); break;
			case GB_FLAG_MBC2: strcat(cbuf, "+MBC2"); break;
			case GB_FLAG_MBC3: strcat(cbuf, "+MBC3"); break;
			case GB_FLAG_MBC4: strcat(cbuf, "+MBC4"); break;
			case GB_FLAG_MBC5: strcat(cbuf, "+MBC5"); break;
			case GB_FLAG_HUC1: strcat(cbuf, "+HUC1"); break;
			case GB_FLAG_HUC3: strcat(cbuf, "+HUC3"); break;
		}
	}
	if (flags & GB_FLAG_RUMBLE) {
		strcat(cbuf, "+RUMBLE");
	}
	if (flags & GB_FLAG_TIMER) {
		strcat(cbuf, "+TIMER");
	}
	if (flags & GB_FLAG_RAM) {
		strcat(cbuf, "+RAM");
	}
	if (flags & GB_FLAG_BATTERY) {
		strcat(cbuf, "+BATTERY");
	}

	return cbuf;
}

void printGBCartType(unsigned char type)
{
	fputs(getCartTypeString(type), stdout);
}

int getGBCartROMSize(unsigned char code)
{
	// Based on pandocs.txt, "The Cartridge Header" section
	switch(code)
	{
		case 0x00: return 32 * 1024; // No banking
		case 0x01: return 64 * 1024; // 4 banks
		case 0x02: return 128 * 1024; // 8 banks
		case 0x03: return 256 * 1024; // 16 banks
		case 0x04: return 512 * 1024; // 32 banks
		case 0x05: return 1 * 1024 * 1024; // 64 banks (only 63 banks used by MBC1)
		case 0x06: return 2 * 1024 * 1024; // 128 banks (only 125 banks used by MBC1)
		case 0x07: return 4 * 1024 * 1024; // 256 banks
		case 0x52: return 72 * 16 * 1024; // 72 banks
		case 0x53: return 80 * 16 * 1024; // 80 banks
		case 0x54: return 96 * 16 * 1024; // 96 banks
	}
	return 0;
}

int getGBCartRAMSize(unsigned char code)
{
	switch(code)
	{
		case 0x00: return 0;
		case 0x01: return 2048;
		case 0x02: return 8192;
		case 0x03: return 32768; // 4 banks of 8KB
	}
	return 0;
}

int getGBCartTypeFlags(unsigned char type)
{
	// Based on pandocs.txt, "The Cartridge Header" section
	switch(type)
	{
		case 0x00: return 0;
		case 0x01: return GB_FLAG_MBC1;
		case 0x02: return GB_FLAG_MBC1 | GB_FLAG_RAM;
		case 0x03: return GB_FLAG_MBC1 | GB_FLAG_RAM | GB_FLAG_BATTERY;
		case 0x05: return GB_FLAG_MBC2;
		case 0x06: return GB_FLAG_MBC2 | GB_FLAG_BATTERY;
		case 0x08: return GB_FLAG_RAM;
		case 0x09: return GB_FLAG_RAM | GB_FLAG_BATTERY;
		case 0x0B: return GB_FLAG_MMM01;
		case 0x0C: return GB_FLAG_MMM01 | GB_FLAG_RAM;
		case 0x0D: return GB_FLAG_MMM01 | GB_FLAG_RAM | GB_FLAG_BATTERY;
		case 0x0F: return GB_FLAG_MBC3 | GB_FLAG_TIMER | GB_FLAG_BATTERY;
		case 0x10: return GB_FLAG_MBC3 | GB_FLAG_TIMER | GB_FLAG_RAM | GB_FLAG_BATTERY;
		case 0x11: return GB_FLAG_MBC3;
		case 0x12: return GB_FLAG_MBC3 | GB_FLAG_RAM;
		case 0x13: return GB_FLAG_MBC3 | GB_FLAG_RAM | GB_FLAG_BATTERY;
		case 0x15: return GB_FLAG_MBC4;
		case 0x16: return GB_FLAG_MBC4 | GB_FLAG_RAM;
		case 0x17: return GB_FLAG_MBC4 | GB_FLAG_RAM | GB_FLAG_BATTERY;
		case 0x19: return GB_FLAG_MBC5;
		case 0x1A: return GB_FLAG_MBC5 | GB_FLAG_RAM;
		case 0x1B: return GB_FLAG_MBC5 | GB_FLAG_RAM | GB_FLAG_BATTERY;
		case 0x1C: return GB_FLAG_MBC5 | GB_FLAG_RUMBLE;
		case 0x1D: return GB_FLAG_MBC5 | GB_FLAG_RUMBLE | GB_FLAG_RAM;
		case 0x1E: return GB_FLAG_MBC5 | GB_FLAG_RUMBLE | GB_FLAG_RAM | GB_FLAG_BATTERY;
		case 0xFE: return GB_FLAG_HUC3;
		case 0xFF: return GB_FLAG_HUC1 | GB_FLAG_RAM | GB_FLAG_BATTERY;
	}
	return 0;
}

void gbcart_printInfo(const struct gbcart_info *info)
{
	printf("Title: %s\n", info->title);
	printf("Cartridge type: ");
	printGBCartType(info->type);
	printf("\n");

	printf("ROM size: %d bytes (%.2f kB)\n",
								info->rom_size,
								info->rom_size / 1024.0);

	printf("RAM size: %d bytes (%.2f kB)\n",
								info->ram_size,
								info->ram_size / 1024.0);

	if (info->flags & GB_FLAG_JAPANESE_MARKET) {
		printf("Japanese\n");
	} else {
		printf("Non-japanese\n");
	}
}

