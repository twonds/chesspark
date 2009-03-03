#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>

#include "box.h"

#include "button.h"
#include "combobox.h"
#include "titledrag.h"

#include "button2.h"
#include "constants.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "view.h"
#include "util.h"

#include "newchat.h"


struct newchatdata_s
{
	struct Box_s *nameentry;
	struct Box_s *nametitle;
	struct Box_s *systemcombo;
	struct Box_s *error;
};

void NewChat_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}


void NewChat_OnOK(struct Box_s *pbox)
{
	struct newchatdata_s *data = Box_GetRoot(pbox)->boxdata;
	char *name;
	
	name = Edit2Box_GetText(data->nameentry);

	if (!Jid_IsValid(name))
	{
		Box_SetText(data->error, _("That is not a valid jid!"));
		Box_Repaint(data->error);
		return;
	}

	if (name && strlen(name) > 0 && strcmp(ComboBox_GetSelectionName(data->systemcombo), _("Chesspark")) == 0)
	{
		char *at;
		char jid2[256];

		strcpy(jid2, name);

		at = strchr(jid2, '@');
		if (at)
		{
			*at = '\0';
		}

		strcat(jid2, "@chesspark.com");

		View_PopupChatDialog(jid2, Ctrl_GetDefaultNick(), 0);

		NewChat_OnCancel(pbox);
		return;
	}

	/*
	{
		char *barejid = Jid_Strip(name);
		char *loginjid = Jid_Strip(Model_GetLoginJid());

		if (stricmp(barejid, loginjid) == 0)
		{
			Box_SetText(data->error, "Need company? Try a chatroom!");
			Box_Repaint(data->error);
			free(loginjid);
			free(barejid);
			return;
		}
		free(loginjid);
		free(barejid);
	}
	*/

	View_PopupChatDialog(name, Ctrl_GetDefaultNick(), 0);
	
	NewChat_OnCancel(pbox);
}


void NewChat_OnDestroy(struct Box_s *pbox)
{
	struct newchatdata_s *data = pbox->boxdata;
}

void NewChatEdit_OnKey(struct Box_s *pbox, char *text)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct newchatdata_s *data = dialog->boxdata;

	if (text && strchr(text, '@'))
	{
		ComboBox_SetSelection(data->systemcombo, _("Other"));
		Box_SetText(data->nametitle, _("Jabber ID"));
	}
	else
	{
		ComboBox_SetSelection(data->systemcombo, _("Chesspark"));
		Box_SetText(data->nametitle, _("Chesspark Username"));
	}
	Box_Repaint(data->nametitle);
}

void NewChat_OnEnter(struct Box_s *pbox, char *text)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	NewChat_OnOK(dialog);
}


void NewChatCombo_OnSelection(struct Box_s *pbox, char *selection)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct newchatdata_s *data = dialog->boxdata;

	if (stricmp(selection, "Chesspark") == 0)
	{
		Box_SetText(data->nametitle, _("Chesspark Username"));
	}
	else
	{
		Box_SetText(data->nametitle, _("Jabber ID"));
	}
	Box_Repaint(data->nametitle);
}

struct Box_s *NewChat_Create(struct Box_s *roster)
{
	struct Box_s *dialog;
	struct Box_s *pbox;
	struct newchatdata_s *data = malloc(sizeof(*data));
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

	dialog->titlebar = TitleBarOnly_Add(dialog, _("New Chat With Friend"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	pbox = Box_Create(32, 32, 100, 16, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	/*Box_SetText(pbox, _("Jabber ID"));*/
	Box_SetText(pbox, _("Chesspark Username"));
	Box_AddChild(dialog, pbox);
	data->nametitle = pbox;

	pbox = Box_Create(32, 72, 200, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = PresenceFG;
	Box_AddChild(dialog, pbox);
	data->error = pbox;

	pbox = Box_Create(32, 128 - 32, 60, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("System"));
	Box_AddChild(dialog, pbox);

	pbox = ComboBox_Create(85, 128 - 32, 100, 20, BOX_VISIBLE);
	ComboBox_AddEntry(pbox, _("Chesspark"));
	ComboBox_AddEntry(pbox, _("Other"));
	ComboBox_SetSelection(pbox, _("Chesspark"));
	ComboBox_SetOnSelection(pbox, NewChatCombo_OnSelection);
	Box_AddChild(dialog, pbox);
	data->systemcombo = pbox;

	pbox = Edit2Box_Create(32, 48, 256+96, 20, BOX_VISIBLE, E2_HORIZ);
	pbox->bgcol = RGB(255, 255, 255);
	Box_AddChild(dialog, pbox);
	Edit2Box_SetOnKey(pbox, NewChatEdit_OnKey);
	Edit2Box_SetOnEnter(pbox, NewChat_OnEnter);
	data->nameentry = pbox;

	pbox = StdButton_Create(432 - 200, 128 - 32, 80, _("OK"), 0);
	Button2_SetOnButtonHit(pbox, NewChat_OnOK);
	Box_AddChild(dialog, pbox);
	
	pbox = StdButton_Create(432 - 100, 128 - 32, 80, _("Cancel"), 0);
	Button2_SetOnButtonHit(pbox, NewChat_OnCancel);
	Box_AddChild(dialog, pbox);

	dialog->OnDestroy = NewChat_OnDestroy;
	dialog->boxdata = data;
	
	Box_CreateWndCustom(dialog, _("New Chat With Friend"), roster->hwnd);

	Box_SetFocus(data->nameentry);
	
	BringWindowToTop(dialog->hwnd);

	return dialog;
}
