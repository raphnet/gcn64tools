#ifndef _mempak_fill_h__
#define _mempak_fill_h__

#include <stdint.h>
#include "raphnetadapter.h"

int mempak_fill(rnt_hdl_t hdl, int channel, uint8_t pattern, int no_confirm);

#endif // _mempak_fill_h__
