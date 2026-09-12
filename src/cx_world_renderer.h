#ifndef CX_WORLD_RENDERER_H
#define CX_WORLD_RENDERER_H

#define CX_OBJECT_ID_CATEGORY_ENTITY 1

struct cx_world;
struct cx_render_draw_command_buffer;

void cx_world_renderer_record_draw_commands_lit(
		const struct cx_world* p_world,
		struct cx_render_draw_command_buffer* p_render_draw_command_buffer);

void cx_world_renderer_record_draw_commands_object_id(
		const struct cx_world* p_world,
		struct cx_render_draw_command_buffer* p_render_draw_command_buffer);

#endif
