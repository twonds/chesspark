#include <stdio.h>
#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "edit.h"
#include "titledrag.h"
#include "text.h"

#include "constants.h"

#include "button2.h"
#include "combobox.h"
#include "checkbox.h"
#include "customtime.h"
#include "ctrl.h"
#include "editcustomtimes.h"
#include "edit2.h"
#include "gameslist.h"
#include "i18n.h"
#include "imagemgr.h"
#include "info.h"
#include "model.h"
#include "namedlist.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"


struct gamecreateaddata_s
{
	char *jid;
	char *id;

	struct gamesearchinfo_s *info;

	struct Box_s *ratedgamecheck;
	struct Box_s *sidecombo;
	struct Box_s *variantcombo;
	struct Box_s *timecombo;
	struct Box_s *commentedit;
	struct Box_s *ratingcombo;
	struct Box_s *ratedplayercheck;
	/*
	struct Box_s *ratingdirectioncombo;
	struct Box_s *ratingrangecombo;
	*/
	/*struct Box_s *ratingtypecombo;*/
	struct Box_s *okbutton;
	struct Box_s *cancelbutton;

	struct Box_s *customtime;
	struct Box_s *editcustom;
};


void GameCreateAd_OnCancel(struct Box_s *pbox)
{
	View_CloseGameCreateAdDialog();
}


void GameCreateAd_OnOK(struct Box_s *pbox)
{
	struct gamecreateaddata_s *data = pbox->parent->boxdata;

	data->info->comment = strdup(Edit2Box_GetText(data->commentedit));

	if (data->id)
	{
		Ctrl_RemoveGameAd(data->id);
	}

	Ctrl_PostGameAd(data->info);
	GameCreateAd_OnCancel(pbox);
}


void GameCreateAdEdit_OnEnter(struct Box_s *pbox, char *text)
{
	GameCreateAd_OnOK(pbox);
}


void GameCreateAd_OnSideCombo(struct Box_s *combo, char *side)
{
	struct Box_s *dialog = combo->parent;
	struct gamecreateaddata_s *data = dialog->boxdata;
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


void GameCreateAd_OnRatedGameCheckHit(struct Box_s *pbox, int checked)
{
	struct Box_s *dialog = pbox->parent;
	struct gamecreateaddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	info->rated = !checked;
}

void GameCreateAd_OnRatedPlayerCheckHit(struct Box_s *pbox, int checked)
{
	struct Box_s *dialog = pbox->parent;
	struct gamecreateaddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	info->filterunrated = checked;
}

void GameCreateAd_OnVariantCombo(struct Box_s *combo, char *variant)
{
	struct Box_s *dialog = combo->parent;
	struct gamecreateaddata_s *data = dialog->boxdata;
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
}

void GameCreateAd_OnTimeCombo(struct Box_s *combo, char *time);

void GameCreateAd_OnSetCustomTimeControl(struct Box_s *dialog, struct timecontrol_s *whitetimecontrol, struct timecontrol_s *blacktimecontrol, char *name)
{
	struct gamecreateaddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	if (!whitetimecontrol)
	{
		ComboBox_SetSelection(data->timecombo, Info_TimeControlsToText(info->timecontrol, info->blacktimecontrol));
		return;
	}

	if (!info->timecontrol)
	{
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
	}

	if (info->timecontrol->controlarray)
	{
		free(info->timecontrol->controlarray);
		info->timecontrol->controlarray = NULL;
	}

	info->timecontrol = Info_DupeTimeControl(whitetimecontrol);
	info->blacktimecontrol = Info_DupeTimeControl(blacktimecontrol);
	info->correspondence = whitetimecontrol->correspondence;

	ComboBox_SetSelection(data->timecombo, Info_TimeControlsToText(info->timecontrol, info->blacktimecontrol));

	Box_Repaint(data->timecombo);
}

void GameCreateAd_RefreshTimeCombo(struct Box_s *dialog)
{
	struct gamecreateaddata_s *data = dialog->boxdata;

	ComboBox_RemoveAllEntries(data->timecombo);

	ComboBox_AddEntry2(data->timecombo, "Long", _("Long - ^bGame^n in ^b30m^n (5s)"));
	ComboBox_AddEntry2(data->timecombo, "Speed", _("Speed - ^bGame^n in ^b15m^n (5s)"));
#ifdef CHESSPARK_RAPIDTIMECONTROL
	ComboBox_AddEntry2(data->timecombo, "Rapid", _("Rapid - ^bGame^n in ^b10m^n (5s)"));
#endif
	ComboBox_AddEntry2(data->timecombo, "Blitz", _("Blitz - ^bGame^n in ^b5m^n (2s)"));
	ComboBox_AddEntry2(data->timecombo, "Bullet", _("Bullet - ^bGame^n in ^b1m^n (2s)"));
#ifdef CHESSPARK_CORRESPONDENCE
	ComboBox_AddEntry2(data->timecombo, "Correspondence", _("Correspondence - ^b10 moves^n per ^b30d^n"));
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

			sprintf(txt, _("%s (about %s)"), entry->name, Info_SecsToText1((float)Info_TimeControlToEstimatedTime(tcp->white)));
			ComboBox_AddEntry2(data->timecombo, entry->name, txt);
			entry = entry->next;
		}
	}
	ComboBox_AddEntry(data->timecombo, NULL);
	ComboBox_AddEntry2(data->timecombo, "Custom...", _("Custom..."));
	ComboBox_AddEntry2(data->timecombo, "Manage Custom Controls...", _("Manage Custom Controls..."));
}

void GameCreateAd_OnCustomTimeDestroy(struct Box_s *dialog)
{
	struct gamecreateaddata_s *data = dialog->boxdata;

	data->customtime = NULL;
}

void GameCreateAd_OnEditCustomDestroy(struct Box_s *dialog)
{
	struct gamecreateaddata_s *data = dialog->boxdata;

	data->editcustom = NULL;
}

void GameCreateAd_OnTimeCombo(struct Box_s *combo, char *time)
{
	struct Box_s *dialog = combo->parent;
	struct gamecreateaddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	if (strcmp(time, "Long") == 0)
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
	else if (strcmp(time, "Speed") == 0)
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
	else if (strcmp(time, "Rapid") == 0)
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
	else if (strcmp(time, "Blitz") == 0)
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
	else if (strcmp(time, "Bullet") == 0)
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
	else if (strcmp(time, "Custom...") == 0)
	{
		data->customtime = CustomTime_Create(dialog, GameCreateAd_OnSetCustomTimeControl, NULL, NULL, NULL, GameCreateAd_RefreshTimeCombo, GameCreateAd_OnCustomTimeDestroy, 1);
	}
	else if (strcmp(time, "Correspondence") == 0)
	{
		Info_DestroyTimeControl(info->timecontrol);
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 3);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = -1;
		info->timecontrol->controlarray[2] = 30 * 24 * 60 * 60;
		info->timecontrol->delayinc = 0;
		Info_DestroyTimeControl(info->blacktimecontrol);
		info->blacktimecontrol = NULL;
		info->correspondence = 1;
	}
	else if (strcmp(time, "Manage Custom Controls...") == 0)
	{
		data->editcustom = EditCustomTimes_Create(dialog, NULL, GameCreateAd_OnEditCustomDestroy);
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
}
#if 0
void GameCreateAd_OnRatingRangeCombo(struct Box_s *combo, char *range)
{
	struct Box_s *dialog = combo->parent;
	struct gamecreateaddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	char *type = "rating"; /*ComboBox_GetSelectionName(data->ratingtypecombo);*/
	char *direction = ComboBox_GetSelectionName(data->ratingdirectioncombo);

	if (!info->limit)
	{
		info->limit = malloc(sizeof(*(info->limit)));
		info->limit->type = NULL;
		info->limit->low = -1;
		info->limit->high = -1;
	}

	info->limit->type = strdup(type);

	if (strcmp(direction, _("At Least")) == 0)
	{
		sscanf(range, "%d", &(info->limit->low));
		info->limit->high = -1;
	}
	else if (strcmp(direction, _("At Most")) == 0)
	{
		sscanf(range, "%d", &(info->limit->high));
		info->limit->low = -1;
	}
	else if (strcmp(direction, _("Between")) == 0)
	{
		char tx1[20], tx2[20], *p;
		strcpy(tx1, range);

		strcpy(tx1, range);
		p = strchr(tx1, '-');
		
		if (p)
		{
			strcpy(tx2, p + 1);
			*p = '\0';
			sscanf(tx1, "%d", &(info->limit->low));
			sscanf(tx2, "%d", &(info->limit->high));
		}
	}
}

void GameCreateAd_OnRatingTypeCombo(struct Box_s *combo, char *type)
{
	struct Box_s *dialog = combo->parent;
	struct gamecreateaddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	if (type && strcmp(type, _("Rating")) == 0)
	{
		char *direction = ComboBox_GetSelectionName(data->ratingdirectioncombo);

		if (strcmp(direction, _("At Least")) == 0)
		{
			ComboBox_SetDisabled(data->ratingrangecombo, 0);
			ComboBox_RemoveAllEntries(data->ratingrangecombo);
			ComboBox_AddEntry(data->ratingrangecombo, "0");
			ComboBox_AddEntry(data->ratingrangecombo, "500");
			ComboBox_AddEntry(data->ratingrangecombo, "1000");
			ComboBox_AddEntry(data->ratingrangecombo, "1300");
			ComboBox_AddEntry(data->ratingrangecombo, "1700");
			ComboBox_AddEntry(data->ratingrangecombo, "2000");
			ComboBox_AddEntry(data->ratingrangecombo, "2200");
			ComboBox_AddEntry(data->ratingrangecombo, "2400");
			ComboBox_AddEntry(data->ratingrangecombo, "2600");
			ComboBox_AddEntry(data->ratingrangecombo, "2800");
		}
		else if (strcmp(direction, _("At Most")) == 0)
		{
			ComboBox_SetDisabled(data->ratingrangecombo, 0);
			ComboBox_RemoveAllEntries(data->ratingrangecombo);
			ComboBox_AddEntry(data->ratingrangecombo, "500");
			ComboBox_AddEntry(data->ratingrangecombo, "1000");
			ComboBox_AddEntry(data->ratingrangecombo, "1300");
			ComboBox_AddEntry(data->ratingrangecombo, "1700");
			ComboBox_AddEntry(data->ratingrangecombo, "2000");
			ComboBox_AddEntry(data->ratingrangecombo, "2200");
			ComboBox_AddEntry(data->ratingrangecombo, "2400");
			ComboBox_AddEntry(data->ratingrangecombo, "2600");
			ComboBox_AddEntry(data->ratingrangecombo, "2800");
			ComboBox_AddEntry(data->ratingrangecombo, "3000");
		}
		else if (strcmp(direction, _("Between")) == 0)
		{
			ComboBox_SetDisabled(data->ratingrangecombo, 0);
			ComboBox_RemoveAllEntries(data->ratingrangecombo);
			ComboBox_AddEntry(data->ratingrangecombo, "0-500");
			ComboBox_AddEntry(data->ratingrangecombo, "500-1000");
			ComboBox_AddEntry(data->ratingrangecombo, "1000-1300");
			ComboBox_AddEntry(data->ratingrangecombo, "1300-1700");
			ComboBox_AddEntry(data->ratingrangecombo, "1700-2000");
			ComboBox_AddEntry(data->ratingrangecombo, "2000-2200");
			ComboBox_AddEntry(data->ratingrangecombo, "2200-2400");
			ComboBox_AddEntry(data->ratingrangecombo, "2400-2600");
			ComboBox_AddEntry(data->ratingrangecombo, "2600-2800");
			ComboBox_AddEntry(data->ratingrangecombo, "2800-3000");
		}
	}
}


void GameCreateAd_OnRatingDirectionCombo(struct Box_s *combo, char *direction)
{
	struct Box_s *dialog = combo->parent;
	struct gamecreateaddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	if (direction && strcmp(direction, _("Any Rating")) == 0)
	{
		/*ComboBox_SetSelection(data->ratingtypecombo, NULL);
		ComboBox_RemoveAllEntries(data->ratingtypecombo);*/
		ComboBox_SetSelection(data->ratingrangecombo, NULL);
		ComboBox_RemoveAllEntries(data->ratingrangecombo);
		if (info->limit)
		{
			free(info->limit);
			info->limit = NULL;
		}
		/*ComboBox_SetDisabled(data->ratingtypecombo, 1);*/
		ComboBox_SetDisabled(data->ratingrangecombo, 1);
	}
	else
	{
		/*ComboBox_RemoveAllEntries(data->ratingtypecombo);
		ComboBox_AddEntry(data->ratingtypecombo, _("Rating"));
		ComboBox_SetSelection(data->ratingtypecombo, _("Rating"));*/
		GameCreateAd_OnRatingTypeCombo(data->ratingrangecombo /*typecombo*/, _("Rating"));
		/*ComboBox_SetDisabled(data->ratingtypecombo, 0);*/
		ComboBox_SetDisabled(data->ratingrangecombo, 0);
	}
}
#endif

void GameCreateAd_OnRatingCombo(struct Box_s *combo, char *name)
{
	struct Box_s *dialog = Box_GetRoot(combo);
	struct gamecreateaddata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info = data->info;

	info->relativerating = 0;

	if (!name)
	{
	}
	else if (stricmp(name, "+/- 50") == 0)
	{
		info->relativerating = 50;
	}
	else if (stricmp(name, "+/- 100") == 0)
	{
		info->relativerating = 100;
	}
	else if (stricmp(name, "+/- 150") == 0)
	{
		info->relativerating = 150;
	}
	else if (stricmp(name, "+/- 200") == 0)
	{
		info->relativerating = 200;
	}
	else if (stricmp(name, "+/- 250") == 0)
	{
		info->relativerating = 250;
	}
	else if (stricmp(name, "+/- 300") == 0)
	{
		info->relativerating = 300;
	}
	else if (stricmp(name, "+/- 350") == 0)
	{
		info->relativerating = 350;
	}
	else if (stricmp(name, "+/- 400") == 0)
	{
		info->relativerating = 400;
	}
	else if (stricmp(name, "+/- 450") == 0)
	{
		info->relativerating = 450;
	}
	else if (stricmp(name, "+/- 500") == 0)
	{
		info->relativerating = 500;
	}
	else if (stricmp(name, _("+/- Any")) == 0)
	{
	}
}

void GameCreateAd_ShowInfo(struct Box_s *dialog)
{
	struct gamecreateaddata_s *data = dialog->boxdata;
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

	CheckBox_SetChecked(data->ratedgamecheck, !info->rated);

	if (info->variant)
	{
		if (stricmp(info->variant, "standard") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Standard"));
		}
		else if (stricmp(info->variant, "atomic") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Atomic"));
		}
		else if (stricmp(info->variant, "chess960") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Chess960"));
		}
		else if (stricmp(info->variant, "crazyhouse") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Crazyhouse"));
		}
		else if (stricmp(info->variant, "checkers") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Checkers"));
		}
		else if (stricmp(info->variant, "losers") == 0)
		{
			ComboBox_SetSelection(data->variantcombo, _("Loser's"));
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

	if (info->timecontrolrange)
	{
		ComboBox_SetSelection(data->timecombo, info->timecontrolrange);
	}
	else if (info->timecontrol)
	{
		ComboBox_SetSelection(data->timecombo, Info_TimeControlsToText(info->timecontrol, info->blacktimecontrol));
	}
	else
	{
		ComboBox_SetSelection(data->timecombo, _("Long"));
	}
#if 0
	if (info->limit)
	{
		if (info->limit->type)
		{
			char txt[80];

			/*ComboBox_SetSelection(data->ratingtypecombo, info->limit->type);*/

			if (info->limit->low != -1 && info->limit->high != -1)
			{
				ComboBox_SetSelection(data->ratingdirectioncombo, _("Between"));
				sprintf(txt, "%d-%d", info->limit->low, info->limit->high);
				ComboBox_SetSelection(data->ratingrangecombo, txt);
			}
			else if (info->limit->low == -1 && info->limit->high != -1)
			{
				ComboBox_SetSelection(data->ratingdirectioncombo, _("At Most"));
				sprintf(txt, "%d", info->limit->high);
				ComboBox_SetSelection(data->ratingrangecombo, txt);
			}
			else if (info->limit->low != -1 && info->limit->high == -1)
			{
				ComboBox_SetSelection(data->ratingdirectioncombo, _("At Least"));
				sprintf(txt, "%d", info->limit->low);
				ComboBox_SetSelection(data->ratingrangecombo, txt);
			}
		}
		/*ComboBox_SetDisabled(data->ratingtypecombo, 0);*/
		ComboBox_SetDisabled(data->ratingrangecombo, 0);
	}
	else
	{
		ComboBox_SetSelection(data->ratingdirectioncombo, _("Any Rating"));
		/*ComboBox_SetSelection(data->ratingtypecombo, NULL);*/
		ComboBox_SetSelection(data->ratingrangecombo, NULL);
		/*ComboBox_SetDisabled(data->ratingtypecombo, 1);*/
		ComboBox_SetDisabled(data->ratingrangecombo, 1);
	}
#endif
	if (info->relativerating)
	{
		char txt[256];
		char txt2[256];

		sprintf(txt, "%d", info->relativerating);
		i18n_stringsub(txt2, 256, _("Rating +/- %1"), txt);

		Box_SetText(data->ratingcombo, txt2);
	}
		
	if (info->comment)
	{
		Edit2Box_SetText(data->commentedit, info->comment);
	}
}

void GameCreateAdLink_OnClick(struct Box_s *pbox, char *url)
{
	Util_OpenURL(url);
}

void GameCreateAd_OnDestroy(struct Box_s *dialog)
{
	struct gamecreateaddata_s *data = dialog->boxdata;

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

struct Box_s *GameCreateAd_Create(struct Box_s *roster, struct gamesearchinfo_s *info)
{
	struct gamecreateaddata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *subbox;
	int x, y;

	memset(data, 0, sizeof(*data));

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

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - 420) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - 450) / 2;
	}

	memset(data, 0, sizeof(*data));

	dialog = Box_Create(x, y, 420, 450, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;

	dialog->boxdata = data;

	if (info)
	{
		data->id = strdup(info->gameid);
		data->info = Info_DupeGameSearchInfo(info);
		if (!info->variant)
		{
			data->info->variant = strdup("standard");
		}
		if (info->limit)
		{
			free(info->limit->type);
			free(info->limit);
			info->limit = NULL;
		}
	}
	else
	{
		data->info = malloc(sizeof(*(data->info)));
		memset(data->info, 0, sizeof(*(data->info)));
		data->info->rated = 1;
		data->info->variant = strdup("standard");
	}

	dialog->titlebar = TitleBarOnly_Add(dialog, _("Looking for a New Game"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	subbox = Box_Create(25, 38, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Side"));
	Box_AddChild(dialog, subbox);

	subbox = ComboBox_Create(20, 55, 100, 20, BOX_VISIBLE | BOX_BORDER);
	subbox->bgcol = RGB(255, 255, 255);
	ComboBox_AddEntry(subbox, _("White"));
	ComboBox_AddEntry(subbox, _("Black"));
	ComboBox_AddEntry(subbox, _("White or Black"));
	ComboBox_SetSelection(subbox, _("White or Black"));
	ComboBox_SetOnSelection(subbox, GameCreateAd_OnSideCombo);
	data->sidecombo = subbox;
	Box_AddChild(dialog, subbox);

	subbox = Box_Create(25, 78, 180, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	subbox->font = tahoma10_f;
	Box_SetText(subbox, _("Select the side you would like to play."));
	Box_AddChild(dialog, subbox);

	subbox = CheckBox_Create(195, 58, BOX_VISIBLE);
	CheckBox_SetOnHit(subbox, GameCreateAd_OnRatedGameCheckHit);
	data->ratedgamecheck = subbox;
	Box_AddChild(dialog, subbox);

	subbox = CheckBoxLinkedText_Create(210, 58, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->ratedgamecheck);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Unrated Game"));
	Box_AddChild(dialog, subbox);

	subbox = Box_Create(210, 76, 205, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	subbox->font = tahoma10_f;
	Box_SetText(subbox, _("Unrated games will not modify your rating."));
	Box_AddChild(dialog, subbox);

	subbox = Box_Create(25, 108, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Game Variant"));
	Box_AddChild(dialog, subbox);

	subbox = ComboBox_Create(20, 125, 170, 20, BOX_VISIBLE | BOX_BORDER);
	subbox->bgcol = RGB(255, 255, 255);
	ComboBox_AddEntry(subbox, _("Standard"));
	ComboBox_AddEntry(subbox, "Chess960");
#ifdef CHESSPARK_CRAZYHOUSE
	ComboBox_AddEntry(subbox, "Crazyhouse");
#endif
#ifdef CHESSPARK_LOSERS
	ComboBox_AddEntry(subbox, _("Loser's"));
#endif
	ComboBox_AddEntry(subbox, _("Atomic"));
#ifdef CHESSPARK_CHECKERS
	ComboBox_AddEntry(subbox, _("Checkers"));
#endif
	ComboBox_SetSelection(subbox, _("Standard"));
	ComboBox_SetOnSelection(subbox, GameCreateAd_OnVariantCombo);
	data->variantcombo = subbox;
	Box_AddChild(dialog, subbox);

	subbox = Text_Create(25, 148, 320, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	subbox->font = tahoma10_f;
	subbox->fgcol = UserInfoFG2;
	Text_SetLinkColor(subbox, CR_LtOrange);
	Text_SetText(subbox, _("Variants are modifications of the standard chess rules. ^lLearn More...^l"));
	Text_SetLinkCallback(subbox, 1, Util_OpenURL2, _("http://chesspark.com/help/variants/"));
	Box_AddChild(dialog, subbox);

	subbox = Box_Create(25, 183, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Time Settings"));
	Box_AddChild(dialog, subbox);

	subbox = ComboBox_Create(20, 200, 370, 20, BOX_VISIBLE | BOX_BORDER);
	subbox->bgcol = RGB(255, 255, 255);
	ComboBox_AddEntry2(subbox, "Long", _("Long - ^bGame^n in ^b30m^n (5s)"));
	ComboBox_AddEntry2(subbox, "Speed", _("Speed - ^bGame^n in ^b15m^n (5s)"));
	ComboBox_AddEntry2(subbox, "Rapid", _("Rapid - ^bGame^n in ^b10m^n (5s)"));
	ComboBox_AddEntry2(subbox, "Blitz", _("Blitz - ^bGame^n in ^b5m^n (2s)"));
	ComboBox_AddEntry2(subbox, "Bullet", _("Bullet - ^bGame^n in ^b1m^n (2s)"));
#ifdef CHESSPARK_CORRESPONDENCE
	ComboBox_AddEntry2(subbox, "Correspondence", _("Correspondence - ^b10 moves^n per ^b30d^n"));
#endif
	ComboBox_AddEntry2(subbox, "Custom...", _("Custom..."));
	ComboBox_AddEntry2(subbox, "Manage Custom Controls...", _("Manage Custom Controls..."));
	ComboBox_SetSelection(subbox, "Long");
	ComboBox_SetOnSelection(subbox, GameCreateAd_OnTimeCombo);
	data->timecombo = subbox;
	GameCreateAd_RefreshTimeCombo(dialog);
	Box_AddChild(dialog, subbox);

	subbox = Text_Create(25, 223, 300, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	subbox->fgcol = UserInfoFG2;
	subbox->font = tahoma10_f;
	Text_SetLinkColor(subbox, CR_LtOrange);
	Text_SetText(subbox, _("Set the time limits for each side. ^lLearn More...^l"));
	Text_SetLinkCallback(subbox, 1, Util_OpenURL2, strdup(_("http://chesspark.com/help/timecontrols/")));
	Box_AddChild(dialog, subbox);

	subbox = Box_Create(25, 253, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Opponent Rating"));
	Box_AddChild(dialog, subbox);

	subbox = ComboBox_Create(20, 270, 140, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry(subbox, "+/- 50");
	ComboBox_AddEntry(subbox, "+/- 100");
	ComboBox_AddEntry(subbox, "+/- 150");
	ComboBox_AddEntry(subbox, "+/- 200");
	ComboBox_AddEntry(subbox, "+/- 250");
	ComboBox_AddEntry(subbox, "+/- 300");
	ComboBox_AddEntry(subbox, "+/- 350");
	ComboBox_AddEntry(subbox, "+/- 400");
	ComboBox_AddEntry(subbox, "+/- 450");
	ComboBox_AddEntry(subbox, "+/- 500");
	ComboBox_AddEntry(subbox, _("+/- Any"));
	ComboBox_SetSelection(subbox, _("+/- Any"));
	ComboBox_SetOnSelection(subbox, GameCreateAd_OnRatingCombo);
	data->ratingcombo = subbox;
	Box_AddChild(dialog, subbox);

	subbox = CheckBox_Create(180, 273, BOX_VISIBLE);
	CheckBox_SetOnHit(subbox, GameCreateAd_OnRatedPlayerCheckHit);
	data->ratedplayercheck = subbox;
	Box_AddChild(dialog, subbox);

	subbox = CheckBoxLinkedText_Create(195, 273, 120, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->ratedplayercheck);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Block Unrated Players"));
	Box_AddChild(dialog, subbox);

#if 0
	subbox = ComboBox_Create(20, 270, 185, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry(subbox, _("Any Rating"));
	ComboBox_AddEntry(subbox, _("At Least"));
	ComboBox_AddEntry(subbox, _("At Most"));
	ComboBox_AddEntry(subbox, _("Between"));
	ComboBox_SetOnSelection(subbox, GameCreateAd_OnRatingDirectionCombo);
	data->ratingdirectioncombo = subbox;
	Box_AddChild(dialog, subbox);
#if 0
	subbox = ComboBox_Create(145, 270, 120, 20, BOX_VISIBLE | BOX_BORDER);
	/*ComboBox_AddEntry(subbox, "Award Level");*/
	/*ComboBox_AddEntry(subbox, "Rating");*/
	ComboBox_SetOnSelection(subbox, GameCreateAd_OnRatingTypeCombo);
	data->ratingtypecombo = subbox;
	Box_AddChild(dialog, subbox);
#endif
	subbox = ComboBox_Create(210, 270, 185, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_SetSelection(subbox, NULL);
	ComboBox_SetOnSelection(subbox, GameCreateAd_OnRatingRangeCombo);
	data->ratingrangecombo = subbox;
	Box_AddChild(dialog, subbox);

	ComboBox_SetSelection(data->ratingdirectioncombo, _("Any Rating"));
	/*ComboBox_SetSelection(data->ratingtypecombo, NULL);*/
	ComboBox_SetSelection(data->ratingrangecombo, NULL);
	/*ComboBox_SetDisabled(data->ratingtypecombo, 1);*/
	ComboBox_SetDisabled(data->ratingrangecombo, 1);
#endif
	
	subbox = Text_Create(25, 293, 300, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	subbox->fgcol = UserInfoFG2;
	subbox->font = tahoma10_f;
	Text_SetLinkColor(subbox, CR_LtOrange);
	Text_SetText(subbox, _("Filter who can respond to your ad. ^lLearn More...^l"));
	Text_SetLinkCallback(subbox, 1, Util_OpenURL2, _("http://chesspark.com/help/responsefilter/"));
	Box_AddChild(dialog, subbox);

	subbox = Box_Create(25, 328, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Additional Info"));
	Box_AddChild(dialog, subbox);

	subbox = Edit2Box_Create(20, 345, 370, 20, BOX_VISIBLE | BOX_BORDER, 1);
	subbox->bgcol = RGB(255, 255, 255);
	data->commentedit = subbox;
	Edit2Box_SetOnEnter(subbox, GameCreateAdEdit_OnEnter);
	Box_AddChild(dialog, subbox);

	subbox = Text_Create(25, 368, 300, 40, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
	subbox->fgcol = UserInfoFG2;
	subbox->font = tahoma10_f;
	Text_SetText(subbox, _("Enter any other comments or additional information you would like to include in the Game Ad."));
	Box_AddChild(dialog, subbox);

	subbox = StdButton_Create(420 - 175, 450 - 30, 80, _("OK"), 0);
	Button2_SetOnButtonHit(subbox, GameCreateAd_OnOK);
	Box_AddChild(dialog, subbox);
	data->okbutton = subbox;
	
	subbox = StdButton_Create(420 - 90, 450 - 30, 80, _("Cancel"), 0);
	Button2_SetOnButtonHit(subbox, GameCreateAd_OnCancel);
	Box_AddChild(dialog, subbox);
	data->cancelbutton = subbox;

	Box_CreateWndCustom(dialog, _("Create New Game Ad"), roster->hwnd);
	Box_SetFocus(data->commentedit);

	data->ratedgamecheck->nextfocus = data->sidecombo;
	data->sidecombo->nextfocus = data->variantcombo;
	data->variantcombo->nextfocus = data->timecombo;
	data->timecombo->nextfocus = data->ratingcombo; /*data->ratingdirectioncombo;*/
	/*data->ratingdirectioncombo->nextfocus = data->ratingrangecombo;*/
	/*data->ratingrangecombo->nextfocus = data->commentedit; *//*data->ratingtypecombo;*/
	/*data->ratingtypecombo->nextfocus = data->commentedit;*/
	data->ratingcombo->nextfocus = data->commentedit;
	data->commentedit->nextfocus = data->okbutton;
	data->okbutton->nextfocus = data->cancelbutton;
	data->cancelbutton->nextfocus = data->ratedgamecheck;

	data->ratedgamecheck->prevfocus = data->cancelbutton;
	data->sidecombo->prevfocus = data->ratedgamecheck;
	data->variantcombo->prevfocus = data->sidecombo;
	data->timecombo->prevfocus = data->variantcombo;
	data->ratingcombo->prevfocus = data->timecombo;
	/*data->ratingdirectioncombo->prevfocus = data->timecombo;*/
	/*data->ratingrangecombo->prevfocus = data->ratingdirectioncombo;*/
	/*data->ratingtypecombo->prevfocus = data->ratingrangecombo;*/
	data->commentedit->prevfocus = data->ratingcombo; /*data->ratingrangecombo;*/ /*data->ratingtypecombo;*/
	data->okbutton->prevfocus = data->commentedit;
	data->cancelbutton->prevfocus = data->okbutton;

	GameCreateAd_ShowInfo(dialog);
	
	if (!info || !info->timecontrol)
	{
#ifdef CHESSPARK_RAPIDTIMECONTROL
		char *membertype = Model_GetLocalMemberType();
		if (membertype && stricmp(membertype, "4") == 0)
		{
			ComboBox_SetSelection(data->timecombo, "Rapid");
			GameCreateAd_OnTimeCombo(data->timecombo, "Rapid");
		}
		else
		{
			ComboBox_SetSelection(data->timecombo, "Long");
			GameCreateAd_OnTimeCombo(data->timecombo, "Long");
		}
#else
		ComboBox_SetSelection(data->timecombo, "Long");
		GameCreateAd_OnTimeCombo(data->timecombo, "Long");
#endif
	}

	BringWindowToTop(dialog->hwnd);

	dialog->OnDestroy = GameCreateAd_OnDestroy;

	return dialog;
}
