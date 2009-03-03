#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>

#include <commctrl.h>

#include "box.h"

#include "button.h"
#include "titledrag.h"

#include "button2.h"
#include "constants.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "imagemgr.h"
#include "imgcombo.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

#include "addstatus.h"


struct addstatusdata_s
{
	struct Box_s *typecombo;
	struct Box_s *nameentry;
};

void AddStatus_OnCancel(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}


void AddStatus_OnOK(struct Box_s *pbox)
{
	struct addstatusdata_s *data = pbox->parent->boxdata;
	char *name;

	name = Edit2Box_GetText(data->nameentry);

	if (!name || strlen(name) == 0)
	{
		return;
	}

	if (stricmp(ImgCombo_GetSelectionName(data->typecombo), _("Away")) == 0)
	{
		Ctrl_AddStatus(SSTAT_AWAY, name);
		Ctrl_SetPresence(SSTAT_AWAY, name);
	}
	else
	{
		Ctrl_AddStatus(SSTAT_AVAILABLE, name);
		Ctrl_SetPresence(SSTAT_AVAILABLE, name);
	}
	
	AddStatus_OnCancel(pbox);
}

void AddStatus_OnEnter(struct Box_s *pbox, char *text)
{
	AddStatus_OnOK(pbox);
}


struct Box_s *AddStatus_Create(struct Box_s *roster)
{
	struct Box_s *dialog;
	struct Box_s *pbox, *pbox2, *subbox;
	struct addstatusdata_s *data = malloc(sizeof(*data));

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
	
	dialog->titlebar = TitleBarOnly_Add(dialog, _("Add New Status"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	pbox = Box_Create(32, 32, 60, 16, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, _("Type"));
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(144, 32, 60, 16, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, _("Name"));
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(432 - 200, 128 - 32, 80, _("Ok"), 0);
	Button2_SetOnButtonHit(pbox, AddStatus_OnOK);
	Box_AddChild(dialog, pbox);
	
	pbox = StdButton_Create(432 - 100, 128 - 32, 80, _("Cancel"), 0);
	Button2_SetOnButtonHit(pbox, AddStatus_OnCancel);
	Box_AddChild(dialog, pbox);

	pbox = Edit2Box_Create(144, 48, 256, 20, BOX_VISIBLE | BOX_BORDER, 1);
	Edit2Box_SetOnEnter(pbox, AddStatus_OnEnter);
	pbox->bgcol = EditBG;
	pbox->fgcol = EditFG;
	Box_AddChild(dialog, pbox);
	data->nameentry = pbox;

	pbox = ImgCombo_Create(32, 48, 96, 20, BOX_VISIBLE | BOX_BORDER);
	pbox->bgcol = EditBG;
	pbox->fgcol = EditFG;
	Box_AddChild(dialog, pbox);
	data->typecombo = pbox;

	dialog->boxdata = data;
	
	Box_CreateWndCustom(dialog, _("Add New Status"), roster->hwnd);
	
	/*
	data->typecombo = CreateWindow("ComboBoxEx32", NULL, WS_VISIBLE | CBS_DROPDOWNLIST | WS_CHILD,
				32, 48, 96, 9 * 8,
				dialog->hwnd, NULL, GetModuleHandle(NULL), NULL);
	*/

/*
	data->nameentry = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD,
				144, 48, 256, 24,
				dialog->hwnd, NULL, GetModuleHandle(NULL), NULL);
*/
	pbox = Box_Create(0, 0, 86, 20, BOX_VISIBLE);
	pbox->bgcol = ComboBG;

	subbox = Box_Create(0, 0, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->bgcol = ComboBG;
	subbox->img = ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16);
	Box_AddChild(pbox, subbox);

	subbox = Box_Create(18, 2, 58, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = ComboFG;
	Box_SetText(subbox, _("Available"));
	Box_AddChild(pbox, subbox);

	pbox2 = Box_Create(0, 0, 86, 20, BOX_VISIBLE);
	/*pbox2->bgcol = RGB(160, 160, 160);*/

	subbox = Box_Create(0, 0, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16);
	Box_AddChild(pbox2, subbox);

	subbox = Box_Create(18, 2, 58, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_SetText(subbox, "Available");
	Box_AddChild(pbox2, subbox);

	ImgCombo_AddEntry(data->typecombo, _("Available"), pbox, pbox2);
	
	pbox->child->img = ImageMgr_GetSubImage("presenceAway", "PresenceIcons.png", 48, 0, 16, 16);
	Box_SetText(pbox->child->sibling, _("Away"));
	
	pbox2->child->img = ImageMgr_GetSubImage("presenceAway", "PresenceIcons.png", 48, 0, 16, 16);
	Box_SetText(pbox2->child->sibling, _("Away"));

	ImgCombo_AddEntry(data->typecombo, _("Away"), pbox, pbox2);

	Box_Destroy(pbox);

	ImgCombo_SetSelection(data->typecombo, _("Available"));

	/*dialog->focus = data->nameentry;*/
	Box_SetFocus(data->nameentry);
/*	SetFocus(data->nameentry);*/

	BringWindowToTop(dialog->hwnd);

	return dialog;
}
