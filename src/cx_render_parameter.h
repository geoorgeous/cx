#ifndef CX_RENDER_PARAMETER_H
#define CX_RENDER_PARAMETER_H

#include <stdint.h>

#include "cx_gfx_program.h"

// how to reference the "same" parameter across multiple shaders?
// e.g camera matrix
// because a render pass parameter set might have to be applied across multiple materials
//
// also... we want to maybe have an object have the same "material" but different texture or properties...
// maybe rethink the idea of material?

struct cx_render_param {
	const char* s_name;
	enum cx_gfx_program_param_type type;
	void* p_data;
};

struct cx_render_param_set {
	struct cx_render_param* p_params;
	uint16_t num_params;
};

#endif
