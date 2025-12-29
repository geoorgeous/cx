#ifndef _H__JSON
#define _H__JSON

enum json_type {
    JSON_TYPE_null,
    JSON_TYPE_true,
    JSON_TYPE_false,
    JSON_TYPE_object,
    JSON_TYPE_array,
    JSON_TYPE_string,
    JSON_TYPE_number
};

struct json_value;

int  json_parse(const char* s_json, size_t len, struct json_value** p_result);
void json_free(struct json_value* p_json_root_value);

enum json_type json_typeof(const struct json_value* p_value);

void json_init_null(struct json_value* p_value);
void json_init_bool(struct json_value* p_value);
void json_init_object(struct json_value* p_value);
void json_init_array(struct json_value* p_value);
void json_init_string(struct json_value* p_value);
void json_init_number(struct json_value* p_value);

int json_is_null(const struct json_value* p_value);
int json_is_bool(const struct json_value* p_value);
int json_is_object(const struct json_value* p_value);
int json_is_array(const struct json_value* p_value);
int json_is_string(const struct json_value* p_value);
int json_is_number(const struct json_value* p_value);

int  json_bool(const struct json_value* p_json_bool);
void json_bool_set(struct json_value* p_json_bool, int b);

size_t             json_object_num_properties(const struct json_value* p_json_object);
struct json_value* json_object_get(const struct json_value* p_json_object, const char* s_key);
struct json_value* json_object_add(struct json_value* p_json_object, const char* s_key);
void               json_object_remove(struct json_value* p_json_object, const char* s_key);

size_t             json_array_len(const struct json_value* p_json_array);
struct json_value* json_array_get(const struct json_value* p_json_array, size_t index);
struct json_value* json_array_push(struct json_value* p_json_array);
void               json_array_remove(struct json_value* p_json_array, size_t index);

const char* json_string(const struct json_value* p_json_string);
void        json_string_set(struct json_value* p_json_string, const char* s_src, size_t src_len);

double json_number(const struct json_value* p_json_number);
void   json_number_set(struct json_value* p_json_number, double d);

#endif