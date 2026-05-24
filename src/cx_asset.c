#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cx_asset.h"
#include "cx_logging.h"
#include "hashtable.h"
#include "serialization.h"

#define CX_ASSET_TYPE_NAME_MAX_LEN 60

static struct asset_type_table {
    char s_display_name[CX_ASSET_TYPE_NAME_MAX_LEN + 1];
    size_t asset_size;
    cx_asset_serialize_fn f_serialize;
    cx_asset_deserialize_fn f_deserialize;
    cx_asset_free_fn f_free;
} asset_type_tables[CX_ASSET_TYPE_ID_MAX];

static struct asset_directory {
    const struct cx_asset_package** pp_packages;
    size_t n_packages;
} directory;

static void cx_asset_package_new_record_internal(
	struct cx_asset_package* p_package,
	cx_asset_id id,
	struct cx_asset_package_record** pp_out);

static uint32_t cx_asset_generate_random_idn(void);

void cx_asset_register_type(
	uint8_t asset_type_id,
	const char* s_display_name,
	size_t size,
	cx_asset_serialize_fn f_serialize,
	cx_asset_deserialize_fn f_deserialize,
	cx_asset_free_fn f_free) {

    if (asset_type_tables[asset_type_id].f_serialize) {
        CX_LOG_FMT(ERROR, ASSET, "Asset type id %d already registered by type '%s'.\n",
			asset_type_id, asset_type_tables[asset_type_id].s_display_name);
        return;
    }

    CX_LOG_FMT(INFO, ASSET, "Asset type registered: { id=%d, name='%s' }\n", asset_type_id, s_display_name);

    strcpy(asset_type_tables[asset_type_id].s_display_name, s_display_name);
    asset_type_tables[asset_type_id].asset_size = size;
    asset_type_tables[asset_type_id].f_serialize = f_serialize;
    asset_type_tables[asset_type_id].f_deserialize = f_deserialize;
    asset_type_tables[asset_type_id].f_free = f_free;
}

int cx_asset_load(struct cx_asset_package_record* p_record) {
	if (!p_record->p_package_) {
        CX_LOG_FMT(ERROR, ASSET, "Failed to load asset %x: asset is no not associated with a package\n",
			p_record->asset_.id_);
        return 0;
	}

	if (p_record->p_package_->s_filename_[0] == '\0') {
        CX_LOG_FMT(ERROR, ASSET, "Failed to load asset %x: package has no filename\n",
			p_record->asset_.id_);
        return 0;
	}

    FILE* p_file = fopen(p_record->p_package_->s_filename_, "rb");

    if (!p_file) {
        CX_LOG_FMT(ERROR, ASSET, "Failed to load asset %x: unable to open package file '%s'\n",
			p_record->asset_.id_, p_record->p_package_->s_filename_);
        return 0;
    }

    fseek(p_file, p_record->file_location_, SEEK_CUR);
    
    const struct asset_type_table* p_type_table = &asset_type_tables[CX_ASSET_GET_TYPE_ID(p_record->asset_.id_)];

    p_record->asset_.p_data_ = calloc(1, p_type_table->asset_size);

    const int b_result = p_type_table->f_deserialize(p_file, p_record->asset_.p_data_);

    fclose(p_file);

    return b_result;
}

void cx_asset_free(struct cx_asset_package_record* p_record) {
    if (!p_record->asset_.p_data_) {
        return;
	}
    
    const struct asset_type_table* p_type_table = &asset_type_tables[CX_ASSET_GET_TYPE_ID(p_record->asset_.id_)];
    p_type_table->f_free(p_record->asset_.p_data_);

    free(p_record->asset_.p_data_);
    p_record->asset_.p_data_ = 0;
}

void cx_asset_package_init(struct cx_asset_package* p_package) {
    *p_package = (struct cx_asset_package) {0};
    hashtable_init(&p_package->asset_type_record_tables_, sizeof(struct hashtable));
}

void cx_asset_package_free(struct cx_asset_package* p_package) {
    struct hashtable_itr asset_type_record_tables_itr;
    struct hashtable_itr asset_type_records_itr;

    hashtable_itr(&p_package->asset_type_record_tables_, &asset_type_record_tables_itr);
    while (hashtable_itr_is_valid(&asset_type_record_tables_itr)) {
        hashtable_itr(asset_type_record_tables_itr.p_value, &asset_type_records_itr);

        while(hashtable_itr_is_valid(&asset_type_records_itr)) {
            struct cx_asset_package_record* p_record = asset_type_records_itr.p_value;
            cx_asset_free(p_record);
            hashtable_itr_next(&asset_type_records_itr);
        }

        hashtable_free(asset_type_record_tables_itr.p_value);

        hashtable_itr_next(&asset_type_record_tables_itr);
    }

    hashtable_free(&p_package->asset_type_record_tables_);
}

int cx_asset_package_load_records(struct cx_asset_package* p_result, const char* s_filename) {
    FILE* p_file = fopen(s_filename, "rb");

    if (!p_file) {
       CX_LOG_FMT(ERROR, ASSET, "Failed to load asset package records: unable to open file '%s\n", s_filename);
        return 0;
    }

    strcpy(p_result->s_filename_, s_filename);

    uint32_t num_records = 0;
    deserialize_uint32(p_file, &num_records);
    
    if (num_records == 0) {
        return 0;
    }

    CX_LOG_FMT(INFO, ASSET, "Loading %d asset records from package file '%s'...\n",
		num_records, p_result->s_filename_);

    for (size_t i = 0; i < num_records; ++i) {
        cx_asset_id id;
        deserialize_uint32(p_file, &id);

		struct cx_asset_package_record* p_new_record;
		cx_asset_package_new_record_internal(p_result, id, &p_new_record);

        deserialize_uint32(p_file, &p_new_record->file_location_);
    }
    
    return (int)num_records;
}

void cx_asset_package_save(struct cx_asset_package* p_package) {
    FILE* p_file = fopen(p_package->s_filename_, "wb");
    
    if (!p_file) {
        CX_LOG_FMT(ERROR, ASSET, "Failed to save asset package: unable to open file '%s'\n",
			p_package->s_filename_);
        return;
    }

    serialize_uint32(p_file, p_package->asset_type_record_tables_.n_elements_);

    CX_LOG_FMT(TRACE, ASSET, "Saving assets to package file '%s'...\n", p_package->s_filename_);

    struct hashtable_itr asset_type_record_tables_itr;
    struct hashtable_itr asset_type_records_itr;
    
    hashtable_itr(&p_package->asset_type_record_tables_, &asset_type_record_tables_itr);
    while (hashtable_itr_is_valid(&asset_type_record_tables_itr)) {
		const struct hashtable* p_asset_type_record_table = asset_type_record_tables_itr.p_value;
		const uint8_t asset_type_id = *(const uint32_t*)asset_type_record_tables_itr.p_key;

		CX_LOG_FMT(TRACE, ASSET, "Saving %d %s assets...\n",
			asset_type_tables[asset_type_id].s_display_name,
			p_asset_type_record_table->n_elements_);

        hashtable_itr(p_asset_type_record_table, &asset_type_records_itr);
        while(hashtable_itr_is_valid(&asset_type_records_itr)) {
            struct cx_asset_package_record* p_record = asset_type_records_itr.p_value;

            serialize_uint32(p_file, p_record->asset_.id_);

            serialize_uint32(p_file, 0);
            
            hashtable_itr_next(&asset_type_records_itr);
        }

        hashtable_itr_next(&asset_type_record_tables_itr);
    }
    
    const size_t asset_record_size = sizeof(cx_asset_id) + sizeof(uint32_t); // ID + FILE_LOCATION

    uint32_t asset_data_file_location = ftell(p_file);
    int i = 0;
    
    hashtable_itr(&p_package->asset_type_record_tables_, &asset_type_record_tables_itr);
    while (hashtable_itr_is_valid(&asset_type_record_tables_itr)) {
		const struct hashtable* p_asset_type_record_table = asset_type_record_tables_itr.p_value;
		const uint8_t asset_type_id = *(const uint32_t*)asset_type_record_tables_itr.p_key;
		const struct asset_type_table* p_asset_type_table = &asset_type_tables[asset_type_id];

        hashtable_itr(p_asset_type_record_table, &asset_type_records_itr);
        while(hashtable_itr_is_valid(&asset_type_records_itr)) {
            struct cx_asset_package_record* p_record = asset_type_records_itr.p_value;

            p_record->file_location_ = asset_data_file_location;
            
            // DATA
            const int b_result = p_asset_type_table->f_serialize(p_file, p_record->asset_.p_data_);
            CX_LOG_FMT(TRACE, ASSET, "  Asset saved %x...\n", p_record->asset_.id_);
            
            if (!b_result) {
                // todo: handle serialization error
                CX_LOG_FMT(ERROR, ASSET, "Asset serialization error: asset_id=%x\n", p_record->asset_.id_);
            }

            // Cache the next record's data location
            asset_data_file_location = ftell(p_file);
    		
			// write the asset's records' file location now that we know it
            fseek(p_file, sizeof(uint32_t) + (asset_record_size * i) + sizeof(cx_asset_id), SEEK_SET);
            serialize_uint32(p_file, p_record->file_location_);
            fseek(p_file, asset_data_file_location, SEEK_SET);
                
            hashtable_itr_next(&asset_type_records_itr);
            ++i;
        }

        hashtable_itr_next(&asset_type_record_tables_itr);
    }

    fclose(p_file);
}

void cx_asset_package_save_as(struct cx_asset_package* p_package, const char* s_filename) {
    strcpy(p_package->s_filename_, s_filename);
    cx_asset_package_save(p_package);
}

int cx_asset_package_find_record(
	const struct cx_asset_package* p_package,
	cx_asset_id id,
	struct cx_asset_package_record** pp_out) {

    const uint8_t asset_type = CX_ASSET_GET_TYPE_ID(id);

	struct hashtable_itr itr;
    
	if (!hashtable_i_find(&p_package->asset_type_record_tables_, (uint32_t)asset_type, &itr)) {
		return 0;
	}

	struct hashtable* p_records = itr.p_value;
	hashtable_i_find(p_records, id, &itr);
	*pp_out = itr.p_value;
	return 1;
}

void cx_asset_package_new_record(
	struct cx_asset_package* p_package,
	uint8_t type,
	struct cx_asset_package_record** pp_out) {

    const cx_asset_id new_asset_id = CX_ASSET_MAKE_ID(type, cx_asset_generate_random_idn());
	cx_asset_package_new_record_internal(p_package, new_asset_id, pp_out);
}

void cx_asset_package_delete_record(struct cx_asset_package* p_package, cx_asset_id id) {
    const uint8_t asset_type = CX_ASSET_GET_TYPE_ID(id);

	struct hashtable_itr itr;

    if (!hashtable_i_find(&p_package->asset_type_record_tables_, (uint32_t)asset_type, &itr)) {
        return;
    }

	struct hashtable* p_records = itr.p_value;
    hashtable_i_remove(p_records, id);
}

void cx_asset_directory_register_package(const struct cx_asset_package* p_package) {
    ++directory.n_packages;
    directory.pp_packages = realloc(directory.pp_packages, directory.n_packages * sizeof(*directory.pp_packages));
    directory.pp_packages[directory.n_packages - 1] = p_package;
}

int cx_asset_directory_find(cx_asset_id id, struct cx_asset_package_record** pp_out) {
	struct hashtable_itr itr;

    const uint8_t asset_type = CX_ASSET_GET_TYPE_ID(id);

    for (size_t i = directory.n_packages; i-- > 0;) {
        const struct cx_asset_package* p_package = directory.pp_packages[i];

        if (!hashtable_i_find(&p_package->asset_type_record_tables_, (uint32_t)asset_type, &itr)) {
            continue;
        }
        
		const struct hashtable* p_asset_records = itr.p_value;

        if (!hashtable_find(p_asset_records, &id, sizeof(id), &itr)) {
            continue;
        }

		*pp_out = itr.p_value;
		return 1;
    }

    return 0;
}

const struct cx_asset_package** cx_asset_directory_get_packages(size_t* p_num_packages) {
    *p_num_packages = directory.n_packages;
    return directory.pp_packages;
}

void cx_asset_serialize_handle(FILE* p_file, const cx_asset_handle p_asset_handle) {
    serialize_uint32(p_file, p_asset_handle ? p_asset_handle->asset_.id_ : 0);
}

void cx_asset_deserialize_handle(FILE* p_file, struct cx_asset_package_record** pp_result) {
    cx_asset_id id;
    deserialize_uint32(p_file, &id);
    if (cx_asset_directory_find(id, pp_result)) {
        CX_LOG_FMT(ERROR, ASSET, "Failed to deserialize asset handle: asset %x not found in asset directory\n", id);
    }
}

void cx_asset_package_new_record_internal(
	struct cx_asset_package* p_package,
	cx_asset_id id,
	struct cx_asset_package_record** pp_out) {

	const uint8_t type = CX_ASSET_GET_TYPE_ID(id);

	struct hashtable_itr itr;

	struct hashtable* p_records;

    if (hashtable_find(&p_package->asset_type_record_tables_, &type, sizeof(type), &itr)) {
		p_records = itr.p_value;
    } else {
        p_records = hashtable_add(&p_package->asset_type_record_tables_, &type, sizeof(type));
        hashtable_init(p_records, sizeof(struct cx_asset_package_record));
	}
    
    *pp_out = hashtable_add(p_records, &id, sizeof(id));
    **pp_out = (struct cx_asset_package_record) {
        .asset_ = { .id_ = id },
        .p_package_ = p_package
    };
    
    CX_LOG_FMT(INFO, ASSET, "New asset created: (%s) %x\n",
		asset_type_tables[type].s_display_name, (*pp_out)->asset_.id_);
}

uint32_t cx_asset_generate_random_idn(void) {
    uint32_t result = (uint32_t)(((uint16_t)rand() << 17) + ((uint16_t)rand() << 2) + ((uint16_t)rand()>>13));
    return result % CX_ASSET_IDN_MAX;
}
