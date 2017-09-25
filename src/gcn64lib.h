#ifndef _gcn64_lib_h__
#define _gcn64_lib_h__

#include "raphnetadapter.h"

#define CTL_TYPE_NONE	0
#define CTL_TYPE_N64	1
#define CTL_TYPE_GC		2
#define CTL_TYPE_GCKB	3

int gcn64lib_suspendPolling(rnt_hdl_t hdl, unsigned char suspend);
int gcn64lib_setConfig(rnt_hdl_t hdl, unsigned char param, unsigned char *data, unsigned char len);
int gcn64lib_getConfig(rnt_hdl_t hdl, unsigned char param, unsigned char *rx, unsigned char rx_max);
int gcn64lib_rawSiCommand(rnt_hdl_t hdl, unsigned char channel, unsigned char *tx, unsigned char tx_len, unsigned char *rx, unsigned char max_rx);
int gcn64lib_getVersion(rnt_hdl_t hdl, char *dst, int dstmax);
int gcn64lib_getSignature(rnt_hdl_t hdl, char *dst, int dstmax);
int gcn64lib_forceVibration(rnt_hdl_t hdl, unsigned char channel, unsigned char vibrate);
int gcn64lib_getControllerType(rnt_hdl_t hdl, int chn);
const char *gcn64lib_controllerName(int type);
int gcn64lib_bootloader(rnt_hdl_t hdl);

int gcn64lib_n64_expansionWrite(rnt_hdl_t hdl, unsigned short addr, const unsigned char *data, int len);
int gcn64lib_n64_expansionRead(rnt_hdl_t hdl, unsigned short addr, unsigned char *dst, int max_len);

int gcn64lib_8bit_scan(rnt_hdl_t hdl, unsigned char min, unsigned char max);
int gcn64lib_16bit_scan(rnt_hdl_t hdl, unsigned short min, unsigned short max);

#define BIO_RXTX_MASK		0x3F
#define BIO_RX_LEN_TIMEDOUT	0x80
#define BIO_RX_LEN_PARTIAL	0x40

struct blockio_op {
	unsigned char chn;
	unsigned char tx_len;
	unsigned char *tx_data;
	unsigned char rx_len; // expected/max bytes
	unsigned char *rx_data;
};

int gcn64lib_blockIO(rnt_hdl_t hdl, struct blockio_op *iops, int n_iops);

#endif // _gcn64_lib_h__
