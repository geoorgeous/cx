#ifndef CX_ALLOC_H
#define CX_ALLOC_H

#include <stddef.h>

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
