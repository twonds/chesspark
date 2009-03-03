#include <stdlib.h>
#include <stdio.h>

#include "box.h"

#include "button.h"
#include "edit.h"
#include "text.h"

#include "autodialog.h"
#include "button2.h"
#include "checkbox.h"
#include "combobox.h"
#include "conn.h"
#include "constants.h"
#include "ctrl.h"
#include "edit2.h"
#include "imagemgr.h"
#include "i18n.h"
#include "info.h"
#include "link.h"
#include "list.h"
#include "log.h"
#include "model.h"
#include "namedlist.h"
#include "stdbutton.h"
#include "util.h"
#include "view.h"

extern int localadjournedgamesnum;
extern int localcorrespondencegamesnum;

enum gameslistpagetype
{
	GLP_HOME,
	GLP_QUICKMATCH,
	GLP_SEARCH,
	GLP_TOURNAMENTSEARCH,
	GLP_SEARCHRESULTS,
	GLP_MYGAMEADS
};

struct gameslistpageentry_s
{
	enum gameslistpagetype type;
	char *node;
	struct gamesearchinfo_s *info;
};

struct gameslistdata_s
{
	struct Box_s *list;
	struct Box_s *toplabel;
	struct Box_s *bottomlabel;
	struct Box_s *refresh;
	struct Box_s *refreshtext;
	struct Box_s *combobox;
	struct Box_s *backbutton;
	struct namedlist_s *pagehistory;
	int totalgamescount;
	int lastupdate;
	int cangoback;
};

struct gameslistentrydata_s
{
	struct Box_s *gameslist_box;
	char *name;
};

struct searchresultdata_s
{
	struct Box_s *gameslist_box;
	char *itemid;
	char *node;
	char *jid;
	struct gamesearchinfo_s *info;
	struct tournamentinfo_s *tournamentinfo;
};

void GamesList_SetPage(struct Box_s *gameslist_box, struct gameslistpageentry_s *pageentry);
void GamesList_RefreshTimeout(struct Box_s *gameslist_box, void *userdata);

void GamesList_UpdateLastUpdatedTime(struct Box_s *gameslist_box, void *userdata)
{
	struct gameslistdata_s *data = gameslist_box->boxdata;
	char txt[120];

	data->lastupdate++;

	if (data->refreshtext)
	{
		char updatedtxt[256];
		char buffer[120];

		i18n_stringsub(updatedtxt, 256, _("Updated %1 ago"), Info_SecsToTextShort(buffer, 120, (float)(data->lastupdate)));
		sprintf(txt, "%s - %s", _("Refresh"), updatedtxt);

		Box_SetText(data->refreshtext, txt);

		Box_Repaint(data->refreshtext);
	}
	else
	{
		Box_RemoveTimedFunc(gameslist_box, GamesList_UpdateLastUpdatedTime, 1000);
	}
}

void GamesList_OnDestroy(struct Box_s *gameslist_box)
{
	Box_RemoveTimedFunc(gameslist_box, GamesList_UpdateLastUpdatedTime, 1000);
	Box_RemoveTimedFunc(gameslist_box, GamesList_RefreshTimeout, 20000);
}


void GamesList_SetPageAndHistory(struct Box_s *gameslist_box, struct gameslistpageentry_s *pageentry)
{
	struct gameslistdata_s *gldata = gameslist_box->boxdata;

	NamedList_AddToTop(&(gldata->pagehistory), NULL, pageentry, NULL);
	GamesList_SetPage(gameslist_box, pageentry);

	if (!gldata->pagehistory->next)
	{
		Button_SetNormalImg (gldata->backbutton, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
		Button_SetPressedImg(gldata->backbutton, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
		Button_SetHoverImg  (gldata->backbutton, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
	}
	else
	{
		Button_SetNormalImg (gldata->backbutton, ImageMgr_GetSubImage("BackButton1", "BackButton.png",  0, 0, 22, 24));
		Button_SetHoverImg  (gldata->backbutton, ImageMgr_GetSubImage("BackButton2", "BackButton.png", 22, 0, 22, 24));
		Button_SetPressedImg(gldata->backbutton, ImageMgr_GetSubImage("BackButton3", "BackButton.png", 44, 0, 22, 24));
	}
}

void GamesListPageCombo_OnSelection(struct Box_s *pbox, char *name)
{
	struct Box_s *gameslist_box = pbox->parent->parent;
	struct gameslistdata_s *gldata = gameslist_box->boxdata;

	if (strcmp(name, _("Home")) == 0)
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_HOME;

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}
	else if (strcmp(name, _("Play Now")) == 0)
	{
/*
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_QUICKMATCH;

		GamesList_SetPageAndHistory(gameslist_box, pageentry);

		Ctrl_RequestGameSearch("quickplay", NULL);

*/
		Ctrl_PlayNow();
	}
	else if (strcmp(name, _("Search")) == 0)
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_SEARCH;

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}
	else if (strcmp(name, _("My Game Ads")) == 0)
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_SEARCHRESULTS;
		pageentry->node = strdup("myads");

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}
	else if (strcmp(name, _("My Correspondence Games")) == 0)
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_SEARCHRESULTS;
		pageentry->node = strdup("correspondence");

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}
	else if (strcmp(name, _("My Adjourned Games")) == 0)
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_SEARCHRESULTS;
		pageentry->node = strdup("adjourned");

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}
}

void GamesListEntryLink_OnClick(struct Box_s *pbox, void *userdata)
{
	struct Box_s *entrybox = pbox->parent;
	struct gameslistentrydata_s *data = entrybox->boxdata;
	struct Box_s *gameslist_box = data->gameslist_box;
	struct gameslistdata_s *gldata = gameslist_box->boxdata;
	
	if (strcmp(data->name, _("Play Now")) == 0)
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		Ctrl_PlayNow();

		/*Ctrl_RequestGameSearch("quickplay", NULL);*/
/*
		pageentry->type = GLP_QUICKMATCH;
		GamesList_SetPageAndHistory(gameslist_box, pageentry);
*/
	}
	else if (strcmp(data->name, _("Search for Games")) == 0)
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_SEARCH;

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}
	else if (strcmp(data->name, _("Place an Ad")) == 0)
	{
		View_PopupGameCreateAdDialog(NULL);
	}
	else if (strcmp(data->name, _("Correspondence Games")) == 0)
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_SEARCHRESULTS;
		pageentry->node = strdup("correspondence");

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}
	else if (strcmp(data->name, _("Adjourned Games")) == 0)
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_SEARCHRESULTS;
		pageentry->node = strdup("adjourned");

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}
	else if (strcmp(data->name, _("My Game Ads")) == 0)
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_SEARCHRESULTS;
		pageentry->node = strdup("myads");

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}
	else if (strcmp(data->name, _("Tournaments")) == 0)
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_TOURNAMENTSEARCH;
		/*pageentry->node = strdup("tournament");*/

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}
}

void GamesList_AddEntry(struct Box_s *gameslist_box, char *name, char *description)
{
	struct Box_s *entrybox, *subbox;
	struct gameslistdata_s *data;
	struct gameslistentrydata_s *entrydata;

	data = gameslist_box->boxdata;

	entrydata = malloc(sizeof(*entrydata));
	memset(entrydata, 0, sizeof(*entrydata));

	entrydata->name = strdup(name);
	entrydata->gameslist_box = gameslist_box;

	entrybox = Box_Create(0, 0, 100, 70, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->fgcol = TabFG1;
	entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	entrybox->OnSizeHeight = Box_OnSizeHeight_Stretch;

	subbox = LinkBox_Create(10, 5, 80, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	LinkBox_SetClickFunc(subbox, GamesListEntryLink_OnClick, NULL);
	Box_SetText(subbox, name);
	Box_AddChild(entrybox, subbox);

	subbox = Text_Create(10, 25, 80, 40, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
	subbox->fgcol = RGB(0, 0, 0);
	subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	Text_SetText(subbox, description);
	Box_AddChild(entrybox, subbox);
	
	entrybox->boxdata = entrydata;

	List_AddEntry(data->list, name, NULL, entrybox);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

void GamesList_AddEntry2(struct Box_s *gameslist_box, char *name, char *showname, char *description)
{
	struct Box_s *entrybox, *subbox;
	struct gameslistdata_s *data;
	struct gameslistentrydata_s *entrydata;

	data = gameslist_box->boxdata;

	entrydata = malloc(sizeof(*entrydata));
	memset(entrydata, 0, sizeof(*entrydata));

	entrydata->name = strdup(name);
	entrydata->gameslist_box = gameslist_box;

	entrybox = Box_Create(0, 0, 100, 70, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->fgcol = TabFG1;
	entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	entrybox->OnSizeHeight = Box_OnSizeHeight_Stretch;

	subbox = LinkBox_Create(10, 5, 80, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	LinkBox_SetClickFunc(subbox, GamesListEntryLink_OnClick, NULL);
	Box_SetText(subbox, showname);
	Box_AddChild(entrybox, subbox);

	subbox = Text_Create(10, 25, 80, 40, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
	subbox->fgcol = RGB(0, 0, 0);
	subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	Text_SetText(subbox, description);
	Box_AddChild(entrybox, subbox);
	
	entrybox->boxdata = entrydata;

	List_AddEntry(data->list, name, NULL, entrybox);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

struct gamessearchpaneldata_s
{
	struct Box_s *gameslist_box;
	struct Box_s *playradio;
	struct Box_s *watchradio;
	struct Box_s *variantcombo;
	struct Box_s *keywordsedit;
	struct Box_s *ratedcheck;
	struct Box_s *aicheck;
	struct Box_s *timecombo;
	struct Box_s *ratingdirectioncombo;
	struct Box_s *ratingtypecombo;
	struct Box_s *ratingrangecombo;
	struct Box_s *searchbutton;

	struct Box_s *pairingcombo;

	int gamestoplay;
	int ratedgame;
	int aigame;
	int tournament;
};

void GamesSearchPanel_OnPlayRadioHit(struct Box_s *pbox)
{
	struct Box_s *searchpanel = pbox->parent;
	struct gamessearchpaneldata_s *data = searchpanel->boxdata;

	Button_SetNormalImg (data->playradio,  ImageMgr_GetSubImage("RadioButton2", "RadioButton.png", 13, 0, 13, 13));
	Button_SetPressedImg(data->playradio,  ImageMgr_GetSubImage("RadioButton1", "RadioButton.png", 0, 0, 13, 13));

	Button_SetNormalImg (data->watchradio, ImageMgr_GetSubImage("RadioButton1", "RadioButton.png", 0, 0, 13, 13));
	Button_SetPressedImg(data->watchradio, ImageMgr_GetSubImage("RadioButton2", "RadioButton.png", 13, 0, 13, 13));

	Box_Repaint(data->playradio);
	Box_Repaint(data->watchradio);

	data->gamestoplay = TRUE;
}

void GamesSearchPanel_OnWatchRadioHit(struct Box_s *pbox)
{
	struct Box_s *searchpanel = pbox->parent;
	struct gamessearchpaneldata_s *data = searchpanel->boxdata;

	Button_SetNormalImg (data->playradio,  ImageMgr_GetSubImage("RadioButton1", "RadioButton.png", 0, 0, 13, 13));
	Button_SetPressedImg(data->playradio,  ImageMgr_GetSubImage("RadioButton2", "RadioButton.png", 13, 0, 13, 13));

	Button_SetNormalImg (data->watchradio, ImageMgr_GetSubImage("RadioButton2", "RadioButton.png", 13, 0, 13, 13));
	Button_SetPressedImg(data->watchradio, ImageMgr_GetSubImage("RadioButton1", "RadioButton.png", 0, 0, 13, 13));

	Box_Repaint(data->playradio);
	Box_Repaint(data->watchradio);

	data->gamestoplay = FALSE;
}

void GamesSearchPanel_OnRatedCheckHit(struct Box_s *pbox, int checked)
{
	struct Box_s *searchpanel = pbox->parent;
	struct gamessearchpaneldata_s *data = searchpanel->boxdata;

	data->ratedgame = checked;
}

void GamesSearchPanel_OnAICheckHit(struct Box_s *pbox, int checked)
{
	struct Box_s *searchpanel = pbox->parent;
	struct gamessearchpaneldata_s *data = searchpanel->boxdata;

	data->aigame = checked;
}

/*

void Conn_RequestGameSearch(char *node, char *variant, char *keywords, int rated, int computer,
							char *timecontrolrange, int titled, char *limittype, char *lowlimit, char *highlimit)
*/

struct gamesearchinfo_s *GamesSearchPanel_MakeInfo(struct Box_s *searchpanel)
{
	struct gamessearchpaneldata_s *data = searchpanel->boxdata;
	char tx1[30], tx2[30], *txt;
	struct gamesearchinfo_s *info = malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	txt = ComboBox_GetSelectionName(data->variantcombo);

	if (strcmp(txt, _("Standard")) == 0)
	{
		info->variant = strdup("standard");
	}
	else if (strcmp(txt, _("Chess960")) == 0)
	{
		info->variant = strdup("chess960");
	}
	else if (strcmp(txt, _("Loser's")) == 0)
	{
		info->variant = strdup("losers");
	}
	else if (strcmp(txt, _("Atomic")) == 0)
	{
		info->variant = strdup("atomic");
	}

	txt = Edit2Box_GetText(data->keywordsedit);

	if (txt && strlen(txt) == 0)
	{
		txt = NULL;
	}

	info->keywords = strdup(txt);

	info->rated = data->ratedgame;

	info->computers = data->aigame;

	if (data->timecombo)
	{
		txt = ComboBox_GetSelectionName(data->timecombo);
		info->correspondence = 0;

		if (strcmp(txt, _("Long")) == 0)
		{
			info->timecontrolrange = strdup("long");
		}
		else if (strcmp(txt, _("Speed")) == 0)
		{
			info->timecontrolrange = strdup("speed");
		}
		else if (strcmp(txt, _("Rapid")) == 0)
		{
			info->timecontrolrange = strdup("rapid");
		}
		else if (strcmp(txt, _("Blitz")) == 0)
		{
			info->timecontrolrange = strdup("blitz");
		}
		else if (strcmp(txt, _("Bullet")) == 0)
		{
			info->timecontrolrange = strdup("bullet");
		}
		else if (strcmp(txt, _("Correspondence")) == 0)
		{
			info->timecontrolrange = NULL;
			info->correspondence = 1;
		}
	}

	info->titled = FALSE; /* FIXME: what's titled mean? */
	
	if (data->ratingtypecombo)
	{

	txt = ComboBox_GetSelectionName(data->ratingtypecombo);

	if (txt && strcmp(txt, _("Rating")) == 0)
	{
		char *direction = ComboBox_GetSelectionName(data->ratingdirectioncombo);
		char *range = ComboBox_GetSelectionName(data->ratingrangecombo);

		info->limit = malloc(sizeof(*(info->limit)));
		info->limit->type = strdup("rating");
		info->limit->low = -1;
		info->limit->high = -1;

		if (strcmp(direction, _("At Least")) == 0)
		{
			sscanf(range, "%d", &(info->limit->low));
		}
		else if (strcmp(direction, _("At Most")) == 0)
		{
			sscanf(range, "%d", &(info->limit->high));
		}
		else if (strcmp(direction, _("Between")) == 0)
		{
			char *p;

			strcpy(tx1, range);
			p = strchr(tx1, '-');
			strcpy(tx2, p + 1);
			*p = '\0';

			sscanf(tx1, "%d", &(info->limit->low));
			sscanf(tx2, "%d", &(info->limit->high));
		}
	}
	}

	return info;
}

void GamesSearchPanel_OnSearch(struct Box_s *pbox)
{
	struct Box_s *searchpanel = pbox->parent;
	struct gamessearchpaneldata_s *data = searchpanel->boxdata;
	struct Box_s *gameslist_box = data->gameslist_box;
	struct gameslistdata_s *gldata = gameslist_box->boxdata;
	char *node;
	struct gamesearchinfo_s *info;

	if (data->gamestoplay)
	{
		node = "play";
	}
	else
	{
		node = "watch";
	}

	if (data->tournament)
	{
		node = "tournament";
	}

	info = GamesSearchPanel_MakeInfo(searchpanel);
	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));
		pageentry->info = info;
		pageentry->node = strdup(node);
		pageentry->type = GLP_SEARCHRESULTS;

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}
}

void GamesSearchPanelEdit_OnEnter(struct Box_s *pbox, char *text)
{
	GamesSearchPanel_OnSearch(pbox);
}

void GamesListSearch_OnRatingTypeCombo(struct Box_s *combo, char *type)
{
	struct Box_s *gamessearchpanel_box = combo->parent;
	struct gamessearchpaneldata_s *paneldata = gamessearchpanel_box->boxdata;

	if (type && strcmp(type, _("Rating")) == 0)
	{
		char *direction = ComboBox_GetSelectionName(paneldata->ratingdirectioncombo);

		if (strcmp(direction, _("At Least")) == 0)
		{
			ComboBox_RemoveAllEntries(paneldata->ratingrangecombo);
			ComboBox_AddEntry(paneldata->ratingrangecombo, "0");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "500");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "1000");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "1300");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "1700");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2000");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2200");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2400");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2600");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2800");
		}
		else if (strcmp(direction, _("At Most")) == 0)
		{
			ComboBox_RemoveAllEntries(paneldata->ratingrangecombo);
			ComboBox_AddEntry(paneldata->ratingrangecombo, "500");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "1000");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "1300");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "1700");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2000");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2200");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2400");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2600");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2800");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "3000");
		}
		else if (strcmp(direction, _("Between")) == 0)
		{
			ComboBox_RemoveAllEntries(paneldata->ratingrangecombo);
			ComboBox_AddEntry(paneldata->ratingrangecombo, "0-500");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "500-1000");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "1000-1300");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "1300-1700");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "1700-2000");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2000-2200");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2200-2400");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2400-2600");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2600-2800");
			ComboBox_AddEntry(paneldata->ratingrangecombo, "2800-3000");
		}
	}
}


void GamesListSearch_OnRatingDirectionCombo(struct Box_s *combo, char *direction)
{
	struct Box_s *gamessearchpanel_box = combo->parent;
	struct gamessearchpaneldata_s *paneldata = gamessearchpanel_box->boxdata;

	if (direction && strcmp(direction, _("Any Rating")) == 0)
	{
		ComboBox_SetSelection(paneldata->ratingtypecombo, NULL);
		ComboBox_RemoveAllEntries(paneldata->ratingtypecombo);
		ComboBox_SetSelection(paneldata->ratingrangecombo, NULL);
		ComboBox_RemoveAllEntries(paneldata->ratingrangecombo);
	}
	else
	{
		ComboBox_RemoveAllEntries(paneldata->ratingtypecombo);
		ComboBox_AddEntry(paneldata->ratingtypecombo, _("Rating"));
		GamesListSearch_OnRatingTypeCombo(paneldata->ratingtypecombo, ComboBox_GetSelectionName(paneldata->ratingtypecombo));
	}
}


void GamesList_AddSearchPanel(struct Box_s *gameslist_box)
{
	struct Box_s *entrybox, *subbox;
	struct gameslistdata_s *data;
	struct gamessearchpaneldata_s *paneldata;

	data = gameslist_box->boxdata;

	paneldata = malloc(sizeof(*paneldata));
	memset(paneldata, 0, sizeof(*paneldata));

	paneldata->gameslist_box = gameslist_box;

	entrybox = Box_Create(0, 0, 100, 290, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->fgcol = TabFG1;
	entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	entrybox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	entrybox->boxdata = paneldata;

	subbox = Box_Create(10, 5, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = TabFG2;
	Box_SetText(subbox, _("Search for"));
	Box_AddChild(entrybox, subbox);

	subbox = Button_Create(65, 5, 13, 13, BOX_VISIBLE | BOX_TRANSPARENT);
	Button_SetNormalImg(subbox, ImageMgr_GetSubImage("RadioButton2", "RadioButton.png", 13, 0, 13, 13));
	Button_SetPressedImg(subbox, ImageMgr_GetSubImage("RadioButton1", "RadioButton.png", 0, 0, 13, 13));
	Button_SetOnButtonHit(subbox, GamesSearchPanel_OnPlayRadioHit);
	paneldata->playradio = subbox;
	Box_AddChild(entrybox, subbox);

	subbox = ButtonLinkedText_Create(80, 5, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, paneldata->playradio);
	subbox->fgcol = RGB(0, 0, 0);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_SetText(subbox, _("Games to Play"));
	Box_AddChild(entrybox, subbox);

	subbox = Button_Create(65, 25, 13, 13, BOX_VISIBLE | BOX_TRANSPARENT);
	Button_SetNormalImg(subbox, ImageMgr_GetSubImage("RadioButton1", "RadioButton.png", 0, 0, 13, 13));
	Button_SetPressedImg(subbox, ImageMgr_GetSubImage("RadioButton2", "RadioButton.png", 13, 0, 13, 13));
	Button_SetOnButtonHit(subbox, GamesSearchPanel_OnWatchRadioHit);
	paneldata->watchradio = subbox;
	Box_AddChild(entrybox, subbox);

	subbox = ButtonLinkedText_Create(80, 25, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, paneldata->watchradio);
	subbox->fgcol = RGB(0, 0, 0);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_SetText(subbox, _("Games to Watch"));
	Box_AddChild(entrybox, subbox);

	paneldata->gamestoplay = TRUE;

	subbox = ComboBox_Create(10, 55, 200, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry(subbox, _("Standard"));
	/*
	ComboBox_AddEntry(subbox, "Chess960");
	ComboBox_AddEntry(subbox, "Loser's");
	*/
	ComboBox_AddEntry(subbox, _("Atomic"));
	ComboBox_AddEntry(subbox, _("Any Variant"));
	ComboBox_SetSelection(subbox, _("Any Variant"));
	paneldata->variantcombo = subbox;
	Box_AddChild(entrybox, subbox);
	
	subbox = Edit2Box_Create(10, 80, 200, 20, BOX_VISIBLE | BOX_BORDER, 1);
	subbox->bgcol = RGB(255, 255, 255);
	subbox->fgcol = RGB(0, 0, 0);
	Edit2Box_SetOnEnter(subbox, GamesSearchPanelEdit_OnEnter);
	paneldata->keywordsedit = subbox;
	Box_AddChild(entrybox, subbox);

	subbox = CheckBox_Create(10, 105, BOX_VISIBLE);
	CheckBox_SetOnHit(subbox, GamesSearchPanel_OnRatedCheckHit);
	paneldata->ratedcheck = subbox;
	Box_AddChild(entrybox, subbox);

	subbox = CheckBoxLinkedText_Create(25, 105, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, paneldata->ratedcheck);
	subbox->fgcol = RGB(0, 0, 0);
	Box_SetText(subbox, _("Only Rated Games"));
	Box_AddChild(entrybox, subbox);

	paneldata->ratedgame = FALSE;

	subbox = CheckBox_Create(10, 125, BOX_VISIBLE);
	CheckBox_SetOnHit(subbox, GamesSearchPanel_OnAICheckHit);
	CheckBox_SetChecked(subbox, 1);
	paneldata->aicheck = subbox;
	paneldata->aigame = 1;
	Box_AddChild(entrybox, subbox);

	subbox = CheckBoxLinkedText_Create(25, 125, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, paneldata->aicheck);
	subbox->fgcol = RGB(0, 0, 0);
	Box_SetText(subbox, _("Include Computer Opponents"));
	Box_AddChild(entrybox, subbox);

	paneldata->aigame = TRUE;

	subbox = Box_Create(25, 145, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = TabFG2;
	Box_SetText(subbox, _("Advanced Search Options"));
	Box_AddChild(entrybox, subbox);

	subbox = ComboBox_Create(20, 165, 190, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry2(subbox, _("Long"), _("Long (about 30m)"));
	ComboBox_AddEntry2(subbox, _("Speed"), _("Speed (about 15m)"));
	ComboBox_AddEntry2(subbox, _("Blitz"), _("Blitz (about 5m)"));
	ComboBox_AddEntry2(subbox, _("Bullet"), _("Bullet (about 1m)"));
#ifdef CHESSPARK_CORRESPONDENCE
	ComboBox_AddEntry2(subbox, _("Correspondence"), _("Correspondence"));
#endif
	ComboBox_AddEntry(subbox, NULL);
	ComboBox_AddEntry(subbox, _("Any Time Limit"));
	ComboBox_SetSelection(subbox, _("Any Time Limit"));
	paneldata->timecombo = subbox;
	Box_AddChild(entrybox, subbox);

	subbox = ComboBox_Create(20, 190, 190, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry(subbox, _("Any Rating"));
	ComboBox_AddEntry(subbox, _("At Least"));
	ComboBox_AddEntry(subbox, _("At Most"));
	ComboBox_AddEntry(subbox, _("Between"));
	ComboBox_SetSelection(subbox, _("Any Rating"));
	ComboBox_SetOnSelection(subbox, GamesListSearch_OnRatingDirectionCombo);
	paneldata->ratingdirectioncombo = subbox;
	Box_AddChild(entrybox, subbox);

	subbox = ComboBox_Create(20, 215, 190, 20, BOX_VISIBLE | BOX_BORDER);
	/*ComboBox_AddEntry(subbox, "Award Level");*/
	/*ComboBox_AddEntry(subbox, "Rating");*/
	ComboBox_SetSelection(subbox, NULL);
	ComboBox_SetOnSelection(subbox, GamesListSearch_OnRatingTypeCombo);
	paneldata->ratingtypecombo = subbox;
	Box_AddChild(entrybox, subbox);

	subbox = ComboBox_Create(20, 240, 190, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_SetSelection(subbox, NULL);
	paneldata->ratingrangecombo = subbox;
	Box_AddChild(entrybox, subbox);

	subbox = StdButton_Create(105, 265, 100, _("Search"), 1);
	Button2_SetOnButtonHit(subbox, GamesSearchPanel_OnSearch);
	Box_AddChild(entrybox, subbox);
	paneldata->searchbutton = subbox;

	paneldata->playradio->nextfocus = paneldata->watchradio;
	paneldata->watchradio->nextfocus = paneldata->variantcombo;
	paneldata->variantcombo->nextfocus = paneldata->keywordsedit;
	paneldata->keywordsedit->nextfocus = paneldata->ratedcheck;
	paneldata->ratedcheck->nextfocus = paneldata->aicheck;
	paneldata->aicheck->nextfocus = paneldata->timecombo;
	paneldata->timecombo->nextfocus = paneldata->ratingdirectioncombo;
	paneldata->ratingdirectioncombo->nextfocus = paneldata->ratingtypecombo;
	paneldata->ratingtypecombo->nextfocus = paneldata->ratingrangecombo;
	paneldata->ratingrangecombo->nextfocus = paneldata->searchbutton;
	paneldata->searchbutton->nextfocus = paneldata->playradio;

	paneldata->playradio->prevfocus = paneldata->searchbutton;
	paneldata->watchradio->prevfocus = paneldata->playradio;
	paneldata->variantcombo->prevfocus = paneldata->watchradio;
	paneldata->keywordsedit->prevfocus = paneldata->variantcombo;
	paneldata->ratedcheck->prevfocus = paneldata->keywordsedit;
	paneldata->aicheck->prevfocus = paneldata->ratedcheck;
	paneldata->timecombo->prevfocus = paneldata->aicheck;
	paneldata->ratingdirectioncombo->prevfocus = paneldata->timecombo;
	paneldata->ratingtypecombo->prevfocus = paneldata->ratingdirectioncombo;
	paneldata->ratingrangecombo->prevfocus = paneldata->ratingtypecombo;
	paneldata->searchbutton->prevfocus = paneldata->ratingrangecombo;

	List_AddEntry(data->list, "searchpanel", NULL, entrybox);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);

	Box_SetFocus(paneldata->keywordsedit);
}


void GamesList_AddTournamentSearchPanel(struct Box_s *gameslist_box)
{
	struct Box_s *entrybox, *subbox;
	struct gameslistdata_s *data;
	struct gamessearchpaneldata_s *paneldata;

	data = gameslist_box->boxdata;

	paneldata = malloc(sizeof(*paneldata));
	memset(paneldata, 0, sizeof(*paneldata));

	paneldata->tournament = 1;
	paneldata->gameslist_box = gameslist_box;

	entrybox = Box_Create(0, 0, 100, 290, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->fgcol = TabFG1;
	entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	entrybox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	entrybox->boxdata = paneldata;

	subbox = Box_Create(10, 5, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = TabFG2;
	Box_SetText(subbox, _("Search for"));
	Box_AddChild(entrybox, subbox);

	subbox = Button_Create(65, 5, 13, 13, BOX_VISIBLE | BOX_TRANSPARENT);
	Button_SetNormalImg(subbox, ImageMgr_GetSubImage("RadioButton2", "RadioButton.png", 13, 0, 13, 13));
	Button_SetPressedImg(subbox, ImageMgr_GetSubImage("RadioButton1", "RadioButton.png", 0, 0, 13, 13));
	Button_SetOnButtonHit(subbox, GamesSearchPanel_OnPlayRadioHit);
	paneldata->playradio = subbox;
	Box_AddChild(entrybox, subbox);

	subbox = ButtonLinkedText_Create(80, 5, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, paneldata->playradio);
	subbox->fgcol = RGB(0, 0, 0);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_SetText(subbox, _("Tournaments to Play"));
	Box_AddChild(entrybox, subbox);

	subbox = Button_Create(65, 25, 13, 13, BOX_VISIBLE | BOX_TRANSPARENT);
	Button_SetNormalImg(subbox, ImageMgr_GetSubImage("RadioButton1", "RadioButton.png", 0, 0, 13, 13));
	Button_SetPressedImg(subbox, ImageMgr_GetSubImage("RadioButton2", "RadioButton.png", 13, 0, 13, 13));
	Button_SetOnButtonHit(subbox, GamesSearchPanel_OnWatchRadioHit);
	paneldata->watchradio = subbox;
	Box_AddChild(entrybox, subbox);

	subbox = ButtonLinkedText_Create(80, 25, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, paneldata->watchradio);
	subbox->fgcol = RGB(0, 0, 0);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_SetText(subbox, _("Tournaments to Watch"));
	Box_AddChild(entrybox, subbox);

	paneldata->gamestoplay = TRUE;

	subbox = ComboBox_Create(10, 55, 200, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry(subbox, _("Standard"));
	/*
	ComboBox_AddEntry(subbox, "Chess960");
	ComboBox_AddEntry(subbox, "Loser's");
	*/
	ComboBox_AddEntry(subbox, _("Atomic"));
	ComboBox_AddEntry(subbox, _("Any Variant"));
	ComboBox_SetSelection(subbox, _("Any Variant"));
	paneldata->variantcombo = subbox;
	Box_AddChild(entrybox, subbox);
	
	subbox = ComboBox_Create(10, 80, 200, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry(subbox, _("Swiss Pairing"));
	ComboBox_AddEntry(subbox, _("Round Robin"));
	ComboBox_AddEntry(subbox, _("Elimination"));
	ComboBox_AddEntry(subbox, _("Any Pairing"));
	ComboBox_SetSelection(subbox, _("Any Pairing"));
	paneldata->pairingcombo = subbox;
	Box_AddChild(entrybox, subbox);

	subbox = Edit2Box_Create(10, 105, 200, 20, BOX_VISIBLE | BOX_BORDER, 1);
	subbox->bgcol = RGB(255, 255, 255);
	subbox->fgcol = RGB(0, 0, 0);
	paneldata->keywordsedit = subbox;
	Edit2Box_SetOnEnter(subbox, GamesSearchPanelEdit_OnEnter);
	Box_AddChild(entrybox, subbox);

	subbox = StdButton_Create(105, 130, 100, _("Search"), 1);
	Button2_SetOnButtonHit(subbox, GamesSearchPanel_OnSearch);
	Box_AddChild(entrybox, subbox);

	List_AddEntry(data->list, "searchpanel", NULL, entrybox);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);

	Box_SetFocus(paneldata->keywordsedit);
}

void GamesListBottomLink_OnClick(struct Box_s *pbox, void *userdata)
{
	Util_OpenURL("http://chesspark.com");
}

void GamesList_SetShowTopLabel(struct Box_s *gameslist_box, int show)
{
	struct gameslistdata_s *data = gameslist_box->boxdata;
	int dh;

	if (show)
	{
		if (data->toplabel->h != 0)
		{
			return;
		}
		dh = 70 - data->toplabel->h;

		Box_OnSizeHeight_Stretch(data->toplabel, dh);
		Box_OnSizeHeight_Stretch(data->list, -dh);
		data->list->y += dh;
	}
	else
	{
		if (data->toplabel->h == 0)
		{
			return;
		}
		dh = 0 - data->toplabel->h;

		Box_OnSizeHeight_Stretch(data->toplabel, dh);
		Box_OnSizeHeight_Stretch(data->list, -dh);
		data->list->y += dh;
	}
}

int SearchListSortByName(struct Box_s *lbox, struct Box_s *rbox)
{
	struct searchresultdata_s *ldata = lbox->boxdata;
	struct searchresultdata_s *rdata = rbox->boxdata;
	int compare;

	if (!ldata || !rdata)
	{
		return lbox < rbox;
	}

	if (ldata->info && rdata->info)
	{
		if (ldata->info->adplayer && rdata->info->adplayer)
		{
			compare = stricmp(Model_GetFriendNick(ldata->info->adplayer->jid), Model_GetFriendNick(rdata->info->adplayer->jid));

			if (compare != 0)
			{
				return compare < 0;
			}
		}

		if (ldata->info->white && rdata->info->white)
		{
			compare = stricmp(Model_GetFriendNick(ldata->info->white->jid), Model_GetFriendNick(rdata->info->white->jid));

			if (compare != 0)
			{
				return compare < 0;
			}
		}

		if (ldata->info->black && rdata->info->black)
		{
			compare = stricmp(Model_GetFriendNick(ldata->info->black->jid), Model_GetFriendNick(rdata->info->black->jid));

			if (compare != 0)
			{
				return compare < 0;
			}
		}

		return (strcmp(ldata->info->gameid, rdata->info->gameid) < 0);
	}
	else if (ldata->tournamentinfo && rdata->tournamentinfo)
	{
		compare = stricmp(ldata->tournamentinfo->name, rdata->tournamentinfo->name);

		if (compare != 0)
		{
			return compare < 0;
		}
	}

	return lbox < rbox;
}

int SearchListSortByVariant(struct Box_s *lbox, struct Box_s *rbox)
{
	struct searchresultdata_s *ldata = lbox->boxdata;
	struct searchresultdata_s *rdata = rbox->boxdata;
	int compare;

	if (!ldata || !rdata)
	{
		return lbox < rbox;
	}

	if (ldata->info && rdata->info)
	{
		if (ldata->info->variant && rdata->info->variant)
		{
			Log_Write(0, "%s %s\n", ldata->info->variant, rdata->info->variant);
			compare = strcmp(ldata->info->variant, rdata->info->variant) < 0;

			if (compare != 0)
			{
				return compare < 0;
			}
		}
	}

	return SearchListSortByName(lbox, rbox);
}

int SearchListSortByEstTime(struct Box_s *lbox, struct Box_s *rbox)
{
	struct searchresultdata_s *ldata = lbox->boxdata;
	struct searchresultdata_s *rdata = rbox->boxdata;
	int compare;

	if (!ldata || !rdata)
	{
		return lbox < rbox;
	}

	if (ldata->info && rdata->info)
	{
		compare = Info_TimeControlToEstimatedTime(ldata->info->timecontrol) - Info_TimeControlToEstimatedTime(rdata->info->timecontrol);

		if (compare != 0)
		{
			return compare < 0;
		}
	}

	return SearchListSortByVariant(lbox, rbox);
}

int SearchListSortByOnline(struct Box_s *lbox, struct Box_s *rbox)
{
	struct searchresultdata_s *ldata = lbox->boxdata;
	struct searchresultdata_s *rdata = rbox->boxdata;

	if (!ldata || !rdata)
	{
		return lbox < rbox;
	}

	if (ldata->info && rdata->info)
	{
		if (ldata->info->offline  && !rdata->info->offline)
		{
			return 0;
		}
		else if (!ldata->info->offline  && rdata->info->offline)
		{
			return 1;
		}
	}

	return SearchListSortByName(lbox, rbox);
}


void SearchSortCombo_OnSelection(struct Box_s *pbox, char *name)
{
	struct Box_s *gameslist_box = pbox->parent->parent;
	struct gameslistdata_s *data = gameslist_box->boxdata;

	if (strcmp(name, _("Online")) == 0)
	{
		List_SetEntrySortFunc(data->list, SearchListSortByOnline);
	}
	else if (strcmp(name, _("Variant")) == 0)
	{
		List_SetEntrySortFunc(data->list, SearchListSortByVariant);
	}
	else if (strcmp(name, _("Est. time")) == 0)
	{
		List_SetEntrySortFunc(data->list, SearchListSortByEstTime);
	}
	else if (strcmp(name, _("Name")) == 0)
	{
		List_SetEntrySortFunc(data->list, SearchListSortByName);
	}

	List_Resort(data->list);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

void GamesListRefreshButton_OnHit(struct Box_s *pbox)
{
	View_RefreshGamesListPage();
}

void GamesList_RefreshTimeout(struct Box_s *gameslist_box, void *userdata)
{
	struct gameslistdata_s *data = gameslist_box->boxdata;

	Box_RemoveTimedFunc(gameslist_box, GamesList_RefreshTimeout, 20000);

	if (data->refreshtext)
	{
		Box_SetText(data->refreshtext, "Refresh timed out!");

		Box_Repaint(data->refreshtext);
	}
}

void GamesList_SetPage(struct Box_s *gameslist_box, struct gameslistpageentry_s *pageentry)
{
	struct gameslistdata_s *data = gameslist_box->boxdata;

	List_RemoveAllEntries(data->list);
	List_SetEntrySortFunc(data->list, NULL);

	switch(pageentry->type)
	{
		case GLP_QUICKMATCH:
			GamesList_SetShowTopLabel(gameslist_box, 0);
			List_SetEntrySelectable(data->list, FALSE);
			GamesList_AddEntry(gameslist_box, _("Standard"), _("Standard chess, standard rules."));
			GamesList_AddEntry(gameslist_box, _("Chess960"), _("Random arrangement of pieces, 960 possible combinations"));
			GamesList_AddEntry(gameslist_box, _("Loser's"),  _("Play to lose your pieces!"));
			GamesList_AddEntry(gameslist_box, _("Atomic"),  _("Every capture takes out all non-pawns in a 3x3 square."));
			ComboBox_SetSelection(data->combobox, "Quickmatch");
			
			Text_SetText(data->bottomlabel, _("^0Don't see what you're looking for? Try searching the Chesspark Game Ads..."));
			break;
		case GLP_SEARCH:
			GamesList_SetShowTopLabel(gameslist_box, 0);
			List_SetEntrySelectable(data->list, FALSE);
			GamesList_AddSearchPanel(gameslist_box);
			ComboBox_SetSelection(data->combobox, _("Search"));
			
			Text_SetText(data->bottomlabel, _("^lSearch Help...^l"));
			Text_SetLinkCallback(data->bottomlabel, 1, GamesListBottomLink_OnClick, NULL);
			break;
		case GLP_TOURNAMENTSEARCH:
			GamesList_SetShowTopLabel(gameslist_box, 0);
			List_SetEntrySelectable(data->list, FALSE);
			GamesList_AddTournamentSearchPanel(gameslist_box);
			ComboBox_SetSelection(data->combobox, _("Search for Tournaments"));
			
			Text_SetText(data->bottomlabel, _("^lTournament Search Help...^l"));
			Text_SetLinkCallback(data->bottomlabel, 1, GamesListBottomLink_OnClick, NULL);
			break;
		case GLP_SEARCHRESULTS:
			GamesList_SetShowTopLabel(gameslist_box, 1);
			{
				char txt[512];
				struct Box_s *pbox, *subbox, *subbox2;

				while(data->toplabel->child)
				{
					Box_Destroy(data->toplabel->child);
				}

				data->refresh = NULL;
				data->refreshtext = NULL;
				Box_RemoveTimedFunc(gameslist_box, GamesList_UpdateLastUpdatedTime, 1000);
				Box_RemoveTimedFunc(gameslist_box, GamesList_RefreshTimeout, 20000);

				pbox = Text_Create(0, 5, data->toplabel->w, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);

				strcpy(txt, "^0");

				if (strcmp(pageentry->node, "watch") == 0)
				{
					strcat(txt, _("You searched for a ^bgame to watch:^b"));
				}
				else if (strcmp(pageentry->node, "play") == 0)
				{
					strcat(txt, _("You searched for a ^bgame to play:^b"));
				}
				else if (strcmp(pageentry->node, "myads") == 0)
				{
					strcat(txt, _("Your game ads:"));
				}
				else if (strcmp(pageentry->node, "adjourned") == 0)
				{
					strcat(txt, _("Your adjourned games"));
				}
				else if (strcmp(pageentry->node, "correspondence") == 0)
				{
					strcat(txt, _("Your correspondence games:"));
				}
				else if (strcmp(pageentry->node, "tournament") == 0)
				{
					strcat(txt, _("You searched for a ^btournament:^b"));
				}
				else
				{
					strcat(txt, _("Unknown search."));
				}

				Text_SetText(pbox, txt);
				Box_AddChild(data->toplabel, pbox);

				pbox = Text_Create(0, 41, 45, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
				Text_SetText(pbox, "Sort By:");
				Box_AddChild(data->toplabel, pbox);

				pbox = ComboBox_Create(45, 41, 80, 20, BOX_VISIBLE | BOX_BORDER);
				pbox->bgcol = RGB(255, 255, 255);
				pbox->fgcol = RGB(0, 0, 0);
				ComboBox_AddEntry(pbox, _("Online"));
				ComboBox_AddEntry(pbox, _("Variant"));
				ComboBox_AddEntry(pbox, _("Est. time"));
				ComboBox_AddEntry(pbox, _("Name"));
				ComboBox_SetSelection(pbox, _("Online"));
				ComboBox_SetOnSelection(pbox, SearchSortCombo_OnSelection);
				Box_AddChild(data->toplabel, pbox);

				List_SetEntrySortFunc(data->list, SearchListSortByOnline);

	subbox = Button2_Create(5, 21, data->toplabel->w - 10, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(data->toplabel, subbox);
	Button2_SetOnButtonHit(subbox, GamesListRefreshButton_OnHit);
	data->refresh = subbox;

	subbox = Box_Create(5, 21, data->toplabel->w - 10, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(data->toplabel, subbox);

	subbox2 = Box_Create(15, 2, 12, 14, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("spinbutton1", "spinbutton.png", 0, 0, 12, 14);
	Box_AddChild(subbox, subbox2);

	Button2_SetNormal(data->refresh, subbox);

	subbox = Box_Create(5, 21, data->toplabel->w - 10, 18, BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(data->toplabel, subbox);

	subbox2 = Box_Create(0, 0, 7, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonL2", "roomrefreshbutton.png", 0, 18, 7, 18);
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(7, 0, subbox->w - 7 - 7, 18, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonC2", "roomrefreshbutton.png", 7, 18, 156, 18);
	subbox2->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(subbox->w - 7, 0, 7, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonR2", "roomrefreshbutton.png", 163, 18, 7, 18);
	subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(15, 2, 12, 14, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("spinbutton2", "spinbutton.png", 12, 0, 12, 14);
	Box_AddChild(subbox, subbox2);

	Button2_SetPressed(data->refresh, subbox);

	subbox = Box_Create(5, 21, data->toplabel->w - 10, 18,  BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(data->toplabel, subbox);

	subbox2 = Box_Create(0, 0, 7, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonL3", "roomrefreshbutton.png", 0, 36, 7, 18);
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(7, 0, subbox->w - 7 - 7, 18, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonC3", "roomrefreshbutton.png", 7, 36, 156, 18);
	subbox2->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(subbox->w - 7, 0, 7, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonR3", "roomrefreshbutton.png", 163, 36, 7, 18);
	subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(15, 2, 12, 14, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("spinbutton3", "spinbutton.png", 24, 0, 12, 14);
	Box_AddChild(subbox, subbox2);

	Button2_SetHover(data->refresh, subbox);

	subbox = Box_Create(5, 21, data->toplabel->w - 10, 18, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->fgcol = TabFG1;
	Box_SetText(subbox, _("Searching..."));
	Box_AddChild(data->toplabel, subbox);
	data->refreshtext = subbox;

			}
			List_SetEntrySelectable(data->list, FALSE);
			Text_SetText(data->bottomlabel, NULL);
			ComboBox_SetSelection(data->combobox, _("Search Results"));

			Box_RemoveTimedFunc(gameslist_box, GamesList_RefreshTimeout, 20000);
			Box_AddTimedFunc(gameslist_box, GamesList_RefreshTimeout, NULL, 20000);
			
			break;
		case GLP_HOME:
		default:
			GamesList_SetShowTopLabel(gameslist_box, 0);
			List_SetEntrySelectable(data->list, FALSE);
			GamesList_AddEntry(gameslist_box, _("Play Now"), _(/*"Pick a game type and "*/ "Chesspark will match you with a " /*"comparable"*/ "random opponent."));
#ifdef CHESSPARK_CORRESPONDENCE
			{
				char txt[256];
				char numtxt[256];

				sprintf(numtxt, "%d", localcorrespondencegamesnum);
				i18n_stringsub(txt, 256, _("Correspondence Games (%1)"), numtxt);

				GamesList_AddEntry2(gameslist_box, _("Correspondence Games"), txt, _("View and play your long-term games from this list."));
			}
#endif
			{
				char txt[256];
				char numtxt[256];

				sprintf(numtxt, "%d", localadjournedgamesnum);
				i18n_stringsub(txt, 256, _("Adjourned Games (%1)"), numtxt);

				GamesList_AddEntry2(gameslist_box, _("Adjourned Games"), txt, _("Resume or forfeit your adjourned games from this list."));
			}
			GamesList_AddEntry(gameslist_box, _("Search for Games"), _("Find a game to play or a game to watch by searching Chesspark Game Ads."));
			GamesList_AddEntry(gameslist_box, _("Place an Ad"), _("Have games come to you! Place a Game Ad if you want to play a specific variant or against a certain skill level."));
			/*GamesList_AddEntry(gameslist_box, "Browse the Game Library", "Use Chesspark's extensive game library to replay games and learn new things.");*/
			GamesList_AddEntry(gameslist_box, _("My Game Ads"), _("View or remove your existing Game Ads."));
			/*GamesList_AddEntry(gameslist_box, _("Tournaments"), _("Look for tournaments to watch or play in."));*/
			/*sprintf(txt, _("^4Currently %d games being played."), data->totalgamescount);*/
			Text_SetText(data->bottomlabel, NULL);
			ComboBox_SetSelection(data->combobox, _("Home"));
			break;
	}

	List_RedoEntries(data->list);
	List_ScrollToTop(data->list);
	Box_Repaint(data->list);

	if (pageentry->node)
	{
		Ctrl_RequestGameSearch(pageentry->node, pageentry->info);
	}
}

void GamesList_UpdateAdjournedCount(struct Box_s *gameslist, int adjourned)
{
	struct gameslistdata_s *data = gameslist->boxdata;
	struct gameslistpageentry_s *currentpage = data->pagehistory->data;

	if (currentpage->type == GLP_HOME)
	{
		View_RefreshGamesListPage();
	}
}

void GamesListBackButton_OnHit(struct Box_s *pbox)
{
	struct gameslistdata_s *data;
	struct gameslistpageentry_s *pageentry;
	struct Box_s *gameslist_box = pbox->parent->parent;

	data = gameslist_box->boxdata;

	if (!data->pagehistory->next)
	{
		return;
	}

	NamedList_Remove(&(data->pagehistory));

	pageentry = data->pagehistory->data;

	GamesList_SetPage(gameslist_box, pageentry);

	if (!data->pagehistory->next)
	{
		Button_SetNormalImg (data->backbutton, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
		Button_SetHoverImg  (data->backbutton, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
		Button_SetPressedImg(data->backbutton, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
	}
	else
	{
		Button_SetNormalImg (data->backbutton, ImageMgr_GetSubImage("BackButton1", "BackButton.png",  0, 0, 22, 24));
		Button_SetHoverImg  (data->backbutton, ImageMgr_GetSubImage("BackButton2", "BackButton.png", 22, 0, 22, 24));
		Button_SetPressedImg(data->backbutton, ImageMgr_GetSubImage("BackButton3", "BackButton.png", 44, 0, 22, 24));
	}
}

struct Box_s *GamesList_Create()
{
	struct Box_s *toppanel, *subbox;
	struct gameslistdata_s *data;

	struct Box_s *gameslist_box = Box_Create(0, 0, 100, 100, BOX_TRANSPARENT);
	gameslist_box->OnSizeWidth = Box_OnSizeWidth_Stretch;
	gameslist_box->OnSizeHeight = Box_OnSizeHeight_Stretch;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	toppanel = Box_Create(0, 0, 100, 49, BOX_VISIBLE);
	toppanel->OnSizeWidth = Box_OnSizeWidth_Stretch;
	toppanel->bgcol = TabBG2;
	toppanel->fgcol = TabFG2;
	Box_AddChild(gameslist_box, toppanel);

	subbox = Button_Create(8, 11, 21, 24, BOX_VISIBLE | BOX_TRANSPARENT);
	Button_SetNormalImg (subbox, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
	Button_SetPressedImg(subbox, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
	Button_SetHoverImg  (subbox, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
	Button_SetOnButtonHit(subbox, GamesListBackButton_OnHit);
	Button_SetTooltipText(subbox, _("Back"));
	Box_AddChild(toppanel, subbox);
	data->backbutton = subbox;

	subbox = ComboBox_Create(36, 12, 100-36-5, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry(subbox, _("Home"));
	ComboBox_AddEntry(subbox, NULL);
#ifdef CHESSPARK_CORRESPONDENCE
	ComboBox_AddEntry(subbox, _("My Correspondence Games"));
#endif
	ComboBox_AddEntry(subbox, _("My Adjourned Games"));
	ComboBox_AddEntry(subbox, NULL);
	/*ComboBox_AddEntry(subbox, "Play Now");*/
	ComboBox_AddEntry(subbox, "Search");
	/*ComboBox_AddEntry(subbox, "Library");*/
	ComboBox_AddEntry(subbox, NULL);
	ComboBox_AddEntry(subbox, _("My Game Ads"));
	ComboBox_SetSelection(subbox, _("Home"));
	ComboBox_SetOnSelection(subbox, GamesListPageCombo_OnSelection);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(toppanel, subbox);
	data->combobox = subbox;

	subbox = Box_Create(5, 44, 90, 2, BOX_VISIBLE | BOX_TILEIMAGE);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->img = ImageMgr_GetImage("Horizrule.png");
	Box_AddChild(toppanel, subbox);

	subbox = Box_Create(0, 49, 5, 100-49-30, BOX_VISIBLE);
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	subbox->bgcol = TabBG2;
	Box_AddChild(gameslist_box, subbox);

	subbox = Box_Create(95, 49, 5, 100-49-30, BOX_VISIBLE);
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	subbox->bgcol = TabBG2;
	Box_AddChild(gameslist_box, subbox);

	subbox = Box_Create(5, 49, 90, 0, BOX_VISIBLE);
	subbox->bgcol = TabBG2;
	subbox->fgcol = TabFG2;
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(gameslist_box, subbox);
	data->toplabel = subbox;

	subbox = List_Create(5, 49, 90, 100-49-30, BOX_VISIBLE, FALSE);
	subbox->bgcol = TabBG2;
	subbox->fgcol = TabFG2;
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	List_SetEntrySelectable(subbox, FALSE);
	Box_AddChild(gameslist_box, subbox);
	data->list = subbox;
	
	subbox = Text_Create(0, 100-16, 100, 16, BOX_VISIBLE, TX_CENTERED);
	subbox->bgcol = TabBG2;
	subbox->fgcol = TabFG2;
	subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_AddChild(gameslist_box, subbox);
	data->bottomlabel = subbox;

	gameslist_box->boxdata = data;
	gameslist_box->OnDestroy = GamesList_OnDestroy;

	{
		struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
		memset(pageentry, 0, sizeof(*pageentry));

		pageentry->type = GLP_HOME;

		GamesList_SetPageAndHistory(gameslist_box, pageentry);
	}

	return gameslist_box;
}

void SearchResultDeletePopup_OnRemove(struct Box_s *dialog, struct searchresultdata_s *data)
{
	struct Box_s *gameslist_box = data->gameslist_box;
	struct gameslistdata_s *gldata = gameslist_box->boxdata;

	Ctrl_RemoveGameAd(data->itemid);
	List_RemoveEntryByName(gldata->list, data->itemid, NULL);
	List_RedoEntries(gldata->list);
	Box_Repaint(gldata->list);
	Box_Destroy(Box_GetRoot(dialog));
}

void SearchResultDelete_OnHit(struct Box_s *button)
{
	struct Box_s *entry = button->parent;
	struct searchresultdata_s *data = entry->boxdata;
	struct Box_s *gameslist_box = data->gameslist_box;
	struct gameslistdata_s *gldata = gameslist_box->boxdata;
	struct gamesearchinfo_s *info = data->info;
	char *timecontrolrange;
	char txt[256], txt2[512];

	if (!info->timecontrolrange)
	{
		timecontrolrange = Info_TimeControlToCategory(info->timecontrol);
	}
	else
	{
		timecontrolrange = Util_Capitalize(info->timecontrolrange);
	}

	txt[0] = '\0';

	strcat(txt, info->rated ? _("Rated ") : _("Unrated "));

	if (info->correspondence)
	{
		strcat(txt, _("correspondence"));
	}
	else if (stricmp(info->variant, "standard") == 0)
	{
		strcat(txt, timecontrolrange);
	}
	else
	{
		strcat(txt, info->variant);
		strcat(txt, " ");
		strcat(txt, timecontrolrange);
	}

	{
		char removetxt[512];

		i18n_stringsub(removetxt, 512, _("Are you sure that you want to permanently remove your Game Ad for %1?"), txt);
		sprintf(txt2, "%s\n%s", removetxt, _("You can always re-create this Game Ad from the plus menu in the games list."));
	}

	AutoDialog_Create(Box_GetRoot(button), 500, _("Remove Game Ad?"), txt2, _("Don't Remove"), _("Remove"), NULL, SearchResultDeletePopup_OnRemove, data);
/*
	Ctrl_RemoveGameAd(data->itemid);
	List_RemoveEntryByName(gldata->list, data->itemid, NULL);
	*/
}

void SearchResult_OnLButtonDblClk(struct Box_s *entry, int x, int y)
{
	struct searchresultdata_s *data = entry->boxdata;
	struct Box_s *gameslist_box = data->gameslist_box;
	struct gameslistdata_s *gldata = gameslist_box->boxdata;

	if (strcmp(data->node, "watch") == 0)
	{
		Ctrl_WatchGame(data->jid, 0);
	}
	else if (strcmp(data->node, "play") == 0)
	{
		View_PopupGameRespondAdDialog(data->jid, data->info);
	}
	else if (strcmp(data->node, "myads") == 0)
	{
		/*
		Ctrl_RemoveGameAd(data->itemid);
		List_RemoveEntryByName(gldata->list, data->itemid, NULL);
		*/
	}
	else if (strcmp(data->node, "adjourned") == 0)
	{
		Model_PopupReconvene(data->itemid, data->info);
	}
	else if (strcmp(data->node, "correspondence") == 0)
	{
		View_PopupChessGame(data->itemid, NULL, NULL);
		Ctrl_GetCorGameState(data->itemid);
	}
	else if (strcmp(data->node, "tournament") == 0)
	{
		Ctrl_ViewTournament(data->itemid);
	}
}

void EditAd_OnClick(struct Box_s *link, void *userdata)
{
	struct Box_s *entry = link->parent;
	struct searchresultdata_s *data = entry->boxdata;
	struct Box_s *gameslist_box = data->gameslist_box;
	struct gameslistdata_s *gldata = gameslist_box->boxdata;

	View_PopupGameCreateAdDialog(data->info);
}

void GamesList_AddSearchTournament(struct Box_s *gameslist_box, struct tournamentinfo_s *info)
{
	struct Box_s *entrybox, *subbox;
	struct gameslistdata_s *data = gameslist_box->boxdata;
	struct searchresultdata_s *entrydata;
	char txt[256];
	char *timecontrolrange = _("No Time Limit");
	struct gameslistpageentry_s *pageentry = data->pagehistory->data;
	
	Box_RemoveTimedFunc(gameslist_box, GamesList_RefreshTimeout, 20000);

	if (pageentry->type != GLP_SEARCHRESULTS)
	{
		return;
	}

	entrydata = malloc(sizeof(*entrydata));
	memset(entrydata, 0, sizeof(*entrydata));

	entrydata->gameslist_box = gameslist_box;
	entrydata->itemid = strdup(info->id);
	entrydata->node = strdup("tournament");
	entrydata->tournamentinfo = Info_DupeTournamentInfo(info);

	entrybox = Box_Create(0, 0, 100, 70, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->fgcol = TabFG1;
	entrybox->OnLButtonDblClk = SearchResult_OnLButtonDblClk;
	entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	entrybox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	entrybox->boxdata = entrydata;

	subbox = Box_Create(5, 5, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = TabFG1;
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	i18n_stringsub(txt, 256, _("Tournament: %1"), info->name);
	Box_SetText(subbox, txt);
	Box_AddChild(entrybox, subbox);

	subbox = Box_Create(5, 25, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = RGB(0, 0, 0);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	i18n_stringsub(txt, 256, _("%1, %2, Manager: %3"), info->timecontrol, info->variant, Model_GetFriendNick(info->manager));
	Box_SetText(subbox, txt);
	Box_AddChild(entrybox, subbox);

	List_AddEntry(data->list, info->id, NULL, entrybox);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

void GamesList_ClearSearch(struct Box_s *gameslist_box)
{
	struct gameslistdata_s *data = gameslist_box->boxdata;
	struct gameslistpageentry_s *currentpage = data->pagehistory->data;

	if (currentpage->type != GLP_SEARCHRESULTS)
	{
		return;
	}

	List_RemoveAllEntries(data->list);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

void GamesList_AddSearchGame(struct Box_s *gameslist_box, char *itemid, char *node, char *jid, struct gamesearchinfo_s *info)
{
	struct Box_s *entrybox, *subbox;
	struct gameslistdata_s *data = gameslist_box->boxdata;
	struct searchresultdata_s *entrydata;
	char txt[512];
	char *timecontrolrange = _("No Time Limit"), *variant = _("Standard");
	struct gameslistpageentry_s *pageentry = data->pagehistory->data;

	Box_RemoveTimedFunc(gameslist_box, GamesList_RefreshTimeout, 20000);

	if (node && strcmp(node, "quickplay") == 0)
	{
		View_PopupGameRespondAdDialog(info->adplayer->jid, info);
		return;
	}
	
	if (pageentry->type != GLP_SEARCHRESULTS)
	{
		return;
	}

	entrydata = malloc(sizeof(*entrydata));
	memset(entrydata, 0, sizeof(*entrydata));

	entrydata->gameslist_box = gameslist_box;
	entrydata->itemid = strdup(itemid);
	entrydata->node = strdup(node);
	entrydata->jid = strdup(jid);
	entrydata->info = Info_DupeGameSearchInfo(info);

	if (stricmp(info->variant, "standard") == 0)
	{
		variant = _("Standard");
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

	if (!info->timecontrolrange || info->timecontrolrange[0] == '\0')
	{
		timecontrolrange = Info_TimeControlToCategory(info->timecontrol);
	}
	else
	{
		if (stricmp(info->timecontrolrange, "Long") == 0)
		{
			timecontrolrange = _("Long");
		}
		else if (stricmp(info->timecontrolrange, "Speed") == 0)
		{
			timecontrolrange = _("Speed");
		}
		else if (stricmp(info->timecontrolrange, "Blitz") == 0)
		{
			timecontrolrange = _("Blitz");
		}
		else if (stricmp(info->timecontrolrange, "Bullet") == 0)
		{
			timecontrolrange = _("Bullet");
		}
		else
		{
			timecontrolrange = _(info->timecontrolrange);
		}
	}

	txt[0] = '\0';

	if (!info->offline)
	{
		strcat(txt, "^l");
	}
	else
	{
		strcat(txt, "^4");
	}

	strcat(txt, info->rated ? _("Rated") : _("Unrated"));

	strcat(txt, " ");

	if (info->correspondence)
	{
		strcat(txt, _("Correspondence"));
	}
	else if (stricmp(info->variant, "standard") == 0)
	{
		strcat(txt, timecontrolrange);
	}
	else
	{
		strcat(txt, variant);
		strcat(txt, " ");
		strcat(txt, timecontrolrange);
	}
/*
	if (!info->offline)
	{
		strcat(txt, "^l");
	}
*/
	strcat(txt, "\n");
#if 0
	/*
	if (!info->offline && ((info->lastmove && strlen(info->lastmove) > 0) || info->movenum))
	{
		strcat(txt, "^l");
	}
*/
	if (info->lastmove && strlen(info->lastmove) > 0)
	{
		char shortlastmove[255];
		char movetxt[255];
		char *p;
		strcpy(shortlastmove, info->lastmove);

		p = strchr(shortlastmove, ' ');
		if (p)
		{
			*p = '\0';
		}

		i18n_stringsub(movetxt, 255, _("Last move %1"), shortlastmove);

		strcat(txt, movetxt);
	}

	if (info->movenum)
	{
		char num[64];
		char movetxt[255];
		sprintf(num, "%d", info->movenum);

		if (info->lastmove && strlen(info->lastmove) > 0)
		{
			strcat(txt, ", ");
		}

		i18n_stringsub(movetxt, 255, "Move: %1", num);
		strcat(txt, movetxt);

		if (info->whitetomove)
		{
			strcat(txt, " (");
			strcat(txt, _("White"));
			strcat(txt, ")");
		}
		else if (info->blacktomove)
		{
			strcat(txt, " (");
			strcat(txt, _("Black"));
			strcat(txt, ")");
		}
	}

/*	if (!info->offline && ((info->lastmove && strlen(info->lastmove) > 0) || info->movenum))
	{
		strcat(txt, "^l");
	}
*/
	if ((info->lastmove && strlen(info->lastmove) > 0) || info->movenum)
	{
		strcat(txt, "\n");
	}
#endif
/*
	if (!info->offline)
	{
		strcat(txt, "^l");
	}
*/
	if (info->correspondence && info->timecontrol)
	{
		char txt2[256];
		char movestxt[80];
		char daystxt[80];
		
		sprintf(movestxt, "%d", info->timecontrol->controlarray[1]);
		sprintf(daystxt, "%d", info->timecontrol->controlarray[2] / (60 * 60 * 24));

		i18n_stringsub(txt2, 256, _("%1 moves within %2 days"), movestxt, daystxt);
		strcat(txt, txt2);
	}
	else
	{
		strcat(txt, _("Est. Game Time: "));
		strcat(txt, Info_SecsToText1((float)(Info_TimeControlToEstimatedTime(info->timecontrol))));
	}

	if (!info->offline)
	{
		strcat(txt, "^l");
	}

	strcat(txt, "\n");

	if (info->comment && strlen(info->comment) > 0)
	{
		strcat(txt, info->comment);
	}

	if (strcmp(node, "watch") == 0)
	{
		entrybox = Box_Create(0, 0, 100, 90, BOX_VISIBLE | BOX_TRANSPARENT);
		entrybox->fgcol = TabFG1;
		entrybox->OnLButtonDblClk = SearchResult_OnLButtonDblClk;
		entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		entrybox->OnSizeHeight = Box_OnSizeHeight_Stretch;
		entrybox->boxdata = entrydata;

		subbox = Text_Create(5, 5, 90, 80, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
		subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;

		strcat(txt, "\n^E\n^E^E");

		{
			struct namedlist_s *entry = info->white->titles;

			if (entry)
			{
				strcat(txt, entry->name);
				strcat(txt, " ");
				entry = entry->next;
			}

			strcat(txt, Model_GetFriendNick(info->white->jid));
			strcat(txt, "  ");

			strcat(txt, info->white->rating ? info->white->rating : _("Unrated"));
		}
		
		strcat(txt, " vs. ");

		{
			struct namedlist_s *entry = info->black->titles;

			if (entry)
			{
				strcat(txt, entry->name);
				strcat(txt, " ");
				entry = entry->next;
			}

			strcat(txt, Model_GetFriendNick(info->black->jid));
			strcat(txt, "  ");

			strcat(txt, info->black->rating ? info->black->rating : _("Unrated"));
		}

		Box_AddChild(entrybox, subbox);
		Text_SetText(subbox, txt);
		Text_SetLinkCallback(subbox, 1, ViewLink_WatchGame_OnClick, strdup(info->gameid));
		Text_SetLinkCallback(subbox, 2, ViewLink_WatchGame_OnClick, strdup(info->gameid));
		Text_SetLinkCallback(subbox, 3, ViewLink_WatchGame_OnClick, strdup(info->gameid));

		{
			char *avatarhash;
			struct BoxImage_s *img = NULL;

			subbox = Box_Create(entrybox->w - 35, 5, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;

			avatarhash = Model_GetFriendAvatarHash(info->white->jid);

			if (avatarhash)
			{
				char filename[MAX_PATH];
				img = ImageMgr_GetRootImage(Ctrl_GetAvatarFilenameByHash(avatarhash, filename, MAX_PATH));
			}

			if (!img)
			{
	                        img = ImageMgr_GetScaledImage("DefaultAvatar30x30", "DefaultAvatar.png", 30, 30);
			}

			subbox->img = img;

			Box_AddChild(entrybox, subbox);
		}
		{
			char *avatarhash;
			struct BoxImage_s *img = NULL;

			subbox = Box_Create(entrybox->w - 70, 5, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;

			avatarhash = Model_GetFriendAvatarHash(info->black->jid);

			if (avatarhash)
			{
				char filename[MAX_PATH];
				img = ImageMgr_GetRootImage(Ctrl_GetAvatarFilenameByHash(avatarhash, filename, MAX_PATH));
			}

			if (!img)
			{
	                        img = ImageMgr_GetScaledImage("DefaultAvatar30x30", "DefaultAvatar.png", 30, 30);
			}

			subbox->img = img;

			Box_AddChild(entrybox, subbox);
		}
	}
	else if (strcmp(node, "play") == 0)
	{
		entrybox = Box_Create(0, 0, 100, 10, BOX_VISIBLE | BOX_TRANSPARENT);
		entrybox->fgcol = TabFG1;
		entrybox->OnLButtonDblClk = SearchResult_OnLButtonDblClk;
		entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		entrybox->OnSizeHeight = Box_OnSizeHeight_Stretch;
		entrybox->boxdata = entrydata;

		subbox = Text_Create(5, 5, 90, 0, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
		subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;

		strcat(txt, "\n^E\n^E^E");

		{
			struct namedlist_s *entry = info->adplayer->titles;

			if (entry)
			{
				strcat(txt, entry->name);
				strcat(txt, " ");
				entry = entry->next;
			}

			strcat(txt, Model_GetFriendNick(info->adplayer->jid));
			strcat(txt, "  ");

			entry = info->adplayer->roles;

			while (entry)
			{
				strcat(txt, entry->name);
				if (entry->next)
				{
					strcat(txt, ", ");
				}
				else
				{
					strcat(txt, "  ");
				}
				entry = entry->next;
			}

			strcat(txt, info->adplayer->rating ? info->adplayer->rating : _("Unrated"));
		}
		
		Box_AddChild(entrybox, subbox);
		Text_SetText(subbox, txt);
		Text_SetLinkCallback(subbox, 1, ViewLink_RespondAd_OnClick, Info_DupeGameSearchInfo(info));
		Text_SetLinkCallback(subbox, 2, ViewLink_RespondAd_OnClick, Info_DupeGameSearchInfo(info));
		Text_SetLinkCallback(subbox, 3, ViewLink_RespondAd_OnClick, Info_DupeGameSearchInfo(info));

		{
			char *avatarhash;
			struct BoxImage_s *img = NULL;

			subbox = Box_Create(entrybox->w - 35, 5, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;

			avatarhash = Model_GetFriendAvatarHash(info->adplayer->jid);

			if (avatarhash)
			{
				char filename[MAX_PATH];
				img = ImageMgr_GetRootImage(Ctrl_GetAvatarFilenameByHash(avatarhash, filename, MAX_PATH));
			}

			if (!img)
			{
	                        img = ImageMgr_GetScaledImage("DefaultAvatar30x30", "DefaultAvatar.png", 30, 30);
			}

			subbox->img = img;

			Box_AddChild(entrybox, subbox);
		}
	}
	else if (strcmp(node, "myads") == 0)
	{
		entrybox = Box_Create(0, 0, 100, 10, BOX_VISIBLE | BOX_TRANSPARENT);
		entrybox->fgcol = TabFG1;
		entrybox->OnLButtonDblClk = SearchResult_OnLButtonDblClk;
		entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		entrybox->OnSizeHeight = Box_OnSizeHeight_Stretch;
		entrybox->boxdata = entrydata;

		subbox = Text_Create(5, 5, 80, 0, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
		subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;

		if (stricmp(info->variant, "standard") == 0)
		{
			sprintf(txt, "^b%s %s^n\n", info-> rated ? _("Rated") : _("Unrated"), timecontrolrange);
		}
		else
		{
			sprintf(txt, "^b%s %s %s^n\n", info-> rated ? _("Rated") : _("Unrated"), info->variant, timecontrolrange);
		}

		if (info->limit)
		{
			char txt2[512];
			if (stricmp(info->limit->type, "rating") == 0)
			{
				if ((info->limit->low != -1) && (info->limit->high == -1))
				{
					strcpy(txt2, txt);
					sprintf(txt, _("%sRated over %d\n"), txt2, info->limit->low);
				}
				else if ((info->limit->low == -1) && (info->limit->high != -1))
				{
					strcpy(txt2, txt);
					sprintf(txt, _("%sRated under %d\n"), txt2, info->limit->high);
				}
				else if ((info->limit->low != -1) && (info->limit->high != -1))
				{
					strcpy(txt2, txt);
					sprintf(txt, _("%sRated between %d and %d\n"), txt2, info->limit->low, info->limit->high);
				}
				else
				{
					strcat(txt, _("Any Opponent\n"));
				}
			}
			else
			{
				strcat(txt, _("Any Opponent\n"));
			}
		}
		else
		{
			strcat(txt, _("Any Opponent\n"));
		}

		strcat(txt, _("^lEdit this game ad...^l"));

		Log_Write(0, "Written %s\n", txt);

		Box_AddChild(entrybox, subbox);
		Text_SetText(subbox, txt);
		Text_SetLinkCallback(subbox, 1, EditAd_OnClick, NULL);

		subbox = Button_Create(entrybox->w - 15, (entrybox->h - 13) / 2, 13, 13, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Button_SetNormalImg(subbox, ImageMgr_GetSubImage("DeleteIcon1", "DeleteIcon.png", 0, 0, 13, 13));
		Button_SetHoverImg(subbox, ImageMgr_GetSubImage("DeleteIcon2", "DeleteIcon.png", 13, 0, 13, 13));
		Button_SetPressedImg(subbox, ImageMgr_GetSubImage("DeleteIcon2", "DeleteIcon.png", 13, 0, 13, 13));
		Button_SetOnButtonHit(subbox, SearchResultDelete_OnHit);
		Button_SetTooltipText(subbox, _("Clear"));
		Box_AddChild(entrybox, subbox);
	}
	else if (strcmp(node, "correspondence") == 0)
	{
		entrybox = Box_Create(0, 0, 100, 10, BOX_VISIBLE | BOX_TRANSPARENT);
		entrybox->fgcol = TabFG1;
		entrybox->OnLButtonDblClk = SearchResult_OnLButtonDblClk;
		entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		entrybox->OnSizeHeight = Box_OnSizeHeight_Stretch;
		entrybox->boxdata = entrydata;

		subbox = Text_Create(5, 5, 90, 0, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
		subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;

		strcat(txt, "\n^E\n^E^E");

		{
			struct namedlist_s *entry = info->white->titles;

			if (entry)
			{
				strcat(txt, entry->name);
				strcat(txt, " ");
				entry = entry->next;
			}

			strcat(txt, Model_GetFriendNick(info->white->jid));
			strcat(txt, "  ");

			strcat(txt, info->white->rating ? info->white->rating : _("Unrated"));
		}
		
		strcat(txt, " vs. ");

		{
			struct namedlist_s *entry = info->black->titles;

			if (entry)
			{
				strcat(txt, entry->name);
				strcat(txt, " ");
				entry = entry->next;
			}

			strcat(txt, Model_GetFriendNick(info->black->jid));
			strcat(txt, "  ");

			strcat(txt, info->black->rating ? info->black->rating : _("Unrated"));
		}

		Box_AddChild(entrybox, subbox);
		Text_SetText(subbox, txt);
		Text_SetLinkCallback(subbox, 1, ViewLink_CorGameOpen_OnClick, strdup(info->gameid));
		Text_SetLinkCallback(subbox, 2, ViewLink_CorGameOpen_OnClick, strdup(info->gameid));
		Text_SetLinkCallback(subbox, 3, ViewLink_CorGameOpen_OnClick, strdup(info->gameid));

		{
			char *avatarhash;
			struct BoxImage_s *img = NULL;

			subbox = Box_Create(entrybox->w - 35, 5, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;

			avatarhash = Model_GetFriendAvatarHash(info->white->jid);

			if (avatarhash)
			{
				char filename[MAX_PATH];
				img = ImageMgr_GetRootImage(Ctrl_GetAvatarFilenameByHash(avatarhash, filename, MAX_PATH));
			}

			if (!img)
			{
	                        img = ImageMgr_GetSubImage("DefaultAvatar1", "DefaultAvatar.png", 0, 0, 50, 50);
			}

			subbox->img = img;

			Box_AddChild(entrybox, subbox);
		}
		{
			char *avatarhash;
			struct BoxImage_s *img = NULL;

			subbox = Box_Create(entrybox->w - 70, 5, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;

			avatarhash = Model_GetFriendAvatarHash(info->black->jid);

			if (avatarhash)
			{
				char filename[MAX_PATH];
				img = ImageMgr_GetRootImage(Ctrl_GetAvatarFilenameByHash(avatarhash, filename, MAX_PATH));
			}

			if (!img)
			{
	                        img = ImageMgr_GetSubImage("DefaultAvatar1", "DefaultAvatar.png", 0, 0, 50, 50);
			}

			subbox->img = img;

			Box_AddChild(entrybox, subbox);
		}
	}
	else if (strcmp(node, "adjourned") == 0)
	{
		entrybox = Box_Create(0, 0, 100, 10, BOX_VISIBLE | BOX_TRANSPARENT);
		entrybox->fgcol = TabFG1;
		entrybox->OnLButtonDblClk = SearchResult_OnLButtonDblClk;
		entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		entrybox->OnSizeHeight = Box_OnSizeHeight_Stretch;
		entrybox->boxdata = entrydata;

		subbox = Text_Create(5, 5, 90, 0, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT);
		subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;

		strcat(txt, "\n^E\n^E^E");

		{
			struct namedlist_s *entry = info->white->titles;

			if (entry)
			{
				strcat(txt, entry->name);
				strcat(txt, " ");
				entry = entry->next;
			}

			strcat(txt, Model_GetFriendNick(info->white->jid));
			strcat(txt, "  ");

			strcat(txt, info->white->rating ? info->white->rating : _("Unrated"));
		}
		
		strcat(txt, _(" vs. "));

		{
			struct namedlist_s *entry = info->black->titles;

			if (entry)
			{
				strcat(txt, entry->name);
				strcat(txt, " ");
				entry = entry->next;
			}

			strcat(txt, Model_GetFriendNick(info->black->jid));
			strcat(txt, "  ");

			strcat(txt, info->black->rating ? info->black->rating : _("Unrated"));
		}

		Box_AddChild(entrybox, subbox);
		Text_SetText(subbox, txt);
		Text_SetLinkCallback(subbox, 1, ViewLink_GameOpen_OnClick, Info_DupeGameSearchInfo(info));
		Text_SetLinkCallback(subbox, 2, ViewLink_GameOpen_OnClick, Info_DupeGameSearchInfo(info));
		Text_SetLinkCallback(subbox, 3, ViewLink_GameOpen_OnClick, Info_DupeGameSearchInfo(info));

		{
			char *avatarhash;
			struct BoxImage_s *img = NULL;

			subbox = Box_Create(entrybox->w - 35, 5, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;

			avatarhash = Model_GetFriendAvatarHash(info->white->jid);

			if (avatarhash)
			{
				char filename[MAX_PATH];
				img = ImageMgr_GetRootImage(Ctrl_GetAvatarFilenameByHash(avatarhash, filename, MAX_PATH));
			}

			if (!img)
			{
	                        img = ImageMgr_GetSubImage("DefaultAvatar1", "DefaultAvatar.png", 0, 0, 50, 50);
			}

			subbox->img = img;

			Box_AddChild(entrybox, subbox);
		}
		{
			char *avatarhash;
			struct BoxImage_s *img = NULL;

			subbox = Box_Create(entrybox->w - 70, 5, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;

			avatarhash = Model_GetFriendAvatarHash(info->black->jid);

			if (avatarhash)
			{
				char filename[MAX_PATH];
				img = ImageMgr_GetRootImage(Ctrl_GetAvatarFilenameByHash(avatarhash, filename, MAX_PATH));
			}

			if (!img)
			{
	                        img = ImageMgr_GetSubImage("DefaultAvatar1", "DefaultAvatar.png", 0, 0, 50, 50);
			}

			subbox->img = img;

			Box_AddChild(entrybox, subbox);
		}
	}

	List_AddEntry(data->list, itemid, NULL, entrybox);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

void GamesList_PopupAddGameAdDialog(struct Box_s *pbox, void *userdata)
{
	View_PopupGameCreateAdDialog(NULL);
}

void GamesList_ReSearch(struct Box_s *pbox, void *userdata)
{
	struct Box_s *gameslist_box = View_GetGamesListBox();
	struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
	memset(pageentry, 0, sizeof(*pageentry));

	pageentry->type = GLP_SEARCH;

	GamesList_SetPageAndHistory(gameslist_box, pageentry);
}

void GamesList_FinishGameResults(struct Box_s *gameslist_box, int noresults)
{
	struct Box_s *entrybox, *subbox;
	struct gameslistdata_s *data = gameslist_box->boxdata;
	struct gameslistpageentry_s *pageentry = data->pagehistory->data;
	
	Box_AddTimedFunc(gameslist_box, GamesList_RefreshTimeout, NULL, 20000);

	if (pageentry->type != GLP_SEARCHRESULTS)
	{
		return;
	}

	if (noresults)
	{
		entrybox = Box_Create(0, 0, 100, 70, BOX_VISIBLE | BOX_TRANSPARENT);
		entrybox->fgcol = TabFG1;
		entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		entrybox->OnSizeHeight = Box_OnSizeHeight_Stretch;

		subbox = Text_Create(10, 5, 80, 40, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
		subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;

		if (pageentry->node && strcmp(pageentry->node, "myads") == 0)
		{
			Text_SetText(subbox, _("You currently have no game ads.\n^lPlace a game ad...^l"));
			Text_SetLinkCallback(subbox, 1, GamesList_PopupAddGameAdDialog, NULL);
		}
		else if (pageentry->node && strcmp(pageentry->node, "correspondence") == 0)
		{
			Text_SetText(subbox, _("You currently have no correspondence games.\n^lSearch for a game?^l"));
			Text_SetLinkCallback(subbox, 1, GamesList_ReSearch, NULL);
		}
		else if (pageentry->node && strcmp(pageentry->node, "adjourned") == 0)
		{
			Text_SetText(subbox, _("You currently have no adjourned games.\n^lSearch for a game?^l"));
			Text_SetLinkCallback(subbox, 1, GamesList_ReSearch, NULL);
		}
		else
		{
			Text_SetText(subbox, _("No results found based on your game search.\n^lSearch again?^l"));
			Text_SetLinkCallback(subbox, 1, GamesList_ReSearch, NULL);
		}

		Box_AddChild(entrybox, subbox);

		List_AddEntry(data->list, NULL, NULL, entrybox);
		List_RedoEntries(data->list);
    		Box_Repaint(data->list);
	}

	if (data->refreshtext)
	{
		data->lastupdate = 0;
		Box_AddTimedFunc(gameslist_box, GamesList_UpdateLastUpdatedTime, NULL, 1000);
	}
}

void GamesList_SetTotalGamesCount(struct Box_s *gameslist_box, int count)
{
	struct gameslistdata_s *data = gameslist_box->boxdata;
	struct gameslistpageentry_s *currentpage = data->pagehistory->data;
	
	data->totalgamescount = count;

	if (currentpage->type == GLP_HOME)
	{
		/*
		sprintf(txt, _("^4Currently %d games being played."), data->totalgamescount);
		Text_SetText(data->bottomlabel, txt);
		*/
	}
}

void GamesList_RefreshPage(struct Box_s *gameslist_box)
{
	struct gameslistdata_s *data = gameslist_box->boxdata;
	struct gameslistpageentry_s *currentpage = data->pagehistory->data;

	GamesList_SetPage(gameslist_box, currentpage);
}

void GamesList_ExternalSearch(struct Box_s *gameslist_box, char *node)
{
	struct gameslistpageentry_s *pageentry = malloc(sizeof(*pageentry));
	memset(pageentry, 0, sizeof(*pageentry));

	pageentry->type = GLP_SEARCHRESULTS;
	pageentry->node = strdup(node);

	GamesList_SetPageAndHistory(gameslist_box, pageentry);
}