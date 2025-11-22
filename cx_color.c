#include "cx_color.h"

extern inline void color_u8_from_color_f32(struct color_u8* p_color, const struct color_f32* p_color_f32);
extern inline void color_f32_from_color_u8(struct color_f32* p_color, const struct color_u8* p_color_u8);
extern inline void color_f32_from_u32_r8g8b8a8(struct color_f32* p_color, u32_r8g8b8a8 r8g8b8a8);