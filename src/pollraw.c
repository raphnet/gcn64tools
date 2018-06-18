#include <stdio.h>
#include <string.h>
#include "raphnetadapter.h"
#include "gcn64lib.h"
#include "wusbmotelib.h"
#include "gcn64_protocol.h"
#include "pollraw.h"
#include "hexdump.h"
#include "psxlib.h"
#include "db9lib.h"
#include "sleep.h"

int pollraw_gamecube(rnt_hdl_t hdl, int chn)
{
	uint8_t getstatus[3] = { GC_GETSTATUS1, GC_GETSTATUS2, 0x00 };
	uint8_t status[8];
	int res;
	int unique_x_seen = 0;
	uint8_t seen_x_values[256] = { };
	int unique_y_seen = 0;
	uint8_t seen_y_values[256] = { };

	printf("Suspending polling. Please use --resume_polling later.\n");
	rnt_suspendPolling(hdl, 1);

	printf("CTRL+C to stop\n");
	while(1)
	{
		res = gcn64lib_rawSiCommand(hdl, chn, getstatus, sizeof(getstatus), status, sizeof(status));
		if (res != GC_GETSTATUS_REPLY_LENGTH) {
			printf("Not enough data received\n");
			break;
		}

		if (!seen_x_values[status[2]]) {
			seen_x_values[status[2]] = 1;
			unique_x_seen++;
		}
		if (!seen_y_values[status[3]]) {
			seen_y_values[status[3]] = 1;
			unique_y_seen++;
		}

		printf("X: %4d (%3d), Y: %4d (%3d), CX: %4d, CY: %4d, LT: %4d, RT: %4d\r",
			(int8_t)(status[2]+0x80),
			unique_x_seen,
			(int8_t)(status[3]+0x80),
			unique_y_seen,
			(int8_t)(status[4]+0x80), (int8_t)(status[5]+0x80),
			status[6], status[7]);
		fflush(stdout);
	}

	return 0;
}

int pollraw_gamecube_keyboard(rnt_hdl_t hdl, int chn)
{
	uint8_t getstatus[3] = { 0x54, 0x00, 0x00 };
	uint8_t status[8];
	int res;
	int i;
	uint8_t lrc;
	uint8_t prev_keys[3] = { };
	uint8_t active_keys[256] = { };

	printf("Suspending polling. Please use --resume_polling later.\n");
	rnt_suspendPolling(hdl, 1);

	printf("Polling gamecube keyboard.\n");
	printf("CTRL+C to stop\n");
	while(1)
	{
		res = gcn64lib_rawSiCommand(hdl, chn, getstatus, sizeof(getstatus), status, sizeof(status));
		if (res != sizeof(status)) {
			printf("Not enough data received\n");
			break;
		}

		// Status
		//       Bit
		// Byte  7    | 6    | 5  |  4  |  3    2    1    0   |
		//    0  ERR  | ERRL | ?  |  ?  |  Sequence counter   |
		//    1  ????????
		//    2  ????????
		//    3  ????????
		//    4  Keycode 1
		//    5  Keycode 2
		//    6  Keycode 3
		//    7  LRC (XOR of bytes 0-6)
		//
		for (i=0, lrc=0; i<sizeof(status)-1; i++) {
			lrc ^= status[i];
		}
		if (status[7] != lrc) {
			printf("LRC error! ");
			printHexBuf(status, res);
		}

		if (status[4] == 0x02 && status[5] == 0x02 && status[6] == 0x02) {
			printf("Too many keys down\n");
		}

		// Detect changes
		if (memcmp(prev_keys, status + 4, sizeof(prev_keys))) {
			for (i=1; i<sizeof(active_keys); i++) {
				// If a given key is reported active by the keyboard...
				if (memchr(status+4, i, 3)) {
					// ... and we do not already know:
					if (!active_keys[i]) {
						printf("KEY 0x%02x down\n", i);
						active_keys[i] = 1;
					}
				}
				else {
					// If a key is not reported active by the keyboard
					// but it was in the previous iteration
					if (active_keys[i]) {
						printf("KEY 0x%02x up\n", i);
						active_keys[i] = 0;
					}
				}
			}
		}
		memcpy(prev_keys, status + 4, sizeof(prev_keys));
	}

	return 0;
}

int pollraw_randnet_keyboard(rnt_hdl_t hdl, int chn)
{
	uint8_t getstatus[] = { 0x13, 0x00 };
	uint8_t status[7];
	uint8_t prev_status[7] = { };
	int res;
	int i;

	printf("Suspending polling. Please use --resume_polling later.\n");
	rnt_suspendPolling(hdl, 1);
	printf("Polling randnet keyboard...\n");
	printf("CTRL+C to stop\n");

	while(1)
	{
		res = gcn64lib_rawSiCommand(hdl, chn, getstatus, sizeof(getstatus), status, sizeof(status));
		if (res != sizeof(status)) {
			printf("Not enough data received (Expected %d bytes but got %d)\n", (int)sizeof(status), res);
			break;
		}

		// Big thanks to fraser125 for figuring this out, sharing it and testing!
		//
		// https://sites.google.com/site/consoleprotocols/home/nintendo-joy-bus-documentation/randnet-keyboard
		//

		// Status
		//       Bit
		// Byte  7  | 6 | 5 | 4    | 3 | 2 | 1 | 0        |
		//    0  Keycode 1 MSB                            |
		//    1  Keycode 1 LSB                            |
		//    2  Keycode 2 MSB                            |
		//    3  Keycode 2 LSB                            |
		//    4  Keycode 3 MSB                            |
		//    5  Keycode 3 LSB                            |
		//    6  ? | ? | ? | Error | ? | ? | ? | Home key |
		//
		// Up to 3 keys can be pressed. When inactive, the keycodes
		// are simply set to zero.
		//

		if (memcmp(prev_status, status, sizeof(status))) {
			printf("---------------------\n");
			printf("Raw: ");
			printHexBuf(status, res);
			printf("Active keys(s): ");
			for (i=0; i<3; i++) {
				uint16_t keycode = status[i*2] << 8 | status[i*2+1];
				if (keycode) {
					printf("%04x ", keycode);
				}
			}
			printf("\n");
			printf("Status: %02x %s%s\n",
						status[6],
						status[6] & 0x10 ? "Error ":"",
						status[6] & 0x01 ? "Home key":"");

		}
		memcpy(prev_status, status, sizeof(status));
	}

	return 0;
}

int pollraw_psx(rnt_hdl_t hdl, int chn)
{
	uint8_t request[2] = { 0x01, 0x42 };
	uint8_t answer[9];
	int res;

	printf("Suspending polling. Please use --resume_polling later.\n");
	rnt_suspendPolling(hdl, 1);

	printf("Polling PSX controller\n");
	printf("CTRL+C to stop\n");

	while(1)
	{
		memset(answer, 0, sizeof(answer));

		res = psxlib_exchange(hdl, chn, request, sizeof(request), answer, sizeof(answer));
		if (res <= 0) {
			printf("Error: psxlib_exchange returned %d\n", res);
			break;
		}

		printHexBuf(answer, res);
		break;
	}

	return 0;
}

int pollraw_wii(rnt_hdl_t hdl, int chn)
{
	uint8_t extmem[256];
	uint16_t ext_id;
	uint8_t status[16], prev_status[16];
	uint8_t high_res = 0;
	int res;
	classic_pad_data pad_data;

	printf("Polling PSX controller\n");
	printf("CTRL+C to stop\n");

	printf("Suspending polling. Please use --resume_polling later.\n");
	rnt_suspendPolling(hdl, 1);

	wusbmotelib_disableEncryption(hdl, chn);
	wusbmotelib_dumpMemory(hdl, chn, extmem, 1);

	ext_id = extmem[0xFA] << 8 | extmem[0xFF];
	switch (ext_id)
	{
		case ID_NUNCHUK: printf("Nunchuk detected\n"); break;
		case ID_CLASSIC: printf("Classic Controller detected\n"); break;
		case ID_CLASSIC_PRO: printf("Classic Controller Pro detected\n"); break;
		default:
			printf("Extension ID 0x%04x not implemented\n", ext_id); return 0;
		break;
	}

	// Enable high-resolution mode
	high_res = 1;
	if (high_res)
	{
		uint8_t regval = 0x03;
		printf("Enabling high resolution reports\n");
		res = wusbmotelib_writeRegs(hdl, chn, 0xFE, &regval, 1);
		if (res) {
			printf("Error enabling high resolution reports\n");
			return -1;
		}
	}

	while(1)
	{
		res = wusbmotelib_readRegs(hdl, chn, 0, status, sizeof(status));
		if (res < 0) {
			fprintf(stderr, "error reading registers\n");
			return -1;
		}

		// Only display changes
		if (0 == memcmp(prev_status, status, sizeof(status)))
			continue;
		memcpy(prev_status, status, sizeof(status));

		printHexBuf(status, sizeof(status));

		switch (ext_id)
		{
			case ID_CLASSIC:
			case ID_CLASSIC_PRO:
				wusbmotelib_bufferToClassicPadData(status, &pad_data, ext_id, high_res);
				printf("LX: %4d, LY: %4d, RX: %4d, RY: %4d, LT: %4d, RT: %4d\n",
						pad_data.lx, pad_data.ly, pad_data.rx, pad_data.ry,
						pad_data.lt, pad_data.rt);

				break;
		}
	}

	return 0;
}

int pollraw_db9(rnt_hdl_t hdl, int chn)
{
	uint8_t buf[32];
	int res;

	printf("Polling DB9 controller\n");
	printf("CTRL+C to stop\n");

	printf("Suspending polling. Please use --resume_polling later.\n");
	rnt_suspendPolling(hdl, 1);

	while (1)
	{
		res = db9lib_getPollData(hdl, chn, buf, sizeof(buf));
		if (res < 0) {
			break;
		}

		printHexBuf(buf, res);

		sleep(1);
	}

	return 0;
}

