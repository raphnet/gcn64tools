#ifndef _gui_update_progress_dialog_h__
#define _gui_update_progress_dialog_h__

#include "gcn64ctl_gui.h"

gboolean updateProgress(gpointer data);
gint update_progress_dialog_run(struct application *app, GtkWindow *parent, const char *filename, GThreadFunc threadfunc);

void setUpdateStatus(struct application *app, const char *status_str, int percent);
void updateThreadDone(struct application *app, int dialog_result);

#endif // _gui_update_progress_dialog_h__
