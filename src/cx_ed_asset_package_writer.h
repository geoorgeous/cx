#ifndef CX_ED_ASSET_PACKAGE_WRITER_H
#define CX_ED_ASSET_PACKAGE_WRITER_H

struct cx_asset_package;
struct cx_stream;

int cx_asset_package_serialize(const struct cx_asset_package* p_package, struct cx_stream* p_stream);

void cx_asset_package_save_to_file(struct cx_asset_package* p_package);

void cx_asset_package_save_to_file_as(struct cx_asset_package* p_package, const char* s_filename);

void cx_asset_package_new_record(
	struct cx_asset_package* p_package,
	uint8_t type,
	struct cx_asset_package_record** pp_out);

void cx_asset_package_delete_record(struct cx_asset_package* p_package, cx_asset_id id);

#endif
