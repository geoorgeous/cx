#ifndef _H__CX_LOGGING
#define _H__CX_LOGGING

#define CX_LOG_ALL       0
#define CX_LOG_TRACE     0
#define CX_LOG_INFO      1
#define CX_LOG_WARNING   2
#define CX_LOG_ERROR     3
#define CX_LOG_DEBUG     4
#define CX_LOG_DISABLED -1

#define CX_LOG_LABEL_TRACE   "  trace"
#define CX_LOG_LABEL_INFO    "   info"
#define CX_LOG_LABEL_WARNING "warning"
#define CX_LOG_LABEL_ERROR   "  ERROR"
#define CX_LOG_LABEL_DEBUG   "~~~~~~ "

#ifndef NDEBUG
#define CX_DBG_LOG(CAT, MSG) cx_log(CX_LOG_DEBUG, CAT, MSG)
#define CX_DBG_LOG_FMT(CAT, FMT, ...) cx_log_fmt(CX_LOG_DEBUG, CAT, FMT, __VA_ARGS__)
#else
#define CX_DBG_LOG(CAT, MSG) ((void)0)
#define CX_DBG_LOG_FMT(CAT, FMT, ...) ((void)0)
#endif

void cx_log(int level, const char* s_cat, const char* s_msg);
void cx_log_fmt(int level, const char* s_cat, const char* s_fmt, ...);
void cx_log_cat_set(const char* s_cat, int level);

#endif
