#include <windowsx.h>

#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "titledrag.h"
#include "text.h"

#include "constants.h"

#include "button2.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

#include "add.h"


struct invitetochatdata_s
{
	struct Box_s *error;
	struct Box_s *systemcombo;
	struct Box_s *nametitle;
	struct Box_s *friendentry;
	struct Box_s *chatentry;
};


void InviteToChat_Error(struct Box_s *pbox, char *error)
{
	struct invitetochatdata_s *data = pbox->boxdata;
	
	data->error->fgcol = CR_Red;
	Text_SetText(data->error, error);
	Box_Repaint(data->error);
}

void InviteToChat_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}


void InviteToChat_OnOK(struct Box_s *pbox)
{
	struct invitetochatdata_s *data = pbox->parent->boxdata;
	char *friendjid, *chatjid;
	char escapedchatjid[1024];

	friendjid = Edit2Box_GetText(data->friendentry);
	chatjid = Edit2Box_GetText(data->chatentry);	

	if (!Jid_IsValid(friendjid))
	{
		InviteToChat_Error(Box_GetRoot(pbox), _("The friend JID you entered appears to be malformed."));
		return;
	}
/*
	if (!Jid_IsValid(chatjid))
	{
		InviteToChat_Error(Box_GetRoot(pbox), _("The chat JID you entered appears to be malformed."));
		return;
	}
*/
	EscapeJID(chatjid, escapedchatjid, 1024);

	if (friendjid && strlen(friendjid) > 0 && strcmp(ComboBox_GetSelectionName(data->systemcombo), _("Chesspark")) == 0)
	{
		char *at;
		char jid2[512];

		strcpy(jid2, friendjid);

		at = strchr(jid2, '@');
		if (at)
		{
			*at = '\0';
		}

		strcat(jid2, "@chesspark.com");
		Ctrl_InviteToChat(jid2, escapedchatjid);

		InviteToChat_OnCancel(pbox);
		return;
	}

	Ctrl_InviteToChat(friendjid, escapedchatjid);

	InviteToChat_OnCancel(pbox);
}

void InviteToChatEdit_OnKey(struct Box_s *pbox, char *text)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct invitetochatdata_s *data = dialog->boxdata;

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

void InviteToChatEdit_OnEnter(struct Box_s *pbox, char *text)
{
	InviteToChat_OnOK(pbox);
}

void InviteToChatCombo_OnSelection(struct Box_s *pbox, char *selection)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct invitetochatdata_s *data = dialog->boxdata;

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

struct Box_s *InviteToChat_Create(struct Box_s *roster, char *friendjid, char *chatjid)
{
	struct invitetochatdata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *pbox;
	int x, y;
	char unescaped[1024];

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
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - 256) / 2;
	}

	memset(data, 0, sizeof(*data));

	dialog = Box_Create(x, y, 432, 256, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;

	pbox = Box_Create(32, 32, 368, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Chesspark Username"));
	data->nametitle = pbox;
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(32, 80, 368, 16, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, _("For example: ChessFan, roboknight@chesspark.com"));
	Box_AddChild(dialog, pbox);
	data->error = pbox;

	pbox = Box_Create(32, 112, 368, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("Chatroom"));
	Box_AddChild(dialog, pbox);

	pbox = Text_Create(32, 160, 368, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
	pbox->fgcol = UserInfoFG2;
	Text_SetText(pbox, _("This is the chatroom your friend will be invited to.\n"));
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(432 - 200, 256 - 32, 80, _("OK"), 0);
	Button2_SetOnButtonHit(pbox, InviteToChat_OnOK);
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(432 - 100, 256 - 32, 80, _("Cancel"), 0);
	Button2_SetOnButtonHit(pbox, InviteToChat_OnCancel);
	Box_AddChild(dialog, pbox);

	pbox = Edit2Box_Create(32, 54, 368, 18, BOX_VISIBLE, 1);
	Edit2Box_SetOnKey(pbox, InviteToChatEdit_OnKey);
	Edit2Box_SetOnEnter(pbox, InviteToChatEdit_OnEnter);
	pbox->bgcol = CR_White;
	pbox->brcol = CR_Black;
	pbox->fgcol = CR_Black;
	Box_AddChild(dialog, pbox);
	data->friendentry = pbox;

	pbox = Edit2Box_Create(32, 134, 368, 18, BOX_VISIBLE, 1);
	Edit2Box_SetOnEnter(pbox, InviteToChatEdit_OnEnter);
	pbox->bgcol = CR_White;
	pbox->brcol = CR_Black;
	pbox->fgcol = CR_Black;
	Box_AddChild(dialog, pbox);
	data->chatentry = pbox;

	pbox = Box_Create(32, 256 - 32, 60, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = UserInfoFG2;
	Box_SetText(pbox, _("System"));
	Box_AddChild(dialog, pbox);

	pbox = ComboBox_Create(85, 256 - 32, 100, 20, BOX_VISIBLE);
	ComboBox_AddEntry(pbox, _("Chesspark"));
	ComboBox_AddEntry(pbox, _("Other"));
	ComboBox_SetSelection(pbox, _("Chesspark"));
	ComboBox_SetOnSelection(pbox, InviteToChatCombo_OnSelection);
	Box_AddChild(dialog, pbox);
	data->systemcombo = pbox;

	dialog->titlebar = TitleBarOnly_Add(dialog, _("Invite Friend to Chat"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	dialog->boxdata = data;
	
	Box_CreateWndCustom(dialog, _("Invite Friend To Chat"), roster->hwnd);
	
	if (friendjid)
	{
		Edit2Box_SetText(data->friendentry, friendjid);
		Box_Repaint(data->friendentry);
	}

	if (chatjid)
	{
		Edit2Box_SetText(data->chatentry, UnescapeJID(chatjid, unescaped, 1024));
		Box_Repaint(data->chatentry);
	}

	BringWindowToTop(dialog->hwnd);

	return dialog;
}
