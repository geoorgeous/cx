#ifndef CX_DBG_H
#define CX_DBG_H

#include <stdlib.h>

#include "cx_logging.h"
#include "cx_macro.h"

#ifdef NDEBUG
#define CX_DBG(X) ((void)0)
#else
#define CX_DBG(X) do {X;} while(0)
#endif

#define CX_ASSERT(X) CX_DBG(\
	if (!(X)) {\
		CX_LOG_FMT(ERROR, DONTCARE, "ASSERTION FAILED | " CX_STRINGIFY(X) " | " CX_FILE_LINE " (%s)\n", __func__);\
		abort();\
	})

#define CX_ASSERT_MSG(X, MSG) CX_DBG(\
	if (!(X)) {\
		CX_LOG_FMT(ERROR, DONTCARE, "ASSERTION FAILED | " CX_STRINGIFY(X) " | " MSG " | " CX_FILE_LINE " (%s)\n",\
			__func__);\
		abort();\
	})

#define CX_ASSERT_MSG_FMT(X, MSG, ...) CX_DBG(\
	if (!(X)) {\
		CX_LOG_FMT(ERROR, DONTCARE,\
			"ASSERTION FAILED | " CX_STRINGIFY(X) " | " MSG " | " CX_FILE_LINE " (%s)\n",\
			__VA_ARGS__, __func__);\
		abort();\
	})

#endif
