#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_fwupd.h"
#include "ihex_signature.h"

gboolean updateDonefunc(gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkDialog, firmware_update_dialog);

	printf("updateDonefunc\n");
	gtk_dialog_response(firmware_update_dialog, app->update_dialog_response);
	g_thread_join(app->updater_thread);

	rebuild_device_list_store(data);
	syncGuiToCurrentAdapter(app);
	app->inhibit_periodic_updates = 0;

	return FALSE;
}

gboolean updateProgress(gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkProgressBar, updateProgress);
	GET_UI_ELEMENT(GtkLabel, updateStatus);

	gtk_progress_bar_set_fraction(updateProgress, app->update_percent / 100.0);
	gtk_label_set_text(updateStatus, app->update_status);

	return FALSE;
}

gboolean closeAdapter(gpointer data)
{
	struct application *app = data;

	deselect_adapter(app);

	return FALSE;
}

gpointer updateThreadFunc(gpointer data)
{
	struct application *app = data;
	int res;
	FILE *dfu_fp;
	char linebuf[256];
	char cmd[256];
	int retries = 10;
	int n_adapters_before;

	app->inhibit_periodic_updates = 1;

	app->update_status = "Starting...";
	app->update_percent = 1;
	gdk_threads_add_idle(updateProgress, data);

	if (!app->recovery_mode) {
		app->update_percent = 10;
		app->update_status = "Enter bootloader...";

		gcn64lib_bootloader(app->current_adapter_handle);
		gdk_threads_add_idle(closeAdapter, data);
	} else {
		/* In recovery mode, the adapter is already in the bootloader. */
		n_adapters_before = gcn64_countDevices();
		printf("%d adapters present before recovery\n", n_adapters_before);
	}

	app->update_percent = 19;
	app->update_status = "Erasing chip...";
	do {
		app->update_percent++;
		gdk_threads_add_idle(updateProgress, data);

		if (app->at90usb1287) {
			dfu_fp = popen("dfu-programmer at90usb1287 erase --suppress-validation", "r");
		} else {
			dfu_fp = popen("dfu-programmer atmega32u2 erase --suppress-validation", "r");
		}
		if (!dfu_fp) {
			app->update_dialog_response = GTK_RESPONSE_REJECT;
			gdk_threads_add_idle(updateDonefunc, data);
			return NULL;
		}

		do {
			fgets(linebuf, sizeof(linebuf), dfu_fp);
		} while(!feof(dfu_fp));

		res = pclose(dfu_fp);
		printf("Pclose: %d\n", res);

		if (res==0) {
			break;
		}
		sleep(1);
	} while (retries--);

	if (!retries) {
		app->update_dialog_response = GTK_RESPONSE_REJECT;
		gdk_threads_add_idle(updateDonefunc, data);
	}

	app->update_status = "Chip erased";
	app->update_percent = 20;
	gdk_threads_add_idle(updateProgress, data);


	app->update_status = "Programming ...";
	app->update_percent = 30;
	gdk_threads_add_idle(updateProgress, data);

	snprintf(cmd, sizeof(cmd), "dfu-programmer %s flash %s",
				app->at90usb1287 ? "at90usb1287" : "atmega32u2",
				app->updateHexFile);
	dfu_fp = popen(cmd, "r");
	if (!dfu_fp) {
		app->update_dialog_response = GTK_RESPONSE_REJECT;
		gdk_threads_add_idle(updateDonefunc, data);
		return NULL;
	}

	do {
		fgets(linebuf, sizeof(linebuf), dfu_fp);
		printf("ln: %s\n\n", linebuf);
		if (strstr(linebuf, "Validating")) {
			app->update_status = "Validating...";
			app->update_percent = 70;
			gdk_threads_add_idle(updateDonefunc, data);
		}
	} while(!feof(dfu_fp));

	res = pclose(dfu_fp);
	printf("Pclose: %d\n", res);

	if (res != 0) {
		app->update_dialog_response = GTK_RESPONSE_REJECT;
		gdk_threads_add_idle(updateDonefunc, data);
		return NULL;
	}

	app->update_status = "Starting firmware...";
	app->update_percent = 80;
	gdk_threads_add_idle(updateProgress, data);

	if (app->at90usb1287) {
		dfu_fp = popen("dfu-programmer at90usb1287 start", "r");
	} else {
		dfu_fp = popen("dfu-programmer atmega32u2 start", "r");
	}
	if (!dfu_fp) {
		app->update_dialog_response = GTK_RESPONSE_REJECT;
		gdk_threads_add_idle(updateDonefunc, data);
		return NULL;
	}

	res = pclose(dfu_fp);
	printf("Pclose: %d\n", res);

	if (res!=0) {
		app->update_dialog_response = GTK_RESPONSE_REJECT;
		gdk_threads_add_idle(updateDonefunc, data);
		return NULL;
	}

	app->update_status = "Waiting for device...";
	app->update_percent = 90;
	gdk_threads_add_idle(updateProgress, data);

	retries = 6;
	do {
		if (app->recovery_mode) {
			/* In recovery mode, we cannot know which serial number to expect. Detect the
			 * new adapter by counting and comparing to what we had before reprogramming
			 * the firmware. */
			if (n_adapters_before != gcn64_countDevices())
				break;
		} else {
			app->current_adapter_handle = gcn64_openBy(&app->current_adapter_info, GCN64_FLG_OPEN_BY_SERIAL);
			if (app->current_adapter_handle)
				break;
		}

		sleep(1);
		app->update_percent++;
		gdk_threads_add_idle(updateProgress, data);
	} while (retries--);

	/* In recovery mode, don't bother selecting the now functional adapter. The
	 * list of serial numbers would need to be stored before programming, etc etc. */
	if (!app->recovery_mode) {
		if (!app->current_adapter_handle) {
			app->update_dialog_response = GTK_RESPONSE_REJECT;
			gdk_threads_add_idle(updateDonefunc, data);
			return NULL;
		}
	}

	app->update_status = "Done";
	app->update_percent = 100;
	gdk_threads_add_idle(updateProgress, data);

	printf("Update done\n");
	app->update_dialog_response = GTK_RESPONSE_OK;
	gdk_threads_add_idle(updateDonefunc, data);
	return NULL;
}

G_MODULE_EXPORT void updatestart_btn_clicked_cb(GtkWidget *w, gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkButtonBox, update_dialog_btnBox);

	app->updater_thread = g_thread_new("updater", updateThreadFunc, app);
	gtk_widget_set_sensitive(GTK_WIDGET(update_dialog_btnBox), FALSE);
}

G_MODULE_EXPORT void recover_usbadapter_firmware(GtkWidget *w, gpointer data)
{
	struct application *app = data;
	gint res;
	GtkWidget *dialog = NULL;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	GET_UI_ELEMENT(GtkFileFilter, hexfilter);
	GET_UI_ELEMENT(GtkDialog, firmware_update_dialog);
	GET_UI_ELEMENT(GtkLabel, lbl_firmware_filename);
	GET_UI_ELEMENT(GtkButtonBox, update_dialog_btnBox);
	FILE *dfu_fp;
	char *filename = NULL, *basename = NULL;
//	char adap_sig[64];
#ifndef WINDOWS
	const char *notfound = "dfu-programmer not found. Cannot perform update.";
#else
	const char *notfound = "dfu-programmer.exe not found. Cannot perform update.";
#endif

	app->recovery_mode = 1;

	/* Test for dfu-programmer presence in path*/
	dfu_fp = popen("dfu-programmer --version", "r");
	//dfu_fp = popen("dfu2-programmer --version", "r");
	if (!dfu_fp) {
		perror("popen");
		errorPopup(app, notfound);
		return;
	}
	res = pclose(dfu_fp);
#ifdef WINDOWS
	// Under Mingw, 0 is returned when dfu-programmmer was found 
	// and executed. Otherwise 1.
	if (res != 0) {
#else
	// Under Unix, the usual is available.
	if (!WIFEXITED(res) || (WEXITSTATUS(res)!=1)) {
#endif
		if (res) {
			errorPopup(app, notfound);
			return;
		}
	}

	dialog = gtk_file_chooser_dialog_new("Open .hex file",
										app->mainwindow,
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

		printf("File selected: %s\n", filename);
		app->updateHexFile = filename;

		if (check_ihex_for_signature(filename, "e106420a-7c54-11e5-ae9a-001bfca3c593")) {
			app->at90usb1287 = 1;
		} else if (check_ihex_for_signature(filename, "9c3ea8b8-753f-11e5-a0dc-001bfca3c593")) {
			app->at90usb1287 = 0;
		} else {
			errorPopup(app, "Signature not found - This file is invalid or not meant for this adapter");
			goto done;
		}

		/* Prepare the update dialog widgets... */
		gtk_label_set_text(lbl_firmware_filename, basename);
		gtk_widget_set_sensitive(GTK_WIDGET(update_dialog_btnBox), TRUE);
		app->update_percent = 0;
		app->update_status = "Ready";
		updateProgress(data);

		/* Run the dialog */
		res = gtk_dialog_run(firmware_update_dialog);
		gtk_widget_hide( GTK_WIDGET(firmware_update_dialog));

		if (res == GTK_RESPONSE_OK) {
			infoPopup(app, "Update succeeded.");
		} else if (res == GTK_RESPONSE_REJECT) {
			errorPopup(app, "Update failed.");
		}
		printf("Update dialog done\n");

	}

done:
	if (filename)
		g_free(filename);
	if (basename)
		g_free(basename);
	if (dialog)
		gtk_widget_destroy(dialog);
}

G_MODULE_EXPORT void update_usbadapter_firmware(GtkWidget *w, gpointer data)
{
	struct application *app = data;
	gint res;
	GtkWidget *dialog = NULL;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	GET_UI_ELEMENT(GtkFileFilter, hexfilter);
	GET_UI_ELEMENT(GtkDialog, firmware_update_dialog);
	GET_UI_ELEMENT(GtkLabel, lbl_firmware_filename);
	GET_UI_ELEMENT(GtkButtonBox, update_dialog_btnBox);
	FILE *dfu_fp;
	char *filename = NULL, *basename = NULL;
	char adap_sig[64];
#ifndef WINDOWS
	const char *notfound = "dfu-programmer not found. Cannot perform update.";
#else
	const char *notfound = "dfu-programmer.exe not found. Cannot perform update.";
#endif

	app->recovery_mode = 0;

	if (gcn64lib_getSignature(app->current_adapter_handle, adap_sig, sizeof(adap_sig))) {
		errorPopup(app, "Could not read adapter signature - This file may not be meant for it (Bricking hazard!)");
		goto done;
	}

	/* Test for dfu-programmer presence in path*/
	dfu_fp = popen("dfu-programmer --version", "r");
	//dfu_fp = popen("dfu2-programmer --version", "r");
	if (!dfu_fp) {
		perror("popen");
		errorPopup(app, notfound);
		return;
	}
	res = pclose(dfu_fp);
#ifdef WINDOWS
	// Under Mingw, 0 is returned when dfu-programmmer was found 
	// and executed. Otherwise 1.
	if (res != 0) {
#else
	// Under Unix, the usual is available.
	if (!WIFEXITED(res) || (WEXITSTATUS(res)!=1)) {
#endif
		if (res) {
			errorPopup(app, notfound);
			return;
		}
	}

	dialog = gtk_file_chooser_dialog_new("Open .hex file",
										app->mainwindow,
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

		printf("File selected: %s\n", filename);
		app->updateHexFile = filename;

		printf("Checking file for signature...\n");
		if (!check_ihex_for_signature(filename, adap_sig)) {
			errorPopup(app, "Signature not found - This file is invalid or not meant for this adapter");
			goto done;
		}

		// For my development board based on at90usb1287. Need to pass the correct
		// MCU argument to dfu-programmer
		if (0 == strcmp("e106420a-7c54-11e5-ae9a-001bfca3c593", adap_sig)) {
			app->at90usb1287 = 1;
		} else {
			app->at90usb1287 = 0;
		}

		/* Prepare the update dialog widgets... */
		gtk_label_set_text(lbl_firmware_filename, basename);
		gtk_widget_set_sensitive(GTK_WIDGET(update_dialog_btnBox), TRUE);
		app->update_percent = 0;
		app->update_status = "Ready";
		updateProgress(data);

		/* Run the dialog */
		res = gtk_dialog_run(firmware_update_dialog);
		gtk_widget_hide( GTK_WIDGET(firmware_update_dialog));

		if (res == GTK_RESPONSE_OK) {
			infoPopup(app, "Update succeeded.");
		} else if (res == GTK_RESPONSE_REJECT) {
			errorPopup(app, "Update failed.");
		}
		printf("Update dialog done\n");

	}

done:
	if (filename)
		g_free(filename);
	if (basename)
		g_free(basename);
	if (dialog)
		gtk_widget_destroy(dialog);
}


