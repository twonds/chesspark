#ifndef __FRIENDENTRY_H__
#define __FRIENDENTRY_H__

#include "drop.h"

struct FriendDropData_s
{
	char *jid;
	char *groupname;
};

struct Box_s *FriendEntry_CreateReal(int x, int y, int w, int h, char *jid, char *nickname, char *groupname, enum SStatus status, char *statusmsg);
#if 1
#define FriendEntry_Create(a, b, c, d, e, f, g, h, i) FriendEntry_CreateReal(a, b, c, d, e, f, g, h, i)
#else
#define FriendEntry_Create(a, b, c, d, e, f, g, h, i) FriendEntry_CreateWrap(a, b, c, d, e, f, g, h, i, __FILE__, __LINE__)
#endif

void FriendEntry_SetStatus(struct Box_s *pbox, char *nickname,
	enum SStatus status, char *statusmsg, char *avatarhash, char *rating,
	struct namedlist_s *roles, struct namedlist_s *titles,
	char *gameid, struct gamesearchinfo_s *info, int stopped, int watching,
	struct adhoccommand_s *command);
BOOL FriendEntry_SortFunc(struct Box_s *lesser, struct Box_s *greater);
BOOL FriendEntry_VisibleFunc(struct Box_s *pbox);
void FriendEntry_SetShowIcon(int show);
void FriendEntry_SetShowOffline(int show);

void FriendEntry_Refresh(struct Box_s *pbox, void *userdata);

#endif