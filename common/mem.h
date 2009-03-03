#ifndef __MEM_H__
#define __MEM_H__

void Mem_InitMemRec();
void Mem_AddMemRec(void *pointer, int size, char *objecttype, char *file, unsigned int line);
void Mem_RemoveMemRec(void *pointer, const char *file, unsigned int line);
void Mem_DumpLeaks();
void *Mem_WrapMalloc(unsigned int size, const char *file, unsigned int line);
char *Mem_WrapStrdup(char *text, const char *file, unsigned int line);
char *Mem_WrapWcsdup(void *text, const char *file, unsigned int line);
void Mem_WrapFree(void *pointer, const char *file, unsigned int line);

#endif