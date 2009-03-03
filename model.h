#ifndef __MODEL_H__
#define __MODEL_H__

#if 0
#define NUM_UIFLAGS 6

enum uiflags_e
{
	UIFLAG_UIFLAGSET         = 0x00000001,
	UIFLAG_SHOWFRIENDOFFLINE = 0x00000002,
	UIFLAG_SHOWFRIENDGROUPS  = 0x00000004,
	UIFLAG_SHOWFRIENDAVATARS = 0x00000008,
	UIFLAG_SHOWFRIENDSTAB    = 0x00000010,
	UIFLAG_SHOWROOMSTAB      = 0x00000020,
	UIFLAG_SHOWGAMESTAB      = 0x00000040,
	UIFLAG_AUTOAPPROVEFRIEND = 0x00000080,
	UIFLAG_AUTOAWAY          = 0x00000100,
	UIFLAG_DONTSHOWWELCOME   = 0x00000200,
	UIFLAG_CHATSHOWNOTICES   = 0x00000400,
	UIFLAG_CHATNOTICESCLOSED = 0x00000800,
	UIFLAG_HIDEPARTICIPANTS  = 0x00001000,
	UIFLAG_NOALERTONMESSAGE  = 0x00002000,
	UIFLAG_NOALERTONGAME     = 0x00004000,
	UIFLAG_GAMENOAUTOFLAG    = 0x00008000,
	UIFLAG_NOINITIALROSTER   = 0x00010000,
};
#endif

enum modelmsgtypes_e
{
	MODELMSG_STARTROSTER      = 0,
	MODELMSG_ADDROSTERENTRY,
	MODELMSG_FINISHROSTER,
	MODELMSG_ASKSUBSCRIBE,
	MODELMSG_SETSUBSCRIBED,
	MODELMSG_ASKUNSUBSCRIBE,
	MODELMSG_SETUNSUBSCRIBED,
	MODELMSG_HANDLEPRESENCE,
	MODELMSG_HANDLEMESSAGE,
	MODELMSG_NUMMESSAGES      = 0xFF
};

void Model_Init();
void Model_Poll();

/* All ModelConn_* functions should only be called from conn.c. */
void ModelConn_Login(char *jid);
void ModelConn_PurgeRoster();
void ModelConn_SetFriend(char *jid, char *nickname, struct namedlist_s *newgroups, int subscribedTo, int subscribedFrom, int inroster);
void ModelConn_SetFriendPresence(char *jid, int ismuc, enum SStatus status,
	char *statusmsg, char *role, char *affiliation, char *realjid, int statuscode,
	char *vcardphotohash, char *nickchange, struct namedlist_s *roleslist,
	struct namedlist_s *titleslist, int prioritynum);
void ModelConn_AskApprove(char *jid);
void ModelConn_AddChatMessage(char *jid, char *msg, char *msgid, int reqcomposing);
void ModelConn_LostConnection();

void Model_SetLogin(int remember, char *jid, char *pass);
void Model_ChangeUser();
void Model_SetPresence(enum SStatus status, char *statusmsg);

void Model_AddFriend(char *jid, char *nickname);
void Model_RemoveFriend(char *jid);
void Model_SyncFriend(char *jid);

void Model_AddFriendToGroup(char *jid, char *groupname);
void Model_RemoveFriendFromGroup(char *jid, char *groupname);
void Model_MoveFriendFromGroupToGroup(char *jid, char *fromgroup, char *togroup);

void Model_RemoveGroup(char *groupname);
void Model_RemoveGroupAndContents(char *groupname);
void Model_RenameGroup(char *oldname, char *newname);

void Model_CancelLogin();
void Model_Disconnect();
void Model_AddStatus(enum SStatus status, char *statusmsg);
void Model_SetStatusList(enum SStatus status, struct StatusList_s *list);
char *Model_GetDefaultNick();

void Model_SetFriendAvatarHash(char *jid, char *hash, int vcard, int retrieve);

void ModelConn_AddChatroom(char *jid, char *name, char *topic, int users);
void ModelConn_AddChatroomError(char *jid);
void ModelConn_ParseRoomNames(struct namedlist_s *newroomlist);

void ModelConn_AddGroupChatMessage(char *jid, char *msg, char *timestamp);
void ModelConn_RoomNickConflict(char *jid);

void Model_GetRooms();
void Model_ChangeNick(char *chatjid, char *newnick);
void Model_KickUser(char *chatjid, char *nick, char *reason);
void Model_BanUser(char *chatjid, char *nick, char *reason);
void Model_UnbanUser(char *chatjid, char *nick, char *reason);
void Model_InviteUser(char *chatjid, char *userjid, char *reason);
void Model_GiveVoice(char *chatjid, char *nick, char *reason);
void Model_GiveMod(char *chatjid, char *userjid, char *reason);
void Model_GiveAdmin(char *chatjid, char *userjid, char *reason);
void Model_GiveOwner(char *chatjid, char *userjid, char *reason);
void Model_RevokeVoice(char *chatjid, char *nick, char *reason);
void Model_RevokeMod(char *chatjid, char *userjid, char *reason);
void Model_RevokeAdmin(char *chatjid, char *userjid, char *reason);
void Model_RevokeOwner(char *chatjid, char *userjid, char *reason);
void Model_AddChatToFavorites(char *chatjid);
void Model_RemoveChatFromFavorites(char *chatjid);
int Model_IsChatInFavorites(char *chatjid);
void Model_SendMessage(char *jid, char *text);
void ModelConn_JoinGame(char *gameid, char *room, char *role, char *oldgameid);
void ModelConn_ParseGameMove(char *gameid, char *move, char *annotation, char *whitejid, char *blackjid, int correspondence, int numtakebacks, int ply);
void ModelConn_ResetGamePosition(char *gameid, int illegalmove);
void ModelConn_ParseGameState(char *gameid, char *state);
void ModelConn_SyncClock(char *gameid, char *time, char *side, char *control, int tick);
void ModelConn_SetClockControl(char *gameid, char *side, int *controlarray);

void ModelConn_AddSearchGame(char *itemid, char *node, char *jid, struct gamesearchinfo_s *info);

void Model_RequestMatch(char *opponentjid, struct gamesearchinfo_s *info);

void ModelConn_RequestMatch(char *from, struct gamesearchinfo_s *info, char *oldgameid);
void ModelConn_SetProfile(char *jid, struct namedlist_s *profileinfo);
void ModelConn_MucError(char *from, char *code);

void ModelConn_SetWhitePlayer(char *gameid, char *name);
void ModelConn_SetBlackPlayer(char *gameid, char *name);

void Model_SetAvatar(char *filename);

void Model_PopupChatDialog(char *jid, int isGroupChat);
void Model_SetShowFriendGroups(int showfriendgroups);

char *Model_GetFriendNick(char *jid);

void Model_ShowProfile(char *jid);
void Model_SaveProfile(char *jid, char *nick);
void Model_LeaveChat(char *chatjid);
void Model_RequestRematch(char *opponentjid);

void ModelConn_RequestReconvene(char *from, char *gameid, struct gamesearchinfo_s *info);
void ModelConn_SetSubscribed(char *jid);
void ModelConn_OpenAllGroup();

void ModelConn_SetComposing(char *jid, char *msgid);
void ModelConn_UnsetComposing(char *jid, char *msgid);

void Model_SetComposing(char *jid);
void Model_UnsetComposing(char *jid);

void ModelConn_SetRating(char *jid, struct rating_s *rating, char *category, char *from);
void ModelConn_ShowGameMessage(char *jid, char *gameid, struct gamesearchinfo_s *info);
void ModelConn_AddMoveToMoveList(char *gameid, char *move, char *annotation);
void ModelConn_SetGameState(char *gameid, char *initialstate, char *state,
        struct gamesearchinfo_s *info,
        char *whitecurrentclock, char *blackcurrentclock,
        char *tourneyid);

void ModelConn_HandleResign(char *gameid);
void ModelConn_HandleAdjourn(char *gameid, char *white, char *black);
void ModelConn_HandleDraw(char *gameid, char *from, int whiteaccept, int blackaccept, int correspondence);
void ModelConn_HandleRejectDraw(char *gameid, char *from, int correspondence);
void ModelConn_HandleRejectAdjourn(char *gameid);
void ModelConn_HandleGameOver(char *gameid, char *whitejid, char *blackjid, char *type, char *win, char *lose, char *reason, int correspondence, char *datestarted);
void ModelConn_HandleTotalGamesCount(int count);

void ModelConn_HandlevCardPhoto(char *jid, char *type, char *binvaldata, int len);

void ModelConn_RequestMatchError(char *opponentjid, char *error);
void ModelConn_ReconveneError(char *gameid, char *error);

void ModelConn_NoSearchGameResults();
void ModelConn_RefreshGameSearch();
void ModelConn_SetFriendHasChesspark(char *jid);

void Model_PopupReconvene(char *gameid, struct gamesearchinfo_s *info);
void Model_SetUIFlag(enum uiflags_e flagchange, int state);
char *Model_GetUserRegString(char *key, char *in, int size);
void Model_SetUserRegString(char *key, char *value);

char *Model_GetLoginJid();

void Model_SubscribeProfile(char *jid, void (*callbackfunc)(char *, struct profile_s *, void *), void *userdata);
void Model_UnsubscribeProfile(char *jid, void (*callbackfunc)(char *, struct profile_s *, void *), void *userdata);
void Model_SubscribeProfileNoRequest(char *jid, void (*callbackfunc)(char *, struct profile_s *, void *), void *userdata);
void Model_SubscribeProfileForceRequest(char *jid, void (*callbackfunc)(char *, struct profile_s *, void *), void *userdata);
void Model_SubscribeProfileAndAvatar(char *jid, void (*callbackfunc)(char *, struct profile_s *, void *), void *userdata);

char *Model_GetFriendAvatarHash(char *jid);
struct namedlist_s *Model_GetCustomTimeControls();
void Model_RemoveSavedCustomTimeControl(char *name);

void Model_SetPriority(int priority);
void Model_JoinGroupChat(char *jid, char *nickname);
void Model_SetTopic(char *chatjid, char *topic);
void Model_AddChatToAutojoin(char *chatjid);
void Model_RemoveChatFromAutojoin(char *chatjid);
int Model_IsChatInAutojoin(char *chatjid);
void Model_JoinTournament(char *tourneyid);
void Model_ForfeitTournament(char *tourneyid);
void Model_SetLocalProfileDescription(char *description);
void Model_AdHocCommand(char *jid, char *node);

void ModelConn_GameRequestError(char *opponentjid, char *gameid, char *error, char *actor, char *adid);
void ModelConn_FinishRoster();
void ModelConn_ClearMoveList(char *gameid);
int Model_HasFriend(char *jid);
void ModelConn_ShowChatInviteMessage(char *friendjid, char *chatjid);
void ModelConn_SetGroupChatTopic(char *jid, char *topic);
void ModelConn_ShowAnnouncement(char *from, char *text, char *subject);
void ModelConn_ShowClientUpgradeMessage(char *from, char *number, char *version, char *build, char *url);
void ModelConn_SetGameStatus(char *jid, char *gameid, struct gamesearchinfo_s *info, int stopped, int watching, int update, int clear);
void ModelConn_ShowCorGameAccepted(char *gameid, char *from);
void ModelConn_SetAdjournedCount(int adjourned);
void ModelConn_SetCorrespondenceCount(int correspondence);
void ModelConn_ShowLoginMessage();
void ModelConn_TournamentAddPlayer(char *tourneyid, char *playerjid);
void ModelConn_TournamentUpdatePlayer(char *tourneyid, char *jid, struct tournamentplayerinfo_s *pinfo);
void ModelConn_TournamentRemovePlayer(char *tourneyid, char *playerjid);
void ModelConn_TournamentStartRound(char *tourneyid, int round, struct namedlist_s *pairinglist);
void ModelConn_TournamentEndRound(char *tourneyid, int round);
void ModelConn_TournamentEnd(char *tourneyid, char *winner);
void ModelConn_TournamentGamePlaying(char *tourneyid, char *gameid, int round, char *white, char *black);
void ModelConn_TournamentGameStoppedPlaying(char *tourneyid, char *gameid, int round, char *white, char *black, char *winner);
void ModelConn_ShowGameLogoutMessage(char *jid);
void ModelConn_AskUnsubscribe(char *jid);
void ModelConn_SetUnsubscribed(char *jid);
void ModelConn_RequestMatchSetID(char *opponentjid, char *gameid);
void ModelConn_GameRematchError(char *oldgameid, char *error, char *actor);
void ModelConn_RematchSetID(char *oldgameid, char *newgameid);
void ModelConn_SetSharedRosterFlag();
int Model_IsLoginJid(char *jid);
void ModelConn_ClearGamesSearch(char *node);
void ModelConn_AddSearchTournament(struct tournamentinfo_s *info);
void ModelConn_FinishGameResults(int noresults);
void ModelConn_SetTournamentInfo(struct tournamentinfo_s *info);

enum SStatus Model_GetBestStatus(char *jid);

void Model_SetOption(char *option, int state, char *data);
int Model_GetOption(char *option);
struct namedlist_s *Model_GetOptions();

void Model_LaunchUpdater();

void Model_ReallyDisconnect();
void Model_Quit();
int Model_JidInRoster(char *jid);
int Model_IsJIDIgnored(char *jid);
void Model_SetIgnoreList(struct namedlist_s *ignorelist);
void Model_SetOption(char *option, int state, char *data);
void Model_SetAllOptions(struct namedlist_s *newoptions);
struct namedlist_s *Model_GetIgnoreList();

struct gamesearchinfo_s *Model_GetSearchFilterPref(char *node);
void Model_SetSearchFilterPref(char *node, struct gamesearchinfo_s *info);

int Model_GetChessparkLogin();
void Model_TestDisconnect();
int Model_GetPermission(char *permission);
int Model_CustomTimeControlExists(char *name);
void Model_SaveCustomTimeControl(char *name, struct timecontrol_s *whitetc, struct timecontrol_s *blacktc);
void Model_SetLoginLast();
void Model_LoginToChesspark();
void Model_Ping(char *jid, int show);
void Model_AddJIDToIgnore(char *jid);
void Model_RemoveJIDFromIgnore(char *jid);
struct gamesearchinfo_s *Model_HaveAdjournedGameWithJid(char *jid);

void ModelConn_HandleClientUpgradeMessage(char *from, char *number, char *version, char *build, char *url);
void ModelConn_ShowMemberExpired();
void ModelConn_ShowMemberNotFound();
void ModelConn_ShowMemberLoginsExceeded();
void Model_SetChessparkExpiry(char *expiretype, char *expiredate);
void ModelConn_OnChessparkLogin(char *membertype, struct namedlist_s *permissions, struct namedlist_s *rooms);

void Model_QueueMessage(struct mpmessage_s *msg, unsigned int priority);

void ModelConn_HandleTakeback(char *gameid, char *white, char *black);
void ModelConn_HandleRejectTakeback(char *gameid);
void ModelConn_SetGameMoveList(char *gameid, struct namedlist_s *textmovelist);
void ModelConn_HandleAbort(char *gameid, char *white, char *black);
void ModelConn_HandleRejectAbort(char *gameid);
void ModelConn_InviteUserError(char *roomjid);
void ModelConn_SetProfileDescription(char *jid, char *description);
void ModelConn_AddPushGame(char *type, char *id, struct gamesearchinfo_s *info);
void ModelConn_RemovePushGame(char *type, char *id);
void ModelConn_SetTopicError(char *roomjid);
void ModelConn_MucModerationError(char *roomjid);
void ModelConn_MoveSuccess(char *gameid, unsigned int lag);
void ModelConn_GenericError(char *errortitle, char *error);
void ModelConn_AddAdSuccess(char *id);
void ModelConn_HandleBanlist(char *jid, struct namedlist_s *banlist);
void ModelConn_HandleBanlistError(char *chatjid, char *code);
void ModelConn_HandlePing(char *jid, int pingtime, int repeat);
void ModelConn_HandlePingError(char *jid);

void ModelConn_HandlePrivacyListNames(char *activename, char *defaultname, struct namedlist_s *listnames);
void ModelConn_HandlePrivacyList(char *listname, struct namedlist_s *privacylist);
void ModelConn_HandleChessparkPrivacyList(struct namedlist_s *privacylist);
void ModelConn_HandlePrefs(struct namedlist_s *infavoritechats,
	struct namedlist_s *inautojoinchats,
	struct namedlist_s *inavailablestatuses,
	struct namedlist_s *inawaystatuses,
	struct namedlist_s *incustomtimecontrols,
	struct namedlist_s *inoptions);

struct profile_s *Model_GetProfile(char *jid, int request);

int Model_IsFreeMember();

#endif
