#ifndef CX_DBG_H
#define CX_DBG_H

#include <stdlib.h>

#include "cx_logging.h"
#include "cx_macro.h"

#ifdef NDEBUG

#define CX_DBG(X) ((void)0)
#define CX_ASSERT(X, LOG_CAT) ((void)(X))
#define CX_ASSERT_MSG(X, LOG_CAT, MSG) ((void)(X))
#define CX_ASSERT_MSG_FMT(X, LOG_CAT, MSG, ...) ((void)(X))

#else

#define CX_DBG(X) \
	do { \
		X; \
	} while(0)

#define CX_ASSERT(X, LOG_CAT) \
	do { \
		if (!(X)) {\
			CX_LOG_FMT(ERROR, LOG_CAT, "ASSERTION FAILED | " CX_STRINGIFY(X) " | " CX_FILE_LINE " (%s)\n", \
				__func__); \
			abort(); \
		} \
	} while(0)

#define CX_ASSERT_MSG(X, LOG_CAT, MSG) \
	do { \
		if (!(X)) { \
			CX_LOG_FMT(ERROR, LOG_CAT, "ASSERTION FAILED | " CX_STRINGIFY(X) " | " MSG " | " CX_FILE_LINE " (%s)\n", \
				__func__); \
		abort(); \
		} \
	} while(0)

#define CX_ASSERT_MSG_FMT(X, LOG_CAT, MSG, ...) \
	do { \
		if (!(X)) { \
			CX_LOG_FMT(ERROR, LOG_CAT, \
				"ASSERTION FAILED | " CX_STRINGIFY(X) " | " MSG " | " CX_FILE_LINE " (%s)\n", \
				__VA_ARGS__, __func__); \
			abort(); \
		} \
	} while(0)

#endif

#endif
