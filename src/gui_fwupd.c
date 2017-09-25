#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gui_fwupd.h"
#include "gui_update_progress_dialog.h"
#include "ihex_signature.h"

#define BOOTLOADER_DELAY	5
#define FORCE_ERASE_DELAY	2
#define ERASE_RETRIES	10
#define ERASE_DELAY_S	5
#define PROGRAMMING_RETRIES	3
#define PROGRAMMING_DELAY_S	5

gboolean closeAdapter(gpointer data)
{
	struct application *app = data;

	deselect_adapter(app);

	return FALSE;
}

/* Monitor the dfu-programmer output to give feedback when the verify step is reached */
static void dfu_flash_outputCallback(void *ctx, const char *ln)
{
	struct application *app = ctx;

	if (strstr(ln, "Validating")) {
		setUpdateStatus(app, "Validating...", 70);
	}
}

/* This is the thread that handles the update. Started when the 'Start update'
 * button is clicked.  */
gpointer gcn64usb_updateFunc(gpointer data)
{
	struct application *app = data;
	int res;
	int retries = 10;
	int n_adapters_before;
	const char *mcu = "atmega32u2";
	const char *xtra = "";
	char cmdstr[1024];

	xtra = " 2>&1";

	if (app->at90usb1287) {
		mcu = "at90usb1287";
	}

	app->inhibit_periodic_updates = 1;

	setUpdateStatus(app, "Starting...", 1);

	if (!app->recovery_mode) {
		setUpdateStatus(app, "Enter bootloader...", 10);

		gcn64lib_bootloader(app->current_adapter_handle);
	} else {
		/* In recovery mode, the adapter is already in the bootloader. */
		n_adapters_before = rnt_countDevices();
		updatelog_append("%d adapters present before recovery\n", n_adapters_before);
	}

	sleep(BOOTLOADER_DELAY);

	/************* ERASE CHIP **************/
	setUpdateStatus(app, "Erasing chip...", 19);
	for (retries = 0; retries < ERASE_RETRIES; retries++)
	{
		snprintf(cmdstr, sizeof(cmdstr), "dfu-programmer %s erase --debug 99%s", mcu, xtra);
		res = dfu_wrapper(cmdstr, NULL, NULL);

		/* For some reason, many users got in a situation where the adapter
		 * was erased according to dfu-programmer:
		 *
		 *  "Chip already blank, to force erase use --force".
		 *
		 * Yet, flashing the firmware would fail until the command with --force
		 * was run manually...
		 *
		 * I would just use --force here, but since it is not available on older
		 * dfu-programmer versions, I run the old command too and ignore the result
		 * from the second.
		 */
		sleep(FORCE_ERASE_DELAY);
		snprintf(cmdstr, sizeof(cmdstr), "dfu-programmer %s erase --force --debug 99%s", mcu, xtra);
		dfu_wrapper(cmdstr, NULL, NULL);

		// res from the first dfu-programmer execution without --force.
		if (res == DFU_OK)
			break;

		if (res == DFU_POPEN_FAILED) {
			updateThreadDone(app, GTK_RESPONSE_REJECT);
			updatelog_append("Could not run dfu-programmer to erase chip");
			return NULL;
		}

		setUpdateStatus(app, NULL, ++app->update_percent);
		sleep(ERASE_DELAY_S);
	}

	if (retries == ERASE_RETRIES) {
		updatelog_append("Failed to erase chip\n");
		updateThreadDone(app, GTK_RESPONSE_REJECT);
		return NULL;
	}

	setUpdateStatus(app, "Chip erased", 20);

	/************* PROGRAMMING CHIP **************/
	setUpdateStatus(app, "Programming...", 30);
	snprintf(cmdstr, sizeof(cmdstr),
							"dfu-programmer %s flash \"%s\" --debug 99%s",
							mcu, app->updateHexFile, xtra);

	for (retries = 0; retries < PROGRAMMING_RETRIES; retries++)
	{
		/* Note: dfu_flash_outputCallback updates the status to Validating */
		res = dfu_wrapper(cmdstr, dfu_flash_outputCallback, app);
		if (res == DFU_OK)
			break;

		if (res == DFU_POPEN_FAILED)
		{
			updateThreadDone(app, GTK_RESPONSE_REJECT);
			updatelog_append("Could not run dfu-programmer to flash chip\n");
			return NULL;
		}

		setUpdateStatus(app, NULL, ++app->update_percent);
		sleep(PROGRAMMING_DELAY_S);
	}

	if (retries == PROGRAMMING_RETRIES) {
		updatelog_append("Failed to flash chip\n");
		updateThreadDone(app, GTK_RESPONSE_REJECT);
		return NULL;
	}


	/************* START **************/
	setUpdateStatus(app, "Starting firmware...", 80);
	snprintf(cmdstr, sizeof(cmdstr),
						"dfu-programmer %s start%s",
						mcu, xtra);

	res = dfu_wrapper(cmdstr, NULL, NULL);
	if (res != DFU_OK) {
		updateThreadDone(app, GTK_RESPONSE_REJECT);
		return NULL;
	}


	setUpdateStatus(app, "Waiting for device...", 90);

	retries = 6;
	do {
		if (app->recovery_mode) {
			/* In recovery mode, we cannot know which serial number to expect. Detect the
			 * new adapter by counting and comparing to what we had before reprogramming
			 * the firmware. */
			if (n_adapters_before != rnt_countDevices())
				break;
		} else {
			app->current_adapter_handle = rnt_openBy(&app->current_adapter_info, GCN64_FLG_OPEN_BY_SERIAL);
			if (app->current_adapter_handle)
				break;
		}

		sleep(1);
		setUpdateStatus(app, NULL, ++app->update_percent);
	} while (retries--);

	/* In recovery mode, don't bother selecting the now functional adapter. The
	 * list of serial numbers would need to be stored before programming, etc etc. */
	if (!app->recovery_mode) {
		if (!app->current_adapter_handle) {
			updateThreadDone(app, GTK_RESPONSE_REJECT);
			return NULL;
		}
	}

	setUpdateStatus(app, "Done", 100);

	printf("Update done\n");
	updateThreadDone(app, GTK_RESPONSE_OK);
	return NULL;
}

/** Called when the 'Recover adapter' menu is selected */
G_MODULE_EXPORT void recover_usbadapter_firmware(GtkWidget *w, gpointer data)
{
	struct application *app = data;
	gint res;
	GtkWidget *dialog = NULL;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	GET_UI_ELEMENT(GtkFileFilter, hexfilter);
	GET_UI_ELEMENT(GtkWindow, mainWindow);
	//GET_UI_ELEMENT(GtkDialog, firmware_update_dialog);
	//GET_UI_ELEMENT(GtkLabel, lbl_firmware_filename);
//	GET_UI_ELEMENT(GtkButtonBox, update_dialog_btnBox);
	char *filename = NULL, *basename = NULL;
//	char adap_sig[64];
#ifndef WINDOWS
	const char *notfound = "dfu-programmer not found. Cannot perform update.";
#else
	const char *notfound = "dfu-programmer.exe not found. Cannot perform update.";
#endif

	updatelog_init(app);
	updatelog_append("Recover adapter clicked\n");

	app->recovery_mode = 1;

	res = dfu_wrapper("dfu-programmer --version", NULL, NULL);
	if (res == DFU_POPEN_FAILED) {
		errorPopup(app, notfound);
		updatelog_appendln(notfound);
		updatelog_shutdown();
		return;
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

	updatelog_appendln("Running file dialog...");
	res = gtk_dialog_run (GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

		filename = gtk_file_chooser_get_filename(chooser);
		basename = g_path_get_basename(filename);

		gtk_widget_destroy(dialog);
		dialog = NULL;

		app->updateHexFile = filename;
		updatelog_appendln("Selected file: %s", filename);

		if (check_ihex_for_signature(filename, "e106420a-7c54-11e5-ae9a-001bfca3c593")) {
			app->at90usb1287 = 1;
		} else if (check_ihex_for_signature(filename, "9c3ea8b8-753f-11e5-a0dc-001bfca3c593")) {
			app->at90usb1287 = 0;
		} else {
			const char *errstr = "Signature not found - This file is invalid or not meant for this adapter";
			errorPopup(app, errstr);
			updatelog_appendln(errstr);
			goto done;
		}
		updatelog_append("Signature OK\n");

		res = update_progress_dialog_run(app, mainWindow, basename, gcn64usb_updateFunc);
#if 0
		/* Prepare the update dialog widgets... */
		gtk_label_set_text(lbl_firmware_filename, basename);
		gtk_widget_set_sensitive(GTK_WIDGET(update_dialog_btnBox), TRUE);
		app->update_percent = 0;
		app->update_status = "Ready";
		updateProgress(data);

		/* Run the dialog */
		updatelog_append("Running the update dialog...\n");
		res = gtk_dialog_run(firmware_update_dialog);
		gtk_widget_hide( GTK_WIDGET(firmware_update_dialog));
#endif

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

		rebuild_device_list_store(data);
		syncGuiToCurrentAdapter(app);
		app->inhibit_periodic_updates = 0;
	}

done:
	updatelog_shutdown();

	if (filename)
		g_free(filename);
	if (basename)
		g_free(basename);
	if (dialog)
		gtk_widget_destroy(dialog);
}

/* Called when the 'Update firmware...' button is clicked in the main GUI */
G_MODULE_EXPORT void update_usbadapter_firmware(GtkWidget *w, gpointer data)
{
	struct application *app = data;
	gint res;
	GtkWidget *dialog = NULL;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	GET_UI_ELEMENT(GtkFileFilter, hexfilter);
	GET_UI_ELEMENT(GtkWindow, mainWindow);
	//GET_UI_ELEMENT(GtkDialog, firmware_update_dialog);
	//GET_UI_ELEMENT(GtkLabel, lbl_firmware_filename);
//	GET_UI_ELEMENT(GtkButtonBox, update_dialog_btnBox);
	char *filename = NULL, *basename = NULL;
	char adap_sig[64];
#ifndef WINDOWS
	const char *notfound = "dfu-programmer not found. Cannot perform update.";
#else
	const char *notfound = "dfu-programmer.exe not found. Cannot perform update.";
#endif

	updatelog_init(app);
	updatelog_append("Update firmware clicked\n");

	app->recovery_mode = 0;

	if (gcn64lib_getSignature(app->current_adapter_handle, adap_sig, sizeof(adap_sig))) {
		char *errmsg = "Could not read adapter signature - This file may not be meant for it (Bricking hazard!)";
		errorPopup(app, errmsg);
		updatelog_appendln(errmsg);
		goto done;
	}

	/* Test for dfu-programmer presence in path*/
	res = dfu_wrapper("dfu-programmer --version", NULL, NULL);
	if (res == DFU_POPEN_FAILED) {
		errorPopup(app, notfound);
		updatelog_appendln(notfound);
		updatelog_shutdown();
		return;
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

	updatelog_appendln("Running file dialog...");
	res = gtk_dialog_run (GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

		filename = gtk_file_chooser_get_filename(chooser);
		basename = g_path_get_basename(filename);

		gtk_widget_destroy(dialog);
		dialog = NULL;

		app->updateHexFile = filename;
		updatelog_appendln("Selected file: %s", filename);

		updatelog_append("Checking file for signature...\n");
		if (!check_ihex_for_signature(filename, adap_sig)) {
			const char *errstr = "Signature not found - This file is invalid or not meant for this adapter";
			errorPopup(app, errstr);
			updatelog_appendln(errstr);
			goto done;
		}
		updatelog_append("Signature OK\n");

		// For my development board based on at90usb1287. Need to pass the correct
		// MCU argument to dfu-programmer
		if (0 == strcmp("e106420a-7c54-11e5-ae9a-001bfca3c593", adap_sig)) {
			app->at90usb1287 = 1;
		} else {
			app->at90usb1287 = 0;
		}

		res = update_progress_dialog_run(app, mainWindow, basename, gcn64usb_updateFunc);
#if 0
		/* Prepare the update dialog widgets... */
		gtk_label_set_text(lbl_firmware_filename, basename);
		gtk_widget_set_sensitive(GTK_WIDGET(update_dialog_btnBox), TRUE);
		app->update_percent = 0;
		app->update_status = "Ready";
		updateProgress(data);

		/* Run the dialog */
		updatelog_append("Running the update dialog...\n");
		res = gtk_dialog_run(firmware_update_dialog);
		gtk_widget_hide( GTK_WIDGET(firmware_update_dialog));
#endif

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

		rebuild_device_list_store(data);
		syncGuiToCurrentAdapter(app);
		app->inhibit_periodic_updates = 0;
	}

done:
	updatelog_shutdown();

	if (filename)
		g_free(filename);
	if (basename)
		g_free(basename);
	if (dialog)
		gtk_widget_destroy(dialog);
}


