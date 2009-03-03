#ifndef __CHATBOX_H__
#define __CHATBOX_H__

struct Box_s *ChatBox_Create(int x, int y, int w, int h, enum Box_flags flags, struct Box_s *roster, int group);
struct Box_s *ChatBox_AddText(struct Box_s *pbox, char *targetjid, char *name, char *text, char *timestamp, int notify);
void ChatBox_AddCustom(struct Box_s *chatbox, char *targetjid, struct Box_s *custombox, int notify);

void ChatBox_SetParticipantStatus(struct Box_s *pbox, char *targetjid,
  char *name, enum SStatus status, char *statusmsg, char *role,
  char *affiliation, char *realjid, char *nickchange, struct namedlist_s *roleslist,
  struct namedlist_s *titleslist, char *actorjid, char *reason, int notactivated,
  char *membertype);
void ChatBox_ToggleShowParticipants(struct Box_s *pbox);

int ChatBox_HasChat(struct Box_s *pbox, char *jid, int isGroupChat);
void ChatBox_ActivateChat(struct Box_s *pbox, char *targetjid, char *nick, int isGroupChat, int selecttab);
void ChatBox_MoveChat(struct Box_s *pboxsrc, struct Box_s *pboxdst, char *jid);

void ChatBox_RoomNickConflict(struct Box_s *pbox, char *roomjid, char *nick);

void ChatBox_OpenDrawer(struct Box_s *pbox);
void ChatBox_ReconnectChats(struct Box_s *chatbox, int chessparkchats);

void ChatBox_SetComposing(struct Box_s *chatbox, char *jid, char *msgid);
void ChatBox_UnsetComposing(struct Box_s *chatbox, char *jid, char *msgid);

void ChatBox_QuickDrawerOpen(struct Box_s *pbox, int w);
void ChatBox_ShowDisconnect(struct Box_s *chatbox);
void ChatBox_SetStatusOnAllChats(struct Box_s *chatbox, enum SStatus status,
	char *statusmsg);
void ChatBox_SetChatTopic(struct Box_s *pbox, char *targetjid, char *name, char *topic);
void ChatBox_UpdateTitlebar(struct Box_s *chatbox);
int ChatBox_LocalUserHasMucPower(struct Box_s *pbox, char *mucjid);
int ChatBox_GetChatboxGroup(struct Box_s *chatbox);

void ChatBox_SetDisconnect(struct Box_s *chatbox);
void ChatBox_ShowDisconnect(struct Box_s *chatbox);
int ChatBox_IsInRoom(struct Box_s *chatbox, char *roomjid);
void ChatBox_InitialIMSound();
void ChatBox_MucError(struct Box_s *chatbox, char *roomjid, int icode, char *error);
void ChatBox_UpdateTitlebarIcon(struct Box_s *chatbox);

#endif