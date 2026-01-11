#include "gl.h"
#include <GL/glx.h>

#include "cx_gfx_context.h"
#include "logging.h"
#include "platform_window.h"
#include "platform_window.nix_x11.h"

typedef GLXContext glXCreateContextAttribsARB_fn(
    Display *dpy, GLXFBConfig config,
	GLXContext share_context, Bool direct,
    const int *attrib_list);
glXCreateContextAttribsARB_fn* glXCreateContextAttribsARB;

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_FLAGS_ARB                     0x2094

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x0001
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002

#ifndef NDEBUG

typedef void (*glDebugProcARB_fn)(GLenum, GLenum, GLuint, GLenum, GLsizei, const char*, const void*);
typedef void glDebugMessageCallbackARB_fn(glDebugProcARB_fn, const void*);
glDebugMessageCallbackARB_fn* glDebugMessageCallbackARB;

typedef void glDebugMessageControlARB_fn(GLenum, GLenum, GLenum, GLsizei, const GLuint*, GLboolean);
glDebugMessageControlARB_fn* glDebugMessageControlARB;

#define GL_DEBUG_SOURCE_API_ARB                   0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB         0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER_ARB       0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY_ARB           0x8249
#define GL_DEBUG_SOURCE_APPLICATION_ARB           0x824A
#define GL_DEBUG_SOURCE_OTHER_ARB                 0x824B

#define GL_DEBUG_TYPE_ERROR_ARB                   0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB     0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB      0x824E
#define GL_DEBUG_TYPE_PORTABILITY_ARB             0x824F
#define GL_DEBUG_TYPE_PERFORMANCE_ARB             0x8250
#define GL_DEBUG_TYPE_OTHER_ARB                   0x8251

#define GL_DEBUG_SEVERITY_HIGH_ARB                0x9146
#define GL_DEBUG_SEVERITY_MEDIUM_ARB              0x9147
#define GL_DEBUG_SEVERITY_LOW_ARB                 0x9148
#define GL_DEBUG_SEVERITY_NOTIFICATION            0x826B

#define GL_DEBUG_OUTPUT                           0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB           0x8242

static void gl_debug_message_callback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const char* s_message,
	const void* p_user_param);

#endif

struct gl_context_nix_x11_internals {
    const struct platform_window* p_window;
    XVisualInfo*                  p_visualinfo;
    GLXContext                    context;
};

enum error gl_context_create(
	int gl_version_major,
	int gl_version_minor,
	const struct platform_window* p_window,
	struct gl_context* p_out_context) {

    struct gl_context_nix_x11_internals* p_context_internals = (void*)p_out_context->_bytes;
    const struct platform_window_nix_x11_internals* p_window_internals = (const void*)p_window->_bytes;

    GLint glx_version_major = 0;
    GLint glx_version_minor = 0;

    glXQueryVersion(p_window_internals->p_display, &glx_version_major, &glx_version_minor);
    if (glx_version_major <= gl_version_major && glx_version_minor < gl_version_minor) {
        cx_log_fmt(
			CX_LOG_ERROR,
			CX_LOG_CAT_GFX_CORE,
			"GLX %d.%d or greater is required\n",
			gl_version_major, gl_version_minor);
        return 1;
    }

#ifndef NDEBUG
	glDebugMessageCallbackARB =
		(glDebugMessageCallbackARB_fn*)glXGetProcAddress((const GLubyte*)"glDebugMessageCallbackARB");
	glDebugMessageControlARB =
		(glDebugMessageControlARB_fn*)glXGetProcAddress((const GLubyte*)"glDebugMessageControlARB");
#endif
	glXCreateContextAttribsARB =
		(glXCreateContextAttribsARB_fn*)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

	// todo: create debug context via glXCreateContextAttribsARB

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
    p_context_internals->p_visualinfo = glXChooseVisual(
		p_window_internals->p_display,
		DefaultScreen(p_window_internals->p_display),
		glx_attribs);
	
    if (!p_context_internals->p_visualinfo) {
        return 1;
    }

    p_context_internals->context = glXCreateContext(
		p_window_internals->p_display,
		p_context_internals->p_visualinfo,
		NULL,
		GL_TRUE);

    glXMakeCurrent(p_window_internals->p_display, p_window_internals->window, p_context_internals->context);

#ifndef NDEBUG
	if (glDebugMessageCallbackARB) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB, 0, NULL, GL_FALSE);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
		glDebugMessageCallbackARB(gl_debug_message_callback, NULL);
	}
#endif

    p_context_internals->p_window = p_window;

	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	cx_log_fmt(
		CX_LOG_INFO,
		CX_LOG_CAT_GFX_CORE,
		"OpenGL %scontext created (v%s, GLSL v%s)\n",
		context_flags & 0x2 ? "Debug " : "", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
	cx_log_fmt(
		CX_LOG_INFO,
		CX_LOG_CAT_GFX_CORE,
		"Graphics platform: %s, %s\n",
		glGetString(GL_VENDOR), glGetString(GL_RENDERER));

    return ERROR_OK;
}

void gl_context_destroy(struct gl_context* p_context) {
    const struct gl_context_nix_x11_internals* p_context_internals = (const void*)p_context->_bytes;
    const struct platform_window_nix_x11_internals* p_window_internals =
		(const void*)p_context_internals->p_window->_bytes;
    glXDestroyContext(p_window_internals->p_display, p_context_internals->context);
}

enum error gl_context_make_current(const struct gl_context* p_context) {
    const struct gl_context_nix_x11_internals* p_context_internals = (const void*)p_context->_bytes;
    const struct platform_window_nix_x11_internals* p_window_internals =
		(const void*)p_context_internals->p_window->_bytes;
    glXMakeCurrent(p_window_internals->p_display, p_window_internals->window, p_context_internals->context);
    return ERROR_OK;
}

enum error gl_context_swap_buffers(const struct gl_context* p_context) {
    const struct gl_context_nix_x11_internals* p_context_internals = (const void*)p_context->_bytes;
    const struct platform_window_nix_x11_internals* p_window_internals =
		(const void*)p_context_internals->p_window->_bytes;
    glXSwapBuffers(p_window_internals->p_display, p_window_internals->window);
    return ERROR_OK;
}

#ifndef NDEBUG
void gl_debug_message_callback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const char* s_message,
	const void* p_user_ptr) {
	
	(void)length;
	(void)p_user_ptr;

    const char* s_source = 0;
    const char* s_type = 0;

    switch(source) {
		case GL_DEBUG_SOURCE_API_ARB:             s_source = "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:   s_source = "Window system"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: s_source = "Shader compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:     s_source = "Third party"; break;
		case GL_DEBUG_SOURCE_APPLICATION_ARB:     s_source = "Application"; break;
		case GL_DEBUG_SOURCE_OTHER_ARB:           s_source = "Other"; break;
		default:                                  s_source = "???"; break;
	};

    switch (type) {
		case GL_DEBUG_TYPE_ERROR_ARB:               s_type = "Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: s_type = "Deprecated behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  s_type = "Undedfined behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB:         s_type = "Port"; break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB:         s_type = "Perf"; break;
		case GL_DEBUG_TYPE_OTHER_ARB:               s_type = "Other"; break;
		default:                                    s_type = "???"; break;
	}

	int log_level = CX_LOG_TRACE;

	if (severity == GL_DEBUG_SEVERITY_LOW_ARB) {
		log_level = CX_LOG_INFO;
	} else if (severity == GL_DEBUG_SEVERITY_MEDIUM_ARB || severity == GL_DEBUG_SEVERITY_HIGH_ARB) {
		log_level = CX_LOG_WARNING;
	}
	
	if (type == GL_DEBUG_TYPE_ERROR_ARB) {
		log_level = CX_LOG_ERROR;
	}

	cx_log_fmt(
		log_level,
		CX_LOG_CAT_GFX_CORE,
		"Message: { id=%u, source='%s', type='%s' } %s\n",
		id, s_source, s_type, s_message);
}
#endif
