#include "cx_app.h"
#include "cx_ed.h"

static int cx_editor_init(void);
static void cx_editor_update(double);
static void cx_editor_draw(const struct cx_gfx_framebuffer*);
static void cx_editor_shutdown(void);

int cx_editor_init(void) {
	cx_ed_init(cx_app_primary_window());
	return 0;
}

void cx_editor_update(double frame_delta_time) {
	cx_ed_update(frame_delta_time);
}

void cx_editor_draw(const struct cx_gfx_framebuffer* p_frambuffer) {
	cx_ed_draw(p_frambuffer, 1920, 1080);
}

void cx_editor_shutdown(void) {
	cx_ed_shutdown();
}

int main(int argc, const char** argv) {
	(void)argc;
	(void)argv;

	cx_app_init("cx editor", 1920, 1080, cx_editor_init);
	cx_app_run(cx_editor_update, cx_editor_draw);
	cx_app_shutdown(cx_editor_shutdown);
}
