#ifndef _H__LOGGING
#define _H__LOGGING

#define CX_LOG_TRACE   0
#define CX_LOG_INFO    1
#define CX_LOG_WARNING 2
#define CX_LOG_ERROR   3
#define CX_LOG_DEBUG   4

#ifndef CX_LOG_MIN
#define CX_LOG_MIN CX_LOG_TRACE
#endif

#ifndef NDEBUG
#define CX_DBG_LOG(CAT, MSG) cx_log(CX_LOG_DEBUG, CAT, MSG)
#define CX_DBG_LOG_FMT(CAT, FMT, ...) cx_log_fmt(CX_LOG_DEBUG, CAT, FMT, __VA_ARGS__)
#else
#define CX_DBG_LOG(CAT, MSG) ((void)0)
#define CX_DBG_LOG_FMT(CAT, FMT, ...) ((void)0)
#endif

void cx_log(int level, const char* s_category, const char* s_msg);
void cx_log_fmt(int level, const char* s_category, const char* s_fmt, ...);

#endif