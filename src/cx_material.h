#ifndef CX_MATERIAL_H
#define CX_MATERIAL_H

struct cx_render_pipeline;

struct cx_material {
	const struct cx_render_pipeline* p_render_pipeline;
	// todo: per-material draw command parameters
};

#endif
