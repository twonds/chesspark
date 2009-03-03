#ifndef __SPAWN_H__
#define __SPAWN_H__

struct Box_s *Spawn_Create(int x, int y, int w, int h, enum Box_flags flags);
void Spawn_AddTab(struct Box_s *pbox, char *name, struct Box_s *target);
void Spawn_RemoveTab(struct Box_s *pbox, char *name);
void Spawn_ActivateFirstTab(struct Box_s *pbox);
struct Box_s *Spawn_GetTabContentBox(struct Box_s *spawn, char *name);

#endif