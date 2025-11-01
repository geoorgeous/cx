#ifndef _H__ASSET
#define _H__ASSET

#include <stdio.h>
#include <stdint.h>

#include "hashtable.h"

#define ASSET_NAME_MAX_LEN 64
#define ASSET_PACKAGE_FILENAME_MAX_LEN 260
#define ASSET_TYPE_MAX 0xFF
#define ASSET_IDN_MAX 0x00FFFFFF
#define ASSET_IDN_MASK ASSET_IDN_MAX
#define ASSET_ID(TYPE, IDN) ((uint32_t)(((uint8_t)(TYPE)) << 24) | (((int)(IDN)) & ASSET_IDN_MASK))
#define GET_ASSET_TYPE(ID) ((uint8_t)((ID) >> 24))
#define GET_ASSET_IDN(ID) (((uint32_t)(ID)) & ASSET_IDN_MASK)

#define ASSET_STRUCT(NAME)\
int  NAME##_serialize(FILE* p_file, const void* p_##NAME);\
int  NAME##_deserialize(FILE* p_file, void* p_##NAME);\
void NAME##_free(void* p_##NAME);\
struct NAME

#define ASSET_REGISTER_TYPE(NAME, TYPE_ID)\
register_asset_type(TYPE_ID, #NAME, sizeof(struct NAME), NAME##_serialize, NAME##_deserialize, NAME##_free)

typedef uint32_t asset_id;

struct asset {
    asset_id _id;
    char     s_name[ASSET_NAME_MAX_LEN];
    void*    _p_data;
};

typedef int(*asset_serialize_proc)(FILE*, const void*);
typedef int(*asset_deserialize_proc)(FILE*, void*);
typedef void(*asset_free_proc)(void*);

void register_asset_type(uint8_t asset_type_id, const char* s_display_name, size_t size, asset_serialize_proc f_serialize, asset_deserialize_proc f_deserialize, asset_free_proc f_free);

struct asset_package_record {
    struct asset          _asset;
    struct asset_package* _p_package;
    uint32_t              _file_location;
};

typedef struct asset_package_record* asset_handle;

int  asset_load(asset_handle p_record);
void asset_free(asset_handle p_record);

struct asset_package {
    char             _s_filename[ASSET_PACKAGE_FILENAME_MAX_LEN];
    struct hashtable _asset_type_record_tables;
};

void asset_package_init(struct asset_package* p_package);
void asset_package_free(struct asset_package* p_package);
int  asset_package_load_records(struct asset_package* p_result, const char* s_filename);
void asset_package_save(struct asset_package* p_package);
void asset_package_save_as(struct asset_package* p_package, const char* s_filename);
asset_handle asset_package_find_record(const struct asset_package* p_package, asset_id id);
asset_handle asset_package_new_record(struct asset_package* p_package, uint8_t type);
void asset_package_delete_record(struct asset_package* p_package, asset_id id);

void                   asset_directory_register_package(const struct asset_package* p_package);
asset_handle           asset_directory_find(asset_id id);

void serialize_asset_handle(FILE* p_file, const asset_handle p_asset_handle);
void deserialize_asset_handle(FILE* p_file, asset_handle* p_result);

#endif