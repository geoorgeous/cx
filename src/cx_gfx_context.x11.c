#include "gl.h"
#include <GL/glx.h>

#include <GL/glxext.h>
#include <string.h>
#include <X11/Xlib.h>

#include "cx_gfx_context.h"
#include "cx_gfx_framebuffer.h"
#include "cx_logging.h"
#include "cx_error.h"
#include "platform_window.h"
#include "platform_window.x11.h"

#define GLX_MIN_VERSION_MAJOR 1
#define GLX_MIN_VERSION_MINOR 2

typedef GLXContext glXCreateContextAttribsARB_fn(
    Display *dpy, GLXFBConfig config,
	GLXContext share_context, Bool direct,
    const int *attrib_list);
glXCreateContextAttribsARB_fn* f_glXCreateContextAttribsARB;

#define GLX_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB             0x2092
#define GLX_CONTEXT_PROFILE_MASK_ARB              0x9126
#define GLX_CONTEXT_FLAGS_ARB                     0x2094

#ifndef GLX_CONTEXT_CORE_PROFILE_BIT_ARB
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB          0x0001
#endif
#ifndef GLX_CONTEXT_DEBUG_BIT_ARB
#define GLX_CONTEXT_DEBUG_BIT_ARB                 0x0001
#endif
#ifndef GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002

#endif
#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT             0x00000002
#endif

typedef void glXSwapIntervalEXT_fn(Display*, GLXDrawable, int);
glXSwapIntervalEXT_fn* f_glXSwapIntervalEXT;

#ifndef NDEBUG

typedef void (*glDebugProcARB_fn)(GLenum, GLenum, GLuint, GLenum, GLsizei, const char*, const void*);
typedef void glDebugMessageCallbackARB_fn(glDebugProcARB_fn, const void*);
glDebugMessageCallbackARB_fn* f_glDebugMessageCallbackARB;

typedef void glDebugMessageControlARB_fn(GLenum, GLenum, GLenum, GLsizei, const GLuint*, GLboolean);
glDebugMessageControlARB_fn* f_glDebugMessageControlARB;

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

#define CX_GFX_CONTEXT_GET_GLX_PROC(PROC_NAME) \
	do { \
		f_##PROC_NAME = (PROC_NAME##_fn*)glXGetProcAddressARB((const GLubyte*)#PROC_NAME); \
		if (!(*f_##PROC_NAME)) { \
			CX_LOG(ERROR, GFX_CORE, "Failed to load glX function "#PROC_NAME"\n"); \
		} \
	} while(0)
	
struct gl_context_nix_x11_internals {
    const struct platform_window* p_window;
    XVisualInfo*                  p_visualinfo;
    GLXContext                    context;
};

enum cx_error cx_gfx_context_create(
	const struct platform_window* p_window,
	struct cx_gfx_context* p_out_context) {

    struct gl_context_nix_x11_internals* p_context_internals = (void*)p_out_context->bytes_;
    const struct platform_window_nix_x11_internals* p_window_internals = (const void*)p_window->bytes_;
	
	GLint glx_version_major = 0;
	GLint glx_version_minor = 0;

	glXQueryVersion(p_window_internals->p_display, &glx_version_major, &glx_version_minor);
	if (glx_version_major < GLX_MIN_VERSION_MAJOR || glx_version_minor < GLX_MIN_VERSION_MINOR) {
		CX_LOG_FMT(ERROR, GFX_CORE,
			"glX %d.%d or greater is required (glX version = %d.%d)\n",
			GLX_MIN_VERSION_MAJOR, GLX_MIN_VERSION_MINOR, glx_version_major, glx_version_minor);
		return CX_ERROR_api_glx;
	}

	const int screen = DefaultScreen(p_window_internals->p_display);
	const char* s_extension_list = glXQueryExtensionsString(p_window_internals->p_display, screen);
	CX_LOG_FMT(TRACE, GFX_CORE, "glX supported extensions: %s\n", s_extension_list);

	CX_GFX_CONTEXT_GET_GLX_PROC(glXCreateContextAttribsARB);
	if (!!f_glXCreateContextAttribsARB) {
		int glx_context_flags = GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
#ifndef NDEBUG
		glx_context_flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
#endif

		int glx_context_attribs[] = {
			GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
			GLX_CONTEXT_MINOR_VERSION_ARB, 3,
			GLX_CONTEXT_FLAGS_ARB,         glx_context_flags,
			GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
			None
		};

		p_context_internals->context = f_glXCreateContextAttribsARB(
			p_window_internals->p_display,
			p_window_internals->fbconfig,
			0,
			True,
			glx_context_attribs);
	} else {
		p_context_internals->p_visualinfo = glXGetVisualFromFBConfig(
			p_window_internals->p_display,
			p_window_internals->fbconfig);

		p_context_internals->context = glXCreateContext(
			p_window_internals->p_display,
			p_context_internals->p_visualinfo,
			0,
			True);
	}

	if (!p_context_internals->context) {
		return CX_ERROR_api_glx;
	}

    glXMakeCurrent(p_window_internals->p_display, p_window_internals->window, p_context_internals->context);

#ifndef NDEBUG
	CX_GFX_CONTEXT_GET_GLX_PROC(glDebugMessageCallbackARB);
	CX_GFX_CONTEXT_GET_GLX_PROC(glDebugMessageControlARB);
	if (f_glDebugMessageCallbackARB && f_glDebugMessageControlARB) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		f_glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB, 0, NULL, GL_FALSE);
		f_glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
		f_glDebugMessageCallbackARB(gl_debug_message_callback, NULL);
	}
#endif

    p_context_internals->p_window = p_window;

	CX_GFX_CONTEXT_GET_GLX_PROC(glXSwapIntervalEXT);

	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	
	CX_LOG_FMT(INFO, GFX_CORE,
		"OpenGL %scontext created (v%s, GLSL v%s)\n",
		(context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) ? "Debug " : "",
		glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
	
	CX_LOG_FMT(INFO, GFX_CORE,
		"Graphics platform: %s, %s\n",
		glGetString(GL_VENDOR), glGetString(GL_RENDERER));

    return CX_ERROR_none;
}

void cx_gfx_context_destroy(struct cx_gfx_context* p_context) {
    const struct gl_context_nix_x11_internals* p_context_internals = (const void*)p_context->bytes_;
    const struct platform_window_nix_x11_internals* p_window_internals =
		(const void*)p_context_internals->p_window->bytes_;
    glXDestroyContext(p_window_internals->p_display, p_context_internals->context);
}

enum cx_error cx_gfx_context_make_current(const struct cx_gfx_context* p_context) {
    const struct gl_context_nix_x11_internals* p_context_internals = (const void*)p_context->bytes_;
    const struct platform_window_nix_x11_internals* p_window_internals =
		(const void*)p_context_internals->p_window->bytes_;
    glXMakeCurrent(p_window_internals->p_display, p_window_internals->window, p_context_internals->context);
    return CX_ERROR_none;
}

const struct cx_gfx_framebuffer* cx_gfx_context_get_backbuffer(const struct cx_gfx_context* p_context) {
	(void)p_context;
	static const struct cx_gfx_framebuffer default_fb = {0};
	return &default_fb;
}

enum cx_error cx_gfx_context_swap_buffers(const struct cx_gfx_context* p_context) {
    const struct gl_context_nix_x11_internals* p_context_internals = (const void*)p_context->bytes_;
    const struct platform_window_nix_x11_internals* p_window_internals =
		(const void*)p_context_internals->p_window->bytes_;
    glXSwapBuffers(p_window_internals->p_display, p_window_internals->window);
    return CX_ERROR_none;
}

unsigned int cx_gfx_context_get_swap_interval(const struct cx_gfx_context* p_context) {
    const struct gl_context_nix_x11_internals* p_context_internals = (const void*)p_context->bytes_;
	const struct platform_window_nix_x11_internals* p_platform_window_internals =
		(const void*)p_context_internals->p_window->bytes_;
	unsigned int interval;
	glXQueryDrawable(
		p_platform_window_internals->p_display,
		p_platform_window_internals->window,
		GLX_SWAP_INTERVAL_EXT,
		&interval);
	return interval;
}

enum cx_error cx_gfx_context_set_swap_interval(const struct cx_gfx_context* p_context, unsigned int interval) {
	if (f_glXSwapIntervalEXT) {
    	const struct gl_context_nix_x11_internals* p_context_internals = (const void*)p_context->bytes_;
		const struct platform_window_nix_x11_internals* p_platform_window_internals =
			(const void*)p_context_internals->p_window->bytes_;
		f_glXSwapIntervalEXT(p_platform_window_internals->p_display, p_platform_window_internals->window, (int)interval);
		CX_LOG_FMT(INFO, GFX_CORE, "Swap interval set to %d\n", interval);
		return CX_ERROR_none;
	}
	CX_LOG(INFO, GFX_CORE, "Failed to set swap interval: gl function not loaded\n");
	return CX_ERROR_not_supported;
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

	int log_level = CX_LOG_LEVEL_TRACE;

	if (severity == GL_DEBUG_SEVERITY_LOW_ARB) {
		log_level = CX_LOG_LEVEL_INFO;
	} else if (severity == GL_DEBUG_SEVERITY_MEDIUM_ARB || severity == GL_DEBUG_SEVERITY_HIGH_ARB) {
		log_level = CX_LOG_LEVEL_WARNING;
	}
	
	if (type == GL_DEBUG_TYPE_ERROR_ARB) {
		log_level = CX_LOG_LEVEL_ERROR;
	}

	cx_log_fmt(log_level, CX_LOG_CAT_GFX_CORE,
		"Message: { id=%u, source='%s', type='%s' } %s\n",
		id, s_source, s_type, s_message);
}
#endif
