#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asset.h"
#include "darr.h"
#include "logging.h"
#include "serialization.h"

#define LOG_CAT_ASSET "asset"

static struct asset_type_table {
    char                   s_display_name[ASSET_NAME_MAX_LEN];
    size_t                 size;
    asset_serialize_proc   f_serialize;
    asset_deserialize_proc f_deserialize;
    asset_free_proc        f_free;
} asset_type_tables[ASSET_TYPE_MAX];

static struct asset_type_table asset_type_tables[ASSET_TYPE_MAX];

static struct asset_directory {
    const struct asset_package** pp_packages;
    size_t n_packages;
} g_directory;

static uint32_t rand_idn(void);

void register_asset_type(uint8_t asset_type_id, const char* s_display_name, size_t size, asset_serialize_proc f_serialize, asset_deserialize_proc f_deserialize, asset_free_proc f_free) {
    if (asset_type_tables[asset_type_id].f_serialize) {
        log_msg(LOG_ERROR, LOG_CAT_ASSET, "Asset type %d already registered by type with display name '%s'.\n", asset_type_id, asset_type_tables[asset_type_id].s_display_name);
        return;
    }

    log_msg(LOG_INFO, LOG_CAT_ASSET, "New asset type registered: { id=%d, name='%s' }\n", asset_type_id, s_display_name);

    strcpy(asset_type_tables[asset_type_id].s_display_name, s_display_name);
    asset_type_tables[asset_type_id].size = size;
    asset_type_tables[asset_type_id].f_serialize = f_serialize;
    asset_type_tables[asset_type_id].f_deserialize = f_deserialize;
    asset_type_tables[asset_type_id].f_free = f_free;
}

int asset_load(struct asset_package_record* p_record) {
    FILE* p_file = fopen(p_record->_p_package->_s_filename, "rb");

    if (!p_file) {
        log_msg(LOG_ERROR, LOG_CAT_ASSET, "Couldn't load asset. Failed to open file '%s'\n", p_record->_p_package->_s_filename);
        return 0;
    }

    fseek(p_file, p_record->_file_location, SEEK_CUR);
    
    const struct asset_type_table* p_type_table = &asset_type_tables[GET_ASSET_TYPE(p_record->_asset._id)];

    p_record->_asset._p_data = calloc(1, p_type_table->size);

    const int b_result = p_type_table->f_deserialize(p_file, p_record->_asset._p_data);

    fclose(p_file);

    return b_result;
}

void asset_free(struct asset_package_record* p_record) {
    if (!p_record->_asset._p_data)
        return;
    
    const struct asset_type_table* p_type_table = &asset_type_tables[GET_ASSET_TYPE(p_record->_asset._id)];
    p_type_table->f_free(p_record->_asset._p_data);
    free(p_record->_asset._p_data);
    p_record->_asset._p_data = 0;
}

void asset_package_init(struct asset_package* p_package) {
    *p_package = (struct asset_package) {0};
    hashtable_init(&p_package->_asset_type_record_tables, sizeof(struct hashtable));
}

void asset_package_free(struct asset_package* p_package) {
    struct hashtable_itr itr;
    struct hashtable_itr records_itr;

    hashtable_itr(&p_package->_asset_type_record_tables, &itr);
    while (hashtable_itr_is_valid(&itr)) {
        hashtable_itr(itr.p_value, &records_itr);

        while(hashtable_itr_is_valid(&records_itr)) {
            struct asset_package_record* p_record = records_itr.p_value;
            asset_free(p_record);
            hashtable_itr_next(&records_itr);
        }

        hashtable_free(itr.p_value);

        hashtable_itr_next(&itr);
    }

    hashtable_free(&p_package->_asset_type_record_tables);
}

int asset_package_load_records(struct asset_package* p_result, const char* s_filename) {
    FILE* p_file = fopen(s_filename, "rb");

    if (!p_file) {
        log_msg(LOG_ERROR, LOG_CAT_ASSET, "Couldn't load asset package: failed to open file '%s\n", s_filename);
        return 0;
    }

    strcpy(p_result->_s_filename, s_filename);

    uint32_t num_records = 0;
    deserialize_uint32(p_file, &num_records);
    
    log_msg(LOG_INFO, LOG_CAT_ASSET, "Loading %d assets from package file '%s'...\n", num_records, p_result->_s_filename);

    if (num_records == 0) {
        log_msg(LOG_ERROR, LOG_CAT_ASSET, "Couldn't load asset package from file '%s': file contains no assets.\n", s_filename);
        return 0;
    }

    for (size_t i = 0; i < num_records; ++i) {
        asset_id id;
        deserialize_uint32(p_file, &id);

        struct hashtable* p_asset_type_table = hashtable_i_add(&p_result->_asset_type_record_tables, GET_ASSET_TYPE(id));

        struct asset_package_record* p_new_record = hashtable_i_add(p_asset_type_table, id);
        *p_new_record = (struct asset_package_record) {
            ._asset = { ._id = id },
            ._p_package = p_result
        };

        deserialize_uint32(p_file, &p_new_record->_file_location);

        long offset = ftell(p_file);

        fseek(p_file, p_new_record->_file_location, SEEK_SET);
        
        uint32_t name_len = 0;
        deserialize_str(p_file, 0, &name_len);
        deserialize_str(p_file, p_new_record->_asset.s_name, &name_len);

        log_msg(LOG_TRACE, LOG_CAT_ASSET, "  Asset record loaded: '%s' (%s:%x)...\n", p_new_record->_asset.s_name, asset_type_tables[GET_ASSET_TYPE(p_new_record->_asset._id)].s_display_name, p_new_record->_asset._id);

        fseek(p_file, offset, SEEK_SET);
    }
    
    // for (size_t i = 0; i < num_records; ++i) {
    //     struct asset_package_record** pp_record = dynarray_get(&p_result->_records, i);
    //     struct asset_package_record* p_record = *pp_record;

    //     fseek(p_file, p_record->_file_location, SEEK_SET);
        
    //     uint32_t name_len = 0;
    //     deserialize_str(p_file, 0, &name_len);
    //     deserialize_str(p_file, p_record->_asset.s_name, &name_len);

    //     log_msg(LOG_LEVEL_TRACE, LOG_CAT_ASSET, "  Asset record loaded: '%s' (%s:%x)...\n", p_record->_asset.s_name, asset_type_tables[GET_ASSET_TYPE(p_record->_asset._id)].s_display_name, p_record->_asset._id);

    // }

    return (int)num_records;
}

void asset_package_save(struct asset_package* p_package) {
    FILE* p_file = fopen(p_package->_s_filename, "wb");
    
    if (!p_file) {
        log_msg(LOG_ERROR, LOG_CAT_ASSET, "Couldn't save asset package: failed to open file '%s' for writing\n", p_package->_s_filename);
        return;
    }

    serialize_uint32(p_file, p_package->_asset_type_record_tables._n_elements);

    log_msg(LOG_TRACE, LOG_CAT_ASSET, "Saving assets to package file '%s'...\n", p_package->_s_filename);

    struct hashtable_itr itr;
    struct hashtable_itr records_itr;
    
    hashtable_itr(&p_package->_asset_type_record_tables, &itr);
    while (hashtable_itr_is_valid(&itr)) {
        hashtable_itr(itr.p_value, &records_itr);

        while(hashtable_itr_is_valid(&records_itr)) {
            struct asset_package_record* p_record = records_itr.p_value;

            // ID
            serialize_uint32(p_file, p_record->_asset._id);

            // FILE LOCATION
            serialize_uint32(p_file, 0);
            
            hashtable_itr_next(&records_itr);
        }

        hashtable_itr_next(&itr);
    }
    
    const size_t asset_record_size = sizeof(asset_id) + sizeof(uint32_t); // ID + FILE_LOCATION

    uint32_t asset_data_file_location = ftell(p_file);
    int i = 0;
    
    hashtable_itr(&p_package->_asset_type_record_tables, &itr);
    while (hashtable_itr_is_valid(&itr)) {
        hashtable_itr(itr.p_value, &records_itr);

        while(hashtable_itr_is_valid(&records_itr)) {
            struct asset_package_record* p_record = records_itr.p_value;

            p_record->_file_location = asset_data_file_location;

            // NAME
            serialize_str(p_file, p_record->_asset.s_name, 0);
            
            // DATA
            const struct asset_type_table* p_type_table = &asset_type_tables[GET_ASSET_TYPE(p_record->_asset._id)];
            const int b_result = p_type_table->f_serialize(p_file, p_record->_asset._p_data);
            log_msg(LOG_TRACE, LOG_CAT_ASSET, "  Asset saved: '%s' (%s:%x)...\n", p_record->_asset.s_name, asset_type_tables[GET_ASSET_TYPE(p_record->_asset._id)].s_display_name, p_record->_asset._id);
            
            if (!b_result) {
                // todo: handle serialization error
                log_msg(LOG_ERROR, LOG_CAT_ASSET, "Asset serialization error: asset_id=%x\n", p_record->_asset._id);
            }

            // Cache the next record's data location
            asset_data_file_location = ftell(p_file);
            
            fseek(p_file, sizeof(uint32_t) + (asset_record_size * i) + sizeof(asset_id), SEEK_SET);
            serialize_uint32(p_file, p_record->_file_location);
            fseek(p_file, asset_data_file_location, SEEK_SET);
                
            hashtable_itr_next(&records_itr);
            ++i;
        }

        hashtable_itr_next(&itr);
    }

    // for (size_t i = 0; i < p_package->_records._n_elems; ++i) {
    //     const struct asset_package_record** pp_record = dynarray_get(&p_package->_records, i);
    //     const struct asset_package_record* p_record = *pp_record;

    //     // ID
    //     serialize_uint32(p_file, p_record->_asset._id);

    //     // FILE LOCATION
    //     serialize_uint32(p_file, 0);
    // }

    // const size_t record_size = sizeof(asset_id) + sizeof(uint32_t); // ID + FILE_LOCATION

    // uint32_t asset_data_file_location = ftell(p_file);

    // for (size_t i = 0; i < p_package->_records._n_elems; ++i) {
    //     struct asset_package_record** pp_record = dynarray_get(&p_package->_records, i);
    //     struct asset_package_record* p_record = *pp_record;
    //     p_record->_file_location = asset_data_file_location;

    //     // NAME
    //     serialize_str(p_file, p_record->_asset.s_name, 0);
        
    //     // DATA
    //     const struct asset_type_table* p_type_table = &asset_type_tables[GET_ASSET_TYPE(p_record->_asset._id)];
    //     const int b_result = p_type_table->f_serialize(p_file, p_record->_asset._p_data);
    //     log_msg(LOG_LEVEL_TRACE, LOG_CAT_ASSET, "  Asset saved: '%s' (%s:%x)...\n", p_record->_asset.s_name, asset_type_tables[GET_ASSET_TYPE(p_record->_asset._id)].s_display_name, p_record->_asset._id);
        
    //     if (!b_result) {
    //         // todo: handle serialization error
    //         log_msg(LOG_LEVEL_ERROR, LOG_CAT_ASSET, "Asset serialization error: asset_id=%x\n", p_record->_asset._id);
    //     }

    //     // Cache the next record's data location
    //     asset_data_file_location = ftell(p_file);
        
    //     fseek(p_file, sizeof(uint32_t) + (record_size * i) + sizeof(asset_id), SEEK_SET);
    //     serialize_uint32(p_file, p_record->_file_location);
    //     fseek(p_file, asset_data_file_location, SEEK_SET);
    // }
    
    fclose(p_file);
}

void asset_package_save_as(struct asset_package* p_package, const char* s_filename) {
    strcpy(p_package->_s_filename, s_filename);
    asset_package_save(p_package);
}

asset_handle asset_package_find_record(const struct asset_package* p_package, asset_id id) {
    const uint8_t asset_type = GET_ASSET_TYPE(id);
    struct hashtable* p_records = hashtable_find(&p_package->_asset_type_record_tables, &asset_type, sizeof(asset_type));
    
    if (!p_records) {
        return 0;
    }

    return hashtable_find(p_records, &id, sizeof(id));
}

asset_handle asset_package_new_record(struct asset_package* p_package, uint8_t type) {
    const asset_id new_asset_id = ASSET_ID(type, rand_idn());

    struct hashtable* p_records = hashtable_find(&p_package->_asset_type_record_tables, &type, sizeof(type));

    if (!p_records) {
        p_records = hashtable_add(&p_package->_asset_type_record_tables, &type, sizeof(type));
        hashtable_init(p_records, sizeof(struct asset_package_record));
    }
    
    struct asset_package_record* p_asset_record = hashtable_add(p_records, &new_asset_id, sizeof(new_asset_id));
    *p_asset_record = (struct asset_package_record) {
        ._asset = { ._id = new_asset_id },
        ._p_package = p_package
    };
    
    strcpy(p_asset_record->_asset.s_name, "New ");
    strcpy(&p_asset_record->_asset.s_name[4], asset_type_tables[type].s_display_name);
    
    log_msg(LOG_INFO, LOG_CAT_ASSET, "New asset created (%s:%x)\n", asset_type_tables[type].s_display_name, p_asset_record->_asset._id);

    return p_asset_record;
}

void asset_package_delete_record(struct asset_package* p_package, asset_id id) {
    const uint8_t asset_type = GET_ASSET_TYPE(id);
    struct hashtable* p_records = hashtable_find(&p_package->_asset_type_record_tables, &asset_type, sizeof(asset_type));
    
    if (!p_records) {
        return;
    }

    hashtable_remove(p_records, &id, sizeof(id));
}

void asset_directory_register_package(const struct asset_package* p_package) {
    ++g_directory.n_packages;
    g_directory.pp_packages = realloc(g_directory.pp_packages, g_directory.n_packages * sizeof(*g_directory.pp_packages));
    g_directory.pp_packages[g_directory.n_packages - 1] = p_package;
}

struct asset_package_record* asset_directory_find(asset_id id) {
    for (size_t i = g_directory.n_packages; i-- > 0;) {
        const struct asset_package* p_package = g_directory.pp_packages[i];

        const uint8_t asset_type = GET_ASSET_TYPE(id);
        const struct hashtable* p_asset_records = hashtable_find(&p_package->_asset_type_record_tables, &asset_type, sizeof(asset_type));

        if (!p_asset_records) {
            continue;
        }
        
        struct asset_package_record* p_record = hashtable_find(p_asset_records, &id, sizeof(id));

        if (!p_record) {
            continue;
        }

        return p_record;
    }

    return 0;
}

void serialize_asset_handle(FILE* p_file, const asset_handle p_asset_handle) {
    serialize_uint32(p_file, p_asset_handle ? p_asset_handle->_asset._id : 0);
}

void deserialize_asset_handle(FILE* p_file, asset_handle* p_result) {
    asset_id id;
    deserialize_uint32(p_file, &id);
    *p_result = asset_directory_find(id);

    if (!*p_result) {
        log_msg(LOG_ERROR, LOG_CAT_ASSET, "Failed to deserialize asset handle: asset not found (id=%x)\n", id);
    }
}

uint32_t rand_idn(void) {
    uint32_t result = (uint32_t)(((uint16_t)rand() << 17) + ((uint16_t)rand() << 2) + ((uint16_t)rand()>>13));
    return result % ASSET_IDN_MAX;
}