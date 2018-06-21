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

	app->inhibit_periodic_updates = 1;
	rnt_suspendPolling(app->current_adapter_handle, 1);
	res = psxlib_readMemoryCard(app->current_adapter_handle, 0, mc_data, u);
	rnt_suspendPolling(app->current_adapter_handle, 0);
	app->inhibit_periodic_updates = 0;

	if (res < 0) {
		if (res != PSXLIB_ERR_USER_CANCELLED) {
			u->error(psxlib_getErrorString(res));
		}
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

	dialog = gtk_file_chooser_dialog_new("Load file",
										app->mainwindow,
										action,
										"_Cancel",
										GTK_RESPONSE_CANCEL,
										"_Load",
										GTK_RESPONSE_ACCEPT,
										NULL);
	chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), psxcard_raw_filter);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *filename;
		GtkWidget *confirm_write_dialog;

		// Hide the file chooser
		gtk_widget_hide(dialog);

		// Try loadig the selected file
		filename = gtk_file_chooser_get_filename(chooser);

		res = psxlib_loadMemoryCardFromFile(filename, PSXLIB_FILE_FORMAT_AUTO, mc_data);
		if (res < 0) {
			u->error("Could not load file '%s' : %s", filename, psxlib_getErrorString(res));
			g_free(filename);
			g_free(mc_data);
			return;
		}
		g_free(filename);

		// Warn the user about what is about to happen
		confirm_write_dialog = gtk_message_dialog_new(app->mainwindow, GTK_DIALOG_DESTROY_WITH_PARENT,
														GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
														"This will write to your memory card. Continue?");
		gtk_dialog_add_buttons(GTK_DIALOG(confirm_write_dialog), "Cancel", GTK_RESPONSE_CANCEL, "Yes", GTK_RESPONSE_ACCEPT, NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(confirm_write_dialog), GTK_RESPONSE_CANCEL);
		res = gtk_dialog_run(GTK_DIALOG(confirm_write_dialog));
		gtk_widget_destroy(confirm_write_dialog);

		if (res == GTK_RESPONSE_ACCEPT) {
			app->inhibit_periodic_updates = 1;
			rnt_suspendPolling(app->current_adapter_handle, 1);
			res = psxlib_writeMemoryCard(app->current_adapter_handle, 0, mc_data, u);
			if (res < 0) {
				if (res != PSXLIB_ERR_USER_CANCELLED) {
					u->error(psxlib_getErrorString(res));
				}
			}
			rnt_suspendPolling(app->current_adapter_handle, 0);
			app->inhibit_periodic_updates = 0;
		}
	}
	gtk_widget_destroy(dialog);

	g_free(mc_data);

}
