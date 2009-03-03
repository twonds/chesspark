#include <stdlib.h>
#include <string.h>

#include "namedlist.h"
#include "leak.h"

struct namedlist_s **NamedList_Add(struct namedlist_s **list, char *name, void *data, void (*destroydata)(void *))
{
	while (*list)
	{
		list = &((*list)->next);
	}

	*list = malloc(sizeof(**list));
	memset(*list, 0, sizeof(**list));

	if (name)
	{
		(*list)->name = strdup(name);
	}
	(*list)->data = data;
	(*list)->destroydata = destroydata;

	return list;
}

struct namedlist_s **NamedList_AddToTop(struct namedlist_s **list, char *name, void *data, void (*destroydata)(void *))
{
	struct namedlist_s *next = *list;

	*list = malloc(sizeof(**list));
	memset(*list, 0, sizeof(**list));

	if (name)
	{
		(*list)->name = strdup(name);
	}
	(*list)->data = data;
	(*list)->destroydata = destroydata;
	(*list)->next = next;

	return list;
}

void NamedList_Unlink(struct namedlist_s **list)
{
	struct namedlist_s *old;

	old = *list;
	
	*list = old->next;

	free(old->name);
	free(old);

}

void NamedList_Remove(struct namedlist_s **list)
{
	struct namedlist_s *old;

	if (!list)
	{
		return;
	}

	old = *list;
	
	*list = old->next;

	if (old->destroydata)
	{
		old->destroydata(old->data);
	}

	free(old->name);
	free(old);

}

void NamedList_RemoveLast(struct namedlist_s **list)
{
	while (*list && (*list)->next)
	{
		list = &((*list)->next);
	}

	if (!*list)
	{
		return;
	}

	NamedList_Remove(list);
}

struct namedlist_s **NamedList_GetByName(struct namedlist_s **list, char *name)
{
	while (*list)
	{
		if ((name && (*list)->name && stricmp(name, (*list)->name) == 0)
			|| !name && !(*list)->name)
		{
			return list;
		}
		else
		{
			list = &((*list)->next);
		}
	}

	return NULL;
}

struct namedlist_s **NamedList_GetByNameAndBump(struct namedlist_s **list, char *name)
{
	struct namedlist_s **entry = NamedList_GetByName(list, name);

	if (entry)
	{
		struct namedlist_s *oldentry = *entry;

		*entry = oldentry->next;
		oldentry->next = *list;
		*list = oldentry;
		entry = list;
	}

	return entry;
	
}

struct namedlist_s **NamedList_GetNextByName(struct namedlist_s **list, char *name)
{
	if (!*list)
	{
		return NULL;
	}

	list = &((*list)->next);

	return NamedList_GetByName(list, name);
}

void NamedList_RemoveByName(struct namedlist_s **list, char *name)
{
	struct namedlist_s **ppremove;

	while(ppremove = NamedList_GetByName(list, name))
	{
		NamedList_Remove(ppremove);
	}
}

void NamedList_Destroy(struct namedlist_s **list)
{
	while (*list)
	{
		NamedList_Remove(list);
	}
}

void NamedList_Destroy2(struct namedlist_s *list)
{
	NamedList_Destroy(&list);
}

/* Yeah, this is a quicksort using linked lists, and using the first entry in the list
   as the pivot pretty much ensures worst case performance.

   FIXME: Make not stupid. :) */
/* changed with a bubble sort (ick) to see if my algorithm is bad */
#if 0
void NamedList_Sort(struct namedlist_s **entry, int (*sortfunc)(struct namedlist_s *lesser, struct namedlist_s *greater, void *userdata), void *userdata)
{
	struct namedlist_s **current;

	if (!(*entry))
	{
		return;
	}

	current = &((*entry)->next);

	while (*current)
	{
		if (sortfunc(*current, *entry, userdata))
		{
			void *swap;

			swap = (*current)->data;
			(*current)->data = (*entry)->data;
			(*entry)->data = swap;

			swap = (*current)->name;
			(*current)->name = (*entry)->name;
			(*entry)->name = swap;

			swap = (*current)->destroydata;
			(*current)->destroydata = (*entry)->destroydata;
			(*entry)->destroydata = swap;
			/*
			struct namedlist_s *swap;
			
			swap = (*current)->next;
			(*current)->next = (*entry)->next;
			(*entry)->next = swap;

			swap = *current;
			*current = *entry;
			*entry = swap;
			*/
		}
		current = &((*current)->next);
	}

	current = &((*entry)->next);
	NamedList_Sort(current, sortfunc, userdata);
}
#endif

void NamedList_Sort(struct namedlist_s **entry, int (*sortfunc)(struct namedlist_s *lesser, struct namedlist_s *greater, void *userdata), void *userdata)
{
	struct namedlist_s *firstleft = NULL, **currentleft = &firstleft, *firstright = NULL, **currentright = &firstright, *pivot = *entry, *current;

	if (!pivot || !sortfunc)
	{
		return;
	}

	current = pivot->next;
	while (current)
	{
		struct namedlist_s *next = current->next;
		current->next = NULL;
		if (sortfunc(current, pivot, userdata))
		{
			*currentleft = current;
			currentleft = &((*currentleft)->next);
		}
		else
		{
			*currentright = current;
			currentright = &((*currentright)->next);
		}
		current = next;
	}

	NamedList_Sort(&firstleft, sortfunc, userdata);
	NamedList_Sort(&firstright, sortfunc, userdata);

	currentleft = &firstleft;
	while (*currentleft)
	{
		currentleft = &((*currentleft)->next);
	}

	*currentleft = pivot;
	pivot->next = firstright;

	*entry = firstleft;
}


int NamedList_NameSortFunc(struct namedlist_s *lesser, struct namedlist_s *greater, void *userdata)
{
	return ((!lesser->name && greater->name)
			|| (lesser->name && greater->name && stricmp(lesser->name, greater->name) < 0));
}

void NamedList_SortByName(struct namedlist_s **entry)
{
	NamedList_Sort(entry, NamedList_NameSortFunc, NULL);
}

char *NamedList_ListToString(struct namedlist_s **list)
{
	struct namedlist_s *entry = *list;
	char *string;
	int length = 1;

	while (entry)
	{
		length += (int)strlen(entry->name) + 1;
		entry = entry->next;
	}

	if (length == 1)
	{
		return "";
	}

	entry = *list;
	string = malloc(length);
	string[0] = 0;

	while (entry)
	{
		strcat(string, entry->name);
		length = (int)strlen(string);
		string[length] = 27;
		string[length+1] = 0;
		entry = entry->next;
	}

	return string;
}

struct namedlist_s *NamedList_StringToList(char *instring)
{
	struct namedlist_s *list = NULL;
	char *string, *start, *end;

	if (!instring)
	{
		return NULL;
	}

	string = strdup(instring);
	start = string;

	while (end = strchr(start, 27))
	{
		*end = '\0';
		NamedList_Add(&list, start, NULL, NULL);
		start = end + 1;
	}

	free(string);

	return list;
}

void NamedList_FreeString(char *string)
{
	free(string);
}

void NamedList_AddString(struct namedlist_s **list, char *name, char *string)
{
	NamedList_Add(list, name, strdup(string), NamedList_FreeString);
}

char *NamedList_ListToString2(struct namedlist_s **list)
{
	struct namedlist_s *entry = *list;
	char *string;
	int length = 1;

	while (entry)
	{
		length += (int)strlen(entry->name) + 1 + (int)strlen(entry->data) + 1;
		entry = entry->next;
	}

	if (length == 1)
	{
		return "";
	}

	entry = *list;
	string = malloc(length);
	string[0] = 0;

	while (entry)
	{
		strcat(string, entry->name);
		length = (int)(strlen(string));
		string[length] = 27;
		string[length+1] = 0;
		strcat(string, entry->data);
		length = (int)(strlen(string));
		string[length] = 27;
		string[length+1] = 0;
		entry = entry->next;
	}

	return string;
}

struct namedlist_s *NamedList_String2ToList(char *instring)
{
	struct namedlist_s *list = NULL;
	char *string, *start, *end, *name, *liststring;

	if (!instring)
	{
		return NULL;
	}

	string = strdup(instring);
	start = string;

	while (end = strchr(start, 27))
	{
		*end = '\0';
		name = start;
		start = end + 1;
		end = strchr(start, 27);
		if (!end)
		{
			break;
		}
		*end = '\0';
		liststring = start;
		start = end + 1;
		NamedList_Add(&list, name, strdup(liststring), NamedList_FreeString);
	}

	free(string);

	return list;
}

struct namedlist_s *NamedList_DupeStringList(struct namedlist_s *src)
{
	struct namedlist_s *dst = NULL;

	while (src)
	{
		NamedList_Add(&dst, src->name, strdup(src->data), NamedList_FreeString);

		src = src->next;
	}

	return dst;
}

struct namedlist_s *NamedList_DupeList(struct namedlist_s *src, void *(*dupedata)(void *), void (*destroydata)(void *))
{
	struct namedlist_s *dst = NULL;

	while (src)
	{
		NamedList_Add(&dst, src->name, dupedata(src->data), destroydata);

		src = src->next;
	}

	return dst;
}
