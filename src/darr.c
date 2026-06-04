#include <stdlib.h>
#include <string.h>

#include "darr.h"

#define DARR_INITIAL_CAPACITY 1

void darr_init(struct darr* p_darr, size_t element_size) {
	*p_darr = (struct darr) {
		.element_size_ = element_size
	};
}

void darr_free(struct darr* p_darr) {
	free(p_darr->p_buffer_);
	darr_init(p_darr, p_darr->element_size_);
}

void* darr_get(const struct darr* p_darr, size_t index) {
	return (unsigned char*)p_darr->p_buffer_ + index * p_darr->element_size_;
}

void* darr_push(struct darr* p_darr) {
	++p_darr->length_;
	if (p_darr->length_ > p_darr->capacity_) {
		const size_t new_capacity = p_darr->capacity_ == 0 ? 1 : p_darr->capacity_ * 2;
		darr_set_capacity(p_darr, new_capacity);
	}
	return darr_get(p_darr, p_darr->length_ - 1);
}

void darr_remove(struct darr* p_darr, size_t index) {
	--p_darr->length_;
	if (index >= p_darr->length_) {
		return;
	}
	void* dst = darr_get(p_darr, index);
	void* src = darr_get(p_darr, p_darr->length_);
	memcpy(dst, src, p_darr->element_size_);
}

void darr_remove_back(struct darr* p_darr) {
	if (p_darr->length_ < 1) {
		return;
	}
	--p_darr->length_;
}

void darr_set_capacity(struct darr* p_darr, size_t capacity) {
	if (p_darr->capacity_ == capacity) {
		return;
	}
	if (capacity == 0) {
		darr_free(p_darr);
		return;
	}
	p_darr->capacity_ = capacity;
	p_darr->p_buffer_ = realloc(p_darr->p_buffer_, p_darr->capacity_ * p_darr->element_size_);
	if (p_darr->length_ <= p_darr->capacity_) {
		return;
	}
	p_darr->length_ = p_darr->capacity_;
}

void darr_set_length(struct darr* p_darr, size_t length) {
	if (p_darr->capacity_ < length) {
		darr_set_capacity(p_darr, length);
	}
	p_darr->length_ = length;
}

void darr_shrink(struct darr* p_darr) {
	darr_set_capacity(p_darr, p_darr->length_);
}
