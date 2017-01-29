#ifndef _gui_fwupd_h__
#define _gui_fwupd_h__

#include "gcn64ctl_gui.h"

gboolean updateDonefunc(gpointer data);
gboolean updateProgress(gpointer data);
gboolean closeAdapter(gpointer data);
gpointer updateThreadFunc(gpointer data);

#endif
