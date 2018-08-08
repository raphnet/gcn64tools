#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pcelib.h"
#include "requests.h"
#include "hexdump.h"

#undef DEBUG_REQUEST_BUFFERS

#define BTN_XE1AP_START		0x0001
#define BTN_XE1AP_SELECT	0x0002
#define BTN_XE1AP_A			0x0004
#define BTN_XE1AP_B			0x0008
#define BTN_XE1AP_C			0x0010
#define BTN_XE1AP_D			0x0020
#define BTN_XE1AP_A2		0x0040
#define BTN_XE1AP_B2		0x0080
#define BTN_XE1AP_E1		0x0100
#define BTN_XE1AP_E2		0x0200

struct xe1ap_report {
	uint16_t buttons;
	uint8_t x,y;
	uint8_t throttle;
};

void print_xe1ap_buttons(uint16_t btn)
{
	if (btn & BTN_XE1AP_A)
		printf("A ");
	if (btn & BTN_XE1AP_B)
		printf("B ");
	if (btn & BTN_XE1AP_C)
		printf("C ");
	if (btn & BTN_XE1AP_D)
		printf("D ");
	if (btn & BTN_XE1AP_START)
		printf("START ");
	if (btn & BTN_XE1AP_SELECT)
		printf("SELECT ");
	if (btn & BTN_XE1AP_A2)
		printf("A' ");
	if (btn & BTN_XE1AP_B2)
		printf("B' ");
	if (btn & BTN_XE1AP_E1)
		printf("E1 ");
	if (btn & BTN_XE1AP_E2)
		printf("E2 ");
}

char parse_xe1ap_report(const uint8_t *raw, struct xe1ap_report *rp)
{
	if ((raw[5] & 0xf) != 0x0F) {
		return -1;
	}
	if ((raw[4] & 0xf) != 0x00) {
		return -1;
	}
	if ((raw[2] & 0xf) != 0x00) {
		return -1;
	}
	rp->y = (raw[1] & 0xf0) | (raw[3] >> 4);
	rp->x = (raw[1] << 4) | (raw[3] & 0xf);
	rp->throttle = (raw[2] & 0xf0) | (raw[4] >> 4);

	rp->buttons = 0;

	// A and A' also control raw[0] & 0x80
	// B and B' also control raw[0] & 0x40
	if (0 == (raw[5] & 0x80)) {
		rp->buttons |= BTN_XE1AP_A;
	}
	if (0 == (raw[5] & 0x40)) {
		rp->buttons |= BTN_XE1AP_B;
	}
	if (0 == (raw[5] & 0x20)) {
		rp->buttons |= BTN_XE1AP_A2;
	}
	if (0 == (raw[5] & 0x10)) {
		rp->buttons |= BTN_XE1AP_B2;
	}

	if (0 == (raw[0] & 0x20)) {
		rp->buttons |= BTN_XE1AP_C;
	}
	if (0 == (raw[0] & 0x10)) {
		rp->buttons |= BTN_XE1AP_D;
	}

	if (0 == (raw[0] & 0x02)) {
		rp->buttons |= BTN_XE1AP_START;
	}
	if (0 == (raw[0] & 0x01)) {
		rp->buttons |= BTN_XE1AP_SELECT;
	}

	if (0 == (raw[0] & 0x08)) {
		rp->buttons |= BTN_XE1AP_E1;
	}
	if (0 == (raw[0] & 0x04)) {
		rp->buttons |= BTN_XE1AP_E2;
	}

	return 0;
}

int pcelib_rawpoll(rnt_hdl_t hdl)
{
	uint8_t requestbuf[63] = { RQ_PCENGINE_RAW };
	uint8_t replybuf[63] = { RQ_PCENGINE_RAW };
	struct xe1ap_report xe1ap;
	int n;
	int i;

	while(1)
	{
		usleep(100000);

#ifdef DEBUG_REQUEST_BUFFERS
		printf("PCE rawpoll request: ");
		printHexBuf(requestbuf, sizeof(requestbuf));
#endif

		n = rnt_exchange(hdl, requestbuf, sizeof(requestbuf), replybuf, sizeof(replybuf));
		if (n < 0) {
			return n;
		}
		if (n != sizeof(replybuf)) {
			fprintf(stderr, "Unexpected reply size\n");
			return -1;
		}

#ifdef DEBUG_REQUEST_BUFFERS
		printf("PCE rawpoll answer: ");
		printHexBuf(replybuf, sizeof(replybuf));
#endif

		if (replybuf[0] == 0x8D) {
			printf("RX timeout\n");
			continue;
		}

		for (i=0; i<16; i++) {
			printf("%02x ", replybuf[i+1]);
		}

		if (parse_xe1ap_report(replybuf + 1, &xe1ap) != 0) {
			uint8_t a, b, t;
			static int sixbutton_mode = 0, prev_sixbutton_mode = 0;

			a = replybuf[1 + 6];
			b = replybuf[1 + 7];

			// So, the values are different, which is a good sign.
			// Also, there is a set of "4 bits low" in only one
			// of the bytes.
			if ((a != b) && (a == 0xF0) && (b == 0xFF)) {
				sixbutton_mode = 1;
			}
			if ((a != b) && (a == 0xFF) && (b == 0xF0)) {
				sixbutton_mode = 1;
			}
			// No "4 bits low" at all? Surely a 3 button controller
			if ((a & 0xf) && (b & 0xf)) {
				sixbutton_mode = 0;
			}

			if (prev_sixbutton_mode != sixbutton_mode) {
				printf("Six button mode: %d\n", sixbutton_mode);
			}

			if (sixbutton_mode) {
				// hack: unswap bits
				if (!(a & 0xf)) {
				 	t = a;
					a = b;
					b = t;
				}
			}

			printf("PCE Standard: ");
			if (!(a & 0x01)) { printf("Up "); }
			if (!(a & 0x02)) { printf("Down "); }
			if (!(a & 0x04)) { printf("Left "); }
			if (!(a & 0x08)) { printf("Right "); }
			if (!(a & 0x10)) { printf("I "); }
			if (!(a & 0x20)) { printf("SELECT "); }
			if (!(a & 0x40)) { printf("RUN "); }
			if (!(a & 0x80)) { printf("II "); }

			if (sixbutton_mode) {
				if (!(b & 0x10)) { printf("III "); }
				if (!(b & 0x20)) { printf("V "); }
				if (!(b & 0x40)) { printf("VI "); }
				if (!(b & 0x80)) { printf("IV "); }
			}

			printf("\n");
			continue;
		}

		printf(" ");
		printf("X: %02x, Y: %02x, throttle: %02x", xe1ap.x, xe1ap.y, xe1ap.throttle);
		printf(", buttons: ");
		print_xe1ap_buttons(xe1ap.buttons);
		printf("\n");

	}

	return 0;
}
