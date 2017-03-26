#ifndef _xferpak_tools_h__
#define _xferpak_tools_h__

#include "gcn64.h"

int gcn64lib_xferpak_readRAM_to_file(gcn64_hdl_t hdl, int channel, const char *output_filename);
int gcn64lib_xferpak_readROM_to_file(gcn64_hdl_t hdl, int channel, const char *output_filename);
int gcn64lib_xferpak_writeRAM_from_file(gcn64_hdl_t hdl, int channel, const char *input_filename, int verify);
int gcn64lib_xferpak_printInfo(gcn64_hdl_t hdl, int channel);

#endif