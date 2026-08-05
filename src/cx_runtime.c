#include "cx_app.h"
#include "cx_rt_manifest.h"

static int cx_runtime_init(int argc, const char** argv);
static void cx_runtime_update(double);
static void cx_runtime_draw(const struct cx_gfx_framebuffer*);
static void cx_runtime_shutdown(void);

int main(int argc, const char** argv) {
	(void)argc;
	(void)argv;

	const char* s_default_manifest_filename = ".cxman";
	
	const char* s_manifest_filename = s_default_manifest_filename;

	if (argc > 1) {
		s_manifest_filename = argv[1];
	}

	//struct cx_rt_manifest manifest;
	//cx_rt_manifest_load_from_file(s_manifest_filename, &manifest);

	cx_app_init("cx demo", 800, 600, cx_runtime_init, argc, argv);
	cx_app_run(cx_runtime_update, cx_runtime_draw);
	cx_app_shutdown(cx_runtime_shutdown);

	return 0;
}

int cx_runtime_init(int argc, const char** argv) {
	(void)argc;
	(void)argv;
	// todo: initialize game and world
	return 0;
}

void cx_runtime_update(double frame_delta_time) {
	(void)frame_delta_time;
	// todo: update game and world
}

void cx_runtime_draw(const struct cx_gfx_framebuffer* p_framebuffer) {
	(void)p_framebuffer;
	// todo: draw game and world
}

void cx_runtime_shutdown(void) {
	// todo: clean up game and world
}
