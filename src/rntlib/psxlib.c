#include <string.h>
#include <stdio.h>
#include "rnt_priv.h"
#include "psxlib.h"
#include "requests.h"
#include "hexdump.h"

int psxlib_exchange(rnt_hdl_t hdl, unsigned char channel, unsigned char *tx, unsigned char tx_len, unsigned char *rx, unsigned char max_rx)
{
	unsigned char cmd[4 + tx_len];
	unsigned char rep[2 + 64];
	int cmdlen, rx_len, n;

	if (!hdl) {
		return -1;
	}

	if (!tx) {
		return -1;
	}

	cmd[0] = RQ_PSX_RAW;
	cmd[1] = channel;
	cmd[2] = tx_len;
	cmd[3] = max_rx;
	memcpy(cmd+4, tx, tx_len);
	cmdlen = 4 + tx_len;

	printf("psxlib exchange: CMD=");
	printHexBuf(cmd, sizeof(cmd));

	n = rnt_exchange(hdl, cmd, cmdlen, rep, sizeof(rep));
	if (n<0)
		return n;

	printf("psxlib exchange: out %d in %d/%d\n", cmdlen, rep[1], n);

	printf("psxlib exchange: REP=");
	printHexBuf(rep, n);

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

