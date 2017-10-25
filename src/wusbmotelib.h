#ifndef wusbmotelib_h__
#define wusbmotelib_h__

#include "raphnetadapter.h"
#include <stdint.h>

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

#endif // _wusbmotelib_h__

