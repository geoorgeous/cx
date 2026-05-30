#include "cx_alloc.h"
#include "cx_image.h"

void cx_asset_free_image(void* p) {
	CX_FREE(((struct cx_image*)p)->p_pixel_data);
}
