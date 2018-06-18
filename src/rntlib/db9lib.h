#ifndef _db9lib_h__
#define _db9lib_h__

#include "raphnetadapter.h"
#include <stdint.h>

int db9lib_getPollData(rnt_hdl_t hdl, uint8_t chn, uint8_t *dst, int maxlen);

#endif // _db9lib_h__

