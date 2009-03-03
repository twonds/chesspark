#include <string.h>
#include <stdlib.h>

#include "log.h"

struct memrec_s
{
	struct memrec_s *next;
	char *objecttype;
	char *file;
	unsigned int line;
	void *pointer;
	unsigned int size;
};

struct memrec_s *allocs[64];

void Mem_InitMemRec()
{
	int i;

	for (i=0; i<64; i++)
	{
		allocs[i] = malloc(sizeof(*allocs[i]));
		memset(allocs[i], 0, sizeof(*allocs[i]));
		allocs[i]->next = allocs[i];
	}
}

void Mem_AddMemRec(void *pointer, int size, char *objecttype, const char *file, unsigned int line)
{
	struct memrec_s *rec = malloc(sizeof(*rec));
	struct memrec_s *oldnext;
	int key = ((unsigned int)(pointer) / 4) % 64;

	rec->objecttype = strdup(objecttype);
	rec->file = strdup(file);
	rec->line = line;
	rec->pointer = pointer;
	rec->size = size;

	oldnext = allocs[key]->next;
	allocs[key]->next = rec;
	rec->next = oldnext;
}

void Mem_RemoveMemRec(void *pointer, const char *file, unsigned int line)
{
	struct memrec_s *next;
	int key = ((unsigned int)(pointer) / 4) % 64;
	next = allocs[key];

	while(next->next != allocs[key])
	{
		if (next->next->pointer == pointer)
		{
			struct memrec_s *oldrec = next->next;
			next->next = next->next->next;
			free(oldrec->objecttype);
			free(oldrec->file);
			free(oldrec);
			return;
		}

		next = next->next;
	}

	Log_Write(0, "Free of unreserved memory at %s:%d, pointer %d\n", file, line, pointer);
}

struct fileleak_s
{
	struct fileleak_s *next;
	char *name;
	int size;
};

void Mem_DumpLeaks()
{
	struct memrec_s *next;
	unsigned int totalleak = 0;
	struct fileleak_s *files = NULL;
	int i;

	for (i=0; i<64; i++)
	{
		next = allocs[i];
		while (next->next != allocs[i])
		{
			struct fileleak_s *find = files;

			while (find && strcmp(find->name, next->next->file) != 0)
			{
				find = find->next;
			}

			if (find)
			{
				find->size += next->next->size;
			}
			else
			{
				find = malloc(sizeof(*find));
				find->name = strdup(next->next->file);
				find->size = next->next->size;
				find->next = files;
				files = find;
			}
				
			Log_Write(0, "Leaked allocation, type %s\nat %s:%d\npointer %d size %d\n", next->next->objecttype, next->next->file, next->next->line, next->next->pointer, next->next->size);

			totalleak += next->next->size;

			next = next->next;
		}
	}

	while(files)
	{
		Log_Write(0, "Total leaked in %s: %d\n", files->name, files->size);
		free(files->name);
		files = files->next;
	}

	Log_Write(0, "Total leaked: %d\n", totalleak);
}

void *Mem_WrapMalloc(unsigned int size, const char *file, unsigned int line)
{
	void *newalloc = malloc(size);
	Mem_AddMemRec(newalloc, size, "malloc", file, line);

	return newalloc;
}

char *Mem_WrapStrdup(char *text, const char *file, unsigned int line)
{
	void *newalloc = strdup(text);

	if (text)
	{
		Mem_AddMemRec(newalloc, (int)(strlen(newalloc) + 1), "strdup", file, line);
	}

	return newalloc;
}

char *Mem_WrapWcsdup(void *text, const char *file, unsigned int line)
{
	void *newalloc = wcsdup(text);

	if (text)
	{
		Mem_AddMemRec(newalloc, (int)((wcslen(newalloc) + 1) * 2), "wcsdup", file, line);
	}

	return newalloc;
}
void Mem_WrapFree(void *pointer, const char *file, unsigned int line)
{
	if (!pointer)
	{
		return;
	}

	Mem_RemoveMemRec(pointer, file, line);

	free(pointer);
}
