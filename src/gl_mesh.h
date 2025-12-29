#ifndef _H__GL_MESH
#define _H__GL_MESH

#include "gl.h"

#define GL_MESH_MAX_VBOS 8

struct mesh_primitive;

struct gl_mesh {
    GLuint  _gl_vao;
    GLuint  _gl_vbos[GL_MESH_MAX_VBOS];
    size_t  _gl_vbos_len;
    GLuint  _gl_ibo;
    GLenum  _gl_ibo_type;
    GLenum  _gl_draw_mode;
    GLsizei _num_elements;
    GLfloat _bounds_min[3];
    GLfloat _bounds_max[3];
};

void gl_mesh_create(struct gl_mesh* p_gl_mesh, const struct mesh_primitive* p_mesh_primitive);
void gl_mesh_destroy(struct gl_mesh* p_gl_mesh);
void gl_mesh_draw(const struct gl_mesh* p_gl_mesh);

#endif