#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "gcn64ctl_gui.h"
#include "gui_xferpak.h"
#include "xferpak.h"
#include "xferpak_tools.h"
#include "uiio_gtk.h"

G_MODULE_EXPORT void gui_xferpak_readram(GtkWidget *win, gpointer data)
{
	struct application *app = data;
	xferpak *xpak;
	uiio *u = getUIIO_gtk(NULL, app->mainwindow);
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	GET_UI_ELEMENT(GtkFileFilter, gbram_filter);
	char namebuf[128];
	struct gbcart_info cartinfo;
	gint res;
	unsigned char *mem;
	u->caption = "Reading cartridge RAM...";

	if (!app->current_adapter_handle)
		return;

	// Suspend polling for better performance
	rnt_suspendPolling(app->current_adapter_handle, 1);


	/* Prepare xferpak */
	xpak = gcn64lib_xferpak_init(app->current_adapter_handle, 0, u);
	if (!xpak) {
		u->error("Transfer Pak not found or no cartridge present");

		// Resume polling
		rnt_suspendPolling(app->current_adapter_handle, 0);
		return;
	}

	res = xferpak_gb_readInfo(xpak, &cartinfo);
	if (res < 0) {
		u->error("Error reading cartridge header");
		xferpak_free(xpak);

		// Resume polling
		rnt_suspendPolling(app->current_adapter_handle, 0);
		return;
	}

	if (cartinfo.ram_size <= 0) {
		u->error("This cartridge does not contain RAM\nNothing to do.");
		xferpak_free(xpak);

		// Resume polling
		rnt_suspendPolling(app->current_adapter_handle, 0);
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
	// Propose the name of the game
	snprintf(namebuf, sizeof(namebuf), "%s.sav", cartinfo.title);
	gtk_file_chooser_set_current_name(chooser, namebuf);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), gbram_filter);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *filename;
		int mem_size;

		gtk_widget_hide(dialog);

		filename = gtk_file_chooser_get_filename(chooser);
		mem_size = xferpak_gb_readRAM(xpak, &cartinfo, &mem);
		if (mem_size < 0) {
			if (mem_size != XFERPAK_USER_CANCELLED) {
				u->error(xferpak_errStr(mem_size));
			}
		} else {
			FILE *fptr;
			fptr = fopen(filename, "wb");
			if (fptr) {
				printf("Writing to '%s'\n", filename);
				fwrite(mem, mem_size, 1, fptr);
				fclose(fptr);
			} else {
				u->perror(filename);
			}
			free(mem);
		}
		g_free(filename);
	}
	gtk_widget_destroy(dialog);

	xferpak_free(xpak);

	// Resume polling
	rnt_suspendPolling(app->current_adapter_handle, 0);
}

G_MODULE_EXPORT void gui_xferpak_readrom(GtkWidget *win, gpointer data)
{
	struct application *app = data;
	xferpak *xpak;
	uiio *u = getUIIO_gtk(NULL, app->mainwindow);
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	GET_UI_ELEMENT(GtkFileFilter, gbrom_filter);
	char namebuf[128];
	struct gbcart_info cartinfo;
	gint res;
	unsigned char *mem;
	u->caption = "Reading cartridge ROM...";

	if (!app->current_adapter_handle)
		return;

	// Suspend polling for better performance
	rnt_suspendPolling(app->current_adapter_handle, 1);

	/* Prepare xferpak */
	xpak = gcn64lib_xferpak_init(app->current_adapter_handle, 0, u);
	if (!xpak) {
		u->error("Transfer Pak not found or no cartridge present");

		// Resume polling
		rnt_suspendPolling(app->current_adapter_handle, 0);
		return;
	}

	res = xferpak_gb_readInfo(xpak, &cartinfo);
	if (res < 0) {
		u->error("Error reading cartridge header");
		xferpak_free(xpak);

		// Resume polling
		rnt_suspendPolling(app->current_adapter_handle, 0);
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
	// Propose the name of the game
	snprintf(namebuf, sizeof(namebuf), "%s.gb", cartinfo.title);
	gtk_file_chooser_set_current_name(chooser, namebuf);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), gbrom_filter);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *filename;
		int mem_size;

		gtk_widget_hide(dialog);

		filename = gtk_file_chooser_get_filename(chooser);
		mem_size = xferpak_gb_readROM(xpak, &cartinfo, &mem);
		if (mem_size < 0) {
			if (mem_size != XFERPAK_USER_CANCELLED) {
				u->error(xferpak_errStr(mem_size));
			}
		} else {
			FILE *fptr;
			fptr = fopen(filename, "wb");
			if (fptr) {
				printf("Writing to '%s'\n", filename);
				fwrite(mem, mem_size, 1, fptr);
				fclose(fptr);
			} else {
				u->perror(filename);
			}
			free(mem);
		}
		g_free(filename);
	}
	gtk_widget_destroy(dialog);

	xferpak_free(xpak);

	// Resume polling
	rnt_suspendPolling(app->current_adapter_handle, 0);
}

G_MODULE_EXPORT void gui_xferpak_writeram(GtkWidget *win, gpointer data)
{
	struct application *app = data;
	xferpak *xpak;
	uiio *u = getUIIO_gtk(NULL, app->mainwindow);
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	GET_UI_ELEMENT(GtkFileFilter, gbram_filter);
	struct gbcart_info cartinfo;
	gint res;
	u->caption = "Writing cartridge RAM...";

	if (!app->current_adapter_handle)
		return;

	// Suspend polling for better performance
	rnt_suspendPolling(app->current_adapter_handle, 1);


	/* Prepare xferpak */
	xpak = gcn64lib_xferpak_init(app->current_adapter_handle, 0, u);
	if (!xpak) {
		u->error("Transfer Pak not found or no cartridge present");

		// Resume polling
		rnt_suspendPolling(app->current_adapter_handle, 0);
		return;
	}

	res = xferpak_gb_readInfo(xpak, &cartinfo);
	if (res < 0) {
		u->error("Error reading cartridge header");
		xferpak_free(xpak);

		// Resume polling
		rnt_suspendPolling(app->current_adapter_handle, 0);
		return;
	}

	if (cartinfo.ram_size <= 0) {
		u->error("This cartridge does not contain RAM\nNothing to do.");
		xferpak_free(xpak);

		// Resume polling
		rnt_suspendPolling(app->current_adapter_handle, 0);
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
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), gbram_filter);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *filename;

		gtk_widget_hide(dialog);

		filename = gtk_file_chooser_get_filename(chooser);
		res = gcn64lib_xferpak_writeRAM_from_file(app->current_adapter_handle, 0, filename, 1, u);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);

	xferpak_free(xpak);

	// Resume polling
	rnt_suspendPolling(app->current_adapter_handle, 0);
}


G_MODULE_EXPORT void gui_xferpak_showinfo(GtkWidget *win, gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkDialog, gbcart_info_dialog);
	gint res;
	xferpak *xpak;
	struct gbcart_info cartinfo;
	GET_UI_ELEMENT(GtkLabel, lbl_gbcart_title);
	GET_UI_ELEMENT(GtkLabel, lbl_gbcart_cart_type);
	GET_UI_ELEMENT(GtkLabel, lbl_gbcart_rom_size);
	GET_UI_ELEMENT(GtkLabel, lbl_gbcart_ram_size);
	char tmpstr[64];
	uiio *u = getUIIO_gtk(NULL, app->mainwindow);

	if (!app->current_adapter_handle)
		return;

	/* Prepare xferpak */
	xpak = gcn64lib_xferpak_init(app->current_adapter_handle, 0, u);
	if (!xpak) {
		u->error("Transfer Pak not found or no cartridge present");
		return;
	}

	res = xferpak_gb_readInfo(xpak, &cartinfo);
	if (res < 0) {
		xferpak_free(xpak);
		u->error("Failed to read cartridge header");
		return;
	}

	gtk_label_set_text(lbl_gbcart_title, cartinfo.title);
	gtk_label_set_text(lbl_gbcart_cart_type, getCartTypeString(cartinfo.type));
	sprintf(tmpstr, "%d KiB", cartinfo.rom_size / 1024);
	gtk_label_set_text(lbl_gbcart_rom_size, tmpstr);
	sprintf(tmpstr, "%d KiB", cartinfo.ram_size / 1024);
	gtk_label_set_text(lbl_gbcart_ram_size, tmpstr);

	res = gtk_dialog_run(GTK_DIALOG(gbcart_info_dialog));

	gtk_widget_hide(GTK_WIDGET(gbcart_info_dialog));

	xferpak_free(xpak);
}
