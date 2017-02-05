#ifndef _gcn64ctl_gui_h__
#define _gcn64ctl_gui_h__

#include <glib.h>
#include <gtk/gtk.h>

#include "gcn64.h"
#include "gcn64lib.h"
#include "gui_mpkedit.h"
#include "gui_fwupd.h"
#include "gui_logger.h"
#include "gui_dfu_programmer.h"

#define GET_ELEMENT(TYPE, ELEMENT)	(TYPE *)gtk_builder_get_object(app->builder, #ELEMENT)
#define GET_UI_ELEMENT(TYPE, ELEMENT)   TYPE *ELEMENT = GET_ELEMENT(TYPE, ELEMENT)


struct application {
	GtkBuilder *builder;
	GtkWindow *mainwindow;

	gcn64_hdl_t current_adapter_handle;
	struct gcn64_info current_adapter_info;

	GThreadFunc updater_thread_func;
	GThread *updater_thread;

	int recovery_mode;
	const char *update_status;
	const char *updateHexFile;
	int update_percent;
	int update_dialog_response;

	struct mpkedit_data *mpke;
	int stop_mempak_io;
	int inhibit_periodic_updates;
	int controller_type;
	int firmware_maj, firmware_min, firmware_build;
	int at90usb1287;
};

void errorPopup(struct application *app, const char *message);
void infoPopup(struct application *app, const char *message);

/** Deselect the current adapter */
void deselect_adapter(struct application *app);

/** Based on app->current_adapter_handle, update GUI elements */
void syncGuiToCurrentAdapter(struct application *app);

/** Scan for device and rebuild the list for the UI */
gboolean rebuild_device_list_store(gpointer data);

#endif // _gcn64ctl_gui_h__
