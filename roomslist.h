#ifndef __ROOMSLIST_H__
#define __ROOMSLIST_H__

struct Box_s *RoomsList_Create();

void RoomsList_AddChatroom(struct Box_s *roomslist_box, char *group, char *jid, char *name, char *topic, int users);
void RoomsList_RemoveChatroom(struct Box_s *roomslist_box, char *group, char *jid);
void RoomsList_ClearChatRooms(struct Box_s *roomslist_box);
void RoomsList_SetChatroomLoadStatusFinished(struct Box_s *roomslist_box);

#endif