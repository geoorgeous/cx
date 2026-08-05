#include "cx_ed_import.h"
#include "cx_ed_import_bdf.h"
#include "cx_ed_import_gltf.h"
#include "cx_ed_import_image.h"
#include "cx_io.h"
#include "cx_logging.h"
#include "cx_macro.h"
#include "cx_str.h"

int cx_ed_import_file(const char* s_filepath, struct cx_asset_ref* p_out_asset_ref) {

	const char* p_ext;
	size_t ext_len;
	
	if (!cx_io_filepath_ext(s_filepath, &p_ext, &ext_len)) {
		CX_LOG_FMT(ERROR, IMPORT, "Failed to import file '%s': Unsupported file type\n", s_filepath);
		// todo: determine file from magic number (PNG, JPEG, BMP, GLB, WAV)
		return CX_FALSE;
	}

	if (cx_str_icmp(p_ext, "jpeg") == 0 ||
		cx_str_icmp(p_ext, "jpg" ) == 0 ||
		cx_str_icmp(p_ext, "png" ) == 0 ||
		cx_str_icmp(p_ext, "bmp" ) == 0 ||
		cx_str_icmp(p_ext, "tga" ) == 0) {

		return cx_ed_import_image_file(s_filepath, p_out_asset_ref);
	}

	if (cx_str_icmp(p_ext, "gltf") == 0 ||
		cx_str_icmp(p_ext, "glb") == 0) {

		return cx_ed_import_gltf_file(s_filepath, p_out_asset_ref);
	}

	if (cx_str_icmp(p_ext, "bdf") == 0) {
		return cx_ed_import_bdf_file(s_filepath, p_out_asset_ref);
	}

	CX_LOG_FMT(ERROR, IMPORT, "Failed to import file '%s': Unsupported file type (ext=%s)\n",
		s_filepath, p_ext ? p_ext : "");

	return CX_FALSE;
}
