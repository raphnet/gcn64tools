#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "uiio.h"

static int uiio_std_ask(int type, const char *fmt, ...)
{
	va_list ap;
	int c, def;

	printf("\033[33;1mWARNING:\033[0m ");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	fflush(stdin);

	if (type == UIIO_YESNO) {
		def = UIIO_YES;
		printf(" [Y/n]");
		c = getchar();


	} else {
		def = UIIO_NO;
		printf(" [y/N]");
		c = getchar();


	}

	if (c == 'y' || c == 'Y')
		return UIIO_YES;
	if (c == 'n' || c == 'N')
		return UIIO_NO;

	return def;
}

static void uiio_std_perror(const char *s)
{
	perror(s);
}

static int uiio_std_error(const char *fmt, ...)
{
	va_list ap;
	int i;

	va_start(ap, fmt);
	i = vfprintf(stderr, fmt, ap);
	va_end(ap);

	return i;
}

static int uiio_std_printf(const char *fmt, ...)
{
	va_list ap;
	int i;

	va_start(ap, fmt);
	i = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return i;
}

static void uiio_std_update_start(uiio *u)
{
	fputs("\n", stdout);
	u->progress_status = UIIO_PROGRESS_STARTED;
}

static void uiio_std_update_end(uiio *u, const char *msg)
{
	if (u->progress_status < UIIO_PROGRESS_STARTED)
		return;

	printf("\n%s\n", msg);
	u->progress_status = UIIO_PROGRESS_STOPPED;
}

static int uiio_std_update(uiio *u)
{
	int i;
	float progress_pc;

	if (u->progress_status < UIIO_PROGRESS_STARTED)
		return 0;

	progress_pc = u->cur_progress / (float)u->max_progress * 100.0;

	if (u->progress_type == PROGRESS_TYPE_ADDRESS) {
		printf("\r%s : [", u->caption);

		for (i=0; i<20; i++) {
			if (i * 100 / 20.0 <= progress_pc) {
				fputs("#", stdout);
			} else {
				fputs(" ", stdout);
			}
		}

		printf("] 0x%04x / 0x%04x (%.1f%%)",
			u->cur_progress,
			u->max_progress,
			progress_pc	);
	} else {
		// percent
		printf("%s : %.2f%%\r",
			u->caption,
			u->cur_progress / (float)u->max_progress * 100.0);

	}

	fflush(stdout);

	return 0;
}

static uiio uiio_std = {
	.ask = uiio_std_ask,
	.error = uiio_std_error,
	.perror = uiio_std_perror,
	.printf = uiio_std_printf,
	.progressStart = uiio_std_update_start,
	.update = uiio_std_update,
	.progressEnd = uiio_std_update_end,
};


void uiio_init_std(uiio *u)
{
	if (u)
		memcpy(u, &uiio_std, sizeof(uiio));
}

uiio *getUIIO(uiio *u)
{
	if (u)
		return u;
	return &uiio_std;
}
