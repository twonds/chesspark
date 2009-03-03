#include <stdio.h>
#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "titledrag.h"
#include "text.h"

#include "constants.h"

#include "autosize.h"
#include "button2.h"
#include "conn.h"
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


struct gamerespondaddata_s
{
	char *gameid;
	char *jid;
	char *timecontrol;
	struct namedlist_s *ratinglist;

	struct gamesearchinfo_s *info;

	struct Box_s *sizeablecontent;

	struct Box_s *infobox;
	struct Box_s *messagebox;
	struct Box_s *button1;
	struct Box_s *button2;
	struct Box_s *button3;
	struct Box_s *opponentbox;
};


void GameRespondAd_OnCancel(struct Box_s *pbox)
{
	View_CloseGameRespondAdDialog();
}

void GameRespondAd_OnResearch(struct Box_s *pbox)
{
	Ctrl_RequestGameSearch("quickplay", NULL);
}

void GameRespondAd_OnRequest(struct Box_s *pbox)
{
	struct gamerespondaddata_s *data = Box_GetRoot(pbox)->boxdata;
	char txt[256];
	char nametxt[256];

	i18n_stringsub(nametxt, 256, "Awaiting response from %1...", Model_GetFriendNick(data->jid));
	sprintf(txt, "%s\n%s", _("Your request has been sent."), nametxt);

	Text_SetText(data->messagebox, txt);

	data->messagebox->flags |= BOX_VISIBLE;
	data->infobox->flags &= ~BOX_VISIBLE;
	data->button1->flags &= ~BOX_VISIBLE;
	if (data->button3)
	{
		data->button1->flags &= ~BOX_VISIBLE;
	}
	StdButton_SetText(data->button2, _("Cancel Request"));
	Box_Repaint(pbox->parent);

	if (data->info->colorpreference == 1)
	{
		data->info->colorpreference = 2;
	}
	else if (data->info->colorpreference == 2)
	{
		data->info->colorpreference = 1;
	}

	free(data->info->gameid);
	data->info->gameid = NULL;

	Conn_RespondAd(data->jid, data->info);
}

void GameRespondAd_ShowProfileRating(struct Box_s *dialog, char *variant, char *timecontrol)
{
	struct gamerespondaddata_s *data = dialog->boxdata;
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

void GameRespondAd_SetProfile(char *jid, struct profile_s *profile, struct Box_s *dialog)
{
	struct gamerespondaddata_s *data = dialog->boxdata;
	char *jid1 = Jid_Strip(jid);
	char *jid2 = Jid_Strip(data->jid);
	struct namedlist_s **allratingslist = NULL;
	struct namedlist_s *allratings = NULL;
	struct namedlist_s **ratinglist = NULL;
	struct rating_s *rating = NULL;
	
	if (stricmp(jid1, jid2) != 0)
	{
		free(jid1);
		free(jid2);
		return;
	}

	MiniProfile_SetProfile(data->opponentbox, jid, profile);

	data->ratinglist = Info_DupeRatingList(profile->ratings);

	GameRespondAd_ShowProfileRating(dialog, data->info->variant, data->timecontrol);

	Box_Repaint(data->opponentbox);
}

void GameRespondAd_OnDestroy(struct Box_s *dialog)
{
	struct gamerespondaddata_s *data = dialog->boxdata;
	Model_UnsubscribeProfile(data->jid, GameRespondAd_SetProfile, dialog);
}

struct Box_s *GameRespondAd_Create(struct Box_s *roster, char *fromjid, struct gamesearchinfo_s *info)
{
	struct gamerespondaddata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *pbox, *vertsize;
	char txt[512];

	memset(data, 0, sizeof(*data));

	dialog = Box_Create(0, 0, 432, 295, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;

	dialog->boxdata = data;

	data->jid = strdup(fromjid);
	data->info = Info_DupeGameSearchInfo(info);
	
	if (info->node && strcmp(info->node, "quickplay") == 0)
	{
		i18n_stringsub(txt, 512, _("Play Now with %1"), Model_GetFriendNick(fromjid));
	}
	else
	{
		i18n_stringsub(txt, 512, _("Respond To Game Ad by %1"), Model_GetFriendNick(fromjid));
	}
	dialog->titlebar = TitleBarOnly_Add(dialog, txt);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	data->timecontrol = Info_TimeControlToCategory(info->timecontrol);

	vertsize = AutoSize_Create(10, 43, 0, 0, 0, 0, AUTOSIZE_VERT);
	Box_AddChild(dialog, vertsize);
	data->sizeablecontent = vertsize;
	{
		struct Box_s *horizsize;
		struct Box_s *vertsize2;

		pbox = Text_Create(10, 2, 280, 60, BOX_TRANSPARENT, TX_WRAP);
		/*pbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
		pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
		*/
		pbox->fgcol = RGB(222, 222, 222);
		Box_AddChild(vertsize, pbox);
		data->messagebox = pbox;

		vertsize2 = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		data->infobox = vertsize2;
		Box_AddChild(vertsize, vertsize2);
		{
			struct Box_s *vertsize = AutoSize_Create(10, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
			Box_AddChild(vertsize2, vertsize);
			{
				struct Box_s *horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
				Box_AddChild(vertsize, horizsize);
				{
					pbox = Text_Create(0, 0, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
					pbox->fgcol = UserInfoFG2;
					Text_SetText(pbox, _("Side"));
					Box_AddChild(horizsize, pbox);

					AutoSize_AddSpacer(horizsize, 10);

					pbox = Text_Create(10 /*115*/, 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
					pbox->fgcol =  RGB(222, 222, 222);
					if (info->white && !info->black)
					{
						Text_SetText(pbox, _("Black"));
					}
					else if (!info->white && info->black)
					{
						Text_SetText(pbox, _("White"));
					}
					else if (!info->white && !info->black)
					{
						Text_SetText(pbox, _("White or Black"));
					}
					Box_AddChild(horizsize, pbox);

					pbox = Text_Create(10 /*215*/, 0, 60, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
					pbox->fgcol = UserInfoFG2;
					Text_SetText(pbox, _("Rated Game"));
					Box_AddChild(horizsize, pbox);

					AutoSize_AddSpacer(horizsize, 10);

					pbox = Text_Create(10, /*280*/ 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
					pbox->fgcol =  RGB(222, 222, 222);
					Text_SetText(pbox, info->rated ? _("Yes") : _("No"));
					Box_AddChild(horizsize, pbox);
				}

				AutoSize_AddSpacer(vertsize, 2);

				horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
				Box_AddChild(vertsize, horizsize);
				{
					pbox = Text_Create(0, 0, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
					pbox->fgcol = UserInfoFG2;
					Text_SetText(pbox, _("Game Variant"));
					Box_AddChild(horizsize, pbox);

					AutoSize_AddSpacer(horizsize, 10);

					pbox = Text_Create(10, 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
					pbox->fgcol =  RGB(222, 222, 222);
					Text_SetText(pbox, Util_Capitalize(info->variant));
					Box_AddChild(horizsize, pbox);
				}

				AutoSize_AddSpacer(vertsize, 2);

				horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
				Box_AddChild(vertsize, horizsize);
				{
					pbox = Text_Create(10, 0, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
					pbox->fgcol = UserInfoFG2;
					Text_SetText(pbox, _("Time Settings"));
					Box_AddChild(horizsize, pbox);

					AutoSize_AddSpacer(horizsize, 10);

					pbox = Text_Create(10, 0, 265, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
					pbox->fgcol =  RGB(222, 222, 222);
					Text_SetText(pbox, Info_TimeControlToLongText(info->timecontrol));
					Box_AddChild(horizsize, pbox);
				}

				AutoSize_AddSpacer(vertsize, 2);

				horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
				Box_AddChild(vertsize, horizsize);
				{
					pbox = Text_Create(10, 0, 95, 40, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
					pbox->fgcol = UserInfoFG2;
					Text_SetText(pbox, _("Desired Opponent"));
					Box_AddChild(horizsize, pbox);

					AutoSize_AddSpacer(horizsize, 10);

					pbox = Text_Create(10, 0, 265, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
					pbox->fgcol =  RGB(222, 222, 222);
					Text_SetText(pbox, Info_SearchLimitToText(info->limit));
					Box_AddChild(horizsize, pbox);
				}
				
				AutoSize_AddSpacer(vertsize, 2);

				horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
				Box_AddChild(vertsize, horizsize);
				{
					pbox = Text_Create(10, 0, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
					pbox->fgcol = UserInfoFG2;
					Text_SetText(pbox, _("Additional Info"));
					Box_AddChild(horizsize, pbox);

					AutoSize_AddSpacer(horizsize, 10);

					pbox = Text_Create(10, 0, 265, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
					pbox->fgcol =  RGB(222, 222, 222);
					Text_SetText(pbox, info->comment);
					Box_AddChild(horizsize, pbox);
				}
			}

			AutoSize_AddSpacer(vertsize2, 2);

			pbox = Box_Create(0, 0, vertsize2->w, 1, BOX_VISIBLE);
			pbox->bgcol = RGB(77, 77, 77);
			pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
			Box_AddChild(vertsize2, pbox);

			pbox = Box_Create(0, 0, vertsize2->w, 1, BOX_VISIBLE);
			pbox->bgcol = RGB(102, 102, 102);
			pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
			Box_AddChild(vertsize2, pbox);

			AutoSize_AddSpacer(vertsize2, 13);

			horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			Box_AddChild(vertsize2, horizsize);
			{
				AutoSize_AddSpacer(horizsize, 40);

				pbox = Text_Create(0, 5, 50, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
				pbox->fgcol = UserInfoFG2;
				Text_SetText(pbox, _("Opponent"));
				Box_AddChild(horizsize, pbox);

				AutoSize_AddSpacer(horizsize, 20);

				pbox = Box_Create(0, 0, 225, 50, BOX_VISIBLE | BOX_BORDER);
				pbox->brcol = RGB(90, 97, 108);
				pbox->bgcol = DrawerBG;
				data->opponentbox = pbox;
				Box_AddChild(horizsize, pbox);
			}
			
		}

		AutoSize_AddSpacer(vertsize, 37);

		horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
		horizsize->OnSizeWidth = Box_OnSizeWidth_Stretch;
		Box_AddChild(vertsize, horizsize);
		{
			struct Box_s *horizsize2 = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			horizsize2->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Box_AddChild(horizsize, horizsize2);
			{
			
				if (info->node && strcmp(info->node, "quickplay") == 0)
				{
					pbox = StdButton_Create(0, 0, 130, _("Request This Game"), 0);
					Button2_SetOnButtonHit(pbox, GameRespondAd_OnRequest);
					data->button1 = pbox;
					Box_AddChild(horizsize2, pbox);

					pbox = StdButton_Create(0, 0, 130, _("Find Another Game"), 0);
					Button2_SetOnButtonHit(pbox, GameRespondAd_OnResearch);
					data->button3 = pbox;
					Box_AddChild(horizsize2, pbox);
				
					pbox = StdButton_Create(0, 0, 80, _("Cancel"), 0);
					Button2_SetOnButtonHit(pbox, GameRespondAd_OnCancel);
					data->button2 = pbox;
					Box_AddChild(horizsize2, pbox);
				}
				else
				{
					pbox = StdButton_Create(0, 0, 90, _("Request Game"), 0);
					Button2_SetOnButtonHit(pbox, GameRespondAd_OnRequest);
					data->button1 = pbox;
					Box_AddChild(horizsize2, pbox);
				
					pbox = StdButton_Create(0, 0, 80, _("Cancel"), 0);
					Button2_SetOnButtonHit(pbox, GameRespondAd_OnCancel);
					data->button2 = pbox;
					Box_AddChild(horizsize2, pbox);
				}
			}
		}
	}

	AutoSize_Fit(data->sizeablecontent);
	AutoSize_Fill(data->sizeablecontent);

	Box_OnSizeWidth_Stretch(dialog, data->sizeablecontent->w + 20 - dialog->w);
	Box_OnSizeHeight_Stretch(dialog, data->sizeablecontent->h + 50 - dialog->h);

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

		dialog->x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - dialog->w) / 2;
		dialog->y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - dialog->h) / 2;
	}

#if 0
	pbox = Box_Create(20, 40, dialog->w - 40, dialog->h - 80, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_AddChild(dialog, pbox);
	data->infobox = pbox;

	pbox = Text_Create(20, 40, dialog->w - 40, dialog->h - 80, BOX_TRANSPARENT, TX_WRAP);
	pbox->fgcol = RGB(255, 255, 255);
	Box_AddChild(dialog, pbox);
	data->messagebox = pbox;

	pbox = Box_Create(0, 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Side"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(100, 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = RGB(255, 255, 255);
	if (info->white && !info->black)
	{
		Box_SetText(pbox, _("Black"));
	}
	else if (!info->white && info->black)
	{
		Box_SetText(pbox, _("White"));
	}
	else if (!info->white && !info->black)
	{
		Box_SetText(pbox, _("White or Black"));
	}
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(280, 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Rated Game"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(370, 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = RGB(255, 255, 255);
	if (info->rated)
	{
		Box_SetText(pbox, _("Yes"));
	}
	else
	{
		Box_SetText(pbox, _("No"));
	}
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(0, 25, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Game Variant"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(100, 25, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = RGB(255, 255, 255);
	Box_SetText(pbox, info->variant);
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(0, 50, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Time Settings"));
	Box_AddChild(data->infobox, pbox);

	pbox = Text_Create(100, 50, 292, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->fgcol = RGB(255, 255, 255);
	Text_SetText(pbox, Info_TimeControlToLongText(info->timecontrol));
	data->timecontrol = Info_TimeControlToCategory(info->timecontrol);
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(0, 75, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Desired Opponent"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(100, 75, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = RGB(255, 255, 255);
	Box_SetText(pbox, Info_SearchLimitToText(info->limit));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(0, 100, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Additional Info"));
	Box_AddChild(data->infobox, pbox);

	/*
	pbox = Box_Create(100, 100, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = RGB(255, 255, 255);
	Box_SetText(pbox, comment);
	Box_AddChild(data->infobox, pbox);
	*/

	pbox = Box_Create(0, 140, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Opponent"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(0, 165, 300, 55, BOX_VISIBLE | BOX_BORDER);
	pbox->brcol = RGB(90, 97, 108);
	pbox->bgcol = DrawerBG;
	data->opponentbox = pbox;
	Box_AddChild(data->infobox, pbox);

	if (info->node && strcmp(info->node, "quickplay") == 0)
	{
		pbox = StdButton_Create(432 - 240, 295 - 32, 130, _("Request This Game"), 0);
		Button2_SetOnButtonHit(pbox, GameRespondAd_OnRequest);
		data->button1 = pbox;
		Box_AddChild(dialog, pbox);

		pbox = StdButton_Create(20, 295 - 32, 130, _("Find Another Game"), 0);
		Button2_SetOnButtonHit(pbox, GameRespondAd_OnResearch);
		data->button3 = pbox;
		Box_AddChild(dialog, pbox);
	
		pbox = StdButton_Create(432 - 100, 295 - 32, 80, _("Cancel"), 0);
		Button2_SetOnButtonHit(pbox, GameRespondAd_OnCancel);
		data->button2 = pbox;
		Box_AddChild(dialog, pbox);
	}
	else
	{
		pbox = StdButton_Create(432 - 200, 295 - 32, 90, _("Request Game"), 0);
		Button2_SetOnButtonHit(pbox, GameRespondAd_OnRequest);
		data->button1 = pbox;
		Box_AddChild(dialog, pbox);
	
		pbox = StdButton_Create(432 - 100, 295 - 32, 80, _("Cancel"), 0);
		Button2_SetOnButtonHit(pbox, GameRespondAd_OnCancel);
		data->button2 = pbox;
		Box_AddChild(dialog, pbox);
	}

#endif
	Box_CreateWndCustom(dialog, _("Game Info"), roster->hwnd);

	MiniProfile_SetProfile(data->opponentbox, data->jid, NULL);
	Model_SubscribeProfile(data->jid, GameRespondAd_SetProfile, dialog);
	dialog->OnDestroy = GameRespondAd_OnDestroy;

	BringWindowToTop(dialog->hwnd);

	return dialog;
}

void GameRespondAd_SetError(struct Box_s *dialog, char *opponentjid, char *error)
{
	struct gamerespondaddata_s *data = dialog->boxdata;
	char *barejid1 = Jid_Strip(opponentjid);
	char *barejid2 = Jid_Strip(data->jid);

	if (stricmp(barejid1, barejid2) == 0)
	{
		/*sprintf(txt, _("Game invite error!\n%s"), error);*/

		Text_SetText(data->messagebox, error);

		data->infobox->flags &= ~BOX_VISIBLE;
		data->messagebox->flags |= BOX_VISIBLE;
		StdButton_SetText(data->button2, _("Close"));

		Box_Repaint(dialog);
	}

	free(barejid1);
	free(barejid2);
}
