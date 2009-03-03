#ifndef __NAMEDLIST_H__
#define __NAMEDLIST_H__

struct namedlist_s
{
	struct namedlist_s *next;
	char *name;
	void *data;
	void (*destroydata)(void *);
};

struct namedlist_s **NamedList_Add(struct namedlist_s **list, char *name, void *data, void (*destroydata)(void *));
struct namedlist_s **NamedList_AddToTop(struct namedlist_s **list, char *name, void *data, void (*destroydata)(void *));

void NamedList_Remove(struct namedlist_s **list);
struct namedlist_s **NamedList_GetByName(struct namedlist_s **list, char *name);
struct namedlist_s **NamedList_GetByNameAndBump(struct namedlist_s **list, char *name);
struct namedlist_s **NamedList_GetNextByName(struct namedlist_s **list, char *name);
void NamedList_RemoveByName(struct namedlist_s **list, char *name);
void NamedList_RemoveLast(struct namedlist_s **list);
void NamedList_Unlink(struct namedlist_s **list);
void NamedList_Destroy(struct namedlist_s **list);
void NamedList_Destroy2(struct namedlist_s *list);
void NamedList_Sort(struct namedlist_s **entry, int (*sortfunc)(struct namedlist_s *lesser, struct namedlist_s *greater, void *userdata), void *userdata);
void NamedList_SortByName(struct namedlist_s **entry);

char *NamedList_ListToString(struct namedlist_s **list);
char *NamedList_ListToString2(struct namedlist_s **list);
void NamedList_FreeString(char *string);
void NamedList_AddString(struct namedlist_s **list, char *name, char *string);
struct namedlist_s *NamedList_StringToList(char *instring);
struct namedlist_s *NamedList_String2ToList(char *instring);
struct namedlist_s *NamedList_DupeStringList(struct namedlist_s *src);
struct namedlist_s *NamedList_DupeList(struct namedlist_s *src, void *(*dupedata)(void *), void (*destroydata)(void *));

#endif