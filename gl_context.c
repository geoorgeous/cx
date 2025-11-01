#include "gl_context.h"

#include <stdio.h>

#include "gl.h"
#include "logging.h"
#include "platform_window.h"

#if PLATFORM_WINDOWS

typedef const char* APIENTRY wglGetExtensionsStringARB_fn(HDC);
wglGetExtensionsStringARB_fn* wglGetExtensionsStringARB;

typedef HGLRC APIENTRY wglCreateContextAttribsARB_fn(HDC, HGLRC, const int*);
wglCreateContextAttribsARB_fn* wglCreateContextAttribsARB;

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_FLAGS_ARB                     0x2094

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x0001
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002

typedef BOOL APIENTRY wglChoosePixelFormatARB_fn(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
wglChoosePixelFormatARB_fn* wglChoosePixelFormatARB;

#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#define WGL_ACCELERATION_ARB                      0x2003
#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#define WGL_PIXEL_TYPE_ARB                        0x2013
#define WGL_COLOR_BITS_ARB                        0x2014
#define WGL_DEPTH_BITS_ARB                        0x2022
#define WGL_STENCIL_BITS_ARB                      0x2023

#define WGL_TYPE_RGBA_ARB                         0x202B
#define WGL_FULL_ACCELERATION_ARB                 0x2027

typedef BOOL APIENTRY wglSwapIntervalEXT_fn(int);
wglSwapIntervalEXT_fn* wglSwapIntervalExt;

#ifndef NDEBUG

typedef void (APIENTRY *glDebugProcARB_fn)(GLenum, GLenum, GLuint, GLenum, GLsizei, const char*, const void*);
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

#endif

static enum error win32_load_gl_functions(void);

#ifndef NDEBUG
static void gl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* user_param);
#endif

enum error gl_context_create(const struct platform_window* p_platform_window, int gl_version_major, int gl_version_minor, struct gl_context* p_gl_context) {
    enum error err = win32_load_gl_functions();
	if (err != ERROR_OK) {
		return err;
	}

    // Now we can choose a pixel format the modern way, using wglChoosePixelFormatARB.
	int pixel_format_attribs[] = {
		WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
		WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
		WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,         32,
		WGL_DEPTH_BITS_ARB,         24,
		WGL_STENCIL_BITS_ARB,       8,
		0
	};

	int pixel_format;
	UINT num_formats;
	if (!wglChoosePixelFormatARB(p_platform_window->_hdc, pixel_format_attribs, 0, 1, &pixel_format, &num_formats)) {
		return ERROR_WGL_CHOOSE_PIXEL_FORMAT;
	}

	if (num_formats == 0) {
		return ERROR_WGL_NO_SUITABLE_PIXEL_FORMAT;
	}

	PIXELFORMATDESCRIPTOR pfd;
	if (DescribePixelFormat(p_platform_window->_hdc, pixel_format, sizeof(pfd), &pfd) == 0) {
		return ERROR_WINGDI_DESCRIBE_PIXEL_FORMAT;
	}

	if (!SetPixelFormat(p_platform_window->_hdc, pixel_format, &pfd)) {
		return ERROR_WINGDI_SET_PIXEL_FORMAT;
	}

	int gl_context_flags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
	
#ifndef NDEBUG
	gl_context_flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

	// Specify that we want to create an OpenGL x.x core profile context
	int gl_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, gl_version_major,
		WGL_CONTEXT_MINOR_VERSION_ARB, gl_version_minor,
		WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		WGL_CONTEXT_FLAGS_ARB,         gl_context_flags,
		0,
	};

	HGLRC hrc = wglCreateContextAttribsARB(p_platform_window->_hdc, 0, gl_attribs);
	if (!hrc) {
		return ERROR_WGL_CREATE_CONTEXT;
	}

	if (!wglMakeCurrent(p_platform_window->_hdc, hrc)) {
		return ERROR_WGL_MAKE_CURRENT;
	}

#ifndef NDEBUG
	if (glDebugMessageCallbackARB) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB, 0, NULL, GL_FALSE);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
		glDebugMessageCallbackARB(gl_debug_message_callback, NULL);
	}
#endif

	p_gl_context->_hdc = p_platform_window->_hdc;
	p_gl_context->_hrc = hrc;
	            
	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	printf("OpenGL %scontext created (v%s, GLSL v%s)\n", context_flags & 0x2 ? "Debug " : "", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("Graphics platform: %s, %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));

	return ERROR_OK;
}

void gl_context_destroy(struct gl_context* p_gl_context) {
	wglDeleteContext(p_gl_context->_hrc);
}

enum error gl_context_make_current(const struct gl_context* p_gl_context) {
	if (wglMakeCurrent(p_gl_context->_hdc, p_gl_context->_hrc)) {
		return ERROR_OK;
	}
	return ERROR_WGL_MAKE_CURRENT;
}

enum error gl_context_swap_buffers(struct gl_context* p_gl_context) {
	if (wglSwapLayerBuffers(p_gl_context->_hdc, WGL_SWAP_MAIN_PLANE)) {
		return ERROR_OK;
	}

	return ERROR_WGL;
}

static enum error win32_load_gl_functions(void) {
    static int b_loaded = 0;

	if (b_loaded) {
		return ERROR_OK;
	}

	// Before we can load extensions, we need a dummy OpenGL context, created using a dummy window.
	// We use a dummy window because you can only set the pixel format for a window once. For the
	// real window, we want to use wglChoosePixelFormatARB (so we can potentially specify options
	// that aren't available in PIXELFORMATDESCRIPTOR), but we can't load and use that before we
	// have a context.
	WNDCLASS wndclass = {
		.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
		.lpfnWndProc = DefWindowProcA,
		.hInstance = GetModuleHandle(0),
		.lpszClassName = "GLExtProcLoader",
	};

	if (!RegisterClass(&wndclass)) {
		return ERROR_WIN32_REGISTER_CLASS;
	}

	HWND hwnd = CreateWindowEx(
		0,
		wndclass.lpszClassName,
		"OpenGL Extension Procedure Loader",
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		wndclass.hInstance,
		0);

	if (!hwnd) {
		return ERROR_WIN32_CREATE_WINDOW;
	}

	HDC hdc = GetDC(hwnd);

	PIXELFORMATDESCRIPTOR pfd = {
		.nSize = sizeof(pfd),
		.nVersion = 1,
		.iPixelType = PFD_TYPE_RGBA,
		.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		.cColorBits = 32,
		.cAlphaBits = 8,
		.iLayerType = PFD_MAIN_PLANE,
		.cDepthBits = 24,
		.cStencilBits = 8,
	};

	int pixel_format = ChoosePixelFormat(hdc, &pfd);
	if (!pixel_format) {
		return ERROR_WINGDI_CHOOSE_PIXEL_FORMAT;
	}
	
	if (!SetPixelFormat(hdc, pixel_format, &pfd)) {
		return ERROR_WINGDI_SET_PIXEL_FORMAT;
	}

	HGLRC hglrc = wglCreateContext(hdc);
	if (!hglrc) {
		return ERROR_WGL_CREATE_CONTEXT;
	}

	if (!wglMakeCurrent(hdc, hglrc)) {
		return ERROR_WGL_MAKE_CURRENT;
	}

	// GLint extensions_n = 0;
	// glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_n);
	// printf("OpenGL extensions supported:\n");
	// for (GLint i = 0; i < extensions_n; ++i) {
	// 	const char* extension_str = (const char*)glGetStringi(GL_EXTENSIONS, i);
	// 	printf(" %d. %s\n", i + 1, extension_str);
	// }

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

#ifndef NDEBUG
	glDebugMessageCallbackARB = (glDebugMessageCallbackARB_fn*)wglGetProcAddress("glDebugMessageCallbackARB");
	glDebugMessageControlARB = (glDebugMessageControlARB_fn*)wglGetProcAddress("glDebugMessageControlARB");
#endif
	wglGetExtensionsStringARB = (wglGetExtensionsStringARB_fn*)wglGetProcAddress("wglGetExtensionsStringARB");
	wglCreateContextAttribsARB = (wglCreateContextAttribsARB_fn*)wglGetProcAddress("wglCreateContextAttribsARB");
	wglChoosePixelFormatARB = (wglChoosePixelFormatARB_fn*)wglGetProcAddress("wglChoosePixelFormatARB");
	wglSwapIntervalExt = (wglSwapIntervalEXT_fn*)wglGetProcAddress("wglSwapIntervalEXT");

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

	// const char* extensions_str = wglGetExtensionsStringARB(hdc);
	// printf("WGL extentions supported:\n");
	// const char* pos = extensions_str;
	// int i = 0;
	// while (*pos) {
	// 	if (*pos == ' ') {
	// 		printf(" %d. %.*s\n", i + 1, (int)(pos - extensions_str), extensions_str);
	// 		extensions_str = pos + 1;
	// 		++i;
	// 	}
	// 	pos++;
	// }

	wglMakeCurrent(hdc, NULL);
	wglDeleteContext(hglrc);
	ReleaseDC(hwnd, hdc);
	DestroyWindow(hwnd);

	b_loaded = 1;

	return ERROR_OK;
}

#ifndef NDEBUG
void gl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const char* message, const void*) {
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

	unsigned int log_level = LOG_TRACE;

	if (severity == GL_DEBUG_SEVERITY_LOW_ARB) {
		log_level = LOG_INFO;
	} else if (severity == GL_DEBUG_SEVERITY_MEDIUM_ARB || severity == GL_DEBUG_SEVERITY_HIGH_ARB) {
		log_level = LOG_WARNING;
	}
	
	if (type == GL_DEBUG_TYPE_ERROR_ARB) {
		log_level = LOG_ERROR;
	}

	log_msg(log_level, "opengl", "Message: { id=%u, source='%s', type='%s' } %s\n", id, s_source, s_type, message);
}
#endif

#endif