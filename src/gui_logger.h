#ifndef _gui_logger_h__
#define _gui_logger_h__

#include "gcn64ctl_gui.h"

void updatelog_append(const char *message, ...);
void updatelog_appendln(const char *message, ...);
void updatelog_init(struct application *app);
void updatelog_shutdown(void);

#endif // _gui_logger_h__

