#ifndef _gui_dfu_programmer_h__
#define _gui_dfu_programmer_h__

/** \brief Ok! */
#define DFU_OK				0
/** \brief Failed to execute the tool */
#define DFU_POPEN_FAILED	-1
/** \brief The tool returned an error (eg: No device found) */
#define DFU_ERROR_RETURN	-2

int dfu_wrapper(const char *command, void (*lineCallback)(void *ctx, const char *ln), void *ctx);

#endif // _gui_dfu_programmer_h__
