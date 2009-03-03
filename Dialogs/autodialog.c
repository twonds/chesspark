#include <stdlib.h>

#include "box.h"

#include "text.h"

#include "button2.h"
#include "checkbox.h"
#include "constants.h"
#include "i18n.h"
#include "stdbutton.h"
#include "titlebar.h"

#include "menu.h"

struct autodialogdata_s
{
	struct Box_s *textbox;
	struct Box_s *dontshowagaincheck;
	void (*button1callback)(struct Box_s *dialog, void *userdata);
	void (*button2callback)(struct Box_s *dialog, void *userdata);
	void (*dontshowcallback)(struct Box_s *dialog, char *name);
	void *userdata;
	char *name;
};

void AutoDialog_OnOK(struct Box_s *dialog)
{
	Box_Destroy(Box_GetRoot(dialog));
}

void AutoDialog_OnButton1(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct autodialogdata_s *data = dialog->boxdata;

	if (data->dontshowcallback && CheckBox_GetChecked(data->dontshowagaincheck))
	{
		data->dontshowcallback(dialog, data->name);
	}

	if (data->button1callback)
	{
		data->button1callback(dialog, data->userdata);
	}
	else
	{
		AutoDialog_OnOK(dialog);
	}
}

void AutoDialog_OnButton2(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct autodialogdata_s *data = dialog->boxdata;

	if (data->dontshowcallback && CheckBox_GetChecked(data->dontshowagaincheck))
	{
		data->dontshowcallback(dialog, data->name);
	}

	if (data->button2callback)
	{
		data->button2callback(dialog, data->userdata);
	}
	else
	{
		AutoDialog_OnOK(dialog);
	}
}

void AutoDialog_OnKeyDown(struct Box_s *pbox, int vk, int scan)
{
	if (vk == 27 || vk == 13)
	{
		AutoDialog_OnButton1(pbox);
	}
}


struct Box_s *AutoDialog_Create2(struct Box_s *parent, int w, char *titletext,
  char *dialogtext, char *button1text, char *button2text,
  void (*button1callback)(struct Box_s *, void *),
  void (*button2callback)(struct Box_s *, void *), void *userdata, void (*dontshowcallback)(struct Box_s *, char *name), char *name)
{
	struct autodialogdata_s *data;

	struct Box_s *dialog, *subbox;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	data->button1callback = button1callback;
	data->button2callback = button2callback;
	data->userdata = userdata;
	data->dontshowcallback = dontshowcallback;
	data->name = strdup(name);

	dialog = Box_Create(0, 0, w, 10, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;
	dialog->titlebar = TitleBarOnly_Add(dialog, titletext);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;
	dialog->OnMinimize = Box_OnMinimize;
	dialog->OnRestore = Box_OnRestore;
	dialog->OnClose = Box_Destroy;
	dialog->focus = dialog;
	dialog->OnKeyDown = AutoDialog_OnKeyDown;

	subbox = Text_Create(20, 22 + 20, w - 40, 10, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT | TX_SELECTABLE | TX_COPYMENU);
	subbox->fgcol = UserInfoFG2;
	Text_SetLinkColor(subbox, CR_LtOrange);
	Text_SetText(subbox, dialogtext);
	Box_AddChild(dialog, subbox);
	data->textbox = subbox;
	dialog->w = subbox->w + 40;
	dialog->h = subbox->h + 22 + 20 + 20 + 20 + 5;

	if (button1text)
	{
		subbox = StdButton_Create(dialog->w - 110, dialog->h - 32, 90, button1text, 0);
	}
	else
	{
		subbox = StdButton_Create(dialog->w - 110, dialog->h - 32, 90, _("Close"), 0);
	}
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button2_SetOnButtonHit(subbox, AutoDialog_OnButton1);
	Box_AddChild(dialog, subbox);
	
	if (button2text)
	{
		subbox = StdButton_Create(dialog->w - 220, dialog->h - 32, 90, button2text, 0);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Button2_SetOnButtonHit(subbox, AutoDialog_OnButton2);
		Box_AddChild(dialog, subbox);
	}

	if (dontshowcallback)
	{
		subbox = CheckBox_Create(20, dialog->h - 32, BOX_VISIBLE);
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(dialog, subbox);
		data->dontshowagaincheck = subbox;

		subbox = CheckBoxLinkedText_Create(40, dialog->h - 32, 90, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->dontshowagaincheck);
		subbox->fgcol = UserInfoFG2;
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_SetText(subbox, _("Don't show again"));
		Box_AddChild(dialog, subbox);
	}

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;

		if (parent)
		{
			windowrect.left = parent->x;
			windowrect.right = windowrect.left + parent->w - 1;
			windowrect.top = parent->y;
			windowrect.bottom = windowrect.top + parent->h - 1;

			hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);
			mi.cbSize = sizeof(mi);
			GetMonitorInfo(hm, &mi);

			dialog->x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - dialog->w) / 2;
			dialog->y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - dialog->h) / 2;

			Box_CreateWndCustom(dialog, titletext, parent->hwnd);
		}
		else
		{
			dialog->x = (GetSystemMetrics(SM_CXFULLSCREEN) - dialog->w) / 2;
			dialog->y = (GetSystemMetrics(SM_CYFULLSCREEN) - dialog->h) / 2;
		
			Box_CreateWndCustom(dialog, titletext, NULL);
		}
	}


	dialog->boxdata = data;
	dialog->OnCommand = Menu_OnCommand;
	
	BringWindowToTop(dialog->hwnd);

	return dialog;
}

struct Box_s *AutoDialog_Create(struct Box_s *parent, int w, char *titletext,
  char *dialogtext, char *button1text, char *button2text,
  void (*button1callback)(struct Box_s *, void *),
  void (*button2callback)(struct Box_s *, void *), void *userdata)
{
	return AutoDialog_Create2(parent, w, titletext, dialogtext, button1text, button2text, button1callback, button2callback, userdata, NULL, NULL);
}

void AutoDialog_SetLinkCallback(struct Box_s *dialog, int linknum, void (*OnClick)(struct Box_s *, void *), void *userdata)
{
	struct autodialogdata_s *data = dialog->boxdata;

	Text_SetLinkCallback(data->textbox, linknum, OnClick, userdata);
}

void AutoDialog_ResetText(struct Box_s *dialog, char *titletext, char *dialogtext)
{
	struct autodialogdata_s *data = dialog->boxdata;
	int x, y, w, h;

	TitleBar_SetText(dialog->titlebar, titletext);
	Text_SetText(data->textbox, dialogtext);
	
	w = data->textbox->w + 40;
	h = data->textbox->h + 22 + 20 + 20 + 20 + 5;

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;

		windowrect.left = dialog->x;
		windowrect.right = windowrect.left + dialog->w - 1;
		windowrect.top = dialog->y;
		windowrect.bottom = windowrect.top + dialog->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - dialog->w) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - dialog->h) / 2;
	}

	Box_MoveWndCustom(dialog, x, y, w, h);
}
