#ifndef _gbcart_h__
#define _gbcart_h__

#define GB_FLAG_MBC1	0x0001
#define GB_FLAG_MBC2	0x0002
#define GB_FLAG_MMM01	0x0004
#define GB_FLAG_MBC3	0x0008
#define GB_FLAG_MBC4	0x0010
#define GB_FLAG_MBC5	0x0020
#define GB_FLAG_HUC1	0x0040
#define GB_FLAG_HUC3	0x0080

#define GB_FLAG_JAPANESE_MARKET	0x0100

#define GB_FLAG_RUMBLE	0x1000
#define GB_FLAG_TIMER	0x2000
#define GB_FLAG_BATTERY	0x4000
#define	GB_FLAG_RAM		0x8000

#define GB_MBC_MASK(a)	((a) & 0xFF)

struct gbcart_info {
	char title[17]; // Zero-terminated string
	unsigned char type;
	unsigned int rom_size;
	unsigned int ram_size;
	int flags;
};

/** \brief Return a string describing the cartridge type.
 *
 * \return Pointer to string. Will be overwritten by subsequent calls.
 */
const char *getCartTypeString(unsigned char type);
void printGBCartType(unsigned char type);
int getGBCartROMSize(unsigned char code);
int getGBCartRAMSize(unsigned char code);
int getGBCartTypeFlags(unsigned char type);
void gbcart_printInfo(const struct gbcart_info *info);


#endif // _gbcart_h__
