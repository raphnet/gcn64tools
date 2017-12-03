#include "timer.h"
#include <time.h>
#include <unistd.h>

#ifdef WINDOWS
#include <windows.h>
#endif

uint64_t getMilliseconds()
{
#ifndef WINDOWS
	struct timespec time_now;
	clock_gettime(CLOCK_MONOTONIC, &time_now);
	return time_now.tv_sec * 1000 + time_now.tv_nsec / 1000 / 1000;
#else
	return GetTickCount64();
#endif
}


#ifdef TEST_TIMER
#include <stdio.h>
#include <inttypes.h>
// gcc timer.c -DTEST_TIMER -o ttest -Wall
int main(void)
{
	uint64_t start, now;

	start = getMilliseconds();
	usleep(100000); // 100ms
	now = getMilliseconds();

	if ((now-start) < 99 && (now-start) > 101) {
		printf("Error: 100ms timed as %" PRId64 "\n", (now-start));
		return 1;
	}

	return 0;
}
#endif
