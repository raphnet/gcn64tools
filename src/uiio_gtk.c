#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "uiio_gtk.h"

static GtkWindow *g_mainwin;

GtkWidget *g_progressDialog, *g_progressBar;

static int uiio_gtk_error(const char *fmt, ...)
{
	GtkDialogFlags errorDialogFlags = GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkWidget *errorDialog;
	char messagebuffer[512];
	va_list ap;
	int i;

	va_start(ap, fmt);
	i = vsnprintf(messagebuffer, 512, fmt, ap);
	va_end(ap);

	errorDialog = gtk_message_dialog_new(g_mainwin,
									errorDialogFlags,
									GTK_MESSAGE_ERROR,
									GTK_BUTTONS_CLOSE,
									messagebuffer);
	gtk_dialog_run(GTK_DIALOG(errorDialog));
	gtk_widget_destroy(errorDialog);

	return i;
}

static void cancel_progress(GtkDialog *dialog, gint response_id, gpointer user_data)
{
	uiio *u = user_data;

	if (u->progress_status < UIIO_PROGRESS_STARTED)
		return;

	// Any response ID means cancel
	u->progress_status = UIIO_PROGRESS_CANCELLED;
}

static void progressStart(uiio *uiio)
{
	GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkWidget *content;

	// When using multi_progress, a progress dialog may already be available
	if (!g_progressDialog) {
		g_progressDialog = gtk_dialog_new_with_buttons(
							uiio->caption,
							g_mainwin,
							flags,
							"_Cancel",
							GTK_RESPONSE_CANCEL,
							"_Close",
							GTK_RESPONSE_CLOSE,
							NULL);
	}

	gtk_dialog_set_response_sensitive(GTK_DIALOG(g_progressDialog), GTK_RESPONSE_CLOSE, FALSE);

	g_progressBar = gtk_progress_bar_new();
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_progressBar), 0.0);
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(g_progressBar), TRUE);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_progressBar), uiio->caption);

	content = gtk_dialog_get_content_area(GTK_DIALOG(g_progressDialog));
	gtk_container_add(GTK_CONTAINER(content), g_progressBar);

	g_signal_connect(g_progressDialog, "response", G_CALLBACK(cancel_progress), uiio);

	gtk_widget_show_all(g_progressDialog);

	uiio->progress_status = UIIO_PROGRESS_STARTED;
}

static void progressEnd(uiio *uiio, const char *message)
{
	printf("progressEnd: %d\n", uiio->progress_status);
	if (uiio->progress_status < UIIO_PROGRESS_STARTED)
		return;

	gtk_dialog_set_response_sensitive(GTK_DIALOG(g_progressDialog), GTK_RESPONSE_CANCEL, FALSE);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(g_progressDialog), GTK_RESPONSE_CLOSE, TRUE);

	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_progressBar), message);

	if (!uiio->multi_progress) {
		(void)gtk_dialog_run(GTK_DIALOG(g_progressDialog));
		// The only option is to close the dialog. Ignore the return value.
		gtk_widget_destroy(g_progressDialog);
		g_progressDialog = NULL;
	}
	uiio->progress_status = UIIO_PROGRESS_STOPPED;
}

static int progressUpdate(uiio *uiio)
{
	if (uiio->progress_status < UIIO_PROGRESS_STARTED)
		return 0;

	while (gtk_events_pending ())
		  gtk_main_iteration ();

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_progressBar), uiio->cur_progress / (float)uiio->max_progress);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(g_progressBar), uiio->caption);

	if (uiio->progress_status == UIIO_PROGRESS_CANCELLED) {
		return 1;
	}

	return 0;
}

static uiio uiio_gtk;

uiio *getUIIO_gtk(uiio *u, GtkWindow *mainWindow)
{
	/* Init with default stdout/err by default */
	uiio_init_std(&uiio_gtk);

	uiio_gtk.error = uiio_gtk_error,
	uiio_gtk.progressStart = progressStart,
	uiio_gtk.progressEnd = progressEnd,
	uiio_gtk.update = progressUpdate,

	g_mainwin = mainWindow;

	return &uiio_gtk;
}
