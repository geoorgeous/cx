#ifndef _H__OBJECT_POOL
#define _H__OBJECT_POOL

#include <stdint.h>

struct object_pool {
    size_t _object_size;
    size_t _capacity;
    void*  _p_objects;
    void*  _p_next_free;
    void*  _p_allocated_head;
};

void  object_pool_init(struct object_pool* p_pool, size_t object_size, size_t capacity);
void* object_pool_get(struct object_pool* p_pool);
void  object_pool_return(struct object_pool* p_pool, void* p_object);
void  object_pool_free(struct object_pool* p_pool);



#endif