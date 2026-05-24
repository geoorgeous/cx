#include "cx_asset.h"
#include "cx_ed_import_image.h"
#include "cx_image.h"
#include "cx_logging.h"
#include "stb_image.h"

static void cx_ed_import_image_from_stbi_retval(
	struct cx_asset_package* p_package,
	int x, int y,
	int comp,
	uint8_t* p_pixel_data,
	struct cx_asset_package_record** pp_out) {

    struct cx_image* p_image = malloc(sizeof(struct cx_image));

    p_image->width = x;
    p_image->height = y;
	p_image->p_pixel_data = p_pixel_data;
	p_image->pixel_data_format.pixel_type = CX_PIXEL_TYPE_u8;

	switch(comp) {
		case 1: p_image->pixel_data_format.pixel_format = CX_PIXEL_FORMAT_red; break;
		case 2: p_image->pixel_data_format.pixel_format = CX_PIXEL_FORMAT_rg; break;
		case 3: p_image->pixel_data_format.pixel_format = CX_PIXEL_FORMAT_rgb; break;
		case 4: p_image->pixel_data_format.pixel_format = CX_PIXEL_FORMAT_rgba; break;
	}
    
    cx_asset_package_new_record(p_package, ASSET_TYPE_IMAGE, pp_out);
    (*pp_out)->asset_.p_data_ = p_image;
}

int cx_ed_import_image(
	struct cx_asset_package* p_package,
	const uint8_t* p_bytes,
	size_t size,
	struct cx_asset_package_record** pp_out) {
    
	int x, y, comp;
    uint8_t* p_pixel_data = stbi_load_from_memory(p_bytes, size, &x, &y, &comp, 0);

    if (!p_pixel_data) {
        CX_LOG_FMT(ERROR, IMPORT_IMAGE, "Failed to import image from %d bytes\n", size);
        return 0;
    }

	cx_ed_import_image_from_stbi_retval(p_package, x, y, comp, p_pixel_data, pp_out);

	return 1;
}

int cx_ed_import_image_file(
	struct cx_asset_package* p_package,
	const char* s_filepath,
	struct cx_asset_package_record** pp_out) {

    int x, y, comp;
    uint8_t* p_pixel_data = stbi_load(s_filepath, &x, &y, &comp, 0);

    if (!p_pixel_data) {
        CX_LOG_FMT(ERROR, IMPORT_IMAGE, "Failed to import image from file '%s'\n", s_filepath);
        return 0;
	}

	cx_ed_import_image_from_stbi_retval(p_package, x, y, comp, p_pixel_data, pp_out);

	return 1;
}
