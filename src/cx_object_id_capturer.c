#include "cx_gfx_framebuffer.h"
#include "cx_gfx_texture.h"
#include "cx_object_id_capturer.h"
#include "cx_pixel_format.h"
#include "gl.h"
#include "math_utils.h"
#include <stdint.h>

static void cx_object_id_capturer_destroy_framebuffer(struct cx_object_id_capturer* p_capturer);
static void cx_object_id_capturer_rebuild_framebuffer(
	struct cx_object_id_capturer* p_capturer,
	uint32_t fb_width,
	uint32_t fb_height);

void cx_object_id_capturer_free(struct cx_object_id_capturer* p_capturer) {
	cx_object_id_capturer_destroy_framebuffer(p_capturer);
}

void cx_object_id_capturer_set_fb_size(
	struct cx_object_id_capturer* p_capturer,
	uint32_t fb_width,
	uint32_t fb_height) {

	if (p_capturer->framebuffer_width != fb_width ||
		p_capturer->framebuffer_height!= fb_height) {
		cx_object_id_capturer_rebuild_framebuffer(p_capturer, fb_width, fb_height);
	}
}

uint32_t cx_object_id_capturer_query(const struct cx_object_id_capturer* p_capturer, float x, float y) {
	x = clampf(x, 0, 1);
	y = clampf(y, 0, 1);
	
	uint32_t pixel_location[] = { 
		(uint32_t)((float)p_capturer->framebuffer_width * x),
		(uint32_t)((float)p_capturer->framebuffer_height * (1.0f - y))
	};
	unsigned int pixel_value;

	cx_gfx_framebuffer_read(
		&p_capturer->framebuffer,
		CX_GFX_FRAMEBUFFER_ATTACHMENT_color0,
		pixel_location,
		(uint32_t[]){ 1, 1 },
		&pixel_value);

	return pixel_value;
}

void cx_object_id_capturer_destroy_framebuffer(struct cx_object_id_capturer* p_capturer) {
	cx_gfx_framebuffer_destroy(&p_capturer->framebuffer);
	cx_gfx_texture_destroy(&p_capturer->framebuffer_color);
	cx_gfx_texture_destroy(&p_capturer->framebuffer_depth_stencil);
	*p_capturer = (struct cx_object_id_capturer){0};
}

void cx_object_id_capturer_rebuild_framebuffer(
	struct cx_object_id_capturer* p_capturer,
	uint32_t fb_width,
	uint32_t fb_height) {

	cx_object_id_capturer_destroy_framebuffer(p_capturer);	
	
	cx_gfx_texture_create(
		&p_capturer->framebuffer_color,
		fb_width, fb_height,
		CX_PIXEL_FORMAT_red_u32);

	cx_gfx_texture_create(
		&p_capturer->framebuffer_depth_stencil,
		fb_width, fb_height,
		CX_PIXEL_FORMAT_depth_stencil);

	cx_gfx_framebuffer_create(&p_capturer->framebuffer);
	cx_gfx_framebuffer_set_attachment(
		&p_capturer->framebuffer,
		CX_GFX_FRAMEBUFFER_ATTACHMENT_color0,
		&p_capturer->framebuffer_color);
	cx_gfx_framebuffer_set_attachment(
		&p_capturer->framebuffer,
		CX_GFX_FRAMEBUFFER_ATTACHMENT_depth_stencil,
		&p_capturer->framebuffer_depth_stencil);

	p_capturer->framebuffer_width = fb_width;
	p_capturer->framebuffer_height = fb_height;
}
