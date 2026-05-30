#ifndef CX_MACRO_H
#define CX_MACRO_H

#ifdef _MSC_VER
#define CX_PRI_SIZE "Iu"
#else
#define CX_PRI_SIZE "zu"
#endif

#define CX_NULL ((void*)0)
#define CX_TRUE  1
#define CX_FALSE 0

#define CX_PRAGMA(X) _Pragma(#X)

#ifdef __clang__
#define CX_PRAGMA_DIAGNOSTIC_PUSH()	CX_PRAGMA(clang diagnostic push)
#elif defined(__GNUC__)
#define CX_PRAGMA_DIAGNOSTIC_PUSH() CX_PRAGMA(GCC diagnostic push)
#else
#efine CXCX_PRAGMA_DIAGNOSTIC_PUSH()
#endif

#ifdef __clang__
#define CX_PRAGMA_DIAGNOSTIC_POP()	CX_PRAGMA(clang diagnostic pop)
#elif defined(__GNUC__)
#define CX_PRAGMA_DIAGNOSTIC_POP() CX_PRAGMA(GCC diagnostic pop)
#else
#define CCX_PRAGMA_DIAGNOSTIC_POP()
#endif

#ifdef __clang__
#define CX_PRAGMA_IGNORE_WARNING(W)	CX_PRAGMA(clang diagnostic ignored W)
#elif defined(__GNUC__)
#define CX_PRAGMA_IGNORE_WARNING(W) CX_PRAGMA(GCC diagnostic ignored W)
#else
#define CX_PCX_PRAGMA_IGNORE_WARNING(W)
#endif

#define CX_STRINGIFY_INTERNAL(X) #X
#define CX_STRINGIFY(X) CX_STRINGIFY_INTERNAL(X)

#define CX_CONCAT_INTERNAL(A, B) A##B
#define CX_CONCAT(A, B) CX_CONCAT_INTERNAL(A, B)

#define CX_FILE_LINE __FILE__":"CX_STRINGIFY(__LINE__)

#define CX_ARRAY_LEN(P_ARRAY) (sizeof(P_ARRAY) / (sizeof(*(P_ARRAY))))

#define CX_SIZEOF_MEMBER(TYPE, MEMBER) (sizeof(((TYPE*)0)->MEMBER))

#define CX_PADDING(N) uint8_t CX_CONCAT(CX_CONCAT(PADDING, __LINE__), _##N##_)[N]

#define CX_ALIGN_DEFAULT_ALIGNMENT 16

#define CX_ALIGN(X, ALIGNMENT) (((X)+ (ALIGNMENT) - 1u) & ~((ALIGNMENT) - 1u))

#define CX_ALIGN_DEFAULT(X)  CX_ALIGN(X, CX_ALIGN_DEFAULT_ALIGNMENT)

#define CX_BSEARCH(ARRAY, NUM, KEY, F_CMP_KEY, P_OUT_INDEX, P_OUT_B_FOUND) do {\
	*(P_OUT_B_FOUND) = 0;\
	*(P_OUT_INDEX) = 0;\
	size_t hi = (NUM);\
	while (*(P_OUT_INDEX) < hi) {\
		const size_t mid = *(P_OUT_INDEX) + (hi - *(P_OUT_INDEX)) / 2;\
		const int cmp = F_CMP_KEY((ARRAY)[mid], (KEY));\
		if (cmp < 0) {\
			*(P_OUT_INDEX) = mid + 1;\
		} else if (cmp > 0) {\
			hi = mid;\
		} else {\
			*(P_OUT_INDEX) = mid;\
			*(P_OUT_B_FOUND) = 1;\
			break;\
		}\
	} } while(0)

#define CX_SORTED_ADD(P_ARRAY, P_NUM, P_CAP, INDEX, VAL) do {\
	if (*(P_CAP) == *(P_NUM)) {\
		*(P_CAP) = (*(P_CAP)) ? *(P_CAP) * 2 : 8;\
		const size_t new_size = *(P_CAP) * sizeof(*(P_ARRAY));\
		(P_ARRAY) = realloc(P_ARRAY, new_size);\
	}\
	if ((INDEX) < *(P_NUM)) {\
		void* p_dst = (P_ARRAY) + (INDEX) + 1;\
		const void* p_src = (P_ARRAY) + (INDEX);\
		const size_t size = sizeof(*(P_ARRAY)) * (*(P_NUM) - INDEX);\
		memmove(p_dst, p_src, size);\
	}\
	++(*P_NUM);\
	(P_ARRAY)[INDEX] = (VAL);\
} while(0)

#define CX_SORTED_REMOVE(P_ARRAY, P_NUM, INDEX) do {\
	--(*P_NUM);\
	if (INDEX == (*P_NUM)) break;\
	void* p_dst = (P_ARRAY) + (INDEX);\
	const void* p_src = (P_ARRAY) + (INDEX) + 1;\
	const size_t size = sizeof(*(P_ARRAY)) * (*(P_NUM) - (INDEX));\
	memmove(p_dst, p_src, size);\
} while(0)

#endif
