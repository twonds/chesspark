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
#include "customtime.h"
#include "i18n.h"
#include "imagemgr.h"
#include "info.h"
#include "list.h"
#include "model.h"
#include "namedlist.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

struct editcustomtimesdata_s
{
	struct Box_s *list;
	struct Box_s *parent;
	void (*refreshCallback)(struct Box_s *);
	void (*destroyCallback)(struct Box_s *);
	struct Box_s *customtime;
};

struct editcustomtimesentrydata_s
{
	char *name;
	struct timecontrol_s *wtc;
	struct timecontrol_s *btc;
};

void EditCustomTimes_OnSetCustomTimeControl(struct Box_s *pbox, struct timecontrol_s *wtc, struct timecontrol_s *btc, char *name);
/*
void EditCustomTimes_OnSave(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct editcustomtimesdata_s *data = pbox->parent->boxdata;
	
	if (data->refreshCallback)
	{
		data->refreshCallback(data->parent);
	}

	Box_Destroy(dialog);
}
*/

void EditCustomTimes_OnOK(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct editcustomtimesdata_s *data = dialog->boxdata;

	Box_Destroy(dialog);
}

void EditCustomTimesEntry_Remove(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct editcustomtimesdata_s *rootdata = dialog->boxdata;
	struct editcustomtimesentrydata_s *data = pbox->parent->boxdata;

	List_RemoveEntryByName(rootdata->list, data->name, NULL);

	Model_RemoveSavedCustomTimeControl(data->name);

	if (rootdata->refreshCallback)
	{
		rootdata->refreshCallback(rootdata->parent);
	}
}

void EditCustomTimes_OnCustomTimeDestroy(struct Box_s *dialog)
{
	struct editcustomtimesdata_s *data = dialog->boxdata;

	data->customtime = NULL;
}

void EditCustomTimesEntry_OnEditEntry(struct Box_s *pbox, int x, int y)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct editcustomtimesdata_s *data = dialog->boxdata;
	struct editcustomtimesentrydata_s *entrydata = pbox->boxdata;

	data->customtime = CustomTime_Create(dialog, EditCustomTimes_OnSetCustomTimeControl, entrydata->name, entrydata->wtc, entrydata->btc, NULL, EditCustomTimes_OnCustomTimeDestroy, 0);
}

void EditCustomTimes_AddEntry(struct Box_s *pbox, char *name, struct tcpair_s *tcp)
{
	struct editcustomtimesdata_s *data = pbox->boxdata;
	struct Box_s *entry, *subbox;
	struct editcustomtimesentrydata_s *entrydata = malloc(sizeof(*entrydata));
	
	entry = Box_Create(0, 0, pbox->w - 2 * 8 - 32, 3 * 8, BOX_VISIBLE | BOX_TRANSPARENT);
	entry->OnSizeWidth = Box_OnSizeWidth_Stretch;
	entry->OnLButtonDblClk = EditCustomTimesEntry_OnEditEntry;

	subbox = Box_Create(3 * 8 + 16, (entry->h - 16) / 2, entry->w - 3 * 8, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = TabFG1;
	Box_SetText(subbox, name);
	Box_AddChild(entry, subbox);

	subbox = Button_Create((entry->w) - 13 - 16, (entry->h - 13) / 2, 13, 13, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetImage("deleteIcon.png");
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	Button_SetOnButtonHit(subbox, EditCustomTimesEntry_Remove);
	Box_AddChild(entry, subbox);

	entrydata->name = strdup(name);
	entrydata->wtc = Info_DupeTimeControl(tcp->white);
	entrydata->btc = Info_DupeTimeControl(tcp->black);

	entry->boxdata = entrydata;

	List_AddEntry(data->list, name, NULL, entry);

	List_RedoEntries(data->list);
	Box_Repaint(data->list);
}

void EditCustomTimes_OnSetCustomTimeControl(struct Box_s *pbox, struct timecontrol_s *wtc, struct timecontrol_s *btc, char *name)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct editcustomtimesdata_s *rootdata = dialog->boxdata;
	if (wtc && name)
	{
		if (Model_CustomTimeControlExists(name))
		{
			List_RemoveEntryByName(rootdata->list, name, NULL);
		}

		Model_SaveCustomTimeControl(name, wtc, btc);
		{
			struct tcpair_s *tcp;

			tcp = malloc(sizeof(*tcp));
			memset(tcp, 0, sizeof(*tcp));
			tcp->white = Info_DupeTimeControl(wtc);
			tcp->black = Info_DupeTimeControl(btc);

			EditCustomTimes_AddEntry(dialog, name, tcp);

			Info_DestroyTCPair(tcp);
		}

		if (rootdata->refreshCallback)
		{
			rootdata->refreshCallback(rootdata->parent);
		}
	}
}

void EditCustomTimes_OnAdd(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct editcustomtimesdata_s *data = dialog->boxdata;

	data->customtime = CustomTime_Create(dialog, EditCustomTimes_OnSetCustomTimeControl, NULL, NULL, NULL, NULL, EditCustomTimes_OnCustomTimeDestroy, 0);
}


void EditCustomTimes_OnClose(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);

	Box_Destroy(pbox);
}

void EditCustomTimes_OnDestroy(struct Box_s *dialog)
{
	struct editcustomtimesdata_s *data = dialog->boxdata;

	if (data->customtime)
	{
		Box_Destroy(data->customtime);
	}

	if (data->destroyCallback && data->parent)
	{
		data->destroyCallback(data->parent);
	}
}

struct Box_s *EditCustomTimes_Create(struct Box_s *parent, void (*refreshCallback)(struct Box_s *), void (*destroyCallback)(struct Box_s *))
{
	struct editcustomtimesdata_s *data = malloc(sizeof(*data));
	struct Box_s *pbox, *dialog;
	int x, y, w, h;

	memset(data, 0, sizeof(*data));

	w = 432;
	h = 440;

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;

		windowrect.left = parent->x;
		windowrect.right = windowrect.left + parent->w - 1;
		windowrect.top = parent->y;
		windowrect.bottom = windowrect.top + parent->h - 1;

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

	data->parent = parent;
	data->refreshCallback = refreshCallback;
	data->destroyCallback = destroyCallback;
	
	SizerSet_Create(dialog);
	
	dialog->titlebar = TitleBarOnly_Add(dialog, _("Edit Saved Custom Time Controls"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;
	dialog->OnClose = EditCustomTimes_OnClose;

	pbox = List_Create(1 * 8, 5 * 8, w - 2 * 8, h - 13 * 8 - 5 * 8, BOX_VISIBLE, FALSE);
	pbox->bgcol = TabBG2;
	pbox->fgcol = TabFG2;
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(dialog, pbox);
	data->list = pbox;	
	
	pbox = StdButton_Create(20, (int)(h - 5.5 * 8), (int)(10.5 * 8), _("Add"), 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, EditCustomTimes_OnAdd);
	Box_AddChild(dialog, pbox);
/*
	pbox = StdButton_Create((int)(w - 23 * 8), (int)(h - 5.5 * 8), (int)(10.5 * 8), "Save", 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, EditCustomTimes_OnSave);
	Box_AddChild(dialog, pbox);
*/	
	pbox = StdButton_Create((int)(w - 12 * 8), (int)(h - 5.5 * 8), (int)(10.5 * 8), _("OK"), 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(pbox, EditCustomTimes_OnOK);
	Box_AddChild(dialog, pbox);

	dialog->boxdata = data;

	{

		struct namedlist_s *customtimecontrols = Model_GetCustomTimeControls();
		struct namedlist_s *entry = customtimecontrols;

		while (entry)
		{
			EditCustomTimes_AddEntry(dialog, entry->name, entry->data);
			entry = entry->next;
		}
	}

	Box_CreateWndCustom(dialog, _("Edit Saved Custom Times"), parent->hwnd);
	
	dialog->OnSizeWidth = Box_OnSizeWidth_Stretch;
	dialog->OnSizeHeight = Box_OnSizeHeight_Stretch;
	dialog->OnDestroy = EditCustomTimes_OnDestroy;

	BringWindowToTop(dialog->hwnd);

	return dialog;
}
