#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <wingdi.h>

#include "box.h"

#include "button.h"
#include "edit.h"
#include "text.h"
#include "util.h"

#include "constants.h"
#include "edit2.h"
#include "namedlist.h"

#include "imagemgr.h"

struct comboboxdata_s
{
	struct namedlist_s *entries;
	struct Box_s *button;
	struct Box_s *selectedbox;
	struct Box_s *popup;
	char *selectionname;
	int hasedit;
	int hover;
	int disabled;
	void (*onSelection)(struct Box_s *, char *);
	void (*onLoseFocus)(struct Box_s *);
};

void ComboBox_OnDestroy(struct Box_s *combo)
{
	struct comboboxdata_s *data = combo->boxdata;

	if (data->popup)
	{
		data->popup->OnInactive = NULL;
		Box_Destroy(data->popup);
	}
}

void ComboBox_SetSelection(struct Box_s *combo, char *name);

void ComboBox_AddEntry(struct Box_s *combo, char *name)
{
	struct comboboxdata_s *data = combo->boxdata;

	NamedList_Add(&(data->entries), name, NULL, NULL);
}

void ComboBox_AddEntry2(struct Box_s *combo, char *name, char *showtext)
{
	struct comboboxdata_s *data = combo->boxdata;

	NamedList_Add(&(data->entries), name, strdup(showtext), NULL);
}


void ComboBox_RemoveAllEntries(struct Box_s *combo)
{
	struct comboboxdata_s *data = combo->boxdata;

	while (data->entries)
	{
		NamedList_Remove(&(data->entries));
	}
}

void ComboBox_RemoveEntry(struct Box_s *combo, char *name)
{
	struct comboboxdata_s *data = combo->boxdata;

	NamedList_RemoveByName(&(data->entries), name);
}



struct comboboxpopupdata_s
{
	struct Box_s *combo;
	struct namedlist_s *comboentries;
	struct Box_s *currentselection;
};

void ComboBoxPopup_OnKeyDown(struct Box_s *popup, int vk, int scan)
{
	struct comboboxpopupdata_s *data = popup->boxdata;
	char keystate[256];
	WCHAR unicode;
	
	GetKeyboardState(keystate);

	if (Util_OldWinVer())
	{
		char ascii[2];

		int len = ToAscii(vk, scan, keystate, (LPWORD)ascii, 0);

		if (!len)
		{
			return;
		}

		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ascii, len, &unicode, 1);
	}
	else
	{
		ToUnicode(vk, scan, keystate, &unicode, 1, 0);
	}

	if (data->currentselection)
	{
		Button_SetNormalState(data->currentselection);
	}

	if (vk == 38) /* up */
	{
		struct namedlist_s *entry = data->comboentries;
		int set = 0;

		if (entry->data == data->currentselection)
		{
			while (entry && entry->next)
			{
				entry = entry->next;
			}
			data->currentselection = entry->data;
		}
		else
		{
			while (entry && entry->next && !set)
			{
				if (entry->next && entry->next->data == data->currentselection)
				{
					data->currentselection = entry->data;
					set = 1;
				}
				entry = entry->next;
			}
		}
	}
	else if (vk == 40) /* down */
	{
		struct namedlist_s *entry = data->comboentries;
		int set = 0;

		while (entry && !set)
		{
			if (entry->data == data->currentselection)
			{
				if (entry->next)
				{
					data->currentselection = entry->next->data;
				}
				else
				{
					data->currentselection = NULL;
				}
				set = 1;
			}
			entry = entry->next;
		}
		if (!data->currentselection)
		{
			data->currentselection = data->comboentries->data;
		}
	}
	else if (vk == 13) /* enter */
	{
		if (data->currentselection)
		{
			Button_Trigger(data->currentselection);
		}
	}
	else if (vk == 27) /* escape */
	{
		popup->OnInactive = NULL;
		Box_Destroy(popup);
		return;
	}

	if (data->currentselection)
	{
		Button_SetHoverState(data->currentselection);
	}
}

void ComboBoxPopup_OnDestroy(struct Box_s *popup)
{
	struct comboboxpopupdata_s *pdata = popup->boxdata;
	struct Box_s *combo = pdata->combo;
	struct comboboxdata_s *data = combo->boxdata;

	data->button->img = ImageMgr_GetSubImage("combobox-normal", "combobox.png", 0, 0, 19, 18);
	Box_Repaint(data->button);
	data->popup = NULL;
}


void ComboBoxPopup_OnChoice(struct Box_s *pbox, char *name)
{
	struct Box_s *popup = Box_GetRoot(pbox);
	struct comboboxpopupdata_s *pdata = popup->boxdata;
	struct Box_s *combo = pdata->combo;
	struct comboboxdata_s *data = combo->boxdata;
	
	if (name)
	{
		ComboBox_SetSelection(combo, name);
	}

	/* so Box_Destroy is not called twice */
	popup->OnInactive = NULL;

	Box_Destroy(popup);

	if (name && data->onSelection)
	{
		data->onSelection(combo, name);
	}
}

void ComboBox_ShowPopup(struct Box_s *combo)
{
	struct Box_s *popup;
	struct comboboxdata_s *data = combo->boxdata;
	struct namedlist_s *current = data->entries;
	struct comboboxpopupdata_s *pdata;
	int x, y;

	if (!current)
	{
		return;
	}

	data->button->img = ImageMgr_GetSubImage("combobox-pressed", "combobox.png", 19, 0, 19, 18);
	Box_Repaint(data->button);

	Box_GetScreenCoords(combo, &x, &y);

	pdata = malloc(sizeof(*pdata));
	memset(pdata, 0, sizeof(*pdata));

	pdata->combo = combo;

	popup = Box_Create(x, y + combo->h - 1, combo->w, combo->h, BOX_VISIBLE | BOX_BORDER);
	popup->boxdata = pdata;
	popup->bgcol = RGB(255, 255, 255);
	popup->OnInactive = Box_Destroy;
	popup->OnDestroy = ComboBoxPopup_OnDestroy;
	popup->OnKeyDown = ComboBoxPopup_OnKeyDown;
	data->popup = popup;

	y = 2;

	while (current)
	{
		char *text = current->data;

		if (!text)
		{
			text = current->name;
		}

		if (text)
		{
			struct Box_s *subbox = Button_Create(5, y, combo->w - 10, 16, BOX_VISIBLE);

			subbox->bgcol = RGB(255, 255, 255);
			Button_SetNormalBG(subbox, RGB(255, 255, 255));
			Button_SetNormalFG(subbox, RGB(0, 0, 0));
			Button_SetHoverBG(subbox, RGB(153, 153, 153));
			Button_SetHoverFG(subbox, RGB(255, 255, 255));
			Button_SetPressedBG(subbox, RGB(153, 153, 153));
			Button_SetPressedFG(subbox, RGB(255, 255, 255));
			Button_SetOnButtonHit2(subbox, ComboBoxPopup_OnChoice, strdup(current->name));
			Box_AddChild(popup, subbox);

			NamedList_Add(&(pdata->comboentries), NULL, subbox, NULL);

			subbox = Text_Create(10, y, combo->w - 10, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
			Text_SetText(subbox, text);
			Box_AddChild(popup, subbox);

			y += subbox->h;
		}
		else
		{
			struct Box_s *subbox = Box_Create(5, y + 5, combo->w - 10, 1, BOX_VISIBLE);
			y += 12;
			subbox->bgcol = RGB(153, 153, 153);
			Box_AddChild(popup, subbox);
		}

		current = current->next;
	}

	popup->h = y + 2;

	Box_CreateWndMenu(popup, combo->hwnd);
	SetForegroundWindow(popup->hwnd);
	BringWindowToTop(popup->hwnd);
	popup->active = TRUE;
	Box_SetFocus(popup);
}

void ComboBox_OnLButtonDown(struct Box_s *combo, int x, int y)
{
	struct comboboxdata_s *data = combo->boxdata;
	struct namedlist_s *current = data->entries;

	if (data->hasedit && x < combo->w - combo->h)
	{
		Box_SetFocus(combo);
		Box_OnLButtonDown(combo, x, y);
		return;
	}

	if (!current)
	{
		return;
	}

	Box_SetFocus(combo);

	ComboBox_ShowPopup(combo);
}

void ComboBox_OnKeyDown(struct Box_s *pbox, int vk, int scan)
{
	struct comboboxdata_s *data = pbox->boxdata;
	char keystate[256];
	WCHAR unicode;
	
	GetKeyboardState(keystate);

	if (Util_OldWinVer())
	{
		char ascii[2];

		int len = ToAscii(vk, scan, keystate, (LPWORD)ascii, 0);

		if (!len)
		{
			return;
		}

		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ascii, len, &unicode, 1);
	}
	else
	{
		ToUnicode(vk, scan, keystate, &unicode, 1, 0);
	}

	if (vk == 38) /* up */
	{
		ComboBox_ShowPopup(pbox);
	}
	else if (vk == 40) /* down */
	{
		ComboBox_ShowPopup(pbox);
	}
	if (vk == 9) /* tab */
	{
		Box_SetNextFocus(pbox, keystate[VK_SHIFT] & 0x8000);
	}
	else if (data->hasedit)
	{
		Edit2Box_OnKeyDown(data->selectedbox, vk, scan);
	}
}

void ComboBox_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct comboboxdata_s *data = pbox->boxdata;

	if (data->disabled)
	{
		return;
	}

	Box_OnMouseMove(pbox, xmouse, ymouse);

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

void ComboBox_SetSelection(struct Box_s *combo, char *name)
{
	struct comboboxdata_s *data = combo->boxdata;
	struct namedlist_s **entry = NamedList_GetByName(&(data->entries), name);
	char *showtext;

	if (entry && (*entry)->data)
	{
		showtext = (*entry)->data;
	}
	else
	{
		showtext = name;
	}

	if (data->hasedit)
	{
		Edit2Box_SetText(data->selectedbox, name);
	}
	else
	{
		Text_SetText(data->selectedbox, showtext);
	}

	free(data->selectionname);
	data->selectionname = strdup(name);

	Box_Repaint(data->selectedbox);
}

void ComboBox_SetSelection2(struct Box_s *combo, char *name, char *showtext)
{
	struct comboboxdata_s *data = combo->boxdata;

	if (data->hasedit)
	{
		Edit2Box_SetText(data->selectedbox, showtext);
	}
	else
	{
		Text_SetText(data->selectedbox, showtext);
	}

	free(data->selectionname);
	data->selectionname = strdup(name);

	Box_Repaint(data->selectedbox);
}

char *ComboBox_GetSelectionName(struct Box_s *combo)
{
	struct comboboxdata_s *data = combo->boxdata;

	if (data->hasedit)
	{
		return Edit2Box_GetText(data->selectedbox);
	}

	return data->selectionname;
/*
	return data->selectedbox->text;
*/
}

void ComboBox_OnGetFocus(struct Box_s *combo)
{
	struct comboboxdata_s *data = combo->boxdata;

	if (data->hasedit)
	{
		Edit2Box_OnGetFocus(data->selectedbox);
	}

	Box_Repaint(data->selectedbox);
}

void ComboBox_OnLoseFocus(struct Box_s *combo)
{
	struct comboboxdata_s *data = combo->boxdata;

	if (data->hasedit)
	{
		Edit2Box_OnLoseFocus(data->selectedbox);
	}

	if (data->onLoseFocus)
	{
		data->onLoseFocus(combo);
	}

	Box_Repaint(data->selectedbox);
}

struct Box_s *ComboBox_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Box_s *combo, *subbox;
	struct comboboxdata_s *data;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	combo = Box_Create(x, y, w, h, flags);
	combo->bgcol = RGB(255, 255, 255);
	combo->fgcol = RGB(0, 0, 0);
	combo->OnLButtonDown = ComboBox_OnLButtonDown;
	combo->OnMouseMove = ComboBox_OnMouseMove;
	combo->boxdata = data;
	combo->OnDestroy = ComboBox_OnDestroy;
	combo->OnGetFocus = ComboBox_OnGetFocus;
	combo->OnLoseFocus = ComboBox_OnLoseFocus;
	combo->OnKeyDown = ComboBox_OnKeyDown;

	subbox = Text_Create(5, (h - 16) / 2, w - 10 - h, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
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
	data->button = subbox;
	Box_AddChild(combo, subbox);

	return combo;
}

void ComboBox_SetOnSelection(struct Box_s *combo, void (*onSelection)(struct Box_s *, char *))
{
	struct comboboxdata_s *data = combo->boxdata;

	data->onSelection = onSelection;
}

void ComboBox_SetDisabled(struct Box_s *combo, int disabled)
{
	struct comboboxdata_s *data = combo->boxdata;

	data->disabled = disabled;

	if (disabled)
	{
		combo->bgcol = RGB(128, 128, 128);
		data->button->img = ImageMgr_GetSubImage("combobox-disabled", "combobox.png", 57, 0, 19, 18);
	}
	else
	{
		combo->bgcol = RGB(255, 255, 255);
		data->button->img = ImageMgr_GetSubImage("combobox-normal", "combobox.png", 0, 0, 19, 18);
	}

	Box_Repaint(combo);
}

struct Box_s *ComboEditBox_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Box_s *combo = ComboBox_Create(x, y, w, h, flags);
	struct Box_s *subbox;
	struct comboboxdata_s *data = combo->boxdata;

	Box_Destroy(data->selectedbox);

	subbox = Edit2Box_Create(0, 0, w - 10 - h, 20, BOX_VISIBLE, E2_HORIZ | E2_NOFOCUS);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->bgcol = RGB(255, 255, 255);
	subbox->fgcol = RGB(0, 0, 0);
	data->selectedbox = subbox;
	Box_AddChild(combo, subbox);

	data->hasedit = 1;

	return combo;
}

void ComboEditBox_SetOnEnter(struct Box_s *combo, void *pfunc)
{
	struct comboboxdata_s *data = combo->boxdata;

	Edit2Box_SetOnEnter(data->selectedbox, pfunc);
}

void ComboEditBox_SetOnLoseFocus(struct Box_s *combo, void *pfunc)
{
	struct comboboxdata_s *data = combo->boxdata;

	data->onLoseFocus = pfunc;
}