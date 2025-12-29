#ifndef _H__MESH_ID_CAPTURER
#define _H__MESH_ID_CAPTURER

#include <stdint.h>

#include "gl.h"

struct gl_mesh;

struct mesh_id_capturer {
    size_t       _fb_size[2];
    GLuint       _gl_fb;
    GLuint       _gl_fba_color;
    GLuint       _gl_fba_depth_stencil;
};

void         mesh_id_capturer_init(struct mesh_id_capturer* p_mesh_id_capturer);
void         mesh_id_capturer_destroy(struct mesh_id_capturer* p_mesh_id_capturer);
void         mesh_id_capturer_begin(struct mesh_id_capturer* p_mesh_id_capturer, size_t framebuffer_width, size_t framebuffer_height, const float* p_projection_matrix, const float* p_view_matrix);
void         mesh_id_capturer_submit(struct mesh_id_capturer* p_mesh_id_capturer, const struct gl_mesh* p_gl_mesh, const float* p_transform, unsigned int id);
unsigned int mesh_id_capturer_query(const struct mesh_id_capturer* p_mesh_id_capturer, float x, float y);

#endif