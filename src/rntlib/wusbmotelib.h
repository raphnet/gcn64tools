#ifndef wusbmotelib_h__
#define wusbmotelib_h__

#include "raphnetadapter.h"
#include <stdint.h>

#define ID_NUNCHUK	0x0000
#define ID_CLASSIC	0x0001
#define ID_CLASSIC_PRO	0x0101
#define ID_GH_GUITAR	0x0003 // GH3 or GHWT
#define ID_UDRAW		0xFF12
#define ID_DRAWSOME		0xFF13
#define ID_DJHERO		0x0303

#define PAD_TYPE_CLASSIC    1
#define PAD_TYPE_CLASSIC_PRO	13

typedef struct _classic_pad_data {
    uint8_t pad_type; // PAD_TYPE_CLASSIC and PAD_TYPE_CLASSIC_PRO
    int8_t high_resolution;
    int8_t lx, ly;
    int8_t rx, ry;
    uint8_t lt, rt;
    uint16_t buttons;
} classic_pad_data;

#define CPAD_BTN_DPAD_RIGHT 0x8000
#define CPAD_BTN_DPAD_DOWN  0x4000
#define CPAD_BTN_DPAD_LEFT  0x0002
#define CPAD_BTN_DPAD_UP    0x0001
#define CPAD_BTN_MINUS      0x1000
#define CPAD_BTN_HOME       0x0800
#define CPAD_BTN_PLUS       0x0400
#define CPAD_BTN_TRIG_LEFT  0x2000
#define CPAD_BTN_TRIG_RIGHT 0x0200
#define CPAD_BTN_RSVD_HIGH  0x0100
#define CPAD_BTN_B          0x0040
#define CPAD_BTN_Y          0x0020
#define CPAD_BTN_A          0x0010
#define CPAD_BTN_X          0x0008
#define CPAD_BTN_ZL         0x0080
#define CPAD_BTN_ZR         0x0004

typedef struct _nunchuk_pad_data {
    uint8_t pad_type; // PAD_TYPE_NUNCHUK
    int8_t sx, sy; // stick
    int16_t ax, ay, az; // Acceleration
    uint16_t buttons;
} nunchuk_pad_data;

#define NUNCHK_BTN_Z    0x0001
#define NUNCHK_BTN_C    0x0002

typedef struct _udraw_data {
	uint16_t x, y;
	uint8_t pressure;
	uint8_t buttons;
} udraw_tablet_data;

#define UDRAW_BTN_UPPER	0x01
#define UDRAW_BTN_LOWER	0x02
#define UDRAW_BTN_NIB	0x04

typedef struct _drawsom_data {
	uint16_t x, y;
	uint16_t pressure;
	uint8_t status;
} drawsome_tablet_data;

typedef struct _djhero_data {
	uint8_t x, y;
	uint8_t left_turntable, right_turntable;
	uint8_t effect_dial;
	uint8_t crossfade;
	uint16_t buttons;
} djhero_turntable_data;

struct i2c_transaction {
	/** Adapter channel / port number */
	uint8_t chn;
	/** I2C device address (right-aligned) */
	uint8_t addr;
	/** Number of bytes to write */
	uint8_t wr_len;
	/** Number of bytes to read */
	uint8_t rd_len;
	/** Data to write */
	const uint8_t *wr_data;
	/** Destination for read data */
	uint8_t *rd_data;
	/** Result for this transaction (0 = success) */
	uint8_t result;
};

/** Process a group of transaction.
 *
 * This creates a single request, sends it to the adapter who
 * performs the transactions and provides the result as a group.
 *
 * \param hdl The adapter handle
 * \param transactions Pointer to an array of i2c_transaction.
 * \param n_transactions Number of transactions.
 * */
int wusbmote_i2c_transactions(rnt_hdl_t hdl, struct i2c_transaction *transactions, int n_transactions);

#define wusbmote_i2c_transaction(hdl, i2c)	wusbmote_i2c_transactions(hdl, i2c, 1)

/** Scan all I2C addresses to detect chip presence
 *
 * \param hdl The adapter handle
 * \param chn The channel
 * \param dstBuf Array of 128 values indicating presence (non-zero) or basence. Set to NULL if not needed.
 * \param verbose When non-zero, displays scan result in i2cdetect-style format.
 */
int wusbmotelib_i2c_detect(rnt_hdl_t hdl, uint8_t chn, uint8_t *dstBuf, char verbose);

/** Perform the two magic writes that disable encryption/scrambling.
 *
 * As described here:
 *    http://wiibrew.org/wiki/Wiimote/Extension_Controllers
 */
int wusbmotelib_disableEncryption(rnt_hdl_t hdl, uint8_t chn);

/** Read all 256 memory location/registers of a Wiimote extension device
 *
 * \param hdl The adapter handle
 * \param chn The channel
 * \param dstBuf Array of 256 bytes. Set to NULL if not interested.
 * \param verbose When non-zero, write the content to stdout in hexdump format.
 */
int wusbmotelib_dumpMemory(rnt_hdl_t hdl, uint8_t chn, uint8_t *dstBuf, char verbose);

/** Read registers
 *
 * \param hdl The adapter handle
 * \param chn The channel
 * \param start_reg The address of the first register to read
 * \param dstBuf The destination buffer
 * \param n_regs The number of registers to read
 */
int wusbmotelib_readRegs(rnt_hdl_t hdl, uint8_t chn, uint8_t start_reg, uint8_t *dstBuf, int n_regs);

/** Write registers
 *
 * \param hdl The adapter handle
 * \param chn The channel
 * \param start_reg The address of the first register to write
 * \param srctBuf The source buffer
 * \param n_regs The number of registers to write
 */
int wusbmotelib_writeRegs(rnt_hdl_t hdl, uint8_t chn, uint8_t start_reg, const uint8_t *srcBuf, int n_regs);


/** Convert a status buffer to a structure
 *
 * \param buf The input buffer (min. 8 bytes)
 * \param dst The destination structure
 * \param peripheral_id The peripheral ID as read from addresses FA and FF (0xFAFF)
 * \param high_res_mode Non-zero if high resolution is used.
 */
void wusbmotelib_bufferToClassicPadData(const uint8_t *buf, classic_pad_data *dst, uint16_t peripheral_id, uint8_t high_res_mode);

/** Convert a status buffer to a structure
 *
 * \param buf The input buffer (min. 8 bytes)
 * \param dst The destination structure
 */
void wusbmotelib_bufferToUdrawData(const uint8_t *buf, udraw_tablet_data *dst);

/** Convert a status buffer to a structure
 *
 * \param buf The input buffer (min. 6 bytes)
 * \param dst The destination structure
 */
void wusbmotelib_bufferToDrawsomeData(const uint8_t *buf, drawsome_tablet_data *dst);

void wusbmotelib_bufferToTurntableData(const uint8_t *buf, djhero_turntable_data *dst);

void wusbmotelib_bufferToNunchukPadData(const uint8_t *buf, nunchuk_pad_data *dst);

#endif // _wusbmotelib_h__

