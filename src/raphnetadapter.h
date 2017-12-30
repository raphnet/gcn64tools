#ifndef _raphnetadapter_h__
#define _raphnetadapter_h__

#include <wchar.h>
#include <stdint.h>

#define OUR_VENDOR_ID 	0x289b
#define PRODNAME_MAXCHARS	256
#define SERIAL_MAXCHARS		256
#define PATH_MAXCHARS		256

// RNTF : RaphNeT adapter Feature
#define RNTF_FW_UPDATE				0x000001
#define RNTF_BLOCK_IO				0x000002
#define RNTF_SUSPEND_POLLING		0x000004
#define RNTF_POLL_RATE				0x000008
#define RNTF_CONTROLLER_TYPE		0x000010

#define RNTF_GC_FULL_SLIDERS		0x001000
#define RNTF_GC_INVERT_TRIG			0x002000
#define RNTF_TRIGGER_AS_BUTTONS		0x004000
#define RNTF_DPAD_AS_BUTTONS		0x008000
#define RNTF_DPAD_AS_AXES			0x010000
#define RNTF_ADAPTER_MODE			0x020000 // Select between mouse and joystick mode
#define RNTF_MOUSE_INVERT_SCROLL	0x040000
#define RNTF_SWAP_RL_STICKS			0x080000
#define RNTF_NUNCHUK_ACC_ENABLE		0x100000
#define RNTF_DISABLE_ANALOG_TRIGGERS	0x200000

// Flags obtainable through RQ_RNT_GET_SUPPORTED_CFG_PARAMS and company
#define RNTF_DYNAMIC_FEATURES	0x80000000

struct rnt_adap_caps {
	int rpsize; // report size for adapter IO. (Set to non-zero to override default
	int n_channels; // Number of ports. Defaults to 1 (if set to zero)
	int n_raw_channels;

	// Configuration parameters whose availability depends
	// on the firmware version.
	uint32_t features;

	// when non-zero, GUI imposes a 1ms minimum
	int min_poll_interval;
};

struct rnt_dyn_features {
	/* The requests/commands (eg: RQ_RNT_RESET_FIRMWARE) the adapter supports */
	int n_supported_requests;
	uint8_t supported_requests[256];

	/* The modes that can be passed to CFG_PARAM_MODE */
	int n_supported_modes;
	uint8_t supported_modes[256];

	/* Which of CFG_PARAM_* have an effect */
	int n_supported_cfg_params;
	uint8_t supported_cfg_params[256];
};

struct rnt_adap_info {
	wchar_t str_prodname[PRODNAME_MAXCHARS];
	wchar_t str_serial[SERIAL_MAXCHARS];
	char str_path[PATH_MAXCHARS];
	int usb_vid, usb_pid;
	int access; // True unless direct access to read serial/prodname failed due to permissions.
	struct rnt_adap_caps caps;
	int legacy_adapter; // non-openable (hid)
	// Version from USB device descriptor. Most release are in the x.y.z format,
	// so you should use rnt_getVersion() for a complete version.
	uint8_t version_major;
	uint8_t version_minor;
};

struct rnt_adap_list_ctx;

typedef struct _rnt_hdl_t *rnt_hdl_t;

int rnt_init(int verbose);
void rnt_shutdown(void);

struct rnt_adap_list_ctx *rnt_allocListCtx(void);
void rnt_freeListCtx(struct rnt_adap_list_ctx *ctx);
struct rnt_adap_info *rnt_listDevices(struct rnt_adap_info *info, struct rnt_adap_list_ctx *ctx);
int rnt_countDevices(void);

rnt_hdl_t rnt_openDevice(struct rnt_adap_info *dev);

#define GCN64_FLG_OPEN_BY_SERIAL	1	/** Serial must match */
#define GCN64_FLG_OPEN_BY_PATH		2	/** Path must match */
#define GCN64_FLG_OPEN_BY_VID		4	/** USB VID must match */
#define GCN64_FLG_OPEN_BY_PID		8	/** USB PID MUST match */
/**
 * \brief Open a device matching a serial number
 * \param dev The device structure
 * \param flags Flags 
 * \return A handle to the opened device, or NULL if not found
 **/
rnt_hdl_t rnt_openBy(struct rnt_adap_info *dev, unsigned char flags);

void rnt_closeDevice(rnt_hdl_t hdl);

int rnt_send_cmd(rnt_hdl_t hdl, const unsigned char *cmd, int len);
int rnt_poll_result(rnt_hdl_t hdl, unsigned char *cmd, int cmdlen);
int rnt_exchange(rnt_hdl_t hdl, unsigned char *outcmd, int outlen, unsigned char *result, int result_max);

int rnt_suspendPolling(rnt_hdl_t hdl, unsigned char suspend);
int rnt_setConfig(rnt_hdl_t hdl, unsigned char param, unsigned char *data, unsigned char len);
int rnt_getConfig(rnt_hdl_t hdl, unsigned char param, unsigned char *rx, unsigned char rx_max);
int rnt_getVersion(rnt_hdl_t hdl, char *dst, int dstmax);
int rnt_getSignature(rnt_hdl_t hdl, char *dst, int dstmax);
int rnt_forceVibration(rnt_hdl_t hdl, unsigned char channel, unsigned char vibrate);
int rnt_getControllerType(rnt_hdl_t hdl, int chn);
const char *rnt_controllerName(int type);
int rnt_bootloader(rnt_hdl_t hdl);
int rnt_reset(rnt_hdl_t hdl);
int rnt_getSupportedFeatures(rnt_hdl_t hdl, struct rnt_dyn_features *dst_dynfeat);


#endif // _raphnetadapter_h__

