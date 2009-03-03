#ifndef __CONN_H__
#define __CONN_H__

enum connstate_e
{
	CONN_NOTCONNECTED = 0,
	CONN_CONNECTED,
	CONN_CONNECTING,
	CONN_DISCONNECTING,
	CONN_DISCONNECTED
};

enum connerror_e
{
	CONNERROR_NONE = 0,
	CONNERROR_BADPASSWORD,
	CONNERROR_LOSTCONNECTION,
	CONNERROR_CANTCONNECT,
	CONNERROR_CONFLICT,
	CONNERROR_UNKNOWN
};

struct rosterentry_s
{
	char *jid;
	char *name;
	struct namedlist_s *groups;
	int subscribedto;
	int subscribedfrom;
	int pending;
};

struct presenceinfo_s
{
	char *jid;
	int ismuc;
	int status;
	char *statusmsg;
	char *role;
	char *affiliation;
	char *realjid;
	int statuscode;
	char *vcardphotohash;
	char *nickchange;
	struct namedlist_s *roleslist;
	struct namedlist_s *titleslist;
	int prioritynum;
	char *actorjid;
	char *reason;
	int notactivated;
	char *membertype;
};

struct messageinfo_s
{
	char *jid;
	char *text;
	char *msgid;
	char *timestamp;
	int ismuc;
	int reqcomposing;
};

void Conn_SetKeepAlive(int ka);

void Conn_Init();
void Conn_Reset();
void Conn_Poll();
void Conn_Login(char *jid, char *pass);
void Conn_GetRoster();
void Conn_SetFriend(char *jid, char *name, struct namedlist_s *grouplist);
void Conn_RemoveFriend(char *jid);
void Conn_SendMessage(char *tojid, char *text, int isGroupChat);
void Conn_SendPresence(enum SStatus fstatus, char *statusmsg, char *tojid, int ismuc, char *avatarhash, int prioritynum, int inclpriority);
void Conn_SetPresence(enum SStatus fstatus, char *statusmsg);
void Conn_RequestFriend(char *jid);
void Conn_ApproveFriend(char *jid);
void Conn_RejectFriend(char *jid);
void Conn_Disconnect();
int Conn_GetConnState();

void Conn_GetRooms();
void Conn_GetRoomInfo(char *jid);

void Conn_RequestInstantRoom(char *roomjid);

void Conn_GiveMod(char *roomjid, char *nick, char *comment);
void Conn_RevokeMod(char *roomjid, char *nick, char *comment);
void Conn_GiveVoice(char *roomjid, char *nick, char *comment);
void Conn_RevokeVoice(char *roomjid, char *nick, char *comment);
void Conn_KickUser(char *roomjid, char *nick, char *comment);
void Conn_GiveOwner(char *roomjid, char *userjid, char *comment);
void Conn_RevokeOwner(char *roomjid, char *userjid, char *comment);
void Conn_GiveAdmin(char *roomjid, char *userjid, char *comment);
void Conn_RevokeAdmin(char *roomjid, char *userjid, char *comment);
void Conn_BanUser(char *roomjid, char *userjid, char *comment);
void Conn_UnbanUser(char *roomjid, char *userjid, char *comment);
void Conn_InviteUser(char *roomjid, char *userjid, char *comment);
void Conn_SetNick(char *roomjid, char *nick);
void Conn_RequestInstantRoom(char *roomjid);
void Conn_RequestChatConfigForm(char *roomjid);

void Conn_RequestMatch(char *opponentjid, struct gamesearchinfo_s *info);
void Conn_AcceptMatch(char *gameid, char *oldgameid);
void Conn_GameReconvene(char *gameid);
void Conn_SendMove(char *gameid, char *movetext, int corgame);

void Conn_SendGameAdjourn(char *gameid, int correspondence);
void Conn_SendGameDraw(char *gameid, int correspondence);
void Conn_SendGameResign(char *gameid, int correspondence);

void Conn_RejectGameAdjourn(char *gameid, int correspondence);
void Conn_RejectGameDraw(char *gameid, int correspondence);

void Conn_RequestDiscoInfo(char *jid);
void Conn_RequestvCard(char *jid);
void Conn_RequestProfile(char *jid);
void Conn_SubscribeRating(char *jid);

void Conn_RequestGameSearch(char *node, struct gamesearchinfo_s *info);
void Conn_PostGameAd(struct gamesearchinfo_s *info);
void Conn_RemoveGameAd(char *itemid);

void Conn_SendComposing(char *jid, char *msgid);
void Conn_CancelComposing(char *jid, char *msgid);

void Conn_PublishvCard(unsigned char *avatardata, int avatarlen, char *avatartype);

void Conn_GetCorGameState(char *gameid);

void Conn_UnsubscribeFriend(char *jid);
void Conn_SetTopic(char *tojid, char *topic);
void Conn_JoinTournament(char *tourneyid, char *jid);
void Conn_ForfeitTournament(char *tourneyid, char *jid);

void Conn_AdHocCommand(char *jid, char *node);

void Conn_AcceptCorMatch(char *gameid);
void Conn_DeclineMatch(char *gameid, char *oldgameid);
void Conn_RequestRematch(char *gameid);
void Conn_GetTournamentInfo(char *tid);
void Conn_SetTournamentRoundStart(char *tourneyid, char *round, char *timestamp);
void Conn_StartNextTournamentRound(char *tourneyid);
void Conn_SetProfileField(char *fieldvar, char *valuecontent);
void Conn_GetPrivacyList(char *listname);
void Conn_GetServerPrefs();
void Conn_SetServerPrefs(struct namedlist_s *outfavoritechats,
	struct namedlist_s *outautojoinchats,
	struct namedlist_s *outavailablestatuses,
	struct namedlist_s *outawaystatuses,
	struct namedlist_s *outcustomtimecontrols,
	struct namedlist_s *outoptions);
void Conn_Ping(char *jid, int show, int repeat);
void Conn_PlayAd(char *adid);
void Conn_RespondAd(char *opponentjid, struct gamesearchinfo_s *info);
void Conn_SendAddTime(char *gameid, char *addtext);
void Conn_SendChessparkLogin();
void Conn_SendGameTag(char *gameid, char *ctag, int correspondence);
void Conn_SetActivePrivacyList(char *listname);
void Conn_SetBanlist(char *jid, struct namedlist_s *addban, struct namedlist_s *removeban);
void Conn_SetChessparkPrivacyList(struct namedlist_s *privacylist);
void Conn_SetPrivacyList(char *listname, struct namedlist_s *privacylist);
void Conn_SetPushFilter(char *node, struct gamesearchinfo_s *info);
void Conn_SetPushGamesActivation(int active);

void Conn_SubscribeProfile(char *jid);
void Conn_UnsubscribeProfile(char *jid);

void Conn_SubscribeRating(char *jid);
void Conn_UnsubscribeRating(char *jid);

void Conn_UnsubscribeFromFriend(char *jid);
void Conn_UnsubscribeToFriend(char *jid);

void Conn_GetChessparkPrivacyList();
void Conn_RequestBanlist(char *jid);
void Conn_SetBanlist(char *jid, struct namedlist_s *addban, struct namedlist_s *removeban);

#endif