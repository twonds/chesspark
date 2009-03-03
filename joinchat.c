#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>

#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "titledrag.h"
#include "text.h"
#include "constants.h"

#include "button2.h"
#include "combobox.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

#include "joinchat.h"


struct joinchatdata_s
{
	struct Box_s *error;
	struct Box_s *systemcombo;
	struct Box_s *nameentry;
};


void JoinChat_OnDestroy(struct Box_s *pbox)
{
	struct joinchatdata_s *data = pbox->boxdata;
}


void JoinChat_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}


void JoinChat_OnOK(struct Box_s *pbox)
{
	struct joinchatdata_s *data = Box_GetRoot(pbox)->boxdata;
	char *system, *name, fullname[512], escaped[1024];
	
	system = ComboBox_GetSelectionName(data->systemcombo);
	name = Edit2Box_GetText(data->nameentry);

	if (!name || strlen(name) == 0)
	{
		Text_SetText(data->error, _("Please enter a room name."));
		data->error->fgcol = PresenceFG;
		Box_Repaint(data->error);
		return;
	}
/*
	if (strchr(name, ' '))
	{
		Text_SetText(data->error, _("Room names cannot have spaces!"));
		data->error->fgcol = PresenceFG;
		Box_Repaint(data->error);
		return;
	}
*/
	if (strcmp(system, _("Chesspark")) == 0)
	{
		char *at = strchr(fullname, '@');
		strcpy(fullname, name);
		/*if(at)
		{
			*at = '\0';
		}*/
	
		strcat(fullname, "@chat.chesspark.com");
	}
	else
	{
		strcpy(fullname, name);
	}

	EscapeJID(fullname, escaped, 1024);

	Ctrl_JoinGroupChatDefaultNick(escaped);

	JoinChat_OnCancel(pbox);
}

void JoinChat_OnHelp(struct Box_s *pbox)
{
	Util_OpenURL("http://chesspark.com");
}

void JoinChat_OnEnter(struct Box_s *pbox, char *text)
{
	JoinChat_OnOK(Box_GetRoot(pbox));
}

void JoinChatEdit_OnKey(struct Box_s *pbox, char *text)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct joinchatdata_s *data = dialog->boxdata;

	if (text && strchr(text, '@'))
	{
		ComboBox_SetSelection(data->systemcombo, _("Other"));
	}
	else
	{
		ComboBox_SetSelection(data->systemcombo, _("Chesspark"));
	}
}

struct Box_s *JoinChat_Create(struct Box_s *roster)
{
	struct joinchatdata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *pbox;
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
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - 140) / 2;
	}

	memset(data, 0, sizeof(*data));

	dialog = Box_Create(x, y, 432, 140, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;

	pbox = Box_Create(39, 43, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, _("System"));
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(18, 75, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, _("Room Name"));
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(16, 114, 200, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	pbox->fgcol = UserInfoFG1;
	Text_SetLinkColor(pbox, CR_LtOrange);
	Text_SetText(pbox, _("^lNeed Help?^l"));
	Text_SetLinkCallback(pbox, 1, Util_OpenURL2, _("http://chesspark.com/help/joinchat/"));
	Box_AddChild(dialog, pbox);
	data->error = pbox;

	pbox = ComboBox_Create(80, 40, 312, 20, BOX_VISIBLE);
	ComboBox_AddEntry(pbox, _("Chesspark"));
	ComboBox_AddEntry(pbox, _("Other"));
	ComboBox_SetSelection(pbox, _("Chesspark"));
	Box_AddChild(dialog, pbox);
	data->systemcombo = pbox;

	pbox = Edit2Box_Create(80, 72, 312, 20, BOX_VISIBLE, E2_HORIZ);
	pbox->bgcol = RGB(255, 255, 255);
	Edit2Box_SetOnEnter(pbox, JoinChat_OnEnter);
	Edit2Box_SetOnKey(pbox, JoinChatEdit_OnKey);
	Box_AddChild(dialog, pbox);
	data->nameentry = pbox;
	
	pbox = StdButton_Create(432 - 200, 114, 80, _("OK"), 0);
	Button2_SetOnButtonHit(pbox, JoinChat_OnOK);
	Box_AddChild(dialog, pbox);
	
	pbox = StdButton_Create(432 - 100, 114, 80, _("Cancel"), 0);
	Button2_SetOnButtonHit(pbox, JoinChat_OnCancel);
	Box_AddChild(dialog, pbox);

	dialog->titlebar = TitleBarOnly_Add(dialog, _("Join Chat Room"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;
	dialog->OnDestroy = JoinChat_OnDestroy;
	dialog->boxdata = data;
	
	Box_CreateWndCustom(dialog, _("Join Chat Room"), roster->hwnd);
/*	
	data->systemcombo = CreateWindow("COMBOBOX", NULL, WS_VISIBLE | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP,
				80, 40, 312, 22 * 3,
				dialog->hwnd, NULL, GetModuleHandle(NULL), NULL);

	data->nameentry = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		80, 72, 312, 22,
		dialog->hwnd, NULL, GetModuleHandle(NULL), NULL);

	ComboBox_AddString(data->systemcombo, "Chesspark");
	ComboBox_AddString(data->systemcombo, "Other");
	ComboBox_SetCurSel(data->systemcombo, 0);
*/
	Box_SetFocus(data->nameentry);

	BringWindowToTop(dialog->hwnd);

	return dialog;
}
