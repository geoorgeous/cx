#ifndef _H__HASHTABLE
#define _H__HASHTABLE

#include <stddef.h>
#include <stdint.h>

#define CX_LOG_CAT_HASHTABLE "hashtable"

struct hashtable {
    size_t _element_size;
    size_t _n_elements;
    void*  _p_buckets;
    size_t _n_buckets;
};

struct hashtable_itr {
    const void*  p_key;
    size_t       key_len;
    void*        p_value;

    const struct hashtable* _p_table;
    size_t                  _bucket_index;
    const void*             _p_element;
};

void  hashtable_init(struct hashtable* p_table, size_t element_size);
void  hashtable_free(struct hashtable* p_table);
int   hashtable_find(const struct hashtable* p_table, const void* p_key, size_t key_len, struct hashtable_itr* p_out_itr);
void* hashtable_add(struct hashtable* p_table, const void* p_key, size_t key_len);
void* hashtable_get(struct hashtable* p_table, const void* p_key, size_t key_len);
void  hashtable_remove(struct hashtable* p_table, const void* p_key, size_t key_len);

int   hashtable_s_find(const struct hashtable* p_table, const char* s_key, struct hashtable_itr* p_out_itr);
void* hashtable_s_add(struct hashtable* p_table, const char* s_key);
void* hashtable_s_get(struct hashtable* p_table, const char* s_key);
void  hashtable_s_remove(struct hashtable* p_table, const char* s_key);

int   hashtable_i_find(const struct hashtable* p_table, uint32_t key, struct hashtable_itr* p_out_itr);
void* hashtable_i_add(struct hashtable* p_table, uint32_t key);
void* hashtable_i_get(struct hashtable* p_table, const uint32_t key);
void  hashtable_i_remove(struct hashtable* p_table, uint32_t key);

void hashtable_itr(const struct hashtable* p_table, struct hashtable_itr* p_itr);
void hashtable_itr_next(struct hashtable_itr* p_itr);
int  hashtable_itr_is_valid(const struct hashtable_itr* p_itr);

#endif
