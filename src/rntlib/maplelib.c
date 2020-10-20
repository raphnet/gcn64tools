#include <stdio.h>
#include <string.h>
#include "requests.h"
#include "hexdump.h"
#include "db9lib.h"

int maple_sendFrame1W(rnt_hdl_t hdl, uint8_t chn, uint8_t cmd, uint8_t dst_addr, uint8_t src_addr, uint32_t data, uint8_t *dst, int maxlen, uint8_t flags)
{
	uint8_t requestbuf[63] = { RQ_MAPLE_RAW };
	uint8_t replybuf[63] = { };
	int n;

	requestbuf[1] = cmd;
	requestbuf[2] = dst_addr;
	requestbuf[3] = src_addr;
	requestbuf[4] = data;
	requestbuf[5] = data >> 8;
	requestbuf[6] = data >> 16;
	requestbuf[7] = data >> 24;
	requestbuf[8] = flags;

#if 0
	printf("Maple raw request: ");
	printHexBuf(requestbuf, sizeof(requestbuf));
#endif

	n = rnt_exchange(hdl, requestbuf, sizeof(requestbuf), replybuf, sizeof(replybuf));
	if (n < 0) {
		return n;
	}

#if 0
	printf("Maple answer: ");
	printHexBuf(replybuf, n);
#endif
	if (n > maxlen) {
		fprintf(stderr, "Answer does not fit in maxlen\n");
		return -1;
	}

	memcpy(dst, replybuf, n);

	return n;

}

int maple_getPollData(rnt_hdl_t hdl, uint8_t chn, uint8_t *dst, int maxlen)
{
	uint8_t requestbuf[63] = { RQ_MAPLE_RAW };
	uint8_t replybuf[63] = { };
	int n;


#if 0
	printf("Maple raw request: ");
	printHexBuf(requestbuf, sizeof(requestbuf));
#endif

	n = rnt_exchange(hdl, requestbuf, sizeof(requestbuf), replybuf, sizeof(replybuf));
	if (n < 0) {
		return n;
	}

#if 0
	printf("Maple answer: ");
	printHexBuf(replybuf, n);
#endif
	if (n > maxlen) {
		fprintf(stderr, "Answer does not fit in maxlen\n");
		return -1;
	}

	memcpy(dst, replybuf, n);

	return n;
}
