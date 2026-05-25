#ifndef CX_WORLD_RENDERER_H
#define CX_WORLD_RENDERER_H

struct cx_world;

void cx_world_renderer_draw(struct cx_world* p_world, const float* p_projection_matrix, const float* p_view_matrix);

#endif
