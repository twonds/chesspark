#include <stdio.h>
#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "button2.h"
#include "titledrag.h"
#include "text.h"

#include "constants.h"

#include "autosize.h"
#include "ctrl.h"
#include "i18n.h"
#include "info.h"
#include "log.h"
#include "model.h"
#include "profile.h"
#include "namedlist.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

#include "add.h"


struct gamereconvenedata_s
{
	char *gameid;
	char *jid;
	int inviting;
	int sent;

	struct gamesearchinfo_s *info;

	struct Box_s *sizeablecontent;

	struct Box_s *infobox;
	struct Box_s *messagebox;
	struct Box_s *forfeitbutton;
	struct Box_s *openchatbutton;
	struct Box_s *acceptbutton;
	struct Box_s *cancelbutton;
	struct Box_s *opponentbox;
};


void GameReconvene_OnForfeit(struct Box_s *pbox)
{
	struct gamereconvenedata_s *data = Box_GetRoot(pbox)->boxdata;

	Ctrl_SendGameResign(data->gameid, 0);
	View_CloseReconveneDialog();
}


void GameReconvene_OnChat(struct Box_s *pbox)
{
	struct gamereconvenedata_s *data = Box_GetRoot(pbox)->boxdata;

	Model_PopupChatDialog(data->jid, 0);
}


void GameReconvene_OnCancel(struct Box_s *pbox)
{
	struct gamereconvenedata_s *data = Box_GetRoot(pbox)->boxdata;

	if (data->inviting)
	{
	}
	else
	{
		Ctrl_DeclineMatch(data->gameid);
	}

	View_CloseReconveneDialog();
}


void GameReconvene_OnRequest(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct gamereconvenedata_s *data = Box_GetRoot(pbox)->boxdata;
	char txt[256];
	int w, h;

	i18n_stringsub(txt, 256, _("Your request has been sent to %1. Awaiting response..."), Model_GetFriendNick(data->jid));

	View_CloseChessGame(data->gameid);

	Text_SetText(data->messagebox, txt);

	data->messagebox->flags |= BOX_VISIBLE;
	data->infobox->flags &= ~BOX_VISIBLE;
	data->forfeitbutton->flags &= ~BOX_VISIBLE;
	data->acceptbutton->flags &= ~BOX_VISIBLE;
	StdButton_SetTextAndResize(data->cancelbutton, _("Cancel Request"), 90);
	Box_Repaint(pbox->parent);

	/*Conn_RequestMatch(data->jid, white, data->variant, data->ratedgame, data->delayinc, time, comment);*/

	if (data->inviting)
	{
		Ctrl_GameReconvene(data->gameid);
		data->inviting = 0;
	}
	else
	{
		Ctrl_AcceptMatch(data->gameid);
	}

	data->sent = 1;

	AutoSize_Fit(data->sizeablecontent);
	AutoSize_Fill(data->sizeablecontent);

	w = data->sizeablecontent->w + 20;
	h = data->sizeablecontent->h + 50;

	Box_MoveWndCustom(dialog, dialog->x + (dialog->w - w) / 2, dialog->y + (dialog->h - h) / 2, w, h);

/*	View_CloseGameReconveneDialog();*/
}

void GameReconvene_SetProfile(char *jid, struct profile_s *profile, struct Box_s *dialog)
{
	struct gamereconvenedata_s *data = dialog->boxdata;
	char *jid1 = Jid_Strip(jid);
	char *jid2 = Jid_Strip(data->jid);
	struct namedlist_s **ratinglist = NULL;
	struct rating_s *rating = NULL;

	char txt[120];
	char ratingtext[80];
	char *category = NULL;

	if (stricmp(jid1, jid2) != 0)
	{
		free(jid1);
		free(jid2);
		return;
	}

	MiniProfile_SetProfile(data->opponentbox, jid, profile);

	/*
	if (data->info->timecontrolrange)
	{
		category = data->info->timecontrolrange;
	}
	else
	{
		category = Info_TimeControlToCategory(data->info->timecontrol);
	}
	*/

	if (data->info->variant)
	{
		category = data->info->variant;
	}
	else
	{
		category = "Standard";
	}

	{
		ratinglist = NamedList_GetByName(&(profile->ratings), category);
		if (ratinglist)
		{
			rating = (*ratinglist)->data;
		}
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
/*
	if (rating)
	{
		subbox = Box_Create(5, 25, data->opponentbox->w - 10, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->fgcol =   RGB(222, 222, 222);
		
		i18n_stringsub(txt, 512, _("Rating for %1 games: %2"), data->info->timecontrolrange, ratingtxt);
		Box_SetText(data->opponentbox->child->sibling, txt);
		Box_AddChild(data->opponentbox, subbox);
	}
*/

	Box_Repaint(data->opponentbox);
}

void GameReconvene_OnDestroy(struct Box_s *dialog)
{
	struct gamereconvenedata_s *data = dialog->boxdata;
	Model_UnsubscribeProfile(data->jid, GameReconvene_SetProfile, dialog);
}

struct Box_s *GameReconvene_Create(struct Box_s *roster, char *fromjid,
	char *gameid, struct gamesearchinfo_s *info, int inviting)
{
	struct gamereconvenedata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *pbox, *vertsize;
	char txt[512];

	memset(data, 0, sizeof(*data));

	dialog = Box_Create(0, 0, 400, 285, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;

	dialog->boxdata = data;

	data->jid = strdup(fromjid);
	data->info = Info_DupeGameSearchInfo(info);
	data->gameid = strdup(gameid);
	data->inviting = inviting;
	
	if (inviting)
	{
		i18n_stringsub(txt, 512, _("Resume Adjourned Game with %1"), Model_GetFriendNick(fromjid));
	}
	else
	{
		i18n_stringsub(txt, 512, _("Game Reconvene From %1"), Model_GetFriendNick(fromjid));
	}
	dialog->titlebar = TitleBarOnly_Add(dialog, txt);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;
/*
	pbox = Box_Create(10, 43, dialog->w - 20, dialog->h - 80, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_AddChild(dialog, pbox);
	data->infobox = pbox;

	pbox = Text_Create(20, 40, dialog->w - 40, dialog->h - 80, BOX_TRANSPARENT, TX_WRAP);
	pbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	pbox->fgcol =  RGB(222, 222, 222);
	Box_AddChild(dialog, pbox);
	data->messagebox = pbox;
*/
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
					{
						char *bareloginjid = Jid_Strip(Model_GetLoginJid());
						char *barewhitejid = Jid_Strip(info->white->jid);
						char *bareblackjid = Jid_Strip(info->black->jid);

						if (stricmp(bareloginjid, barewhitejid) == 0)
						{
							Text_SetText(pbox, _("White"));
						}
						else if (stricmp(bareloginjid, bareblackjid) == 0)
						{
							Text_SetText(pbox, _("Black"));
						}
						else
						{
							Text_SetText(pbox, _("Unknown"));
						}
						free(bareloginjid);
						free(barewhitejid);
						free(bareblackjid);
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
					pbox = Text_Create(10, 0, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
					pbox->fgcol = UserInfoFG2;
					Text_SetText(pbox, _("Game Status"));
					Box_AddChild(horizsize, pbox);

					AutoSize_AddSpacer(horizsize, 10);

					pbox = Text_Create(5, 0, 255, 40, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
					pbox->fgcol =  RGB(222, 222, 222);
					{
						char txt[512];

						txt[0] = '\0';
			
						if (info->movenum)
						{
							char movetxt[256];

							strcat(txt, _("Move "));

							sprintf(movetxt, "%d", info->movenum);
							strcat(txt, movetxt);

							if (info->whitetomove)
							{
								strcat(txt, _(", White"));
							}
							else if (info->blacktomove)
							{
								strcat(txt, _(", Black"));
							}

							if (info->lastmove && strlen(info->lastmove) > 0)
							{
								strcat(txt, _(", "));
							}
						}

						if (info->lastmove && strlen(info->lastmove) > 0)
						{
							char shortmove[512];
							char *space;

							strcpy(shortmove, info->lastmove);
							space = strchr(shortmove, ' ');
							if (space)
							{
								*space = '\0';
							}

							strcat(txt, _("Last move "));
							strcat(txt, shortmove);
						}

						if (info->movenum || (info->lastmove && strlen(info->lastmove) > 0))
						{
							strcat(txt, "\n");
						}

						strcat(txt, _("White time remaining: "));
						strcat(txt, Info_SecsToText1(info->whiteclock));
						strcat(txt, _("\nBlack time remaining: "));
						strcat(txt, Info_SecsToText1(info->blackclock));

						Text_SetText(pbox, txt);
					}
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

				pbox = MiniProfile_Create(0, 0, 225, 50, fromjid);
				pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
				data->opponentbox = pbox;
				Box_AddChild(horizsize, pbox);
			}
			
		}

		AutoSize_AddSpacer(vertsize, 37);

		horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
		horizsize->OnSizeWidth = Box_OnSizeWidth_Stretch;
		Box_AddChild(vertsize, horizsize);
		{
			struct Box_s *horizsize2 = AutoSizeSpace_Create(0, 0, 0, 0, 0, 0, 10, AUTOSIZE_HORIZ);
			Box_AddChild(horizsize, horizsize2);
			{
				pbox = StdButton_Create(0, 0, 90, _("Forfeit Game"), 0);
				Button2_SetOnButtonHit(pbox, GameReconvene_OnForfeit);
				Box_AddChild(horizsize2, pbox);
				data->forfeitbutton = pbox;

				pbox = StdButton_Create(0, 0, 90, _("Open Chat"), 0);
				Button2_SetOnButtonHit(pbox, GameReconvene_OnChat);
				Box_AddChild(horizsize2, pbox);
				data->openchatbutton = pbox;

				AutoSize_AddSpacer(horizsize2, 0);
			}

			horizsize2 = AutoSizeSpace_Create(0, 0, 0, 0, 0, 0, 10, AUTOSIZE_HORIZ);
			horizsize2->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Box_AddChild(horizsize, horizsize2);
			{
			
				pbox = StdButton_Create(0, 0, 100, inviting ? _("Request Reconvene") : _("Accept"), 0);
				Button2_SetOnButtonHit(pbox, GameReconvene_OnRequest);
				data->acceptbutton = pbox;
				Box_AddChild(horizsize2, pbox);
	
				pbox = StdButton_Create(0, 0, 100, inviting ? _("Close") : _("Decline"), 0);
				Button2_SetOnButtonHit(pbox, GameReconvene_OnCancel);
				data->cancelbutton = pbox;
				Box_AddChild(horizsize2, pbox);
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
	pbox = Box_Create(10, 0, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_RIGHTTEXT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Side"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(115, 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol =  RGB(222, 222, 222);
	{
		char *bareloginjid = Jid_Strip(Model_GetLoginJid());
		char *barewhitejid = Jid_Strip(info->white->jid);
		char *bareblackjid = Jid_Strip(info->black->jid);

		if (stricmp(bareloginjid, barewhitejid) == 0)
		{
			Box_SetText(pbox, _("White"));
		}
		else if (stricmp(bareloginjid, bareblackjid) == 0)
		{
			Box_SetText(pbox, _("Black"));
		}
		else
		{
			Box_SetText(pbox, _("Unknown"));
		}
		free(bareloginjid);
		free(barewhitejid);
		free(bareblackjid);
	}
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(215, 0, 60, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_RIGHTTEXT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Rated Game"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(280, 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol =  RGB(222, 222, 222);
	if (info->rated)
	{
		Box_SetText(pbox, _("Yes"));
	}
	else
	{
		Box_SetText(pbox, _("No"));
	}
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(10, 22, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_RIGHTTEXT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Game Variant"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(115, 22, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol =  RGB(222, 222, 222);
	Box_SetText(pbox, Util_Capitalize(info->variant));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(10, 44, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_RIGHTTEXT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Time Settings"));
	Box_AddChild(data->infobox, pbox);

	pbox = Text_Create(115, 44, 265, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->fgcol =  RGB(222, 222, 222);
	Text_SetText(pbox, Info_TimeControlToLongText(info->timecontrol));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(10, 66, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_RIGHTTEXT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Game Status"));
	Box_AddChild(data->infobox, pbox);

	pbox = Text_Create(115, 66, 255, 40, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
	pbox->fgcol =  RGB(222, 222, 222);
	{
		char txt[512];

		txt[0] = '\0';
		
		if (info->movenum)
		{
			char movetxt[256];

			strcat(txt, _("Move "));

			sprintf(movetxt, "%d", info->movenum);
			strcat(txt, movetxt);

			if (info->whitetomove)
			{
				strcat(txt, _(", White"));
			}
			else if (info->blacktomove)
			{
				strcat(txt, _(", Black"));
			}

			if (info->lastmove && strlen(info->lastmove) > 0)
			{
				strcat(txt, _(", "));
			}
		}

		if (info->lastmove && strlen(info->lastmove) > 0)
		{
			char shortmove[512];
			char *space;

			strcpy(shortmove, info->lastmove);
			space = strchr(shortmove, ' ');
			if (space)
			{
				*space = '\0';
			}

			strcat(txt, _("Last move "));
			strcat(txt, shortmove);
		}

		if (info->movenum || (info->lastmove && strlen(info->lastmove) > 0))
		{
			strcat(txt, "\n");
		}

		strcat(txt, _("White time remaining: "));
		strcat(txt, Info_SecsToText1(info->whiteclock));
		strcat(txt, _("\nBlack time remaining: "));
		strcat(txt, Info_SecsToText1(info->blackclock));

		Text_SetText(pbox, txt);
	}
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(0, 110, 380, 1, BOX_VISIBLE);
	pbox->bgcol = RGB(77, 77, 77);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(0, 111, 380, 1, BOX_VISIBLE);
	pbox->bgcol = RGB(102, 102, 102);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(50, 130, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Opponent"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(110, 125, 225, 50, BOX_VISIBLE | BOX_BORDER);
	pbox->brcol = RGB(90, 97, 108);
	pbox->bgcol = DrawerBG;
	data->opponentbox = pbox;
	Box_AddChild(data->infobox, pbox);

	pbox = StdButton_Create(10, 285 - 30, 110, _("Forfeit Game"), 0);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, GameReconvene_OnForfeit);
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(400 - 215, 285 - 30, 100, _("Resume Game"), 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, GameReconvene_OnRequest);
	data->acceptbutton = pbox;
	Box_AddChild(dialog, pbox);
	
	if (!inviting)
	{
		pbox = StdButton_Create(400 - 110, 285 - 30, 100, _("Decline"), 0);
	}
	else
	{
		pbox = StdButton_Create(400 - 110, 285 - 30, 100, _("Close"), 0);
	}
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, GameReconvene_OnCancel);
	data->cancelbutton = pbox;
	Box_AddChild(dialog, pbox);
#endif

	Box_CreateWndCustom(dialog, txt, roster->hwnd);

	MiniProfile_SetProfile(data->opponentbox, data->jid, NULL);
	Model_SubscribeProfile(data->jid, GameReconvene_SetProfile, dialog);

	dialog->OnDestroy = GameReconvene_OnDestroy;

	if (!inviting)
	{
		FlashWindow(dialog->hwnd, 1);
	}
	else
	{
		BringWindowToTop(dialog->hwnd);
	}

	return dialog;
}


void GameReconvene_SetError(struct Box_s *dialog, char *gameid, char *error, char *actor)
{
	struct gamereconvenedata_s *data = dialog->boxdata;

	Log_Write(0, "GameReconvene_SetError %s == %s?\n", gameid, data->gameid);

	if (strcmp(gameid, data->gameid) == 0)
	{
		char txt[512];
		int w, h;

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
					i18n_stringsub(txt, 512, _("Game invite cancelled.\nMember is offline."));
				}
			}
			else if (stricmp(error, "reject") == 0)
			{
				if (data->sent)
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
					i18n_stringsub(txt, 512, _("Game invite cancelled.\nMember is playing a game."));
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
					i18n_stringsub(txt, 512, _("Game invite cancelled.\nMembership has expired."));
				}
			}
			else if (stricmp(error, "badrequest") == 0)
			{
				i18n_stringsub(txt, 512, _("Bad Request."));
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
			i18n_stringsub(txt, 512, _("Unknown error."));
		}

/*
		if (data->sent && stricmp(error, _("Game request cancelled.")) == 0)
		{
			sprintf(txt, _("Game request declined."));
		}
		else
		{
			sprintf(txt, _("Game reconvene error!\n%s"), error);
		}
*/
		Text_SetText(data->messagebox, txt);

		data->infobox->flags &= ~BOX_VISIBLE;
		data->messagebox->flags |= BOX_VISIBLE;

		AutoSize_Fit(data->sizeablecontent);
		AutoSize_Fill(data->sizeablecontent);

		w = data->sizeablecontent->w + 20;
		h = data->sizeablecontent->h + 50;

		Box_MoveWndCustom(dialog, dialog->x + (dialog->w - w) / 2, dialog->y + (dialog->h - h) / 2, w, h);

		Box_Repaint(dialog);
	}
}
