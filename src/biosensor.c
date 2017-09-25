#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "gcn64lib.h"
#include "mempak_gcn64usb.h"
#include "hexdump.h"

/**
 * \brief Poll the bio sensor once.
 *
 * \return 0: Not in beat, 32: In beat, [1..31] maybe?
 */
int gcn64lib_biosensorPoll(rnt_hdl_t hdl)
{
	unsigned char buf[32];
	int res, i;

	res = gcn64lib_mempak_readBlock(hdl, 0xC000, buf);
	if (res < 0) {
		return res;
	}

	/* Between beats, the sensor returs a buffer full of 0x03:
	 *
	 *    03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03
	 *
	 * At each pulse, the sensor returns a buffer full of 0x00:
	 *
	 *    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
	 *
	 * But when sampled right at the beginning of a pulse, the buffer can be part 0x00s, and part 0x03s.
	 *
	 * Occasionally there is noise and there may be short sequences of 0x03 or 0x00 that appear. The approach
	 * here is to count the number of 0x03 (in pulse) and return it.
	 *
	 **/

	for (res=32, i=0; i<sizeof(buf); i++) {
		if (buf[i] == 0x03) {
			res--;
		} else if (buf[i] != 0x00) {
			fprintf(stderr, "Sensor reports unknown/bad status");
			printHexBuf(buf, 32);
			return -1;
		}
	}

	return res;
}

#define MIN_PULSE_LENGTH	5
#define AVERAGES			3

int gcn64lib_biosensorMonitor(rnt_hdl_t hdl)
{
	int res;
	int in_pulse = 0;
	struct timeval prev_pulse, cur_pulse;
	int first = 1;
	int averages[AVERAGES];
	int avg_cur = 0;

	/* Stating the obvious(tm) */
	printf("********************************************\n");
	printf("*******  WARNING  WARNING  WARNING  ********\n");
	printf("********************************************\n");
	printf("* The Biosensor is a video game accessory. *\n");
	printf("* It is NOT a medical device and is for    *\n");
	printf("* entertainment purposes only.             *\n");
	printf("********************************************\n");
	printf("\n");
	printf("Bio sensor heart-beat monitor started\n");

	while(1)
	{
		usleep(16000);

		res = gcn64lib_biosensorPoll(hdl);
		if (res < 0) {
			return res;
		}

		/* Ignore partial pulses and some noise... */
		if (res > 1 && res < 32) {
			continue;
		}

		if (res) {
			in_pulse++;
			if (in_pulse < MIN_PULSE_LENGTH)
				continue;
			if (in_pulse > MIN_PULSE_LENGTH)
				continue;

			gettimeofday(&cur_pulse, NULL);

			if (!first) {
				int elaps_ms, i, avg_bpm;

				elaps_ms = (cur_pulse.tv_sec - prev_pulse.tv_sec) * 1000 + (cur_pulse.tv_usec - prev_pulse.tv_usec) / 1000;

				averages[avg_cur] = 60000 / elaps_ms;
				avg_cur++;
				if (avg_cur >= AVERAGES)
					avg_cur = 0;

				for (i=0,avg_bpm=0; i < AVERAGES; i++) {
					avg_bpm += averages[i];
				}
				avg_bpm /= AVERAGES;

				printf("BPM: %d\n", avg_bpm);
				fflush(stdout);
			}
			else {
				first = 0;
			}
			memcpy(&prev_pulse, &cur_pulse, sizeof(struct timeval));
		} else {
			in_pulse = 0;
		}

	}

/*
	while(1)
	{
		res = gcn64lib_biosensorPoll(hdl);
		if (res < 0) {
			return res;
		}

		switch (res)
		{
			case 0: fputs("0", stdout); break;
			case 1 ... 31: fputs("?", stdout); break;
			case 32: fputs("1", stdout); break;
		}

		fflush(stdout);
		usleep(16000);
	}
*/
}


