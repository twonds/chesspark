#include <stdio.h>
#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "titledrag.h"
#include "text.h"

#include "constants.h"

#include "autosize.h"
#include "boxtypes.h"
#include "button2.h"
#include "ctrl.h"
#include "i18n.h"
#include "info.h"
#include "model.h"
#include "namedlist.h"
#include "profile.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

#include "add.h"


struct gameinviteerrordata_s
{
	char *jid;

	struct namedlist_s *ratinglist;

	struct Box_s *sizeablecontent;

	struct Box_s *messagebox;

	struct Box_s *opponentbox;
	struct Box_s *cancelbutton;
	struct Box_s *openchatbutton;
};

void GameInviteError_OnChat(struct Box_s *pbox)
{
	struct gameinviteerrordata_s *data = Box_GetRoot(pbox)->boxdata;

	Model_PopupChatDialog(data->jid, 0);
}

void GameInviteError_OnDecline(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}


void GameInviteError_OnClose(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}

#if 0
void GameInviteError_ShowProfileRating(struct Box_s *dialog, char *variant, char *timecontrol)
{
	struct gameinviteerrordata_s *data = dialog->boxdata;
	struct namedlist_s **ratinglistentry = NULL;
	struct rating_s *rating = NULL;

	char txt[512];
	char ratingtext[80];
	char *category = NULL;

	category = variant;

	if (timecontrol && stricmp(timecontrol, "correspondence") == 0)
	{
		category = "correspondence";
	}

	ratinglistentry = NamedList_GetByName(&(data->ratinglist), category);

	if (ratinglistentry)
	{
		rating = (*ratinglistentry)->data;
	}

	if (rating)
	{
		sprintf(ratingtext, "%d +/- %d", rating->rating, rating->rd);
	}
	else
	{
		sprintf(ratingtext, _("Unrated"));
	}

	i18n_stringsub(txt, 512, _("Rating for %1 games: %2"), category, ratingtext);
	Box_SetText(data->opponentbox->child->sibling, txt);

	Box_Repaint(data->opponentbox);
}

void GameInviteError_SetProfile(char *jid, struct profile_s *profile, struct Box_s *dialog)
{
	struct gameinviteerrordata_s *data = dialog->boxdata;
	char *jid1 = Jid_Strip(jid);
	char *jid2 = Jid_Strip(data->jid);
	
	struct namedlist_s **allratingslist = NULL;
	struct namedlist_s *allratings = NULL;
	struct namedlist_s **ratinglist = NULL;
	struct rating_s *rating = NULL;
	/*
	char txt[120];
	struct Box_s *subbox;
	*/
	char *variant = NULL, *timecontrol = NULL;

	if (stricmp(jid1, jid2) != 0)
	{
		free(jid1);
		free(jid2);
		return;
	}

	MiniProfile_SetProfile(data->opponentbox, jid, profile);

	data->ratinglist = Info_DupeRatingList(profile->ratings);

	GameInviteError_ShowProfileRating(dialog, data->info->variant, Info_TimeControlToCategory(data->info->timecontrol));

	Box_Repaint(data->opponentbox);

}
#endif

void GameInviteError_OnDestroy(struct Box_s *dialog)
{
	struct gameinviteerrordata_s *data = dialog->boxdata;
	/*Model_UnsubscribeProfile(data->jid, GameInviteError_SetProfile, dialog);*/
}

struct Box_s *GameInviteError_Create(struct Box_s *roster, char *fromjid, char *error, char *actor, int sent)
{
	struct gameinviteerrordata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *pbox, *vertsize;
	char titlebartxt[512];

	memset(data, 0, sizeof(*data));

	dialog = Box_Create(0, 0, 440, 270, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;

	dialog->boxdata = data;

	data->jid = strdup(fromjid);

	{
		i18n_stringsub(titlebartxt, 512, _("Game Invitation From %1"), Model_GetFriendNick(fromjid));
	}
	dialog->titlebar = TitleBarOnly_Add(dialog, titlebartxt);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	vertsize = AutoSize_Create(10, 43, 0, 0, 0, 0, AUTOSIZE_VERT);
	Box_AddChild(dialog, vertsize);
	data->sizeablecontent = vertsize;
	{
		struct Box_s *horizsize;

		pbox = Text_Create(10, 2, 280, 60, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
		/*pbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
		pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
		*/
		pbox->fgcol = RGB(222, 222, 222);
		Box_AddChild(vertsize, pbox);
		data->messagebox = pbox;

		AutoSize_AddSpacer(vertsize, 37);

		horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
		horizsize->OnSizeWidth = Box_OnSizeWidth_Stretch;
		Box_AddChild(vertsize, horizsize);
		{
			struct Box_s *horizsize2 = AutoSizeSpace_Create(0, 0, 0, 0, 0, 0, 10, AUTOSIZE_HORIZ);
			Box_AddChild(horizsize, horizsize2);
			{
				pbox = StdButton_Create(0, 0, 90, _("Open Chat"), 0);
				Button2_SetOnButtonHit(pbox, GameInviteError_OnChat);
				Box_AddChild(horizsize2, pbox);
				data->openchatbutton = pbox;

				AutoSize_AddSpacer(horizsize2, 0);
			}

			horizsize2 = AutoSizeSpace_Create(0, 0, 0, 0, 0, 0, 10, AUTOSIZE_HORIZ);
			horizsize2->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Box_AddChild(horizsize, horizsize2);
			{	
				pbox = StdButton_Create(0, 0, 90, _("Close"), 0);
				Button2_SetOnButtonHit(pbox, GameInviteError_OnDecline);
				Box_AddChild(horizsize2, pbox);
				data->cancelbutton = pbox;
			}
		}
	}

	{
		char txt[512];

		if (error)
		{
			if (stricmp(error, "offline") == 0)
			{
				if (actor && actor[0])
				{
					i18n_stringsub(txt, 512, _("Game invite cancelled.\n%1 is offline."), Model_GetFriendNick(actor));
				}
				else
				{
					i18n_stringsub(txt, 512, _("Game invite cancelled.\nOur data indicates that you are offline.  Please try logging out and in again.  If this problem persists, please submit a problem report or contact a Chesspark representative for assistance."));
				}
			}
			else if (stricmp(error, "reject") == 0)
			{
				if (sent)
				{
					if (actor && actor[0])
					{
						i18n_stringsub(txt, 512, _("Game request declined by %1."), Model_GetFriendNick(actor));
					}
					else
					{
						i18n_stringsub(txt, 512, _("Game request declined."));
					}
				}
				else
				{
					if (actor && actor[0])
					{
						i18n_stringsub(txt, 512, _("Game request cancelled by %1."), Model_GetFriendNick(actor));
					}
					else
					{
						i18n_stringsub(txt, 512, _("Game request cancelled."));
					}

				}
			}
			else if (stricmp(error, "playing") == 0)
			{
				if (actor && actor[0])
				{
					i18n_stringsub(txt, 512, _("Game invite cancelled.\n%1 is playing a game."), Model_GetFriendNick(actor));
				}
				else
				{
					i18n_stringsub(txt, 512, _("Game invite cancelled.\nOur data indicates that you are already playing a game.  If this is in error, please submit a problem report or contact a Chesspark representative for assistance."));
				}
			}
			else if (stricmp(error, "expired") == 0)
			{
				if (actor && actor[0])
				{
					i18n_stringsub(txt, 512, _("Game invite cancelled.\n%1's membership has expired"), Model_GetFriendNick(actor));
				}
				else
				{
					i18n_stringsub(txt, 512, _("Game invite cancelled.\nYour membership has expired."));
				}
			}
			else if (stricmp(error, "badrequest") == 0)
			{
				i18n_stringsub(txt, 512, _("Bad Request."));
			}
			else if (stricmp(error, "internalservice") == 0)
			{
				i18n_stringsub(txt, 512, _("Internal service error.\n\nPlease try your request again.  If this problem persists, please submit a problem report or contact a Chesspark representative for assistance."));
			}
			else
			{
				i18n_stringsub(txt, 512, _("Unknown error: %1"), error);
			}
		}
		else
		{
			i18n_stringsub(txt, 512, _("Unknown error."));
		}

		Text_SetText(data->messagebox, txt);
	}

	AutoSize_Fit(data->sizeablecontent);
	AutoSize_Fill(data->sizeablecontent);

	Box_OnSizeWidth_Stretch(dialog, data->sizeablecontent->w + 20 - dialog->w);
	Box_OnSizeHeight_Stretch(dialog, data->sizeablecontent->h + 50 - dialog->h);

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;
		int remainw, remainh;

		windowrect.left = roster->x;
		windowrect.right = windowrect.left + roster->w - 1;
		windowrect.top = roster->y;
		windowrect.bottom = windowrect.top + roster->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		remainw = mi.rcWork.right - mi.rcWork.left - dialog->w;
		remainh = mi.rcWork.bottom - mi.rcWork.top - dialog->h;

		dialog->x = remainw / 2;
		dialog->y = remainh / 2;

		/*
		dialog->x += 20 * cascade;
		dialog->x %= remainw;
		dialog->y += 20 * cascade;
		dialog->y %= remainh;
		*/

		dialog->x += mi.rcWork.left;
		dialog->y += mi.rcWork.top;
	}

	Box_CreateWndCustom(dialog, titlebartxt, roster->hwnd);

	/*
	MiniProfile_SetProfile(data->opponentbox, data->jid, NULL);

	Model_SubscribeProfile(data->jid, GameInviteError_SetProfile, dialog);
	*/

	dialog->OnDestroy = GameInviteError_OnDestroy;

	FlashWindow(dialog->hwnd, 1);

	return dialog;
}