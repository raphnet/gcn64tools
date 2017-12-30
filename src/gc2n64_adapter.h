#ifndef _gc2n64_adapter_h__
#define _gc2n64_adapter_h__

#include "raphnetadapter.h"

#define GC2N64_MAX_MAPPING_PAIRS	32
#define GC2N64_NUM_MAPPINGS			5

struct gc2n64_adapter_mapping_pair {
	int gc, n64;
};

struct gc2n64_adapter_mapping {
	int n_pairs;
	struct gc2n64_adapter_mapping_pair pairs[GC2N64_MAX_MAPPING_PAIRS];
};

#define GC2N64_CONVERSION_MODE_OLD_1v5	1
#define GC2N64_CONVERSION_MODE_V2		2
#define GC2N64_CONVERSION_MODE_EXTENDED	3

#define ADAPTER_TYPE_GC_TO_N64		0
#define ADAPTER_TYPE_SNES_TO_N64	1
#define ADAPTER_TYPE_CLASSIC_TO_N64	2
#define ADAPTER_TYPE_SNES_TO_GC		3
#define ADAPTER_TYPE_N64_TO_GC		4
#define ADAPTER_TYPE_CLASSIC_TO_GC	5

const char *x2gcn64_adapter_type_name(int t);

struct gc2n64_adapter_info {
	unsigned char default_mapping_id;
	unsigned char deadzone_enabled;
	unsigned char old_v1_5_conversion;
	// conversion_mode: New field in v2.1. If non-zero, old_v1_5_conversion should be ignored.
	unsigned char conversion_mode;
	// gc_controller_detected: New field in v2.2. Always zero on previous versions.
	unsigned char gc_controller_detected;
	struct gc2n64_adapter_mapping mappings[GC2N64_NUM_MAPPINGS];
};

struct snes2gc_adapter_info {
};

struct x2gcn64_adapter_info_app {
	uint8_t adapter_type;
	unsigned char upgradeable;
	char version[16];
	union {
		struct gc2n64_adapter_info gc2n64;
		struct snes2gc_adapter_info snes2gc;
	};
};

struct x2gcn64_adapter_info_bootloader {
	char version[16];
	unsigned char mcu_page_size;
	unsigned short bootloader_start_address;
};

struct x2gcn64_adapter_info {
	int in_bootloader;
	union {
		struct x2gcn64_adapter_info_app app;
		struct x2gcn64_adapter_info_bootloader bootldr;
	};
};

/* Generic/Common functions */
int x2gcn64_adapter_echotest(rnt_hdl_t hdl, int channel, int verbose);
int x2gcn64_adapter_getInfo(rnt_hdl_t hdl,  int channel, struct x2gcn64_adapter_info *inf);
void x2gcn64_adapter_printInfo(struct x2gcn64_adapter_info *inf);

int x2gcn64_adapter_waitNotBusy(rnt_hdl_t hdl, int channel, int verbose);
int x2gcn64_adapter_boot_isBusy(rnt_hdl_t hdl, int channel);
int x2gcn64_adapter_boot_eraseAll(rnt_hdl_t hdl, int channel);
int x2gcn64_adapter_boot_readBlock(rnt_hdl_t hdl, int channel, unsigned int block_id, unsigned char dst[32]);
int x2gcn64_adapter_dumpFlash(rnt_hdl_t hdl, int channel);
int x2gcn64_adapter_enterBootloader(rnt_hdl_t hdl, int channel);
int x2gcn64_adapter_bootApplication(rnt_hdl_t hdl, int channel);
int x2gcn64_adapter_sendFirmwareBlocks(rnt_hdl_t hdl, int channel, unsigned char *firmware, int len);
int x2gcn64_adapter_verifyFirmware(rnt_hdl_t hdl, int channel, unsigned char *firmware, int len);
int x2gcn64_adapter_waitForBootloader(rnt_hdl_t hdl, int channel, int timeout_s);


/* Gamecube to N64 adapter specific */
void gc2n64_adapter_printMapping(struct gc2n64_adapter_mapping *map);
const char *gc2n64_adapter_getConversionModeName(struct gc2n64_adapter_info *inf);

#define MAPPING_SLOT_BUILTIN_CURRENT	0
#define MAPPING_SLOT_DPAD_UP			1
#define MAPPING_SLOT_DPAD_DOWN			2
#define MAPPING_SLOT_DPAD_LEFT			3
#define MAPPING_SLOT_DPAD_RIGHT			4
const char *gc2n64_adapter_getMappingSlotName(unsigned char id, int default_context);

int gc2n64_adapter_getMapping(rnt_hdl_t hdl, int channel, int mapping_id, struct gc2n64_adapter_mapping *dst_mapping);
int gc2n64_adapter_setMapping(rnt_hdl_t hdl, int channel, struct gc2n64_adapter_mapping *mapping);
int gc2n64_adapter_storeCurrentMapping(rnt_hdl_t hdl, int channel, int dst_slot);

int gc2n64_adapter_saveMapping(struct gc2n64_adapter_mapping *map, const char *dstfile);
struct gc2n64_adapter_mapping *gc2n64_adapter_loadMapping(const char *srcfile);

int gc2n64_adapter_updateFirmware(rnt_hdl_t hdl, int channel, const char *hexfile);

#endif // _gc2n64_adapter_h__

