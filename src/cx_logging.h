#ifndef _H__CX_LOGGING
#define _H__CX_LOGGING

#define CX_LOG_LEVEL_SILENT   -1
#define CX_LOG_LEVEL_ALL       0
#define CX_LOG_LEVEL_UNDEFINED 0
#define CX_LOG_LEVEL_TRACE     1
#define CX_LOG_LEVEL_INFO      2
#define CX_LOG_LEVEL_WARNING   3
#define CX_LOG_LEVEL_ERROR     4

#define CX_LOG_LABEL_UNDEFINED "  *  *  "
#define CX_LOG_LABEL_TRACE     "   trace"
#define CX_LOG_LABEL_INFO      "    info"
#define CX_LOG_LABEL_WARNING   " warning"
#define CX_LOG_LABEL_ERROR     "   ERROR"

#define CX_LOG_CAT_ALL      0
#define CX_LOG_CAT_DONTCARE 0

#define CX_LOG_CAT_LOGGING  "logging"

/**
 * shorthand macros
 */

#define CX_LOG(LEVEL, CAT, MSG) (cx_log(CX_LOG_LEVEL_##LEVEL, CX_LOG_CAT_##CAT, MSG))
#define CX_LOG_FMT(LEVEL, CAT, MSG, ...) (cx_log_fmt(CX_LOG_LEVEL_##LEVEL, CX_LOG_CAT_##CAT, MSG, __VA_ARGS__))

#include "cx_dbg.h"
#define CX_LAZYLOG(MSG) CX_DBG(CX_LOG(UNDEFINED, DONTCARE, MSG))
#define CX_LAZYLOG_FMT(MSG, ...) CX_DBG(CX_LOG_FMT(UNDEFINED, DONTCARE, MSG, __VA_ARGS__))

/**
 * Faster than cx_log_fmt; uses `sputs()`
 */
void cx_log(int level, const char* s_cat, const char* s_msg);

/**
 * Glorified `fprintf()` wrapper
 */
void cx_log_fmt(int level, const char* s_cat, const char* s_fmt, ...);

/**
 * Sets a log cetegory's minimum log level.
 * 
 * Any log messages with a log category matching `s_cat` will not be printed if the message log level is less than
 * `min_level`.
 *
 * If `s_cat` is null (0), or `CX_LOG_CAT_ALL` then the global minimum log level will be set to `min_level`.
 * This will supercede all log category minimum log levels.
 *
 * If `min_level` is `CX_LOG_LEVEL_SILENT` then log messages with a log category matching `s_cat` will be silenced.
 */
void cx_log_cat_set(const char* s_cat, int min_level);

#endif
