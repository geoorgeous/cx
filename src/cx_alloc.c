#include <string.h>

#include "cx_alloc.h"

int cx_alloc_ring_can_fit(const struct cx_alloc_ring* p_ring, size_t size) {
	if (p_ring->entries_count_ == 0) {
		return 1;
	}

	if (p_ring->buf_head_ >= p_ring->buf_tail_) {
		return p_ring->buf_head_ + size <= p_ring->buf_cap || size <= p_ring->buf_tail_;
	}

	return p_ring->buf_head_ + size < p_ring->buf_tail_;
}

const void* cx_alloc_ring_push(
	struct cx_alloc_ring* p_ring,
	const void* p_src,
	size_t size,
	enum cx_alloc_ring_push_policy policy) {
	
	if (size > p_ring->buf_cap ||
		(!cx_alloc_ring_can_fit(p_ring, size) && policy == CX_ALLOC_RING_PUSH_POLICY_no)) {
		return 0;
	}

	while(!cx_alloc_ring_can_fit(p_ring, size)) {
		cx_alloc_ring_pop(p_ring);
	}
	
	if (p_ring->buf_head_ + size > p_ring->buf_cap) {
		p_ring->buf_head_ = 0;
	}
	
	memcpy((char*)p_ring->p_buf + p_ring->buf_head_, p_src, size);
	
	p_ring->p_entries_[p_ring->entries_head_] = (struct cx_alloc_ring_entry) {
		.offset = p_ring->buf_head_,
		.size = size
	};

	const void* p = (char*)p_ring->p_buf + p_ring->buf_head_;

	p_ring->buf_head_ += size;
	p_ring->entries_head_ = (p_ring->entries_head_ + 1) % p_ring->entries_cap_;
	++p_ring->entries_count_;

	return p;;
}

void cx_alloc_ring_pop(struct cx_alloc_ring* p_ring) {
	const struct cx_alloc_ring_entry* p_entry = p_ring->p_entries_ + p_ring->entries_tail_;
	p_ring->buf_tail_ = (p_entry->offset + p_entry->size) % p_ring->buf_cap;
	p_ring->entries_tail_ = (p_ring->entries_tail_ + 1) % p_ring->entries_cap_;
	--p_ring->entries_count_;
}

const void* cx_alloc_ring_get(const struct cx_alloc_ring* p_ring, size_t age, size_t* p_out_size) {
	const size_t index = (p_ring->entries_head_ + p_ring->entries_cap_ - 1 - age) % p_ring->entries_cap_;
	const struct cx_alloc_ring_entry* p_entry = p_ring->p_entries_ + index;
	*p_out_size = p_entry->size;
	return (char*)p_ring->p_buf + p_entry->offset;
}
