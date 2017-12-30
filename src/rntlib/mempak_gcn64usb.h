#ifndef _mempak_gcn64usb_h__
#define _mempak_gcn64usb_h__

#include "mempak.h"

uint16_t pak_address_crc( uint16_t address );

int gcn64lib_mempak_detect(rnt_hdl_t hdl);
int gcn64lib_mempak_readBlock(rnt_hdl_t hdl, unsigned short addr, unsigned char dst[32]);
int gcn64lib_mempak_writeBlock(rnt_hdl_t hdl, unsigned short addr, const unsigned char data[32]);

int gcn64lib_mempak_download(rnt_hdl_t hdl, int channel, mempak_structure_t **mempak, int (*progressCb)(int cur_addr, void *ctx), void *ctx);
int gcn64lib_mempak_upload(rnt_hdl_t hdl, int channel, mempak_structure_t *pak, int (*progressCb)(int cur_addr, void *ctx), void *ctx);

#endif // _mempak_gcn64usb_h__
