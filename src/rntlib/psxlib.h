#ifndef _psxlib_h__
#define _psxlib_h__

#include "raphnetadapter.h"
#include "uiio.h"

#define PSXLIB_ERR_UNKNOWN			-1
#define PSXLIB_ERR_IO_ERROR			-2
#define PSXLIB_ERR_NO_CARD_DETECTED	-3
#define PSXLIB_ERR_NO_COMMAND_ACK	-4
#define PSXLIB_ERR_INVALID_SECTOR	-5
#define PSXLIB_ERR_BAD_CHECKSUM		-6
#define PSXLIB_ERR_INVALID_DATA		-7
#define PSXLIB_ERR_FILE_NOT_FOUND	-8
#define PSXLIB_ERR_FILE_FORMAT_NOT_SUPPORTED	-9
#define PSXLIB_ERR_FILE_READ_ERROR	-10
#define PSXLIB_ERR_USER_CANCELLED	-11

#define PSXLIB_ERR_BUFFER_TOO_SMALL	-100
#define PSXLIB_ERR_BAD_PARAM		-101

#define PSXLIB_MC_TOTAL_SIZE	0x20000
#define PSXLIB_MC_BLOCK_SIZE	0x2000
#define PSXLIB_MC_SECTOR_SIZE	0x80
#define PSXLIB_MC_N_BLOCKS		(PSXLIB_MC_TOTAL_SIZE / PSXLIB_MC_BLOCK_SIZE)
#define PSXLIB_MC_N_SECTORS		(PSXLIB_MC_TOTAL_SIZE / PSXLIB_MC_SECTOR_SIZE)

struct psx_memorycard {
	uint8_t contents[PSXLIB_MC_TOTAL_SIZE];
};

int psxlib_exchange(rnt_hdl_t hdl, unsigned char channel, unsigned char *tx, unsigned char tx_len, unsigned char *rx, unsigned char max_rx);

#define PSXLIB_PORT_1 0
#define PSXLIB_PORT_2 1
#define PSXLIB_PORT_3 2
#define PSXLIB_PORT_4 3

int psxlib_pollStatus(rnt_hdl_t hdl, uint8_t chn, uint8_t port, uint8_t extra1, uint8_t extra2, uint16_t *dst_id, uint8_t *dst_data, int datatmaxlen);
int psxlib_enterConfigurationMode(rnt_hdl_t hdl, uint8_t chn, uint8_t port, uint8_t enter, uint8_t *entered);
int psxlib_unlockRumble(rnt_hdl_t hdl, uint8_t chn, uint8_t port);
int psxlib_enableAnalog(rnt_hdl_t hdl, uint8_t chn, uint8_t port, uint8_t enable);

int psxlib_readMemoryCard(rnt_hdl_t hdl, uint8_t chn, struct psx_memorycard *dst, uiio *u);
int psxlib_readMemoryCardSector(rnt_hdl_t hdl, uint8_t chn, uint16_t sector, uint8_t dst[128]);
int psxlib_writeMemoryCard(rnt_hdl_t hdl, uint8_t chn, const struct psx_memorycard *src, uiio *u);
int psxlib_writeMemoryCardSector(rnt_hdl_t hdl, uint8_t chn, uint16_t sector, const uint8_t data[128]);

#define PSXLIB_FILE_FORMAT_AUTO	-1
#define PSXLIB_FILE_FORMAT_RAW	0 // 128kB headerless image
int psxlib_loadMemoryCardFromFile(const char *filename, int format, struct psx_memorycard *dst_mc_data);
int psxlib_writeMemoryCardToFile(const struct psx_memorycard *mc_data, const char *filename, int format);

#define PSX_CTL_ID_NONE			0xFF
#define PSX_CTL_ID_NEGCON		0x23
#define PSX_CTL_ID_DIGITAL		0x41
#define PSX_CTL_ID_ANALOG_RED	0x73
#define PSX_CTL_ID_ANALOG_GREEN	0x53
#define PSX_CTL_ID_MOUSE		0x12
#define PSX_CTL_ID_CONFIG		0xF3

const char *psxlib_idToString(uint16_t id);
const char *psxlib_getErrorString(int code);

#endif // _psxlib_h__
