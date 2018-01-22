#include <stdio.h>
#include "raphnetadapter.h"
#include "gcn64lib.h"
#include "wusbmotelib.h"
#include "gcn64_protocol.h"
#include "pollraw.h"

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
