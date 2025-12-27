
#include "gl.h"
#include <GL/glx.h>

#include "logging.h"
#include "platform_window_nix_x11.h"
#include "platform_window.h"

#define CX_LOG_CAT_OPENGL "opengl"

struct gl_context_nix_x11_internals {
    const struct platform_window* p_window;
    XVisualInfo*                  p_visualinfo;
    GLXContext                    context;
};

enum error gl_context_create(int gl_version_major, int gl_version_minor, const struct platform_window* p_window, struct gl_context* p_out_context) {
    struct gl_context_nix_x11_internals* p_context_internals = (void*)p_out_context->_bytes;
    const struct platform_window_nix_x11_internals* p_window_internals = (const void*)p_window->_bytes;

    GLint glx_version_major = 0;
    GLint glx_version_minor = 0;

    glXQueryVersion(p_window_internals->p_display, &glx_version_major, &glx_version_minor);
    if (glx_version_major <= gl_version_major && glx_version_minor < gl_version_minor) {
        cx_log_fmt(CX_LOG_ERROR, CX_LOG_CAT_OPENGL, "GLX %d.%d or greater is required\n", gl_version_major, gl_version_minor);
        return 1;
    }

    GLint glx_attribs[] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        GLX_DEPTH_SIZE,     24,
        GLX_STENCIL_SIZE,   8,
        GLX_RED_SIZE,       8,
        GLX_GREEN_SIZE,     8,
        GLX_BLUE_SIZE,      8,
        GLX_SAMPLE_BUFFERS, 0,
        GLX_SAMPLES,        0,
        None
    };
    p_context_internals->p_visualinfo = glXChooseVisual(p_window_internals->p_display, DefaultScreen(p_window_internals->p_display), glx_attribs);

    if (!p_context_internals->p_visualinfo) {
        return 1;
    }

    p_context_internals->context = glXCreateContext(p_window_internals->p_display, p_context_internals->p_visualinfo, NULL, GL_TRUE);

    glXMakeCurrent(p_window_internals->p_display, p_window_internals->window, p_context_internals->context);

    p_context_internals->p_window = p_window;

	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	cx_log_fmt(CX_LOG_INFO, CX_LOG_CAT_OPENGL, "OpenGL %scontext created (v%s, GLSL v%s)\n", context_flags & 0x2 ? "Debug " : "", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
	cx_log_fmt(CX_LOG_INFO, CX_LOG_CAT_OPENGL, "Graphics platform: %s, %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));

    return ERROR_OK;
}

void gl_context_destroy(struct gl_context* p_context) {
    const struct gl_context_nix_x11_internals* p_context_internals = (const void*)p_context->_bytes;
    const struct platform_window_nix_x11_internals* p_window_internals = (const void*)p_context_internals->p_window->_bytes;
    glXDestroyContext(p_window_internals->p_display, p_context_internals->context);
}

enum error gl_context_make_current(const struct gl_context* p_context) {
    const struct gl_context_nix_x11_internals* p_context_internals = (const void*)p_context->_bytes;
    const struct platform_window_nix_x11_internals* p_window_internals = (const void*)p_context_internals->p_window->_bytes;
    glXMakeCurrent(p_window_internals->p_display, p_window_internals->window, p_context_internals->context);
    return ERROR_OK;
}

enum error gl_context_swap_buffers(const struct gl_context* p_context) {
    const struct gl_context_nix_x11_internals* p_context_internals = (const void*)p_context->_bytes;
    const struct platform_window_nix_x11_internals* p_window_internals = (const void*)p_context_internals->p_window->_bytes;
    glXSwapBuffers(p_window_internals->p_display, p_window_internals->window);
    return ERROR_OK;
}