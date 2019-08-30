#include <string.h>
#include <stdio.h>
#include "rnt_priv.h"
#include "psxlib.h"
#include "requests.h"
#include "hexdump.h"

//#define DEBUG_EXCHANGES
//#define DISABLE_COMMAND_ACK_CHECK

int psxlib_exchange(rnt_hdl_t hdl, unsigned char channel, unsigned char *tx, unsigned char tx_len, unsigned char *rx, unsigned char max_rx)
{
	unsigned char cmd[4 + tx_len];
	unsigned char rep[2 + 64];
	int cmdlen = 0, rx_len, n;

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
	uint8_t r = 0;
	while (len--) {
		r ^= *buf++;
	}
	return r;
}

int psxlib_readMemoryCard(rnt_hdl_t hdl, uint8_t chn, struct psx_memorycard *dst, uiio *u)
{
	uint16_t sector;
	int res;

	u = getUIIO(u);

	u->cur_progress = 0;
	u->max_progress = PSXLIB_MC_N_SECTORS;
	u->progress_type = PROGRESS_TYPE_ADDRESS;
	u->caption = "Reading memory card...";
	u->progressStart(u);

	for (sector = 0; sector < PSXLIB_MC_N_SECTORS; sector++) {
		res = psxlib_readMemoryCardSector(hdl, chn, sector, dst->contents + sector * PSXLIB_MC_SECTOR_SIZE);
		if (res) {
			u->progressEnd(u, "Error");
			return res;
		}

		u->cur_progress = sector;
		if (u->update(u)) {
			u->progressEnd(u, "Aborted");
			return PSXLIB_ERR_USER_CANCELLED;
		}
	}

	u->progressEnd(u, "Done");

	return 0;
}

int psxlib_writeMemoryCard(rnt_hdl_t hdl, uint8_t chn, const struct psx_memorycard *dst, uiio *u)
{
	uint16_t sector;
	int res;

	u = getUIIO(u);

	u->cur_progress = 0;
	u->max_progress = PSXLIB_MC_N_SECTORS;
	u->progress_type = PROGRESS_TYPE_ADDRESS;
	u->caption = "Writing to memory card...";
	u->progressStart(u);

	for (sector = 0; sector < PSXLIB_MC_N_SECTORS; sector++) {
		res = psxlib_writeMemoryCardSector(hdl, chn, sector, dst->contents + sector * PSXLIB_MC_SECTOR_SIZE);
		if (res) {
			u->progressEnd(u, "Error");
			return res;
		}

		u->cur_progress = sector;
		if (u->update(u)) {
			res = u->ask(UIIO_NOYES, "If you interrupt the transfer, some or all saves on your memory card will be corrupted.\n\nReally stop?");
			if (res == UIIO_YES) {
				u->progressEnd(u, "Aborted");
				return PSXLIB_ERR_USER_CANCELLED;
			}
		}
	}

	u->progressEnd(u, "Done");

	return 0;
}

int psxlib_writeMemoryCardSector(rnt_hdl_t hdl, uint8_t chn, uint16_t sector, const uint8_t data[128])
{
	uint8_t request[128+10] = {
		0x81, 'W', 0x00, 0x00, sector >> 8, sector & 0xff
	};
	uint8_t inbuf[128+10];
	uint8_t flgchn;
	int res;

	memcpy(request + 6, data, 128);

	// Xor of sector address and data bytes
	request[134] = xorbuf(request + 4, 2+128);

	//printf("Out: "); printHexBuf(request, sizeof(request));

	flgchn = chn | (FLG_NO_DESELECT << 4);
	res = psxlib_exchange(hdl, flgchn, request + 0, 50, inbuf + 0, 50);
	if (res < 0) {
		return PSXLIB_ERR_IO_ERROR;
	}
	if (res != 50) {
		printf("Incorrect length. Expected %d, received %d\n", 50, res);
		return PSXLIB_ERR_IO_ERROR;
	}

	res = psxlib_exchange(hdl, flgchn, request + 50, 50, inbuf + 60, 50);
	if (res < 0) {
		return PSXLIB_ERR_IO_ERROR;
	}
	if (res != 50) {
		printf("Incorrect length. Expected %d, received %d\n", 50, res);
		return PSXLIB_ERR_IO_ERROR;
	}

	flgchn = chn | (FLG_POST_DELAY << 4);
	res = psxlib_exchange(hdl, flgchn, request + 50 + 50, 38, inbuf + 50 + 50, 38);
	if (res < 0) {
		return PSXLIB_ERR_IO_ERROR;
	}
	if (res != 38) {
		printf("Incorrect length. Expected %d, received %d\n", 38, res);
		return PSXLIB_ERR_IO_ERROR;
	}


	//printf("In: "); printHexBuf(inbuf, sizeof(inbuf));

	// Now check if it worked.

	// Memory card ID?
	if (inbuf[2] != 0x5A) {
		return PSXLIB_ERR_NO_CARD_DETECTED;
	}
	if (inbuf[3] != 0x5D) {
		return PSXLIB_ERR_NO_CARD_DETECTED;
	}

#ifndef DISABLE_COMMAND_ACK_CHECK
	// Acknowledge?
	if (inbuf[135] != 0x5C) {
		return PSXLIB_ERR_NO_COMMAND_ACK;
	}
	if (inbuf[136] != 0x5D) {
		return PSXLIB_ERR_NO_COMMAND_ACK;
	}
#endif

	// Examine the end byte
	switch(inbuf[137])
	{
		case 0x4E: return PSXLIB_ERR_BAD_CHECKSUM;
		case 0xFF: return PSXLIB_ERR_INVALID_SECTOR;
		default: return PSXLIB_ERR_UNKNOWN;

		// OK
		case 0x47: return 0;
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
	int res, done;
	uint16_t confirmed_sector;
	uint8_t chk;
	int todo, chunksize;
	uint8_t flgchn;
	int txlen;

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

	todo = 140;
	done = 0;
	do
	{
		chunksize = todo;
		if (chunksize > 60) {
			chunksize = 60;
		}

		flgchn = chn;

		// First exchange?
		if (done == 0) {
			txlen = sizeof(request);
			// insert a delay (or expect a longer time before the ACK) before the 8th byte
			// (command ACKnowlege)
			flgchn |= FLG_LATE_8TH<<4;
		} else {
			// Keep sending zero while we are just reading
			txlen = 0;
		}

		// Last exchange?
		if (chunksize < todo) {
			// Make sure the cart stays selected until the end
			flgchn |= FLG_NO_DESELECT<<4;
		}

		res = psxlib_exchange(hdl, flgchn, request, txlen, inbuf + done, chunksize);
		if (res < 0) {
			return PSXLIB_ERR_IO_ERROR;
		}
		if (res != chunksize) {
			printf("Incorrect length. Expected %d, received %d\n", chunksize, res);
			return PSXLIB_ERR_IO_ERROR;
		}

		done += chunksize;
		todo -= chunksize;
	}
	while (todo > 0);

	/* Now a few checks */

	// Check for memory card ID
	if (inbuf[2] != 0x5A ||
		inbuf[3] != 0x5D)
	{
		return PSXLIB_ERR_NO_CARD_DETECTED;
	}

#ifndef DISABLE_COMMAND_ACK_CHECK
	// Check for command acknowledge
	if (inbuf[6] != 0x5C ||
		inbuf[7] != 0x5D)
	{
		return PSXLIB_ERR_NO_COMMAND_ACK;
	}
#endif

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

int psxlib_loadMemoryCardFromFile(const char *filename, int format, struct psx_memorycard *dst_mc_data)
{
	FILE *fptr;
	long filesize;

	if (!dst_mc_data || !filename) {
		return PSXLIB_ERR_BAD_PARAM;
	}

	fptr = fopen(filename, "rb");
	if (!fptr) {
		return PSXLIB_ERR_FILE_NOT_FOUND;
	}

	fseek(fptr, 0, SEEK_END);
	filesize = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);

	if (filesize != PSXLIB_MC_TOTAL_SIZE) {
		fclose(fptr);
		return PSXLIB_ERR_FILE_FORMAT_NOT_SUPPORTED;
	}

	if (1 != fread(dst_mc_data->contents, PSXLIB_MC_TOTAL_SIZE, 1, fptr)) {
		fclose(fptr);
		return PSXLIB_ERR_FILE_READ_ERROR;
	}

	fclose(fptr);

	return 0;
}

const char *psxlib_idToString(uint16_t id)
{
	switch(id)
	{
		case PSX_CTL_ID_NEGCON: return "Negcon";
		case PSX_CTL_ID_DIGITAL: return "Digital pad";
		case PSX_CTL_ID_ANALOG_RED: return "Analog pad (red)";
		case PSX_CTL_ID_CONFIG: return "Config mode";
		case PSX_CTL_ID_NONE: return "No controller";
	}
	return "(unknown)";
}

const char *psxlib_getErrorString(int code)
{
	if (code >=0)
		return "OK";

	switch (code)
	{
		case PSXLIB_ERR_IO_ERROR: return "IO Error";
		case PSXLIB_ERR_NO_CARD_DETECTED: return "No card detected";
		case PSXLIB_ERR_NO_COMMAND_ACK: return "No command ACK";
		case PSXLIB_ERR_INVALID_SECTOR: return "Invalid sector";
		case PSXLIB_ERR_BAD_CHECKSUM: return "Bad checksum";
		case PSXLIB_ERR_INVALID_DATA: return "Invalid data";
		case PSXLIB_ERR_FILE_NOT_FOUND: return "File not found / no access";
		case PSXLIB_ERR_BUFFER_TOO_SMALL: return "Buffer too small";
		case PSXLIB_ERR_FILE_FORMAT_NOT_SUPPORTED: return "File format not supported";
		case PSXLIB_ERR_USER_CANCELLED: return "Cancelled";
	}

	return "(unknown - internal error)";
}

/** \brief Read controller button/axis status
 *
 * \param hdl The adapter handle
 * \param chn The channel (adapter ports)
 * \param port The port number (in multitap)
 * \param dst_ud Pointer to an uint16_t to hold the controller ID received. May be NULL.
 * \param dst_data Pointer to buffer for status data (what comes after the ID). May be NULL.
 */
int psxlib_pollStatus(rnt_hdl_t hdl, uint8_t chn, uint8_t port, uint8_t extra1, uint8_t extra2, uint16_t *dst_id, uint8_t *dst_data, int datatmaxlen)
{
	uint8_t cmd_poll[] = { 0x01+port, 0x42, 0x00, extra1, extra2 };
	uint8_t answer[9];
	uint16_t id;
	int res;

	res = psxlib_exchange(hdl, chn, cmd_poll, sizeof(cmd_poll), answer, sizeof(answer));
	if (res <= 0) {
		return PSXLIB_ERR_IO_ERROR;
	}
	if (res >= 3) {
		id = answer[1]; // | answer[2]<<8;
	} else {
		return PSXLIB_ERR_INVALID_DATA;
	}

	if (dst_data)
	{
		if (datatmaxlen < res - 3) {
			return PSXLIB_ERR_BUFFER_TOO_SMALL;
		}

		memcpy(dst_data, answer + 3, res - 3);
	}

	if (dst_id)
		*dst_id = id;

	return res - 3;
}

int psxlib_enterConfigurationMode(rnt_hdl_t hdl, uint8_t chn, uint8_t port, uint8_t enter, uint8_t *in_cfg)
{
	uint8_t cmd_enter_config[9] = { 0x01 + port, 0x43, 0x00, enter ? 0x01 : 0x00 };
	uint8_t answer[9];
	uint16_t id;
	int res;

	res = psxlib_exchange(hdl, chn, cmd_enter_config, sizeof(cmd_enter_config), answer, sizeof(answer));
	if (res <=0 ) {
		return -1;
	}

	// Check if it worked (ID must change)
	res = psxlib_pollStatus(hdl, chn, PSXLIB_PORT_1, 0, 0, &id, NULL, 0);
	if (res < 0) {
		return -1;
	}

	if (in_cfg) {
		*in_cfg = (id == PSX_CTL_ID_CONFIG) ? 0x01 : 0x00;
	}

	return 0;
}

/** Enable/Disable analog mode (and the LED)
 * \param Enter analog mode if non-zero
 */
int psxlib_enableAnalog(rnt_hdl_t hdl, uint8_t chn, uint8_t port, uint8_t enable)
{
	uint8_t cmd_enter_analog[] = { 0x01 + port, 0x44, 0x00, enable ? 0x01 : 0x00, 0x02 };
	uint8_t answer[9];
	int res;

	res = psxlib_exchange(hdl, chn, cmd_enter_analog, sizeof(cmd_enter_analog), answer, sizeof(answer));
	return res;
}

int psxlib_unlockRumble(rnt_hdl_t hdl, uint8_t chn, uint8_t port)
{
	uint8_t cmd_unlock_rumble[] = { 0x01 + port, 0x4D, 0x00,
			0x00, 0x01, 0xff, 0xff, 0xff, 0xff
	};
	uint8_t answer[9];
	int res;

	res = psxlib_exchange(hdl, chn, cmd_unlock_rumble, sizeof(cmd_unlock_rumble), answer, sizeof(answer));

	return res;
}


