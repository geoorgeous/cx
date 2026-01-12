#ifndef _H__CX_DBG
#define _H__CX_DBG

#ifdef NDEBUG

#define CX_DBG(X) ((void)0)

#else

#define CX_DBG(X) do {X;} while(0)

#endif

#endif
