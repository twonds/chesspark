#ifndef __MENU_H__
#define __MENU_H__

void Menu_GetStatusLists(struct StatusList_s **outawaystatuslist, struct StatusList_s **outavailstatuslist);
void Menu_PopupPresenceMenu(struct Box_s *pbox);
void Menu_PopupPrefsMenu(struct Box_s *pbox);
void Menu_PopupAddMenu(struct Box_s *pbox);
void Menu_PopupListMenu(struct Box_s *pbox, char *name, char *group,
	int x, int y, int subscribedTo, int online, int chessparkfeatures,
	struct namedlist_s *gamesplaying, struct namedlist_s *gameswatching,
	struct adhoccommand_s *command);
void Menu_PopupChatboxPrefsMenu(struct Box_s *pbox, char *name, int drawervisible, int isGroupChat, char *oldtopic, int x, int y);
void Menu_PopupRosterBlankMenu(struct Box_s *pbox, int x, int y);
void Menu_PopupRoomsBlankMenu(struct Box_s *pbox, int x, int y);
void Menu_PopupRoomMenu(struct Box_s *pbox, char *jid, int x, int y);
void Menu_PopupGroupMenu(struct Box_s *pbox, char *name, int x, int y);
void Menu_PopupChatParticipantMenu(struct Box_s *pbox, char *jid, char *chatjid, char *nick, char *role, char *affiliation, int showmod, int showmessage, int x, int y);
void Menu_PopupChatboxAddMenu(struct Box_s *pbox, char *name, int x, int y);
void Menu_PopupGameMenu(struct Box_s *pbox, char *gameid, char *gamemuc, int gamelocal, int gameover, int x, int y);
void Menu_PopupTourneyMenu(struct Box_s *pbox, char *tourneyid, int x, int y, int isplayer, int ismanager, int tourneystarted);
void Menu_PopupURLMenu(struct Box_s *pbox, char *url, int x, int y);
void Menu_PopupProfileMenu(struct Box_s *pbox, char *name, int x, int y, struct namedlist_s *gamesplaying, struct namedlist_s *gameswatching);
void Menu_PopupEditMenu(struct Box_s *pbox, int x, int y, int canedit, int cancopy, int canselect);

void Menu_InitAvatarList();
void Menu_ClearAvatarMenu(struct Box_s *pbox);
void Menu_SetAvatarToBoxImg(struct Box_s *pbox);
void Menu_PopupAvatarMenu(struct Box_s *pbox);
void Menu_OnCommand(struct Box_s *pbox, int command);
void Menu_Init();
void Menu_RedoStatusMenu();
void Menu_SetStatusList(enum SStatus status, struct StatusList_s *list);

void Menu_CheckPrefsItems();

#endif