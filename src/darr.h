#ifndef DARR_H
#define DARR_H

#include <stddef.h>

struct darr {
    void*  p_buffer_;
    size_t length_;
    size_t capacity_;
    size_t element_size_;
};

void  darr_init(struct darr* p_darr, size_t element_size);
void  darr_free(struct darr* p_darr);
void* darr_get(const struct darr* p_darr, size_t index);
void* darr_push(struct darr* p_darr);
void  darr_remove(struct darr* p_darr, size_t index);
void  darr_remove_back(struct darr* p_darr);
void  darr_set_capacity(struct darr* p_darr, size_t capacity);
void  darr_set_length(struct darr* p_darr, size_t length);
void  darr_shrink(struct darr* p_darr);

#endif
