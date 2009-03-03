#ifndef __LEAK_H__
#define __LEAK_H__

/* low level leak detecton */
#if 0
#include "mem.h"

#define malloc(x) Mem_WrapMalloc(x, __FILE__, __LINE__)
#define strdup(x) Mem_WrapStrdup(x, __FILE__, __LINE__)
#define free(x)   Mem_WrapFree(x, __FILE__, __LINE__)
#define wcsdup(x) Mem_WrapWcsdup(x, __FILE__, __LINE__)
#endif

#endif