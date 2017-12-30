#ifndef _xferpak_tools_h__
#define _xferpak_tools_h__

#include "raphnetadapter.h"
#include "uiio.h"

int gcn64lib_xferpak_readRAM_to_file(rnt_hdl_t hdl, int channel, const char *output_filename, uiio *u);
int gcn64lib_xferpak_readROM_to_file(rnt_hdl_t hdl, int channel, const char *output_filename, uiio *u);
int gcn64lib_xferpak_writeRAM_from_file(rnt_hdl_t hdl, int channel, const char *input_filename, int verify, uiio *u);
int gcn64lib_xferpak_printInfo(rnt_hdl_t hdl, int channel);

#endif
