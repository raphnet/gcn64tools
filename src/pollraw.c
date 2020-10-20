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
#include "delay.h"
#include "maplelib.h"

// when defined, a roll angle is computed for the nunchuk.
// Must also add -lm to the makefile for atan2...
#undef DISPLAY_NUNCHUK_ROLL

#ifdef DISPLAY_NUNCHUK_ROLL
#include <math.h>
#endif

int pollraw_n64(rnt_hdl_t hdl, int chn)
{
	uint8_t getstatus[1] = { N64_GET_STATUS };
	uint8_t status[4];
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
		if (res != 4) {
			printf("Unexpected data length\n");
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

		printf("X: %4d (%3d), Y: %4d (%3d), Buttons=0x%02x%02x   \r",
			(int8_t)(status[2]),
			unique_x_seen,
			(int8_t)(status[3]),
			unique_y_seen,
			status[0], status[1]);
		fflush(stdout);
	}

	return 0;
}


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
	uint8_t answer[9];
	uint16_t id;
	int res;
	uint8_t incfg;

	printf("Suspending polling. Please use --resume_polling later.\n");
	rnt_suspendPolling(hdl, 1);

	printf("Polling PSX controller\n");
	printf("CTRL+C to stop\n");

	res = psxlib_pollStatus(hdl, chn, PSXLIB_PORT_1, 0, 0, &id, answer, sizeof(answer));
	if (res <= 0) {
		printf("Error: psxlib_pollStatus returned %d\n", res);
		return -1;
	}
	printHexBuf(answer, res);

	printf("ID = 0x%04x : %s\n", id, psxlib_idToString(id));

	printf("Trying to enter configuration mode...\n");
	res = psxlib_enterConfigurationMode(hdl, chn, PSXLIB_PORT_1, 1, &incfg);
	if (res < 0) {
		printf("Error: psxlib_enterConfigurationMode returned %d\n", res);
		return -1;
	}

	if (!incfg) {
		printf("Not in configuration mode (real digital pad?)\n");
	} else {
		printf("In configuration mode\n");

		printf("Enabling analog mode\n");
		psxlib_enableAnalog(hdl, chn, PSXLIB_PORT_1, 1);

		printf("Unlocking rumble\n");
		psxlib_unlockRumble(hdl, chn, PSXLIB_PORT_1);

		printf("Exiting configuration mode\n");
		// Exit configuration mode
		psxlib_enterConfigurationMode(hdl, chn, PSXLIB_PORT_1, 0, &incfg);
	}

	while(1)
	{
		int i;

		memset(answer, 0, sizeof(answer));

		for (i=PSXLIB_PORT_1; i<=PSXLIB_PORT_4; i++) {

			res = psxlib_pollStatus(hdl, chn, i, 0x00, 0x00, &id, answer, sizeof(answer));
			if (res <= 0) {
				printf("Error: psxlib_pollStatus returned %d\n", res);
				return -1;
			}

			printf("Port %d : ID = 0x%04x : %s : ", i+1, id, psxlib_idToString(id));
			printHexBuf(answer, res);
		}
		printf("-------------------\n");

		sleep(1);
	}

	return 0;
}

int pollraw_wii(rnt_hdl_t hdl, int chn, int enable_high_res)
{
	uint8_t extmem[256];
	uint16_t ext_id;
	uint8_t status[16], prev_status[16];
	uint8_t high_res = 0;
	int res;
	classic_pad_data pad_data;
	udraw_tablet_data udraw_data;
	drawsome_tablet_data drawsome_data;
	djhero_turntable_data turntable_data;
	nunchuk_pad_data nunchuk_data;

	printf("Polling Wii controller\n");
	printf("CTRL+C to stop\n");

	printf("Suspending polling. Please use --resume_polling later.\n");
	rnt_suspendPolling(hdl, 1);
	_delay_us(40000);

	wusbmotelib_disableEncryption(hdl, chn);
	_delay_us(40000);

	wusbmotelib_dumpMemory(hdl, chn, extmem, 1);

	ext_id = extmem[0xFA] << 8 | extmem[0xFF];
	switch (ext_id)
	{
		case ID_DJHERO: printf("DJ-Hero Turntable detected\n"); break;
		case ID_DRAWSOME: printf("Drawsome tablet detected\n"); break;
		case ID_UDRAW: printf("UDraw tablet detected\n"); break;
		case ID_NUNCHUK: printf("Nunchuk detected\n"); break;
		case ID_GH_GUITAR: printf("Guitar detected\n"); break;
		case ID_CLASSIC: printf("Classic Controller detected\n");
			if (enable_high_res) {
				high_res = 1;
			}
			break;
		case ID_CLASSIC_PRO: printf("Classic Controller Pro detected\n");
			if (enable_high_res) {
				high_res = 1;
			}
			break;
		default:
			printf("Warning: Extension ID 0x%04x not implemented\n", ext_id);
			break;
	}

	// Enable high-resolution mode
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

	if (ext_id == ID_DRAWSOME)
	{
		uint8_t regval;

		printf("Enabling drawsome tablet...");

		regval = 0x01;
		res = wusbmotelib_writeRegs(hdl, chn, 0xFB, &regval, 1);
		if (res) {
			printf("test write failed\n");
			return -1;
		}

		regval = 0x55;
		res = wusbmotelib_writeRegs(hdl, chn, 0xF0, &regval, 1);
		if (res) {
			printf("test write failed\n");
			return -1;
		}

		printf("Done.\n\n");
	}

	while(1)
	{
		int readlen = sizeof(status);

		if (ext_id == ID_DRAWSOME)
			readlen = 6;

		_delay_us(40000);

		res = wusbmotelib_readRegs(hdl, chn, 0, status, readlen);
		if (res < 0) {
			fprintf(stderr, "error reading registers\n");
			return -1;
		}

		// Only display changes
		if (0 == memcmp(prev_status, status, readlen))
			continue;
		memcpy(prev_status, status, readlen);

		printHexBuf(status, readlen);

		switch (ext_id)
		{
			case ID_NUNCHUK:
				wusbmotelib_bufferToNunchukPadData(status, &nunchuk_data);
				printf("SX: %d, XY: %d, AX: %3d, AY: %3d, AZ: %3d\n",
						nunchuk_data.sx, nunchuk_data.sy,
						nunchuk_data.ax, nunchuk_data.ay, nunchuk_data.az);
#ifdef DISPLAY_NUNCHUK_ROLL
				// Z and X for roll
				printf(" Roll angle: %2.1f\n", atan2(nunchuk_data.ax , (double)nunchuk_data.az) / M_PI * 2.0 * 90.0);
#endif
				break;

			case ID_CLASSIC:
			case ID_CLASSIC_PRO:
				wusbmotelib_bufferToClassicPadData(status, &pad_data, ext_id, high_res);

				if (high_res) {
					printf("LX: %4d, LY: %4d, RX: %4d, RY: %4d, LT: %4d, RT: %4d\n",
						pad_data.lx, pad_data.ly, pad_data.rx, pad_data.ry,
						pad_data.lt, pad_data.rt);
				}
				else {
					printf("LX: %4d, LY: %4d, RX: %4d, RY: %4d, LT: %4d, RT: %4d\n",
						pad_data.lx, pad_data.ly, pad_data.rx, pad_data.ry,
						pad_data.lt, pad_data.rt);
				}
				break;

			case ID_UDRAW:
				wusbmotelib_bufferToUdrawData(status, &udraw_data);
				printf("X: %6d, Y: %6d, P=%3d, Buttons: 0x%02x  (%3d %3d)\n",
						udraw_data.x,
						udraw_data.y,
						udraw_data.pressure,
						udraw_data.buttons,
						(udraw_data.x - 90 - (1440-90)/2),
						(udraw_data.y - 90 - (1920-90)/2)

						);
				break;

			case ID_DRAWSOME:
				wusbmotelib_bufferToDrawsomeData(status, &drawsome_data);
				printf("X: %6d, Y: %6d, P=%3d, status: 0x%02x (%s)\n",
						drawsome_data.x,
						drawsome_data.y,
						drawsome_data.pressure,
						drawsome_data.status,
						drawsome_data.status & 0x08 ? "Pen out of range":"Pen in range"
						);

				break;
				
			case ID_DJHERO:
				wusbmotelib_bufferToTurntableData(status, &turntable_data);
				printf("X: %3d, Y: %3d, LTT=%2d, RTT=%2d, Effect=%2d, Crossfade=%2d, Buttons=%04x\n",
					turntable_data.x,
					turntable_data.y,
					turntable_data.left_turntable,
					turntable_data.right_turntable,
					turntable_data.effect_dial,
					turntable_data.crossfade,
					turntable_data.buttons
					);
				
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

		if (res >= 10) {
			// Mouse mode
			if (buf[9]) {
				uint8_t *mouse_data = buf + 10;

				uint8_t buttons = mouse_data[3];
				int8_t x, y;

				x = (mouse_data[4] << 4) | (mouse_data[5] & 0xf);
				y = (mouse_data[6] << 4) | (mouse_data[7] & 0xf);
				x ^= 0xff;
				y ^= 0xff;

				printf("X = %4d  Y= %4d\n", x, y);

				if (!(buttons&0x01)) {
					printf("LEFT ");
				}
				if (!(buttons&0x02)) {
					printf("RIGHT ");
				}
				if (!(buttons&0x04)) {
					printf("CENTER ");
				}
				if (!(buttons&0x08)) {
					printf("START ");
				}
				printf("\n");
			}
		}

		sleep(1);
	}

	return 0;
}

int pollraw_dreamcast_controller(rnt_hdl_t hdl, int chn)
{
	uint8_t buf[64];

	int res;
	int result, datalen;

	printf("Polling DC controller\n");
	printf("CTRL+C to stop\n");

	printf("Suspending polling. Please use --resume_polling later.\n");
	rnt_suspendPolling(hdl, 1);

	while (1)
	{
		//res = maple_getPollData(hdl, chn, buf, sizeof(buf));
		res = maple_sendFrame1W(hdl, chn,
				MAPLE_CMD_GET_CONDITION,
				MAPLE_ADDR_PORTB | MAPLE_ADDR_MAIN,
				MAPLE_ADDR_PORTB | MAPLE_DC_ADDR,
//				MAPLE_FUNC_MOUSE,
				MAPLE_FUNC_CONTROLLER,
				buf, sizeof(buf), MAPLE_FLAG_KEEP_DATA );

		if (res < 0) {
			break;
		}

		if (buf[0] == 0x86)
		{
			if (res < 3) {
				fprintf(stderr, "invalid data\n");
				return -1;
			}

			result = (int16_t)(buf[1] | buf[2] << 8);
			datalen = res - 3;

			printf("Result %d, data[%d] = ", result, datalen);
			if (datalen > 0) {
				printHexBuf(buf + 3, datalen);
			} else {
				printf(" (no data)\n");
			}

		}
		else {
			printHexBuf(buf, res);
		}

		_delay_us(100000);
	}

	return 0;
}

int pollraw_dreamcast_mouse(rnt_hdl_t hdl, int chn)
{
	uint8_t buf[64];

	int res;
	int result, datalen;

	printf("Polling DC mouse\n");
	printf("CTRL+C to stop\n");

	printf("Suspending polling. Please use --resume_polling later.\n");
	rnt_suspendPolling(hdl, 1);

	while (1)
	{
		//res = maple_getPollData(hdl, chn, buf, sizeof(buf));
		res = maple_sendFrame1W(hdl, chn,
				MAPLE_CMD_GET_CONDITION,
				MAPLE_ADDR_PORTB | MAPLE_ADDR_MAIN,
				MAPLE_ADDR_PORTB | MAPLE_DC_ADDR,
				MAPLE_FUNC_MOUSE,
				buf, sizeof(buf), MAPLE_FLAG_KEEP_DATA | MAPLE_FLAG_MOUSE_RX );

		if (res < 0) {
			break;
		}

		if (buf[0] == 0x86)
		{
			if (res < 3) {
				fprintf(stderr, "invalid data\n");
				return -1;
			}

			result = (int16_t)(buf[1] | buf[2] << 8);
			datalen = res - 3;

			printf("Result %d, data[%d] = ", result, datalen);
			if (datalen > 0) {
				printHexBuf(buf + 3, datalen);
			} else {
				printf(" (no data)\n");
			}

		}
		else {
			printHexBuf(buf, res);
		}

		_delay_us(100000);
	}

	return 0;
}

