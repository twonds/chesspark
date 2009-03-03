#include <stdio.h>
#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "edit.h"
#include "titledrag.h"
#include "text.h"

#include "constants.h"

#include "autosize.h"
#include "boxtypes.h"
#include "button2.h"
#include "checkbox.h"
#include "combobox.h"
#include "customtime.h"
#include "ctrl.h"
#include "edit2.h"
#include "editcustomtimes.h"
#include "i18n.h"
#include "info.h"
#include "imagemgr.h"
#include "log.h"
#include "model.h"
#include "options.h"
#include "profile.h"
#include "namedlist.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"


struct gameinvitefrienddata_s
{
	char *jid;
	struct gamesearchinfo_s *info;
	struct namedlist_s *ratinglist;

	struct Box_s *sizeablecontent;

	struct Box_s *infobox;
	struct Box_s *messagebox;

	struct Box_s *ratedcheck;
	struct Box_s *sidecombo;
	struct Box_s *variantcombo;
	struct Box_s *timecombo;
	struct Box_s *commentedit;
	struct Box_s *takebackscombo;

	struct Box_s *opponentbox;

	struct Box_s *okbutton;
	struct Box_s *cancelbutton;

	struct Box_s *customtime;
	struct Box_s *editcustom;

	int sent;
};

void GameInviteFriend_ShowProfileRating(struct Box_s *dialog, char *variant, char *timecontrol);
void GameInviteFriend_OnRatedCheckHit(struct Box_s *pbox, int checked);

void GameInviteFriend_Close(struct Box_s *pbox)
{
	struct gameinvitefrienddata_s *data = Box_GetRoot(pbox)->boxdata;

	View_CloseGameRequestDialog(data->jid, data->info->gameid);
}

void GameInviteFriend_OnCancel(struct Box_s *pbox)
{
	struct gameinvitefrienddata_s *data = Box_GetRoot(pbox)->boxdata;
	
	if (data->info->gameid)
	{
		Ctrl_DeclineMatch(data->info->gameid);
	}

	View_CloseGameRequestDialog(data->jid, data->info->gameid);
}


void GameInviteFriend_OnOK(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct gameinvitefrienddata_s *data = Box_GetRoot(pbox)->boxdata;
	char *text;
	struct gamesearchinfo_s *info = data->info;
	char txt[256];
	int w, h;

	if (info->correspondence && data->sent)
	{
		View_CloseGameRequestDialog(data->jid, info->gameid);
		return;
	}

	if (data->customtime)
	{
		Box_Destroy(data->customtime);
	}

	if (data->editcustom)
	{
		Box_Destroy(data->editcustom);
	}

	text = Edit2Box_GetText(data->commentedit);

	if (text && strlen(text) == 0)
	{
		text = NULL;
	}

	info->comment = strdup(text);

	if (info->correspondence)
	{
		sprintf(txt, "%s\n%s", _("Your request has been sent."), _("As this is a correspondence game, there may not be a response for some time."));
		pbox->flags &= ~BOX_VISIBLE;
		StdButton_SetText(data->cancelbutton, _("OK"));
		Button2_SetOnButtonHit(data->cancelbutton, GameInviteFriend_Close);
	}
	else
	{
		char nametxt[256];
		i18n_stringsub(nametxt, 256, "Awaiting response from %1...", Model_GetFriendNick(data->jid));

		sprintf(txt, "%s\n%s", _("Your request has been sent."), nametxt);
		pbox->flags &= ~BOX_VISIBLE;
		pbox->OnClose = GameInviteFriend_OnCancel;
	}

	Text_SetText(data->messagebox, txt);
	Ctrl_RequestMatch(data->jid, info);

	data->infobox->flags &= ~BOX_VISIBLE;
	data->messagebox->flags |= BOX_VISIBLE;

	data->sent = 1;

	AutoSize_Fit(data->sizeablecontent);
	AutoSize_Fill(data->sizeablecontent);

	w = data->sizeablecontent->w + 20;
	h = data->sizeablecontent->h + 50;

	Box_MoveWndCustom(dialog, dialog->x + (dialog->w - w) / 2, dialog->y + (dialog->h - h) / 2, w, h);

	Box_Repaint(Box_GetRoot(pbox));
}

void GameInviteFriend_OnVariantCombo(struct Box_s *combo, char *variant)
{
	struct Box_s *dialog = Box_GetRoot(combo);
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	if (stricmp(variant, _("Standard")) == 0)
	{
		free(info->variant);
		info->variant = strdup("standard");
	}
	else if (stricmp(variant, _("Atomic")) == 0)
	{
		free(info->variant);
		info->variant = strdup("atomic");
	}
	else if (stricmp(variant, _("Chess960")) == 0)
	{
		free(info->variant);
		info->variant = strdup("chess960");
	}
	else if (stricmp(variant, _("Crazyhouse")) == 0)
	{
		free(info->variant);
		info->variant = strdup("crazyhouse");
	}
	else if (stricmp(variant, _("Checkers")) == 0)
	{
		free(info->variant);
		info->variant = strdup("checkers");
	}
	else if (stricmp(variant, _("Loser's")) == 0)
	{
		free(info->variant);
		info->variant = strdup("losers");
	}

	GameInviteFriend_ShowProfileRating(dialog, info->variant, Info_TimeControlToCategory(data->info->timecontrol));
}

void GameInviteFriend_OnTakebacksCombo(struct Box_s *combo, char *takebacks)
{
	struct Box_s *dialog = Box_GetRoot(combo);
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	if (stricmp(takebacks, _("None")) == 0)
	{
		free(info->takebacks);
		info->takebacks = NULL;
	}
	else if (stricmp(takebacks, _("White Only")) == 0)
	{
		free(info->takebacks);
		info->takebacks = strdup("white");
		CheckBox_SetChecked(data->ratedcheck, 1);
		GameInviteFriend_OnRatedCheckHit(data->ratedcheck, 1);
	}
	else if (stricmp(takebacks, _("White Only")) == 0)
	{
		free(info->takebacks);
		info->takebacks = strdup("black");
		CheckBox_SetChecked(data->ratedcheck, 1);
		GameInviteFriend_OnRatedCheckHit(data->ratedcheck, 1);
	}
	else if (stricmp(takebacks, _("Both")) == 0)
	{
		free(info->takebacks);
		info->takebacks = strdup("both");
		CheckBox_SetChecked(data->ratedcheck, 1);
		GameInviteFriend_OnRatedCheckHit(data->ratedcheck, 1);
	}
}


void GameInviteFriend_OnSideCombo(struct Box_s *combo, char *side)
{
	struct Box_s *dialog = Box_GetRoot(combo);
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	if (strcmp(side, _("White")) == 0)
	{
		info->colorpreference = 1;
	}
	else if (strcmp(side, _("Black")) == 0)
	{
		info->colorpreference = 2;
	}
	else
	{
		info->colorpreference = 0;
	}
}

void GameInviteFriend_OnTimeCombo(struct Box_s *combo, char *time);

void GameInviteFriend_OnSetCustomTimeControl(struct Box_s *dialog, struct timecontrol_s *whitetimecontrol, struct timecontrol_s *blacktimecontrol, char *name)
{
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	if (!whitetimecontrol)
	{
		ComboBox_SetSelection(data->timecombo, Info_TimeControlToLongText(info->timecontrol));
		return;
	}

	Info_DestroyTimeControl(info->timecontrol);
	Info_DestroyTimeControl(info->blacktimecontrol);

	info->timecontrol = Info_DupeTimeControl(whitetimecontrol);
	info->blacktimecontrol = Info_DupeTimeControl(blacktimecontrol);
	info->correspondence = whitetimecontrol->correspondence;

	ComboBox_SetSelection(data->timecombo, Info_TimeControlToLongText(info->timecontrol));

	GameInviteFriend_ShowProfileRating(dialog, info->variant, Info_TimeControlToCategory(data->info->timecontrol));

	Box_Repaint(data->timecombo);
}

void GameInviteFriend_RefreshTimeCombo(struct Box_s *dialog)
{
	struct gameinvitefrienddata_s *data = dialog->boxdata;

	ComboBox_RemoveAllEntries(data->timecombo);

	ComboBox_AddEntry2(data->timecombo, _("Long"), _("Long - ^bGame^n in ^b30m^n (5s)"));
	ComboBox_AddEntry2(data->timecombo, _("Speed"), _("Speed - ^bGame^n in ^b15m^n (5s)"));
#ifdef CHESSPARK_RAPIDTIMECONTROL
	ComboBox_AddEntry2(data->timecombo, _("Rapid"), _("Rapid - ^bGame^n in ^b10m^n (5s)"));
#endif
	ComboBox_AddEntry2(data->timecombo, _("Blitz"), _("Blitz - ^bGame^n in ^b5m^n (2s)"));
	ComboBox_AddEntry2(data->timecombo, _("Bullet"), _("Bullet - ^bGame^n in ^b1m^n (2s)"));
#ifdef CHESSPARK_CORRESPONDENCE
	ComboBox_AddEntry2(data->timecombo, _("Correspondence"), _("Correspondence - ^b10 moves^n per ^b30d^n"));
#endif
	{
		struct namedlist_s *customtimecontrols = Model_GetCustomTimeControls();
		struct namedlist_s *entry = customtimecontrols;

		if (customtimecontrols)
		{
			ComboBox_AddEntry(data->timecombo, NULL);
		}

		while (entry)
		{
			struct tcpair_s *tcp = entry->data;
			char txt[512];

			i18n_stringsub(txt, 512, _("%1 (about %2)"), entry->name, Info_SecsToText1((float)Info_TimeControlToEstimatedTime(tcp->white)));
			ComboBox_AddEntry2(data->timecombo, entry->name, txt);
			entry = entry->next;
		}
	}
	ComboBox_AddEntry(data->timecombo, NULL);
	ComboBox_AddEntry(data->timecombo, _("Custom..."));
	ComboBox_AddEntry(data->timecombo, _("Manage Custom Controls..."));
}

void GameInviteFriend_OnCustomTimeDestroy(struct Box_s *dialog)
{
	struct gameinvitefrienddata_s *data = dialog->boxdata;

	data->customtime = NULL;
}

void GameInviteFriend_OnEditCustomDestroy(struct Box_s *dialog)
{
	struct gameinvitefrienddata_s *data = dialog->boxdata;

	data->editcustom = NULL;
}

void GameInviteFriend_OnTimeCombo(struct Box_s *combo, char *time)
{
	struct Box_s *dialog = Box_GetRoot(combo);
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	if (!time)
	{
		return;
	}

	if (strcmp(time, _("Long")) == 0)
	{
		Info_DestroyTimeControl(info->timecontrol);
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 3);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = -1;
		info->timecontrol->controlarray[2] = 30 * 60;
		info->timecontrol->delayinc = -5;
		Info_DestroyTimeControl(info->blacktimecontrol);
		info->blacktimecontrol = NULL;
		info->correspondence = 0;
	}
	else if (strcmp(time, _("Speed")) == 0)
	{
		Info_DestroyTimeControl(info->timecontrol);
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 3);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = -1;
		info->timecontrol->controlarray[2] = 15 * 60;
		info->timecontrol->delayinc = -5;
		Info_DestroyTimeControl(info->blacktimecontrol);
		info->blacktimecontrol = NULL;
		info->correspondence = 0;
	}
	else if (strcmp(time, _("Rapid")) == 0)
	{
		Info_DestroyTimeControl(info->timecontrol);
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 3);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = -1;
		info->timecontrol->controlarray[2] = 10 * 60;
		info->timecontrol->delayinc = -5;
		Info_DestroyTimeControl(info->blacktimecontrol);
		info->blacktimecontrol = NULL;
		info->correspondence = 0;
	}
	else if (strcmp(time, _("Blitz")) == 0)
	{
		Info_DestroyTimeControl(info->timecontrol);
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 3);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = -1;
		info->timecontrol->controlarray[2] = 5 * 60;
		info->timecontrol->delayinc = -2;
		Info_DestroyTimeControl(info->blacktimecontrol);
		info->blacktimecontrol = NULL;
		info->correspondence = 0;
	}
	else if (strcmp(time, _("Bullet")) == 0)
	{
		Info_DestroyTimeControl(info->timecontrol);
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 3);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = -1;
		info->timecontrol->controlarray[2] = 1 * 60;
		info->timecontrol->delayinc = -2;
		Info_DestroyTimeControl(info->blacktimecontrol);
		info->blacktimecontrol = NULL;
		info->correspondence = 0;
	}
	else if (strcmp(time, _("Custom...")) == 0)
	{
		data->customtime = CustomTime_Create(dialog, GameInviteFriend_OnSetCustomTimeControl, NULL, NULL, NULL, GameInviteFriend_RefreshTimeCombo, GameInviteFriend_OnCustomTimeDestroy, 1);
	}
	else if (strcmp(time, _("Correspondence")) == 0)
	{
		Info_DestroyTimeControl(info->timecontrol);
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 5);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = 10;
		info->timecontrol->controlarray[2] = 30 * 24 * 60 * 60;
		info->timecontrol->controlarray[3] = -1;
		info->timecontrol->controlarray[4] = 30 * 24 * 60 * 60;
		info->timecontrol->delayinc = 0;
		Info_DestroyTimeControl(info->blacktimecontrol);
		info->blacktimecontrol = NULL;
		info->correspondence = 1;
	}
	else if (strcmp(time, _("Manage Custom Controls...")) == 0)
	{
		data->editcustom = EditCustomTimes_Create(dialog, GameInviteFriend_RefreshTimeCombo, GameInviteFriend_OnEditCustomDestroy);
		ComboBox_SetSelection(data->timecombo, Info_TimeControlToLongText(info->timecontrol));
	}
	else
	{
		struct namedlist_s *customtimecontrols = Model_GetCustomTimeControls();
		struct namedlist_s *entry = customtimecontrols;

		while (entry && strcmp(time, entry->name) != 0)
		{
			entry = entry->next;
		}

		if (entry)
		{
			struct tcpair_s *tcp = entry->data;
			Info_DestroyTimeControl(info->timecontrol);
			Info_DestroyTimeControl(info->blacktimecontrol);
			info->timecontrol = Info_DupeTimeControl(tcp->white);
			info->blacktimecontrol = Info_DupeTimeControl(tcp->black);
			info->correspondence = tcp->white->correspondence;
		}
		else
		{
			Info_DestroyTimeControl(info->timecontrol);
			info->timecontrol = malloc(sizeof(*(info->timecontrol)));
			memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
			info->timecontrol->controlarray = NULL;
			info->timecontrol->delayinc = -5;
			Info_DestroyTimeControl(info->blacktimecontrol);
			info->blacktimecontrol = NULL;
			info->correspondence = 0;
		}
	}

	GameInviteFriend_ShowProfileRating(dialog, info->variant, Info_TimeControlToCategory(data->info->timecontrol));
}

void GameInviteFriend_OnRatedCheckHit(struct Box_s *pbox, int checked)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	info->rated = !checked;
	if (!checked && data->takebackscombo)
	{
		ComboBox_SetSelection(data->takebackscombo, _("None"));
		GameInviteFriend_OnTakebacksCombo(data->takebackscombo, _("None")); 
	}
}

void GameInviteFriend_ShowInfo(struct Box_s *dialog)
{
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	if (info->colorpreference == 1)
	{
		ComboBox_SetSelection(data->sidecombo, _("White"));
	}
	else if (info->colorpreference == 2)
	{
		ComboBox_SetSelection(data->sidecombo, _("Black"));
	}
	else
	{
		ComboBox_SetSelection(data->sidecombo, _("White or Black"));
	}

	CheckBox_SetChecked(data->ratedcheck, !info->rated);

	if (info->variant)
	{
		if (stricmp(info->variant, "standard") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Standard"));
		}
		else if (stricmp(info->variant, "chess960") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Chess960"));
		}
		else if (stricmp(info->variant, "losers") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Loser's"));
		}
		else if (stricmp(info->variant, "atomic") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Atomic"));
		}
		else if (stricmp(info->variant, "crazyhouse") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Crazyhouse"));
		}
		else if (stricmp(info->variant, "checkers") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Checkers"));
		}
		else
		{
			ComboBox_SetSelection(data->variantcombo, info->variant);
		}
	}
	else
	{
		ComboBox_SetSelection(data->variantcombo, _("Standard"));
	}

	if (info->correspondence)
	{
		ComboBox_SetSelection(data->timecombo, _("Correspondence"));
	}
	else if (!info->timecontrol)
	{
		ComboBox_SetSelection(data->timecombo, _("Long"));
	}
	else
	{
		ComboBox_SetSelection(data->timecombo, Info_TimeControlsToText(info->timecontrol, info->blacktimecontrol));
	}

	Box_Repaint(dialog);

}


void GameInviteFriend_ShowProfileRating(struct Box_s *dialog, char *variant, char *timecontrol)
{
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	struct namedlist_s **ratinglistentry = NULL;
	struct rating_s *rating = NULL;
	
	char txt[512];
	char ratingtext[80];
	char *category = "standard";
	char *transcategory;

	if (variant)
	{
		category = variant;
	}

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

	if (stricmp(category, "standard") == 0)
	{
		transcategory = _("Standard");
	}
	else if (stricmp(category, "atomic") == 0)
	{
		transcategory = _("Atomic");
	}
	else if (stricmp(category, "chess960") == 0)
	{
		transcategory = _("Chess960");
	}
	else if (stricmp(category, "crazyhouse") == 0)
	{
		transcategory = _("Crazyhouse");
	}
	else if (stricmp(category, "checkers") == 0)
	{
		transcategory = _("Checkers");
	}
	else if (stricmp(category, "losers") == 0)
	{
		transcategory = _("Loser's");
	}
	else
	{
		transcategory = _("Standard");
	}

	i18n_stringsub(txt, 512, _("Rating for %1 games: %2"), transcategory, ratingtext);
	Box_SetText(data->opponentbox->child->sibling, txt);

	Box_Repaint(data->opponentbox);
}

void GameInviteFriend_SetProfile(char *jid, struct profile_s *profile, struct Box_s *dialog)
{
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	char *jid1 = Jid_Strip(jid);
	char *jid2 = Jid_Strip(data->jid);
	
	if (stricmp(jid1, jid2) != 0)
	{
		free(jid1);
		free(jid2);
		return;
	}

	MiniProfile_SetProfile(data->opponentbox, jid, profile);

	data->ratinglist = Info_DupeRatingList(profile->ratings);

	GameInviteFriend_ShowProfileRating(dialog, data->info->variant, Info_TimeControlToCategory(data->info->timecontrol));

	Box_Repaint(data->opponentbox);
}

void GameInviteFriend_OnDestroy(struct Box_s *dialog)
{
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	Model_UnsubscribeProfile(data->jid, GameInviteFriend_SetProfile, dialog);

	if (data->customtime)
	{
		Box_Destroy(data->customtime);
	}

	if (data->editcustom)
	{
		Box_Destroy(data->editcustom);
	}
}

extern HFONT tahoma10_f;

struct Box_s *GameInviteFriend_Create(struct Box_s *roster, char *jid, struct gamesearchinfo_s *info, int replace, int playnow)
{
	struct gameinvitefrienddata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *pbox, *pbox2, *vertsize;
	char titlebartxt[512];

	memset(data, 0, sizeof(*data));

	dialog = Box_Create(0, 0, 420, 475, BOX_VISIBLE);
	dialog->boxtypeid = BOXTYPE_GAMEINVITEFRIEND;

	dialog->bgcol = DefaultBG;

	dialog->boxdata = data;

	if (info)
	{
		data->info = Info_DupeGameSearchInfo(info);
	}
	else
	{
		data->info = malloc(sizeof(*(data->info)));
		memset(data->info, 0, sizeof(*(data->info)));
		data->info->rated = 1;
	}

	data->jid = strdup(jid);

	if (replace)
	{
		i18n_stringsub(titlebartxt, 512, _("Game Renegotiation To %1"), Model_GetFriendNick(jid));
	}
	else
	{
		i18n_stringsub(titlebartxt, 512, _("Invite %1 To Game"), Model_GetFriendNick(jid));
	}
	dialog->titlebar = TitleBarOnly_Add(dialog, titlebartxt);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;
/*
	pbox = Box_Create(0, 0, dialog->w, dialog->h, BOX_VISIBLE | BOX_TRANSPARENT);
	data->infobox = pbox;
	Box_AddChild(dialog, pbox);
*/
	vertsize = AutoSize_Create(10, 38, 0, 0, 0, 0, AUTOSIZE_VERT);
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

				struct Box_s *horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
				Box_AddChild(vertsize, horizsize);

				{
					struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
					Box_AddChild(horizsize, vertsize);

					{
						pbox = Text_Create(5, 0, 90, 17, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
						pbox->fgcol = UserInfoFG2;
						Text_SetText(pbox, _("Side"));
						Box_AddChild(vertsize, pbox);

						pbox = ComboBox_Create(0, 0, 100, 20, BOX_VISIBLE | BOX_BORDER);
						pbox->bgcol = RGB(255, 255, 255);
						ComboBox_AddEntry(pbox, _("White"));
						ComboBox_AddEntry(pbox, _("Black"));
						ComboBox_AddEntry(pbox, _("White or Black"));
						ComboBox_SetOnSelection(pbox, GameInviteFriend_OnSideCombo);
						data->sidecombo = pbox;
						Box_AddChild(vertsize, pbox);

						pbox = Text_Create(5, 0, 180, 17, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHVERT | TX_WRAP);
						pbox->fgcol = UserInfoFG2;
						pbox->font = tahoma10_f;
						Text_SetText(pbox, _("Select the side you would like to play."));
						Box_AddChild(vertsize, pbox);
					}

					vertsize = AutoSize_Create(0, 20, 0, 0, 0, 0, AUTOSIZE_VERT);
					Box_AddChild(horizsize, vertsize);

					{
						struct Box_s *horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
						Box_AddChild(vertsize, horizsize);

						{
							pbox = CheckBox_Create(0, 0, BOX_VISIBLE);
							CheckBox_SetOnHit(pbox, GameInviteFriend_OnRatedCheckHit);
							data->ratedcheck = pbox;
							Box_AddChild(horizsize, pbox);

							AutoSize_AddSpacer(horizsize, 5);

							pbox2 = CheckBoxLinkedText_Create(0, 0, 0, 17, BOX_VISIBLE | BOX_TRANSPARENT, pbox);
							pbox2->fgcol = UserInfoFG2;
							Box_SetText(pbox2, _("Unrated Game"));
							Box_MeasureText(pbox2, pbox2->text, &(pbox2->w), NULL);
							Box_AddChild(horizsize, pbox2);
						}

						pbox = Text_Create(5, 76, 205, 17, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHVERT | TX_WRAP);
						pbox->fgcol = UserInfoFG2;
						pbox->font = tahoma10_f;
						Text_SetText(pbox, _("Unrated games will not modify your rating."));
						Box_AddChild(vertsize, pbox);
					}
				}

				AutoSize_AddSpacer(vertsize, 15);

				pbox = Text_Create(5, 0, 0, 17, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
				pbox->fgcol = UserInfoFG2;
				Text_SetText(pbox, _("Game Variant"));
				Box_AddChild(vertsize, pbox);

				pbox = ComboBox_Create(0, 0, 170, 20, BOX_VISIBLE | BOX_BORDER);
				pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
				pbox->bgcol = RGB(255, 255, 255);
				ComboBox_AddEntry(pbox, _("Standard"));
				ComboBox_AddEntry(pbox, "Chess960");
#ifdef CHESSPARK_CRAZYHOUSE
				ComboBox_AddEntry(pbox, "Crazyhouse");
#endif
#ifdef CHESSPARK_LOSERS
				ComboBox_AddEntry(pbox, _("Loser's"));
#endif
				ComboBox_AddEntry(pbox, _("Atomic"));
#ifdef CHESSPARK_CHECKERS
				ComboBox_AddEntry(pbox, _("Checkers"));
#endif
				ComboBox_SetOnSelection(pbox, GameInviteFriend_OnVariantCombo);
				data->variantcombo = pbox;
				Box_AddChild(vertsize, pbox);

				AutoSize_AddSpacer(vertsize, 3);

				pbox = Text_Create(5, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
				pbox->font = tahoma10_f;
				pbox->fgcol = UserInfoFG2;
				Text_SetLinkColor(pbox, CR_LtOrange);
				Text_SetText(pbox, _("Variants are modifications of the standard chess rules. ^lLearn More...^l"));
				Text_SetLinkCallback(pbox, 1, Util_OpenURL2, _("http://chesspark.com/help/variants/"));
				Box_AddChild(vertsize, pbox);

				AutoSize_AddSpacer(vertsize, 15);

				pbox = Text_Create(5, 0, 0, 17, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
				pbox->fgcol = UserInfoFG2;
				Text_SetText(pbox, _("Time Settings"));
				Box_AddChild(vertsize, pbox);

				pbox = ComboBox_Create(0, 0, 330, 20, BOX_VISIBLE | BOX_BORDER);
				pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
				pbox->bgcol = RGB(255, 255, 255);
				ComboBox_SetOnSelection(pbox, GameInviteFriend_OnTimeCombo);
				ComboBox_SetSelection(pbox, _("Long"));
				data->timecombo = pbox;
				GameInviteFriend_RefreshTimeCombo(dialog);
				Box_AddChild(vertsize, pbox);

				AutoSize_AddSpacer(vertsize, 3);

				pbox = Text_Create(5, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
				pbox->font = tahoma10_f;
				pbox->fgcol = UserInfoFG2;
				Text_SetLinkColor(pbox, CR_LtOrange);
				Text_SetText(pbox, _("Set the time limits for each side. ^lLearn More...^l"));
				Text_SetLinkCallback(pbox, 1, Util_OpenURL2, strdup(_("http://chesspark.com/help/timecontrols/")));
				Box_AddChild(vertsize, pbox);

				if (Model_GetPermission("takebacks"))
				{
					AutoSize_AddSpacer(vertsize, 15);

					pbox = Text_Create(5, 0, 0, 17, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
					pbox->fgcol = UserInfoFG2;
					Text_SetText(pbox, _("Takebacks"));
					Box_AddChild(vertsize, pbox);

					pbox = ComboBox_Create(0, 0, 170, 20, BOX_VISIBLE | BOX_BORDER);
					pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
					pbox->bgcol = RGB(255, 255, 255);
					ComboBox_AddEntry(pbox, _("None"));
					ComboBox_AddEntry(pbox, _("White Only"));
					ComboBox_AddEntry(pbox, _("Black Only"));
					ComboBox_AddEntry(pbox, _("Both"));
					ComboBox_SetSelection(pbox, _("None"));
					ComboBox_SetOnSelection(pbox, GameInviteFriend_OnTakebacksCombo);
					data->takebackscombo = pbox;
					Box_AddChild(vertsize, pbox);

					AutoSize_AddSpacer(vertsize, 3);

					pbox = Text_Create(5, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
					pbox->font = tahoma10_f;
					pbox->fgcol = UserInfoFG2;
					Text_SetLinkColor(pbox, CR_LtOrange);
					Text_SetText(pbox, _("Takebacks allow one or both sides to take back any move. ^lLearn More...^l"));
					Text_SetLinkCallback(pbox, 1, Util_OpenURL2, _("http://chesspark.com/help/takebacks/"));
					Box_AddChild(vertsize, pbox);
				}

				AutoSize_AddSpacer(vertsize, 15);

				pbox = Text_Create(5, 0, 0, 17, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
				pbox->fgcol = UserInfoFG2;
				Text_SetText(pbox, _("Game Comments"));
				Box_AddChild(vertsize, pbox);

				pbox = Edit2Box_Create(0, 0, 330, 20, BOX_VISIBLE | BOX_BORDER, E2_HORIZ);
				pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
				pbox->bgcol = RGB(255, 255, 255);
				data->commentedit = pbox;
				Box_AddChild(vertsize, pbox);

				AutoSize_AddSpacer(vertsize, 3);

				pbox = Text_Create(5, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
				pbox->font = tahoma10_f;
				pbox->fgcol = UserInfoFG2;
				Text_SetText(pbox, _("Enter any other comments you would like your opponent to see."));
				Box_AddChild(vertsize, pbox);
			}

			AutoSize_AddSpacer(vertsize2, 16);

			pbox = Box_Create(0, 0, vertsize->w, 1, BOX_VISIBLE);
			pbox->bgcol = RGB(77, 77, 77);
			pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
			Box_AddChild(vertsize2, pbox);

			pbox = Box_Create(0, 330, vertsize->w, 1, BOX_VISIBLE);
			pbox->bgcol = RGB(102, 102, 102);
			pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
			Box_AddChild(vertsize2, pbox);

			AutoSize_AddSpacer(vertsize2, 9);

			pbox = Box_Create(15, 340, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
			pbox->fgcol = UserInfoFG2;
			Box_SetText(pbox, _("Opponent"));
			Box_AddChild(vertsize2, pbox);

			pbox = MiniProfile_Create(15, 360, 300, 50, jid);
			pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
			data->opponentbox = pbox;
			Box_AddChild(vertsize2, pbox);
		}

		AutoSize_AddSpacer(vertsize, 33);

		vertsize2 = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		vertsize2->OnSizeWidth = Box_OnSizeWidth_Stretch;
		Box_AddChild(vertsize, vertsize2);
		{

			horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			horizsize->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Box_AddChild(vertsize2, horizsize);
			{

				pbox = StdButton_Create(0, 0, 80, _("OK"), 0);
				Button2_SetOnButtonHit(pbox, GameInviteFriend_OnOK);
				Box_AddChild(horizsize, pbox);
				data->okbutton = pbox;
	
				AutoSize_AddSpacer(horizsize, 20);

				pbox = StdButton_Create(0, 0, 80, _("Cancel"), 0);
				Button2_SetOnButtonHit(pbox, GameInviteFriend_OnCancel);
				Box_AddChild(horizsize, pbox);
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

		windowrect.left = roster->x;
		windowrect.right = windowrect.left + roster->w - 1;
		windowrect.top = roster->y;
		windowrect.bottom = windowrect.top + roster->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		dialog->x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - 420) / 2;
		dialog->y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - 475) / 2;
	}

#if 0

	pbox = Box_Create(25, 38, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Side"));
	Box_AddChild(data->infobox, pbox);

	pbox = ComboBox_Create(20, 55, 100, 20, BOX_VISIBLE | BOX_BORDER);
	pbox->bgcol = RGB(255, 255, 255);
	ComboBox_AddEntry(pbox, _("White"));
	ComboBox_AddEntry(pbox, _("Black"));
	ComboBox_AddEntry(pbox, _("White or Black"));
	ComboBox_SetOnSelection(pbox, GameInviteFriend_OnSideCombo);
	data->sidecombo = pbox;
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(25, 78, 180, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	pbox->font = tahoma10_f;
	Box_SetText(pbox, _("Select the side you would like to play."));
	Box_AddChild(data->infobox, pbox);

	pbox = CheckBox_Create(195, 58, BOX_VISIBLE);
	CheckBox_SetOnHit(pbox, GameInviteFriend_OnRatedCheckHit);
	data->ratedcheck = pbox;
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(210, 58, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Unrated Game"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(210, 76, 205, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	pbox->font = tahoma10_f;
	Box_SetText(pbox, _("Unrated games will not modify your rating."));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(25, 108, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Game Variant"));
	Box_AddChild(data->infobox, pbox);

	pbox = ComboBox_Create(20, 125, 170, 20, BOX_VISIBLE | BOX_BORDER);
	pbox->bgcol = RGB(255, 255, 255);
	ComboBox_AddEntry(pbox, _("Standard"));
	/*ComboBox_AddEntry(pbox, "Chess960");*/
	/*ComboBox_AddEntry(pbox, "Loser's");*/
	ComboBox_AddEntry(pbox, _("Atomic"));
	ComboBox_SetOnSelection(pbox, GameInviteFriend_OnVariantCombo);
	data->variantcombo = pbox;
	Box_AddChild(data->infobox, pbox);

	pbox = Text_Create(25, 148, 320, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->font = tahoma10_f;
	pbox->fgcol = UserInfoFG2;
	Text_SetLinkColor(pbox, CR_LtOrange);
	Text_SetText(pbox, _("Variants are modifications of the standard chess rules. ^lLearn More...^l"));
	Text_SetLinkCallback(pbox, 1, Util_OpenURL2, _("http://chesspark.com/help/variants/"));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(25, 183, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Time Settings"));
	Box_AddChild(data->infobox, pbox);

	pbox = ComboBox_Create(20, 200, 330, 20, BOX_VISIBLE | BOX_BORDER);
	pbox->bgcol = RGB(255, 255, 255);
	ComboBox_SetOnSelection(pbox, GameInviteFriend_OnTimeCombo);
	ComboBox_SetSelection(pbox, _("Long"));
	data->timecombo = pbox;
	GameInviteFriend_RefreshTimeCombo(dialog);
	Box_AddChild(data->infobox, pbox);

	pbox = Text_Create(25, 223, 300, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->font = tahoma10_f;
	pbox->fgcol = UserInfoFG2;
	Text_SetLinkColor(pbox, CR_LtOrange);
	Text_SetText(pbox, _("Set the time limits for each side. ^lLearn More...^l"));
	Text_SetLinkCallback(pbox, 1, Util_OpenURL2, strdup(_("http://chesspark.com/help/timecontrols/")));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(25, 253, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Game Comments"));
	Box_AddChild(data->infobox, pbox);

	pbox = Edit2Box_Create(20, 270, 330, 20, BOX_VISIBLE | BOX_BORDER, E2_HORIZ);
	pbox->bgcol = RGB(255, 255, 255);
	data->commentedit = pbox;
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(25, 293, 320, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->font = tahoma10_f;
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Enter any other comments you would like your opponent to see."));
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(10, 329, data->infobox->w - 20, 1, BOX_VISIBLE);
	pbox->bgcol = RGB(77, 77, 77);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(10, 330, data->infobox->w - 20, 1, BOX_VISIBLE);
	pbox->bgcol = RGB(102, 102, 102);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(data->infobox, pbox);

	pbox = Box_Create(25, 340, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Opponent"));
	Box_AddChild(data->infobox, pbox);

	pbox = MiniProfile_Create(25, 360, 300, 50, jid);
	data->opponentbox = pbox;
	Box_AddChild(data->infobox, pbox);

	pbox = StdButton_Create(420 - 200, 475 - 32, 80, _("OK"), 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, GameInviteFriend_OnOK);
	Box_AddChild(dialog, pbox);
	data->okbutton = pbox;
	
	pbox = StdButton_Create(420 - 100, 475 - 32, 80, _("Cancel"), 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, GameInviteFriend_OnCancel);
	Box_AddChild(dialog, pbox);
	data->cancelbutton = pbox;

#endif

	data->sidecombo->nextfocus = data->ratedcheck;
	data->ratedcheck->nextfocus = data->variantcombo;
	data->variantcombo->nextfocus = data->timecombo;
	data->timecombo->nextfocus = data->commentedit;
	data->commentedit->nextfocus = data->okbutton;
	data->okbutton->nextfocus = data->cancelbutton;
	data->cancelbutton->nextfocus = data->sidecombo;

	data->sidecombo->prevfocus = data->cancelbutton;
	data->ratedcheck->prevfocus = data->sidecombo;
	data->variantcombo->prevfocus = data->ratedcheck;
	data->timecombo->prevfocus = data->variantcombo;
	data->commentedit->prevfocus = data->timecombo;
	data->okbutton->prevfocus = data->commentedit;
	data->cancelbutton->prevfocus = data->okbutton;

	Box_CreateWndCustom(dialog, titlebartxt, roster->hwnd);

	GameInviteFriend_ShowInfo(dialog);

	if (!info || (!info->timecontrol && !info->correspondence))
	{
#ifdef CHESSPARK_RAPIDTIMECONTROL
		char *membertype = Model_GetLocalMemberType();
		if (membertype && stricmp(membertype, "4") == 0)
		{
			ComboBox_SetSelection(data->timecombo, "Rapid");
			GameInviteFriend_OnTimeCombo(data->timecombo, _("Rapid"));
		}
		else
		{
			ComboBox_SetSelection(data->timecombo, "Long");
			GameInviteFriend_OnTimeCombo(data->timecombo, _("Long"));
		}
#else
		ComboBox_SetSelection(data->timecombo, "Long");
		GameInviteFriend_OnTimeCombo(data->timecombo, _("Long"));
#endif
	}
	else if (info->correspondence)
	{
		ComboBox_SetSelection(data->timecombo, "Correspondence");
		GameInviteFriend_OnTimeCombo(data->timecombo, _("Correspondence"));
	}

	MiniProfile_SetProfile(data->opponentbox, data->jid, NULL);

	Model_SubscribeProfile(data->jid, GameInviteFriend_SetProfile, dialog);
	dialog->OnDestroy = GameInviteFriend_OnDestroy;
	dialog->OnClose = GameInviteFriend_Close;

	BringWindowToTop(dialog->hwnd);

	if (playnow)
	{
		GameInviteFriend_OnOK(data->okbutton);
	}

	return dialog;
}

void GameInviteFriend_SetError(struct Box_s *dialog, char *opponentjid, char *error, char *actor)
{
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	char *barejid1 = Jid_Strip(opponentjid);
	char *barejid2 = Jid_Strip(data->jid);

	if (barejid1 && barejid2 && stricmp(barejid1, barejid2) != 0)
	{
		free(barejid1);
		free(barejid2);
		return;
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
				if (actor && actor[0])
				{
					i18n_stringsub(txt, 512, _("Game request declined by %1."), Model_GetFriendNick(actor));
				}
				else
				{
					i18n_stringsub(txt, 512, _("Game request declined."));
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

		Text_SetText(data->messagebox, txt);

		data->infobox->flags &= ~BOX_VISIBLE;
		data->messagebox->flags |= BOX_VISIBLE;
		StdButton_SetText(data->cancelbutton, _("Close"));

		Box_Repaint(dialog);
	}
}

void GameInviteFriend_SetGameID(struct Box_s *dialog, char *opponentjid, char *gameid)
{
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	char *barejid1 = Jid_Strip(opponentjid);
	char *barejid2 = Jid_Strip(data->jid);

	Log_Write(0, "%s %s!", barejid1, barejid2);

	if (stricmp(barejid1, barejid2) == 0)
	{
		free(data->info->gameid);
		data->info->gameid = strdup(gameid);
	}

	free(barejid1);
	free(barejid2);
}

void GameInviteFriend_ShowCorGameAccepted(struct Box_s *dialog, char *gameid)
{
	struct gameinvitefrienddata_s *data = dialog->boxdata;
	char txt[512];
	char gametxt[256];

	i18n_stringsub(gametxt, 256, "Click ^lhere^l to open game#%1", gameid);
	sprintf(txt, "%s\n%s", _("Correspondence game accepted."), gametxt);

	Text_SetText(data->messagebox, txt);
	Text_SetLinkCallback(data->messagebox, 1, ViewLink_CorGameOpen_OnClick, strdup(gameid));

	Box_Repaint(dialog);
	return;
}
