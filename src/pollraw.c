#include <stdio.h>
#include <string.h>
#include "raphnetadapter.h"
#include "gcn64lib.h"
#include "wusbmotelib.h"
#include "gcn64_protocol.h"
#include "pollraw.h"
#include "hexdump.h"

int pollraw_gamecube(rnt_hdl_t hdl, int chn)
{
	uint8_t getstatus[3] = { GC_GETSTATUS1, GC_GETSTATUS2, 0x00 };
	uint8_t status[8];
	int res;
	int unique_x_seen = 0;
	uint8_t seen_x_values[256] = { };
	int unique_y_seen = 0;
	uint8_t seen_y_values[256] = { };


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

	printf("Polling randnet keyboard (experimental, not tested).\n");
	printf("CTRL+C to stop\n");
	while(1)
	{
		res = gcn64lib_rawSiCommand(hdl, chn, getstatus, sizeof(getstatus), status, sizeof(status));
		if (res != sizeof(status)) {
			printf("Not enough data received (Expected %d bytes but got %d)\n", (int)sizeof(status), res);
			break;
		}

		// Thanks to https://sites.google.com/site/consoleprotocols/home/nintendo-joy-bus-documentation/randnet-keyboard
		//
		// Note: Untested

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
		// Up to 3 keys can be pressed. When inactive, the key
		// CMD 0x1300 answer: 00 00 00 00 00 00 00
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
			printf("Status: %02x %s%s",
						status[6],
						status[6] & 0x10 ? "Error ":"",
						status[6] & 0x01 ? "Home key":"");

		}
		memcpy(prev_status, status, sizeof(status));
	}

	return 0;


	return 0;
}
