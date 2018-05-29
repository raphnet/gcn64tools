#ifndef _psxlib_h__
#define _psxlib_h__

#include "raphnetadapter.h"

int psxlib_exchange(rnt_hdl_t hdl, unsigned char channel, unsigned char *tx, unsigned char tx_len, unsigned char *rx, unsigned char max_rx);

#endif // _psxlib_h__
