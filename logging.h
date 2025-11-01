#ifndef _H__LOGGING
#define _H__LOGGING

#define LOG_TRACE   0
#define LOG_INFO    1
#define LOG_WARNING 2
#define LOG_ERROR   3
#define LOG_DEBUG   4
#ifndef LOG_MIN
#define LOG_MIN LOG_TRACE
#endif

void log_msg(int type, const char* s_category, const char* s_fmt, ...);

#endif