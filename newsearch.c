#include <stdio.h>

#include "box.h"

#include "constants.h"

#include "sizer.h"
#include "text.h"

#include "i18n.h"

#include "boxtypes.h"

#include "autodialog.h"
#include "button.h"
#include "button2.h"
#include "checkbox.h"
#include "combobox.h"
#include "ctrl.h"
#include "imagemgr.h"
#include "info.h"
#include "list.h"
#include "log.h"
#include "menu.h"
#include "model.h"
#include "namedlist.h"
#include "options.h"
#include "probreport.h"
#include "stdbutton.h"
#include "tabs.h"
#include "titlebar.h"
#include "view.h"
#include "autosize.h"

#include "conn.h"

#include "util.h"

#include "newsearch.h"

#define NG_PLAY           1
#define NG_WATCH          2
#define NG_ADJOURNED      3
#define NG_CORRESPONDENCE 4

extern HFONT tahoma24b_f;

struct newgamesgamedata_s
{
	struct gamesearchinfo_s *info;
	char *gametype;
	int estgametime;
	char *showname;
	int showrating;
	char *showname2;
	int showrating2;
	unsigned int time;
	char *limittype;
	int side;
	char *id;
};

struct newgamescategorydata_s
{
	int x;
	int w;
	char *name;
	struct Box_s *box;
	int stretchw;
};

struct newgamesdata_s
{
	char dummy;
	struct Box_s *list;
	int lastcategoryx;
	struct namedlist_s *playcategories;
	int clicked;
	struct Box_s *listmecheckbox;
	int hasad;
	struct Box_s *pagetitle;
	struct Box_s *otherpagelinks;
	/*struct Box_s *dontshowagaincheck;*/
	struct Box_s *listtitles;
	char *currentpagetype;
	struct Box_s *refreshbutton;
	struct Box_s *newgamebutton;
	/*struct Box_s *managetext;*/
	struct Box_s *filtertext;
	struct Box_s *filtersbox;
	struct Box_s *quicklinks;
	struct Box_s *sizeablecontent;
	/*
	int playfiltersvisible;
	int watchfiltersvisible;
	int adjournedfiltersvisible;
	*/

	struct Box_s *ratedgamecheck;
	struct Box_s *aicheck;
	struct Box_s *quickfiltercheck;
	struct Box_s *quickfilterchecktext;
	struct Box_s *variantcombo;
	struct Box_s *timecombo;
	struct Box_s *ratingcombo;
	struct Box_s *myadscombo;
	struct Box_s *myadstext;
	struct Box_s *groupscombo;
	struct Box_s *groupstext;
	struct Box_s *ratedplayerscheck;

	struct gamesearchinfo_s *playsearchinfo;
	struct gamesearchinfo_s *watchsearchinfo;
	struct gamesearchinfo_s *adjournedsearchinfo;
	struct gamesearchinfo_s *myadssearchinfo;
	struct gamesearchinfo_s *searchinfo;
	int numgameads;

	struct namedlist_s *openads;

	struct Box_s *innerpage;
	int showtrialwarning;
};

void NewGamesLinkCallback_OnClick(struct Box_s *tabctrl, char *name);
void NewGamesFilterLinkCallback_OnClick(struct Box_s *pbox, void *userdata);

#if 0
void NewGames_UpdateManageText(struct Box_s *dialog)
{
	struct newgamesdata_s *data = dialog->boxdata;
	char txt1[512];
	char txt2[512];
	char txt3[1024];

	if (data->numgameads)
	{
		char numtext[10];

		sprintf(numtext, "%d", data->numgameads);
		if (data->numgameads == 1)
		{
                        i18n_stringsub(txt1, 512, _("You are looking for 1 game."), numtext);
		}
		else
		{
                        i18n_stringsub(txt1, 512, _("You are looking for %1 games."), numtext);
		}
	}
	else
	{
		i18n_stringsub(txt1, 512, _("You are not currently looking for a game."));
	}
/*
	if (data->currentpagetype && stricmp(data->currentpagetype, "myads") == 0)
	{
                i18n_stringsub(txt2, 512, _("Back to Games To Play"));
	}
	else
	{
                i18n_stringsub(txt2, 512, _("Manage Your Games"));
	}

	sprintf(txt3, "%s\n> ^l%s^l", txt1, txt2);
*/
	sprintf(txt3, "%s\nv ^lFiltering Options^l", txt1);
	Text_SetText(data->managetext, txt3);
/*
	if (data->currentpagetype && stricmp(data->currentpagetype, "myads") == 0)
	{
		Text_SetLinkCallback(data->managetext, 1, NewGamesLinkCallback_OnClick, "play");
	}
	else
	{
		Text_SetLinkCallback(data->managetext, 1, NewGamesLinkCallback_OnClick, "myads");
	}
*/
	Text_SetLinkCallback(data->managetext, 1, NewGamesFilterLinkCallback_OnClick, NULL);

	Box_Repaint(data->managetext);
}
#endif

void NewSearchListTitles_OnSizeWidth_Stretch(struct Box_s *titles, int dw)
{
	struct Box_s *child = titles->child;
	struct Box_s *bigsmall;
	struct newgamescategorydata_s *cdata;
	int num = 0, dwper, lastw;

	if (dw == 0)
	{
		return;
	}

	titles->w += dw;

	/* count children we need to resize */
	child = titles->child;
	while(child)
	{
		cdata = child->boxdata;
		if (cdata->stretchw)
		{
			num++;
		}
		
		child = child->sibling;
	}

	if (num == 0)
	{
		return;
	}

	/* change them by the same amount */
	dwper = dw / num;
	child = titles->child;
	while(child)
	{
		cdata = child->boxdata;
		if (cdata->stretchw)
		{
			struct Box_s *child2 = child->sibling;
			Box_OnSizeWidth_Stretch(child, dwper);
			/*child->w += dwper;*/

			while (child2)
			{
				child2->x += dwper;
				child2 = child2->sibling;
			}
		}
		child = child->sibling;
	}

	/* increase the smallest/decrease the biggest by any remainder */
	dwper = dw - (dwper * num);
	if (dwper)
	{
		if (dw > 0)
		{
			lastw = titles->w;
			child = titles->child;
			while (child)
			{
				cdata = child->boxdata;	
				if (cdata->stretchw && child->w < lastw)
				{
					bigsmall = child;
					lastw = child->w;
				}
				child = child->sibling;
			}
		}
		else
		{
			lastw = 0;
			child = titles->child;
			while (child)
			{
				cdata = child->boxdata;	
				if (cdata->stretchw && child->w > lastw)
				{
					bigsmall = child;
					lastw = child->w;
				}
				child = child->sibling;
			}
		}

		if (bigsmall)
		{
			struct Box_s *child2 = bigsmall->sibling;
			Box_OnSizeWidth_Stretch(bigsmall, dwper);
			/*bigsmall->w += dwper;*/

			while (child2)
			{
				child2->x += dwper;
				child2 = child2->sibling;
			}
		}
	}

	/* align stored x values */
	child = titles->child;
	while(child)
	{
		cdata = child->boxdata;
		cdata->x = child->x;

		child = child->sibling;
	}
}

void NewGamesListEntry_OnSizeWidth_Stretch(struct Box_s *pbox, int dw)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct newgamesdata_s *data = dialog->boxdata;
	struct Box_s *srcchild, *dstchild;

	srcchild = data->listtitles->child;
	dstchild = pbox->child;

	while (srcchild && dstchild)
	{
		struct newgamescategorydata_s *cdata = srcchild->boxdata;

		if (cdata->name == NULL)
		{
			dstchild->x = srcchild->x;
		}
		else
		{
			dstchild->x = srcchild->x + 8;
		}
		if (dstchild->OnSizeWidth)
		{
			dstchild->OnSizeWidth(dstchild, srcchild->w - 15 - dstchild->w);
		}
		else
		{
			Box_OnSizeWidth_Stretch(dstchild, srcchild->w - 15 - dstchild->w);
		}

		srcchild = srcchild->sibling;
		dstchild = dstchild->sibling;
	}
}
/*
int NewGamesListEntry_VisibleFunc(struct Box_s *entrybox)
{
	struct Box_s *dialog = Box_GetRoot(entrybox);
	struct newgamesdata_s *data = dialog->boxdata;
	struct newgamesgamedata_s *gdata = entrybox->boxdata;

	if (!data->aicheck && gdata->info->computers)
	{
		return 0;
	}

	if (data->ratedgamecheck && !gdata->info->rated)
	{
		return 0;
	}

	if (data->variantfilter)
	{
		if (!gdata->info->variant)
		{
			return 0;
		}

		if (stricmp(data->variantfilter, gdata->info->variant) != 0)
		{
			return 0;
		}
	}

	if (data->lowtimefilter && gdata->estgametime < data->lowtimefilter)
	{
		return 0;
	}

	if (data->hightimefilter && gdata->estgametime > hightimefilter)
	{
		return 0;
	}

	return 1;
}
*/
int NewGamesList_SortByOpponentOrWhite(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;
	int compare;

	if (!ldata->showname || !gdata->showname)
	{
		return ldata < gdata;
	}

	compare = stricmp(ldata->showname, gdata->showname);

	if (compare == 0)
	{
		return ldata < gdata;
	}

	return compare < 0;
}

int NewGamesList_SortByOpponentOrWhiteRating(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	if (ldata->showrating == gdata->showrating)
	{
		return NewGamesList_SortByOpponentOrWhite(lbox, gbox);
	}

	return ldata->showrating < gdata->showrating;
}

int NewGamesList_SortByBlack(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	return stricmp(ldata->showname2, gdata->showname2) < 0;
}

int NewGamesList_SortByBlackRating(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	if (ldata->showrating2 == gdata->showrating2)
	{
		return NewGamesList_SortByBlack(lbox, gbox);
	}

	return ldata->showrating2 < gdata->showrating2;
}

int NewGamesList_SortByRatedGame(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	if (ldata->info->rated == gdata->info->rated)
	{
		return NewGamesList_SortByOpponentOrWhite(lbox, gbox);
	}

	return gdata->info->rated;
}

int NewGamesList_SortByGameType(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;
	int compare = stricmp(ldata->gametype, gdata->gametype);

	if (compare == 0)
	{
		return NewGamesList_SortByOpponentOrWhite(lbox, gbox);
	}

	return compare < 0;
}

int NewGamesList_SortBySide(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	if (ldata->side == gdata->side)
	{
		return NewGamesList_SortByOpponentOrWhite(lbox, gbox);
	}

	return ldata->side < gdata->side;
}

int NewGamesList_SortByEstGameTime(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	if (ldata->estgametime == gdata->estgametime)
	{
		return NewGamesList_SortByOpponentOrWhite(lbox, gbox);
	}

	return ldata->estgametime < gdata->estgametime;
}

int NewGamesList_SortByOpponentOrWhiteReverse(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;
	int compare = stricmp(ldata->showname, gdata->showname);

	if (compare == 0)
	{
		return ldata < gdata;
	}

	return compare > 0;
}

int NewGamesList_SortByOpponentOrWhiteRatingReverse(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	if (ldata->showrating == gdata->showrating)
	{
		return NewGamesList_SortByOpponentOrWhite(lbox, gbox);
	}

	return ldata->showrating > gdata->showrating;
}

int NewGamesList_SortByBlackReverse(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	return stricmp(ldata->showname2, gdata->showname2) > 0;
}

int NewGamesList_SortByBlackRatingReverse(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	if (ldata->showrating2 == gdata->showrating2)
	{
		return NewGamesList_SortByBlack(lbox, gbox);
	}

	return ldata->showrating2 > gdata->showrating2;
}

int NewGamesList_SortByRatedGameReverse(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	if (ldata->info->rated == gdata->info->rated)
	{
		return NewGamesList_SortByOpponentOrWhite(lbox, gbox);
	}

	return !gdata->info->rated;
}

int NewGamesList_SortByGameTypeReverse(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;
	int compare = stricmp(ldata->gametype, gdata->gametype);

	if (compare == 0)
	{
		return NewGamesList_SortByOpponentOrWhite(lbox, gbox);
	}

	return compare > 0;
}

int NewGamesList_SortBySideReverse(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	if (ldata->side == gdata->side)
	{
		return NewGamesList_SortByOpponentOrWhite(lbox, gbox);
	}

	return ldata->side > gdata->side;
}

int NewGamesList_SortByEstGameTimeReverse(struct Box_s *lbox, struct Box_s *gbox)
{
	struct newgamesgamedata_s *ldata = lbox->boxdata;
	struct newgamesgamedata_s *gdata = gbox->boxdata;

	if (ldata->estgametime == gdata->estgametime)
	{
		return NewGamesList_SortByOpponentOrWhite(lbox, gbox);
	}

	return ldata->estgametime > gdata->estgametime;
}

void NewGamesCategory_OnLButtonDown(struct Box_s *categorybox, int x, int y)
{
	struct newgamescategorydata_s *cdata = categorybox->boxdata;
	struct Box_s *dialog = Box_GetRoot(categorybox);
	struct newgamesdata_s *data = dialog->boxdata;
	void *func = List_GetEntrySortFunc(data->list);
	int forward = 1;
	struct Box_s *sortbox;

	{
		struct namedlist_s *entry = data->playcategories;
		struct newgamescategorydata_s *cdata2;

		while (entry)
		{
			cdata2 = entry->data;
			sortbox = cdata2->box->child->sibling;

			sortbox->flags &= ~BOX_VISIBLE;

			entry = entry->next;
		}
	}

	if (cdata->name == NULL)
	{
		return;
	}

	sortbox = categorybox->child->sibling;	

	if (stricmp(cdata->name, "Opponent Name") == 0 || stricmp(cdata->name, "White") == 0)
	{
		if (func == NewGamesList_SortByOpponentOrWhite)
		{
                        List_SetEntrySortFunc(data->list, NewGamesList_SortByOpponentOrWhiteReverse);
			forward = 0;
		}
		else
		{
                        List_SetEntrySortFunc(data->list, NewGamesList_SortByOpponentOrWhite);
		}
	}
	else if (stricmp(cdata->name, "Opponent Rating") == 0 || stricmp(cdata->name, "White Rating") == 0 || stricmp(cdata->name, "Low") == 0)
	{
		if (func == NewGamesList_SortByOpponentOrWhiteRating)
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByOpponentOrWhiteRatingReverse);
			forward = 0;
		}
		else
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByOpponentOrWhiteRating);
		}
	}
	if (stricmp(cdata->name, "Black") == 0)
	{
		if (func == NewGamesList_SortByBlack)
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByBlackReverse);
			forward = 0;
		}
		else
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByBlack);
		}
	}
	else if (stricmp(cdata->name, "Black Rating") == 0 || stricmp(cdata->name, "High") == 0)
	{
		if (func == NewGamesList_SortByBlackRating)
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByBlackRatingReverse);
			forward = 0;
		}
		else
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByBlackRating);
		}
	}
	else if (stricmp(cdata->name, "Rated Game") == 0)
	{
		if (func == NewGamesList_SortByRatedGame)
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByRatedGameReverse);
			forward = 0;
		}
		else
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByRatedGame);
		}
	}
	else if (stricmp(cdata->name, "Game Type") == 0)
	{
		if (func == NewGamesList_SortByGameType)
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByGameTypeReverse);
			forward = 0;
		}
		else
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByGameType);
		}
	}
	else if (stricmp(cdata->name, "Time Control") == 0)
	{
		if (func == NewGamesList_SortByEstGameTime)
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByEstGameTimeReverse);
			forward = 0;
		}
		else
		{
			List_SetEntrySortFunc(data->list, NewGamesList_SortByEstGameTime);
		}
	}
	else if (stricmp(cdata->name, "Your Color") == 0)
	{
		if (func == NewGamesList_SortBySide)
		{
                	List_SetEntrySortFunc(data->list, NewGamesList_SortBySideReverse);
			forward = 0;
		}
		else
		{
                	List_SetEntrySortFunc(data->list, NewGamesList_SortBySide);
		}
	}

	sortbox->flags |= BOX_VISIBLE;
	if (forward)
	{
		sortbox->img = ImageMgr_GetSubImage("findersortdown", "findersort.png", 0, 0, 9, 5);
	}
	else
	{
		sortbox->img = ImageMgr_GetSubImage("findersortup", "findersort.png", 9, 0, 9, 5);
	}


	List_Resort(data->list);
	List_RedoEntries(data->list);
	Box_Repaint(dialog);


}
/*
	NewGames_AddListCategory(dialog, "Opponent Name", 225);
	NewGames_AddListCategory(dialog, "Opponent Rating", 125);
	NewGames_AddListCategory(dialog, "Rated Game", 100);
	NewGames_AddListCategory(dialog, "Game Type", 100);
	NewGames_AddListCategory(dialog, "Time Control", 125);
	NewGames_AddListCategory(dialog, NULL, 75);
*/

void NewGamesCategoryData_Destroy(struct newgamescategorydata_s *cdata)
{
	Box_Destroy(cdata->box);
	free(cdata->name);
}

void NewGames_AddListCategory(struct Box_s *dialog, char *name, int w, int stretchw)
{
	struct newgamesdata_s *data = dialog->boxdata;
	struct Box_s *pbox, *subbox;

	pbox = Box_Create(data->lastcategoryx, 0, w, 40, BOX_VISIBLE | BOX_BORDER);
	if (stretchw == 1)
	{
		pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	}
	else if (stretchw == 2)
	{
		pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	}
	pbox->bgcol = CR_Gray;
	pbox->brcol = TabBG2;
	pbox->OnLButtonDown = NewGamesCategory_OnLButtonDown;
	Box_AddChild(data->listtitles, pbox);

	subbox = Text_Create(8, 6, w - 16, 26, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
	subbox->fgcol = RGB(255, 255, 255);
	Text_SetText(subbox, name);
	Box_AddChild(pbox, subbox);

	subbox = Box_Create(w - 12, 40 - 10, 9, 5, BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	subbox->img = ImageMgr_GetSubImage("findersortdown", "findersort.png", 0, 0, 9, 5);
	Box_AddChild(pbox, subbox);

	{
		struct newgamescategorydata_s *cdata = malloc(sizeof(*cdata));
		memset(cdata, 0, sizeof(*cdata));
		cdata->x = data->lastcategoryx;
		cdata->w = w;
		cdata->name = strdup(name);
		cdata->box = pbox;
		cdata->stretchw = stretchw;
		NamedList_Add(&(data->playcategories), name, cdata, NewGamesCategoryData_Destroy);
		pbox->boxdata = cdata;
	}

	data->lastcategoryx += w;
}

void NewGames_RefreshPage(struct Box_s *dialog, char *page)
{
	struct newgamesdata_s *data = dialog->boxdata;

	if (stricmp(page, data->currentpagetype) != 0)
	{
		return;
	}

	if (stricmp(page, "play") == 0)
	{
		NewGames_ClearSearch(dialog, page);
		if (data->searchinfo->showonlymine)
		{
			Ctrl_RequestGameSearch("myads", data->searchinfo);
		}
		else
		{
			Ctrl_SetPushFilter("play", data->searchinfo);
		}
		return;
	}

	NewGames_ClearSearch(dialog, page);

	if (data->currentpagetype && stricmp(data->currentpagetype , "myads") == 0)
	{
		data->numgameads = 0;
		NamedList_Destroy(&(data->openads));
	}

	Ctrl_RequestGameSearch(data->currentpagetype, data->searchinfo);

	/*NewGames_UpdateManageText(dialog);*/
}

void NewGames_RefreshCurrentPage(struct Box_s *dialog)
{
	struct newgamesdata_s *data = dialog->boxdata;

	if (!data->currentpagetype)
	{
		return;
	}

	NewGames_RefreshPage(dialog, data->currentpagetype);
}

void NewGamesRefreshButton_OnHit(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct newgamesdata_s *data = dialog->boxdata;

	NewGames_RefreshPage(dialog, data->currentpagetype);
}

void NewGamesNewGameButton_OnHit(struct Box_s *pbox)
{
	View_PopupGameCreateAdDialog(NULL);
}

void NewGames_AddAdSuccess(struct Box_s *dialog, char *id)
{
	struct newgamesdata_s *data = dialog->boxdata;

	if (stricmp("play", data->currentpagetype) == 0 && data->searchinfo->showonlymine)
	{
		NewGames_RefreshPage(dialog, "play");
	}
	else
	{
		if (id && id[0])
		{
			NamedList_AddString(&(data->openads), id, NULL);
		}
		data->numgameads++;
		/*NewGames_UpdateManageText(dialog);*/
	}
}

void NewGames_SetPage(struct Box_s *dialog, char *name)
{
	struct newgamesdata_s *data = dialog->boxdata;
	int w;

	while(data->playcategories)
	{
		NamedList_Remove(&(data->playcategories));
	}

	data->lastcategoryx = 0;

	List_SetEntrySortFunc(data->list, NULL);

	if (stricmp(name, "play") == 0)
	{
		NewGames_AddListCategory(dialog, "Game Type", 85, 0);
		NewGames_AddListCategory(dialog, "Opponent Name", dialog->w - 445, 1);
		NewGames_AddListCategory(dialog, "Opponent Rating", 70, 0);
		NewGames_AddListCategory(dialog, "Rated Game", 50, 0);
		NewGames_AddListCategory(dialog, "Time Control", 70, 0);
		NewGames_AddListCategory(dialog, "Your Color", 50, 0);
		NewGames_AddListCategory(dialog, NULL, 70, 0);
		NewGames_AddListCategory(dialog, NULL, 22, 0);
		Box_SetText(data->pagetitle, "Games To Play");
		Box_MeasureText(data->pagetitle, data->pagetitle->text, &w, NULL);
		data->pagetitle->w = w;
		data->otherpagelinks->x = data->pagetitle->x + w;
		Text_SetText(data->otherpagelinks, " | ^lGames To Watch^l | ^lAdjourned Games^l");
		Text_SetLinkCallback(data->otherpagelinks, 1, NewGamesLinkCallback_OnClick, "watch");
		Text_SetLinkCallback(data->otherpagelinks, 2, NewGamesLinkCallback_OnClick, "adjourned");
		data->searchinfo = data->playsearchinfo;
		data->myadstext->flags |= BOX_VISIBLE;
		data->myadscombo->flags |= BOX_VISIBLE;
		data->quickfiltercheck->flags |= BOX_VISIBLE;
		data->quickfilterchecktext->flags |= BOX_VISIBLE;
	}
	else if (stricmp(name, "watch") == 0)
	{
		NewGames_AddListCategory(dialog, "Game Type", 85, 0);
		NewGames_AddListCategory(dialog, "White", (dialog->w - 386) / 2, 1);
		NewGames_AddListCategory(dialog, "White Rating", 60, 0);
		NewGames_AddListCategory(dialog, "Black", (dialog->w - 386) / 2, 1);
		NewGames_AddListCategory(dialog, "Black Rating", 60, 0);
		NewGames_AddListCategory(dialog, "Time Control", 70, 0);
		NewGames_AddListCategory(dialog, NULL, 60, 0);
		NewGames_AddListCategory(dialog, NULL, 22, 0);
		Box_SetText(data->pagetitle, "Games To Watch");
		Box_MeasureText(data->pagetitle, data->pagetitle->text, &w, NULL);
		data->pagetitle->w = w;
		data->otherpagelinks->x = data->pagetitle->x + w;
		Text_SetText(data->otherpagelinks, " | ^lGames To Play^l | ^lAdjourned Games^l");
		Text_SetLinkCallback(data->otherpagelinks, 1, NewGamesLinkCallback_OnClick, "play");
		Text_SetLinkCallback(data->otherpagelinks, 2, NewGamesLinkCallback_OnClick, "adjourned");
		data->searchinfo = data->watchsearchinfo;
		data->myadstext->flags &= ~BOX_VISIBLE;
		data->myadscombo->flags &= ~BOX_VISIBLE;
		data->quickfiltercheck->flags &= ~BOX_VISIBLE;
		data->quickfilterchecktext->flags &= ~BOX_VISIBLE;
	}
	else if (stricmp(name, "adjourned") == 0) /* 590 */
	{
		NewGames_AddListCategory(dialog, "Date", 130, 0);
		NewGames_AddListCategory(dialog, "Your Color", 50, 0);
		NewGames_AddListCategory(dialog, "Rated Game", 50, 0);
		NewGames_AddListCategory(dialog, "Game Type", 85, 0);
		NewGames_AddListCategory(dialog, "Opponent Name", dialog->w - 515, 1); /* 115 */
		NewGames_AddListCategory(dialog, "Opponent Rating", 70, 0);
		NewGames_AddListCategory(dialog, NULL, 80, 0);
		NewGames_AddListCategory(dialog, NULL, 22, 0);
		Box_SetText(data->pagetitle, "Adjourned Games");
		Box_MeasureText(data->pagetitle, data->pagetitle->text, &w, NULL);
		data->pagetitle->w = w;
		data->otherpagelinks->x = data->pagetitle->x + w;
		Text_SetText(data->otherpagelinks, " | ^lGames To Play^l | ^lGames To Watch^l");
		Text_SetLinkCallback(data->otherpagelinks, 1, NewGamesLinkCallback_OnClick, "play");
		Text_SetLinkCallback(data->otherpagelinks, 2, NewGamesLinkCallback_OnClick, "watch");
		data->searchinfo = data->adjournedsearchinfo;
		data->myadstext->flags &= ~BOX_VISIBLE;
		data->myadscombo->flags &= ~BOX_VISIBLE;
		data->quickfiltercheck->flags &= ~BOX_VISIBLE;
		data->quickfilterchecktext->flags &= ~BOX_VISIBLE;
	}
	else if (stricmp(name, "myads") == 0)
	{
		NewGames_AddListCategory(dialog, "Rated Game", 50, 1);
		NewGames_AddListCategory(dialog, "Game Type", 85, 1);
		NewGames_AddListCategory(dialog, "Your Color", 50, 1);
		NewGames_AddListCategory(dialog, "Rating Filter", 150, 1);
		NewGames_AddListCategory(dialog, "Time Control", 70, 1);
		NewGames_AddListCategory(dialog, NULL, 80, 0);
		NewGames_AddListCategory(dialog, NULL, 80, 0);
		NewGames_AddListCategory(dialog, NULL, 22, 0);
		Box_SetText(data->pagetitle, "Manage Your Games");
		Box_MeasureText(data->pagetitle, data->pagetitle->text, &w, NULL);
		data->pagetitle->w = w;
		data->otherpagelinks->x = data->pagetitle->x + w;
		Text_SetText(data->otherpagelinks, " | ^lGames To Watch^l | ^lAdjourned Games^l");
		Text_SetLinkCallback(data->otherpagelinks, 1, NewGamesLinkCallback_OnClick, "watch");
		Text_SetLinkCallback(data->otherpagelinks, 2, NewGamesLinkCallback_OnClick, "adjourned");
		data->searchinfo = data->myadssearchinfo;
		data->myadstext->flags &= ~BOX_VISIBLE;
		data->myadscombo->flags &= ~BOX_VISIBLE;
		data->quickfiltercheck->flags &= ~BOX_VISIBLE;
		data->quickfilterchecktext->flags &= ~BOX_VISIBLE;
	}

	if (/*Model_GetOption(OPTION_ENABLEPUSHADS) && */stricmp(name, "play") == 0)
	{
		data->refreshbutton->flags &= ~BOX_VISIBLE;
		/*data->newgamebutton->x = 16;
		data->quicklinks->x = 105;*/
	}
	else
	{
		data->refreshbutton->flags |= BOX_VISIBLE;
		/*data->newgamebutton->x = 96;
		data->quicklinks->x = 185;*/
	}

	AutoSize_Fit(data->sizeablecontent);
	data->sizeablecontent->h = dialog->h - (data->showtrialwarning ? 30 : 0) - 8 - 36;
	AutoSize_Fill(data->sizeablecontent);

	data->currentpagetype = strdup(name);
	Box_Repaint(dialog);
}

void NewGames_UpdateFilters(struct Box_s *dialog)
{
	struct newgamesdata_s *data = dialog->boxdata;
	int *filtersvisible = NULL;

	CheckBox_SetChecked(data->aicheck, data->searchinfo->computers);
	CheckBox_SetChecked(data->ratedgamecheck, data->searchinfo->rated);
	CheckBox_SetChecked(data->ratedplayerscheck, data->searchinfo->filterunrated);

	if (data->searchinfo->variant == NULL)
	{
		ComboBox_SetSelection(data->variantcombo, _("Any"));
	}
	else if (stricmp(data->searchinfo->variant, "standard") == 0)
	{
		ComboBox_SetSelection(data->variantcombo, _("Standard"));
	}
	else if (stricmp(data->searchinfo->variant, "chess960") == 0)
	{
		ComboBox_SetSelection(data->variantcombo, _("Chess960"));
	}
	else if (stricmp(data->searchinfo->variant, "crazyhouse") == 0)
	{
		ComboBox_SetSelection(data->variantcombo, _("Crazyhouse"));
	}
	else if (stricmp(data->searchinfo->variant, "atomic") == 0)
	{
		ComboBox_SetSelection(data->variantcombo, _("Atomic"));
	}
	else if (stricmp(data->searchinfo->variant, "checkers") == 0)
	{
		ComboBox_SetSelection(data->variantcombo, _("Checkers"));
	}
	else if (stricmp(data->searchinfo->variant, "losers") == 0)
	{
		ComboBox_SetSelection(data->variantcombo, _("Loser's"));
	}

	if (data->searchinfo->relativerating == 0)
	{
		ComboBox_SetSelection(data->ratingcombo, _("+/- Any"));
	}
	else
	{
		char txt[256];
		sprintf (txt, "+/- %d", data->searchinfo->relativerating);
		ComboBox_SetSelection(data->ratingcombo, txt);
	}

	if (data->searchinfo->timecontrolrange == NULL)
	{
		ComboBox_SetSelection(data->timecombo, _("Any"));
	}
	else if (stricmp(data->searchinfo->timecontrolrange, "long") == 0)
	{
		ComboBox_SetSelection(data->timecombo, _("Long"));
	}
	else if (stricmp(data->searchinfo->timecontrolrange, "speed") == 0)
	{
		ComboBox_SetSelection(data->timecombo, _("Speed"));
	}
	else if (stricmp(data->searchinfo->timecontrolrange, "rapid") == 0)
	{
		ComboBox_SetSelection(data->timecombo, _("Rapid"));
	}
	else if (stricmp(data->searchinfo->timecontrolrange, "blitz") == 0)
	{
		ComboBox_SetSelection(data->timecombo, _("Blitz"));
	}
	else if (stricmp(data->searchinfo->timecontrolrange, "bullet") == 0)
	{
		ComboBox_SetSelection(data->timecombo, _("Bullet"));
	}

	if (data->searchinfo->quickapply)
	{
		CheckBox_SetChecked(data->quickfiltercheck, 1);
	}
	else
	{
		CheckBox_SetChecked(data->quickfiltercheck, 0);
	}

	if (stricmp(data->currentpagetype, "play") == 0)
	{
		filtersvisible = &(data->playsearchinfo->filteropen);
		/*
		Text_SetText(data->filtertext, "v ^lFiltering Options^l");
		Text_SetLinkCallback(data->filtertext, 1, NewGamesFilterLinkCallback_OnClick, NULL);
		*/
	}
	else if (stricmp(data->currentpagetype, "watch") == 0)
	{
		filtersvisible = &(data->watchsearchinfo->filteropen);
		/*
		Text_SetText(data->filtertext, "v ^lFiltering Options^l");
		Text_SetLinkCallback(data->filtertext, 1, NewGamesFilterLinkCallback_OnClick, NULL);
		*/
	}
	else if (stricmp(data->currentpagetype, "adjourned") == 0)
	{
		filtersvisible = &(data->adjournedsearchinfo->filteropen);
		/*
		Text_SetText(data->filtertext, "v ^lFiltering Options^l");
		Text_SetLinkCallback(data->filtertext, 1, NewGamesFilterLinkCallback_OnClick, NULL);
		*/
	}
	else
	{
		/*
		Text_SetText(data->filtertext, NULL);
		*/
	}

	if (data->searchinfo->showonlymine)
	{
		ComboBox_SetSelection(data->myadscombo, "Only");
	}
	else if (data->searchinfo->hideown)
	{
		ComboBox_SetSelection(data->myadscombo, "No");
	}
	else
	{
		ComboBox_SetSelection(data->myadscombo, "Yes");
	}

	{
		struct namedlist_s *groupslist = Model_GetLocalGroups();
		struct namedlist_s *entry;

		ComboBox_RemoveAllEntries(data->groupscombo);

		ComboBox_AddEntry2(data->groupscombo, "^none^", "None");

		if (!groupslist)
		{
			data->groupscombo->flags &= ~BOX_VISIBLE;
			data->groupstext->flags &= ~BOX_VISIBLE;
		}
		else
		{
#ifdef CHESSPARK_GROUPS
			if (!Model_IsInHiddenGroup())
			{
				entry = groupslist;
				data->groupstext->flags  |= BOX_VISIBLE;
				data->groupscombo->flags |= BOX_VISIBLE;

				while (entry)
				{
					struct groupinfo_s *ginfo = entry->data;
					ComboBox_AddEntry2(data->groupscombo, ginfo->id, ginfo->name);

					entry = entry->next;
				}
			}
#endif
		}

		entry = data->searchinfo->groupids;

		if (entry && entry->name)
		{
                        ComboBox_SetSelection(data->groupscombo, entry->name);
		}
		else
		{
			ComboBox_SetSelection(data->groupscombo, "^none^");
		}
	}

	if (filtersvisible && *filtersvisible)
	{
		/*data->listtitles->y = 116 + 80;*/
		/*data->filtertext->y = 96 + 80;*/
		/*data->list->y = 157 + 80;
		Box_OnSizeHeight_Stretch(data->list, dialog->h - 137 - 80 - 16 - 32 - 20 - data->list->h);*/
		data->filtersbox->flags |= BOX_VISIBLE;
	}
	else
	{
		/*data->listtitles->y = 116;*/
		/*data->filtertext->y = 96;*/
		/*data->list->y = 157;
		Box_OnSizeHeight_Stretch(data->list, dialog->h - 137 - 16 - 32 - 20 -data->list->h);*/
		data->filtersbox->flags &= ~BOX_VISIBLE;
	}

	AutoSize_Fit(data->sizeablecontent);
	data->sizeablecontent->h = dialog->h - (data->showtrialwarning ? 30 : 0) - 8 - 36;
	AutoSize_Fill(data->sizeablecontent);

	Box_Repaint(dialog);
}

void NewGamesLinkCallback_OnClick(struct Box_s *pbox, char *name)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct newgamesdata_s *data = dialog->boxdata;

	/*if (data->currentpagetype && stricmp(data->currentpagetype, "play") == 0)*/
	{
		Ctrl_SetPushGamesActivation(0);
	}

	List_RemoveAllEntries(data->list);
	List_ScrollToTop(data->list);
	List_RedoEntries(data->list);

	Model_SetSearchFilterPref(name, data->searchinfo);

	NewGames_SetPage(dialog, name);
	
	if (stricmp(name, "play") == 0 /*&& Model_GetOption(OPTION_ENABLEPUSHADS)*/)
	{
		if (data->searchinfo->showonlymine)
		{
			Ctrl_RequestGameSearch("myads", data->searchinfo);
		}
		else
		{
			Ctrl_SetPushFilter(name, data->searchinfo);
		}
	}
	else
	{
		Ctrl_RequestGameSearch(name, data->searchinfo);
	}

	NewGames_UpdateFilters(dialog);

	if (stricmp(name, "myads") == 0)
	{
		data->numgameads = 0;
		NamedList_Destroy(&(data->openads));
	}
	/*NewGames_UpdateManageText(dialog);*/

	Box_Repaint(dialog);
}

void NewGames_ExternalSetPage(struct Box_s *pbox, char *name)
{
	NewGamesLinkCallback_OnClick(pbox, name);
}

void NewGamesFilterLinkCallback_OnClick(struct Box_s *pbox, void *userdata)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct newgamesdata_s *data = dialog->boxdata;
	int *filtersvisible;

	if (stricmp(data->currentpagetype, "play") == 0)
	{
		filtersvisible = &(data->playsearchinfo->filteropen);
	}
	else if (stricmp(data->currentpagetype, "watch") == 0)
	{
		filtersvisible = &(data->watchsearchinfo->filteropen);
	}
	else if (stricmp(data->currentpagetype, "adjourned") == 0)
	{
		filtersvisible = &(data->adjournedsearchinfo->filteropen);
	}
	else
	{
		return;
	}

	if (*filtersvisible)
	{
		*filtersvisible = 0;
	}
	else
	{
		*filtersvisible = 1;
	}

	NewGamesLinkCallback_OnClick(dialog, data->currentpagetype);
}

void NewGamesFilter_OnVariantCombo(struct Box_s *combo, char *name)
{
	struct Box_s *dialog = Box_GetRoot(combo);
	struct newgamesdata_s *data = dialog->boxdata;

	free(data->searchinfo->variant);
	data->searchinfo->variant = NULL;

	if (!name)
	{
	}
	else if (stricmp(name, _("Standard")) == 0)
	{
		data->searchinfo->variant = strdup("standard");
	}
	else if (stricmp(name, _("Chess960")) == 0)
	{
		data->searchinfo->variant = strdup("chess960");
	}
	else if (stricmp(name, _("Crazyhouse")) == 0)
	{
		data->searchinfo->variant = strdup("crazyhouse");
	}
	else if (stricmp(name, _("Loser's")) == 0)
	{
		data->searchinfo->variant = strdup("losers");
	}
	else if (stricmp(name, _("Atomic")) == 0)
	{
		data->searchinfo->variant = strdup("atomic");
	}
	else if (stricmp(name, _("Checkers")) == 0)
	{
		data->searchinfo->variant = strdup("checkers");
	}

	NewGamesLinkCallback_OnClick(dialog, data->currentpagetype);
}

void NewGamesFilter_OnRatingCombo(struct Box_s *combo, char *name)
{
	struct Box_s *dialog = Box_GetRoot(combo);
	struct newgamesdata_s *data = dialog->boxdata;

	data->searchinfo->relativerating = 0;

	if (!name)
	{
	}
	else if (stricmp(name, "+/- 50") == 0)
	{
		data->searchinfo->relativerating = 50;
	}
	else if (stricmp(name, "+/- 100") == 0)
	{
		data->searchinfo->relativerating = 100;
	}
	else if (stricmp(name, "+/- 150") == 0)
	{
		data->searchinfo->relativerating = 150;
	}
	else if (stricmp(name, "+/- 200") == 0)
	{
		data->searchinfo->relativerating = 200;
	}
	else if (stricmp(name, "+/- 250") == 0)
	{
		data->searchinfo->relativerating = 250;
	}
	else if (stricmp(name, "+/- 300") == 0)
	{
		data->searchinfo->relativerating = 300;
	}
	else if (stricmp(name, "+/- 350") == 0)
	{
		data->searchinfo->relativerating = 350;
	}
	else if (stricmp(name, "+/- 400") == 0)
	{
		data->searchinfo->relativerating = 400;
	}
	else if (stricmp(name, "+/- 450") == 0)
	{
		data->searchinfo->relativerating = 450;
	}
	else if (stricmp(name, "+/- 500") == 0)
	{
		data->searchinfo->relativerating = 500;
	}
	else if (stricmp(name, _("+/- Any")) == 0)
	{
	}

	NewGamesLinkCallback_OnClick(dialog, data->currentpagetype);
}

void NewGamesFilter_OnTimeControlCombo(struct Box_s *combo, char *name)
{
	struct Box_s *dialog = Box_GetRoot(combo);
	struct newgamesdata_s *data = dialog->boxdata;

	free(data->searchinfo->timecontrolrange);
	data->searchinfo->timecontrolrange = NULL;

	if (!name)
	{
	}
	else if (stricmp(name, _("Long")) == 0)
	{
		data->searchinfo->timecontrolrange = strdup("long");
	}
	else if (stricmp(name, _("Speed")) == 0)
	{
		data->searchinfo->timecontrolrange = strdup("speed");
	}
	else if (stricmp(name, _("Rapid")) == 0)
	{
		data->searchinfo->timecontrolrange = strdup("rapid");
	}
	else if (stricmp(name, _("Blitz")) == 0)
	{
		data->searchinfo->timecontrolrange = strdup("blitz");
	}
	else if (stricmp(name, _("Bullet")) == 0)
	{
		data->searchinfo->timecontrolrange = strdup("bullet");
	}

	NewGamesLinkCallback_OnClick(dialog, data->currentpagetype);
}

void NewGamesFilter_OnMyAdsCombo(struct Box_s *combo, char *name)
{
	struct Box_s *dialog = Box_GetRoot(combo);
	struct newgamesdata_s *data = dialog->boxdata;

	/*
	free(data->searchinfo->timecontrolrange);
	data->searchinfo->timecontrolrange = NULL;
	*/

	if (!name)
	{
	}
	else if (stricmp(name, _("Yes")) == 0)
	{
		data->searchinfo->hideown = 0;
		data->searchinfo->showonlymine = 0;
	}
	else if (stricmp(name, _("No")) == 0)
	{
		data->searchinfo->hideown = 1;
		data->searchinfo->showonlymine = 0;
	}
	else if (stricmp(name, _("Only")) == 0)
	{
		data->searchinfo->hideown = 0;
		data->searchinfo->showonlymine = 1;
	}

	NewGamesLinkCallback_OnClick(dialog, data->currentpagetype);
}

void NewGamesFilter_OnGroupsCombo(struct Box_s *combo, char *name)
{
	struct Box_s *dialog = Box_GetRoot(combo);
	struct newgamesdata_s *data = dialog->boxdata;

	NamedList_Destroy(&(data->searchinfo->groupids));

	if (name && stricmp(name, "^none^"))
	{
		NamedList_Add(&(data->searchinfo->groupids), name, NULL, NULL);
	}

	NewGamesLinkCallback_OnClick(dialog, data->currentpagetype);
}

void NewGamesFilter_OnRatedGameCheckHit(struct Box_s *checkbox, int checked)
{
	struct Box_s *dialog = Box_GetRoot(checkbox);
	struct newgamesdata_s *data = dialog->boxdata;

	data->searchinfo->rated = checked;

	NewGamesLinkCallback_OnClick(dialog, data->currentpagetype);
}

void NewGamesFilter_OnRatedPlayersCheckHit(struct Box_s *checkbox, int checked)
{
	struct Box_s *dialog = Box_GetRoot(checkbox);
	struct newgamesdata_s *data = dialog->boxdata;

	data->searchinfo->filterunrated = checked;

	NewGamesLinkCallback_OnClick(dialog, data->currentpagetype);
}

void NewGamesFilter_OnAICheckHit(struct Box_s *checkbox, int checked)
{
	struct Box_s *dialog = Box_GetRoot(checkbox);
	struct newgamesdata_s *data = dialog->boxdata;

	data->searchinfo->computers = checked;

	NewGamesLinkCallback_OnClick(dialog, data->currentpagetype);
}

void NewGamesFilter_OnQuickFilterCheckHit(struct Box_s *checkbox, int checked)
{
	struct Box_s *dialog = Box_GetRoot(checkbox);
	struct newgamesdata_s *data = dialog->boxdata;

	data->playsearchinfo->quickapply = checked;

	NewGamesLinkCallback_OnClick(dialog, data->currentpagetype);
}

void NewGamesDontShowAgain_OnCheckHit(struct Box_s *checkbox, int checked)
{
	Model_SetOption(OPTION_NOGAMESEARCHONLOGIN, checked, NULL);
}

void NewGames_OnRestore(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_RESTORE);
}

void NewGames_OnMinimize(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	/* might as well save the window position here */
	View_SetSavedWindowPos("GameFinder", dialog->x, dialog->y, dialog->w, dialog->h);

	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_MINIMIZE);
}

void NewGames_OnClose(struct Box_s *pbox)
{
	View_CloseNewGamesDialog();
}

void NewGames_KillGameAds(struct Box_s *pbox)
{
	struct newgamesdata_s *data = pbox->boxdata;

	while (data->openads)
	{
		Ctrl_RemoveGameAd(data->openads->name);
		NamedList_Remove(&(data->openads));
	}

	data->numgameads = 0;
	/*NewGames_UpdateManageText(pbox);*/
}

void NewGames_OnDestroy(struct Box_s *pbox)
{
	struct newgamesdata_s *data = pbox->boxdata;

	if (data->currentpagetype && stricmp(data->currentpagetype, "play") == 0)
	{
		Ctrl_SetPushGamesActivation(0);
	}

	NewGames_KillGameAds(pbox);

	if (!IsIconic(pbox->hwnd))
	{
		View_SetSavedWindowPos("GameFinder", pbox->x, pbox->y, pbox->w, pbox->h);
	}
}

extern HFONT tahoma13_f;

void NewGamesLink_AddQuickAd(struct Box_s *pbox, char *length)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct newgamesdata_s *data = dialog->boxdata;
	struct gamesearchinfo_s *info;

	if (data->playsearchinfo->quickapply)
	{
		info = Info_DupeGameSearchInfo(data->playsearchinfo);

		if (!info->variant)
		{
			info->variant = strdup("standard");
		}
	}
	else
	{
                info = malloc(sizeof(*info));
		memset(info, 0, sizeof(*info));

		info->variant = strdup("standard");
	}

	if (stricmp(length, "speed") == 0)
	{
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 3);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = -1;
		info->timecontrol->controlarray[2] = 15 * 60;
		info->timecontrol->delayinc = -5;
	}
	else if (stricmp(length, "rapid") == 0)
	{
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 3);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = -1;
		info->timecontrol->controlarray[2] = 10 * 60;
		info->timecontrol->delayinc = -5;
	}
	else if (stricmp(length, "blitz") == 0)
	{
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 3);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = -1;
		info->timecontrol->controlarray[2] = 5 * 60;
		info->timecontrol->delayinc = -2;
	}
	else if (stricmp(length, "bullet") == 0)
	{
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 3);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = -1;
		info->timecontrol->controlarray[2] = 1 * 60;
		info->timecontrol->delayinc = -2;
	}
	else
	{
		info->timecontrol = malloc(sizeof(*(info->timecontrol)));
		memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
		info->timecontrol->controlarray = malloc(sizeof(int) * 3);
		info->timecontrol->controlarray[0] = 1;
		info->timecontrol->controlarray[1] = -1;
		info->timecontrol->controlarray[2] = 30 * 60;
		info->timecontrol->delayinc = -5;
	}
	info->rated = 1;

	Ctrl_PostGameAd(info);

	Info_DestroyGameSearchInfo(info);
/*
	if (stricmp("myads", data->currentpagetype) == 0)
	{
		NewGames_RefreshPage(dialog, "myads");
	}
	else
	{
		data->numgameads++;
		NewGames_UpdateManageText(dialog);
	}
	*/
}

void NewGamesBubble_OnClose(struct Box_s *bubbleclose, int x, int y)
{
	struct Box_s *bubble = bubbleclose->parent;
	struct Box_s *dialog = Box_GetRoot(bubbleclose);
	struct newgamesdata_s *data = dialog->boxdata;

	bubble->flags &= ~BOX_VISIBLE;

	AutoSize_Fit(data->sizeablecontent);
	data->sizeablecontent->h = dialog->h - (data->showtrialwarning ? 30 : 0) - 8 - 36;
	AutoSize_Fill(data->sizeablecontent);

	Model_SetOption(OPTION_HIDEGAMEFINDERHELP, 1, NULL);

	Box_Repaint(dialog);
}

extern HFONT tahoma10_f;

void ReportProblemLink_OnClick(struct Box_s *pbox, void *userdata)
{
	/* do a ping here so we know what the user latency was */
	Ctrl_Ping("match.chesspark.com", 0, 0);
	ProblemReport_Create(Box_GetRoot(pbox));
}

struct Box_s *NewGames_Create(int x, int y, int w, int h, struct Box_s *roster)
{
	struct newgamesdata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *pbox, *vertsize;

	memset(data, 0, sizeof(*data));

	data->showtrialwarning = Model_IsLocalMemberFree(0);

	data->playsearchinfo = Info_DupeGameSearchInfo(Model_GetSearchFilterPref("play"));
	if (!data->playsearchinfo)
	{
		data->playsearchinfo = malloc(sizeof(*(data->playsearchinfo)));
		memset(data->playsearchinfo, 0, sizeof(*(data->playsearchinfo)));
		data->playsearchinfo->computers = 1;
	}

	data->watchsearchinfo = Info_DupeGameSearchInfo(Model_GetSearchFilterPref("watch"));
	if (!data->watchsearchinfo)
	{
		data->watchsearchinfo = malloc(sizeof(*(data->watchsearchinfo)));
		memset(data->watchsearchinfo, 0, sizeof(*(data->watchsearchinfo)));
		data->watchsearchinfo->computers = 1;
	}

	data->adjournedsearchinfo = Info_DupeGameSearchInfo(Model_GetSearchFilterPref("adjourned"));
	if (!data->adjournedsearchinfo)
	{
		data->adjournedsearchinfo = malloc(sizeof(*(data->adjournedsearchinfo)));
		memset(data->adjournedsearchinfo, 0, sizeof(*(data->adjournedsearchinfo)));
		data->adjournedsearchinfo->computers = 1;
	}

	data->myadssearchinfo = Info_DupeGameSearchInfo(Model_GetSearchFilterPref("myads"));
	if (!data->myadssearchinfo)
	{
		data->myadssearchinfo = malloc(sizeof(*(data->myadssearchinfo)));
		memset(data->myadssearchinfo, 0, sizeof(*(data->myadssearchinfo)));
	}

	data->searchinfo = data->playsearchinfo;

	dialog = Box_Create(x, y, w, h, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;
	dialog->minw = 640;
	dialog->minh = 400;

	SizerSet_Create(dialog);

	dialog->boxdata = data;

	dialog->titlebar   = TitleBar_Add2(dialog, _("Game Finder"), NULL, NewGames_OnMinimize, NULL, 0);
	dialog->OnActive   = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	dialog->OnMinimize = NewGames_OnMinimize;
	dialog->OnRestore  = NewGames_OnRestore;
	dialog->OnClose    = NewGames_OnClose;
	dialog->OnCommand  = Menu_OnCommand;

	pbox = Box_Create(8, 30, dialog->w - 16, dialog->h  - (data->showtrialwarning ? 30 : 0) - 8 - 32, BOX_VISIBLE | BOX_BORDER);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	pbox->bgcol = TabBG2;
	pbox->brcol = RGB(176,178,183);
	Box_AddChild(dialog, pbox);
	data->innerpage = pbox;
	
	{
		struct Box_s *subbox2;
		subbox2 = Box_Create(0, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersul", "contentcorners.png", 0, 0, 5, 5);
		Box_AddChild(pbox, subbox2);

		subbox2 = Box_Create(pbox->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersur", "contentcorners.png", 5, 0, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(pbox, subbox2);

		subbox2 = Box_Create(pbox->w - 5, pbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdr", "contentcorners.png", 5, 5, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(pbox, subbox2);

		subbox2 = Box_Create(0, pbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdl", "contentcorners.png", 0, 5, 5, 5);
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(pbox, subbox2);
	}

	vertsize = AutoSize_Create(16, 32, dialog->w - 32, dialog->h - 30 - 8 - 34, 0, 0, AUTOSIZE_VERT);
	vertsize->OnSizeWidth = Box_OnSizeWidth_Stretch;
	vertsize->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(dialog, vertsize);
	data->sizeablecontent = vertsize;
	{
		struct Box_s *horizsize;

		horizsize = AutoSize_Create(4, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
		horizsize->OnSizeWidth = Box_OnSizeWidth_Stretch;
		Box_AddChild(vertsize, horizsize);
		{
                        pbox = Box_Create(0, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT);
			pbox->font = tahoma13_f;
			Box_AddChild(horizsize, pbox);
			data->pagetitle = pbox;

			pbox = Text_Create(0, 2, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Box_AddChild(horizsize, pbox);
			data->otherpagelinks = pbox;

			pbox = Text_Create(0, 2, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Text_SetText(pbox, "^lReport a Problem^l");
			Text_SetLinkCallback(pbox, 1, ReportProblemLink_OnClick, NULL);
			Box_AddChild(horizsize, pbox);
		}
	
		pbox = Box_Create(0, 50, dialog->w - 32, 1, BOX_VISIBLE);
		pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		pbox->bgcol = UserInfoFG2;
		Box_AddChild(vertsize, pbox);

		AutoSize_AddSpacer(vertsize, 5);

		pbox = Box_Create(4, 0, dialog->w - 40, 20, (Model_GetOption(OPTION_HIDEGAMEFINDERHELP) ? 0 : BOX_VISIBLE));
		pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		pbox->bgcol = RGB(236, 204, 127);
		{
			struct Box_s *subbox;

			subbox = Box_Create(pbox->w - 15, 0, 10, 15, BOX_VISIBLE | BOX_TRANSPARENT);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
			subbox->fgcol = RGB(0x55, 0x55,0x55);
			Box_SetText(subbox, "x");
			subbox->OnLButtonDown = NewGamesBubble_OnClose;
			Box_AddChild(pbox, subbox);

			subbox = Text_Create(10, 10, pbox->w - 20, 0, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT | TX_STRETCHPARENT);
			subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
			subbox->fgcol = RGB(0x55, 0x55,0x55);
			/*subbox->font = tahoma10_f;*/
			Box_AddChild(pbox, subbox);
			Text_SetText(subbox, "^C^bWelcome to the Game Finder!^b^C^E\n^E"
				"Available games are listed below. Click the ^bPlay^b button to "
				"join one. ^bSort the list^b by clicking the column headings, and "
				"control what games are displayed with ^bfilter options^b.^E\n^E"
				"Don't like the listed games? Start a game fast with the ^bQuick "
				"Play^b links. To start a customized game, click ^bNew Game^b.^E\n^E"
				"Limit who can choose your game using the filter options setting "
				"^bApply Filters to Quick Play^b or use the limit controls in "
				"^bNew Game^b.");
		}
		Box_AddChild(vertsize, pbox);

		horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
		horizsize->OnSizeWidth = Box_OnSizeWidth_Stretch;
		Box_AddChild(vertsize, horizsize);
		{
			pbox = StdButton_Create(0, 0, 40, "Refresh", 1);
			Button2_SetOnButtonHit(pbox, NewGamesRefreshButton_OnHit);
			Box_AddChild(horizsize, pbox);
			data->refreshbutton = pbox;

			AutoSize_AddSpacer(horizsize, 5);

			pbox = StdButton_Create(0, 0, 40, "New Game", 1);
			Button2_SetOnButtonHit(pbox, NewGamesNewGameButton_OnHit);
			Box_AddChild(horizsize, pbox);
			data->newgamebutton = pbox;

			AutoSize_AddSpacer(horizsize, 5);

			pbox = Text_Create(0, 0, 0, 25, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_CENTERVERT);
			/*pbox->OnSizeWidth = Text_OnSizeWidth_Stretch;*/
#ifdef CHESSPARK_RAPIDTIMECONTROL
			Text_SetText(pbox, "^bQuick Play:^n   ^l+ Quick (1 min)^l  ^l+ Quick (5 min)^l  ^l+ Quick (10 min)^l  ^l+ Quick (15 min)^l  ^l+ Quick (30 min)^l");
			Text_SetLinkCallback(pbox, 1, NewGamesLink_AddQuickAd, "bullet");
			Text_SetLinkCallback(pbox, 2, NewGamesLink_AddQuickAd, "blitz");
			Text_SetLinkCallback(pbox, 3, NewGamesLink_AddQuickAd, "rapid");
			Text_SetLinkCallback(pbox, 4, NewGamesLink_AddQuickAd, "speed");
			Text_SetLinkCallback(pbox, 5, NewGamesLink_AddQuickAd, "long");
#else
			Text_SetText(pbox, "^bQuick Play:^n   ^l+ Quick (1 min)^l  ^l+ Quick (5 min)^l  ^l+ Quick (15 min)^l  ^l+ Quick (30 min)^l");
			Text_SetLinkCallback(pbox, 1, NewGamesLink_AddQuickAd, "bullet");
			Text_SetLinkCallback(pbox, 2, NewGamesLink_AddQuickAd, "blitz");
			Text_SetLinkCallback(pbox, 3, NewGamesLink_AddQuickAd, "speed");
			Text_SetLinkCallback(pbox, 4, NewGamesLink_AddQuickAd, "long");
#endif
			Box_AddChild(horizsize, pbox);
			data->quicklinks = pbox;
		}

		AutoSize_AddSpacer(vertsize, 5);

		pbox = Box_Create(0, 0, dialog->w - 32, 1, BOX_VISIBLE);
		pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		pbox->bgcol = UserInfoFG2;
		Box_AddChild(vertsize, pbox);

		AutoSize_AddSpacer(vertsize, 5);

		pbox = Text_Create(4, 0, dialog->w - 32, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		Text_SetText(pbox, "v ^lFiltering Options^l");
		Text_SetLinkCallback(pbox, 1, NewGamesFilterLinkCallback_OnClick, NULL);
		Box_AddChild(vertsize, pbox);
		data->filtertext = pbox;

		AutoSize_AddSpacer(vertsize, 5);

		pbox = Box_Create(0, 0, dialog->w - 32, 80, BOX_TRANSPARENT);
		pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		Box_AddChild(vertsize, pbox);
		data->filtersbox = pbox;
		{

			pbox = Box_Create(8, 2, 80, 20, BOX_VISIBLE | BOX_TRANSPARENT);
			Box_SetText(pbox, "Variant");
			Box_AddChild(data->filtersbox, pbox);

			pbox = ComboBox_Create(100, 0, 100, 20, BOX_VISIBLE | BOX_BORDER);
			ComboBox_AddEntry(pbox, _("Standard"));
			ComboBox_AddEntry(pbox, _("Chess960"));
#ifdef CHESSPARK_CRAZYHOUSE
			ComboBox_AddEntry(pbox, _("Crazyhouse"));
#endif
#ifdef CHESSPARK_LOSERS
			ComboBox_AddEntry(pbox, _("Loser's"));
#endif
			ComboBox_AddEntry(pbox, _("Atomic"));
#ifdef CHESSPARK_CHECKERS
			ComboBox_AddEntry(pbox, _("Checkers"));
#endif
			ComboBox_AddEntry(pbox, _("Any"));
			ComboBox_SetSelection(pbox, _("Any"));
			ComboBox_SetOnSelection(pbox, NewGamesFilter_OnVariantCombo);
			data->variantcombo = pbox;
			Box_AddChild(data->filtersbox, pbox);

			pbox = Box_Create(8, 27, 100, 20, BOX_VISIBLE | BOX_TRANSPARENT);
			Box_SetText(pbox, "Opponent Rating");
			Box_AddChild(data->filtersbox, pbox);

			pbox = ComboBox_Create(100, 25, 100, 20, BOX_VISIBLE | BOX_BORDER);
			ComboBox_AddEntry(pbox, "+/- 50");
			ComboBox_AddEntry(pbox, "+/- 100");
			ComboBox_AddEntry(pbox, "+/- 150");
			ComboBox_AddEntry(pbox, "+/- 200");
			ComboBox_AddEntry(pbox, "+/- 250");
			ComboBox_AddEntry(pbox, "+/- 300");
			ComboBox_AddEntry(pbox, "+/- 350");
			ComboBox_AddEntry(pbox, "+/- 400");
			ComboBox_AddEntry(pbox, "+/- 450");
			ComboBox_AddEntry(pbox, "+/- 500");
			ComboBox_AddEntry(pbox, _("+/- Any"));
			ComboBox_SetSelection(pbox, _("+/- Any"));
			ComboBox_SetOnSelection(pbox, NewGamesFilter_OnRatingCombo);
			data->ratingcombo = pbox;
			Box_AddChild(data->filtersbox, pbox);

			pbox = Box_Create(8, 52, 100, 20, BOX_VISIBLE | BOX_TRANSPARENT);
			Box_SetText(pbox, "Time Control");
			Box_AddChild(data->filtersbox, pbox);

			pbox = ComboBox_Create(100, 50, 100, 20, BOX_VISIBLE | BOX_BORDER);
			ComboBox_AddEntry(pbox, _("Long"));
			ComboBox_AddEntry(pbox, _("Speed"));
#ifdef CHESSPARK_RAPIDTIMECONTROL
			ComboBox_AddEntry(pbox, _("Rapid"));
#endif
			ComboBox_AddEntry(pbox, _("Blitz"));
			ComboBox_AddEntry(pbox, _("Bullet"));
			ComboBox_AddEntry(pbox, _("Any"));
			ComboBox_SetSelection(pbox, _("Any"));
			ComboBox_SetOnSelection(pbox, NewGamesFilter_OnTimeControlCombo);
			data->timecombo = pbox;
			Box_AddChild(data->filtersbox, pbox);

			pbox = CheckBox_Create(220, 2, BOX_VISIBLE);
			CheckBox_SetOnHit(pbox, NewGamesFilter_OnRatedGameCheckHit);
			Box_AddChild(data->filtersbox, pbox);
			data->ratedgamecheck = pbox;

			pbox = CheckBoxLinkedText_Create(235, 2, 165, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->ratedgamecheck);
			pbox->fgcol = RGB(0, 0, 0);
			Box_SetText(pbox, _("Only Rated Games"));
			Box_AddChild(data->filtersbox, pbox);

			pbox = CheckBox_Create(220, 18, BOX_VISIBLE);
			CheckBox_SetOnHit(pbox, NewGamesFilter_OnAICheckHit);
			Box_AddChild(data->filtersbox, pbox);
			data->aicheck = pbox;

			pbox = CheckBoxLinkedText_Create(235, 18, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->aicheck);
			pbox->fgcol = RGB(0, 0, 0);
			Box_SetText(pbox, _("Include Computer Opponents"));
			Box_AddChild(data->filtersbox, pbox);

			pbox = CheckBox_Create(220, 34, BOX_VISIBLE);
			CheckBox_SetOnHit(pbox, NewGamesFilter_OnRatedPlayersCheckHit);
			Box_AddChild(data->filtersbox, pbox);
			data->ratedplayerscheck = pbox;

			pbox = CheckBoxLinkedText_Create(235, 34, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->ratedplayerscheck);
			pbox->fgcol = RGB(0, 0, 0);
			Box_SetText(pbox, _("Exclude Unrated Players"));
			Box_AddChild(data->filtersbox, pbox);

			pbox = Box_Create(220, 52, 100, 20, BOX_VISIBLE | BOX_TRANSPARENT);
			Box_SetText(pbox, _("Show My Games:"));
			Box_AddChild(data->filtersbox, pbox);
			data->myadstext = pbox;

			pbox = ComboBox_Create(320, 50, 60, 20, BOX_VISIBLE | BOX_BORDER);
			ComboBox_AddEntry(pbox, _("Yes"));
			ComboBox_AddEntry(pbox, _("No"));
			ComboBox_AddEntry(pbox, _("Only"));
			ComboBox_SetSelection(pbox, _("Yes"));
			ComboBox_SetOnSelection(pbox, NewGamesFilter_OnMyAdsCombo);
			Box_AddChild(data->filtersbox, pbox);
			data->myadscombo = pbox;

			pbox = CheckBox_Create(400, 2, BOX_VISIBLE);
			CheckBox_SetOnHit(pbox, NewGamesFilter_OnQuickFilterCheckHit);
			Box_AddChild(data->filtersbox, pbox);
			data->quickfiltercheck = pbox;

			pbox = CheckBoxLinkedText_Create(415, 2, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->quickfiltercheck);
			pbox->fgcol = RGB(0, 0, 0);
			Box_SetText(pbox, _("Apply filters to quick links"));
			Box_AddChild(data->filtersbox, pbox);
			data->quickfilterchecktext = pbox;

			pbox = Box_Create(400, 52, 100, 20, BOX_VISIBLE | BOX_TRANSPARENT);
			Box_SetText(pbox, _("Group filter:"));
			Box_AddChild(data->filtersbox, pbox);
			data->groupstext = pbox;

			pbox = ComboBox_Create(500, 50, 60, 20, BOX_VISIBLE | BOX_BORDER);
			ComboBox_SetOnSelection(pbox, NewGamesFilter_OnGroupsCombo);
			Box_AddChild(data->filtersbox, pbox);
			data->groupscombo = pbox;
		}

		pbox = Box_Create(0, 0, dialog->w - 32, 40, BOX_VISIBLE | BOX_TRANSPARENT);
		pbox->OnSizeWidth = NewSearchListTitles_OnSizeWidth_Stretch;
		Box_AddChild(vertsize, pbox);
		data->listtitles = pbox;

		horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
		horizsize->OnSizeWidth = Box_OnSizeWidth_Stretch;
		horizsize->OnSizeHeight = Box_OnSizeHeight_Stretch;
		Box_AddChild(vertsize, horizsize);
		{
			pbox = List_Create(0, 0, dialog->w - 32, 0, BOX_VISIBLE, FALSE);
			List_SetEntrySelectable(pbox, 0);
			pbox->bgcol = TabBG2;
			pbox->fgcol = TabFG2;
			pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
			pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
			Box_AddChild(horizsize, pbox);
			data->list = pbox;

			AutoSize_AddSpacer(horizsize, 3);
		}
	}
/*
	pbox = CheckBox_Create(20, dialog->h - 32, BOX_VISIBLE);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	CheckBox_SetChecked(pbox, Model_GetOption(OPTION_NOGAMESEARCHONLOGIN));
	CheckBox_SetOnHit(pbox, NewGamesDontShowAgain_OnCheckHit);
	Box_AddChild(dialog, pbox);
	data->dontshowagaincheck = pbox;

	pbox = CheckBoxLinkedText_Create(40, dialog->h - 32, 100, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->dontshowagaincheck);
	pbox->fgcol = UserInfoFG2;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_SetText(pbox, _("Don't show on login"));
	Box_AddChild(dialog, pbox);
*/
	pbox = Box_Create(dialog->w - GrabberWidth, dialog->h - GrabberHeight, GrabberWidth, GrabberHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->img = ImageMgr_GetImage("windowResizeHandle.png");
	pbox->OnSizeWidth  = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_AddChild(dialog, pbox);

	AutoSize_Fit(data->sizeablecontent);
	data->sizeablecontent->h = dialog->h - (data->showtrialwarning ? 30 : 0) - 8 - 36;
	AutoSize_Fill(data->sizeablecontent);
	/*
	pbox = TabCtrl_Create(8, 31, 400, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0, 1, 0);
	TabCtrl_AddTab2(pbox, "play", _("Play"), NULL, -60, NULL, NULL, 0, NULL, NULL);
	TabCtrl_AddTab2(pbox, "watch", _("Watch"), NULL, -60, NULL, NULL, 0, NULL, NULL);
	TabCtrl_AddTab2(pbox, "adjourned", _("Adjourned"), NULL, -60, NULL, NULL, 0, NULL, NULL);
	TabCtrl_AddTab2(pbox, "myads", _("My Game Ads"), NULL, -60, NULL, NULL, 0, NULL, NULL);
	TabCtrl_ActivateFirst(pbox);
	TabCtrl_SetTabActivateFunc(pbox, NewGamesLinkCallback_OnClick);
	Box_AddChild(dialog, pbox);
	data->tabctrl = pbox;
*/
	if (data->showtrialwarning)
	{
#if 0
		char txt[255];
		pbox = Box_Create(8, dialog->h - 8 - 28, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_RIGHTTEXT);
		pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		pbox->fgcol = CR_LtOrange;
		pbox->font = tahoma24b_f;
		sprintf(txt, "%d", data->showtrialwarning);
		Box_SetText(pbox, txt);
		Box_AddChild(dialog, pbox);

		pbox = Text_Create(43, dialog->h - 8 - 26, 400, 30, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Text_SetLinkColor(pbox, CR_LtOrange);
		Text_SetText(pbox, "^ldays left in your free trial.^l\n^lClick here to subscribe!^l");
		Text_SetLinkCallback(pbox, 1, Util_OpenURL2, "http://www.chesspark.com/purchase/");
		Text_SetLinkCallback(pbox, 2, Util_OpenURL2, "http://www.chesspark.com/purchase/");
		Box_AddChild(dialog, pbox);
#endif
		pbox = Text_Create(8, dialog->h - 8 - 26, 400, 30, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Text_SetLinkColor(pbox, CR_LtOrange);
		Text_SetText(pbox, "^lThere's more to Chesspark!^l\n^lUpgrade your FREE account to a PRO account today! Click here!^l");
		Text_SetLinkCallback(pbox, 1, Util_OpenURL2, "http://www.chesspark.com/help/upgrade/");
		Text_SetLinkCallback(pbox, 2, Util_OpenURL2, "http://www.chesspark.com/help/upgrade/");
		Box_AddChild(dialog, pbox);
	}
#if 0
	pbox = Box_Create(20, 32, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->font = tahoma13_f;
	Box_AddChild(dialog, pbox);
	data->pagetitle = pbox;

	pbox = Text_Create(200, 34, dialog->w - 200 - 20, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	/*pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;*/
	pbox->fgcol = UserInfoFG2;
	Box_AddChild(dialog, pbox);
	data->otherpagelinks = pbox;
	
	pbox = Box_Create(16, 50, dialog->w - 32, 1, BOX_VISIBLE);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->bgcol = UserInfoFG2;
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(16, 58, 40, "Refresh", 1);
	Button2_SetOnButtonHit(pbox, NewGamesRefreshButton_OnHit);
	Box_AddChild(dialog, pbox);
	data->refreshbutton = pbox;

	pbox = StdButton_Create(96, 58, 40, "New Game", 1);
	Button2_SetOnButtonHit(pbox, NewGamesNewGameButton_OnHit);
	Box_AddChild(dialog, pbox);
	data->newgamebutton = pbox;

	pbox = Text_Create(185, 56, 0, 25, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ | TX_CENTERVERT);
	pbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
#ifdef CHESSPARK_RAPIDTIMECONTROL
	Text_SetText(pbox, "^bQuick Play:^n   ^l+ Quick (1 min)^l  ^l+ Quick (5 min)^l  ^l+ Quick (10 min)^l  ^l+ Quick (15 min)^l  ^l+ Quick (30 min)^l");
	Text_SetLinkCallback(pbox, 1, NewGamesLink_AddQuickAd, "bullet");
	Text_SetLinkCallback(pbox, 2, NewGamesLink_AddQuickAd, "blitz");
	Text_SetLinkCallback(pbox, 3, NewGamesLink_AddQuickAd, "rapid");
	Text_SetLinkCallback(pbox, 4, NewGamesLink_AddQuickAd, "speed");
	Text_SetLinkCallback(pbox, 5, NewGamesLink_AddQuickAd, "long");
#else
	Text_SetText(pbox, "^bQuick Play:^n   ^l+ Quick (1 min)^l  ^l+ Quick (5 min)^l  ^l+ Quick (15 min)^l  ^l+ Quick (30 min)^l");
	Text_SetLinkCallback(pbox, 1, NewGamesLink_AddQuickAd, "bullet");
	Text_SetLinkCallback(pbox, 2, NewGamesLink_AddQuickAd, "blitz");
	Text_SetLinkCallback(pbox, 3, NewGamesLink_AddQuickAd, "speed");
	Text_SetLinkCallback(pbox, 4, NewGamesLink_AddQuickAd, "long");
#endif
	Box_AddChild(dialog, pbox);
	data->quicklinks = pbox;

	pbox = Box_Create(16, 90, dialog->w - 32, 1, BOX_VISIBLE);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->bgcol = UserInfoFG2;
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(20, 96, dialog->w - 32, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	Text_SetText(pbox, "v ^lFiltering Options^l");
	Text_SetLinkCallback(pbox, 1, NewGamesFilterLinkCallback_OnClick, NULL);
	Box_AddChild(dialog, pbox);
	data->filtertext = pbox;

	/*NewGames_UpdateManageText(dialog);*/
/*
	pbox = Text_Create(dialog->w - 200 - 20, 56 + 11, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	Text_SetText(pbox, "v ^lFiltering Options^l");
	Text_SetLinkCallback(pbox, 1, NewGamesFilterLinkCallback_OnClick, NULL);
	Box_AddChild(dialog, pbox);
	data->filtertext = pbox;
*/
	pbox = Box_Create(16, 116, dialog->w - 32, 40, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeWidth = NewSearchListTitles_OnSizeWidth_Stretch;
	Box_AddChild(dialog, pbox);
	data->listtitles = pbox;

	pbox = Box_Create(16, 116, dialog->w - 32, 80, BOX_TRANSPARENT);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(dialog, pbox);
	data->filtersbox = pbox;

	pbox = Box_Create(8, 2, 80, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_SetText(pbox, "Variant");
	Box_AddChild(data->filtersbox, pbox);

	pbox = ComboBox_Create(100, 0, 100, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry(pbox, _("Standard"));
	ComboBox_AddEntry(pbox, _("Chess960"));
	/*
	ComboBox_AddEntry(subbox, "Loser's");
	*/
	ComboBox_AddEntry(pbox, _("Atomic"));
	ComboBox_AddEntry(pbox, _("Any"));
	ComboBox_SetSelection(pbox, _("Any"));
	ComboBox_SetOnSelection(pbox, NewGamesFilter_OnVariantCombo);
	data->variantcombo = pbox;
	Box_AddChild(data->filtersbox, pbox);

	pbox = Box_Create(8, 27, 100, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_SetText(pbox, "Opponent Rating");
	Box_AddChild(data->filtersbox, pbox);

	pbox = ComboBox_Create(100, 25, 100, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry(pbox, "+/- 50");
	ComboBox_AddEntry(pbox, "+/- 100");
	ComboBox_AddEntry(pbox, "+/- 150");
	ComboBox_AddEntry(pbox, "+/- 200");
	ComboBox_AddEntry(pbox, "+/- 250");
	ComboBox_AddEntry(pbox, "+/- 300");
	ComboBox_AddEntry(pbox, "+/- 350");
	ComboBox_AddEntry(pbox, "+/- 400");
	ComboBox_AddEntry(pbox, "+/- 450");
	ComboBox_AddEntry(pbox, "+/- 500");
	ComboBox_AddEntry(pbox, _("+/- Any"));
	ComboBox_SetSelection(pbox, _("+/- Any"));
	ComboBox_SetOnSelection(pbox, NewGamesFilter_OnRatingCombo);
	data->ratingcombo = pbox;
	Box_AddChild(data->filtersbox, pbox);

	pbox = Box_Create(8, 52, 100, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_SetText(pbox, "Time Control");
	Box_AddChild(data->filtersbox, pbox);

	pbox = ComboBox_Create(100, 50, 100, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry(pbox, _("Long"));
	ComboBox_AddEntry(pbox, _("Speed"));
#ifdef CHESSPARK_RAPIDTIMECONTROL
	ComboBox_AddEntry(pbox, _("Rapid"));
#endif
	ComboBox_AddEntry(pbox, _("Blitz"));
	ComboBox_AddEntry(pbox, _("Bullet"));
	ComboBox_AddEntry(pbox, _("Any"));
	ComboBox_SetSelection(pbox, _("Any"));
	ComboBox_SetOnSelection(pbox, NewGamesFilter_OnTimeControlCombo);
	data->timecombo = pbox;
	Box_AddChild(data->filtersbox, pbox);

	pbox = CheckBox_Create(220, 2, BOX_VISIBLE);
	CheckBox_SetOnHit(pbox, NewGamesFilter_OnRatedCheckHit);
	Box_AddChild(data->filtersbox, pbox);
	data->ratedcheck = pbox;

	pbox = CheckBoxLinkedText_Create(235, 2, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->ratedcheck);
	pbox->fgcol = RGB(0, 0, 0);
	Box_SetText(pbox, _("Only Rated Games"));
	Box_AddChild(data->filtersbox, pbox);

	pbox = CheckBox_Create(220, 18, BOX_VISIBLE);
	CheckBox_SetOnHit(pbox, NewGamesFilter_OnAICheckHit);
	Box_AddChild(data->filtersbox, pbox);
	data->aicheck = pbox;

	pbox = CheckBoxLinkedText_Create(235, 18, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->aicheck);
	pbox->fgcol = RGB(0, 0, 0);
	Box_SetText(pbox, _("Include Computer Opponents"));
	Box_AddChild(data->filtersbox, pbox);

	pbox = CheckBox_Create(220, 34, BOX_VISIBLE);
	CheckBox_SetOnHit(pbox, NewGamesFilter_OnQuickFilterCheckHit);
	Box_AddChild(data->filtersbox, pbox);
	data->quickfiltercheck = pbox;

	pbox = CheckBoxLinkedText_Create(235, 34, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->quickfiltercheck);
	pbox->fgcol = RGB(0, 0, 0);
	Box_SetText(pbox, _("Apply filters to quick links"));
	Box_AddChild(data->filtersbox, pbox);
	data->quickfilterchecktext = pbox;

	pbox = Box_Create(220, 50, 100, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_SetText(pbox, _("Show My Games:"));
	Box_AddChild(data->filtersbox, pbox);
	data->myadstext = pbox;

	pbox = ComboBox_Create(320, 50, 60, 20, BOX_VISIBLE | BOX_BORDER);
	ComboBox_AddEntry(pbox, _("Yes"));
	ComboBox_AddEntry(pbox, _("No"));
	ComboBox_AddEntry(pbox, _("Only"));
	ComboBox_SetSelection(pbox, _("Yes"));
	ComboBox_SetOnSelection(pbox, NewGamesFilter_OnMyAdsCombo);
	Box_AddChild(data->filtersbox, pbox);
	data->myadscombo = pbox;

	pbox = List_Create(16, 157, dialog->w - 32, dialog->h - 137 - 16 - 32 - 20, BOX_VISIBLE, FALSE);
	List_SetEntrySelectable(pbox, 0);
	pbox->bgcol = TabBG2;
	pbox->fgcol = TabFG2;
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(dialog, pbox);
	data->list = pbox;

	pbox = CheckBox_Create(20, dialog->h - 32, BOX_VISIBLE);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	CheckBox_SetChecked(pbox, Model_GetOption(OPTION_NOGAMESEARCHONLOGIN));
	CheckBox_SetOnHit(pbox, NewGamesDontShowAgain_OnCheckHit);
	Box_AddChild(dialog, pbox);
	data->dontshowagaincheck = pbox;

	pbox = CheckBoxLinkedText_Create(40, dialog->h - 32, 100, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->dontshowagaincheck);
	pbox->fgcol = UserInfoFG2;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_SetText(pbox, _("Don't show on login"));
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(dialog->w - GrabberWidth, dialog->h - GrabberHeight, GrabberWidth, GrabberHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->img = ImageMgr_GetImage("windowResizeHandle.png");
	pbox->OnSizeWidth  = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_AddChild(dialog, pbox);
#endif
	Box_CreateWndCustom(dialog, _("Game Finder"), roster->hwnd);

	dialog->OnDestroy = NewGames_OnDestroy;

	NewGamesLinkCallback_OnClick(dialog, "play");

	return dialog;
}

void NewGames_ClearSearch(struct Box_s *dialog, char *node)
{
	struct newgamesdata_s *data = dialog->boxdata;

	if (node && data->currentpagetype && stricmp(node, data->currentpagetype) != 0)
	{
		return;
	}

	List_RemoveAllEntries(data->list);
	List_ScrollToTop(data->list);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

struct tempplaydata_s
{
	char *id;
	char *name;
};

void NewGameGameEntry_RealPlay(struct tempplaydata_s *tpd)
{
	Ctrl_PlayAd(tpd->id);

	{
		char titletext[256];
		char dialogtext[256];

		i18n_stringsub(titletext, 256, _("Responding to ad by %1"), tpd->name);
		i18n_stringsub(dialogtext, 256, _("Joining game..."));

		View_PopupGameWaitingDialog(titletext, dialogtext);
	}
}

void NewGameGameEntryWarning_OnOK(struct Box_s *warningbox, struct tempplaydata_s *tpd)
{
	NewGameGameEntry_RealPlay(tpd);
	Box_Destroy(warningbox);

	free(tpd->id);
	free(tpd->name);
	free(tpd);
}

void NewGameGameEntryWarning_OnCancel(struct Box_s *warningbox, struct tempplaydata_s *tpd)
{
	Box_Destroy(warningbox);

	free(tpd->id);
	free(tpd->name);
	free(tpd);
}

void NewGameGameEntryWarning_OnDontShow(struct Box_s *warningbox, char *name)
{
	Model_SetOption(OPTION_HIDEVARIANTWARNING, 1, NULL);
}

void NewGameGameEntry_OnPlayHit(struct Box_s *pbox)
{
	struct Box_s *entrybox = pbox->parent;
	struct newgamesgamedata_s *gdata = entrybox->boxdata;
	struct tempplaydata_s *tpd;

	tpd = malloc(sizeof(*tpd));
	memset(tpd, 0, sizeof(*tpd));

	tpd->id = strdup(gdata->id);
	tpd->name = strdup(gdata->showname);

	if (gdata->info->variant && stricmp(gdata->info->variant, "Standard") != 0 && !Model_GetOption(OPTION_HIDEVARIANTWARNING))
	{
		char txt[512];

		i18n_stringsub(txt, 512, _("This game will be played with %1, a non-standard game variant.\n\nIf you wish to play this game, press OK.  If you wish to cancel, press Cancel."), gdata->info->variant);
                AutoDialog_Create2(Box_GetRoot(pbox), 500, _("Variant Warning!"), txt , _("Cancel"), _("OK"), NewGameGameEntryWarning_OnCancel, NewGameGameEntryWarning_OnOK, tpd, NewGameGameEntryWarning_OnDontShow, "VariantWarning");
	}
	else
	{
		NewGameGameEntry_RealPlay(tpd);
		free(tpd->id);
		free(tpd->name);
		free(tpd);
	}
}

void NewGameGameEntry_OnReconveneHit(struct Box_s *pbox)
{
	struct Box_s *entrybox = pbox->parent;
	struct newgamesgamedata_s *gdata = entrybox->boxdata;
	struct gamesearchinfo_s *info = Info_DupeGameSearchInfo(gdata->info);

	Model_PopupReconvene(gdata->info->gameid, info);
}

void NewGameGameEntry_OnWatchHit(struct Box_s *pbox)
{
	struct Box_s *entrybox = pbox->parent;
	struct newgamesgamedata_s *gdata = entrybox->boxdata;

	Ctrl_WatchGame(gdata->info->gameid, 0);
}

void NewGameGameEntry_OnEditHit(struct Box_s *pbox)
{
	struct Box_s *entrybox = pbox->parent;
	struct newgamesgamedata_s *gdata = entrybox->boxdata;

	View_PopupGameCreateAdDialog(gdata->info);
}

void NewGameGameEntry_OnRemoveHit(struct Box_s *pbox)
{
	struct Box_s *entrybox = pbox->parent;
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct newgamesdata_s *data = dialog->boxdata;
	struct newgamesgamedata_s *gdata = entrybox->boxdata;

	NamedList_RemoveByName(&(data->openads), gdata->info->gameid);

	Ctrl_RemoveGameAd(gdata->id);
	List_RemoveEntryByName(data->list, gdata->id, NULL);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);

	data->numgameads--;
	/*NewGames_UpdateManageText(dialog);*/
}

static void JidLink_ShowMenu(struct Box_s *pbox, char *jid, int x, int y)
{
	Menu_PopupProfileMenu(pbox->parent, jid, x, y, NULL, NULL);
}

void NewGamesListEntry_Flash(struct Box_s *entrybox, int *flashstate)
{
	(*flashstate)++;

	if (*flashstate == 5)
	{
		entrybox->flags |= BOX_TRANSPARENT;
		Box_Repaint(entrybox->parent);
		Box_RemoveTimedFunc(entrybox, NewGamesListEntry_Flash, 300);
		return;
	}

	if (*flashstate % 2)
	{
		entrybox->flags |= BOX_TRANSPARENT;
	}
	else
	{
		entrybox->flags &= ~BOX_TRANSPARENT;
	}

	Box_Repaint(entrybox->parent);
}

void NewGamesListEntry_SlideIn(struct Box_s *entrybox, void *userdata)
{
	entrybox->xoffset -= 40;
	if (entrybox->xoffset < 0)
	{
		entrybox->xoffset = 0;
		Box_RemoveTimedFunc(entrybox, NewGamesListEntry_SlideIn, 17);
		if ((entrybox->flags & BOX_TRANSPARENT) == 0)
		{
			int *flashstate = malloc(sizeof(*flashstate));
			*flashstate = 0;
			Box_AddTimedFunc(entrybox, NewGamesListEntry_Flash, flashstate, 300);
		}
	}
	Box_Repaint(entrybox->parent);
}

struct slideoutdata_s
{
	struct Box_s *list;
	char *id;
};

void NewGamesListEntry_SlideOut(struct Box_s *entrybox, struct slideoutdata_s *sod)
{
	entrybox->xoffset -= 40;
	if (entrybox->xoffset < -entrybox->w)
	{
		Box_RemoveTimedFunc(entrybox, NewGamesListEntry_SlideOut, 17);
		List_RemoveEntryByName(sod->list, sod->id, NULL);
		List_RedoEntries(sod->list);
	}
	Box_Repaint(sod->list);
}

void NewGames_AddPlayGame(struct Box_s *dialog, char *itemid, char *node, char *jid, struct gamesearchinfo_s *info)
{
	struct newgamesdata_s *data = dialog->boxdata;
	struct newgamesgamedata_s *gdata;
	struct Box_s *entrybox, *subbox;
	struct namedlist_s **entry;
	struct newgamescategorydata_s *cdata;
	char *variant = NULL;
	char *timecontrolrange = NULL;
	int correspondence;
	char txt[256];
	int adstaff = 0;
	int adhelper = 0;
	int self = 0;
	COLORREF gametypecol;

	if (info->offline)
	{
		return;
	}

	{
		char *barejid1 = Jid_Strip(info->adplayer->jid);
		char *barejid2 = Jid_Strip(Model_GetLoginJid());

		if (stricmp(barejid1, barejid2) == 0)
		{
			self = 1;
		}
	}

	gdata = malloc(sizeof(*gdata));
	memset(gdata, 0, sizeof(*gdata));

	gdata->id = strdup(itemid);

	entrybox = Box_Create(0, 0, data->list->w - 16, 25, BOX_VISIBLE | (self ? 0 : BOX_TRANSPARENT));
	entrybox->bgcol = TabBG4;
	entrybox->OnSizeWidth = NewGamesListEntry_OnSizeWidth_Stretch;

	/*
		NewGames_AddListCategory(dialog, "Game Type", 85, 0);
		NewGames_AddListCategory(dialog, "Opponent Name", dialog->w - 375, 1);
		NewGames_AddListCategory(dialog, "Opponent Rating", 70, 0);
		NewGames_AddListCategory(dialog, "Rated Game", 50, 0);
		NewGames_AddListCategory(dialog, "Time Control", 70, 0);
		NewGames_AddListCategory(dialog, NULL, 50, 0);
		NewGames_AddListCategory(dialog, NULL, 22, 0);
	*/

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
		else if (stricmp(info->variant, "crazyhouse") == 0)
		{
			variant = _("Crazyhouse");
		}
		else if (stricmp(info->variant, "losers") == 0)
		{
			variant = _("Loser's");
		}
		else if (stricmp(info->variant, "checkers") == 0)
		{
			variant = _("Checkers");
		}
	}

	correspondence = info->correspondence;
	timecontrolrange = Info_TimeControlToCategory(info->timecontrol);

	gdata->info = Info_DupeGameSearchInfo(info);
	gdata->showname = strdup(Model_GetFriendNick(info->adplayer->jid));
	gdata->gametype = strdup(correspondence ? _("(Correspondence)") : (variant ? variant : timecontrolrange));
	if (variant && stricmp(variant, "standard") != 0)
	{
		gametypecol = RGB(136, 0, 0);
	}
	else
	{
		gametypecol = RGB(0, 0, 0);
	}
	if (!(info->adplayer->rating) || (info->adplayer->rating)[0] == '\0')
	{
		gdata->showrating = -1;
	}
	else
	{
		sscanf(info->adplayer->rating, "%d", &(gdata->showrating));
	}
	gdata->estgametime = Info_TimeControlToEstimatedTime(info->timecontrol);
	gdata->side = info->colorpreference;
	{
		struct namedlist_s *entry = info->adplayer->roles;

		while (entry)
		{
			if (entry->name)
			{
				if (stricmp(entry->name, "staff") == 0)
				{
					adstaff = 1;
				}
				else if (stricmp(entry->name, "helper") == 0)
				{
					adhelper = 1;
				}
			}
			entry = entry->next;
		}
	}

	entrybox->boxdata = gdata;

	entry = NamedList_GetByName(&(data->playcategories), "Game Type");
	cdata = (*entry)->data;
	subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_SetText(subbox, gdata->gametype);
	subbox->fgcol = gametypecol;
	Box_AddChild(entrybox, subbox);

	entry = NamedList_GetByName(&(data->playcategories), "Opponent Name");
	cdata = (*entry)->data;
	subbox = Text_Create(cdata->x + 8, 4, cdata->w - 20, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_ELLIPSIS);
	subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	Text_SetLinkColor(subbox, CR_DkOrange);
	{
		struct namedlist_s *entry = info->adplayer->titles;
		txt[0] = '\0';

		if (entry)
		{
			strcat(txt, entry->name);
			strcat(txt, " ");
			entry = entry->next;
		}

		strcat(txt, "^l");
		if (adstaff)
		{
			strcat(txt, "^2");
		}
		else if (adhelper)
		{
			strcat(txt, "^7");
		}
		else if (info->adplayer->membertype && stricmp(info->adplayer->membertype, "pro") == 0)
		{
			strcat(txt, "^8");
		}
		else if (info->adplayer->notactivated)
		{
			strcat(txt, "^9");
		}
		strcat(txt, Model_GetFriendNick(info->adplayer->jid));
		strcat(txt, "^l");

		entry = info->adplayer->roles;

		if (entry)
		{
			strcat(txt, ", ");
		}

		while (entry)
		{
			strcat(txt, entry->name);
			if (entry->next)
			{
				strcat(txt, ", ");
			}
			entry = entry->next;
		}

		Text_SetText(subbox, txt);
	}
	Text_SetLinkCallback(subbox, 1, ViewLink_ShowProfile,  strdup(info->adplayer->jid));
	Text_SetRLinkCallback(subbox, 1, JidLink_ShowMenu, strdup(info->adplayer->jid));
	Log_Write(0, "text %d jid %s\n", subbox, info->adplayer->jid);
	Box_AddChild(entrybox, subbox);

	entry = NamedList_GetByName(&(data->playcategories), "Opponent Rating");
	cdata = (*entry)->data;
	subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	if (gdata->showrating == -1)
	{
                Box_SetText(subbox, _("Unrated"));
	}
	else
	{
		sprintf(txt, "%d", gdata->showrating);
		Box_SetText(subbox, txt);
	}
	Box_AddChild(entrybox, subbox);

	entry = NamedList_GetByName(&(data->playcategories), "Rated Game");
	cdata = (*entry)->data;
	subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_SetText(subbox, info->rated ? _("Rated") : _("Unrated"));
	Box_AddChild(entrybox, subbox);

	entry = NamedList_GetByName(&(data->playcategories), "Time Control");
	cdata = (*entry)->data;
	subbox = Text_Create(cdata->x + 8, 4, cdata->w, 17, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	{
		char *txt = Info_TimeControlsToShortText(gdata->info->timecontrol, gdata->info->blacktimecontrol);

		if (txt && strlen(txt) > 10)
		{
			Text_SetText(subbox, "^lCustom^l");
			Text_SetLinkTooltip(subbox, 1, txt);
		}
		else
		{
			Text_SetText(subbox, txt);
		}
	}
	Box_AddChild(entrybox, subbox);

	entry = NamedList_GetByName(&(data->playcategories), "Your Color");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		if (info->colorpreference == 2)
		{
			Box_SetText(subbox, _("White"));
		}
		else if (info->colorpreference == 1)
		{
			Box_SetText(subbox, _("Black"));
		}
		else
		{
			Box_SetText(subbox, _("Random"));
		}
		Box_AddChild(entrybox, subbox);
	}
	
/*
	if (subbox->h + 8 > entrybox->h)
	{
		entrybox->h = subbox->h + 8;
	}
*/
	if (self)
	{
		entry = NamedList_GetByName(&(data->playcategories), NULL);
		cdata = (*entry)->data;
		subbox = StdButton_Create(cdata->x, (entrybox->h - 23) / 2, cdata->w, _("Remove"), 1);
		Button2_SetOnButtonHit(subbox, NewGameGameEntry_OnRemoveHit);
		Box_AddChild(entrybox, subbox);
	}
	else
	{
		entry = NamedList_GetByName(&(data->playcategories), NULL);
		cdata = (*entry)->data;
		subbox = StdButton_Create(cdata->x, (entrybox->h - 23) / 2, cdata->w, _("Play"), 1);
		Button2_SetOnButtonHit(subbox, NewGameGameEntry_OnPlayHit);
		Box_AddChild(entrybox, subbox);
	}

	/* funky animation, starto! */
	entrybox->xoffset = entrybox->w;

	List_AddEntry(data->list, itemid, NULL, entrybox);
	List_RedoEntries(data->list);
	Box_AddTimedFunc(entrybox, NewGamesListEntry_SlideIn, NULL, 17);

	Box_Repaint(data->list);
}

void NewGames_AddWatchGame(struct Box_s *dialog, char *itemid, char *node, struct gamesearchinfo_s *info)
{
	struct newgamesdata_s *data = dialog->boxdata;
	struct newgamesgamedata_s *gdata;
	struct Box_s *entrybox, *subbox;
	struct namedlist_s **entry;
	struct newgamescategorydata_s *cdata;
	char *variant = NULL;
	char *timecontrolrange = NULL;
	int correspondence;
	char txt[256];
	int whitestaff = 0, whitehelper = 0, blackstaff = 0, blackhelper = 0;
	COLORREF gametypecol;

	if (info->offline)
	{
		return;
	}

	if (List_GetEntryBox(data->list, info->gameid, NULL))
	{
		return;
	}

	gdata = malloc(sizeof(*gdata));
	memset(gdata, 0, sizeof(*gdata));

	gdata->id = strdup(itemid);

	entrybox = Box_Create(0, 0, data->list->w, 25, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->OnSizeWidth = NewGamesListEntry_OnSizeWidth_Stretch;
	/*
		NewGames_AddListCategory(dialog, "Game Type", 85, 0);
		NewGames_AddListCategory(dialog, "White", (dialog->w - 320) / 2, 1);
		NewGames_AddListCategory(dialog, "White Rating", 60, 0);
		NewGames_AddListCategory(dialog, "Black", (dialog->w - 320) / 2, 1);
		NewGames_AddListCategory(dialog, "Black Rating", 60, 0);
		NewGames_AddListCategory(dialog, NULL, 60, 0);
		NewGames_AddListCategory(dialog, NULL, 22, 0);
	*/

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
		else if (stricmp(info->variant, "crazyhouse") == 0)
		{
			variant = _("Crazyhouse");
		}
		else if (stricmp(info->variant, "losers") == 0)
		{
			variant = _("Loser's");
		}
		else if (stricmp(info->variant, "checkers") == 0)
		{
			variant = _("Checkers");
		}
	}

	correspondence = info->correspondence;
	timecontrolrange = Info_TimeControlToCategory(info->timecontrol);

	gdata->info = Info_DupeGameSearchInfo(info);
	gdata->showname = strdup(Model_GetFriendNick(info->white->jid));
	gdata->showname2 = strdup(Model_GetFriendNick(info->black->jid));
	gdata->gametype = strdup(correspondence ? _("(Correspondence)") : (variant ? variant : timecontrolrange));
	if (variant && stricmp(variant, "standard") != 0)
	{
		gametypecol = RGB(136, 0, 0);
	}
	else
	{
		gametypecol = RGB(0, 0, 0);
	}
	if (!(info->white->rating) || (info->white->rating)[0] == '\0')
	{
		gdata->showrating = -1;
	}
	else
	{
		sscanf(info->white->rating, "%d", &(gdata->showrating));
	}
	if (!(info->black->rating) || (info->black->rating)[0] == '\0')
	{
		gdata->showrating2 = -1;
	}
	else
	{
		sscanf(info->black->rating, "%d", &(gdata->showrating2));
	}
	gdata->estgametime = Info_TimeControlToEstimatedTime(info->timecontrol);
	{
		struct namedlist_s *entry = info->white->roles;

		while (entry)
		{
			if (entry->name)
			{
				if (stricmp(entry->name, "staff") == 0)
				{
					whitestaff = 1;
				}
				else if (stricmp(entry->name, "helper") == 0)
				{
					whitehelper = 1;
				}
			}
			entry = entry->next;
		}

		entry = info->black->roles;

		while (entry)
		{
			if (entry->name)
			{
				if (stricmp(entry->name, "staff") == 0)
				{
					blackstaff = 1;
				}
				else if (stricmp(entry->name, "helper") == 0)
				{
					blackhelper = 1;
				}
			}
			entry = entry->next;
		}
	}

	entrybox->boxdata = gdata;

	entry = NamedList_GetByName(&(data->playcategories), "Game Type");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->fgcol = gametypecol;
		Box_SetText(subbox, gdata->gametype);
		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), "White");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Text_Create(cdata->x + 8, 4, cdata->w - 20, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_ELLIPSIS);
		Text_SetLinkColor(subbox, CR_DkOrange);
		{
			struct namedlist_s *entry = info->white->titles;
			txt[0] = '\0';

			if (entry)
			{
				strcat(txt, entry->name);
				strcat(txt, " ");
				entry = entry->next;
			}

			strcat(txt, "^l");
			if (whitestaff)
			{
				strcat(txt, "^2");
			}
			else if (whitehelper)
			{
				strcat(txt, "^7");
			}
			else if (info->white->membertype && stricmp(info->white->membertype, "pro") == 0)
			{
				strcat(txt, "^8");
			}
			else if (info->white->notactivated)
			{
				strcat(txt, "^9");
			}
			strcat(txt, Model_GetFriendNick(info->white->jid));
			strcat(txt, "^l");

			entry = info->white->roles;

			if (entry)
			{
				strcat(txt, ", ");
			}

			while (entry)
			{
				strcat(txt, entry->name);
				if (entry->next)
				{
					strcat(txt, ", ");
				}
				entry = entry->next;
			}

			Text_SetText(subbox, txt);
		}
		Text_SetLinkCallback(subbox, 1, ViewLink_ShowProfile,  strdup(info->white->jid));
		Text_SetRLinkCallback(subbox, 1, JidLink_ShowMenu, strdup(info->white->jid));
		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), "White Rating");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		if (gdata->showrating == -1)
		{
			Box_SetText(subbox, _("Unrated"));
		}
		else
		{
			sprintf(txt, "%d", gdata->showrating);
			Box_SetText(subbox, txt);
		}
		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), "Black");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Text_Create(cdata->x + 8, 4, cdata->w - 20, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_ELLIPSIS);
		Text_SetLinkColor(subbox, CR_DkOrange);
		{
			struct namedlist_s *entry = info->black->titles;
			txt[0] = '\0';

			if (entry)
			{
				strcat(txt, entry->name);
				strcat(txt, " ");
				entry = entry->next;
			}

			strcat(txt, "^l");
			if (blackstaff)
			{
				strcat(txt, "^2");
			}
			else if (blackhelper)
			{
				strcat(txt, "^7");
			}
			else if (info->black->membertype && stricmp(info->black->membertype, "pro") == 0)
			{
				strcat(txt, "^8");
			}
			else if (info->black->notactivated)
			{
				strcat(txt, "^9");
			}
			strcat(txt, Model_GetFriendNick(info->black->jid));
			strcat(txt, "^l");

			entry = info->black->roles;

			if (entry)
			{
				strcat(txt, ", ");
			}

			while (entry)
			{
				strcat(txt, entry->name);
				if (entry->next)
				{
					strcat(txt, ", ");
				}
				entry = entry->next;
			}

			Text_SetText(subbox, txt);
		}
		Text_SetLinkCallback(subbox, 1, ViewLink_ShowProfile,  strdup(info->black->jid));
		Text_SetRLinkCallback(subbox, 1, JidLink_ShowMenu, strdup(info->black->jid));
		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), "Black Rating");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		if (gdata->showrating2 == -1)
		{
			Box_SetText(subbox, _("Unrated"));
		}
		else
		{
			sprintf(txt, "%d", gdata->showrating2);
			Box_SetText(subbox, txt);
		}
		Box_AddChild(entrybox, subbox);
	}
/*
	entry = NamedList_GetByName(&(data->playcategories), "Rated Game");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		Box_SetText(subbox, info->rated ? _("Rated") : _("Unrated"));
		Box_AddChild(entrybox, subbox);
	}
*/
	entry = NamedList_GetByName(&(data->playcategories), "Time Control");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Text_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		{
			char *txt = Info_TimeControlsToShortText(gdata->info->timecontrol, gdata->info->blacktimecontrol);

			if (txt && strlen(txt) > 10)
			{
				Text_SetText(subbox, "^lCustom^l");
				Text_SetLinkTooltip(subbox, 1, txt);
			}
			else
			{
				Text_SetText(subbox, txt);
			}
		}
		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), NULL);
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = StdButton_Create(cdata->x, 1, cdata->w, _("Watch"), 1);
		Button2_SetOnButtonHit(subbox, NewGameGameEntry_OnWatchHit);
		Box_AddChild(entrybox, subbox);
	}

	List_AddEntry(data->list, info->gameid, NULL, entrybox);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

void NewGames_AddAdjournedGame(struct Box_s *dialog, char *itemid, char *node, char *jid, struct gamesearchinfo_s *info)
{
	struct newgamesdata_s *data = dialog->boxdata;
	struct newgamesgamedata_s *gdata;
	struct Box_s *entrybox, *subbox;
	struct namedlist_s **entry;
	struct newgamescategorydata_s *cdata;
	char *variant = NULL;
	char *timecontrolrange = NULL;
	char *localjid = Jid_Strip(Model_GetLoginJid());
	int correspondence;
	int opponentwhite = 0;
	int adstaff = 0, adhelper = 0;
	char txt[256];
	COLORREF gametypecol;

	if (info->offline)
	{
		return;
	}

	if (stricmp(localjid, info->black->jid) == 0)
	{
		opponentwhite = 1;
	}

	gdata = malloc(sizeof(*gdata));
	memset(gdata, 0, sizeof(*gdata));

	gdata->id = strdup(itemid);

	entrybox = Box_Create(0, 0, data->list->w, 25, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->OnSizeWidth = NewGamesListEntry_OnSizeWidth_Stretch;

	/*
		NewGames_AddListCategory(dialog, "Date", 20, 0);
		NewGames_AddListCategory(dialog, "Your Color", 50, 0);
		NewGames_AddListCategory(dialog, "Rated Game", 50, 0);
		NewGames_AddListCategory(dialog, "Game Type", 85, 0);
		NewGames_AddListCategory(dialog, "Opponent Name", 115, 1);
		NewGames_AddListCategory(dialog, "Opponent Rating", 70, 0);
		NewGames_AddListCategory(dialog, NULL, 50, 0);
		NewGames_AddListCategory(dialog, NULL, 22, 0);
	*/

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
		else if (stricmp(info->variant, "crazyhouse") == 0)
		{
			variant = _("Crazyhouse");
		}
		else if (stricmp(info->variant, "losers") == 0)
		{
			variant = _("Loser's");
		}
		else if (stricmp(info->variant, "checkers") == 0)
		{
			variant = _("Checkers");
		}
	}

	correspondence = info->correspondence;
	timecontrolrange = Info_TimeControlToCategory(info->timecontrol);

	gdata->info = Info_DupeGameSearchInfo(info);
	gdata->gametype = strdup(correspondence ? _("(Correspondence)") : (variant ? variant : timecontrolrange));
	if (variant && stricmp(variant, "standard") != 0)
	{
		gametypecol = RGB(136, 0, 0);
	}
	else
	{
		gametypecol = RGB(0, 0, 0);
	}
	if (opponentwhite)
	{
		struct namedlist_s *entry;
		gdata->showname = strdup(Model_GetFriendNick(info->white->jid));
		if (!(info->white->rating) || (info->white->rating)[0] == '\0')
		{
			gdata->showrating = -1;
		}
		else
		{
			sscanf(info->white->rating, "%d", &(gdata->showrating));
		}
		entry = info->white->roles;

		while (entry)
		{
			if (entry->name)
			{
				if (stricmp(entry->name, "staff") == 0)
				{
					adstaff = 1;
				}
				else if (stricmp(entry->name, "helper") == 0)
				{
					adhelper = 1;
				}
			}
			entry = entry->next;
		}
	}
	else
	{
		struct namedlist_s *entry;
		gdata->showname = strdup(Model_GetFriendNick(info->black->jid));
		if (!(info->black->rating) || (info->black->rating)[0] == '\0')
		{
			gdata->showrating = -1;
		}
		else
		{
			sscanf(info->black->rating, "%d", &(gdata->showrating));
		}
		entry = info->black->roles;

		while (entry)
		{
			if (entry->name)
			{
				if (stricmp(entry->name, "staff") == 0)
				{
					adstaff = 1;
				}
				else if (stricmp(entry->name, "helper") == 0)
				{
					adhelper = 1;
				}
			}
			entry = entry->next;
		}
	}
	gdata->side = !opponentwhite;
	gdata->estgametime = Info_TimeControlToEstimatedTime(info->timecontrol);

	entrybox->boxdata = gdata;
	
	entry = NamedList_GetByName(&(data->playcategories), "Date");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		Box_SetText(subbox, Info_ConvertTimestampToLongDateAndTime(info->date));
		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), "Your Color");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		if (opponentwhite)
		{
			Box_SetText(subbox, _("Black"));
		}
		else
		{
			Box_SetText(subbox, _("White"));
		}
		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), "Rated Game");
	cdata = (*entry)->data;
	subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_SetText(subbox, info->rated ? _("Rated") : _("Unrated"));
	Box_AddChild(entrybox, subbox);

	entry = NamedList_GetByName(&(data->playcategories), "Game Type");
	cdata = (*entry)->data;
	subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = gametypecol;
	Box_SetText(subbox, gdata->gametype);
	Box_AddChild(entrybox, subbox);

	entry = NamedList_GetByName(&(data->playcategories), "Opponent Name");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Text_Create(cdata->x + 8, 4, cdata->w - 20, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_ELLIPSIS);
		Text_SetLinkColor(subbox, CR_DkOrange);
		if (opponentwhite)
		{
			struct namedlist_s *entry = info->white->titles;
			txt[0] = '\0';

			if (entry)
			{
				strcat(txt, entry->name);
				strcat(txt, " ");
				entry = entry->next;
			}

			strcat(txt, "^l");
			if (adstaff)
			{
				strcat(txt, "^2");
			}
			else if (adhelper)
			{
				strcat(txt, "^7");
			}
			else if (info->white->membertype && stricmp(info->white->membertype, "pro") == 0)
			{
				strcat(txt, "^8");
			}
			else if (info->white->notactivated)
			{
				strcat(txt, "^9");
			}
			strcat(txt, Model_GetFriendNick(info->white->jid));
			strcat(txt, "^l");

			entry = info->white->roles;

			if (entry)
			{
				strcat(txt, ", ");
			}

			while (entry)
			{
				strcat(txt, entry->name);
				if (entry->next)
				{
					strcat(txt, ", ");
				}
				entry = entry->next;
			}

			Text_SetText(subbox, txt);
			Text_SetLinkCallback(subbox, 1, ViewLink_ShowProfile,  strdup(info->white->jid));
			Text_SetRLinkCallback(subbox, 1, JidLink_ShowMenu, strdup(info->white->jid));
		}
		else
		{
			struct namedlist_s *entry = info->black->titles;
			txt[0] = '\0';

			if (entry)
			{
				strcat(txt, entry->name);
				strcat(txt, " ");
				entry = entry->next;
			}

			strcat(txt, "^l");
			if (adstaff)
			{
				strcat(txt, "^2");
			}
			else if (adhelper)
			{
				strcat(txt, "^7");
			}
			else if (info->black->membertype && stricmp(info->black->membertype, "pro") == 0)
			{
				strcat(txt, "^8");
			}
			else if (info->black->notactivated)
			{
				strcat(txt, "^9");
			}
			strcat(txt, Model_GetFriendNick(info->black->jid));
			strcat(txt, "^l");

			entry = info->black->roles;

			if (entry)
			{
				strcat(txt, ", ");
			}

			while (entry)
			{
				strcat(txt, entry->name);
				if (entry->next)
				{
					strcat(txt, ", ");
				}
				entry = entry->next;
			}

			Text_SetText(subbox, txt);
			Text_SetLinkCallback(subbox, 1, ViewLink_ShowProfile,  strdup(info->black->jid));
			Text_SetRLinkCallback(subbox, 1, JidLink_ShowMenu, strdup(info->black->jid));
		}
	}
	Box_AddChild(entrybox, subbox);

	entry = NamedList_GetByName(&(data->playcategories), "Opponent Rating");
	cdata = (*entry)->data;
	subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	if (gdata->showrating == -1)
	{
                Box_SetText(subbox, _("Unrated"));
	}
	else
	{
		sprintf(txt, "%d", gdata->showrating);
		Box_SetText(subbox, txt);
	}
	Box_AddChild(entrybox, subbox);

	entry = NamedList_GetByName(&(data->playcategories), "Time Control");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Text_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		{
			char *txt = Info_TimeControlsToShortText(gdata->info->timecontrol, gdata->info->blacktimecontrol);

			if (txt && strlen(txt) > 10)
			{
				Text_SetText(subbox, "^lCustom^l");
				Text_SetLinkTooltip(subbox, 1, txt);
			}
			else
			{
				Text_SetText(subbox, txt);
			}
		}
		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), NULL);
	cdata = (*entry)->data;
	subbox = StdButton_Create(cdata->x, 1, cdata->w, _("Reconvene"), 1);
	Button2_SetOnButtonHit(subbox, NewGameGameEntry_OnReconveneHit);
	Box_AddChild(entrybox, subbox);

	List_AddEntry(data->list, jid, NULL, entrybox);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

void NewGames_AddMyAdsGame(struct Box_s *dialog, char *itemid, char *node, char *jid, struct gamesearchinfo_s *info)
{
	struct newgamesdata_s *data = dialog->boxdata;
	struct newgamesgamedata_s *gdata;
	struct Box_s *entrybox, *subbox;
	struct namedlist_s **entry;
	struct newgamescategorydata_s *cdata;
	char *variant = NULL;
	char *timecontrolrange = NULL;
	char *localjid = Jid_Strip(Model_GetLoginJid());
	int correspondence;
	COLORREF gametypecol;

	if (info->offline)
	{
		return;
	}

	data->numgameads++;
	NamedList_AddString(&(data->openads), itemid, NULL);
	/*NewGames_UpdateManageText(dialog);*/

	gdata = malloc(sizeof(*gdata));
	memset(gdata, 0, sizeof(*gdata));

	gdata->id = strdup(itemid);

	entrybox = Box_Create(0, 0, data->list->w, 25, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->OnSizeWidth = NewGamesListEntry_OnSizeWidth_Stretch;

	/*
		NewGames_AddListCategory(dialog, "Rated Game", 80);
		NewGames_AddListCategory(dialog, "Game Type", 100);
		NewGames_AddListCategory(dialog, "Your Color", 60);
		NewGames_AddListCategory(dialog, "Rating Filter", 125);
		NewGames_AddListCategory(dialog, "Low", 55);
		NewGames_AddListCategory(dialog, "High", 55);
		NewGames_AddListCategory(dialog, "Time Control", 125);
		NewGames_AddListCategory(dialog, NULL, 75);
		NewGames_AddListCategory(dialog, NULL, 75);
		NewGames_AddListCategory(dialog, NULL, 20);
	*/

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
		else if (stricmp(info->variant, "crazyhouse") == 0)
		{
			variant = _("Crazyhouse");
		}
		else if (stricmp(info->variant, "losers") == 0)
		{
			variant = _("Loser's");
		}
		else if (stricmp(info->variant, "checkers") == 0)
		{
			variant = _("Checkers");
		}
	}

	correspondence = info->correspondence;
	timecontrolrange = Info_TimeControlToCategory(info->timecontrol);

	gdata->info = Info_DupeGameSearchInfo(info);
	gdata->gametype = strdup(correspondence ? _("(Correspondence)") : (variant ? variant : timecontrolrange));
	if (variant && stricmp(variant, "standard") != 0)
	{
		gametypecol = RGB(136, 0, 0);
	}
	else
	{
		gametypecol = RGB(0, 0, 0);
	}
	gdata->estgametime = Info_TimeControlToEstimatedTime(info->timecontrol);
	gdata->side = info->colorpreference;

	if (info->relativerating)
	{
		gdata->showrating = info->relativerating;
	}
	if (info->limit)
	{
		if (stricmp(info->limit->type, "rating") == 0)
		{
			gdata->limittype = _("rating");
		}
		gdata->showrating = info->limit->low;
		gdata->showrating2 = info->limit->high;
	}
	else
	{
		gdata->limittype = NULL;
		gdata->showrating = -1;
		gdata->showrating2 = -1;
	}

	entrybox->boxdata = gdata;
	
	entry = NamedList_GetByName(&(data->playcategories), "Rated Game");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		Box_SetText(subbox, info->rated ? _("Rated") : _("Unrated"));
		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), "Game Type");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->fgcol = gametypecol;
		Box_SetText(subbox, gdata->gametype);
		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), "Your Color");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		if (gdata->side == 1)
		{
			Box_SetText(subbox, _("Black"));
		}
		else if (gdata->side == 2)
		{
			Box_SetText(subbox, _("White"));
		}
		else
		{
			Box_SetText(subbox, _("Either"));
		}

		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), "Rating Filter");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
		if (gdata->info->relativerating)
		{
			char txt[256];
			char txt2[256];

			sprintf(txt, "%d", gdata->info->relativerating);
			i18n_stringsub(txt2, 256, _("Rating +/- %1"), txt);

			Box_SetText(subbox, txt2);
		}
		else if (gdata->limittype)
		{
			if (gdata->showrating == -1 && gdata->showrating2 != -1)
			{
				char txt[256];
				char txt2[256];

				sprintf(txt, "%d", gdata->showrating2);

				i18n_stringsub(txt2, 256, _("%1 below %2"), gdata->limittype, txt);
				Box_SetText(subbox, txt2);
			}
			else if (gdata->showrating != -1 && gdata->showrating2 == -1)
			{
				char txt[256];
				char txt2[256];

				sprintf(txt, "%d", gdata->showrating);

				i18n_stringsub(txt2, 256, _("%1 above %2"), gdata->limittype, txt);
				Box_SetText(subbox, txt2);
			}
			else 
			{
				char txt[256];
				char txt2[256];
				char txt3[256];

				sprintf(txt, "%d", gdata->showrating);
				sprintf(txt2, "%d", gdata->showrating2);

				i18n_stringsub(txt3, 256, _("%1 %2-%3"), gdata->limittype, txt, txt2);
				Box_SetText(subbox, txt3);
			}
		}

		Box_AddChild(entrybox, subbox);
	}

	/*
	entry = NamedList_GetByName(&(data->playcategories), "Low");
	cdata = (*entry)->data;
	subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	if (gdata->showrating == -1)
	{
                Box_SetText(subbox, "-");
	}
	else
	{
		sprintf(txt, "%d", gdata->showrating);
		Box_SetText(subbox, txt);
	}
	Box_AddChild(entrybox, subbox);

	entry = NamedList_GetByName(&(data->playcategories), "High");
	cdata = (*entry)->data;
	subbox = Box_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	if (gdata->showrating2 == -1)
	{
                Box_SetText(subbox, "-");
	}
	else
	{
		sprintf(txt, "%d", gdata->showrating2);
		Box_SetText(subbox, txt);
	}
	Box_AddChild(entrybox, subbox);
	*/

	entry = NamedList_GetByName(&(data->playcategories), "Time Control");
	if (entry)
	{
		cdata = (*entry)->data;
		subbox = Text_Create(cdata->x + 8, 4, cdata->w, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		{
			char *txt = Info_TimeControlsToShortText(gdata->info->timecontrol, gdata->info->blacktimecontrol);

			if (txt && strlen(txt) > 10)
			{
				Text_SetText(subbox, "^lCustom^l");
				Text_SetLinkTooltip(subbox, 1, txt);
			}
			else
			{
				Text_SetText(subbox, txt);
			}
		}
		Box_AddChild(entrybox, subbox);
	}

	entry = NamedList_GetByName(&(data->playcategories), NULL);
	cdata = (*entry)->data;
	subbox = StdButton_Create(cdata->x, 1, cdata->w, _("Edit"), 1);
	Button2_SetOnButtonHit(subbox, NewGameGameEntry_OnEditHit);
	Box_AddChild(entrybox, subbox);

	entry = NamedList_GetByName(&(data->playcategories), NULL);
	entry = &((*entry)->next);
	cdata = (*entry)->data;
	subbox = StdButton_Create(cdata->x, 1, cdata->w, _("Remove"), 1);
	Button2_SetOnButtonHit(subbox, NewGameGameEntry_OnRemoveHit);
	Box_AddChild(entrybox, subbox);

	List_AddEntry(data->list, info->gameid, NULL, entrybox);
	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}


void NewGames_AddSearchGame(struct Box_s *dialog, char *itemid, char *node, char *jid, struct gamesearchinfo_s *info)
{
	struct newgamesdata_s *data = dialog->boxdata;
	char *variant = NULL;
	char *timecontrolrange = NULL;
	struct Box_s *entrybox = List_GetEntryBox(data->list, itemid, NULL);

/*
	if (stricmp(node, data->currentpagetype) != 0)
	{
		return;
	}
*/
	if (!entrybox && stricmp(node, "myads") == 0 && stricmp(data->currentpagetype, "play") == 0)
	{
		/*NewGames_AddMyAdsGame(dialog, itemid, node, jid, info);*/
		/*CheckBox_SetChecked(data->listmecheckbox, 1);*/
		/*data->hasad = 1;*/
		NewGames_AddPlayGame(dialog, itemid, node, jid, info);
	}
	else if (!entrybox && stricmp(node, "play") == 0 && stricmp(data->currentpagetype, "play") == 0)
	{
		/*if (Model_GetOption(OPTION_ENABLEPUSHADS))*/
		{
			return; /* push protocol doesn't use this */
		}
		NewGames_AddPlayGame(dialog, itemid, node, jid, info);
	}
	else if (!entrybox && stricmp(node, "watch") == 0 && stricmp(data->currentpagetype, "watch") == 0)
	{
		NewGames_AddWatchGame(dialog, itemid, node, info);
	}
	else if (!entrybox && stricmp(node, "adjourned") == 0 && stricmp(data->currentpagetype, "adjourned") == 0)
	{
		NewGames_AddAdjournedGame(dialog, itemid, node, jid, info);
	}
}

void NewGames_AddPushGame(struct Box_s *dialog, char *type, char *id, struct gamesearchinfo_s *info)
{
	struct newgamesdata_s *data = dialog->boxdata;

	/*if (!Model_GetOption(OPTION_ENABLEPUSHADS))
	{
		return;
	}*/

	if (!type || stricmp(type, data->currentpagetype) != 0)
	{
		return;
	}

	if (stricmp(type, "play") == 0)
	{
		NewGames_AddPlayGame(dialog, id, type, NULL, info);
	}
}

void NewGames_RemovePushGame(struct Box_s *dialog, char *type, char *id)
{
	struct newgamesdata_s *data = dialog->boxdata;

	/*if (!Model_GetOption(OPTION_ENABLEPUSHADS))
	{
		return;
	}*/

	if (!type || stricmp(type, data->currentpagetype) != 0)
	{
		return;
	}

	if (stricmp(type, "play") == 0)
	{
		struct Box_s *entrybox = List_GetEntryBox(data->list, id, NULL);
		if (entrybox)
		{
			struct slideoutdata_s *sod = malloc(sizeof(*sod));
			sod->list = data->list;
			sod->id = strdup(id);

			Box_AddTimedFunc(entrybox, NewGamesListEntry_SlideOut, sod, 17);
		}
		/*
		List_RemoveEntryByName(data->list, id, NULL);
		List_RedoEntries(data->list);
		Box_Repaint(data->list);
		*/
	}
}

void NewGames_ClearPushGames(struct Box_s *dialog /*, char *type*/)
{
	struct newgamesdata_s *data = dialog->boxdata;

	/*if (!Model_GetOption(OPTION_ENABLEPUSHADS))
	{
		return;
	}*/

	/*if (!type)*/
	{
		List_RemoveAllEntries(data->list);
	}
}

void NewGames_ClearAds(struct Box_s *dialog)
{
	struct newgamesdata_s *data = dialog->boxdata;

	data->numgameads = 0;
	/*NewGames_UpdateManageText(dialog);*/

	if (stricmp(data->currentpagetype, "myads") == 0)
	{
		List_RemoveAllEntries(data->list);
	}
}

void NewGames_SetGameFinderExpiryReminder(struct Box_s *dialog, int expiry)
{
	struct newgamesdata_s *data = dialog->boxdata;
	data->showtrialwarning = expiry;

	Box_OnSizeHeight_Stretch(data->innerpage, dialog->h  - (data->showtrialwarning ? 30 : 0) - 8 - 32 - data->innerpage->h);

	AutoSize_Fit(data->sizeablecontent);
	data->sizeablecontent->h = dialog->h - (data->showtrialwarning ? 30 : 0) - 8 - 36;
	AutoSize_Fill(data->sizeablecontent);

	Box_Repaint(dialog);
}
