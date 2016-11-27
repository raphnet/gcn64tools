#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include "perftest.h"
#include "gcn64_protocol.h"
#include "gcn64lib.h"

#define N_CYCLES	60

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
	int n, i, res;
	unsigned char cmd[64];
	char version[64];
	long total_us = 0;
	struct timeval tv_before, tv_after;

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

	return 0;
}
