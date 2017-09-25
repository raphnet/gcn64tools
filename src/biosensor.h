#ifndef _bio_sensor_h__
#define _bio_sensor_h__

#include "raphnetadapter.h"

int gcn64lib_biosensorPoll(rnt_hdl_t hdl);
int gcn64lib_biosensorMonitor(rnt_hdl_t hdl);

#endif // _bio_sensor_h__
