#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>

#include <commctrl.h>

#include "box.h"
#include "edit.h"

#include "button.h"
#include "sizer.h"
#include "titledrag.h"

#include "button2.h"
#include "constants.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "imagemgr.h"
#include "imgcombo.h"
#include "list.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

#include "editstatus.h"

struct editstatusdata_s
{
	struct Box_s *list;
	struct Box_s *typecombo;
	struct Box_s *nameentry;
	struct Box_s *addbutton;
	struct StatusList_s *awaylist;
	struct StatusList_s *availlist;
};

struct editstatusentrydata_s
{
	char *name;
	BOOL away;
};

void EditStatus_OnSave(struct Box_s *pbox)
{
	struct editstatusdata_s *data = pbox->parent->boxdata;
	
	Ctrl_SetStatusList(SSTAT_AWAY, data->awaylist);
	Ctrl_SetStatusList(SSTAT_AVAILABLE, data->availlist);
	
	Box_Destroy(Box_GetRoot(pbox));
}


void EditStatus_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}

void EditStatusEntry_Remove(struct Box_s *pbox)
{
	struct editstatusdata_s *rootdata = Box_GetRoot(pbox)->boxdata;
	struct editstatusentrydata_s *data = pbox->parent->boxdata;

	if (data->away)
	{
		StatusList_Remove(&(rootdata->awaylist), SSTAT_AWAY, data->name);
	}
	else
	{
		StatusList_Remove(&(rootdata->availlist), SSTAT_AVAILABLE, data->name);
	}
	
	if (data->away)
	{
		List_RemoveEntryByName(rootdata->list, data->name, _("Away"));
	}
	else
	{
		List_RemoveEntryByName(rootdata->list, data->name, _("Available"));
	}

	List_RedoEntries(rootdata->list);
	Box_Repaint(rootdata->list);
}


void EditStatus_AddEntry(struct Box_s *pbox, char *name, BOOL away)
{
	struct editstatusdata_s *data = pbox->boxdata;
	struct Box_s *entry, *subbox;
	struct editstatusentrydata_s *entrydata = malloc(sizeof(*entrydata));
	
	entry = Box_Create(0, 0, pbox->w - 2 * 8 - 32, 3 * 8, BOX_VISIBLE | BOX_TRANSPARENT);
	entry->OnSizeWidth = Box_OnSizeWidth_Stretch;

	subbox = Box_Create(3 * 8 + 16, (entry->h - 16) / 2, entry->w - 3 * 8 - 16 - 13 - 16 - 5, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->fgcol = TabFG1;
	Box_SetText(subbox, name);
	Box_AddChild(entry, subbox);

	subbox = Button_Create((entry->w) - 13 - 16, (entry->h - 13) / 2, 13, 13, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetImage("deleteIcon.png");
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	Button_SetOnButtonHit(subbox, EditStatusEntry_Remove);
	Button_SetTooltipText(subbox, _("Delete"));
	Box_AddChild(entry, subbox);

	entrydata->name = strdup(name);

	if (away)
	{
		entrydata->away = TRUE;
		subbox = Box_Create(16, (entry->h - 16) / 2, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("presenceAway", "PresenceIcons.png", 48, 0, 16, 16);
		Box_AddChild(entry, subbox);
		
		List_AddEntry(data->list, name, _("Away"), entry);
		StatusList_Add(&(data->awaylist), SSTAT_AWAY, name);
	}
	else
	{
		entrydata->away = FALSE;
		subbox = Box_Create(16, (entry->h - 16) / 2, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16);
		Box_AddChild(entry, subbox);
				
		List_AddEntry(data->list, name, _("Available"), entry);
		StatusList_Add(&(data->availlist), SSTAT_AVAILABLE, name);
	}

	entry->boxdata = entrydata;

	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}


void EditStatus_OnAdd(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct editstatusdata_s *data = dialog->boxdata;

	char *name;
	
	name = Edit2Box_GetText(data->nameentry);

	if (!name || strlen(name) == 0)
	{
		return;
	}

	if (stricmp(ImgCombo_GetSelectionName(data->typecombo), _("Away")) == 0)
	{
		if (!StatusList_Find(data->awaylist, name))
		{
			EditStatus_AddEntry(pbox->parent, name, TRUE);
		}
	}
	else
	{
		if (!StatusList_Find(data->availlist, name))
		{
			EditStatus_AddEntry(pbox->parent, name, FALSE);
		}
	}

	Edit2Box_SetText(data->nameentry, NULL);
	Button2_SetDisabledState(data->addbutton, 1);
}

void EditStatusEdit_OnKey(struct Box_s *pbox, char *text)
{
	struct editstatusdata_s *data = pbox->parent->boxdata;

	if (!text || strlen(text) == 0)
	{
		Button2_SetDisabledState(data->addbutton, 1);
	}
	else
	{
		Button2_SetDisabledState(data->addbutton, 0);
	}
}

void EditStatus_OnClose(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}

struct Box_s *EditStatus_Create(struct Box_s *roster, struct StatusList_s *awaylist, struct StatusList_s *availlist)
{
	struct editstatusdata_s *data = malloc(sizeof(*data));
	struct Box_s *pbox, *dialog, *subbox, *pbox2;
	int x, y, w, h;

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
	dialog->fgcol = RGB(0, 0, 0);
	dialog->minw = 384;
	dialog->minh = 256;
	
	SizerSet_Create(dialog);
	
	dialog->titlebar = TitleBarOnly_Add(dialog, _("Edit My Statuses"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	pbox = List_Create(1 * 8, 5 * 8, w - 2 * 8, h - 13 * 8 - 5 * 8, BOX_VISIBLE, FALSE);
	pbox->bgcol = TabBG2;
	pbox->fgcol = TabFG2;
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(dialog, pbox);
	data->list = pbox;
	
	pbox = Box_Create(2 * 8, h - 12 * 8, 9 * 8, 2 * 8, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = TabFG2;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_SetText(pbox, _("New Status:"));
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create((int)(w - 8 * 8), (int)(h - 12.5 * 8), 6 * 8, _("Add"), 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, EditStatus_OnAdd);
	Box_AddChild(dialog, pbox);
	data->addbutton = pbox;
	Button2_SetDisabledState(data->addbutton, 1);
	
	pbox = StdButton_Create((int)(w - 23 * 8), (int)(h - 5.5 * 8), (int)(10.5 * 8), _("Save"), 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, EditStatus_OnSave);
	Box_AddChild(dialog, pbox);
	
	pbox = StdButton_Create((int)(w - 12 * 8), (int)(h - 5.5 * 8), (int)(10.5 * 8), _("Cancel"), 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, EditStatus_OnCancel);
	Box_AddChild(dialog, pbox);

	pbox = Edit2Box_Create(25 * 8, h - 100/*12.5 * 8*/, w - 25 * 8 - 9 * 8, 20, BOX_VISIBLE | BOX_BORDER, 1);
	pbox->bgcol = RGB(255, 255, 255);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Edit2Box_SetOnKey(pbox, EditStatusEdit_OnKey);
	Edit2Box_SetAltWText(pbox, _L("Name"));
	Box_AddChild(dialog, pbox);
	data->nameentry = pbox;

	pbox = ImgCombo_Create((int)(11.5 * 8), (int)(h - 12.5 * 8), (int)(13 * 8), 20, BOX_VISIBLE | BOX_BORDER);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->bgcol = RGB(255, 255, 255);
	Box_AddChild(dialog, pbox);
	data->typecombo = pbox;

	dialog->boxdata = data;

	List_AddGroup(data->list, _("Available"));
	List_AddGroup(data->list, _("Away"));

	data->availlist = NULL;
	while (availlist)
	{
		EditStatus_AddEntry(dialog, availlist->statusmsg, FALSE);
		availlist = availlist->next;
	}

	data->awaylist = NULL;
	while (awaylist)
	{
		EditStatus_AddEntry(dialog, awaylist->statusmsg, TRUE);
		awaylist = awaylist->next;
	}

	List_RedoEntries(data->list);

	Box_CreateWndCustom(dialog, _("Edit My Statuses"), roster->hwnd);

	dialog->OnSizeWidth = Box_OnSizeWidth_Stretch;
	dialog->OnSizeHeight = Box_OnSizeHeight_Stretch;

	pbox = Box_Create(0, 0, 104, 20, BOX_VISIBLE);
	pbox->bgcol = RGB(255, 255, 255);

	subbox = Box_Create(0, 0, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->bgcol = RGB(255, 255, 255);
	subbox->img = ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16);
	Box_AddChild(pbox, subbox);

	subbox = Box_Create(18, 2, 58, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_SetText(subbox, _("Available"));
	Box_AddChild(pbox, subbox);

	pbox2 = Box_Create(0, 0, 104, 20, BOX_VISIBLE);
	pbox2->bgcol = RGB(160, 160, 160);

	subbox = Box_Create(0, 0, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->bgcol = RGB(160, 160, 160);
	subbox->img = ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16);
	Box_AddChild(pbox2, subbox);

	subbox = Box_Create(18, 2, 58, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_SetText(subbox, _("Available"));
	Box_AddChild(pbox2, subbox);

	ImgCombo_AddEntry(data->typecombo, _("Available"), pbox, pbox2);
	
	pbox->child->img = ImageMgr_GetSubImage("presenceAway", "PresenceIcons.png", 48, 0, 16, 16);
	Box_SetText(pbox->child->sibling, _("Away"));
	
	pbox2->child->img = ImageMgr_GetSubImage("presenceAway", "PresenceIcons.png", 48, 0, 16, 16);
	Box_SetText(pbox2->child->sibling, _("Away"));

	ImgCombo_AddEntry(data->typecombo, _("Away"), pbox, pbox2);

	Box_Destroy(pbox);

	ImgCombo_SetSelection(data->typecombo, _("Available"));

	BringWindowToTop(dialog->hwnd);

	return dialog;
}
