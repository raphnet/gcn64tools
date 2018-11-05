/*	Raphnet adapter management tool
	Copyright (C) 2007-2017  Raphael Assenat <raph@raphnet.net>

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

#include <stdio.h>
#include <string.h>
#include "wusbmotelib.h"
#include "requests.h"
#include "hexdump.h"

#undef DEBUG_REQUEST_BUFFERS
#undef DEBUG_REQUEST_BUILDING

int wusbmote_i2c_transactions(rnt_hdl_t hdl, struct i2c_transaction *transactions, int n_transactions)
{
	int i, p, n;
	uint8_t requestbuf[63];
	struct i2c_transaction *txn = transactions;

	// Request format:
	//
	//   RQ_WUSBMOTE_I2C_TRANSACTIONS
	//   repeat 1...n {
	//      chn
	//      addr
	//      wr_len
	//      rd_len
	//      wr_data[len]
	//   }
	//

	// first, mark all transaction as incomplete
	for (i=0; i<n_transactions; i++) {
		transactions[i].result = 0xff;
	}

	// Build the request
	memset(requestbuf, 0, sizeof(requestbuf));
	requestbuf[0] = RQ_WUSBMOTE_I2C_TRANSACTIONS;
	for (p=1, i=0; i<n_transactions; i++, txn++) {
		if (p + 4 + txn->wr_len > sizeof(requestbuf)) {
			fprintf(stderr, "transactions do not fit in buffer\n");
			return -1;
		}

		requestbuf[p++] = txn->chn;
		requestbuf[p++] = txn->addr;
		requestbuf[p++] = txn->wr_len;
		requestbuf[p++] = txn->rd_len;
		if (txn->wr_len) {
			memcpy(requestbuf + p, txn->wr_data, txn->wr_len);
			p += txn->wr_len;
		}

#ifdef DEBUG_REQUEST_BUILDING
		printf("  Adding -> channel %d: addr=0x%02x, rd_len: %d, wr_len: %d, wr_data:",
					txn->chn, txn->addr, txn->rd_len, txn->wr_len);
		printHexBuf(txn->wr_data, txn->wr_len);
#endif
	}

#ifdef DEBUG_REQUEST_BUFFERS
	printf("I2C request: ");
	printHexBuf(requestbuf, sizeof(requestbuf));
#endif

	n = rnt_exchange(hdl, requestbuf, sizeof(requestbuf), requestbuf, sizeof(requestbuf));
	if (n < 0) {
		return n;
	}
	if (n != sizeof(requestbuf)) {
		fprintf(stderr, "Unexpected reply size\n");
		return -1;
	}

#ifdef DEBUG_REQUEST_BUFFERS
	printf("I2C answer: ");
	printHexBuf(requestbuf, sizeof(requestbuf));
#endif

	// Answer format:
	//
	//   RQ_WUSBMOTE_I2C_TRANSACTIONS
	//   repeat 1...n {
	//     RESULT (0 = ok)
	//     ACTUAL_READ_LEN (omitted if result non-zero)
	//     read_data[]
	//   }
	//

	if (requestbuf[0] != RQ_WUSBMOTE_I2C_TRANSACTIONS) {
		fprintf(stderr, "Unexpected reply type\n");
		return -1;
	}

	txn = transactions;
	for (i=0, p=1; i<n_transactions; i++, txn++) {
		if (p >= sizeof(requestbuf)) {
			fprintf(stderr, "Error: Ran out of answer data\n");
			return -1;
		}

		txn->result = requestbuf[p++];
		if (txn->result == 0) {
			// Success means a length and (probably) data
			if (p >= sizeof(requestbuf)) {
				fprintf(stderr, "Error: Ran out of answer data\n");
				return -1;
			}

			if (requestbuf[p] > txn->rd_len) {
				fprintf(stderr, "Error: I2C read longer than requested\n");
				return -1;
			}

			txn->rd_len = requestbuf[p++];
			if (p + txn->rd_len >= sizeof(requestbuf)) {
				fprintf(stderr, "Error: Ran out of answer data\n");
				return -1;
			}

			memcpy(txn->rd_data, requestbuf + p, txn->rd_len);
			p += txn->rd_len;
		} else {
			// i2c transaction failed (no answer)
			txn->rd_len = 0;
		}

#ifdef DEBUG_REQUEST_BUILDING
		printf("  Result 0x%02x on channel %d: addr=0x%02x, rd_len: %d, rd_data: ",
					txn->result,
					txn->chn, txn->addr, txn->rd_len);
		printHexBuf(txn->rd_data, txn->rd_len);
#endif
	}

	return 0;
}

/** Scan all I2C addresses to detect chip presence
 *
 * \param hdl The adapter handle
 * \param chn The channel
 * \param dstBuf Array of 128 values indicating presence (non-zero) or absence. Set to NULL if not needed.
 * \param verbose When non-zero, displays scan result in i2cdetect-style format.
 */
int wusbmotelib_i2c_detect(rnt_hdl_t hdl, uint8_t chn, uint8_t *dstBuf, char verbose)
{
	int i, j, res;
	uint8_t buf;
	uint8_t addresses[128];
	int addr_min = 0;
	int addr_max = 0x7f;

	memset(addresses, -1, sizeof(addresses));

	if (verbose) {
		printf("Scanning I2C address from 0x%02x to 0x%02x...\n", addr_min, addr_max);
	}

	for (i=addr_min; i<=addr_max; i++) {
		struct i2c_transaction txn = {
			.chn = chn,
			.wr_len = 0,
			.rd_len = 1,
			.rd_data = &buf,
			.addr = i,
		};

		res = wusbmote_i2c_transaction(hdl, &txn);
		if (res < 0) {
			fprintf(stderr, "error executing transaction\n");
			return -1;
		}

		addresses[i] = txn.result == 0;
	}

	if (verbose) {
		printf("    ");
		for (i=0; i<16; i++) {
			printf("%x  ", i);
		}
		printf("\n");

		for (i=0; i<=0x70; i+=0x10) {
			printf("%02x: ", i);
			for (j=i; j<i+0x10; j++) {
				if (addresses[j]) {
					printf("%02x ", j);
				} else {
					printf("-- ");
				}
			}
			printf("\n");
		}
	}

	if (dstBuf) {
		memcpy(dstBuf, addresses, sizeof(addresses));
	}

	return 0;
}

int wusbmotelib_disableEncryption(rnt_hdl_t hdl, uint8_t chn)
{
	uint8_t step1[2] = { 0xF0, 0x55 }; // Write 0x55 to register 0xF0
	uint8_t step2[2] = { 0xFB, 0x00 }; // Write 0x00 to register 0xFB
	struct i2c_transaction txns[2] = {
		{ .chn = chn, .wr_len = 2, .wr_data = step1, .addr = 0x52, },
		{ .chn = chn, .wr_len = 2, .wr_data = step2, .addr = 0x52, },
	};
	int res;

	res = wusbmote_i2c_transactions(hdl, txns, 2);
	if (res < 0) {
		fprintf(stderr, "error executing transaction\n");
		return -1;
	}

	if (txns[0].result || txns[1].result) {
		fprintf(stderr, "One or more I2C writes failed\n");
		return -1;
	}

	return 0;
}

int wusbmotelib_dumpMemory(rnt_hdl_t hdl, uint8_t chn, uint8_t *dstBuf, char verbose)
{
	int i,j,res;
	uint8_t memory[256] = { };

	for (i=0x0; i<0x100; i+=0x10) {
		uint8_t reg = i;
		struct i2c_transaction txn = {
			.chn = chn,
			.wr_len = 1, .wr_data = &reg,
			.rd_len = 0x10, .rd_data = memory + i,
			.addr = 0x52,
		};

		res = wusbmote_i2c_transaction(hdl, &txn);
		if (res < 0) {
			fprintf(stderr, "error executing transaction\n");
			return -1;
		}

		if (txn.result) {
			fprintf(stderr, "I2C transaction failed\n");
			return -1;
		}
	}

	if (verbose) {
		printf("    ");
		for (i=0; i<16; i++) {
			printf("%x  ", i);
		}
		printf("\n");

		for (i=0; i<0x100; i+=0x10) {
			printf("%02x: ", i);
			for (j=0; j<0x10; j++) {
				printf("%02x ", memory[i+j]);
			}
			printf("\n");
		}
	}

	if (dstBuf) {
		memcpy(dstBuf, memory, sizeof(memory));
	}

	return 0;
}

int wusbmotelib_readRegs(rnt_hdl_t hdl, uint8_t chn, uint8_t start_reg, uint8_t *dstBuf, int n_regs)
{
	struct i2c_transaction txn = {
		.chn = chn,
		.wr_len = 1, .wr_data = &start_reg,
		.rd_len = n_regs, .rd_data = dstBuf,
		.addr = 0x52,
	};
	int res;

	if (n_regs > 256 || n_regs < 0) {
		return -1;
	}

	res = wusbmote_i2c_transaction(hdl, &txn);
	if (res < 0) {
		return res;
	}

	if (txn.result) {
		return -1;
	}

	return 0;
}

int wusbmotelib_writeRegs(rnt_hdl_t hdl, uint8_t chn, uint8_t start_reg, const uint8_t *srcBuf, int n_regs)
{
	uint8_t tmpbuf[n_regs + 1];
	struct i2c_transaction txn = {
		.chn = chn,
		.wr_len = 1 + n_regs, .wr_data = tmpbuf,
		.rd_len = 0,
		.addr = 0x52,
	};
	int res;

	if (n_regs > 256 || n_regs < 0) {
		return -1;
	}

	tmpbuf[0] = start_reg;
	memcpy(tmpbuf + 1, srcBuf, n_regs);

	res = wusbmote_i2c_transaction(hdl, &txn);
	if (res < 0) {
		return res;
	}

	if (txn.result) {
		return -1;
	}

	return 0;
}


void wusbmotelib_bufferToClassicPadData(const uint8_t *buf, classic_pad_data *dst, uint16_t peripheral_id, uint8_t high_res_mode)
{
	char mask_rx = 0;

	switch(peripheral_id) {
		case ID_GH_GUITAR:
			mask_rx = 1;
			default:
		case ID_CLASSIC: dst->pad_type = PAD_TYPE_CLASSIC; break;
		case ID_CLASSIC_PRO: dst->pad_type = PAD_TYPE_CLASSIC_PRO; break;
	}

	if (high_res_mode) {
		// High resolution mode used by the NES/SNES Classic editions
		//
		// Bit    7    6    5    4    3    2    1    0
		// Byte
		// 0      LX<7:0>
		// 1      RX<7:0>
		// 2      LY<7:0>
		// 3      RY<7:0>
		// 4      LT<7:0>
		// 5      RT<7:0>
		// 6      BDR  BDD  BLT  B-   BH   B+   BRT  1
		// 7      BZL  BB   BY   BA   BX   BZR  BDL  BDU

		dst->lx = buf[0] - 0x80;
		dst->rx = buf[1] - 0x80;
		dst->ly = buf[2] - 0x80;
		dst->ry = buf[3] - 0x80;
		dst->lt = buf[4];
		dst->rt = buf[5];
		dst->buttons = ~(buf[6] << 8 | buf[7]);
		dst->high_resolution = 1;
	}
	else {
		// Reference: http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Classic_Controller
		//
		//     7        6     5    4    3     2     1     0
		// 0   RX<4:3>        LX<5:0>
		// 1   RX<2:1>        LY<5:0>
		// 2   RX<0>    LT<4:3>    RY<4:0>
		// 3   LT<2:0>             RT<4:0>
		// 4   BDR      BDD   BLT  B-   BH    B+    BRT   1
		// 5   BZL      BB    BY   BA   BX    BZR   BDL   BDU
		//
		dst->lx = (buf[0] & 0x3f) - 32;
		dst->ly = (buf[1] & 0x3f) - 32;
		dst->rx = (((buf[0] & 0xc0) >> 3) | ((buf[1] & 0xc0) >> 5) | (buf[2]>>7)) - 16;
		dst->ry = (buf[2] & 0x1f) - 16;
		dst->lt = ((buf[3]>>5) | ((buf[2] & 0x60) >> 2) );
		dst->rt = buf[3]&0x1f;
		dst->buttons = ~(buf[4] << 8 | buf[5]);

		if (mask_rx) {
			dst->rx = 0;
		}
		dst->high_resolution = 0;
	}
}


void wusbmotelib_bufferToUdrawData(const uint8_t *buf, udraw_tablet_data *dst)
{
	dst->x = ((buf[2] & 0x0f) << 8) | buf[0];
	dst->y = ((buf[2] & 0xf0) << 4) | buf[1];
	dst->pressure = buf[3];
	dst->buttons = buf[5] ^ 0xfb;
}

