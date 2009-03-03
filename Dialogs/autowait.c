#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>

#include "box.h"

#include "button.h"
#include "text.h"
#include "titledrag.h"

#include "button2.h"
#include "constants.h"
#include "edit2.h"
#include "i18n.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "view.h"

#include "addstatus.h"


struct autowaitdata_s
{
	struct Box_s *waitfullbox;
	struct Box_s *waitpartialbox;
	struct Box_s *cancelbutton;
	struct Box_s *boxtext;
	char *finishedtext;
};

void AutoWait_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}

void AutoWait_SetProgress(struct Box_s *pbox, float progress)
{
	struct autowaitdata_s *data = pbox->boxdata;

	data->waitpartialbox->w = (int)((pbox->w - 42) * progress);
	Box_Repaint(data->waitfullbox);

	if (progress >= 1.0f)
	{
		StdButton_SetText(data->cancelbutton, _("OK"));
		data->cancelbutton->flags |= BOX_VISIBLE;
		Box_Repaint(data->cancelbutton);
		Text_SetText(data->boxtext, data->finishedtext);
		data->waitfullbox->flags &= ~BOX_VISIBLE;
		data->waitpartialbox->flags &= ~BOX_VISIBLE;
	}
}

void AutoWaitName_SetProgress(char *name, float progress)
{
	struct Box_s *autowait = Box_GetWindowByName(name);

	if (!autowait)
	{
		return;
	}

	AutoWait_SetProgress(autowait, progress);
}

void AutoWait_OnDestroy(struct Box_s *pbox)
{
	struct autowaitdata_s *data = pbox->boxdata;
}

struct Box_s *AutoWait_Create(struct Box_s *parent, int w, char *titlebartxt,
  char *waitingtext, char *finishedtext)
{
	struct Box_s *dialog;
	struct Box_s *pbox;
	struct autowaitdata_s *data = malloc(sizeof(*data));
	int x, y, h;

	memset(data, 0, sizeof(*data));
	data->finishedtext = strdup(finishedtext);

	dialog = Box_Create(0, 0, w, 0, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;
	
	dialog->titlebar = TitleBarOnly_Add(dialog, titlebartxt);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	h = TitleBarHeight + 5;

	pbox = Text_Create(20, h, w - 40, 0, BOX_TRANSPARENT | BOX_VISIBLE, TX_WRAP | TX_STRETCHVERT);
	pbox->fgcol = PresenceFG;
	Text_SetText(pbox, waitingtext);
	Box_AddChild(dialog, pbox);
	data->boxtext = pbox;

	h += pbox->h + 5;

	pbox = Box_Create(20, h, w - 40, 20, BOX_VISIBLE);
	pbox->bgcol = RGB(255, 255, 255);
	Box_AddChild(dialog, pbox);
	data->waitfullbox = pbox;

	pbox = Box_Create(21, h + 1, 0, 18, BOX_VISIBLE);
	pbox->bgcol = RGB(0, 0, 0);
	Box_AddChild(dialog, pbox);
	data->waitpartialbox = pbox;

	h += pbox->h + 5;
/*
	pbox = Text_Create(20, h, w - 40, 0, BOX_TRANSPARENT | BOX_VISIBLE, TX_WRAP | TX_STRETCHVERT);
	pbox->fgcol = PresenceFG;
	Text_SetText(pbox, bottominfotext);
	Box_AddChild(dialog, pbox);
*/
	h += pbox->h + 20;
	
	pbox = StdButton_Create(w - 100, h, 80, _("Cancel"), 0);
	Button2_SetOnButtonHit(pbox, AutoWait_OnCancel);
	Box_AddChild(dialog, pbox);
	data->cancelbutton = pbox;
	pbox->flags &= ~BOX_VISIBLE;

	h += pbox->h + 5;

	if (parent)
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
	else
	{
		POINT pt;
		HMONITOR hm;
		MONITORINFO mi;

		GetCursorPos(&pt);

		hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - w) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - h) / 2;
	}

	dialog->x = x;
	dialog->y = y;
	dialog->h = h;

	dialog->OnDestroy = AutoWait_OnDestroy;
	dialog->boxdata = data;
	
	if (parent)
	{
		Box_CreateWndCustom(dialog, titlebartxt, parent->hwnd);
	}
	else
	{
		Box_CreateWndCustom(dialog, titlebartxt, NULL);
	}

	BringWindowToTop(dialog->hwnd);

	return dialog;
}
