#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "logging.h"

static const char* k_log_level_strings[] = {
    "   trace",
    "    info",
    " warning",
    "   ERROR",
    "   debug"
};

static void print_prefix(FILE* p_file, int log_level, const char* s_category);

void cx_log(int level, const char* s_category, const char* s_msg) {
    if (level < CX_LOG_MIN) {
        return;
    }

    FILE* const p_file = level == CX_LOG_ERROR ? stderr : stdout;

    print_prefix(p_file, level, s_category);
    (void)fputs(s_msg, p_file);
}

void cx_log_fmt(int level, const char* s_category, const char* s_fmt, ...) {
    if (level < CX_LOG_MIN) {
        return;
    }

    va_list args;
    va_start(args, s_fmt);
    
    FILE* const p_file = level == CX_LOG_ERROR ? stderr : stdout;
    
    print_prefix(p_file, level, s_category);

	// Suppress warning about passing non-literal string to printf func.	
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
    (void)vfprintf(p_file, s_fmt, args);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNU__)
#pragma GCC diagnostic pop
#endif

    va_end(args);
}

void print_prefix(FILE* p_file, int log_level, const char* s_category) {
    static char timestamp_str_buffer[20] = {0};

    time_t timestamp = time(0);
    struct tm* tm = localtime(&timestamp);
    (void)strftime(timestamp_str_buffer, 20, "%Y-%m-%d %H:%M:%S", tm);

    if (s_category) {
        (void)fprintf(p_file, "[%s] %s: (%s) ", timestamp_str_buffer, k_log_level_strings[log_level], s_category);
    } else {
        (void)fprintf(p_file, "[%s] %s: ", timestamp_str_buffer, k_log_level_strings[log_level]);
    }
}
