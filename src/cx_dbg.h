#ifndef CX_DBG_H
#define CX_DBG_H

#ifdef NDEBUG

#define CX_DBG(X) ((void)0)

#else

#define CX_DBG(X) do {X;} while(0)

#endif

#endif
