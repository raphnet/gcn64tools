#include <stdio.h>
#include <string.h>
#include "usbtest.h"
#include "requests.h"
#include "hexdump.h"

#define TESTBUF_SIZE	63

#define LFSR_SEED	0xACE1
static uint16_t lfsr = LFSR_SEED;

static void lfsr_clock(void)
{
	unsigned lsb = lfsr & 1;   /* Get LSB (i.e., the output bit). */
	lfsr >>= 1;                /* Shift register */
	if (lsb)                   /* If the output bit is 1, apply toggle mask. */
	lfsr ^= 0xB400u;
}

static void fill_pseudoRandom(uint8_t *dst, int len)
{
	int i;

	for (i=0; i<len; i++) {
		dst[i] = lfsr;
		lfsr_clock();
	}
}


int usbtest(rnt_hdl_t hdl, int verbose)
{
	uint8_t outbuf[TESTBUF_SIZE] = { RQ_RNT_ECHO };
	uint8_t inbuf[TESTBUF_SIZE];
	int res;

	fill_pseudoRandom(outbuf + 1, sizeof(outbuf) - 1);

	res = rnt_exchange(hdl, outbuf, sizeof(outbuf), inbuf , sizeof(inbuf));
	if (res != sizeof(inbuf)) {
		printf("usbtest: exchange failed: %d\n", res);
		return res;
	}

	if (memcmp(outbuf, inbuf, sizeof(outbuf))) {
		printf("usbtest: Error, test buffer data mismatch\n");
		printf("outbuf: "); printHexBuf(outbuf, sizeof(outbuf));
		printf(" inbuf: "); printHexBuf(inbuf, sizeof(inbuf));
		return 1;
	}

	printf("ok: "); printHexBuf(inbuf, sizeof(inbuf));

	return 0;
}
