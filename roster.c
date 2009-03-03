#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <stdio.h>
#include <stdlib.h>

#include "box.h"

#include "button.h"
#include "button2.h"
#include "dragbox.h"
#include "image.h"
#include "scroll.h"
#include "sizer.h"
#include "tabs.h"
#include "text.h"
#include "titledrag.h"
#include "tooltip.h"

#include "boxtypes.h"
#include "ctrl.h"
#include "constants.h"
#include "friendentry.h"
#include "i18n.h"
#include "imagemgr.h"
#include "info.h"
#include "list.h"
#include "log.h"
#include "menu.h"
#include "model.h"
#include "options.h"
#include "namedlist.h"
#include "spawn.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

#include "roster.h"

int Roster_OnDragDrop(struct Box_s *roster_box, struct Box_s *psrc, int x, int y, int dropid, void *dropdata);

struct Rosterdata_s
{
	struct Box_s *tabs, *prefs, *add, *gfinder, *presencemenu, *avatarbutton;
	struct Box_s *presencelabel, *userinfo, *grabber;
	struct Box_s *errortext, *connectanimation, *reconnectbutton;
	struct Box_s *tabcontentbg;
	struct Box_s *avatarnormal, *avatarhover;
	struct Box_s *lagometer;
	char *jid;
	int oldwidth, oldheight;
	int normh, normy;
	int numtabs;
	int isnub;
	struct namedlist_s *ratinglist;
	struct namedlist_s *roleslist;
	struct namedlist_s *titleslist;
	char *lagtext;
};

void Roster_OnManageAccount(struct Box_s *pbox, void *userdata)
{
	struct Rosterdata_s *data = Box_GetRoot(pbox)->boxdata;

	Ctrl_ShowProfile(data->jid);
	/*
	ShellExecute(NULL, NULL, "http://chesspark.com", NULL, ".", SW_SHOW);
	*/
}

void Roster_OnClose(struct Box_s *pbox)
{
	Model_Quit();
}

void Roster_OnRestore(struct Box_s *pbox);
void Roster_OnMinimize(struct Box_s *pbox);
void Roster_OnMaximize(struct Box_s *pbox);

void Roster_OnRestore(struct Box_s *pbox)
{
	Log_Write(0, "roster restore\n");
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_RESTORE);
	/*
	pbox->OnMinimize = Roster_OnMinimize;
	pbox->OnRestore  = NULL;
	pbox->OnMaximize = Roster_OnMaximize;
	*/
}

void Roster_OnMinimize(struct Box_s *pbox)
{
	Log_Write(0, "roster minimize\n");
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_MINIMIZE);
	/*
	pbox->OnMinimize = NULL;
	pbox->OnRestore  = Roster_OnRestore;
	pbox->OnMaximize = Roster_OnMaximize;
	*/
}

void Roster_OnMaximize(struct Box_s *pbox)
{
	/*
	Log_Write(0, "roster maximize\n");
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_MAXIMIZE);
	pbox->OnMinimize = Roster_OnMinimize;
	pbox->OnRestore  = Roster_OnRestore;
	pbox->OnMaximize = NULL;
*/
	int maxx, maxy, maxw, maxh;
	RECT rc;
	struct Rosterdata_s *data;

	RECT windowrect;
	HMONITOR hm;
	MONITORINFO mi;

	pbox = Box_GetRoot(pbox);
	data = pbox->boxdata;

	windowrect.left = pbox->x;
	windowrect.right = windowrect.left + pbox->w - 1;
	windowrect.top = pbox->y;
	windowrect.bottom = windowrect.top + pbox->h - 1;

	hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hm, &mi);

	rc = mi.rcWork;

	maxx = rc.left;
	maxy = rc.top;
	maxw = rc.right - rc.left;
	maxh = rc.bottom - rc.top;

	if (pbox->h != maxh)
	{
		data->normh = pbox->h;
		data->normy = pbox->y;

		Box_MoveWndCustom(pbox, pbox->x, maxy, pbox->w, maxh);

		Box_ForceDraw(pbox);

	}
	else if (data->normh != 0)
	{
		Box_MoveWndCustom(pbox, pbox->x, data->normy, pbox->w, data->normh);

		Box_ForceDraw(pbox);
	}

}

void Roster_OnReconnectCancel(struct Box_s *pbox)
{
	Roster_SetError(Box_GetRoot(pbox), _("Reconnect cancelled."), 0);

	Ctrl_CancelLogin();
}


void Roster_OnReconnect(struct Box_s *pbox)
{
	struct Rosterdata_s *data = Box_GetRoot(pbox)->boxdata;

	Roster_SetReconnect(Box_GetRoot(pbox));

	Ctrl_LoginLast();
}

void RosterButton_PopupGameFinder(struct Box_s *pbox)
{
	View_ToggleNewGamesDialog();
}

void RosterLagOMeter_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *roster = Box_GetRoot(pbox);
	struct Rosterdata_s *data = roster->boxdata;

	if (data->lagtext && Box_CheckXYInBox(pbox, xmouse, ymouse)) 
	{
		if (!pbox->tooltipvisible)
		{
			Box_AddTimedFunc(pbox, ToolTip_Popup, strdup(data->lagtext), 1000);
			pbox->tooltipvisible = 1;
		}
	}
	else
	{
		ToolTip_PopDown(pbox);
		pbox->tooltipvisible = 0;
	}
}

struct Box_s *Roster_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Rosterdata_s *data = malloc(sizeof(*data));
	struct Box_s *pbox, *roster;

	memset(data, 0, sizeof(*data));

	roster = Box_Create(x, y, w, h, BOX_VISIBLE);
	roster->bgcol = DefaultBG;
	roster->fgcol = RGB(0, 0, 0);
	roster->minh = 256;
	roster->minw = 256;

	SizerSet_Create(roster);

	roster->titlebar = TitleBar_Add(roster, _("Chesspark"), Roster_OnClose, Roster_OnMinimize, Roster_OnMaximize);
	roster->OnActive = TitleBarRoot_OnActive;
	roster->OnInactive = TitleBarRoot_OnInactive;

	roster->OnClose = Roster_OnClose;
	roster->OnMinimize = Roster_OnMinimize;
	roster->OnMaximize = Roster_OnMaximize;
	roster->OnRestore = Roster_OnRestore;

	/* Prefs button */
	/*pbox = Box_Create(6, 224, 29, 22, BOX_VISIBLE);*/
	pbox = Button_Create(Margin, h - ButtonHeight - Margin - 2, 30, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button_SetOnButtonHit(pbox, Menu_PopupPrefsMenu);
	Button_SetNormalImg (pbox, ImageMgr_GetSubImage("preferenceMenuButton1", "preferenceMenu.png", 0,  0, 30, 23));
	Button_SetPressedImg(pbox, ImageMgr_GetSubImage("preferenceMenuButton2", "preferenceMenu.png", 30, 0, 30, 23));
	Button_SetHoverImg  (pbox, ImageMgr_GetSubImage("preferenceMenuButton3", "preferenceMenu.png", 60, 0, 30, 23));
	Button_SetTooltipText(pbox, _("Preferences Menu"));
	Box_AddChild(roster, pbox);
	data->prefs = pbox;

	/* Roster add button */
	/*pbox = Box_Create(42, 224, 29, 22, BOX_VISIBLE);*/
	pbox = Button_Create(Margin * 2 + ButtonWidth, h - ButtonHeight - Margin - 2, 30, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button_SetOnButtonHit(pbox, Menu_PopupAddMenu);
	Button_SetNormalImg (pbox, ImageMgr_GetSubImage("addMenuButton1", "addMenuStrip.png", 0,  0, 30, 23));
	Button_SetPressedImg(pbox, ImageMgr_GetSubImage("addMenuButton2", "addMenuStrip.png", 30, 0, 30, 23));
	Button_SetHoverImg  (pbox, ImageMgr_GetSubImage("addMenuButton3", "addMenuStrip.png", 60, 0, 30, 23));
	Button_SetTooltipText(pbox, _("Add Menu"));
	Box_AddChild(roster, pbox);
	data->add = pbox;

	/* lagometer */
	pbox = Box_Create(w - 62 - Margin * 2, h - 28 - Margin - 2, 32, 32, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->OnMouseMove = RosterLagOMeter_OnMouseMove;
	pbox->img = ImageMgr_GetScaledImage("lagsmall1", "lag1.png", 32, 32);
	Box_AddChild(roster, pbox);
	data->lagometer = pbox;

	/* Roster pop up game finder button */
	/*pbox = Box_Create(42, 224, 29, 22, BOX_VISIBLE);*/
	pbox = Button_Create(w - 30 - Margin, h - ButtonHeight - Margin - 2, 30, 23, BOX_TRANSPARENT | BOX_STRETCHIMG);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button_SetOnButtonHit(pbox, RosterButton_PopupGameFinder);
	Button_SetNormalImg (pbox, ImageMgr_GetSubImage("gameMenuSmall-normal",  "gameMenuSmall.png",  0, 0, 30, 23));
	Button_SetPressedImg(pbox, ImageMgr_GetSubImage("gameMenuSmall-pressed", "gameMenuSmall.png", 30, 0, 30, 23));
	Button_SetHoverImg  (pbox, ImageMgr_GetSubImage("gameMenuSmall-hover",   "gameMenuSmall.png", 60, 0, 30, 23));
	Button_SetTooltipText(pbox, _("Game Finder"));
	Box_AddChild(roster, pbox);
	data->gfinder = pbox;

	/* Presence Menu */
	/* pbox = Box_Create(6, 5, 21, 16, BOX_VISIBLE);*/
	pbox = Button_Create(Margin, TitleBarHeight + Margin, PresenceWidth, PresenceHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	Button_SetOnButtonHit(pbox, Menu_PopupPresenceMenu);
	Button_SetNormalImg (pbox, ImageMgr_GetSubImage("presenceMenuAvailable1", "presenceMenuAvailable.PNG", 0, 0, 32, 33));
	Button_SetPressedImg(pbox, ImageMgr_GetSubImage("presenceMenuAvailable2", "presenceMenuAvailable.PNG", 32, 0, 32, 33));
	Button_SetHoverImg  (pbox, ImageMgr_GetSubImage("presenceMenuAvailable3", "presenceMenuAvailable.PNG", 64, 0, 32, 33));
	Button_SetTooltipText(pbox, _("Presence Menu"));
	Box_AddChild(roster, pbox);
	data->presencemenu = pbox;

	/* Avatar */
	/*pbox = Box_Create(210, 5, 40, 40, BOX_VISIBLE);*/
	pbox = Button2_Create(w - Margin - AvatarWidth, TitleBarHeight + Margin, AvatarWidth, AvatarHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	Button2_SetOnButtonHit(pbox, Menu_PopupAvatarMenu);
	Button2_SetTooltipText(pbox, _("Avatar Menu"));
	Box_AddChild(roster, pbox);
	data->avatarbutton = pbox;

	{
		struct Box_s *subbox, *subbox2;
		struct BoxImage_s *avatarimage, *avatarimagedim;

		avatarimage = ImageMgr_GetSubImage("DefaultAvatar1", "DefaultAvatar.png", 0, 0, 50, 50);
		avatarimagedim = ImageMgr_GetRootDimmedImage("DefaultAvatar1Dim", "DefaultAvatar1", 70);

		subbox = Box_Create(w - Margin - AvatarWidth, TitleBarHeight + Margin, AvatarWidth, AvatarHeight, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
		subbox->img = avatarimage;
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(roster, subbox);
		data->avatarnormal = subbox;

		subbox = Box_Create(w - Margin - AvatarWidth, TitleBarHeight + Margin, AvatarWidth, AvatarHeight, BOX_TRANSPARENT | BOX_FITASPECTIMG);
		subbox->img = avatarimagedim;
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(roster, subbox);

		subbox2 = Box_Create(0, 0, AvatarWidth, AvatarHeight, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetImage("avatarhover.png");
		Box_AddChild(subbox, subbox2);

		data->avatarhover = subbox;

		Button2_SetNormal(data->avatarbutton, data->avatarnormal);
		Button2_SetHover(data->avatarbutton, data->avatarhover);
	}

	/* Presence Label */
	/*pbox = Box_Create(30, 25, 126, 16, BOX_VISIBLE | BOX_TRANSPARENT);*/
	pbox = Box_Create(Margin * 2 + PresenceWidth, TitleBarHeight + Margin + TextHeight, 180, TextHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = PresenceFG;
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(roster, pbox);
	data->presencelabel = pbox;

	/* User Info Label */
	/*pbox = Box_Create(30, 5, 126, 16, BOX_VISIBLE | BOX_TRANSPARENT);*/
	pbox = Text_Create(Margin * 2 + PresenceWidth, TitleBarHeight + Margin, roster->w - Margin * 3 - PresenceWidth - AvatarWidth, TextHeight, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->fgcol = UserInfoFG1;
	pbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	Text_SetLinkColor(pbox, CR_LtOrange);
	Box_AddChild(roster, pbox);
	data->userinfo = pbox;

	/* Error label */
	pbox = Box_Create(Margin, data->avatarbutton->y + data->avatarbutton->h + Margin * 2, roster->w - Margin * 2, TextHeight, BOX_TRANSPARENT | BOX_CENTERTEXT);
	pbox->fgcol = UserInfoFG2;
	pbox->OnSizeWidth = Box_OnSizeWidth_Center;
	Box_AddChild(roster, pbox);
	data->errortext = pbox;

	/* Connect animation */
	pbox = Box_Create((roster->w - 50) / 2, data->errortext->y + data->errortext->h + Margin, 50, 10, BOX_TRANSPARENT);
	pbox->img = ImageMgr_GetSubAnim("loadinganim", "loading.png", 17, 50, 10);
	pbox->OnSizeWidth = Box_OnSizeWidth_Center;
	Box_AddChild(roster, pbox);
	data->connectanimation = pbox;

	/* Reconnect button */
	pbox = StdButton_Create((roster->w - 100) / 2, data->connectanimation->y + data->connectanimation->h + Margin, 100, _("Reconnect"), 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_Center;
	Box_AddChild(roster, pbox);
	data->reconnectbutton = pbox;

	/* Grabber */
	pbox = Box_Create(w - GrabberWidth, h - GrabberHeight, GrabberWidth, GrabberHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->img = ImageMgr_GetImage("windowResizeHandle.png");
	pbox->OnSizeWidth  = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_AddChild(roster, pbox);
	data->grabber = pbox;

	/* Tab BG */
	pbox = Box_Create(Margin, TitleBarHeight + Margin * 2 + TextHeight * 2 + TabHeight - 1, w - Margin * 2, h - TitleBarHeight - Margin * 2 - TextHeight * 2 - TabHeight - Margin * 3 - ButtonHeight + 1, BOX_VISIBLE | BOX_BORDER);
	pbox->bgcol = TabBG2;
	pbox->brcol = TabBR;
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(roster, pbox);
	data->tabcontentbg = pbox;
	
	{
		struct Box_s *subbox;
		subbox = Box_Create(pbox->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcornersur", "contentcorners.png", 5, 0, 5, 5);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(pbox, subbox);

		subbox = Box_Create(pbox->w - 5, pbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcornersdr", "contentcorners.png", 5, 5, 5, 5);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(pbox, subbox);

		subbox = Box_Create(0, pbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcornersdl", "contentcorners.png", 0, 5, 5, 5);
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(pbox, subbox);
	}

	/* Tab Control */
	/*pbox = TabCtrl_Create(7, 45, 244, 40, BOX_VISIBLE | BOX_TRANSPARENT);*/
	pbox = TabCtrl_Create(Margin, TitleBarHeight + Margin * 2 + TextHeight * 2, w - Margin * 2, TabHeight, BOX_VISIBLE | BOX_TRANSPARENT, TRUE, TRUE, ROSTERTABDROPDATA_ID);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->bgcol = TabBG2;
	pbox->fgcol = TabFG1;
	Box_AddChild(roster, pbox);
	data->tabs = pbox;

	roster->boxdata = data;
	roster->boxtypeid = BOXTYPE_ROSTER;
	roster->OnDragDrop = Roster_OnDragDrop;

	data->oldwidth = w;
	data->oldheight = h;

	Box_CreateWndCustom(roster, _("Chesspark"), NULL);
	BringWindowToTop(roster->hwnd);

	return roster;
}

void Roster_UpdateProfile(struct Box_s *roster)
{
	struct Rosterdata_s *data = roster->boxdata;
	struct namedlist_s **entry;
	int showrating = -1;

	/*
	entry = data->ratinglist;
	while (entry)
	{
		struct rating_s *rating = entry->data;
		if (rating->rating > highrating)
		{
			highrating = rating->rating;
			Log_Write(0, "highrating %d\n", highrating);
		}
		entry = entry->next;
	}
	*/

	entry = NamedList_GetByName(&data->ratinglist, "Standard");
	if (entry)
	{
		struct rating_s *rating = (*entry)->data;

		showrating = rating->rating;
	}

	{
		char txt[512];
		int notfirst = 0;
		struct namedlist_s *entry;

		strcpy(txt, "^l");

		entry = data->titleslist;

		while (entry)
		{
			if (notfirst)
			{
				strcat(txt, ", ");
			}
			else
			{
				notfirst = 1;
			}

			strcat(txt, entry->data);

			entry = entry->next;
		}

		if (notfirst)
		{
			strcat(txt, " ");
		}

		strcat(txt, Model_GetFriendNick(data->jid));
		strcat(txt, "  ");

		notfirst = 0;
		entry = data->roleslist;

		while (entry)
		{
			if (notfirst)
			{
				strcat(txt, ", ");
			}
			else
			{
				notfirst = 1;
			}

			strcat(txt, entry->data);

			entry = entry->next;
		}

		if (showrating != -1)
		{
			char txtrating[80];
			if (notfirst)
			{
				strcat(txt, "  ");
			}
			else
			{
				notfirst = 1;
			}

			strcat(txt, "");
			sprintf(txtrating, "%d", showrating);
			strcat(txt, txtrating);
		}

		strcat(txt, "^l");

		Text_SetText(data->userinfo, txt);

		Text_SetLinkCallback(data->userinfo, 1, Roster_OnManageAccount, NULL);
	}

	Box_Repaint(data->userinfo);
}

void Roster_SetUser(struct Box_s *pbox, char *jid, char *nick)
{
	struct Rosterdata_s *data = pbox->boxdata;
	/*
	char txt[120];

	sprintf(txt, "^l%s^l", nick);

	Text_SetText(data->userinfo, txt);
	Text_SetLinkCallback(data->userinfo, 1, Roster_OnManageAccount, NULL);
*/
	data->jid = strdup(jid);

	Roster_UpdateProfile(pbox);
}


void Roster_SetPresence(struct Box_s *pbox, enum SStatus status, char *statusmsg)
{
	struct Rosterdata_s *data = pbox->boxdata;

	switch(status)
	{
		case SSTAT_AWAY:
		{
			Button_SetNormalImg (data->presencemenu, ImageMgr_GetSubImage("presenceMenuAway1", "presenceMenuAway.PNG", 0, 0, 32, 33));
			Button_SetPressedImg(data->presencemenu, ImageMgr_GetSubImage("presenceMenuAway2", "presenceMenuAway.PNG", 32, 0, 32, 33));
			Button_SetHoverImg  (data->presencemenu, ImageMgr_GetSubImage("presenceMenuAway3", "presenceMenuAway.PNG", 64, 0, 32, 33));
		}
		break;

		case SSTAT_IDLE:
		{
			Button_SetNormalImg (data->presencemenu, ImageMgr_GetSubImage("presenceMenuIdle1", "presenceMenuIdle.PNG", 0, 0, 32, 33));
			Button_SetPressedImg(data->presencemenu, ImageMgr_GetSubImage("presenceMenuIdle2", "presenceMenuIdle.PNG", 32, 0, 32, 33));
			Button_SetHoverImg  (data->presencemenu, ImageMgr_GetSubImage("presenceMenuIdle3", "presenceMenuIdle.PNG", 64, 0, 32, 33));
		}
		break;

		case SSTAT_OFFLINE:
		{
			Button_SetNormalImg (data->presencemenu, ImageMgr_GetSubImage("presenceMenuOffline1", "presenceMenuAway.PNG", 96, 0, 32, 33));
			Button_SetPressedImg(data->presencemenu, ImageMgr_GetSubImage("presenceMenuOffline2", "presenceMenuAvailable.PNG", 96, 0, 32, 33));
			Button_SetHoverImg  (data->presencemenu, ImageMgr_GetSubImage("presenceMenuOffline2", "presenceMenuAvailable.PNG", 96, 0, 32, 33));
		}
		break;

		default:
		{
			Button_SetNormalImg (data->presencemenu, ImageMgr_GetSubImage("presenceMenuAvailable1", "presenceMenuAvailable.PNG", 0, 0, 32, 33));
			Button_SetPressedImg(data->presencemenu, ImageMgr_GetSubImage("presenceMenuAvailable2", "presenceMenuAvailable.PNG", 32, 0, 32, 33));
			Button_SetHoverImg  (data->presencemenu, ImageMgr_GetSubImage("presenceMenuAvailable3", "presenceMenuAvailable.PNG", 64, 0, 32, 33));
		}
		break;
	}

	if (!statusmsg)
	{
		if (status == SSTAT_AWAY)
		{
			statusmsg = _("Away");
		}
		else if (status == SSTAT_IDLE)
		{
			statusmsg = _("Idle");
		}
		else if (status == SSTAT_OFFLINE)
		{
			statusmsg = _("Offline");
		}
		else
		{
			statusmsg = _("Available");
		}
	}

	Box_SetText(data->presencelabel, statusmsg);

	Box_Repaint(data->presencemenu);
	Box_Repaint(data->presencelabel);
}

void Roster_SetNub(struct Box_s *pbox)
{
	struct Rosterdata_s *data = pbox->boxdata;

	if (data->tabs->flags & BOX_VISIBLE)
	{
		data->tabs->flags    &= ~BOX_VISIBLE;
		TabCtrl_HideAll(data->tabs);
		data->errortext->flags       &= ~BOX_VISIBLE;
		data->reconnectbutton->flags &= ~BOX_VISIBLE;
		data->connectanimation->flags &= ~BOX_VISIBLE;
		data->tabcontentbg->flags	  &= ~BOX_VISIBLE;
		data->lagometer->flags &= ~BOX_VISIBLE;
		Button2_SetOnButtonHit(data->reconnectbutton, NULL);
	
		data->oldwidth = pbox->w;
		data->oldheight = pbox->h;

		pbox->minh = 165;
	
		Box_MoveWndCustom(pbox, pbox->x, pbox->y, 256, 165);

		data->isnub = 1;
	}
	
	Box_Repaint(pbox);
}

void Roster_UnsetNub(struct Box_s *pbox)
{
	struct Rosterdata_s *data = pbox->boxdata;
	
	data->tabs->flags             |= BOX_VISIBLE;
	TabCtrl_UnhideAll(data->tabs);
	/*TabCtrl_ActivateFirst(data->tabs);*/
	data->errortext->flags        &= ~BOX_VISIBLE;
	data->reconnectbutton->flags  &= ~BOX_VISIBLE;
	data->connectanimation->flags &= ~BOX_VISIBLE;
	data->tabcontentbg->flags	  |= BOX_VISIBLE;
	data->lagometer->flags |= BOX_VISIBLE;
	Button2_SetOnButtonHit(data->reconnectbutton, NULL);
	Button_SetOnButtonHit(data->add, Menu_PopupAddMenu);
	Button_SetNormalImg (data->add, ImageMgr_GetSubImage("addMenuButton1", "addMenuStrip.png", 0,  0, 30, 23));
	Button_SetPressedImg(data->add, ImageMgr_GetSubImage("addMenuButton2", "addMenuStrip.png", 30, 0, 30, 23));
	Button_SetHoverImg  (data->add, ImageMgr_GetSubImage("addMenuButton3", "addMenuStrip.png", 60, 0, 30, 23));
	/*if (Model_GetOption(OPTION_DEVFEATURES))*/
	{
		data->gfinder->flags |= BOX_VISIBLE;
		Button_SetOnButtonHit(data->gfinder, RosterButton_PopupGameFinder);
	}
	Button_SetNormalImg (data->gfinder, ImageMgr_GetSubImage("gameMenuSmall-normal",  "gameMenuSmall.png",  0, 0, 30, 23));
	Button_SetPressedImg(data->gfinder, ImageMgr_GetSubImage("gameMenuSmall-pressed", "gameMenuSmall.png", 30, 0, 30, 23));
	Button_SetHoverImg  (data->gfinder, ImageMgr_GetSubImage("gameMenuSmall-hover",   "gameMenuSmall.png", 60, 0, 30, 23));

	pbox->minh = 256;
	
	Box_MoveWndCustom(pbox, pbox->x, pbox->y, data->oldwidth, data->oldheight);

	data->isnub = 0;
	SetWindowText(pbox->hwnd, "Chesspark");
	
	Box_Repaint(pbox);
}

int Roster_IsNub(struct Box_s *roster)
{
	struct Rosterdata_s *data = roster->boxdata;

	return data->isnub;
}

void Roster_SetError(struct Box_s *pbox, char *error, int notify)
{
	struct Rosterdata_s *data = pbox->boxdata;
	char fulltxt[512];

	Roster_SetNub(pbox);

	data->errortext->flags       |= BOX_VISIBLE;
	data->reconnectbutton->flags |= BOX_VISIBLE;
	data->connectanimation->flags &= ~BOX_VISIBLE;
	data->lagometer->flags &= ~BOX_VISIBLE;

	i18n_stringsub(fulltxt, 512, _("Disconnected: %1"), error);

	StdButton_SetText(data->reconnectbutton, _("Reconnect"));
	Button2_SetOnButtonHit(data->reconnectbutton, Roster_OnReconnect);
	Button_SetOnButtonHit(data->add, NULL);
	Button_SetNormalImg (data->add, ImageMgr_GetSubImage("addMenuButton4", "addMenuStrip.png", 90, 0, 30, 23));
	Button_SetPressedImg(data->add, ImageMgr_GetSubImage("addMenuButton4", "addMenuStrip.png", 90, 0, 30, 23));
	Button_SetHoverImg  (data->add, ImageMgr_GetSubImage("addMenuButton4", "addMenuStrip.png", 90, 0, 30, 23));
	data->gfinder->flags &= ~BOX_VISIBLE;
	Button_SetOnButtonHit(data->gfinder, NULL);
	Button_SetNormalImg (data->gfinder, ImageMgr_GetSubImage("gameMenuSmall-disabled", "gameMenuSmall.png", 90, 0, 30, 23));
	Button_SetPressedImg(data->gfinder, ImageMgr_GetSubImage("gameMenuSmall-disabled", "gameMenuSmall.png", 90, 0, 30, 23));
	Button_SetHoverImg  (data->gfinder, ImageMgr_GetSubImage("gameMenuSmall-disabled", "gameMenuSmall.png", 90, 0, 30, 23));
	Box_SetText(data->errortext, fulltxt);

	{
		i18n_stringsub(fulltxt, 512, "Chesspark - %1", error);
		SetWindowText(pbox->hwnd, fulltxt);
		if(notify)
		{
			FlashWindow(pbox->hwnd, 1);
		}
	}

	Box_Repaint(pbox);
}

void Roster_SetReconnect(struct Box_s *pbox)
{
	struct Rosterdata_s *data = pbox->boxdata;

	Roster_SetError(pbox, "", 0);

	data->connectanimation->flags |= BOX_VISIBLE;

	Box_SetText(data->errortext, _("Reconnecting to network..."));
	StdButton_SetText(data->reconnectbutton, _("Cancel"));
	Button2_SetOnButtonHit(data->reconnectbutton, Roster_OnReconnectCancel);
	Box_Repaint(pbox);
}

int Roster_OnDragDrop(struct Box_s *roster_box, struct Box_s *psrc, int x, int y, int dropid, void *dropdata)
{
	struct Box_s *srcdialog = Box_GetRoot(psrc);
	struct Rosterdata_s *data = roster_box->boxdata;
	char *tabname = dropdata;

	if (dropid != ROSTERTABDROPDATA_ID)
	{
		return Box_OnDragDrop(roster_box, psrc, x, y, dropid, dropdata);
	}

	if (roster_box != srcdialog)
	{
		if (srcdialog->boxtypeid == BOXTYPE_ROSTER)
		{
			/* Should not happen, only one roster */
			return Box_OnDragDrop(roster_box, psrc, x, y, dropid, dropdata);
		}
		else if (srcdialog->boxtypeid == BOXTYPE_ROSTERSPAWN)
		{
			struct Box_s *tabcontentbox = Spawn_GetTabContentBox(srcdialog, tabname);

			if (!tabcontentbox)
			{
				return 0;
			}

			Box_Unlink(tabcontentbox);
			Box_Destroy(srcdialog);
			Roster_AddTab(roster_box, tabname, tabcontentbox);
			Roster_ActivateTab(roster_box, tabname);

			return 1;
		}
	}

	x -= (data->tabs->x);
	y -= (data->tabs->y);

	if (Box_CheckXYInBox(data->tabs, x, y))
	{
		TabCtrl_HandleTabDrop(data->tabs, tabname, x, y);
	}

	return 1;
}

void RosterTab_OnDropEmpty(struct Box_s *tabctrl, int x, int y, int dropid, void *dropdata)
{
	char *tabname = dropdata;
	struct Box_s *roster_box = Box_GetRoot(tabctrl);

	View_SpawnRosterTabWindow(tabname, x, y, 256, 256);
	Roster_RemoveTab(roster_box, tabname);
	Roster_ActivateFirstTab(roster_box);
}

void Roster_AddTab(struct Box_s *pbox, char *name, struct Box_s *target)
{
	struct Rosterdata_s *data = pbox->boxdata;

	target->x = Margin * 2;
	target->y = data->tabs->y + data->tabs->h + Margin;

	if (target->OnSizeWidth)
	{
		target->OnSizeWidth(target, (pbox->w - Margin * 4) - target->w);
	}

	if (target->OnSizeHeight)
	{
		target->OnSizeHeight(target, (pbox->h - target->y - Margin * 4 - ButtonHeight) - target->h);
	}

	TabCtrl_AddTab2(data->tabs, name, name, target, -58, NULL, NULL, ROSTERTABDROPDATA_ID, strdup(name), RosterTab_OnDropEmpty);
	Box_AddChild(pbox, target);

	data->numtabs++;
	if (data->numtabs == 1)
	{
		Roster_UnsetNub(pbox);
	}
}

void Roster_RemoveTab(struct Box_s *pbox, char *name)
{
	struct Rosterdata_s *data = pbox->boxdata;

	TabCtrl_RemoveTab(data->tabs, name);

	data->numtabs--;
	
	if (data->numtabs == 0)
	{
		Roster_SetNub(pbox);
	}
}

void Roster_ActivateTab(struct Box_s *pbox, char *name)
{
	struct Rosterdata_s *data = pbox->boxdata;

	TabCtrl_ActivateTabByName(data->tabs, name);
}

void Roster_ActivateFirstTab(struct Box_s *pbox)
{
	struct Rosterdata_s *data = pbox->boxdata;

	TabCtrl_ActivateFirst(data->tabs);
}

void Roster_SetAvatar(struct Box_s *pbox, char *filename)
{
	struct Rosterdata_s *data = pbox->boxdata;
	struct BoxImage_s *image, *parentimage, *imagedim;
	int w, h;
	char scaledname[256], dimname[256];

	parentimage = ImageMgr_GetRootImage(filename);

	if (!parentimage)
	{
		return;
	}

	if (parentimage->w > parentimage->h)
	{
		h = parentimage->h * 30 / parentimage->w;
		w = 30;
	}
	else
	{
		w = parentimage->w * 30 / parentimage->h;
		h = 30;
	}

	sprintf(scaledname, "%dx%d-%s", w, h, filename);
	sprintf(dimname, "%dx%d-%s-dim", w, h, scaledname);

	image = ImageMgr_GetRootScaledImage(scaledname, filename, w, h);
	imagedim = ImageMgr_GetRootDimmedImage(dimname, scaledname, 70);

	data->avatarnormal->img = image;
	data->avatarhover->img = imagedim;
	Box_Repaint(data->avatarbutton);
}


void Roster_SetProfile(struct Box_s *roster, struct namedlist_s *ratinglist, struct namedlist_s *roleslist, struct namedlist_s *titleslist)
{
	struct Rosterdata_s *data = roster->boxdata;

	NamedList_Destroy(&data->ratinglist);
	NamedList_Destroy(&data->roleslist);
	NamedList_Destroy(&data->titleslist);
	data->ratinglist = Info_DupeRatingList(ratinglist);
	data->roleslist = NamedList_DupeStringList(roleslist);
	data->titleslist = NamedList_DupeStringList(titleslist);

	Roster_UpdateProfile(roster);
}

void Roster_SetTabActivateFunc(struct Box_s *roster, void (*OnTabActivate)(struct Box_s *, char *))
{
	struct Rosterdata_s *data = roster->boxdata;

	TabCtrl_SetTabActivateFunc(data->tabs, OnTabActivate);
}

void Roster_UpdateToOptions(struct Box_s *roster)
{
	struct Rosterdata_s *data = roster->boxdata;

	if (Model_GetOption(OPTION_SHOWAVATARS))
	{
		roster->minw = 256;
		if (roster->w < 256)
		{
			Box_MoveWndCustom(roster, roster->x, roster->y, 256, roster->h);
		}
	}
	else
	{
		roster->minw = 128;
	}

	if (!data->isnub/* && Model_GetOption(OPTION_DEVFEATURES)*/)
	{
		data->gfinder->flags |= BOX_VISIBLE;
	}

	Box_Repaint(roster);
}

void Roster_SetNewGamesButtonActive(struct Box_s *roster, int active)
{
	struct Rosterdata_s *data = roster->boxdata;

	Log_Write(0, "wutwut %d\n", active);

	if (active)
	{
		Button_SetNormalImg (data->gfinder, ImageMgr_GetSubImage("gameMenuSmall-hover",  "gameMenuSmall.png", 60, 0, 30, 23));
	}
	else
	{
		Button_SetNormalImg (data->gfinder, ImageMgr_GetSubImage("gameMenuSmall-normal", "gameMenuSmall.png",  0, 0, 30, 23));
	}

	Box_Repaint(data->gfinder);
}

void Roster_UpdatePings(struct Box_s *roster, int last, int avg, int spike)
{
	struct Rosterdata_s *data = roster->boxdata;
	char txt[1024];
	char num1[512], num2[512], num3[512];

	sprintf(num1, "%d", last);
	sprintf(num2, "%d", avg);
	sprintf(num3, "%d", spike);

	i18n_stringsub(txt, 1024, _("Last: %1ms Avg: %2ms Spike: %3ms"), num1, num2, num3);

	free(data->lagtext);
	data->lagtext = strdup(txt);

	if (avg < 400)
	{
		data->lagometer->img = ImageMgr_GetScaledImage("lagsmall3", "lag3.png", 32, 32);
	}
	else if (avg < 800)
	{
		data->lagometer->img = ImageMgr_GetScaledImage("lagsmall2", "lag2.png", 32, 32);
	}
	else
	{
		data->lagometer->img = ImageMgr_GetScaledImage("lagsmall1", "lag1.png", 32, 32);
	}

	Box_Repaint(data->lagometer);
}