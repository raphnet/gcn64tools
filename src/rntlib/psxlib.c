#include <string.h>
#include <stdio.h>
#include "rnt_priv.h"
#include "psxlib.h"
#include "requests.h"
#include "hexdump.h"

// #define DEBUG_EXCHANGES

int psxlib_exchange(rnt_hdl_t hdl, unsigned char channel, unsigned char *tx, unsigned char tx_len, unsigned char *rx, unsigned char max_rx)
{
	unsigned char cmd[4 + tx_len];
	unsigned char rep[2 + 64];
	int cmdlen, rx_len, n;

	if (!hdl) {
		return -1;
	}

	if (!tx && tx_len > 0) {
		return -1;
	}

	cmd[0] = RQ_PSX_RAW;
	cmd[1] = channel;
	cmd[2] = tx_len;
	cmd[3] = max_rx;
	if (tx) {
		memcpy(cmd+4, tx, tx_len);
		cmdlen = 4 + tx_len;
	}

#ifdef DEBUG_EXCHANGES
	printf("psxlib exchange: CMD=");
	printHexBuf(cmd, sizeof(cmd));
#endif

	n = rnt_exchange(hdl, cmd, cmdlen, rep, sizeof(rep));
	if (n<0)
		return n;

#ifdef DEBUG_EXCHANGES
	printf("psxlib exchange: out %d in %d/%d\n", cmdlen, rep[1], n);
	printf("psxlib exchange: REP=");
	printHexBuf(rep, n);
#endif

	rx_len = rep[1];
	if (rx_len > max_rx) {
		fprintf(stderr, "Received too much data - truncated\n");
		rx_len = max_rx;
	}
	if (rx) {
		memcpy(rx, rep + 2, rx_len);
	}

	return rx_len;
}

static uint8_t xorbuf(const uint8_t *buf, int len)
{
	uint8_t r;
	while (len--) {
		r ^= *buf++;
	}
	return r;
}

int psxlib_readMemoryCard(rnt_hdl_t hdl, uint8_t chn, struct psx_memorycard *dst)
{
	uint16_t sector;
	int res;

	for (sector = 0; sector < PSXLIB_MC_N_SECTORS; sector++) {
		res = psxlib_readMemoryCardSector(hdl, chn, sector, dst->contents + sector * PSXLIB_MC_SECTOR_SIZE);
		if (res)
			return res;
	}

	return 0;
}

int psxlib_readMemoryCardSector(rnt_hdl_t hdl, uint8_t chn, uint16_t sector, uint8_t dst[128])
{
	uint8_t request[6] = {
		0x81, 0x52, 0x00, 0x00,
		sector >> 8, sector & 0xff
	};
	uint8_t inbuf[140];
	int res;
	uint16_t confirmed_sector;
	uint8_t chk;

	// Out:
	//   81 52 00 00 MSB LSB lots_of_zeros[]
	// In:
	//
	//   N/A FLAG 5A 5D 00 00 5C 5D MSB LSB data[128] CHK 47H
	//

	// Hardware limitations (maximum USB endpoint size in micro-controller)
	// makes it impossible to read 128 bytes in one go.
	//
	// And due to the adapter protocol overhead, the maximum is 60. Three exchanges
	// are therefore required.


	// A total of 140 bytes should be sent.
	//
	// First send 7 bytes. (the pause after this helps for the extra delay after 5C is sent.
	// Then 60
	// Then 60
	// Then 13
	//
	//
	res = psxlib_exchange(hdl, (0x01 << 4) | chn, request, sizeof(request), inbuf, 7);
	if (res < 0) {
		return PSXLIB_ERR_IO_ERROR;
	}
	if (res != 7) {
		printf("Incorrect length. Expected 7, received %d\n", res);
		return PSXLIB_ERR_IO_ERROR;
	}

	res = psxlib_exchange(hdl, (0x01 << 4) | chn, NULL, 0, inbuf + 7, 60);
	if (res < 0) {
		return PSXLIB_ERR_IO_ERROR;
	}
	if (res != 60) {
		printf("Incorrect length. Expected 60, received %d\n", res);
		return PSXLIB_ERR_IO_ERROR;
	}

	res = psxlib_exchange(hdl, (0x01 << 4) | chn, NULL, 0, inbuf + 7 + 60, 60);
	if (res < 0) {
		return PSXLIB_ERR_IO_ERROR;
	}
	if (res != 60) {
		printf("Incorrect length. Expected 60, received %d\n", res);
		return PSXLIB_ERR_IO_ERROR;
	}

	// This last exchange does not set the "NO DESELECT" flag
	res = psxlib_exchange(hdl, chn, NULL, 0, inbuf + 7 + 60 + 60, 13);
	if (res < 0) {
		return PSXLIB_ERR_IO_ERROR;
	}
	if (res != 13) {
		printf("Incorrect length. Expected 13, received %d\n", res);
		return PSXLIB_ERR_IO_ERROR;
	}


	printf(" **** Sector 0x%04x: ", sector);
	printHexBuf(inbuf, sizeof(inbuf));

	/* Now a few checks */

	// Check for memory card ID
	if (inbuf[2] != 0x5A ||
		inbuf[3] != 0x5D)
	{
		return PSXLIB_ERR_NO_CARD_DETECTED;
	}

	// Check for command acknowledge
	if (inbuf[6] != 0x5C ||
		inbuf[7] != 0x5D)
	{
		return PSXLIB_ERR_NO_COMMAND_ACK;
	}

	// Make sure confirmed sector matches
	confirmed_sector = inbuf[8] << 8 | inbuf[9];
	if (confirmed_sector != sector) {
		return PSXLIB_ERR_INVALID_SECTOR;
	}

	// Validate the checksum
	chk = xorbuf(inbuf + 8, 2 + 128 + 1);
	if (chk != 0) {
		return PSXLIB_ERR_BAD_CHECKSUM;
	}

	// Make sure the 'G' at the end is present
	if (inbuf[139] != 'G') {
		return PSXLIB_ERR_UNKNOWN;
	}

	memcpy(dst, inbuf + 10, PSXLIB_MC_SECTOR_SIZE);

	return 0;
}

int psxlib_writeMemoryCardToFile(const struct psx_memorycard *mc_data, const char *filename, int format)
{
	FILE *fptr;

	if (!filename) {
		printf("PSX Memory Card contents: ");
		printHexBuf(mc_data->contents, PSXLIB_MC_TOTAL_SIZE);
		return 0;
	}

	fptr = fopen(filename, "wb");
	if (!fptr) {
		perror(filename);
		return -1;
	}

	if (1 != fwrite(mc_data->contents, PSXLIB_MC_TOTAL_SIZE, 1, fptr)) {
		perror("error writing memory card data");
		fclose(fptr);
		return -2;
	}

	fclose(fptr);

	return 0;
}

