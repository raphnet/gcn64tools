#define _GNU_SOURCE // for memmem
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ihex.h"

#ifdef WINDOWS
#include "memmem.h"
#endif

#define IHEX_MAX_FILE_SIZE	0x20000

char check_ihex_for_signature(const char *filename, const char *signature)
{
	unsigned char *buf;
	int max_address;

	buf = malloc(IHEX_MAX_FILE_SIZE);
	if (!buf) {
		perror("malloc");
		return 0;
	}

	max_address= load_ihex(filename, buf, IHEX_MAX_FILE_SIZE);

	if (max_address > 0) {
		if (!memmem(buf, max_address + 1, signature, strlen(signature))) {
			return 0;
		}
		return 1;
	}

	return 0;
}


