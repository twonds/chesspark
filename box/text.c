#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <wingdi.h>

#include "box.h"

#include "list.h"
#include "log.h"
#include "mem.h"
#include "text.h"
#include "tooltip.h"
#include "util.h"
#include "namedlist.h"

extern HFONT tahoma11b_f, tahoma11_f, tahoma10b_f, tahoma10_f, tahoma10i_f, tahoma11ub_f, tahoma10ub_f, tahoma10u_f, tahoma11u_f, tahoma11i_f;

/* FIXME: rearrange code so I don't have to declare these as extern */
void Menu_PopupEditMenu(struct Box_s *pbox, int x, int y, int canedit, int cancopy, int canselect);

void Text_UpdateCursor(struct Box_s *pbox);
int Text_GetCharPosAtXYPos(struct Box_s *pbox, int x, int y);
void Text_OnPaint(struct Box_s *pbox, HDC hdc, RECT rcClip, int x, int y, int paintmask);
void Text_LinkedDeselect(struct Box_s *pbox, int fromnext);

#define TABPIXELWIDTH 40

struct textchar_s
{
	struct textchar_s *next;
	WCHAR c;
	unsigned short x;
	unsigned short y;
	unsigned short w;
	unsigned short h;
	COLORREF fgcol;
	COLORREF bgcol;
	HFONT font;
	unsigned short flags;
};

#define CHARBOXFLAG_HIGHLIGHT    0x01
#define CHARBOXFLAG_HIDDEN       0x02
#define CHARBOXFLAG_CLEARCTRLS   0x04
#define CHARBOXFLAG_CENTER       0x08
#define CHARBOXFLAG_ONEHALFSPACE 0x10

struct textdata_s
{
	WCHAR *text;
	WCHAR *alttext;
	enum Text_flags flags;
	int cursorpos;
	int selectpos;
	int showcursor;
	int mouseselect;
	COLORREF palette[10];
	int cursorx;
	int cursory;
	int cursorh;
	int cursorflash;
	struct Box_s *ellipsisbox;
	struct Box_s *redirectfocusonkey;
	struct namedlist_s *linkposlist;
	struct namedlist_s *linkcallbacks;
	struct namedlist_s *rlinkcallbacks;
	struct namedlist_s *linkuserdatas;
	struct namedlist_s *rlinkuserdatas;
	struct namedlist_s *linktips;
	int activelink;
	int clickedlink;
	int delayedarrange;
	struct Box_s *linkedselectprev;
	struct Box_s *linkedselectnext;
	struct textchar_s *chars;
	struct textchar_s *lastchar;
	struct Box_s *parentlist;
};

void Text_OnCopy(struct Box_s *pbox);

void Text_FlashCursor(struct Box_s *pbox, void *userdata)
{
	struct textdata_s *data = pbox->boxdata;

	if (data->showcursor)
	{
		data->cursorflash = !data->cursorflash;

		Box_Repaint(pbox);
	}
}

void Text_Destroy(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;
#if 0
	Mem_RemoveMemRec(pbox);
#endif
	ToolTipParent_OnDestroy(pbox);
	Box_RemoveTimedFunc(pbox, Text_FlashCursor, 500);
	Box_UnlockMouseCursorImg(pbox);
	free(data->text);
	while (data->chars)
	{
		struct textchar_s *old = data->chars;
		data->chars = data->chars->next;
		free(old);
	}

	if (data->parentlist)
	{
		List_RemoveLastText(data->parentlist, pbox, data->linkedselectprev);
	}

	if (data->linkedselectprev)
	{
		struct textdata_s *pdata =  data->linkedselectprev->boxdata;
		pdata->linkedselectnext = data->linkedselectnext;
	}

	if (data->linkedselectnext)
	{
		struct textdata_s *ndata =  data->linkedselectnext->boxdata;
		ndata->linkedselectprev = data->linkedselectprev;
	}
}

void Text_OnKeyDown(struct Box_s *pbox, int vk, int scan)
{
	struct textdata_s *data = pbox->boxdata;
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


	if (data->flags & TX_SELECTABLE)
	{
		if ((vk == 88 && (keystate[VK_CONTROL] & 0x8000)) || (vk == 46 && (keystate[VK_SHIFT] & 0x8000))) /* ctrl+x, shift+del */
		{
			Text_OnCopy(pbox);
			return;
		}
		else if ((vk == 67 && (keystate[VK_CONTROL] & 0x8000))) /* ctrl+c */
		{
			Text_OnCopy(pbox);
			return;
		}
		else if ((vk == 65 && (keystate[VK_CONTROL] & 0x8000))) /* ctrl+a */
		{
			struct Box_s *start = pbox;
			struct textdata_s *data2 = start->boxdata;

			while (data2->linkedselectprev)
			{
				start = data2->linkedselectprev;
				data2 = start->boxdata;
			}

			while (start)
			{
				data2 = start->boxdata;
				data2->cursorpos = (int)(wcslen(data2->text));
				data2->selectpos = 0;
				Text_UpdateCursor(start);
				start = data2->linkedselectnext;
			}

			return;
		}
	}

	if (data->redirectfocusonkey && unicode > 31)
	{
		Box_SetFocus(data->redirectfocusonkey);
		data->redirectfocusonkey->OnKeyDown(data->redirectfocusonkey, vk, scan);
	}
}

void Text_SetRedirectFocusOnKey(struct Box_s *pbox, struct Box_s *focus)
{
	struct textdata_s *data = pbox->boxdata;

	data->redirectfocusonkey = focus;
}

void Text_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct textdata_s *data = pbox->boxdata;

	Box_OnLButtonDown(pbox, xmouse, ymouse);

	if (data->flags & TX_FOCUS)
	{
		Box_SetFocus(pbox);
	}

	{
		int charpos = Text_GetCharPosAtXYPos(pbox, xmouse, ymouse);
		struct namedlist_s *entry = data->linkposlist;
		int overlink = 0, linknum = 0;

		while (entry && entry->next)
		{
			int start = (int)(entry->data);
			int end = (int)(entry->next->data);
			linknum++;

			if (charpos >= start && charpos < end)
			{
				overlink = linknum;
			}

			entry = entry->next->next;
		}

		if (overlink && xmouse >= 0 && ymouse >= 0 && xmouse < pbox->w && ymouse < pbox->h)
		{
			data->clickedlink = overlink;
		}
		else
		{
			data->clickedlink = -1;
		}
	}
		
	if (data->mouseselect)
	{
		data->mouseselect = 0;
	}
	
	if (/*Box_CheckXYInBox(pbox, xmouse, ymouse) &&*/ (data->flags & TX_SELECTABLE))
	{
		Text_SetCursorToXYPos(pbox, xmouse, ymouse);
		data->selectpos = -1;
		Text_UpdateCursor(pbox);
		data->mouseselect = 1;
		Log_Write(0, "%d data->mouseselect 1\n", pbox);
		if (data->linkedselectnext)
		{
			Text_LinkedDeselect(data->linkedselectnext, 0);
		}
		if (data->linkedselectprev)
		{
			Text_LinkedDeselect(data->linkedselectprev, 1);
		}
	}
}

void Text_OnLButtonDblClk(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct textdata_s *data = pbox->boxdata;

	Box_OnLButtonDblClk(pbox, xmouse, ymouse);

	if (!(data->flags & TX_SELECTABLE))
	{
		return;
	}

	if (!data->text)
	{
		return;
	}

	Text_SetCursorToXYPos(pbox, xmouse, ymouse);

	data->selectpos = data->cursorpos;

	while (data->cursorpos != 0 && !wcschr(L" .,()[]\n", data->text[data->cursorpos]))
	{
		data->cursorpos--;
	}

	if (data->cursorpos != wcslen(data->text) && wcschr(L" .,()[]\n", data->text[data->cursorpos]))
	{
		data->cursorpos++;
	}

	while (data->selectpos != wcslen(data->text) && !wcschr(L" .,()[]\n", data->text[data->selectpos]))
	{
		data->selectpos++;
	}

	if (data->selectpos != 0 && wcschr(L".,()[]\n", data->text[data->cursorpos]))
	{
		data->selectpos--;
	}

	Text_UpdateCursor(pbox);
}

void Text_SelectAll(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;

	if (!(data->flags & TX_SELECTABLE))
	{
		return;
	}

	if (!data->text)
	{
		return;
	}

	data->cursorpos = 0;
	data->selectpos = (int)(wcslen(data->text));
	
	Text_UpdateCursor(pbox);
}

void Text_OnLButtonTrpClk(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct textdata_s *data = pbox->boxdata;

	Box_OnLButtonTrpClk(pbox, xmouse, ymouse);

	Text_SelectAll(pbox);
}

void Text_RefreshActiveLink(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;
	struct namedlist_s *entry = data->linkposlist;
	struct namedlist_s *tipentry = data->linktips;
	struct textchar_s *letter = data->chars;
	int currentlink = 0, currentboxnum = 0;

	while (entry && entry->next)
	{
		int start = (int)(entry->data);
		int end = (int)(entry->next->data);

		currentlink++;

		while (currentboxnum != start)
		{
			currentboxnum++;
			letter = letter->next;
		}

		if (currentlink == data->activelink)
		{
			char *urltext;
			int urlpos = 0, showtip = 0;

			/* FIXME: convert characters to UTF8 in case someone uses an URL with extended characters in it */
			urltext = malloc(end - start + 1);

                        while (currentboxnum != end)
			{
				if (letter->font == tahoma10b_f)
				{
					letter->font = tahoma10ub_f;
				}
				else if (letter->font == tahoma11_f)
				{
					letter->font = tahoma11u_f;
				}
				else if (letter->font == tahoma11b_f)
				{
					letter->font = tahoma11ub_f;
				}
				urltext[urlpos++] = (char)letter->c;
				if (letter->c == '|')
				{
					showtip = 1;
				}

				currentboxnum++;
				letter = letter->next;
			}

			urltext[urlpos] = '\0';

			if (tipentry->data)
			{
				showtip = 1;
			}

			if (showtip && !pbox->tooltipvisible /*&& Box_GetRoot(pbox)->active*/)
			{
				if (tipentry->data)
				{
					Box_AddTimedFunc(pbox, ToolTip_Popup, strdup(tipentry->data), 1000);
				}
				else
				{
					Box_AddTimedFunc(pbox, ToolTip_Popup, urltext, 1000);
				}
				pbox->tooltipvisible = 1;
			}

		}
		else
		{
                        while (currentboxnum != end)
			{
				if (letter->font == tahoma10ub_f)
				{
					letter->font = tahoma10b_f;
				}
				else if (letter->font == tahoma11u_f)
				{
					letter->font = tahoma11_f;
				}
				else if (letter->font == tahoma11ub_f)
				{
					letter->font = tahoma11b_f;
				}
				currentboxnum++;
				letter = letter->next;
			}
		}

		entry = entry->next->next;
		tipentry = tipentry->next;
	}
	Box_Repaint(pbox);
}

void Text_LinkedSelect(struct Box_s *pbox, int fromnext, int xmouse, int ymouse)
{
	struct textdata_s *data = pbox->boxdata;
	int x, y;

	Box_GetScreenCoords(pbox, &x, &y);

	xmouse -= x;
	ymouse -= y;

	if (fromnext)
	{
		if (data->linkedselectprev && ymouse < 0)
		{
			int x, y;
			Box_GetScreenCoords(pbox, &x, &y);
			x += xmouse;
			y += ymouse;
			Text_LinkedSelect(data->linkedselectprev, 1, x, y);
			data->cursorpos = 0;
			data->selectpos = (int)(wcslen(data->text));
		}
	}
	else
	{
		if (data->linkedselectnext && ymouse > pbox->h)
		{
			int x, y;
			Box_GetScreenCoords(pbox, &x, &y);
			x += xmouse;
			y += ymouse;
			Text_LinkedSelect(data->linkedselectnext, 0, x, y);
			data->cursorpos = (int)(wcslen(data->text));
			data->selectpos = 0;
		}
	}
	
	if (ymouse >= 0 && ymouse < pbox->h)
	{
		if (fromnext)
		{
			data->selectpos = (int)(wcslen(data->text));
		}
		else
		{
			data->selectpos = 0;
		}
		Text_SetCursorToXYPos(pbox, xmouse, ymouse);
	}

	Text_UpdateCursor(pbox);
}

void Text_LinkedDeselect(struct Box_s *pbox, int fromnext)
{
	struct textdata_s *data = pbox->boxdata;

	if (fromnext)
	{
		if (data->linkedselectprev)
		{
			Text_LinkedDeselect(data->linkedselectprev, fromnext);
		}
	}
	else
	{
		if (data->linkedselectnext)
		{
			Text_LinkedDeselect(data->linkedselectnext, fromnext);
		}
	}
	data->selectpos = -1;
	Text_UpdateCursor(pbox);
}

void Text_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct textdata_s *data = pbox->boxdata;

	Box_OnMouseMove(pbox, xmouse, ymouse);

	if (Box_CheckXYInBox(pbox, xmouse, ymouse))
	{
		int charpos = Text_GetCharPosAtXYPos(pbox, xmouse, ymouse);
		struct namedlist_s *entry = data->linkposlist;
		int islink = 1, overlink = 0, currentlink = 0;

		while (entry && entry->next)
		{
			int start = (int)(entry->data);
			int end = (int)(entry->next->data);

			currentlink++;

			if (charpos >= start && charpos < end)
			{
				overlink = currentlink;
			}

			if (charpos == start && data->chars)
			{
				/* make sure we're actually over the first character */
				struct textchar_s *c = data->chars;
				int i = charpos;

				while (i && c)
				{
					c = c->next;
					i--;
				}

				if (c)
				{
					if (!(xmouse >= c->x && ymouse >= c->y && xmouse < c->x + c->w && ymouse < c->y + c->h))
					{
						overlink = 0;
					}
				}
			}
			else if (charpos == end - 1 && data->chars)
			{
				/* make sure we're actually over the last character */
				struct textchar_s *c = data->chars;
				int i = charpos;

				while (i && c)
				{
					c = c->next;
					i--;
				}

				if (c)
				{
					if (!(xmouse >= c->x && ymouse >= c->y && xmouse < c->x + c->w && ymouse < c->y + c->h))
					{
						overlink = 0;
					}
				}
			}

			entry = entry->next->next;
		}

		/*Box_UnlockMouseCursorImg(pbox);*/

		if (overlink)
		{
			Box_LockMouseCursorImg(pbox, LoadCursor(NULL, IDC_HAND));
		}
		else if (data->flags & TX_SELECTABLE)
		{
			Box_LockMouseCursorImg(pbox, LoadCursor(NULL, IDC_IBEAM));
		}
		else
		{
			Box_UnlockMouseCursorImg(pbox);
		}

		if (data->activelink != overlink)
		{
			data->activelink = overlink;

			Text_RefreshActiveLink(pbox);
		}
	}
	else
	{	
		if (data->activelink != 0)
		{
			data->activelink = 0;

			Text_RefreshActiveLink(pbox);
		}

		ToolTip_PopDown(pbox);
		pbox->tooltipvisible = 0;

		if (data->flags & TX_SELECTABLE /*&& data->flags & TX_PARENTHOVER && Box_CheckXYInBox(pbox->parent, xmouse + pbox->x - pbox->parent->xoffset, ymouse + pbox->y - pbox->parent->yoffset)*/)
		{
			Box_LockMouseCursorImg(pbox, LoadCursor(NULL, IDC_IBEAM));
		}
		/*else
		{
                        Box_UnlockMouseCursorImg(pbox);
		}*/
	}

	if (data->mouseselect)
	{
		if (data->mouseselect == 1)
		{
			data->mouseselect = 2;
			data->selectpos = data->cursorpos;
		}

		if (ymouse < 0)
		{
			data->cursorpos = 0;
		}

		if (data->linkedselectprev)
		{
			if (ymouse < 0)
			{
				int x, y;
				Box_GetScreenCoords(pbox, &x, &y);
				x += xmouse;
				y += ymouse;
				Text_LinkedSelect(data->linkedselectprev, 1, x, y);
			}
			else
			{
				Text_LinkedDeselect(data->linkedselectprev, 1);
			}
		}

		if (ymouse > pbox->h)
		{
			if (data->text)
			{
				data->cursorpos = (int)(wcslen(data->text));
			}
			else
			{
				data->cursorpos = 0;
			}
		}

		if (data->linkedselectnext)
		{
			if (ymouse > pbox->h)
			{
				int x, y;
				Box_GetScreenCoords(pbox, &x, &y);
				x += xmouse;
				y += ymouse;
				Text_LinkedSelect(data->linkedselectnext, 0, x, y);
			}
			else
			{
				Text_LinkedDeselect(data->linkedselectnext, 0);
			}
		}
		
		if (ymouse >= 0 && ymouse < pbox->h)
		{
			Text_SetCursorToXYPos(pbox, xmouse, ymouse);
		}

		Text_UpdateCursor(pbox);
	}
}

void Text_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct textdata_s *data = pbox->boxdata;

	if (data->mouseselect)
	{
		data->mouseselect = 0;
	}

	{
		int charpos = Text_GetCharPosAtXYPos(pbox, xmouse, ymouse);
		struct namedlist_s *entry = data->linkposlist;
		struct namedlist_s *callbackentry, *userdataentry;
		int overlink = 0, linknum = 0;
		void (*onclick)(struct Box_s *pbox, void *userdata);

		while (entry && entry->next)
		{
			int start = (int)(entry->data);
			int end = (int)(entry->next->data);
			linknum++;

			if (charpos >= start && charpos <= end)
			{
				overlink = linknum;
			}

			entry = entry->next->next;
		}

		if (overlink && overlink == data->clickedlink && xmouse >= 0 && ymouse >= 0 && xmouse < pbox->w && ymouse < pbox->h)
		{
			callbackentry = data->linkcallbacks;
			userdataentry = data->linkuserdatas;

			overlink--;

			while (overlink)
			{
				overlink--;
				callbackentry = callbackentry->next;
				userdataentry = userdataentry->next;
			}

			onclick = callbackentry->data;

			if (onclick)
			{
				onclick(pbox, userdataentry->data);
				return;
			}
		}
	}
}

void Text_OnRButtonDown_PopupMenu(struct Box_s *pbox, int xmouse, int ymouse)
{	
	struct textdata_s *data = pbox->boxdata;
	int sx, sy;

	Box_GetScreenCoords(pbox, &sx, &sy);
	sx += xmouse;
	sy += ymouse;

	{
		int charpos = Text_GetCharPosAtXYPos(pbox, xmouse, ymouse);
		struct namedlist_s *entry = data->linkposlist;
		struct namedlist_s *callbackentry, *userdataentry;
		int overlink = 0, linknum = 0;
		void (*onclick)(struct Box_s *pbox, void *userdata, int x, int y);

		while (entry && entry->next)
		{
			int start = (int)(entry->data);
			int end = (int)(entry->next->data);
			linknum++;

			if (charpos >= start && charpos <= end)
			{
				overlink = linknum;
			}

			entry = entry->next->next;
		}

		if (overlink && xmouse >= 0 && ymouse >= 0 && xmouse < pbox->w && ymouse < pbox->h)
		{
			callbackentry = data->rlinkcallbacks;
			userdataentry =  data->rlinkuserdatas;

			overlink--;

			while (overlink)
			{
				overlink--;
				callbackentry = callbackentry->next;
				userdataentry = userdataentry->next;
			}

			onclick = callbackentry->data;

			if (onclick)
			{
				onclick(pbox, userdataentry->data, sx, sy);
				return;
			}
		}
	}

	if (data->flags & TX_COPYMENU)
	{
		Menu_PopupEditMenu(pbox, sx, sy, 0, data->selectpos != -1, data->flags && TX_SELECTABLE);
	}
}

void Text_OnLoseFocus(struct Box_s *text)
{
	struct textdata_s *data = text->boxdata;
	struct Box_s *next;
	Text_UpdateCursor(text);

	next = data->linkedselectnext;
	while (next)
	{
		struct textdata_s *data2 = next->boxdata;
		Text_UpdateCursor(next);
		next = data2->linkedselectnext;
	}

	next = data->linkedselectprev;
	while (next)
	{
		struct textdata_s *data2 = next->boxdata;
		Text_UpdateCursor(next);
		next = data2->linkedselectprev;
	}
}


struct Box_s *Text_CreateReal(int x, int y, int w, int h, enum Box_flags flags, enum Text_flags tflags)
{
	struct Box_s *pbox = Box_Create(x, y, w, h, flags);
	struct textdata_s *data = malloc(sizeof(*data));

	memset(data, 0, sizeof(*data));

	data->text      = NULL;
	data->flags     = tflags;
	data->selectpos = -1;
	/*data->linkcol   = RGB(163, 97, 7);*/

	/* set default palette colors */
	data->palette[0] = RGB(0, 0, 0);
	data->palette[1] = RGB(255, 0, 0);
	data->palette[2] = RGB(66, 88, 158);
	data->palette[3] = RGB(255, 255, 255);
	data->palette[4] = RGB(102, 102, 102);
	data->palette[5] = RGB(116, 73, 58);
	data->palette[6] = RGB(163, 97, 7);
	data->palette[7] = RGB(125, 36, 134);
	data->palette[8] = RGB(54, 102, 35);
	data->palette[9] = RGB(128, 128, 128);

	pbox->OnDestroy = Text_Destroy;
	pbox->OnLButtonDown = Text_OnLButtonDown;
	pbox->OnLButtonDblClk = Text_OnLButtonDblClk;
	pbox->OnLButtonTrpClk = Text_OnLButtonTrpClk;
	pbox->OnMouseMove = Text_OnMouseMove;
	pbox->OnLButtonUp = Text_OnLButtonUp;
	pbox->OnKeyDown = Text_OnKeyDown;
	pbox->OnPaint = Text_OnPaint;
	/*if (tflags & TX_COPYMENU)*/
	{
		pbox->OnRButtonDown = Text_OnRButtonDown_PopupMenu;
	}
	if (tflags & TX_FOCUS)
	{
		pbox->OnLoseFocus = Text_OnLoseFocus;
	}
	pbox->OnCopy = Text_OnCopy;
	pbox->boxdata = data;

	return pbox;
}

struct Box_s *Text_CreateWrap(int x, int y, int w, int h, enum Box_flags flags, enum Text_flags tflags, const char *file, unsigned int line)
{
	struct Box_s *alloc = Text_CreateReal(x, y, w, h, flags, tflags);
	Mem_AddMemRec(alloc, 0, "textboxtype", file, line);

	return alloc;
}

int TextChar_GetWidth(struct textchar_s *tc, int x)
{
	int w;

	if (tc->c == '\t')
	{
		w = TABPIXELWIDTH - x % TABPIXELWIDTH;
		tc->w = w;
	}
	else if (tc->font == tahoma10i_f || tc->font == tahoma11i_f)
	{
		w = tc->w - 5;
	}
	else
	{
		w = tc->w - 1;
	}

	return w;
}


void Text_UpdateCursor(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;
	struct textchar_s *current = data->chars;
	int x, y, h, letternum;

	if (data->flags & TX_FOCUS)
	{
		int focus = Box_HasFocus(pbox);

		if (!focus)
		{
			struct Box_s *next;

			next = data->linkedselectnext;
			while (next && !focus)
			{
				struct textdata_s *data2 = next->boxdata;
				if (Box_HasFocus(next))
				{
					focus = 1;
				}
				next = data2->linkedselectnext;
			}

			next = data->linkedselectprev;
			while (next && !focus)
			{
				struct textdata_s *data2 = next->boxdata;
				if (Box_HasFocus(next))
				{
					focus = 1;
				}
				next = data2->linkedselectprev;
			}
		}

		if (!focus)
		{
			while (current)
			{
				current->flags &= ~CHARBOXFLAG_HIGHLIGHT;
				current = current->next;
			}

			data->selectpos = -1;

			Box_Repaint(pbox);
			return;
		}
	}

	letternum = 0;
	x = 0;
	y = 0;
	h = 14;

	if (data->selectpos != -1 && data->selectpos < data->cursorpos)
	{
		while (letternum != data->selectpos && current)
		{
			current->flags &= ~CHARBOXFLAG_HIGHLIGHT;
			current = current->next;
			letternum++;
		}
		while (letternum != data->cursorpos && current)
		{
			x = current->x + TextChar_GetWidth(current, current->x);
			y = current->y;
			h = current->h;

			current->flags |= CHARBOXFLAG_HIGHLIGHT;

			current = current->next;
			letternum++;
		}
	}
	else
	{
		while (letternum != data->cursorpos && current)
		{
			x = current->x + TextChar_GetWidth(current, current->x);
			y = current->y;
			h = current->h;

			current->flags &= ~CHARBOXFLAG_HIGHLIGHT;
			
			current = current->next;
			letternum++;
		}
	}

	if (data->selectpos != -1 && data->selectpos > data->cursorpos)
	{
		while (letternum != data->selectpos && current)
		{
			current->flags |= CHARBOXFLAG_HIGHLIGHT;

			current = current->next;
			letternum++;
		}
	}

	while (current)
	{
		current->flags &= ~CHARBOXFLAG_HIGHLIGHT;
		current = current->next;
	}

	data->cursorx = x;
	data->cursory = y;
	data->cursorh = h;
		
	Box_Repaint(pbox);
}


void Text_ArrangeBoxes(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;
	
	struct textchar_s *subchar;
	int x, y, h, maxx;

	data->delayedarrange = 0;

	subchar = data->chars;
	maxx = 0;
	x = 0;
	y = 0;
	h = 14;

	/* find an appropriate height */
	/*
	while (subbox)
	{
		if (h < subbox->h - 1 && subbox->text && subbox->text[0] != '\n')
		{
			h = subbox->h;
		}
		subbox = subbox->sibling;
	}
*/
	subchar = data->chars;

	while (subchar)
	{
		struct textchar_s *end;
		struct textchar_s *next;
		struct textchar_s *current;
		int startx = x, w;
		int wordbreak = FALSE;
		int newlines = 0;
		int onehalfspace = 0;

		/* find end of word */
		h = 0;
		end = subchar;
		while(end && end->c != ' ' && end->c != '\n')
		{
			if (end->h > h)
			{
				h = end->h;
			}
			end = end->next;
		}
		
		if (end)
		{
			if (end->h > h)
			{
                                h = end->h;
			}
			if (end->c == '\n')
			{
				newlines++;
			}
		}

		/* find next word */
		if (end)
		{
			next = end->next;
		}
		else
		{
			next = NULL;
		}
/*
		if (end && end->c != '\n')
		{
			while(next && (next->c == ' '))
			{
				next = next->next;
			}
		}
*/
		/* calculate word width */
		w = 0;
		current = subchar;
		while (current != end)
		{
			w += TextChar_GetWidth(current, w);

			if (current->flags & CHARBOXFLAG_ONEHALFSPACE)
			{
				onehalfspace = 1;
			}
			current = current->next;
		}

		/* if we overflow the line, and text wrap is on, go to the next one */
		if ((x + w > pbox->w) && (x > 2 * pbox->w / 5 || pbox->w < 100) && (data->flags & TX_WRAP))
		{
			x = 0;
			y += h - 1 + (onehalfspace ? ((h - 1) / 2) : 0);
		}

		/* if we still overflow the line, find a place to cut the word */
		if (x + w > pbox->w && (data->flags & TX_WRAP))
		{
			w = 0;
			current = subchar;
			while (current)
			{
				int neww;

				neww = w + TextChar_GetWidth(current, w);

				if (x + neww > pbox->w)
				{
					break;
				}

				w = neww;

				if (current->flags & CHARBOXFLAG_ONEHALFSPACE)
				{
					onehalfspace = 1;
				}

				current = current->next;
			}

			if (current != subchar)
			{
				end = current;
				next = end;
			}
			else
			{
				end = current->next;
				next = end;
			}
		}

		/* position the boxes */
		current = subchar;

		while (current != next)
		{
			if (current->c != '\n'/* && current->text[0] != '\f'*/)
			{
				int w = current->w;

				w = TextChar_GetWidth(current, x);

				current->x = x;
				current->y = y;
				x += w;
			}
			else
			{
				if (current->flags & CHARBOXFLAG_ONEHALFSPACE)
				{
					onehalfspace = 1;
				}
				current->x = 0;
				current->y = y + h - 1 + (onehalfspace ? ((h - 1) / 2) : 0);
			}
			current = current->next;
		}

		if (x > maxx)
		{
			maxx = x;
		}

		while (newlines)
		{
			x = 0;
			y += h - 1 + (onehalfspace ? ((h - 1) / 2) : 0);
			newlines--;
		}

		subchar = current;
	}

	y += h;

	if (data->flags & TX_STRETCHHORIZ && pbox->w < maxx + 1)
	{
		pbox->w = maxx + 1;
	}

	if (data->flags & TX_STRETCHPARENT)
	{
		if (pbox->parent) {
			pbox->parent->h = pbox->parent->h - pbox->h + y;
		}
	}

	if ((data->flags & TX_STRETCHPARENT) || (data->flags & TX_STRETCHVERT))
	{
		pbox->h = y;
	}

	/* If there's an ellipsis, place it at the end */
	if (data->flags & TX_ELLIPSIS)
	{
		int x = 0;
		subchar = data->chars;

		while (subchar && subchar->x < pbox->w - 15)
		{
			subchar->flags &= ~CHARBOXFLAG_HIDDEN;
			subchar = subchar->next;
		}

		if (subchar)
		{
			if (!data->ellipsisbox)
			{
				data->ellipsisbox = Box_Create(0, 0, 10, 20, BOX_VISIBLE | BOX_TRANSPARENT);
				Box_AddChild(pbox, data->ellipsisbox);
				Box_SetText(data->ellipsisbox, "...");
			}
			data->ellipsisbox->fgcol = subchar->fgcol;
			data->ellipsisbox->font = subchar->font;
			data->ellipsisbox->flags |= BOX_VISIBLE;
			data->ellipsisbox->x = subchar->x;
			data->ellipsisbox->y = subchar->y;

			while (subchar)
			{
				subchar->flags |= CHARBOXFLAG_HIDDEN;
				subchar = subchar->next;
			}
		}
		else
		{
			if (data->ellipsisbox)
			{
				data->ellipsisbox->flags &= ~BOX_VISIBLE;
			}
		}
	}

	/* If we're centered, center each line */
	/*if (data->flags & TX_CENTERED)*/
	{
		int center = 0;
		subchar = data->chars;
		
		while (subchar)
		{
			struct textchar_s *next;
			struct textchar_s *current;

			while(subchar && !(data->flags & TX_CENTERED) && !(subchar->flags & CHARBOXFLAG_CENTER))
			{
				subchar = subchar->next;
			}

			next = subchar;
			x = 0;

			while (next && next->y == subchar->y)
			{
				x += TextChar_GetWidth(next, x);

				next = next->next;
			}

			x = (pbox->w - x) / 2;

			if (x < 0)
			{
				x = 0;
			}

			current = subchar;
			
			while (current != next)
			{
				current->x = x;

				x += TextChar_GetWidth(current, x);

				current = current->next;
			}

			subchar = current;
		}
	}

	/* If we're right justified, right justify each line */
	if (data->flags & TX_RIGHT)
	{
		subchar = data->chars;
		
		while (subchar)
		{
			struct textchar_s *next;
			struct textchar_s *current;

			next = subchar;
			x = 0;

			while (next && next->y == subchar->y)
			{
				x += TextChar_GetWidth(next, x);

				next = next->next;
			}

			x = pbox->w - x;

			current = subchar;
			
			while (current != next)
			{
				current->x = x;

				x += TextChar_GetWidth(current, x);

				current = current->next;
			}

			subchar = current;
		}
	}

	/* If we're vertically centered....yeah. :) */
	if (data->flags & TX_CENTERVERT)
	{
		int diff = (pbox->h - y) / 2;
		subchar = data->chars;
		
		while (subchar)
		{
			subchar->y += diff;
			subchar = subchar->next;
		}
	}

	/* find and place the cursor */
	Text_UpdateCursor(pbox);
}

extern RECT cliprects(RECT rc1, RECT rc2);

void TextChar_OnPaint(struct textchar_s *pchar, HDC hdc, RECT rcClip, int x, int y, int paintmask)
{
	RECT rcBox, rc;

	if (pchar->flags & CHARBOXFLAG_HIDDEN)
	{
		return;
	}

	/* Set up the box rect */
	rcBox.left   = x;
	rcBox.top    = y;
	rcBox.right  = rcBox.left + pchar->w - 1;
	rcBox.bottom = rcBox.top  + pchar->h - 1;

	/* Clip the box rect against the draw rect */
	rc = cliprects(rcBox, rcClip);
	
	/* If nothing to draw, punt */
	if ((rc.right < rc.left) || (rc.bottom < rc.top))
	{
		return;
	}

	if (paintmask)
	{
		BitBlt(hdc, rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1, NULL, 0, 0, WHITENESS);
	}

	if (pchar->flags & CHARBOXFLAG_HIGHLIGHT)
	{
		if (!paintmask)
		{
			HBRUSH bgbrush, oldbrush;
			HPEN bgpen, oldpen;

			bgbrush = CreateSolidBrush(pchar->bgcol);
			bgpen   = CreatePen(PS_SOLID, 0, pchar->bgcol);

			oldbrush = SelectObject(hdc, bgbrush);
			oldpen   = SelectObject(hdc, bgpen);

			Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);

			SelectObject(hdc, oldbrush);
			SelectObject(hdc, oldpen);

			DeleteObject(bgbrush);
			DeleteObject(bgpen);
		}
		else
		{
			BitBlt(hdc, rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1, NULL, 0, 0, BLACKNESS);
		}
	}

	{
		COLORREF oldfgcol;
		int oldbkmode;
		HFONT oldfont;
		RECT draw = rcBox;

		oldbkmode = SetBkMode(hdc, TRANSPARENT);
		if (!paintmask)
		{
			oldfgcol = SetTextColor(hdc, pchar->fgcol);
		}
		else
		{
			oldfgcol = SetTextColor(hdc, RGB(0, 0, 0));
		}
		
		if (pchar->font)
		{
			oldfont = SelectObject(hdc, pchar->font);
		}
		else
		{
			oldfont = SelectObject(hdc, tahoma11_f);
		}

		
		{
			RECT rc2;

			rc2.top = rc.top;
			rc2.left = rc.left;
			rc2.right = rc.right + 1;
			rc2.bottom = rc.bottom + 1;

			ExtTextOutW(hdc, draw.left, draw.top, ETO_CLIPPED, &rc2, &(pchar->c), 1, NULL);
		}

		SelectObject(hdc, oldfont);
	
		SetBkMode(hdc, oldbkmode);
		SetTextColor(hdc, oldfgcol);
	}
}

void Text_OnPaint(struct Box_s *pbox, HDC hdc, RECT rcClip, int x, int y, int paintmask)
{
	struct textdata_s *data = pbox->boxdata;
	struct textchar_s *current;
	RECT rcBox, rc;

	if (data->delayedarrange)
	{
		Text_ArrangeBoxes(pbox);
	}

	Box_OnPaint(pbox, hdc, rcClip, x, y, paintmask);

	/* If box is not visible, punt */
	if (!(pbox->flags & BOX_VISIBLE))
	{
		return;
	}

	/* Set up the box rect */
	rcBox.left   = x;
	rcBox.top    = y;
	rcBox.right  = rcBox.left + pbox->w - 1;
	rcBox.bottom = rcBox.top  + pbox->h - 1;

	/* Clip the box rect against the draw rect */
	rc = cliprects(rcBox, rcClip);
	
	/* If nothing to draw, punt */
	if ((rc.right < rc.left) || (rc.bottom < rc.top))
	{
		return;
	}

	if (data->showcursor && data->cursorflash && Box_GetRoot(pbox)->active)
	{
		MoveToEx(hdc, rcBox.left + data->cursorx, rc.top + data->cursory, (LPPOINT)NULL); 
		LineTo(hdc, rcBox.left + data->cursorx, rc.top + data->cursory + data->cursorh + 1);
	}

	current = data->chars;

	while (current)
	{
		TextChar_OnPaint(current, hdc, rc, current->x + rcBox.left - pbox->xoffset, current->y + rcBox.top - pbox->yoffset, paintmask);

		current = current->next;
	}
}

void Text_RedoTextBoxes(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;
	/*char *start, *end, *tag;*/
	WCHAR *p;
	BOOL finished = FALSE;
	int islink = FALSE;
	COLORREF currcol = pbox->fgcol;
	COLORREF lastcol;
	HFONT currfont = pbox->font;
	int onehalfspace = 0;
	int boxnum = 0;
	int hiddenlink = 0;
	int thicklink = 0;
	int center = 0;

	while(data->chars)
	{
		struct textchar_s *old = data->chars;
		data->chars = data->chars->next;
		free(old);
	}
	data->lastchar = NULL;


	NamedList_Destroy(&data->linkposlist);
	NamedList_Destroy(&data->linkcallbacks);
	NamedList_Destroy(&data->rlinkcallbacks);
	NamedList_Destroy(&data->linkuserdatas);
	NamedList_Destroy(&data->rlinkuserdatas);
	NamedList_Destroy(&data->linktips);

	p = data->text;

	if (!p || wcslen(p) == 0)
	{
		if (data->alttext)
		{
			p = data->alttext;
			currcol = data->palette[9];
		}
		else
		{
			/* arrange boxes so the cursor is shown */
			Text_ArrangeBoxes(pbox);
			return;
		}
	}

	while (*p)
	{
		int skip = 0;

		if (*p == L'^' && !(data->flags & TX_NOCOMMANDS) && !(data->flags & TX_PASSWORD))
		{
			p++;
			skip = 1;

			switch (*p)
			{
				case L'0':
				case L'1':
				case L'2':
				case L'3':
				case L'4':
				case L'5':
				case L'6':
				case L'7':
				case L'8':
				case L'9':
					currcol = data->palette[(*p)-L'0'];
					p++;
					break;
				case L'b':
					if (currfont == tahoma11b_f)
					{
						currfont = tahoma11_f;
					}
					else if (currfont == tahoma10b_f)
					{
						currfont = tahoma10_f;
					}
					else if (!pbox->font || pbox->font == tahoma11_f)
					{
						currfont = tahoma11b_f;
					}
					else if (pbox->font == tahoma10_f)
					{
						currfont = tahoma10b_f;
					}
					p++;
					break;
				case L'i':
					if (!pbox->font || pbox->font == tahoma11_f)
					{
						currfont = tahoma11i_f;
					}
					p++;
					break;				case L'l':
				case L'L':
					islink = !islink;
					if (islink)
					{
						WCHAR *pipe = wcsstr(p+1, L"|");
						WCHAR *endlink = wcsstr(p+1, L"^l");
						WCHAR *endlink2 = wcsstr(p+1, L"^L");
						WCHAR *doubleslash = wcsstr(p+1, L"//");
						NamedList_Add(&data->linkcallbacks, NULL, NULL, NULL);
						NamedList_Add(&data->rlinkcallbacks, NULL, NULL, NULL);
						NamedList_Add(&data->linkuserdatas, NULL, NULL, NULL);
						NamedList_Add(&data->rlinkuserdatas, NULL, NULL, NULL);
						NamedList_Add(&data->linktips, NULL, NULL, NULL);
						lastcol = currcol;
						currcol = data->palette[6];
						
						if (endlink2 && (!endlink || endlink2 < endlink))
						{
							endlink = endlink2;
						}

						if (pipe)
						{
							WCHAR *hidden = pipe + 1;

							if (*hidden = L'"')
							{
								hidden++;
							}

							if (wcsncmp(hidden, L"http://", 7) != 0
								&& wcsncmp(hidden, L"ftp://", 6) != 0
								&& wcsncmp(hidden, L"https://", 8) != 0
								&& wcsncmp(hidden, L"http://", 7) != 0
								&& wcsncmp(hidden, L"http://", 7) != 0
								&& wcsncmp(hidden, L"game:", 5) != 0
								&& wcsncmp(hidden, L"room:", 5) != 0 
								&& wcsncmp(hidden, L"contact:", 8) != 0
								&& wcsncmp(hidden, L"match:", 6) != 0
								&& wcsncmp(hidden, L"help:", 5) != 0
								&& wcsncmp(hidden, L"profile:", 8) != 0)
							{
								if ((pipe && endlink - 1 > pipe))
								/*&& !(doubleslash && doubleslash < pipe))*/
								{
									hiddenlink = 3;
								}
							}

						}

						if (*p == 'l')
						{
							thicklink = 1;
						}
						else
						{
							thicklink = 0;
						}
					}
					else
					{
						hiddenlink = 0;
						currcol = lastcol;
					}
					NamedList_Add(&data->linkposlist, NULL, (void *)(boxnum), NULL);
					p++;
					break;
				case L'^':
					skip = 0;
					break;
				case L'\0':
					break;
				case L'C':
					center = !center;
					p++;
					break;
				case L'E':
					onehalfspace = !onehalfspace;
					p++;
					break;
				case L'n':
				default:
					currfont = pbox->font;
					currcol = pbox->fgcol;
					p++;
			}
		}

		if (!skip)
		{
			RECT measure;
			HDC hdc;
			HFONT oldfont;
			struct textchar_s *subchar;
			WCHAR *end = NULL;
			int length;
/*
			if (islink)
			{
				end = wcsstr(p, L"^l");
				if (!end)
				{
					end = wcschr(p, L'\0');
				}

				length = (int)(end - p);
			}
			else
*/			{
				length = 1;
			}

			measure.left = 0;
			measure.top = 0;
			measure.right = 0;
			measure.bottom = 0;

			hdc = GetDC(NULL);

			if (currfont && !islink)
			{
				oldfont = SelectObject(hdc, currfont);
			}
			else
			{
				if (islink && thicklink)
				{
					if (currfont == tahoma10_f)
					{
						oldfont = SelectObject(hdc, tahoma10b_f);
					}
					else
					{
						oldfont = SelectObject(hdc, tahoma11b_f);
					}
				}
				else
				{
					if (currfont)
					{
						oldfont = SelectObject(hdc, currfont);
					}
					else
					{
						oldfont = SelectObject(hdc, tahoma11_f);
					}
				}
			}

			if (*p == '\n' || *p == '\f' || *p == '\r')
			{
				DrawTextA(hdc, " ", 1, &measure, DT_CALCRECT | DT_NOPREFIX);
				measure.right = measure.left;

				/*if (entersize && *p == 13)*/
				/*
				{
					measure.bottom = (measure.bottom - measure.top) * (entersize + 2) / 2 + measure.top;
				}
				*/
			}
			else if (data->flags & TX_PASSWORD)
			{
				DrawTextA(hdc, "*", 1, &measure, DT_CALCRECT | DT_NOPREFIX);
			}
			else if (Util_OldWinVer())
			{
				WCHAR *converttext;
				char *temptext;

				converttext = malloc((length + 1) * sizeof(WCHAR));
				wcsncpy(converttext, p, length);
				converttext[length] = L'\0';
				temptext = Util_ConvertWCHARToANSI(converttext);

				DrawTextA(hdc, temptext, length, &measure, DT_CALCRECT | DT_NOPREFIX);

				free(converttext);
				free(temptext);
			}
			else
			{
				DrawTextW(hdc, p, length, &measure, DT_CALCRECT | DT_NOPREFIX);
			}

			SelectObject(hdc, oldfont);

			ReleaseDC(NULL, hdc);

			if (hiddenlink == 3 || (hiddenlink && *p == '"'))
			{
				measure.right = 0;
			}

			subchar = malloc(sizeof(*subchar));
			memset(subchar, 0, sizeof(*subchar));

			if (islink)
			{
				subchar->w = (unsigned short)(measure.right + 1);
				subchar->h = (unsigned short)(measure.bottom + 1);
				if (thicklink)
				{
					if (currfont == tahoma10_f)
					{
						subchar->font = tahoma10b_f;
					}
					else
					{
						subchar->font = tahoma11b_f;
					}
				}
				else
				{
					subchar->font = currfont;
				}
			}
			else
			{
				if (currfont == tahoma10i_f || currfont == tahoma11i_f)
				{
					subchar->w = (unsigned short)(measure.right + 5);
					subchar->h = (unsigned short)(measure.bottom + 1);
				}
				else
				{
					subchar->w = (unsigned short)(measure.right + 1);
					subchar->h = (unsigned short)(measure.bottom + 1);
				}
				subchar->font = currfont;
			}

			if (data->flags & TX_PASSWORD)
			{
				subchar->c = L'*';
			}
			/*
			else if (hiddenlink == 3 || (hiddenlink && *p == '"'))
			{
				subchar->c = L' ';
			}
			*/
			else
			{
				subchar->c = *p;
			}

			if (hiddenlink == 3 || (hiddenlink && *p == '"'))
			{
				subchar->flags |= CHARBOXFLAG_HIDDEN;
			}

			subchar->fgcol = currcol;
			subchar->bgcol = RGB(200, 200, 200);

			if (center)
			{
				subchar->flags |= CHARBOXFLAG_CENTER;
			}

			if (onehalfspace)
			{
				subchar->flags |= CHARBOXFLAG_ONEHALFSPACE;
			}

			if ((*p == '"') && hiddenlink)
			{
				hiddenlink--;
			}
			else if ((*p == '|') && hiddenlink == 3)
			{
				if (*(p+1) == '"')
				{
					hiddenlink = 2;
				}
				else
				{
					hiddenlink = 0;
				}
			}

			p += length;

			if (data->lastchar)
			{
				data->lastchar->next = subchar;
			}
			else
			{
				data->chars = subchar;
			}
			data->lastchar = subchar;

			boxnum++;
		}
	}

	Text_ArrangeBoxes(pbox);
}


void Text_SetText(struct Box_s *pbox, char *text)
{
	struct textdata_s *data = pbox->boxdata;
	int widesize;

	free(data->text);

	if (text)
	{
		widesize = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);

		data->text = malloc(widesize * sizeof(WCHAR));

		MultiByteToWideChar(CP_UTF8, 0, text, -1, data->text, widesize);
		data->cursorpos = (int)(wcslen(data->text));
		data->selectpos = -1;

	}
	else
	{
		data->text = wcsdup(L"");
		data->cursorpos = 0;
		data->selectpos = -1;
	}

	Text_RedoTextBoxes(pbox);
}

void Text_SetWText(struct Box_s *pbox, WCHAR *wtext)
{
	struct textdata_s *data = pbox->boxdata;

	free(data->text);
	if (wtext)
	{
		data->text = malloc((wcslen(wtext) + 1) * sizeof(WCHAR));
		wcscpy(data->text, wtext);
		data->cursorpos = (int)(wcslen(data->text));
		data->selectpos = -1;
	}
	else
	{
		data->text = wcsdup(L"");
		data->cursorpos = 0;
		data->selectpos = -1;
	}

	Text_RedoTextBoxes(pbox);
}

void Text_SetAltWText(struct Box_s *pbox, WCHAR *wtext)
{
	struct textdata_s *data = pbox->boxdata;

	free(data->alttext);
	if (wtext)
	{
		data->alttext = wcsdup(wtext);
	}
	else
	{
		data->alttext = NULL;
	}

	Text_RedoTextBoxes(pbox);
}

void Text_OnSizeWidth_Stretch(struct Box_s *pbox, int dwidth)
{
	struct textdata_s *data = pbox->boxdata;

	/*data->delayedarrange = 1;*/
	pbox->w += dwidth;
	Text_ArrangeBoxes(pbox);
	Box_Repaint(pbox);
}

void Text_SetLinkCallback(struct Box_s *pbox, int linknum, void (*OnClick)(struct Box_s *, void *), void *userdata)
{
	struct textdata_s *data = pbox->boxdata;
	struct namedlist_s *callbackentry = data->linkcallbacks;
	struct namedlist_s *userdataentry = data->linkuserdatas;
	int currlink = 0;

	while (callbackentry)
	{
		currlink++;

		if (currlink == linknum)
		{
			callbackentry->data = OnClick;
			userdataentry->data = userdata;

			return;
		}

		callbackentry = callbackentry->next;
		userdataentry = userdataentry->next; 
	}
}

void Text_SetRLinkCallback(struct Box_s *pbox, int linknum, void (*OnRClick)(struct Box_s *, void *, int x, int y), void *userdata)
{
	struct textdata_s *data = pbox->boxdata;
	struct namedlist_s *callbackentry =  data->rlinkcallbacks;
	struct namedlist_s *userdataentry =  data->rlinkuserdatas;
	int currlink = 0;

	while (callbackentry)
	{
		currlink++;

		if (currlink == linknum)
		{
			callbackentry->data  = OnRClick;
			userdataentry->data  = userdata;

			return;
		}

		callbackentry  = callbackentry->next;
		userdataentry  = userdataentry->next; 
	}
}

void Text_SetLinkTooltip(struct Box_s *pbox, int linknum, char *text)
{
	struct textdata_s *data = pbox->boxdata;
	struct namedlist_s *tipentry =  data->linktips;
	int currlink = 0;

	while (tipentry)
	{
		currlink++;

		if (currlink == linknum)
		{
			tipentry->data = strdup(text);
			return;
		}

		tipentry = tipentry->next;
	}
}

void Text_CursorLeft(struct Box_s *pbox, int shift, int ctrl)
{
	struct textdata_s *data = pbox->boxdata;

	if (!data->text)
	{
		return;
	}

	if (shift && data->selectpos == -1)
	{
		data->selectpos = data->cursorpos;
	}

	data->cursorpos--;

	if (data->cursorpos < 0)
	{
		data->cursorpos = 0;
	}

	if (ctrl)
	{
		while (data->cursorpos != 0 && data->text[data->cursorpos] == ' ')
		{
			data->cursorpos--;
		}
		while (data->cursorpos != 0 && data->text[data->cursorpos] != ' ')
		{
			data->cursorpos--;
		}
		if (data->text[data->cursorpos] == ' ')
		{
			data->cursorpos++;
		}
	}

	if (!shift)
	{
		data->selectpos = -1;
	}

	Text_UpdateCursor(pbox);
}

void Text_CursorRight(struct Box_s *pbox, int shift, int ctrl)
{
	struct textdata_s *data = pbox->boxdata;

	if (!data->text)
	{
		return;
	}

	if (shift && data->selectpos == -1)
	{
		data->selectpos = data->cursorpos;
	}

	if (ctrl)
	{
		while (data->cursorpos != wcslen(data->text) && data->text[data->cursorpos] != ' ')
		{
			data->cursorpos++;
		}
		while (data->cursorpos != wcslen(data->text) && data->text[data->cursorpos] == ' ')
		{
			data->cursorpos++;
		}
	}
	else
	{
		data->cursorpos++;
	}

	if (data->cursorpos > (int)(wcslen(data->text)))
	{
		data->cursorpos = (int)(wcslen(data->text));
	}

	if (!shift)
	{
		data->selectpos = -1;
	}

	Text_UpdateCursor(pbox);
}

void Text_CursorUp(struct Box_s *pbox, int shift)
{
	struct textdata_s *data = pbox->boxdata;

	if (!data->text)
	{
		return;
	}

	if (shift && data->selectpos == -1)
	{
		data->selectpos = data->cursorpos;
	}

	Text_SetCursorToXYPos(pbox, data->cursorx, data->cursory - data->cursorh / 2);

	if (!shift)
	{
		data->selectpos = -1;
	}

	Text_UpdateCursor(pbox);
}

void Text_CursorDown(struct Box_s *pbox, int shift)
{
	struct textdata_s *data = pbox->boxdata;

	if (!data->text)
	{
		return;
	}

	if (shift && data->selectpos == -1)
	{
		data->selectpos = data->cursorpos;
	}

	Text_SetCursorToXYPos(pbox, data->cursorx, data->cursory + data->cursorh + data->cursorh / 2);

	if (!shift)
	{
		data->selectpos = -1;
	}

	Text_UpdateCursor(pbox);
}

void Text_HomeKey(struct Box_s *pbox, int shift)
{
	struct textdata_s *data = pbox->boxdata;

	if (!data->text)
	{
		return;
	}

	if (shift && data->selectpos == -1)
	{
		data->selectpos = data->cursorpos;
	}

	data->cursorpos = 0;

	if (!shift)
	{
		data->selectpos = -1;
	}

	Text_UpdateCursor(pbox);
}

void Text_EndKey(struct Box_s *pbox, int shift)
{
	struct textdata_s *data = pbox->boxdata;

	if (!data->text)
	{
		return;
	}
	
	if (shift && data->selectpos == -1)
	{
		data->selectpos = data->cursorpos;
	}

	data->cursorpos = (int)(wcslen(data->text));

	if (!shift)
	{
		data->selectpos = -1;
	}

	Text_UpdateCursor(pbox);
}

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

void Text_DeleteSelection(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;
	WCHAR *src, *dst;

	if (!data->text || data->selectpos == -1)
	{
		return;
	}

	src = data->text + MAX(data->cursorpos, data->selectpos);
	dst = data->text + MIN(data->cursorpos, data->selectpos);

	while (*src)
	{
		*dst++ = *src++;
	}

	*dst = L'\0';

	data->cursorpos = MIN(data->cursorpos, data->selectpos);
	data->selectpos = -1;

	Text_RedoTextBoxes(pbox);
}

void Text_BackspaceKey(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;

	if (!data->text)
	{
		return;
	}

	if (data->selectpos != -1)
	{
		Text_DeleteSelection(pbox);
	}
	else if (data->cursorpos > 0)
	{
		WCHAR *src = data->text + data->cursorpos;
		WCHAR *dst = data->text + data->cursorpos - 1;

		while (*src)
		{
			*dst++ = *src++;
		}

		*dst = L'\0';

		data->cursorpos--;

		Text_RedoTextBoxes(pbox);
	}
}

void Text_DeleteKey(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;

	if (!data->text)
	{
		return;
	}

	if (data->selectpos != -1)
	{
		Text_DeleteSelection(pbox);
	}
	else if (data->cursorpos < (int)(wcslen(data->text)))
	{
		WCHAR *src = data->text + data->cursorpos + 1;
		WCHAR *dst = data->text + data->cursorpos;

		while (*src)
		{
			*dst++ = *src++;
		}

		*dst = L'\0';

		Text_RedoTextBoxes(pbox);
	}
}

void Text_InsertWTextAtCursor(struct Box_s *pbox, WCHAR *text)
{
	struct textdata_s *data = pbox->boxdata;
	WCHAR *newtext;
	int leftpos, rightpos;

	if (!data->text)
	{
		newtext = malloc(sizeof(WCHAR) * (wcslen(text) + 1));
	}
	else
	{
		newtext = malloc(sizeof(WCHAR) * (wcslen(text) + wcslen(data->text) + 1));
	}

	newtext[0] = L'\0';

	if (data->text)
	{
		if (data->selectpos != -1 && data->selectpos < data->cursorpos)
		{
			wcsncpy(newtext, data->text, data->selectpos);
			newtext[data->selectpos] = L'\0';
		}
		else
		{
			wcsncpy(newtext, data->text, data->cursorpos);
			newtext[data->cursorpos] = L'\0';
		}
	}

	leftpos = (int)(wcslen(newtext));

	wcscat(newtext, text);

	rightpos = (int)(wcslen(newtext));

	if (data->text)
	{
		wcscat(newtext, data->text + MAX(data->cursorpos, data->selectpos));
	}

	/*
	if (select)
	{
		data->cursorpos = rightpos;
		data->selectpos = leftpos;
	}
	else
	*/
	{
		data->cursorpos = rightpos;
		data->selectpos = -1;
	}

	free(data->text);
	data->text = newtext;

	Text_RedoTextBoxes(pbox);
}

void Text_SetCursorPos(struct Box_s *pbox, int cursorpos, int selectpos)
{
	struct textdata_s *data = pbox->boxdata;

	data->cursorpos = cursorpos;
	data->selectpos = selectpos;

	data->delayedarrange = 1;
	Box_Repaint(pbox);
	/*Text_ArrangeBoxes(pbox);*/
}


void Text_SetShowCursor(struct Box_s *pbox, int show)
{
	struct textdata_s *data = pbox->boxdata;

	data->showcursor = show;

	Box_RemoveTimedFunc(pbox, Text_FlashCursor, 500);

	if (show)
	{
		Box_AddTimedFunc(pbox, Text_FlashCursor, NULL, 500);
	}

	data->delayedarrange = 1;
	Box_Repaint(pbox);
	/*Text_ArrangeBoxes(pbox);*/
}

void Text_GetCursorXY(struct Box_s *pbox, int *x, int *y)
{
	struct textdata_s *data = pbox->boxdata;

	*x = data->cursorx;
	*y = data->cursory;
}

int Text_GetCharPosAtXYPos(struct Box_s *pbox, int x, int y)
{
	struct textdata_s *data = pbox->boxdata;
	struct textchar_s *letter = data->chars;
	struct textchar_s *prev = NULL;
	int cpos, rpos;

	if (!data->text || wcslen(data->text) == 0)
	{
		return -1;
	}

	rpos = -1;
	cpos = 0;

#define ABS(x) ((x)>0?(x):(-(x)))

	while (letter)
	{
		/* test vertical */
		if (y >= letter->y && y <= letter->y + letter->h)
		{
			if (x <= letter->x && (!prev || prev->y != letter->y))
			{
				rpos = cpos;
			}
			else if (x > letter->x && x <= letter->x + letter->w / 2)
			{
				rpos = cpos;
			}
			else if (x > letter->x + letter->w / 2)
			{
				rpos = cpos + 1;
			}

			/* test left gap */
			/*
			if (ABS(x - child->x) < dx)
			{
				rpos = cpos;
				if (rpos < 0)
				{
					rpos = 0;
				}
				dx = ABS(x - child->x);
			}

			/* test right gap */
			/*
			if (ABS(x - (child->x + child->w)) < dx)
			{
				rpos = cpos + 1;
				dx = ABS(x - (child->x + child->w));
			}
			*/
		}
		cpos++;

		prev = letter;
		letter = letter->next;
	}

	if (y < 0)
	{
		rpos = 0;
	}
	else if (y > pbox->h)
	{
		rpos = cpos;
	}

#undef ABS

	return rpos;
}

void Text_SetCursorToXYPos(struct Box_s *pbox, int x, int y)
{
	struct textdata_s *data = pbox->boxdata;
	int rpos;

	rpos = Text_GetCharPosAtXYPos(pbox, x, y);

	if (rpos != -1 && rpos != data->cursorpos)
	{
                data->cursorpos = rpos;

		data->delayedarrange = 1;
		Box_Repaint(pbox);
		/*Text_ArrangeBoxes(pbox);*/
	}
}

char *Text_GetText(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;

	return Util_ConvertWCHARToUTF8(data->text);
}

WCHAR *Text_GetTextFilterCommands(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;
	WCHAR *newtext, *src, *dst;

	if (!data->text || wcslen(data->text) == 0)
	{
		return NULL;
	}

	if (data->flags & TX_NOCOMMANDS)
	{
		return wcsdup(data->text);
	}

	newtext = malloc(sizeof(WCHAR) * (wcslen(data->text) + 1));
	src = data->text;
	dst = newtext;

	while (*src)
	{
		int skip = 0;
		if (*src == L'^')
		{
			skip = 1;
			src++;
			if (*src == L'^' || *src == L'\0')
			{
				skip = 0;
			}
			else
			{
				src++;
			}
		}

		if (!skip)
		{
			*dst++ = *src++;
		}
	}

	*dst = *src;

	return newtext;
}

void Text_OnCopy(struct Box_s *pbox)
{
	struct textdata_s *data = pbox->boxdata;
	struct textdata_s *cdata;
	int len = 0, min, max;
	struct Box_s *start, *end, *current;
	WCHAR *copytext;

	if (!data->text || data->selectpos == -1)
	{
		return;
	}
	
	min = MIN(data->cursorpos, data->selectpos);
	max = MAX(data->cursorpos, data->selectpos);

	len += max - min;

	start = pbox;
	end = pbox;

	if (data->linkedselectprev)
	{
		struct Box_s *prev = data->linkedselectprev;
		struct textdata_s *pdata = prev->boxdata;

		while (prev && pdata->selectpos != -1)
		{
			int min2, max2;
			min2 = MIN(pdata->cursorpos, pdata->selectpos);
			max2 = MAX(pdata->cursorpos, pdata->selectpos);
			len += max2 - min2 + 2;
			start = prev;
			prev = pdata->linkedselectprev;
			if (prev)
			{
				pdata = prev->boxdata;
			}
		}
	}

	if (data->linkedselectnext)
	{
		struct Box_s *next = data->linkedselectnext;
		struct textdata_s *ndata = next->boxdata;

		while (next && ndata->selectpos != -1)
		{
			int min2, max2;
			min2 = MIN(ndata->cursorpos, ndata->selectpos);
			max2 = MAX(ndata->cursorpos, ndata->selectpos);
			len += max2 - min2 + 2;
			end = next;
			next = ndata->linkedselectnext;
			if (next)
			{
				ndata = next->boxdata;
			}
		}
	}

	copytext = malloc(sizeof(WCHAR) * (len + 1));
	copytext[0] = L'\0';

	current = start;
	cdata = current->boxdata;

	while (current && cdata->selectpos != -1)
	{
		int min2, max2;
		WCHAR *from = Text_GetTextFilterCommands(current);
		min2 = MIN(cdata->cursorpos, cdata->selectpos);
		max2 = MAX(cdata->cursorpos, cdata->selectpos);

		if (current != start)
		{
			wcscat(copytext, L"\r\n");
		}

		wcsncat(copytext, from + min2, max2 - min2);
		free(from);

		current = cdata->linkedselectnext;
		if (current)
		{
			cdata = current->boxdata;
		}
	}

	if (OpenClipboard(pbox->hwnd))
	{
		HGLOBAL hg;

		EmptyClipboard();

		if (Util_OldWinVer())
		{
			char *temp;
			char *ansitext = Util_ConvertWCHARToANSI(copytext);
			hg = GlobalAlloc(GMEM_MOVEABLE, sizeof(char) * (len + 1));

			if (!hg)
			{
				CloseClipboard();
				Log_Write(0, "copy error!\n");
				return;
			}

			temp = GlobalLock(hg);
			strcpy(temp, ansitext);
			GlobalUnlock(hg);

			SetClipboardData(CF_TEXT, hg);
			free(ansitext);
		}
		else
		{
			WCHAR *temp;
			hg = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * (len + 1));

			if (!hg)
			{
				CloseClipboard();
				Log_Write(0, "copy error!\n");
				return;
			}

			temp = GlobalLock(hg);
			wcscpy(temp, copytext);
			temp[len] = L'\0';
			GlobalUnlock(hg);

			SetClipboardData(CF_UNICODETEXT, hg);
		}

		CloseClipboard();

		free(copytext);
	}
#if 0
	if (OpenClipboard(pbox->hwnd))
	{
		HGLOBAL hg;
		WCHAR *copytext = Text_GetTextFilterCommands(pbox);

		EmptyClipboard();

		if (Util_OldWinVer())
		{
			char *temp;
			char *ansitext = Util_ConvertWCHARToANSI(copytext);
			int max = MAX(data->cursorpos, data->selectpos);
			int min = MIN(data->cursorpos, data->selectpos);
			int len = max - min;
			hg = GlobalAlloc(GMEM_MOVEABLE, sizeof(char) * (len + 1));

			if (!hg)
			{
				CloseClipboard();
				Log_Write(0, "copy error!\n");
				return;
			}

			temp = GlobalLock(hg);
			strncpy(temp, ansitext + min, len);
			temp[len] = L'\0';
			GlobalUnlock(hg);

			SetClipboardData(CF_TEXT, hg);
			free(ansitext);
		}
		else
		{
			WCHAR *temp;
			int max = MAX(data->cursorpos, data->selectpos);
			int min = MIN(data->cursorpos, data->selectpos);
			int len = max - min;
			hg = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * (len + 1));

			if (!hg)
			{
				CloseClipboard();
				Log_Write(0, "copy error!\n");
				return;
			}

			temp = GlobalLock(hg);
			wcsncpy(temp, copytext + min, len);
			temp[len] = L'\0';
			GlobalUnlock(hg);

			SetClipboardData(CF_UNICODETEXT, hg);
		}

		CloseClipboard();

		free(copytext);
	}
#endif
}

void Text_SetLinkedSelectPrev(struct Box_s *text, struct Box_s *prev)
{
	struct textdata_s *data = text->boxdata;
	data->linkedselectprev = prev;
}

void Text_SetLinkedSelectNext(struct Box_s *text, struct Box_s *next)
{
	struct textdata_s *data = text->boxdata;
	data->linkedselectnext = next;
}

void Text_SetParentList(struct Box_s *text, struct Box_s *list)
{
	struct textdata_s *data = text->boxdata;
	data->parentlist = list;
}

int Text_TextSelected(struct Box_s *text)
{
	struct textdata_s *data = text->boxdata;

	return (data->selectpos != -1);
}

void Text_SetPaletteColor(struct Box_s *text, int colornum, COLORREF col)
{
	struct textdata_s *data = text->boxdata;

	data->palette[colornum] = col;
}

void Text_SetLinkColor(struct Box_s *text, COLORREF col)
{
	struct textdata_s *data = text->boxdata;

	data->palette[6] = col;
}