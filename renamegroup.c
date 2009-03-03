#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>

#include "box.h"

#include "button.h"
#include "button2.h"
#include "titledrag.h"

#include "constants.h"
#include "ctrl.h"
#include "i18n.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "view.h"

#include "renamegroup.h"


struct renamegroupdata_s
{
	char *oldname;
	HWND nameentry;
};

void RenameGroup_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}


void RenameGroup_OnOK(struct Box_s *pbox)
{
	struct renamegroupdata_s *data = pbox->parent->boxdata;
	char name[80];
	
	GetWindowText(data->nameentry, name, 80);

	Ctrl_RenameGroup(data->oldname, name);
	
	RenameGroup_OnCancel(pbox);
}


void RenameGroup_OnDestroy(struct Box_s *pbox)
{
	struct renamegroupdata_s *data = pbox->boxdata;
	DestroyWindow(data->nameentry);
}


struct Box_s *RenameGroup_Create(struct Box_s *roster, char *name)
{
	struct Box_s *dialog;
	struct Box_s *pbox;
	struct renamegroupdata_s *data = malloc(sizeof(*data));
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

	dialog->titlebar = TitleBarOnly_Add(dialog, _("Rename Group"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	pbox = Box_Create(32, 32, 100, 16, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, _("Group Name"));
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(432 - 200, 128 - 32, 80, _("Save"), 0);
	Button2_SetOnButtonHit(pbox, RenameGroup_OnOK);
	Box_AddChild(dialog, pbox);
	
	pbox = StdButton_Create(432 - 100, 128 - 32, 80, _("Cancel"), 0);
	Button2_SetOnButtonHit(pbox, RenameGroup_OnCancel);
	Box_AddChild(dialog, pbox);

	dialog->OnDestroy = RenameGroup_OnDestroy;
	data->oldname = strdup(name);
	dialog->boxdata = data;
	
	Box_CreateWndCustom(dialog, _("Rename Group"), roster->hwnd);

	data->nameentry = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD,
				32, 48, 256 + 96, 24,
				dialog->hwnd, NULL, GetModuleHandle(NULL), NULL);

	SetWindowText(data->nameentry, name);

	BringWindowToTop(dialog->hwnd);

	return dialog;
}
