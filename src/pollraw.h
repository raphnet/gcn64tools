#ifndef _pollraw_h__
#define _pollraw_h__

int pollraw_gamecube_keyboard(rnt_hdl_t hdl, int chn);
int pollraw_gamecube(rnt_hdl_t hdl, int chn);
int pollraw_randnet_keyboard(rnt_hdl_t hdl, int chn);
int pollraw_psx(rnt_hdl_t hdl, int chn);
int pollraw_wii(rnt_hdl_t hdl, int chn);
int pollraw_db9(rnt_hdl_t hdl, int chn);

#endif // _pollraw_h__
