#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>

#include "box.h"
#include "scroll.h"
#include "text.h"

#include "imagemgr.h"
#include "log.h"
#include "menu.h"
#include "scrollable.h"
#include "util.h"
#include "text.h"

#include "edit2.h"

struct edit2boxdata_s
{
	int disabled;
	int editflags;
	void (*OnEnter)(struct Box_s *pbox, char *text);
	void (*OnKey)(struct Box_s *pbox, char *text);
	void (*OnLoseFocus)(struct Box_s *pbox);
	void (*editsizefunc)(struct Box_s *pbox, int edith);
	struct Box_s *scrollable;
	struct Box_s *textbox;
	WCHAR *alttext;
};

void Edit2Box_ScrollCursorVisible(struct Box_s *pbox)
{
	struct edit2boxdata_s *data = pbox->boxdata;
	int cx, cy;

	Text_GetCursorXY(data->textbox, &cx, &cy);

	if (data->editflags & E2_HORIZ)
	{
		int lx = Scrollable_GetHScroll(data->scrollable);

		if (lx > cx)
		{
			Scrollable_SetHScroll(data->scrollable, cx);
		}
		else if (lx < cx - data->scrollable->w + 1)
		{
			Scrollable_SetHScroll(data->scrollable, cx - data->scrollable->w + 1);
		}
	}
	else
	{
		int ly = Scrollable_GetScroll(data->scrollable);

		if (ly > cy)
		{
	        Scrollable_SetScroll(data->scrollable, cy);
		}
		else if (ly < cy - data->scrollable->h + 14)
		{
			Scrollable_SetScroll(data->scrollable, cy - data->scrollable->h + 14);
		}
	}

	Box_Repaint(pbox);
}

void Edit2Box_ScrollToStart(struct Box_s *pbox)
{
	struct edit2boxdata_s *data = pbox->boxdata;
	int cx = 0, cy = 0;

	if (data->editflags & E2_HORIZ)
	{
		int lx = Scrollable_GetHScroll(data->scrollable);

		if (lx > cx)
		{
			Scrollable_SetHScroll(data->scrollable, cx);
		}
		else if (lx < cx - data->scrollable->w + 1)
		{
			Scrollable_SetHScroll(data->scrollable, cx - data->scrollable->w + 1);
		}
	}
	else
	{
		int ly = Scrollable_GetScroll(data->scrollable);

		if (ly > cy)
		{
	        Scrollable_SetScroll(data->scrollable, cy);
		}
		else if (ly < cy - data->scrollable->h + 14)
		{
			Scrollable_SetScroll(data->scrollable, cy - data->scrollable->h + 14);
		}
	}

	Box_Repaint(pbox);
}

void Edit2Box_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	if (data->disabled)
	{
		return;
	}

	Box_OnLButtonDown(pbox, xmouse, ymouse);

	if (!(data->editflags & E2_NOFOCUS))
	{
		Box_SetFocus(pbox);
	}
/*
	if (xmouse - data->scrollable->x > data->scrollable->w)
	{
		return;
	}

	Text_SetCursorToXYPos(data->textbox,
		xmouse - data->textbox->x + Scrollable_GetHScroll(data->scrollable),
		ymouse - data->textbox->y + Scrollable_GetScroll(data->scrollable));

	Edit2Box_ScrollCursorVisible(pbox);
*/
	Box_Repaint(pbox);
}

void Edit2Box_OnCut(struct Box_s *pbox)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	Text_OnCopy(data->textbox);
	Text_DeleteSelection(data->textbox);
}

void Edit2Box_OnCopy(struct Box_s *pbox)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	Text_OnCopy(data->textbox);
}

void Edit2Box_OnPaste(struct Box_s *pbox)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	if (IsClipboardFormatAvailable(CF_UNICODETEXT))
	{
		if (OpenClipboard(pbox->hwnd))
		{
			HGLOBAL hg;
			hg = GetClipboardData(CF_UNICODETEXT);
			if (hg)
			{
				WCHAR *pastetext = NULL;
				WCHAR *temp = GlobalLock(hg);

				if (temp)
				{
					pastetext = wcsdup(temp);
					GlobalUnlock(hg);

					Text_InsertWTextAtCursor(data->textbox, pastetext);

					free(pastetext);

					if (data->OnKey)
					{
						data->OnKey(pbox, Edit2Box_GetText(pbox));
					}
				}
			}
			CloseClipboard();
		}
	}
	else if (IsClipboardFormatAvailable(CF_TEXT))
	{
		if (OpenClipboard(pbox->hwnd))
		{
			HGLOBAL hg;
			hg = GetClipboardData(CF_TEXT);
			if (hg)
			{
				char *temptext = NULL;
				WCHAR *pastetext = NULL;
				char *temp = GlobalLock(hg);
				int len;

				if (temp)
				{
					temptext = strdup(temp);
					GlobalUnlock(hg);

					len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, temptext, -1, NULL, 0);

					pastetext = malloc((len + 1) * sizeof(WCHAR));

					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, temptext, -1, pastetext, len);

					pastetext[len] = L'\0';

					Text_InsertWTextAtCursor(data->textbox, pastetext);

					free(pastetext);

					if (data->OnKey)
					{
						data->OnKey(pbox, Edit2Box_GetText(pbox));
					}
				}
			}
			CloseClipboard();
		}
	}

	if (data->editsizefunc)
	{
		data->editsizefunc(pbox, data->textbox->h + 4);
	}

	Scrollable_Refresh(data->scrollable);

	Edit2Box_ScrollCursorVisible(pbox);

	Box_Repaint(pbox);
}

void Edit2Box_OnDelete(struct Box_s *pbox)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	Text_DeleteSelection(data->textbox);
}

void Edit2Box_OnKeyDown(struct Box_s *pbox, int vk, int scan)
{
	struct edit2boxdata_s *data = pbox->boxdata;
	char keystate[256];
	WCHAR unicode;
	int shiftdown, ctrldown, altdown;

	if (data->disabled)
	{
		return;
	}

	/* reject alt, ctrl, and shift keys, since they won't add a character */
	if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU)
	{
		Log_Write(0, "bail!\n");
		return;
	}
	
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

	ctrldown =  keystate[VK_CONTROL] & 0x8000;
	altdown =   keystate[VK_MENU]    & 0x8000;
	shiftdown = keystate[VK_SHIFT]   & 0x8000;
	
	if ((vk == 88 && ctrldown && !altdown) || (vk == 46 && shiftdown)) /* ctrl+x, shift+del */
	{
		Edit2Box_OnCut(pbox);
	}
	else if ((vk == 67 && ctrldown && !altdown)) /* ctrl+c */
	{
		Edit2Box_OnCopy(pbox);
	}
	else if ((vk == 86 && ctrldown && !altdown) || (vk == 45 && shiftdown)) /* ctrl+v, shift+ins */
	{
		Edit2Box_OnPaste(pbox);
	}
	else if ((vk == 65 && ctrldown && !altdown)) /* ctrl+a */
	{
		Text_SelectAll(data->textbox);
	}
	else if (vk == 8) /* backspace */
	{
		Text_BackspaceKey(data->textbox);
		if (data->OnKey)
		{
			data->OnKey(pbox, Edit2Box_GetText(pbox));
		}
	}
	else if (vk == 46) /* delete */
	{
		Text_DeleteKey(data->textbox);
		if (data->OnKey)
		{
			data->OnKey(pbox, Edit2Box_GetText(pbox));
		}
	}
	else if (vk == 9) /* tab */
	{
		Box_SetNextFocus(pbox, shiftdown);
	}
	else if (vk == 13) /* enter */
	{
		if (data->OnEnter && !ctrldown)
		{
			data->OnEnter(pbox, Edit2Box_GetText(pbox));
			return; /* Don't do anything in case the callback destroyed the window */
		}
		else if (!(data->editflags & E2_HORIZ))
		{
			WCHAR text[2];
			text[0] = L'\n';
			text[1] = L'\0';

			Text_InsertWTextAtCursor(data->textbox, text);

			if (data->OnKey)
			{
				data->OnKey(pbox, Edit2Box_GetText(pbox));
			}
		}
	}
	else if (vk == 35) /* end */
	{
		Text_EndKey(data->textbox, shiftdown);
	}
	else if (vk == 36) /* home */
	{
		Text_HomeKey(data->textbox, shiftdown);
	}
	else if (vk == 37) /* left */
	{
		Text_CursorLeft(data->textbox, shiftdown, ctrldown);
	}
	else if (vk == 38) /* up */
	{
		Text_CursorUp(data->textbox, shiftdown);
	}
	else if (vk == 39) /* right */
	{
		Text_CursorRight(data->textbox, shiftdown, ctrldown);
	}
	else if (vk == 40) /* down */
	{
		Text_CursorDown(data->textbox, shiftdown);
	}
	else if (unicode > 31) /* Displayable character */
	{
		WCHAR text[2];
		text[0] = unicode;
		text[1] = L'\0';

		Text_InsertWTextAtCursor(data->textbox, text);

		if (data->OnKey)
		{
			data->OnKey(pbox, Edit2Box_GetText(pbox));
		}
	}
	else
	{
		Log_Write(0, "unhandled vk %d\n", vk);
	}

	if (data->editsizefunc)
	{
		data->editsizefunc(pbox, data->textbox->h + 4);
	}

	Scrollable_Refresh(data->scrollable);

	Edit2Box_ScrollCursorVisible(pbox);

	Box_Repaint(pbox);
}

void Edit2Box_OnRButtonDown(struct Box_s *pbox, int x, int y)
{	
	struct edit2boxdata_s *data = pbox->boxdata;
	int sx, sy;
	int cancopy;

	Box_GetScreenCoords(pbox, &sx, &sy);
	sx += x;
	sy += y;

	cancopy = Text_TextSelected(data->textbox);

	Menu_PopupEditMenu(pbox, sx, sy, 1, cancopy, 1);
}

#define MIN(a,b) ((a)<(b))?(a):(b)
#define MAX(a,b) ((a)>(b))?(a):(b)
#define MIN3(a,b,c) MIN(MIN((a),(b)),(c))

extern RECT cliprects(RECT rc1, RECT rc2);

void Edit2Box_OnGetFocus(struct Box_s *pbox)
{
	struct edit2boxdata_s *data = pbox->boxdata;
	Text_SetShowCursor(data->textbox, 1);

	if (data->alttext)
	{
		Text_SetAltWText(data->textbox, NULL);
	}
}

void Edit2Box_OnLoseFocus(struct Box_s *pbox)
{
	struct edit2boxdata_s *data = pbox->boxdata;
	Text_SetShowCursor(data->textbox, 0);

	if (data->alttext)
	{
		Text_SetAltWText(data->textbox, data->alttext);
	}
}

struct Box_s *Edit2Box_Create(int x, int y, int w, int h, enum Box_flags flags, enum Edit2_flags editflags)
{
	struct Box_s *pbox = Box_Create(x, y, w, h, flags);
	struct Box_s *subbox;
	struct edit2boxdata_s *data = malloc(sizeof(*data));

	memset(data, 0, sizeof(*data));
	
	data->editflags = editflags;

	pbox->OnLButtonDown = Edit2Box_OnLButtonDown;
	pbox->OnRButtonDown = Edit2Box_OnRButtonDown;
	pbox->OnKeyDown     = Edit2Box_OnKeyDown;
	pbox->OnCut         = Edit2Box_OnCut;
	pbox->OnCopy        = Edit2Box_OnCopy;
	pbox->OnPaste	    = Edit2Box_OnPaste;
	pbox->OnDelete	    = Edit2Box_OnDelete;
	pbox->OnGetFocus    = Edit2Box_OnGetFocus;
	pbox->OnLoseFocus   = Edit2Box_OnLoseFocus;

	pbox->boxdata = data;

	subbox = Scrollable_Create(2, 2, w - 4, h - 4, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->fgcol = RGB(0, 0, 0);
	Box_AddChild(pbox, subbox);
	data->scrollable = subbox;

	subbox = Text_Create(0, 0, w - 4, h - 4, BOX_VISIBLE | BOX_TRANSPARENT, TX_SELECTABLE | TX_NOCOMMANDS
		| ((editflags & E2_HORIZ)    ? TX_STRETCHHORIZ : (TX_WRAP | TX_STRETCHVERT))
		| ((editflags & E2_PASSWORD) ? TX_PASSWORD     : 0));

	subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	subbox->fgcol = RGB(0, 0, 0);
	Scrollable_SetBox(data->scrollable, subbox);
	data->textbox = subbox;
	Text_SetCursorPos(subbox, 0, -1);
	/*Text_SetShowCursor(subbox, 1);*/
	
	return pbox;
}

void Edit2Box_SetOnEnter(struct Box_s *pbox, void *pfunc)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	data->OnEnter = pfunc;
}

void Edit2Box_SetOnKey(struct Box_s *pbox, void *pfunc)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	data->OnKey = pfunc;
}

void Edit2Box_ClearText(struct Box_s *pbox)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	Text_SetWText(data->textbox, NULL);

	if (data->editsizefunc)
	{
		data->editsizefunc(pbox, data->textbox->h + 4);
	}

	Scrollable_Refresh(data->scrollable);

	Edit2Box_ScrollCursorVisible(pbox);

	Box_Repaint(pbox);
}

char *Edit2Box_GetText(struct Box_s *pbox)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	return Text_GetText(data->textbox);
}

void Edit2Box_SetText(struct Box_s *pbox, char *text)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	Text_SetText(data->textbox, text);

	if (data->editsizefunc)
	{
		data->editsizefunc(pbox, data->textbox->h + 4);
	}

	Scrollable_Refresh(data->scrollable);

	Edit2Box_ScrollCursorVisible(pbox);

	Box_Repaint(pbox);
}

void Edit2Box_SetDisabled(struct Box_s *pbox, int disabled)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	data->disabled = disabled;

	if (disabled)
	{
		pbox->bgcol = RGB(128, 128, 128);
	}
	else
	{
		pbox->bgcol = RGB(255, 255, 255);
	}

	Box_Repaint(pbox);
}

int Edit2Box_GetDisabled(struct Box_s *pbox)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	return data->disabled;
}

void Edit2Box_SetEditSizeFunc(struct Box_s *pbox, void (*editsizefunc)(struct Box_s *pbox, int edith))
{
	struct edit2boxdata_s *data = pbox->boxdata;

	data->editsizefunc = editsizefunc;
}

void Edit2Box_SetTextCol(struct Box_s *pbox, int col)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	data->textbox->fgcol = col;
}

void Edit2Box_SetAltWText(struct Box_s *pbox, WCHAR *wtext)
{
	struct edit2boxdata_s *data = pbox->boxdata;

	free(data->alttext);
	data->alttext = wcsdup(wtext);

	if (!Box_HasFocus(pbox))
	{
		Text_SetAltWText(data->textbox, wtext);
	}
}

void Edit2Box_SetAltText(struct Box_s *pbox, char *text)
{
	WCHAR *wtext = Util_ConvertUTF8ToWCHAR(text);
	Edit2Box_SetAltWText(pbox, wtext);
}