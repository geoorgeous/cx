#ifndef CX_MATH_H
#define CX_MATH_H

#include <stdint.h>

#define CX_M_MIN(X, Y) (((X) > (Y)) ? (Y) : (X))
#define CX_M_MAX(X, Y) (((X) < (Y)) ? (Y) : (X))
#define CX_M_CLAMP(MIN, MAX, X) CX_M_MIN(CX_M_MAX(MIN, X), MAX)

#endif
