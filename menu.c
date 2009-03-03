#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>

#include "box.h"
#include "button.h"
#include "edit.h"

#include "autoedit.h"
#include "button2.h"
#include "ctrl.h"
#include "conn.h"
#include "chatbox.h"
#include "constants.h"
#include "i18n.h"
#include "imagemgr.h"
#include "info.h"
#include "log.h"
#include "model.h"
#include "namedlist.h"
#include "options.h"
#include "text.h"
#include "view.h"

#include "util.h"

#define MENU_STATUS_AVAILABLE	1000
#define MENU_STATUS_AWAY		1500
#define MENU_STATUS_OFFLINE		1997
#define MENU_ADDSTATUS			1998
#define MENU_EDITSTATUSES		1999

#define MENU_SHOWFRIENDOFFLINE          2000
#define MENU_SHOWFRIENDAVATARS          2001
#define MENU_SHOWFRIENDGROUPS           2002
#define MENU_SHOWFRIENDSTAB             2003
#define MENU_SHOWROOMSTAB               2004
#define MENU_SHOWGAMESTAB               2005
#define MENU_SHOWGAMESDIALOG            2006
#define MENU_AUTOAPPROVEFRIEND          2007
#define MENU_OPTIONS                    2008
#define MENU_GAMEHISTORY                2009
#define MENU_CHANGEUSER                 2010
#define MENU_ABOUTCHESSPARK             2011
#define MENU_PING                       2012
#define MENU_SENDDEBUGLOG               2013
#define MENU_QUIT                       2014
#define MENU_DEMOGAME                   2015

#define MENU_ADDFRIEND                  3000
#define MENU_ADDGROUP                   3001
#define MENU_ADDGAMEAD                  3002
#define MENU_NEWCHATFRIEND              3003
#define MENU_JOINCHATROOM               3004
	
#define MENU_MESSAGEFRIEND              4001
#define MENU_GAMEFRIEND                 4002
#define MENU_REMOVEFRIEND               4003
#define MENU_INFOFRIEND                 4004
#define MENU_LOGFRIEND                  4005
#define MENU_GAMEFRIENDCORR             4006
#define MENU_FRIENDADHOC                4018
#define MENU_INVITEFRIENDTOCHESSPARK    4019
#define MENU_IGNOREFRIEND               4020
#define MENU_UNIGNOREFRIEND             4021
#define MENU_OBSERVEFRIEND              4500
#define MENU_LASTOBSERVEFRIEND          4999

#define MENU_GIVEVOICE                  4007
#define MENU_REVOKEVOICE                4008
#define MENU_KICK                       4009
#define MENU_BAN                        4010
#define MENU_GIVEMOD                    4011
#define MENU_REVOKEMOD			4012
#define MENU_GIVEADMIN			4013
#define MENU_REVOKEADMIN		4014
#define MENU_GIVEOWNER			4015
#define MENU_REVOKEOWNER		4016
#define MENU_CLEARCHATHISTORY           4017

#define MENU_REQUESTFRIEND		5000

#define MENU_SHOWCHATPARTICIPANTS       6000
#define MENU_INVITETOTHISCHAT           6001
#define MENU_CHANGECHATTOPIC            6002
#define MENU_CHATOPTIONS                6003
#define MENU_ADDCHATTOFAVORITES         6004
#define MENU_REMOVECHATFROMFAVORITES	6005
#define MENU_ADDCHATTOAUTOJOIN          6006
#define MENU_REMOVECHATFROMAUTOJOIN     6007

#define MENU_RENAMEGROUP		7000
#define MENU_REMOVEGROUP		7001

#define MENU_SAVEGAME                   7501
#define MENU_SAVEGAMEAS                 7502
#define MENU_REQUESTADJOURN             7503
#define MENU_REQUESTDRAW                7504
#define MENU_REQUESTABORT               7505
#define MENU_RESIGNGAME	                7506
#define MENU_ADDOPPTIME                 7507
#define MENU_REQUESTTAKEBACK            7508
#define MENU_FLAGOPPTIME                7509
#define MENU_CASTLEKINGSIDE             7510
#define MENU_CASTLEQUEENSIDE            7511

#define MENU_JOINTHISCHAT		8000

#define MENU_JOINTOURNEY		8500
#define MENU_FORFEITTOURNEY		8501
#define MENU_SETTOURNEYROUNDSTART 8502
#define MENU_STARTNEXTTOURNEYROUND 8503

#define MENU_CUT				9997
#define MENU_COPY				9996
#define MENU_PASTE				9995
#define MENU_DELETE				9994
#define MENU_OPENURL				9993
#define MENU_COPYURL				9992
#define MENU_TEST 9999
#define MENU_TEST2 9998

HMENU presencehmenu;
HMENU prefshmenu;
HMENU addhmenu;
HMENU friendhmenu;
HMENU pendingfriendhmenu;
HMENU chatboxprefshmenu;
HMENU friendstabblankhmenu;
HMENU grouphmenu;
HMENU chatparticipanthmenu;
HMENU chataddhmenu;
HMENU roomhmenu;
HMENU roomstabblankhmenu;
HMENU gamehmenu;
HMENU edithmenu;
HMENU tourneyhmenu;
HMENU urlhmenu;
HMENU gamefriendsubhmenu;
HMENU moderationsubhmenu;
HMENU profilehmenu;

static struct StatusList_s *awaystatuslist  = NULL;
static struct StatusList_s *availstatuslist = NULL;

char *cmdtarget = NULL;
char *cmdtargetmuc = NULL;
char *cmdtargetnick = NULL;
char *cmdgroup = NULL;
char *cmdgame = NULL;
char *cmdadhoc = NULL;
char *cmdtopic = NULL;
struct Box_s *edittarget = NULL;
struct Box_s *cmdbox = NULL;
struct namedlist_s *cmdlist1 = NULL;
struct namedlist_s *cmdlist2 = NULL;
struct namedlist_s *cmdlist3 = NULL;

void Menu_RedoStatusMenu();
void Menu_RedoStatusMenu2();

BOOL AppendMenuWrapper(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCTSTR lpNewItem)
{
	BOOL result;

	if ((uFlags & MF_BITMAP) != MF_BITMAP && (uFlags & MF_OWNERDRAW) != MF_OWNERDRAW)
	{
		WCHAR *wideitem = Util_ConvertUTF8ToWCHAR(lpNewItem);

		if (Util_OldWinVer())
		{
			char *ansiitem = Util_ConvertWCHARToANSI(wideitem);

			result = AppendMenuA(hMenu, uFlags, uIDNewItem, ansiitem);

			free(ansiitem);
		}
		else
		{
			result = AppendMenuW(hMenu, uFlags, uIDNewItem, wideitem);
		}

		free (wideitem);
	}
	else
	{
		result = AppendMenuA(hMenu, uFlags, uIDNewItem, lpNewItem);
	}

	return result;
}

BOOL ModifyMenuWrapper(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCTSTR lpNewItem)
{
	BOOL result;

	if ((uFlags & MF_BITMAP) != MF_BITMAP && (uFlags & MF_OWNERDRAW) != MF_OWNERDRAW)
	{
		WCHAR *wideitem = Util_ConvertUTF8ToWCHAR(lpNewItem);

		if (Util_OldWinVer())
		{
			char *ansiitem = Util_ConvertWCHARToANSI(wideitem);

			result = ModifyMenuA(hMenu, uPosition, uFlags, uIDNewItem, ansiitem);

			free(ansiitem);
		}
		else
		{
			result = ModifyMenuW(hMenu, uPosition, uFlags, uIDNewItem, wideitem);
		}

		free (wideitem);
	}
	else
	{
		result = ModifyMenuA(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
	}

	return result;
}

void AppendGamesListHelper(HMENU hmenu, char *name, struct namedlist_s *games, char *ignoregame, int *addedentry, int watch)
{
	struct namedlist_s *currententry = games;
	char txt[256];
	struct gamesearchinfo_s *info;
	char *localjid = Jid_Strip(Model_GetLoginJid());
	char *friendjid = Jid_Strip(name);

	while (currententry)
	{
		int enabled = 1;
		info = currententry->data;

		if (info)
		{
			char *gamejid1 = Jid_Strip(info->white->jid);
			char *gamejid2 = Jid_Strip(info->black->jid);

			if (!info->variant || (info->variant && stricmp(info->variant, "standard") == 0))
			{
				char txt2[256];

				i18n_stringsub(txt2, 256, _("%1 vs. %2 - %3 %4"), Model_GetFriendNick(info->white->jid), Model_GetFriendNick(info->black->jid), info->rated ? _("Rated") : _("Unrated"), Info_TimeControlToCategory(info->timecontrol));
				sprintf(txt, "      %s", txt2);
			}
			else
			{
				char txt2[256];

				i18n_stringsub(txt2, 256, _("%1 vs. %2 - %3 %4, %5"), Model_GetFriendNick(info->white->jid), Model_GetFriendNick(info->black->jid), info->rated ? _("Rated") : _("Unrated"), info->variant, Info_TimeControlToCategory(info->timecontrol));
				sprintf(txt, "      %s", txt2);
			}
				
			
			if (ignoregame && stricmp(ignoregame, info->gameid) == 0)
			{
				enabled = 0;
			}
			else if (watch)
			{
                                if (stricmp(gamejid1, localjid) == 0 || stricmp(gamejid2, localjid) == 0)
				{
					enabled = 0;
				}
			}
			else
			{
                                if (stricmp(gamejid1, friendjid) == 0 || stricmp(gamejid2, friendjid) == 0)
				{
					enabled = 0;
				}
			}

			AppendMenuWrapper(hmenu, MF_STRING | (enabled ? 0 : MF_GRAYED), MENU_OBSERVEFRIEND + *addedentry, txt);

			free(gamejid1);
			free(gamejid2);
		}

		(*addedentry)++;
		currententry = currententry->next;
	}

	free(localjid);
	free(friendjid);
}

void AppendGamesList(HMENU hmenu, char *name, struct namedlist_s *gamesplaying, struct namedlist_s *gameswatching, struct namedlist_s *localgames, char *ignoregame)
{
	char txt[256];
	int addedentry = 0;
	int skip = 0;

	if (gamesplaying || gameswatching || localgames)
	{
		AppendMenuWrapper(hmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	}

	if (gamesplaying)
	{
		i18n_stringsub(txt, 256, _("%1 is playing:"), Model_GetFriendNick(name));

		AppendMenuWrapper(hmenu, MF_STRING | MF_GRAYED, (UINT_PTR)NULL, txt);

		AppendGamesListHelper(hmenu, name, gamesplaying, ignoregame, &addedentry, 1);
	}

	if (gameswatching)
	{
		if (gamesplaying)
		{
			AppendMenuWrapper(hmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		}

		i18n_stringsub(txt, 256, _("%1 is watching:"), Model_GetFriendNick(name));

		AppendMenuWrapper(hmenu, MF_STRING | MF_GRAYED, (UINT_PTR)NULL, txt);

		AppendGamesListHelper(hmenu, name, gameswatching, ignoregame, &addedentry, 1);

		AppendMenuWrapper(hmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	}

	if (localgames)
	{
		if (gameswatching || gamesplaying)
		{
			AppendMenuWrapper(hmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		}

		i18n_stringsub(txt, 256, _("Invite %1 to watch:"), Model_GetFriendNick(name));

		AppendMenuWrapper(hmenu, MF_STRING | MF_GRAYED, (UINT_PTR)NULL, txt);

		AppendGamesListHelper(hmenu, name, localgames, ignoregame, &addedentry, 0);
	}

#if 0
	DestroyMenu(gamefriendsubhmenu);
	gamefriendsubhmenu = CreatePopupMenu();

	if (gamesplaying)
	{
		i18n_stringsub(txt, 256, _("%1 is playing:"), Model_GetFriendNick(name));

		AppendMenuWrapper(gamefriendsubhmenu, MF_STRING | MF_GRAYED, (UINT_PTR)NULL, txt);

		AppendGamesListHelper(gamefriendsubhmenu, name, gamesplaying, ignoregame, &addedentry, 1);
	}

	if (gameswatching)
	{
		if (gamesplaying)
		{
			AppendMenuWrapper(gamefriendsubhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		}

		i18n_stringsub(txt, 256, _("%1 is watching:"), Model_GetFriendNick(name));

		AppendMenuWrapper(gamefriendsubhmenu, MF_STRING | MF_GRAYED, (UINT_PTR)NULL, txt);

		AppendGamesListHelper(gamefriendsubhmenu, name, gameswatching, ignoregame, &addedentry, 1);

		AppendMenuWrapper(gamefriendsubhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	}

	if (localgames)
	{
		if (gameswatching || gamesplaying)
		{
			AppendMenuWrapper(gamefriendsubhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		}

		i18n_stringsub(txt, 256, _("Invite %1 to watch:"), Model_GetFriendNick(name));

		AppendMenuWrapper(gamefriendsubhmenu, MF_STRING | MF_GRAYED, (UINT_PTR)NULL, txt);

		AppendGamesListHelper(gamefriendsubhmenu, name, localgames, ignoregame, &addedentry, 0);
	}

	if (gamesplaying || gameswatching || localgames)
	{
		AppendMenuWrapper(hmenu, MF_POPUP, (UINT)gamefriendsubhmenu, _("Active Games"));
	}
#endif
}

void Menu_GetStatusLists(struct StatusList_s **outawaystatuslist, struct StatusList_s **outavailstatuslist)
{
	*outawaystatuslist = awaystatuslist;
	*outavailstatuslist = availstatuslist;
}

void Menu_PopupPresenceMenu(struct Box_s *pbox)
{
	int x, y;

	Box_GetScreenCoords(pbox, &x, &y);
	x += pbox->w / 2;
	y += pbox->h / 2;
/*
	SetForegroundWindow(pbox->hwnd);
	TrackPopupMenuEx(presencehmenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
	*/
	Menu_RedoStatusMenu2(pbox, x, y);
}


void Menu_PopupPrefsMenu(struct Box_s *pbox)
{
	int x, y;

	Box_GetScreenCoords(pbox, &x, &y);
	x += pbox->w / 2;
	y += pbox->h / 2;

	SetForegroundWindow(pbox->hwnd);
	TrackPopupMenuEx(prefshmenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}


void Menu_PopupAddMenu(struct Box_s *pbox)
{
	int x, y;

	cmdtarget = NULL;

	if (Model_IsInHiddenGroup())
	{
                ModifyMenuWrapper(addhmenu, 4, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_JOINCHATROOM, _("Join Chat Room..."));
	}
	else
	{
		ModifyMenuWrapper(addhmenu, 4, MF_BYPOSITION | MF_STRING, MENU_JOINCHATROOM, _("Join Chat Room..."));
	}

	Box_GetScreenCoords(pbox, &x, &y);
	x += pbox->w / 2;
	y += pbox->h / 2;

	SetForegroundWindow(pbox->hwnd);
	TrackPopupMenuEx(addhmenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}


void Menu_PopupListMenu(struct Box_s *pbox, char *name, char *group,
	int x, int y, int subscribedTo, int online, int chessparkfeatures,
	struct namedlist_s *gamesplaying, struct namedlist_s *gameswatching,
	struct adhoccommand_s *command)
{
	struct namedlist_s *localgames = View_GetOpenGames();
	int chessparklogin = Model_GetChessparkLogin();

	cmdtarget = name;
	cmdgroup = group;
	cmdlist1 = gamesplaying;
	cmdlist2 = gameswatching;
	cmdlist3 = localgames;

	if (command)
	{
		cmdadhoc = command->command;
	}

	SetForegroundWindow(pbox->hwnd);

	if (subscribedTo) 
	{
		DestroyMenu(friendhmenu);

		friendhmenu = CreatePopupMenu();
		AppendMenuWrapper (friendhmenu, MF_STRING, MENU_MESSAGEFRIEND, _("Send Message..."));

		if (online)
		{
			if (chessparkfeatures)
			{
				int addedentry = 0;

				if (command)
				{
					AppendMenuWrapper(friendhmenu, MF_STRING | (chessparklogin ? 0 : MF_GRAYED), MENU_FRIENDADHOC, command->name);
				}
				else
				{
					AppendMenuWrapper (friendhmenu, MF_STRING | (chessparklogin ? 0 : MF_GRAYED), MENU_GAMEFRIEND, _("Invite to Game..."));
				}

				AppendGamesList(friendhmenu, name, gamesplaying, gameswatching, localgames, NULL);
			}
			else
			{

				if (command)
				{
					AppendMenuWrapper(friendhmenu, MF_STRING | (chessparklogin ? 0 : MF_GRAYED), MENU_FRIENDADHOC, command->name);
				}
				else
				{
					AppendMenuWrapper (friendhmenu, MF_STRING | (chessparklogin ? 0 : MF_GRAYED), MENU_INVITEFRIENDTOCHESSPARK, _("Invite to Chesspark..."));
				}
			}
		}
		else
		{
#ifdef CHESSPARK_CORRESPONDENCE
			AppendMenuWrapper(friendhmenu, MF_STRING | (chessparklogin ? 0 : MF_GRAYED), MENU_GAMEFRIENDCORR, _("Invite to Correspondence Game..."));
#else
			AppendMenuWrapper(friendhmenu, MF_STRING | MF_GRAYED, MENU_GAMEFRIENDCORR, _("Invite to Game..."));
#endif
		}

		AppendMenuWrapper (friendhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);

		AppendMenuWrapper (friendhmenu, MF_STRING, MENU_REMOVEFRIEND, _("Remove Friend"));

		if (Model_IsJIDIgnored(name))
		{
			AppendMenuWrapper (friendhmenu, MF_STRING, MENU_UNIGNOREFRIEND, _("Remove from Ignore List"));
		}
		else
		{
			AppendMenuWrapper (friendhmenu, MF_STRING, MENU_IGNOREFRIEND, _("Add to Ignore List"));
		}
		AppendMenuWrapper (friendhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);

		AppendMenuWrapper (friendhmenu, MF_STRING, MENU_INFOFRIEND, _("View Info"));
		AppendMenuWrapper (friendhmenu, MF_STRING | MF_GRAYED, MENU_LOGFRIEND, _("View Logs"));

		TrackPopupMenuEx(friendhmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	}
	else
	{
		TrackPopupMenuEx(pendingfriendhmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	}

	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}


void Menu_PopupChatboxPrefsMenu(struct Box_s *pbox, char *name, int drawervisible, int isGroupChat, int mucpower, char *oldtopic, int x, int y)
{
	cmdbox = Box_GetRoot(pbox);
	cmdtarget = name;
	cmdtargetmuc = name;
	cmdtopic = oldtopic;

	DestroyMenu(chatboxprefshmenu);
	chatboxprefshmenu = CreatePopupMenu();
	AppendMenuWrapper (chatboxprefshmenu, MF_STRING | (drawervisible ? MF_CHECKED : MF_UNCHECKED), MENU_SHOWCHATPARTICIPANTS, _("Show Participants"));
	AppendMenuWrapper (chatboxprefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_INVITETOTHISCHAT, _("Invite Someone to this Chat..."));
	AppendMenuWrapper (chatboxprefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	if (isGroupChat)
	{
		if (mucpower)
		{
			AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_CHANGECHATTOPIC, _("Change Chat Topic..."));
			AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_CHATOPTIONS, _("Ban List..."));
			AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_CLEARCHATHISTORY, _("Clear chat history"));
			AppendMenuWrapper (chatboxprefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		}

		if (!Ctrl_IsChatInFavorites(name))
		{
			AppendMenuWrapper(chatboxprefshmenu, MF_STRING, MENU_ADDCHATTOFAVORITES, _("Add to Favorites"));
			AppendMenuWrapper(chatboxprefshmenu, MF_STRING | MF_GRAYED, MENU_ADDCHATTOAUTOJOIN, _("Join Chat on Login"));
		}
		else
		{
			if (Model_IsHiddenGroupChat(name))
			{
				AppendMenuWrapper(chatboxprefshmenu, MF_STRING | MF_GRAYED, MENU_REMOVECHATFROMFAVORITES, _("Remove from Favorites"));
			}
			else
			{
				AppendMenuWrapper(chatboxprefshmenu, MF_STRING, MENU_REMOVECHATFROMFAVORITES, _("Remove from Favorites"));
			}
			if (!Ctrl_IsChatInAutojoin(name))
			{
				AppendMenuWrapper(chatboxprefshmenu, MF_STRING | MF_UNCHECKED, MENU_ADDCHATTOAUTOJOIN, _("Join Chat on Login"));
			}
			else
			{
				AppendMenuWrapper(chatboxprefshmenu, MF_STRING | MF_CHECKED, MENU_REMOVECHATFROMAUTOJOIN, _("Join Chat on Login"));
			}
		}
	}
	else
	{
		if (!Model_JidInRoster(name))
		{
			AppendMenuWrapper(chatboxprefshmenu, MF_STRING, MENU_ADDFRIEND,  _("Add Friend..."));
		}
		else
		{
			AppendMenuWrapper(chatboxprefshmenu, MF_STRING, MENU_REMOVEFRIEND,  _("Remove Friend..."));
		}
		AppendMenuWrapper(chatboxprefshmenu, MF_STRING,    MENU_INFOFRIEND, _("View Info..."));
		AppendMenuWrapper(chatboxprefshmenu, MF_SEPARATOR, (UINT_PTR)NULL,  NULL);
		if (Model_IsJIDIgnored(name))
		{
			AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_UNIGNOREFRIEND, _("Remove from Ignore List"));
		}
		else
		{
			AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_IGNOREFRIEND, _("Add to Ignore List"));
		}
		AppendMenuWrapper(chatboxprefshmenu, MF_SEPARATOR, (UINT_PTR)NULL,  NULL);
		AppendMenuWrapper(chatboxprefshmenu, MF_STRING, MENU_GAMEFRIEND, _("Invite to Game..."));
	}

	SetForegroundWindow(pbox->hwnd);

	TrackPopupMenuEx(chatboxprefshmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_PopupRosterBlankMenu(struct Box_s *pbox, int x, int y)
{
	cmdtarget = NULL;

	SetForegroundWindow(pbox->hwnd);
	
	TrackPopupMenuEx(friendstabblankhmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_PopupGroupMenu(struct Box_s *pbox, char *name, int x, int y)
{
	cmdtarget = name;

	SetForegroundWindow(pbox->hwnd);
	if (name)
	{
		ModifyMenuWrapper(grouphmenu, 0, MF_BYPOSITION | MF_STRING, MENU_RENAMEGROUP, _("Rename Group..."));
		ModifyMenuWrapper(grouphmenu, 2, MF_BYPOSITION | MF_STRING, MENU_REMOVEGROUP, _("Remove Group"));
	}
	else
	{
		ModifyMenuWrapper(grouphmenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_RENAMEGROUP, _("Rename Group..."));
		ModifyMenuWrapper(grouphmenu, 2, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_REMOVEGROUP, _("Remove Group"));
	}
	TrackPopupMenuEx(grouphmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_SetModerationSubMenu(char *role, char *affiliation, int hasmucpower)
{
	DestroyMenu(moderationsubhmenu);
	moderationsubhmenu = CreatePopupMenu();

	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_GIVEVOICE, _("Give Voice Privilege"));
	AppendMenuWrapper (moderationsubhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_KICK, _("Kick from Room"));
	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_BAN, _("Ban from Room"));
	AppendMenuWrapper (moderationsubhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_GIVEMOD, _("Give Moderator Status"));
	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_GIVEADMIN, _("Give Admin Status"));
	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_GIVEOWNER, _("Give Owner Status"));

	if (role && strcmp(role, "visitor") == 0)
	{
		ModifyMenuWrapper(moderationsubhmenu, 0, MF_BYPOSITION | MF_STRING, MENU_GIVEVOICE, _("Give Voice Privilege"));
		ModifyMenuWrapper(moderationsubhmenu, 5, MF_BYPOSITION | MF_STRING, MENU_GIVEMOD, _("Give Moderator Status"));
	}
	else if (role && strcmp(role, "participant") == 0)
	{
		ModifyMenuWrapper(moderationsubhmenu, 0, MF_BYPOSITION | MF_STRING, MENU_REVOKEVOICE, _("Revoke Voice Privilege"));
		ModifyMenuWrapper(moderationsubhmenu, 5, MF_BYPOSITION | MF_STRING, MENU_GIVEMOD, _("Give Moderator Status"));
	}
	else if (role && strcmp(role, "moderator") == 0)
	{
		ModifyMenuWrapper(moderationsubhmenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_GIVEVOICE, _("Cannot revoke voice from moderator"));
		ModifyMenuWrapper(moderationsubhmenu, 5, MF_BYPOSITION | MF_STRING, MENU_REVOKEMOD, _("Revoke Moderator Status"));
	}

	if (affiliation && strcmp(affiliation, "none") == 0)
	{
		ModifyMenuWrapper(moderationsubhmenu, 6, MF_BYPOSITION | MF_STRING, MENU_GIVEADMIN, _("Give Admin Status"));
		ModifyMenuWrapper(moderationsubhmenu, 7, MF_BYPOSITION | MF_STRING, MENU_GIVEOWNER, _("Give Owner Status"));
	}
	else if (affiliation && strcmp(affiliation, "admin") == 0)
	{
		ModifyMenuWrapper(moderationsubhmenu, 6, MF_BYPOSITION | MF_STRING, MENU_REVOKEADMIN, _("Revoke Admin Status"));
		ModifyMenuWrapper(moderationsubhmenu, 7, MF_BYPOSITION | MF_STRING, MENU_GIVEOWNER, _("Give Owner Status"));
	}
	else if (affiliation && strcmp(affiliation, "owner") == 0)
	{
		ModifyMenuWrapper(moderationsubhmenu, 6, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_GIVEADMIN, _("Cannot revoke admin from owner"));
		ModifyMenuWrapper(moderationsubhmenu, 7, MF_BYPOSITION | MF_STRING, MENU_REVOKEOWNER, _("Revoke Owner Status"));
	}

	if (!hasmucpower)
	{
		ModifyMenuWrapper(moderationsubhmenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_GIVEVOICE, _("Cannot revoke voice"));
		ModifyMenuWrapper(moderationsubhmenu, 2, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_GIVEVOICE, _("Cannot kick"));
		ModifyMenuWrapper(moderationsubhmenu, 3, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_GIVEVOICE, _("Cannot ban"));
		ModifyMenuWrapper(moderationsubhmenu, 5, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_REVOKEMOD, _("Cannot revoke moderator"));
		ModifyMenuWrapper(moderationsubhmenu, 6, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_GIVEADMIN, _("Cannot revoke admin"));
		ModifyMenuWrapper(moderationsubhmenu, 7, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_REVOKEOWNER, _("Cannot revoke owner"));
	}
}

void Menu_PopupChatParticipantMenu(struct Box_s *pbox, char *jid, char *chatjid, char *nick, char *role, char *affiliation, int showmod, int showmessage, int x, int y)
{
	char *ignoregame = NULL;
	int hasmucpower = View_LocalUserHasMucPower(chatjid);
	int chessparklogin = Model_GetChessparkLogin();

	cmdtarget = jid;
	cmdtargetmuc = chatjid;
	cmdtargetnick = nick;
	cmdlist1 = NULL;
	cmdlist2 = NULL;
	cmdlist3 = View_GetOpenGames();

	if (cmdtargetmuc && strstr(cmdtargetmuc, "@games.chesspark.com"))
	{
		ignoregame = Jid_GetBeforeAt(cmdtargetmuc);
	}

	DestroyMenu(chatparticipanthmenu);
	chatparticipanthmenu = CreatePopupMenu();

	if (showmessage)
	{
		AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_MESSAGEFRIEND, _("Send Message..."));
	}

	if (role && stricmp(role, "player") == 0)
	{
		AppendMenuWrapper (chatparticipanthmenu, MF_STRING | MF_GRAYED, MENU_GAMEFRIEND, _("Invite to Game..."));
	}
	else
	{
		AppendMenuWrapper (chatparticipanthmenu, MF_STRING | (chessparklogin ? 0 : MF_GRAYED), MENU_GAMEFRIEND, _("Invite to Game..."));
	}
	AppendGamesList(chatparticipanthmenu, jid, NULL, NULL, View_GetOpenGames(), ignoregame);

	/*if (ismuc)*/
	{
		AppendMenuWrapper (chatparticipanthmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
		if (!Model_JidInRoster(jid))
		{
			AppendMenuWrapper(chatparticipanthmenu, MF_STRING, MENU_ADDFRIEND,  _("Add Friend..."));
		}
		else
		{
			AppendMenuWrapper(chatparticipanthmenu, MF_STRING, MENU_REMOVEFRIEND,  _("Remove Friend..."));
		}
	}

	AppendMenuWrapper (chatparticipanthmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);

	if (Model_IsJIDIgnored(jid))
	{
		AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_UNIGNOREFRIEND, _("Remove from Ignore List"));
	}
	else
	{
		AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_IGNOREFRIEND, _("Add to Ignore List"));
	}

	if (showmod)
	{
		AppendMenuWrapper (chatparticipanthmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);

		Menu_SetModerationSubMenu(role, affiliation, hasmucpower);

		AppendMenuWrapper (chatparticipanthmenu, MF_POPUP, (UINT_PTR)moderationsubhmenu, _("Moderation"));
	}

	AppendMenuWrapper (chatparticipanthmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_INFOFRIEND, _("View Info..."));

	SetForegroundWindow(pbox->hwnd);
	
	TrackPopupMenuEx(chatparticipanthmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_PopupChatboxAddMenu(struct Box_s *pbox, char *name, int x, int y)
{
	cmdtarget = name;

	SetForegroundWindow(pbox->hwnd);
	
	TrackPopupMenuEx(chataddhmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_PopupRoomsBlankMenu(struct Box_s *pbox, int x, int y)
{
	SetForegroundWindow(pbox->hwnd);
	
	TrackPopupMenuEx(roomstabblankhmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_PopupRoomMenu(struct Box_s *pbox, char *jid, int x, int y)
{
	cmdtarget = jid;
	cmdtargetmuc = jid;

	if (!Ctrl_IsChatInFavorites(jid))
	{
		ModifyMenuWrapper(roomhmenu, 1, MF_BYPOSITION | MF_STRING, MENU_ADDCHATTOFAVORITES, _("Add to Favorites"));
		ModifyMenuWrapper(roomhmenu, 2, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_ADDCHATTOAUTOJOIN, _("Join Chat on Login"));
		CheckMenuItem(roomhmenu, 2, MF_BYPOSITION | MF_UNCHECKED);
	}
	else
	{
		if (Model_IsHiddenGroupChat(jid))
		{
			ModifyMenuWrapper(roomhmenu, 1, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_REMOVECHATFROMFAVORITES, _("Remove from Favorites"));
		}
		else
		{
			ModifyMenuWrapper(roomhmenu, 1, MF_BYPOSITION | MF_STRING, MENU_REMOVECHATFROMFAVORITES, _("Remove from Favorites"));
		}
		
		if (!Ctrl_IsChatInAutojoin(jid))
		{
			ModifyMenuWrapper(roomhmenu, 2, MF_BYPOSITION | MF_STRING, MENU_ADDCHATTOAUTOJOIN, _("Join Chat on Login"));
			CheckMenuItem(roomhmenu, 2, MF_BYPOSITION | MF_UNCHECKED);
		}
		else
		{
			ModifyMenuWrapper(roomhmenu, 2, MF_BYPOSITION | MF_STRING, MENU_REMOVECHATFROMAUTOJOIN, _("Join Chat on Login"));
			CheckMenuItem(roomhmenu, 2, MF_BYPOSITION | MF_CHECKED);
		}
	}

	SetForegroundWindow(pbox->hwnd);
	
	TrackPopupMenuEx(roomhmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_PopupGameMenu(struct Box_s *pbox, char *gameid, char *gamemuc, int gamelocal, int gameover, int x, int y)
{
	cmdbox = pbox;
	cmdtarget = gameid;
	cmdtargetmuc = gamemuc;

	SetForegroundWindow(pbox->hwnd);

		/*
	AppendMenuWrapper (gamehmenu, MF_STRING | MF_GRAYED, MENU_SAVEGAME, "Save Game");
	AppendMenuWrapper (gamehmenu, MF_STRING | MF_GRAYED, MENU_SAVEGAMEAS, "Save Game As...");
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_REQUESTADJOURN, "Request Adjournment");
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_FLAGOPPTIME, "Flag Opponent's Time");
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_REQUESTDRAW, "Request a Draw");
	AppendMenuWrapper (gamehmenu, MF_STRING | MF_GRAYED, MENU_REQUESTABORT, "Request Game Abort");
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_RESIGNGAME, "Resign Game");
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING | MF_GRAYED, MENU_ADDOPPTIME, "Add More Time to Opponent's Clock...");
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_CASTLEKINGSIDE, _("Quick Castle King's Side"));
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_CASTLEQUEENSIDE, _("Quick Castle King's Side"));
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING | MF_GRAYED, MENU_REQUESTTAKEBACK, _("Request a Move Take Back"));
*/
	if (gamelocal && !gameover)
	{
		ModifyMenuWrapper(gamehmenu, 3,  MF_BYPOSITION | MF_STRING, MENU_REQUESTADJOURN,  _("Request Adjournment"));
		ModifyMenuWrapper(gamehmenu, 5,  MF_BYPOSITION | MF_STRING, MENU_FLAGOPPTIME,     _("Flag Opponent's Time"));
		ModifyMenuWrapper(gamehmenu, 7,  MF_BYPOSITION | MF_STRING, MENU_REQUESTDRAW,     _("Request a Draw"));
		ModifyMenuWrapper(gamehmenu, 8,  MF_BYPOSITION | MF_STRING, MENU_REQUESTABORT,     _("Request Game Abort"));
		ModifyMenuWrapper(gamehmenu, 9,  MF_BYPOSITION | MF_STRING, MENU_RESIGNGAME,      _("Resign Game"));
		ModifyMenuWrapper(gamehmenu, 11, MF_BYPOSITION | MF_STRING, MENU_ADDOPPTIME,      _("Add More Time to Opponent's Clock..."));
		ModifyMenuWrapper(gamehmenu, 13, MF_BYPOSITION | MF_STRING, MENU_CASTLEKINGSIDE,  _("Quick Castle King's Side"));
		ModifyMenuWrapper(gamehmenu, 14, MF_BYPOSITION | MF_STRING, MENU_CASTLEQUEENSIDE, _("Quick Castle Queen's Side"));
		ModifyMenuWrapper(gamehmenu, 16, MF_BYPOSITION | MF_STRING, MENU_REQUESTTAKEBACK, _("Request a Move Take Back"));
	}
	else
	{
		ModifyMenuWrapper(gamehmenu, 3,  MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_REQUESTADJOURN,  _("Request Adjournment"));
		ModifyMenuWrapper(gamehmenu, 5,  MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_FLAGOPPTIME,     _("Flag Opponent's Time"));
		ModifyMenuWrapper(gamehmenu, 7,  MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_REQUESTDRAW,     _("Request a Draw"));
		ModifyMenuWrapper(gamehmenu, 8,  MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_REQUESTABORT,     _("Request Game Abort"));
		ModifyMenuWrapper(gamehmenu, 9,  MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_RESIGNGAME,      _("Resign Game"));
		ModifyMenuWrapper(gamehmenu, 11, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_ADDOPPTIME,      _("Add More Time to Opponent's Clock..."));
		ModifyMenuWrapper(gamehmenu, 13, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_CASTLEKINGSIDE,  _("Quick Castle King's Side"));
		ModifyMenuWrapper(gamehmenu, 14, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_CASTLEQUEENSIDE, _("Quick Castle Queen's Side"));
		ModifyMenuWrapper(gamehmenu, 16, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_REQUESTTAKEBACK, _("Request a Move Take Back"));
	}

	if (!gamemuc && !gameover)
	{
		ModifyMenuWrapper(gamehmenu, 3, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_REQUESTADJOURN, _("Request Adjournment"));
	}
	
	TrackPopupMenuEx(gamehmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_PopupEditMenu(struct Box_s *pbox, int x, int y, int canedit, int cancopy, int canselect)
{
	edittarget = pbox;
	SetForegroundWindow(pbox->hwnd);

	DestroyMenu(edithmenu);
	edithmenu = CreatePopupMenu();

	if (canedit)
	{
		AppendMenuWrapper (edithmenu, MF_STRING | (cancopy ? 0 : MF_GRAYED), MENU_CUT, _("Cut"));
		AppendMenuWrapper (edithmenu, MF_STRING | (cancopy ? 0 : MF_GRAYED), MENU_COPY, _("Copy"));
		AppendMenuWrapper (edithmenu, MF_STRING, MENU_PASTE, _("Paste"));
		AppendMenuWrapper (edithmenu, MF_STRING | (cancopy ? 0 : MF_GRAYED), MENU_DELETE, _("Delete"));
	}
	else
	{
		AppendMenuWrapper (edithmenu, MF_STRING | (cancopy ? 0 : MF_GRAYED), MENU_COPY, _("Copy"));
		/*AppendMenuWrapper (edithmenu, MF_STRING | (canselect ? 0 : MF_GRAYED), MENU_COPY, _("Select All"));*/
	}	

	TrackPopupMenuEx(edithmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_PopupTourneyMenu(struct Box_s *pbox, char *tourneyid, int x, int y, int isplayer, int ismanager, int tourneystarted)
{
	cmdtarget = tourneyid;
	SetForegroundWindow(pbox->hwnd);

	/*
	tourneyhmenu = CreatePopupMenu();
	AppendMenuWrapper (tourneyhmenu, MF_STRING, MENU_JOINTOURNEY, "Join Tournament");
	/*AppendMenuWrapper (tourneyhmenu, MF_STRING, MENU_FORFEITTOURNEY, "Forfeit Tournament");*
	AppendMenuWrapper (tourneyhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (tourneyhmenu, MF_STRING, MENU_SETTOURNEYROUNDSTART, "Set Tournament Round Start Time");
	AppendMenuWrapper (tourneyhmenu, MF_STRING, MENU_STARTNEXTTOURNEYROUND, "Start Next Round");
	*/

	if (isplayer)
	{
		if (tourneystarted)
		{
			ModifyMenuWrapper(tourneyhmenu, 0, MF_BYPOSITION | MF_STRING, MENU_FORFEITTOURNEY, _("Forfeit Tournament")); 
		}
		else
		{
			ModifyMenuWrapper(tourneyhmenu, 0, MF_BYPOSITION | MF_STRING, MENU_FORFEITTOURNEY, _("Leave Tournament")); 
		}
	}
	else
	{
		ModifyMenuWrapper(tourneyhmenu, 0, MF_BYPOSITION | MF_STRING, MENU_JOINTOURNEY, _("Join Tournament"));
	}

	if (ismanager)
	{
		ModifyMenuWrapper(tourneyhmenu, 2, MF_BYPOSITION | MF_STRING, MENU_SETTOURNEYROUNDSTART, _("Set Tournament Round Start Time"));
		ModifyMenuWrapper(tourneyhmenu, 3, MF_BYPOSITION | MF_STRING, MENU_STARTNEXTTOURNEYROUND, _("Start Next Round"));
	}
	else
	{
		ModifyMenuWrapper(tourneyhmenu, 2, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_SETTOURNEYROUNDSTART, _("Set Tournament Round Start Time"));
		ModifyMenuWrapper(tourneyhmenu, 3, MF_BYPOSITION | MF_STRING | MF_GRAYED, MENU_STARTNEXTTOURNEYROUND, _("Start Next Round"));
	}

	TrackPopupMenuEx(tourneyhmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);

	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_PopupProfileMenu(struct Box_s *pbox, char *name, int x, int y, struct namedlist_s *gamesplaying, struct namedlist_s *gameswatching)
{
	struct namedlist_s *localgames = View_GetOpenGames();

	cmdbox = Box_GetRoot(pbox);
	cmdtarget = name;
	cmdlist1 = gamesplaying;
	cmdlist2 = gameswatching;
	cmdlist3 = localgames;

	DestroyMenu(profilehmenu);
	profilehmenu = CreatePopupMenu();
	{
		AppendMenuWrapper(profilehmenu, MF_STRING, MENU_MESSAGEFRIEND, _("Open Chat..."));
		AppendMenuWrapper(profilehmenu, MF_STRING, MENU_GAMEFRIEND, _("Invite to Game..."));
		AppendGamesList(profilehmenu, name, gamesplaying, gameswatching, localgames, NULL);

		AppendMenuWrapper(profilehmenu, MF_SEPARATOR, (UINT_PTR)NULL,  NULL);
		if (Model_IsJIDIgnored(name))
		{
			AppendMenuWrapper (profilehmenu, MF_STRING, MENU_UNIGNOREFRIEND, _("Remove from Ignore List"));
		}
		else
		{
			AppendMenuWrapper (profilehmenu, MF_STRING, MENU_IGNOREFRIEND, _("Add to Ignore List"));
		}
		AppendMenuWrapper(profilehmenu, MF_SEPARATOR, (UINT_PTR)NULL,  NULL);
		if (!Model_JidInRoster(name))
		{
			AppendMenuWrapper(profilehmenu, MF_STRING, MENU_ADDFRIEND,  _("Add Friend..."));
		}
		else
		{
			AppendMenuWrapper(profilehmenu, MF_STRING, MENU_REMOVEFRIEND,  _("Remove Friend..."));
		}
	}

	SetForegroundWindow(pbox->hwnd);

	TrackPopupMenuEx(profilehmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_PopupURLMenu(struct Box_s *pbox, char *url, int x, int y)
{
	cmdtarget = url;
	SetForegroundWindow(pbox->hwnd);

	TrackPopupMenuEx(urlhmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);

	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}


void Popup_DestroyAll(struct Box_s *pbox)
{
	if (pbox->hwnd)
	{
		HWND old;
		/*
		Log_Write(0, "animate %d %d\n", pbox->hwnd, AnimateWindow(pbox->hwnd, 200, AW_BLEND | AW_HIDE));
		Log_Write(0, "error %d\n", GetLastError());
		*/
		old = pbox->hwnd;
		pbox->hwnd = NULL;
		DestroyWindow(old);
		Box_Destroy(pbox);
	}
}

struct BoxImage_s **avatarimages;
int avatarsinit = 0;

void Menu_InitAvatarList()
{
	int i;
	char savepath[MAX_PATH];

	Util_GetPrivateSavePath(savepath, MAX_PATH);
	strcat(savepath, "/avatars");
	mkdir(savepath);

	if (avatarsinit == 0)
	{
		avatarimages = malloc(sizeof(*avatarimages) * 10);
	}

	for (i = 0; i < 10; i++)
	{
		char filename[MAX_PATH];

		sprintf(filename, "%s/%02d", savepath, i);

		if (avatarsinit)
		{
			BoxImage_Destroy(avatarimages[i]);
		}

		avatarimages[i] = BoxImage_LoadImageNoExtension(filename);
	}

	avatarsinit = 1;
}

void Menu_LoadNewAvatar(struct Box_s *pbox)
{
	OPENFILENAME ofn;
	char filename[MAX_PATH];
	char olddirectory[MAX_PATH];
	char scaledname[MAX_PATH];
	char fromfile[MAX_PATH];
	char tofile[MAX_PATH];
	char savepath[MAX_PATH];
	int i, w, h;

	struct BoxImage_s *parentimage, *image;

	Popup_DestroyAll(Box_GetRoot(pbox));

	filename[0] = 0;

	GetCurrentDirectory(MAX_PATH - 1, olddirectory);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = pbox->hwnd;
	ofn.lpstrFilter = "Image Files (*.jpg;*.gif;*.png)\0*.jpg;*.gif;*.png\0\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 511;
	ofn.lpstrTitle = _("New Image...");
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if (GetOpenFileName(&ofn) == 0)
	{
		return;
	}

	SetCurrentDirectory(olddirectory);

	parentimage = ImageMgr_GetRootImage(filename);

	if (!parentimage)
	{
		return;
	}

	if (parentimage->w > 300 || parentimage->h > 300)
	{
		if (parentimage->w > parentimage->h)
		{
			h = parentimage->h * 300 / parentimage->w;
			w = 300;
		}
		else
		{
			w = parentimage->w * 300 / parentimage->h;
			h = 300;
		}

		sprintf(scaledname, "%dx%d-%s", w, h, filename);

		image = ImageMgr_GetRootScaledImage(scaledname, filename, w, h);
	}
	else
	{
		image = parentimage;
	}

	Util_GetPrivateSavePath(savepath, MAX_PATH);
	strncpy(fromfile, savepath, MAX_PATH);
	strncat(fromfile, "/avatars/09.png", MAX_PATH);
	DeleteFile(fromfile);
	strncpy(fromfile, savepath, MAX_PATH);
	strncat(fromfile, "/avatars/09.jpg", MAX_PATH);
	DeleteFile(fromfile);

	for (i = 8; i >= 0; i--)
	{
		char from[MAX_PATH], to[MAX_PATH];
		
		sprintf(from, "%02d.png", i);
		strncpy(fromfile, savepath, MAX_PATH);
		strncat(fromfile, "/avatars/", MAX_PATH);
		strncat(fromfile, from, MAX_PATH);

		sprintf(to, "%02d.png", i+1);
		strncpy(tofile, savepath, MAX_PATH);
		strncat(tofile, "/avatars/", MAX_PATH);
		strncat(tofile, to, MAX_PATH);

		MoveFile(fromfile, tofile);

		sprintf(from, "%02d.jpg", i);
		strncpy(fromfile, savepath, MAX_PATH);
		strncat(fromfile, "/avatars/", MAX_PATH);
		strncat(fromfile, from, MAX_PATH);

		sprintf(to, "%02d.jpg", i+1);
		strncpy(tofile, savepath, MAX_PATH);
		strncat(tofile, "/avatars/", MAX_PATH);
		strncat(tofile, to, MAX_PATH);

		MoveFile(fromfile, tofile);
	}

	if (image->hbmpmask)
	{
		strncpy(tofile, savepath, MAX_PATH);
		strncat(tofile, "/avatars/00.png", MAX_PATH);
                BoxImage_SavePNG(image, tofile);
	}
	else
	{
		strncpy(tofile, savepath, MAX_PATH);
		strncat(tofile, "/avatars/00.jpg", MAX_PATH);
                BoxImage_SaveJPG(image, tofile);
	}

	Menu_InitAvatarList();

	/*View_SetAvatar(image);*/

	Model_SetAvatar(tofile);
}

void Menu_ClearAvatarMenu(struct Box_s *pbox)
{
	int i;
	char savepath[MAX_PATH];

	Util_GetPrivateSavePath(savepath, MAX_PATH);

	Popup_DestroyAll(Box_GetRoot(pbox));

	/*View_SetAvatar(NULL);*/

	for (i = 0; i < 10; i++)
	{
		char filename[MAX_PATH];
		sprintf(filename, "%s/avatars/%02d.png", savepath, i);
		DeleteFile(filename);
		sprintf(filename, "%s/avatars/%02d.jpg", savepath, i);
		DeleteFile(filename);
	}

	Menu_InitAvatarList();
}

void Menu_SetAvatarToBoxImg(struct Box_s *pbox)
{
	char filename[80];
	struct Box_s *child = pbox->parent->child;
	int i;
	char savepath[MAX_PATH];

	Util_GetPrivateSavePath(savepath, MAX_PATH);

	strcpy(filename, savepath);
	strcat(filename, "/avatars/");

	for (i=0; i<10; i++)
	{
		if (child == pbox)
		{
			char num[4];
			sprintf(num, "%02d", i);
			strcat(filename, num);
		}
		child = child->sibling;
	}
	Model_SetAvatarNoExtension(filename);

	Popup_DestroyAll(Box_GetRoot(pbox));
}


void Menu_PopupAvatarMenu(struct Box_s *pbox)
{
	struct Box_s *popup, *subbox;
	int x, y, i, j;

	Box_GetScreenCoords(pbox, &x, &y);
	x += pbox->w / 2;
	y += pbox->h / 2;

	popup = Box_Create(x - 180, y, 180, 115, BOX_VISIBLE);
	popup->bgcol = TabBG2;
	popup->OnInactive = Popup_DestroyAll;

	if (!avatarsinit)
	{
		Menu_InitAvatarList();
	}

	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 5; j++)
		{
			if (avatarimages[j + i * 5])
			{
				subbox = Button_Create(j * 35 + 5, i * 35 + 5, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERIMG);
				subbox->img = avatarimages[j + i * 5];
			}
			else
			{
				subbox = Button_Create(j * 35 + 5, i * 35 + 5, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_BORDER | BOX_CENTERIMG);
			}
			Button_SetOnButtonHit(subbox, Menu_SetAvatarToBoxImg);
			Box_AddChild(popup, subbox);
		}
	}

	subbox = Button_Create(5, 75, 170, 20, BOX_VISIBLE);
	subbox->bgcol = TabBG2;
	Button_SetNormalBG(subbox, TabBG2);
	Button_SetNormalFG(subbox, RGB(0, 0, 0));
	Button_SetHoverBG(subbox, RGB(0, 0, 128));
	Button_SetHoverFG(subbox, RGB(255, 255, 255));
	Button_SetPressedBG(subbox, RGB(0, 0, 128));
	Button_SetPressedFG(subbox, RGB(255, 255, 255));
	Button_SetOnButtonHit(subbox, Menu_LoadNewAvatar);
	Box_SetText(subbox, _("New Image..."));
	Box_AddChild(popup, subbox);

	subbox = Button_Create(5, 95, 170, 20, BOX_VISIBLE);
	subbox->bgcol = TabBG2;
	Button_SetNormalBG(subbox, TabBG2);
	Button_SetNormalFG(subbox, RGB(0, 0, 0));
	Button_SetHoverBG(subbox, RGB(0, 0, 128));
	Button_SetHoverFG(subbox, RGB(255, 255, 255));
	Button_SetPressedBG(subbox, RGB(0, 0, 128));
	Button_SetPressedFG(subbox, RGB(255, 255, 255));
	Button_SetOnButtonHit(subbox, Menu_ClearAvatarMenu);
	Box_SetText(subbox, _("Clear All Images"));
	Box_AddChild(popup, subbox);

	Box_CreateWndMenu(popup, pbox->hwnd);
	SetForegroundWindow(popup->hwnd);
	BringWindowToTop(popup->hwnd);
	popup->active = TRUE;
}

void Menu_OnCommand(struct Box_s *pbox, int command)
{

	if (command < MENU_STATUS_AWAY && command > MENU_STATUS_AVAILABLE)
	{
		struct StatusList_s *pstatus = StatusList_GetNum(availstatuslist, command - MENU_STATUS_AVAILABLE);
		Ctrl_SetPresence(pstatus->status, pstatus->statusmsg);
		return;
	}
	else if (command > MENU_STATUS_AWAY && command < MENU_STATUS_OFFLINE)
	{
		struct StatusList_s *pstatus = StatusList_GetNum(awaystatuslist, command - MENU_STATUS_AWAY);

		Ctrl_SetPresence(pstatus->status, pstatus->statusmsg);
		return;
	}

	if (command >= MENU_OBSERVEFRIEND && command <= MENU_LASTOBSERVEFRIEND)
	{
		struct namedlist_s *currententry = cmdlist1;

		Log_Write(0, "command is %d\n", command - MENU_OBSERVEFRIEND);

		while (currententry && command != MENU_OBSERVEFRIEND)
		{
			currententry = currententry->next;
			command--;
		}

		if (!currententry)
		{
			currententry = cmdlist2;
		}

		while (currententry && command != MENU_OBSERVEFRIEND)
		{
			currententry = currententry->next;
			command--;
		}

		if (currententry)
		{
			struct gamesearchinfo_s *info = currententry->data;
			char *barejid1 = Jid_Strip(info->black->jid);
			char *barejid2 = Jid_Strip(cmdtarget);
			int rotate;

			rotate = (stricmp(barejid1, barejid2) == 0);

			free(barejid1);
			free(barejid2);

			Ctrl_WatchGame(currententry->name, rotate);
			return;
		}
		else
		{
			currententry = cmdlist3;
		}

		while (currententry && command != MENU_OBSERVEFRIEND)
		{
			currententry = currententry->next;
			command--;
		}

		if (currententry)
		{
			Ctrl_InviteWatchGame(cmdtarget, currententry->name);
		}
		return;
	}

	switch (command)
	{
		case MENU_STATUS_AVAILABLE:
		{
			Ctrl_SetPresence(SSTAT_AVAILABLE, NULL);
		}
		break;

		case MENU_STATUS_AWAY:
		{
			Ctrl_SetPresence(SSTAT_AWAY, NULL);
		}
		break;

		case MENU_STATUS_OFFLINE:
		{
			Ctrl_Disconnect();
		}
		break;

		case MENU_ADDSTATUS:
		{
			View_PopupSimpleDialog("addstatus_box", NULL);
		}
		break;

		case MENU_EDITSTATUSES:
		{
			View_PopupSimpleDialog("editstatus_box", NULL);
		}
		break;

		case MENU_CHANGEUSER:
		{
			Ctrl_ChangeUser();
		}
		break;

		case MENU_ABOUTCHESSPARK:
		{
			View_PopupSimpleDialog("about_box", NULL);
		}
		break;

		case MENU_PING:
		{
			View_PopupPingDialog();
		}
		break;

		case MENU_SENDDEBUGLOG:
		{
			View_PopupSendDebugLogDialog();
		}
		break;

		case MENU_QUIT:
		{
			Model_Quit();
		}
		break;

		case MENU_ADDFRIEND:
		{
			View_PopupSimpleDialog("addfriend_box", cmdtarget);
		}
		break;

		case MENU_ADDGROUP:
		{
			View_PopupSimpleDialog("addgroup_box", NULL);
		}
		break;

		case MENU_ADDGAMEAD:
		{
			View_PopupGameCreateAdDialog(NULL);
		}
		break;

		case MENU_NEWCHATFRIEND:
		{
			View_PopupSimpleDialog("newchat_box", NULL);
		}
		break;

		case MENU_MESSAGEFRIEND:
		{
			Model_PopupChatDialog(cmdtarget, 0);
		}
		break;

		case MENU_GAMEFRIEND:
		{
			Ctrl_InviteToGame(cmdtarget);
		}
		break;

		case MENU_INVITEFRIENDTOCHESSPARK:
		{
			View_PopupInviteFriendToChessparkDialog(cmdtarget);
		}
		break;

		case MENU_GAMEFRIENDCORR:
		{
			struct gamesearchinfo_s info;

			memset(&info, 0, sizeof(info));
			info.correspondence = 1;
			info.rated = 1;

			View_PopupGameInviteFriendDialog(cmdtarget, &info, 0);
		}
		break;

		case MENU_FRIENDADHOC:
		{
			Ctrl_AdHocCommand(cmdtarget, cmdadhoc);
		}
		break;
/*
		case MENU_OBSERVEFRIEND:
		{
			Ctrl_WatchGame(cmdgame);
		}
		break;
*/
		case MENU_REMOVEFRIEND:
		{
			View_PopupRemoveFriendDialog(cmdtarget, cmdgroup);
		}
		break;

		case MENU_INFOFRIEND:
		{
			Ctrl_ShowProfile(cmdtarget);
			/*View_PopupProfileDialog(cmdtarget);*/
		}
		break;

		case MENU_IGNOREFRIEND:
		{
			View_PopupIgnoreDialog(cmdtarget);
			/*Ctrl_Ignore(cmdtarget);*/
		}
		break;

		case MENU_UNIGNOREFRIEND:
		{
			Ctrl_Unignore(cmdtarget);
		}
		break;

		case MENU_REQUESTFRIEND:
		{
			Ctrl_RequestFriend(cmdtarget);
		}
		break;

		case MENU_REMOVEGROUP:
		{
			View_PopupSimpleDialog("removegroup_box", cmdtarget);
		}
		break;

		case MENU_RENAMEGROUP:
		{
			View_PopupSimpleDialog("renamegroup_box", cmdtarget);
		}
		break;

		case MENU_SHOWFRIENDOFFLINE:
		{
			 Model_SetOption(OPTION_SHOWOFFLINE, 2, NULL);
		}
		break;

		case MENU_SHOWFRIENDAVATARS:
		{
			 Model_SetOption(OPTION_SHOWAVATARS, 2, NULL);
		}
		break;

		case MENU_SHOWFRIENDGROUPS:
		{
			Model_SetOption(OPTION_SHOWFRIENDGROUPS, 2, NULL);
		}
		break;

		case MENU_SHOWFRIENDSTAB:
		{
			Model_SetOption(OPTION_HIDEFRIENDSTAB, 2, NULL);
		}
		break;

		case MENU_SHOWROOMSTAB:
		{
			Model_SetOption(OPTION_HIDEROOMSTAB, 2, NULL);
		}
		break;

		case MENU_SHOWGAMESTAB:
		{
			Model_SetOption(OPTION_HIDEGAMESTAB, 2, NULL);
		}
		break;

		case MENU_SHOWGAMESDIALOG:
		{
			View_PopupNewGamesDialog();
		}
		break;

		case MENU_AUTOAPPROVEFRIEND:
		{
			Model_SetOption(OPTION_AUTOAPPROVE, 2, NULL);
		}
		break;

		case MENU_JOINCHATROOM:
		{
			View_PopupSimpleDialog("joinchat_box", NULL);
		}
		break;

		case MENU_SHOWCHATPARTICIPANTS:
		{
			View_ToggleShowChatParticipants(cmdtarget);
		}
		break;

		case MENU_INVITETOTHISCHAT:
		{
			View_PopupInviteToChatDialog(NULL, cmdtarget);
		}
		break;

		case MENU_CHANGECHATTOPIC:
		{
			AutoEdit_Create(cmdbox, 500, _("Change room topic"), _("Enter the new topic here"), NULL, cmdtopic, Ctrl_SetTopicCallback, cmdtargetmuc, NULL);
		}
		break;

		case MENU_CHATOPTIONS:
		{
			Ctrl_RequestBanlist(cmdtargetmuc);
		}
		break;

		case MENU_JOINTHISCHAT:
		{
			Ctrl_JoinGroupChatDefaultNick(cmdtarget);
		}
		break;

		case MENU_GIVEVOICE:
		{
			Ctrl_GiveVoice(cmdtargetmuc, cmdtargetnick, NULL);
		}
		break;

		case MENU_REVOKEVOICE:
		{
			Ctrl_RevokeVoice(cmdtargetmuc, cmdtargetnick, NULL);
		}
		break;

		case MENU_KICK:
		{
			Ctrl_KickUser(cmdtargetmuc, cmdtargetnick, NULL);
		}
		break;

		case MENU_BAN:
		{
			View_PopupBanDialog(cmdtargetmuc, cmdtarget);
			/*Ctrl_BanUser(cmdtargetmuc, cmdtarget, NULL);*/
		}
		break;

		case MENU_GIVEMOD:
		{
			Ctrl_GiveMod(cmdtargetmuc, cmdtargetnick, NULL);
		}
		break;

		case MENU_REVOKEMOD:
		{
			Ctrl_RevokeMod(cmdtargetmuc, cmdtargetnick, NULL);
		}
		break;

		case MENU_GIVEADMIN:
		{
			Ctrl_GiveAdmin(cmdtargetmuc, cmdtarget, NULL);
		}
		break;

		case MENU_REVOKEADMIN:
		{
			Ctrl_RevokeAdmin(cmdtargetmuc, cmdtarget, NULL);
		}
		break;

		case MENU_GIVEOWNER:
		{
			Ctrl_GiveOwner(cmdtargetmuc, cmdtarget, NULL);
		}
		break;

		case MENU_REVOKEOWNER:
		{
			Ctrl_RevokeOwner(cmdtargetmuc, cmdtarget, NULL);
		}
		break;

		case MENU_CLEARCHATHISTORY:
		{
			Ctrl_ClearChatHistory(cmdtargetmuc);
		}
		break;

		case MENU_ADDCHATTOFAVORITES:
		{
			Ctrl_AddChatToFavorites(cmdtargetmuc);
		}
		break;

		case MENU_REMOVECHATFROMFAVORITES:
		{
			Ctrl_RemoveChatFromFavorites(cmdtargetmuc);
		}
		break;

		case MENU_ADDCHATTOAUTOJOIN:
		{
			Ctrl_AddChatToAutojoin(cmdtargetmuc);
		}
		break;

		case MENU_REMOVECHATFROMAUTOJOIN:
		{
			Ctrl_RemoveChatFromAutojoin(cmdtargetmuc);
		}
		break;

		case MENU_JOINTOURNEY:
		{
			Ctrl_JoinTournament(cmdtarget);
		}
		break;

		case MENU_FORFEITTOURNEY:
		{
			Ctrl_ForfeitTournament(cmdtarget);
		}
		break;

		case MENU_SETTOURNEYROUNDSTART:
		{
			View_PopupRoundStartDialog(cmdtarget);
		}
		break;

		case MENU_STARTNEXTTOURNEYROUND:
		{
			Ctrl_StartNextTournamentRound(cmdtarget);
		}
		break;

		case MENU_TEST:
		{
			Model_TestDisconnect();
			/*
			{
				char *crash = 0;
				*crash = 0;
			}
			*/
			/*Mem_DumpLeaks();*/
			/*View_PopupChessGame(NULL);*/
			/*Ctrl_RequestMatch("test@chesspark.com/EngineBot", TRUE, "standard", FALSE, -5, NULL, NULL);*/
			/*View_PopupGameInviteFriendDialog(NULL);*/
			/*View_PopupGameCreateAdDialog();*/
			
			/*View_PopupProfileDialog("jcanete@chesspark.com");*/
				/*
				malloc(sizeof(*blacktimecontrol));
			memset(blacktimecontrol, 0, sizeof(*blacktimecontrol));
*/
		}
		break;

		case MENU_TEST2:
		{
			/*View_HandleAdjourn(NULL);*/
		}
		break;

		case MENU_REQUESTADJOURN:
		{
			Ctrl_SendGameAdjourn(cmdtarget, cmdtargetmuc == NULL);
		}
		break;

		case MENU_FLAGOPPTIME:
		{
			Ctrl_SendGameFlag(cmdtarget, cmdtargetmuc == NULL);
		}
		break;

		case MENU_REQUESTDRAW:
		{
			Ctrl_SendGameDraw(cmdtarget, cmdtargetmuc == NULL);
		}
		break;

		case MENU_REQUESTABORT:
		{
			Ctrl_SendGameAbort(cmdtarget, cmdtargetmuc == NULL);
		}
		break;

		case MENU_RESIGNGAME:
		{
			Ctrl_SendGameResign(cmdtarget, cmdtargetmuc == NULL);
		}
		break;

		case MENU_ADDOPPTIME:
		{
			AutoEdit_Create(cmdbox, 500, _("Add time to opponent's clock"), _("Enter the amount of time (in seconds) here."), NULL, "10", Ctrl_AddTimeCallback, cmdtarget, NULL);
		}
		break;

		case MENU_CASTLEKINGSIDE:
		{
			Ctrl_SendMove(cmdtarget, "O-O");
		}
		break;

		case MENU_CASTLEQUEENSIDE:
		{
			Ctrl_SendMove(cmdtarget, "O-O-O");
		}
		break;

		case MENU_REQUESTTAKEBACK:
		{
			Ctrl_SendGameTakeback(cmdtarget, cmdtargetmuc == NULL);
		}
		break;

		case MENU_DEMOGAME:
		{
			struct gamesearchinfo_s *info = malloc(sizeof(*info));
			memset(info, 0, sizeof(*info));

			info->timecontrol = malloc(sizeof(*(info->timecontrol)));
			memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
			info->timecontrol->controlarray = malloc(sizeof(int) * 3);
			info->timecontrol->controlarray[0] = 1;
			info->timecontrol->controlarray[1] = -1;
			info->timecontrol->controlarray[2] = 30 * 60;
			info->timecontrol->delayinc = -5;
			info->takebacks = strdup("both");
			info->solitaire = 1;

			Ctrl_RequestMatch(Model_GetLoginJid(), info);
		}
		break;

		case MENU_CUT:
		{
			if (edittarget && edittarget->OnCut)
			{
				edittarget->OnCut(edittarget);
			}
		}
		break;

		case MENU_COPY:
		{
			if (edittarget && edittarget->OnCopy)
			{
				edittarget->OnCopy(edittarget);
			}
		}
		break;

		case MENU_PASTE:
		{
			if (edittarget && edittarget->OnPaste)
			{
				edittarget->OnPaste(edittarget);
			}
		}
		break;

		case MENU_DELETE:
		{
			if (edittarget && edittarget->OnDelete)
			{
				edittarget->OnDelete(edittarget);
			}
		}
		break;

		case MENU_COPYURL:
		{
			if (OpenClipboard(NULL))
			{
				HGLOBAL hg;
				char *temp;

				EmptyClipboard();

				hg = GlobalAlloc(GMEM_MOVEABLE, sizeof(char) * (strlen(cmdtarget) + 1));

				if (!hg)
				{
					CloseClipboard();
					Log_Write(0, "copy error!\n");
					return;
				}

				temp = GlobalLock(hg);
				strcpy(temp, cmdtarget);
				GlobalUnlock(hg);

				SetClipboardData(CF_TEXT, hg);

				CloseClipboard();
			}
		}
		break;

		case MENU_OPENURL:
		{
			Util_OpenURL(cmdtarget);
		}
		break;

		case MENU_OPTIONS:
		{
			View_PopupOptionsDialog();
		}
		break;

		case MENU_GAMEHISTORY:
		{
			Util_OpenURL("http://www.chesspark.com/people/games/");
		}
		break;

		default:
		break;
	}

	return;
}

void Menu_Init()
{
	Menu_RedoStatusMenu();
	
	prefshmenu = CreatePopupMenu();
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_SHOWFRIENDOFFLINE, _("Show Offline Friends"));
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_SHOWFRIENDAVATARS, _("Show Friend Avatars"));
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_SHOWFRIENDGROUPS, _("Show Friend Groups"));
	AppendMenuWrapper (prefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_SHOWFRIENDSTAB, _("Show Friends Tab"));
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_SHOWROOMSTAB, _("Show Rooms Tab"));
	/*AppendMenuWrapper (prefshmenu, MF_STRING, MENU_SHOWROOMSTAB, _("Show Games Tab"));*/
	AppendMenuWrapper (prefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	/*AppendMenuWrapper (prefshmenu, MF_STRING, MENU_AUTOAPPROVEFRIEND, "Auto Approve Friend Requests");*/
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_OPTIONS, _("Chesspark Options..."));
	AppendMenuWrapper (prefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_GAMEHISTORY, _("Game History"));
	AppendMenuWrapper (prefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_STATUS_OFFLINE, _("Go Offline"));
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_CHANGEUSER, _("Change User..."));
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_ABOUTCHESSPARK, _("About Chesspark..."));
	AppendMenuWrapper (prefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_PING, _("Ping Server"));
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_SENDDEBUGLOG, _("Submit Debug Log..."));
	/*AppendMenuWrapper (prefshmenu, MF_STRING, MENU_DEMOGAME, _("Start demonstration game"));*/
	AppendMenuWrapper (prefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (prefshmenu, MF_STRING, MENU_QUIT, _("Quit Chesspark"));

	addhmenu = CreatePopupMenu();
	AppendMenuWrapper (addhmenu, MF_STRING, MENU_ADDFRIEND, _("Add Friend..."));
	AppendMenuWrapper (addhmenu, MF_STRING, MENU_ADDGROUP, _("Add Group..."));
	/*AppendMenuWrapper (addhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (addhmenu, MF_STRING, MENU_ADDGAMEAD, _("New Game Ad..."));*/
	AppendMenuWrapper (addhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (addhmenu, MF_STRING, MENU_NEWCHATFRIEND, _("New Chat With Friend..."));
	AppendMenuWrapper (addhmenu, MF_STRING, MENU_JOINCHATROOM, _("Join Chat Room..."));
	/*AppendMenuWrapper (addhmenu, MF_STRING, MENU_TEST, "Test");*/

	friendhmenu = CreatePopupMenu();
	AppendMenuWrapper (friendhmenu, MF_STRING, MENU_MESSAGEFRIEND, _("Send Message..."));
	AppendMenuWrapper (friendhmenu, MF_STRING, MENU_GAMEFRIEND, _("Invite to Game..."));
	AppendMenuWrapper (friendhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (friendhmenu, MF_STRING, MENU_REMOVEFRIEND, _("Remove Friend"));
	AppendMenuWrapper (friendhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (friendhmenu, MF_STRING, MENU_INFOFRIEND, _("View Info"));
	AppendMenuWrapper (friendhmenu, MF_STRING | MF_GRAYED, MENU_LOGFRIEND, _("View Logs"));

	pendingfriendhmenu = CreatePopupMenu();
	AppendMenuWrapper (pendingfriendhmenu, MF_STRING, MENU_MESSAGEFRIEND, _("Send Message..."));
	AppendMenuWrapper (pendingfriendhmenu, MF_STRING, MENU_GAMEFRIEND, _("Invite to Game..."));
	AppendMenuWrapper (pendingfriendhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (pendingfriendhmenu, MF_STRING, MENU_REMOVEFRIEND, _("Remove Friend"));
	AppendMenuWrapper (pendingfriendhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (pendingfriendhmenu, MF_STRING, MENU_INFOFRIEND, _("View Info"));
	AppendMenuWrapper (pendingfriendhmenu, MF_STRING | MF_GRAYED, MENU_LOGFRIEND, _("View Logs"));
	AppendMenuWrapper (pendingfriendhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (pendingfriendhmenu, MF_STRING, MENU_REQUESTFRIEND, _("Resend Friend Request"));
	
	chatboxprefshmenu = CreatePopupMenu();
	AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_SHOWCHATPARTICIPANTS, _("Show Participants"));
	AppendMenuWrapper (chatboxprefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_INVITETOTHISCHAT, _("Invite Someone to this Chat..."));
	AppendMenuWrapper (chatboxprefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_CHANGECHATTOPIC, _("Change Chat Topic..."));
	AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_CHATOPTIONS, _("Ban List..."));
	AppendMenuWrapper (chatboxprefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_ADDCHATTOFAVORITES, _("Add to Favorites"));
	AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_ADDCHATTOAUTOJOIN, _("Join Chat Room on Login"));
	AppendMenuWrapper (chatboxprefshmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (chatboxprefshmenu, MF_STRING, MENU_GAMEFRIEND, _("Invite to Game..."));

	friendstabblankhmenu = CreatePopupMenu();
	AppendMenuWrapper (friendstabblankhmenu, MF_STRING, MENU_ADDFRIEND, _("Add Friend..."));
	AppendMenuWrapper (friendstabblankhmenu, MF_STRING, MENU_ADDGROUP, _("Add Group..."));

	grouphmenu = CreatePopupMenu();
	AppendMenuWrapper (grouphmenu, MF_STRING, MENU_RENAMEGROUP, _("Rename Group..."));
	AppendMenuWrapper (grouphmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (grouphmenu, MF_STRING, MENU_REMOVEGROUP, _("Remove Group"));

	chatparticipanthmenu = CreatePopupMenu();
	AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_MESSAGEFRIEND, _("Send Message..."));
	AppendMenuWrapper (chatparticipanthmenu, MF_STRING | MF_GRAYED, MENU_INFOFRIEND, _("View Info..."));
	AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_ADDFRIEND, _("Add Friend..."));
	AppendMenuWrapper (chatparticipanthmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_GIVEVOICE, _("Give Voice Privilege"));
	AppendMenuWrapper (chatparticipanthmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_KICK, _("Kick from Room"));
	AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_BAN, _("Ban from Room"));
	AppendMenuWrapper (chatparticipanthmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_GIVEMOD, _("Give Moderator Status"));
	AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_GIVEADMIN, _("Give Admin Status"));
	AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_GIVEOWNER, _("Give Owner Status"));
	AppendMenuWrapper (chatparticipanthmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (chatparticipanthmenu, MF_STRING, MENU_GAMEFRIEND, _("Invite to Game..."));
	
	chataddhmenu = CreatePopupMenu();
	AppendMenuWrapper (chataddhmenu, MF_STRING, MENU_INVITETOTHISCHAT, _("Invite Someone to this Chat..."));

	roomhmenu = CreatePopupMenu();
	AppendMenuWrapper (roomhmenu, MF_STRING, MENU_JOINTHISCHAT, _("Join Chat Room"));
	AppendMenuWrapper (roomhmenu, MF_STRING, MENU_ADDCHATTOFAVORITES, _("Add to Favorites"));
	AppendMenuWrapper (roomhmenu, MF_STRING, MENU_ADDCHATTOAUTOJOIN, _("Join Chat Room on login"));

	roomstabblankhmenu = CreatePopupMenu();
	AppendMenuWrapper (roomstabblankhmenu, MF_STRING, MENU_JOINCHATROOM, _("New Chat Room"));

	gamehmenu = CreatePopupMenu();
	AppendMenuWrapper (gamehmenu, MF_STRING | MF_GRAYED, MENU_SAVEGAME, _("Save Game"));
	AppendMenuWrapper (gamehmenu, MF_STRING | MF_GRAYED, MENU_SAVEGAMEAS, _("Save Game As..."));
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_REQUESTADJOURN, _("Request Adjournment"));
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_FLAGOPPTIME, _("Flag Opponent's Time"));
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_REQUESTDRAW, _("Request a Draw"));
	AppendMenuWrapper (gamehmenu, MF_STRING | MF_GRAYED, MENU_REQUESTABORT, _("Request Game Abort"));
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_RESIGNGAME, _("Resign Game"));
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING | MF_GRAYED, MENU_ADDOPPTIME, _("Add More Time to Opponent's Clock..."));
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_CASTLEKINGSIDE, _("Quick Castle King's Side"));
	AppendMenuWrapper (gamehmenu, MF_STRING, MENU_CASTLEQUEENSIDE, _("Quick Castle King's Side"));
	AppendMenuWrapper (gamehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (gamehmenu, MF_STRING | MF_GRAYED, MENU_REQUESTTAKEBACK, _("Request a Move Take Back"));
	/*AppendMenuWrapper (gamehmenu, MF_STRING, MENU_TEST2, "Test 2");*/

	edithmenu = CreatePopupMenu();
	AppendMenuWrapper (edithmenu, MF_STRING, MENU_CUT, _("Cut"));
	AppendMenuWrapper (edithmenu, MF_STRING, MENU_COPY, _("Copy"));
	AppendMenuWrapper (edithmenu, MF_STRING, MENU_PASTE, _("Paste"));
	AppendMenuWrapper (edithmenu, MF_STRING, MENU_DELETE, _("Delete"));

	tourneyhmenu = CreatePopupMenu();
	AppendMenuWrapper (tourneyhmenu, MF_STRING, MENU_JOINTOURNEY, _("Join Tournament"));
	/*AppendMenuWrapper (tourneyhmenu, MF_STRING, MENU_FORFEITTOURNEY, "Forfeit Tournament");*/
	AppendMenuWrapper (tourneyhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (tourneyhmenu, MF_STRING, MENU_SETTOURNEYROUNDSTART, _("Set Tournament Round Start Time"));
	AppendMenuWrapper (tourneyhmenu, MF_STRING, MENU_STARTNEXTTOURNEYROUND, _("Start Next Round"));

	urlhmenu = CreatePopupMenu();
	AppendMenuWrapper (urlhmenu, MF_STRING, MENU_OPENURL, _("Open link"));
	AppendMenuWrapper (urlhmenu, MF_STRING, MENU_COPYURL, _("Copy link"));

	gamefriendsubhmenu = CreatePopupMenu();
	AppendMenuWrapper (gamefriendsubhmenu, MF_STRING, MENU_GAMEFRIEND, _("Invite to game..."));

	moderationsubhmenu = CreatePopupMenu();
	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_GIVEVOICE, _("Give Voice Privilege"));
	AppendMenuWrapper (moderationsubhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_KICK, _("Kick from Room"));
	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_BAN, _("Ban from Room"));
	AppendMenuWrapper (moderationsubhmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_GIVEMOD, _("Give Moderator Status"));
	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_GIVEADMIN, _("Give Admin Status"));
	AppendMenuWrapper (moderationsubhmenu, MF_STRING, MENU_GIVEOWNER, _("Give Owner Status"));

	profilehmenu = CreatePopupMenu();
}

void Menu_RedoStatusMenu()
{
	int statusnum;
	struct StatusList_s *pstatus;
	struct Box_s *pbox, *icon, *text;
	HBITMAP hbmp;
	MENUINFO mi;

	if (presencehmenu)
	{
		DestroyMenu(presencehmenu);
	}

	presencehmenu = CreatePopupMenu();

	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIM_BACKGROUND;
	mi.hbrBack = CreateSolidBrush(TabBG3);

	SetMenuInfo(presencehmenu, &mi);

	pbox = Box_Create(0, 0, 120, 16, BOX_VISIBLE);
	pbox->bgcol = TabBG3;

	icon = Box_Create(0, 0, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	icon->img = ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16);
	Box_AddChild(pbox, icon);

	text = Box_Create(22, 0, pbox->w - 22, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_SetText(text, _("Available"));
	Box_AddChild(pbox, text);

	hbmp = Box_Draw_Hbmp(pbox, FALSE);

	AppendMenuWrapper(presencehmenu, MF_BITMAP, MENU_STATUS_AVAILABLE, (LPCTSTR)hbmp);

	statusnum = MENU_STATUS_AVAILABLE + 1;
	pstatus = availstatuslist;

	while (pstatus)
	{
		if (pstatus->status == SSTAT_AVAILABLE)
		{
			Box_SetText(text, pstatus->statusmsg);

			hbmp = Box_Draw_Hbmp(pbox, FALSE);

			AppendMenuWrapper(presencehmenu, MF_BITMAP, statusnum, (LPCTSTR)hbmp);

			statusnum++;
		}
		pstatus = pstatus->next;
	}

	AppendMenuWrapper (presencehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);

	icon->img = ImageMgr_GetSubImage("presenceAway", "PresenceIcons.png", 48, 0, 16, 16);
	Box_SetText(text, _("Away"));

	hbmp = Box_Draw_Hbmp(pbox, FALSE);

	AppendMenuWrapper (presencehmenu, MF_BITMAP, MENU_STATUS_AWAY, (LPCTSTR)hbmp);

	statusnum = MENU_STATUS_AWAY + 1;
	pstatus = awaystatuslist;

	while (pstatus)
	{
		if (pstatus->status == SSTAT_AWAY)
		{
			Box_SetText(text, pstatus->statusmsg);

			hbmp = Box_Draw_Hbmp(pbox, FALSE);

			AppendMenuWrapper(presencehmenu, MF_BITMAP, statusnum, (LPCTSTR)hbmp);

			statusnum++;
		}
		pstatus = pstatus->next;
	}

	AppendMenuWrapper (presencehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (presencehmenu, MF_STRING, MENU_STATUS_OFFLINE, _("Offline"));
	AppendMenuWrapper (presencehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (presencehmenu, MF_STRING, MENU_ADDSTATUS, _("Add New Status..."));
	AppendMenuWrapper (presencehmenu, MF_STRING, MENU_EDITSTATUSES, _("Edit My Statuses..."));
	AppendMenuWrapper (presencehmenu, MF_SEPARATOR, (UINT_PTR)NULL, NULL);
	AppendMenuWrapper (presencehmenu, MF_STRING, MENU_CHANGEUSER, _("Change User..."));

	Box_Destroy(pbox);
}


void View_SetStatusList(enum SStatus status, struct StatusList_s *list)
{
	if (status == SSTAT_AWAY)
	{
		StatusList_Destroy(&awaystatuslist);
		awaystatuslist = StatusList_Copy(list);
		Menu_RedoStatusMenu();
	}
	else if (status == SSTAT_AVAILABLE)
	{
		StatusList_Destroy(&availstatuslist);
		availstatuslist = StatusList_Copy(list);
		Menu_RedoStatusMenu();
	}
}

void Menu_CheckPrefsItems()
{
	int showfriendoffline = Model_GetOption(OPTION_SHOWOFFLINE);
	int showfriendgroups  = Model_GetOption(OPTION_SHOWFRIENDGROUPS);
	int showfriendavatars = Model_GetOption(OPTION_SHOWAVATARS);
	int showfriendstab    = !Model_GetOption(OPTION_HIDEFRIENDSTAB);
	int showroomstab      = !Model_GetOption(OPTION_HIDEROOMSTAB);
	int showgamestab      = !Model_GetOption(OPTION_HIDEGAMESTAB);
	int autoapprovefriend = Model_GetOption(OPTION_AUTOAPPROVE);

	CheckMenuItem(prefshmenu, MENU_SHOWFRIENDOFFLINE, MF_BYCOMMAND | (showfriendoffline ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(prefshmenu, MENU_SHOWFRIENDAVATARS, MF_BYCOMMAND | (showfriendavatars ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(prefshmenu, MENU_SHOWFRIENDGROUPS,  MF_BYCOMMAND | (showfriendgroups  ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(prefshmenu, MENU_SHOWFRIENDSTAB,    MF_BYCOMMAND | (showfriendstab    ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(prefshmenu, MENU_SHOWROOMSTAB,      MF_BYCOMMAND | (showroomstab      ? MF_CHECKED : MF_UNCHECKED));
	/*CheckMenuItem(prefshmenu, MENU_SHOWGAMESTAB,      MF_BYCOMMAND | (showgamestab      ? MF_CHECKED : MF_UNCHECKED));*/
	/*CheckMenuItem(prefshmenu, MENU_AUTOAPPROVEFRIEND, MF_BYCOMMAND | (autoapprovefriend ? MF_CHECKED : MF_UNCHECKED));*/
}

void CustomMenuEntry_DefaultOnHit(struct Box_s *pbox, int command)
{
	struct Box_s *menu = Box_GetRoot(pbox);

	menu->OnInactive = NULL;
	Box_Destroy(menu);

	Menu_OnCommand(NULL, command);
}

void CustomMenu_AddEntry2(struct Box_s *menu, char *text, struct BoxImage_s *icon, void (*OnHitCallback)(struct Box_s *, void *), void *userdata)
{
	struct Box_s *itembox, *textbox, *hover, *iconbox, *bar;

	textbox = Text_Create(30, 3, 1, 14, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
	Text_SetText(textbox, text);
	textbox->y = menu->h + 3;

	if (menu->w > textbox->w + 30 + 22)
	{
		textbox->w = menu->w - 30 - 22;
	}

	itembox = Button2_Create(4, 0, textbox->w + 30 + 22 - 4 - 4, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	if (text)
	{
		Button2_SetOnButtonHit2(itembox, OnHitCallback, userdata);
	}
	itembox->bgcol = RGB(241, 243, 231);
	itembox->y = menu->h;
	
	if (text)
	{
		hover = Box_Create(4, 0, itembox->w, 20, 0);
		hover->bgcol = RGB(237, 200, 125);
		Button2_SetHover(itembox, hover);
		hover->y = menu->h;
	}
	else
	{
		bar = Box_Create(4, 0, itembox->w, 1, BOX_VISIBLE);
		bar->bgcol = RGB(222, 225, 207);
		bar->y = menu->h + 10;
	}

	if (icon)
	{
		iconbox = Box_Create(11, 2, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		iconbox->img = icon;
		iconbox->y = menu->h + 2;
	}

	if (menu->w < itembox->w + 8)
	{
		struct Box_s *child;
		menu->w = itembox->w + 8;

		child = menu->child;
		while (child)
		{
			if (child->x == 4)
			{
				child->w = itembox->w;
			}
			child = child->sibling;
		}
	}

	Box_AddChild(menu, itembox);
	if (text)
	{
		Box_AddChild(menu, hover);
	}
	else
	{
		Box_AddChild(menu, bar);
	}
	if (icon)
	{
		Box_AddChild(menu, iconbox);
	}
	Box_AddChild(menu, textbox);

	menu->h += itembox->h;
}

void CustomMenu_AddEntry(struct Box_s *menu, char *text, struct BoxImage_s *icon, int command)
{
	CustomMenu_AddEntry2(menu, text, icon, CustomMenuEntry_DefaultOnHit, (void *)command);
}

void CustomMenu_PopMenu(struct Box_s *menu, struct Box_s *parent, int mousex, int mousey)
{
	menu->h += 8;

	menu->OnInactive = Box_Destroy;

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;
		int x, y;

		windowrect.left = mousex;
		windowrect.right = mousex;
		windowrect.top = mousey;
		windowrect.bottom = mousey;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		x = mousex;
		x = (x + menu->w > mi.rcWork.right) ? mi.rcWork.right - menu->w : x;
		x = (x < mi.rcWork.left) ? mi.rcWork.left : x;

		y = mousey;
		y = (y + menu->h > mi.rcWork.bottom) ? mi.rcWork.bottom - menu->h : y;
		y = (y < mi.rcWork.top) ? mi.rcWork.top : y;

		menu->x = x;
		menu->y = y;
	}

	Box_CreateWndMenu(menu, parent->hwnd);
}

void Menu_RedoStatusMenu2(struct Box_s *parent, int mousex, int mousey)
{
	struct Box_s *menu;
	struct StatusList_s *pstatus;
	int statusnum;

	menu = Box_Create(100, 100, 1, 8, BOX_VISIBLE | BOX_BORDER);
	menu->bgcol = RGB(241, 243, 231);
	menu->brcol = RGB(153, 153, 153);

	CustomMenu_AddEntry(menu, _("Available"), ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16), MENU_STATUS_AVAILABLE);

	statusnum = MENU_STATUS_AVAILABLE + 1;
	pstatus = availstatuslist;

	while (pstatus)
	{
		if (pstatus->status == SSTAT_AVAILABLE)
		{
			CustomMenu_AddEntry(menu, pstatus->statusmsg, ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16), statusnum);
			statusnum++;
		}
		pstatus = pstatus->next;
	}

	CustomMenu_AddEntry(menu, NULL, NULL, 0);

	CustomMenu_AddEntry(menu, _("Away"), ImageMgr_GetSubImage("presenceAway", "PresenceIcons.png", 48, 0, 16, 16), MENU_STATUS_AWAY);

	statusnum = MENU_STATUS_AWAY + 1;
	pstatus = awaystatuslist;

	while (pstatus)
	{
		if (pstatus->status == SSTAT_AWAY)
		{
			CustomMenu_AddEntry(menu, pstatus->statusmsg, ImageMgr_GetSubImage("presenceAway", "PresenceIcons.png", 48, 0, 16, 16), statusnum);
			statusnum++;
		}
		pstatus = pstatus->next;
	}

	CustomMenu_AddEntry(menu, NULL, NULL, 0);
	CustomMenu_AddEntry(menu, _("Offline"), NULL, MENU_STATUS_OFFLINE);
	if (Model_GetOption(OPTION_DEVFEATURES))
	{
		CustomMenu_AddEntry(menu, NULL, NULL, 0);
		CustomMenu_AddEntry(menu, _("Test disconnect"), NULL, MENU_TEST);
	}
	CustomMenu_AddEntry(menu, NULL, NULL, 0);
	CustomMenu_AddEntry(menu, _("Add New Status..."), NULL, MENU_ADDSTATUS);
	CustomMenu_AddEntry(menu, _("Edit My Statuses..."), NULL, MENU_EDITSTATUSES);
	CustomMenu_AddEntry(menu, NULL, NULL, 0);
	CustomMenu_AddEntry(menu, _("Change User..."), NULL, MENU_CHANGEUSER);

	CustomMenu_PopMenu(menu, parent, mousex, mousey);
}
