#include <stdlib.h>
#include <string.h>

#include "cx_logging.h"
#include "object_pool.h"

void object_pool_init(struct object_pool* p_pool, size_t object_size, size_t capacity) {
	if (object_size < sizeof(void*)) {
		object_size = sizeof(void*);
	}

	*p_pool = (struct object_pool) {
		.object_size_ = object_size,
		.capacity_ = capacity,
		.p_objects_ = calloc(capacity, object_size)
	};
	p_pool->p_next_free_ = p_pool->p_objects_;

	for (size_t i = 0; i < capacity - 1; ++i) {
		void* p = (unsigned char*)p_pool->p_objects_ + object_size * i;
		void* p_next = ((unsigned char*)p + object_size);
		*((void**)p) = p_next;
	}
}

void* object_pool_get(struct object_pool* p_pool) {
	void* p = p_pool->p_next_free_;
	if (!p) {
		CX_LOG(ERROR, DONTCARE, "Object pool exhausted!\n");
		return 0;
	}
	p_pool->p_next_free_ = *((void**)p);
	return p;
}

void object_pool_return(struct object_pool* p_pool, void* p_object) {
	void* p = p_object;
	*((void**)p) = p_pool->p_next_free_;
	p_pool->p_next_free_ = p;
}

void object_pool_free(struct object_pool* p_pool) {
	free(p_pool->p_objects_);
	*p_pool = (struct object_pool){0};
}
