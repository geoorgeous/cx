#ifndef CX_ALLOC_H
#define CX_ALLOC_H

#include <stddef.h>

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

int cx_alloc_ring_push(struct cx_alloc_ring* p_ring, const void* p_src, size_t size);

const void* cx_alloc_ring_get(const struct cx_alloc_ring* p_ring, size_t age, size_t* p_out_size);

void cx_alloc_ring_evict_oldest(struct cx_alloc_ring* p_ring);

#endif
