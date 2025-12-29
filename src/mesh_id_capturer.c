#include "gl_mesh.h"
#include "gl_program.h"
#include "input.h"
#include "mesh_id_capturer.h"

struct mesh_id_capturer_item {
    const struct gl_mesh* p_gl_mesh;
    const float*          p_transform;
    unsigned int          id;
};

static struct gl_program g_gl_program;

void mesh_id_capturer_init(struct mesh_id_capturer* p_mesh_id_capturer) {
    *p_mesh_id_capturer = (struct mesh_id_capturer){0};
}
 
void mesh_id_capturer_destroy(struct mesh_id_capturer* p_mesh_id_capturer) {
    glDeleteFramebuffers(1, &p_mesh_id_capturer->_gl_fb);
    glDeleteTextures(1, &p_mesh_id_capturer->_gl_fba_color);
    glDeleteTextures(1, &p_mesh_id_capturer->_gl_fba_depth_stencil);
}

void mesh_id_capturer_begin(struct mesh_id_capturer* p_mesh_id_capturer, size_t framebuffer_width, size_t framebuffer_height, const float* p_projection_matrix, const float* p_view_matrix) {
    if (g_gl_program.gl_handle == 0) {
        struct gl_shader gl_vertex_shader;
        struct gl_shader gl_fragment_shader;

        gl_shader_create(&gl_vertex_shader, GL_VERTEX_SHADER);
        gl_shader_compile(&gl_vertex_shader,
            "#version 330 core\n"
            "uniform mat4 u_projection_matrix;"
            "uniform mat4 u_view_matrix;"
            "uniform mat4 u_model_matrix;"
            "layout (location=0) in vec3 a_pos;"
            "void main() {"
                "gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix * vec4(a_pos, 1.0);"
            "}");

        gl_shader_create(&gl_fragment_shader, GL_FRAGMENT_SHADER);
        gl_shader_compile(&gl_fragment_shader,
            "#version 330 core\n"
            "uniform uint u_id;"
            "out uint f_color;"
            "void main() {"
                "f_color = u_id;"
            "}");

        gl_program_create(&g_gl_program);
        gl_program_attach_shader(&g_gl_program, &gl_vertex_shader);
        gl_program_attach_shader(&g_gl_program, &gl_fragment_shader);
        gl_program_link(&g_gl_program);

        gl_shader_destroy(&gl_vertex_shader);
        gl_shader_destroy(&gl_fragment_shader);
    }

    if (p_mesh_id_capturer->_fb_size[0] != framebuffer_width || p_mesh_id_capturer->_fb_size[1] != framebuffer_height) {
        glDeleteFramebuffers(1, &p_mesh_id_capturer->_gl_fb);
        glDeleteTextures(1, &p_mesh_id_capturer->_gl_fba_color);
        glDeleteTextures(1, &p_mesh_id_capturer->_gl_fba_depth_stencil);
        
        glGenTextures(1, &p_mesh_id_capturer->_gl_fba_color);
        glBindTexture(GL_TEXTURE_2D, p_mesh_id_capturer->_gl_fba_color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, framebuffer_width, framebuffer_height, 0,  GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenTextures(1, &p_mesh_id_capturer->_gl_fba_depth_stencil);
        glBindTexture(GL_TEXTURE_2D, p_mesh_id_capturer->_gl_fba_depth_stencil);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, framebuffer_width, framebuffer_height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

        glGenFramebuffers(1, &p_mesh_id_capturer->_gl_fb);
        glBindFramebuffer(GL_FRAMEBUFFER, p_mesh_id_capturer->_gl_fb);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_mesh_id_capturer->_gl_fba_color, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, p_mesh_id_capturer->_gl_fba_depth_stencil, 0);

        p_mesh_id_capturer->_fb_size[0] = framebuffer_width;
        p_mesh_id_capturer->_fb_size[1] = framebuffer_height;
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, p_mesh_id_capturer->_gl_fb);
    }
    
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST); 
    glViewport(0, 0, framebuffer_width, framebuffer_height);

    glUseProgram(g_gl_program.gl_handle);

    GLuint gl_uniform_location;
    
    gl_uniform_location = glGetUniformLocation(g_gl_program.gl_handle, "u_projection_matrix");
    glUniformMatrix4fv(gl_uniform_location, 1, GL_FALSE, p_projection_matrix);
    
    gl_uniform_location = glGetUniformLocation(g_gl_program.gl_handle, "u_view_matrix");
    glUniformMatrix4fv(gl_uniform_location, 1, GL_FALSE, p_view_matrix);
}

void mesh_id_capturer_submit(struct mesh_id_capturer* p_mesh_id_capturer, const struct gl_mesh* p_gl_mesh, const float* p_transform, unsigned int id) {
    GLuint gl_uniform_location;

    gl_uniform_location = glGetUniformLocation(g_gl_program.gl_handle, "u_model_matrix");
    glUniformMatrix4fv(gl_uniform_location, 1, GL_FALSE, p_transform);

    gl_uniform_location = glGetUniformLocation(g_gl_program.gl_handle, "u_id");
    glUniform1ui(gl_uniform_location, (GLuint)id);

    gl_mesh_draw(p_gl_mesh);
}

unsigned int mesh_id_capturer_query(const struct mesh_id_capturer* p_mesh_id_capturer, float x, float y) {
    if (x < 0 || y < 0 || x > 1 || y > 1) {
        return 0;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, p_mesh_id_capturer->_gl_fb);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    GLint pixel_x = (float)p_mesh_id_capturer->_fb_size[0] * x;
    GLint pixel_y = (float)p_mesh_id_capturer->_fb_size[1] * (1.0f - y);
    GLuint data;
    glReadPixels(pixel_x, pixel_y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &data);
    return data;
}