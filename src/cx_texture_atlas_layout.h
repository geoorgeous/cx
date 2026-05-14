#ifndef CX_TEXTURE_ATLAS_LAYOUT
#define CX_TEXTURE_ATLAS_LAYOUT

#include <stddef.h>
#include <stdint.h>

struct cx_texture_atlas_entry {
	float u0;
	float u1;
	float v0;
	float v1;
};

struct cx_texture_atlas_layout {
	struct cx_texture_atlas_entry* p_entries;
	size_t                         num_entries;
};

#endif
