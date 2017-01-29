#include <stdio.h>
#include "gui_logger.h"

static FILE *logfptr = NULL;

void updatelog_append(const char *message, ...)
{
	GTimeVal tv;
	gchar *ts_str;
	va_list args;

	if (logfptr) {
		g_get_current_time(&tv);
		ts_str = g_time_val_to_iso8601(&tv);

		// Timetstamp :
		if (ts_str) {
			fprintf(logfptr, "%s: ", ts_str);
			g_free(ts_str);
		}

		// Log message
		va_start(args, message);
		vfprintf(logfptr, message, args);
		va_end(args);
		fflush(logfptr);
	}
}

void updatelog_appendln(const char *message, ...)
{
	GTimeVal tv;
	gchar *ts_str;
	va_list args;

	if (logfptr) {
		g_get_current_time(&tv);
		ts_str = g_time_val_to_iso8601(&tv);

		// Timetstamp :
		if (ts_str) {
			fprintf(logfptr, "%s: ", ts_str);
			g_free(ts_str);
		}

		// Log message
		va_start(args, message);
		vfprintf(logfptr, message, args);
		va_end(args);
		fputs("\n", logfptr);
		fflush(logfptr);
	}

}

void updatelog_init(struct application *app)
{
	const char *homedir;
	gchar *filename;

	homedir = g_get_home_dir();
	if (!homedir)
		return;

	filename = g_build_filename(homedir, "raphnetupdatelog.txt", NULL);
	if (!filename)
		return;

	logfptr = fopen(filename, "a");
	if (!logfptr) {
		errorPopup(app, "Could not create log file.");
		return;
	}

	updatelog_append("Logging started\n");

	g_free(filename);
}

void updatelog_shutdown(void)
{
	updatelog_append("Logging ended\n");

	if (logfptr) {
		fclose(logfptr);
	}
}


