#ifndef __LIST_H__
#define __LIST_H__

struct Box_s *List_Create(int x, int y, int w, int h, enum Box_flags flags, int allgroup);

void List_AddGroup(struct Box_s *pbox, char *name);
void List_AddCustomGroup(struct Box_s *pbox, char *name, struct Box_s *groupbox);
void List_RenameGroup(struct Box_s *pbox, char *oldname, char *newname);
void List_RemoveGroupByName(struct Box_s *pbox, char *name);

void List_AddEntry(struct Box_s *pbox, char *entryname, char *groupname, struct Box_s *entrybox);
void List_RemoveEntry(struct Box_s *pbox, struct listentry_s **entry);
void List_RemoveEntryByName(struct Box_s *pbox, char *entryname, char *groupname);
void List_RemoveEntryByNameAllGroups(struct Box_s *pbox, char *entryname);

void List_RedoEntries(struct Box_s *pbox);

struct Box_s *List_GetEntryBox(struct Box_s *pbox, char *entryname, char *groupname);
struct Box_s *List_GetEntryBoxAllGroups(struct Box_s *pbox, char *entryname);

void List_RemoveAllEntries(struct Box_s *pbox);
void List_ReinsertEntry(struct Box_s *pbox, char *entryname, char *groupname);

void List_ScrollToTop(struct Box_s *pbox);
void List_ScrollToBottom(struct Box_s *pbox);
void List_ScrollVisible(struct Box_s *pbox);

void List_SetShowGroups(struct Box_s *pbox, int show);
void List_OpenGroup(struct Box_s *pbox, char *groupname);

void List_SetSelectionToEntry(struct Box_s *pbox, char *entryname, char *groupname);

void List_SetEntrySortFunc(struct Box_s *pbox, BOOL (*entrysortfunc)(struct Box_s *, struct Box_s *));
void List_SetEntryVisibleFunc(struct Box_s *pbox, BOOL (*entryvisiblefunc)(struct Box_s *));
void List_CallEntryFunc(struct Box_s *pbox, void (*func)(struct Box_s *, void *), void *userdata);
void List_CallEntryFuncOnGroup(struct Box_s *pbox, char *groupname, void (*func)(struct Box_s *, void *), void *userdata);
struct Box_s *List_GetGroupBox(struct Box_s *pbox, char *groupname);
void List_SetEmptyRClickFunc(struct Box_s *pbox, void (*listrclickfunc)(struct Box_s *list, int xmouse, int ymouse));
void List_SetGroupRMenuFunc(struct Box_s *pbox, void (*listgrouprclickfunc)(struct Box_s *list, char *groupname, int xmouse, int ymouse));

void List_SetStripeBG1(struct Box_s *pbox, int col);
void List_SetStripeBG2(struct Box_s *pbox, int col);
void List_SetStripeBG(struct Box_s *pbox, int col);

void List_SetEntrySelectable(struct Box_s *pbox, int selectable);
void List_SetGroupSelectable(struct Box_s *pbox, int selectable);

void List_SetGroupCollapsible(struct Box_s *pbox, int collapsible);
void List_SetStickyBottom(struct Box_s *pbox, int stickybottom);
void List_SetHideDisclosureTriangles(struct Box_s *pbox, int hidetriangles);
void List_SetSortGroups(struct Box_s *pbox, int sort);
int List_HasVisibleEntry(struct Box_s *pbox);

int List_IsAtBottom(struct Box_s *pbox);
void List_SetLastText(struct Box_s *list, struct Box_s *text);
struct Box_s *List_GetLastText(struct Box_s *list);
void List_RemoveLastText(struct Box_s *list, struct Box_s *remove, struct Box_s *replace);

void List_SetOnGroupDragDrop(struct Box_s *list, void (*ongroupdragdrop)(struct Box_s *, struct Box_s *, int, void *, char *));
void List_ScrollPageUp(struct Box_s *pbox);
void List_ScrollPageDown(struct Box_s *pbox);
void List_ScrollEntryVisible(struct Box_s *list, char *entryname, char *groupname);
void List_SetEntryLimit(struct Box_s *list, int numentries);

void* List_GetEntrySortFunc(struct Box_s *pbox);
void List_Resort(struct Box_s *list);

#endif