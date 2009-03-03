#ifndef __ROOMENTRY_H__
#define __ROOMENTRY_H__

#include "drop.h"

BOOL RoomEntry_VisibleFunc(struct Box_s *pbox);
void RoomEntry_CountUsers(struct Box_s *pbox, int *users);
void RoomEntry_CountRooms(struct Box_s *pbox, int *rooms);
struct Box_s *RoomEntry_Create(int x, int y, int w, int h, enum Box_flags flags, struct Box_s *edit, char *jid, char *name, char *topic, int users);
int RoomEntry_SortFunc(struct Box_s *lbox, struct Box_s *gbox);

#endif