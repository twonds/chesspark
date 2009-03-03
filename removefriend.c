#include <stdlib.h>

#include "box.h"

#include "button.h"
#include "titledrag.h"

#include "button2.h"
#include "constants.h"
#include "ctrl.h"
#include "i18n.h"
#include "model.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "view.h"

#include "removefriend.h"

struct removefrienddata_s
{
	char *name;
	char *group;
};


void RemoveFriend_OnRemoveAll(struct Box_s *pbox)
{
	struct removefrienddata_s *data = Box_GetRoot(pbox)->boxdata;

	Ctrl_RemoveFriend(data->name);

	Box_Destroy(Box_GetRoot(pbox));
}

void RemoveFriend_OnRemoveGroup(struct Box_s *pbox)
{
	struct removefrienddata_s *data = Box_GetRoot(pbox)->boxdata;

	Ctrl_RemoveFriendFromGroup(data->name, data->group);

	Box_Destroy(Box_GetRoot(pbox));
}

void RemoveFriend_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}

void RemoveFriend_OnKeyDown(struct Box_s *pbox, int vk, int scan)
{
	if (vk == 27)
	{
		RemoveFriend_OnCancel(pbox);
	}
	else if (vk == 13)
	{
		RemoveFriend_OnRemoveAll(pbox);
	}
}


struct Box_s *RemoveFriend_Create(struct Box_s *roster, char *name, char *group)
{
	struct Box_s *dialog;
	struct Box_s *pbox;
	struct removefrienddata_s *data = malloc(sizeof(*data));
	char gonnaremove[256];
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

	dialog = Box_Create(x, y, 520, 128, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;
	dialog->focus = dialog;
	dialog->OnKeyDown = RemoveFriend_OnKeyDown;

	i18n_stringsub(gonnaremove, 256, _("Remove %1 from Friends List?"), Model_GetFriendNick(name));
	
	dialog->titlebar = TitleBarOnly_Add(dialog, gonnaremove);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	gonnaremove[0] = 0;
	i18n_stringsub(gonnaremove, 256, _("Are you sure that you want to remove %1 from your friends list?"), Model_GetFriendNick(name));
	
	pbox = Box_Create(32, 48, 400, 16, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, gonnaremove);
	Box_AddChild(dialog, pbox);

	if (group)
	{
		pbox = StdButton_Create(dialog->w - 500, 128 - 32, 170, _("Remove from all groups"), 0);
		Button2_SetOnButtonHit(pbox, RemoveFriend_OnRemoveAll);
		Box_AddChild(dialog, pbox);
	
		pbox = StdButton_Create(dialog->w - 310, 128 - 32, 170, _("Remove from this group"), 0);
		Button2_SetOnButtonHit(pbox, RemoveFriend_OnRemoveGroup);
		Box_AddChild(dialog, pbox);
	
		pbox = StdButton_Create(dialog->w - 120, 128 - 32, 100, _("Don't Remove"), 0);
		Button2_SetOnButtonHit(pbox, RemoveFriend_OnCancel);
		Box_AddChild(dialog, pbox);
	}
	else
	{
		pbox = StdButton_Create(dialog->w - 310, 128 - 32, 170, _("Remove from all groups"), 0);
		Button2_SetOnButtonHit(pbox, RemoveFriend_OnRemoveAll);
		Box_AddChild(dialog, pbox);
		
		pbox = StdButton_Create(dialog->w - 120, 128 - 32, 100, _("Don't Remove"), 0);
		Button2_SetOnButtonHit(pbox, RemoveFriend_OnCancel);
		Box_AddChild(dialog, pbox);
	}

	data->name = strdup(name);
	data->group = strdup(group);
	dialog->boxdata = data;

	Box_CreateWndCustom(dialog, _("Remove Friend"), roster->hwnd);
	
	BringWindowToTop(dialog->hwnd);

	return dialog;
}
