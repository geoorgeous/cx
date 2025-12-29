#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "object_pool.h"

void object_pool_init(struct object_pool* p_pool, size_t object_size, size_t capacity) {
    if (object_size < sizeof(void*)) {
        object_size = sizeof(void*);
    }

    *p_pool = (struct object_pool) {
        ._object_size = object_size,
        ._capacity = capacity,
        ._p_objects = calloc(capacity, object_size)
    };
    p_pool->_p_next_free = p_pool->_p_objects;

    for (size_t i = 0; i < capacity - 1; ++i) {
        void* p = (unsigned char*)p_pool->_p_objects + object_size * i;
        void* p_next = ((unsigned char*)p + object_size);
        *((void**)p) = p_next;
    }
}

void* object_pool_get(struct object_pool* p_pool) {
    void* p = p_pool->_p_next_free;
    if (!p) {
        cx_log(CX_LOG_ERROR, 0, "Object pool exhausted!\n");
        return 0;
    }
    p_pool->_p_next_free = *((void**)p);
    return p;
}

void object_pool_return(struct object_pool* p_pool, void* p_object) {
    void* p = p_object;
    *((void**)p) = p_pool->_p_next_free;
    p_pool->_p_next_free = p;
}

void object_pool_free(struct object_pool* p_pool) {
    free(p_pool->_p_objects);
    *p_pool = (struct object_pool){0};
}