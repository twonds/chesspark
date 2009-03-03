#include <stdio.h>

#include "box.h"

#include "list.h"

#include "button.h"
#include "button2.h"
#include "conn.h"
#include "constants.h"
#include "imagemgr.h"
#include "i18n.h"
#include "model.h"
#include "namedlist.h"
#include "sizer.h"
#include "stdbutton.h"
#include "titlebar.h"

struct banlistdata_s
{
	char *jid;
	struct Box_s *list;

	struct namedlist_s *banlist;
	struct namedlist_s *removedbans;
	struct namedlist_s *addedbans;
};

struct banlistentrydata_s
{
	char *name;
	char *showname;
};

void Banlist_RemoveEntry(struct Box_s *dialog, char *name)
{
	struct banlistdata_s *data = dialog->boxdata;

	NamedList_Add(&(data->removedbans), name, NULL, NULL);

	List_RemoveEntryByName(data->list, name, NULL);
}

void BanlistEntry_Remove(struct Box_s *entry)
{
	struct banlistentrydata_s *entrydata = entry->parent->boxdata;
	struct Box_s *dialog = Box_GetRoot(entry);
	struct banlistdata_s *data = dialog->boxdata;

	Banlist_RemoveEntry(dialog, entrydata->name);

	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

int BanlistEntry_Sort(struct Box_s *lbox, struct Box_s *rbox)
{
	struct banlistentrydata_s *ldata = lbox->boxdata;
	struct banlistentrydata_s *rdata = rbox->boxdata;

	return (stricmp(ldata->showname, rdata->showname) < 0);
}


void Banlist_AddEntry(struct Box_s *dialog, char *name, char *reason, int changed)
{
	struct banlistdata_s *data = dialog->boxdata;
	struct Box_s *entry, *subbox;
	struct banlistentrydata_s *entrydata = malloc(sizeof(*entrydata));
	char showtext[512];

	sprintf(showtext, "%s", Model_GetFriendNick(name));

	entry = Box_Create(0, 0, dialog->w - 2 * 8 - 32, 3 * 8, BOX_VISIBLE | BOX_TRANSPARENT);
	entry->OnSizeWidth = Box_OnSizeWidth_Stretch;

	subbox = Box_Create(5, (entry->h - 16) / 2, 200, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->fgcol = TabFG1;
	Box_SetText(subbox, showtext);
	Box_AddChild(entry, subbox);

	if (reason)
	{
		subbox = Box_Create(205, (entry->h - 16) / 2, entry->w - 205 - 13 - 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		subbox->fgcol = TabFG1;
		Box_SetText(subbox, reason);
		Box_AddChild(entry, subbox);
	}

	subbox = Button_Create((entry->w) - 13 - 16, (entry->h - 13) / 2, 13, 13, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetImage("deleteIcon.png");
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	Button_SetOnButtonHit(subbox, BanlistEntry_Remove);
	Button_SetTooltipText(subbox, _("Delete"));
	Box_AddChild(entry, subbox);

	entrydata->name = strdup(name);
	entrydata->showname = strdup(showtext);

	entry->boxdata = entrydata;

	List_AddEntry(data->list, name, NULL, entry);

	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

void Banlist_OnCancel(struct Box_s *button)
{
	struct Box_s *dialog = Box_GetRoot(button);
	Box_Destroy(dialog);
}

void Banlist_OnSave(struct Box_s *button)
{
	struct Box_s *dialog = Box_GetRoot(button);
	struct banlistdata_s *data = dialog->boxdata;

	Conn_SetBanlist(data->jid, data->addedbans, data->removedbans);

	Banlist_OnCancel(dialog);
}

struct Box_s *Banlist_Create(struct Box_s *roster, char *jid, struct namedlist_s *banlist)
{
	struct banlistdata_s *data = malloc(sizeof(*data));
	struct Box_s *pbox, *dialog;
	struct namedlist_s *entry;
	int x, y, w, h;
	char titletxt[256];

	memset(data, 0, sizeof(*data));

	data->jid = strdup(jid);

	w = 432;
	h = 440;

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

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - w) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - h) / 2;
	}

	dialog = Box_Create(x, y, w, h, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;
	/*dialog->fgcol = RGB(0, 0, 0);*/
	dialog->minw = 384;
	dialog->minh = 256;
	
	SizerSet_Create(dialog);
	
	i18n_stringsub(titletxt, 256, _("Banned users from %1"), jid);

	dialog->titlebar = TitleBarOnly_Add(dialog, titletxt);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	pbox = Box_Create(1 * 8, 5 * 8, w - 2 * 8, h - 13 * 8 - 5 * 8, BOX_VISIBLE | BOX_BORDER);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	pbox->bgcol = TabBG2;
	pbox->brcol = RGB(176,178,183);
	Box_AddChild(dialog, pbox);
	
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

	pbox = Box_Create(1 * 8 + 10, 5 * 8 + 5, 50, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = TabFG1;
	Box_SetText(pbox, _("ID"));
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(1 * 8 + 210, 5 * 8 + 5, 50, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = TabFG1;
	Box_SetText(pbox, _("Reason"));
	Box_AddChild(dialog, pbox);

	pbox = List_Create(1 * 8 + 5, 5 * 8 + 25, w - 2 * 8 - 10, h - 13 * 8 - 5 * 8 - 30, BOX_VISIBLE, FALSE);
	List_SetEntrySortFunc(pbox, BanlistEntry_Sort);
	List_SetEntrySelectable(pbox, 0);
	pbox->bgcol = TabBG2;
	pbox->fgcol = TabFG2;
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(dialog, pbox);
	data->list = pbox;
	
	pbox = StdButton_Create((int)(w - 23 * 8), (int)(h - 5.5 * 8), (int)(10.5 * 8), _("Save"), 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, Banlist_OnSave);
	Box_AddChild(dialog, pbox);
	
	pbox = StdButton_Create((int)(w - 12 * 8), (int)(h - 5.5 * 8), (int)(10.5 * 8), _("Cancel"), 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, Banlist_OnCancel);
	Box_AddChild(dialog, pbox);

	dialog->boxdata = data;

	entry = banlist;
	while (entry)
	{
		Banlist_AddEntry(dialog, entry->name, entry->data, 0);
		entry = entry->next;
	}

	List_RedoEntries(data->list);

	Box_CreateWndCustom(dialog, titletxt, NULL);

	dialog->OnSizeWidth = Box_OnSizeWidth_Stretch;
	dialog->OnSizeHeight = Box_OnSizeHeight_Stretch;

	BringWindowToTop(dialog->hwnd);

	return dialog;
}