#ifndef CX_ALLOC_H
#define CX_ALLOC_H

#include <stdlib.h>
#include <stddef.h>

#include "cx_dbg.h"
#include "cx_logging.h"
#include "cx_macro.h"

#define CX_LOG_CAT_ALLOC "alloc"

#define CX_MALLOC(SIZE) cx_malloc(SIZE, __FILE__, __LINE__, __func__)
#define CX_CALLOC(SIZE) cx_calloc(SIZE, __FILE__, __LINE__, __func__)
#define CX_REALLOC(P, SIZE) cx_realloc(P, SIZE, __FILE__, __LINE__, __func__)
#define CX_FREE(P) free(P)

static inline void* cx_malloc(size_t size, const char* s_file, int line, const char* s_func) {
	void* p = malloc(size);

#ifdef NDEBUG
	(void)s_file;
	(void)line;
	(void)s_func;
#else
	if (!p) {
		CX_LOG_FMT(ERROR, ALLOC, "Failed to allocate %"CX_PRI_SIZE" bytes! %s:%d (%s)\n",
			size, s_file, line, s_func);
		abort();
	}
#endif

	return p;
}

static inline void* cx_calloc(size_t size, const char* s_file, int line, const char* s_func) {
	void* p = calloc(1, size);

#ifdef NDEBUG
	(void)s_file;
	(void)line;
	(void)s_func;
#else
	if (!p) {
		CX_LOG_FMT(ERROR, ALLOC, "Failed to allocate %"CX_PRI_SIZE" bytes! %s:%d (%s)\n", size, s_file, line, s_func);
		abort();
	}
#endif

	return p;
}

static inline void* cx_realloc(void* p, size_t size, const char* s_file, int line, const char* s_func) {
	p = realloc(p, size);

#ifdef NDEBUG
	(void)s_file;
	(void)line;
	(void)s_func;
#else
	if (!p) {
		CX_LOG_FMT(ERROR, ALLOC, "Failed to reallocate %"CX_PRI_SIZE" bytes! %s:%d (%s)\n",
			size, s_file, line, s_func);
		abort();
	}
#endif

	return p;
}

enum cx_alloc_ring_push_policy {
	CX_ALLOC_RING_PUSH_POLICY_auto,
	CX_ALLOC_RING_PUSH_POLICY_no
};

struct cx_alloc_ring_entry {
	size_t offset;
	size_t size;
};

struct cx_alloc_ring {
	void* p_buf;
	size_t buf_cap;
	size_t buf_head_;
	size_t buf_tail_;
	struct cx_alloc_ring_entry* p_entries_;
	size_t entries_cap_;
	size_t entries_head_;
	size_t entries_tail_;
	size_t entries_count_;
};

#define CX_ALLOC_RING_INIT_ARRAYS(P_RING, BUF, ENTRIES) (\
	cx_alloc_ring_init(\
		P_RING,\
		BUF, sizeof(BUF),\
		ENTRIES, CX_ARRAY_LEN(ENTRIES))\
	)

static inline void cx_alloc_ring_init(
	struct cx_alloc_ring* p_ring,
	void* p_buf, size_t buf_size,
	struct cx_alloc_ring_entry* p_entries,
	size_t entries_len) {

	*p_ring = (struct cx_alloc_ring) {
		.p_buf = p_buf,
		.buf_cap = buf_size,
		.p_entries_ = p_entries,
		.entries_cap_ = entries_len
	};
}

int cx_alloc_ring_can_fit(const struct cx_alloc_ring* p_ring, size_t size);

const void* cx_alloc_ring_push(
	struct cx_alloc_ring* p_ring,
	const void* p_src,
	size_t size,
	enum cx_alloc_ring_push_policy policy);

void cx_alloc_ring_pop(struct cx_alloc_ring* p_ring);

const void* cx_alloc_ring_get(const struct cx_alloc_ring* p_ring, size_t age, size_t* p_out_size);

void cx_alloc_ring_clear(struct cx_alloc_ring* p_ring);

#endif
