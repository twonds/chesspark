#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>

#include "box.h"

#include "button.h"
#include "edit.h"
#include "sizer.h"
#include "text.h"
#include "titledrag.h"

#include "about.h"
#include "add.h"
#include "addgroup.h"
#include "addstatus.h"
#include "approve.h"
#include "autodialog.h"
#include "autoedit.h"
#include "autowait.h"
#include "banlist.h"
#include "boxtypes.h"
#include "chatbox.h"
#include "chessbox.h"
#include "conn.h"
#include "constants.h"
#include "cornerpop.h"
#include "ctrl.h"
#include "editstatus.h"
#include "friendentry.h"
#include "gamecreatead.h"
#include "gameinvitation.h"
#include "gameinviteerror.h"
#include "gameinvitefriend.h"
#include "gamereconvene.h"
#include "gamerespondad.h"
#include "gameslist.h"
#include "i18n.h"
#include "imagemgr.h"
#include "info.h"
#include "invitetochat.h"
#include "joinchat.h"
#include "list.h"
#include "log.h"
#include "login.h"
#include "model.h"
#include "menu.h"
#include "namedlist.h"
#include "newchat.h"
#include "newsearch.h"
#include "options.h"
#include "profile.h"
#include "removefriend.h"
#include "removegroup.h"
#include "renamegroup.h"
#include "roomentry.h"
#include "roomslist.h"
#include "roster.h"
#include "spawn.h"
#include "tourneybox.h"
#include "util.h"

#include "view.h"

struct Box_s *login_box           = NULL;
struct Box_s *roster_box          = NULL;
struct Box_s *gamecreatead_box    = NULL;
struct Box_s *gamerespondad_box   = NULL;
struct Box_s *gamereconvene_box   = NULL;
struct Box_s *newgames_box        = NULL;

struct Box_s *lastactivechatbox = NULL;

BOOL rosternub = FALSE;
int roomsfirstrefresh = 0;

char *currentgameid = NULL;

void View_GenericError(char *errortitle, char *error);

struct namedlist_s *firstchat                      = NULL;
struct namedlist_s *rostertabcontents              = NULL;
struct namedlist_s *firstrosterspawn               = NULL;
struct namedlist_s *gamedialoglist                 = NULL;
struct namedlist_s *gamerequestdialoglistbyid      = NULL;
struct namedlist_s *gamerequestdialoglistbyjid     = NULL;
struct namedlist_s *forwardgamerequesttogamedialog = NULL;

static NOTIFYICONDATA local_nid;

void ViewLink_CorGameOpen_OnClick(struct Box_s *pbox, char *gameid)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		View_PopupChessGame(gameid, NULL, NULL);
		Conn_GetCorGameState(gameid);
	}
}

void ViewLink_CorGameResign_OnClick(struct Box_s *pbox, char *gameid)
{
	Ctrl_SendGameResign(gameid, 1);
}

void ViewLink_CorGameAcceptDraw_OnClick(struct Box_s *pbox, char *gameid)
{
	Ctrl_SendGameDraw(gameid, 1);
}

void ViewLink_CorGameRejectDraw_OnClick(struct Box_s *pbox, char *gameid)
{
	Ctrl_RejectGameDraw(gameid, 1);
}

void ViewLink_GameOpen_OnClick(struct Box_s *pbox, struct gamesearchinfo_s *info)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
		Model_PopupReconvene(info->gameid, info);
	}
}

void ViewLink_GameResign_OnClick(struct Box_s *pbox, char *gameid)
{
	Ctrl_SendGameResign(gameid, 0);
}

void ViewLink_RespondAd_OnClick(struct Box_s *pbox, struct gamesearchinfo_s *info)
{
	if (Conn_GetConnState() == CONN_CONNECTED)
	{
                View_PopupGameRespondAdDialog(info->adplayer->jid, info);
	}
}

void ViewLink_WatchGame_OnClick(struct Box_s *pbox, char *txt)
{
	char gameid[256];
	char *pipe;

	strcpy(gameid, txt);
	if (pipe = strchr(gameid, '|'))
	{
	    *pipe = '\0';
	}

	Ctrl_WatchGame(gameid, 0);
}

void ViewLink_JoinChat_OnClick(struct Box_s *pbox, char *txt)
{
	char name[256];
	char *pipe;

	strcpy(name, txt);
	if (pipe = strchr(name, '|'))
	{
	    *pipe = '\0';
	}
	
	if (!Jid_IsValid(name))
	{
		if(strchr(name, '@'))
		{
			*(strchr(name, '@')) = 0;
		}

		strcat(name, "@chat.chesspark.com");
	}
	else
	{
		if (!strchr(name, '@'))
		{
			strcat(name, "@chat.chesspark.com");
		}
	}

	Ctrl_JoinGroupChatDefaultNick(name);
}

void ViewLink_AddContact_OnClick(struct Box_s *pbox, char *txt)
{
	char jid[256];
	char *pipe;

	strcpy(jid, txt);
	if (pipe = strchr(jid, '|'))
	{
		*pipe = '\0';
	}

	View_PopupSimpleDialog("addfriend_box", jid);
}

void ViewLink_ProfileLink_OnClick(struct Box_s *pbox, char *txt)
{
	char jid[256];
	char url[512];
	char escapedjid[512];
	char *pipe;

	strcpy(jid, txt);
	if (pipe = strchr(jid, '|'))
	{
		*pipe = '\0';
	}

	if (!(strchr(jid, '@')))
	{
		strcat(jid, "@chesspark.com");
	}

	sprintf(url, "http://www.chesspark.com/people/%s/", EscapeURL(jid, escapedjid, 512));

	/*Ctrl_ShowProfile(jid);*/
	Util_OpenURL(url);
}


void ViewLink_ShowProfile(struct Box_s *pbox, char *jid)
{
	Ctrl_ShowProfile(jid);
}

void ViewLink_Quickmatch(struct Box_s *pbox, void *userdata)
{
	Ctrl_PlayNow();
	/*Ctrl_RequestGameSearch("quickplay", NULL);*/
}

void ViewLink_ShowGameFinder(struct Box_s *pbox, void *userdata)
{
	if (newgames_box)
	{
		BringWindowToTop(newgames_box->hwnd);
	}
	else
	{
		View_PopupNewGamesDialog();
		Model_SetOption(OPTION_NOGAMESEARCHONLOGIN, 0, NULL);
	}
}

void ViewLink_ShowAdjournedGames(struct Box_s *pbox, void *userdata)
{
	if (newgames_box)
	{
		NewGames_ExternalSetPage(newgames_box, "adjourned");

		BringWindowToTop(newgames_box->hwnd);
	}
#if 0
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Games"));
	struct Box_s *gameslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	gameslist_box = (*tabcontent)->data;
/*
	if (gameslist_box)
	{
		Model_SetOption(OPTION_HIDEGAMESTAB, 0, NULL);
		Roster_ActivateTab(roster_box, _("Games"));
		GamesList_ExternalSearch(gameslist_box, "adjourned");
		{
			WINDOWPLACEMENT wp;
			
			wp.length = sizeof(wp);
			GetWindowPlacement(gameslist_box->hwnd, &wp);

			if (wp.showCmd == SW_SHOWMINIMIZED)
			{
                                ShowWindow(gameslist_box->hwnd, SW_RESTORE);
			}
		}
		BringWindowToTop(gameslist_box->hwnd);
	}
	*/
#endif
}

void ViewLink_ShowCorrespondenceGames(struct Box_s *pbox, void *userdata)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Games"));
	struct Box_s *gameslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	gameslist_box = (*tabcontent)->data;
/*
	if (gameslist_box)
	{
		Model_SetOption(OPTION_HIDEGAMESTAB, 0, NULL);
		Roster_ActivateTab(roster_box, _("Games"));
		GamesList_ExternalSearch(gameslist_box, "correspondence");
		{
			WINDOWPLACEMENT wp;
			
			wp.length = sizeof(wp);
			GetWindowPlacement(gameslist_box->hwnd, &wp);

			if (wp.showCmd == SW_SHOWMINIMIZED)
			{
                                ShowWindow(gameslist_box->hwnd, SW_RESTORE);
			}
		}
		BringWindowToTop(gameslist_box->hwnd);
	}
	*/
}


void ViewLink_MatchLink_OnClick(struct Box_s *pbox, char *jid)
{
	char newjid[256];
	char tc[2][256];
	char *find;
	struct gamesearchinfo_s *info = NULL;

	strcpy(newjid, jid);

	if (find = strchr(newjid, '|'))
	{
		*find = '\0';
	}

	if (find = strchr(newjid, ':'))
	{
		strcpy(tc[0], find + 1);
		*find = '\0';
	}
	else
	{
		tc[0][0] = '\0';
	}

	if (tc[0][0] && (find = strchr(tc[0], ':')))
	{
		strcpy(tc[1], find + 1);
		*find = '\0';
	}
	else
	{
		tc[1][0] = '\0';
	}

	if (find = strchr(newjid, '@'))
	{
		
	}
	else
	{
		strcat(newjid, "@chesspark.com");
	}

	{
		char *bareloginjid = Jid_Strip(Model_GetLoginJid());

		if (stricmp(bareloginjid, newjid) == 0)
		{
			return;
		}
	}



	if (tc[0][0])
	{
		int i;
		info = malloc(sizeof(*info));
		memset(info, 0, sizeof(*info));

		for (i = 0; i < 2; i++)
		{
			if (stricmp(tc[i], "long") == 0)
			{
				info->timecontrol = malloc(sizeof(*(info->timecontrol)));
				memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
				info->timecontrol->controlarray = malloc(sizeof(int) * 3);
				info->timecontrol->controlarray[0] = 1;
				info->timecontrol->controlarray[1] = -1;
				info->timecontrol->controlarray[2] = 30 * 60;
				info->timecontrol->delayinc = -5;
			}
			else if (stricmp(tc[i], "speed") == 0)
			{
				info->timecontrol = malloc(sizeof(*(info->timecontrol)));
				memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
				info->timecontrol->controlarray = malloc(sizeof(int) * 3);
				info->timecontrol->controlarray[0] = 1;
				info->timecontrol->controlarray[1] = -1;
				info->timecontrol->controlarray[2] = 15 * 60;
				info->timecontrol->delayinc = -5;
			}
			else if (stricmp(tc[i], "rapid") == 0)
			{
				info->timecontrol = malloc(sizeof(*(info->timecontrol)));
				memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
				info->timecontrol->controlarray = malloc(sizeof(int) * 3);
				info->timecontrol->controlarray[0] = 1;
				info->timecontrol->controlarray[1] = -1;
				info->timecontrol->controlarray[2] = 10 * 60;
				info->timecontrol->delayinc = -5;
			}
			else if (stricmp(tc[i], "blitz") == 0)
			{
				info->timecontrol = malloc(sizeof(*(info->timecontrol)));
				memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
				info->timecontrol->controlarray = malloc(sizeof(int) * 3);
				info->timecontrol->controlarray[0] = 1;
				info->timecontrol->controlarray[1] = -1;
				info->timecontrol->controlarray[2] = 5 * 60;
				info->timecontrol->delayinc = -2;
			}
			else if (stricmp(tc[i], "bullet") == 0)
			{
				info->timecontrol = malloc(sizeof(*(info->timecontrol)));
				memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
				info->timecontrol->controlarray = malloc(sizeof(int) * 3);
				info->timecontrol->controlarray[0] = 1;
				info->timecontrol->controlarray[1] = -1;
				info->timecontrol->controlarray[2] = 1 * 60;
				info->timecontrol->delayinc = -2;
			}
			else if (stricmp(tc[i], "atomic") == 0)
			{
				info->variant = strdup("atomic");
			}
			else if (stricmp(tc[i], "chess960") == 0)
			{
				info->variant = strdup("chess960");
			}
#ifdef CHESSPARK_CRAZYHOUSE
			else if (stricmp(tc[i], "crazyhouse") == 0)
			{
				info->variant = strdup("crazyhouse");
			}
#endif
#ifdef CHESSPARK_LOSERS
			else if (stricmp(tc[i], "losers") == 0)
			{
				info->variant = strdup("losers");
			}
#endif
#ifdef CHESSPARK_CHECKERS
			else if (stricmp(tc[i], "checkers") == 0)
			{
				info->variant = strdup("checkers");
			}
#endif
		}

		info->rated = 1;
	}

	View_PopupGameInviteFriendDialog(newjid, info, info && info->timecontrol);
}

void ViewLink_HelpLink_OnClick(struct Box_s *pbox, char *helplink)
{
	char *linkcopy;
	char fullurl[512];
	char *find;

	linkcopy = strdup(helplink);

	if (find = strchr(linkcopy, '|'))
	{
		*find = '\0';
	}

	sprintf(fullurl, "http://www.chesspark.com/help/%s/", linkcopy);
	free(linkcopy);

	Util_OpenURL(fullurl);
}

void ViewLink_OpenChat(struct Box_s *pbox, char *jid)
{
	Model_PopupChatDialog(jid, 0);
}

void ViewLink_OpenURLNoTopmost(struct Box_s *pbox, char *url)
{
	struct Box_s *dialog = Box_GetRoot(pbox);

	SetWindowPos(dialog->hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);

	Util_OpenURL(url);
}

void View_SetTrayIcon()
{
	/*
	memset(&local_nid, 0, sizeof(local_nid));

	local_nid.cbSize = sizeof(local_nid);
	local_nid.hWnd = roster_box->hwnd;
	local_nid.uFlags = NIF_ICON;
	local_nid.hIcon = LoadImage(NULL, "app.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE | LR_SHARED);
	Shell_NotifyIcon(NIM_ADD, &local_nid);
	*/
}

void View_KillTrayIcon()
{
	/*
	Shell_NotifyIcon(NIM_DELETE, &local_nid);
	*/
}

struct windowposinfo_s
{
	char *id;
	int x;
	int y;
	int w;
	int h;
	int dw; /* drawer width */
	int group;
	long timestamp;
};

struct windowposinfo_s windowposinfo[25];
int windowposinit = 0;

void View_GetWindowPosFromRegistry()
{
	char buffer[2048];
	char *txt = strdup(Model_GetUserRegString("WindowPos", buffer, 2048));
	char *parse = txt;
	int i = 0;
	RECT screensize, windowrect;
	HMONITOR hm;
	MONITORINFO mi;

	{
		/* null stored window positions */
		int i;

		for (i = 0; i < 25; i++)
		{
			windowposinfo[i].id = NULL;
			windowposinfo[i].x = 100;
			windowposinfo[i].y = 100;
			windowposinfo[i].w = 400;
			windowposinfo[i].h = 400;
			windowposinfo[i].dw = 0;
			windowposinfo[i].group = 0;
			windowposinfo[i].timestamp = 0;
		}
	}

	while (parse && *parse)
	{
		char *colon = strchr(parse, ':');

		if (colon)
		{
			*colon = '\0';

			windowposinfo[i].dw = 0;
			windowposinfo[i].group = 0;
			sscanf(colon + 1, "%d:%d:%d:%d:%d:%d:%d",
				&(windowposinfo[i].x), &(windowposinfo[i].y),
				&(windowposinfo[i].w), &(windowposinfo[i].h),
				&(windowposinfo[i].timestamp),
				&(windowposinfo[i].dw),
				&(windowposinfo[i].group));

			windowposinfo[i].id = strdup(parse);

			windowrect.left = windowposinfo[i].x;
			windowrect.right = windowrect.left + windowposinfo[i].w - 1;
			windowrect.top = windowposinfo[i].y;
			windowrect.bottom = windowrect.top + windowposinfo[i].h - 1;

			hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

			mi.cbSize = sizeof(mi);
			GetMonitorInfo(hm, &mi);

			screensize = mi.rcWork;

			/* make sure window is on screen and at least 100x100 */
			if (windowposinfo[i].w < 100)
			{
				windowposinfo[i].w = 100;
			}

			if (windowposinfo[i].h < 100)
			{
				windowposinfo[i].h = 100;
			}

			if (windowposinfo[i].x < screensize.left)
			{
				windowposinfo[i].x = screensize.left;
			}

			if (windowposinfo[i].x > screensize.right - windowposinfo[i].w)
			{
				windowposinfo[i].x = screensize.right - windowposinfo[i].w;
			}

			if (windowposinfo[i].y < screensize.top)
			{
				windowposinfo[i].y = screensize.top;
			}

			if (windowposinfo[i].y > screensize.bottom - windowposinfo[i].h)
			{
				windowposinfo[i].y = screensize.bottom - windowposinfo[i].h;
			}

			i++;

			parse = strchr(colon + 1, ';');

			if (parse)
			{
				parse++;
			}
		}
		else
		{
			parse = NULL;
		}
	}
	
	free(txt);

	windowposinit = 1;
}

void View_SetWindowPosToRegistry()
{
	int i;
	char txt[512], finaltxt[512*25];

	finaltxt[0] = 0;

	for (i = 0; i < 25; i++)
	{
		if (windowposinfo[i].id)
		{
			sprintf(txt, "%s:%d:%d:%d:%d:%d:%d:%d;", windowposinfo[i].id, windowposinfo[i].x, windowposinfo[i].y, windowposinfo[i].w, windowposinfo[i].h, windowposinfo[i].timestamp, windowposinfo[i].dw, windowposinfo[i].group);
			strcat(finaltxt, txt);
		}
	}

	Model_SetUserRegString("WindowPos", finaltxt);
}

int autocascadepos = 100;

static int CheckIfCascade(int x, int y)
{
	RECT windowrect;
	HMONITOR hm;
	MONITORINFO mi;
	
	if (!roster_box)
	{
		return 0;
	}

	windowrect.left = roster_box->x;
	windowrect.right = windowrect.left + roster_box->w - 1;
	windowrect.top = roster_box->y;
	windowrect.bottom = windowrect.top + roster_box->h - 1;

	hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hm, &mi);

	if ((x - mi.rcWork.left == y - mi.rcWork.top) && ((x - mi.rcWork.left) % 20) == 0)
	{
		return 1;
	}

	return 0;
}

static int CheckIfOffscreen(int x, int y, int w, int h)
{
	RECT windowrect;
	HMONITOR hm;
	MONITORINFO mi;

	if (!roster_box)
	{
		return 0;
	}

	windowrect.left = roster_box->x;
	windowrect.right = windowrect.left + roster_box->w - 1;
	windowrect.top = roster_box->y;
	windowrect.bottom = windowrect.top + roster_box->h - 1;

	hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hm, &mi);

	if ((x + mi.rcWork.left > mi.rcWork.right - w)
	    || (y + mi.rcWork.top > mi.rcWork.bottom - h))
	{
		Log_Write(0, "%dx%d at %d,%d is offscreen for screen %dx%d at %d,%d\n", x, y, w, h, mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top, mi.rcWork.left, mi.rcWork.top);
		return 1;
	}

	Log_Write(0, "%dx%d at %d,%d is not offscreen for screen %dx%d at %d,%d\n", x, y, w, h, mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top, mi.rcWork.left, mi.rcWork.top);
	return 0;
}

void GetAutoCascadePos(int *x, int *y)
{
	RECT windowrect;
	HMONITOR hm;
	MONITORINFO mi;

	if (!roster_box)
	{
		return;
	}

	windowrect.left = roster_box->x;
	windowrect.right = windowrect.left + roster_box->w - 1;
	windowrect.top = roster_box->y;
	windowrect.bottom = windowrect.top + roster_box->h - 1;

	hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hm, &mi);

	*x = autocascadepos + mi.rcWork.left;
	*y = autocascadepos + mi.rcWork.top;

	Log_Write(0, "Autocascade position is %d, %d for screen %dx%d at %d,%d\n", *x, *y, mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top, mi.rcWork.left, mi.rcWork.top);

	/* autocascade doesn't make sense for tabs */
	/*autocascadepos += 20;*/
}

int View_GetSavedWindowPos2(char *id, int *x, int *y, int *w, int *h, int *dw, int *group)
{
	int i;
	
	for (i = 0; i < 25; i++)
	{
		if (windowposinfo[i].id && stricmp(id, windowposinfo[i].id) == 0)
		{
			/* HACK: ignore automatic cascade positions */
			if (!(CheckIfCascade(windowposinfo[i].x, windowposinfo[i].y)))
			{
				*x     = windowposinfo[i].x;
				*y     = windowposinfo[i].y;
				*w     = windowposinfo[i].w;
				*h     = windowposinfo[i].h;
				*dw    = windowposinfo[i].dw;
				*group = windowposinfo[i].group;

				/* check position is on screen, and if not, force it there */
				{
					RECT windowrect;
					HMONITOR hm;
					MONITORINFO mi;

					windowrect.left = *x - *dw;
					windowrect.right = *x + *w - *dw;
					windowrect.top = *y;
					windowrect.bottom = *y + *h;

					hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

					mi.cbSize = sizeof(mi);
					GetMonitorInfo(hm, &mi);

					if (*x + *w > mi.rcMonitor.right)
					{
						*x = mi.rcMonitor.right - *w;
					}

					if (*y + *h > mi.rcMonitor.bottom)
					{
						*y = mi.rcMonitor.bottom - *h;
					}

					if (*dw > 0 && *x - *dw < mi.rcMonitor.left)
					{
						*x = mi.rcMonitor.left + *dw;
					}

					if (*y < mi.rcMonitor.top)
					{
						*y = mi.rcMonitor.top;
					}
				}

				return 1;
			}
		}
	}

	if (CheckIfOffscreen(autocascadepos, autocascadepos, *w, *h))
	{
		autocascadepos = 100;
	}

	GetAutoCascadePos(x, y);

	return 0;
}

int View_GetSavedWindowPos(char *id, int *x, int *y, int *w, int *h)
{
	int dummy;
	return View_GetSavedWindowPos2(id, x, y, w, h, &dummy, &dummy);
}

void View_SetSavedWindowPos2(char *id, int x, int y, int w, int h, int dw, int group)
{
	int i, found, set;
	long currenttime = (long)time(NULL);
	long oldesttime = currenttime;

	/* HACK: Don't save window position if x == y and x is a multiple of 20, means an automatic cascade position */
	if (CheckIfCascade(x, y))
	{
		return;
	}

	found = 0;

	for (i = 0; i < 25; i++)
	{
		if (windowposinfo[i].id && stricmp(id, windowposinfo[i].id) == 0)
		{
			set = i;
			i = 25;
			found = 1;
		}
	}

	if (!found)
	{
		set = 0;

		for (i = 0; i < 25; i++)
		{
			if (!windowposinfo[i].id)
			{
				set = i;
				i = 25;
			}
			else if (windowposinfo[i].timestamp < oldesttime)
			{
				set = i;
				oldesttime = windowposinfo[i].timestamp;
			}
		}
	}
	free(windowposinfo[set].id);
	windowposinfo[set].id = strdup(id);
	windowposinfo[set].x  = x;
	windowposinfo[set].y  = y;
	windowposinfo[set].w  = w;
	windowposinfo[set].h  = h;
	windowposinfo[set].dw = dw;
	windowposinfo[set].group = group;
	windowposinfo[set].timestamp = currenttime;

	View_SetWindowPosToRegistry();
}

void View_SetSavedWindowPos(char *id, int x, int y, int w, int h)
{
	View_SetSavedWindowPos2(id, x, y, w, h, 0, 0);
}


void View_RemoveSavedWindowPos(char *id)
{
	int i;

	for (i = 0; i < 25; i++)
	{
		if (windowposinfo[i].id && stricmp(id, windowposinfo[i].id) == 0)
		{
			free(windowposinfo[i].id);
			windowposinfo[i].id = NULL;
		}
	}

	View_SetWindowPosToRegistry();
}

int View_GetEmptySavedWindowGroupNum()
{
	int groupnum = 0;
	int found;

	do
	{
		struct namedlist_s *pchat;
		groupnum++;
		found = 0;

		pchat = firstchat;
		while(pchat)
		{
			struct Box_s *chatbox = pchat->data;
			int cbgroup = ChatBox_GetChatboxGroup(chatbox);

			if (cbgroup == groupnum)
			{
				found = 1;
				pchat = NULL;
			}
			else
			{
				pchat = pchat->next;
			}
		}

		if (!found)
		{
			int i;
			for (i = 0; i < 25; i++)
			{
				if (windowposinfo[i].group == groupnum)
				{
					found = 1;
					i = 25;
				}
			}
		}
	}
	while (found);

	return groupnum;
}


void View_PopupSimpleDialog(char *dialogtype, void *userdata)
{
	struct Box_s *dialog = Box_GetWindowByName(dialogtype);

	if (dialog)
	{
		BringWindowToTop(dialog->hwnd);
		return;
	}

	if (stricmp(dialogtype, "about_box") == 0)
	{
		dialog = About_Create(roster_box);
	}
	else if (stricmp(dialogtype, "addfriend_box") == 0)
	{
		char *jid = userdata;
		dialog = AddFriend_Create(roster_box, jid);
	}
	else if (stricmp(dialogtype, "addgroup_box") == 0)
	{
		dialog = AddGroup_Create(roster_box);
	}
	else if (stricmp(dialogtype, "renamegroup_box") == 0)
	{
		char *name = userdata;
		dialog = RenameGroup_Create(roster_box, name);
	}
	else if (stricmp(dialogtype, "addstatus_box") == 0)
	{
		dialog = AddStatus_Create(roster_box);
	}
	else if (stricmp(dialogtype, "approve_box") == 0)
	{
		char *jid = userdata;
		dialog = Approve_Create(roster_box, jid);
	}
	else if (stricmp(dialogtype, "removegroup_box") == 0)
	{
		char *name = userdata;
		dialog = RemoveGroup_Create(roster_box, name);
	}
	else if (stricmp(dialogtype, "editstatus_box") == 0)
	{
		struct StatusList_s *awaystatuslist, *availstatuslist;
		Menu_GetStatusLists(&awaystatuslist, &availstatuslist);
		dialog = EditStatus_Create(roster_box, awaystatuslist, availstatuslist);
	}
	else if (stricmp(dialogtype, "joinchat_box") == 0)
	{
		dialog = JoinChat_Create(roster_box);
	}
	else if (stricmp(dialogtype, "newchat_box") == 0)
	{
		dialog = NewChat_Create(roster_box);
	}

	Box_RegisterWindowName(dialog, dialogtype);
}

struct namedlist_s **View_GetPtrToChat(char *jid, int isGroupChat)
{
	struct namedlist_s **ppchat = &firstchat;
	
	while(*ppchat)
	{
		struct Box_s *chatbox = (*ppchat)->data;
		if (ChatBox_HasChat(chatbox, jid, isGroupChat))
		{
			return ppchat;
		}
		ppchat = &((*ppchat)->next);
	}

	return ppchat;
 /* autotab chats */
#if 0
	return &firstchat; /* return the last created chat */
#endif
}


void View_PopupChatDialog(char *jid, char *nick, int isGroupChat)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppchat = View_GetPtrToChat(jid, isGroupChat);
	struct namedlist_s **ppchat2 = View_GetPtrToChat(barejid, 1);
	struct Box_s *chatbox;
	int x, y, w, h, dw, group;

	if (*ppchat)
	{
		struct Box_s *chatbox = (*ppchat)->data;
		/*BringWindowToTop(chatbox->hwnd);*/

		/* If there's an existing groupchat with this name, then this is a groupchat private chat, so preserve the resource */
		if (!isGroupChat && ppchat2 && *ppchat2)
		{
			ChatBox_ActivateChat(chatbox, jid, nick, isGroupChat, 0);
		}
		else
		{
			ChatBox_ActivateChat(chatbox, barejid, nick, isGroupChat, 0);
		}
		return;
	}

	x = 100;
	y = 100;
	w = 400;
	h = 400;
	dw = 0;
	group = 0;

	{
		View_GetSavedWindowPos2(barejid, &x, &y, &w, &h, &dw, &group);
	}
#if 0
	if (group != 0)
	{
		struct namedlist_s **ppchat3 = &firstchat;
		while(*ppchat3)
		{
			struct Box_s *chatbox = (*ppchat3)->data;
			int cbgroup;
			cbgroup = ChatBox_GetChatboxGroup(chatbox);

			if (cbgroup == group)
			{
				if (!isGroupChat && ppchat2 && *ppchat2)
				{
					ChatBox_ActivateChat(chatbox, jid, nick, isGroupChat, 0);
				}
				else
				{
					ChatBox_ActivateChat(chatbox, barejid, nick, isGroupChat, 0);
				}
				return;
			}

			ppchat3 = &((*ppchat3)->next);
		}
	}
#endif
	if (!*ppchat && firstchat && Model_GetOption(OPTION_TABNEWCHATS))
	{
		struct Box_s *chatbox;
		ppchat = &firstchat;
		
		if (lastactivechatbox)
		{
			chatbox = lastactivechatbox;
		}
		else
		{
			chatbox = (*ppchat)->data;
		}

		/*BringWindowToTop(chatbox->hwnd);*/

		/* If there's an existing groupchat with this name, then this is a groupchat private chat, so preserve the resource */
		if (!isGroupChat && ppchat2 && *ppchat2)
		{
			ChatBox_ActivateChat(chatbox, jid, nick, isGroupChat, 0);
		}
		else
		{
			ChatBox_ActivateChat(chatbox, barejid, nick, isGroupChat, 0);
		}
		return;
	}

	if (group == 0)
	{
		group = View_GetEmptySavedWindowGroupNum();
	}

	chatbox = ChatBox_Create(x, y, w, h, BOX_VISIBLE, roster_box, group);

	/* If there's an existing groupchat with this name, then this is a groupchat private chat, so preserve the resource */
	if (!isGroupChat && ppchat2 && *ppchat2)
	{
		Log_Write(0, "1-Adding chat %s\n", jid);
		NamedList_Add(&firstchat, jid, chatbox, Box_Destroy);
		ChatBox_ActivateChat(chatbox, jid, nick, isGroupChat, 0);
	}
	else
	{
		Log_Write(0, "2-Adding chat %s\n", barejid);
		NamedList_Add(&firstchat, barejid, chatbox, Box_Destroy);
		ChatBox_ActivateChat(chatbox, barejid, nick, isGroupChat, 0);
	}

	if (dw > 0)
	{
		ChatBox_QuickDrawerOpen(chatbox, dw);
	}
	else if (!Model_GetOption(OPTION_HIDEPARTICIPANTS) && isGroupChat && dw != -1)
	{
		ChatBox_QuickDrawerOpen(chatbox, 150);
	}
	/*BringWindowToTop(chatbox->hwnd);*/
}

void View_ActivateChatDialog(char *jid, char *nick, int isGroupChat)
{
	struct namedlist_s **ppchat = View_GetPtrToChat(jid, isGroupChat);
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppchat2 = View_GetPtrToChat(barejid, 1);

	if (ppchat && *ppchat)
	{
		struct Box_s *chatbox = (*ppchat)->data;
		{
			WINDOWPLACEMENT wp;
			
			wp.length = sizeof(wp);
			GetWindowPlacement(chatbox->hwnd, &wp);

			if (wp.showCmd == SW_SHOWMINIMIZED)
			{
                                ShowWindow(chatbox->hwnd, SW_RESTORE);
			}
		}
		BringWindowToTop(chatbox->hwnd);

		/* If there's an existing groupchat with this name, then this is a groupchat private chat, so preserve the resource */
		if (!isGroupChat && ppchat2 && *ppchat2)
		{
			ChatBox_ActivateChat(chatbox, jid, nick, isGroupChat, 1);
		}
		else
		{
			ChatBox_ActivateChat(chatbox, barejid, nick, isGroupChat, 1);
		}
	}

	free(barejid);
}

void View_SetLastActiveChatBox(struct Box_s *chatbox)
{
	lastactivechatbox = chatbox;
}

void View_CloseChatDialog(struct Box_s *pbox)
{
	struct namedlist_s **ppchat = &firstchat;

	while(*ppchat)
	{
		struct Box_s *chatbox = (*ppchat)->data;
		if (lastactivechatbox == pbox)
		{
			lastactivechatbox = NULL;
		}
		if (chatbox == pbox)
		{
			Log_Write(0, "Removing chatbox %d\n", pbox);
			NamedList_Remove(ppchat);
			continue;
		}
		ppchat = &((*ppchat)->next);
	}
}

void View_CloseAllChatDialogs()
{
	while (firstchat)
	{
		NamedList_Remove(&firstchat);
	}
	lastactivechatbox = NULL;
}

void View_CloseAllGameRequestDialogs()
{
	NamedList_Destroy(&gamerequestdialoglistbyid);
	NamedList_Destroy(&gamerequestdialoglistbyjid);
}


void View_SetDisconnectOnChats()
{
	struct namedlist_s *entry = firstchat;

	while (entry)
	{
		struct Box_s *chatbox = entry->data;
		ChatBox_SetDisconnect(chatbox);
		entry = entry->next;
	}

	entry = gamedialoglist;

	while (entry)
	{
		struct Box_s *chessbox = entry->data;
		ChessBox_SetDisconnect(chessbox);
		entry = entry->next;
	}
/*
	if (tourneybox)
	{
		TourneyBox_ShowDisconnect(tourneybox);
	}
*/
}

void View_ShowDisconnectOnChats()
{
	struct namedlist_s *entry = firstchat;

	while (entry)
	{
		struct Box_s *chatbox = entry->data;
		ChatBox_ShowDisconnect(chatbox);
		entry = entry->next;
	}

	entry = gamedialoglist;

	while (entry)
	{
		struct Box_s *chessbox = entry->data;
		ChessBox_ShowDisconnect(chessbox);
		entry = entry->next;
	}
/*
	if (tourneybox)
	{
		TourneyBox_ShowDisconnect(tourneybox);
	}
*/
}

void View_ReconnectChatDialogs(int chessparkchats)
{
	struct namedlist_s *ppchat = firstchat;

	while (ppchat)
	{
		struct Box_s *chatbox = ppchat->data;
		ChatBox_ReconnectChats(chatbox, chessparkchats);
		ppchat = ppchat->next;
	}
}

void View_SetStatusOnAllChats(enum SStatus status, char *statusmsg)
{
	struct namedlist_s *ppchat = firstchat;
	struct namedlist_s *pgame = gamedialoglist;

	while (ppchat)
	{
		struct Box_s *chatbox = ppchat->data;
		ChatBox_SetStatusOnAllChats(chatbox, status, statusmsg);
		ppchat = ppchat->next;
	}

	while (pgame)
	{
		struct Box_s *chessbox = pgame->data;
		ChessBox_SetStatusOnChat(chessbox, status, statusmsg);
		pgame = pgame->next;
	}
}

void View_OnChatTabDropEmpty(struct Box_s *psrc, char *jid, int x, int y)
{
	struct Box_s *pdst;
	/* Spawn a new window */
	pdst = ChatBox_Create(x, y, 400, 400, BOX_VISIBLE, roster_box, View_GetEmptySavedWindowGroupNum());
	NamedList_Add(&firstchat, jid, pdst, Box_Destroy);
	ChatBox_MoveChat(psrc, pdst, jid);
}

struct Box_s *View_CreateEmptyChatBox(int x, int y)
{
	struct Box_s *pdst;
	pdst = ChatBox_Create(x, y, 400, 400, BOX_VISIBLE, roster_box, View_GetEmptySavedWindowGroupNum());
	NamedList_Add(&firstchat, NULL, pdst, Box_Destroy);

	return pdst;
}

void View_SetComposing(char *jid, char *msgid)
{
	struct namedlist_s **ppchat = View_GetPtrToChat(jid, 0);
	struct Box_s *chatbox;

	if (!*ppchat)
	{
		return;
	}

	chatbox = (*ppchat)->data;

	if (!ChatBox_HasChat(chatbox, jid, 0))
	{
		return;
	}
	
	ChatBox_SetComposing(chatbox, jid, msgid);
}

void View_UnsetComposing(char *jid, char *msgid)
{
	struct namedlist_s **ppchat = View_GetPtrToChat(jid, 0);
	struct Box_s *chatbox;

	if (!*ppchat)
	{
		return;
	}

	chatbox = (*ppchat)->data;

	if (!ChatBox_HasChat(chatbox, jid, 0))
	{
		return;
	}
	
	ChatBox_UnsetComposing(chatbox, jid, msgid);
}

int View_IsInRoom(char *roomjid)
{
	struct namedlist_s **ppchat = View_GetPtrToChat(roomjid, 1);
	struct Box_s *chatbox;

	if (!ppchat || !*ppchat)
	{
		return 0;
	}

	chatbox = (*ppchat)->data;

	return ChatBox_IsInRoom(chatbox, roomjid);
}


void View_AddChatMessage(char *jid, char *nick, char *msg)
{
	struct namedlist_s **ppchat = View_GetPtrToChat(jid, 0);
	struct Box_s *chatbox;

	if (!ppchat || !*ppchat)
	{
		View_PopupChatDialog(jid, nick, 0);
		ChatBox_InitialIMSound();
		ppchat = View_GetPtrToChat(jid, 0);
		chatbox = (*ppchat)->data;
		if (!Model_GetOption(OPTION_NOCHATNOTIFY))
		{
			FlashWindow(chatbox->hwnd, TRUE);
		}
	}

	if (!ppchat || !*ppchat)
	{
		return;
	}

	chatbox = (*ppchat)->data;

#if 0
	if (!ChatBox_HasChat(chatbox, jid, 0))
	{
		ChatBox_ActivateChat(chatbox, jid, nick, 0);
	}
#endif

	ChatBox_AddText(chatbox, jid, nick, msg, NULL, 1);
}


void View_AddGroupChatMessage(char *jid, char *msg, char *timestamp)
{
	char *barejid = Jid_Strip(jid);
	char *resource = Jid_GetResource(jid);
	struct namedlist_s **ppchat = View_GetPtrToChat(barejid, 1);
	struct Box_s *chatbox;

	/*
	if (!ppchat || !*ppchat)
	{
		View_PopupChatDialog(barejid, Ctrl_GetDefaultNick(), 1);
		ppchat = View_GetPtrToChat(barejid, 1);
	}
	*/

	if (!ppchat || !*ppchat)
	{
		return;
	}

	chatbox = (*ppchat)->data;

	if (!ChatBox_HasChat(chatbox, barejid, 1))
	{
		ChatBox_ActivateChat(chatbox, barejid, Ctrl_GetDefaultNick(), 1, 0);
	}

	ChatBox_AddText(chatbox, barejid, resource, msg, timestamp, 1);
	free(barejid);
	free(resource);
}

void View_GroupChatError(char *roomjid, int icode, char *error)
{
	char *barejid = Jid_Strip(roomjid);
	struct namedlist_s **ppchat = View_GetPtrToChat(barejid, 1);
	struct Box_s *chatbox;

	if (!ppchat || !*ppchat)
	{
		return;
	}

	chatbox = (*ppchat)->data;

	if (!ChatBox_HasChat(chatbox, barejid, 1))
	{
		return;
	}

	ChatBox_MucError(chatbox, barejid, icode, error);
	free(barejid);
}

void View_SetGroupChatTopic(char *jid, char *topic)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppchat = View_GetPtrToChat(barejid, 1);
	struct Box_s *chatbox;
	char *name = Jid_GetResource(jid);

	if (!ppchat || !*ppchat)
	{
		View_PopupChatDialog(barejid, Ctrl_GetDefaultNick(), 1);
		ppchat = View_GetPtrToChat(barejid, 1);
	}

	if (!ppchat || !*ppchat)
	{
		return;
	}

	chatbox = (*ppchat)->data;

	if (!ChatBox_HasChat(chatbox, barejid, 1))
	{
		ChatBox_ActivateChat(chatbox, barejid, Ctrl_GetDefaultNick(), 1, 0);
	}

	ChatBox_SetChatTopic(chatbox, barejid, name, topic);
	free(barejid);
}


void View_SetChatParticipantStatus(char *jid, enum SStatus status, char *statusmsg, char *role, char *affiliation, char *realjid, char *nickchange,
  struct namedlist_s *roleslist, struct namedlist_s *titleslist, char *actorjid, char *reason, int notactivated, char *membertype)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppchat = View_GetPtrToChat(barejid, 1);
	struct Box_s *chatbox;

	if (!ppchat || !*ppchat)
	{
		ppchat = View_GetPtrToChat(barejid, 0);
	}

	if (!ppchat || !*ppchat)
	{
		return;
	}

	chatbox = (*ppchat)->data;

	ChatBox_SetParticipantStatus(chatbox, barejid, jid, status,
		statusmsg, role, affiliation, realjid, nickchange, roleslist,
		titleslist, actorjid, reason, notactivated, membertype);
}

void View_SetGameChatParticipantStatus(char *jid, enum SStatus status,
  char *statusmsg, char *role, char *affiliation, char *realjid,
  char *nickchange, struct namedlist_s *roleslist,
  struct namedlist_s *titleslist, char *actorjid, char *reason,
  int notactivated, char *membertype)
{
	char *barejid = Jid_Strip(jid);
	char *nick = Jid_GetResource(jid);
	char *gameid = Jid_GetBeforeAt(jid);
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;

	if (!gamelistentry || !*gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_SetParticipantStatus(chessbox, barejid, jid, status,
	  statusmsg, role, affiliation, realjid, nickchange, roleslist,
	  titleslist, notactivated, membertype);
	return;
}

void View_RefreshChatIcon(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppchat = View_GetPtrToChat(barejid, 1);

	if (!ppchat || !*ppchat)
	{
		ppchat = View_GetPtrToChat(barejid, 0);
	}

	if (!ppchat || !*ppchat)
	{
		return;
	}

	ChatBox_UpdateTitlebarIcon((*ppchat)->data);
}

int View_LocalUserHasMucPower(char *jid)
{
	char *barejid = Jid_Strip(jid);
	struct namedlist_s **ppchat = View_GetPtrToChat(barejid, 1);
	
	free(barejid);
	if (!ppchat || !*ppchat)
	{
		return 0;
	}

	return ChatBox_LocalUserHasMucPower((*ppchat)->data, jid);
}

void View_ToggleShowChatParticipants(char *jid)
{
	struct namedlist_s **ppchat = View_GetPtrToChat(jid, 0);
	struct Box_s *chatbox;

	if (!ppchat || !(*ppchat))
	{
		char *barejid = Jid_Strip(jid);
		ppchat = View_GetPtrToChat(barejid, 1);

		free(barejid);

		if (!ppchat || !(*ppchat))
		{
			return;
		}
	}

	chatbox = (*ppchat)->data;

	ChatBox_ToggleShowParticipants(chatbox);
}


void View_RoomNickConflict(char *jid)
{
	char *barejid = Jid_Strip(jid);
	char *nick = Jid_GetResource(jid);
	struct namedlist_s **ppchat = View_GetPtrToChat(barejid, 1);
	struct Box_s *chatbox;

	if (!ppchat || !(*ppchat))
	{
		free(barejid);
		free(nick);
		return;
	}

	chatbox = (*ppchat)->data;

	ChatBox_RoomNickConflict(chatbox, barejid, nick);
	free(barejid);
	free(nick);
}


void View_PopupRemoveFriendDialog(char *name, char *group)
{
	struct Box_s *removefriend_box = Box_GetWindowByName("removefriend_box");

	if (removefriend_box)
	{
		BringWindowToTop(removefriend_box->hwnd);
		return;
	}

	removefriend_box = RemoveFriend_Create(roster_box, name, group);
	Box_RegisterWindowName(removefriend_box, "removefriend_box");
}


void View_ShowRosterTab(BOOL show, char *name)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, name);
	struct Box_s *tabcontentbox;
	struct Box_s *rootbox;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	tabcontentbox = (*tabcontent)->data;

	if (!tabcontentbox)
	{
		return;
	}

	rootbox = Box_GetRoot(tabcontentbox);


	if (show)
	{
		if (rootbox == tabcontentbox)
		{
			Roster_AddTab(roster_box, name, tabcontentbox);
			Roster_ActivateTab(roster_box, name);
		}
	}
	else
	{
		if (rootbox == roster_box)
		{
			Box_Unlink(tabcontentbox);
			Roster_RemoveTab(roster_box, name);
		}
		else if (rootbox != tabcontentbox)
		{
			Box_Unlink(tabcontentbox);
			Box_Destroy(rootbox);
		}
	}
}

void View_SetOptions()
{
	int showfriendoffline = Model_GetOption(OPTION_SHOWOFFLINE);
	int showfriendgroups  = Model_GetOption(OPTION_SHOWFRIENDGROUPS);
	int showfriendavatars = Model_GetOption(OPTION_SHOWAVATARS);
	int showfriendstab    = !Model_GetOption(OPTION_HIDEFRIENDSTAB);
	int showroomstab      = !Model_GetOption(OPTION_HIDEROOMSTAB);
	int showgamestab      = !Model_GetOption(OPTION_HIDEGAMESTAB);
	struct namedlist_s **tabcontent;
	
	View_ShowRosterTab(showfriendstab, _("Friends"));
	View_ShowRosterTab(showroomstab,   _("Rooms"));
	/*View_ShowRosterTab(showgamestab,   _("Games"));*/

	if (roster_box)
	{
		Roster_UpdateToOptions(roster_box);
	}

	tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));

	if (tabcontent)
	{
		struct Box_s *friendslist_box = (*tabcontent)->data;

		List_SetShowGroups(friendslist_box, showfriendgroups);
		FriendEntry_SetShowIcon(showfriendavatars);
		FriendEntry_SetShowOffline(showfriendoffline);
		List_CallEntryFunc(friendslist_box, FriendEntry_Refresh, NULL);
		List_SetEntryVisibleFunc(friendslist_box, showfriendoffline ? NULL : FriendEntry_VisibleFunc);

		List_RedoEntries(friendslist_box);
		List_ScrollVisible(friendslist_box);
		Box_Repaint(friendslist_box);
	}

	Menu_CheckPrefsItems();
}

	
void View_Init()
{
	Menu_Init();
}


void View_Start()
{
}

void View_End()
{
	int i;

	while(firstchat)
	{
		NamedList_Remove(&firstchat);
	}

	View_CloseRoster();
	View_CloseLogin();
	View_CloseNewGamesDialog();

	for (i=0; i<25; i++)
	{
		free(windowposinfo[i].id);
	}
}

void View_ClearLogin(struct Box_s *pbox)
{
	login_box = NULL;
}

void View_PopupLogin()
{
	if (!login_box)
	{
		login_box = Login_Create();
		login_box->OnDestroy = View_ClearLogin;

		ShowWindow(login_box->hwnd, SW_SHOW);
	}
}


void View_CloseLogin()
{
	if (login_box)
	{
		Box_Destroy(login_box);
		login_box = NULL;
	}
}

void ViewRoster_OnTabActivate(struct Box_s *tabs, char *tabname)
{
	if (roster_box && strcmp(tabname, _("Rooms")) == 0 && !roomsfirstrefresh)
	{
		roomsfirstrefresh = 1;
		Ctrl_GetRooms();
	}
}

void FriendsList_OnGroupDragDrop(struct Box_s *psrc, struct Box_s *pdst, int id, void *data, char *dropname)
{
	if (id == FRIENDDROPDATA_ID)
	{
		struct FriendDropData_s *srcdata = data;

		if (GetKeyState(VK_SHIFT) & 0x8000)
		{
			Ctrl_AddFriendToGroup(srcdata->jid, dropname);
		}
		else
		{
			Ctrl_MoveFriendFromGroupToGroup(srcdata->jid, srcdata->groupname, dropname);
		}
	}
}


void View_PopupRoster()
{
	int x, y, w, h, found;
	int newroster = 0;

	if (!roster_box)
	{
		x = 100;
		y = 30;
		w = 256;
		h = 512;

		View_GetSavedWindowPos("roster", &x, &y, &w, &h);
		roster_box = Roster_Create(x, y, w, h, BOX_VISIBLE);
		roster_box->OnCommand = Menu_OnCommand;
		Roster_SetTabActivateFunc(roster_box, ViewRoster_OnTabActivate);
		View_SetTrayIcon();
		newroster = 1;
	}
	else if (rosternub)
	{
		View_UnnubRoster();
	}

	roomsfirstrefresh = 0;

	/*
	AutoDialog_Create(roster_box, 500, "Welcome to Chesspark",
	  "As a guest user, you have a limited number of privileges on the "
	  "Chesspark System.  For example, you'll be able to watch and play "
	  "games, but no one will be able to add you to their friends list, "
	  "and you won't be able to add anyone to your friends list.\n\n"
	  "^bIf you'd like access to all of Chesspark's features,^n "
	  "^lregister a free trial account.^l\n"
	  "It's fast, easy, and did we mention that it's free?", NULL, NULL,
	  NULL, NULL, NULL);
	  */

	if (!NamedList_GetByName(&rostertabcontents, _("Friends")))
	{
		struct Box_s *friendslist_box;

		x = 0;
		y = 0;
		w = 100;
		h = 100;

		found = View_GetSavedWindowPos("Friends", &x, &y, &w, &h);

		friendslist_box = List_Create(0, 0, 100, 100, BOX_VISIBLE, TRUE);
		friendslist_box->bgcol = TabBG2;
		friendslist_box->fgcol = TabFG2;
		friendslist_box->OnSizeWidth = Box_OnSizeWidth_Stretch;
		friendslist_box->OnSizeHeight = Box_OnSizeHeight_Stretch;
		List_SetEntrySortFunc(friendslist_box, FriendEntry_SortFunc);
		List_SetEntryVisibleFunc(friendslist_box, FriendEntry_VisibleFunc);
		List_SetEmptyRClickFunc(friendslist_box, Menu_PopupRosterBlankMenu);
		List_SetGroupRMenuFunc(friendslist_box, Menu_PopupGroupMenu);
		List_SetOnGroupDragDrop(friendslist_box, FriendsList_OnGroupDragDrop);
		NamedList_Add(&rostertabcontents, _("Friends"), friendslist_box, Box_Destroy);

		if (found)
		{
			View_SpawnRosterTabWindow(_("Friends"), x, y, w, h);
		}
		else
		{
			Roster_AddTab(roster_box, _("Friends"), friendslist_box);
		}
	}

	if (!NamedList_GetByName(&rostertabcontents, _("Rooms")))
	{
		struct Box_s *roomslist_box;

		x = 0;
		y = 0;
		w = 100;
		h = 100;

		found = View_GetSavedWindowPos("Rooms", &x, &y, &w, &h);

		roomslist_box = RoomsList_Create();

		NamedList_Add(&rostertabcontents, _("Rooms"), roomslist_box, Box_Destroy);

		if (found)
		{
			View_SpawnRosterTabWindow(_("Rooms"), x, y, w, h);
		}
		else
		{
			Roster_AddTab(roster_box, _("Rooms"), roomslist_box);
		}
	}
/*
	if (!NamedList_GetByName(&rostertabcontents, _("Games")))
	{
		struct Box_s *gameslist_box;

		x = 0;
		y = 0;
		w = 100;
		h = 100;

		found = View_GetSavedWindowPos("Games", &x, &y, &w, &h);

		gameslist_box = GamesList_Create();

		NamedList_Add(&rostertabcontents, _("Games"), gameslist_box, Box_Destroy);

		if (found)
		{
			View_SpawnRosterTabWindow(_("Games"), x, y, w, h);
		}
		else
		{
			Roster_AddTab(roster_box, _("Games"), gameslist_box);
		}
	}
*/
	if (newroster)
	{
		Roster_ActivateTab(roster_box, _("Friends"));
	}
}


void View_CloseRoster()
{
	struct namedlist_s **tabcontent;
	struct Box_s *tabcontentbox;
	struct Box_s *rootbox;
	char *tabnames[3] = {_("Friends"), _("Rooms"), _("Games")};
	int i;

	for (i = 0; i < 3; i++)
	{
		tabcontent = NamedList_GetByName(&rostertabcontents, tabnames[i]);

		if (tabcontent)
		{
			tabcontentbox = (*tabcontent)->data;
			rootbox = Box_GetRoot(tabcontentbox);

			if (rootbox == roster_box)
			{
				View_RemoveSavedWindowPos(tabnames[i]);
			}
			else
			{
				if (!IsIconic(rootbox->hwnd))
				{
					View_SetSavedWindowPos(tabnames[i], rootbox->x, rootbox->y, rootbox->w, rootbox->h);
				}
			}

			NamedList_RemoveByName(&rostertabcontents, tabnames[i]);
		}
	}

	if (roster_box)
	{
		if (!IsIconic(roster_box->hwnd) && !Roster_IsNub(roster_box))
		{
			View_SetSavedWindowPos("roster", roster_box->x, roster_box->y, roster_box->w, roster_box->h);
		}
		View_KillTrayIcon();
		Box_Destroy(roster_box);
		roster_box = NULL;
	}
}


void View_SpawnRosterTabWindow(char *name, int x, int y, int w, int h)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, name);
	struct Box_s *tabcontentbox;
	struct Box_s *spawnbox;

	if (!tabcontent)
	{
		return;
	}

	tabcontentbox = (*tabcontent)->data;

	if (!tabcontentbox)
	{
		return;
	}

	spawnbox = Spawn_Create(x, y, w, h, BOX_VISIBLE);

	Box_Unlink(tabcontentbox);
	Spawn_AddTab(spawnbox, name, tabcontentbox);
	Spawn_ActivateFirstTab(spawnbox);

}

#if 0
void View_OnRosterTabDrop(struct Box_s *pbox, char *name, int x, int y)
{
	struct Box_s *root = Box_GetRoot(pbox);

	/* Check if over roster.  If so, remerge to roster */
	if (roster_box && x >= roster_box->x && y >= roster_box->y && x <= roster_box->x + roster_box->w && y <= roster_box->y + roster_box->h)
	{
		if (root != roster_box)
		{
			struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, name);
			struct Box_s *tabcontentbox;

			if (!tabcontent)
			{
				return;
			}

			tabcontentbox = (*tabcontent)->data;

			if (!tabcontentbox)
			{
				return;
			}

			Box_Unlink(tabcontentbox);
			Box_Destroy(root);
			Roster_AddTab(roster_box, name, tabcontentbox);
			Roster_ActivateTab(roster_box, name);
		}
	}
	else
	{
		if (root == roster_box)
		{
			View_SpawnRosterTabWindow(name, x, y, 256, 256);
			Roster_RemoveTab(roster_box, name);
			Roster_ActivateFirstTab(roster_box);
		}
	}
}
#endif

void View_CloseSpawnWindow(struct Box_s *pbox)
{
	struct namedlist_s *tabcontent;
	struct Box_s *tabcontentbox;

	tabcontent = rostertabcontents;

	while (tabcontent)
	{
		tabcontentbox = tabcontent->data;
		if (Box_GetRoot(tabcontentbox) == pbox)
		{
			if (stricmp(tabcontent->name, _("Friends")) == 0)
			{
				Model_SetOption(OPTION_HIDEFRIENDSTAB, 1, NULL);
			}
			else if (stricmp(tabcontent->name, _("Rooms")) == 0)
			{
				Model_SetOption(OPTION_HIDEROOMSTAB, 1, NULL);
			}
			else if (stricmp(tabcontent->name, _("Games")) == 0)
			{
				Model_SetOption(OPTION_HIDEGAMESTAB, 1, NULL);
			}

		}
		tabcontent = tabcontent->next;
	}
}


void View_RemoveAllFriends()
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));
	struct Box_s *friendslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	friendslist_box = (*tabcontent)->data;

	if (friendslist_box)
	{
		List_RemoveAllEntries(friendslist_box);
		List_RedoEntries(friendslist_box);
		List_ScrollToTop(friendslist_box);
		Box_Repaint(friendslist_box);
	}
}

void View_AddFriend(char *jid, char *nickname, char *group, enum SStatus status, char *statusmsg, int inroster)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));
	struct Box_s *friendslist_box;
	struct Box_s *entrybox;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	friendslist_box = (*tabcontent)->data;

	if (!friendslist_box)
	{
		return;
	}

	if (!inroster)
	{
		if (List_GetEntryBox(friendslist_box, jid, group))
		{
			return;
		}
	}

	entrybox = FriendEntry_Create(0, 0, friendslist_box->w, 38, jid, nickname, group, status, statusmsg);
	
	if (!List_GetGroupBox(friendslist_box, group))
	{
		List_AddGroup(friendslist_box, group);
	}

	List_AddEntry(friendslist_box, jid, group, entrybox);

	if (!inroster)
	{
		List_RedoEntries(friendslist_box);
		Box_Repaint(friendslist_box);
	}
}


void View_RemoveFriend(char *jid, char *group, int inroster)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));
	struct Box_s *friendslist_box;

	if (!tabcontent)
	{
		return;
	}

	friendslist_box = (*tabcontent)->data;

	if (!friendslist_box)
	{
		return;
	}

	List_RemoveEntryByName(friendslist_box, jid, group);

	if (!inroster)
	{
		List_RedoEntries(friendslist_box);
		List_ScrollVisible(friendslist_box);
		Box_Repaint(friendslist_box);
	}
}


void View_FinishRoster()
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));
	struct Box_s *friendslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	friendslist_box = (*tabcontent)->data;

	List_RedoEntries(friendslist_box);
	Box_Repaint(friendslist_box);
}


void View_AddGroup(char *name)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));
	struct Box_s *friendslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	friendslist_box = (*tabcontent)->data;

	if (friendslist_box)
	{
		List_AddGroup(friendslist_box, name);
	}

    	List_RedoEntries(friendslist_box);
	Box_Repaint(friendslist_box);

	Model_SetOption(OPTION_SHOWFRIENDGROUPS, 1, NULL);
}

void View_RemoveGroup(char *name)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));
	struct Box_s *friendslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	friendslist_box = (*tabcontent)->data;

	if (friendslist_box)
	{
		List_RemoveGroupByName(friendslist_box, name);
	}

	List_RedoEntries(friendslist_box);
	Box_Repaint(friendslist_box);
}

void View_RenameGroup(char *oldname, char *newname)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));
	struct Box_s *friendslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	friendslist_box = (*tabcontent)->data;

	if (friendslist_box)
	{
		List_RenameGroup(friendslist_box, oldname, newname);
	}

	List_RedoEntries(friendslist_box);
	Box_Repaint(friendslist_box);
}

void View_SetUser(char *jid, char *nick)
{
	if (roster_box)
	{
		Roster_SetUser(roster_box, jid, nick);
	}
}


void View_SetPresence(enum SStatus status, char *statusmsg)
{
	if (roster_box)
	{
		Roster_SetPresence(roster_box, status, statusmsg);
	}
}

void View_SetFriendStatus(char *jid, char *nick, char *group,
	enum SStatus status, char *statusmsg, char *avatarhash, char *rating,
	struct namedlist_s *roles, struct namedlist_s *titles,
	char *gameid, struct gamesearchinfo_s *info, int stopped, int watching,
	struct adhoccommand_s *command)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));

	struct Box_s *friendslist_box;
	struct Box_s *entrybox;

	if (!tabcontent)
	{
		return;
	}

	friendslist_box = (*tabcontent)->data;

	if (!friendslist_box)
	{
		return;
	}

	if (entrybox = List_GetEntryBox(friendslist_box, jid, group))
	{
		FriendEntry_SetStatus(entrybox, nick, status, statusmsg, avatarhash,
			rating, roles, titles, gameid, info, stopped, watching, command);
		List_ReinsertEntry(friendslist_box, jid, group);
		List_RedoEntries(friendslist_box);
		Box_Repaint(friendslist_box);
	}
}

static BOOL tickinit = FALSE;
static int tickcount;

void View_Poll()
{
	if (!tickinit)
	{
		tickcount = GetTickCount();
		tickinit = TRUE;
	}
	else
	{
		int newtickcount = GetTickCount();
		if (newtickcount - tickcount > 1000 / 60)
		{
			ImageMgr_Animate();

			/* currently only the roster box has animation */
			/*
			if (roster_box)
			{
				Box_ForceDraw(roster_box);
			}
			
			if (login_box)
			{
				Box_ForceDraw(login_box);
			}
*/
			tickcount = newtickcount;
		}
	}
}

void View_NubRoster()
{
	if (roster_box)
	{
		Roster_SetNub(roster_box);
		rosternub = TRUE;
	}
}

void View_UnnubRoster()
{
	if (roster_box)
	{
		Roster_UnsetNub(roster_box);
		rosternub = FALSE;
	}
}


void View_RosterAutoReconnect()
{
	if (roster_box)
	{
		Roster_SetReconnect(roster_box);
	}
}


void View_RosterDisconnectError(char *errortext, int notify)
{
	if (roster_box)
	{
		Roster_SetError(roster_box, errortext, notify);
		rosternub = TRUE;
	}
}

void View_SetRosterNormal()
{
	if (roster_box)
	{
		Roster_UnsetNub(roster_box);
	}
}


void View_SetLoginNormal()
{
	if (login_box)
	{
		Login_SetLoginState(login_box);
	}
}


void View_SetLoginConnecting()
{
	if (login_box)
	{
		Login_SetConnectingState(login_box);
	}
}


void View_SetLoginFailed(int badpassword)
{
	if (login_box)
	{
		if (badpassword)
		{
			Login_SetErrorState(login_box, _("The username and/or password aren't correct."), _("Check your password and make sure caps lock isn't on."));
		}
		else
		{
			Login_SetErrorState(login_box, _("Can't connect to server"), _("Check your connection and ensure your username is not misspelled"));
		}
	}
}


void View_SetLoginNoUsername()
{
	if (login_box)
	{
		Login_SetErrorState(login_box, _("The username field is blank."), _("Did you forget to enter a username?"));
	}
}


void View_SetLoginBadUsername()
{
	if (login_box)
	{
		Login_SetErrorState(login_box, _("The username is not a valid JID."), _("Did you remember the @chesspark bit?"));
	}
}


void View_SetLoginNoPassword()
{
	if (login_box)
	{
		Login_SetErrorState(login_box, _("The password field is blank."), _("Did you forget to enter a password?"));
	}
}



void View_AddChatroom(char *group, char *jid, char *name, char *topic, int users)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Rooms"));
	struct Box_s *roomslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	roomslist_box = (*tabcontent)->data;
	
	if (roomslist_box)
	{
		RoomsList_AddChatroom(roomslist_box, group, jid, name, topic, users);
	}
}

void View_RemoveChatroom(char *group, char *jid)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Rooms"));
	struct Box_s *roomslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	roomslist_box = (*tabcontent)->data;
	
	if (roomslist_box)
	{
		RoomsList_RemoveChatroom(roomslist_box, group, jid);
	}
}

void View_SetAvatar(char *filename)
{
	if (roster_box)
	{
		Roster_SetAvatar(roster_box, filename);
	}
}

void View_OnJIDDrop(char *jid, int xmouse, int ymouse)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));
	struct Box_s *friendslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	friendslist_box = (*tabcontent)->data;

	if (friendslist_box && Box_GetRoot(friendslist_box) != friendslist_box && Box_IsVisible(friendslist_box))
	{
		int x, y;
		Box_GetScreenCoords(friendslist_box, &x, &y);

		/* Check if over friends list */
		if (xmouse >= x && ymouse >= y && xmouse <= x + friendslist_box->w && ymouse <= y + friendslist_box->h)
		{
			char *barejid = Jid_Strip(jid);
			Ctrl_AddFriend(barejid, NULL);
			List_OpenGroup(friendslist_box, NULL);
			List_SetSelectionToEntry(friendslist_box, barejid, NULL);
			List_ScrollEntryVisible(friendslist_box, barejid, NULL);
			List_RedoEntries(friendslist_box);
			Box_Repaint(friendslist_box);
			SetFocus(friendslist_box->hwnd);
			free(barejid);
		}
	}
}

void View_ClearChatrooms()
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Rooms"));
	struct Box_s *roomslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	roomslist_box = (*tabcontent)->data;

	if (roomslist_box)
	{
		RoomsList_ClearChatRooms(roomslist_box);
	}
}

void View_SetChatroomLoadStatusFinished()
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Rooms"));
	struct Box_s *roomslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	roomslist_box = (*tabcontent)->data;

	if (roomslist_box)
	{
		RoomsList_SetChatroomLoadStatusFinished(roomslist_box);
	}
}

void View_PopupChessGame(char *gameid, char *roomjid, char *oldgameid)
{
	int x, y, w, h, chatw, group;
	struct Box_s *chessbox;
	struct namedlist_s **ppentry;

	if (gamerespondad_box)
	{
		View_CloseGameRespondAdDialog();
	}

	if (gamereconvene_box)
	{
		View_CloseReconveneDialog();
	}

	Box_Destroy(Box_GetWindowByName("gameadhoc_box"));

	NamedList_RemoveByName(&gamedialoglist, oldgameid);

	if ((ppentry = NamedList_GetByName(&gamedialoglist, gameid)))
	{
		NamedList_RemoveByName(&gamedialoglist, gameid);

#if 0
		chessbox = (*ppentry)->data;
		ShowWindow(chessbox->hwnd, SW_SHOW);

		/* in case we're coming back from a disconnect, reconnect */
		ChessBox_Reconnect(chessbox);
		return;
#endif
	}

	NamedList_RemoveByName(&gamerequestdialoglistbyid, gameid);
	/*View_RefreshGamesListPage();*/

	currentgameid = strdup(gameid);

	{
		int found = View_GetSavedWindowPos2("ChessBox3", &x, &y, &w, &h, &chatw, &group);
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;
		int screenw, screenh;
		int remainw, remainh;

		windowrect.left = roster_box->x;
		windowrect.right = windowrect.left + roster_box->w - 1;
		windowrect.top = roster_box->y;
		windowrect.bottom = windowrect.top + roster_box->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		screenw = mi.rcWork.right - mi.rcWork.left;
		screenh = mi.rcWork.bottom - mi.rcWork.top;

		/* reset chessboard position if partially offscreen */
		if (x < mi.rcWork.left || x + w > mi.rcWork.right || y < mi.rcWork.top || y + h > mi.rcWork.bottom)
		{
			found = 0;
		}

		if (!found || (!chatw && w < 815) || (chatw && w < 490) || h < 360)
		{
			if (roomjid)
			{
				w = 815;
				h = 480;
			}
			else
			{
			}

			remainw = screenw - w;
			remainh = screenh - h;

			x = remainw / 2 + 1;
			y = remainh / 2;

			x += mi.rcWork.left;
			y += mi.rcWork.top;

			chatw = 0;
		}
	}

	chessbox = ChessBox_Create(x, y, w, h, chatw, gameid, roomjid);

	NamedList_Add(&gamedialoglist, gameid, chessbox, Box_Destroy);
}

int View_GameToFront(char *gameid)
{
	struct namedlist_s **entry = NamedList_GetByName(&gamedialoglist, gameid);

	if (entry)
	{
		struct Box_s *chessbox = (*entry)->data;

		BringWindowToTop(chessbox->hwnd);
		return 1;
	}

	return 0;
}

void View_CloseChessGame(char *gameid)
{
	NamedList_RemoveByName(&gamedialoglist, gameid);
}

void View_CloseGamesWithSamePlayers(char *newgameid, char *player1, char *player2)
{
	struct namedlist_s *entry = gamedialoglist;

	while (entry)
	{
		struct namedlist_s *next = entry->next;
		ChessBox_CloseIfSamePlayers(entry->data, newgameid, player1, player2);
		entry = next;
	}
}

void View_CloseAllChessGames()
{
	NamedList_Destroy(&gamedialoglist);
}

void View_ShowGameMessage(char *jid, char *nick, char *gameid, struct gamesearchinfo_s *info, int whitelocal, int blacklocal)
{
	struct namedlist_s **ppchat;
	struct Box_s *chatbox, *subbox;
	char txt[1024];

	ppchat = View_GetPtrToChat(jid, 0);

	if (!ppchat || !*ppchat)
	{
		View_PopupChatDialog(jid, nick, 0);
		ppchat = View_GetPtrToChat(jid, 0);
	}

	if (!ppchat || !*ppchat)
	{
		return;
	}
	
	chatbox = (*ppchat)->data;

	if (!ChatBox_HasChat(chatbox, jid, 0))
	{
		ChatBox_ActivateChat(chatbox, jid, nick, 0, 0);
	}

	if (whitelocal)
	{
		if (info->correspondence && info->blacktomove && info->lastmove)
		{
			char movetxt[120];
			char linktxt[120];

			i18n_stringsub(movetxt, 120, _("Last move was yours, %1"), info->lastmove);
			i18n_stringsub(linktxt, 120, _("^lOpen game %1^l or ^lResign Game %1^l"), gameid);

			sprintf(txt, _("^0v. %s: %s, %s, %s\n"
			               "%s\n"
			               "%s"),
			        Model_GetFriendNick(info->black->jid), info->variant, _("Correspondence"), info->rated ? _("Rated") : _("Unrated"),
			        movetxt, linktxt);
		}
		else if (info->correspondence && info->whitetomove && info->lastmove)
		{
			char movetxt[120];
			char linktxt[120];

			i18n_stringsub(movetxt, 120, _("%1 has moved, %2.  It is now your turn."), Model_GetFriendNick(info->black->jid), info->lastmove);
			i18n_stringsub(linktxt, 120, _("^lOpen game %1^l or ^lResign Game %1^l"), gameid);

			sprintf(txt, _("^0v. %s: %s, %s, %s\n"
			             "%s\n"
			             "%s"),
			        Model_GetFriendNick(info->black->jid), info->variant, _("Correspondence"), info->rated ? _("Rated") : _("Unrated"),
			        movetxt, linktxt);
		}
		else
		{
			char linktxt[120];
			i18n_stringsub(linktxt, 120, _("^lOpen game %1^l or ^lResign Game %1^l"), gameid);

			sprintf(txt, _("^0v. %s: %s, %s, %s\n"
			             "%s"),
			        Model_GetFriendNick(info->black->jid), info->variant, Info_TimeControlsToText(info->timecontrol, info->blacktimecontrol), info->rated ? _("Rated") : _("Unrated"),
			        linktxt);
		}
	}
	else if (blacklocal)
	{
		if (info->correspondence && info->whitetomove && info->lastmove)
		{
			char movetxt[120];
			char linktxt[120];

			i18n_stringsub(movetxt, 120, _("Last move was yours, %1"), info->lastmove);
			i18n_stringsub(linktxt, 120, _("^lOpen game %1^l or ^lResign Game %1^l"), gameid);

			sprintf(txt, _("^0v. %s: %s, %s, %s\n"
			               "%s\n"
			               "%s"),
			        Model_GetFriendNick(info->white->jid), info->variant, _("Correspondence"), info->rated ? _("Rated") : _("Unrated"),
			        movetxt, linktxt);
		}
		else if (info->correspondence && info->blacktomove && info->lastmove)
		{
			char movetxt[120];
			char linktxt[120];

			i18n_stringsub(movetxt, 120, _("%1 has moved, %2.  It is now your turn."), Model_GetFriendNick(info->white->jid), info->lastmove);
			i18n_stringsub(linktxt, 120, _("^lOpen game %1^l or ^lResign Game %1^l"), gameid);

			sprintf(txt, _("^0v. %s: %s, %s, %s\n"
			             "%s\n"
			             "%s"),
			        Model_GetFriendNick(info->white->jid), info->variant, _("Correspondence"), info->rated ? _("Rated") : _("Unrated"),
			        movetxt, linktxt);
		}
		else
		{
			char linktxt[120];
			i18n_stringsub(linktxt, 120, _("^lOpen game %1^l or ^lResign Game %1^l"), gameid);

			sprintf(txt, _("^0v. %s: %s, %s, %s\n"
			             "%s"),
			        Model_GetFriendNick(info->white->jid), info->variant, Info_TimeControlsToText(info->timecontrol, info->blacktimecontrol), info->rated ? _("Rated") : _("Unrated"),
			        linktxt);
		}
	}
	else
	{
		char linktxt[120];
		i18n_stringsub(linktxt, 120, _("^lOpen game %1^l or ^lResign Game %1^l"), gameid);

		sprintf(txt, _("^0%s vs. %s - %s, %s, %s\n%s"),
		        Model_GetFriendNick(info->white->jid), Model_GetFriendNick(info->black->jid),
		        info->variant, Info_TimeControlsToText(info->timecontrol, info->blacktimecontrol),
		        info->rated ? _("Rated") : _("Unrated"), linktxt);
	}

	subbox = Text_Create(32, 8, chatbox->w - 32 - 48, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
	subbox->bgcol = TabBG1;
	subbox->fgcol = TabFG1;
	subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	Text_SetText(subbox, txt);

	if (info->correspondence)
	{
		Text_SetLinkCallback(subbox, 1, ViewLink_CorGameOpen_OnClick, strdup(gameid));
		Text_SetLinkCallback(subbox, 2, ViewLink_CorGameResign_OnClick, strdup(gameid));
	}
	else
	{
		Text_SetLinkCallback(subbox, 1, ViewLink_GameOpen_OnClick, Info_DupeGameSearchInfo(info));
		Text_SetLinkCallback(subbox, 2, ViewLink_GameResign_OnClick, strdup(gameid));
	}

	ChatBox_AddCustom(chatbox, jid, subbox, 1);
}

void View_ParseGameMove(char *gameid, char *move, char *annotation, char *opponentjid, int correspondence, int numtakebacks, int ply)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		if (correspondence)
		{
			struct namedlist_s **ppchat;
			struct Box_s *chatbox, *subbox;
			char txt[256];

			if (!opponentjid)
			{
				i18n_stringsub(txt, 256, _("Game %1"), gameid);
				opponentjid = strdup(txt);
			}

			ppchat = View_GetPtrToChat(opponentjid, 0);

			if (!*ppchat)
			{
				View_PopupChatDialog(opponentjid, Model_GetFriendNick(opponentjid), 0);
				ppchat = View_GetPtrToChat(opponentjid, 0);
			}

			chatbox = (*ppchat)->data;

			if (!ChatBox_HasChat(chatbox, opponentjid, 0))
			{
				ChatBox_ActivateChat(chatbox, opponentjid, Model_GetFriendNick(opponentjid), 0, 0);
			}

			i18n_stringsub(txt, 256, _("%1 has moved, %2.  It is now your turn. ^lOpen game %3^l"), Model_GetFriendNick(opponentjid), move, gameid);

			subbox = Text_Create(32, 8, chatbox->w - 32 - 48, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
			subbox->bgcol = TabBG1;
			subbox->fgcol = TabFG1;
			subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
			Text_SetText(subbox, txt);

			Text_SetLinkCallback(subbox, 1, ViewLink_CorGameOpen_OnClick, strdup(gameid));

			ChatBox_AddCustom(chatbox, opponentjid, subbox, 1);
		}
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_ParseGameMove(chessbox, move, annotation, numtakebacks, ply);
}

void View_ClearMoveList(char *gameid)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_ClearMoveList(chessbox);
}

void View_AddMoveToMoveList(char *gameid, char *move, char *annotation)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_AddMoveToList(chessbox, move, annotation, NULL);
}

void View_SetGameMoveList(char *gameid, struct namedlist_s *textmovelist)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_SetMoveList(chessbox, textmovelist);
}

void View_ResetGamePosition(char *gameid, int illegalmove)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_ResetGamePosition(chessbox, illegalmove);
}

void View_SwitchGameClockToNonlocalPlayer(char *gameid, unsigned int lag)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_SwitchClockToNonLocalPlayer(chessbox, lag);
}


void View_ParseGameState(char *gameid, char *initialstate, char *state)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_SetState(chessbox, initialstate, state);
}

void View_SyncClock(char *gameid, char *time, char *side, char *control, int tick)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_SyncClock(chessbox, time, side, control, tick);
}

void View_HandleFlagError(char *gameid, char *time)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_HandleFlagError(chessbox, time);
}

void View_SetClockControl(char *gameid, char *side, int *controlarray)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_SetClockControl(chessbox, side, controlarray);
}

void View_AddGameChatMessage(char *gameid, char *nick, char *msg)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_AddChatMessage(chessbox, nick, msg);
}

void View_SetGameInfo(char *gameid, struct gamesearchinfo_s *info, int whitelocal, int blacklocal, char *tourneyid)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_SetGameInfo(chessbox, info, whitelocal, blacklocal, tourneyid);

	if (newgames_box && (whitelocal || blacklocal))
	{
		NewGames_ClearAds(newgames_box);
	}
}

void View_HandleAdjourn(char *gameid, char *white, char *black)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_HandleAdjourn(chessbox, white, black);
}

void View_HandleAbort(char *gameid, char *white, char *black)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_HandleAbort(chessbox, white, black);
}

void View_HandleTakeback(char *gameid, char *white, char *black)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_HandleTakeback(chessbox, white, black);
}
void View_HandleDraw(char *gameid, char *from, int whiteaccept,	int blackaccept, int correspondence)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		if (correspondence)
		{
			struct namedlist_s **ppchat;
			struct Box_s *chatbox, *subbox;
			char txt[256];

			if (!from)
			{
				i18n_stringsub(txt, 256, _("Game %1"), gameid);
				from = strdup(txt);
			}

			ppchat = View_GetPtrToChat(from, 0);

			if (!*ppchat)
			{
				View_PopupChatDialog(from, Model_GetFriendNick(from), 0);
				ppchat = View_GetPtrToChat(from, 0);
			}

			chatbox = (*ppchat)->data;

			if (!ChatBox_HasChat(chatbox, from, 0))
			{
				ChatBox_ActivateChat(chatbox, from, Model_GetFriendNick(from), 0, 0);
			}

			i18n_stringsub(txt, 256, _("%1 offers a draw in game %2. ^lOpen game %2^l, ^laccept^l, or ^ldecline^l?"), Model_GetFriendNick(from), gameid);

			subbox = Text_Create(32, 8, chatbox->w - 32 - 48, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
			subbox->bgcol = TabBG1;
			subbox->fgcol = TabFG1;
			subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
			Text_SetText(subbox, txt);

			Text_SetLinkCallback(subbox, 1, ViewLink_CorGameOpen_OnClick, strdup(gameid));
			Text_SetLinkCallback(subbox, 2, ViewLink_CorGameAcceptDraw_OnClick, strdup(gameid));
			Text_SetLinkCallback(subbox, 3, ViewLink_CorGameRejectDraw_OnClick, strdup(gameid));

			ChatBox_AddCustom(chatbox, from, subbox, 1);
		}
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_HandleDraw(chessbox, whiteaccept, blackaccept);
}

void View_HandleRejectAdjourn(char *gameid)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_HandleRejectAdjourn(chessbox);
}

void View_HandleRejectAbort(char *gameid)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_HandleRejectAbort(chessbox);
}

void View_HandleRejectTakeback(char *gameid)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_HandleRejectTakeback(chessbox);
}
void View_HandleRejectDraw(char *gameid, char *from, int correspondence)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		if (correspondence)
		{
			struct namedlist_s **ppchat;
			struct Box_s *chatbox, *subbox;
			char txt[256];

			if (!from)
			{
				i18n_stringsub(txt, 256, _("Game %1"), gameid);
				from = strdup(txt);
			}

			ppchat = View_GetPtrToChat(from, 0);

			if (!*ppchat)
			{
				View_PopupChatDialog(from, Model_GetFriendNick(from), 0);
				ppchat = View_GetPtrToChat(from, 0);
			}

			chatbox = (*ppchat)->data;

			if (!ChatBox_HasChat(chatbox, from, 0))
			{
				ChatBox_ActivateChat(chatbox, from, Model_GetFriendNick(from), 0, 0);
			}

			i18n_stringsub(txt, 256, _("Draw request declined. ^lOpen game %1^l"), gameid);

			subbox = Text_Create(32, 8, chatbox->w - 32 - 48, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
			subbox->bgcol = TabBG1;
			subbox->fgcol = TabFG1;
			subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
			Text_SetText(subbox, txt);

			Text_SetLinkCallback(subbox, 1, ViewLink_CorGameOpen_OnClick, strdup(gameid));

			ChatBox_AddCustom(chatbox, from, subbox, 1);
		}

		return;
	}

	chessbox = (*gamelistentry)->data;

	ChessBox_HandleRejectDraw(chessbox);
}

void View_ShowGameMucError(char *gameid, int icode)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}

	chessbox = (*gamelistentry)->data;

	if (!ChessBox_IsInRoom(chessbox))
	{
		View_CloseChessGame(gameid);

		AutoDialog_Create(roster_box, 400, _("Error: Game does not exist"), _("This game doesn't exist.  Either it is over/adjourned and all players and observers have left, or the game has yet to be played."), NULL, NULL, NULL, NULL, NULL);
	}
	else
	{
		ChessBox_ShowError(chessbox, icode);
	}

	/*ChessBox_ShowGameForbidden(chessbox);*/
}

void View_HandleGameOver(char *gameid, char *type, char *win, char *lose,
  char *reason, int correspondence, char *opponentjid, int localwin,
  int locallose, char *datestarted)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;

	/*View_RefreshGamesListPage();*/

	if (!gamelistentry)
	{
#if 0
		struct namedlist_s **ppchat;
		struct Box_s *chatbox, *subbox;
		char txt[256];

		if (!correspondence)
		{
			return;
		}

		if (!opponentjid)
		{
			i18n_stringsub(txt, 256, _("Game %1"), gameid);
			opponentjid = strdup(txt);
		}

		ppchat = View_GetPtrToChat(opponentjid, 0);

		if (!*ppchat)
		{
			View_PopupChatDialog(opponentjid, Model_GetFriendNick(opponentjid), 0);
			ppchat = View_GetPtrToChat(opponentjid, 0);
		}

		chatbox = (*ppchat)->data;

		if (!ChatBox_HasChat(chatbox, opponentjid, 0))
		{
			ChatBox_ActivateChat(chatbox, opponentjid, Model_GetFriendNick(opponentjid), 0, 0);
		}

		if (correspondence)
		{
			i18n_stringsub(txt, 256, _("^0Correspondence game %1 is over.  Type %2, winner %3, loser %4."), gameid, type, win, lose);
		}
		else
		{
			i18n_stringsub(txt, 256, _("^0Adjourned game %1 is over.  Type %2, winner %3, loser %4."), gameid, type, win, lose);
		}

		subbox = Text_Create(32, 8, chatbox->w - 32 - 48, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
		subbox->bgcol = TabBG1;
		subbox->fgcol = TabFG1;
		subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
		Text_SetText(subbox, txt);

		ChatBox_AddCustom(chatbox, opponentjid, subbox, 1);
#endif
		char txt[1024], txt2[512];

		if (reason && stricmp(reason, "autoresign") == 0)
		{
			if (locallose)
			{
				if (datestarted)
				{
					i18n_stringsub(txt, 1024, _("You have automatically forfeited game #%1 (started %3) against %2."), gameid, Model_GetFriendNick(opponentjid), Info_ConvertTimestampToLongDate(datestarted));
				}
				else
				{
					i18n_stringsub(txt, 1024, _("You have automatically forfeited game #%1 against %2."), gameid, Model_GetFriendNick(opponentjid));
				}
				i18n_stringsub(txt2, 512, _("Reminder: When you log out when a game is running and do not re-login during the timeout period, you forfeit the game"));
				strcat(txt, "\n\n");
				strcat(txt, txt2);

				AutoDialog_Create(roster_box, 400, _("Game Forfeited"), txt, NULL, NULL, NULL, NULL, NULL);
			}
			else if (localwin)
			{
				if (datestarted)
				{
                                        i18n_stringsub(txt, 1024, _("%2 has automatically forfeited game #%1 (started %3) to you.  You win."), gameid, Model_GetFriendNick(opponentjid), Info_ConvertTimestampToLongDate(datestarted));
				}
				else
				{
					i18n_stringsub(txt, 1024, _("%2 has automatically forfeited game #%1 to you.  You win."), Model_GetFriendNick(opponentjid), gameid);
				}

				AutoDialog_Create(roster_box, 400, _("Game won"), txt, NULL, NULL, NULL, NULL, NULL);
			}
		}

		return;
	}

	chessbox = (*gamelistentry)->data;

	ChessBox_HandleGameOver(chessbox, type, win, lose, reason);
}


void View_AddGameTimeNonLocal(char *gameid, int itime)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;

	/*View_RefreshGamesListPage();*/

	if (!gamelistentry)
	{
		return;
	}

	chessbox = (*gamelistentry)->data;

	ChessBox_AddTimeNonLocal(chessbox, itime);
}


void View_ClearGamesSearch(char *node)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Games"));
	struct Box_s *gameslist_box;

	if (newgames_box)
	{
		NewGames_ClearSearch(newgames_box, node);
	}

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	gameslist_box = (*tabcontent)->data;

	if (gameslist_box)
	{
		GamesList_ClearSearch(gameslist_box);
	}
}


void View_AddSearchGame(char *itemid, char *node, char *jid, struct gamesearchinfo_s *info)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Games"));
	struct Box_s *gameslist_box;

	if (newgames_box)
	{
		NewGames_AddSearchGame(newgames_box, itemid, node, jid, info);
	}

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	gameslist_box = (*tabcontent)->data;

	if (gameslist_box)
	{
		GamesList_AddSearchGame(gameslist_box, itemid, node, jid, info);
	}
}

void View_AddSearchTournament(struct tournamentinfo_s *info)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Games"));
	struct Box_s *gameslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	gameslist_box = (*tabcontent)->data;

	if (gameslist_box)
	{
		GamesList_AddSearchTournament(gameslist_box, info);
	}
}

void View_FinishGameResults(int noresults)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Games"));
	struct Box_s *gameslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	gameslist_box = (*tabcontent)->data;

	if (gameslist_box)
	{
		GamesList_FinishGameResults(gameslist_box, noresults);
	}
}

void View_CloseGameRequestDialog(char *jid, char *gameid)
{
	char *barejid = Jid_Strip(jid);

	Log_Write(0, "View_PopupGameInviteFriendDialog(): Removing %s\n", gameid);
	NamedList_RemoveByName(&gamerequestdialoglistbyid, gameid);
	Log_Write(0, "View_PopupGameInviteFriendDialog(): Removing %s\n", barejid);
	NamedList_RemoveByName(&gamerequestdialoglistbyjid, barejid);

	free(barejid);
}

void View_PopupGameInviteFriendDialog(char *jid, struct gamesearchinfo_s *info, int autojoin)
{
	char *barejid = Jid_Strip(jid);
	int replace = 0;

	if (info && info->gameid)
	{
		struct namedlist_s **entry = NamedList_GetByName(&gamerequestdialoglistbyid, info->gameid);

		if (entry)
		{
			replace = 1;
			NamedList_Remove(entry);
		}

		NamedList_Add(&gamerequestdialoglistbyid, info->gameid, GameInviteFriend_Create(roster_box, barejid, info, replace, autojoin), Box_Destroy);
	}
	else
	{
		struct namedlist_s **entry = NamedList_GetByName(&gamerequestdialoglistbyjid, barejid);

		if (entry)
		{
			replace = 1;
			NamedList_Remove(entry);
		}

		NamedList_Add(&gamerequestdialoglistbyjid, barejid, GameInviteFriend_Create(roster_box, barejid, info, replace, autojoin), Box_Destroy);
	}

	free(barejid);
}

void View_PlayNow()
{
	NamedList_Add(&gamerequestdialoglistbyjid, "robopawn@chesspark.com", GameInviteFriend_Create(roster_box, "robopawn@chesspark.com", NULL, 0, 1), Box_Destroy);
}

void View_PopupGameInvitationDialog(char *jid, struct gamesearchinfo_s *info, char *oldgameid)
{
	char *barejid = Jid_Strip(jid);
	int replace = 0;
	int cascade = 0;
	struct namedlist_s *cascadecounter;

	if (!roster_box)
	{
		return;
	}

	if (oldgameid)
	{
		struct namedlist_s **entry = NamedList_GetByName(&gamedialoglist, oldgameid);

		if (entry)
		{
			struct Box_s *chessbox = (*entry)->data;

			NamedList_Add(&forwardgamerequesttogamedialog, info->gameid, strdup(oldgameid), free);

			ChessBox_ShowRematchRequest(chessbox, info);
			return;
		}
	}

	cascadecounter = gamerequestdialoglistbyid;
	while (cascadecounter)
	{
	    cascadecounter = cascadecounter->next;
	    cascade++;
	}

	cascadecounter = gamerequestdialoglistbyjid;
	while (cascadecounter)
	{
	    cascadecounter = cascadecounter->next;
	    cascade++;
	}
	
	if (info && info->gameid)
	{
		struct namedlist_s **entry = NamedList_GetByName(&gamerequestdialoglistbyid, info->gameid);

		if (entry)
		{
			replace = 1;
			NamedList_Remove(entry);
		}

		NamedList_Add(&gamerequestdialoglistbyid, info->gameid, GameInvitation_Create(roster_box, barejid, info, replace, cascade), Box_Destroy);
	}
	else
	{
		struct namedlist_s **entry = NamedList_GetByName(&gamerequestdialoglistbyjid, barejid);

		if (entry)
		{
			replace = 1;
			NamedList_Remove(entry);
		}

		NamedList_Add(&gamerequestdialoglistbyjid, barejid, GameInvitation_Create(roster_box, barejid, info, replace, cascade), Box_Destroy);
	}

	free(barejid);
}

void View_PopupGameCreateAdDialog(struct gamesearchinfo_s *info)
{
	if (!gamecreatead_box && roster_box)
	{
		gamecreatead_box = GameCreateAd_Create(roster_box, info);
	}
}

void View_CloseGameCreateAdDialog()
{
	if (gamecreatead_box)
	{
		Box_Destroy(gamecreatead_box);
		gamecreatead_box = NULL;
	}
}

void View_PopupGameRespondAdDialog(char *from, struct gamesearchinfo_s *info)
{
	if (gamerespondad_box && info && info->node && strcmp(info->node, "quickplay") == 0)
	{
		Box_Destroy(gamerespondad_box);
		gamerespondad_box = NULL;
	}

	if (!gamerespondad_box)
	{
		gamerespondad_box = GameRespondAd_Create(roster_box, from, info);
	}
}

void View_CloseGameRespondAdDialog()
{
	if (gamerespondad_box)
	{
		Box_Destroy(gamerespondad_box);
		gamerespondad_box = NULL;
	}
}

void View_PopupReconveneDialog(char *opponentjid, char *gameid, struct gamesearchinfo_s *info, int inviting)
{
	if (!gamereconvene_box)
	{
		gamereconvene_box = GameReconvene_Create(roster_box, opponentjid, gameid, info, inviting);
	}
}

void View_CloseReconveneDialog()
{
	if (gamereconvene_box)
	{
		Box_Destroy(gamereconvene_box);
		gamereconvene_box = NULL;
	}
}

void View_GameRematchError(char *oldgameid, char *error, char *actor)
{
	struct namedlist_s **entry;
	entry = NamedList_GetByName(&gamedialoglist, oldgameid);

	if (entry)
	{
		struct Box_s *dialog = (*entry)->data;

		ChessBox_SetRematchError(dialog, error, actor);
	}
}

void View_PopupGameInviteError(char *jid, char *error, char *actor, int sent)
{
	struct Box_s *dialog = Box_GetWindowByName("gameinviteerror_box");

	if (dialog)
	{
		Box_Destroy(dialog);
	}

	dialog = GameInviteError_Create(roster_box, jid, error, actor, sent);
	Box_RegisterWindowName(dialog, "gameinviteerror_box");
}

void View_PopupNotAProError(char *error)
{
	char txt[512];
	struct Box_s *dialog;

/*	if (error)
	{
	}
	else
	{*/
	i18n_stringsub(txt, 512, _("You have tried to use a ^bpro-only^n feature!\n\n"
		"To enable this and ^lother great features^l at Chesspark, become a pro by ^lclicking here^l.\n\n"
		"Free accounts are limited to the 4 basic time controls and standard variant games only."));
	
	dialog = AutoDialog_Create(roster_box, 500, _("Not a pro!"), txt, NULL, NULL, NULL, NULL, NULL);
	AutoDialog_SetLinkCallback(dialog, 1, Util_OpenURL2, "http://www.chesspark.com/help/upgrade");
	AutoDialog_SetLinkCallback(dialog, 2, Util_OpenURL2, "http://www.chesspark.com/purchase/");
}

void View_PopupNotAProMucError(char *roomjid)
{
	char txt[512];
	struct Box_s *dialog;
	char *barejid = Jid_Strip(roomjid);
	struct namedlist_s **ppchat = View_GetPtrToChat(barejid, 1);

	if (ppchat && *ppchat)
	{
		struct Box_s *chatbox = (*ppchat)->data;

                if (ChatBox_HasChat(chatbox, barejid, 1))
		{
			ChatBox_CloseChat(chatbox, roomjid);
		}
	}

	i18n_stringsub(txt, 512, _("You have tried to use a ^bpro-only^n feature!\n\n"
		"To enable this and ^lother great features^l at Chesspark, become a pro by clicking ^lhere^l.\n\n"
		"Free accounts are limited to non-pro rooms."));
	
	dialog = AutoDialog_Create(roster_box, 500, _("Not a pro!"), txt, NULL, NULL, NULL, NULL, NULL);
	AutoDialog_SetLinkCallback(dialog, 1, Util_OpenURL2, "http://www.chesspark.com/help/upgrade");
	AutoDialog_SetLinkCallback(dialog, 2, Util_OpenURL2, "http://www.chesspark.com/purchase/");
}

void View_GameRequestError(char *opponentjid, char *gameid, char *error, char *actor, char *adid)
{
	struct namedlist_s **entry;
	char *barejid = Jid_Strip(opponentjid);
/*
	if (oldgameid)
	{
		entry = NamedList_GetByName(&gamedialoglist, oldgameid);

		if (entry)
		{
			struct Box_s *chessbox = (*entry)->data;

			ChessBox_SetRematchError(chessbox, error);
		}
	}
*/
	if (adid)
	{
		char txt[512];
		Box_Destroy(Box_GetWindowByName("gameadhoc_box"));

		if (error)
		{
			if (stricmp(error, "offline") == 0)
			{
				if (actor && actor[0])
				{
					i18n_stringsub(txt, 512, _("Game join cancelled.\n%1 is offline."), Model_GetFriendNick(actor));
				}
				else
				{
					i18n_stringsub(txt, 512, _("Game join cancelled.\nOur data indicates that you are offline.  Please try logging out and in again.  If this problem persists, please submit a problem report or contact a Chesspark representative for assistance."));
				}

				if (newgames_box)
				{
					/*NewGames_RemovePushGame(newgames_box, "play", adid);*/
				}
			}
			else if (stricmp(error, "playing") == 0)
			{
				if (actor && actor[0])
				{
					i18n_stringsub(txt, 512, _("Game join cancelled.\n%1 is playing a game."), Model_GetFriendNick(actor));
				}
				else
				{
					i18n_stringsub(txt, 512, _("Game join cancelled.\nOur data indicates that you are already playing a game.  If this is in error, please submit a problem report or contact a Chesspark representative for assistance."));
				}

				if (newgames_box)
				{
					/*NewGames_RemovePushGame(newgames_box, "play", adid);*/
				}
			}
			else if (stricmp(error, "expired") == 0)
			{
				if (actor && actor[0])
				{
					i18n_stringsub(txt, 512, _("Game join cancelled.\n%1's membership has expired"), Model_GetFriendNick(actor));
				}
				else
				{
					i18n_stringsub(txt, 512, _("Game join cancelled.\nYour membership has expired."));
				}

				if (newgames_box)
				{
					/*NewGames_RemovePushGame(newgames_box, "play", adid);*/
				}
			}
			else if (stricmp(error, "notfound") == 0)
			{
				i18n_stringsub(txt, 512, _("Game join cancelled.\nGame not found.\n\nPlease try another game.  If this problem persists, please submit a problem report or contact a Chesspark representative for assistance."), error);

				if (newgames_box)
				{
					NewGames_RemovePushGame(newgames_box, "play", adid);
				}
			}
			else if (stricmp(error, "notapro") == 0)
			{
				View_PopupNotAProError("finderreply");
				return;
			}
			else if (stricmp(error, "internalservice") == 0)
			{
				i18n_stringsub(txt, 512, _("Internal service error.\n\nPlease try your request again.  If this problem persists, please submit a problem report or contact a Chesspark representative for assistance."), error);
			}
			else
			{
				i18n_stringsub(txt, 512, _("Unknown error: %1.\n\nPlease try your request again.  If this problem persists, please submit a problem report or contact a Chesspark representative for assistance."), error);
			}
		}
		else
		{
			i18n_stringsub(txt, 512, _("Unknown error.\n\nPlease try your request again.  If this problem persists, please submit a problem report or contact a Chesspark representative for assistance."));
		}

		View_GenericError(_("Game join error!"), txt);

		return;
	}

	entry = NamedList_GetByName(&gamerequestdialoglistbyid, gameid);

	if (!entry)
	{
		Log_Write(0, "View_GameRequestError(): %s not found\n", gameid);
		entry = NamedList_GetByName(&gamerequestdialoglistbyjid, barejid);
	}

	if (!entry)
	{
		Log_Write(0, "View_GameRequestError(): %s not found\n", barejid);
	}

	if (entry)
	{
		struct Box_s *dialog = (*entry)->data;

		if (dialog->boxtypeid == BOXTYPE_GAMEINVITEFRIEND)
		{
			Log_Write(0, "View_GameRequestError(): type is BOXTYPE_GAMEINVITEFRIEND");
			if (error && stricmp(error, "notapro") == 0)
			{
				View_PopupNotAProError("gameinviteto");
				GameInviteFriend_Close(dialog);
			}
			else
			{
				GameInviteFriend_SetError(dialog, barejid, error, actor);
			}
		}
		else if (dialog->boxtypeid == BOXTYPE_GAMEINVITATION)
		{
			char *jid;
			Log_Write(0, "View_GameRequestError(): type is BOXTYPE_GAMEINVITATION");
			if (error && stricmp(error, "notapro") == 0)
			{
				View_PopupNotAProError("gameinvitefrom");
				GameInvitation_OnClose(dialog);
			}
			else
			{
				jid = strdup(GameInvitation_GetJid(dialog));
				GameInvitation_OnClose(dialog);
				View_PopupGameInviteError(jid, error, actor, 0);
				free(jid);
			}
			/*GameInvitation_SetError(dialog, barejid, gameid, error);*/
		}
		else
		{
			Log_Write(0, "View_GameRequestError(): type is WTF");
		}
	}

	entry = NamedList_GetByName(&forwardgamerequesttogamedialog, gameid);

	if (entry)
	{
		struct namedlist_s **entry2 = NamedList_GetByName(&gamedialoglist, (*entry)->data);

		if (entry2)
		{
			struct Box_s *dialog = (*entry2)->data;
                        
			ChessBox_SetRematchError(dialog, error, actor);
		}
	}

	if (gamereconvene_box && gameid)
	{
		GameReconvene_SetError(gamereconvene_box, gameid, error, actor);
	}
/*
	if (gamerespondad_box && opponentjid)
	{
		GameRespondAd_SetError(gamerespondad_box, opponentjid, error);
	}
*/
	free(barejid);
}

void View_RequestMatchSetID(char *opponentjid, char *gameid)
{
	struct namedlist_s **entry;
	char *barejid = Jid_Strip(opponentjid);

	entry = NamedList_GetByName(&gamerequestdialoglistbyjid, barejid);

	if (entry)
	{
		struct Box_s *dialog = (*entry)->data;

		if (dialog->boxtypeid == BOXTYPE_GAMEINVITEFRIEND)
		{
			GameInviteFriend_SetGameID(dialog, barejid, gameid);
			NamedList_Unlink(entry);
			NamedList_Add(&gamerequestdialoglistbyid, gameid, dialog, Box_Destroy);
		}
	}

	free(barejid);
}


void View_RematchSetID(char *oldgameid, char *newgameid)
{
	struct namedlist_s **entry;

	Log_Write(0, "View_RematchSetID(%s, %s)\n", oldgameid, newgameid);

	entry = NamedList_GetByName(&gamedialoglist, oldgameid);

	if (entry)
	{
		struct Box_s *dialog = (*entry)->data;
		Log_Write(0, "Set!\n");
		NamedList_Add(&forwardgamerequesttogamedialog, newgameid, strdup(oldgameid), free);
	}
}

void View_ReconveneError(char *gameid, char *error)
{
	if (gamereconvene_box)
	{
		GameReconvene_SetError(gamereconvene_box, gameid, error, NULL);
	}
}

#if 0
void View_SetProfile(char *jid, struct profile_s *profile)
{
	struct namedlist_s *entry;
	/* FIXME: nastyhack */

	entry = gamerequestdialoglistbyid;
	while (entry)
	{
		struct Box_s *dialog = entry->data;

		if (dialog->boxtypeid == BOXTYPE_GAMEINVITATION)
		{
                        GameInvitation_SetProfile(dialog, jid, profile);
		}
		else if (dialog->boxtypeid == BOXTYPE_GAMEINVITEFRIEND)
		{
			GameInviteFriend_SetProfile(dialog, jid, profile);
		}
		entry = entry->next;
	}
	entry = gamerequestdialoglistbyjid;
	while (entry)
	{
		struct Box_s *dialog = entry->data;

		if (dialog->boxtypeid == BOXTYPE_GAMEINVITATION)
		{
                        GameInvitation_SetProfile(dialog, jid, profile);
		}
		else if (dialog->boxtypeid == BOXTYPE_GAMEINVITEFRIEND)
		{
			GameInviteFriend_SetProfile(dialog, jid, profile);
		}
		entry = entry->next;
	}

	if (gamerespondad_box)
	{
		GameRespondAd_SetProfile(gamerespondad_box, jid, profile);
	}

	if (gamereconvene_box)
	{
		GameReconvene_SetProfile(gamereconvene_box, jid, profile);
	}
}
#endif


void View_SetRosterProfile(struct namedlist_s *ratinglist, struct namedlist_s *roleslist, struct namedlist_s *titleslist)
{
	if (roster_box)
	{
		Roster_SetProfile(roster_box, ratinglist, roleslist, titleslist);
	}
}

void View_SetGameViewRotated(char *gameid, int rotated)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_SetGameViewRotated(chessbox, rotated);
}

void View_PopupProfileDialog(char *jid, char *nick)
{
	struct Box_s *dialog = Box_GetWindowByName("profile_box");

	if (dialog)
	{
		Box_Destroy(dialog);
	}

	if (roster_box)
	{
		char *barejid1 = Jid_Strip(jid);
		char *barejid2 = Jid_Strip(Model_GetLoginJid());
		int local = (stricmp(barejid1, barejid2) == 0);

		dialog = Profile_Create(roster_box, jid, nick, local);
		Box_RegisterWindowName(dialog, "profile_box");
	}
}

void View_FriendsOpenAllGroup()
{
	struct namedlist_s **tabcontent;
	tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));
	
	if (tabcontent)
	{
		struct Box_s *friendslist_box = (*tabcontent)->data;
		List_OpenGroup(friendslist_box, NULL);
		List_RedoEntries(friendslist_box);
		Box_Repaint(friendslist_box);
	}
}

void View_SetTotalGamesCount(int count)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Games"));
	struct Box_s *gameslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	gameslist_box = (*tabcontent)->data;

	if (gameslist_box)
	{
		GamesList_SetTotalGamesCount(gameslist_box, count);
	}
}


void View_RefreshGamesListPage(char *node)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Games"));
	struct Box_s *gameslist_box;

	if (newgames_box)
	{
		NewGames_RefreshPage(newgames_box, node);
	}

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	gameslist_box = (*tabcontent)->data;

	if (gameslist_box)
	{
		GamesList_RefreshPage(gameslist_box);
	}
}

void View_AddAdSuccess(char *id)
{
	if (newgames_box)
	{
		NewGames_AddAdSuccess(newgames_box, id);
	}
}

struct Box_s *tourneybox = NULL;
char *currenttourneyid = NULL;

void View_PopupTourneyDialog(char *tourneyid, char *tourneychatjid)
{
	int x, y;

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;

		windowrect.left = roster_box->x;
		windowrect.right = windowrect.left + roster_box->w - 1;
		windowrect.top = roster_box->y;
		windowrect.bottom = windowrect.top + roster_box->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - 660) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - 582) / 2;
	}

	if (tourneybox)
	{
		Box_Destroy(tourneybox);
	}

	currenttourneyid = strdup(tourneyid);
	tourneybox = TourneyBox_Create(x, y, tourneyid, tourneychatjid);
}

void View_CloseTourneyDialog(char *tourneyid)
{
	if (!tourneybox || !currenttourneyid || strcmp(tourneyid, currenttourneyid) != 0)
	{
		return;
	}

	Box_Destroy(tourneybox);
	tourneybox = NULL;
}

void View_AddTourneyChatMessage(char *tourneyid, char *nick, char *msg)
{
	if (!tourneybox || !currenttourneyid || strcmp(tourneyid, currenttourneyid) != 0)
	{
		return;
	}

	TourneyBox_AddChatMessage(tourneybox, nick, msg);
}

void View_SetTourneyChatParticipantStatus(char *jid, enum SStatus status,
  char *statusmsg, char *role, char *affiliation, char *realjid,
  char *nickchange, struct namedlist_s *roleslist,
  struct namedlist_s *titleslist, char *actorjid, char *reason)
{
	char *barejid = Jid_Strip(jid);
	char *nick = Jid_GetResource(jid);
	char *tourneyid = strdup(jid + 10);

	*strchr(tourneyid, '@') = '\0';

	if (!tourneybox || !currenttourneyid || strcmp(tourneyid, currenttourneyid) != 0)
	{
		free(tourneyid);
		free(barejid);
		return;
	}

	TourneyBox_SetParticipantStatus(tourneybox, barejid, nick, status, statusmsg, role, affiliation, realjid, nickchange, roleslist, titleslist);

	free(tourneyid);
	free(barejid);
}

void View_SetTournamentInfo(struct tournamentinfo_s *info)
{
	if (!tourneybox || !currenttourneyid || strcmp(info->id, currenttourneyid) != 0)
	{
		return;
	}

	TourneyBox_SetTournamentInfo(tourneybox, info);

}

void View_PopupRoundStartDialog(char *tourneyid)
{
	RoundStartBox_Create(tourneybox, tourneyid);
}

void View_TournamentAddPlayer(char *tourneyid, char *playerjid)
{
	if (!tourneybox || !currenttourneyid || strcmp(tourneyid, currenttourneyid) != 0)
	{
		return;
	}

	TourneyBox_AddPlayer(tourneybox, playerjid);
}

void View_TournamentRemovePlayer(char *tourneyid, char *playerjid)
{
	if (!tourneybox || !currenttourneyid || strcmp(tourneyid, currenttourneyid) != 0)
	{
		return;
	}

	TourneyBox_RemovePlayer(tourneybox, playerjid);
}

void View_TournamentStartRound(char *tourneyid, int round, struct namedlist_s *pairinglist)
{
	if (!tourneybox || !currenttourneyid || strcmp(tourneyid, currenttourneyid) != 0)
	{
		return;
	}

	TourneyBox_StartRound(tourneybox, round, pairinglist);
}

void View_TournamentEndRound(char *tourneyid, int round)
{
	if (!tourneybox || !currenttourneyid || strcmp(tourneyid, currenttourneyid) != 0)
	{
		return;
	}

	TourneyBox_EndRound(tourneybox, round);
}

void View_TournamentEnd(char *tourneyid, char *winner)
{
	if (!tourneybox || !currenttourneyid || strcmp(tourneyid, currenttourneyid) != 0)
	{
		return;
	}

	TourneyBox_End(tourneybox, winner);
}

void View_TournamentGamePlaying(char *tourneyid, char *gameid, int round, char *white, char *black)
{
	if (!tourneybox || !currenttourneyid || strcmp(tourneyid, currenttourneyid) != 0)
	{
		return;
	}

	TourneyBox_GamePlaying(tourneybox, gameid, round, white, black);
}

void View_TournamentGameStoppedPlaying(char *tourneyid, char *gameid, int round, char *white, char *black, char *winner)
{
	if (!tourneybox || !currenttourneyid || strcmp(tourneyid, currenttourneyid) != 0)
	{
		return;
	}

	TourneyBox_GameStoppedPlaying(tourneybox, gameid, round, white, black, winner);
}

void View_TournamentUpdatePlayer(char *tourneyid, char *jid, struct tournamentplayerinfo_s *pinfo)
{
	if (!tourneybox || !currenttourneyid || strcmp(tourneyid, currenttourneyid) != 0)
	{
		return;
	}

	TourneyBox_UpdatePlayer(tourneybox, jid, pinfo);
}

void GameLogin_OnClick(struct Box_s *pbox, void *userdata)
{
	Ctrl_LoginToGameServer();
}


void View_ShowGameLogoutMessage(char *jid, char *nick)
{
	struct Box_s *dialog = AutoDialog_Create(roster_box, 500, _("Logged out of chesspark!"), _("You have been logged out of the Chesspark game server!\n"
	  "You will be able to chat, but you will not be able to participate in games and tournaments without logging in again.\n"
	  "Click ^lhere^l to log in again."), NULL, NULL, NULL, NULL, NULL);

	AutoDialog_SetLinkCallback(dialog, 1, GameLogin_OnClick, NULL);
}

void SetLowBandwidth(struct Box_s *pbox, void *userdata)
{
	Model_SetOption(OPTION_LOWBANDWIDTH, 1, NULL);
}

void ReportFailedLogin(struct Box_s *pbox, void *userdata)
{
	Ctrl_PostFullProblemReport("Technical Issues", "Login failed, please check log", 1);
}

void GameLogin_OnClickAndClose(struct Box_s *pbox, void *userdata)
{
	GameLogin_OnClick(pbox, userdata);
	Box_Destroy(Box_GetRoot(pbox));
}

void View_ShowGameLoginFailed(char *jid, char *nick)
{
	struct Box_s *dialog = AutoDialog_Create(roster_box, 500, _("Chesspark login failed!"), _("You have been logged out of the Chesspark game server!\n"
	  "You will be able to chat, but you will not be able to participate in games and tournaments without logging in.\n\n"
	  "This may be caused by slow internet speeds.  To set this client to low bandwidth mode, click ^lhere^l.  This will "
	  "disable certain high bandwidth features, such as avatars and automatic updates.\n\n"
	  "We recommend sending a problem report so we can better diagnose the problem.\n\n"
	  "Click ^lhere^l to send a problem report, and/or OK to try to log in again."),
	  _("OK"), NULL, GameLogin_OnClickAndClose, NULL, NULL);

	AutoDialog_SetLinkCallback(dialog, 1, SetLowBandwidth, NULL);
	AutoDialog_SetLinkCallback(dialog, 2, ReportFailedLogin, NULL);
}

void View_ShowMemberExpired()
{
	struct Box_s *dialog = AutoDialog_Create(roster_box, 500, _("Chesspark Account Expired"),
	  _("Your account has ^bEXPIRED.^b  Until you update your account information, you will not be able to log in.\n\n"
	  "^bTo resume full access to Chesspark, ^n^lupdate your account information.^l.\nThanks!"),
	  NULL, NULL, NULL, NULL, NULL);

	AutoDialog_SetLinkCallback(dialog, 1, ViewLink_OpenURLNoTopmost, strdup(_("http://chesspark.com/people/edit/account/")));

	SetWindowPos(dialog->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
}

void View_ShowMemberNotFound()
{
	struct Box_s *dialog = AutoDialog_Create(roster_box, 500, _("Join Chesspark Today"),
	  _("You can use Chesspark as a regular chat client with your existing Jabber ID.\n\n"
	  "If you'd like access to all of Chesspark's features,^n ^lregister a free trial account.^l\n"
	  "It's fast, easy, and did we mention it's free?"),
	  NULL, NULL, NULL, NULL, NULL);

	AutoDialog_SetLinkCallback(dialog, 1, Util_OpenURL2, strdup(_("http://chesspark.com")));
}

void View_ShowMemberLoginsExceeded()
{
	struct Box_s *dialog = AutoDialog_Create(roster_box, 500, _("Chesspark logins exceeded"), _("You appear to be logged into Chesspark from too many different places.\n\n"
	"If you are unable to log out your other locations, try contacting a staff member for assistance."),
	NULL, NULL, NULL, NULL, NULL);
}

void View_ShowMemberCancelled()
{
	struct Box_s *dialog = AutoDialog_Create(roster_box, 500, _("Chesspark Account Cancelled"),
	  _("Your account has been ^bCANCELLED.^b  Until you update your account information, you will not be able to log in.\n\n"
	  "^bTo resume full access to Chesspark, ^n^lupdate your account information.^l.\nThanks!"),
	  NULL, NULL, NULL, NULL, NULL);

	AutoDialog_SetLinkCallback(dialog, 1, ViewLink_OpenURLNoTopmost, strdup(_("http://chesspark.com/people/edit/account/")));

	SetWindowPos(dialog->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
}


void View_PopupInviteFriendToChessparkDialog(char *jid)
{
	char txt[512];

	sprintf(txt, _("Want to play some chess?  Join Chesspark!  Please visit http://www.chesspark.com ."));

	View_PopupChatDialog(jid, Model_GetFriendNick(jid), 0);
	View_ActivateChatDialog(jid, Model_GetFriendNick(jid), 0);
	View_AddChatMessage(jid, Model_GetLoginJid(), txt);
	Ctrl_SendMessage(jid, txt);

	/*
	char invitetitle[256];

	sprintf(invitetitle, "Invite %s to Chesspark", Model_GetFriendNick(jid));
	
	AutoDialog_Create(roster_box, 500, invitetitle, "Inviting people to "
		"chesspark is easy!\n\nSince they already have a jabber "
		"account, they don't even need to sign up on our site.  All "
		"they need to do is sign on to their regular jabber account,"
		" using our clean, easy to use client or simple, small "
		"webclient.  Just push the OK button, and we'll "
		"automatically give them a trial account to play with, and "
		"send them a link to access Chesspark.\n\nOr we would, if "
		"our coders knew how to do that.", "D'oh!", NULL, NULL, NULL,
		NULL);
	*/
}

void View_PopupInviteToChatDialog(char *friendjid, char *chatjid)
{
	struct Box_s *dialog = Box_GetWindowByName("invitetochat_box");

	if (dialog)
	{
		Box_Destroy(dialog);
	}

	dialog = InviteToChat_Create(roster_box, friendjid, chatjid);
	Box_RegisterWindowName(dialog, "invitetochat_box");
}


void ChatInvite_OnClick(struct Box_s *pbox, char *chatjid)
{
	Ctrl_JoinGroupChatDefaultNick(chatjid);
}

void View_ShowChatInviteMessage(char *friendjid, char *chatjid)
{
	struct namedlist_s **ppchat;
	struct Box_s *chatbox, *subbox;
	char txt[1024];
	char unescaped[1024];

	ppchat = View_GetPtrToChat(friendjid, 0);

	if (!ppchat || !*ppchat)
	{
		View_PopupChatDialog(friendjid, Model_GetFriendNick(friendjid), 0);
		ppchat = View_GetPtrToChat(friendjid, 0);
	}

	if (!ppchat || !*ppchat)
	{
		return;
	}
	
	chatbox = (*ppchat)->data;

	if (!ChatBox_HasChat(chatbox, friendjid, 0))
	{
		ChatBox_ActivateChat(chatbox, friendjid, Model_GetFriendNick(friendjid), 0, 0);
	}

	{
		char subtxt[256];

		i18n_stringsub(subtxt, 256, _("%1 has invited you to chat in ^l%2^l"), Model_GetFriendNick(friendjid), UnescapeJID(chatjid, unescaped, 1024));
		sprintf(txt, "^0%s\n%s", subtxt, _("Click ^lhere^l to join this chat."));
	}

	subbox = Text_Create(32, 8, chatbox->w - 32 - 48, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
	subbox->bgcol = TabBG1;
	subbox->fgcol = TabFG1;
	subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	Text_SetText(subbox, txt);

	Text_SetLinkCallback(subbox, 1, ChatInvite_OnClick, strdup(chatjid));
	Text_SetLinkCallback(subbox, 2, ChatInvite_OnClick, strdup(chatjid));

	ChatBox_AddCustom(chatbox, friendjid, subbox, 1);
}

static struct viewlinkwatchgameinviteinfo_s
{
	char *chatjid;
	char *friendjid;
};

void View_SetGameViewRotatedIfBlack(char *gameid, char *blackjid)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_SetGameViewRotatedIfBlack(chessbox, blackjid);
}

void ViewLink_WatchGameInvite_OnClick(struct Box_s *pbox, struct viewlinkwatchgameinviteinfo_s *info)
{
	char gamejid[256];
	char *pipe;

	strcpy(gamejid, info->chatjid);
	if (pipe = strchr(gamejid, '|'))
	{
	    *pipe = '\0';
	}

	Ctrl_WatchGame(gamejid, 0);

	if (pipe = strchr(gamejid, '@'))
	{
	    *pipe = '\0';
	}

	View_SetGameViewRotatedIfBlack(gamejid, info->friendjid);
}

void View_ShowGameWatchInviteMessage(char *friendjid, char *chatjid)
{
	struct namedlist_s **ppchat;
	struct Box_s *chatbox, *subbox;
	char *gameid = Jid_GetBeforeAt(chatjid);
	char txt[1024];

	ppchat = View_GetPtrToChat(friendjid, 0);

	if (!ppchat || !*ppchat)
	{
		View_PopupChatDialog(friendjid, Model_GetFriendNick(friendjid), 0);
		ppchat = View_GetPtrToChat(friendjid, 0);
	}

	if (!ppchat || !*ppchat)
	{
		return;
	}
	
	chatbox = (*ppchat)->data;

	if (!ChatBox_HasChat(chatbox, friendjid, 0))
	{
		ChatBox_ActivateChat(chatbox, friendjid, Model_GetFriendNick(friendjid), 0, 0);
	}

	{
		char subtxt[256];

		i18n_stringsub(subtxt, 256, _("%1 has invited you to watch game %2"), Model_GetFriendNick(friendjid), gameid);
		sprintf(txt, "^0%s\n%s", subtxt, _("Click ^lhere^l to watch this game."));
	}

	subbox = Text_Create(32, 8, chatbox->w - 32 - 48, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
	subbox->bgcol = TabBG1;
	subbox->fgcol = TabFG1;
	subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	Text_SetText(subbox, txt);

	{
		struct viewlinkwatchgameinviteinfo_s *info = malloc(sizeof(*info));

		info->chatjid = strdup(chatjid);
		info->friendjid = strdup(friendjid);

		Text_SetLinkCallback(subbox, 1, ViewLink_WatchGameInvite_OnClick, info);
	}

	ChatBox_AddCustom(chatbox, friendjid, subbox, 1);
}

static void LaunchUpdater(struct Box_s *dummy1, void *dummy2)
{
	Model_LaunchUpdater();
}

void View_ShowClientUpgradeMessage(char *from, char *number, char *version, char *build, char *url)
{
	char txt[1024];
	struct Box_s *upgrade_box = NULL;

	if (upgrade_box)
	{
		Box_Destroy(upgrade_box);
	}

	if (!version)
	{
		version = "";
	}

	i18n_stringsub(txt, 1024,
	  _("A new version of Chesspark has been automatically "
	  "downloaded and is ready to install.\n\n"
	  "Current version: %1 build %2\n"
	  "Latest version: %3 %4 build %5\n\n"
	  "If you wish to install it now, click Restart and Chesspark "
	  "will restart and update itself.  If you wish to update at the "
	  "next client restart, click Not Now."),
	  CHESSPARK_VERSION, CHESSPARK_BUILD,
	  number, version, build);

	upgrade_box = AutoDialog_Create(roster_box, 500, _("New Chesspark "
		"Client downloaded!"), txt, "Not Now", "Restart", NULL, LaunchUpdater, NULL);

	SetWindowPos(upgrade_box->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
}

void View_ShowUpgradedMessage()
{
	struct Box_s *upbox2;
	char txt[1024];

	i18n_stringsub(txt, 1024,
	  _("You are now running %1 build %2.\n\n"
	  "To see a full changelog, click ^lhere^l"), CHESSPARK_VERSION, CHESSPARK_BUILD);
	upbox2 = AutoDialog_Create(roster_box, 500, _("Chesspark Client update successful!"), txt, NULL, NULL, NULL, NULL, NULL);
	AutoDialog_SetLinkCallback(upbox2, 1, Util_OpenURL2, "http://www.chesspark.com/changes/desktop/");
}


struct namedlist_s *View_GetOpenGames()
{
	struct namedlist_s *entry;
	struct namedlist_s *dstlist = NULL;

	entry = gamedialoglist;

	while (entry)
	{
		struct Box_s *chessbox = entry->data;
		struct gamesearchinfo_s *info = ChessBox_GetGameInfo(chessbox);

		NamedList_Add(&dstlist, entry->name, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);

		entry = entry->next;
	}

	return dstlist;
}

struct Box_s *loginmessage_box = NULL;
int localadjournedgamesnum = 0;
int localcorrespondencegamesnum = 0;

void LoginMessage_OnDestroy(struct Box_s *pbox)
{
	loginmessage_box = NULL;
}

void LoginMessage_Quickmatch(struct Box_s *pbox, void *userdata)
{
	ViewLink_Quickmatch(pbox, NULL);
	Box_Destroy(Box_GetRoot(pbox));
}

void View_UpdateLoginMessage()
{
	char txt[1024];

	strcpy(txt, _("Welcome to Chesspark!\n\nIf you're new here, you may want to try the ^lGame Finder^l.\n"));

	if (localadjournedgamesnum)
	{
		strcat(txt, _("\nYou have adjourned games to play!  Click ^lhere^l to view them in the game finder and see which opponents are online.\n"));
	}
#ifdef CHESSPARK_CORRESPONDENCE
	if (localcorrespondencegamesnum)
	{
		strcat(txt, _("\nYou have correspondence games to play!  Click ^lhere^l to view them in your roster.\n"));
	}
#endif

	if (Model_IsLocalMemberFree(0))
	{
		strcat(txt, _("\nYou currently have a ^bfree account^n. To participate in tournaments, groups, and to join the pro room, you should consider upgrading to a pro account. Click ^lhere^l for details."));
	}

	if (loginmessage_box)
	{
		int linknum = 2;
		AutoDialog_ResetText(loginmessage_box, _("Welcome to Chesspark!"), txt);
		AutoDialog_SetLinkCallback(loginmessage_box, 1, ViewLink_ShowGameFinder, NULL);

		if (localadjournedgamesnum)
		{
			AutoDialog_SetLinkCallback(loginmessage_box, linknum++, ViewLink_ShowAdjournedGames, NULL);
		}
		
		if (localcorrespondencegamesnum)
		{
			AutoDialog_SetLinkCallback(loginmessage_box, linknum++, ViewLink_ShowCorrespondenceGames, NULL);
		}

		if (Model_IsLocalMemberFree(0))
		{
			AutoDialog_SetLinkCallback(loginmessage_box, linknum++, Util_OpenURL2, "http://www.chesspark.com/help/upgrade/");
		}
	}
}

void LoginMessage_OnDontShow(struct Box_s *pbox, char *name)
{
	Model_SetOption(OPTION_HIDEWELCOMEDIALOG, 1, NULL);
}

void View_ShowLoginMessage()
{
	if (roster_box && !Model_GetOption(OPTION_HIDEWELCOMEDIALOG))
	{
		char txt[1024];

		strcpy(txt, _("Welcome to Chesspark!\n\nIf you're new here, you may want to try the ^lGame Finder^l.\n"));

		loginmessage_box = AutoDialog_Create2(roster_box, 500, _("Welcome to Chesspark!"), txt, NULL, NULL, NULL, NULL, NULL, LoginMessage_OnDontShow, "InitialLoginDialog");
		AutoDialog_SetLinkCallback(loginmessage_box, 1, ViewLink_ShowGameFinder, NULL);

		if (localadjournedgamesnum || localcorrespondencegamesnum || Model_IsLocalMemberFree(0))
		{
			View_UpdateLoginMessage();
		}
	
		loginmessage_box->OnDestroy = LoginMessage_OnDestroy;
	}
}



void View_SetAdjournedCount(int adjourned)
{
	localadjournedgamesnum = adjourned; 

	View_UpdateLoginMessage();

	{
		struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Games"));
		struct Box_s *gameslist_box;

		if (!tabcontent || !*tabcontent)
		{
			return;
		}

		gameslist_box = (*tabcontent)->data;

		/*GamesList_UpdateAdjournedCount(gameslist_box, adjourned);*/
	}
}

void View_SetCorrespondenceCount(int correspondence)
{
	localcorrespondencegamesnum = correspondence; 

	View_UpdateLoginMessage();
}

struct Box_s *announcementbox = NULL;

void AnnouncementBox_OnClose(struct Box_s *dialog, void *userdata)
{
	Box_Destroy(announcementbox);
	announcementbox = NULL;
}

void View_ShowAnnouncement(char *from, char *text, char *subject)
{
	char txt[1024];
	char txt2[1024];

	i18n_stringsub(txt, 1024, _("Announcement from %1"), from);
	if (subject)
	{
		sprintf(txt2, "%s - %s", txt, subject);
	}
	else
	{
		strcpy(txt2, txt);
	}

	if (announcementbox)
	{
		Box_Destroy(announcementbox);
	}

	CornerPop_Create(roster_box, 300, txt2, text);

	/*announcementbox = AutoDialog_Create(roster_box, 500, txt2, text, NULL, NULL, AnnouncementBox_OnClose, NULL, NULL);*/
}

void View_ShowCorGameAccepted(char *gameid, char *from)
{
	struct namedlist_s **entry;

	entry = NamedList_GetByName(&gamerequestdialoglistbyid, gameid);

	if (!entry)
	{
		char *barejid = Jid_Strip(from);
		entry = NamedList_GetByName(&gamerequestdialoglistbyjid, barejid);
		free(barejid);
	}

	if (entry)
	{
		struct Box_s *dialog = (*entry)->data;

		if (dialog->boxtypeid == BOXTYPE_GAMEINVITEFRIEND)
		{
			GameInviteFriend_ShowCorGameAccepted(dialog, gameid);
		}
		else if (dialog->boxtypeid == BOXTYPE_GAMEINVITATION)
		{
			GameInvitation_ShowCorGameAccepted(dialog, gameid);
		}
	}
	else
	{
		struct namedlist_s **ppchat;
		struct Box_s *chatbox, *subbox;
		char *nick = Model_GetFriendNick(from);
		char txt[1024];

		ppchat = View_GetPtrToChat(from, 0);

		if (!ppchat || !*ppchat)
		{
			View_PopupChatDialog(from, nick, 0);
			ppchat = View_GetPtrToChat(from, 0);
		}

		if (!ppchat || !*ppchat)
		{
			return;
		}
	
		chatbox = (*ppchat)->data;

		if (!ChatBox_HasChat(chatbox, from, 0))
		{
			ChatBox_ActivateChat(chatbox, from, nick, 0, 0);
		}

	{
		char subtxt1[256];
		char subtxt2[256];

		i18n_stringsub(subtxt1, 256, _("Correspondence game with %1 has started!"), Model_GetFriendNick(from));
		i18n_stringsub(subtxt2, 256, _("Click ^lhere^l to open game#%1"), gameid);
		sprintf(txt, "^0%s\n%s", subtxt1, subtxt2);
	}

		subbox = Text_Create(32, 8, chatbox->w - 32 - 48, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
		subbox->bgcol = TabBG1;
		subbox->fgcol = TabFG1;
		subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
		Text_SetText(subbox, txt);

		Text_SetLinkCallback(subbox, 1, ViewLink_CorGameOpen_OnClick, strdup(gameid));

		ChatBox_AddCustom(chatbox, from, subbox, 1);
	}
}

void View_ResetLanguage(char *langcode)
{
	View_CloseAllChatDialogs();
	View_CloseLogin();
	i18n_ResetLanguage(langcode);
	View_PopupLogin();
	Menu_Init();
}

void View_PopupBanlist(char *jid, struct namedlist_s *banlist)
{
	Banlist_Create(roster_box, jid, banlist);
}

void View_GenericError(char *errortitle, char *error)
{
	AutoDialog_Create(roster_box, 500, errortitle, error, NULL, NULL, NULL, NULL, NULL);
}

struct Box_s *pingdialog = NULL;

void PopupPingDialog_Ping(char *jid, void *userdata)
{
	if (!jid || (jid && jid[0] == '\0'))
	{
                Ctrl_Ping(NULL, 1, 0);
	}
	else
	{
		Ctrl_Ping(jid, 1, 0);
	}
}

void View_PopupPingDialog()
{
	pingdialog = AutoEdit_Create(roster_box, 600, "Ping a peer", "Enter a jabber entity here:", NULL, "match.chesspark.com", PopupPingDialog_Ping, NULL, "Ping!");
}

struct Box_s *optionsdialog = NULL;

void View_PopupOptionsDialog()
{
	if (optionsdialog)
	{
	}
	else
	{
		optionsdialog = Options_Create(roster_box);
	}
}

void View_CloseOptionsDialog()
{
	if (optionsdialog)
	{
		Box_Destroy(optionsdialog);
		optionsdialog = NULL;
	}
}

struct baninfo_s
{
	char *muc;
	char *target;
};

void BanDialog_OnBan(char *reason, struct baninfo_s *info)
{
	Ctrl_BanUser(info->muc, info->target, reason);
	free(info);
}

void View_PopupBanDialog(char *muc, char *target)
{
	char titletxt[120];
	struct baninfo_s *info;

	info = malloc(sizeof(*info));
	info->muc = strdup(muc);
	info->target = strdup(target);
	
	i18n_stringsub(titletxt, 120, _("Ban %1 from %2"), Model_GetFriendNick(target), muc);
	AutoEdit_Create(roster_box, 600, titletxt, "Enter reason here, or blank for none.", NULL, NULL, BanDialog_OnBan, info, "Ban");
}

void IgnoreDialog_OnIgnore(struct Box_s *dialog, char *target)
{
	Ctrl_Ignore(target);
	Box_Destroy(Box_GetRoot(dialog));
}

void IgnoreDialog_OnCancel(struct Box_s *dialog, char *target)
{
	Box_Destroy(Box_GetRoot(dialog));
}

void View_PopupIgnoreDialog(char *target)
{
	char titletxt[120];
	char ignoretxt[512];
	char *barejid, *bareloginjid;

	i18n_stringsub(titletxt, 120, "Ignore %1", Model_GetFriendNick(target));

	barejid = Jid_Strip(target);
	bareloginjid = Jid_Strip(Model_GetLoginJid());

	if (stricmp(barejid, bareloginjid) == 0)
	{
		i18n_stringsub(ignoretxt, 512, "You cannot ignore yourself!");

		AutoDialog_Create(roster_box, 500, titletxt, ignoretxt, NULL, NULL, NULL, NULL, NULL);
	}
	else
	{
		i18n_stringsub(ignoretxt, 512, "Are you sure that you want to ignore ^b%1^b?\n\nIgnoring a user means you can no longer see that user's online status, and will no longer see messages nor game requests from them.", Model_GetFriendNick(target));

		AutoDialog_Create(roster_box, 500, titletxt, ignoretxt, "Cancel", "OK", IgnoreDialog_OnCancel, IgnoreDialog_OnIgnore, strdup(target));
	}

	free(barejid);
	free(bareloginjid);
}

void View_ShowRosterLoading()
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));
	struct Box_s *friendslist_box;
	struct Box_s *entrybox;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	friendslist_box = (*tabcontent)->data;

	if (!friendslist_box)
	{
		return;
	}

	List_RemoveAllEntries(friendslist_box);

	entrybox = Box_Create(0, 0, friendslist_box->w, 38, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
	Box_SetText(entrybox, _("Loading roster..."));
	
	List_AddEntry(friendslist_box, NULL, NULL, entrybox);

	List_RedoEntries(friendslist_box);
	Box_Repaint(friendslist_box);
}

void AdhocDialog_Close(struct Box_s *box, void *userdata)
{
	Box_Destroy(Box_GetWindowByName("gameadhoc_box"));
}

void View_PopupGameWaitingDialog(char *titletext, char *dialogtext)
{
	struct Box_s *dialog = Box_GetWindowByName("gameadhoc_box");

	if (dialog)
	{
		Box_Destroy(dialog);
	}

	dialog = AutoDialog_Create(roster_box, 400, titletext, dialogtext, _("Close"), NULL, AdhocDialog_Close, NULL, NULL);
	Box_RegisterWindowName(dialog, "gameadhoc_box");
}

void View_ScrollFriendVisible(char *jid)
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Friends"));
	struct Box_s *friendslist_box;

	if (!tabcontent || !*tabcontent)
	{
		return;
	}

	friendslist_box = (*tabcontent)->data;

	if (!friendslist_box)
	{
		return;
	}

	List_OpenGroup(friendslist_box, NULL);
	List_ScrollEntryVisible(friendslist_box, jid, NULL);
}

void View_PopupSendDebugLogDialog()
{
	AutoEdit_Create(roster_box, 400, "Submit debug log", "Please enter the reason for submitting this log:", NULL, NULL, Ctrl_PostDebugLog, NULL, "Send");
}


void View_PopupNewGamesDialog()
{
	if (!newgames_box && roster_box)
	{
		int x, y, w, h;
		int found = View_GetSavedWindowPos("GameFinder", &x, &y, &w, &h);

		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;
		int screenw, screenh;
		int remainw, remainh;

		windowrect.left = roster_box->x;
		windowrect.right = windowrect.left + roster_box->w - 1;
		windowrect.top = roster_box->y;
		windowrect.bottom = windowrect.top + roster_box->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		screenw = mi.rcWork.right - mi.rcWork.left;
		screenh = mi.rcWork.bottom - mi.rcWork.top;

		if (!found || ((w < 800 && h < 600) && !Model_GetOption(OPTION_HIDEGAMEFINDERHELP)))
		{
			if (screenw >= 800 && screenh >= 600)
			{
				w = 800;
				h = 600;
			}
			else
			{
				w = 640;
				h = 480;
			}

			remainw = screenw - w;
			remainh = screenh - h;

			x = remainw / 2 + 1;
			y = remainh / 2;

			x += mi.rcWork.left;
			y += mi.rcWork.top;
		}

		/* check to make sure at least the titlebar is on screen */
		if (y < mi.rcWork.top)
		{
			y = mi.rcWork.top;
		}

		newgames_box = NewGames_Create(x, y, w, h, roster_box);

		Roster_SetNewGamesButtonActive(roster_box, 1);
	}
}

void View_RefreshNewGamesDialog()
{
	if (newgames_box)
	{
		NewGames_RefreshCurrentPage(newgames_box);
	}
}

void View_CloseNewGamesDialog()
{
	if (newgames_box)
	{
		Box_Destroy(newgames_box);
		newgames_box = NULL;
	}

	if (roster_box)
	{
		Roster_SetNewGamesButtonActive(roster_box, 0);
	}
}

void View_ToggleNewGamesDialog()
{
	if (newgames_box)
	{
		View_CloseNewGamesDialog();
		Model_SetOption(OPTION_NOGAMESEARCHONLOGIN, 1, NULL);

	}
	else
	{
		View_PopupNewGamesDialog();
		Model_SetOption(OPTION_NOGAMESEARCHONLOGIN, 0, NULL);
	}
}

struct Box_s *View_GetGamesListBox()
{
	struct namedlist_s **tabcontent = NamedList_GetByName(&rostertabcontents, _("Games"));

	if (!tabcontent || !*tabcontent)
	{
		return NULL;
	}

	return (*tabcontent)->data;
}


void View_AddPushGame(char *type, char *id, struct gamesearchinfo_s *info)
{
	if (newgames_box)
	{
		NewGames_AddPushGame(newgames_box, type, id, info);
	}
}

void View_RemovePushGame(char *type, char *id)
{
	if (newgames_box)
	{
		NewGames_RemovePushGame(newgames_box, type, id);
	}
}

void View_ClearPushGames()
{
	if (newgames_box)
	{
		NewGames_ClearPushGames(newgames_box);
	}
}

void View_KillGameAds()
{
	if (newgames_box)
	{
		NewGames_KillGameAds(newgames_box);
	}
}

void View_SetGameFinderExpiryReminder(int expiry)
{
	if (newgames_box)
	{
		NewGames_SetGameFinderExpiryReminder(newgames_box, expiry);
	}
}

void View_UpdateGameFinderGroups()
{
	if (newgames_box)
	{
		NewGames_UpdateFilters(newgames_box);
	}
}

int View_IsPlayingAGame()
{
	struct namedlist_s *entry;
	struct namedlist_s *dstlist = NULL;

	entry = gamedialoglist;

	while (entry)
	{
		struct Box_s *chessbox = entry->data;

		if (ChessBox_IsPlaying(chessbox))
		{
			return 1;
		}

		entry = entry->next;
	}

	return 0;
}

void QuitWarning_ReallyQuit(struct Box_s *pbox, void *userdata)
{
	View_CloseAllDialogs();
	PostQuitMessage(0);
}

void View_PopupQuitWarning()
{
	AutoDialog_Create(roster_box, 500, "Game warning", "You are still in a game!\n\nIf you close Chesspark now and do not log in within the timeout period, you will ^bFORFEIT^b your game.\n\nTo close Chesspark, click OK.  To go back, click Cancel.", "Cancel", "OK", NULL, QuitWarning_ReallyQuit, NULL);
}

void OfflineWarning_ReallyOffline(struct Box_s *pbox, void *userdata)
{
	Model_ReallyDisconnect();
	Box_Destroy(Box_GetRoot(pbox));
}

void View_PopupOfflineWarning()
{
	AutoDialog_Create(roster_box, 500, "Game warning", "You are still in a game!\n\nIf you go offline now and do not log in within the timeout period, you will ^bFORFEIT^b your game.\n\nTo go offline, click OK.  To go back, click Cancel.", "Cancel", "OK", NULL, OfflineWarning_ReallyOffline, NULL);
}

void DisconnectWarning_Reconnect(struct Box_s *pbox, void *userdata)
{
	Ctrl_LoginLast();
	Box_Destroy(Box_GetRoot(pbox));
}

void View_PopupDisconnectWarning()
{
	AutoDialog_Create(roster_box, 500, "Game warning", "You have lost connection to chesspark and are still in a game!\n\nIf you are not able to reconnect within the timeout period, you will ^bFORFEIT^b your game.  To try to reconnect, click OK.", "Cancel", "OK", NULL, DisconnectWarning_Reconnect, NULL);
}

struct reconvenereminderinfo_s
{
	struct gamesearchinfo_s *info;
	char *jid;
};

void ReconveneReminder_Invite(struct Box_s *pbox, struct reconvenereminderinfo_s *rrinfo)
{
	View_PopupGameInviteFriendDialog(rrinfo->jid, NULL, 0);
	Box_Destroy(Box_GetRoot(pbox));
}

void ReconveneReminder_Reconvene(struct Box_s *pbox, struct reconvenereminderinfo_s *rrinfo)
{
	View_PopupReconveneDialog(rrinfo->jid, rrinfo->info->gameid, rrinfo->info, 1);
	Box_Destroy(Box_GetRoot(pbox));
}

void View_PopupReconveneReminderDialog(char *jid, struct gamesearchinfo_s *info)
{
	char txt[512];
	struct reconvenereminderinfo_s *rrinfo = malloc(sizeof(*rrinfo));

	rrinfo->info = Info_DupeGameSearchInfo(info);
	rrinfo->jid = strdup(jid);

	i18n_stringsub(txt, 512, _("You have an existing adjourned game with %1.\n\nDo you wish to reconvene this game, or continue with a normal game invite?"), Model_GetFriendNick(jid));
	AutoDialog_Create(roster_box, 500, "Adjourned game exists", txt, "Reconvene", "Invite", ReconveneReminder_Reconvene, ReconveneReminder_Invite, rrinfo);
}


struct gamesearchinfo_s *View_GetGameInfo(char *gameid)
{
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;
		
	if (!gamelistentry)
	{
		return NULL;
	}
		
	chessbox = (*gamelistentry)->data;

	return ChessBox_GetGameInfo(chessbox);
}

struct Box_s *View_PopupWaitingDialog(char *titlebartxt,
  char *topinfotext, char *bottominfotext)
{
	struct Box_s *waitingbox = AutoWait_Create(newgames_box, 400, titlebartxt, topinfotext, bottominfotext);

	return waitingbox;
}

void View_ShowRatingUpdate(char *from, char *jid, struct rating_s *newrating)
{
	char *gameid = Jid_GetBeforeAt(from);
	struct namedlist_s **gamelistentry = NamedList_GetByName(&gamedialoglist, gameid);
	struct Box_s *chessbox;

	if (!gamelistentry || !*gamelistentry)
	{
		return;
	}
		
	chessbox = (*gamelistentry)->data;

	ChessBox_ShowRatingUpdate(chessbox, jid, newrating);

}

void View_ShowFriendPlayingPopup(char *from, char *gameid, struct gamesearchinfo_s *info)
{
	char txt[512];
	char txt2[512];
	char *white = NULL, *black = NULL, *variant = NULL, *tc = _("Long"), *gametype;
	int correspondence = 0;
	int rated = 0;
	struct Box_s *popbox;

	if (Model_GetOption(OPTION_DISABLEROSTERGAMENOTIFICATIONS))
	{
		return;
	}

	sprintf(txt, _("%s is playing!"), Model_GetFriendNick(from));

	if (info && info->white && info->white->jid)
	{
		white = Model_GetFriendNick(info->white->jid);
	}

	if (info && info->black && info->black->jid)
	{
		black = Model_GetFriendNick(info->black->jid);
	}

	if (info && info->timecontrol)
	{
		tc = Info_TimeControlToCategory(info->timecontrol);
	}

	if (info && info->variant)
	{
		if (stricmp(info->variant, "standard") == 0)
		{
			variant = NULL;
		}
		else if (stricmp(info->variant, "atomic") == 0)
		{
			variant = _("Atomic");
		}
		else if (stricmp(info->variant, "chess960") == 0)
		{
			variant = _("Chess960");
		}
		else if (stricmp(info->variant, "losers") == 0)
		{
			variant = _("Loser's");
		}
		else if (stricmp(info->variant, "crazyhouse") == 0)
		{
			variant = _("Crazyhouse");
		}
		else if (stricmp(info->variant, "checkers") == 0)
		{
			variant = _("Checkers");
		}
	}

	if (info && info->correspondence)
	{
		correspondence = 1;
	}

	if (info && info->rated)
	{
		rated = 1;
	}

	gametype = (correspondence ? _("(Correspondence)") : (variant ? variant : tc));

	sprintf(txt2, "^b%s is playing a game:^n\n\n%s vs %s\n%s %s\n\n^lWatch this!^l", Model_GetFriendNick(from), white, black, rated ? _("Rated") : _("Unrated"), gametype);
	popbox = CornerPop_Create(roster_box, 300, txt, txt2);

	if (!popbox)
	{
		return;
	}

	CornerPop_SetLinkCallback(popbox, 1, ViewLink_WatchGame_OnClick, strdup(gameid));
}

void View_CloseAllDialogs()
{
	NamedList_Destroy(&gamedialoglist);
	NamedList_Destroy(&firstchat);
}

void View_ClearChatHistory(char *chatjid)
{
	struct namedlist_s **ppchat = View_GetPtrToChat(chatjid, 1);

	if (*ppchat)
	{
		struct Box_s *chatbox = (*ppchat)->data;
		ChatBox_ClearChatHistory(chatbox, chatjid);
	}
}

void View_Adpop()
{
	Adpop_Create(roster_box);
}

void View_Adpop2(struct Box_s *pbox, void *dummy)
{
	Box_RemoveTimedFunc(roster_box, View_Adpop2, 3000);
	View_Adpop();
}

void View_AdpopDelay()
{
	Box_AddTimedFunc(roster_box, View_Adpop2, NULL, 3000);
}

void View_UpdatePings(int last, int avg, int spike)
{
	if (roster_box)
	{
		Roster_UpdatePings(roster_box, last, avg, spike);
	}
}

void View_SetChessPieceTheme(char *theme)
{
	struct namedlist_s *entry = gamedialoglist;

	while (entry)
	{
		struct Box_s *chessbox = entry->data;
		ChessBox_SetTheme(chessbox, theme);
		entry = entry->next;
	}
}