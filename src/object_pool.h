#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include <stddef.h>

struct object_pool {
    size_t object_size_;
    size_t capacity_;
    void*  p_objects_;
    void*  p_next_free_;
    void*  p_allocated_head_;
};

void  object_pool_init(struct object_pool* p_pool, size_t object_size, size_t capacity);
void* object_pool_get(struct object_pool* p_pool);
void  object_pool_return(struct object_pool* p_pool, void* p_object);
void  object_pool_free(struct object_pool* p_pool);



#endif
