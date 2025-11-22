#ifndef _H__CX_COLOR
#define _H__CX_COLOR

#include <stdint.h>

#define CX_U32_R8G8B8A8_R(U32) ((U32 >> 24) & 0xff)
#define CX_U32_R8G8B8A8_B(U32) ((U32 >> 16) & 0xff)
#define CX_U32_R8G8B8A8_G(U32) ((U32 >>  8) & 0xff)
#define CX_U32_R8G8B8A8_A(U32) ( U32        & 0xff)
#define CX_U32_R8G8B8A8(R8, G8, B8, A8) ((u32_r8g8b8a8)(((R8) << 24) | ((G8) << 16) | ((B8) << 8) | (A8)))

#define CX_U8_FROM_F32_PERCENT(F32) ((uint8_t)(F32 * 0xff))
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

typedef uint32_t u32_r8g8b8a8;

struct color_u8 {
	union {
		struct {
			uint8_t  r;
			uint8_t  g;
			uint8_t  b;
			uint8_t  a;
		};
		uint8_t      rgba[4];
		u32_r8g8b8a8 rgba_packed;
	};
};

struct color_f32 {
	union {
		float     rgba[4];
		struct {
			float r;
			float g;
			float b;
			float a;
		};
	};
};

inline void color_u8_from_color_f32(struct color_u8* p_color, const struct color_f32* p_color_f32) {
	*p_color = (struct color_u8) {
		.r = CX_U8_FROM_F32_PERCENT(p_color_f32->r),
		.g = CX_U8_FROM_F32_PERCENT(p_color_f32->g),
		.b = CX_U8_FROM_F32_PERCENT(p_color_f32->b),
		.a = CX_U8_FROM_F32_PERCENT(p_color_f32->a)
	};
}

inline void color_f32_from_color_u8(struct color_f32* p_color, const struct color_u8* p_color_u8) {
	*p_color = (struct color_f32) {
		.r = CX_F32_PERCENT_FROM_U8(p_color_u8->r),
		.g = CX_F32_PERCENT_FROM_U8(p_color_u8->g),
		.b = CX_F32_PERCENT_FROM_U8(p_color_u8->b),
		.a = CX_F32_PERCENT_FROM_U8(p_color_u8->a)
	};
}

inline void color_f32_from_u32_r8g8b8a8(struct color_f32* p_color, u32_r8g8b8a8 r8g8b8a8) {
	*p_color = (struct color_f32) {
		.r = CX_F32_PERCENT_FROM_U8(CX_U32_R8G8B8A8_R(r8g8b8a8)),
		.g = CX_F32_PERCENT_FROM_U8(CX_U32_R8G8B8A8_G(r8g8b8a8)),
		.b = CX_F32_PERCENT_FROM_U8(CX_U32_R8G8B8A8_B(r8g8b8a8)),
		.a = CX_F32_PERCENT_FROM_U8(CX_U32_R8G8B8A8_A(r8g8b8a8))
	};
}

#endif