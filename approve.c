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
#include "text.h"

#include "approve.h"

struct approvedata_s
{
	char *jid;
};


void Approve_OnYes(struct Box_s *pbox)
{
	struct approvedata_s *data = pbox->parent->boxdata;

	Ctrl_ApproveFriend(data->jid);
	Ctrl_AddFriend(data->jid, NULL);
	Ctrl_RequestFriend(data->jid);

	Box_Destroy(Box_GetRoot(pbox));
}


void Approve_OnNo(struct Box_s *pbox)
{
	struct approvedata_s *data = pbox->parent->boxdata;

	Ctrl_RejectFriend(data->jid);

	Box_Destroy(Box_GetRoot(pbox));
}


struct Box_s *Approve_Create(struct Box_s *roster, char *jid)
{
	struct Box_s *dialog;
	struct Box_s *pbox;
	struct approvedata_s *data = malloc(sizeof(*data));
	char gonnaadd[100];
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

	dialog->titlebar = TitleBarOnly_Add(dialog, _("Authorization Request"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	gonnaadd[0] = 0;
	i18n_stringsub(gonnaadd, 100, "^b%1^b would like to add you to their friends list.", Model_GetFriendNick(jid));
	
	pbox = Text_Create(32, 32, 400, 16, BOX_TRANSPARENT | BOX_VISIBLE, 0);
	pbox->fgcol = PresenceFG;
	Text_SetText(pbox, gonnaadd);
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(32, 48, 400, 16, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, _("Do you accept?"));
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(432 - 200, 128 - 32, 80, _("Yes"), 0);
	Button2_SetOnButtonHit(pbox, Approve_OnYes);
	Box_AddChild(dialog, pbox);
	
	pbox = StdButton_Create(432 - 100, 128 - 32, 80, _("No"), 0);
	Button2_SetOnButtonHit(pbox, Approve_OnNo);
	Box_AddChild(dialog, pbox);

	data->jid = strdup(jid);
	dialog->boxdata = data;

	Box_CreateWndCustom(dialog, _("Authorization Request"), roster->hwnd);

	FlashWindow(dialog->hwnd, TRUE);

	return dialog;
}
