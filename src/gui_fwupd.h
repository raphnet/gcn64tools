#ifndef _gui_fwupd_h__
#define _gui_fwupd_h__

#include "gcn64ctl_gui.h"

gboolean closeAdapter(gpointer data);

void fwupd_firmwareFolderShortcutAndSet(GtkFileChooser *chooser, struct application *app, const char *sig);

#endif
