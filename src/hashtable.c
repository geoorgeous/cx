#include <string.h>
#include <stdlib.h>

#include "hashtable.h"
#include "cx_logging.h"

#define HASHTABLE_LOAD_THRESHOLD 0.7f
#define HASHTABLE_MIN_BUCKETS 8

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

static void                     hashtable_bucket_append(
	struct hashtable_bucket* p_bucket,
	struct hashtable_element* p_element);

static struct hashtable_bucket* hashtable_find_bucket(
	const struct hashtable* p_table,
	const void* p_key,
	size_t key_len,
	size_t* p_out_index);

static int                      key_cmp(const void* p_key0, size_t key0_len, const void* p_key1, size_t key1_len);

static int                      hashtable_resize(struct hashtable* p_table, size_t num_buckets);

void hashtable_init(struct hashtable* p_table, size_t element_size) {
    *p_table = (struct hashtable) {
        .element_size_ = element_size
    };
}

void hashtable_free(struct hashtable* p_table) {
    for(size_t i = 0; i < p_table->n_buckets_; ++i) {
        struct hashtable_element* p_elem = ((struct hashtable_bucket*)p_table->p_buckets_)[i].p_first;
        while (p_elem) {
            void* p_next = p_elem->p_next;
            free(p_elem);
            p_elem = p_next;
        }
    }
    free(p_table->p_buckets_);
    *p_table = (struct hashtable){0};
}

int hashtable_find(const struct hashtable* p_table, const void* p_key, size_t key_len, struct hashtable_itr* p_out_itr) {
	if (p_out_itr) {
		*p_out_itr = (struct hashtable_itr){0};
	}

    if (p_table->n_elements_ == 0) {
        return 0;
    }

	size_t bucket_index;

    const struct hashtable_bucket* p_bucket = hashtable_find_bucket(p_table, p_key, key_len, &bucket_index);
    const struct hashtable_element* p_elem = p_bucket->p_first;

    while (p_elem) {
        if (key_cmp(p_elem->p_key, p_elem->key_len, p_key, key_len)) {
			if (p_out_itr) {
				*p_out_itr = (struct hashtable_itr) {
					.p_key = p_elem->p_key,
					.key_len = p_elem->key_len,
					.p_value = p_elem->p_value,
					.p_table_ = p_table,
					._bucket_index = bucket_index,
					._p_element = p_elem
				};
			}
			return 1;
        }
        p_elem = p_elem->p_next;
    }

    return 0;
}

void* hashtable_add(struct hashtable* p_table, const void* p_key, size_t key_len) {
    if (hashtable_find(p_table, p_key, key_len, 0)) {
        CX_LOG_FMT(ERROR, HASHTABLE,
			"hashtable_add: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x },"
			"p_key=%x, key_len=%llu: Couldn't add new item. An item with the specified key already exists\n",
			p_table, p_table->element_size_, p_table->n_elements_, p_table->n_buckets_, p_table->p_buckets_,
			p_key, key_len);
        return 0;
    }

    const size_t elem_size = sizeof(struct hashtable_element) + key_len + p_table->element_size_;
    unsigned char* p_new_elem_bytes = malloc(elem_size);

    if (!p_new_elem_bytes) {
        CX_LOG_FMT(ERROR, HASHTABLE,
			"hashtable_add: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x }, "
			"p_key=%x, key_len=%llu: Couldn't allocate memory for new item\n",
			p_table, p_table->element_size_, p_table->n_elements_, p_table->n_buckets_, p_table->p_buckets_,
			p_key, key_len);
        return 0;
    }

    struct hashtable_element* p_new_elem = (void*)p_new_elem_bytes;
    *p_new_elem = (struct hashtable_element) {
        .p_key = p_new_elem_bytes + sizeof(*p_new_elem),
        .key_len = key_len,
        .p_value = p_new_elem_bytes + sizeof(*p_new_elem) + key_len
    };

    memcpy(p_new_elem->p_key, p_key, key_len);

    const float new_load_ratio = (float)(p_table->n_elements_ + 1) / p_table->n_buckets_;
    if (p_table->n_buckets_ == 0 || new_load_ratio > HASHTABLE_LOAD_THRESHOLD) {
        const size_t new_n_buckets = p_table->n_elements_ ? p_table->n_elements_ * 2 : HASHTABLE_MIN_BUCKETS;
        if (!hashtable_resize(p_table, new_n_buckets)) {
            CX_LOG_FMT(ERROR, HASHTABLE,
				"hashtable_add: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x }, "
				"p_key=%x, key_len=%llu: Couldn't resize hashtable for new item\n",
				p_table, p_table->element_size_, p_table->n_elements_, p_table->n_buckets_, p_table->p_buckets_,
				p_key, key_len);
            free(p_new_elem);
            return 0;
        }
    }

    hashtable_bucket_append(hashtable_find_bucket(p_table, p_key, key_len, 0), p_new_elem);
    ++p_table->n_elements_;
    
    return p_new_elem->p_value;
}

void* hashtable_get(struct hashtable* p_table, const void* p_key, size_t key_len) {
	struct hashtable_itr itr;
	if (!hashtable_find(p_table, p_key, key_len, &itr)) {
		return hashtable_add(p_table, p_key, key_len);
	}
	
	return itr.p_value;
}

void hashtable_remove(struct hashtable* p_table, const void* p_key, size_t key_len) {
    struct hashtable_bucket* p_bucket = hashtable_find_bucket(p_table, p_key, key_len, 0);
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

int hashtable_s_find(const struct hashtable* p_table, const char* s_key, struct hashtable_itr* p_out_itr) {
    return hashtable_find(p_table, s_key, strlen(s_key) + 1, p_out_itr);
}

void* hashtable_s_add(struct hashtable* p_table, const char* s_key) {
    void* result = hashtable_add(p_table, s_key, strlen(s_key) + 1);

CX_DBG(
	CX_LOG_FMT(TRACE, HASHTABLE,
		"Hashtable 0x%p: element_size=%llu, n_elements=%llu, n_buckets=%llu, buckets=0x%p\n",
		p_table,
		p_table->element_size_,
		p_table->n_elements_,
		p_table->n_buckets_,
		p_table->p_buckets_);
    
    struct hashtable_itr itr;
	hashtable_itr(p_table, &itr);

	while (hashtable_itr_is_valid(&itr)) {
		CX_LOG_FMT(TRACE, HASHTABLE, "  [%s] -> 0x%p\n", itr.p_key, itr.p_value);
			hashtable_itr_next(&itr);
	}
);

	return result;
}

void* hashtable_s_get(struct hashtable* p_table, const char* s_key) {
    return hashtable_get(p_table, s_key, strlen(s_key) + 1);
}

void hashtable_s_remove(struct hashtable* p_table, const char* s_key) {
    hashtable_remove(p_table, s_key, strlen(s_key) + 1);
}

int hashtable_i_find(const struct hashtable* p_table, uint32_t key, struct hashtable_itr* p_out_itr) {
    return hashtable_find(p_table, &key, sizeof(key), p_out_itr);
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

    if (p_table->n_elements_ == 0) {
        return;
    }

    const struct hashtable_bucket* p_buckets = p_table->p_buckets_;
    const struct hashtable_element* p_elem = p_buckets[0].p_first;

    while (!p_elem) {
        ++(p_itr->bucket_index_);

        if (p_itr->bucket_index_ < p_table->n_buckets_) {
            p_elem = p_buckets[p_itr->bucket_index_].p_first;
        } else {
            *p_itr = (struct hashtable_itr){0};
            return;
        }
    }

    p_itr->p_key = p_elem->p_key;
    p_itr->key_len = p_elem->key_len;
    p_itr->p_value = p_elem->p_value;
    p_itr->p_table_ = p_table;
    p_itr->p_element_ = p_elem;
}

void hashtable_itr_next(struct hashtable_itr* p_itr) {
    const struct hashtable_element* p_elem = p_itr->p_element_;
    p_elem = p_elem->p_next;

    const struct hashtable_bucket* p_buckets = p_itr->p_table_->p_buckets_;
    while (!p_elem) {
        ++(p_itr->bucket_index_);

        if (p_itr->bucket_index_ < p_itr->p_table_->n_buckets_) {
            p_elem = p_buckets[p_itr->bucket_index_].p_first;
        } else {
            *p_itr = (struct hashtable_itr){0};
            return;
        }
    }

    p_itr->p_key = p_elem->p_key;
    p_itr->key_len = p_elem->key_len;
    p_itr->p_value = p_elem->p_value;
    p_itr->p_element_ = p_elem;
}

int hashtable_itr_is_valid(const struct hashtable_itr* p_itr) {
    return !!p_itr->p_element_;
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

struct hashtable_bucket* hashtable_find_bucket(
	const struct hashtable* p_table,
	const void* p_key,
	size_t key_len,
	size_t* p_out_index) {

    const size_t index = hash_key(p_key, key_len) % p_table->n_buckets_;

	if (p_out_index) {
		*p_out_index = index;
	}

    return (struct hashtable_bucket*)p_table->p_buckets_ + index;
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
    const size_t n_buckets_old = p_table->n_buckets_;
    struct hashtable_bucket* p_buckets_old = p_table->p_buckets_;

    void* p_buckets = calloc(n_buckets, sizeof(struct hashtable_bucket));
    if (!p_buckets) {
        CX_LOG_FMT(ERROR, HASHTABLE,
			"hashtable_resize: p_table=(%x){ element_size=%llu, n_elements=%llu, n_buckets=%llu, p_buckets=%x }, "
			"new_n_buckets=%llu: Couldn't allocate memory\n",
			p_table, p_table->element_size_, p_table->n_elements_, p_table->n_buckets_, p_table->p_buckets_,
			n_buckets);
        return 0;
    }

    p_table->n_buckets_ = n_buckets;
    p_table->p_buckets_ = p_buckets;
    
    for (size_t i = 0; i < n_buckets_old; ++i) {
        struct hashtable_element* p_elem = p_buckets_old[i].p_first;
        while (p_elem) {
            struct hashtable_element* p_next = p_elem->p_next;
            struct hashtable_bucket* p_new_bucket = hashtable_find_bucket(p_table, p_elem->p_key, p_elem->key_len, 0);
            hashtable_bucket_append(p_new_bucket, p_elem);
            p_elem = p_next;
        }
    }

    free(p_buckets_old);

    return 1;
}
