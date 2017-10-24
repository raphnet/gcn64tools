#ifndef _gcn64_requests_h__
#define _gcn64_requests_h__

/* Generic commands */
#define RQ_GCN64_SET_CONFIG_PARAM		0x01
#define RQ_GCN64_GET_CONFIG_PARAM		0x02
#define RQ_GCN64_SUSPEND_POLLING		0x03
#define RQ_GCN64_GET_VERSION			0x04
#define RQ_GCN64_GET_SIGNATURE			0x05
#define RQ_GCN64_GET_CONTROLLER_TYPE	0x06
#define RQ_GCN64_SET_VIBRATION			0x07
#define RQ_GCN64_RESET_FIRMWARE			0xFE
#define RQ_GCN64_JUMP_TO_BOOTLOADER		0xFF

/* N64/GC specific */
#define RQ_GCN64_RAW_SI_COMMAND			0x80
#define RQ_GCN64_BLOCK_IO				0x81

/* Get controller type values */
// Legacy values (GC/N64 specific)
#define CTL_TYPE_NONE	0
#define CTL_TYPE_N64	1
#define CTL_TYPE_GC		2
#define CTL_TYPE_GCKB	3
// New values (follows PAD_TYPE_* + 100)
#define CTL_TYPE_NONE_NEW	100
#define CTL_TYPE_CLASSIC	101
#define	CTL_TYPE_SNES		102
#define CTL_TYPE_NES		103
#define CTL_TYPE_N64_NEW	104
#define CTL_TYPE_GAMECUBE_NEW	105
#define CTL_TYPE_MD			106
#define CTL_TYPE_SMS		107
#define CTL_TYPE_SNES_NDK10	108
#define CTL_TYPE_SNES_MOUSE	109
#define CTL_TYPE_PCE		110
#define CTL_TYPE_PCE6		111
#define CTL_TYPE_NUNCHUK	112

/* Configuration parameters and constants */
#define CFG_PARAM_MODE			0x00

/* Values for mode */
#define CFG_MODE_STANDARD   	0x00
#define CFG_MODE_N64_ONLY		0x01
#define CFG_MODE_GC_ONLY		0x02

#define CFG_MODE_2P_STANDARD	0x10
#define CFG_MODE_2P_N64_ONLY	0x11
#define CFG_MODE_2P_GC_ONLY		0x12

#define CFG_MODE_MOUSE			0x20

#define CFG_PARAM_RESERVED		0x00
#define CFG_PARAM_SERIAL		0x01

#define CFG_PARAM_POLL_INTERVAL0	0x10
#define CFG_PARAM_POLL_INTERVAL1	0x11
#define CFG_PARAM_POLL_INTERVAL2	0x12
#define CFG_PARAM_POLL_INTERVAL3	0x13

#define CFG_PARAM_N64_SQUARE		0x20 // Not implemented
#define CFG_PARAM_GC_MAIN_SQUARE	0x21 // Not implemented
#define CFG_PARAM_GC_CSTICK_SQUARE	0x22 // Not implemented
#define CFG_PARAM_FULL_SLIDERS		0x23
#define CFG_PARAM_INVERT_TRIG		0x24
#define CFG_PARAM_TRIGGERS_AS_BUTTONS	0x25
#define CFG_PARAM_MOUSE_INVERT_SCROLL	0x26
// eg: Swap left and right sticks on classic controller
#define CFG_PARAM_SWAP_STICKS		0x27

#define CFG_PARAM_DPAD_AS_BUTTONS		0x30
#define CFG_PARAM_DPAD_AS_AXES			0x31


#endif
