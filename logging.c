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

static void log_internal(FILE* p_file, int level, const char* s_category, const char* s_fmt, va_list args);

void log_msg(int level, const char* s_category, const char* s_fmt, ...) {
    va_list args;
    va_start(args, s_fmt);
    log_internal(stdout, level, s_category, s_fmt, args);
    va_end(args);
}

void log_internal(FILE* p_file, int level, const char* s_category, const char* s_fmt, va_list args) {
    static char timestamp_str_buffer[20] = {0};

    if (level < LOG_MIN) {
        return;
    }
    
    time_t timestamp = time(0);
    struct tm* tm = localtime(&timestamp);
    (void)strftime(timestamp_str_buffer, 20, "%Y-%m-%d %H:%M:%S", tm);

    if (s_category) {
        (void)fprintf(p_file, "[%s] %s: (%s) ", timestamp_str_buffer, k_log_level_strings[level], s_category);
    } else {
        (void)fprintf(p_file, "[%s] %s: ", timestamp_str_buffer, k_log_level_strings[level]);
    }
    
    (void)vfprintf(p_file, s_fmt, args);
}