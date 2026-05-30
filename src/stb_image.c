#include "cx_macro.h"
CX_PRAGMA_DIAGNOSTIC_PUSH()
CX_PRAGMA_IGNORE_WARNING("-Wconversion")
CX_PRAGMA_IGNORE_WARNING("-Wdouble-promotion")
CX_PRAGMA_IGNORE_WARNING("-Wpadded")
CX_PRAGMA_IGNORE_WARNING("-Wsign-conversion")
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
// #define STBI_ONLY_PSD
#define STBI_ONLY_TGA
// #define STBI_ONLY_GIF
// #define STBI_ONLY_HDR
// #define STBI_ONLY_PIC
// #define STBI_ONLY_PNM
#include "stb_image.h"
CX_PRAGMA_DIAGNOSTIC_POP()
