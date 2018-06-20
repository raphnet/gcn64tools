#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "gcn64ctl_gui.h"
#include "gui_psx_memcard.h"
#include "uiio_gtk.h"
#include "psxlib.h"

G_MODULE_EXPORT void read_psx_memcard(GtkWidget *wid, gpointer data)
{
	struct application *app = data;
	uiio *u = getUIIO_gtk(NULL, app->mainwindow);
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	GET_UI_ELEMENT(GtkFileFilter, psxcard_raw_filter);
	gint res;
	struct psx_memorycard *mc_data;

	if (!app->current_adapter_handle)
		return;

	mc_data = g_malloc0(sizeof(struct psx_memorycard));
	if (!mc_data) {
		u->perror("Could not allocate memory for card contents");
		return;
	}

	rnt_suspendPolling(app->current_adapter_handle, 1);
	res = psxlib_readMemoryCard(app->current_adapter_handle, 0, mc_data, u);
	rnt_suspendPolling(app->current_adapter_handle, 0);

	if (res < 0) {
		u->error(psxlib_getErrorString(res));
		g_free(mc_data);
		return;
	}

	dialog = gtk_file_chooser_dialog_new("Save file",
										app->mainwindow,
										action,
										"_Cancel",
										GTK_RESPONSE_CANCEL,
										"_Save",
										GTK_RESPONSE_ACCEPT,
										NULL);
	chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), psxcard_raw_filter);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *filename;

		gtk_widget_hide(dialog);

		filename = gtk_file_chooser_get_filename(chooser);

		res = psxlib_writeMemoryCardToFile(mc_data, filename, PSXLIB_FILE_FORMAT_RAW);
		if (res < 0) {
			u->error(psxlib_getErrorString(res));
		}

		g_free(filename);
	}
	gtk_widget_destroy(dialog);

	g_free(mc_data);
}

G_MODULE_EXPORT void write_psx_memcard(GtkWidget *wid, gpointer data)
{
	struct application *app = data;
	uiio *u = getUIIO_gtk(NULL, app->mainwindow);

	u->error("Not implemented yet");
}
