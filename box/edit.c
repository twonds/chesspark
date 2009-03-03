#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>

#include "box.h"

#include "constants.h"

#include "log.h"
#include "util.h"

#include "edit.h"


/* FIXME: rearrange code so I don't have to declare these as extern */
/*extern int Util_OldWinVer();
extern char *Util_ConvertWCHARToUTF8(const WCHAR *intext);
extern char *Util_ConvertWCHARToANSI(WCHAR *intext);*/
extern void Menu_PopupEditMenu(struct Box_s *pbox, int x, int y, int canedit, int cancopy, int canselect);

struct editboxdata_s
{
	WCHAR *text;
	int curpos;
	int hidetext;
	int disabled;
	void (*OnEnter)(struct Box_s *pbox, char *text);
	void (*OnKey)(struct Box_s *pbox, char *text);
};


void EditBox_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct editboxdata_s *data = pbox->boxdata;

	if (data->disabled)
	{
		return;
	}

	Box_SetFocus(pbox);

	Box_Repaint(pbox);
}

void EditBox_OnPaste(struct Box_s *pbox)
{
	WCHAR beforecur[1024], aftercur[1024];
	struct editboxdata_s *data = pbox->boxdata;
	wcsncpy(beforecur, data->text, data->curpos);
	beforecur[data->curpos] = 0;
	aftercur[0] = 0;
	wcscpy(aftercur, data->text + data->curpos);

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

					data->text[0] = 0;
					wcscat(data->text, beforecur);
					wcscat(data->text, pastetext);
					data->curpos = (int)wcslen(data->text);
					wcscat(data->text, aftercur);
					if (data->OnKey)
					{
						data->OnKey(pbox, EditBox_GetText(pbox));
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

					data->text[0] = 0;
					wcscat(data->text, beforecur);
					wcscat(data->text, pastetext);
					data->curpos = (int)wcslen(data->text);
					wcscat(data->text, aftercur);
					if (data->OnKey)
					{
						data->OnKey(pbox, EditBox_GetText(pbox));
					}
				}
			}
			CloseClipboard();
		}
	}

	Box_Repaint(pbox);
}

void EditBox_OnKeyDown(struct Box_s *pbox, int vk, int scan)
{
	struct editboxdata_s *data = pbox->boxdata;
	int len = (int)wcslen(data->text);
	WCHAR beforecur[1024], aftercur[1024];
	char keystate[256];
	WCHAR unicode;

	if (data->disabled)
	{
		return;
	}
	
	/* reject alt, ctrl, and shift keys, since they won't add a character */
	if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU)
	{
		return;
	}

	GetKeyboardState(keystate);

	if (Util_OldWinVer())
	{
		char ascii[2];

		len = ToAscii(vk, scan, keystate, (LPWORD)ascii, 0);

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
	
	wcsncpy(beforecur, data->text, data->curpos);
	beforecur[data->curpos] = 0;
	aftercur[0] = 0;
	wcscpy(aftercur, data->text + data->curpos);

	if (vk == 8) /* backspace */
	{
		if (data->curpos != 0) 
		{
			data->curpos--;
			beforecur[data->curpos] = 0;
			data->text[0] = 0;
			wcscat(data->text, beforecur);
			wcscat(data->text, aftercur);
			if (data->OnKey)
			{
				data->OnKey(pbox, EditBox_GetText(pbox));
			}
		}
	}
	else if (vk == 46) /* delete */
	{
		if (wcslen(aftercur) > 0)
		{
			data->text[0] = 0;
			wcscat(data->text, beforecur);
			wcscat(data->text, aftercur + 1);
			if (data->OnKey)
			{
				data->OnKey(pbox, EditBox_GetText(pbox));
			}

		}
	}
	else if (vk == 9) /* tab */
	{
		Box_SetNextFocus(pbox, keystate[VK_SHIFT] & 0x8000);
	}
	else if (vk == 13) /* enter */
	{
		if (data->OnKey)
		{
			data->OnKey(pbox, EditBox_GetText(pbox));
		}
		if (data->OnEnter)
		{
			data->OnEnter(pbox, EditBox_GetText(pbox));
		}
	}
	else if (vk == 35) /* end */
	{
		data->curpos = len;
	}
	else if (vk == 36) /* home */
	{
		data->curpos = 0;
	}
	else if (vk == 37) /* left */
	{
		if (data->curpos != 0)
		{
			data->curpos--;
		}
	}
	else if (vk == 39) /* right */
	{
		if (data->curpos != len)
		{
			data->curpos++;
		}
	}
	else if ((vk == 86 && (keystate[VK_CONTROL] & 0x8000)) || (vk == 45 && (keystate[VK_SHIFT] & 0x8000))) /* ctrl+v, shift+ins */
	{
		EditBox_OnPaste(pbox);
	}
	else if (len < 1023 && unicode > 31) /* Displayable character */
	{
		beforecur[data->curpos] = unicode;
		data->curpos++;
		beforecur[data->curpos] = 0;
		data->text[0] = 0;
		wcscat(data->text, beforecur);
		wcscat(data->text, aftercur);
		if (data->OnKey)
		{
			data->OnKey(pbox, EditBox_GetText(pbox));
		}
	}
	else
	{
		Log_Write(0, "unhandled vk %d\n", vk);
	}

	Box_Repaint(pbox);
}

void EditBox_OnRButtonDown(struct Box_s *pbox, int x, int y)
{	
	int sx, sy;
	Box_GetScreenCoords(pbox, &sx, &sy);
	sx += x;
	sy += y;
	Menu_PopupEditMenu(pbox, sx, sy, 1, 1, 1);
}

#define MIN(a,b) ((a)<(b))?(a):(b)
#define MAX(a,b) ((a)>(b))?(a):(b)
#define MIN3(a,b,c) MIN(MIN((a),(b)),(c))

extern RECT cliprects(RECT rc1, RECT rc2);

extern HFONT tahoma11b_f, tahoma11_f;

void EditBox_OnPaint(struct Box_s *pbox, HDC hdc, RECT rcClip, int x, int y, int paintmask)
{
	struct editboxdata_s *data = pbox->boxdata;

	struct Box_s *child = pbox->child;
	RECT rcBox, rc;

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
	rc.left   = MAX(rcBox.left,   rcClip.left);
	rc.right  = MIN(rcBox.right,  rcClip.right);
	rc.top    = MAX(rcBox.top,    rcClip.top);
	rc.bottom = MIN(rcBox.bottom, rcClip.bottom);

	/* If nothing to draw, punt */
	if ((rc.right <= rc.left) || (rc.bottom <= rc.top))
	{
		return;
	}

	Box_OnPaint(pbox, hdc, rcClip, x, y, paintmask);

	if (data->text)
	{
		COLORREF oldfgcol;
		int oldbkmode;
		HFONT oldfont;
		RECT draw = rcBox;
		WCHAR firsthalf[1024], *secondhalf;
		RECT measure = rcBox;
		int pass = 0, xtext;

		oldbkmode = SetBkMode(hdc, TRANSPARENT);
		oldfgcol = SetTextColor(hdc, pbox->fgcol);
		
		if (pbox->font)
		{
			oldfont = SelectObject(hdc, pbox->font);
		}
		else
		{
			oldfont = SelectObject(hdc, tahoma11_f);
		}

		pass = data->curpos - data->curpos % 40;

		if (data->hidetext)
		{
			int i;

			for (i = 0; i < data->curpos - pass; i++)
			{
				firsthalf[i] = L'*';
			}
			firsthalf[i] = 0;
			secondhalf = malloc((wcslen(data->text + data->curpos) + 1) * sizeof(WCHAR));
			for (i = 0; i < (int)wcslen(data->text + data->curpos); i++)
			{
				secondhalf[i] = L'*';
			}
			secondhalf[i] = 0;
		}
		else
		{
			wcsncpy(firsthalf, data->text + pass, data->curpos - pass);
			firsthalf[data->curpos - pass] = 0;
			secondhalf = data->text + data->curpos;
		}
		
		if (Util_OldWinVer())
		{
			char *ansifirst = Util_ConvertWCHARToANSI(firsthalf);
			char *ansisecond = Util_ConvertWCHARToANSI(secondhalf);

			measure.right = measure.left;
			DrawTextA(hdc, ansifirst, (int)strlen(ansifirst), &measure, DT_CALCRECT | DT_NOPREFIX);

			ExtTextOutA(hdc, draw.left + 2, draw.top + 2, ETO_CLIPPED, &rc, ansifirst, (UINT)strlen(ansifirst), NULL);

			xtext = draw.left + 2 + measure.right - measure.left;

			if (Box_HasFocus(pbox))
			{
				MoveToEx(hdc, xtext, draw.top + 2, NULL);
				LineTo(hdc, xtext, draw.top + 18);
			}

			ExtTextOutA(hdc, xtext, draw.top + 2, ETO_CLIPPED, &rc, ansisecond, (UINT)strlen(ansisecond), NULL);

			if (pbox->font)
			{
				SelectObject(hdc, oldfont);
			}

			SetBkMode(hdc, oldbkmode);
			SetTextColor(hdc, oldfgcol);
		}
		else
		{
			measure.right = measure.left;
			DrawTextW(hdc, firsthalf, (int)wcslen(firsthalf), &measure, DT_CALCRECT | DT_NOPREFIX);

			ExtTextOutW(hdc, draw.left + 2, draw.top + 2, ETO_CLIPPED, &rc, firsthalf, (UINT)wcslen(firsthalf), NULL);

			xtext = draw.left + 2 + measure.right - measure.left;

			if (Box_HasFocus(pbox))
			{
				MoveToEx(hdc, xtext, draw.top + 2, NULL);
				LineTo(hdc, xtext, draw.top + 18);
			}

			ExtTextOutW(hdc, xtext, draw.top + 2, ETO_CLIPPED, &rc, secondhalf, (UINT)wcslen(secondhalf), NULL);
		}

		if (pbox->font)
		{
			SelectObject(hdc, oldfont);
		}

		SetBkMode(hdc, oldbkmode);
		SetTextColor(hdc, oldfgcol);

	}
}

struct Box_s *EditBox_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Box_s *pbox = Box_Create(x, y, w, h, flags);
	struct editboxdata_s *data = malloc(sizeof(*data));

	memset(data, 0, sizeof(*data));
	
	pbox->OnLButtonDown = EditBox_OnLButtonDown;
	pbox->OnRButtonDown = EditBox_OnRButtonDown;
	pbox->OnKeyDown		= EditBox_OnKeyDown;
	pbox->OnPaste		= EditBox_OnPaste;
	data->text = malloc(1024 * sizeof(WCHAR));
	data->text[0] = L'\0';
	data->curpos = 0;
	pbox->boxdata = data;
	pbox->OnPaint = EditBox_OnPaint;

	return pbox;
}

void EditBox_SetOnEnter(struct Box_s *pbox, void *pfunc)
{
	struct editboxdata_s *data = pbox->boxdata;

	data->OnEnter = pfunc;
}

void EditBox_SetOnKey(struct Box_s *pbox, void *pfunc)
{
	struct editboxdata_s *data = pbox->boxdata;

	data->OnKey = pfunc;
}

void EditBox_ClearText(struct Box_s *pbox)
{
	struct editboxdata_s *data = pbox->boxdata;

	data->text[0] = 0;
	data->curpos = 0;

}

char *EditBox_GetText(struct Box_s *pbox)
{
	struct editboxdata_s *data = pbox->boxdata;
	return Util_ConvertWCHARToUTF8(data->text);
}

void EditBox_SetText(struct Box_s *pbox, char *text)
{
	struct editboxdata_s *data = pbox->boxdata;

	if (!text)
	{
		*(data->text) = L'\0';
		data->curpos = 0;

	}
	else
	{
		MultiByteToWideChar(CP_UTF8, 0, text, -1, data->text, 1024);
		data->curpos = (int)wcslen(data->text);
	}

	Box_Repaint(pbox);
}

struct Box_s *EditPassBox_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Box_s *pbox = EditBox_Create(x, y, w, h, flags);
	struct editboxdata_s *data = pbox->boxdata;

	data->hidetext = 1;

	return pbox;
}

void EditBox_SetDisabled(struct Box_s *pbox, int disabled)
{
	struct editboxdata_s *data = pbox->boxdata;

	data->disabled = disabled;

	if (disabled)
	{
		pbox->bgcol = EditDisabledBG;
	}
	else
	{
		pbox->bgcol = EditBG;
	}

	Box_Repaint(pbox);
}