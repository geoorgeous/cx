#include <stdlib.h>
#include <string.h>

#include "darr.h"

#define DARR_INITIAL_CAPACITY 1

void darr_init(struct darr* p_darr, size_t element_size) {
    *p_darr = (struct darr) {
        ._element_size = element_size
    };
}

void darr_free(struct darr* p_darr) {
    free(p_darr->_p_buffer);
    darr_init(p_darr, p_darr->_element_size);
}

void* darr_get(const struct darr* p_darr, size_t index) {
    return (unsigned char*)p_darr->_p_buffer + index * p_darr->_element_size;
}

void* darr_push(struct darr* p_darr) {
    ++p_darr->_length;
    if (p_darr->_length > p_darr->_capacity) {
        const size_t new_capacity = p_darr->_capacity == 0 ? 1 : p_darr->_capacity * 2;
        darr_set_capacity(p_darr, new_capacity);
    }
    return darr_get(p_darr, p_darr->_length - 1);
}

void darr_remove(struct darr* p_darr, size_t index) {
    --p_darr->_length;
    if (index >= p_darr->_length) {
        return;
    }
    void* dst = darr_get(p_darr, index);
    void* src = darr_get(p_darr, p_darr->_length);
    memcpy(dst, src, p_darr->_element_size);
}

void darr_remove_back(struct darr* p_darr) {
    if (p_darr->_length < 1) {
        return;
    }
    --p_darr->_length;
}

void darr_set_capacity(struct darr* p_darr, size_t capacity) {
    if (p_darr->_capacity == capacity) {
        return;
    }
    if (capacity == 0) {
        darr_free(p_darr);
        return;
    }
    p_darr->_capacity = capacity;
    p_darr->_p_buffer = realloc(p_darr->_p_buffer, p_darr->_capacity * p_darr->_element_size);
    if (p_darr->_length <= p_darr->_capacity) {
        return;
    }
    p_darr->_length = p_darr->_capacity;
}

void darr_set_length(struct darr* p_darr, size_t length) {
    if (p_darr->_capacity < length) {
        darr_set_capacity(p_darr, length);
    }
    p_darr->_length = length;
}

void darr_shrink(struct darr* p_darr) {
    darr_set_capacity(p_darr, p_darr->_length);
}