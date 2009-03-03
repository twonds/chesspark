#ifndef __CTRL_H__
#define __CTRL_H__
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

void Ctrl_Init(void);
void Ctrl_Start();

void Ctrl_Login(int remember, char *jid, char *pass);
void Ctrl_LoginCallback();
void Ctrl_CancelLogin();
void Ctrl_LoginLast();

void Ctrl_Disconnect();

void Ctrl_ChangeUser();

void Ctrl_SetPresence(enum SStatus sstatus, char *statusmsg);
void Ctrl_SendPresence();

void Ctrl_GetRoster();
void Ctrl_GetRooms();

void Ctrl_PurgeRoster();
void Ctrl_AddFriendFromRoster(char *jid, char *name, char *group, BOOL subscribedTo, BOOL subscribedFrom);

void Ctrl_AddFriend(char *jid, char *name);
void Ctrl_RemoveFriend(char *jid);
void Ctrl_RequestFriend(char *jid);
void Ctrl_ApproveFriend(char *jid);
void Ctrl_RejectFriend(char *jid);

void Ctrl_AddFriendToGroup(char *jid, char *group);
void Ctrl_RemoveFriendFromGroup(char *jid, char *group);
void Ctrl_MoveFriendFromGroupToGroup(char *jid, char *fromgroup, char *togroup);

void Ctrl_SendMessage(char *jid, char *text);
void Ctrl_SendGroupMessage(char *tojid, char *text);

BOOL Ctrl_Poll(void);

void Ctrl_RemoveGroup(char *name);
void Ctrl_RemoveGroupAndContents(char *name);
void Ctrl_RenameGroup(char *oldname, char *newname);

char *Ctrl_GetDefaultNick();

void Ctrl_JoinGroupChatDefaultNick(char *jid);
char *Ctrl_GetAvatarFilenameByHash(char *hash, char *buffer, int *bufferlen);

void Ctrl_AddStatus(enum SStatus status, char *statusmsg);
void Ctrl_SetStatusList(enum SStatus status, struct StatusList_s *list);

void Ctrl_ChangeNick(char *chatjid, char *newnick);
void Ctrl_KickUser(char *chatjid, char *nick, char *reason);
void Ctrl_BanUser(char *chatjid, char *nick, char *reason);
void Ctrl_UnbanUser(char *chatjid, char *nick, char *reason);
void Ctrl_InviteUser(char *chatjid, char *userjid, char *reason);
void Ctrl_GiveVoice(char *chatjid, char *nick, char *reason);
void Ctrl_RevokeVoice(char *chatjid, char *nick, char *reason);
void Ctrl_GiveMod(char *chatjid, char *userjid, char *reason);
void Ctrl_RevokeMod(char *chatjid, char *userjid, char *reason);
void Ctrl_GiveAdmin(char *chatjid, char *userjid, char *reason);
void Ctrl_RevokeAdmin(char *chatjid, char *userjid, char *reason);
void Ctrl_GiveOwner(char *chatjid, char *userjid, char *reason);
void Ctrl_RevokeOwner(char *chatjid, char *userjid, char *reason);
void Ctrl_AddChatToFavorites(char *chatjid);
void Ctrl_RemoveChatFromFavorites(char *chatjid);
int Ctrl_IsChatInFavorites(char *chatjid);
void Ctrl_RequestMatch(char *opponentjid, struct gamesearchinfo_s *info);
void Ctrl_DeclineMatch(char *gameid);
void Ctrl_DeclineRematch(char *gameid, char *oldgameid);

void Ctrl_SendMove(char *gameid, char *move);
void Ctrl_WatchGame(char *jid, int rotate);
void Ctrl_WatchGame2(struct Box_s *pbox, char *gameid);

void Ctrl_PostGameAd(struct gamesearchinfo_s *info);
void Ctrl_RequestProfile(char *jid);
void Ctrl_AcceptMatch(char *gameid);
void Ctrl_AcceptRematch(char *gameid, char *oldgameid);
void Ctrl_RemoveGameAd(char *itemid);
void Ctrl_RequestGameSearch(char *node, struct gamesearchinfo_s *info);

void Ctrl_ShowProfile(char *jid);
void Ctrl_ShowProfile2(struct Box_s *pbox, char *jid);
void Ctrl_SaveProfile(char *jid, char *nick);

void Ctrl_AcceptCorMatch(char *gameid);

void Ctrl_SendCorMove(char *gameid, char *move);

void Ctrl_LeaveChat(char *chatjid);

void Ctrl_RequestRematch(char *opponentjid);

void Ctrl_SetTopic(char *chatjid, char *topic);
void Ctrl_SetTopicCallback(char *topic, char *chatjid);

void Ctrl_LoginToGameServer();

void Ctrl_SetTournamentRoundStart(char *tourneyid, char *round, char *timestamp);

void Ctrl_SetProfileDescription(char *description);

int Ctrl_IsChatInAutojoin(char *chatjid);

void Ctrl_InviteWatchGame(char *friendjid, char *gameid);

void Ctrl_AdHocCommand(char *jid, char *node);

void Ctrl_AddChatToAutojoin(char *chatjid);
void Ctrl_RemoveChatFromAutojoin(char *chatjid);
void Ctrl_JoinTournament(char *id);
void Ctrl_ForfeitTournament(char *id);

void Ctrl_StartNextTournamentRound(char *tourneyid);
void Ctrl_InviteToChat(char *friendjid, char *chatjid);
void Ctrl_ViewTournament(char *id);

void Ctrl_PlayNow();
void Ctrl_AddTimeCallback(char *time, char *gameid);

void Ctrl_PostDebugLog(char *reason, void *dummy);
void Ctrl_PostProblem(char *type, char *prob, int log);

void Ctrl_SendGameFlag(char *gameid, int correspondence);
void Ctrl_SendGameAdjudication(char *gameid, int correspondence);
void Ctrl_SendGameAdjourn(char *gameid, int correspondence);
void Ctrl_SendGameDraw(char *gameid, int correspondence);
void Ctrl_SendGameAbort(char *gameid, int correspondence);
void Ctrl_SendGameTakeback(char *gameid, int correspondence);
void Ctrl_SendGameResign(char *gameid, int correspondence);
void Ctrl_RejectGameAdjourn(char *gameid, int correspondence);
void Ctrl_RejectGameAbort(char *gameid, int correspondence);
void Ctrl_RejectGameTakeback(char *gameid, int correspondence);
void Ctrl_RejectGameDraw(char *gameid, int correspondence);

void Ctrl_Ping(char *jid, int show, int repeat);
void Ctrl_Ignore(char *jid);
void Ctrl_Unignore(char *jid);

void Ctrl_RequestGameSearch(char *node, struct gamesearchinfo_s *info);
void Ctrl_SetPushGamesActivation(int active);
void Ctrl_SetPushFilter(char *node, struct gamesearchinfo_s *info);

void Ctrl_PlayAd(char *adid);
void Ctrl_GameReconvene(char *gameid);
void Ctrl_GetCorGameState(char *gameid);
void Ctrl_InviteToGame(char *jid);
void Ctrl_RequestBanlist(char *jid);
void Ctrl_SendCorLookAheadMove(char *gameid, char *move);

#endif