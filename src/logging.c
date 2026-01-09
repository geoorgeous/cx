#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "logging.h"

#define CX_LOG_CAT_MIN_LEVEL_DEFAULT CX_LOG_ALL
#define CX_LOG_CAT_DELIM ':'
#define CX_LOG_CAT_STR_BUF_LEN 2048
#define CX_MAX_LOG_CATS 128

#define CX_LOG_CAT_LOGGING "logging"

static const char* k_log_level_strings[] = {
    CX_LOG_LABEL_TRACE,
    CX_LOG_LABEL_INFO,
    CX_LOG_LABEL_WARNING,
    CX_LOG_LABEL_ERROR,
    CX_LOG_LABEL_DEBUG
};

static int cmp_log_levels(int min_level, int level);
static struct cx_log_cat* get_log_cat(const char* s_cat, size_t cat_len);
static int is_log_visible(const char* s_cat, int level);
static void print_prefix(FILE* p_file, int log_level, const char* s_category);

struct cx_log_cat {
	const char* s_display_str;
	size_t      display_str_len;
	int         min_level;
};

static char              log_cat_str_buf[CX_LOG_CAT_STR_BUF_LEN];
static char*             p_log_cat_str_buf_next = log_cat_str_buf;
static struct cx_log_cat log_cats[CX_MAX_LOG_CATS];
static int               log_cat_global_min_level = CX_LOG_CAT_MIN_LEVEL_DEFAULT;

void cx_log(int level, const char* s_cat, const char* s_msg) {
	if (!is_log_visible(s_cat, level)) {
		return;
	}

    FILE* const p_file = level == CX_LOG_ERROR ? stderr : stdout;

    print_prefix(p_file, level, s_cat);
    (void)fputs(s_msg, p_file);
}

void cx_log_fmt(int level, const char* s_cat, const char* s_fmt, ...) {
	if (!is_log_visible(s_cat, level)) {
		return;
	}

    va_list args;
    va_start(args, s_fmt);
    
    FILE* const p_file = level == CX_LOG_ERROR ? stderr : stdout;
    
    print_prefix(p_file, level, s_cat);

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

void cx_log_cat_set(const char *s_cat, int min_level) {
	if (!s_cat) {
		log_cat_global_min_level = min_level;
		return;
	}

	struct cx_log_cat* p_cat = get_log_cat(s_cat, 0);
	p_cat->min_level = min_level;
}

int cmp_log_levels(int min_level, int level) {
	return min_level >= 0 && min_level <= level && log_cat_global_min_level <= level;
}

struct cx_log_cat* get_log_cat(const char* s_cat, size_t cat_len) {
	struct cx_log_cat* p_cat = log_cats;

	while (p_cat->s_display_str) {
		const int b_match = 
			cat_len == 0 ?
			strcmp(s_cat, p_cat->s_display_str) == 0 :
			strncmp(
				s_cat,
				p_cat->s_display_str,
				cat_len < p_cat->display_str_len ? cat_len : p_cat->display_str_len) == 0;

		if (b_match) {
			return p_cat;
		}

		p_cat++;
	}

	*p_cat = (struct cx_log_cat) {
		.s_display_str = p_log_cat_str_buf_next,
		.display_str_len = cat_len ? cat_len : strlen(s_cat),
		.min_level = CX_LOG_CAT_MIN_LEVEL_DEFAULT
	};

	memcpy(p_log_cat_str_buf_next, s_cat, p_cat->display_str_len);
	p_log_cat_str_buf_next += p_cat->display_str_len;
	*p_log_cat_str_buf_next++ = '\0';

	return p_cat;
}

int is_log_visible(const char* s_cat, int level) {
	if (log_cat_global_min_level < 0) {
		return 0;
	}

	if (s_cat == 0) {
		return log_cat_global_min_level <= level;
	}

	const char* p = s_cat;
	size_t len = 0;

	while (*p) {
		if (*p == CX_LOG_CAT_DELIM && !cmp_log_levels(get_log_cat(s_cat, len)->min_level, level)) {
			return 0;
		}
		++len;
		p++;
	}

	return cmp_log_levels(get_log_cat(s_cat, len)->min_level, level);
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
