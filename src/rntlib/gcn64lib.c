/*	gc_n64_usb : Gamecube or N64 controller to USB adapter firmware
	Copyright (C) 2007-2015  Raphael Assenat <raph@raphnet.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <stdio.h>
#include "rnt_priv.h"
#include "gcn64lib.h"
#include "requests.h"
#include "gcn64_protocol.h"
#include "hexdump.h"


int gcn64lib_rawSiCommand(rnt_hdl_t hdl, unsigned char channel, unsigned char *tx, unsigned char tx_len, unsigned char *rx, unsigned char max_rx)
{
	unsigned char cmd[3 + tx_len];
	unsigned char rep[3 + 64];
	int cmdlen, rx_len, n;

	if (!hdl) {
		return -1;
	}

	if (!tx) {
		return -1;
	}

	cmd[0] = RQ_GCN64_RAW_SI_COMMAND;
	cmd[1] = channel;
	cmd[2] = tx_len;
	memcpy(cmd+3, tx, tx_len);
	cmdlen = 3 + tx_len;

	n = rnt_exchange(hdl, cmd, cmdlen, rep, sizeof(rep));
	if (n<0)
		return n;

	rx_len = rep[2];
	if (rx) {
		memcpy(rx, rep + 3, rx_len);
	}

	return rx_len;
}

int gcn64lib_16bit_scan(rnt_hdl_t hdl, unsigned short min, unsigned short max)
{
	int id, n;
	unsigned char buf[64];

	if (!hdl) {
		return -1;
	}

	for (id = min; id<=max; id++) {
		buf[0] = id >> 8;
		buf[1] = id & 0xff;
		n = gcn64lib_rawSiCommand(hdl, 0, buf, 2, buf, sizeof(buf));
		if (n > 0) {
			printf("CMD 0x%04x answer: ", id);
			printHexBuf(buf, n);
		}
	}

	return 0;
}

int gcn64lib_8bit_scan(rnt_hdl_t hdl, unsigned char min, unsigned char max)
{
	int id, n;
	unsigned char buf[64];

	if (!hdl) {
		return -1;
	}

	for (id = min; id<=max; id++) {
		buf[0] = id;
		n = gcn64lib_rawSiCommand(hdl, 0, buf, 1, buf, sizeof(buf));
		if (n > 0) {
			printf("CMD 0x%02x answer: ", id);
			printHexBuf(buf, n);
		}
	}

	return 0;
}

int gcn64lib_n64_expansionWrite(rnt_hdl_t hdl, unsigned short addr, const unsigned char *data, int len)
{
	unsigned char cmd[3 + len];
	int cmdlen;
	int n;

	if (!hdl) {
		return -1;
	}

	cmd[0] = N64_EXPANSION_WRITE;
	cmd[1] = addr>>8; // Address high byte
	cmd[2] = addr&0xff; // Address low byte
	memcpy(cmd + 3, data, len);
	cmdlen = 3 + len;

	n = gcn64lib_rawSiCommand(hdl, 0, cmd, cmdlen, cmd, sizeof(cmd));
	if (n != 1) {
		printf("expansion write returned != 1 (%d)\n", n);
		return -1;
	}

	return cmd[0];
}

int gcn64lib_n64_expansionRead(rnt_hdl_t hdl, unsigned short addr, unsigned char *dst, int max_len)
{
	unsigned char cmd[3];
	int n;

	if (!hdl) {
		return -1;
	}

	cmd[0] = N64_EXPANSION_READ;
	cmd[1] = addr>>8; // Address high byte
	cmd[2] = addr&0xff; // Address low byte

	n = gcn64lib_rawSiCommand(hdl, 0, cmd, 3, dst, max_len);
	if (n < 0)
		return n;

	return n;
}

static int gcn64lib_blockIO_compat(rnt_hdl_t hdl, struct blockio_op *iops, int n_iops)
{
	int i;
	int res;

	for (i=0; i<n_iops; i++) {
		res = gcn64lib_rawSiCommand(hdl, iops[i].chn, iops[i].tx_data, iops[i].tx_len, iops[i].rx_data, iops[i].rx_len);
		if (res <= 0) {
			// Timeout
			iops[i].rx_len &= BIO_RXTX_MASK;
			iops[i].rx_len |= BIO_RX_LEN_TIMEDOUT;
		} else if (res != iops[i].rx_len) {
			// Less bytes than expected
			iops[i].rx_len &= BIO_RXTX_MASK;
			iops[i].rx_len |= BIO_RX_LEN_TIMEDOUT;
		} else {
			// Read n bytes as expected.
		}
	}

	return 0;
}

int gcn64lib_blockIO(rnt_hdl_t hdl, struct blockio_op *iops, int n_iops)
{
	unsigned char iobuf[63];
	int p, i, n;
	if (!hdl)
		return -1;

	if (!(hdl->caps.features & RNTF_BLOCK_IO)) {
		return gcn64lib_blockIO_compat(hdl, iops, n_iops);
	}
	else {
		// Request format:
		//
		// RQ_GCN64_BLOCK_IO
		// chn, n_tx, n_rx, tx[]
		// ...
		//
		// The adapter stops processing the buffer when the
		// buffer ends or when a channel set to 0xff is encountered.
		//
		memset(iobuf, 0xff, sizeof(iobuf));
		iobuf[0] = RQ_GCN64_BLOCK_IO;
		for (p=1, i=0; i<n_iops; i++) {
			if (p + 3 + iops[i].tx_len > sizeof(iobuf)) {
				fprintf(stderr, "io blocks do not fit in buffer\n");
				return -1;
			}
			iobuf[p] = iops[i].chn;
			p++;
			iobuf[p] = iops[i].tx_len & BIO_RXTX_MASK;
			p++;
			iobuf[p] = iops[i].rx_len & BIO_RXTX_MASK;
			p++;

			memcpy(iobuf + p, iops[i].tx_data, iops[i].tx_len);
			p += iops[i].tx_len;
		}
#ifdef DEBUG_BLOCKIO
		fputs("blockIO request: ", stdout);
		printHexBuf(iobuf, sizeof(iobuf));
#endif

		n = rnt_exchange(hdl, iobuf, sizeof(iobuf), iobuf, sizeof(iobuf));
		if (n < 0) {
			return n;
		}
		if (n != sizeof(iobuf)) {
			fprintf(stderr, "Unexpected iobuf reply size\n");
			return -1;
		}
#ifdef DEBUG_BLOCKIO
		fputs("blockIO answer.: ", stdout);
		printHexBuf(iobuf, sizeof(iobuf));
		printf("\n");
#endif
		// Answer format:
		//
		// RQ_GCN64_BLOCK_IO
		// n_rx, rx[n_rx]
		// ...
		//
		// n_rx will have bits set according to the result. See BIO_RX*. rx will always
		// occupy the number of bytes set in the request, regardless of the result.
		//
		if (iobuf[0] != RQ_GCN64_BLOCK_IO) {
			fprintf(stderr, "Invalid iobuf reply\n");
			return -1;
		}

		for (p=1,i=0; i<n_iops; i++) {
			if (p >= sizeof(iobuf)) {
				fprintf(stderr, "blockIO: adapter reports too much received data\n");
				break;
			}

			iops[i].rx_len = iobuf[p];
			p++;
			if (p + (iops[i].rx_len & BIO_RXTX_MASK) >= sizeof(iobuf)) {
				fprintf(stderr, "blockIO: adapter reports too much received data\n");
				break;
			}
			memcpy(iops[i].rx_data, iobuf + p, iops[i].rx_len & BIO_RXTX_MASK);
			p += iops[i].rx_len & BIO_RXTX_MASK;
		}
	}

	return 0;
}

