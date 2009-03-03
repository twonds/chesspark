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


struct autoeditdata_s
{
	struct Box_s *textentry;
	void (*callbackfunc)(char *text, void *userdata);
	void *userdata;
};

void AutoEdit_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}


void AutoEdit_OnOK(struct Box_s *pbox)
{
	struct autoeditdata_s *data = Box_GetRoot(pbox)->boxdata;
	char *text = Edit2Box_GetText(data->textentry);

	if (!text || text[0] == '\0')
	{
		text = NULL;
	}
	
	data->callbackfunc(text, data->userdata);
	
	AutoEdit_OnCancel(pbox);
}


void AutoEdit_OnDestroy(struct Box_s *pbox)
{
	struct autoeditdata_s *data = pbox->boxdata;
}

void AutoEdit_OnEnter(struct Box_s *pbox, char *text)
{
	AutoEdit_OnOK(pbox);
}

struct Box_s *AutoEdit_Create(struct Box_s *parent, int w, char *titlebartxt,
  char *topinfotext, char *bottominfotext, char *defaulttext,
  void (*callbackfunc)(char *, void *), void *userdata, char *oktext)
{
	struct Box_s *dialog;
	struct Box_s *pbox;
	struct autoeditdata_s *data = malloc(sizeof(*data));
	int x, y, h;

	dialog = Box_Create(0, 0, w, 0, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;
	
	dialog->titlebar = TitleBarOnly_Add(dialog, titlebartxt);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	data->callbackfunc = callbackfunc;
	data->userdata = userdata;

	h = TitleBarHeight + 5;

	pbox = Text_Create(20, h, w - 40, 0, BOX_TRANSPARENT | BOX_VISIBLE, TX_WRAP | TX_STRETCHVERT);
	pbox->fgcol = PresenceFG;
	Text_SetText(pbox, topinfotext);
	Box_AddChild(dialog, pbox);

	h += pbox->h + 5;

	pbox = Edit2Box_Create(20, h, w - 40, 20, BOX_VISIBLE, E2_HORIZ);
	pbox->bgcol = RGB(255, 255, 255);
	Edit2Box_SetText(pbox, defaulttext);
	Edit2Box_SetOnEnter(pbox, AutoEdit_OnEnter);
	Box_AddChild(dialog, pbox);
	data->textentry = pbox;

	h += pbox->h + 5;

	pbox = Text_Create(20, h, w - 40, 0, BOX_TRANSPARENT | BOX_VISIBLE, TX_WRAP | TX_STRETCHVERT);
	pbox->fgcol = PresenceFG;
	Text_SetText(pbox, bottominfotext);
	Box_AddChild(dialog, pbox);

	h += pbox->h + 20;

	if (!oktext)
	{
		oktext = _("Ok");
	}

	pbox = StdButton_Create(w - 200, h, 80, oktext, 0);
	Button2_SetOnButtonHit(pbox, AutoEdit_OnOK);
	Box_AddChild(dialog, pbox);
	
	pbox = StdButton_Create(w - 100, h, 80, _("Cancel"), 0);
	Button2_SetOnButtonHit(pbox, AutoEdit_OnCancel);
	Box_AddChild(dialog, pbox);

	h += pbox->h + 5;

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

	dialog->x = x;
	dialog->y = y;
	dialog->h = h;

	dialog->OnDestroy = AutoEdit_OnDestroy;
	dialog->boxdata = data;
	
	Box_CreateWndCustom(dialog, titlebartxt, parent->hwnd);

	Box_SetFocus(data->textentry);

	BringWindowToTop(dialog->hwnd);

	return dialog;
}
