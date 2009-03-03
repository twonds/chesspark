#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>

#include "box.h"

#include "button.h"
#include "edit2.h"
#include "titledrag.h"

#include "button2.h"
#include "constants.h"
#include "i18n.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "view.h"

#include "addstatus.h"


struct addgroupdata_s
{
	struct Box_s *nameentry;
};

void AddGroup_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}


void AddGroup_OnOK(struct Box_s *pbox)
{
	struct addgroupdata_s *data = pbox->parent->boxdata;
	char *name;
	
	name = Edit2Box_GetText(data->nameentry);

	View_AddGroup(name);
	
	AddGroup_OnCancel(pbox);
}

void AddGroupEdit_OnEnter(struct Box_s *pbox, char *text)
{
	AddGroup_OnOK(pbox);
}


void AddGroup_OnDestroy(struct Box_s *pbox)
{
	struct addgroupdata_s *data = pbox->boxdata;
}


struct Box_s *AddGroup_Create(struct Box_s *roster)
{
	struct Box_s *dialog;
	struct Box_s *pbox;
	struct addgroupdata_s *data = malloc(sizeof(*data));
	int x, y;

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

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - 432) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - 128) / 2;
	}

	dialog = Box_Create(x, y, 432, 128, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;
	
	dialog->titlebar = TitleBarOnly_Add(dialog, _("Add New Group"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	pbox = Box_Create(32, 32, 100, 16, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, "Group Name");
	Box_AddChild(dialog, pbox);

	pbox = Edit2Box_Create(32, 48, 256+96, 20, BOX_VISIBLE, E2_HORIZ);
	Edit2Box_SetOnEnter(pbox, AddGroupEdit_OnEnter);
	pbox->bgcol = EditBG;
	pbox->fgcol = EditFG;
	Box_AddChild(dialog, pbox);
	data->nameentry = pbox;

	pbox = StdButton_Create(432 - 200, 128 - 32, 80, _("Save"), 0);
	Button2_SetOnButtonHit(pbox, AddGroup_OnOK);
	Box_AddChild(dialog, pbox);
	
	pbox = StdButton_Create(432 - 100, 128 - 32, 80, _("Cancel"), 0);
	Button2_SetOnButtonHit(pbox, AddGroup_OnCancel);
	Box_AddChild(dialog, pbox);

	dialog->OnDestroy = AddGroup_OnDestroy;
	dialog->boxdata = data;
	
	Box_CreateWndCustom(dialog, _("Add New Group"), roster->hwnd);

	Box_SetFocus(data->nameentry);

        BringWindowToTop(dialog->hwnd);

	return dialog;
}
