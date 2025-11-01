#ifndef _H__DARR
#define _H__DARR

#include <stdint.h>

struct darr {
    void*  _p_buffer;
    size_t _length;
    size_t _capacity;
    size_t _element_size;
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