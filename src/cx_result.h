#ifndef CX_RESULT_H
#define CX_RESULT_H

typedef int cx_result;

#define CX_SUCCESS 0

#define CX_ERROR_UNKNOWN            -1
#define CX_ERROR_INVALID_ARG         1
#define CX_ERROR_OUT_OF_MEMORY       2
#define CX_ERROR_UNSUPPORTED         3
#define CX_ERROR_NOT_FOUND           4
#define CX_ERROR_ALREADY_EXISTS      5
#define CX_ERROR_IO                  6
#define CX_ERROR_SERIALIZE           7
#define CX_ERROR_DESERIALIZE         8
#define CX_ERROR_PERMISSION_DENIED   9

#endif
