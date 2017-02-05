#include <stdio.h>
#include "gcn64ctl_gui.h"
#include "ihex.h"
#include "ihex_signature.h"

#include "gc2n64_adapter.h"
#include "gui_update_progress_dialog.h"

G_MODULE_EXPORT void gc2n64_manager_show(GtkWidget *win, gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkWindow, win_gc2n64);
	int channel = 0;

	/* Shouldn't happen (menuitem non sensitive when no adapter selected) */
	if (!app->current_adapter_handle) {
		errorPopup(app, "No adapter selected");
		gtk_widget_hide(win);
		return;
	}

	if (gc2n64_adapter_echotest(app->current_adapter_handle, channel, 0)) {
		errorPopup(app, "No gamecube to N64 adapter detected.\nEither too old (pre 2.0) or not connected the first port.");
		return;
	}

	gtk_widget_show((GtkWidget*)win_gc2n64);
}

G_MODULE_EXPORT void gc2n64_manager_on_show(GtkWidget *win, gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkLabel, label_gc2n64_firmware);
	GET_UI_ELEMENT(GtkLabel, label_gc2n64_upgradeable);
	GET_UI_ELEMENT(GtkLabel, label_gc2n64_old_v15);
	GET_UI_ELEMENT(GtkLabel, label_gc2n64_deadzone);
	int channel = 0;

	printf("Enter gc2n64 manager window\n");

	struct gc2n64_adapter_info inf;

	if (gc2n64_adapter_getInfo(app->current_adapter_handle, channel, &inf)) {
		errorPopup(app, "No gamecube to N64 adapter detected.\nEither too old (pre 2.0) or not connected the first port.");
		return;
	}

	if (inf.in_bootloader) {
		gtk_label_set_text(label_gc2n64_firmware, "Unknown (currently in bootloader)");
		gtk_label_set_text(label_gc2n64_upgradeable, "Yes");
	} else {
		gtk_label_set_text(label_gc2n64_firmware, (char*)inf.app.version);
		gtk_label_set_text(label_gc2n64_upgradeable, inf.app.upgradeable ? "Yes":"No");
		gtk_label_set_text(label_gc2n64_old_v15, inf.app.old_v1_5_conversion ? "Yes":"No");
		gtk_label_set_text(label_gc2n64_deadzone, inf.app.deadzone_enabled ? "Enabled":"Disabled");
	}
}
#if 0
static gboolean updateDonefunc(gpointer data)
{
	struct application *app = data;

	g_thread_join(app->updater_thread);

	rebuild_device_list_store(data);
	syncGuiToCurrentAdapter(app);
	app->inhibit_periodic_updates = 0;

	return FALSE;
}
#endif

gpointer gc2n64_updateFunc(gpointer data)
{
	struct application *app = data;
	int channel = 0;

	setUpdateStatus(app, "Installing new firmware...", 50);
	gc2n64_adapter_updateFirmware(app->current_adapter_handle, channel, app->updateHexFile);
	setUpdateStatus(app, "Done", 100);

	updateThreadDone(app, GTK_RESPONSE_OK);
	return NULL;
}

/* Called when the Update Firmware button is clicked */
G_MODULE_EXPORT void gc2n64_manager_upgrade(GtkWidget *win, gpointer data)
{
	struct application *app = data;
	GtkWidget *dialog = NULL;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	GET_UI_ELEMENT(GtkFileFilter, hexfilter);
	GET_UI_ELEMENT(GtkWindow, win_gc2n64);
	gint res;
	char *filename = NULL, *basename = NULL;

	updatelog_init(app);
	updatelog_append("GC2N64 firmware update clicked");

	app->inhibit_periodic_updates = 1;

	dialog = gtk_file_chooser_dialog_new("Open .hex file",
										win_gc2n64,
										action,
										"_Cancel",
										GTK_RESPONSE_CANCEL,
										"_Open",
										GTK_RESPONSE_ACCEPT,
										NULL);

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), hexfilter);

	res = gtk_dialog_run (GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

		filename = gtk_file_chooser_get_filename(chooser);
		basename = g_path_get_basename(filename);

		gtk_widget_destroy(dialog);
		dialog = NULL;

		app->updateHexFile = filename;
		updatelog_appendln("Selected file: %s", filename);

		if (!check_ihex_for_signature(filename, "41d938a8-6f8a-11e5-a45e-001bfca3c593")) {
			const char *errstr = "Signature not found - This file is invalid or not meant for this adapter";
			errorPopup(app, errstr);
			updatelog_appendln(errstr);
			updatelog_shutdown();
			return;
		}
		updatelog_append("Signature OK\n");

		res = update_progress_dialog_run(app, win_gc2n64, basename, gc2n64_updateFunc);
		if (res == GTK_RESPONSE_OK) {
			const char *msg = "Update succeeded.";
			infoPopup(app, msg);
			updatelog_appendln(msg);
		} else if (res == GTK_RESPONSE_REJECT) {
			const char *msg = "Update failed.";
			errorPopup(app, msg);
			updatelog_appendln(msg);
		}
		updatelog_append("Update dialog done\n");

		gc2n64_manager_on_show(win, (gpointer)app);
	}

	app->inhibit_periodic_updates = 0;
	updatelog_shutdown();
}

