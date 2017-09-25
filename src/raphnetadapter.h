#ifndef _raphnetadapter_h__
#define _raphnetadapter_h__

#include <wchar.h>

#define OUR_VENDOR_ID 	0x289b
#define PRODNAME_MAXCHARS	256
#define SERIAL_MAXCHARS		256
#define PATH_MAXCHARS		256

struct rnt_adap_caps {
	int rpsize; // report size for adapter IO. (Set to non-zero to override default
	int n_raw_channels;
	int bio_support;

	// Configuration parameters whose availability depends
	// on the firmware version.
	int gc_full_sliders;
	int gc_invert_trig;
	int triggers_as_buttons;
	int dpad_as_buttons;
};

struct rnt_adap_info {
	wchar_t str_prodname[PRODNAME_MAXCHARS];
	wchar_t str_serial[SERIAL_MAXCHARS];
	char str_path[PATH_MAXCHARS];
	int usb_vid, usb_pid;
	int access; // True unless direct access to read serial/prodname failed due to permissions.
	struct rnt_adap_caps caps;
};

struct rnt_adap_list_ctx;

typedef struct _rnt_hdl_t *rnt_hdl_t; // Cast from hid_device

int rnt_init(int verbose);
void rnt_shutdown(void);

struct rnt_adap_list_ctx *rnt_allocListCtx(void);
void rnt_freeListCtx(struct rnt_adap_list_ctx *ctx);
struct rnt_adap_info *gcn64_listDevices(struct rnt_adap_info *info, struct rnt_adap_list_ctx *ctx);
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


#endif // _raphnetadapter_h__

