#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <wingdi.h>

#include "box.h"

#include "button.h"
#include "edit.h"

#include "button2.h"
#include "constants.h"
#include "namedlist.h"

#include "imagemgr.h"

struct imgcombodata_s
{
	struct namedlist_s *entries;
	struct Box_s *selectedbox;
	struct Box_s *button;
	char *currentselection;
	void (*onSelection)(struct Box_s *, char *);
	int hover;
};

struct imgcomboentrydata_s
{
	struct BoxImage_s *normal;
	struct BoxImage_s *selected;
};

void ImgCombo_SetSelection(struct Box_s *combo, char *name);

void ImgCombo_AddEntry(struct Box_s *combo, char *name, struct Box_s *normal, struct Box_s *selected)
{
	struct imgcombodata_s *data = combo->boxdata;
	struct imgcomboentrydata_s *edata = malloc(sizeof(*edata));

	edata->normal   = BoxImage_CreateFromBox(normal, 0);
	edata->selected = BoxImage_CreateFromBox(selected, 0);

	NamedList_Add(&(data->entries), name, edata, NULL);
}

void ImgCombo_RemoveAllEntries(struct Box_s *combo)
{
	struct imgcombodata_s *data = combo->boxdata;

	while (data->entries)
	{
		struct imgcomboentrydata_s *edata = data->entries->data;

		if (edata->normal == data->selectedbox->img)
		{
			data->selectedbox->img = NULL;
			Box_Repaint(data->selectedbox);
		}
		NamedList_Remove(&(data->entries));
	}
}

void ImgCombo_RemoveEntry(struct Box_s *combo, char *name)
{
	struct imgcombodata_s *data = combo->boxdata;
	struct namedlist_s *current = data->entries;

	while (current)
	{
		struct imgcomboentrydata_s *edata = data->entries->data;

		if ((edata->normal == data->selectedbox->img) && stricmp(current->name, name) == 0)
		{
			data->selectedbox->img = NULL;
			Box_Repaint(data->selectedbox);
		}
		current = current->next;
	}

	NamedList_RemoveByName(&(data->entries), name);
}

struct imgcombopopupdata_s
{
	struct Box_s *combo;
	char *name;
};

void ImgComboPopup_DestroyAll(struct Box_s *pbox)
{
	struct Box_s *popup = Box_GetRoot(pbox);
	struct imgcombopopupdata_s *pdata = popup->boxdata;
	struct Box_s *combo = pdata->combo;
	struct imgcombodata_s *data = combo->boxdata;

	data->button->img = ImageMgr_GetSubImage("combobox-normal", "combobox.png", 0, 0, 18, 18);
	Box_Repaint(combo->child->sibling);

	if (pbox->hwnd)
	{
		HWND old;

		old = pbox->hwnd;
		pbox->hwnd = NULL;
		DestroyWindow(old);
		Box_Destroy(pbox);
	}
}

void ImgComboPopup_OnChoice(struct Box_s *pbox, char *name)
{
	struct Box_s *popup = Box_GetRoot(pbox);
	struct imgcombopopupdata_s *pdata = popup->boxdata;
	struct Box_s *combo = pdata->combo;
	struct imgcombodata_s *data = combo->boxdata;
	struct namedlist_s *current = data->entries;
/*
	while (current)
	{
		struct imgcomboentrydata_s *edata = current->data;

		struct BoxImage_s *currentimage = edata->normal;
		if (currentimage == pbox->child->img)
		{
			ImgCombo_SetSelection(combo, current->name);
		}

		current = current->next;
	}
*/
	ImgCombo_SetSelection(combo, name);

	ImgComboPopup_DestroyAll(popup);

	if (data->onSelection)
	{
		data->onSelection(combo, data->currentselection);
	}
}

void ImgCombo_OnLButtonDown(struct Box_s *combo, int x, int y)
{
	struct Box_s *popup;
	struct imgcombodata_s *data = combo->boxdata;
	struct namedlist_s *current = data->entries;
	struct imgcombopopupdata_s *pdata;

	if (!current)
	{
		return;
	}

	data->button->img = ImageMgr_GetSubImage("combobox-pressed", "combobox.png", 19, 0, 19, 18);
	Box_Repaint(combo->child->sibling);

	Box_GetScreenCoords(combo, &x, &y);

	pdata = malloc(sizeof(*pdata));
	memset(pdata, 0, sizeof(*pdata));

	pdata->combo = combo;

	popup = Box_Create(x, y + combo->h, combo->w, combo->h, BOX_VISIBLE | BOX_BORDER);
	popup->boxdata = pdata;
	popup->bgcol = RGB(255, 255, 255);
	popup->OnInactive = ImgComboPopup_DestroyAll;

	y = 2;

	while (current)
	{
		struct Box_s *subbox = Button2_Create(5, y, combo->w - 10, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		struct Box_s *normal, *hover, *pressed;
		struct imgcomboentrydata_s *edata = current->data;

		Button2_SetOnButtonHit2(subbox, ImgComboPopup_OnChoice, current->name);

		Box_AddChild(popup, subbox);

		normal = Box_Create(5, y, combo->w - 10, 16, BOX_VISIBLE);
		normal->img = edata->normal;
		Box_AddChild(popup, normal);

		hover = Box_Create(5, y, combo->w - 10, 16, 0);
		hover->img = edata->selected;
		Box_AddChild(popup, hover);

		pressed = Box_Create(5, y, combo->w - 10, 16, 0);
		pressed->img = edata->selected;
		Box_AddChild(popup, pressed);

		Button2_SetNormal(subbox, normal);
		Button2_SetHover(subbox, hover);
		Button2_SetPressed(subbox, pressed);

		y += subbox->h;
		current = current->next;
	}

	popup->h = y + 2;

	Box_CreateWndMenu(popup, combo->hwnd);
	SetForegroundWindow(popup->hwnd);
	BringWindowToTop(popup->hwnd);
	popup->active = TRUE;
}

void ImgCombo_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct imgcombodata_s *data = pbox->boxdata;

	if (Box_GetRoot(pbox)->active)
	{
		if (!data->hover && Box_CheckXYInBox(pbox, xmouse, ymouse))
		{
			data->button->img = ImageMgr_GetSubImage("combobox-hover", "combobox.png", 38, 0, 19, 18);
			data->hover = 1;
			Box_Repaint(data->button);
		}
		else if (data->hover && !Box_CheckXYInBox(pbox, xmouse, ymouse))
		{
			data->button->img = ImageMgr_GetSubImage("combobox-normal", "combobox.png", 0, 0, 19, 18);
			data->hover = 0;
			Box_Repaint(data->button);
		}
	}
}

void ImgCombo_SetSelection(struct Box_s *combo, char *name)
{
	struct imgcombodata_s *data = combo->boxdata;
	struct namedlist_s **entry = NamedList_GetByName(&(data->entries), name);
	struct imgcomboentrydata_s *edata;

	if (!entry)
	{
		return;
	}

	edata = (*entry)->data;

	data->selectedbox->img = edata->normal;
	free(data->currentselection);
	data->currentselection = strdup((*entry)->name);

	Box_Repaint(data->selectedbox);
}

char *ImgCombo_GetSelectionName(struct Box_s *combo)
{
	struct imgcombodata_s *data = combo->boxdata;

	return data->currentselection;
}

struct Box_s *ImgCombo_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Box_s *combo, *subbox;
	struct imgcombodata_s *data;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	combo = Box_Create(x, y, w, h, flags);
	combo->bgcol = RGB(255, 255, 255);
	combo->fgcol = RGB(0, 0, 0);
	combo->OnLButtonDown = ImgCombo_OnLButtonDown;
	combo->OnMouseMove = ImgCombo_OnMouseMove;
	combo->boxdata = data;

	subbox = Box_Create(5, (h - 16) / 2, w - 10 - h, 16, BOX_VISIBLE);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->bgcol = RGB(255, 255, 255);
	subbox->fgcol = RGB(0, 0, 0);
	data->selectedbox = subbox;
	Box_AddChild(combo, subbox);

	subbox = Box_Create(w - 20, 1, 19, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetSubImage("combobox-normal", "combobox.png", 0, 0, 19, 18);
	subbox->bgcol = RGB(128, 128, 128);
	subbox->fgcol = RGB(0, 0, 0);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	/*Box_SetText(subbox, "^");*/
	Box_AddChild(combo, subbox);
	data->button = subbox;

	return combo;
}

void ImgCombo_SetOnSelection(struct Box_s *combo, void (*onSelection)(struct Box_s *, char *))
{
	struct imgcombodata_s *data = combo->boxdata;

	data->onSelection = onSelection;
}