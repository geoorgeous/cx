#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cx_logging.h"

#define CX_LOG_CAT_DELIM ':'
#define CX_LOG_CAT_STR_BUF_LEN 2048
#define CX_MAX_LOG_CATS 128

struct cx_log_cat {
	char*  s_display_str;
	size_t display_str_len;
	int    min_level;
};

static int  get_log_cat(const char* s_cat, size_t cat_len, struct cx_log_cat** pp_out_cat);
static int  is_log_visible(const char* s_cat, int level);
static void print_prefix(FILE* p_file, int log_level, const char* s_category);

static const char* k_log_level_strings[] = {
	CX_LOG_LABEL_UNDEFINED,
    CX_LOG_LABEL_TRACE,
    CX_LOG_LABEL_INFO,
    CX_LOG_LABEL_WARNING,
    CX_LOG_LABEL_ERROR,
};

static char              log_cat_str_buf[CX_LOG_CAT_STR_BUF_LEN];
static char*             p_log_cat_str_buf_next = log_cat_str_buf;
static struct cx_log_cat log_cats[CX_MAX_LOG_CATS];
static int               log_cat_global_min_level = CX_LOG_LEVEL_TRACE;
static size_t            log_cat_count;

void cx_log(int level, const char* s_cat, const char* s_msg) {
	if (!is_log_visible(s_cat, level)) {
		return;
	}

    FILE* const p_file = level == CX_LOG_LEVEL_ERROR ? stderr : stdout;

    print_prefix(p_file, level, s_cat);

    (void)fputs(s_msg, p_file);
}

void cx_log_fmt(int level, const char* s_cat, const char* s_fmt, ...) {
	if (!is_log_visible(s_cat, level)) {
		return;
	}

    FILE* const p_file = level == CX_LOG_LEVEL_ERROR ? stderr : stdout;
	
    print_prefix(p_file, level, s_cat);

    va_list args;
    va_start(args, s_fmt);
    
	// Suppress warning about passing non-literal string to printf func.	
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
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
		CX_LOG_FMT(INFO, LOGGING, "Log category minimum level set: (Global) -> %d\n", min_level);
		return;
	}

	struct cx_log_cat* p_cat;

	if (get_log_cat(s_cat, 0, &p_cat)) {
		p_cat->min_level = min_level;
		CX_LOG_FMT(INFO, LOGGING, "Log category minimum level set: '%s' -> %d\n", s_cat, min_level);
		return;
	}

	if (log_cat_count == CX_MAX_LOG_CATS) {
		CX_LOG_FMT(ERROR, LOGGING, "Couldn't create new log category '%s': limit exceeded.\n", s_cat);
		return;
	}

	const size_t cat_len = strlen(s_cat);
	const size_t log_cat_str_buf_available = (p_log_cat_str_buf_next - log_cat_str_buf) - CX_LOG_CAT_STR_BUF_LEN;

	if (log_cat_str_buf_available < cat_len) {
		CX_LOG_FMT(ERROR, LOGGING, "Couldn't create new log category '%s': not enough memory.\n", s_cat);
		return;
	}

	*p_cat = (struct cx_log_cat) {
		.s_display_str = p_log_cat_str_buf_next,
		.display_str_len = cat_len,
		.min_level = min_level
	};
	
	strcpy(p_cat->s_display_str, s_cat);
	p_log_cat_str_buf_next += p_cat->display_str_len;
	++log_cat_count;

	CX_LOG_FMT(INFO, LOGGING, "Log category minimum level set: '%s' -> %d\n", s_cat, min_level);
}

int get_log_cat(const char* s_cat, size_t cat_len, struct cx_log_cat** p_out_cat) {
	*p_out_cat = log_cats;

	for (; (*p_out_cat)->s_display_str; (*p_out_cat)++) {
		if (cat_len == 0) {
			if (strcmp(s_cat, (*p_out_cat)->s_display_str) == 0) {
				return 1;
			}
			continue;
		}

		if ((*p_out_cat)->display_str_len == cat_len &&
			strncmp(
				s_cat,
				(*p_out_cat)->s_display_str,
				cat_len > (*p_out_cat)->display_str_len ? cat_len : (*p_out_cat)->display_str_len) == 0) {
			return 1;
		}
	}
	
	return 0;
}

int is_log_visible(const char* s_cat, int level) {
	if (log_cat_global_min_level > level || log_cat_global_min_level < 0) {
		return 0;
	}

	if (s_cat == CX_LOG_CAT_DONTCARE) {
		return 1;
	}

	const char* p = s_cat;
	size_t len = 0;

	for(;; ++len, p++) {
		if (*p != CX_LOG_CAT_DELIM && *p != '\0') {
			continue;
		}
		struct cx_log_cat* p_cat;
		const int cat_min_level = get_log_cat(s_cat, len, &p_cat) ? p_cat->min_level : CX_LOG_LEVEL_INFO;
		const int b_visible = cat_min_level <= level && cat_min_level > CX_LOG_LEVEL_SILENT;  
		if (!b_visible || *p == '\0') {
			return b_visible;
		}
	}
}

void print_prefix(FILE* p_file, int log_level, const char* s_category) {
    static char timestamp_str_buffer[20];

    time_t timestamp = time(0);
    struct tm* tm = localtime(&timestamp);
    (void)strftime(timestamp_str_buffer, sizeof(timestamp_str_buffer), "%Y-%m-%d %H:%M:%S", tm);

    if (s_category) {
        (void)fprintf(p_file, "[%s]%s: (%s) ", timestamp_str_buffer, k_log_level_strings[log_level], s_category);
    } else {
        (void)fprintf(p_file, "[%s]%s: ", timestamp_str_buffer, k_log_level_strings[log_level]);
    }
}
