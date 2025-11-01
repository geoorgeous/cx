#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

#define JSON_OBJECT_TABLE_LOAD_THRESHOLD 0.7f
#define JSON_OBJECT_TABLE_MIN_BUCKETS 8
#define JSON_ARRAY_CAP_MIN 8

struct json_parser {
    size_t      json_source_len;
    const char* s_json_source;
    const char* p_last_char;
};

struct json_object_table {
    size_t                    num_elements;
    struct hash_table_bucket* p_buckets;
    size_t                    n_buckets;
};

struct json_array {
    size_t             capacity;
    size_t             length;
    struct json_value* data;
};

struct json_value {
    enum json_type type;
    union {
        struct json_object_table as_object_table;
        struct json_array        as_array;
        char*                    as_string;
        double                   as_number;
    } value;
};

struct hash_table_bucket_element {
    char*                             s_key;
    struct json_value                 value;
    struct hash_table_bucket_element* next;
};

struct hash_table_bucket {
    struct hash_table_bucket_element* first;
    struct hash_table_bucket_element* last;
};

static void reset_value(struct json_value* p_value);

// Parsing helpers
static void next_token(struct json_parser* p_parser);
static int  cmp_string(struct json_parser* p_parser, const char* s_str);
static int  parse_value(struct json_parser* p_parser, struct json_value* p_value);
static int  parse_null_value(struct json_parser* p_parser, struct json_value* p_null_value);
static int  parse_bool_value(struct json_parser* p_parser, struct json_value* p_bool_value);
static int  parse_object_value(struct json_parser* p_parser, struct json_value* p_object_value);
static int  parse_array_value(struct json_parser* p_parser, struct json_value* p_array_value);
static int  parse_string_value(struct json_parser* p_parser, struct json_value* p_string_value);
static int  parse_number_value(struct json_parser* p_parser, struct json_value* p_number_value);
static int  parse_string(struct json_parser* p_parser, char** p_s_result);

// JSON object hash table helpers
static size_t                            string_hash(const char* s);
static void                              hash_table_bucket_append(struct hash_table_bucket* p_bucket, struct hash_table_bucket_element* p_element);
static struct hash_table_bucket*         json_object_table_find_bucket(const struct json_object_table* p_table, const char* s_key);
static int                               json_object_table_resize(struct json_object_table* p_table, size_t num_buckets);
static struct hash_table_bucket_element* new_hash_table_bucket_element(const char* s_key);

// JSON array helpers
static int json_array_resize(struct json_array* p_array, size_t capacity);
static int json_array_shrink(struct json_array* p_array);

int json_parse(const char* s_json, size_t len, struct json_value** p_result) {
    *p_result = malloc(sizeof(struct json_value));
    json_init_null(*p_result);

    struct json_parser json_parser = {
        .s_json_source = s_json,
        .json_source_len = len,
        .p_last_char = s_json
    };
    
    int n;
    sscanf(json_parser.p_last_char, " %n", &n);
    json_parser.p_last_char += n;

    if (parse_object_value(&json_parser, *p_result) || parse_array_value(&json_parser, *p_result)) {
        return 1;
    }

    free(*p_result);
    return 0;
}

void json_free(struct json_value* p_json_root_value) {
    reset_value(p_json_root_value);
}

enum json_type json_typeof(const struct json_value* p_value) {
    return p_value->type;
}

void json_init_null(struct json_value* p_value) {
    reset_value(p_value);
    p_value->type = JSON_TYPE_null;
}

void json_init_bool(struct json_value* p_value) {
    reset_value(p_value);
    p_value->type = JSON_TYPE_false;
}

void json_init_object(struct json_value* p_value) {
    reset_value(p_value);
    p_value->type = JSON_TYPE_object;
}

void json_init_array(struct json_value* p_value) {
    reset_value(p_value);
    p_value->type = JSON_TYPE_array;
}

void json_init_string(struct json_value* p_value) {
    reset_value(p_value);
    p_value->type = JSON_TYPE_string;
}

void json_init_number(struct json_value* p_value) {
    reset_value(p_value);
    p_value->type = JSON_TYPE_number;
}

int json_is_null(const struct json_value* p_value) {
    return p_value && p_value->type == JSON_TYPE_null;
}

int json_is_bool(const struct json_value* p_value) {
    return p_value && (p_value->type == JSON_TYPE_true || p_value->type == JSON_TYPE_false);
}

int json_is_object(const struct json_value* p_value) {
    return p_value && p_value->type == JSON_TYPE_object;
}

int json_is_array(const struct json_value* p_value) {
    return p_value && p_value->type == JSON_TYPE_array;
}

int json_is_string(const struct json_value* p_value) {
    return p_value && p_value->type == JSON_TYPE_string;
}

int json_is_number(const struct json_value* p_value) {
    return p_value && p_value->type == JSON_TYPE_number;
}

int json_bool(const struct json_value* p_json_bool) {
    return p_json_bool->type == JSON_TYPE_true;
}

void json_bool_set(struct json_value* p_json_bool, int b) {
    if (json_is_bool(p_json_bool)) {
        p_json_bool->type = b ? JSON_TYPE_true : JSON_TYPE_false;
    }
}

size_t json_object_num_properties(const struct json_value* p_json_object) {
    if (json_is_object(p_json_object)) {
        return p_json_object->value.as_object_table.num_elements;
    }
    return 0;
}

struct json_value* json_object_get(const struct json_value* p_json_object, const char* s_key) {
    if (!json_is_object(p_json_object)) {
        return 0;
    }

    const struct json_object_table* p_tab = &p_json_object->value.as_object_table;

    if (p_tab->num_elements == 0) {
        return 0;
    }

    const struct hash_table_bucket* p_bucket = json_object_table_find_bucket(p_tab, s_key);
    struct hash_table_bucket_element* p_elem = p_bucket->first;

    while (p_elem) {
        if (strcmp(p_elem->s_key, s_key) == 0) {
            return &p_elem->value;
        }
        p_elem = p_elem->next;
    }

    return 0;
}

struct json_value* json_object_add(struct json_value* p_json_object, const char* s_key) {
    if (!json_is_object(p_json_object)) {
        return 0;
    }

    struct json_object_table* p_tab = &p_json_object->value.as_object_table;

    struct json_value* p_value = json_object_get(p_json_object, s_key);
    if (p_value) {
        return p_value;
    }

    struct hash_table_bucket_element* p_new_elem = new_hash_table_bucket_element(s_key);
    if (!p_new_elem) {
        return 0;
    }
    
    const float new_load_ratio = (float)(p_tab->num_elements + 1) / p_tab->n_buckets;
    if (p_tab->n_buckets == 0 || new_load_ratio > JSON_OBJECT_TABLE_LOAD_THRESHOLD) {
        const size_t new_num_buckets = p_tab->num_elements ? p_tab->num_elements * 2 : JSON_OBJECT_TABLE_MIN_BUCKETS;
        if (!json_object_table_resize(p_tab, new_num_buckets)) {
            free(p_new_elem->s_key);
            free(p_new_elem);
            return 0;
        }
    }

    hash_table_bucket_append(json_object_table_find_bucket(p_tab, s_key), p_new_elem);
    ++p_tab->num_elements;

    return &p_new_elem->value;
}

void json_object_remove(struct json_value* p_json_object, const char*) {
    if (!json_is_object(p_json_object)) {
        return;
    }

    // todo:
}

size_t json_array_len(const struct json_value* p_json_array) {
    if (json_is_array(p_json_array)) {
        return p_json_array->value.as_array.length;
    }
    return 0;
}

struct json_value* json_array_get(const struct json_value* p_json_array, size_t index) {
    if (json_is_array(p_json_array) && json_array_len(p_json_array) > index) {
        return p_json_array->value.as_array.data + index;
    }
    return 0;
}

struct json_value* json_array_push(struct json_value* p_json_array) {
    if (!json_is_array(p_json_array)) {
        return 0;
    }

    struct json_array* p_arr = &p_json_array->value.as_array;

    if (p_arr->length + 1 > p_arr->capacity) {
        const size_t new_capacity = p_arr->capacity ? p_arr->capacity * 2 : JSON_ARRAY_CAP_MIN;
        if (!json_array_resize(p_arr, new_capacity)) {
            return 0;
        }
    }

    ++p_arr->length;

    return &p_arr->data[p_arr->length - 1];
}

void json_array_remove(struct json_value* p_json_array, size_t index) {
    struct json_array* p_arr = &p_json_array->value.as_array;
    if (!json_is_array(p_json_array) || index >= p_arr->length) {
        return;
    }

    --p_arr->length;

    if (index < p_arr->length) {
        void* p_dst = p_arr->data + index;
        void* p_src = p_arr->data + p_arr->length;
        memcpy(p_dst, p_src, sizeof(*p_arr->data));
    }

    if (p_arr->capacity >= JSON_ARRAY_CAP_MIN * 2 && p_arr->length <= p_arr->capacity / 2) {
        json_array_shrink(p_arr);
    }
}

const char* json_string(const struct json_value* p_json_string) {
    if (json_is_string(p_json_string)) {
        return p_json_string->value.as_string;
    }
    return 0;
}

void json_string_set(struct json_value* p_json_string, const char* s_src, size_t src_len) {
    if (!json_is_string(p_json_string)) {
        return;
    }

    if (!src_len) {
        src_len = strlen(s_src);
    }

    void* p = realloc(p_json_string->value.as_string, src_len + 1);
    if (!p) {
        return;
    }

    p_json_string->value.as_string = p;
    memcpy(p_json_string->value.as_string, s_src, src_len);
    p_json_string->value.as_string[src_len] = '\0';
}

double json_number(const struct json_value* p_json_number) {
    if (json_is_number(p_json_number)) {
        return p_json_number->value.as_number;
    }
    return 0;
}

void json_number_set(struct json_value* p_json_number, double d) {
    if (json_is_number(p_json_number)) {
        p_json_number->value.as_number = d;
    }
}

void reset_value(struct json_value* p_value) {
    switch (p_value->type) {
        case JSON_TYPE_true: {
            p_value->type = JSON_TYPE_false;
            break;
        }

        case JSON_TYPE_object: {
            struct json_object_table* p_tab = &p_value->value.as_object_table;
            for (size_t i = 0; i < p_tab->n_buckets; ++i) {
                struct hash_table_bucket_element* p_elem = p_tab->p_buckets[i].first;
                while (p_elem) {
                    struct hash_table_bucket_element* p_next = p_elem->next;
                    reset_value(&p_elem->value);
                    free(p_elem->s_key);
                    free(p_elem);
                    p_elem = p_next;
                }
            }

            free(p_tab->p_buckets);
            
            break;
        }

        case JSON_TYPE_array: {
            for (size_t i = 0; i < p_value->value.as_array.length; ++i) {
                reset_value(json_array_get(p_value, i));
            }
            free(p_value->value.as_array.data);
            break;
        }

        case JSON_TYPE_string:
            free(p_value->value.as_string);
            break;

        default: break;
    }
    
    memset(&p_value->value, 0, sizeof(p_value->value));
}

void next_token(struct json_parser* p_parser) {
    int n;
    sscanf(++p_parser->p_last_char, " %n", &n);
    p_parser->p_last_char += n;
}

int cmp_string(struct json_parser* p_parser, const char* s_str) {
    for (; *s_str; ++s_str, ++p_parser->p_last_char) {
        if (*p_parser->p_last_char != *s_str) {
            return 0;
        }
    }
    --p_parser->p_last_char;
    return 1;
}

int parse_value(struct json_parser* p_parser, struct json_value* p_value) {
    if (parse_object_value(p_parser, p_value) ||
        parse_array_value(p_parser, p_value) ||
        parse_string_value(p_parser, p_value) ||
        parse_null_value(p_parser, p_value) ||
        parse_bool_value(p_parser, p_value) ||
        parse_number_value(p_parser, p_value)) {
        return 1;
    }
    fprintf(stderr, "JSON parsing error: Unexpected token '%c'. %llu\n", *p_parser->p_last_char, p_parser->p_last_char - p_parser->s_json_source);
    return 0;
}

int parse_null_value(struct json_parser* p_parser, struct json_value* p_null_value) {
    int b_result = cmp_string(p_parser, "null");
    if (b_result) {
        p_null_value->type = JSON_TYPE_null;
    }
    return b_result;
}

int parse_bool_value(struct json_parser* p_parser, struct json_value* p_bool_value) {
    int b_cmp_result;

    b_cmp_result = cmp_string(p_parser, "true");
    if (b_cmp_result) {
        p_bool_value->type = JSON_TYPE_true;
        return 1;
    }

    b_cmp_result = cmp_string(p_parser, "false");
    if (b_cmp_result) {
        p_bool_value->type = JSON_TYPE_false;
        return 1;
    }

    return 0;
}

int parse_object_value(struct json_parser* p_parser, struct json_value* p_object_value) {
    if (*p_parser->p_last_char != '{') {
        return 0;
    }

    p_object_value->type = JSON_TYPE_object;
    memset(&p_object_value->value, 0, sizeof(p_object_value->value));

    char* s_key = 0;
    for (;;) {
        next_token(p_parser);
        if (*p_parser->p_last_char == '}') {
            return 1;
        }

        if (p_object_value->value.as_object_table.num_elements > 0 && *p_parser->p_last_char == ',') {
            next_token(p_parser);
        }

        if (!parse_string(p_parser, &s_key)) {
            break;
        }

        next_token(p_parser);

        if (*p_parser->p_last_char != ':') {
            fprintf(stderr, "Error: Expected ':' after property name.\n");
            break;
        }

        next_token(p_parser);

        struct json_value* p_property_value = json_object_add(p_object_value, s_key);
        if (!p_property_value) {
            break;
        }

        if (!parse_value(p_parser, p_property_value)) {
            break;
        }

        free(s_key);
        s_key = 0;
    }

    free(s_key);
    json_init_null(p_object_value);

    return 0;
}

int parse_array_value(struct json_parser* p_parser, struct json_value* p_array_value) {
    if (*p_parser->p_last_char != '[') {
        return 0;
    }

    p_array_value->type = JSON_TYPE_array;
    memset(&p_array_value->value, 0, sizeof(p_array_value->value));

    for (;;) {
        next_token(p_parser);

        if (*p_parser->p_last_char == ']') {
            return 1;
        }

        if (p_array_value->value.as_array.length > 0 && *p_parser->p_last_char == ',') {
            next_token(p_parser);
        }

        struct json_value* p_new_array_item_value = json_array_push(p_array_value);
        if (!p_new_array_item_value) {
            fprintf(stderr, "Error: Failed to add element to array.\n");
            break;
        }

        if (!parse_value(p_parser, p_new_array_item_value)) {
            printf("Failed to parse array item: %llu\n", p_array_value->value.as_array.length - 1);
            break;
        }
    }

    json_init_null(p_array_value);

    return 0;
}

int parse_string_value(struct json_parser* p_parser, struct json_value* p_string_value) {
    if (!parse_string(p_parser, &p_string_value->value.as_string)) {
        return 0;
    }
    p_string_value->type = JSON_TYPE_string;
    return 1;
}

int parse_number_value(struct json_parser* p_parser, struct json_value* p_number_value) {
    if (*p_parser->p_last_char != '-' && (*p_parser->p_last_char < '0' || *p_parser->p_last_char > '9')) {
        return 0;
    }

    char* end_p;
    double number = strtod(p_parser->p_last_char, &end_p);
    if (end_p == p_parser->p_last_char) {
        fprintf(stderr, "Error: Failed to parse JSON number.\n");
        return 0;
    }
    
    p_number_value->type = JSON_TYPE_number;
    p_number_value->value.as_number = number;

    p_parser->p_last_char = end_p - 1;

    return 1;
}

int parse_string(struct json_parser* p_parser, char** p_s_result) {
    if (*p_parser->p_last_char != '"') {
        return 0;
    }

    const char* src_str = p_parser->p_last_char + 1;
    for (;;) {
        ++p_parser->p_last_char;

        if (*p_parser->p_last_char == '"') {
            size_t temp_string_length = p_parser->p_last_char - src_str;
            if (temp_string_length > 0) {
                *p_s_result = malloc(temp_string_length + 1);

                if (!*p_s_result) {
                    fprintf(stderr, "Error parsing JSON string: Failed to allocate memory for parsed JSON string.\n");
                    return 0;
                }

                strncpy(*p_s_result, src_str, temp_string_length);
                (*p_s_result)[temp_string_length] = '\0';
            }
            return 1;
        }

        if (*p_parser->p_last_char == '\\') {
            ++p_parser->p_last_char;
            if (*p_parser->p_last_char == 'u') {
                for (int i = 0; i < 4; ++i) {
                    p_parser->p_last_char++;
                    if (!(
                        (*p_parser->p_last_char >= '0' && *p_parser->p_last_char <= '9') ||
                        (*p_parser->p_last_char >= 'a' && *p_parser->p_last_char <= 'f') ||
                        (*p_parser->p_last_char >= 'A' && *p_parser->p_last_char <= 'F')
                    )) {
                        fprintf(stderr, "Error parsing JSON string: Bad unicode escape.\n");
                        return 0;
                    }
                }
            }
        } else if ((size_t)(p_parser->p_last_char - p_parser->s_json_source) >= p_parser->json_source_len) {
            break;
        }
    }
    fprintf(stderr, "Error parsing JSON string: Unterminated string.\"\n");
    return 0;
}

size_t string_hash(const char* s) {
    size_t h = 0;
    for (const unsigned char* p = (const unsigned char*)s; *p != '\0'; p++) {
        h = 37 * h + *p;
    }
    return h;
}

void hash_table_bucket_append(struct hash_table_bucket* p_bucket, struct hash_table_bucket_element* p_element) {
    if (!p_bucket->first) {
        p_bucket->first = p_element;
    } else {
        p_bucket->last->next = p_element;
    }

    p_bucket->last = p_element;
    p_bucket->last->next = 0;
}

struct hash_table_bucket* json_object_table_find_bucket(const struct json_object_table* p_table, const char* s_key) {
    const size_t index = string_hash(s_key) % p_table->n_buckets;
    return p_table->p_buckets + index;
}

int json_object_table_resize(struct json_object_table* p_table, size_t num_buckets) {
    const size_t buckets_old_n = p_table->n_buckets;
    struct hash_table_bucket* p_buckets_old = p_table->p_buckets;

    void* p = calloc(num_buckets, sizeof(*p_table->p_buckets));
    if (!p) {
        return 0;
    }

    p_table->n_buckets = num_buckets;
    p_table->p_buckets = p;
    
    for (size_t i = 0; i < buckets_old_n; ++i) {
        struct hash_table_bucket_element* p_elem = p_buckets_old[i].first;
        while (p_elem) {
            struct hash_table_bucket_element* p_next = p_elem->next;
            struct hash_table_bucket* p_new_bucket = json_object_table_find_bucket(p_table, p_elem->s_key);
            hash_table_bucket_append(p_new_bucket, p_elem);
            p_elem = p_next;
        }
    }

    free(p_buckets_old);

    return 1;
}

struct hash_table_bucket_element* new_hash_table_bucket_element(const char* s_key) {
    struct hash_table_bucket_element* p_new_element = malloc(sizeof(struct hash_table_bucket_element));

    if (!p_new_element) {
        return 0;
    }
    
    const size_t key_len = strlen(s_key);
    p_new_element->s_key = malloc(key_len + 1);

    if (!p_new_element->s_key) {
        free(p_new_element);
        return 0;
    }
    
    memcpy(p_new_element->s_key, s_key, key_len);
    p_new_element->s_key[key_len] = '\0';

    p_new_element->value.type = JSON_TYPE_null;
    memset(&p_new_element->value, 0, sizeof(p_new_element->value));

    return p_new_element;
}

int json_array_resize(struct json_array* p_array, size_t capacity) {
    void* p = realloc(p_array->data, sizeof(*p_array->data) * capacity);
    if (!p) {
        return 0;
    }
    p_array->capacity = capacity;
    p_array->data = p;
    return 1;
}

int json_array_shrink(struct json_array* p_array) {
    return json_array_resize(p_array, p_array->length);
}