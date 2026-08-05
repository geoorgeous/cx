#include "cx_app.h"
#include "cx_rt_manifest.h"

static int cx_rt_app_init(int argc, const char** argv);
static void cx_rt_app_update(double);
static void cx_rt_app_draw(const struct cx_gfx_framebuffer*);
static void cx_rt_app_shutdown(void);

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

	cx_app_init("cx demo", 800, 600, cx_rt_app_init, argc, argv);
	cx_app_run(cx_rt_app_update, cx_rt_app_draw);
	cx_app_shutdown(cx_rt_app_shutdown);

	return 0;
}

int cx_rt_app_init(int argc, const char** argv) {
	(void)argc;
	(void)argv;
	// todo: initialize game and world
	return 0;
}

void cx_rt_app_update(double frame_delta_time) {
	(void)frame_delta_time;
	// todo: update game and world
}

void cx_rt_app_draw(const struct cx_gfx_framebuffer* p_framebuffer) {
	(void)p_framebuffer;
	// todo: draw game and world
}

void cx_rt_app_shutdown(void) {
	// todo: clean up game and world
}
