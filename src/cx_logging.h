#ifndef _H__CX_LOGGING
#define _H__CX_LOGGING

#define CX_LOG_ALL       0
#define CX_LOG_TRACE     0
#define CX_LOG_INFO      1
#define CX_LOG_WARNING   2
#define CX_LOG_ERROR     3
#define CX_LOG_DEBUG     4
#define CX_LOG_DISABLED -1
#define CX_LOG_CAT_NONE  0

#define CX_LOG_LABEL_TRACE   "  trace"
#define CX_LOG_LABEL_INFO    "   info"
#define CX_LOG_LABEL_WARNING "warning"
#define CX_LOG_LABEL_ERROR   "  ERROR"
#define CX_LOG_LABEL_DEBUG   " ~  ~  "

#define CX_LOG_CAT_LOGGING "logging"

#ifndef NDEBUG
#define CX_DBG_LOG(CAT, MSG) cx_log(CX_LOG_DEBUG, CAT, MSG)
#define CX_DBG_LOG_FMT(CAT, FMT, ...) cx_log_fmt(CX_LOG_DEBUG, CAT, FMT, __VA_ARGS__)
#else
#define CX_DBG_LOG(CAT, MSG) ((void)0)
#define CX_DBG_LOG_FMT(CAT, FMT, ...) ((void)0)
#endif

void cx_log(int level, const char* s_cat, const char* s_msg);
void cx_log_fmt(int level, const char* s_cat, const char* s_fmt, ...);

/**
 * Sets a log cetegory's minimum log level.
 * 
 * Any log messages with a log category of `s_cat` will not be printed if the message log level is less than
 * `min_level`.
 *
 * If `s_cat` is null, or `CX_LOG_ALL` then the global minimum log level will be set to `min_level`.
 *
 * If `min_level` is `CX_LOG_DISABLED` then log messages with a log category of `s_cat` will be silenced.
 */
void cx_log_cat_set(const char* s_cat, int min_level);

#endif
