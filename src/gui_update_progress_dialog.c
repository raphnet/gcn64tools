#include <stdio.h>
#include "gcn64ctl_gui.h"
#include "gui_update_progress_dialog.h"

gboolean updateProgress(gpointer data)
{
	struct application *app = data;

	GET_UI_ELEMENT(GtkProgressBar, updateProgress);
	GET_UI_ELEMENT(GtkLabel, updateStatus);

	gtk_progress_bar_set_fraction(updateProgress, app->update_percent / 100.0);
	gtk_label_set_text(updateStatus, app->update_status);

	return FALSE;
}

/**
 * \brief Setup and run the update progress dialog.
 *
 * This dialog displays the file that will be used for the update,
 * a status (Initially "Ready"), a progress bar and two buttons (Start and Cancel).
 *
 * When start is pressed, a thread is started (see the threadfunc parameter).
 *
 * During the update process, the thread should use setUpdateStatus() to update
 * the progression and the message.
 *
 * Before exiting, the update thread is to call updateThreadDone()
 *
 * \param app The application data structure
 * \param filename The filename to display as being used for the update.
 * \param threadfunc Pointer to the update function.
 */
gint update_progress_dialog_run(struct application *app, GtkWindow *parent, const char *filename, GThreadFunc threadfunc)
{
	GET_UI_ELEMENT(GtkLabel, lbl_firmware_filename);
	GET_UI_ELEMENT(GtkDialog, firmware_update_dialog);
	GET_UI_ELEMENT(GtkButtonBox, update_dialog_btnBox);
	gint res;

	app->updater_thread_func = threadfunc;

	gtk_window_set_transient_for(GTK_WINDOW(firmware_update_dialog), parent);
	gtk_label_set_text(lbl_firmware_filename, filename);
	gtk_widget_set_sensitive(GTK_WIDGET(update_dialog_btnBox), TRUE);
	app->update_percent = 0;
	app->update_status = "Ready";
	updateProgress((gpointer)app);

	updatelog_append("Running the update dialog...\n");
	res = gtk_dialog_run(firmware_update_dialog);
	gtk_widget_hide( GTK_WIDGET(firmware_update_dialog));

	return res;
}

/* Called when the 'Start update' button is clicked in the firmware update dialog */
G_MODULE_EXPORT void updatestart_btn_clicked_cb(GtkWidget *w, gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkButtonBox, update_dialog_btnBox);

	app->updater_thread = g_thread_new("updater", app->updater_thread_func, app);
	gtk_widget_set_sensitive(GTK_WIDGET(update_dialog_btnBox), FALSE);
}

static gboolean updateDonefunc(gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkDialog, firmware_update_dialog);

	printf("updateDonefunc\n");
	gtk_dialog_response(firmware_update_dialog, app->update_dialog_response);
	g_thread_join(app->updater_thread);

#if 0
	rebuild_device_list_store(data);
	syncGuiToCurrentAdapter(app);
	app->inhibit_periodic_updates = 0;
#endif

	return FALSE;
}


/** \brief The update thread must call this function before exiting.
 *
 * Call this causes the main thread to join the updater thread. The dialog
 * box is also closed, returning 'dialog_result'.
 *
 * \param app The application structure
 * \param dialog_result GTK_RESPONSE_REJECT or GTK_RESPONSE_OK
 */
void updateThreadDone(struct application *app, int dialog_result)
{
	app->update_dialog_response = dialog_result;
	gdk_threads_add_idle(updateDonefunc, (gpointer)app);
}

/** \brief Set the update status fields
 *
 * Update the fields in app and schedule a call to updateProgress for updating the GUI.
 *
 * \param app Application context
 * \param status_str The status string. (If NULL, current status string stays unchanged)
 * \param percent The current progress given in percent.
 */
void setUpdateStatus(struct application *app, const char *status_str, int percent)
{
	app->update_percent = percent;
	if (status_str) {
		app->update_status = status_str;
	}

	updatelog_append("Update status [%d%%] : %s\n", percent, app->update_status);

	gdk_threads_add_idle(updateProgress, app);
}


