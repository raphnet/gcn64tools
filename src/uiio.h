#ifndef _uiio_h__
#define _uiio_h__

#include <stdint.h>

/* Type definitions for ask() */
#define UIIO_YESNO		1
#define UIIO_NOYES		2

/* Return defines for ask() */
#define UIIO_NO			0
#define UIIO_YES		1

#define PROGRESS_TYPE_ADDRESS	0
#define PROGRESS_TYPE_PERCENT	1

#define UIIO_PROGRESS_STOPPED		0
#define UIIO_PROGRESS_STARTED		1
#define UIIO_PROGRESS_CANCELLED		2

typedef struct _uiio {
	/**
	 * \brief Used to ask the user to confirm something before proceeding.
	 *
	 * \param Question to ask. Should be formulated such that a negative answer means not confirmed.
	 * \param type Type of questions and default answer. (UIIO_YESNO Y/n, UIIO_NOYES y/N)
	 *
	 * \return true if confirmed ok
	 */
	int (*ask)(int type, const char *fmt, ...);

	/**
	 * \brief Used to report an error.
	 * \param fmt Format string.
	 */
	int (*error)(const char *fmt, ...);

	/**
	 * \brief Print a system error message
	 * \param s String to be printer before the system error string.
	 */
	void (*perror)(const char *s);

	/**
	 * \param Used for verbose and normal printing.
	 *
	 * May not be visible in GUIs, but could be logged.
	 */
	int (*printf)(const char *fmt, ...);

	/* Progress parameters */
	const char *caption;
	int progress_type;
	uint32_t cur_progress;
	uint32_t max_progress;

	// Indicates the progress will restart for another operation. Used to prevent
	// waiting for the user to close the progress window. (i.e. Write then verify)
	int multi_progress;

	/** Call this once after setting cur_progress and max_progress, before calling update */
	void (*progressStart)(struct _uiio *u);
	/** Call update when progress changes. You are responsible to call it at a reasonable frequency.
	 * Should not display anything when progress_started is false. Return non-zero to cancel. */
	int (*update)(struct _uiio *u);
	/** Call this once after the operation is completed */
	void (*progressEnd)(struct _uiio *u, const char *msg);


	/** Indicate progress has started, has been cancelled, etc. set by progressStart/End */
	int progress_status;
} uiio;

/** \brief Initizlize a uiio object with default (stdio) implementations */
void uiio_init_std(uiio *u);

/** \param Get a valid uiio pointer.
 * \return Returns u if not NULL, otherwise a pointer to a default structure using stdio is returned */
uiio *getUIIO(uiio *u);

#endif // _uiio_h__
