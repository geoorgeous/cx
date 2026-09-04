#ifndef CX_RENDER_DRAW_COMMAND_H
#define CX_RENDER_DRAW_COMMAND_H

struct cx_gfx_mesh;
struct cx_material;
struct cx_render_param_set;

struct cx_render_draw_command {
	const struct cx_material* p_material;
	const struct cx_gfx_mesh* p_mesh;
	const struct cx_render_param_set* p_param_set;
};

#endif
