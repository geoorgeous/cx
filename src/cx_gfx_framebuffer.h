#ifndef CX_GFX_FRAMEBUFFER_H
#define CX_GFX_FRAMEBUFFER_H

#include "errors.h"
#include <stdint.h>

enum cx_gfx_framebuffer_attachment {
	CX_GFX_FRAMEBUFFER_ATTACHMENT_color0,
	CX_GFX_FRAMEBUFFER_ATTACHMENT_color1,
	CX_GFX_FRAMEBUFFER_ATTACHMENT_color2,
	CX_GFX_FRAMEBUFFER_ATTACHMENT_color3,
	CX_GFX_FRAMEBUFFER_ATTACHMENT_color4,
	CX_GFX_FRAMEBUFFER_ATTACHMENT_color5,
	CX_GFX_FRAMEBUFFER_ATTACHMENT_color6,
	CX_GFX_FRAMEBUFFER_ATTACHMENT_color7,
	CX_GFX_FRAMEBUFFER_ATTACHMENT_depth,
	CX_GFX_FRAMEBUFFER_ATTACHMENT_stencil,
	CX_GFX_FRAMEBUFFER_ATTACHMENT_depth_stencil
};

struct cx_gfx_framebuffer {
	char _bytes[4];
};

enum error cx_gfx_framebuffer_create(struct cx_gfx_framebuffer* p_framebuffer);

void       cx_gfx_framebuffer_destroy(struct cx_gfx_framebuffer* p_framebuffer);

struct cx_gfx_texture;

void       cx_gfx_framebuffer_set_attachment(
	const struct cx_gfx_framebuffer* p_framebuffer,
	enum cx_gfx_framebuffer_attachment attachment_point,
	const struct cx_gfx_texture* p_texture);

void       cx_gfx_framebuffer_bind(const struct cx_gfx_framebuffer* p_framebuffer);

void       cx_gfx_framebuffer_read(
	const struct cx_gfx_framebuffer* p_framebuffer,
	enum cx_gfx_framebuffer_attachment attachment,
	const uint32_t* p_read_position,
	const uint32_t* p_read_size,
	void* p_out_read_buffer);

#endif
