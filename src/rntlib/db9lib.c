#include <stdio.h>
#include <string.h>
#include "requests.h"
#include "hexdump.h"
#include "db9lib.h"

int db9lib_getPollData(rnt_hdl_t hdl, uint8_t chn, uint8_t *dst, int maxlen)
{
	uint8_t requestbuf[63] = { RQ_DB9_RAW };
	uint8_t replybuf[63] = { };
	int n;

#if 0
	printf("DB9 getPollData request: ");
	printHexBuf(requestbuf, sizeof(requestbuf));
#endif

	n = rnt_exchange(hdl, requestbuf, sizeof(requestbuf), replybuf, sizeof(replybuf));
	if (n < 0) {
		return n;
	}

#if 0
	printf("DB9 getPollData answer: ");
	printHexBuf(replybuf, n);
#endif
	if (n > maxlen) {
		fprintf(stderr, "Answer does not fit in maxlen\n");
		return -1;
	}

	memcpy(dst, replybuf, n);

	return n;
}
