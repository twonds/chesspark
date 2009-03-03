#include <stdio.h>
#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "titledrag.h"
#include "text.h"

#include "constants.h"

#include "autodialog.h"
#include "autosize.h"
#include "boxtypes.h"
#include "button2.h"
#include "ctrl.h"
#include "i18n.h"
#include "info.h"
#include "log.h"
#include "model.h"
#include "namedlist.h"
#include "options.h"
#include "profile.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

#include "add.h"


struct gameinvitationdata_s
{
	char *jid;

	struct gamesearchinfo_s *info;

	struct namedlist_s *ratinglist;

	struct Box_s *sizeablecontent;

	struct Box_s *infobox;
	struct Box_s *messagebox;

	struct Box_s *opponentbox;
	struct Box_s *cancelbutton;
	struct Box_s *changebutton;
	struct Box_s *acceptbutton;
	struct Box_s *openchatbutton;

	struct Box_s *warningbox;
};

void GameInvitation_OnChange(struct Box_s *pbox)
{
	struct gameinvitationdata_s *data = Box_GetRoot(pbox)->boxdata;

	/*View_CloseGameRequestDialog(data->jid, data->info->gameid);*/
	View_PopupGameInviteFriendDialog(data->jid, data->info, 0);
}


void GameInvitation_OnChat(struct Box_s *pbox)
{
	struct gameinvitationdata_s *data = Box_GetRoot(pbox)->boxdata;

	Model_PopupChatDialog(data->jid, 0);
}

void GameInvitation_OnDecline(struct Box_s *pbox)
{
	struct gameinvitationdata_s *data = Box_GetRoot(pbox)->boxdata;

	Ctrl_DeclineMatch(data->info->gameid);

	if (data->warningbox)
	{
		Box_Destroy(data->warningbox);
		data->warningbox = NULL;
	}

	View_CloseGameRequestDialog(data->jid, data->info->gameid);
}


void GameInvitation_OnClose(struct Box_s *pbox)
{
	struct gameinvitationdata_s *data = Box_GetRoot(pbox)->boxdata;

	if (data->warningbox)
	{
		Box_Destroy(data->warningbox);
		data->warningbox = NULL;
	}

	View_CloseGameRequestDialog(data->jid, data->info->gameid);
}

void GameInvitation_RealAccept(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct gameinvitationdata_s *data = Box_GetRoot(pbox)->boxdata;
	int w, h;

/*	Conn_RequestMatch(data->jid, white, variant, data->ratedgame, delayinc, time, comment);*/

	if (data->info->correspondence)
	{
		Ctrl_AcceptCorMatch(data->info->gameid);
	}
	else
	{
		Ctrl_AcceptMatch(data->info->gameid);
		Text_SetText(data->messagebox, _("Game accepted. Joining..."));
	}

	data->infobox->flags &= ~BOX_VISIBLE;
	data->messagebox->flags |= BOX_VISIBLE;
	data->changebutton->flags &= ~BOX_VISIBLE;
	data->acceptbutton->flags &= ~BOX_VISIBLE;
	data->openchatbutton->flags &= ~BOX_VISIBLE;
	StdButton_SetTextAndResize(data->cancelbutton, _("Cancel"), 90);

	AutoSize_Fit(data->sizeablecontent);
	AutoSize_Fill(data->sizeablecontent);

	w = data->sizeablecontent->w + 20;
	h = data->sizeablecontent->h + 50;

	Box_MoveWndCustom(dialog, dialog->x + (dialog->w - w) / 2, dialog->y + (dialog->h - h) / 2, w, h);

	Box_Repaint(dialog);

	if (data->warningbox)
	{
		Box_Destroy(data->warningbox);
		data->warningbox = NULL;
	}
	/*View_CloseGameRequestDialog(data->jid, data->info->gameid);*/
}

void GameInvitationWarning_OnOK(struct Box_s *warningbox, struct Box_s *pbox)
{
	GameInvitation_RealAccept(pbox);
}

void GameInvitationWarning_OnCancel(struct Box_s *warningbox, struct Box_s *pbox)
{
	struct gameinvitationdata_s *data = Box_GetRoot(pbox)->boxdata;

	Box_Destroy(data->warningbox);
	data->warningbox = NULL;
}

void GameInvitationWarning_OnDontShow(struct Box_s *warningbox, char *name)
{
	Model_SetOption(OPTION_HIDEVARIANTWARNING, 1, NULL);
}

void GameInvitation_OnAccept(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct gameinvitationdata_s *data = Box_GetRoot(pbox)->boxdata;

	if (data->info->variant && stricmp(data->info->variant, "Standard") != 0 && !Model_GetOption(OPTION_HIDEVARIANTWARNING))
	{
		char txt[512];

		if (data->warningbox)
		{
			BringWindowToTop(data->warningbox->hwnd);
		}
		else
		{
                        i18n_stringsub(txt, 512, _("This game will be played with %1, a non-standard game variant.\n\nIf you wish to play this game, press OK.  If you wish to cancel, press Cancel."), data->info->variant);
			data->warningbox = AutoDialog_Create2(dialog, 500, _("Variant Warning!"), txt , _("Cancel"), _("OK"), GameInvitationWarning_OnCancel, GameInvitationWarning_OnOK, pbox, GameInvitationWarning_OnDontShow, "VariantWarning");
		}
		
	}
	else
	{
		GameInvitation_RealAccept(pbox);
	}
}

void GameInvitation_ShowProfileRating(struct Box_s *dialog, char *variant, char *timecontrol)
{
	struct gameinvitationdata_s *data = dialog->boxdata;
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

void GameInvitation_SetProfile(char *jid, struct profile_s *profile, struct Box_s *dialog)
{
	struct gameinvitationdata_s *data = dialog->boxdata;
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

	GameInvitation_ShowProfileRating(dialog, data->info->variant, Info_TimeControlToCategory(data->info->timecontrol));

	Box_Repaint(data->opponentbox);

}

void GameInvitation_OnDestroy(struct Box_s *dialog)
{
	struct gameinvitationdata_s *data = dialog->boxdata;
	Model_UnsubscribeProfile(data->jid, GameInvitation_SetProfile, dialog);
}

struct Box_s *GameInvitation_Create(struct Box_s *roster, char *fromjid, struct gamesearchinfo_s *info, int replace, int cascade)
{
	struct gameinvitationdata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *pbox, *vertsize;
	char titlebartxt[512];

	memset(data, 0, sizeof(*data));

	dialog = Box_Create(0, 0, 440, 270, BOX_VISIBLE);
	dialog->boxtypeid = BOXTYPE_GAMEINVITATION;
	dialog->bgcol = DefaultBG;

	dialog->boxdata = data;

	data->jid = strdup(fromjid);
	data->info = Info_DupeGameSearchInfo(info);

	if (replace)
	{
		i18n_stringsub(titlebartxt, 512, _("Game Renegotiation From %1"), Model_GetFriendNick(fromjid));
	}
	else
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
		struct Box_s *vertsize2;

		pbox = Text_Create(10, 2, 280, 60, BOX_TRANSPARENT, TX_WRAP);
		/*pbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
		pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
		*/
		pbox->fgcol = RGB(222, 222, 222);
		Box_AddChild(vertsize, pbox);
		data->messagebox = pbox;

		vertsize2 = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		vertsize2->OnSizeWidth = Box_OnSizeWidth_Stretch;
		data->infobox = vertsize2;
		Box_AddChild(vertsize, vertsize2);
		{
			struct Box_s *vertsize = AutoSize_Create(10, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
			Box_AddChild(vertsize2, vertsize);
			{
				struct Box_s *horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ | AUTOSIZE_EVENSPACING);
				Box_AddChild(vertsize, horizsize);
				{
					struct Box_s *horizsize2 = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					Box_AddChild(horizsize, horizsize2);
					{
						pbox = Text_Create(0, 0, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
						pbox->fgcol = UserInfoFG2;
						Text_SetText(pbox, _("Side"));
						Box_AddChild(horizsize2, pbox);

						AutoSize_AddSpacer(horizsize2, 10);

						pbox = Text_Create(10 /*115*/, 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
						pbox->fgcol = RGB(222, 222, 222);

						if (info->colorpreference == 1)
						{
							Text_SetText(pbox, _("Black"));
						}
						else if (info->colorpreference == 2)
						{
							Text_SetText(pbox, _("White"));
						}
						else
						{
							Text_SetText(pbox, _("No Preference"));
						}

						Box_AddChild(horizsize2, pbox);
					}

					horizsize2 = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					Box_AddChild(horizsize, horizsize2);
					{
						pbox = Text_Create(10 /*215*/, 0, 60, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
						pbox->fgcol = UserInfoFG2;
						Text_SetText(pbox, _("Rated Game"));
						Box_AddChild(horizsize2, pbox);

						AutoSize_AddSpacer(horizsize2, 10);

						pbox = Text_Create(10, /*280*/ 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
						pbox->fgcol =  RGB(222, 222, 222);
						Text_SetText(pbox, info->rated ? _("Yes") : _("No"));
						Box_AddChild(horizsize2, pbox);
					}
				}

				AutoSize_AddSpacer(vertsize, 2);

				horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ | AUTOSIZE_EVENSPACING);
				Box_AddChild(vertsize, horizsize);
				{
					struct Box_s *horizsize2 = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					Box_AddChild(horizsize, horizsize2);
					{
						pbox = Text_Create(0, 0, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
						pbox->fgcol = UserInfoFG2;
						Text_SetText(pbox, _("Game Variant"));
						Box_AddChild(horizsize2, pbox);

						AutoSize_AddSpacer(horizsize2, 10);

						pbox = Text_Create(10 /*115*/, 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
						pbox->fgcol = RGB(222, 222, 222);
						Text_SetText(pbox, Util_Capitalize(info->variant));
						Box_AddChild(horizsize2, pbox);
					}

					if (info->takebacks)
					{
						horizsize2 = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
						Box_AddChild(horizsize, horizsize2);
						{
							pbox = Text_Create(10 /*215*/, 0, 60, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
							pbox->fgcol = UserInfoFG2;
							Text_SetText(pbox, _("Takebacks"));
							Box_AddChild(horizsize2, pbox);

							AutoSize_AddSpacer(horizsize2, 10);

							pbox = Text_Create(10, /*280*/ 0, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
							pbox->fgcol =  RGB(222, 222, 222);
							Text_SetText(pbox, Util_Capitalize(info->takebacks));
							Box_AddChild(horizsize2, pbox);
						}
					}
				}

				AutoSize_AddSpacer(vertsize, 2);

				horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
				Box_AddChild(vertsize, horizsize);
				{
					char buffer[1024];

					pbox = Text_Create(10, 0, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
					pbox->fgcol = UserInfoFG2;
					Text_SetText(pbox, _("Time Settings"));
					Box_AddChild(horizsize, pbox);

					AutoSize_AddSpacer(horizsize, 10);

					pbox = Text_Create(10, 0, 265, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHVERT);
					pbox->fgcol =  RGB(222, 222, 222);
					Text_SetLinkColor(pbox, CR_LtOrange);
					Text_SetText(pbox, Info_TimeControlsToMultilineText(info->timecontrol, info->blacktimecontrol, buffer, 1024));
					Box_AddChild(horizsize, pbox);
				}

				AutoSize_AddSpacer(vertsize, 2);

				horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
				Box_AddChild(vertsize, horizsize);
				{
					pbox = Text_Create(10, 0, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_RIGHT);
					pbox->fgcol = UserInfoFG2;
					Text_SetText(pbox, _("Comment"));
					Box_AddChild(horizsize, pbox);

					AutoSize_AddSpacer(horizsize, 10);

					pbox = Text_Create(5, 0, 255, 40, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
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
			horizsize->OnSizeWidth = Box_OnSizeWidth_Stretch;
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

				AutoSize_AddSpacer(horizsize, 40);
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
				pbox = StdButton_Create(0, 0, 90, _("Change Game Terms"), 0);
				Button2_SetOnButtonHit(pbox, GameInvitation_OnChange);
				Box_AddChild(horizsize2, pbox);
				data->changebutton = pbox;

				pbox = StdButton_Create(0, 0, 90, _("Open Chat"), 0);
				Button2_SetOnButtonHit(pbox, GameInvitation_OnChat);
				Box_AddChild(horizsize2, pbox);
				data->openchatbutton = pbox;

				AutoSize_AddSpacer(horizsize2, 0);
			}

			horizsize2 = AutoSizeSpace_Create(0, 0, 0, 0, 0, 0, 10, AUTOSIZE_HORIZ);
			horizsize2->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Box_AddChild(horizsize, horizsize2);
			{	
				pbox = StdButton_Create(0, 0, 90, _("Accept Game"), 0);
				Button2_SetOnButtonHit(pbox, GameInvitation_OnAccept);
				Box_AddChild(horizsize2, pbox);
				data->acceptbutton = pbox;

				pbox = StdButton_Create(0, 0, 90, _("Decline Game"), 0);
				Button2_SetOnButtonHit(pbox, GameInvitation_OnDecline);
				Box_AddChild(horizsize2, pbox);
				data->cancelbutton = pbox;
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

		dialog->x += 20 * cascade;
		dialog->x %= remainw;
		dialog->y += 20 * cascade;
		dialog->y %= remainh;

		dialog->x += mi.rcWork.left;
		dialog->y += mi.rcWork.top;
	}

#if 0
	pbox = Text_Create(20, 40, dialog->w - 40, dialog->h - 80, BOX_TRANSPARENT, TX_WRAP);
	pbox->fgcol = RGB(222, 222, 222);
	Box_AddChild(dialog, pbox);
	data->messagebox = pbox;

	pbox = Box_Create(0, 0, dialog->w, dialog->h, BOX_VISIBLE | BOX_TRANSPARENT);
	data->infobox = pbox;
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(20, 45, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_RIGHTTEXT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Side"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(125, 45, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = RGB(222, 222, 222);
	/* Reversed here since it is the opponent's color preference */
	if (info->colorpreference == 1)
	{
		Box_SetText(pbox, _("Black"));
	}
	else if (info->colorpreference == 2)
	{
		Box_SetText(pbox, _("White"));
	}
	else
	{
		Box_SetText(pbox, _("No Preference"));
	}
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(265, 45, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Rated Game"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(330, 45, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = RGB(222, 222, 222);
	if (info->rated)
	{
		Box_SetText(pbox, _("Yes"));
	}
	else
	{
		Box_SetText(pbox, _("No"));
	}
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(20, 65, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_RIGHTTEXT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Game Variant"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(125, 65, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = RGB(222, 222, 222);
	Box_SetText(pbox, Util_Capitalize(info->variant));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(20, 85, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_RIGHTTEXT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Time Settings"));
	Box_AddChild(data->infobox, pbox);

	pbox = Text_Create(125, 85, 315, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->fgcol = RGB(222, 222, 222);
	Text_SetText(pbox, Info_TimeControlToLongText(info->timecontrol));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(20, 105, 95, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_RIGHTTEXT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Comment"));
	Box_AddChild(data->infobox, pbox);

	pbox = Text_Create(125, 105, 290, 40, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
	pbox->fgcol = RGB(222, 222, 222);
	Text_SetText(pbox, info->comment);
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(10, 139, 420, 1, BOX_VISIBLE);
	pbox->bgcol = RGB(77, 77, 77);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(10, 140, 420, 1, BOX_VISIBLE);
	pbox->bgcol = RGB(102, 102, 102);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(65, 165, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Opponent"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(120, 155, 265, 50, BOX_VISIBLE | BOX_BORDER);
	pbox->brcol = RGB(90, 97, 108);
	pbox->bgcol = DrawerBG;
	data->opponentbox = pbox;
	Box_AddChild(data->infobox, pbox);

	pbox = StdButton_Create(10, 270 - 30, 130, _("Change Game Terms"), 0);
	Button2_SetOnButtonHit(pbox, GameInvitation_OnChange);
	Box_AddChild(dialog, pbox);
	data->changebutton = pbox;

	pbox = StdButton_Create(10 + 135, 270 - 30, 80, _("Open Chat"), 0);
	Button2_SetOnButtonHit(pbox, GameInvitation_OnChat);
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(440 - 100 - 95, 270 - 30, 90, _("Accept Game"), 0);
	Button2_SetOnButtonHit(pbox, GameInvitation_OnAccept);
	Box_AddChild(dialog, pbox);
	data->acceptbutton = pbox;
	
	pbox = StdButton_Create(440 - 100, 270 - 30, 90, _("Decline Game"), 0);
	Button2_SetOnButtonHit(pbox, GameInvitation_OnDecline);
	Box_AddChild(dialog, pbox);
	data->cancelbutton = pbox;
#endif
	Box_CreateWndCustom(dialog, titlebartxt, roster->hwnd);

	MiniProfile_SetProfile(data->opponentbox, data->jid, NULL);

	Model_SubscribeProfile(data->jid, GameInvitation_SetProfile, dialog);
	dialog->OnDestroy = GameInvitation_OnDestroy;

	if (!Model_GetOption(OPTION_NOGAMENOTIFY))
	{
		FlashWindow(dialog->hwnd, 1);
	}

	return dialog;
}

void GameInvitation_SetError(struct Box_s *dialog, char *opponentjid, char *gameid, char *error, char *actor)
{
	struct gameinvitationdata_s *data = dialog->boxdata;
	char *barejid1 = Jid_Strip(opponentjid);
	char *barejid2 = Jid_Strip(data->jid);
	char txt[512];
	int w, h;

	Log_Write(0, "GameInvitation_SetError: %s %s %s %s %s\n", barejid1, barejid2, gameid, data->info->gameid, error);

	if (!gameid || !data->info || !data->info->gameid)
	{
		if (stricmp(barejid1, barejid2) != 0)
		{
			free(barejid1);
			free(barejid2);
			return;
		}
	}
	else if (strcmp(gameid, data->info->gameid) != 0)
	{
		free(barejid1);
		free(barejid2);

		return;
	}

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
			if (actor && actor[0])
			{
				i18n_stringsub(txt, 512, _("Game request cancelled by %1."), Model_GetFriendNick(actor));
			}
			else
			{
				i18n_stringsub(txt, 512, _("Game request cancelled by sender."));
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
		else
		{
			i18n_stringsub(txt, 512, _("Unknown error: %1"), error);
		}
	}
	else
	{
		i18n_stringsub(txt, 512, _("Unknown error."));
	}

/*
	if (strcmp(error, _("Game Request Cancelled.")) == 0)
	{
		sprintf(txt, error);
	}
	else
	{
		sprintf(txt, _("Game invite error!\n%s"), error);
	}
*/

	Text_SetText(data->messagebox, txt);

	data->infobox->flags &= ~BOX_VISIBLE;
	data->messagebox->flags |= BOX_VISIBLE;
	data->changebutton->flags &= ~BOX_VISIBLE;
	data->acceptbutton->flags &= ~BOX_VISIBLE;
	StdButton_SetTextAndResize(data->cancelbutton, _("Close"), 90);

	AutoSize_Fit(data->sizeablecontent);
	AutoSize_Fill(data->sizeablecontent);

	w = data->sizeablecontent->w + 20;
	h = data->sizeablecontent->h + 50;

	Box_MoveWndCustom(dialog, dialog->x + (dialog->w - w) / 2, dialog->y + (dialog->h - h) / 2, w, h);

	Box_Repaint(dialog);

	free(barejid1);
	free(barejid2);
}

void GameInvitation_ShowCorGameAccepted(struct Box_s *dialog, char *gameid)
{
	struct gameinvitationdata_s *data = dialog->boxdata;
	char txt[512];
	char gametxt[256];

	i18n_stringsub(gametxt, 256, "Click ^lhere^l to open game#%1", gameid);
	sprintf(txt, "%s\n%s", _("Correspondence game accepted."), gametxt);

	Text_SetText(data->messagebox, txt);
	Text_SetLinkCallback(data->messagebox, 1, ViewLink_CorGameOpen_OnClick, strdup(gameid));

	Box_Repaint(dialog);
	return;
}

char *GameInvitation_GetJid(struct Box_s *dialog)
{
	struct gameinvitationdata_s *data = dialog->boxdata;

	return data->jid;
}
