/**
 * # cx_logging
 *
 *
 * ## Features
 *
 *   - printf()-style message formatting
 *   - Custom category filtering
 *   - Small, simple API
 *
 *
 * ## Log Levels
 *
 * Their are four main log levels:
 *
 *     - TRACE     : Detailed information for program trace purposes
 *     - INFO      : Notable program state information
 *     - WARNING   : Information warning the reader of potential issues
 *     - ERROR     : Something has gone wrong and is likely to cause an issue
 *
 * There exist a few special log level values with specific use-cases:
 *
 *     - UNDEFINED : For cases where you don't care about the log level. This log level shouldn't be used in production
 *                   as it doesn't convey the severity of the message. It's usefulness is for when you don't want to
 *                   spend time thinking about what log level to use.
 *     - ALL       : Used with cx_log_set_cat() to specify that a given category should display messages of all log
 *                   levels. Submitting messages with a log level of ALL will have the same effect as submitting a
 *                   message with a log level of UNDEFINED 
 *     - SILENT    : Used with cx_log_set_cat() to specify that a given category is to be completely silenced.
 *                   Submitting log messages with a log level of SILENT will have no effect.
 *
 * Preprocessor definitions for all log levels are prefixed with `CX_LOG_LEVEL_`
 *
 *
 * ## Categories
 * 
 * Categories are used to group logs to help the reader make better sense of the log message output.
 *
 * No managing of the log categories is required. Simply pass the category string to the logging API as-is:
 *
 *     cx_log(CX_LOG_LEVEL_INFO, "foo", "This is an info log!\n");
 *
 * The minimum log level for all messages belonging to a given category can be set with `cx_log_cat_set`:
 *
 *     // Within the 'foo' category, only show ERROR-level messages
 *     cx_log_cat_set("foo", CX_LOG_LEVEL_ERROR);
 *
 * Log categories can be nested using a colon `:` character:
 *
 *     cx_log(CX_LOG_LEVEL_INFO, "foo:bar", "This is a nested info log!\n");
 *
 * Nested categories will only be printed if all super-categories have a minimum log level equal to or lower than the
 * message's log level.
 *
 * If no log category is supplied (`NULL`, `0`, `CX_LOG_CAT_DONTCARE`) then no log category will be displayed.
 *
 * The minimum log level for all log messages - including those with no category - can be set via `CX_LOG_CAT_ALL`:
 *
 *     // Only show messages where the log level >= WARNING
 *     cx_log_set_cat(CX_LOG_CAT_ALL, CX_LOG_LEVEL_WARNING);
 *
 *
 * ## Macros
 *
 * You can use the `CX_LOG_*` macro functions as shorthand for the logging functions. This allows you to call the log
 * functions without the `CX_LOG_*` prefixes. To use these macros with your own log category, create a preprocessor
 * definition with the `CX_LOG_CAT_` prefix:
 *
 *     #define CX_LOG_CAT_FOO
 *     ...
 *     CX_LOG_FMT(INFO, FOO, "foo=%d\n", bar);
 *
 * The `CX_LAZYLOG_*` macro functions exist for temporary development and debug logging. Logs submitted via these
 * macro functions are only enabled if the `NDEBUG` preprocessor definition has not been defined. These messages have
 * a log level of UNDEFINED and have no category:
 *
 *     CX_LAZYLOG("A very lazy message\n");
 */

#ifndef CX_LOGGING_H
#define CX_LOGGING_H

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
 * Shorthand macros
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
