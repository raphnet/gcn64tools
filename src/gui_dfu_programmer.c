#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "gcn64ctl_gui.h"
#include "gui_dfu_programmer.h"

/**
 * \brief Execute a process, reading its output and returning the exit status.
 *
 * \param command The shell command to execute
 * \param lineCallback Callback called for each line read from the process.
 * \param ctx Context passed to the callback
 * \return DFU_OK, DFU_POPEN_FAILED or DFU_ERROR_RETURN
 */
int dfu_wrapper(const char *command, void (*lineCallback)(void *ctx, const char *ln), void *ctx)
{
	FILE *dfu_fp;
	int res;
	char linebuf[256];

	updatelog_append("popen(%s)\n", command);
	dfu_fp = popen(command, "r");
	if (!dfu_fp) {
		updatelog_append("Could not run dfu-programmer to erase chip (%s)", strerror(errno));
		return DFU_POPEN_FAILED;
	}

	do {
		char *nl = "\n";

		fgets(linebuf, sizeof(linebuf), dfu_fp);

		if (strrchr(linebuf, '\n'))
			nl = "";
		if (strrchr(linebuf, '\r'))
			nl = "";

		updatelog_append("[dfu-update message] : %s%s", linebuf, nl);

		if (lineCallback) {
			lineCallback(linebuf, ctx);
		}

	} while(!feof(dfu_fp));

	res = pclose(dfu_fp);
	if (res < 0) {
		updatelog_append("Pclose: %d (%s)\n", res, strerror(errno));
	} else {
		updatelog_append("Pclose: %d\n", res);
	}
#ifdef WINDOWS
	/* Mingw does not seem to provide the WIFEXITED/WEXITSTATUS macros.... */
	if (res!=0) {
		return DFU_ERROR_RETURN;
	}
#else
	if (!WIFEXITED(res) || (WEXITSTATUS(res)!=0)) {
		return DFU_ERROR_RETURN;
	}
#endif

	return DFU_OK;
}
