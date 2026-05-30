#ifndef CX_COLOR_H
#define CX_COLOR_H

#include <stdint.h>

#include "math_utils.h"

#define CX_COLOR_U32_R8G8B8A8_R(U32) (((U32) >> 24) & 0xff)
#define CX_COLOR_U32_R8G8B8A8_G(U32) (((U32) >> 16) & 0xff)
#define CX_COLOR_U32_R8G8B8A8_B(U32) (((U32) >>  8) & 0xff)
#define CX_COLOR_U32_R8G8B8A8_A(U32) ( (U32)        & 0xff)
#define CX_COLOR_U32_R8G8B8A8(R8, G8, B8, A8) ((u32_r8g8b8a8)(((R8) << 24) | ((G8) << 16) | ((B8) << 8) | (A8)))

#define CX_U8_FROM_F32_PERCENT(F32) ((uint8_t)((F32) * 0xff))
#define CX_F32_PERCENT_FROM_U8(U8) (((float)(U8)) / 0xff)

#define CX_COLOR_NONE                 ((u32_r8g8b8a8)0x00000000)
#define CX_COLOR_BLACK                ((u32_r8g8b8a8)0x000000ff)
#define CX_COLOR_WHITE                ((u32_r8g8b8a8)0xffffffff)
#define CX_COLOR_RED                  ((u32_r8g8b8a8)0xff0000ff)
#define CX_COLOR_GREEN                ((u32_r8g8b8a8)0x00ff00ff)
#define CX_COLOR_BLUE                 ((u32_r8g8b8a8)0x0000ffff)
#define CX_COLOR_YELLOW               ((u32_r8g8b8a8)0xffff00ff)
#define CX_COLOR_MAGENTA              ((u32_r8g8b8a8)0xff00ffff)
#define CX_COLOR_CYAN                 ((u32_r8g8b8a8)0x00ffffff)

#define CX_COLOR_R(COLOR) ((COLOR).rgba[0])
#define CX_COLOR_G(COLOR) ((COLOR).rgba[1])
#define CX_COLOR_B(COLOR) ((COLOR).rgba[2])
#define CX_COLOR_A(COLOR) ((COLOR).rgba[3])

struct cx_color {
	float rgba[4];
};

struct cx_color_u8 {
	uint8_t rgba[4];
};

static inline void cx_color_u8_from_f32(const struct cx_color* p_color, struct cx_color_u8* p_out) {
	*p_out = (struct cx_color_u8) {
		.rgba = {
			CX_U8_FROM_F32_PERCENT(CX_COLOR_R(*p_color)),
			CX_U8_FROM_F32_PERCENT(CX_COLOR_G(*p_color)),
			CX_U8_FROM_F32_PERCENT(CX_COLOR_B(*p_color)),
			CX_U8_FROM_F32_PERCENT(CX_COLOR_A(*p_color))
		}
	};
}

static inline int cx_color_u8_cmp(const struct cx_color_u8* p_a, const struct cx_color_u8* p_b) {
	return
		CX_COLOR_R(*p_a) == CX_COLOR_R(*p_b) &&
		CX_COLOR_G(*p_a) == CX_COLOR_G(*p_b) &&
		CX_COLOR_B(*p_a) == CX_COLOR_B(*p_b) &&
		CX_COLOR_A(*p_a) == CX_COLOR_A(*p_b);
}

static inline void cx_color_f32_from_u8(const struct cx_color_u8* p_color, struct cx_color* p_out) {
	*p_out = (struct cx_color) {
		.rgba = {
			CX_F32_PERCENT_FROM_U8(CX_COLOR_R(*p_color)),
			CX_F32_PERCENT_FROM_U8(CX_COLOR_G(*p_color)),
			CX_F32_PERCENT_FROM_U8(CX_COLOR_B(*p_color)),
			CX_F32_PERCENT_FROM_U8(CX_COLOR_A(*p_color))
		}
	};
}

static inline void cx_color_f32_from_u32(uint32_t r8g8b8a8, struct cx_color* p_color) {
	*p_color = (struct cx_color) {
		.rgba = {
			CX_F32_PERCENT_FROM_U8(CX_COLOR_U32_R8G8B8A8_R(r8g8b8a8)),
			CX_F32_PERCENT_FROM_U8(CX_COLOR_U32_R8G8B8A8_G(r8g8b8a8)),
			CX_F32_PERCENT_FROM_U8(CX_COLOR_U32_R8G8B8A8_B(r8g8b8a8)),
			CX_F32_PERCENT_FROM_U8(CX_COLOR_U32_R8G8B8A8_A(r8g8b8a8))
		}
	};
}

static inline int cx_color_f32_cmp(const struct cx_color* p_a, const struct cx_color* p_b) {
	return
		FLT_CMP(CX_COLOR_R(*p_a), CX_COLOR_R(*p_b)) &&
		FLT_CMP(CX_COLOR_G(*p_a), CX_COLOR_G(*p_b)) &&
		FLT_CMP(CX_COLOR_B(*p_a), CX_COLOR_B(*p_b)) &&
		FLT_CMP(CX_COLOR_A(*p_a), CX_COLOR_A(*p_b));
}

#endif
