#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include "perftest.h"
#include "gcn64_protocol.h"
#include "gcn64lib.h"
#include "hexdump.h"

#define N_CYCLES	60
#define N_CYCLESLONG	1200

static long getElaps_us(struct timeval *tv_before, struct timeval *tv_after)
{
	return (tv_after->tv_sec - tv_before->tv_sec)*1000000 + (tv_after->tv_usec - tv_before->tv_usec);
}

static void printTestResult(long total_us, int cycles)
{
	printf("Total time (microseconds): %ld\n", total_us);
	printf("Cycles: %d\n", cycles);
	printf("Average: %ld us\n", total_us / cycles);
}

int perftest1(gcn64_hdl_t hdl, int channel)
{
	int n, i, res, j;
	unsigned char cmd[64];
	char version[64];
	long total_us = 0;
	struct timeval tv_before, tv_after;
	unsigned char cmd_getcaps[1] = { N64_GET_CAPABILITIES };
	unsigned char cmd_getstatus[1] = { N64_GET_STATUS };
	struct blockio_op ops[4] = {
		{ 0, sizeof(cmd_getcaps), cmd_getcaps, 3, cmd + 0 },
		{ 0, sizeof(cmd_getstatus), cmd_getstatus, 4, cmd + 3 },
		{ 1, sizeof(cmd_getcaps), cmd_getcaps, 3, cmd + 7 },
		{ 1, sizeof(cmd_getstatus), cmd_getstatus, 4, cmd + 10 },
	};
	struct blockio_op ops2[2] = {
		{ 0, sizeof(cmd_getstatus), cmd_getstatus, 4, cmd + 0 },
		{ 1, sizeof(cmd_getstatus), cmd_getstatus, 4, cmd + 4 },
	};

	printf("Requesting the firmware version %d times...\n", N_CYCLES);
	for (i=0; i<N_CYCLES; i++) {

		gettimeofday(&tv_before, NULL);
		res = gcn64lib_getVersion(hdl, version, sizeof(version));
		gettimeofday(&tv_after, NULL);

		total_us += getElaps_us(&tv_before, &tv_after);

		if (res != 0) {
			fprintf(stderr, "error during test after %d cycles\n", i);
			return -1;
		}

	//	printf("Version: %s\n", version);
	}

	printTestResult(total_us, N_CYCLES);

	total_us = 0;
	printf("Sending the N64_GET_CAPABILITIES command %d times...\n", N_CYCLES);
	for (i=0; i<N_CYCLES; i++) {
		cmd[0] = N64_GET_CAPABILITIES;
		gettimeofday(&tv_before, NULL);
		n = gcn64lib_rawSiCommand(hdl, channel, cmd, 1, cmd, sizeof(cmd));
		gettimeofday(&tv_after, NULL);
		if (n >= 0) {
			total_us += getElaps_us(&tv_before, &tv_after);
		} else {
			fprintf(stderr, "error during test after %d cycles\n", i);
			return -1;
		}
	}
	printTestResult(total_us, N_CYCLES);


	total_us = 0;
	printf("Doing N64_GET_CAPS + READ_STATUS on controllers 0 and 1 through the block IO api %d times...\n", N_CYCLES);

	for (i=0; i<N_CYCLES; i++) {
		memset(cmd, 0xff, sizeof(cmd));

		gettimeofday(&tv_before, NULL);
		res = gcn64lib_blockIO(hdl, ops, 4);
		gettimeofday(&tv_after, NULL);
		total_us += getElaps_us(&tv_before, &tv_after);

		if (res < 0) {
			fprintf(stderr, "Error in blockIO\n");
			break;
		}
	}

	for (j=0; j<4; j++) {
		printf("blockio[%d]: tx %d bytes, result: 0x%02x, ", j, ops[j].tx_len, ops[j].rx_len);
		if (ops[j].rx_len & BIO_RX_LEN_TIMEDOUT) {
			printf("Timeout\n");
		} else if (ops[j].rx_len & BIO_RX_LEN_PARTIAL) {
			printf("Partial read\n");
		} else {
			printHexBuf(ops[j].rx_data, ops[j].rx_len);
		}
	}

	printTestResult(total_us, N_CYCLES);

	// This test averages at 14665 us when gcn64lib_blockIO_compat() is used.
	// On adapters supporting block IO this averages at 4466 us... More than 3 times faster.

	total_us = 0;
	printf("Doing N64_GET_CAPS + READ_STATUS on controllers 0 and 1 through the block IO api %d times...\n", N_CYCLESLONG);

	for (i=0; i<N_CYCLESLONG; i++) {
		memset(cmd, 0xff, sizeof(cmd));

		gettimeofday(&tv_before, NULL);
		res = gcn64lib_blockIO(hdl, ops2, 2);
		gettimeofday(&tv_after, NULL);
		total_us += getElaps_us(&tv_before, &tv_after);

		if (res < 0) {
			fprintf(stderr, "Error in blockIO\n");
			break;
		}
	}

	for (j=0; j<2; j++) {
		printf("blockio[%d]: tx %d bytes, result: 0x%02x, ", j, ops[j].tx_len, ops[j].rx_len);
		if (ops[j].rx_len & BIO_RX_LEN_TIMEDOUT) {
			printf("Timeout\n");
		} else if (ops[j].rx_len & BIO_RX_LEN_PARTIAL) {
			printf("Partial read\n");
		} else {
			printHexBuf(ops[j].rx_data, ops[j].rx_len);
		}
	}

	printTestResult(total_us, N_CYCLESLONG);




	return 0;
}
