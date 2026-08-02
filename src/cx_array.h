#ifndef CX_ARRAY_H
#define CX_ARRAY_H

#include <stddef.h>

#define CX_LOG_CAT_ARRAY "array"

struct cx_array {
	size_t element_size;
	size_t length;
	size_t capacity;
	void* p_data;
};

struct cx_array_view {
	size_t element_size;
	size_t length;
	void* p_data;
};

void cx_array_init(size_t element_size, struct cx_array* p_out);
void cx_array_init_capacity(size_t element_size, size_t capacity, struct cx_array* p_out);
void cx_array_free(struct cx_array* p_array);
void cx_array_reserve(struct cx_array* p_array, size_t new_capacity);
void cx_array_resize(struct cx_array* p_array, size_t new_length);
void cx_array_shrink_to_fit(struct cx_array* p_array);
void* cx_array_at(const struct cx_array* p_array, size_t index);
void* cx_array_push(struct cx_array* p_array, const void* p_element);
void cx_array_pop(struct cx_array* p_array);
void* cx_array_insert(struct cx_array* p_array, size_t index, const void* p_element);
void cx_array_remove_at(struct cx_array* p_array, size_t index);
void cx_array_unordered_remove_at(struct cx_array* p_array, size_t index);
void cx_array_sort(struct cx_array* p_array, int(*f_compare)(const void*, const void*));
int cx_array_find(
	const struct cx_array* p_array, const void* p_value, int(*f_compare)(const void*, const void*), size_t* p_out);
int cx_array_find_if(
	const struct cx_array* p_array, int(*f_predicate)(const void*, void*), void* p_context, size_t* p_out);

#endif
