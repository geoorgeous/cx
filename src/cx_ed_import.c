#include <stdint.h>
#include <string.h>

#include "cx_ed_asset_library.h"
#include "cx_ed_import.h"
#include "cx_ed_import_bdf.h"
#include "cx_ed_import_gltf.h"
#include "cx_ed_import_image.h"
#include "cx_io.h"
#include "cx_logging.h"
#include "cx_macro.h"
#include "cx_str.h"
#include "cx_stream_serialization.h"
#include "cx_stream_file.h"

#define CX_ED_IMPORT_FILE_SIG_JPG           (uint8_t[]){ 0xFF, 0xD8, 0xFF }
#define CX_ED_IMPORT_FILE_SIG_PNG           (uint8_t[]){ '%', 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A }
#define CX_ED_IMPORT_FILE_SIG_BMP           (uint8_t[]){ 'B', 'M' }
#define CX_ED_IMPORT_FILE_SIG_GLB           (uint8_t[]){ 'g', 'l', 'T', 'F' }
#define CX_ED_IMPORT_FILE_SIG_ASSET         (uint8_t[]){ 'C', 'X', '.', 'A', 'S', 'S', 'E', 'T' }
#define CX_ED_IMPORT_FILE_SIG_ASSET_PACKAGE (uint8_t[]){ 'C', 'X', '.', '.', '.', 'P', 'K', 'G' }

#define CX_ED_IMPORT_FILE_SIG_EQ(A, B) (memcmp(A, B, sizeof(B)) == 0)

int cx_ed_import_file(const char* s_filepath, struct cx_asset_ref* p_out_asset_ref) {
	struct cx_stream_file stream;
	if (!cx_stream_file_open(s_filepath, "rb", &stream)) {
		return CX_FALSE;
	}

	// First we try to infer import route from file signatures/magic numbers

	uint8_t header[8];
	cx_stream_deserialize_bytes(&stream.base, sizeof(header), header);

	cx_stream_close(&stream.base);

	if (CX_ED_IMPORT_FILE_SIG_EQ(header, CX_ED_IMPORT_FILE_SIG_JPG) ||
		CX_ED_IMPORT_FILE_SIG_EQ(header, CX_ED_IMPORT_FILE_SIG_PNG) ||
		CX_ED_IMPORT_FILE_SIG_EQ(header, CX_ED_IMPORT_FILE_SIG_BMP)) {

		return cx_ed_import_image_file(s_filepath, p_out_asset_ref);
	}

	if (CX_ED_IMPORT_FILE_SIG_EQ(header, CX_ED_IMPORT_FILE_SIG_GLB)) {
		return cx_ed_import_gltf_file(s_filepath, p_out_asset_ref);
	}

	if (CX_ED_IMPORT_FILE_SIG_EQ(header, CX_ED_IMPORT_FILE_SIG_ASSET)) {
		return cx_ed_asset_library_add_file(s_filepath, p_out_asset_ref);
	}

	if (CX_ED_IMPORT_FILE_SIG_EQ(header, CX_ED_IMPORT_FILE_SIG_ASSET_PACKAGE)) {
		// todo: import asset package
	}

	// If we don't recognise a known file signature, we try to infer import route from file extension

	const char* p_ext;
	size_t ext_len;
	
	if (cx_io_filepath_ext(s_filepath, &p_ext, &ext_len)) {
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

		if (cx_str_icmp(p_ext, "cxasset") == 0) {
			return cx_ed_asset_library_add_file(s_filepath, p_out_asset_ref);
		}

		if (cx_str_icmp(p_ext, "cxpkg") == 0) {
			// todo: import asset package
		}
	}

	CX_LOG_FMT(ERROR, IMPORT, "Failed to import file '%s': Unsupported file type\n", s_filepath);

	return CX_FALSE;
}
