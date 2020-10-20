#ifndef _maplelib_h__
#define _maplelib_h__

#include "raphnetadapter.h"
#include <stdint.h>

int maple_sendFrame1W(rnt_hdl_t hdl, uint8_t chn, uint8_t cmd, uint8_t dst_addr, uint8_t src_addr, uint32_t data, uint8_t *dst, int maxlen, uint8_t flags);

#define MAPLE_FLAG_KEEP_DATA	1
#define MAPLE_FLAG_SLOW_RX		2
#define MAPLE_FLAG_MOUSE_RX		4

#define MAPLE_CMD_RESET_DEVICE	3
#define MAPLE_CMD_GET_CONDITION	9

#define MAPLE_FUNC_CONTROLLER   0x001
#define MAPLE_FUNC_MEMCARD      0x002
#define MAPLE_FUNC_LCD          0x004
#define MAPLE_FUNC_CLOCK        0x008
#define MAPLE_FUNC_MIC          0x010
#define MAPLE_FUNC_AR_GUN       0x020
#define MAPLE_FUNC_KEYBOARD     0x040
#define MAPLE_FUNC_LIGHT_GUN    0x080
#define MAPLE_FUNC_PURUPURU     0x100
#define MAPLE_FUNC_MOUSE        0x200

#define MAPLE_ADDR_PORT(id)     ((id)<<6)
#define MAPLE_ADDR_PORTA        MAPLE_ADDR_PORT(0)
#define MAPLE_ADDR_PORTB        MAPLE_ADDR_PORT(1)
#define MAPLE_ADDR_PORTC        MAPLE_ADDR_PORT(2)
#define MAPLE_ADDR_PORTD        MAPLE_ADDR_PORT(3)
#define MAPLE_ADDR_MAIN         0x20
#define MAPLE_ADDR_SUB(id)      ((1)<<id) /* where id is 0 to 4 */

#define MAPLE_DC_ADDR			0

#endif // _maplelib_h__

