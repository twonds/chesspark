#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <stdio.h>

#include "box.h"
#include "text.h"

#include "constants.h"
#include "i18n.h"
#include "imagemgr.h"
#include "titlebar.h"
#include "view.h"

struct adpopdata_s
{
	int seconds;
	struct Box_s *text;
};

void Adpop_OnClose(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
	Box_UnlockInput();
}

void Adpop_OpenPurchase(struct Box_s *pbox, int x, int y)
{
	Util_OpenURL("http://www.chesspark.com/purchase");
}

void Adpop_SecondTimeout(struct Box_s *dialog, void *dummy)
{
	struct adpopdata_s *data = dialog->boxdata;
	char txt[512];

	Box_RemoveTimedFunc(dialog, Adpop_SecondTimeout, 1000);

	data->seconds--;
	if (data->seconds)
	{

		if (data->seconds != 1)
		{
			char num[5];
			sprintf(num, "%d", data->seconds);
			i18n_stringsub(txt, 512, _("Wait %1 seconds..."), num);
		}
		else
		{
			i18n_stringsub(txt, 512, _("Wait 1 second..."));
		}

		Text_SetText(data->text, txt);
		Box_AddTimedFunc(dialog, Adpop_SecondTimeout, NULL, 1000);
	}
	else
	{
		struct Box_s *button;
		data->text->flags &= ~BOX_VISIBLE;

		button = StdButton_Create((dialog->w - 60) / 2, dialog->h - 30, 60, _("Close"), 1);
		Button2_SetOnButtonHit(button, Adpop_OnClose);
		Box_AddChild(dialog, button);
	}

	Box_Repaint(dialog);
}

extern HFONT tahoma11b_f;

struct Box_s *Adpop_Create(struct Box_s *roster)
{
	struct Box_s *dialog, *subbox;
	struct adpopdata_s *data;
	char txt[120];
	char filename[512];
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

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - 350) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - 390/*490*/) / 2;
	}

	dialog = Box_Create(x, y, 350, 390, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;
	dialog->fgcol = UserInfoFG2;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));
	data->seconds = 5;

	dialog->boxdata = data;
/*
	dialog->titlebar = TitleBarOnly_Add(dialog, _("Chesspark Advertisement"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;
*/
	subbox = Box_Create((dialog->w - 350) / 2, 0, 350, 390, BOX_VISIBLE | BOX_TRANSPARENT);
	sprintf(filename, "ad%d.png", rand() % 5 + 1);
	subbox->img = ImageMgr_GetImage(filename);
	Box_AddChild(dialog, subbox);

	subbox = Box_Create(0, 0, dialog->w, dialog->h - 30, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnLButtonDown = Adpop_OpenPurchase;
	Box_LockMouseCursorImg(subbox, LoadCursor(NULL, IDC_HAND));
	Box_AddChild(dialog, subbox);

	subbox = Text_Create(0, dialog->h - 20, 350, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_CENTERED | TX_WRAP);
	subbox->fgcol = AdFG;
	Text_SetText(subbox, _("Wait 5 seconds..."));
	data->text = subbox;
	Box_AddChild(dialog, subbox);
 
	Box_CreateWndCustom(dialog, _("Chesspark Advertisement"), NULL);
	dialog->OnClose = NULL;

	Box_AddTimedFunc(dialog, Adpop_SecondTimeout, NULL, 1000);

	Box_LockInputToBox(dialog);
	BringWindowToTop(dialog->hwnd);

	return dialog;
}
