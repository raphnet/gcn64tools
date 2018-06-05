#include <stdio.h>
#include "gcn64ctl_gui.h"
#include "ihex.h"
#include "ihex_signature.h"

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

	if (x2gcn64_adapter_echotest(app->current_adapter_handle, channel, 0)) {
		errorPopup(app, "No adapter detected. Not connected the first port or too old.");
		return;
	}

	gtk_widget_show((GtkWidget*)win_gc2n64);
}

G_MODULE_EXPORT void gc2n64_manager_on_show(GtkWidget *win, gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkLabel, label_gc2n64_firmware);
	GET_UI_ELEMENT(GtkLabel, label_gc2n64_upgradeable);
	GET_UI_ELEMENT(GtkLabel, label_gc2n64_conversion_mode);
	GET_UI_ELEMENT(GtkLabel, label_gc2n64_deadzone);
	GET_UI_ELEMENT(GtkLabel, label_gc2n64_gamecube_controller_present);
	GET_UI_ELEMENT(GtkLabel, label_gc2n64_adapter_type);
	int channel = 0;

	if (x2gcn64_adapter_getInfo(app->current_adapter_handle, channel, &app->inf)) {
		errorPopup(app, "No adapter detected.\nEither too old or not connected the first port.");
		return;
	}


	if (app->inf.in_bootloader) {
		gtk_label_set_text(label_gc2n64_firmware, "Unknown (currently in bootloader)");
		gtk_label_set_text(label_gc2n64_upgradeable, "Yes");
	} else {
		gtk_label_set_text(label_gc2n64_adapter_type, x2gcn64_adapter_type_name(app->inf.adapter_type));
		gtk_label_set_text(label_gc2n64_firmware, (char*)app->inf.app.version);
		gtk_label_set_text(label_gc2n64_upgradeable, app->inf.app.upgradeable ? "Yes":"No");
		gtk_label_set_text(label_gc2n64_conversion_mode, gc2n64_adapter_getConversionModeName(&app->inf.app.gc2n64));
		gtk_label_set_text(label_gc2n64_deadzone, app->inf.app.gc2n64.deadzone_enabled ? "Enabled":"Disabled");
		gtk_label_set_text(label_gc2n64_gamecube_controller_present, app->inf.app.gc2n64.gc_controller_detected ? "Present":"Absent");
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
	x2gcn64_adapter_updateFirmware(app->current_adapter_handle, channel, app->updateHexFile, NULL);
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
	const char *sig;

	updatelog_init(app);
	updatelog_append("GC2N64 firmware update clicked");

	app->inhibit_periodic_updates = 1;

	sig = x2gcn64_getAdapterSignature(app->inf.adapter_type);

	dialog = gtk_file_chooser_dialog_new("Open .hex file",
										win_gc2n64,
										action,
										"_Cancel",
										GTK_RESPONSE_CANCEL,
										"_Open",
										GTK_RESPONSE_ACCEPT,
										NULL);

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), hexfilter);

	if (sig) {
		fwupd_firmwareFolderShortcutAndSet(GTK_FILE_CHOOSER(dialog), app, sig);
	}

	res = gtk_dialog_run (GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

		filename = gtk_file_chooser_get_filename(chooser);
		basename = g_path_get_basename(filename);

		gtk_widget_destroy(dialog);
		dialog = NULL;

		app->updateHexFile = filename;
		updatelog_appendln("Selected file: %s", filename);

		//if (!check_ihex_for_signature(filename, "41d938a8-6f8a-11e5-a45e-001bfca3c593")) {
		if (!check_ihex_for_signature(filename, sig)) {
			const char *errstr = "Signature not found - This file is invalid or not meant for this adapter";
			errorPopup(app, errstr);
			updatelog_appendln(errstr);
			updatelog_shutdown();
			return;
		}
		updatelog_append("Signature OK\n");

		rnt_suspendPolling(app->current_adapter_handle, 1);
		res = update_progress_dialog_run(app, win_gc2n64, basename, gc2n64_updateFunc);
		rnt_suspendPolling(app->current_adapter_handle, 0);

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
	else {
		gtk_widget_destroy(dialog);
		dialog = NULL;
	}

	app->inhibit_periodic_updates = 0;
	updatelog_shutdown();
}

