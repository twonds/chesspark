#include <stdio.h>
#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "edit.h"
#include "titledrag.h"
#include "text.h"

#include "constants.h"

#include "autosize.h"
#include "button2.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "imagemgr.h"
#include "list.h"
#include "log.h"
#include "menu.h"
#include "model.h"
#include "view.h"
#include "scrollable.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"

#include "add.h"
#include "namedlist.h"
#include "info.h"


struct profiledata_s
{
	char *jid;
	int local;
	struct Box_s *nickedit;
	struct Box_s *error;
	struct Box_s *ratinglist;
	struct Box_s *descriptionbox;
	struct Box_s *descriptionboxscrollable;
	struct Box_s *profileheader;
	struct Box_s *savebutton;
	struct Box_s *cancelbutton;
	struct Box_s *loadingtext;
	struct Box_s *clientinfo;
	struct Box_s *avatarbox;
	struct Box_s *avatarnormal;
	struct Box_s *avatarhover;
	struct Box_s *flagbox;
	struct Box_s *countryname;
	struct Box_s *locationinfo;
	struct Box_s *rankinfo;
	struct Box_s *presenceicon;
	struct Box_s *presencetext;
	struct Box_s *groupinfo;
	struct Box_s *sizeablecontent;

	struct namedlist_s *playinggames;
	struct namedlist_s *watchinggames;

	int normx;
	int normy;
	int normw;
	int normh;
};

void Profile_SetProfile(char *jid, struct profile_s *profile, struct Box_s *dialog);

void Profile_Error(struct Box_s *pbox, char *error)
{
	struct profiledata_s *data = pbox->boxdata;
	
	data->error->fgcol = RGB(255, 0, 0);
	Text_SetText(data->error, error);
	Box_Repaint(data->error);
}


void Profile_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}


void Profile_OnSave(struct Box_s *pbox)
{
	struct profiledata_s *data = pbox->parent->boxdata;
	char *nick = NULL;
	
	if (data->nickedit)
	{
		nick = Edit2Box_GetText(data->nickedit);
	}

	if (nick && *nick == '\0')
	{
		nick = NULL;
	}

	if (data->local)
	{
		Ctrl_SetProfileDescription(Edit2Box_GetText(data->descriptionbox));
	}

	Ctrl_SaveProfile(data->jid, nick);

	Profile_OnCancel(pbox);
}

void ProfileMenu_PopupProfileMenu(struct Box_s *pbox)
{
	struct profiledata_s *data = pbox->parent->boxdata;
	int x, y;

	Box_GetScreenCoords(pbox, &x, &y);
	x += pbox->w / 2;
	y += pbox->h / 2;

	Menu_PopupProfileMenu(pbox->parent, data->jid, x, y, data->playinggames, data->watchinggames);
}

void Profile_OnRestore(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_RESTORE);
}

void Profile_OnMinimize(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_MINIMIZE);
}

void Profile_OnMaximize(struct Box_s *pbox)
{
	int maxx, maxy, maxw, maxh;
	RECT rc;
	struct profiledata_s *data;

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
		data->normx = pbox->x;
		data->normy = pbox->y;
		data->normw = pbox->w;
		data->normh = pbox->h;
		
		Box_MoveWndCustom(pbox, maxx, maxy, maxw, maxh);
		
		Box_ForceDraw(pbox);
	}
	else if (data->normh != 0)
	{
		Box_MoveWndCustom(pbox, data->normx, data->normy, data->normw, data->normh);

		Box_ForceDraw(pbox);
	}
}

void Profile_OnDestroy(struct Box_s *dialog)
{
	struct profiledata_s *data = dialog->boxdata;

	Model_UnsubscribeProfile(data->jid, Profile_SetProfile, dialog);
}

struct Box_s *Profile_Create(struct Box_s *roster, char *jid, char *nickname, int local)
{
	struct profiledata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *pbox, *horizsize;
	char txt[120];
	int x, y;
	int isfriend = Model_JidInRoster(jid);

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;

		windowrect.left = roster->x;
		windowrect.right = windowrect.left + roster->w - 1;
		windowrect.top = roster->y;
		windowrect.bottom = windowrect.top + roster->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - 655) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - 525+45) / 2;
	}

	memset(data, 0, sizeof(*data));
	data->jid = strdup(jid);
	Log_Write(0, "data->jid %s\n", data->jid);
	data->local = local;

	dialog = Box_Create(x, y, 655, 525+45-80, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;

	i18n_stringsub(txt, 120, _("Information about %1"), jid);
	dialog->titlebar   = TitleBar_Add2(dialog, txt, NULL, Profile_OnMinimize, Profile_OnMaximize, 0);
	dialog->OnActive   = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	dialog->OnMinimize = Profile_OnMinimize;
	dialog->OnMaximize = Profile_OnMaximize;
	dialog->OnRestore  = Profile_OnRestore;

	dialog->OnDestroy  = Profile_OnDestroy;
	dialog->OnCommand  = Menu_OnCommand;

	pbox = Text_Create(20, 40, 400, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->fgcol = UserInfoFG2;
	{
		char text[512];

		sprintf(text, "^b%s", _("Profile"));

		Text_SetText(pbox, text);
	}
	Text_SetLinkColor(pbox, UserInfoFG1);
	Box_AddChild(dialog, pbox);
	data->profileheader = pbox;

	horizsize = AutoSize_Create(20, 55, 0, 200, 0, 0, AUTOSIZE_HORIZ);
	Box_AddChild(dialog, horizsize);
	data->sizeablecontent = horizsize;
	{
		struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		Box_AddChild(horizsize, vertsize);
		{
			struct Box_s *avatarholder;
			
			avatarholder = Box_Create(0, 0, 60, 60, BOX_VISIBLE | BOX_TRANSPARENT);
			Box_AddChild(vertsize, avatarholder);

			if (local)
			{
				/*pbox = Box_Create(210, 5, 40, 40, BOX_VISIBLE);*/
				pbox = Button2_Create(0, 0, 60, 60, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
				Button2_SetOnButtonHit(pbox, Menu_PopupAvatarMenu);
				Button2_SetTooltipText(pbox, _("Avatar Menu"));
				Box_AddChild(avatarholder, pbox);
				data->avatarbox = pbox;

				{
					struct Box_s *subbox, *subbox2;

					subbox = Box_Create(0, 0, 60, 60, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
					subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
					Box_AddChild(avatarholder, subbox);
					data->avatarnormal = subbox;

					subbox = Box_Create(0, 0, 60, 60, BOX_TRANSPARENT | BOX_FITASPECTIMG);
					subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
					Box_AddChild(avatarholder, subbox);

					subbox2 = Box_Create(0, 0, 60, 60, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
					subbox2->img = ImageMgr_GetImage("avatarhover.png");
					Box_AddChild(subbox, subbox2);

					data->avatarhover = subbox;

					Button2_SetNormal(data->avatarbox, data->avatarnormal);
					Button2_SetHover(data->avatarbox, data->avatarhover);
				}
			}
			else
			{
				pbox = Box_Create(0, 0, 60, 60, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
				data->avatarbox = pbox;
				Box_AddChild(avatarholder, pbox);
			}

			pbox = Box_Create(0, 0, 62, 43, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			data->flagbox = pbox;
			Box_AddChild(vertsize, pbox);
		}

		AutoSize_AddSpacer(horizsize, 20);

		vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		vertsize->OnSizeHeight = Box_OnSizeHeight_Stretch;
		Box_AddChild(horizsize, vertsize);
		{
			struct Box_s *holder, *horizsize, *descriptionholder;

			holder = Box_Create(0, 0, 300, 16, BOX_VISIBLE | BOX_TRANSPARENT);
			Box_AddChild(vertsize, holder);
			{
				pbox = Box_Create(-20, 0, 16, 16, BOX_TRANSPARENT | BOX_NOCLIP);
                                pbox->img = ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16);
				Box_AddChild(holder, pbox);
				data->presenceicon = pbox;

				pbox = Text_Create(0, 0, 300, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
				pbox->fgcol = UserInfoFG2;
				Box_AddChild(holder, pbox);
				data->presencetext = pbox;
			}

			horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			Box_AddChild(vertsize, horizsize);
			{
				int ischessparkid = (strstr(jid, "@chesspark.com") != NULL);
				pbox = Box_Create(0, 0, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox->fgcol = UserInfoFG2;
				if (ischessparkid)
				{
					Box_SetText(pbox, _("Username"));
				}
				else
				{
					Box_SetText(pbox, _("Jabber ID"));
				}
				Box_AddChild(horizsize, pbox);

				pbox = Text_Create(0, 0, 200, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_SELECTABLE | TX_COPYMENU);
				pbox->fgcol = RGB(255, 255, 255);
				if (ischessparkid)
				{
					Text_SetText(pbox, Jid_GetBeforeAt(jid));
				}
				else
				{
					Text_SetText(pbox, jid);
				}
				Box_AddChild(horizsize, pbox);
			}

			if (local)
			{
				horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
				Box_AddChild(vertsize, horizsize);
				{
					pbox = Box_Create(0, 0, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
					pbox->fgcol = UserInfoFG2;
					Box_SetText(pbox, _("Nickname"));
					Box_AddChild(horizsize, pbox);

					pbox = Edit2Box_Create(0, -2, 200, 18, BOX_VISIBLE, E2_HORIZ);
					pbox->bgcol = RGB(255, 255, 255);
					pbox->fgcol = RGB(0, 0, 0);
					Edit2Box_SetText(pbox, nickname);
					Box_AddChild(horizsize, pbox);
					data->nickedit = pbox;
				}
			}

			horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			Box_AddChild(vertsize, horizsize);
			{
				pbox = Box_Create(0, 0, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox->fgcol = UserInfoFG2;
				Box_SetText(pbox, _("Country"));
				Box_AddChild(horizsize, pbox);

				pbox = Text_Create(0, 0, 200, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_SELECTABLE | TX_COPYMENU);
				pbox->fgcol = RGB(255, 255, 255);
				Box_AddChild(horizsize, pbox);
				data->countryname = pbox;
			}

			horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			Box_AddChild(vertsize, horizsize);
			{
				pbox = Box_Create(0, 0, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox->fgcol = UserInfoFG2;
				Box_SetText(pbox, _("Location"));
				Box_AddChild(horizsize, pbox);

				pbox = Text_Create(0, 0, 200, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_SELECTABLE | TX_COPYMENU);
				pbox->fgcol = RGB(255, 255, 255);
				Box_AddChild(horizsize, pbox);
				data->locationinfo = pbox;
			}

			horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			Box_AddChild(vertsize, horizsize);
			{
				pbox = Box_Create(0, 0, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox->fgcol = UserInfoFG2;
				Box_SetText(pbox, _("Rank"));
				Box_AddChild(horizsize, pbox);

				pbox = Text_Create(0, 0, 200, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_SELECTABLE | TX_COPYMENU);
				pbox->fgcol = RGB(255, 255, 255);
				Box_AddChild(horizsize, pbox);
				data->rankinfo = pbox;
			}
			
#ifdef CHESSPARK_GROUPS
			horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			Box_AddChild(vertsize, horizsize);
			{
				pbox = Box_Create(0, 0, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox->fgcol = UserInfoFG2;
				Box_SetText(pbox, _("Groups"));
				Box_AddChild(horizsize, pbox);

				pbox = Text_Create(0, 0, 200, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_SELECTABLE | TX_COPYMENU | TX_STRETCHVERT | TX_WRAP);
				Text_SetLinkColor(pbox, CR_LtOrange);
				pbox->fgcol = RGB(255, 255, 255);
				Box_AddChild(horizsize, pbox);
				data->groupinfo = pbox;
			}
#endif
			pbox = Box_Create(0, 0, 0, 0, BOX_VISIBLE | BOX_TRANSPARENT);
			pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
			Box_AddChild(vertsize, pbox);

			{
				struct Box_s *vertsize2 = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
				vertsize2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
				Box_AddChild(vertsize, vertsize2);

				pbox = Box_Create(0, 0, 260, 16, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox->fgcol = UserInfoFG2;
				i18n_stringsub(txt, 120, _("More about %1:"), Model_GetFriendNick(jid));
				Box_SetText(pbox, txt);
				Box_AddChild(vertsize2, pbox);

				pbox = Box_Create(0, 0, 255, 70, BOX_VISIBLE | BOX_BORDER);
				if (local)
				{
					pbox->bgcol = RGB(255, 255, 255);
				}
				else
				{
					pbox->bgcol = RGB(31, 37, 48);
				}
				pbox->brcol = UserInfoFG2;
				Box_AddChild(vertsize2, pbox);

				descriptionholder = pbox;

				if (local)
				{
					pbox = Edit2Box_Create(2, 2, 251, 66, BOX_VISIBLE, 0);
					pbox->bgcol = RGB(255, 255, 255);
					Box_AddChild(descriptionholder, pbox);
					data->descriptionbox = pbox;
				}
				else
				{
					struct Box_s *sable;

					sable = Scrollable_Create(2, 2, 251, 66, BOX_VISIBLE);
					sable->bgcol = RGB(31, 37, 48);
					Box_AddChild(descriptionholder, sable);
					data->descriptionboxscrollable = sable;

					pbox = Text_Create(0, 0, 251, 66, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_SELECTABLE | TX_NOCOMMANDS | TX_STRETCHVERT);
					pbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
					pbox->fgcol = CR_White;
					Scrollable_SetBox(sable, pbox);
					data->descriptionbox = pbox;

					Scrollable_Refresh(sable);
				}
			}
		}

		AutoSize_AddSpacer(horizsize, 20);

		vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		vertsize->OnSizeHeight = Box_OnSizeHeight_Stretch;
		Box_AddChild(horizsize, vertsize);
		{
			pbox = Text_Create(0, 0, 200, 100, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
			pbox->fgcol = UserInfoFG2;
			data->clientinfo = pbox;
			Box_AddChild(vertsize, pbox);

			if (isfriend && !local)
			{
				struct Box_s *vertsize2 = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
				vertsize2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
				Box_AddChild(vertsize, vertsize2);

				pbox = Box_Create(0, 0, 150, 16, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox->fgcol = UserInfoFG2;
				Box_SetText(pbox, _("Name"));
				Box_AddChild(vertsize2, pbox);

				pbox = Edit2Box_Create(0, -2, 150, 18, BOX_VISIBLE, E2_HORIZ);
				pbox->bgcol = RGB(255, 255, 255);
				pbox->fgcol = RGB(0, 0, 0);
				Edit2Box_SetText(pbox, nickname);
				Box_AddChild(vertsize2, pbox);
				data->nickedit = pbox;

				pbox = Text_Create(0, 0, 150, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
				pbox->fgcol = UserInfoFG2;
				{
					char text[512];

					sprintf(text, "%s\n%s", _("This is how your friend will be shown on your friends list."), _("For example: Jane Doe, Chessfan1000"));

					Text_SetText(pbox, text);
				}
				Box_AddChild(vertsize2, pbox);
			}
		}
	}

	AutoSize_Fit(data->sizeablecontent);
	AutoSize_Fill(data->sizeablecontent);

#if 0
	pbox = Box_Create(30, 65, 60, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Jabber ID"));
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(30, 80, 310, 20, BOX_VISIBLE | BOX_BORDER);
	pbox->bgcol = RGB(31, 37, 48);
	pbox->brcol = UserInfoFG2;
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(32, 82, 306, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_SELECTABLE | TX_COPYMENU);
	pbox->fgcol = RGB(255, 255, 255);
	Text_SetText(pbox, jid);
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(30, 130, 210, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->fgcol = UserInfoFG2;
	Text_SetLinkColor(pbox, CR_LtOrange);
	{
		char text[512];

		sprintf(text, "%s (^l%s^l)", _("Chesspark Role"), _("Learn More..."));

		Text_SetText(pbox, text);
	}
	Text_SetLinkCallback(pbox, 1, Util_OpenURL2, _("http://chesspark.com/help/roles/"));
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(30, 150, 260, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = RGB(255, 255, 255);
	Box_AddChild(dialog, pbox);
	data->rolesbox = pbox;

	pbox = Box_Create(30, 190, 310, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Name"));
	Box_AddChild(dialog, pbox);

	pbox = Edit2Box_Create(30, 205, 310, 20, BOX_VISIBLE, E2_HORIZ);
	pbox->bgcol = RGB(255, 255, 255);
	pbox->fgcol = RGB(0, 0, 0);
	Edit2Box_SetText(pbox, nickname);
	Box_AddChild(dialog, pbox);
	data->nickedit = pbox;

	pbox = Text_Create(30, 230, 310, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
	pbox->fgcol = UserInfoFG2;
	{
		char text[512];

		sprintf(text, "%s\n%s", _("This is how your friend will be shown on your friends list."), _("For example: Jane Doe, Chessfan1000"));

		Text_SetText(pbox, text);
	}
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(375, 175, 260, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	i18n_stringsub(txt, 120, _("More about %1"), jid);
	Box_SetText(pbox, txt);
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(370, 190, 255, 70, BOX_VISIBLE | BOX_BORDER);
	if (local)
	{
		pbox->bgcol = RGB(255, 255, 255);
	}
	else
	{
		pbox->bgcol = RGB(31, 37, 48);
	}
	pbox->brcol = UserInfoFG2;
	Box_AddChild(dialog, pbox);

	if (local)
	{
		pbox = Edit2Box_Create(372, 192, 251, 66, BOX_VISIBLE, 0);
		pbox->bgcol = RGB(255, 255, 255);
		Box_AddChild(dialog, pbox);
		data->descriptionbox = pbox;
	}
	else
	{
		struct Box_s *sable;

		sable = Scrollable_Create(372, 192, 251, 66, BOX_VISIBLE);
		sable->bgcol = RGB(31, 37, 48);
		Box_AddChild(dialog, sable);
		data->descriptionboxscrollable = sable;

		pbox = Text_Create(0, 0, 251, 66, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_SELECTABLE | TX_NOCOMMANDS | TX_STRETCHVERT);
		pbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
		pbox->fgcol = CR_White;
		Scrollable_SetBox(sable, pbox);
		data->descriptionbox = pbox;

		Scrollable_Refresh(sable);
	}

	if (local)
	{
		/* Avatar */
		/*pbox = Box_Create(210, 5, 40, 40, BOX_VISIBLE);*/
		pbox = Button2_Create(370, 60, 50, 50, BOX_VISIBLE | BOX_TRANSPARENT);
		pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Button2_SetOnButtonHit(pbox, Menu_PopupAvatarMenu);
		Button2_SetTooltipText(pbox, _("Avatar Menu"));
		Box_AddChild(dialog, pbox);
		data->avatarbox = pbox;

		{
			struct Box_s *subbox, *subbox2;

			subbox = Box_Create(370, 60, 50, 50, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Box_AddChild(dialog, subbox);
			data->avatarnormal = subbox;

			subbox = Box_Create(370, 60, 50, 50, BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Box_AddChild(dialog, subbox);

			subbox2 = Box_Create(0, 0, 50, 50, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox2->img = ImageMgr_GetImage("avatarhover.png");
			Box_AddChild(subbox, subbox2);

			data->avatarhover = subbox;

			Button2_SetNormal(data->avatarbox, data->avatarnormal);
			Button2_SetHover(data->avatarbox, data->avatarhover);
		}
	}
	else
	{
                pbox = Box_Create(370, 60, 50, 50, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
		data->avatarbox = pbox;
		Box_AddChild(dialog, pbox);
	}

	pbox = Box_Create(370, 120, 62, 43, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
	data->flagbox = pbox;
	/*pbox->img = ImageMgr_GetImage("canada.png");*/
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(440, 45, 200, 80, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_CENTERVERT);
	pbox->fgcol = UserInfoFG2;
	data->clientinfo = pbox;
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(440, 125, 200, 30, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_CENTERVERT);
	pbox->fgcol = UserInfoFG2;
	data->flaginfo = pbox;
	Box_AddChild(dialog, pbox);
#endif

/*
	pbox = Box_Create(440, 70, 310, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	data->membersince = pbox;
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(440, 85, 310, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	data->lastonline = pbox;
	Box_AddChild(dialog, pbox);
*/
	pbox = Box_Create(10, 235+30, 635, 1, BOX_VISIBLE);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->bgcol = RGB(31, 37, 48);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(10, 236+30, 635, 1, BOX_VISIBLE);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->bgcol = UserInfoFG2;
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(20, 260+30, 180, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->fgcol = UserInfoFG2;
	Text_SetLinkColor(pbox, CR_LtOrange);
	{
		char text[512];

		sprintf(text, "^b%s^b (^l%s^l)", _("Ratings"), _("Learn More..."));

		Text_SetText(pbox, text);
	}
	Text_SetLinkCallback(pbox, 1, Util_OpenURL2, _("http://chesspark.com/help/ratings/"));
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(40, 310+30, 60, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->fgcol = UserInfoFG2;
	{
		char text[512];

		sprintf(text, "^b%s", _("Category"));

		Text_SetText(pbox, text);
	}
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(210, 290+30, 70, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->fgcol = UserInfoFG2;
	{
		char text[512];

		sprintf(text, "^b%s", _("Rating Stats"));

		Text_SetText(pbox, text);
	}
	Box_AddChild(dialog, pbox);
/*
	pbox = Box_Create(210, 310, 50, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, "Level");
	Box_AddChild(dialog, pbox);
*/
	pbox = Text_Create(275 - 65, 310+30, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, _("Rating"));
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(360 - 35, 310+30, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, _("Best"));
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(440, 290+30, 75, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->fgcol = UserInfoFG2;
	{
		char text[512];

		sprintf(text, "^b%s", _("Games Stats"));

		Text_SetText(pbox, text);
	}
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(440, 310+30, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, _("Total"));
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(480, 310+30, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, _("W"));
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(520, 310+30, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, _("L"));
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(560, 310+30, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, _("D"));
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(30, 325+30, dialog->w - 60, dialog->h - 60 - 325-30, BOX_VISIBLE | BOX_BORDER);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->bgcol = RGB(31, 37, 48);
	pbox->brcol = UserInfoFG2;
	Box_AddChild(dialog, pbox);

	pbox = List_Create(30 + 5, 325 + 5+30, dialog->w - 60 - 10, dialog->h - 60 - 10 - 325-30, BOX_VISIBLE, 0);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	List_SetStripeBG1(pbox, DefaultBG);
	List_SetStripeBG2(pbox, RGB(31, 37, 48));
	List_SetStripeBG(pbox, RGB(31, 37, 48));
	List_SetEntrySelectable(pbox, 0);
	Box_AddChild(dialog, pbox);
	data->ratinglist = pbox;

	pbox = Box_Create(205, 285+30, 1, dialog->h - 60 - 285-30, BOX_VISIBLE);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->bgcol = UserInfoFG2;
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(435, 285+30, 1, dialog->h - 60 - 285-30, BOX_VISIBLE);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->bgcol = UserInfoFG2;
	Box_AddChild(dialog, pbox);

	pbox = Button_Create(20, dialog->h - 32, 30, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button_SetOnButtonHit(pbox, ProfileMenu_PopupProfileMenu);
	Button_SetNormalImg (pbox, ImageMgr_GetSubImage("preferenceMenuButton1", "preferenceMenu.png", 0,  0, 30, 23));
	Button_SetPressedImg(pbox, ImageMgr_GetSubImage("preferenceMenuButton2", "preferenceMenu.png", 30, 0, 30, 23));
	Button_SetHoverImg  (pbox, ImageMgr_GetSubImage("preferenceMenuButton3", "preferenceMenu.png", 60, 0, 30, 23));
	Button_SetTooltipText(pbox, _("Friend Menu"));
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(dialog->w - 100, dialog->h - 32, 80, _("Cancel"), 0);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, Profile_OnCancel);
	Box_AddChild(dialog, pbox);
	data->cancelbutton = pbox;
	
	pbox = StdButton_Create(dialog->w - 200, dialog->h - 32, 80, _("Save"), 0);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, Profile_OnSave);
	Box_AddChild(dialog, pbox);
	data->savebutton = pbox;

	pbox = Box_Create(60, dialog->h - 32, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox,_("Requesting profile..."));
	Box_AddChild(dialog, pbox);
	data->loadingtext = pbox;

	dialog->boxdata = data;

	if (local)
	{
		data->nickedit->nextfocus = data->descriptionbox;
		data->descriptionbox->nextfocus = data->savebutton;
		data->savebutton->nextfocus = data->cancelbutton;
		data->cancelbutton->nextfocus = data->nickedit;

		data->nickedit->prevfocus = data->cancelbutton;
		data->descriptionbox->prevfocus = data->nickedit;
		data->savebutton->prevfocus = data->descriptionbox;
		data->cancelbutton->prevfocus = data->savebutton;
	}
	else if (isfriend)
	{
		data->nickedit->nextfocus = data->savebutton;
		data->savebutton->nextfocus = data->cancelbutton;
		data->cancelbutton->nextfocus = data->nickedit;

		data->nickedit->prevfocus = data->cancelbutton;
		data->savebutton->prevfocus = data->nickedit;
		data->cancelbutton->prevfocus = data->savebutton;
	}
	else
	{
		data->savebutton->nextfocus = data->cancelbutton;
		data->cancelbutton->nextfocus = data->cancelbutton;

		data->savebutton->prevfocus = data->cancelbutton;
		data->cancelbutton->prevfocus = data->savebutton;
	}
	
	i18n_stringsub(txt, 120, _("Information about %1"), jid);
	Box_CreateWndCustom(dialog, txt, roster->hwnd);

	Model_SubscribeProfileForceRequest(data->jid, Profile_SetProfile, dialog);

	BringWindowToTop(dialog->hwnd);

	return dialog;
}

extern HFONT tahoma10_f;

void Profile_SetProfile(char *jid, struct profile_s *profile, struct Box_s *dialog)
{
	struct profiledata_s *data = dialog->boxdata;
	char *jid1 = Jid_Strip(jid);
	char *jid2 = Jid_Strip(data->jid);
	struct namedlist_s **ppentry = NULL;
	struct namedlist_s *allratings = NULL;
	struct namedlist_s *ratinglist = NULL;
	struct namedlist_s *roles = NULL;
	struct rating_s *rating = NULL;

	char txt[120];
	struct Box_s *subbox;

	if (strcmp(jid1, jid2) != 0)
	{
		free(jid1);
		free(jid2);
		return;
	}

	data->loadingtext->flags &= ~BOX_VISIBLE;
	Box_Repaint(data->loadingtext);

	List_RemoveAllEntries(data->ratinglist);

	{
		if (data->local)
		{
			Edit2Box_SetText(data->descriptionbox, profile->description);
		}
		else
		{
			Text_SetText(data->descriptionbox, profile->description);
			Scrollable_Refresh(data->descriptionboxscrollable);
		}
	}

	{
		ratinglist = profile->ratings;

		while (ratinglist)
		{
			struct Box_s *entrybox = Box_Create(0, 0, 100, 16, BOX_VISIBLE | BOX_TRANSPARENT);
			rating = ratinglist->data;

			subbox = Text_Create(5, 0, 170, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
			subbox->fgcol = RGB(255, 255, 255);
			Text_SetLinkColor(subbox, CR_LtOrange);
			sprintf(txt, "^l%s^l", ratinglist->name);
			Text_SetText(subbox, txt);
						{
				char urltxt[512];

				sprintf(urltxt, "http://chesspark.com/help/%s/", ratinglist->name);
				Text_SetLinkCallback(subbox, 1, Util_OpenURL2, Util_Lowercase(urltxt));
			}
			Box_AddChild(entrybox, subbox);
/*
			subbox = Text_Create(175, 0, 70, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
			subbox->fgcol = RGB(255, 255, 255);
			sprintf(txt, "Novice (?)");
			Text_SetText(subbox, txt);
			Box_AddChild(entrybox, subbox);*/
/*
	pbox = Text_Create(275, 310, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, "Rating");
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(360, 310, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, "Best");
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(440, 290, 50, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, "^bGames");
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(440, 310, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, "Total");
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(480, 310, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, "W");
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(520, 310, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, "L");
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(560, 310, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, "D");
	Box_AddChild(dialog, pbox);
*/
			subbox = Text_Create(240 - 65, 0, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
			subbox->fgcol = RGB(255, 255, 255);
			sprintf(txt, "%d", rating->rating);
			Text_SetText(subbox, txt);
			Box_AddChild(entrybox, subbox);

			subbox = Text_Create(285 - 65, 0, 35, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
			subbox->font = tahoma10_f;
			subbox->fgcol = RGB(255, 255, 255);
			sprintf(txt, "+/- %d", rating->rd);
			Text_SetText(subbox, txt);
			Box_AddChild(entrybox, subbox);

			subbox = Text_Create(325 - 35, 0, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
			subbox->fgcol = RGB(255, 255, 255);
			sprintf(txt, "%d", rating->best);
			Text_SetText(subbox, txt);
			Box_AddChild(entrybox, subbox);

			subbox = Text_Create(405, 0, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
			subbox->fgcol = RGB(255, 255, 255);
			sprintf(txt, "%d", rating->wins + rating->losses + rating->draws);
			Text_SetText(subbox, txt);
			Box_AddChild(entrybox, subbox);

			subbox = Text_Create(445, 0, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
			subbox->fgcol = RGB(255, 255, 255);
			sprintf(txt, "%d", rating->wins);
			Text_SetText(subbox, txt);
			Box_AddChild(entrybox, subbox);

			subbox = Text_Create(485, 0, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
			subbox->fgcol = RGB(255, 255, 255);
			sprintf(txt, "%d", rating->losses);
			Text_SetText(subbox, txt);
			Box_AddChild(entrybox, subbox);

			subbox = Text_Create(525, 0, 40, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
			subbox->fgcol = RGB(255, 255, 255);
			sprintf(txt, "%d", rating->draws);
			Text_SetText(subbox, txt);
			Box_AddChild(entrybox, subbox);

			List_AddEntry(data->ratinglist, NULL, NULL, entrybox);

			ratinglist = ratinglist->next;
		}
	}

	List_RedoEntries(data->ratinglist);
	Box_Repaint(data->ratinglist);

	roles = profile->roles;

	strcpy(txt, "^bProfile: ");
	strcat(txt, Model_GetFriendNick(data->jid));

	if (!roles)
	{
		if (profile->membertype)
		{
			if (stricmp(profile->membertype, "trial") == 0)
			{
				strcat(txt, " - ");
				strcat(txt, _("Trial"));
			}
			else
			{
				strcat(txt, " - ");
				strcat(txt, profile->membertype);
			}
		}
		else
		{
			strcat(txt, " - ");
			strcat(txt, _("Member"));
		}
	}

	while (roles)
	{
		char *role = roles->data;

		strcat(txt, " - ");
		strcat(txt, role);

		roles = roles->next;

		if (roles)
		{
			strcat(txt, ", ");
		}
	}

	strcat(txt, " (^l");
	strcat(txt, _("View Web Profile"));
	strcat(txt, "^l)");

	Text_SetText(data->profileheader, txt);
	{
		char urldest[512];
		char jidescaped[512];

		sprintf(urldest, "http://www.chesspark.com/people/%s/", EscapeURL(jid, jidescaped, 512));
                Text_SetLinkCallback(data->profileheader, 1, Util_OpenURL2, strdup(urldest));
	}

	if (profile->lastonline)
	{
		int diff;
		char showtime[256];

		data->presenceicon->flags &= ~BOX_VISIBLE;

		if (stricmp(profile->lastonline, "None") == 0)
		{
			i18n_stringsub(txt, 120, _("Never online."));
		}
		else if (stricmp(profile->lastonline, "online") == 0)
		{
			i18n_stringsub(txt, 120, _("Currently online."));
			data->presenceicon->flags |= BOX_VISIBLE;
		}
		else
		{
			diff = Info_GetTimeDiffFromNow(Info_ConvertTimestampToTimeT(profile->lastonline));
			Info_SecsToText2((float)(diff), showtime, NULL, 2);
		
			i18n_stringsub(txt, 120, _("Last online %1 ago."), showtime);
		}

		Text_SetText(data->presencetext, txt);
	}

	{
		char txt2[2048];

		txt2[0] = '\0';

		if (profile->membersince)
		{
			i18n_stringsub(txt, 120, _("^bMember since:^n %1"), Info_ConvertTimestampToLongDate(profile->membersince));
			strcat(txt2, txt);
			strcat(txt2, "\n");
		}

		/*
		if (profile->lastonline)
		{
			unsigned int diff;
			char showtime[256];

			if (stricmp(profile->lastonline, "None") == 0)
			{
				i18n_stringsub(txt, 120, _("Never online."));
			}
			else if (stricmp(profile->lastonline, "online") == 0)
			{
				i18n_stringsub(txt, 120, _("Currently online."));
			}
			else
			{
                                diff = Info_GetTimeDiffFromNow(Info_ConvertTimestampToTimeT(profile->lastonline));
				Info_SecsToText2((float)(diff), showtime, NULL, 2);
			
				i18n_stringsub(txt, 120, _("Last online %1 ago."), showtime);
			}
			strcat(txt2, txt);
			strcat(txt2, "\n");
		}
		*/

		if (profile->clientname)
		{
			/*
			if (profile->clientvendor && stricmp(profile->clientvendor, "unknown") != 0)
			{
				strcat(txt2, profile->clientvendor);
				strcat(txt2, " ");
			}
			*/

			strcat(txt2, "^b");
			strcat(txt2, profile->clientname);
			strcat(txt2, ":^n");

			if (profile->clientversion)
			{
				strcat(txt2, " ");
				strcat(txt2, profile->clientversion);
			}

			strcat(txt2, "\n");
		}

		if (profile->clientos)
		{
			strcat(txt2, "^bOperating System^n:\n");
			strcat(txt2, profile->clientos);
		}

		Text_SetText(data->clientinfo, txt2);
	}

	{
		struct BoxImage_s *img = NULL;
		char *filename = NULL;
		char scaledname[512];
		char buffer[MAX_PATH];

		if (profile && profile->avatarhash)
		{
			filename = Ctrl_GetAvatarFilenameByHash(profile->avatarhash, buffer, MAX_PATH);
			img = ImageMgr_GetRootImage(filename);
		}

		if (img)
		{
			sprintf(scaledname, "%s-60x60", filename);
			img = ImageMgr_GetRootAspectScaledImage(scaledname, filename, 60, 60);
		}

		if (!img)
		{
			img = ImageMgr_GetScaledImage("DefaultAvatar60x60", "DefaultAvatar.png", 60, 60);
		}

		if (data->local)
		{
			struct BoxImage_s *imgdim;
			char scaledname[512];
			char dimname[512];

			if (img)
			{
				sprintf(dimname, "%s-60x60dim", filename);
			
				imgdim = ImageMgr_GetRootDimmedImage(dimname, scaledname, 70);

				data->avatarnormal->img = img;
				data->avatarhover->img = imgdim;
			}
			else
			{
				data->avatarnormal->img = img;
				data->avatarhover->img = ImageMgr_GetAspectScaledTransImage("DefaultAvatar-dim50x50", "DefaultAvatar.png", 50, 50, 70);
			}
		}
		else
		{
                        data->avatarbox->img = img;
		}

		Box_Repaint(data->avatarbox);
	}

	{
		struct namedlist_s *entry;

		NamedList_Destroy(&(data->playinggames));
		entry = profile->playinggames;
		while (entry)
		{
			struct gamesearchinfo_s *info = entry->data;
			NamedList_Add(&(data->playinggames), entry->name, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
			entry = entry->next;
		}

		NamedList_Destroy(&(data->watchinggames));
		entry = profile->watchinggames;
		while (entry)
		{
			struct gamesearchinfo_s *info = entry->data;
			NamedList_Add(&(data->watchinggames), entry->name, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
			entry = entry->next;
		}
	}

	if (profile->countrycode)
	{
		char filename[512];
		char countrycodes[8192];
		FILE *fp;
		char *p;
		char countryname[512];

		fp = fopen("countrycodes.txt", "r");

		if (fp)
		{
			fread(countrycodes, 1, 8191, fp);
			countrycodes[8191] = '\0';
			fclose(fp);
		}
		else
		{
			countrycodes[0] = '\0';
			strcpy(countryname, profile->countrycode);
		}

		sprintf(filename, "flags/%s.png", profile->countrycode);

		data->flagbox->img = ImageMgr_GetImage(filename);
		countryname[0] = '\0';

		p = countrycodes;

		while (*p != '\0' && countryname[0] == '\0')
		{
			if (strnicmp(profile->countrycode, p, 2) == 0)
			{
				int i = 0;
				p += 2;
				while (*p == ' ' && *p != '\0')
				{
					p++;
				}
				while (*p != '\n' && *p != '\0')
				{
					countryname[i] = *p;
					i++;
					p++;
				}
				countryname[i] = '\0';
			}
			else
			{
				while (*p != '\n' && *p != '\0')
				{
					p++;
				}
				if (*p != '\0')
				{
					p++;
				}
			}
		}

		Text_SetText(data->countryname, countryname);

		if (profile->location)
		{
			Text_SetText(data->locationinfo, profile->location);
		}

		Box_Repaint(data->flagbox);
		Box_Repaint(data->countryname);
		Box_Repaint(data->locationinfo);
	}

	if (profile->rank)
	{
		Text_SetText(data->rankinfo, profile->rank);
	}

#ifdef CHESSPARK_GROUPS
	if (profile->groups)
	{
		struct namedlist_s *entry;
		char txt[16384];
		int first = 1;
		txt[0] = '\0';

		entry = profile->groups;
		while (entry)
		{
			struct groupinfo_s *ginfo = entry->data;
			int len = strlen(ginfo->name) + 4 + ((!first) ? 2 : 0) + 1; 

			if (16384 - strlen(txt) < len)
			{
				entry = NULL;
				break;
			}
			
			if (!first)
			{
				strcat(txt, ", ");
			}
			first = 0;

			strcat(txt, "^l");
			strcat(txt, ginfo->name);
			strcat(txt, "^l");

			entry = entry->next;
		}

		Text_SetText(data->groupinfo, txt);

		first = 1;
		entry = profile->groups;
		while (entry)
		{
			char url[512];
			char escapedgroup[512];
			struct groupinfo_s *ginfo = entry->data;

			sprintf(url, "http://www.chesspark.com/groups/%s/", EscapeURL(ginfo->name, escapedgroup, 512));

			Text_SetLinkCallback(data->groupinfo, first, Util_OpenURL2, strdup(url));

			first++;
			entry = entry->next;
		}
	}
#endif

	/*Box_Repaint(data->rolesbox);*/
	AutoSize_Fit(data->sizeablecontent);
	AutoSize_Fill(data->sizeablecontent);

	Box_MoveWndCustom(dialog, dialog->x, dialog->y, dialog->w, 525+45-80-200+data->sizeablecontent->h);

	Box_Repaint(dialog);
}

static void JidLink_ShowMenu(struct Box_s *pbox, char *jid, int x, int y)
{
	Menu_PopupProfileMenu(pbox->parent, jid, x, y, NULL, NULL);
}

struct Box_s *MiniProfile_Create(int x, int y, int w, int h, char *jid)
{
	struct Box_s *miniprofilebox = Box_Create(x, y, w, h, BOX_VISIBLE | BOX_BORDER);
	struct Box_s *subbox;
	char txt[512];

	miniprofilebox->brcol = RGB(90, 97, 108);
	miniprofilebox->bgcol = DrawerBG;

	subbox = Text_Create(15, 8, miniprofilebox->w - 30, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	subbox->fgcol = UserInfoFG1;
	sprintf(txt, "^l%s^l", Model_GetFriendNick(jid));
	Text_SetText(subbox, txt);
	Text_SetLinkCallback(subbox, 1, ViewLink_ShowProfile,  strdup(jid));
	Text_SetRLinkCallback(subbox, 1, JidLink_ShowMenu, strdup(jid));
	Box_AddChild(miniprofilebox, subbox);

	subbox = Box_Create(55, 26, miniprofilebox->w - 30, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = RGB(222, 222, 222);
	Box_SetText(subbox, _("Requesting profile..."));
	Box_AddChild(miniprofilebox, subbox);

	subbox = Box_Create(miniprofilebox->w - 35, (miniprofilebox->h - 30) / 2, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
	subbox->img = ImageMgr_GetScaledImage("DefaultAvatar30x30", "DefaultAvatar.png", 30, 30);
	Box_AddChild(miniprofilebox, subbox);

	return miniprofilebox;
}



void MiniProfile_SetProfile(struct Box_s *miniprofilebox, char *jid, struct profile_s *profile)
{
	struct namedlist_s **allratingslist = NULL;
	struct namedlist_s *allratings = NULL;
	struct namedlist_s **ratinglist = NULL;
	struct rating_s *rating = NULL;
	
	struct Box_s *subbox;

	while (miniprofilebox->child)
	{
		Box_Destroy(miniprofilebox->child);
	}

	subbox = Text_Create(15, 8, miniprofilebox->w - 30, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	Text_SetLinkColor(subbox, UserInfoFG1);
	subbox->fgcol = UserInfoFG1;

	{
		char txt[512];
		int notfirst = 0;
		struct namedlist_s *entry;

		char *barejid = Jid_Strip(jid);

		txt[0] = '\0';
		strcat(txt, "^l");

		if (profile)
		{
			entry = profile->titles;

			while (entry)
			{
				if (notfirst)
				{
					strcat(txt, " ");
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
		}

		strcat(txt, (profile && profile->nickname) ? profile->nickname : Model_GetFriendNick(jid));
		free(barejid);
			
		strcat(txt, "^l  ^3");

		notfirst = 0;

		if (profile)
		{
			entry = profile->roles;

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
		}

		Text_SetText(subbox, txt);
		Text_SetLinkCallback(subbox, 1, ViewLink_ShowProfile,  strdup(jid));
		Text_SetRLinkCallback(subbox, 1, JidLink_ShowMenu, strdup(jid));
	}

	Box_AddChild(miniprofilebox, subbox);

	subbox = Box_Create(15, 26, miniprofilebox->w - 30, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = RGB(222, 222, 222);
	Box_AddChild(miniprofilebox, subbox);

	{
		struct BoxImage_s *parentimage, *finalimage = NULL;
		char smallname[256];

		if (profile && profile->avatarhash)
		{
			char *filename;
			char buffer[MAX_PATH];

			filename = Ctrl_GetAvatarFilenameByHash(profile->avatarhash, buffer, MAX_PATH);

                        parentimage = ImageMgr_GetRootImage(filename);

			if (parentimage)
			{
				sprintf(smallname, "%s-30x30", filename);

				finalimage = ImageMgr_GetRootAspectScaledImage(smallname, filename, 30, 30);
			}
		}

		if (!finalimage)
		{
			finalimage = ImageMgr_GetScaledImage("DefaultAvatar30x30", "DefaultAvatar.png", 30, 30);
		}

		subbox = Box_Create(miniprofilebox->w - 35, (miniprofilebox->h - 30) / 2, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
		subbox->img = finalimage;
		Box_AddChild(miniprofilebox, subbox);
	}

	Box_Repaint(miniprofilebox);
}
