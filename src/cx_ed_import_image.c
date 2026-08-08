#include <string.h>

#include "cx_alloc.h"
#include "cx_asset.h"
#include "cx_ed_asset_library.h"
#include "cx_ed_import_image.h"
#include "cx_image.h"
#include "cx_io.h"
#include "cx_logging.h"
#include "stb_image.h"

static void cx_ed_import_image_from_stbi_retval(
	const char* s_name, int x, int y, int comp, uint8_t* p_pixel_data, struct cx_asset_ref* p_out);

int cx_ed_import_image(const char* s_name, const uint8_t* p_bytes, size_t size, struct cx_asset_ref* p_out) {
	int x, y, comp;
	uint8_t* p_pixel_data = stbi_load_from_memory(p_bytes, (int)size, &x, &y, &comp, 0);

	if (!p_pixel_data) {
		CX_LOG_FMT(ERROR, IMPORT_IMAGE, "Failed to import image from %d bytes\n", size);
		return 0;
	}

	cx_ed_import_image_from_stbi_retval(s_name, x, y, comp, p_pixel_data, p_out);

	return 1;
}

int cx_ed_import_image_file(const char* s_filepath, struct cx_asset_ref* p_out) {
	int x, y, comp;
	uint8_t* p_pixel_data = stbi_load(s_filepath, &x, &y, &comp, 0);

	if (!p_pixel_data) {
		CX_LOG_FMT(ERROR, IMPORT_IMAGE, "Failed to import image from file '%s'\n", s_filepath);
		return 0;
	}

	char asset_name_buf[CX_ASSET_NAME_MAX_LEN];
	size_t asset_name_len;
	cx_io_filepath_stem_cpy(s_filepath, asset_name_buf, &asset_name_len);

	cx_ed_import_image_from_stbi_retval(asset_name_buf, x, y, comp, p_pixel_data, p_out);

	char asset_filepath_buf[CX_ASSET_NAME_MAX_LEN];
	size_t dir_len;
	cx_io_filepath_dir(s_filepath, &dir_len);

	strncpy(asset_name_buf, s_filepath, dir_len);
	asset_name_buf[dir_len] = '/';
	strncpy(asset_name_buf + dir_len + 1, asset_name_buf, asset_name_len);

	cx_ed_asset_library_set_filepath(p_out->asset_id, asset_filepath_buf);

	return 1;
}

void cx_ed_import_image_from_stbi_retval(
	const char* s_name, int x, int y, int comp, uint8_t* p_pixel_data, struct cx_asset_ref* p_out) {

	struct cx_image* p_image = CX_MALLOC(cx_asset_type_size(CX_ASSET_TYPE_IMAGE));

	p_image->width = (uint32_t)x;
	p_image->height = (uint32_t)y;
	p_image->p_pixel_data = p_pixel_data;
	p_image->pixel_data_format.pixel_type = CX_PIXEL_TYPE_u8;

	switch(comp) {
		case 1: p_image->pixel_data_format.pixel_format = CX_PIXEL_FORMAT_red; break;
		case 2: p_image->pixel_data_format.pixel_format = CX_PIXEL_FORMAT_rg; break;
		case 3: p_image->pixel_data_format.pixel_format = CX_PIXEL_FORMAT_rgb; break;
		case 4: p_image->pixel_data_format.pixel_format = CX_PIXEL_FORMAT_rgba; break;
	}
	
	cx_ed_asset_library_new(CX_ASSET_TYPE_IMAGE, s_name, p_image, p_out);
}
