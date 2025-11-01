#include <string.h>
#include <stdlib.h>

#include "hashtable.h"
#include "logging.h"

#define HASHTABLE_LOAD_THRESHOLD 0.7f
#define HASHTABLE_MIN_BUCKETS 8
#define LOG_CAT_HASHTABLE "hashtable"

struct hashtable_element {
    void*                     p_key;
    size_t                    key_len;
    void*                     p_value;
    struct hashtable_element* p_next;
};

struct hashtable_bucket {
    struct hashtable_element* p_first;
    struct hashtable_element* p_last;
};

static size_t                   hash_key(const void* p_key, size_t key_len);
static void                     hashtable_bucket_append(struct hashtable_bucket* p_bucket, struct hashtable_element* p_element);
static struct hashtable_bucket* hashtable_find_bucket(const struct hashtable* p_table, const void* p_key, size_t key_len);
int                             key_cmp(const void* p_key0, size_t key0_len, const void* p_key1, size_t key1_len);
static int                      hashtable_resize(struct hashtable* p_table, size_t num_buckets);

void hashtable_init(struct hashtable* p_table, size_t element_size) {
    *p_table = (struct hashtable) {
        ._element_size = element_size
    };
    //log_msg(LOG_LEVEL_TRACE, LOG_CAT_HASHTABLE, "hashtable_init: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x }\n", p_table, p_table->_element_size, p_table->_n_elements, p_table->_n_buckets, p_table->_p_buckets);
}

void hashtable_free(struct hashtable* p_table) {
    for(size_t i = 0; i < p_table->_n_buckets; ++i) {
        struct hashtable_element* p_elem = ((struct hashtable_bucket*)p_table->_p_buckets)[i].p_first;
        while (p_elem) {
            void* p_next = p_elem->p_next;
            free(p_elem);
            p_elem = p_next;
        }
    }
    free(p_table->_p_buckets);
    *p_table = (struct hashtable){0};
}

void* hashtable_find(const struct hashtable* p_table, const void* p_key, size_t key_len) {
    if (p_table->_n_elements == 0) {
        //log_msg(LOG_LEVEL_TRACE, LOG_CAT_HASHTABLE, "hashtable_find: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x }, p_key=%x, key_len=%llu: Couldn't find item. Hashtable is empty\n", p_table, p_table->_element_size, p_table->_n_elements, p_table->_n_buckets, p_table->_p_buckets, p_key, key_len);
        return 0;
    }

    const struct hashtable_bucket* p_bucket = hashtable_find_bucket(p_table, p_key, key_len);
    const struct hashtable_element* p_elem = p_bucket->p_first;

    while (p_elem) {
        if (key_cmp(p_elem->p_key, p_elem->key_len, p_key, key_len)) {
            return p_elem->p_value;
        }
        p_elem = p_elem->p_next;
    }

    //log_msg(LOG_LEVEL_TRACE, LOG_CAT_HASHTABLE, "hashtable_find: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x }, p_key=%x, key_len=%llu: Couldn't find item. No matching key found\n", p_table, p_table->_element_size, p_table->_n_elements, p_table->_n_buckets, p_table->_p_buckets, p_key, key_len);

    return 0;
}

void* hashtable_add(struct hashtable* p_table, const void* p_key, size_t key_len) {
    if (hashtable_find(p_table, p_key, key_len) != 0) {
        log_msg(LOG_ERROR, LOG_CAT_HASHTABLE, "hashtable_add: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x }, p_key=%x, key_len=%llu: Couldn't add new item. An item with the specified key already exists\n", p_table, p_table->_element_size, p_table->_n_elements, p_table->_n_buckets, p_table->_p_buckets, p_key, key_len);
        return 0;
    }

    const size_t elem_size = sizeof(struct hashtable_element) + key_len + p_table->_element_size;
    unsigned char* p_new_elem_bytes = malloc(elem_size);

    if (!p_new_elem_bytes) {
        log_msg(LOG_ERROR, LOG_CAT_HASHTABLE, "hashtable_add: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x }, p_key=%x, key_len=%llu: Couldn't allocate memory for new item\n", p_table, p_table->_element_size, p_table->_n_elements, p_table->_n_buckets, p_table->_p_buckets, p_key, key_len);
        return 0;
    }

    struct hashtable_element* p_new_elem = (void*)p_new_elem_bytes;
    *p_new_elem = (struct hashtable_element) {
        .p_key = p_new_elem_bytes + sizeof(*p_new_elem),
        .key_len = key_len,
        .p_value = p_new_elem_bytes + sizeof(*p_new_elem) + key_len
    };

    memcpy(p_new_elem->p_key, p_key, key_len);

    const float new_load_ratio = (float)(p_table->_n_elements + 1) / p_table->_n_buckets;
    if (p_table->_n_buckets == 0 || new_load_ratio > HASHTABLE_LOAD_THRESHOLD) {
        const size_t new_n_buckets = p_table->_n_elements ? p_table->_n_elements * 2 : HASHTABLE_MIN_BUCKETS;
        if (!hashtable_resize(p_table, new_n_buckets)) {
            log_msg(LOG_ERROR, LOG_CAT_HASHTABLE, "hashtable_add: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x }, p_key=%x, key_len=%llu: Couldn't resize hashtable for new item\n", p_table, p_table->_element_size, p_table->_n_elements, p_table->_n_buckets, p_table->_p_buckets, p_key, key_len);
            free(p_new_elem);
            return 0;
        }
    }

    hashtable_bucket_append(hashtable_find_bucket(p_table, p_key, key_len), p_new_elem);
    ++p_table->_n_elements;
    
    //log_msg(LOG_LEVEL_TRACE, LOG_CAT_HASHTABLE, "hashtable_add: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x }, p_new_elem=(%x){ p_key=%x, key_len=%llu, p_value=%x }\n", p_table, p_table->_element_size, p_table->_n_elements, p_table->_n_buckets, p_table->_p_buckets, p_new_elem, p_new_elem->p_key, key_len, p_new_elem->p_value);

    return p_new_elem->p_value;
}

void* hashtable_get(struct hashtable* p_table, const void* p_key, size_t key_len) {
    void* p_value = hashtable_find(p_table, p_key, key_len);

    if (!p_value) {
        p_value = hashtable_add(p_table, p_key, key_len);
    }

    return p_value;
}

void hashtable_remove(struct hashtable* p_table, const void* p_key, size_t key_len) {
    struct hashtable_bucket* p_bucket = hashtable_find_bucket(p_table, p_key, key_len);
    struct hashtable_element* p_elem = p_bucket->p_first;
    struct hashtable_element* p_prev = 0;

    while (p_elem) {
        if (key_cmp(p_elem->p_key, p_elem->key_len, p_key, key_len)) {
            if (p_prev) {
                p_prev->p_next = p_elem->p_next;
            } else {
                p_bucket->p_first = p_elem->p_next;
            }

            if (!p_elem->p_next) {
                p_bucket->p_last = p_prev;
            }

            free(p_elem);

            // todo: check if we should shrink size?

            return;
        }

        p_prev = p_elem;
        p_elem = p_elem->p_next;
    }
}

void* hashtable_s_find(const struct hashtable* p_table, const char* s_key) {
    return hashtable_find(p_table, &s_key, strlen(s_key) + 1);
}

void* hashtable_s_add(struct hashtable* p_table, const char* s_key) {
    return hashtable_add(p_table, &s_key, strlen(s_key) + 1);
}

void* hashtable_s_get(struct hashtable* p_table, const char* s_key) {
    return hashtable_get(p_table, &s_key, strlen(s_key) + 1);
}

void hashtable_s_remove(struct hashtable* p_table, const char* s_key) {
    hashtable_remove(p_table, &s_key, strlen(s_key) + 1);
}

void* hashtable_i_find(const struct hashtable* p_table, uint32_t key) {
    return hashtable_find(p_table, &key, sizeof(key));
}

void* hashtable_i_add(struct hashtable* p_table, uint32_t key) {
    return hashtable_add(p_table, &key, sizeof(key));
}

void* hashtable_i_get(struct hashtable* p_table, const uint32_t key) {
    return hashtable_get(p_table, &key, sizeof(key));
}

void hashtable_i_remove(struct hashtable* p_table, uint32_t key) {
    hashtable_remove(p_table, &key, sizeof(key));
}

void hashtable_itr(const struct hashtable* p_table, struct hashtable_itr* p_itr) {
    *p_itr = (struct hashtable_itr){0};

    if (p_table->_n_elements == 0) {
        return;
    }

    const struct hashtable_bucket* p_buckets = p_itr->_p_table->_p_buckets;
    const struct hashtable_element* p_elem = p_buckets[0].p_first;

    while (!p_elem) {
        ++(p_itr->_p_bucket_index);

        if (p_itr->_p_bucket_index < p_itr->_p_table->_n_buckets) {
            p_elem = p_buckets[p_itr->_p_bucket_index].p_first;
        } else {
            *p_itr = (struct hashtable_itr){0};
            return;
        }
    }

    p_itr->p_key = p_elem->p_key;
    p_itr->key_len = p_elem->key_len;
    p_itr->p_value = p_elem->p_value;
    p_itr->_p_table = p_table;
    p_itr->_p_element = p_elem;
}

void hashtable_itr_next(struct hashtable_itr* p_itr) {
    const struct hashtable_element* p_elem = p_itr->_p_element;
    p_elem = p_elem->p_next;

    const struct hashtable_bucket* p_buckets = p_itr->_p_table->_p_buckets;
    while (!p_elem) {
        ++(p_itr->_p_bucket_index);

        if (p_itr->_p_bucket_index < p_itr->_p_table->_n_buckets) {
            p_elem = p_buckets[p_itr->_p_bucket_index].p_first;
        } else {
            *p_itr = (struct hashtable_itr){0};
            return;
        }
    }

    p_itr->p_key = p_elem->p_key;
    p_itr->key_len = p_elem->key_len;
    p_itr->p_value = p_elem->p_value;
    p_itr->_p_element = p_elem;
}

int hashtable_itr_is_valid(const struct hashtable_itr* p_itr) {
    return !!p_itr->_p_element;
}

size_t hash_key(const void* p_key, size_t key_len) {
    size_t h = 0;
    const unsigned char* p = p_key;
    for (size_t i = 0; i < key_len; ++i, ++p) {
        h = 37 * h + *p;
    }
    return h;
}

void hashtable_bucket_append(struct hashtable_bucket* p_bucket, struct hashtable_element* p_element) {
    if (!p_bucket->p_first) {
        p_bucket->p_first = p_element;
    } else {
        p_bucket->p_last->p_next = p_element;
    }

    p_bucket->p_last = p_element;
    p_bucket->p_last->p_next = 0;
}

struct hashtable_bucket* hashtable_find_bucket(const struct hashtable* p_table, const void* p_key, size_t key_len) {
    const size_t index = hash_key(p_key, key_len) % p_table->_n_buckets;
    return (struct hashtable_bucket*)p_table->_p_buckets + index;
}

int key_cmp(const void* p_key0, size_t key0_len, const void* p_key1, size_t key1_len) {
    if (key0_len != key1_len) {
        return 0;
    }

    const unsigned char* p0 = p_key0;
    const unsigned char* p1 = p_key1;

    for (size_t i = 0; i < key0_len; ++i, ++p0, ++p1) {
        if (*p0 != *p1) {
            return 0;
        }
    }

    return 1;
}

int hashtable_resize(struct hashtable* p_table, size_t n_buckets) {
    const size_t n_buckets_old = p_table->_n_buckets;
    struct hashtable_bucket* p_buckets_old = p_table->_p_buckets;

    void* p_buckets = calloc(n_buckets, sizeof(struct hashtable_bucket));
    if (!p_buckets) {
        log_msg(LOG_ERROR, LOG_CAT_HASHTABLE, "hashtable_resize: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x }, new_n_buckets=%llu: Couldn't allocate memory\n", p_table, p_table->_element_size, p_table->_n_elements, p_table->_n_buckets, p_table->_p_buckets, n_buckets);
        return 0;
    }

    p_table->_n_buckets = n_buckets;
    p_table->_p_buckets = p_buckets;
    
    for (size_t i = 0; i < n_buckets_old; ++i) {
        struct hashtable_element* p_elem = p_buckets_old[i].p_first;
        while (p_elem) {
            struct hashtable_element* p_next = p_elem->p_next;
            struct hashtable_bucket* p_new_bucket = hashtable_find_bucket(p_table, p_elem->p_key, p_elem->key_len);
            hashtable_bucket_append(p_new_bucket, p_elem);
            p_elem = p_next;
        }
    }

    free(p_buckets_old);

    return 1;
}
