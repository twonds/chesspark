#include <stdlib.h>

#include "box.h"

#include "button.h"
#include "titledrag.h"

#include "button2.h"
#include "constants.h"
#include "ctrl.h"
#include "i18n.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "view.h"

#include "removegroup.h"

struct removegroupdata_s
{
	char *name;
};


void RemoveGroup_OnRemoveGroup(struct Box_s *pbox)
{
	struct removegroupdata_s *data = pbox->parent->boxdata;

	Ctrl_RemoveGroup(data->name);

	Box_Destroy(Box_GetRoot(pbox));
}


void RemoveGroup_OnRemoveGroupAndContents(struct Box_s *pbox)
{
	struct removegroupdata_s *data = pbox->parent->boxdata;

	Ctrl_RemoveGroupAndContents(data->name);

	Box_Destroy(Box_GetRoot(pbox));
}


void RemoveGroup_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}


struct Box_s *RemoveGroup_Create(struct Box_s *roster, char *name)
{
	struct Box_s *dialog;
	struct Box_s *pbox;
	struct removegroupdata_s *data = malloc(sizeof(*data));
	char gonnaremove[100];
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

	gonnaremove[0] = 0;
	i18n_stringsub(gonnaremove, 100, _("Remove Group %1 from Friends List?"), name);
	
	dialog->titlebar = TitleBarOnly_Add(dialog, gonnaremove);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	pbox = Box_Create(32, 48, 400, 16, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, _("Do you want to remove the group, or the group and its contents?"));
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(432 - 370, 128 - 32, 80, _("Group"), 0);
	Button2_SetOnButtonHit(pbox, RemoveGroup_OnRemoveGroup);
	Box_AddChild(dialog, pbox);
	
	pbox = StdButton_Create(432 - 270, 128 - 32, 140, _("Group & Contents"), 0);
	Button2_SetOnButtonHit(pbox, RemoveGroup_OnRemoveGroupAndContents);
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(432 - 100, 128 - 32, 90, _("Don't Remove"), 0);
	Button2_SetOnButtonHit(pbox, RemoveGroup_OnCancel);
	Box_AddChild(dialog, pbox);

	data->name = strdup(name);
	dialog->boxdata = data;

	Box_CreateWndCustom(dialog, _("Remove Group"), roster->hwnd);
	
	BringWindowToTop(dialog->hwnd);

	return dialog;
}
