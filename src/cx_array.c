#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "cx_alloc.h"
#include "cx_array.h"
#include "cx_dbg.h"

void cx_array_init(size_t element_size, struct cx_array* p_out) {
	*p_out = (struct cx_array) {
		.element_size = element_size
	};
}

void cx_array_init_capacity(size_t element_size, size_t capacity, struct cx_array* p_out) {
	cx_array_init(element_size, p_out);
	cx_array_reserve(p_out, capacity);
}

void cx_array_free(struct cx_array* p_array) {
	CX_FREE(p_array->p_data);
	*p_array = (struct cx_array){0};
}

void cx_array_reserve(struct cx_array* p_array, size_t new_capacity) {
	if (p_array->capacity >= new_capacity) {
		return;
	}
	p_array->p_data = CX_REALLOC(p_array->p_data, p_array->element_size * new_capacity);
	p_array->capacity = new_capacity;
}

void cx_array_resize(struct cx_array* p_array, size_t new_length) {
	cx_array_reserve(p_array, new_length);
	p_array->length = new_length;
}

void cx_array_shrink_to_fit(struct cx_array* p_array) {
	if (p_array->length == p_array->capacity) {
		return;
	}
	p_array->p_data = CX_REALLOC(p_array->p_data, p_array->length * p_array->element_size);
	p_array->capacity = p_array->length;
}

void* cx_array_at(const struct cx_array* p_array, size_t index) {
	CX_ASSERT_MSG_FMT(p_array->length > index, ARRAY, "(%u > %u)\n", p_array->length, index);
	return ((uint8_t*)p_array->p_data) + p_array->element_size * index;
}

void* cx_array_push(struct cx_array* p_array, const void* p_element) {
	return cx_array_insert(p_array, p_array->length, p_element);
}

void cx_array_pop(struct cx_array* p_array) {
	CX_ASSERT(p_array->length > 0, ARRAY);
	cx_array_remove_at(p_array, p_array->length - 1);
}

void* cx_array_insert(struct cx_array* p_array, size_t index, const void* p_element) {
	CX_ASSERT(index <= p_array->length, ARRAY);

	cx_array_reserve(p_array, p_array->length + 1);

	void* p_new_element = ((uint8_t*)p_array->p_data) + p_array->element_size * index;

	if (index < p_array->length) {
		void* p_new_element_next = (uint8_t*)p_new_element + p_array->element_size;
		memmove(p_new_element_next, p_new_element, p_array->element_size * (p_array->length - index));
	}

	p_array->length++;

	if (p_element) {
		memcpy(p_new_element, p_element, p_array->element_size);
	}

	return p_new_element;
}

void cx_array_remove_at(struct cx_array* p_array, size_t index) {
	CX_ASSERT(index < p_array->length, ARRAY);
	void* p_removed = cx_array_at(p_array, index);
	if (index < p_array->length) {
		void* p_removed_next = (uint8_t*)p_removed + p_array->element_size;
		memmove(p_removed, p_removed_next, p_array->element_size);
	}
	p_array->length--;
}

void cx_array_unordered_remove_at(struct cx_array* p_array, size_t index) {
	CX_ASSERT(index < p_array->length, ARRAY);
	cx_array_pop(p_array);
	const void* p_back = cx_array_at(p_array, p_array->length);
	void* p_removed = cx_array_at(p_array, index);
	memcpy(p_removed, p_back, p_array->element_size);
}

void cx_array_sort(struct cx_array* p_array, int(*f_compare)(const void*, const void*)) {
	qsort(p_array->p_data, p_array->length, p_array->element_size, f_compare);
}

int cx_array_find(
	const struct cx_array* p_array, const void* p_value, int(*f_compare)(const void*, const void*), size_t* p_out) {
	
	for (size_t i = 0; i < p_array->length; ++i) {
		const void* p_a = cx_array_at(p_array, i);
		if (f_compare(p_a, p_value) == 0) {
			*p_out = i;
			return CX_TRUE;
		}
	}
	return CX_FALSE;
}

int cx_array_find_if(
	const struct cx_array* p_array, int(*f_predicate)(const void*, void*), void* p_context, size_t* p_out) {

	for (size_t i = 0; i < p_array->length; ++i) {
		const void* p_a = cx_array_at(p_array, i);
		if (f_predicate(p_a, p_context)) {
			*p_out = i;
			return CX_TRUE;
		}
	}
	return CX_FALSE;
}
