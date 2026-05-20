#ifndef CX_MACRO_H
#define CX_MACRO_H

#define CX_STRINGIFY_INTERNAL(X) #X
#define CX_STRINGIFY(X) CX_STRINGIFY_INTERNAL(X)

#define CX_BSEARCH(ARRAY, NUM, KEY, F_CMP_KEY, P_OUT_INDEX, P_B_OUT_FOUND) do {\
	*(P_B_OUT_FOUND) = 0;\
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
			*(P_B_OUT_FOUND) = 1;\
			break;\
		}\
	} } while(0)

#define CX_SORTED_ADD(P_ARRAY, P_NUM, P_CAP, INDEX, P_VAL) do {\
	if (*(P_CAP) == *(P_NUM)) {\
		*(P_CAP) = (*(P_CAP)) ? *(P_CAP) * 2 : 8;\
		const size_t new_size = *(P_CAP) * sizeof(*(P_ARRAY));\
		(P_ARRAY) = realloc(P_ARRAY, new_size);\
	}\
	if ((INDEX) < *(P_NUM)) {\
		void* p_dst = (P_ARRAY) + (INDEX) + 1;\
		const void* p_src = (P_ARRAY) + (INDEX);\
		const size_t size = sizeof(*(P_ARRAY)) * *((P_NUM) - INDEX);\
		memmove(p_dst, p_src, size);\
	}\
	++(*P_NUM);\
	(P_ARRAY)[INDEX] = (P_VAL);\
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
