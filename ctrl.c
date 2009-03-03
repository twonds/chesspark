#include <stdio.h>
#include <stdlib.h>

#include <strophe.h>

#include <zlib.h>

#include "box.h"

#include "conn.h"
#include "constants.h"
#include "httppost.h"
#include "imagemgr.h"
#include "log.h"
#include "model.h"
#include "util.h"
#include "view.h"
#include "info.h"

#include "ctrl.h"

#include "autowait.h"

static char *login_jid = NULL;
static char *login_fulljid = NULL;
static char *password = NULL;

void Ctrl_HandleUpdateCheck(char *filename)
{
	FILE *fp;
	int len;
	char *text;
	char *p, *version;
	char *find;
	float fversion;
	float frunningversion;
	int build, num;
	int runningbuild;

	fp = fopen(filename, "r");

	find = "Latest Version: ";

	if (!fp)
	{
		return;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (len == 0)
	{
		fclose(fp);
		free(filename);
		return;
	}

	text = malloc(len + 1);
	fread(text, 1, len, fp);
	fclose(fp);

	free(filename);

	text[len] = '\0';

	p = strstr(text, find);

	if (!p)
	{
		free(text);
		return;
	}

	version = p + strlen(find);

	num = sscanf(version, "%f Build %d", &fversion, &build);

	if (num != 2)
	{
		return;
	}

	sscanf(CHESSPARK_VERSION, "%f", &frunningversion);
	sscanf(CHESSPARK_BUILD, "%d", &runningbuild);

	if ((frunningversion < fversion) || (frunningversion == fversion && runningbuild < build))
	{
		char *filename;
		char url[512];
		char cnumber[512];
		char cversion[512];
		char cbuild[512];

		find = "href=\"/download/";
		p = strstr(text, find);

		if (!p)
		{
			free(text);
			return;
		}

		filename = p + 6;
		p = strchr(filename, '"');

		if (!p)
		{
			free(text);
			return;
		}

		strcpy(url, "http://www.chesspark.com");
		strncat(url, filename, p - filename);

		cnumber[0] = '\0';
		sprintf(cversion, "%1.1f", fversion);
		sprintf(cbuild, "%d", build);

		Model_HandleClientUpgrade(cnumber, cversion, cbuild, url);
	}

	free(text);

}

void Ctrl_Start()
{
	char buffer1[256], buffer2[256], buffer3[256];
	char filename[MAX_PATH];

	View_PopupLogin();
	if (GetRegInt("autologin") && GetRegString("language", buffer3, 256))
	{
		Ctrl_Login(1, GetRegString("autologin_username", buffer1, 256), GetRegString("autologin_password", buffer2, 256));
	}

	Util_GetPrivateSavePath(filename, MAX_PATH);
	strncat(filename, "/updatecheck.html", MAX_PATH);

	NetTransfer_AddDownload("http://www.chesspark.com/download/", filename, Ctrl_HandleUpdateCheck, strdup(filename));
}


void Ctrl_Init()
{
	Conn_Init();
	Model_Init();
}

char *Ctrl_GetFullLoginJid()
{
	return login_fulljid;
}


void Ctrl_Login(int remember, char *jid, char *pass)
{
	char fulljid[512];
	char *p;
	if (!jid || strlen(jid) == 0)
	{
		View_SetLoginNoUsername();
		return;
	}

	if (!Jid_IsValid(jid))
	{
		View_SetLoginBadUsername();
		return;
	}

	if (!pass || strlen(pass) == 0)
	{
		View_SetLoginNoPassword();
		return;
	}

	strcpy(fulljid, jid);
	p = strchr(fulljid, '/');

	if (p && (*(p+1) == '\0'))
	{
		*p = '\0';
		p = NULL;
	}

	if (!p)
	{
		strcat(fulljid, "/cpc");
	}

	p = fulljid;
	while (*p)
	{
		if (*p >= 'A' && *p <= 'Z')
		{
			*p += 'a' - 'A';
		}
		p++;
	}
	login_fulljid = strdup(fulljid);

	login_jid = strdup (jid);

	p = login_jid;
	while (*p)
	{
		if (*p >= 'A' && *p <= 'Z')
		{
			*p += 'a' - 'A';
		}
		p++;
	}

	password = strdup(pass);

	Model_SetLogin(remember, login_jid, pass);

	View_SetLoginConnecting();
	Conn_Login(fulljid, pass);
}


void Ctrl_CancelLogin()
{
	Model_CancelLogin();
}


void Ctrl_LoginLast()
{
	Model_SetLoginLast();
	Conn_Login(login_fulljid, password);
}


void Ctrl_LoginToGameServer()
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Model_LoginToChesspark();
		Model_SetPriority(1);
	}
}

BOOL Ctrl_Poll()
{
	BOOL boxret;

	Conn_Poll();

	boxret = Box_Poll();

	View_Poll();

	Model_Poll();

	return boxret;
}


void Ctrl_AddFriend(char *jid, char *name)
{
	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	Model_AddFriend(jid, name);
}

void Ctrl_AddFriendToGroup(char *jid, char *group)
{
	Model_AddFriendToGroup(jid, group);
}

void Ctrl_RemoveFriendFromGroup(char *jid, char *group)
{
	Model_RemoveFriendFromGroup(jid, group);
}

void Ctrl_MoveFriendFromGroupToGroup(char *jid, char *fromgroup, char *togroup)
{
	Model_MoveFriendFromGroupToGroup(jid, fromgroup, togroup);
}

void Ctrl_RemoveFriend(char *jid)
{
	Model_RemoveFriend(jid);
}

void Ctrl_ApproveFriend(char *jid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_ApproveFriend(jid);
	}
}

void Ctrl_GetRoster()
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_GetRoster();
	}
}

void Ctrl_SendMessage(char *jid, char *text)
{
	Model_SendMessage(jid, text);
	/*Conn_SendMessage(jid, login_jid, text, 0);*/
}

void Ctrl_SendGroupMessage(char *tojid, char *text)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendMessage(tojid, text, 1);
	}
}

void Ctrl_RequestFriend(char *jid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_RequestFriend(jid);
	}
}

void Ctrl_RejectFriend(char *jid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_RejectFriend(jid);
	}
}

void Ctrl_SetPresence(enum SStatus status, char *statusmsg)
{
	if (Conn_GetConnState() == CONN_NOTCONNECTED && login_jid)
	{
		View_RosterAutoReconnect();
		Ctrl_LoginLast();
	}
	
	if (Conn_GetConnState() == CONN_CONNECTED && status == SSTAT_AWAY)
	{
		View_KillGameAds();
	}
	Model_SetPresence(status, statusmsg);
}

void Ctrl_ChangeUser()
{
	Model_ChangeUser();
};

void Ctrl_Disconnect()
{
	Model_Disconnect();
}

void Ctrl_RemoveGroup(char *name)
{
	Model_RemoveGroup(name);
}

void Ctrl_RemoveGroupAndContents(char *name)
{
	Model_RemoveGroupAndContents(name);
}

void Ctrl_RenameGroup(char *oldname, char *newname)
{
	Model_RenameGroup(oldname, newname);
}

void Ctrl_JoinGroupChat(char *jid, char *nickname)
{
	/* No double join */
	if (View_IsInRoom(jid))
	{
		return;
	}
	Model_JoinGroupChat(jid, nickname);
}

void Ctrl_JoinGroupChatDefaultNick(char *jid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		if (strstr(jid, "@chat.chesspark.com") && CHESSPARK_LOCALCHATSUSEJIDNICK)
		{
			char *barejid = Jid_Strip(Model_GetLoginJid());
			Ctrl_JoinGroupChat(jid, barejid);
			View_PopupChatDialog(jid, barejid, 1);
			View_ActivateChatDialog(jid, barejid, 1);
			free(barejid);
		}
		else
		{
			Ctrl_JoinGroupChat(jid, Model_GetDefaultNick());
			View_PopupChatDialog(jid, Model_GetDefaultNick(), 1);
			View_ActivateChatDialog(jid, Model_GetDefaultNick(), 1);
		}
	}
}

void Ctrl_AddStatus(enum SStatus status, char *statusmsg)
{
	Model_AddStatus(status, statusmsg);
}

void Ctrl_SetStatusList(enum SStatus status, struct StatusList_s *list)
{
	Model_SetStatusList(status, list);
}

char *Ctrl_GetDefaultNick()
{
	return Model_GetDefaultNick();
}

char *Ctrl_GetAvatarFilenameByHash(char *hash, char *filename, int filenamelen)
{
	FILE *fp;
	char savepath[MAX_PATH];

	Util_GetPrivateSavePath(savepath, MAX_PATH);

	strncpy(filename, savepath, filenamelen);
	strncat(filename, "/avatars/", filenamelen);
	strncat(filename, hash, filenamelen);
	strncat(filename, ".gif", filenamelen);

	fp = fopen(filename, "r");
	if (fp)
	{
		fclose(fp);
		return filename;
	}
	
	strncpy(filename, savepath, filenamelen);
	strncat(filename, "/avatars/", filenamelen);
	strncat(filename, hash, filenamelen);
	strncat(filename, ".jpg", filenamelen);

	fp = fopen(filename, "r");
	if (fp)
	{
		fclose(fp);
		return filename;
	}
	
	strncpy(filename, savepath, filenamelen);
	strncat(filename, "/avatars/", filenamelen);
	strncat(filename, hash, filenamelen);
	strncat(filename, ".png", filenamelen);

	fp = fopen(filename, "r");
	if (fp)
	{
		fclose(fp);
		return filename;
	}

	return NULL;
}

void Ctrl_GetRooms()
{
	Model_GetRooms();
}

void Ctrl_SetTopic(char *chatjid, char *topic)
{
	Model_SetTopic(chatjid, topic);
}

void Ctrl_SetTopicCallback(char *topic, char *chatjid)
{
	Model_SetTopic(chatjid, topic);
}

void Ctrl_ChangeNick(char *chatjid, char *newnick)
{
	Model_ChangeNick(chatjid, newnick);
}

void Ctrl_KickUser(char *chatjid, char *nick, char *reason)
{
	Model_KickUser(chatjid, nick, reason);
}

void Ctrl_BanUser(char *chatjid, char *nick, char *reason)
{
	Model_BanUser(chatjid, nick, reason);
}

void Ctrl_UnbanUser(char *chatjid, char *nick, char *reason)
{
	Model_UnbanUser(chatjid, nick, reason);
}

void Ctrl_InviteUser(char *chatjid, char *userjid, char *reason)
{
	Model_InviteUser(chatjid, userjid, reason);
}

void Ctrl_GiveVoice(char *chatjid, char *nick, char *reason)
{
	Model_GiveVoice(chatjid, nick, reason);
}

void Ctrl_RevokeVoice(char *chatjid, char *nick, char *reason)
{
	Model_RevokeVoice(chatjid, nick, reason);
}

void Ctrl_GiveMod(char *chatjid, char *userjid, char *reason)
{
	Model_GiveMod(chatjid, userjid, reason);
}

void Ctrl_RevokeMod(char *chatjid, char *userjid, char *reason)
{
	Model_RevokeMod(chatjid, userjid, reason);
}

void Ctrl_GiveAdmin(char *chatjid, char *userjid, char *reason)
{
	Model_GiveAdmin(chatjid, userjid, reason);
}

void Ctrl_RevokeAdmin(char *chatjid, char *userjid, char *reason)
{
	Model_RevokeAdmin(chatjid, userjid, reason);
}

void Ctrl_GiveOwner(char *chatjid, char *userjid, char *reason)
{
	Model_GiveOwner(chatjid, userjid, reason);
}

void Ctrl_RevokeOwner(char *chatjid, char *userjid, char *reason)
{
	Model_RevokeOwner(chatjid, userjid, reason);
}

void Ctrl_ClearChatHistory(char *chatjid)
{
	Model_ClearChatHistory(chatjid);
}

void Ctrl_AddChatToFavorites(char *chatjid)
{
	Model_AddChatToFavorites(chatjid);
}

void Ctrl_RemoveChatFromFavorites(char *chatjid)
{
	Model_RemoveChatFromFavorites(chatjid);
}

void Ctrl_AddChatToAutojoin(char *chatjid)
{
	Model_AddChatToAutojoin(chatjid);
}

void Ctrl_RemoveChatFromAutojoin(char *chatjid)
{
	Model_RemoveChatFromAutojoin(chatjid);
}

int Ctrl_IsChatInFavorites(char *chatjid)
{
	return Model_IsChatInFavorites(chatjid);
}

int Ctrl_IsChatInAutojoin(char *chatjid)
{
	return Model_IsChatInAutojoin(chatjid);
}

void Ctrl_SendMove(char *gameid, char *move)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendMove(gameid, move, 0);
	}
}

void Ctrl_SendCorMove(char *gameid, char *move)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendMove(gameid, move, 1);
	}
}

void Ctrl_SendCorLookAheadMove(char *gameid, char *move)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendMove(gameid, move, 2);
	}
}

void Ctrl_WatchGame(char *jid, int rotate)
{
	char *gameid;
	char realjid[80];
	char *at;

	if (Conn_GetConnState() != CONN_CONNECTED)
	{
		return;
	}

	gameid = strdup(jid);

	if (View_GameToFront(gameid))
	{
		return;
	}

	at = strchr(gameid, '@');

	if (at)
	{
		*at = '\0';
	}

	strcpy(realjid, gameid);
	strcat(realjid, "@games.chesspark.com");

	ModelConn_JoinGame(gameid, realjid, NULL, NULL);
	View_SetGameViewRotated(gameid, rotate);
}

void Ctrl_WatchGame2(struct Box_s *pbox, char *gameid)
{
	Ctrl_WatchGame(gameid, 0);
}

void Ctrl_RequestMatch(char *opponentjid, struct gamesearchinfo_s *info)
{
	Model_RequestMatch(opponentjid, info);
}

void Ctrl_PostGameAd(struct gamesearchinfo_s *info)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_PostGameAd(info);
	}
}

void Ctrl_RequestProfile(char *jid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		char *barejid = Jid_Strip(jid);
		Conn_RequestProfile(barejid);
		free(barejid);
	}
}

void Ctrl_AcceptMatch(char *gameid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_AcceptMatch(gameid, NULL);
	}
}

void Ctrl_AcceptRematch(char *gameid, char *oldgameid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_AcceptMatch(gameid, oldgameid);
	}
}
void Ctrl_AcceptCorMatch(char *gameid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_AcceptCorMatch(gameid);
	}
}

void Ctrl_DeclineMatch(char *gameid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_DeclineMatch(gameid, NULL);
	}
}

void Ctrl_DeclineRematch(char *gameid, char *oldgameid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_DeclineMatch(gameid, oldgameid);
	}
}
void Ctrl_RemoveGameAd(char *itemid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_RemoveGameAd(itemid);
	}
}

void Ctrl_RequestGameSearch(char *node, struct gamesearchinfo_s *info)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_RequestGameSearch(node, info);
	}
}

void Ctrl_ShowProfile(char *jid)
{
	Model_ShowProfile(jid);
}

void Ctrl_ShowProfile2(struct Box_s *pbox, char *jid)
{
	Model_ShowProfile(jid);
}

void Ctrl_SaveProfile(char *jid, char *nick)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Model_SaveProfile(jid, nick);
	}
}

void Ctrl_LeaveChat(char *chatjid)
{
	Log_Write(0, "Ctrl_LeaveChat(%s)\n", chatjid);

	Model_LeaveChat(chatjid);
}

void Ctrl_RequestRematch(char *gameid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_RequestRematch(gameid);
	}
}

void Ctrl_ViewTournament(char *id)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		char jid[256];
		char chatname[512];

		sprintf(jid, "tournament%s@games.chesspark.com", id);
		sprintf(chatname, "%s/%s", jid, Model_GetDefaultNick());

		Conn_GetTournamentInfo(id);
		Conn_SendPresence(SSTAT_AVAILABLE, NULL, chatname, 1, NULL, 0, 0);
		View_PopupTourneyDialog(id, chatname);
	}
}

void Ctrl_JoinTournament(char *id)
{
	Model_JoinTournament(id);
}

void Ctrl_ForfeitTournament(char *id)
{
	Model_ForfeitTournament(id);
}

void Ctrl_SetTournamentRoundStart(char *tourneyid, char *round, char *timestamp)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SetTournamentRoundStart(tourneyid, round, timestamp);
	}
}

void Ctrl_StartNextTournamentRound(char *tourneyid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_StartNextTournamentRound(tourneyid);
	}
}

void Ctrl_InviteToChat(char *friendjid, char *chatjid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_InviteUser(chatjid, friendjid, NULL);
	}
}

void Ctrl_SetProfileDescription(char *description)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Model_SetLocalProfileDescription(description);
		Conn_SetProfileField("description", description);
	}
}

void Ctrl_InviteWatchGame(char *friendjid, char *gameid)
{
	char chatjid[512];

	strcpy(chatjid, gameid);
	strcat(chatjid, "@games.chesspark.com");

	Ctrl_InviteToChat(friendjid, chatjid);
}

void Ctrl_AdHocCommand(char *jid, char *node)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Model_AdHocCommand(jid, node);
	}
}

void Ctrl_PlayNow()
{
	View_PlayNow();
}

void Ctrl_RequestBanlist(char *jid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_RequestBanlist(jid);
	}
}

void Ctrl_Ping(char *jid, int show, int repeat)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Model_Ping(jid, show, repeat);
	}
}

void Ctrl_Ignore(char *jid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Model_AddJIDToIgnore(jid);
	}
}

void Ctrl_Unignore(char *jid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Model_RemoveJIDFromIgnore(jid);
	}
}

void Ctrl_AddTimeCallback(char *time, char *gameid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		char finaltime[256];
		int itime;

		sscanf(time, "%d", &itime);

		if (itime < 0)
		{
			return;
		}

		if (itime > 1800)
		{
			itime = 1800;
		}

		sprintf(finaltime, "%d", itime);

		Conn_SendAddTime(gameid, finaltime);
		View_AddGameTimeNonLocal(gameid, itime);
	}
}

void Ctrl_FinishProbReportPost(void *data)
{
	free(data);
}

void Ctrl_PostFullProblemReport(char *category, char *reason, int log)
{
	int len = 0;
	unsigned char *data = NULL;
	char appdata[MAX_PATH];
	char filename[MAX_PATH];
	char filename2[MAX_PATH];

	Util_GetPrivateSavePath(appdata, MAX_PATH);

	strcpy(filename, appdata);
	strcat(filename, "/debug.log");

	strcpy(filename2, appdata);
	strcat(filename2, "/debug.log.gz");

	if (log)
	{
		FILE *fp;
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

		{
			gzFile gzf;

			gzf = gzopen(filename2, "wb");
			gzwrite(gzf, data, len);
			gzclose(gzf);
		}

		free(data);

		if (!(fp = fopen(filename2, "rb")))
		{
			return;
		}

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		data = malloc(len);
		fread(data, 1, len, fp);
		fclose(fp);
	}

	{
		struct Box_s *waitingbox;
                /*HttpPost("www.chesspark.com", Model_GetLoginJid(), category, reason, "debug.log.gz", data, len);*/
		if (Model_IsLocalMemberNotActivated())
		{
			waitingbox = View_PopupWaitingDialog("Report upload", "Now uploading your problem report...", "Thank you for submitting a report, we appreciate your help in making Chesspark a better place.\n\nResponses will be sent by email to the address you registered with.");
		}
		else
		{
			waitingbox = View_PopupWaitingDialog("Report upload", "Now uploading your problem report...", "Thank you for submitting a report, we appreciate your help in making Chesspark a better place.\n\nYou may not receive a response, as your email address has not been confirmed.");
		}
		Box_RegisterWindowName(waitingbox, "UploadingWait");
		NetTransfer_PostFile("www.chesspark.com/log/submit/", Model_GetLoginJid(), category, reason, filename2, data, len, Ctrl_FinishProbReportPost, data, AutoWaitName_SetProgress, "UploadingWait");
	}
/*
	if (log)
	{
		free(data);
	}
	*/
}

void Ctrl_PostDebugLog(char *reason, void *dummy)
{
	Ctrl_PostFullProblemReport(NULL, reason, 1);
}

void Ctrl_PostProblem(char *type, char *prob, int log)
{
	Ctrl_PostFullProblemReport(type, prob, log);
}

void Ctrl_GetCorGameState(char *gameid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_GetCorGameState(gameid);
	}
}

void Ctrl_SendGameFlag(char *gameid, int correspondence)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendGameTag(gameid, "flag", correspondence);
	}
}

void Ctrl_SendGameAdjudication(char *gameid, int correspondence)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendGameTag(gameid, "adjudication", correspondence);
	}
}

void Ctrl_SendGameAdjourn(char *gameid, int correspondence)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendGameTag(gameid, "adjourn", correspondence);
	}
}

void Ctrl_SendGameDraw(char *gameid, int correspondence)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendGameTag(gameid, "draw", correspondence);
	}
}

void Ctrl_SendGameAbort(char *gameid, int correspondence)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendGameTag(gameid, "abort", correspondence);
	}
}

void Ctrl_SendGameTakeback(char *gameid, int correspondence)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendGameTag(gameid, "takeback", correspondence);
	}
}

void Ctrl_SendGameResign(char *gameid, int correspondence)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendGameTag(gameid, "resign", correspondence);
	}
}

void Ctrl_RejectGameAdjourn(char *gameid, int correspondence)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendGameTag(gameid, "rejectadjourn", correspondence);
	}
}

void Ctrl_RejectGameAbort(char *gameid, int correspondence)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendGameTag(gameid, "rejectabort", correspondence);
	}
}

void Ctrl_RejectGameTakeback(char *gameid, int correspondence)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendGameTag(gameid, "rejecttakeback", correspondence);
	}
}

void Ctrl_RejectGameDraw(char *gameid, int correspondence)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendGameTag(gameid, "rejectdraw", correspondence);
	}
}

void Ctrl_GameReconvene(char *gameid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_GameReconvene(gameid);
	}
}

void Ctrl_PlayAd(char *adid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_PlayAd(adid);
	}
}

void Ctrl_SetPushFilter(char *node, struct gamesearchinfo_s *info)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SetPushFilter(node, info);
	}
}

void Ctrl_SetPushGamesActivation(int active)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SetPushGamesActivation(active);
	}
}

void Ctrl_SendPresence(enum SStatus fstatus, char *statusmsg, char *tojid, int ismuc, char *avatarhash, int prioritynum, int inclpriority)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Conn_SendPresence(fstatus, statusmsg, tojid, ismuc, avatarhash, prioritynum, inclpriority);
	}
}

void Ctrl_InviteToGame(char *jid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		struct gamesearchinfo_s *info = Model_HaveAdjournedGameWithJid(jid);
		if (info)
		{
			View_PopupReconveneReminderDialog(jid, info);
		}
		else
		{
			View_PopupGameInviteFriendDialog(jid, NULL, 0);
		}
	}
}
