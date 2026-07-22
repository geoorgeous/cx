#include "cx_asset_package.h"

static void cx_asset_package_new_record_internal(
	struct cx_asset_package* p_package,
	cx_asset_id id,
	struct cx_asset_package_record** pp_out);

void cx_asset_package_free(struct cx_asset_package* p_package) {
	struct hashtable_itr itr;

	for (uint8_t i = 0; i < CX_ASSET_TYPE_ID_MAX; ++i) {
		if (p_package->asset_type_record_tables_[i].n_elements_ == 0) {
			continue;
		}

		CX_LOG_FMT(TRACE, ASSET, "Freeing %d assets from asset package %s table...\n",
			p_package->asset_type_record_tables_[i].n_elements_,
			asset_type_tables[i].s_display_name);

		hashtable_itr(&p_package->asset_type_record_tables_[i], &itr);
		while(hashtable_itr_is_valid(&itr)) {
			struct cx_asset_package_record* p_record = itr.p_value;
			cx_asset_free(p_record);
			hashtable_itr_next(&itr);
		}

		hashtable_free(&p_package->asset_type_record_tables_[i]);
	}
}

int cx_asset_package_serialize(const struct cx_asset_package* p_package, struct cx_stream* p_stream) {
	struct hashtable_itr itr;
	
	for (uint8_t i = 0; i < CX_ASSET_TYPE_ID_MAX; ++i) {
		if (p_package->asset_type_record_tables_->n_elements_ == 0) {
			continue;
		}

		cx_stream_serialize_uint8(p_stream, i);

		CX_LOG_FMT(TRACE, ASSET, "Saving %d %s assets...\n",
			asset_type_tables[i].s_display_name,
			p_package->asset_type_record_tables_[i].n_elements_);

		hashtable_itr(&p_package->asset_type_record_tables_[i], &itr);
		while(hashtable_itr_is_valid(&itr)) {
			struct cx_asset_package_record* p_record = itr.p_value;

			cx_stream_serialize_uint32(p_stream, p_record->asset_.id_);

			cx_stream_serialize_uint32(p_stream, 0);
			
			hashtable_itr_next(&itr);
		}
	}
	
	const size_t asset_record_size = sizeof(cx_asset_id) + sizeof(uint32_t); // ID + FILE_LOCATION

	size_t asset_data_location = cx_stream_tell(p_stream);

	uint32_t counter = 0;
	
	for (uint8_t i = 0; i < CX_ASSET_TYPE_ID_MAX; ++i) {
		if (p_package->asset_type_record_tables_[i].n_elements_ == 0) {
			continue;
		}

		hashtable_itr(&p_package->asset_type_record_tables_[i], &itr);
		while(hashtable_itr_is_valid(&itr)) {
			struct cx_asset_package_record* p_record = itr.p_value;

			p_record->file_location_ = (uint32_t)asset_data_location;
			
			// DATA
			CX_LOG_FMT(TRACE, ASSET, "  Saving asset %x...\n", p_record->asset_.id_);
			const int b_result = asset_type_tables[i].f_serialize(p_record->asset_.p_data_, p_stream);
			
			if (!b_result) {
				// todo: handle serialization error
				CX_LOG_FMT(ERROR, ASSET, "Asset serialization error: asset_id=%x\n", p_record->asset_.id_);
			}

			// Cache the next record's data location
			asset_data_location = cx_stream_tell(p_stream);
			
			// write the asset's records' file location now that we know it
			cx_stream_seek(
				p_stream,
				(long)sizeof(uint32_t) + (long)(asset_record_size * counter) + (long)sizeof(cx_asset_id),
				CX_STREAM_SEEK_ORIGIN_begin);

			cx_stream_serialize_uint32(p_stream, p_record->file_location_);

			cx_stream_seek(p_stream, (ptrdiff_t)asset_data_location, CX_STREAM_SEEK_ORIGIN_begin);
				
			hashtable_itr_next(&itr);

			++counter;
		}
	}

	return CX_TRUE;
}

int cx_asset_package_deserialize_records(struct cx_asset_package* p_package, struct cx_stream* p_stream) {
	uint32_t num_records = 0;
	cx_stream_deserialize_uint32(p_stream, &num_records);
	
	CX_LOG_FMT(INFO, ASSET, "Deserializing %u asset records from asset package stream...\n", num_records);

	if (num_records == 0) {
		return CX_TRUE;
	}

	for (size_t i = 0; i < num_records; ++i) {
		cx_asset_id id;
		cx_stream_deserialize_uint32(p_stream, &id);

		struct cx_asset_package_record* p_new_record;
		cx_asset_package_new_record_internal(p_package, id, &p_new_record);

		cx_stream_deserialize_uint32(p_stream, &p_new_record->file_location_);
	}

	return CX_TRUE;
}

void cx_asset_package_load_records_from_file(struct cx_asset_package* p_package, const char* s_filename) {
	struct cx_stream_file stream;
	
	if (!cx_stream_file_open(s_filename, "rb", &stream)) {
		return;
	}

	strcpy(p_package->s_filename_, s_filename);

	CX_LOG_FMT(INFO, ASSET, "Reading asset package records from file '%s'...\n", p_package->s_filename_);

	cx_asset_package_deserialize_records(p_package, &stream.base);

	cx_stream_file_close(&stream);
}

void cx_asset_package_save_to_file(struct cx_asset_package* p_package) {
	struct cx_stream_file stream;
	if (!cx_stream_file_open(p_package->s_filename_, "wb", &stream)) {
		return;
	}

	CX_LOG_FMT(TRACE, ASSET, "Saving assets to package file '%s'...\n", p_package->s_filename_);

	cx_asset_package_serialize(p_package, &stream.base);

	cx_stream_file_close(&stream);
}

void cx_asset_package_save_to_file_as(struct cx_asset_package* p_package, const char* s_filename) {
	strcpy(p_package->s_filename_, s_filename);
	cx_asset_package_save_to_file(p_package);
}

int cx_asset_package_find_record(
	const struct cx_asset_package* p_package,
	cx_asset_id id,
	struct cx_asset_package_record** pp_out) {

	const uint8_t asset_type = CX_ASSET_GET_TYPE_ID(id);

	struct hashtable_itr itr;
	
	if (!hashtable_i_find(&p_package->asset_type_record_tables_[asset_type], (uint32_t)asset_type, &itr)) {
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
	hashtable_i_remove(&p_package->asset_type_record_tables_[asset_type], id);
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

		if (p_package->asset_type_record_tables_[asset_type].n_elements_ == 0) {
			continue;
		}
		
		if (!hashtable_i_find(&p_package->asset_type_record_tables_[i], id, &itr)) {
			continue;
		}

		*pp_out = itr.p_value;
		return CX_TRUE;
	}

	return CX_FALSE;
}

const struct cx_asset_package** cx_asset_directory_get_packages(size_t* p_num_packages) {
	*p_num_packages = directory.n_packages;
	return directory.pp_packages;
}

void cx_asset_package_new_record_internal(
	struct cx_asset_package* p_package,
	cx_asset_id id,
	struct cx_asset_package_record** pp_out) {

	const uint8_t asset_type = CX_ASSET_GET_TYPE_ID(id);

	struct hashtable* p_asset_records = &p_package->asset_type_record_tables_[asset_type];

	if (p_asset_records->element_size_ == 0) {
		hashtable_init(p_asset_records, sizeof(struct cx_asset_package_record));
	}
	
	*pp_out = hashtable_i_add(p_asset_records, id);

	**pp_out = (struct cx_asset_package_record) {
		.asset_ = { .id_ = id },
		.p_package_ = p_package
	};
	
	CX_LOG_FMT(INFO, ASSET, "New asset created: (%s) %x\n", asset_type_tables[asset_type].s_display_name, id);
}

