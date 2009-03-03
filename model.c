#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>
#include <direct.h>

#include <windows.h>
#include <shellapi.h>

#include <strophe.h>

#include "box.h"

#include "audio.h"
#include "conn.h"
#include "ctrl.h"
#include "constants.h"
#include "cornerpop.h"
#include "httppost.h"
#include "i18n.h"
#include "info.h"
#include "log.h"
#include "menu.h"
#include "mp.h"
#include "namedlist.h"
#include "options.h"
/*#include "sha1.h"*/
#include "util.h"
#include "view.h"

#include "model.h"

#define CHESSPARKLOGIN_TIMEOUT 45000
#define CHESSPARKLOGIN_RETRIES 0

struct profilesubscription_s
{
	void (*callbackfunc)(char *, struct profile_s *, void *);
	void *userdata;
	int id;
};

char *login_jid = NULL;
char *login_pass = NULL;
int login_remember = 0;
int login_changed = 0;
int login_connected = 0;
int login_joinedchats = 0;
int login_fromloginwindow = TRUE;
int chessparklogin = 0;
struct namedlist_s *messageids = NULL;
struct namedlist_s *gameinfos = NULL;
char *avatarhash = NULL;
struct namedlist_s *profilecache = NULL;
struct namedlist_s *profilesubscriptions = NULL;
struct namedlist_s *customtimecontrols = NULL;
struct namedlist_s *local_permissions = NULL;
char *local_membertype = NULL;
int local_notactivated = 0;
int currentpriority = 1;
struct namedlist_s *localoptions;
int localautoawaytime;

enum SStatus userstatus = SSTAT_AVAILABLE;
char *userstatusmsg = NULL;

int autoaway = 0;
char *autoawayolduserstatusmsg;

static struct StatusList_s *awaystatuslist  = NULL;
static struct StatusList_s *availstatuslist = NULL;

int discocause = 0;

struct namedlist_s *favechats = NULL;
struct namedlist_s *autojoinchats = NULL;
struct namedlist_s *chatactivejids = NULL;
struct namedlist_s *sentautoaway = NULL;
struct namedlist_s *localprivacylist = NULL;
int localprivacylistloaded = 0;
struct namedlist_s *matchprivacylist = NULL;
int matchprivacylistloaded = 0;
struct namedlist_s *localgroupslist = NULL;
int inhiddengroup = 0;

/*
struct namedlist_s *pushplaygames = NULL;
struct namedlist_s *pushwatchgames = NULL;
*/
struct namedlist_s *adjournedgames = NULL;

struct resourcedata_s
{
	enum SStatus status;
	int chessparkfeatures;
	char *statusmsg;
	char *role;
};

void Resource_Destroy(void *pvoid)
{
	struct resourcedata_s *data = pvoid;
	free(data->statusmsg);
	free(data);
}

struct frienddata_s
{
	char *nickname;
	char *avatarhash;
	int ismuc;
	struct namedlist_s *groups;
	struct namedlist_s *resources;
	int subscribedTo;
	int subscribedFrom;
	int pending;
	struct adhoccommand_s *command;
};

void FriendData_Destroy(void *pvoid)
{
	struct frienddata_s *data = pvoid;
	free(data->nickname);
	free(data->avatarhash);
	NamedList_Destroy(&(data->groups));
	NamedList_Destroy(&(data->resources));
}

struct namedlist_s *roster = NULL;

void Model_AddChatToFavorites(char *chatjid);
void Model_RemoveChatFromFavorites(char *chatjid);
void Model_AddChatToAutojoin(char *chatjid);
void Model_RemoveChatFromAutojoin(char *chatjid);
void Model_SetAvatarToHash(char *hash);
int Model_DoAutoAway(char *jid, int show);

void Model_AddTimeout(int (*callback)(void *userdata), int ms, void *data);
void Model_RemoveTimeout(int (*callback)(void *userdata), int ms);

void Model_UpdateSubscribers(char *jid);

int Model_IsJIDIgnored(char *jid);

void Model_HandlePresence(char *jid, int ismuc, enum SStatus status,
	char *statusmsg, char *role, char *affiliation, char *realjid, int statuscode,
	char *vcardphotohash, char *nickchange, struct namedlist_s *roleslist,
	struct namedlist_s *titleslist, int prioritynum, char *actorjid, char *reason,
	int notactivated, char *membertype);

int ModelMC_StartRoster(void *dummy1, void *dummy2);
int ModelMC_AddRosterEntry(struct rosterentry_s *entry, void *dummy2);
int ModelMC_FinishRoster(void *dummy1, void *dummy2);
int ModelMC_SetSubscribed(char *jid, void *dummy);
int ModelMC_SetUnsubscribed(char *jid, void *dummy);
int ModelMC_AskUnsubscribe(char *jid, void *dummy);
int ModelMC_AskSubscribe(char *jid, void *dummy);
int ModelMC_HandlePresence(struct presenceinfo_s *info, void *dummy);
int ModelMC_HandleMessage(struct messageinfo_s *info, void *dummy);

void Model_SaveServerPrefs();
void Model_RequestAvatarService(char *sha1, char *jid);
void Model_RequestAvatarServiceJid(char *jid);

int Model_PeriodicPing(void *dummy);

struct ModelTimeout_s
{
	struct ModelTimeout_s *next;
	int (*callback)(void *userdata);
	unsigned int lastcall;
	unsigned int delay;
	void *data;
};

struct ModelTimeout_s *modeltimeouts = NULL;
struct mp_s *modelmp;

void Model_AddTimeout(int (*callback)(void *userdata), int ms, void *data)
{
	struct ModelTimeout_s *timeout;

	timeout = malloc(sizeof(*timeout));
	memset(timeout, 0, sizeof(*timeout));

	timeout->callback = callback;
	timeout->lastcall = GetTickCount();
	timeout->delay = ms;
	timeout->data = data;

	timeout->next = modeltimeouts->next;
	modeltimeouts->next = timeout;
}

void Model_RemoveTimeout(int (*callback)(void *userdata), int ms)
{
	struct ModelTimeout_s *timeout = modeltimeouts;

	while (timeout->next != modeltimeouts)
	{
		if (timeout->next->callback == callback && timeout->next->delay == ms)
		{
			struct ModelTimeout_s *temp = timeout->next;
			timeout->next = timeout->next->next;
			free(temp);
		}
		else
		{
			timeout = timeout->next;
		}
	}
}

void Model_QueueMessage(struct mpmessage_s *msg, unsigned int priority)
{
	MP_QueueMessage(modelmp, msg, priority);
}

void Model_Init()
{
	modeltimeouts = malloc(sizeof(*modeltimeouts));
	memset(modeltimeouts, 0, sizeof(*modeltimeouts));

	modeltimeouts->next = modeltimeouts;
	modelmp = MP_Create(5, MODELMSG_NUMMESSAGES);
	MP_SetCallback(modelmp, MODELMSG_STARTROSTER,     ModelMC_StartRoster);
	MP_SetCallback(modelmp, MODELMSG_ADDROSTERENTRY,  ModelMC_AddRosterEntry);
	MP_SetCallback(modelmp, MODELMSG_FINISHROSTER,    ModelMC_FinishRoster);
	MP_SetCallback(modelmp, MODELMSG_ASKSUBSCRIBE,    ModelMC_AskSubscribe);
	MP_SetCallback(modelmp, MODELMSG_SETSUBSCRIBED,   ModelMC_SetSubscribed);
	MP_SetCallback(modelmp, MODELMSG_ASKUNSUBSCRIBE,  ModelMC_AskUnsubscribe);
	MP_SetCallback(modelmp, MODELMSG_SETUNSUBSCRIBED, ModelMC_SetUnsubscribed);
	MP_SetCallback(modelmp, MODELMSG_HANDLEPRESENCE,  ModelMC_HandlePresence);
	MP_SetCallback(modelmp, MODELMSG_HANDLEMESSAGE,   ModelMC_HandleMessage);
}

void Model_Poll()
{
	struct ModelTimeout_s *timeout = modeltimeouts;

	while (timeout->next != modeltimeouts)
	{
		if (GetTickCount() - timeout->next->lastcall > timeout->next->delay)
		{
			int result;

			result = timeout->next->callback(timeout->next->data);
			timeout->next->lastcall = GetTickCount();
			if (result)
			{
				struct ModelTimeout_s *temp = timeout->next;
				timeout->next = timeout->next->next;
				free(temp);
			}
			else
			{
				timeout = timeout->next;
			}
		}
		else
		{
			timeout = timeout->next;
		}
	}

	MP_HandleMessages(modelmp, 300);

	if (Model_GetOption(OPTION_AUTOAWAY) && Conn_GetConnState() == CONN_CONNECTED)
	{
		if (Box_GetIdleTime() > localautoawaytime * 60 * 1000)
		{
			if (!autoaway && userstatus == SSTAT_AVAILABLE)
			{
				free(autoawayolduserstatusmsg);
				autoawayolduserstatusmsg = strdup(userstatusmsg);

				Ctrl_SetPresence(SSTAT_AWAY, "Idle");
				autoaway = 1;
			}
		}
		else
		{
			if (autoaway)
			{
				Ctrl_SetPresence(SSTAT_AVAILABLE, autoawayolduserstatusmsg);
				autoaway = 0;
			}
		}
	}
}

void Model_SetPriority(int priority)
{
	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	currentpriority = priority;
	Conn_SendPresence(userstatus, userstatusmsg, NULL, 0, avatarhash, currentpriority, 1);
}

void Friend_GetBestStatus(struct frienddata_s *pfrienddata, enum SStatus *status, char **statusmsg, int *chessparkfeatures)
{
	struct namedlist_s *presource = pfrienddata->resources;
	
	*status = SSTAT_OFFLINE;
	*statusmsg = NULL;
	*chessparkfeatures = FALSE;

	if (pfrienddata->pending)
	{
		*status = SSTAT_PENDING;
		return;
	}

	while (presource)
	{
		struct resourcedata_s *data = presource->data;

		if (data->status < *status)
		{
			*status = data->status;
			*statusmsg = data->statusmsg;
		}
		if (data->chessparkfeatures && data->status < SSTAT_OFFLINE)
		{
			*chessparkfeatures = TRUE;
		}
		presource = presource->next;
	}
}
enum SStatus Model_GetLocalStatus()
{
	return userstatus;
}

char *Model_GetStatusMsg()
{
	return userstatusmsg;
}

int Model_JidInRoster(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	int result = (ppfriend != NULL);
	free(barejid);

	return result;
}

enum SStatus Model_GetBestStatus(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;
	enum SStatus status;
	char *statusmsg;
	int chessparkfeatures;

	if (!ppfriend)
	{
		char *bareloginjid = Jid_Strip(login_jid);
		if (stricmp(barejid, bareloginjid) == 0)
		{
			free(barejid);
			free(bareloginjid);
			return userstatus;
		}
		free(barejid);
		free(bareloginjid);
		return SSTAT_OFFLINE;
	}
	else
	{
		data = (*ppfriend)->data;
	}

	Friend_GetBestStatus(data, &status, &statusmsg, &chessparkfeatures);

	free(barejid);
	return status;
}

int numloginattempts = 0;

int Model_ResendChessparkLogin(void *userdata)
{
	numloginattempts++;

	if (numloginattempts < CHESSPARKLOGIN_RETRIES)
	{
		if (Conn_GetConnState() != CONN_CONNECTED)
		{
			return 0;
		}
		Conn_SendChessparkLogin();
	}
	else
	{
		View_ShowGameLoginFailed(NULL, NULL);
		return 1;
	}

	return 0;
}

void Model_LoginToChesspark()
{
	numloginattempts = 0;
	chessparklogin = 0;

	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	Conn_SendChessparkLogin();
	/* use an adhoc dialog to show logging in */
	View_PopupGameWaitingDialog(_("Chesspark login"), _("Logging into chesspark game server..."));
	/*Model_AddTimeout(Model_ResendChessparkLogin, CHESSPARKLOGIN_TIMEOUT, NULL);*/
}

void Model_SetLoginLast()
{
	login_changed = FALSE;
}

void Model_SetLogin(int remember, char *jid, char *pass)
{
	login_changed = TRUE;

	free(local_membertype);
	local_membertype = NULL;
	NamedList_Destroy(&local_permissions);
	local_notactivated = 0;

	Log_Write(0, "login_changed 1 %s %s\n", login_jid, jid);
	if (login_jid)
	{
		if (stricmp(login_jid, jid) == 0)
		{
			login_changed = FALSE;
			Log_Write(0, "login_changed 0\n");
		}
		else
		{
			login_joinedchats = 0;
			View_CloseNewGamesDialog();
			View_CloseAllChatDialogs();
			View_CloseAllChessGames();
		}
		free(login_jid);
	}

	if (login_pass)
	{
		free(login_pass);
	}

	login_remember = remember;
	login_jid = strdup(jid);
	login_pass = strdup(pass);
}

void ModelConn_Login(char *jid)
{
	char *login1, *login2, *login3, *login4, *login5;
	char *bareloginjid = Jid_Strip(login_jid);
	char buffer[2048];
	int addcentralpark = 0;

	/* reset hidden group status */
	inhiddengroup = 0;

	/* FIXME: memory leak, find out why destroying the profile cache crashes us */
	profilecache = NULL;
	/*NamedList_Destroy(&profilecache);*/
/*
	NamedList_Destroy(&pushplaygames);
	NamedList_Destroy(&pushwatchgames);
	View_ClearPushGames();
*/
	login_connected = TRUE;

	SetRegString("lastlogin", login_jid);

	if (login_remember)
	{
		SetRegString("autologin_username", login_jid);
		SetRegString("autologin_password", login_pass);
		SetRegString("language", i18n_GetCurrentLangCode());
		SetRegInt("autologin", 1);
	}
	else
	{
		SetRegInt("autologin", 0);
	}

	login1 = strdup(GetRegString("lastlogin1", buffer, 2048));
	login2 = strdup(GetRegString("lastlogin2", buffer, 2048));
	login3 = strdup(GetRegString("lastlogin3", buffer, 2048));
	login4 = strdup(GetRegString("lastlogin4", buffer, 2048));
	login5 = strdup(GetRegString("lastlogin5", buffer, 2048));

	/* FIXME: Do this in a much less messy fashion. */
	if (login1 && stricmp(login_jid, login1) == 0)
	{
		/* do nothing */
	}
	else if (login2 && stricmp(login_jid, login2) == 0)
	{
		free(login2);
		login2 = login1;
		login1 = strdup(login_jid);
	}
	else if (login3 && stricmp(login_jid, login3) == 0)
	{
		free(login3);
		login3 = login2;
		login2 = login1;
		login1 = strdup(login_jid);
	}
	else if (login4 && stricmp(login_jid, login4) == 0)
	{
		free(login4);
		login4 = login3;
		login3 = login2;
		login2 = login1;
		login1 = strdup(login_jid);
	}
	else
	{
		free(login5);
		login5 = login4;
		login4 = login3;
		login3 = login2;
		login2 = login1;
		login1 = strdup(login_jid);
	}

	if (login1)
	{
		SetRegString("lastlogin1", login1);
	}

	if (login2)
	{
		SetRegString("lastlogin2", login2);
	}

	if (login3)
	{
		SetRegString("lastlogin3", login3);
	}

	if (login4)
	{
		SetRegString("lastlogin4", login4);
	}

	if (login5)
	{
		SetRegString("lastlogin5", login5);
	}

	free(login1);
	free(login2);
	free(login3);
	free(login4);
	free(login5);
	
	View_CloseLogin();

	View_GetWindowPosFromRegistry();

	View_PopupRoster();

	login_fromloginwindow = FALSE;

	View_SetUser(bareloginjid, Ctrl_GetDefaultNick());

	/* Don't bother getting server prefs if we're not a chesspark jid */
	if (strstr(bareloginjid, "chesspark.com"))
	{
                Conn_GetServerPrefs();
	}

	localprivacylistloaded = 0;
	matchprivacylistloaded = 0;
	Conn_SetActivePrivacyList("chesspark");
	Conn_GetPrivacyList("chesspark");

	View_ShowRosterLoading();
	View_SetRosterNormal();

	discocause = 0;

	free(bareloginjid);

	Conn_SetKeepAlive(1);
	Model_AddTimeout(Model_PeriodicPing, 60000, NULL);
	Model_PeriodicPing(NULL);

	/* Fake server prefs on non chesspark jid */
	if (!strstr(bareloginjid, "chesspark.com"))
	{
                ModelConn_HandlePrefs(NULL, NULL, NULL, NULL, NULL, NULL);
	}
}


void ModelConn_HandlePrefs(struct namedlist_s *infavoritechats,
	struct namedlist_s *inautojoinchats,
	struct namedlist_s *inavailablestatuses,
	struct namedlist_s *inawaystatuses,
	struct namedlist_s *incustomtimecontrols,
	struct namedlist_s *inoptions)
{
	struct namedlist_s *entry;

	localoptions = inoptions;
	{
		struct namedlist_s **entry = NamedList_GetByName(&localoptions, OPTION_AUTOAWAY);

		if (entry && (*entry)->data)
		{
			sscanf((*entry)->data, "%d", &localautoawaytime);
		}

		entry = NamedList_GetByName(&localoptions, OPTION_VOLUME);
		if (entry && (*entry)->data)
		{
			int volume;
			sscanf((*entry)->data, "%d", &volume);
			Audio_SetVolume(volume);
		}

		entry = NamedList_GetByName(&localoptions, OPTION_NOTIFICATIONTIME);
		if (entry && (*entry)->data)
		{
			int length;
			sscanf((*entry)->data, "%d", &length);
			CornerPop_SetLength(length);
		}

		entry = NamedList_GetByName(&localoptions, OPTION_NOTIFICATIONLOCATION);

		if (entry)
		{
			CornerPop_SetLocation((*entry)->data);
		}
		else
		{
			CornerPop_SetLocation(NULL);
		}

		/* registry setting for low bandwidth overrides server prefs, in case the
		 * server prefs weren't saved correctly */
		entry = NamedList_GetByName(&localoptions, OPTION_LOWBANDWIDTH);
		{
			char *bareloginjid = Jid_Strip(login_jid);
			if (GetUserRegInt(bareloginjid, "LowBandwidth") && !entry)
			{
				NamedList_AddString(&localoptions, "LowBandwidth", NULL);
			}
			free(bareloginjid);
		}
	}

	View_SetOptions();

	StatusList_Destroy(&availstatuslist);
	StatusList_Destroy(&awaystatuslist);

	entry = inavailablestatuses;
	while (entry)
	{
		StatusList_Add(&availstatuslist, SSTAT_AVAILABLE, entry->name);
		entry = entry->next;
	}

	entry = inawaystatuses;
	while (entry)
	{
		StatusList_Add(&awaystatuslist, SSTAT_AWAY, entry->name);
		entry = entry->next;
	}

	View_SetStatusList(SSTAT_AVAILABLE, availstatuslist);
	View_SetStatusList(SSTAT_AWAY, awaystatuslist);

	favechats = infavoritechats;
	autojoinchats = inautojoinchats;
	customtimecontrols = incustomtimecontrols;

	entry = favechats;

	while (entry)
	{
		View_AddChatroom(_("Favorites"), entry->name, NULL, "", 0);
		entry = entry->next;
	}

	if (!login_joinedchats)
	{
		entry = autojoinchats;

		while (entry)
		{
			if (!strstr(entry->name, "@chat.chesspark.com"))
			{
				Ctrl_JoinGroupChatDefaultNick(entry->name);
			}
			entry = entry->next;
		}
	}

	Conn_GetRoster();
	{
		char *bareloginjid = Jid_Strip(login_jid);
		char buffer[2048];

		Model_GetProfile(bareloginjid, 1);
		/*if (!Model_GetOption(OPTION_LOWBANDWIDTH))*/
		{
			if (strstr(bareloginjid, "@chesspark.com"))
			{
				Model_RequestAvatarServiceJid(bareloginjid);
			}
			else
			{
				Conn_RequestvCard(bareloginjid);
			}
		}

		Ctrl_SetPresence(GetUserRegInt(bareloginjid, "LastOnlineStatus"), GetUserRegString(bareloginjid, "LastOnlineStatusMsg", buffer, 2048));
		currentpriority = 1;
		Conn_SubscribeRating(bareloginjid);
		Conn_SubscribeProfile(bareloginjid);
		free(bareloginjid);
	}

	View_ReconnectChatDialogs(0);
}

void ModelConn_SetSharedRosterFlag()
{
	char *bareloginjid = Jid_Strip(login_jid);

	SetUserRegInt(bareloginjid, "SharedRosterLoaded", 1);

	free(bareloginjid);
}

void Model_ChangeUser()
{
	View_CloseRoster();
	
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendPresence(SSTAT_OFFLINE, NULL, NULL, 0, NULL, 0, 0);

		Conn_Disconnect();
		discocause = 1;
	}

	View_PopupLogin();

	login_fromloginwindow = TRUE;
	login_connected = FALSE;
}

void ModelConn_HandleGroups(struct namedlist_s *groupslist)
{
#ifdef CHESSPARK_GROUPS
	NamedList_Destroy(&localgroupslist);
	localgroupslist = groupslist;
	{
		struct namedlist_s *entry = localgroupslist;

		while (entry)
		{
			struct groupinfo_s *ginfo = entry->data;

			if (ginfo->type && stricmp(ginfo->type, "hidden") == 0)
			{
				inhiddengroup = 1;
			}

			entry = entry->next;
		}
	}
	View_UpdateGameFinderGroups();
#endif
}

int Model_IsHiddenGroupChat(char *chatjid)
{
	char *barejid;
	int ihgc = 0;
	struct namedlist_s *entry = localgroupslist;

	if (!inhiddengroup)
	{
		return 0;
	}

	barejid = Jid_Strip(chatjid);

	while (entry)
	{
		struct groupinfo_s *ginfo = entry->data;

		if (ginfo->type && stricmp(ginfo->type, "hidden") == 0)
		{
			if (ginfo->chat)
			{
				char chat[1024];
				EscapeJID(ginfo->chat, chat, 1024);
				if (!strchr(chat, '@'))
				{
					strcat(chat, "@chat.chesspark.com");
				}

				Log_Write(0, "compare %s with %s\n", chat, barejid);
				if (stricmp(chat, barejid) == 0)
				{
					ihgc = 1;
				}
			}
		}

		entry = entry->next;
	}

	free(barejid);

	return ihgc;
}

int Model_IsInHiddenGroup()
{
	return inhiddengroup;
}

struct namedlist_s *Model_GetLocalGroups()
{
	return localgroupslist;
}

void Model_SetPresence(enum SStatus status, char *statusmsg)
{
	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	if (status != SSTAT_OFFLINE && !(status == SSTAT_AWAY && statusmsg && strcmp(statusmsg, "Idle") == 0))
	{
		char *bareloginjid = Jid_Strip(login_jid);
		SetUserRegInt(bareloginjid, "LastOnlineStatus", status);
		SetUserRegString(bareloginjid, "LastOnlineStatusMsg", statusmsg);
		free(bareloginjid);
	}

	userstatus = status;

	if (userstatusmsg)
	{
		free(userstatusmsg);
	}

	userstatusmsg = NULL;

	if (statusmsg)
	{
		userstatusmsg = strdup(statusmsg);
	}

	View_SetPresence(userstatus, userstatusmsg);
	View_SetStatusOnAllChats(userstatus, userstatusmsg);
	Conn_SendPresence(status, statusmsg, NULL, 0, avatarhash, currentpriority, 1);
	Model_HandlePresence(login_jid, 0, status, statusmsg, NULL, NULL, login_jid, 0, avatarhash, NULL, NULL, NULL, currentpriority, NULL, NULL, 0, NULL);

	NamedList_Destroy(&sentautoaway);
	autoaway = 0;
}
/*
void ModelConn_PurgeRoster()
{
	NamedList_Destroy(&roster);
	
	View_RemoveAllFriends();
}
*/

void ModelConn_SetFriend(char *jid, char *nickname, struct namedlist_s *newgroups, int subscribedTo, int subscribedFrom, int pending, int inroster)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;
	struct namedlist_s *group;
	enum SStatus status;
	char *statusmsg;
	int chessparkfeatures;

	if (!ppfriend)
	{
		data = malloc(sizeof(*data));
		memset(data, 0, sizeof(*data));
		NamedList_AddToTop(&roster, barejid, data, FriendData_Destroy);
	}
	else
	{
		data = (*ppfriend)->data;
	}

	free(data->nickname);
	data->nickname = strdup(nickname);
	data->subscribedTo = subscribedTo;
	data->subscribedFrom = subscribedFrom;
	data->pending = pending;

	View_RemoveFriend(barejid, NULL, inroster);

	group = data->groups;
	while (group)
	{
		View_RemoveFriend(barejid, group->name, inroster);
		group = group->next;
	}

	NamedList_Destroy(&(data->groups));

	data->groups = NamedList_DupeStringList(newgroups);
	
	Friend_GetBestStatus(data, &status, &statusmsg, &chessparkfeatures);
	
	View_AddFriend(barejid, nickname, NULL, status, NULL, inroster);
    
	group = data->groups;
	while(group)
	{
		View_AddFriend(barejid, nickname, group->name, status, NULL, 0);
		group = group->next;
	}

	free(barejid);
}

int ModelMC_StartRoster(void *dummy1, void *dummy2)
{
	NamedList_Destroy(&roster);
	View_RemoveAllFriends();

	return 1;
}

int ModelMC_AddRosterEntry(struct rosterentry_s *entry, void *dummy)
{
	char *barejid;
	struct frienddata_s *data;
	struct namedlist_s *group;

	barejid = Jid_Strip(entry->jid);

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));
	NamedList_AddToTop(&roster, barejid, data, FriendData_Destroy);

	data->nickname = strdup(entry->name);
	data->subscribedTo = entry->subscribedto;
	data->subscribedFrom = entry->subscribedfrom;
	data->groups = NamedList_DupeStringList(entry->groups);
	data->pending = entry->pending;
	
	View_AddFriend(barejid, entry->name, NULL, !entry->pending ? SSTAT_OFFLINE : SSTAT_PENDING, NULL, 1);
	
	group = data->groups;
	while(group)
	{
		View_AddFriend(barejid, entry->name, group->name, !entry->pending ? SSTAT_OFFLINE : SSTAT_PENDING, NULL, 1);
		group = group->next;
	}

	free(barejid);

	return 1;
}


int ModelMC_FinishRoster(void *dummy1, void *dummy2)
{
	View_FinishRoster();
	Model_HandlePresence(login_jid, 0, userstatus, userstatusmsg, NULL, NULL, login_jid, 0, avatarhash, NULL, NULL, NULL, currentpriority, NULL, NULL, 0, NULL);
	Model_LoginToChesspark();

	return 1;
}
/*
void ModelConn_HandleRoster(struct namedlist_s *inrosterlist)
{
	struct namedlist_s *listentry = inrosterlist;
	struct mpmessage_s *msg;

	NamedList_Destroy(&roster);
	View_RemoveAllFriends();

	while (listentry)
	{
		msg = MPMessage_Create(MODELMSG_ADDROSTERENTRY, listentry->data, NULL, NULL, NULL, 0);
		MP_QueueMessage(modelmp, msg, 1);
		listentry = listentry->next;
	}

	msg = MPMessage_Create(MODELMSG_FINISHROSTER, NULL, NULL, NULL, NULL, 0);
	MP_QueueMessage(modelmp, msg, 1);
}
*/

void Model_AddFriend(char *jid, char *nickname)
{
	char *barejid = Jid_Strip(jid);

	if (NamedList_GetByNameAndBump(&roster, barejid))
	{
		return;
	}

	ModelConn_SetFriend(barejid, nickname, NULL, 0, 0, 1, 0);
	Conn_SetFriend(barejid, nickname, NULL);
	Conn_RequestFriend(barejid);
	/* bit of a hack here, rerequest profile so the games this person is playing are refreshed */
	Conn_RequestProfile(barejid);

	View_ScrollFriendVisible(barejid);

	free(barejid);
}

char *Model_GetFriendNick(char *jid)
{
	char *barejid = Jid_Strip(jid);
	char *barejid2 = Jid_Strip(login_jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;
	char unescaped[1024];

	if (!jid)
	{
		return NULL;
	}

	if (stricmp(barejid, barejid2) == 0)
	{
		free(barejid);
		free(barejid2);
		return Model_GetDefaultNick();
	}

	free(barejid2);

	if (!ppfriend)
	{
		if (strstr(barejid, "@chesspark.com"))
		{
			char *nickname = UnescapeJID(Jid_GetBeforeAt(barejid), unescaped, 1024);
			free(barejid);
			return strdup(nickname);
		}
		return barejid;
	}

	data = (*ppfriend)->data;

	if (data->nickname && strlen(data->nickname) > 0)
	{
		free(barejid);
		return strdup(data->nickname);
	}

	if (strstr(barejid, "@chesspark.com"))
	{
		char *nickname = UnescapeJID(Jid_GetBeforeAt(barejid), unescaped, 1024);
		free(barejid);
		return strdup(nickname);
	}
	return barejid;
}

void Model_SyncFriend(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;

	if (!ppfriend)
	{
		free(barejid);
		return;
	}

	data = (*ppfriend)->data;
	
	Conn_SetFriend(barejid, data->nickname, data->groups);

	free(barejid);
}

void Model_RemoveFriend(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;
	struct namedlist_s *group;

	if (!ppfriend)
	{
		free(barejid);
		return;
	}

	data = (*ppfriend)->data;
	
	View_RemoveFriend(barejid, NULL, 0);

	group = data->groups;
	while(group)
	{
		View_RemoveFriend(barejid, group->name, 0);
		group = group->next;
	}
	
	NamedList_Remove(ppfriend);

	Conn_RemoveFriend(barejid);
	Conn_UnsubscribeFromFriend(barejid);
	Conn_UnsubscribeToFriend(barejid);
	Conn_UnsubscribeProfile(barejid);
	Conn_UnsubscribeRating(barejid);

	free(barejid);
}


static void Model_AddFriendToGroupNoSync(char *jid, char *groupname)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;
	enum SStatus status;
	char *statusmsg;
	int chessparkfeatures;

	if (!ppfriend || !groupname)
	{
		Log_Write(0, "Break1\n");
		free(barejid);
		return;
	}

	data = (*ppfriend)->data;
		
	Friend_GetBestStatus(data, &status, &statusmsg, &chessparkfeatures);

	View_AddFriend(barejid, data->nickname, groupname, status, statusmsg, 0);	

	if (NamedList_GetByName(&(data->groups), groupname))
	{
		free(barejid);
		return;
	}

	NamedList_Add(&(data->groups), groupname, NULL, NULL);

	free(barejid);
}


void Model_AddFriendToGroup(char *jid, char *groupname)
{
	Model_AddFriendToGroupNoSync(jid, groupname);

	Model_SyncFriend(jid);
}


static void Model_RemoveFriendFromGroupNoSync(char *jid, char *groupname)
{	
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;
	struct namedlist_s **group;
	
	if (!ppfriend || !groupname)
	{
		free(barejid);
		return;
	}

	data = (*ppfriend)->data;

	while (group = NamedList_GetByName(&(data->groups), groupname))
	{
		View_RemoveFriend(barejid, groupname, 0);
		NamedList_Remove(group);
	}

	free(barejid);
}

void Model_RemoveFriendFromGroup(char *jid, char *groupname)
{
	Model_RemoveFriendFromGroupNoSync(jid, groupname);

	Model_SyncFriend(jid);
}

void Model_MoveFriendFromGroupToGroup(char *jid, char *fromgroup, char *togroup)
{
	/* Need to dupe the jid in case we free it somewhere along the way */
	char *dupejid = strdup(jid);

	Model_RemoveFriendFromGroupNoSync(dupejid, fromgroup);

	Model_AddFriendToGroupNoSync(dupejid, togroup);

	Model_SyncFriend(dupejid);

	free(dupejid);
}

void Model_RemoveGroup(char *groupname)
{
	struct namedlist_s *pfriend = roster;

	if (!groupname)
	{
		return;
	}
	
	while (pfriend)
	{
		Model_RemoveFriendFromGroup(pfriend->name, groupname);

		pfriend = pfriend->next;
	}

	View_RemoveGroup(groupname);
}


void Model_RemoveGroupAndContents(char *groupname)
{
	struct namedlist_s *pfriend = roster;

	if (!groupname)
	{
		return;
	}
	
	while (pfriend)
	{
		struct namedlist_s *next = pfriend->next;
		struct frienddata_s *data = pfriend->data;
		struct namedlist_s **group = NamedList_GetByName(&(data->groups), groupname);

		if (group)
		{
			Model_RemoveFriend(pfriend->name);
		}
		pfriend = next;
	}

	View_RemoveGroup(groupname);
}


void Model_RenameGroup(char *oldname, char *newname)
{
	struct namedlist_s *pfriend = roster;

	if (!oldname || !newname)
	{
		return;
	}
	
	while (pfriend)
	{
		struct namedlist_s *next = pfriend->next;
		struct frienddata_s *data = pfriend->data;
		struct namedlist_s **group = NamedList_GetByName(&(data->groups), oldname);

		if (group)
		{
			free((*group)->name);
			(*group)->name = strdup(newname);
			Model_SyncFriend(pfriend->name);
		}

		pfriend = pfriend->next;
	}

	View_RenameGroup(oldname, newname);
}

static void Model_ViewUpdateFriend(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct namedlist_s *group;
	struct frienddata_s *data;
	enum sstatus status;
	char *statusmsg;
	int chessparkfeatures;
	struct namedlist_s *roles = NULL, *titles = NULL;
	char *rating = NULL;
	struct profile_s *profile = Model_GetProfile(jid, 0);

	if (!ppfriend)
	{
		free(barejid);
		return;
	}

	data = (*ppfriend)->data;

	if (Model_IsJIDIgnored(jid))
	{
		status = SSTAT_OFFLINE;
		statusmsg = NULL;
		chessparkfeatures = 0;
	}
	else
	{
		Friend_GetBestStatus(data, &status, &statusmsg, &chessparkfeatures);
	}

	if (profile)
	{
                rating = Info_GetStandardRating(profile->ratings);
		roles = profile->roles;
		titles = profile->titles;
	}

	View_SetFriendStatus(barejid, data->nickname, NULL, status, statusmsg, data->avatarhash, chessparkfeatures ? rating : NULL, roles, titles, NULL, NULL, 0, 0, data->command);

	group = data->groups;
	while(group)
	{
		View_SetFriendStatus(barejid, data->nickname, group->name, status, statusmsg, data->avatarhash, chessparkfeatures ? rating : NULL, roles, titles, NULL, NULL, 0, 0, data->command);
		group = group->next;
	}

	View_SetChatParticipantStatus(jid, status, statusmsg, NULL, NULL, barejid, NULL, roles, titles, NULL, NULL, 0, NULL);

	View_RefreshChatIcon(barejid);
	free(barejid);
}

void Model_HandlePresence(char *jid, int ismuc, enum SStatus status,
	char *statusmsg, char *role, char *affiliation, char *realjid, int statuscode,
	char *vcardphotohash, char *nickchange, struct namedlist_s *roleslist,
	struct namedlist_s *titleslist, int prioritynum, char *actorjid, char *reason,
	int notactivated, char *membertype)
{
	char *barejid = Jid_Strip(jid);
	char *resource = Jid_GetResource(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *frienddata;
	struct namedlist_s **ppresource;
	struct resourcedata_s *data;

/* priority fighting */
#if 0  
	{
		char *bareloginjid = Jid_Strip(login_jid);
		if (stricmp(barejid, bareloginjid) == 0)
		{
			if (prioritynum >= currentpriority)
			{
				currentpriority = prioritynum + 1;
				if (currentpriority == 129)
				{
					currentpriority = 128;
				}

				Conn_SendPresence(userstatus, userstatusmsg, NULL, 0, avatarhash, currentpriority, 1);
			}
		}
		free(bareloginjid);
	}
#endif
	if (!ppfriend)
	{
		if (ismuc)
		{
			if (strstr(jid, "@games.chesspark.com"))
			{
				if (strncmp(jid, "tournament", 10) == 0)
				{
					View_SetTourneyChatParticipantStatus(jid, status, statusmsg, role, affiliation, realjid, nickchange, roleslist, titleslist, actorjid, reason);
				}
				else
				{
					View_SetGameChatParticipantStatus(jid, status, statusmsg, role, affiliation, realjid, nickchange, roleslist, titleslist, actorjid, reason, notactivated, membertype);
				}
			}
			else
			{
				View_SetChatParticipantStatus(jid, status, statusmsg, role, affiliation, realjid, nickchange, roleslist, titleslist, actorjid, reason, notactivated, membertype);
			}
			if (statuscode == 201)
			{
				Conn_RequestInstantRoom(barejid);
				/*Conn_RequestChatConfigForm(jid);*/
			}
			free(barejid);
			free(resource);
			return;
		}
		else
		{
			Model_ViewUpdateFriend(jid);

			if (vcardphotohash)
			{
				char *lcase = strdup(vcardphotohash);
				strlwr(lcase);
				Model_SetFriendAvatarHash(jid, lcase, 1, 1);
				free(lcase);
			}
		}
		/*ModelConn_SetFriend(barejid, NULL, NULL, 1);
		ppfriend = NamedList_GetByName(&roster, barejid);

		if (!ppfriend) /* should be impossible, but just in case */
		{
			free(barejid);
			free(resource);
			return;
		}
	}

	frienddata = (*ppfriend)->data;
	frienddata->ismuc = ismuc;

	ppresource = NamedList_GetByName(&(frienddata->resources), resource);

	if (status == SSTAT_OFFLINE)
	{
		if (ppresource)
		{
			NamedList_Remove(ppresource);
		}
	}
	else
	{
		if (!ppresource)
		{
			data = malloc(sizeof(*data));
			memset(data, 0, sizeof(*data));
			NamedList_Add(&(frienddata->resources), resource, data, Resource_Destroy);

			if (!ismuc)
			{
				Conn_RequestDiscoInfo(jid);
			}
		}
		else
		{
			data = (*ppresource)->data;
		}

		data->status = status;
		free (data->statusmsg);
		data->statusmsg = strdup(statusmsg);
		free (data->role);
		data->role = strdup(role);
	}

	if (!ismuc)
	{
		Model_ViewUpdateFriend(jid);
	}

	if (vcardphotohash)
	{
		char *lcase = strdup(vcardphotohash);
		strlwr(lcase);
		Model_SetFriendAvatarHash(jid, lcase, 1, 1);
		free(lcase);
	}

	free(barejid);
	free(resource);
}

int ModelMC_HandlePresence(struct presenceinfo_s *info, void *dummy)
{
	Model_HandlePresence(info->jid, info->ismuc, info->status,
		info->statusmsg, info->role, info->affiliation, info->realjid,
		info->statuscode, info->vcardphotohash, info->nickchange,
		info->roleslist, info->titleslist, info->prioritynum,
		info->actorjid, info->reason, info->notactivated, info->membertype);

	return 1;
}

int ModelMC_AskSubscribe(char *jid, void *dummy)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *frienddata;
	char *bareloginjid = Jid_Strip(login_jid);

	free(bareloginjid);

	if (Model_GetOption(OPTION_AUTOAPPROVE) && !ppfriend)
	{
		Ctrl_ApproveFriend(barejid);
		Ctrl_AddFriend(barejid, NULL);
		Ctrl_RequestFriend(barejid);		
/*
		ModelConn_SetFriend(barejid, NULL, NULL, 0, 0);
		Conn_SetFriend(barejid, NULL, NULL);
		Conn_ApproveFriend(barejid);
		Conn_RequestFriend(barejid);
*/
		free(barejid);
		return 1;
	}

	if (!ppfriend)
	{
		View_PopupSimpleDialog("approve_box", jid);
		free(barejid);
		return 1;
	}

	frienddata = (*ppfriend)->data;

	if (!Model_GetOption(OPTION_AUTOAPPROVE) && !frienddata->subscribedFrom)
	{
		View_PopupSimpleDialog("approve_box", jid);
	}
	else
	{
		Conn_ApproveFriend(jid);
	}

	free(barejid);
	return 1;
}

int ModelMC_AskUnsubscribe(char *jid, void *dummy)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *frienddata;

	if (!ppfriend)
	{
		Conn_UnsubscribeToFriend(jid);
		free(barejid);
		return 1;
	}

	frienddata = (*ppfriend)->data;

	frienddata->subscribedTo = FALSE;
	Conn_UnsubscribeToFriend(jid);

	free(barejid);
	return 1;
}


int ModelMC_SetUnsubscribed(char *jid, void *dummy)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;
	enum SStatus status;
	char *statusmsg;
	int chessparkfeatures;
	struct namedlist_s *roles = NULL, *titles = NULL;
	char *rating = NULL;
	struct profile_s *profile = Model_GetProfile(jid, 1);

	if (!ppfriend)
	{
		/*
		ModelConn_SetFriend(barejid, NULL, NULL, 1);
		ppfriend = NamedList_GetByNameAndBump(&roster, barejid);

		if (!ppfriend) /* should be impossible, but just in case */
		{
			free(barejid);
			return 1;
		}
	}
	
	data = (*ppfriend)->data;

	data->subscribedTo = FALSE;

	Friend_GetBestStatus(data, &status, &statusmsg, &chessparkfeatures);

	if (profile)
	{
                rating = Info_GetStandardRating(profile->ratings);
		roles = profile->roles;
		titles = profile->titles;
	}

	View_SetFriendStatus(barejid, data->nickname, NULL, status, statusmsg, data->avatarhash, chessparkfeatures ? rating : NULL, roles, titles, NULL, NULL, 0, 0, data->command);

	free(barejid);
	return 1;
}

int ModelMC_SetSubscribed(char *jid, void *dummy)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;
	enum SStatus status;
	char *statusmsg;
	int chessparkfeatures;
	struct namedlist_s *roles = NULL, *titles = NULL;
	char *rating = NULL;
	struct profile_s *profile = Model_GetProfile(jid, 1);

	if (!ppfriend)
	{
		/*
		ModelConn_SetFriend(barejid, NULL, NULL, 1);
		ppfriend = NamedList_GetByNameAndBump(&roster, barejid);

		if (!ppfriend) /* should be impossible, but just in case */
		{
			free(barejid);
			return 1;
		}
	}
	
	data = (*ppfriend)->data;

	data->subscribedTo = TRUE;
	data->pending = 0;

	Friend_GetBestStatus(data, &status, &statusmsg, &chessparkfeatures);

	if (profile)
	{
                rating = Info_GetStandardRating(profile->ratings);
		roles = profile->roles;
		titles = profile->titles;
	}

	View_SetFriendStatus(barejid, data->nickname, NULL, status, statusmsg, data->avatarhash, chessparkfeatures ? rating : NULL, roles, titles, NULL, NULL, 0, 0, data->command);

	free(barejid);
	return 1;
}

void Model_AddChatMessage(char *jid, char *msg, char *msgid, int reqcomposing)
{
	char *barejid;

	if (Model_IsJIDIgnored(jid))
	{
		return;
	}

	barejid = Jid_Strip(jid);

	NamedList_RemoveByName(&chatactivejids, barejid);
	NamedList_AddToTop(&chatactivejids, barejid, strdup(jid), NULL);

	if (reqcomposing)
	{
		NamedList_RemoveByName(&messageids, barejid);
		NamedList_AddToTop(&messageids, barejid, strdup(msgid), NULL);
	}

	Log_Write(0, "Model_AddChatMessage\n");

	View_AddChatMessage(jid, jid, msg);

	free(barejid);
}

void Model_AddGroupChatMessage(char *jid, char *msg, char *timestamp)
{
	if (strstr(jid, "@games.chesspark.com"))
	{
		if (strncmp(jid, "tournament", 10) == 0)
		{
			char *tourneyid = strdup(jid + 10);
			char *nick = strchr(jid, '/');

			*strchr(tourneyid, '@') = '\0';

			if (nick)
			{
				nick++;
			}

			View_AddTourneyChatMessage(tourneyid, nick, msg);

			free(tourneyid);
		}
		else
		{
			char *gameid = strdup(jid);
			char *nick = strchr(jid, '/');

			*strchr(gameid, '@') = '\0';

			if (nick)
			{
				nick++;
			}

			View_AddGameChatMessage(gameid, nick, msg);

			free(gameid);
		}
	}
	else
	{
		View_AddGroupChatMessage(jid, msg, timestamp);
	}
}

int ModelMC_HandleMessage(struct messageinfo_s *info, void *dummy)
{
	if (!info->jid)
	{
		Log_Write(0, "IMPOSSIBLE: message without a from\n");
		return 1;
	}

	if (info->ismuc)
	{
		Model_AddGroupChatMessage(info->jid, info->text, info->timestamp);
	}
	else 
	{
		Model_AddChatMessage(info->jid, info->text, info->msgid, info->reqcomposing);
		Model_DoAutoAway(info->jid, 1);
	}

	return 1;
}

void ModelConn_SetGroupChatTopic(char *jid, char *topic)
{
	View_SetGroupChatTopic(jid, topic);
}

void Model_CancelLogin()
{
	login_connected = FALSE;
	discocause = 1;
	Conn_Disconnect();
	View_SetLoginNormal();
}

void Model_ReallyDisconnect()
{
	login_connected = FALSE;
	discocause = 2;
	Conn_Disconnect();
}

void Model_Disconnect()
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		if (View_IsPlayingAGame() && !Model_GetOption(OPTION_HIDEWARNINGONDISCONNECT))
		{
			View_PopupOfflineWarning();
		}
		else
		{
			Model_ReallyDisconnect();
		}
	}
}

void ModelConn_LostConnection(enum connerror_e error)
{
	View_SetPresence(SSTAT_OFFLINE, NULL);

	Log_Write(0, "error %d discocause %d\n", error, discocause);

	login_connected = FALSE;

	if (discocause == 1)
	{
		discocause = 0;
	}
	else if (discocause == 2)
	{
		if (login_fromloginwindow)
		{
			View_SetLoginFailed(error == CONNERROR_BADPASSWORD);
		}
		else
		{
			switch(error)
			{
				case CONNERROR_NONE:
					View_RosterDisconnectError(_("No error."), 0);
					break;
				case CONNERROR_BADPASSWORD:
					View_RosterDisconnectError(_("Bad Password."), 1);
					break;
				case CONNERROR_LOSTCONNECTION:
					View_RosterDisconnectError(_("Lost connection."), 1);
					break;
				case CONNERROR_CANTCONNECT:
					View_RosterDisconnectError(_("Can't connect."), 1);
					break;
				case CONNERROR_CONFLICT:
					View_RosterDisconnectError(_("Resource conflict."), 1);
					break;
				case CONNERROR_UNKNOWN:
				default:
					View_RosterDisconnectError(_("Unknown."), 1);
					break;
			}
			
			if (View_IsPlayingAGame() && !Model_GetOption(OPTION_HIDEWARNINGONDISCONNECT) && error != CONNERROR_NONE)
			{
				View_PopupDisconnectWarning();
			}
			View_SetDisconnectOnChats();
			View_ShowDisconnectOnChats();
			View_CloseNewGamesDialog();
			View_CloseAllGameRequestDialogs();
			View_CloseNewGamesDialog();
		}
		discocause = 0;
	}
	else
	{
		if (login_fromloginwindow)
		{
			View_SetLoginFailed(error == CONNERROR_BADPASSWORD);
		}
		else
		{
			if (error != CONNERROR_CONFLICT)
			{
				discocause = 2;
				View_SetDisconnectOnChats();
				View_ShowDisconnectOnChats();
				View_RosterAutoReconnect();
				Ctrl_LoginLast();
			}
			else
			{
				View_RosterDisconnectError(_("Resource conflict."), 1);
				View_SetDisconnectOnChats();
				View_ShowDisconnectOnChats();
				View_CloseAllGameRequestDialogs();
				View_CloseNewGamesDialog();
				login_changed = 0;
			}
		}
	}
}

void Model_TestDisconnect()
{
	Conn_Disconnect();
}

void Model_AddStatus(enum SStatus status, char *statusmsg)
{
	char *bareloginjid = Jid_Strip(login_jid);

	if (status == SSTAT_AWAY)
	{
		if (!StatusList_Find(awaystatuslist, statusmsg))
		{
			StatusList_Add(&awaystatuslist, SSTAT_AWAY, statusmsg);
			/*StatusList_Save(awaystatuslist, bareloginjid, "away_statuses");*/
			View_SetStatusList(SSTAT_AWAY, awaystatuslist);
		}
	}
	else if (status == SSTAT_AVAILABLE)
	{
		if (!StatusList_Find(availstatuslist, statusmsg))
		{
			StatusList_Add(&availstatuslist, SSTAT_AVAILABLE, statusmsg);
			/*StatusList_Save(availstatuslist, bareloginjid, "avail_statuses");*/
			View_SetStatusList(SSTAT_AVAILABLE, availstatuslist);
		}
	}

	Model_SaveServerPrefs();
	free (bareloginjid);
}

void Model_SetStatusList(enum SStatus status, struct StatusList_s *list)
{
	char *bareloginjid = Jid_Strip(login_jid);

	if (status == SSTAT_AWAY)
	{
		StatusList_Destroy(&awaystatuslist);
		awaystatuslist = StatusList_Copy(list);
		/*StatusList_Save(awaystatuslist, bareloginjid, "away_statuses");*/
		View_SetStatusList(SSTAT_AWAY, list);
	}
	else if (status == SSTAT_AVAILABLE)
	{
		StatusList_Destroy(&availstatuslist);
		availstatuslist = StatusList_Copy(list);
		/*StatusList_Save(availstatuslist, bareloginjid, "avail_statuses");*/
		View_SetStatusList(SSTAT_AVAILABLE, list);
	}

	if (userstatus == status && userstatusmsg)
	{
		struct StatusList_s *entry = list;
		int remove = 1;

		while (entry && remove)
		{
			if (entry->statusmsg && strcmp(entry->statusmsg, userstatusmsg) == 0)
			{
				remove = 0;
			}
			entry = entry->next;
		}

		if (remove)
		{
			Ctrl_SetPresence(userstatus, NULL);
		}
	}

	Model_SaveServerPrefs();

	free (bareloginjid);
}

char *Model_GetDefaultNick()
{
	char *bareloginjid = Jid_Strip(login_jid);

	struct profile_s *profile = Model_GetProfile(bareloginjid, 0);

	free (bareloginjid);

	if (profile && profile->nickname && profile->nickname[0])
	{
		return strdup(profile->nickname);
	}

	return Jid_GetBeforeAt(login_jid);
}

char *Model_GetLoginJid()
{
	return login_jid;
}

int Model_IsLoginJid(char *jid)
{
	char *bareloginjid = Jid_Strip(login_jid);
	char *barejid = Jid_Strip(jid);
	int match = stricmp(bareloginjid, barejid);

	free(barejid);
	free(bareloginjid);

	return match == 0;
}

void Model_SetFriendAvatarHash(char *jid, char *hash, int vcard, int retrieve)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;
	char *filename;
	char buffer[MAX_PATH];

	{
		char *bareloginjid = Jid_Strip(login_jid);

		if (stricmp(bareloginjid, barejid) == 0)
		{
			Model_SetAvatarToHash(hash);
		}
	}

	{
		struct profile_s *profile = Model_GetProfile(jid, 1);

		if (!profile)
		{
			profile = malloc(sizeof(*profile));
			memset(profile, 0, sizeof(*profile));
			NamedList_Add(&profilecache, barejid, profile, Info_DestroyProfile);
		}

		free(profile->avatarhash);
		profile->avatarhash = strdup(hash);

		Model_UpdateSubscribers(jid);
	}

	if (!ppfriend)
	{
		free(barejid);
		return;
	}

	data = (*ppfriend)->data;


	free(data->avatarhash);
	data->avatarhash = strdup(hash);

	filename = Ctrl_GetAvatarFilenameByHash(hash, buffer, MAX_PATH);

	if (!filename)
	{
		if (vcard && retrieve)
		{
			/*if (!Model_GetOption(OPTION_LOWBANDWIDTH))*/
			{
				if (strstr(barejid, "@chesspark.com"))
				{
					Model_RequestAvatarService(hash, barejid);
				}
				else
				{
					Conn_RequestvCard(barejid);
				}
				return;
			}
		}
	}

	Model_ViewUpdateFriend(jid);
}

char *Model_GetFriendAvatarHash(char *jid)
{
	char *barejid = Jid_Strip(jid);
	char *barejid2 = Jid_Strip(login_jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;

	if (stricmp(barejid, barejid2) == 0)
	{
		free(barejid);
		free(barejid2);
		return avatarhash;
	}

	free(barejid2);

	if (!ppfriend)
	{
		free(barejid);
		return NULL;
	}

	data = (*ppfriend)->data;

	return data->avatarhash;
}


static struct namedlist_s *rooms = NULL;

void Model_ParseNextRoom();

int Model_ParseRoomTimeout(void *userdata)
{
	if (!rooms)
	{
		return 1;
	}

	if (strstr(rooms->name, "@chat.chesspark.com"))
	{
		View_AddChatroom(_("Public Chat Rooms"), rooms->name, NULL, _("Room info timeout"), -1);
	}

	if (NamedList_GetByName(&favechats, rooms->name))
	{
		View_AddChatroom(_("Favorites"), rooms->name, NULL, _("Room info timeout"), -1);
	}

	NamedList_Remove(&rooms);
	Model_ParseNextRoom();

	return 0;
}

void Model_ParseNextRoom()
{
	Model_RemoveTimeout(Model_ParseRoomTimeout, 3000);
	if (rooms)
	{
		if (Conn_GetConnState() == CONN_CONNECTED)
		{
			Model_AddTimeout(Model_ParseRoomTimeout, 3000, NULL);
			Conn_GetRoomInfo(rooms->name);
		}
		else
		{
			Model_ParseRoomTimeout(NULL);
		}
	}
	else
	{
		View_SetChatroomLoadStatusFinished();
	}
}

void ModelConn_AddChatroom(char *jid, char *name, char *topic, int users)
{
	if (strstr(jid, "@chat.chesspark.com"))
	{
		View_AddChatroom(_("Public Chat Rooms"), jid, name, topic, users);
	}

	if (NamedList_GetByName(&favechats, jid))
	{
		View_AddChatroom(_("Favorites"), jid, name, topic, users);
	}

	if (rooms && stricmp(rooms->name, jid) == 0)
	{
		NamedList_Remove(&rooms);
		Model_ParseNextRoom();
	}
}

void ModelConn_AddChatroomError(char *jid)
{
	if (NamedList_GetByName(&favechats, jid))
	{
		View_AddChatroom(_("Favorites"), jid, NULL, _("No information available"), -1);
	}
	if (rooms && stricmp(rooms->name, jid) == 0)
	{
		NamedList_Remove(&rooms);
		Model_ParseNextRoom();
	}
}

void ModelConn_ParseRoomNames(struct namedlist_s *newroomlist)
{
	struct namedlist_s *current;
	if (rooms)
	{
		NamedList_Destroy(&rooms);
	}

	rooms = NamedList_DupeStringList(newroomlist);

	/* add all rooms in favorites so they are refreshed as well */
	current = favechats;
	while (current)
	{
		if (!NamedList_GetByName(&rooms, current->name))
		{
			NamedList_AddString(&rooms, current->name, NULL);
		}
		current = current->next;
	}

	Model_ParseNextRoom();
}

void Model_GetRooms()
{
	struct namedlist_s *entry;
	View_ClearChatrooms();
	entry = favechats;

	while (entry)
	{
		View_AddChatroom(_("Favorites"), entry->name, NULL, "", 0);
		entry = entry->next;
	}

	if (inhiddengroup)
	{
		/* we're in a hidden group, don't get the actual list of rooms, just refresh the favorites */
		ModelConn_ParseRoomNames(NULL);
	}
	else
	{
                Conn_GetRooms();
	}
}

void Model_SetTopic(char *chatjid, char *topic)
{
	Conn_SetTopic(chatjid, topic);
}

void Model_ChangeNick(char *chatjid, char *newnick)
{
	char *tojid;

	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	tojid = malloc(strlen(chatjid) + strlen(newnick) + 2);

	strcpy(tojid, chatjid);
	strcat(tojid, "/");
	strcat(tojid, newnick);

	Conn_SendPresence(userstatus, NULL, tojid, 1, NULL, 0, 0);

	free(tojid);
}

void Model_KickUser(char *chatjid, char *nick, char *reason)
{
	Conn_KickUser(chatjid, nick, reason);
}

void Model_BanUser(char *chatjid, char *nick, char *reason)
{
	Conn_BanUser(chatjid, nick, reason);
}

void Model_UnbanUser(char *chatjid, char *nick, char *reason)
{
	Conn_UnbanUser(chatjid, nick, reason);
}

void Model_InviteUser(char *chatjid, char *userjid, char *reason)
{
	Conn_InviteUser(chatjid, userjid, reason);
}

void Model_GiveVoice(char *chatjid, char *nick, char *reason)
{
	Conn_GiveVoice(chatjid, nick, reason);
}

void Model_GiveMod(char *chatjid, char *userjid, char *reason)
{
	Conn_GiveMod(chatjid, userjid, reason);
}

void Model_GiveAdmin(char *chatjid, char *userjid, char *reason)
{
	Conn_GiveAdmin(chatjid, userjid, reason);
}

void Model_GiveOwner(char *chatjid, char *userjid, char *reason)
{
	Conn_GiveOwner(chatjid, userjid, reason);
}

void Model_RevokeVoice(char *chatjid, char *nick, char *reason)
{
	Conn_RevokeVoice(chatjid, nick, reason);
}

void Model_RevokeMod(char *chatjid, char *userjid, char *reason)
{
	Conn_RevokeMod(chatjid, userjid, reason);
}

void Model_RevokeAdmin(char *chatjid, char *userjid, char *reason)
{
	Conn_RevokeAdmin(chatjid, userjid, reason);
}

void Model_RevokeOwner(char *chatjid, char *userjid, char *reason)
{
	Conn_RevokeOwner(chatjid, userjid, reason);
}

void Model_ClearChatHistory(char *chatjid)
{
	Conn_ClearChatHistory(chatjid);
}

void Model_AddChatToFavorites(char *chatjid)
{
	char *bareloginjid = Jid_Strip(login_jid);

	if (NamedList_GetByName(&favechats, chatjid))
	{
		return;
	}

	NamedList_Add(&favechats, chatjid, NULL, NULL);
	Conn_GetRoomInfo(chatjid);

	Model_SaveServerPrefs();

	free (bareloginjid);
}

void Model_RemoveChatFromFavorites(char *chatjid)
{
	char *bareloginjid = Jid_Strip(login_jid);

	Model_RemoveChatFromAutojoin(chatjid);

	NamedList_RemoveByName(&favechats, chatjid);
	View_RemoveChatroom(_("Favorites"), chatjid);

	Model_SaveServerPrefs();

	free (bareloginjid);
}

void Model_AddChatToAutojoin(char *chatjid)
{
	char *bareloginjid = Jid_Strip(login_jid);

	if (NamedList_GetByName(&autojoinchats, chatjid))
	{
		return;
	}

	Model_AddChatToFavorites(chatjid);

	NamedList_Add(&autojoinchats, chatjid, NULL, NULL);

	Model_SaveServerPrefs();

	free (bareloginjid);
}

void Model_RemoveChatFromAutojoin(char *chatjid)
{
	char *bareloginjid = Jid_Strip(login_jid);

	NamedList_RemoveByName(&autojoinchats, chatjid);

	Model_SaveServerPrefs();

	free (bareloginjid);
}

int Model_IsChatInFavorites(char *chatjid)
{
	if (NamedList_GetByName(&favechats, chatjid))
	{
		return TRUE;
	}
	return FALSE;
}

int Model_IsChatInAutojoin(char *chatjid)
{
	if (NamedList_GetByName(&autojoinchats, chatjid))
	{
		return TRUE;
	}
	return FALSE;
}

void Model_SendMessage(char *jid, char *text)
{
	if (login_connected)
	{
		char *barejid = Jid_Strip(jid);
		struct namedlist_s **active = NamedList_GetByName(&chatactivejids, barejid);
		unsigned int currenttick = GetTickCount();
		
		if (active)
		{
			jid = (*active)->data;
		}

		NamedList_RemoveByName(&sentautoaway, barejid);

		if (Model_GetOption(OPTION_MESSAGEONAWAY))
		{
			NamedList_AddToTop(&sentautoaway, barejid, (void *)(currenttick), NULL);
		}

		free(barejid);

		Conn_SendMessage(jid, text, 0);
	}
}

void ModelConn_JoinGame(char *gameid, char *room, char *role, char *oldgameid)
{
	char *roomjid;
	char *nick;

	if (CHESSPARK_LOCALCHATSUSEJIDNICK)
	{
		nick = Jid_Strip(login_jid);
	}
	else
	{
		nick = Jid_GetBeforeAt(login_jid);
	}

	roomjid = malloc(strlen(room) + strlen(nick) + 2);

	strcpy(roomjid, room);
	strcat(roomjid, "/");
	strcat(roomjid, nick);

	View_PopupChessGame(gameid, roomjid, oldgameid);

	Conn_SendPresence(userstatus, NULL, roomjid, 1, NULL, 0, 0);

	free(roomjid);
	free(nick);
}

void ModelConn_ShowCorGameAccepted(char *gameid, char *from)
{
	View_ShowCorGameAccepted(gameid, from);
}

void ModelConn_ParseGameMove(char *gameid, char *move, char *annotation, char *whitejid, char *blackjid, int correspondence, int numtakebacks, int ply)
{
	char *barejid1 = Jid_Strip(login_jid);
	char *barejidw = Jid_Strip(whitejid);
	char *barejidb = Jid_Strip(blackjid);

	if (barejid1 && barejidw && barejidb && stricmp(barejid1, barejidw) == 0)
	{
		View_ParseGameMove(gameid, move, annotation, blackjid, correspondence, numtakebacks, ply);
	}
	else if (barejid1 && barejidw && barejidb && stricmp(barejid1, barejidb) == 0)
	{
		View_ParseGameMove(gameid, move, annotation, whitejid, correspondence, numtakebacks, ply);
	}
	else
	{
		View_ParseGameMove(gameid, move, annotation, NULL, correspondence, numtakebacks, ply);
	}
}

void ModelConn_ClearMoveList(char *gameid)
{
	View_ClearMoveList(gameid);
}

void ModelConn_SetGameMoveList(char *gameid, struct namedlist_s *textmovelist)
{
	View_SetGameMoveList(gameid, textmovelist);
}

void ModelConn_AddMoveToMoveList(char *gameid, char *move, char *annotation)
{
	View_AddMoveToMoveList(gameid, move, annotation);
}

void ModelConn_ResetGamePosition(char *gameid, int illegalmove)
{
	View_ResetGamePosition(gameid, illegalmove);
}

void ModelConn_MoveSuccess(char *gameid, unsigned int lag)
{
	View_SwitchGameClockToNonlocalPlayer(gameid, lag);
}

void ModelConn_SetGameState(char *gameid, char *initialstate, char *state,
        struct gamesearchinfo_s *info,
        char *whitecurrentclock, char *blackcurrentclock,
        char *tourneyid)
{
/*
	if (whitetimecontrol)
	{
		View_SetClockControl(gameid, "white", whitetimecontrol->controlarray);
	}

	if (blacktimecontrol)
	{
		View_SetClockControl(gameid, "black", blacktimecontrol->controlarray);
	}
*/
	if (whitecurrentclock)
	{
		View_SyncClock(gameid, whitecurrentclock, "white", NULL, 0);
	}

	if (blackcurrentclock)
	{
		View_SyncClock(gameid, blackcurrentclock, "black", NULL, 0);
	}

	if (info)
	{
		char *barewhitejid = Jid_Strip(info->white->jid);
		char *bareblackjid = Jid_Strip(info->black->jid);
		char *barelocaljid = Jid_Strip(login_jid);
		int whitelocal = 0, blacklocal = 0;

		if (stricmp(barewhitejid, barelocaljid) == 0)
		{
			View_SetGameViewRotated(gameid, FALSE);
			whitelocal = 1;
		}
		
		if (stricmp(bareblackjid, barelocaljid) == 0)
		{
			View_SetGameViewRotated(gameid, TRUE);
			blacklocal = 1;
		}

		View_SetGameInfo(gameid, info, whitelocal, blacklocal, tourneyid);

		free(barewhitejid);
		free(bareblackjid);
		free(barelocaljid);
	}

	if (state)
	{
		View_ParseGameState(gameid, initialstate, state);
	}
}

void ModelConn_SyncClock(char *gameid, char *time, char *side, char *control, int tick)
{
	View_SyncClock(gameid, time, side, control, tick);
}

void ModelConn_HandleFlagError(char *gameid, char *time)
{
	View_HandleFlagError(gameid, time);
}

void ModelConn_MucError(char *from, char *code)
{
	char *roomjid = Jid_Strip(from);
	int icode = 0;
	char msg[80];
	int ischessparkroom;

	if (code)
	{
		sscanf(code, "%d", &icode);
	}

	if (strstr(roomjid, "@games.chesspark.com"))
	{
		View_ShowGameMucError(Jid_GetBeforeAt(roomjid), icode);
		return;
	}

	ischessparkroom = (strstr(roomjid, "@chat.chesspark.com") != NULL);

	switch (icode)
	{
		case 401:
			if (ischessparkroom)
			{
				i18n_stringsub(msg, 80, _("You are not authorized to join %1."), Jid_GetBeforeAt(roomjid));
			}
			else
			{
				i18n_stringsub(msg, 80, _("You are not authorized to join %1."), roomjid);
			}
			View_GroupChatError(roomjid, icode, msg);
			break;
		case 402:
			View_GroupChatError(roomjid, icode, _("Error: Payment required."));
			break;
		case 403:
			if (ischessparkroom)
			{
				i18n_stringsub(msg, 80, _("You are currently banned from %1."), Jid_GetBeforeAt(roomjid));
			}
			else
			{
				i18n_stringsub(msg, 80, _("You are currently banned from %1."), roomjid);
			}
			View_GroupChatError(roomjid, icode, msg);
			break;
		case 404:
			View_GroupChatError(roomjid, icode, _("Error: Room not found."));
			break;
		case 405:
			/*if (ischessparkroom && Model_IsLocalMemberFree())
			{
				View_PopupNotAProMucError(roomjid);
			}
			else*/
			{
				View_GroupChatError(roomjid, icode, _("Error: Room creation is restricted."));
			}
			break;
		case 406:
			View_GroupChatError(roomjid, icode, _("Error: Reserved roomnick must be used."));
			break;
		case 407:
			if (ischessparkroom && Model_IsLocalMemberFree(0))
			{
				View_PopupNotAProMucError(roomjid);
			}
			else
			{
				View_GroupChatError(roomjid, icode, _("You are not on the member list"));
			}
			break;
		case 408:
			View_GroupChatError(roomjid, icode, _("Error: Request Timeout."));
			break;
		case 409:
			View_RoomNickConflict(roomjid);
			break;
		case 500:
			View_GroupChatError(roomjid, icode, _("Error: Internal server error."));
			break;
		case 501:
			View_GroupChatError(roomjid, icode, _("Error: Feature not implemented."));
			break;
		case 502:
			View_GroupChatError(roomjid, icode, _("Error: Remote server error."));
			break;
		case 503:
			View_GroupChatError(roomjid, icode, _("Error: Service unavailable."));
			break;
		case 504:
			View_GroupChatError(roomjid, icode, _("Error: Remote server timeout."));
			break;
		case 510:
			View_AddGroupChatMessage(roomjid, _("Error: Disconnected."), NULL);
			break;
		default:
			i18n_stringsub(msg, 80, _("Unknown error %1"), code);
			View_AddGroupChatMessage(roomjid, msg, NULL);
			break;
	}

	free(roomjid);
}

void ModelConn_SetTopicError(char *roomjid)
{
	View_AddGroupChatMessage(roomjid, _("Error: You are not authorized to change topic."), NULL);
}

void ModelConn_MucModerationError(char *roomjid)
{
	View_AddGroupChatMessage(roomjid, _("Error: You lack sufficient privileges for that moderation option."), NULL);
}

void ModelConn_InviteUserError(char *roomjid)
{
	View_AddGroupChatMessage(roomjid, _("Error: You lack sufficient privileges to invite a user."), NULL);
}

void ModelConn_HandleClearChatHistorySuccess(char *roomjid)
{
	View_AddGroupChatMessage(roomjid, _("Room chat history cleared."), NULL);
}

void ModelConn_ClearGamesSearch(char *node)
{
	if (stricmp(node, "adjourned") == 0)
	{
		NamedList_Destroy(&adjournedgames);
	}
	View_ClearGamesSearch(node);
}

void ModelConn_AddSearchGame(char *itemid, char *node, char *jid, struct gamesearchinfo_s *info)
{
	if (stricmp(node, "adjourned") == 0)
	{
		NamedList_Add(&adjournedgames, itemid, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
	}

	View_AddSearchGame(itemid, node, jid, info);
}

void ModelConn_AddSearchTournament(struct tournamentinfo_s *info)
{
	View_AddSearchTournament(info);
}

void ModelConn_FinishGameResults(int noresults)
{
	View_FinishGameResults(noresults);
}

void ModelConn_RequestMatch(char *from, struct gamesearchinfo_s *info, char *oldgameid)
{
	char *barejid = Jid_Strip(from);

	if (Model_IsJIDIgnored(from))
	{
		Ctrl_DeclineMatch(info->gameid);
		return;
	}

	if (Model_DoAutoAway(from, 0) && !Model_GetOption(OPTION_NODECLINEWHENAWAY))
	{
		Ctrl_DeclineMatch(info->gameid);
		return;
	}

	NamedList_RemoveByName(&gameinfos, barejid);
	NamedList_Add(&gameinfos, barejid, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);

	free(barejid);

	View_PopupGameInvitationDialog(from, info, oldgameid);
}

void Model_RequestMatch(char *opponentjid, struct gamesearchinfo_s *info)
{
	char *barejid;
	char *resource = NULL;
	struct namedlist_s **ppfriend;

	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	barejid = Jid_Strip(opponentjid);
	ppfriend = NamedList_GetByNameAndBump(&roster, barejid);

	NamedList_RemoveByName(&gameinfos, barejid);
	NamedList_Add(&gameinfos, barejid, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
/*
	if (!ppfriend)
	{
		data = malloc(sizeof(*data));
		memset(data, 0, sizeof(*data));
		NamedList_Add(&roster, barejid, data, FriendData_Destroy);
	}
	else
	{
		struct frienddata_s *data = (*ppfriend)->data;

		if (NamedList_GetByName(&(data->resources), "cpc"))
		{
			resource = "cpc";
		}
		else
		{
			if (data->resources)
			{
				resource = data->resources->name;
			}
		}
		
	}

	strcpy(finaljid, barejid);
	if (resource)
	{
		strcat(finaljid, "/");
		strcat(finaljid, resource);
	}
*/
	Conn_RequestMatch(barejid, info);

	free(barejid);
}

void Model_ShowLocalProfile()
{
	char *bareloginjid = Jid_Strip(login_jid);
	struct namedlist_s **entry = NamedList_GetByName(&profilecache, bareloginjid);

	if (entry)
	{
		struct profile_s *profile = (*entry)->data;
		View_SetRosterProfile(profile->ratings, profile->roles, profile->titles);
	}

	free(bareloginjid);
}

void Model_UpdateLocalRating(struct rating_s *rating, char *category)
{
	char *bareloginjid = Jid_Strip(login_jid);
	struct namedlist_s **profilelistentry;

	profilelistentry = NamedList_GetByName(&profilecache, bareloginjid);

	if (profilelistentry)
	{
		struct profile_s *profile = (*profilelistentry)->data;

		NamedList_RemoveByName(&(profile->ratings), category);
		NamedList_Add(&(profile->ratings), category, Info_DupeRating(rating), Info_DestroyRating);

		Model_ShowLocalProfile();
	}

	free(bareloginjid);
}

void Model_UpdateSubscribers(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **entry = NamedList_GetByName(&profilesubscriptions, barejid);
	struct profile_s *profile;
	struct namedlist_s **pentry = NamedList_GetByName(&profilecache, barejid);

	if (!pentry)
	{
		free(barejid);
		return;
	}

	profile = (*pentry)->data;

	while (entry && *entry)
	{
		struct profilesubscription_s *ps = (*entry)->data;

		ps->callbackfunc(barejid, profile, ps->userdata);

		entry = NamedList_GetNextByName(entry, barejid);
	}

	free(barejid);
}

void Model_SubscribeProfileReal(char *jid, void (*callbackfunc)(char *, struct profile_s *, void *), void *userdata, int reqlevel)
{
	char *barejid = Jid_Strip(jid);
	struct profilesubscription_s *ps;
	struct profile_s *profile;

	ps = malloc(sizeof(*ps));
	memset(ps, 0, sizeof(*ps));

	ps->callbackfunc = callbackfunc;
	ps->userdata     = userdata;

	NamedList_Add(&profilesubscriptions, barejid, ps, NULL);

	profile = Model_GetProfile(jid, reqlevel);

	if (profile)
	{
		ps->callbackfunc(jid, profile, userdata);
	}

	free(barejid);

}
void Model_SubscribeProfileNoRequest(char *jid, void (*callbackfunc)(char *, struct profile_s *, void *), void *userdata)
{
	Model_SubscribeProfileReal(jid, callbackfunc, userdata, 0);
}

void Model_SubscribeProfile(char *jid, void (*callbackfunc)(char *, struct profile_s *, void *), void *userdata)
{
	Model_SubscribeProfileReal(jid, callbackfunc, userdata, 1);
}

void Model_SubscribeProfileForceRequest(char *jid, void (*callbackfunc)(char *, struct profile_s *, void *), void *userdata)
{
	struct profile_s *profile;
	{
                char *barejid = Jid_Strip(jid);
		char *bareloginjid = Jid_Strip(login_jid);

		/* never force a profile get for yourself or someone on the roster */
		if (stricmp(barejid, bareloginjid) == 0 || NamedList_GetByName(&roster, barejid))
		{
			Model_SubscribeProfileReal(jid, callbackfunc, userdata, 1);
		}
		else
		{
			Model_SubscribeProfileReal(jid, callbackfunc, userdata, 2);
		}

		free(barejid);
	}

	profile = Model_GetProfile(jid, 0);

	if (Conn_GetConnState() == CONN_CONNECTED && (!profile || !profile->avatarhash))
	{
		char *barejid = Jid_Strip(jid);
		/*if (!Model_GetOption(OPTION_LOWBANDWIDTH))*/
		{
			if (strstr(barejid, "@chesspark.com"))
			{
				Model_RequestAvatarServiceJid(barejid);
			}
			else
			{
				Conn_RequestvCard(barejid);
			}
		}
		free(barejid);
	}
}


void Model_SubscribeProfileAndAvatar(char *jid, void (*callbackfunc)(char *, struct profile_s *, void *), void *userdata)
{
	struct profile_s *profile;

	Model_SubscribeProfileForceRequest(jid, callbackfunc, userdata);

	profile = Model_GetProfile(jid, 0);
}

void Model_UnsubscribeProfile(char *jid, void (*callbackfunc)(char *, struct profile_s *, void *), void *userdata)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **entry;

	entry = NamedList_GetByName(&profilesubscriptions, barejid);

	while (entry && *entry)
	{
		struct profilesubscription_s *ps = (*entry)->data;

		if ((ps->userdata == userdata) && (ps->callbackfunc == callbackfunc))
		{
			NamedList_Remove(entry);
			free(barejid);
			return;
		}

		entry = NamedList_GetNextByName(entry, barejid);
	}

	free(barejid);
}

void ModelConn_SetProfile(char *jid, struct namedlist_s *profileinfo)
{
	char *barejid = Jid_Strip(jid);
	char *bareloginjid;
	char *resource = Jid_GetResource(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *data;
	struct namedlist_s **profilelistentry = NULL;
	struct adhoccommand_s *command = NULL;
	struct profile_s *profile = NULL;

	bareloginjid = Jid_Strip(login_jid);

	profile = Model_GetProfile(jid, 0);

	if (!profile)
	{
		profile = malloc(sizeof(*profile));
		memset(profile, 0, sizeof(*profile));
		NamedList_Add(&profilecache, barejid, profile, Info_DestroyProfile);
	}

	if (stricmp(barejid, bareloginjid) == 0)
	{
		profilelistentry = NamedList_GetByName(&profileinfo, "Nick");
		if (profilelistentry)
		{
			profile->nickname = Util_StripLeadingTrailingSpaces((*profilelistentry)->data);
		}
		else
		{
			profile->nickname = NULL;
		}
	}
	else if (ppfriend)
	{
		struct frienddata_s *data = (*ppfriend)->data;
		profile->nickname = strdup(data->nickname);
	}
	
	profilelistentry = NamedList_GetByName(&profileinfo, "Ratings");
	if (profilelistentry)
	{
		NamedList_Destroy(&(profile->ratings));
		profile->ratings = Info_DupeRatingList((*profilelistentry)->data);
	}

	profilelistentry = NamedList_GetByName(&profileinfo, "Roles");
	if (profilelistentry)
	{
		NamedList_Destroy(&(profile->roles));
		profile->roles = NamedList_DupeStringList((*profilelistentry)->data);
	}

	profilelistentry = NamedList_GetByName(&profileinfo, "Titles");
	if (profilelistentry)
	{
		NamedList_Destroy(&(profile->titles));
		profile->titles = NamedList_DupeStringList((*profilelistentry)->data);
	}

	profilelistentry = NamedList_GetByName(&profileinfo, "Description");
	if (profilelistentry)
	{
		free(profile->description);
		profile->description = strdup((*profilelistentry)->data);
	}

	profilelistentry = NamedList_GetByName(&profileinfo, "ad-hoc");
	if (profilelistentry)
	{
		command = (*profilelistentry)->data;
	}

	profilelistentry = NamedList_GetByName(&profileinfo, "member-since");
	if (profilelistentry)
	{
		profile->membersince = strdup((*profilelistentry)->data);
	}

	profilelistentry = NamedList_GetByName(&profileinfo, "last-online");
	if (profilelistentry)
	{
		profile->lastonline = strdup((*profilelistentry)->data);
	}

	profilelistentry = NamedList_GetByName(&profileinfo, "client_info");
	if (profilelistentry)
	{
		struct namedlist_s **entry;
		struct namedlist_s *list;

		free(profile->clientname);
		free(profile->clientversion);
		free(profile->clientvendor);
		free(profile->clientos);

		profile->clientname = NULL;
		profile->clientversion = NULL;
		profile->clientvendor = NULL;
		profile->clientos = NULL;

		list = (struct namedlist_s *)((*profilelistentry)->data);

		entry = NamedList_GetByName(&list, "name");
		if (entry)
		{
			profile->clientname = strdup((*entry)->data);
		}

		entry = NamedList_GetByName(&list, "version");
		if (entry)
		{
			profile->clientversion = strdup((*entry)->data);
		}

		entry = NamedList_GetByName(&list, "vendor");
		if (entry)
		{
			profile->clientvendor = strdup((*entry)->data);
		}

		entry = NamedList_GetByName(&list, "os");
		if (entry)
		{
			profile->clientos = strdup((*entry)->data);
		}
	}

	profilelistentry = NamedList_GetByName(&profileinfo, "member-type");
	if (profilelistentry)
	{
		profile->membertype = strdup((*profilelistentry)->data);
	}

	profilelistentry = NamedList_GetByName(&profileinfo, "country");
	if (profilelistentry)
	{
		profile->countrycode = strdup((*profilelistentry)->data);
	}

	profilelistentry = NamedList_GetByName(&profileinfo, "location");
	if (profilelistentry)
	{
		profile->location = strdup((*profilelistentry)->data);
	}

	profilelistentry = NamedList_GetByName(&profileinfo, "rank");
	if (profilelistentry)
	{
		profile->rank = strdup((*profilelistentry)->data);
	}


	profilelistentry = NamedList_GetByName(&profileinfo, "groups");
	if (profilelistentry)
	{
		profile->groups = NamedList_DupeList((*profilelistentry)->data, Info_DupeGroupInfo, Info_DestroyGroupInfo);
	}

	if (!profile->avatarhash)
	{
		char *bareloginjid = Jid_Strip(login_jid);

		if (stricmp(barejid, bareloginjid) == 0)
		{
			profile->avatarhash = strdup(avatarhash);
		}
		else if (ppfriend)
		{
			struct frienddata_s *data = (*ppfriend)->data;
			profile->avatarhash = strdup(data->avatarhash);
		}
	}

	if (stricmp(barejid, bareloginjid) == 0)
	{
		Model_ShowLocalProfile();
	}

	free(bareloginjid);

	Model_UpdateSubscribers(jid);
	
/*
	View_SetProfile(barejid, profile);
*/
	if (!ppfriend)
	{
		free(barejid);
		free(resource);
		return;
	}

	data = (*ppfriend)->data;

	Info_DestroyAdHocCommand(data->command);
	data->command = Info_DupeAdHocCommand(command);

	Model_ViewUpdateFriend(jid);

	free(barejid);
	free(resource);
}


void ModelConn_SetProfileDescription(char *jid, char *description)
{
	char *barejid = Jid_Strip(jid);
	char *bareloginjid = Jid_Strip(login_jid);
	struct namedlist_s **profilelistentry = NULL;
	struct profile_s *profile = NULL;

	profilelistentry = NamedList_GetByName(&profilecache, barejid);

	if (!profilelistentry)
	{
		free(barejid);
		free(bareloginjid);
		return;
	}

	profile = (*profilelistentry)->data;

	free(profile->description);
	profile->description = strdup(description);

	if (stricmp(barejid, bareloginjid) == 0)
	{
		Model_ShowLocalProfile();
	}
	else
	{
		Model_ViewUpdateFriend(jid);
	}

	{
		struct namedlist_s **entry = NamedList_GetByName(&profilesubscriptions, barejid);

		while (entry && *entry)
		{
			struct profilesubscription_s *ps = (*entry)->data;

			ps->callbackfunc(barejid, profile, ps->userdata);

			entry = NamedList_GetNextByName(entry, barejid);
		}
	}

	free(bareloginjid);
	free(barejid);
}

struct profile_s *Model_GetProfile(char *jid, int request)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **entry = NamedList_GetByName(&profilecache, barejid);

	if ((!entry && request) || request == 2)
	{
		Ctrl_RequestProfile(barejid);
	}

	free(barejid);

	if (entry)
	{
		return (*entry)->data;
	}

	return NULL;
}

void ModelConn_SetRating(char *jid, struct rating_s *rating, char *category, char *from)
{
	char *barejid = Jid_Strip(jid);
	char *bareloginjid = Jid_Strip(login_jid);
	struct namedlist_s **profilelistentry = NULL;
	struct profile_s *profile = NULL;
	
	if (stricmp(barejid, bareloginjid) != 0 && !NamedList_GetByName(&roster, barejid) && from && stricmp(from, "ratings.chesspark.com") == 0)
	{
		/* not on the roster, so unsubscribe to this rating and profile*/
		Conn_UnsubscribeRating(barejid);
		Conn_UnsubscribeProfile(barejid);
	}

	profilelistentry = NamedList_GetByName(&profilecache, barejid);

	if (!profilelistentry)
	{
		free(barejid);
		free(bareloginjid);
		return;
	}

	profile = (*profilelistentry)->data;

	if (stricmp(from, "ratings.chesspark.com") != 0)
	{
		View_ShowRatingUpdate(from, jid, rating);
	}

	NamedList_RemoveByName(&(profile->ratings), category);
	NamedList_Add(&(profile->ratings), category, Info_DupeRating(rating), Info_DestroyRating);

	if (stricmp(barejid, bareloginjid) == 0)
	{
		Model_ShowLocalProfile();
	}
	else
	{
		Model_ViewUpdateFriend(jid);
	}

	{
		struct namedlist_s **entry = NamedList_GetByName(&profilesubscriptions, barejid);

		while (entry && *entry)
		{
			struct profilesubscription_s *ps = (*entry)->data;

			ps->callbackfunc(barejid, profile, ps->userdata);

			entry = NamedList_GetNextByName(entry, barejid);
		}
	}

	free(bareloginjid);
	free(barejid);
}


void ModelConn_HandleAdjourn(char *gameid, char *white, char *black)
{
	View_HandleAdjourn(gameid, white, black);
}

void ModelConn_HandleAbort(char *gameid, char *white, char *black)
{
	View_HandleAbort(gameid, white, black);
}

void ModelConn_HandleTakeback(char *gameid, char *white, char *black)
{
	View_HandleTakeback(gameid, white, black);
}

void ModelConn_HandleDraw(char *gameid, char *from, int whiteaccept, int blackaccept, int correspondence)
{
	/*
	char *barewhitejid = Jid_Strip(whitejid);
	char *bareblackjid = Jid_Strip(blackjid);
	char *barelocaljid = Jid_Strip(login_jid);
	int whitelocal = 0, blacklocal = 0;
	char *opponentjid = NULL, *nick = NULL;

	if (barewhitejid && stricmp(barewhitejid, barelocaljid) == 0)
	{
		whitelocal = 1;
	}
	else if (bareblackjid && stricmp(bareblackjid, barelocaljid) == 0)
	{
		blacklocal = 1;
	}
*/
	View_HandleDraw(gameid, from, whiteaccept, blackaccept, correspondence);
}

void ModelConn_HandleResign(char *gameid)
{
/*	View_HandleResign(gameid);*/
}

void ModelConn_SetFriendHasChesspark(char *jid)
{
	char *barejid = Jid_Strip(jid);
	char *resource = Jid_GetResource(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *frienddata;
	struct namedlist_s **ppresource;
	struct resourcedata_s *data;

	if (!ppfriend)
	{
		free(barejid);
		free(resource);
		return;
	}

	frienddata = (*ppfriend)->data;

	ppresource = NamedList_GetByName(&(frienddata->resources), resource);

	if (!ppresource)
	{
		free(barejid);
		free(resource);
		return;
	}

	data = (*ppresource)->data;

	data->chessparkfeatures = TRUE;

	Model_ViewUpdateFriend(jid);
	/* Don't get the profile now, get it later after the subscribe succeeds */
	/*Model_GetProfile(jid, 1); */
	Conn_SubscribeRating(barejid);
	Conn_SubscribeProfile(barejid);

	free(barejid);
	free(resource);
}

void ModelConn_RequestReconvene(char *from, char *gameid, struct gamesearchinfo_s *info)
{
	if (Model_IsJIDIgnored(from))
	{
		Ctrl_DeclineMatch(gameid);
		return;
	}

	if (Model_DoAutoAway(from, 0) && !Model_GetOption(OPTION_NODECLINEWHENAWAY))
	{
		Ctrl_DeclineMatch(gameid);
		return;
	}

	View_PopupReconveneDialog(from, gameid, info, 0);
}

void ModelConn_HandleGameOver(char *gameid, char *whitejid, char *blackjid, char *type, char *win, char *lose, char *reason, int correspondence, char *datestarted)
{
	char *barewhitejid = Jid_Strip(whitejid);
	char *bareblackjid = Jid_Strip(blackjid);
	char *barelocaljid = Jid_Strip(login_jid);
	char *opponentjid = NULL;
	int localwin = 0, locallose = 0;

	if (barewhitejid && bareblackjid && stricmp(barewhitejid, barelocaljid) == 0)
	{
		if (win && stricmp(win, "white") == 0)
		{
			localwin = 1;
		}

		if (lose && stricmp(lose, "white") == 0)
		{
			locallose = 1;
		}

		opponentjid = blackjid;
	}
	else if (barewhitejid && bareblackjid && stricmp(bareblackjid, barelocaljid) == 0)
	{
		if (win && stricmp(win, "black") == 0)
		{
			localwin = 1;
		}

		if (lose && stricmp(lose, "black") == 0)
		{
			locallose = 1;
		}

		opponentjid = whitejid;
	}

	if (type && stricmp(type, "adjourned") == 0)
	{
		if (!(NamedList_GetByName(&adjournedgames, gameid)))
		{
			struct gamesearchinfo_s *info = View_GetGameInfo(gameid);

			if (!info)
			{
				/* fake the info, we have just enough info to reconvene */
				info = malloc(sizeof(*info));
				memset(info, 0, sizeof(*info));
				
				info->white = malloc(sizeof(*(info->white)));
				memset(info->white, 0, sizeof(*(info->white)));
				info->white->jid = strdup(whitejid);

				info->black = malloc(sizeof(*(info->black)));
				memset(info->black, 0, sizeof(*(info->black)));
				info->black->jid = strdup(blackjid);

				info->gameid = strdup(gameid);
			}

			NamedList_Add(&adjournedgames, gameid, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
		}
	}
	else
	{
                NamedList_RemoveByName(&adjournedgames, gameid);
	}

	View_HandleGameOver(gameid, type, win, lose, reason, correspondence, opponentjid, localwin, locallose, datestarted);
}


void ModelConn_HandleRejectDraw(char *gameid, char *from, int correspondence)
{
	View_HandleRejectDraw(gameid, from, correspondence);
}

void ModelConn_HandleRejectAdjourn(char *gameid)
{
	View_HandleRejectAdjourn(gameid);
}

void ModelConn_HandleRejectAbort(char *gameid)
{
	View_HandleRejectAbort(gameid);
}

void ModelConn_HandleRejectTakeback(char *gameid)
{
	View_HandleRejectTakeback(gameid);
}

void Model_ShowProfile(char *jid)
{
	char *barejid = Jid_Strip(jid);
	char *bareloginjid;
	struct namedlist_s **ppfriend;
	struct frienddata_s *frienddata;

	bareloginjid = Jid_Strip(login_jid);

	if (stricmp(barejid, bareloginjid) == 0)
	{
		struct profile_s *profile = Model_GetProfile(bareloginjid, 0);
		char *nickname = NULL;

		if (profile)
		{
			nickname = profile->nickname;
		}

		View_PopupProfileDialog(barejid, nickname);
		free(barejid);
		free(bareloginjid);
		return;
	}

	free(bareloginjid);

	ppfriend = NamedList_GetByNameAndBump(&roster, barejid);

	if (!ppfriend)
	{
		View_PopupProfileDialog(barejid, NULL);
		free(barejid);
		return;
	}

	frienddata = (*ppfriend)->data;

	View_PopupProfileDialog(barejid, frienddata->nickname);
	free(barejid);
	return;
}

void Model_SetLocalProfileDescription(char *description)
{
	char *bareloginjid = Jid_Strip(login_jid);
	struct namedlist_s **ppentry = NamedList_GetByName(&profilecache, bareloginjid);
	if (ppentry)
	{
		struct profile_s *profile = (*ppentry)->data;

		free(profile->description);
		profile->description = strdup(description);

		Model_UpdateSubscribers(bareloginjid);
	}

	free(bareloginjid);
}

void Model_SaveProfile(char *jid, char *nick)
{
	char *barejid = Jid_Strip(jid);
	char *bareloginjid;
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct frienddata_s *frienddata;

	bareloginjid = Jid_Strip(login_jid);

	{
		struct namedlist_s **ppentry = NamedList_GetByName(&profilecache, barejid);
		if (ppentry)
		{
			struct profile_s *profile = (*ppentry)->data;

			free(profile->nickname);
			profile->nickname = Util_StripLeadingTrailingSpaces(nick);
		}
	}

	if (stricmp(barejid, bareloginjid) == 0)
	{
		Conn_SetProfileField("nick", nick);
		View_SetUser(bareloginjid, Model_GetDefaultNick());
		Model_UpdateSubscribers(barejid);
		free(barejid);
		free(bareloginjid);
		return;
	}

	free(bareloginjid);

	if (!ppfriend)
	{
		free(barejid);
		return;
	}

	frienddata = (*ppfriend)->data;

	free(frienddata->nickname);
	frienddata->nickname = strdup(nick);

	Model_SyncFriend(barejid);
	Model_ViewUpdateFriend(barejid);

	Model_UpdateSubscribers(barejid);

	free(barejid);
	return;
}

void Model_PopupChatDialog(char *jid, int isGroupChat)
{
	if (!isGroupChat)
	{
		View_PopupChatDialog(jid, Model_GetFriendNick(jid), 0);
		View_ActivateChatDialog(jid, Model_GetFriendNick(jid), 0);
	}
	else
	{
		View_PopupChatDialog(jid, Ctrl_GetDefaultNick(), 1);
		View_ActivateChatDialog(jid, Ctrl_GetDefaultNick(), 1);
	}
}

void Model_SaveServerPrefs()
{
	struct StatusList_s *entry;
	struct namedlist_s *availablestatuses = NULL;
	struct namedlist_s *awaystatuses = NULL;
	char *bareloginjid = Jid_Strip(login_jid);

	/* Don't save non chesspark server prefs */
	if (!strstr(bareloginjid, "chesspark.com"))
	{
                return;
	}

	/* always save low bandwidth setting to the registry */
	{
		struct namedlist_s *entry = NamedList_GetByName(&localoptions, OPTION_LOWBANDWIDTH);

		SetUserRegInt(bareloginjid, "LowBandwidth", entry != NULL);
	}

	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	entry = availstatuslist;
	while (entry)
	{
		NamedList_AddString(&availablestatuses, entry->statusmsg, NULL);
		entry = entry->next;
	}

	entry = awaystatuslist;
	while (entry)
	{
		NamedList_AddString(&awaystatuses, entry->statusmsg, NULL);
		entry = entry->next;
	}

	Conn_SetServerPrefs(favechats, autojoinchats, availablestatuses, awaystatuses, customtimecontrols, localoptions);

	NamedList_Destroy(&availablestatuses);
	NamedList_Destroy(&awaystatuses);
}


void Model_SetOption(char *option, int state, char *data)
{
	struct namedlist_s **entry = NamedList_GetByName(&localoptions, option);

	if (state == 2)
	{
		if (entry)
		{
			state = 0;
		}
		else
		{
			state = 1;
		}
	}

	NamedList_Remove(entry);

	if (state)
	{
		NamedList_AddString(&localoptions, option, data);
	}

	if (state && stricmp(option, OPTION_AUTOAWAY) == 0)
	{
		sscanf(data, "%d", &localautoawaytime);
	}
	
	Model_SaveServerPrefs();
	View_SetOptions();
}


void Model_SetAllOptions(struct namedlist_s *newoptions)
{
	NamedList_Destroy(&localoptions);

	{
		localoptions = NULL;

		while (newoptions)
		{
			if (stricmp(newoptions->name, OPTION_SEARCHFILTERS) == 0)
			{
				struct namedlist_s *searchfilters = NULL;
				struct namedlist_s *entry = newoptions->data;
				while (entry)
				{
					NamedList_Add(&searchfilters, entry->name, Info_DupeGameSearchInfo(entry->data), Info_DestroyGameSearchInfo);
					entry = entry->next;
				}
				NamedList_Add(&(localoptions), newoptions->name, searchfilters, NamedList_Destroy2);
			}
			else if (stricmp(newoptions->name, OPTION_LOGINROOMS) == 0)
			{
				struct namedlist_s *loginrooms = NULL;
				struct namedlist_s *entry = newoptions->data;

				while (entry)
				{
					NamedList_AddString(&loginrooms, entry->name, entry->data);
					entry = entry->next;
				}

				NamedList_Add(&(localoptions), newoptions->name, loginrooms, NamedList_Destroy2);
			}
			else
			{
				NamedList_AddString(&(localoptions), newoptions->name, newoptions->data);
			}
			newoptions = newoptions->next;
		}
	}

	{
		struct namedlist_s **entry = NamedList_GetByName(&localoptions, OPTION_AUTOAWAY);

		if (entry)
		{
			sscanf((*entry)->data, "%d", &localautoawaytime);
		}

		entry = NamedList_GetByName(&localoptions, OPTION_NOTIFICATIONTIME);

		if (entry)
		{
			int length;
			sscanf((*entry)->data, "%d", &length);
			CornerPop_SetLength(length);
		}

		entry = NamedList_GetByName(&localoptions, OPTION_NOTIFICATIONLOCATION);

		if (entry)
		{
			CornerPop_SetLocation((*entry)->data);
		}
		else
		{
			CornerPop_SetLocation(NULL);
		}

		entry = NamedList_GetByName(&localoptions, OPTION_PIECESTHEME);

		if (entry)
		{
			View_SetChessPieceTheme((*entry)->data);
		}
		else
		{
			View_SetChessPieceTheme("alpha");
		}
	}

	Model_SaveServerPrefs();

	View_SetOptions();
}

int Model_GetOption(char *option)
{
	struct namedlist_s **entry = NamedList_GetByName(&localoptions, option);

	if (entry)
	{
		return 1;
	}

	return 0;
}

char *Model_GetOptionString(char *option)
{
	struct namedlist_s **entry = NamedList_GetByName(&localoptions, option);

	if (entry)
	{
		return (*entry)->data;
	}

	return NULL;
}

struct namedlist_s *Model_GetOptions()
{
	return localoptions;
}

#if 0
void Model_SetUIFlag(enum uiflags_e flagchange, int state)
{
	char *bareloginjid = Jid_Strip(login_jid);
	enum uiflags_e uiflags = GetUserRegInt(bareloginjid, "uiflags");

	if (state == 2) /* toggle */
	{
		state = (uiflags & flagchange) == 0;
	}

	if (state)
	{
		uiflags |= flagchange;
	}
	else
	{
		uiflags &= ~flagchange;
	}

	SetUserRegInt(bareloginjid, "uiflags", uiflags);
	localuiflags = uiflags;

	View_SetUIFlags(uiflags);

	free(bareloginjid);
}

void Model_SetAllUIFlags(enum uiflags_e newflags)
{
	char *bareloginjid = Jid_Strip(login_jid);
	localuiflags = newflags;

	SetUserRegInt(bareloginjid, "uiflags", newflags);
	View_SetUIFlags(newflags);
	free(bareloginjid);

}

enum uiflags_e Model_GetUIFlags()
{
	char *bareloginjid = Jid_Strip(login_jid);
	enum uiflags_e uiflags = GetUserRegInt(bareloginjid, "uiflags");
	localuiflags = uiflags;
	free(bareloginjid);
	return uiflags;
}

int Model_GetUIFlag(enum uiflags_e flag)
{
	char *bareloginjid = Jid_Strip(login_jid);
	enum uiflags_e uiflags = GetUserRegInt(bareloginjid, "uiflags");
	localuiflags = uiflags;
	free(bareloginjid);
	return (uiflags & flag) != 0;
}

void Model_SetAutoAwayTime(int mins)
{
	char *bareloginjid = Jid_Strip(login_jid);
	localautoawaytime = mins;
	SetUserRegInt(bareloginjid, "autoawaytime", mins);
	free(bareloginjid);
}


int Model_GetAutoAwayTime()
{
	return localautoawaytime;
}
#endif

void ModelConn_OpenAllGroup()
{
	View_FriendsOpenAllGroup();
}

void ModelConn_HandleTotalGamesCount(int count)
{
	View_SetTotalGamesCount(count);
}

void ModelConn_RefreshGameSearch(char *node)
{
	View_RefreshGamesListPage(node);
}

void ModelConn_AddAdSuccess(char *id)
{
	View_AddAdSuccess(id);
}

void Model_LeaveChat(char *chatjid)
{
	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	if (login_connected)
	{
		Conn_SendPresence(SSTAT_OFFLINE, NULL, chatjid, 1, NULL, 0, 0);
	}
}
/*
void ModelConn_RequestMatchError(char *opponentjid, char *error)
{
	View_RequestMatchError(opponentjid, error);
}
*/
void ModelConn_RequestMatchSetID(char *opponentjid, char *gameid)
{
	View_RequestMatchSetID(opponentjid, gameid);
}

void ModelConn_RematchSetID(char *oldgameid, char *newgameid)
{
	View_RematchSetID(oldgameid, newgameid);
}

/*
void ModelConn_ReconveneError(char *gameid, char *error)
{
	View_ReconveneError(gameid, error);
}
*/
void ModelConn_GameRequestError(char *opponentjid, char *gameid, char *error, char *actor, char *adid)
{
	View_GameRequestError(opponentjid, gameid, error, actor, adid);
}

void ModelConn_GameRematchError(char *oldgameid, char *error, char *actor)
{
	View_GameRematchError(oldgameid, error, actor);
}

void ModelConn_NotAProError(char *error)
{
	View_PopupNotAProError(error);
}

void ModelConn_SetComposing(char *jid, char *msgid)
{
	View_SetComposing(jid, msgid);
}

void ModelConn_UnsetComposing(char *jid, char *msgid)
{
	View_UnsetComposing(jid, msgid);
}

void Model_SetComposing(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **msgidroot = NamedList_GetByName(&messageids, barejid);
	struct namedlist_s **active = NamedList_GetByName(&chatactivejids, barejid);
	free(barejid);
		
	if (active)
	{
		jid = (*active)->data;
	}
	
	if (msgidroot && (Conn_GetConnState() == CONN_CONNECTED))
	{
		char *msgid = (*msgidroot)->data;

		Conn_SendComposing(jid, msgid);
	}
}

void Model_UnsetComposing(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **msgidroot = NamedList_GetByName(&messageids, barejid);
	struct namedlist_s **active = NamedList_GetByName(&chatactivejids, barejid);
	free(barejid);
		
	if (active)
	{
		jid = (*active)->data;
	}
	
	if (msgidroot && (Conn_GetConnState() == CONN_CONNECTED))
	{
		char *msgid = (*msgidroot)->data;

		Conn_CancelComposing(jid, msgid);
	}
}

void ModelConn_HandlevCardPhoto(char *jid, char *type, char *binvaldata, int len)
{
	char filename[MAX_PATH], *hash;
	char tempfile[MAX_PATH];
	char savepath[MAX_PATH];
	FILE *fp;
	
	Util_GetPrivateSavePath(savepath, MAX_PATH);

	strcpy(tempfile, savepath);
	strcat(tempfile, "/tempfile");

	fp = fopen(tempfile, "wb");
	fwrite(binvaldata, 1, len, fp);
	fclose(fp);

	hash = SHA1_GetHexDigestForFile(tempfile);
	strlwr(hash);

	strcpy(filename, savepath);
	strcat(filename, "/avatars");
	mkdir(filename);
	strcat(filename, "/");
	strcat(filename, hash);
	if (type && strcmp(type, "image/png") == 0)
	{
		strcat(filename, ".png");
	}
	else if (type && strcmp(type, "image/jpeg") == 0)
	{
		strcat(filename, ".jpg");
	}
	else if (type && strcmp(type, "image/gif") == 0)
	{
		strcat(filename, ".gif");
	}

	MoveFile(tempfile, filename);

	Model_SetFriendAvatarHash(jid, hash, 1, 0);
}

void Model_SetAvatar(char *filename)
{
	int len;
	unsigned char *data, *hash;
	char *bareloginjid;
	char filename2[MAX_PATH];
	FILE *fp;
	char *extension;

	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	fp = fopen(filename, "rb");
	if (!fp)
	{
		return;
	}
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	data = malloc(len);
	fread(data, 1, len, fp);
	fclose(fp);
	extension = strrchr(filename, '.');
	if (extension)
	{
		extension++;
	}
	else
	{
		extension = "png";
	}

	if (stricmp(extension, "png") == 0)
	{
                Conn_PublishvCard(data, len, "image/png");
	}
	else if (stricmp(extension, "png") == 0)
	{
                Conn_PublishvCard(data, len, "image/gif");
	}
	else if (stricmp(extension, "jpg") == 0 || stricmp(extension, "jpeg") == 0)
	{
                Conn_PublishvCard(data, len, "image/jpeg");
	}

	
	hash = SHA1_GetHexDigestForFile(filename);
	strlwr(hash);

	Util_GetPrivateSavePath(filename2, MAX_PATH);

	strcat(filename2, "/avatars");
	mkdir(filename2);
	strcat(filename2, "/");
	strcat(filename2, hash);
	strcat(filename2, ".");
	strcat(filename2, extension);

	CopyFile(filename, filename2, TRUE);

	bareloginjid = Jid_Strip(login_jid);
	free(bareloginjid);

	View_SetAvatar(filename2);

	Conn_SendPresence(userstatus, userstatusmsg, NULL, 0, hash, currentpriority, 1);

	free(avatarhash);
	avatarhash = strdup(hash);
	strlwr(avatarhash);

	Model_HandlePresence(login_jid, 0, userstatus, userstatusmsg, NULL, NULL, login_jid, 0, avatarhash, NULL, NULL, NULL, currentpriority, NULL, NULL, 0, NULL);

	free(data);
}

void Model_SetAvatarNoExtension(char *filename)
{
	char filename2[MAX_PATH];
	FILE *fp;

	strcpy(filename2, filename);
	strcat(filename2, ".png");

	fp = fopen(filename2, "rb");
	if (fp)
	{
		fclose(fp);
		Model_SetAvatar(filename2);
		return;
	}

	strcpy(filename2, filename);
	strcat(filename2, ".jpg");

	fp = fopen(filename2, "rb");
	if (fp)
	{
		fclose(fp);
		Model_SetAvatar(filename2);
		return;
	}

	strcpy(filename2, filename);
	strcat(filename2, ".gif");

	fp = fopen(filename2, "rb");
	if (fp)
	{
		fclose(fp);
		Model_SetAvatar(filename2);
		return;
	}
}

void Model_SetAvatarToHash(char *hash)
{
	char *filename;
	char buffer[MAX_PATH];

	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	filename = Ctrl_GetAvatarFilenameByHash(hash, buffer, MAX_PATH);

	if (!filename)
	{
		return;
	}

	View_SetAvatar(filename);

	if (!avatarhash || stricmp(avatarhash, hash) != 0)
	{
		Conn_SendPresence(userstatus, userstatusmsg, NULL, 0, hash, currentpriority, 1);
	}

	free(avatarhash);
	avatarhash = strdup(hash);
	strlwr(avatarhash);
}

void Model_RequestRematch(char *opponentjid)
{
	char *barejid = Jid_Strip(opponentjid);
	struct namedlist_s **listentry;
	struct gamesearchinfo_s *info;

	listentry = NamedList_GetByName(&gameinfos, barejid);

	if (!listentry || !*listentry)
	{
		return;
	}

	info = (*listentry)->data;

	Conn_RequestMatch(barejid, info);

	free(barejid);
}

void ModelConn_ShowGameMessage(char *jid, char *gameid, struct gamesearchinfo_s *info)
{
	char *barewhitejid = Jid_Strip(info->white->jid);
	char *bareblackjid = Jid_Strip(info->black->jid);
	char *barelocaljid = Jid_Strip(login_jid);

	int whitelocal = 0, blacklocal = 0;

	if (stricmp(barewhitejid, barelocaljid) == 0)
	{
		whitelocal = 1;
	}
	else if (stricmp(bareblackjid, barelocaljid) == 0)
	{
		blacklocal = 1;
	}

	free(barewhitejid);
	free(bareblackjid);
	free(barelocaljid);

	View_ShowGameMessage(jid, jid, gameid, info, whitelocal, blacklocal);
}

void ModelConn_SetGameStatus(char *jid, char *gameid, struct gamesearchinfo_s *info, int stopped, int watching, int update, int clear)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);
	struct namedlist_s *group;
	enum sstatus status;
	char *statusmsg;
	int chessparkfeatures;
	struct namedlist_s *roles = NULL, *titles = NULL;
	char *rating = NULL;
	struct profile_s *profile = Model_GetProfile(jid, 1);

	if (profile)
	{
                rating = Info_GetStandardRating(profile->ratings);
		roles = profile->roles;
		titles = profile->titles;
	}

	if (!profile)
	{
		profile = malloc(sizeof(*profile));
		memset(profile, 0, sizeof(*profile));
		NamedList_Add(&profilecache, barejid, profile, Info_DestroyProfile);
	}

	if (clear)
	{
		NamedList_Destroy(&(profile->watchinggames));
		NamedList_Destroy(&(profile->playinggames));
	}
	else if (watching)
	{
		NamedList_RemoveByName(&(profile->watchinggames), gameid);
		if (!stopped)
		{
			NamedList_Add(&(profile->watchinggames), gameid, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
		}
	}
	else
	{
		NamedList_RemoveByName(&(profile->playinggames), gameid);
		if (!stopped)
		{
			NamedList_Add(&(profile->playinggames), gameid, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
		}
	}

	if (ppfriend)
	{
		struct frienddata_s *data;
		int localinvolved = 0;

		data = (*ppfriend)->data;
		Friend_GetBestStatus(data, &status, &statusmsg, &chessparkfeatures);

		View_SetFriendStatus(barejid, data->nickname, NULL, status, statusmsg, data->avatarhash, chessparkfeatures ? rating : NULL, roles, titles, gameid, info, stopped, watching, data->command);

		group = data->groups;
		while(group)
		{
			View_SetFriendStatus(barejid, data->nickname, group->name, status, statusmsg, data->avatarhash, chessparkfeatures ? rating : NULL, roles, titles, gameid, info, stopped, watching, data->command);
			group = group->next;
		}

		if (info)
		{
			char *bareloginjid = Jid_Strip(login_jid);
			char *barewhitejid = NULL;
			char *bareblackjid = NULL;

			if (info && info->white && info->white->jid)
			{
				barewhitejid = Jid_Strip(info->white->jid);
			}

			if (info && info->black && info->black->jid)
			{
				bareblackjid = Jid_Strip(info->black->jid);
			}

			if ((bareloginjid && barewhitejid && stricmp(bareloginjid, barewhitejid) == 0) || (bareloginjid && bareblackjid && stricmp(bareloginjid, bareblackjid) == 0))
			{
				localinvolved = 1;
			}

			free(bareloginjid);
			free(barewhitejid);
			free(bareblackjid);
		}

		if (!stopped && !watching && update && !localinvolved)
		{
			View_ShowFriendPlayingPopup(jid, gameid, info);
		}
	}

	Model_UpdateSubscribers(barejid);

	free(barejid);
}

void Model_PopupReconvene(char *gameid, struct gamesearchinfo_s *info)
{
	char *barewhitejid = Jid_Strip(info->white->jid);
	char *bareblackjid = Jid_Strip(info->black->jid);
	char *barelocaljid = Jid_Strip(login_jid);
	int whitelocal = 0, blacklocal = 0;

	if (stricmp(barewhitejid, barelocaljid) == 0)
	{
		whitelocal = 1;
	}
	else if (stricmp(bareblackjid, barelocaljid) == 0)
	{
		blacklocal = 1;
	}

	if (whitelocal)
	{
		View_PopupReconveneDialog(info->black->jid, gameid, info, 1);
	}
	else if (blacklocal)
	{
		View_PopupReconveneDialog(info->white->jid, gameid, info, 1);
	}
}

char *Model_GetUserRegString(char *key, char *in, int size)
{
	char *barejid = Jid_Strip(login_jid);
	char *txt = GetUserRegString(barejid, key, in, size);

	free(barejid);

	return txt;
}

void Model_SetUserRegString(char *key, char *value)
{
	char *barejid = Jid_Strip(login_jid);
	SetUserRegString(barejid, key, value);

	free(barejid);
}


int Model_GetUserRegInt(char *key)
{
	char *barejid = Jid_Strip(login_jid);
	int result = GetUserRegInt(barejid, key);

	free(barejid);

	return result;
}

void Model_SetUserRegInt(char *key, int value)
{
	char *barejid = Jid_Strip(login_jid);
	SetUserRegInt(barejid, key, value);

	free(barejid);
}


int Model_HasFriend(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend = NamedList_GetByNameAndBump(&roster, barejid);

	free(barejid);

	if (ppfriend && *ppfriend)
	{
		Log_Write(0, "jid %s on roster\n", jid);
		return 1;
	}

	Log_Write(0, "jid %s not on roster\n", jid);
	return 0;
}

void ModelConn_SetTournamentInfo(struct tournamentinfo_s *info)
{
	View_SetTournamentInfo(info);
}

void Model_JoinTournament(char *tourneyid)
{
	char *barejid = Jid_Strip(login_jid);

	Conn_JoinTournament(tourneyid, barejid);

	free(barejid);
}

void Model_ForfeitTournament(char *tourneyid)
{
	char *barejid = Jid_Strip(login_jid);

	Conn_ForfeitTournament(tourneyid, barejid);

	free(barejid);
}

void ModelConn_TournamentAddPlayer(char *tourneyid, char *playerjid)
{
	View_TournamentAddPlayer(tourneyid, playerjid);
}

void ModelConn_TournamentRemovePlayer(char *tourneyid, char *playerjid)
{
	View_TournamentRemovePlayer(tourneyid, playerjid);
}

void ModelConn_TournamentStartRound(char *tourneyid, int round, struct namedlist_s *pairinglist)
{
	View_TournamentStartRound(tourneyid, round, pairinglist);
}

void ModelConn_TournamentEndRound(char *tourneyid, int round)
{
	View_TournamentEndRound(tourneyid, round);
}

void ModelConn_TournamentEnd(char *tourneyid, char *winner)
{
	View_TournamentEnd(tourneyid, winner);
}

void ModelConn_TournamentGamePlaying(char *tourneyid, char *gameid, int round, char *white, char *black)
{
	char *barewhite = Jid_Strip(white);
	char *bareblack = Jid_Strip(black);

	View_TournamentGamePlaying(tourneyid, gameid, round, barewhite, bareblack);

	free(barewhite);
	free(bareblack);
}

void ModelConn_TournamentGameStoppedPlaying(char *tourneyid, char *gameid, int round, char *white, char *black, char *winner)
{
	char *barewhite = Jid_Strip(white);
	char *bareblack = Jid_Strip(black);

	View_TournamentGameStoppedPlaying(tourneyid, gameid, round, barewhite, bareblack, winner);

	free(barewhite);
	free(bareblack);
}

void ModelConn_TournamentUpdatePlayer(char *tourneyid, char *jid, struct tournamentplayerinfo_s *pinfo)
{
	View_TournamentUpdatePlayer(tourneyid, jid, pinfo);
}

void ModelConn_ShowGameLogoutMessage(char *jid)
{
	View_ShowGameLogoutMessage(jid, jid);
	Model_SetPriority(0);
}

void ModelConn_ShowMemberExpired()
{
	Model_RemoveTimeout(Model_ResendChessparkLogin, CHESSPARKLOGIN_TIMEOUT);

	Model_ChangeUser();

	View_ShowMemberExpired();
}

void ModelConn_ShowMemberNotFound()
{
	Model_RemoveTimeout(Model_ResendChessparkLogin, CHESSPARKLOGIN_TIMEOUT);

	View_ShowMemberNotFound();
}

void ModelConn_ShowMemberLoginsExceeded()
{
	Model_RemoveTimeout(Model_ResendChessparkLogin, CHESSPARKLOGIN_TIMEOUT);

	View_ShowMemberLoginsExceeded();
}

void ModelConn_ShowMemberCancelled()
{
	Model_RemoveTimeout(Model_ResendChessparkLogin, CHESSPARKLOGIN_TIMEOUT);

	Model_ChangeUser();

	View_ShowMemberCancelled();
}

		
void ModelConn_ShowChatInviteMessage(char *friendjid, char *chatjid)
{
	if (Model_IsJIDIgnored(friendjid))
	{
		return;
	}

	if (strstr(chatjid, "@games.chesspark.com"))
	{
		View_ShowGameWatchInviteMessage(friendjid, chatjid);
	}
	else
	{
		View_ShowChatInviteMessage(friendjid, chatjid);
	}
}

static int upgradeinprogress = 0;

struct buildinfo_s
{
	char *number;
	char *version;
	char *build;
	char *url;
	char *path;
};

void Model_HandleClientFinishedDownload(struct buildinfo_s *bi)
{
	int ibuild;

	if (bi->path)
	{
		char src[MAX_PATH], dst[MAX_PATH];

		strcpy(src, bi->path);
		strcat(src, "/setup2.exe");
		strcpy(dst, bi->path);
		strcat(dst, "/setup.exe");

		MoveFile(src, dst);
	}

	sscanf (bi->build, "%d", &ibuild);

	SetRegInt("LastDownloadedBuild", ibuild);
	SetRegInt("UpgradeOnStartup", 1);

	View_ShowClientUpgradeMessage(NULL, bi->number, bi->version, bi->build, bi->url);
}

void Model_HandleClientUpgrade(char *number, char *version, char *build, char *url)
{
	int ibuild;
	struct buildinfo_s *bi;
	char appdata[MAX_PATH];
	char filename[MAX_PATH];

	if (!build || !url)
	{
		return;
	}

	/* don't start another download if we're already grabbing one */
	if (NetTransfer_AlreadyDownloading(url) || upgradeinprogress)
	{
		return;
	}

	upgradeinprogress = 1;

	sscanf (build, "%d", &ibuild);

	bi = malloc(sizeof(*bi));
	memset(bi, 0, sizeof(*bi));
	bi->number = strdup(number);
	bi->version = strdup(version);
	bi->build = strdup(build);
	bi->url = strdup(url);

	Util_GetPrivateSavePath(appdata, MAX_PATH);

	if (ibuild <= GetRegInt("LastDownloadedBuild"))
	{
		/* check if the installer is there */
		FILE *fp;

		strcpy(filename, appdata);
		strcat(filename, "/installers/setup.exe");
		
		fp = fopen(filename, "rb");
		if (fp)
		{
			fclose(fp);
                        Model_HandleClientFinishedDownload(bi);
			return;
		}
	}

	/* clear out any existing installers */
	strcpy(filename, appdata);
	strcat(filename, "/installers/setup3.exe");
	DeleteFile(filename);
	strcpy(filename, appdata);
	strcat(filename, "/installers/setup2.exe");
	DeleteFile(filename);
	strcpy(filename, appdata);
	strcat(filename, "/installers/setup.exe");
	DeleteFile(filename);

	strcpy(filename, appdata);
	strcat(filename, "/installers");
	mkdir(filename);
	bi->path = strdup(filename);
	strcat(filename, "/setup2.exe");

	Log_Write(0, "filename is %s url is %s\n", filename, url);

	NetTransfer_AddDownload(url, filename, Model_HandleClientFinishedDownload, bi);
}

void Model_LaunchUpdater()
{
	char filename[MAX_PATH];
	int build = GetRegInt("LastDownloadedBuild");
	int runningbuild;

	SetRegInt("UpgradeOnStartup", 0);
	SetRegInt("UpgradeInProgress", build);

	sscanf (CHESSPARK_BUILD, "%d", &runningbuild);

	if (runningbuild >= build)
	{
		return;
	}

	Util_GetPrivateSavePath(filename, MAX_PATH);
	strcat(filename, "/upgrader.exe");

	CopyFile("./upgrader.exe", filename, 0);
	ShellExecute(NULL,  NULL, filename, NULL, ".", SW_SHOW);
	PostQuitMessage(0);
}

void ModelConn_OnChessparkLogin(char *membertype, struct namedlist_s *permissions, struct namedlist_s *rooms, int notactivated)
{
	char *bareloginjid = Jid_Strip(login_jid);
	struct namedlist_s *entry;
	struct namedlist_s **ppentry;

	Model_RemoveTimeout(Model_ResendChessparkLogin, CHESSPARKLOGIN_TIMEOUT);
	AdhocDialog_Close(NULL, NULL);

	free(local_membertype);
	local_membertype = strdup(membertype);
	Log_Write(0, "local_membertype %s\n", local_membertype);

	NamedList_Destroy(&local_permissions);
	local_permissions = permissions;
	local_notactivated = notactivated;
/*
	if (!GetUserRegInt(bareloginjid, "SharedRosterLoaded"))
	{
		Conn_RequestGameSearch("shared_roster", NULL);
	}
*/

	if (rooms)
	{
		struct namedlist_s **ppentry = NamedList_GetByName(&localoptions, OPTION_LOGINROOMS);
		struct namedlist_s **loginrooms;
		int changed = 0;

		if (!ppentry)
		{
			ppentry = NamedList_Add(&localoptions, OPTION_LOGINROOMS, NULL, NamedList_Destroy2);
			changed = 1;
		}

		loginrooms = (&(*ppentry)->data);
		entry = rooms;

		while (entry)
		{
			char roomjid[512];
			
			strcpy(roomjid, entry->name);

			if (!strchr(roomjid, '@'))
			{
				strcat(roomjid, "@chat.chesspark.com");
			}

			if (!NamedList_GetByName(loginrooms, roomjid))
			{
				if (!NamedList_GetByName(&favechats, roomjid))
				{
					NamedList_Add(&favechats, roomjid, NULL, NULL);
					View_AddChatroom(_("Favorites"), roomjid, NULL, "", 0);
				}

				if (!NamedList_GetByName(&autojoinchats, roomjid))
				{
                        		NamedList_Add(&autojoinchats, roomjid, NULL, NULL);
				}

				changed = 1;
			}

			entry = entry->next;
		}

		entry = *loginrooms;
		while (entry)
		{
			char roomjid[512];
			
			strcpy(roomjid, entry->name);

			if (!strchr(roomjid, '@'))
			{
				strcat(roomjid, "@chat.chesspark.com");
			}

			if (!NamedList_GetByName(&rooms, roomjid))
			{
				changed = 1;
			}

			entry = entry->next;
		}

		NamedList_Destroy(loginrooms);
		entry = rooms;

		while (entry)
		{
			NamedList_AddString(loginrooms, entry->name, entry->name);

			entry = entry->next;
		}

		if (changed)
		{
                        Model_SaveServerPrefs();
		}
	}
		
	if (!login_joinedchats && !chessparklogin)
	{
		entry = autojoinchats;

		while (entry)
		{
			if (strstr(entry->name, "@chat.chesspark.com"))
			{
				Ctrl_JoinGroupChatDefaultNick(entry->name);
			}
			entry = entry->next;
		}
	}

	free(bareloginjid);

	if (!chessparklogin)
	{
		View_ShowLoginMessage();
		View_RefreshNewGamesDialog();
		if (!Model_GetOption(OPTION_NOGAMESEARCHONLOGIN))
		{
			View_PopupNewGamesDialog();
		}
	}

	{
		int build;
		sscanf (CHESSPARK_BUILD, "%s", &build);
		if (GetRegInt("UpgradeInProgress") == build)
		{
			SetRegInt("UpgradeInProgress", 0);
			View_ShowUpgradedMessage();
		}
	}

	if (!chessparklogin)
	{
		Conn_RequestGameSearch("adjourned", NULL);
	}

	Conn_GetChessparkPrivacyList();

	chessparklogin = 1;

	login_joinedchats = 1;

	/* should be immediately reset to 1 once the groups stanza is parsed, which is immediately after */
	inhiddengroup = 0;

	View_ReconnectChatDialogs(1);
}

void ModelConn_SetAdjournedCount(int adjourned)
{
	View_SetAdjournedCount(adjourned);
}

void ModelConn_SetCorrespondenceCount(int correspondence)
{
	View_SetCorrespondenceCount(correspondence);
}

void ModelConn_ShowAnnouncement(char *from, char *text, char *subject)
{
	View_ShowAnnouncement(from, text, subject);
}

void Model_AdHocCommand(char *jid, char *node)
{
	char *resource = NULL;
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppfriend;

	{
		char titletext[256];
		char dialogtext[256];

		i18n_stringsub(titletext, 256, _("%1 with %2"), node, Model_GetFriendNick(jid));
		i18n_stringsub(dialogtext, 256, _("Sending %1 to %2"), node, Model_GetFriendNick(jid));

		View_PopupGameWaitingDialog(titletext, dialogtext);
	}
	if (!strcmp(barejid, jid) == 0)
	{
		Conn_AdHocCommand(jid, node);
		free(barejid);
		return;
	}

	ppfriend = NamedList_GetByNameAndBump(&roster, barejid);

	if (ppfriend)
	{
		struct frienddata_s *data = (*ppfriend)->data;
		struct namedlist_s *entry;

		entry = data->resources;

		while (entry)
		{
			struct resourcedata_s *rdata = entry->data;

			if (rdata->chessparkfeatures)
			{
				char *newjid = malloc(strlen(barejid) + 1 + strlen(entry->name) + 1);

				strcpy(newjid, barejid);
				strcat(newjid, "/");
				strcat(newjid, entry->name);

				Conn_AdHocCommand(newjid, node);
				
				free(newjid);
				free(barejid);
				return;
			}

			entry = entry->next;
		}
	}

	free(barejid);
}

int Model_CustomTimeControlExists(char *name)
{
	return NamedList_GetByName(&customtimecontrols, name) != NULL;
}

void Model_RemoveSavedCustomTimeControl(char *name)
{
	NamedList_RemoveByName(&customtimecontrols, name);

	Model_SaveServerPrefs();
}

void Model_SaveCustomTimeControl(char *name, struct timecontrol_s *whitetc, struct timecontrol_s *blacktc)
{
	struct tcpair_s *tcp;

	tcp = malloc(sizeof(*tcp));
	memset(tcp, 0, sizeof(*tcp));

	tcp->white = Info_DupeTimeControl(whitetc);
	tcp->black = Info_DupeTimeControl(blacktc);

	NamedList_RemoveByName(&customtimecontrols, name);
	NamedList_Add(&customtimecontrols, name, tcp, Info_DestroyTCPair);

	Model_SaveServerPrefs();
}

struct tcpair_s *Model_GetCustomTimeControl(char *name)
{
	struct namedlist_s **entry = NamedList_GetByName(&customtimecontrols, name);

	if (!entry)
	{
		return NULL;
	}

	return (*entry)->data;
}

struct namedlist_s *Model_GetCustomTimeControls()
{
	return customtimecontrols;
}

void Model_JoinGroupChat(char *jid, char *nickname)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		char chatname[256];

		chatname[0] = 0;
		strcat(chatname, jid);
		strcat(chatname, "/");
		strcat(chatname, nickname);

		Conn_SendPresence(userstatus, userstatusmsg, chatname, 1, NULL, 0, 0);
	}
}

void ModelConn_HandleBanlist(char *jid, struct namedlist_s *banlist)
{
	View_PopupBanlist(jid, banlist);
}

void ModelConn_HandleBanlistError(char *chatjid, char *code)
{
	int icode;
	char error[512];

	sscanf(code, "%d", &icode);

	switch (icode)
	{
		case 403:
			i18n_stringsub(error, 512, "Error: You don't have permission to view the banlist for %1.", chatjid);
			break;
		default:
			i18n_stringsub(error, 512, "Unknown error when retrieving banlist for %1: %2", chatjid, code);
			break;
	}

	View_GenericError(chatjid, error);
}

void ModelConn_GenericError(char *errortitle, char *error)
{
	View_GenericError(errortitle, error);
}


void Model_Ping(char *jid, int show, int repeat)
{
	Conn_Ping(jid, show, repeat);
}

void ModelConn_HandlePing(char *jid, int pingtime, int repeat)
{
	char txt[1024];

	if (!jid)
	{
                sprintf(txt, "Your ping to the server is %dms.", pingtime);
	}
	else
	{
		sprintf(txt, "Your ping to %s is %dms.", jid, pingtime);
	}

	View_GenericError("Ping!", txt);

	if (repeat > 0)
	{
		Ctrl_Ping(jid, 1, repeat - 1);
	}
}

void ModelConn_HandlePingError(char *jid)
{
	char txt[1024];

	sprintf(txt, "Error pinging %s", jid);

	View_GenericError("No ping!", txt);
}

void ModelConn_HandlePrivacyListNames(char *activename, char *defaultname, struct namedlist_s *listnames)
{
}

void Model_MatchPrivacyLists()
{
	struct namedlist_s *listentry;
	int match = 1;
	Log_Write(0, "comparing privacy lists.\n");

	listentry = localprivacylist;
	while (listentry)
	{
		if (!NamedList_GetByName(&matchprivacylist, listentry->name))
		{
			match = 0;
		}
		listentry = listentry->next;
	}

	listentry = matchprivacylist;
	while (listentry)
	{
		if (!NamedList_GetByName(&localprivacylist, listentry->name))
		{
			match = 0;
		}
		listentry = listentry->next;
	}

	if (!match)
	{
		Conn_SetChessparkPrivacyList(localprivacylist);
	}
}

void ModelConn_HandlePrivacyList(char *listname, struct namedlist_s *privacylist)
{
	struct namedlist_s *listentry;

	if (!listname || stricmp(listname, "chesspark") != 0)
	{
		return;
	}

	NamedList_Destroy(&localprivacylist);

	listentry = privacylist;
	while (listentry)
	{
		struct privacylistentry_s *entry = listentry->data;

		NamedList_Add(&localprivacylist, listentry->name, Info_DupePrivacyListEntry(entry), Info_DestroyPrivacyListEntry);
		listentry = listentry->next;
	}

	Log_Write(0, "local privacy list loaded.\n");
	localprivacylistloaded = 1;
	if (matchprivacylistloaded)
	{
		Model_MatchPrivacyLists();
	}

}

void ModelConn_HandleChessparkPrivacyList(struct namedlist_s *privacylist)
{
	struct namedlist_s *listentry;

	NamedList_Destroy(&matchprivacylist);

	listentry = privacylist;
	while (listentry)
	{
		struct privacylistentry_s *entry = listentry->data;

		NamedList_Add(&matchprivacylist, listentry->name, Info_DupePrivacyListEntry(entry), Info_DestroyPrivacyListEntry);
		listentry = listentry->next;
	}

	Log_Write(0, "match privacy list loaded.\n");
	matchprivacylistloaded = 1;
	if (localprivacylistloaded)
	{
		Model_MatchPrivacyLists();
	}
}

int Model_IsJIDIgnored(char *jid)
{
	char *barejid = Jid_Strip(jid);

	if (NamedList_GetByName(&localprivacylist, barejid))
	{
		return 1;
	}

	return 0;
}

void Model_AddJIDToIgnore(char *jid)
{
	char *barejid = Jid_Strip(jid);
	int order;
	struct namedlist_s *listentry;
	struct privacylistentry_s *entry;

	if (NamedList_GetByName(&localprivacylist, barejid))
	{
		return;
	}

	entry = malloc(sizeof(*entry));
	memset(entry, 0, sizeof(*entry));

	entry->action = strdup("deny");
	entry->type   = strdup("jid");
	entry->value  = strdup(barejid);

	NamedList_AddToTop(&localprivacylist, barejid, entry, Info_DestroyPrivacyListEntry);

	listentry = localprivacylist;

	order = 1;

	while (listentry)
	{
		char ordertxt[80];

		entry = listentry->data;
		sprintf(ordertxt, "%d", order);
		free(entry->order);
		entry->order = strdup(ordertxt);
		order++;
		listentry = listentry->next;
	}

	Conn_SetPrivacyList("chesspark", localprivacylist);
	Model_UpdateSubscribers(jid);

	Model_ViewUpdateFriend(barejid);	
}

void Model_RemoveJIDFromIgnore(char *jid)
{
	char *barejid = Jid_Strip(jid);

	if (!NamedList_GetByName(&localprivacylist, barejid))
	{
		return;
	}

	NamedList_RemoveByName(&localprivacylist, barejid);

	Conn_SetPrivacyList("chesspark", localprivacylist);
	Model_UpdateSubscribers(jid);
	Model_ViewUpdateFriend(jid);
}

struct namedlist_s *Model_GetIgnoreList()
{
	return localprivacylist;
}

void Model_SetIgnoreList(struct namedlist_s *ignorelist)
{
	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	NamedList_Destroy(&localprivacylist);
	localprivacylist = NamedList_DupeList(ignorelist, Info_DupePrivacyListEntry, Info_DestroyPrivacyListEntry);

	Conn_SetPrivacyList("chesspark", localprivacylist);
}

void ModelConn_OnSetPrivacyList()
{
	Conn_SetChessparkPrivacyList(localprivacylist);
	Conn_SetActivePrivacyList("chesspark");
}

int Model_DoAutoAway(char *jid, int show)
{
	if (userstatus == SSTAT_AWAY)
	{
		char *barejid = Jid_Strip(jid);
		struct namedlist_s **entry = NamedList_GetByName(&sentautoaway, barejid);
		unsigned int currenttick = GetTickCount();
		char txt[512];

		if (!Model_GetOption(OPTION_MESSAGEONAWAY))
		{
			return 1;
		}

		if (entry)
		{
			unsigned int lastsent = (unsigned int)((*entry)->data);

			if ((currenttick - lastsent) < 5 * 60 * 1000)
			{
				free(barejid);
				return 1;
			}

			NamedList_Remove(entry);
		}

		NamedList_AddToTop(&sentautoaway, barejid, (void *)(currenttick), NULL);

		if (userstatusmsg && userstatusmsg[0])
		{
			i18n_stringsub(txt, 512, _("Auto-response: I am away from my computer: %1"), userstatusmsg);
		}
		else
		{
			i18n_stringsub(txt, 512, _("Auto-response: I am away from my computer"));
		}

		if (show)
		{
			View_PopupChatDialog(jid, Model_GetFriendNick(jid), 0);
			View_AddChatMessage(jid, Model_GetLoginJid(), txt);
		}

		Ctrl_SendMessage(jid, txt);

		free(barejid);
		return 1;
	}

	return 0;
}

int Model_GetChessparkLogin()
{
	return chessparklogin;
}

void ModelConn_AddPushGame(char *type, char *id, struct gamesearchinfo_s *info)
{
	/*
	if (stricmp(type, "play") == 0)
	{
                NamedList_Add(&pushplaygames, id, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
	}
	else if (stricmp(type, "watch") == 0)
	{
                NamedList_Add(&pushwatchgames, id, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
	}
	*/
	View_AddPushGame(type, id, info);
}

void ModelConn_RemovePushGame(char *type, char *id)
{
	/*
	if (stricmp(type, "play") == 0)
	{
                NamedList_RemoveByName(&pushplaygames, id);
	}
	else if (stricmp(type, "watch") == 0)
	{
                NamedList_RemoveByName(&pushwatchgames, id);
	}
	*/
	View_RemovePushGame(type, id);
}
/*
struct namedlist_s *Model_GetPushGames(char *type)
{
	if (stricmp(type, "play") == 0)
	{
                return pushplaygames;
	}
	else if (stricmp(type, "watch") == 0)
	{
                return pushwatchgames;
	}
	return NULL;
}
*/

struct gamesearchinfo_s *Model_GetSearchFilterPref(char *node)
{
	struct namedlist_s **entry = NamedList_GetByName(&localoptions, OPTION_SEARCHFILTERS);
	struct namedlist_s **ppsearchfilters;

	if (!entry)
	{
		return NULL;
	}

	ppsearchfilters = (struct namedlist_s **)(&((*entry)->data));

	entry = NamedList_GetByName(ppsearchfilters, node);

	if (!entry)
	{
		return NULL;
	}

	return (*entry)->data;
}

void Model_SetSearchFilterPref(char *node, struct gamesearchinfo_s *info)
{
	struct namedlist_s **entry = NamedList_GetByName(&localoptions, OPTION_SEARCHFILTERS);
	struct namedlist_s **ppsearchfilters;

	if (!entry)
	{
		NamedList_Add(&localoptions, OPTION_SEARCHFILTERS, NULL, NamedList_Destroy2);
		entry = NamedList_GetByName(&localoptions, OPTION_SEARCHFILTERS);
	}

	ppsearchfilters = (struct namedlist_s **)(&((*entry)->data));

	NamedList_RemoveByName(ppsearchfilters, node);
	NamedList_Add(ppsearchfilters, node, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);

	Model_SaveServerPrefs();
}

void Model_Quit()
{
	if (View_IsPlayingAGame() && !Model_GetOption(OPTION_HIDEWARNINGONDISCONNECT))
	{
		View_PopupQuitWarning();
	}
	else
	{
		View_CloseAllDialogs();
		PostQuitMessage(0);
	}
}

struct gamesearchinfo_s *Model_HaveAdjournedGameWithJid(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s *entry = adjournedgames;

	while (entry)
	{
		struct gamesearchinfo_s *info = entry->data;
		char *whitejid = NULL, *blackjid = NULL;

		if (info && info->white && info->white->jid)
		{
			whitejid = Jid_Strip(info->white->jid);
		}

		if (info && info->black && info->black->jid)
		{
			blackjid = Jid_Strip(info->black->jid);
		}

		if ((whitejid && stricmp(barejid, whitejid) == 0) || (blackjid && stricmp(barejid, blackjid) == 0))
		{
			free(whitejid);
			free(blackjid);
			free(barejid);
			return info;
		}

		free(whitejid);
		free(blackjid);

		entry = entry->next;
	}

	free(barejid);
	return NULL;
}

int Model_GetPermission(char *permission)
{
	return (NamedList_GetByName(&local_permissions, permission) != NULL);
}

unsigned char PNGHeader[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
unsigned char JPGHeader[] = { 0xFF, 0xD8, 0xFF, 0xE0 };
unsigned char GIFHeader[] = { 0x47, 0x49, 0x46, 0x38, 0x37, 0x61 };

struct aservicedata_s
{
	char *sha1;
	char *jid;
	char *filename;
};

void Model_FinishAvatarDownload(struct aservicedata_s *data)
{
	char filename1[MAX_PATH];
	char filename2[MAX_PATH];
	unsigned char compare[8];
	FILE *fp;
	int found = 0;
	char *sha1 = data->sha1;

	fp = fopen(data->filename, "rb");
	if (!fp)
	{
		return;
	}

	fseek(fp, 0, SEEK_SET);
	fread(compare, 8, 1, fp);
	fclose(fp);

	sha1 = SHA1_GetHexDigestForFile(data->filename);
	strlwr(sha1);

	if (!found)
	{
		int i, check = 1;

		for (i = 0; i < 8; i++)
		{
			if (compare[i] != PNGHeader[i])
			{
				check = 0;
			}
		}

		if (check)
		{
			Util_GetPrivateSavePath(filename2, MAX_PATH);
			strncat(filename2, "/avatars", MAX_PATH);
			mkdir(filename2);
			strncat(filename2, "/", MAX_PATH);
			strncat(filename2, sha1, MAX_PATH);
			strncat(filename2, ".png", MAX_PATH);
			MoveFile(data->filename, filename2);
			found = 1;
		}
	}

	if (!found)
	{
		int i, check = 1;

		for (i = 0; i < 4; i++)
		{
			if (compare[i] != JPGHeader[i])
			{
				check = 0;
			}
		}

		if (check)
		{
			Util_GetPrivateSavePath(filename2, MAX_PATH);
			strncat(filename2, "/avatars", MAX_PATH);
			mkdir(filename2);
			strncat(filename2, "/", MAX_PATH);
			strncat(filename2, sha1, MAX_PATH);
			strncat(filename2, ".jpg", MAX_PATH);
			MoveFile(data->filename, filename2);
			found = 1;
		}
	}

	if (!found)
	{
		int i, check = 1;

		for (i = 0; i < 6; i++)
		{
			if (compare[i] != GIFHeader[i])
			{
				check = 0;
			}
		}

		if (check)
		{
			Util_GetPrivateSavePath(filename2, MAX_PATH);
			strncat(filename2, "/avatars", MAX_PATH);
			mkdir(filename2);
			strncat(filename2, "/", MAX_PATH);
			strncat(filename2, sha1, MAX_PATH);
			strncat(filename2, ".gif", MAX_PATH);
			MoveFile(data->filename, filename2);
			found = 1;
		}
	}

	if (found)
	{
		Model_SetFriendAvatarHash(data->jid, sha1, 1, 0);
	}
}

void Model_RequestAvatarService(char *sha1, char *jid)
{
	char url[512];
	char filename[MAX_PATH];
	struct aservicedata_s *data;

	sprintf(url, "http://www.chesspark.com/avatar-service/lookup/?jid=%s", jid);

	Util_GetPrivateSavePath(filename, MAX_PATH);
	strncat(filename, "/avatars", MAX_PATH);
	mkdir(filename);
	strncat(filename, "/", MAX_PATH);
	strncat(filename, sha1, MAX_PATH);

	data = malloc(sizeof(*data));
	data->sha1 = strdup(sha1);
	data->jid = strdup(jid);
	data->filename = strdup(filename);

	NetTransfer_AddDownload(url, filename, Model_FinishAvatarDownload, data);
}

void Model_RequestAvatarServiceJid(char *jid)
{
	char url[512];
	char filename[512];
	struct aservicedata_s *data;

	sprintf(url, "http://www.chesspark.com/avatar-service/lookup/?jid=%s", jid);

	Util_GetPrivateSavePath(filename, MAX_PATH);
	strncat(filename, "/avatars", MAX_PATH);
	mkdir(filename);
	strncat(filename, "/", MAX_PATH);
	strncat(filename, Jid_GetBeforeAt(jid), MAX_PATH);

	data = malloc(sizeof(*data));
	data->sha1 = NULL;
	data->jid = strdup(jid);
	data->filename = strdup(filename);

	NetTransfer_AddDownload(url, filename, Model_FinishAvatarDownload, data);
}


char *Model_GetLocalMemberType()
{
	if (local_membertype)
	{
		return local_membertype;
	}

	return "unknown";
}

int Model_IsLocalMemberFree(int noinforesult)
{
	if (local_membertype)
	{
		if (stricmp(local_membertype, "trial") == 0 || stricmp(local_membertype, "free") == 0)
		{
			return 1;
		}
		return 0;
	}
	return noinforesult;
}

int Model_IsLocalMemberNotActivated()
{
	return local_notactivated;
}

#define NUMPINGS 10

int lastping = 0;
int lastpings[NUMPINGS];
unsigned int lastpingtime[NUMPINGS];
int numlastpings = 0;
int currlastping = 0;

int Model_GetAvgPing()
{
	int avg = 0, i, numpings = 0;;
	for (i = 0; i < numlastpings; i++)
	{
		/*if (GetTickCount() - lastpingtime[i] < 1000 * 60 * 5)*/
		{
			avg += lastpings[i];
			numpings++;
		}
	}

	avg /= numlastpings;

	return avg;
}

int Model_GetLastPing()
{
	return lastping;
}

int Model_GetSpikePing()
{
	int spk = 0, i;
	for (i = 0; i < numlastpings; i++)
	{
		/*if (GetTickCount() - lastpingtime[i] < 1000 * 60 * 5)*/
		{
			if (lastpings[i] > spk)
			{
				spk = lastpings[i];
			}
		}
	}

	return spk;
}

void Model_AddPing(int ping)
{
	lastping = ping;
	lastpings[currlastping] = ping;
	lastpingtime[currlastping] = GetTickCount();
	currlastping = (currlastping + 1) % NUMPINGS;
	
	if (numlastpings < NUMPINGS)
	{
		numlastpings++;
	}

	View_UpdatePings(Model_GetLastPing(), Model_GetAvgPing(), Model_GetSpikePing());
}

int Model_PeriodicPing(void *dummy)
{
	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return 1;
	}

	Conn_Ping("match.chesspark.com", 0, 0);

	return 0;
}

void ModelConn_SubscribeProfileSuccess(char *jid)
{
	/* force a get so it's up to date */
	Model_GetProfile(jid, 2);
}

static unsigned int connecttick;

void Model_SetLoginStartTime()
{
	Log_Write2(0, "Login timing started...\n");
	connecttick = GetTickCount();
}

void Model_ShowLoginTime(char *location)
{
	Log_Write2(0, "Timing location %s, %d ms since connect\n", location, GetTickCount() - connecttick);
}
