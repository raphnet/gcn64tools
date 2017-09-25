#ifndef _gcn64_priv_h__
#define _gcn64_priv_h__

#include "hidapi.h"
#include "raphnetadapter.h"

struct rnt_adap_list_ctx {
	struct hid_device_info *devs, *cur_dev;
};

typedef struct _rnt_hdl_t {
	hid_device *hdev;
	int report_size;
	struct rnt_adap_caps caps;
} *rnt_hdl_t;

#endif
