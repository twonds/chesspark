#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <zmouse.h>
#include <ole2.h>

#include "../resource.h"

#include "log.h"
#include "mem.h"
#include "util.h"

#include "box.h"

struct Box_s *deadboxes = NULL;
struct Box_s *windowboxes = NULL;

static HIMAGELIST dragimglist;
static BOOL dragging = FALSE;
static int dragid = 0;

HFONT tahoma13_f;

HFONT tahoma11_f;
HFONT tahoma11i_f;
HFONT tahoma11b_f;
HFONT tahoma11u_f;
HFONT tahoma11ub_f;

HFONT tahoma10_f;
HFONT tahoma10i_f;
HFONT tahoma10b_f;
HFONT tahoma10u_f;
HFONT tahoma10ub_f;

HFONT tahoma24b_f;

HBITMAP paintoverbmp = NULL;
HDC paintoverdc = NULL;

ATOM boxwndatom;
ATOM boxmenuatom;

struct Box_s *lastactivebox = NULL;
unsigned int lastactiveboxtime = 0;
unsigned int lastmousemovetime = 0;

struct Box_s *Box_CheckLastActive()
{
	if (lastactivebox && lastactivebox->hwnd && (GetTickCount() - lastactiveboxtime <= 5000 || View_IsPlayingAGame())) /* hack so windows don't pop up during a game*/
	{
		return lastactivebox;
	}

	return NULL;
}

struct Box_s *lockedbox = NULL;

void Box_LockInputToBox(struct Box_s *box)
{
	if (!lockedbox)
	{
		lockedbox = box;
	}
}

void Box_UnlockInput()
{
	lockedbox = NULL;
}


BOOL (WINAPI * W32_AlphaBlend)(HDC, int, int, int, int, HDC, int, int, int, int,
                               BLENDFUNCTION);

void Box_InitIdleTracker();
void Box_UninitIdleTracker();

void Box_RemoveTimedFuncsForBox(struct Box_s *pbox);

static BOOL captured = FALSE;
static struct Box_s *capturebox;

BOOL Box_CaptureMouse(struct Box_s *pbox)
{
	if (!captured)
	{
		captured = TRUE;
		SetCapture(pbox->hwnd);
		capturebox = pbox;
		return TRUE;
	}

	if (capturebox == pbox)
	{
		return TRUE;
	}

	return FALSE;
}

void Box_ReleaseMouse(struct Box_s *pbox)
{
	if (captured && pbox == capturebox)
	{
		ReleaseCapture();
		capturebox = NULL;
		captured = FALSE;
	}
}

int Box_IsMouseCaptured()
{
	return captured;
}

int Box_IsMouseCapturedByBox(struct Box_s *pbox)
{
	return (captured && pbox == capturebox);
}

void Box_DragStart(struct Box_s *pbox, int x, int y, int newdragid)
{
	HBITMAP hbmp, hbmpmask;

	dragimglist = ImageList_Create(pbox->w, pbox->h, ILC_COLORDDB | ILC_MASK, 1, 1);
	
	hbmp = Box_Draw_Hbmp(pbox, FALSE);
	hbmpmask = Box_Draw_Hbmp(pbox, TRUE);
	ImageList_Add(dragimglist, hbmp, hbmpmask);
	DeleteObject(hbmp);
	DeleteObject(hbmpmask);

	ImageList_BeginDrag(dragimglist, 0, x, y);
	ImageList_DragShowNolock(TRUE);
	dragging = TRUE;

	dragid = newdragid;
}

void Box_DragMove(struct Box_s *pbox, int x, int y)
{
	ImageList_DragMove(x, y);
}

void Box_DragEnd(struct Box_s *pbox)
{
	ImageList_DragShowNolock(FALSE);
	ImageList_EndDrag();
	ImageList_Destroy(dragimglist);
	dragging = FALSE;
}

BOOL Box_IsDragging()
{
	return dragging;
}

int Box_GetDragId()
{
	return dragid;
}

/***************************************************************************
 * Box_SetHwnd()
 *
 * Internal helper.
 * Assigns the window "hwnd" to the box "pbox" and all subboxes.
 * 
 ***************************************************************************/
static void Box_SetHwnd(struct Box_s *pbox, HWND hwnd)
{
	pbox->hwnd = hwnd;
	pbox = pbox->child;
	while (pbox)
	{
		Box_SetHwnd(pbox, hwnd);
		pbox = pbox->sibling;
	}
}


/***************************************************************************
 * Box_FindPtrToChild()
 *
 * Internal helper.
 * Finds the pointer to the subbox "child" in the box "parent".
 * If "child" is NULL, finds the pointer to where the next child would be.
 * If not found, returns NULL.
 * 
 ***************************************************************************/
#if 0
static struct Box_s **Box_FindPtrToChild(struct Box_s *parent, struct Box_s *child)
{
	struct Box_s **pchild = &(parent->child);
	
	while (*pchild != child && *pchild != NULL)
	{
		pchild = &((*pchild)->sibling);
	}

	if (*pchild != child)
		return NULL;

	return pchild;
}
#endif


/***************************************************************************
 * Box_Create()
 *
 * Allocates and returns a new box and assigns default message handlers.
 * "x", "y" indicate coordinates relative to parent.
 * "w", "h" indicate width and height.
 * "flags" are a bit field of characteristics of the new box.
 * 
 ***************************************************************************/
struct Box_s *Box_CreateReal(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Box_s *pbox = (struct Box_s *)malloc(sizeof(*pbox));
	memset(pbox, 0, sizeof(*pbox));
	pbox->x = x;
	pbox->y = y;
	pbox->w = w;
	pbox->h = h;
	pbox->lastmousex = -1;
	pbox->lastmousey = -1;
	pbox->OnPaint         = Box_OnPaint;
	pbox->OnLButtonDblClk = Box_OnLButtonDblClk;
	pbox->OnLButtonTrpClk = Box_OnLButtonTrpClk;
	pbox->OnLButtonDown   = Box_OnLButtonDown;
	pbox->OnLButtonUp     = Box_OnLButtonUp;
	pbox->OnRButtonDown   = Box_OnRButtonDown;
	pbox->OnRButtonUp     = Box_OnRButtonUp;
	pbox->OnMouseMove     = Box_OnMouseMove;
	pbox->OnDragDrop      = Box_OnDragDrop;
	pbox->OnScrollWheel   = Box_OnScrollWheel;
	pbox->flags = flags;
	return pbox;
}

struct Box_s *Box_CreateWrap(int x, int y, int w, int h, enum Box_flags flags, const char *file, unsigned int line)
{
	struct Box_s *alloc = Box_CreateReal(x, y, w, h, flags);
	Mem_AddMemRec(alloc, 0, "boxtype", file, line);

	return alloc;
}


/***************************************************************************
 * Box_AddChild()
 *
 * Adds the box "child" to the box "parent" as a subbox.
 * Also assigns the hwnd of "parent" hwnd to "child" and all its subboxes.
 * 
 ***************************************************************************/
void Box_AddChild(struct Box_s *parent, struct Box_s *child)
{
	child->parent = parent;
	Box_SetHwnd(child, parent->hwnd);

	if (parent->lastchild)
	{
		parent->lastchild->sibling = child;
	}
	else
	{
		parent->child = child;
	}

	child->prevsibling = parent->lastchild;
	parent->lastchild = child;

/*
	pchild = Box_FindPtrToChild(parent, NULL);
	*pchild = child;
	*/
}


void Box_AddChildToBottom(struct Box_s *parent, struct Box_s *child)
{
	child->parent = parent;
	Box_SetHwnd(child, parent->hwnd);

	child->sibling = parent->child;
	parent->child = child;
	if (child->sibling)
	{
		child->sibling->prevsibling = child;
	}

	if (!parent->lastchild)
	{
		parent->lastchild = child;
	}
}


/***************************************************************************
 * Box_Unlink
 *
 * Unlink "pbox" from its parent and siblings, if any.  Also clears the
 * window assocated with "pbox" and all its children.
 * 
 ***************************************************************************/
void Box_Unlink(struct Box_s *pbox)
{
	if (pbox->parent)
	{
		if (pbox->parent->lastchild == pbox)
		{
			pbox->parent->lastchild = pbox->prevsibling;
		}

		if (pbox->prevsibling)
		{
			pbox->prevsibling->sibling = pbox->sibling;
		}
		else
		{
			pbox->parent->child = pbox->sibling;
		}

		if (pbox->sibling)
		{
			pbox->sibling->prevsibling = pbox->prevsibling;
		}

		pbox->parent = NULL;
		pbox->prevsibling = NULL;
		pbox->sibling = NULL;

		Box_SetHwnd(pbox, NULL);
	}

	if (captured && capturebox == pbox)
	{
		Box_ReleaseMouse(pbox);
	}

	/*Box_RemoveTimedFuncsForBox(pbox);*/
}

void Box_FlushDead()
{
	while (deadboxes)
	{
		struct Box_s *old = deadboxes;
		deadboxes = deadboxes->child;

		free(old->text);
		free(old->boxdata);
		free(old);
	}
}


/***************************************************************************
 * Box_Destroy()
 *
 * Doesn't actually destroy the box now, but calls its ondestroy function,
 * calls Box_Destroy on all its children, unlinks it, and puts the box on
 * the dead box queue.
 *
 *
 ***************************************************************************/
void Box_Destroy(struct Box_s *pbox)
{
	if (!pbox)
	{
		return;
	}
#if 0
	Mem_RemoveMemRec(pbox);
#endif

	if (lockedbox == pbox)
	{
		lockedbox = NULL;
	}

	if (Box_GetRoot(pbox) == deadboxes)
	{
		int x = 5;
		Log_Write(0, "wtf, double destroy!\n");
		x /= 0;
		return;
	}

	if (lastactivebox == pbox)
	{
		lastactivebox = NULL;
	}

	if (pbox == Box_GetRoot(pbox)->focus)
	{
		Box_GetRoot(pbox)->focus = NULL;
	}

	if (pbox->tooltipbox)
	{
		DestroyWindow(pbox->tooltipbox->hwnd);
		Box_Destroy(pbox->tooltipbox);
		pbox->tooltipbox = NULL;
	}

	if (captured && capturebox == pbox)
	{
		Box_ReleaseMouse(pbox);
	}

	if (pbox->OnDestroy)
	{
		pbox->OnDestroy(pbox);
	}

	while (pbox->child)
	{
		Box_Destroy(pbox->child);
	}

	Box_Unlink(pbox);

	Box_RemoveTimedFuncsForBox(pbox);

	if (pbox->hwnd)
	{
		HWND oldhwnd = pbox->hwnd;
		pbox->hwnd = NULL;
		Log_Write(0, "DestroyWindow %d %d\n", pbox, oldhwnd);

		/* remove window from window list */
		{
			struct Box_s **ppbox = &windowboxes;

			while (*ppbox)
			{
				if (*ppbox == pbox)
				{
					free(pbox->windowname);
					*ppbox = pbox->nextwindow;
				}
				else
				{
					ppbox = &((*ppbox)->nextwindow);
				}
			}
		}

		/* make sure this window is never referred to again */
		SetWindowLong(oldhwnd, GWL_USERDATA, (LONG)NULL);
		DestroyWindow(oldhwnd);
	}

	pbox->child = deadboxes;
	deadboxes = pbox;
/*	
	if (pbox->text)
	{
		free(pbox->text);
	}
	
	free(pbox->boxdata);
	free(pbox);
*/
}

#define MIN(a,b) ((a)<(b))?(a):(b)
#define MAX(a,b) ((a)>(b))?(a):(b)
#define MIN3(a,b,c) MIN(MIN((a),(b)),(c))

RECT cliprects(RECT rc1, RECT rc2)
{
	RECT result;

	result.left   = MAX(rc1.left,   rc2.left);
	result.top    = MAX(rc1.top,    rc2.top);
	result.right  = MIN(rc1.right,  rc2.right);
	result.bottom = MIN(rc1.bottom, rc2.bottom);

	return result;
}

void Box_Blend(HDC dst, int dx, int dy, int dw, int dh, struct BoxImage_s *isrc, int sx, int sy, int sw, int sh, int paintmask)
{
	BLENDFUNCTION bf;
	HDC src = CreateCompatibleDC(dst);
	HBITMAP oldbmp = SelectObject(src, isrc->hbmp);

	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 255;

	if (W32_AlphaBlend && !paintmask)
	{
		W32_AlphaBlend(dst, dx, dy, dw, dh, src, sx, sy, sw, sh, bf);
	}
	else if (dw == sw && dh == sh)
	{
		/* Our source and destination height and width are the same, so don't bother using StretchBlt */
		if (!paintmask)
		{
			BitBlt(dst, dx, dy, dw, dh, src, sx, sy, SRCINVERT);
		}

		SelectObject(src, isrc->hbmpmask);

		BitBlt(dst, dx, dy, dw, dh, src, sx, sy, SRCAND);

		if (!paintmask)
		{
			SelectObject(src, isrc->hbmp);

			BitBlt(dst, dx, dy, dw, dh, src, sx, sy, SRCINVERT);
		}
	}
	else
	{
		/* Draw by XORing image, ANDing mask, and XORing image again */
		if (!paintmask)
		{
			StretchBlt(dst, dx, dy, dw, dh, src, sx, sy, sw, sh, SRCINVERT);
		}

		SelectObject(src, isrc->hbmpmask);

		StretchBlt(dst, dx, dy, dw, dh, src, sx, sy, sw, sh, SRCAND);

		if (!paintmask)
		{
			SelectObject(src, isrc->hbmp);

			StretchBlt(dst, dx, dy, dw, dh, src, sx, sy, sw, sh, SRCINVERT);
		}
	}

	SelectObject(src, oldbmp);
	DeleteDC(src);
}

void Box_MeasureText2(struct Box_s *pbox, HFONT hfont, char *text, int *w, int *h)
{
	HWND hwnd;
	HDC hdc;
	HFONT oldfont;
	RECT measure;

	if (pbox)
	{
		hwnd = pbox->hwnd;
	}
	else
	{
		hwnd = NULL;
	}

	hdc = GetDC(hwnd);

	if (hfont)
	{
		oldfont = SelectObject(hdc, hfont);
	}
	else if (pbox && pbox->font)
	{
		oldfont = SelectObject(hdc, pbox->font);
	}
	else
	{
		oldfont = SelectObject(hdc, tahoma11_f);
	}

	measure.top = 0;
	measure.bottom = 1;
	measure.left = 0;
	measure.right = 1;

	if (Util_OldWinVer())
	{
		char *temptext = Util_ConvertUTF8ToANSI(text);

		DrawTextA(hdc, temptext, (UINT)strlen(temptext), &measure, DT_CALCRECT | DT_NOPREFIX);
		
		free(temptext);
	}
	else
	{
		WCHAR *widetext = Util_ConvertUTF8ToWCHAR(text);

		DrawTextW(hdc, widetext, (UINT)wcslen(widetext), &measure, DT_CALCRECT | DT_NOPREFIX);

		free(widetext);
	}

	SelectObject(hdc, oldfont);

	ReleaseDC(hwnd, hdc);

	if (w)
	{
		*w = measure.right - measure.left;
	}

	if (h)
	{
		*h = measure.bottom - measure.top;
	}
}


void Box_MeasureText(struct Box_s *pbox, char *text, int *w, int *h)
{
	Box_MeasureText2(pbox, NULL, text, w, h);
}

/***************************************************************************
 * Box_OnPaint()
 *
 * Draws "pbox" and all its subboxes on the device context "hdc" at the
 * coordinates "x","y", clipped by the rect rcClip.
 *
 * Does not draw the box or any subboxes if the BOX_VISIBLE flag is not set.
 * Does not fill the background if the BOX_TRANSPARENT flag is set.
 *
 ***************************************************************************/
void Box_OnPaint(struct Box_s *pbox, HDC hdc, RECT rcClip, int x, int y, int paintmask)
{
	struct Box_s *child = pbox->child;
	RECT rcBox, rc;
	BLENDFUNCTION bf;
	HRGN cliprgn;

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

	/* if BOX_NOCLIP, the box rect is the clip rect */
	if (pbox->flags & BOX_NOCLIP)
	{
		rcClip = rcBox;
	}

	/* Clip the box rect against the draw rect */
	rc = cliprects(rcBox, rcClip);
	
	/* If nothing to draw, punt */
	if ((rc.right < rc.left) || (rc.bottom < rc.top))
	{
		return;
	}

	if (paintmask)
	{
		/*BitBlt(hdc, rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1, NULL, 0, 0, WHITENESS);*/
	}

	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 255;

	/* set clipping region */
	cliprgn = CreateRectRgn(rc.left, rc.top, rc.right + 1, rc.bottom + 1);
	SelectClipRgn(hdc, cliprgn);

	if (!(pbox->flags & BOX_TRANSPARENT))
	{
		if (!paintmask)
		{
			HBRUSH bgbrush, oldbrush;
			HPEN bgpen, oldpen;

			bgbrush = CreateSolidBrush(pbox->bgcol);
			bgpen   = CreatePen(PS_SOLID, 0, pbox->bgcol);

			oldbrush = SelectObject(hdc, bgbrush);
			oldpen   = SelectObject(hdc, bgpen);

			Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);

			SelectObject(hdc, oldbrush);
			SelectObject(hdc, oldpen);

			DeleteObject(bgbrush);
			DeleteObject(bgpen);
		}
		/*
		else
		{
			BitBlt(hdc, rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1, NULL, 0, 0, WHITENESS);
		}
		*/
	}

	if ((pbox->flags & BOX_BORDERALL) == BOX_BORDER || ((pbox->flags & BOX_BORDERALL) == BOX_BORDER4))
	{
		HPEN brpen, oldpen;
		HBRUSH bgbrush, oldbrush;

		if (!paintmask)
		{
			if ((pbox->flags & BOX_BORDERALL) == BOX_BORDER4)
			{
				brpen = CreatePen(PS_DOT, 0, pbox->brcol);
				bgbrush = CreateSolidBrush(RGB(0, 0, 0));
				oldbrush = SelectObject(hdc, bgbrush);
			}
			else
			{
				brpen = CreatePen(PS_SOLID, 0, pbox->brcol);
			}
		}
		else
		{
			brpen = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
		}
	
		oldpen = SelectObject(hdc, brpen);

		/* left */
		if (rc.left <= rcBox.left)
		{
			MoveToEx(hdc, rcBox.left, rc.top, (LPPOINT)NULL); 
			LineTo(hdc, rcBox.left, rc.bottom + 1);
		}

		/* top */
		if (rc.top <= rcBox.top)
		{
			MoveToEx(hdc, rc.right, rcBox.top, (LPPOINT)NULL); 
			LineTo(hdc, rc.left - 1, rcBox.top);
		}

		/* right */
		if ((rc.right >= rcBox.right) && !((pbox->flags & BOX_BORDERALL) == BOX_BORDER3))
		{
			MoveToEx(hdc, rcBox.right, rc.bottom, (LPPOINT)NULL); 
			LineTo(hdc, rcBox.right, rc.top - 1);
		}

		/* bottom */
		if (rc.bottom >= rcBox.bottom)
		{
			MoveToEx(hdc, rc.left, rcBox.bottom, (LPPOINT)NULL); 
			LineTo(hdc, rc.right + 1, rcBox.bottom);
		}

		if ((pbox->flags & BOX_BORDERALL) == BOX_BORDER4)
		{
			SelectObject(hdc, oldbrush);
			DeleteObject(bgbrush);
		}

		SelectObject(hdc, oldpen);

		DeleteObject(brpen);
	}

	if ((pbox->flags & BOX_BORDERALL) == BOX_BORDER5)
	{
		HPEN brpen, oldpen;
		COLORREF bbg;
		struct Box_s *parent = pbox->parent;

		while (parent)
		{
			if (!(parent->flags & BOX_TRANSPARENT))
			{
				bbg = parent->bgcol;
				parent = NULL;
			}
			else
			{
				parent = parent->parent;
			}
		}

		if (!paintmask)
		{
			brpen = CreatePen(PS_SOLID, 0, pbox->brcol);
		}
		else
		{
			brpen = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
		}
	
		oldpen = SelectObject(hdc, brpen);

		MoveToEx(hdc, rcBox.left + 3, rcBox.top, (LPPOINT)NULL);
		LineTo(hdc, rcBox.left, rcBox.top + 3);
		LineTo(hdc, rcBox.left, rcBox.bottom - 3);
		LineTo(hdc, rcBox.left + 3, rcBox.bottom);
		LineTo(hdc, rcBox.right - 3, rcBox.bottom);
		LineTo(hdc, rcBox.right, rcBox.bottom - 3);
		LineTo(hdc, rcBox.right, rcBox.top + 3);
		LineTo(hdc, rcBox.right - 3, rcBox.top);
		LineTo(hdc, rcBox.left + 3, rcBox.top);

		SelectObject(hdc, oldpen);
		DeleteObject(brpen);
		brpen = CreatePen(PS_SOLID, 0, bbg);
		oldpen = SelectObject(hdc, brpen);
		MoveToEx(hdc, rcBox.left, rcBox.top, (LPPOINT)NULL);
		LineTo(hdc, rcBox.left, rcBox.top + 3);
		MoveToEx(hdc, rcBox.left + 1, rcBox.top, (LPPOINT)NULL);
		LineTo(hdc, rcBox.left + 1, rcBox.top + 2);
		MoveToEx(hdc, rcBox.left + 2, rcBox.top, (LPPOINT)NULL);
		LineTo(hdc, rcBox.left + 2, rcBox.top + 1);

		MoveToEx(hdc, rcBox.left, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.left, rcBox.bottom - 3);
		MoveToEx(hdc, rcBox.left + 1, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.left + 1, rcBox.bottom - 2);
		MoveToEx(hdc, rcBox.left + 2, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.left + 2, rcBox.bottom - 1);

		MoveToEx(hdc, rcBox.right, rcBox.top, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right, rcBox.top + 3);
		MoveToEx(hdc, rcBox.right - 1, rcBox.top, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right - 1, rcBox.top + 2);
		MoveToEx(hdc, rcBox.right - 2, rcBox.top, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right - 2, rcBox.top + 1);

		MoveToEx(hdc, rcBox.right, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right, rcBox.bottom - 3);
		MoveToEx(hdc, rcBox.right - 1, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right - 1, rcBox.bottom - 2);
		MoveToEx(hdc, rcBox.right - 2, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right - 2, rcBox.bottom - 1);

		SelectObject(hdc, oldpen);

		DeleteObject(brpen);
	}

	if ((pbox->flags & BOX_BORDERALL) == BOX_BORDER6)
	{
		HPEN brpen, oldpen;
		COLORREF bbg;
		struct Box_s *parent = pbox->parent;

		while (parent)
		{
			if (!(parent->flags & BOX_TRANSPARENT))
			{
				bbg = parent->bgcol;
				parent = NULL;
			}
			else
			{
				parent = parent->parent;
			}
		}

		if (!paintmask)
		{
			brpen = CreatePen(PS_SOLID, 0, pbox->brcol);
		}
		else
		{
			brpen = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
		}
	
		oldpen = SelectObject(hdc, brpen);

		MoveToEx(hdc, rcBox.left, rcBox.top, (LPPOINT)NULL);
		LineTo(hdc, rcBox.left, rcBox.bottom - 3);
		LineTo(hdc, rcBox.left + 3, rcBox.bottom);
		LineTo(hdc, rcBox.right - 3, rcBox.bottom);
		LineTo(hdc, rcBox.right, rcBox.bottom - 3);
		LineTo(hdc, rcBox.right, rcBox.top + 3);
		LineTo(hdc, rcBox.right - 3, rcBox.top);
		LineTo(hdc, rcBox.left, rcBox.top);

		SelectObject(hdc, oldpen);
		DeleteObject(brpen);
		brpen = CreatePen(PS_SOLID, 0, bbg);
		oldpen = SelectObject(hdc, brpen);

		MoveToEx(hdc, rcBox.left, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.left, rcBox.bottom - 3);
		MoveToEx(hdc, rcBox.left + 1, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.left + 1, rcBox.bottom - 2);
		MoveToEx(hdc, rcBox.left + 2, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.left + 2, rcBox.bottom - 1);

		MoveToEx(hdc, rcBox.right, rcBox.top, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right, rcBox.top + 3);
		MoveToEx(hdc, rcBox.right - 1, rcBox.top, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right - 1, rcBox.top + 2);
		MoveToEx(hdc, rcBox.right - 2, rcBox.top, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right - 2, rcBox.top + 1);

		MoveToEx(hdc, rcBox.right, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right, rcBox.bottom - 3);
		MoveToEx(hdc, rcBox.right - 1, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right - 1, rcBox.bottom - 2);
		MoveToEx(hdc, rcBox.right - 2, rcBox.bottom, (LPPOINT)NULL);
		LineTo(hdc, rcBox.right - 2, rcBox.bottom - 1);

		SelectObject(hdc, oldpen);

		DeleteObject(brpen);
	}


	if (pbox->img)
	{
		if ((pbox->flags & BOX_FITASPECTIMG) == BOX_FITASPECTIMG)
		{
			int sx, sy, w, h;

			/* Try fitting horizontally first */
			w = rcBox.right - rcBox.left + 1;
			sx = rcBox.left;
			h = w * pbox->img->h / pbox->img->w;
			sy = rcBox.top + (rcBox.bottom - rcBox.top + 1 - h) /2;

			/* If we're too tall, fit vertically */

			if (h > rcBox.bottom - rcBox.top + 1)
			{
				h = rcBox.bottom - rcBox.top + 1;
				sy = rcBox.top;
				w = h * pbox->img->w / pbox->img->h;
				sx = rcBox.left + (rcBox.right - rcBox.left + 1 - w) / 2;
			}

			if (pbox->img->hbmpmask)
			{
				Box_Blend(hdc, sx, sy, w, h, pbox->img, pbox->img->x, pbox->img->y, pbox->img->w, pbox->img->h, paintmask);
			}
			else
			{
				if (!paintmask)
				{
					HDC hdcsrc = CreateCompatibleDC(hdc);
					HBITMAP oldbmp = SelectObject(hdcsrc, pbox->img->hbmp);

					StretchBlt(hdc, sx, sy, w, h, hdcsrc, pbox->img->x, pbox->img->y, pbox->img->w, pbox->img->h, SRCCOPY);

					SelectObject(hdcsrc, oldbmp);

					DeleteDC(hdcsrc);
				}
				else
				{
					BitBlt(hdc, rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1, NULL, 0, 0, BLACKNESS);
				}
			}
		}
		else if (pbox->flags & BOX_CENTERIMG)
		{
			int sx, sy, w, h;
			RECT rcSub, rcSubBox;

			w = pbox->img->w;
			h = pbox->img->h;

			rcSubBox.left   = rcBox.left + (rcBox.right - rcBox.left + 1 - w) / 2;
			rcSubBox.top    = rcBox.top  + (rcBox.bottom - rcBox.top + 1 - h) / 2;
			rcSubBox.right  = rcSubBox.left + w - 1;
			rcSubBox.bottom = rcSubBox.top  + h - 1;

			rcSub = cliprects(rcSubBox, rc);

			sx = rcSub.left - rcSubBox.left + pbox->img->x;
			sy = rcSub.top  - rcSubBox.top  + pbox->img->y;

			if (pbox->img->hbmpmask)
			{
				Box_Blend(hdc, rcSub.left, rcSub.top, rcSub.right - rcSub.left + 1, rcSub.bottom - rcSub.top + 1,
					  pbox->img, sx, sy, rcSub.right - rcSub.left + 1, rcSub.bottom - rcSub.top + 1, paintmask);
			}
			else
			{
				if (!paintmask)
				{
					HDC hdcsrc = CreateCompatibleDC(hdc);
					HBITMAP oldbmp = SelectObject(hdcsrc, pbox->img->hbmp);

					BitBlt(hdc, rcSub.left, rcSub.top, rcSub.right - rcSub.left + 1, rcSub.bottom - rcSub.top + 1, hdcsrc, sx, sy, SRCCOPY);

					SelectObject(hdcsrc, oldbmp);

					DeleteDC(hdcsrc);
				}
				else
				{
					BitBlt(hdc, rcSub.left, rcSub.top, rcSub.right - rcSub.left + 1, rcSub.bottom - rcSub.top + 1, NULL, 0, 0, BLACKNESS);
				}
			}
		}
		else if (pbox->flags & BOX_STRETCHIMG)
		{
			int sx, sy, w, h;

			sx = pbox->img->x + (rc.left - rcBox.left) * pbox->img->w / (rcBox.right - rcBox.left + 1);
			sy = pbox->img->y + (rc.top  - rcBox.top)  * pbox->img->h / (rcBox.bottom - rcBox.top + 1);
			w = (rc.right - rc.left) * pbox->img->w / (rcBox.right - rcBox.left + 1);
			h = (rc.bottom - rc.top) * pbox->img->h / (rcBox.bottom - rcBox.top + 1);

			if (pbox->img->hbmpmask)
			{
				Box_Blend(hdc, rcBox.left, rcBox.top, rcBox.right - rcBox.left + 1, rcBox.bottom - rcBox.top + 1,
				          pbox->img, pbox->img->x, pbox->img->y, pbox->img->w, pbox->img->h, paintmask);
			}
			else
			{
				if (!paintmask)
				{
					HDC hdcsrc = CreateCompatibleDC(hdc);
					HBITMAP oldbmp = SelectObject(hdcsrc, pbox->img->hbmp);

					sx = pbox->img->x + (rc.left - rcBox.left) * pbox->img->w / (rcBox.right - rcBox.left + 1);
					sy = pbox->img->y + (rc.top  - rcBox.top)  * pbox->img->h / (rcBox.bottom - rcBox.top + 1);
					w = (rc.right - rc.left + 1) * pbox->img->w / (rcBox.right - rcBox.left + 1);
					h = (rc.bottom - rc.top + 1) * pbox->img->h / (rcBox.bottom - rcBox.top + 1);
					StretchBlt(hdc, rcBox.left, rcBox.top, rcBox.right - rcBox.left + 1, rcBox.bottom - rcBox.top + 1,
					           hdcsrc, pbox->img->x, pbox->img->y, pbox->img->w, pbox->img->h, SRCCOPY);

					SelectObject(hdcsrc, oldbmp);

					DeleteDC(hdcsrc);
				}
				else
				{
					BitBlt(hdc, rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1, NULL, 0, 0, BLACKNESS);
				}
			}
		}
		else
		{
			int sx, sy, w, h, hreps, vreps, nx, ny;

			hreps = vreps = 1;

			if (pbox->flags & BOX_TILEIMAGE)
			{
				hreps = (rcBox.right - rcBox.left + pbox->img->w) / pbox->img->w;
				vreps = (rcBox.bottom - rcBox.top + pbox->img->h) / pbox->img->h;
			}

			if (pbox->img->hbmpmask)
			{
				HDC hdcsrc = CreateCompatibleDC(hdc);
				HBITMAP oldbmp = SelectObject(hdcsrc, pbox->img->hbmp);

				RECT rcSubBox;

				w = pbox->img->w;
				h = pbox->img->h;

				if (W32_AlphaBlend && !paintmask)
				{
					for (nx = 0; nx < hreps; nx++)
					{
						for (ny = 0; ny < vreps; ny++)
						{
							RECT rcSub;

							rcSubBox.left   = rcBox.left + nx * w;
							rcSubBox.top    = rcBox.top  + ny * h;
							rcSubBox.right  = rcSubBox.left + w - 1;
							rcSubBox.bottom = rcSubBox.top  + h - 1;

							rcSub = cliprects(rcSubBox, rc);

							sx = rcSub.left - rcSubBox.left + pbox->img->x;
							sy = rcSub.top - rcSubBox.top + pbox->img->y;

							W32_AlphaBlend(hdc, rcSub.left, rcSub.top, rcSub.right - rcSub.left + 1, rcSub.bottom - rcSub.top + 1,
							               hdcsrc, sx, sy, rcSub.right - rcSub.left + 1, rcSub.bottom - rcSub.top + 1, bf);
						}
					}
				}
				else
				{
					/* Draw by XORing image, ANDing mask, and XORing image again */

					if (!paintmask)
					{
						for (nx = 0; nx < hreps; nx++)
						{
							for (ny = 0; ny < vreps; ny++)
							{
								RECT rcSub;

								rcSubBox.left   = rcBox.left + nx * w;
								rcSubBox.top    = rcBox.top  + ny * h;
								rcSubBox.right  = rcSubBox.left + w - 1;
								rcSubBox.bottom = rcSubBox.top  + h - 1;

								rcSub = cliprects(rcSubBox, rc);

								sx = rcSub.left - rcSubBox.left + pbox->img->x;
								sy = rcSub.top - rcSubBox.top + pbox->img->y;

								BitBlt(hdc, rcSub.left, rcSub.top, rcSub.right - rcSub.left + 1, rcSub.bottom - rcSub.top + 1, hdcsrc, sx, sy, SRCINVERT);
							}
						}
					}

					SelectObject(hdcsrc, pbox->img->hbmpmask);

					for (nx = 0; nx < hreps; nx++)
					{
						for (ny = 0; ny < vreps; ny++)
						{
							RECT rcSub;

							rcSubBox.left   = rcBox.left + nx * w;
							rcSubBox.top    = rcBox.top  + ny * h;
							rcSubBox.right  = rcSubBox.left + w - 1;
							rcSubBox.bottom = rcSubBox.top  + h - 1;
	
							rcSub = cliprects(rcSubBox, rc);
	
							sx = rcSub.left - rcSubBox.left + pbox->img->x;
							sy = rcSub.top - rcSubBox.top + pbox->img->y;

							BitBlt(hdc, rcSub.left, rcSub.top, rcSub.right - rcSub.left + 1, rcSub.bottom - rcSub.top + 1, hdcsrc, sx, sy, SRCAND);
						}
					}

					if (!paintmask)
					{
						SelectObject(hdcsrc, pbox->img->hbmp);

						for (nx = 0; nx < hreps; nx++)
						{
							for (ny = 0; ny < vreps; ny++)
							{
								RECT rcSub;
	
								rcSubBox.left   = rcBox.left + nx * w;
								rcSubBox.top    = rcBox.top  + ny * h;
								rcSubBox.right  = rcSubBox.left + w - 1;
								rcSubBox.bottom = rcSubBox.top  + h - 1;

								rcSub = cliprects(rcSubBox, rc);

								sx = rcSub.left - rcSubBox.left + pbox->img->x;
								sy = rcSub.top - rcSubBox.top + pbox->img->y;

								BitBlt(hdc, rcSub.left, rcSub.top, rcSub.right - rcSub.left + 1, rcSub.bottom - rcSub.top + 1, hdcsrc, sx, sy, SRCINVERT);
							}
						}
					}
				}

				SelectObject(hdcsrc, oldbmp);
				DeleteDC(hdcsrc);
			}
			else
			{
				if (!paintmask)
				{
					HDC hdcsrc = CreateCompatibleDC(hdc);
					HBITMAP oldbmp = SelectObject(hdcsrc, pbox->img->hbmp);

					w = pbox->img->w;
					h = pbox->img->h;

					for (nx = 0; nx < hreps; nx++) {
						for (ny = 0; ny < vreps; ny++) {
							RECT rcSub, rcSubBox;

							rcSubBox.left   = rcBox.left + nx * w;
							rcSubBox.top    = rcBox.top  + ny * h;
							rcSubBox.right  = rcSubBox.left + w - 1;
							rcSubBox.bottom = rcSubBox.top  + h - 1;

							rcSub = cliprects(rcSubBox, rc);

							sx = rcSub.left - rcSubBox.left + pbox->img->x;
							sy = rcSub.top - rcSubBox.top + pbox->img->y;

							BitBlt(hdc, rcSub.left, rcSub.top, rcSub.right - rcSub.left + 1, rcSub.bottom - rcSub.top + 1, hdcsrc, sx, sy, SRCCOPY);
						}
					}

					SelectObject(hdcsrc, oldbmp);
					DeleteDC(hdcsrc);
				}
				else
				{
					BitBlt(hdc, rcBox.left, rcBox.top, rcBox.right - rcBox.left + 1, rcBox.bottom - rcBox.top + 1, NULL, 0, 0, BLACKNESS);
				}
			}
		}
	}

	if (pbox->text)
	{
		COLORREF oldfgcol;
		int oldbkmode;
		HFONT oldfont;
		RECT draw = rcBox;
		WCHAR *widetext;

		widetext = Util_ConvertUTF8ToWCHAR(pbox->text);

		if (widetext)
		{
			oldbkmode = SetBkMode(hdc, TRANSPARENT);
			if (!paintmask)
			{
				oldfgcol = SetTextColor(hdc, pbox->fgcol);
			}
			else
			{
				oldfgcol = SetTextColor(hdc, RGB(0, 0, 0));
			}
		
			if (pbox->font)
			{
				oldfont = SelectObject(hdc, pbox->font);
			}
			else
			{
				oldfont = SelectObject(hdc, tahoma11_f);
			}

			if (pbox->flags & BOX_CENTERTEXT)
			{
				RECT measure = rcBox;

				if (Util_OldWinVer())
				{
					char *temptext = Util_ConvertWCHARToANSI(widetext);

					DrawTextA(hdc, temptext, (UINT)strlen(temptext), &measure, DT_CALCRECT | DT_NOPREFIX);
					
					free(temptext);
				}
				else
				{
					DrawTextW(hdc, widetext, (UINT)wcslen(widetext), &measure, DT_CALCRECT | DT_NOPREFIX);
				}

				draw.left   = rcBox.left + ((rcBox.right - rcBox.left) - (measure.right - measure.left)) / 2;
				draw.top    = rcBox.top  + ((rcBox.bottom - rcBox.top) - (measure.bottom - measure.top)) / 2;
				draw.right  = draw.left + measure.right - measure.left + 1;
				draw.bottom = draw.top  + measure.bottom - measure.top + 1;
			}

			if (pbox->flags & BOX_RIGHTTEXT)
			{
				RECT measure = rcBox;

				if (Util_OldWinVer())
				{
					char *temptext = Util_ConvertWCHARToANSI(widetext);

					DrawTextA(hdc, temptext, (UINT)strlen(temptext), &measure, DT_CALCRECT | DT_NOPREFIX);
					
					free(temptext);
				}
				else
				{
					DrawTextW(hdc, widetext, (UINT)wcslen(widetext), &measure, DT_CALCRECT | DT_NOPREFIX);
				}

				draw.left   = rcBox.right - (measure.right - measure.left);
				draw.top    = rcBox.top;
				draw.right  = draw.left + measure.right - measure.left + 1;
				draw.bottom = draw.top  + measure.bottom - measure.top + 1;
			}
		
			{
				RECT rc2;

				rc2.top = rc.top;
				rc2.left = rc.left;
				rc2.right = rc.right + 1;
				rc2.bottom = rc.bottom + 1;

				ExtTextOutW(hdc, draw.left, draw.top, ETO_CLIPPED, &rc2, widetext, (UINT)wcslen(widetext), NULL);
			}

			SelectObject(hdc, oldfont);
		
			SetBkMode(hdc, oldbkmode);
			SetTextColor(hdc, oldfgcol);

			free(widetext);

		}
		else
		{
			Log_Write(0, "Error widening text: %d\n", GetLastError());
		}
	}
	
	SelectClipRgn(hdc, NULL);
	DeleteObject(cliprgn);

	while (child)
	{
		if (child->OnPaint)
		{
			child->OnPaint(child, hdc, rc, child->x + rcBox.left - pbox->xoffset, child->y + rcBox.top - pbox->yoffset, paintmask);
		}
		child = child->sibling;
	}

	if (((pbox->flags & BOX_BORDERALL) == BOX_BORDER2) || ((pbox->flags & BOX_BORDERALL) == BOX_BORDER3))
	{
		HPEN brpen, oldpen;

		if (!paintmask)
		{
			brpen = CreatePen(PS_SOLID, 0, pbox->brcol);
		}
		else
		{
			brpen = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
		}
	
		oldpen = SelectObject(hdc, brpen);

		/* left */
		if (rc.left <= rcBox.left)
		{
			MoveToEx(hdc, rcBox.left, rc.top, (LPPOINT)NULL); 
			LineTo(hdc, rcBox.left, rc.bottom);
		}

		/* top */
		if (rc.top <= rcBox.top)
		{
			MoveToEx(hdc, rc.left, rcBox.top, (LPPOINT)NULL); 
			LineTo(hdc, rc.right, rcBox.top);
		}

		/* right */
		if ((rc.right >= rcBox.right) && !((pbox->flags & BOX_BORDERALL) == BOX_BORDER3))
		{
			MoveToEx(hdc, rcBox.right, rc.top, (LPPOINT)NULL); 
			LineTo(hdc, rcBox.right, rc.bottom);
		}

		/* bottom */
		if (rc.bottom >= rcBox.bottom)
		{
			MoveToEx(hdc, rc.left, rcBox.bottom, (LPPOINT)NULL); 
			LineTo(hdc, rc.right, rcBox.bottom);
		}

		if ((pbox->flags & BOX_BORDERALL) == BOX_BORDER2)
		{
			Arc(hdc, rcBox.left, rcBox.top, rcBox.left + 15, rcBox.top + 15,
				rcBox.left + 7, rcBox.top, rcBox.left, rcBox.top + 7);
			Arc(hdc, rcBox.right - 14, rcBox.top, rcBox.right + 1, rcBox.top + 15,
				rcBox.right + 1, rcBox.top + 7, rcBox.right - 6, rcBox.top);
		}

		if ((pbox->flags & BOX_BORDERALL) == BOX_BORDER3)
		{
			Arc(hdc, rcBox.left, rcBox.top, rcBox.left + 15, rcBox.top + 15,
				rcBox.left + 7, rcBox.top, rcBox.left, rcBox.top + 7);
			Arc(hdc, rcBox.left, rcBox.bottom - 14, rcBox.left + 15, rcBox.bottom + 1,
				rcBox.left, rcBox.bottom - 6, rcBox.left + 7, rcBox.bottom + 1);
		}

		SelectObject(hdc, oldpen);

		DeleteObject(brpen);
	}
}



#undef MIN3
#undef MIN
#undef MAX

#if 0
void Box_Draw_Hbmp_Part(struct Box_s *pbox, HBITMAP hbmp, int x, int y, int w, int h)
{
	HDC hdc, hdcdraw, hdcold;
	RECT rc;
	
	if (!pbox->OnPaint)
	{
		return NULL;
	}

	if (!(hdc = GetDC(NULL)))
	{
		Log_Write(0, "CRITICAL FAILURE: Box_Draw_Hbmp_Part(): failed GetDC()\n");
	}

	if (!(hdcdraw = CreateCompatibleDC(hdc)))
	{
		Log_Write(0, "CRITICAL FAILURE: Box_Draw_Hbmp_Part(): failed CreateCompatibleDC()\n");
	}
/*
	if (!(hbmp = CreateCompatibleBitmap(hdc, pbox->w, pbox->h)))
	{
		Log_Write(0, "CRITICAL FAILURE: Box_Draw_Hbmp_Part(): failed CreateCompatibleBitmap()\n");
	}
*/
	ReleaseDC(NULL, hdc);

	
	rc.left   = x;
	rc.top    = y;
	rc.right  = x + w;
	rc.bottom = y + h;
	
	hdcold = SelectObject(hdcdraw, hbmp);

	pbox->OnPaint(pbox, hdcdraw, rc, 0, 0, 0);
	
	SelectObject(hdcdraw, hdcold);
	DeleteDC(hdcdraw);

	return hbmp;
}
#endif

/***************************************************************************
 * Box_Draw_Hbmp()
 *
 * Creates a HBITMAP with the size of "pbox" and draws "pbox" and all its 
 * subboxes on it. If "mask" is TRUE, then the resulting hbitmap is a mask.
 *
 * Uses Box_Draw().
 * 
 ***************************************************************************/
HBITMAP Box_Draw_Hbmp(struct Box_s *pbox, BOOL mask)
{
	HDC hdc, hdcdraw, hdcold;
	HBITMAP hbmp;
	RECT rc;
	
	if (!pbox->OnPaint)
	{
		return NULL;
	}

	hdc = GetDC(NULL);
	hdcdraw = CreateCompatibleDC(hdc);
	hbmp = CreateCompatibleBitmap(hdc, pbox->w, pbox->h);
	ReleaseDC(NULL, hdc);

	
	rc.left   = 0;
	rc.top    = 0;
	rc.right  = pbox->w - 1;
	rc.bottom = pbox->h - 1;
	
	hdcold = SelectObject(hdcdraw, hbmp);

	if (mask)
	{
		HBRUSH bgbrush, oldbrush;
		HPEN bgpen, oldpen;

		bgbrush = CreateSolidBrush(RGB(255, 255, 255));
		bgpen   = CreatePen(PS_SOLID, 0, RGB(255, 255, 255));

		oldbrush = SelectObject(hdcdraw, bgbrush);
		oldpen   = SelectObject(hdcdraw, bgpen);

		Rectangle(hdcdraw, 0, 0, pbox->w, pbox->h);

		SelectObject(hdcdraw, oldbrush);
		SelectObject(hdcdraw, oldpen);

		DeleteObject(bgbrush);
		DeleteObject(bgpen);
	
		Box_OnPaint(pbox, hdcdraw, rc, 0, 0, 1);
	}
	else
	{
		pbox->OnPaint(pbox, hdcdraw, rc, 0, 0, 0);
	}
	
	SelectObject(hdcdraw, hdcold);
	DeleteDC(hdcdraw);

	return hbmp;
}


/***************************************************************************
 * Box_GetScreenCoords()
 *
 * Gets the screen coordinates of "pbox" and assigns them to "x", "y".  
 * Uses the fact that the root box contains coordinates relative to the top
 * left of the screen.
 *
 * BUG: Does not work if the root box is not in its own window.
 * 
 ***************************************************************************/
void Box_GetScreenCoords(struct Box_s *pbox, int *x, int *y)
{
	*x = pbox->x;
	*y = pbox->y;
	pbox = pbox->parent;
	while (pbox)
	{
		*x += pbox->x - pbox->xoffset;
		*y += pbox->y - pbox->yoffset;
		pbox = pbox->parent;
	}
}


/***************************************************************************
 * Box_GetRootCoords()
 *
 * Gets the coordinates of "pbox" relative to its root box.
 * 
 ***************************************************************************/
void Box_GetRootCoords(struct Box_s *pbox, int *x, int *y)
{
	*x = 0;
	*y = 0;
	if (pbox->parent) {
		*x = pbox->x;
		*y = pbox->y;
	}
	pbox = pbox->parent;
	while (pbox)
	{
		if (pbox->parent) {
			*x += pbox->x;
			*y += pbox->y;
		}
		*x -= pbox->xoffset;
		*y -= pbox->yoffset;
		pbox = pbox->parent;
	}
}


/***************************************************************************
 * Box_Repaint()
 *
 * Schedules a repaint for "pbox" in the windows messaging queue.
 * 
 ***************************************************************************/
void Box_Repaint(struct Box_s *pbox)
{
	RECT rc;

	Box_GetRootCoords(pbox, &rc.left, &rc.top);
	rc.right  = rc.left + pbox->w;
	rc.bottom = rc.top  + pbox->h;

	/* Invalidate this rect */
	if (pbox->hwnd)
	{
		InvalidateRect(pbox->hwnd, &rc, FALSE);
	}
}


/***************************************************************************
 * Box_GetRoot()
 *
 * Returns the root of the tree of which "pbox" is a child.
 * 
 ***************************************************************************/
struct Box_s *Box_GetRoot(struct Box_s *pbox)
{
	while (pbox->parent)
	{
		pbox = pbox->parent;
	}
	return pbox;
}


/***************************************************************************
 * Box_OnMouseMove()
 *
 * Default message handler.
 * Passes down all mouse movement messages "xmouse", "ymouse" to all
 * subboxes of "pbox"
 * 
 ***************************************************************************/
void Box_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *child = pbox->child;

	while(child)
	{
		if (child->OnMouseMove /* && (child->flags & BOX_VISIBLE)*/)
		{
			child->OnMouseMove(child, xmouse - child->x + pbox->xoffset, ymouse - child->y + pbox->yoffset);
		}
		child = child->sibling;
	}
}


/***************************************************************************
 * Box_OnLButtonDblClk()
 *
 * Default message handler.
 * Passes down all left mouse button down at "xmouse", "ymouse" messages to
 * all subboxes of "pbox" that have xmouse and ymouse within their area.
 * 
 ***************************************************************************/
void Box_OnLButtonDblClk(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *child = pbox->child;

	while(child)
	{
		if (child->OnLButtonDblClk && (child->flags & BOX_VISIBLE))
		{
			int x = xmouse - child->x + pbox->xoffset;
			int y = ymouse - child->y + pbox->yoffset;
			if (x >= 0 && y >= 0 && x < child->w && y < child->h)
			{
				child->OnLButtonDblClk(child, x, y);
			}
		}
		child = child->sibling;
	}
}


void Box_OnLButtonTrpClk(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *child = pbox->child;

	while(child)
	{
		if (child->OnLButtonDblClk && (child->flags & BOX_VISIBLE))
		{
			int x = xmouse - child->x + pbox->xoffset;
			int y = ymouse - child->y + pbox->yoffset;
			if (x >= 0 && y >= 0 && x < child->w && y < child->h)
			{
				child->OnLButtonTrpClk(child, x, y);
			}
		}
		child = child->sibling;
	}
}

/***************************************************************************
 * Box_OnLButtonDown()
 *
 * Default message handler.
 * Passes down all left mouse button down at "xmouse", "ymouse" messages to
 * all subboxes of "pbox" that have xmouse and ymouse within their area.
 * 
 ***************************************************************************/
void Box_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *child = pbox->child;

	if (pbox->tooltipbox)
	{
		Box_Destroy(pbox->tooltipbox);
		pbox->tooltipbox = NULL;
	}

	while(child)
	{
		if (child->OnLButtonDown && (child->flags & BOX_VISIBLE))
		{
			int x = xmouse - child->x + pbox->xoffset;
			int y = ymouse - child->y + pbox->yoffset;
			if (x >= 0 && y >= 0 && x < child->w && y < child->h)
			{
				child->OnLButtonDown(child, x, y);
			}
		}
		child = child->sibling;
	}
}


/***************************************************************************
 * Box_OnLButtonUp()
 *
 * Default message handler.
 * Passes down all left mouse button up at "xmouse", "ymouse" messages to
 * all subboxes of "pbox"
 * 
 ***************************************************************************/
void Box_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *child = pbox->child;

	while(child)
	{
		if (child->OnLButtonUp && (child->flags & BOX_VISIBLE))
		{
			child->OnLButtonUp(child, xmouse - child->x + pbox->xoffset, ymouse - child->y + pbox->yoffset);
		}
		child = child->sibling;
	}
}


/***************************************************************************
 * Box_OnRButtonDown()
 *
 * Default message handler.
 * Passes down all right mouse button down at "xmouse", "ymouse" messages to
 * all subboxes of "pbox" that have xmouse and ymouse within their area.
 * 
 ***************************************************************************/
void Box_OnRButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *child = pbox->child;

	if (pbox->tooltipbox)
	{
		Box_Destroy(pbox->tooltipbox);
		pbox->tooltipbox = NULL;
	}

	while(child)
	{
		if (child->OnRButtonDown && (child->flags & BOX_VISIBLE))
		{
			int x = xmouse - child->x + pbox->xoffset;
			int y = ymouse - child->y + pbox->yoffset;
			if (x >= 0 && y >= 0 && x < child->w && y < child->h)
			{
				child->OnRButtonDown(child, x, y);
			}
		}
		child = child->sibling;
	}
}


/***************************************************************************
 * Box_OnRButtonUp()
 *
 * Default message handler.
 * Passes down all right mouse button up at "xmouse", "ymouse" messages to
 * all subboxes of "pbox"
 * 
 ***************************************************************************/
void Box_OnRButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *child = pbox->child;

	while(child)
	{
		if (child->OnRButtonUp && (child->flags & BOX_VISIBLE))
		{
			child->OnRButtonUp(child, xmouse - child->x + pbox->xoffset, ymouse - child->y + pbox->yoffset);
		}
		child = child->sibling;
	}
}


/***************************************************************************
 * Box_OnMove()
 *
 * Default message handler for root (windowed) boxes.
 * Changes "pbox"'s coordinates to fit where it has been moved.
 * 
 ***************************************************************************/
void Box_OnMove(struct Box_s *pbox, int x, int y)
{
	pbox->x = x;
	pbox->y = y;
}


void Box_OnScrollWheel(struct Box_s *pbox, float delta)
{
	struct Box_s *child = pbox->child;

	while(child)
	{
		if (child->OnScrollWheel)
		{
			child->OnScrollWheel(child, delta);
		}
		child = child->sibling;
	}
}


/***************************************************************************
 * Box_OnSizeWidth_Stretch()
 *
 * Default message handler for root (windowed) boxes.
 * Changes "pbox"'s width to fit its new size, and passes on the size
 * message to all subboxes.  "dwidth" is a size delta.
 *
 ***************************************************************************/
void Box_OnSizeWidth_Stretch(struct Box_s *pbox, int dwidth)
{
	struct Box_s *child;

	if (dwidth > 0 && pbox->xoffset > 0)
	{
		pbox->xoffset -= dwidth;
		if (pbox->xoffset < 0)
		{
			pbox->xoffset = 0;
		}
	}

	pbox->w += dwidth;

	child = pbox->child;
	while(child)
	{
		if (child->OnSizeWidth)
		{
			child->OnSizeWidth(child, dwidth);
		}
		child = child->sibling;
	}
}


/***************************************************************************
 * Box_OnSizeHeight_Stretch()
 *
 * Default message handler for root (windowed) boxes.
 * Changes "pbox"'s height to fit its new size, and passes on the size
 * message to all subboxes.  Adjusts yoffset if box grows beyond the visible
 * bottom and there is still some yoffset to give.
 * "dwidth" is a size delta.
 *
 ***************************************************************************/
void Box_OnSizeHeight_Stretch(struct Box_s *pbox, int dheight)
{
	struct Box_s *child;

	if (dheight > 0 && pbox->yoffset > 0)
	{
		pbox->yoffset -= dheight;
		if (pbox->yoffset < 0)
		{
			pbox->yoffset = 0;
		}
	}

	pbox->h += dheight;

	child = pbox->child;
	while(child)
	{
		if (child->OnSizeHeight)
		{
			child->OnSizeHeight(child, dheight);
		}
		child = child->sibling;
	}
}


/***************************************************************************
 * Box_OnSizeWidth_StickRight()
 *
 * Message handler.
 * Changes "pbox"'s coordinates to fit its parent box's new size.  "pbox"
 * is moved relative to the right side of the window.  "dwidth" is a size
 * delta.
 *
 ***************************************************************************/
void Box_OnSizeWidth_StickRight(struct Box_s *pbox, int dwidth)
{
	pbox->x += dwidth;
}


/***************************************************************************
 * Box_OnSizeHeight_StickBottom()
 *
 * Message handler.
 * Changes "pbox"'s coordinates to fit its parent box's new size.  "pbox"
 * is moved relative to the bottom of the window.  "dheight" is a size
 * delta.
 *
 ***************************************************************************/
void Box_OnSizeHeight_StickBottom(struct Box_s *pbox, int dheight)
{
	pbox->y += dheight;
}

void Box_OnSizeWidth_Center(struct Box_s *pbox, int dwidth)
{
	if (!pbox->parent)
	{
		return;
	}

	pbox->x = (pbox->parent->w - pbox->w) / 2;
}

void Box_OnSizeHeight_Center(struct Box_s *pbox, int dheight)
{
	if (!pbox->parent)
	{
		return;
	}

	pbox->y = (pbox->parent->h - pbox->h) / 2;
}

/***************************************************************************
 * Box_OnClose_CloseApp()
 *
 * Default message handler for root (windowed) boxes with no parent window.
 * Posts a quit message if this window is closed.
 *
 ***************************************************************************/
void Box_OnClose_CloseApp(struct Box_s *pbox)
{
	PostQuitMessage(0);	// Send A Quit Message
}

void Box_OnClose(struct Box_s *pbox)
{
	Log_Write(0, "Box_OnClose %d\n", pbox);
	Box_Destroy(pbox);
}


void Box_OnMinimize(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_MINIMIZE);
}

void Box_OnRestore(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_RESTORE);
}

int Box_OnDragDrop(struct Box_s *pdst, struct Box_s *psrc, int xmouse, int ymouse, int id, void *data)
{
	struct Box_s *child = pdst->child;

	while(child)
	{
		if (child->OnDragDrop && (child->flags & BOX_VISIBLE))
		{
			int x = xmouse - child->x + pdst->xoffset;
			int y = ymouse - child->y + pdst->yoffset;

			if (x >= 0 && y >= 0 && x < child->w && y < child->h)
			{
				int result = child->OnDragDrop(child, psrc, x, y, id, data);
				if (result)
				{
					return 1;
				}
			}
		}
		child = child->sibling;
	}

	return 0;
}

void Box_HandleDragDrop(struct Box_s *psrc, int xmouse, int ymouse, int id, void *data, void (*ondropempty)(struct Box_s *psrc, int xmouse, int ymouse, int id, void *data))
{
	POINT pt;
	HWND hwnd;
	WINDOWINFO wi;
	int result = 0;

	pt.x = xmouse;
	pt.y = ymouse;

	hwnd = WindowFromPoint(pt);

	if (!hwnd)
	{
		if (ondropempty)
		{
			ondropempty(psrc, xmouse, ymouse, id, data);
		}
		return;
	}

	memset(&wi, 0, sizeof(wi));

	wi.cbSize = sizeof(wi);

	GetWindowInfo(hwnd, &wi);

	if (wi.atomWindowType == boxwndatom || wi.atomWindowType == boxmenuatom)
	{
		struct Box_s *pdst = (struct Box_s *)GetWindowLong(hwnd, GWL_USERDATA);

		if (pdst->OnDragDrop)
		{
			xmouse -= pdst->x;
			ymouse -= pdst->y;
			result = pdst->OnDragDrop(pdst, psrc, xmouse, ymouse, id, data);
		}
	}

	if (!result)
	{
		if (ondropempty)
		{
			ondropempty(psrc, xmouse, ymouse, id, data);
		}
	}
}


/*
static struct Box_s *cursorimglockbox = NULL;
static HCURSOR *cursorimg = NULL;
*/

BOOL Box_LockMouseCursorImg(struct Box_s *pbox, HCURSOR cursor)
{
	struct Box_s *rootbox = Box_GetRoot(pbox);
/*
	if (!rootbox->cursorimg)
	{
		rootbox->cursorimg = cursor;
		SetCursor(cursor);
		rootbox->cursorimglockbox = pbox;
		return TRUE;
	}

	if (rootbox->cursorimglockbox == pbox)
	{
		return TRUE;
	}

	return FALSE;
*/
	pbox->cursorimg = cursor;
	return 1;
}

BOOL Box_UnlockMouseCursorImg(struct Box_s *pbox)
{
	struct Box_s *rootbox = Box_GetRoot(pbox);
/*
	if (rootbox->cursorimg && pbox == rootbox->cursorimglockbox)
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		rootbox->cursorimg = NULL;
		rootbox->cursorimglockbox = NULL;
		return TRUE;
	}

	return FALSE;
*/
	pbox->cursorimg = NULL;
	return 1;
}

int Box_SetCursorImg(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *child = pbox->child;
	struct Box_s *root = Box_GetRoot(pbox);

	while(child)
	{
		if ((child->flags & BOX_VISIBLE))
		{
			int x = xmouse - child->x + pbox->xoffset;
			int y = ymouse - child->y + pbox->yoffset;
			if (x >= 0 && y >= 0 && x <= child->w && y <= child->h)
			{
				if (Box_SetCursorImg(child, x, y))
				{
					return 1;
				}
			}
		}
		child = child->sibling;
	}

	if (root == pbox)
	{
		if (root->cursorimg != NULL)
		{
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			root->cursorimg = NULL;
		}
		return 0;
	}

	if (pbox->cursorimg)
	{
		if (root->cursorimg != pbox->cursorimg)
		{
			SetCursor(pbox->cursorimg);
			root->cursorimg = pbox->cursorimg;
		}
		return 1;
	}

	return 0;
}

/***************************************************************************
 * BoxWndProc()
 *
 * Windows message handler.  Determines the box associated with this window
 * and passes all messages to the relevant message handler functions.
 *
 ***************************************************************************/
static LRESULT CALLBACK BoxWndProc(HWND	hWnd,		// Handle For This Window
			 	UINT	uMsg,		// Message For This Window
			 	WPARAM	wParam,		// Additional Message Information
			 	LPARAM	lParam)		// Additional Message Information
{
	struct Box_s *pbox = (struct Box_s *)GetWindowLong(hWnd, GWL_USERDATA);

	/* If there's no pbox assigned, return the default window handler.
	 * This (usually) only happens if the window is drawn before the pbox is
	 * assigned. */
	if (!pbox)
	{
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}

	if (Box_GetRoot(pbox) == deadboxes)
	{
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}

	/*Log_Write(0, "BoxWndProc(%d, %d, %d, %d)\n", hWnd, uMsg, wParam, lParam);*/

	switch (uMsg)
	{
		case WM_PAINT:
		{
			HDC hdc;
			RECT rc;

			GetUpdateRect(hWnd, &rc, FALSE);

			ValidateRect(hWnd, NULL);

			if (dragging)
			{
				ImageList_DragShowNolock(FALSE);
			}

			if (!(hdc = GetDC(hWnd)))
			{
				Log_Write(0, "CRITICAL FAILURE: BoxWndProc(): failed GetDC()\n");
			}

			if (!paintoverdc)
			{
				if (!(paintoverdc = CreateCompatibleDC(hdc)))
				{
					Log_Write(0, "CRITICAL FAILURE: BoxWndProc(): failed CreateCompatibleDC()\n");
				}
			}

			if (!paintoverbmp)
			{
				if (!(paintoverbmp = CreateCompatibleBitmap(hdc, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN))))
				{
					Log_Write(0, "CRITICAL FAILURE: BoxWndProc(): failed CreateCompatibeBitmap()\n");
				}

				SelectObject(paintoverdc, paintoverbmp);
			}

			if (pbox->OnPaint)
			{
				pbox->OnPaint(pbox, paintoverdc, rc, 0, 0, 0);
			}

			BitBlt(hdc, rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1, paintoverdc, rc.left, rc.top, SRCCOPY);

			ReleaseDC(hWnd, hdc);

			if (dragging)
			{
				ImageList_DragShowNolock(TRUE);
			}

			return 0;
		}
		break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			unsigned int vk = (UINT)wParam;

			if (lockedbox && Box_GetRoot(pbox) != Box_GetRoot(lockedbox))
			{
				return 0;
			}

			if (pbox->parentactive)
			{
				pbox = pbox->parentactive;
			}

			{
				char keystate[256];
                                
				GetKeyboardState(keystate);

				if (vk == 87 && (keystate[VK_CONTROL] & 0x8000) && !(keystate[VK_MENU] & 0x8000) && pbox->OnClose)
				{
					pbox->OnClose(pbox);
					return 0;
				}
			}

			lastactivebox = pbox;
			lastactiveboxtime = GetTickCount();
			
			/* If there's an OnKeyDown function, translate the virtual key to
			 * unicode and pass it on */
			if (pbox->focus)
			{
				unsigned int scan = HIWORD(lParam) & 0xff;

				if (pbox->focus->OnKeyDown) {
					pbox->focus->OnKeyDown(pbox->focus, vk, scan);
				}
				else
				{
					char keystate[256];

					GetKeyboardState(keystate);

					if (vk == 9) /* tab */
					{
						Box_SetNextFocus(pbox->focus, keystate[VK_SHIFT] & 0x8000);
					}
				}
			}

			if (pbox->OnKeyShortcut)
			{
				unsigned int scan = HIWORD(lParam) & 0xff;

				pbox->OnKeyShortcut(pbox, vk, scan);
			}


			if (uMsg == WM_SYSKEYDOWN)
			{
				if (wParam == 115)
				{
					/* special case alt-f4 */
					if (pbox->OnClose)
					{
						pbox->OnClose(pbox);
					}
					return 0;
				}

				return DefWindowProc(hWnd,uMsg,wParam,lParam);
			}
			return 0;
		}
		break;


		case WM_LBUTTONDOWN:
		{
			unsigned int currentms = GetTickCount();
			int indblclkrange = 0;
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);

			if (lockedbox && Box_GetRoot(pbox) != Box_GetRoot(lockedbox))
			{
				return 0;
			}

			lastactivebox = pbox;
			lastactiveboxtime = GetTickCount();
			
			BringWindowToTop(hWnd);

			if (x < 0 || y < 0 || x >= pbox->w || y >= pbox->h)
			{
				return 0;
			}

			if ((abs(x - pbox->lastclickx) < 5) && (abs(y - pbox->lastclicky) < 5))
			{
				indblclkrange = 1;
			}

			pbox->lastclickx = x;
			pbox->lastclicky = y;

			if (pbox->OnLButtonDown)
			{
				pbox->OnLButtonDown(pbox, x, y);
			}

			if (currentms - pbox->lastldblclktime <= (unsigned int)(GetDoubleClickTime()) && indblclkrange)
			{
				pbox->lastlclicktime = 0;
				pbox->lastldblclktime = 0;

				Log_Write(0, "OnLButtonTrpClk\n");

				if (pbox->OnLButtonTrpClk)
				{
					pbox->OnLButtonTrpClk(pbox, x, y);
				}
			}
			else if (currentms - pbox->lastlclicktime <= (unsigned int)(GetDoubleClickTime()) && indblclkrange)
			{   
				pbox->lastlclicktime = 0;
				pbox->lastldblclktime = currentms;

				Log_Write(0, "OnLButtonDblClk\n");

				if (pbox->OnLButtonDblClk)
				{
					pbox->OnLButtonDblClk(pbox, x, y);
				}
			}
			else
			{
				Log_Write(0, "Click\n");

				pbox->lastlclicktime = currentms;
				pbox->lastldblclktime = 0;
			}

			return 0;
		}
		break;
		
		case WM_LBUTTONUP:
		{
			if (pbox->OnLButtonUp)
			{
				pbox->OnLButtonUp(pbox, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
			return 0;
		}
		break;

		case WM_RBUTTONDOWN:
		{
			if (lockedbox && Box_GetRoot(pbox) != Box_GetRoot(lockedbox))
			{
				return 0;
			}

			lastactivebox = pbox;
			lastactiveboxtime = GetTickCount();
			
			if (pbox->OnRButtonDown)
			{
				int x = GET_X_LPARAM(lParam) + pbox->xoffset;
				int y = GET_Y_LPARAM(lParam) + pbox->yoffset;
				if (x >= 0 && y >= 0 && x < pbox->w && y < pbox->h)
				{
					pbox->OnRButtonDown(pbox, x, y);
				}
			}

			return 0;
		}
		break;
		
		case WM_RBUTTONUP:
		{
			if (pbox->OnRButtonUp)
			{
				pbox->OnRButtonUp(pbox, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
			return 0;
		}
		break;

		case WM_MOUSEMOVE:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);

			if (Box_IsDragging())
			{
				HWND hwnd2;
				POINT pt;
				Box_GetScreenCoords(pbox, &(pt.x), &(pt.y));

				pt.x += x;
				pt.y += y;

				hwnd2 = WindowFromPoint(pt);

				if (hwnd2 && hwnd2 != GetActiveWindow()) 
				{
					WINDOWINFO wi;
					memset(&wi, 0, sizeof(wi));

					wi.cbSize = sizeof(wi);

					GetWindowInfo(hwnd2, &wi);

					if (wi.atomWindowType == boxwndatom || wi.atomWindowType == boxmenuatom)
					{
						struct Box_s *dialog = GetWindowLong(hwnd2, GWL_USERDATA);

						/* HACK: only activate window if dragging a chat tab and window is a chat window */
						if (dialog->boxtypeid == 0x1006 && Box_GetDragId() == 2)
						{
							SetActiveWindow(hwnd2);
						}
					}
				}
			}

			if (x != pbox->lastmousex || y != pbox->lastmousey)
			{
				Box_SetCursorImg(pbox, x, y);
				lastmousemovetime = GetTickCount();

				if (pbox->OnMouseMove)
				{
					pbox->OnMouseMove(pbox, x, y);
				}

				pbox->lastmousex = x;
				pbox->lastmousey = y;
			}

			return 0;
		}
		break;

		case WM_MOUSEWHEEL:
		{
			POINT pt;
			HWND hwnd2;
			WINDOWINFO wi;
			unsigned int scrolllines = 0;

			if (lockedbox && Box_GetRoot(pbox) != Box_GetRoot(lockedbox))
			{
				return 0;
			}

			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);

			hwnd2 = WindowFromPoint(pt);

			if (hwnd2)
			{
				memset(&wi, 0, sizeof(wi));

				wi.cbSize = sizeof(wi);

				GetWindowInfo(hwnd2, &wi);

				if (wi.atomWindowType == boxwndatom || wi.atomWindowType == boxmenuatom)
				{
					pbox = (struct Box_s *)GetWindowLong(hwnd2, GWL_USERDATA);
				}
			}

			SystemParametersInfo(104 /*SPI_GETWHEELSCROLLLINES*/, 0, &scrolllines, 0);

			if (scrolllines <= 0)
			{
				scrolllines = 1;
			}

			if (pbox->OnScrollWheel)
			{
				pbox->OnScrollWheel(pbox, (float)((short)HIWORD(wParam)) / (float)(120) * (float)(scrolllines));
			}

			return 0;
		}
		break;

		case WM_MOVE:
		{
			if (pbox->OnMove)
			{
				int x = GET_X_LPARAM(lParam);
				int y = GET_Y_LPARAM(lParam);

				if (pbox->x != x || pbox->y != y)
				{
					pbox->OnMove(pbox, x, y);
				}
			}
			return 0;
		}
		break;

		case WM_SIZE:
		{
			if (wParam == SIZE_MINIMIZED)
			{
				return 0;
			}

			if (pbox->OnSizeWidth)
			{
				int dw = LOWORD(lParam) - pbox->w;
				if (dw != 0)
				{
					pbox->OnSizeWidth(pbox, dw);
				}
			}

			if (pbox->OnSizeHeight)
			{
				int dh = HIWORD(lParam) - pbox->h;
				if (dh != 0)
				{
					pbox->OnSizeHeight(pbox, dh);
				}
			}
			
			Box_Repaint(pbox);

			return 0;
		}
		break;

		case WM_CLOSE:
		{
			Log_Write(0, "WM_CLOSE\n");

			if (pbox->OnClose)
			{
				pbox->OnClose(pbox);
			}
			return 0;
		}
		break;

		case WM_COMMAND:
		{
			if (pbox->OnCommand)
			{
				pbox->OnCommand(pbox, LOWORD(wParam));
			}
			return 0;
		}
		break;

		case WM_ACTIVATE:
		{
			if (lockedbox && Box_GetRoot(pbox) != Box_GetRoot(lockedbox))
			{
				BringWindowToTop(lockedbox->hwnd);
				return 0;
			}

			lastactivebox = pbox;
			lastactiveboxtime = GetTickCount();

			if ((LOWORD(wParam) == WA_ACTIVE))
			{
				SetFocus(pbox->hwnd);
			}

			if (pbox->parentactive)
			{
				pbox = pbox->parentactive;
			}

			if ((LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) && !pbox->active)
			{
				pbox->active = TRUE;

				if (pbox->OnActive)
				{
					pbox->OnActive(pbox);
				}

				if (pbox->focus && pbox->focus->OnGetFocus)
				{
					pbox->focus->OnGetFocus(pbox->focus);
				}
			}
			if (LOWORD(wParam) == WA_INACTIVE && pbox->active)
			{
				pbox->active = FALSE;

				if (pbox->OnInactive)
				{
					pbox->OnInactive(pbox);
				}

				if (pbox->focus && pbox->focus->OnLoseFocus)
				{
					pbox->focus->OnLoseFocus(pbox->focus);
				}
			}
			return 0;
		}
		break;

		case WM_PRINT:
		{
			return 0;
		}
		break;

		case WM_PRINTCLIENT:
		{
			return 0;
		}
		break;

		case WM_INITMENU:
		{	
			EnableMenuItem(GetSystemMenu(hWnd, FALSE), SC_MINIMIZE, MF_BYCOMMAND | (pbox->OnMinimize ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(GetSystemMenu(hWnd, FALSE), SC_MAXIMIZE, MF_BYCOMMAND | (pbox->OnMaximize ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(GetSystemMenu(hWnd, FALSE), SC_RESTORE,  MF_BYCOMMAND | (pbox->OnRestore  ? MF_ENABLED : MF_GRAYED));
			DrawMenuBar(hWnd);
			return 0;
		}
		break;

		case WM_SYSCOMMAND:
		{
			wParam &= 0xFFF0;
			/*Log_Write(0, "syscommand %d != %d == %d\n", wParam, SC_CLOSE, wParam != SC_CLOSE);*/

			switch(wParam)
			{
				case SC_CLOSE:
				{
					if (pbox->OnClose)
					{
						pbox->OnClose(pbox);
					}
				}
				break;

				case SC_MAXIMIZE:
				{
					if (pbox->OnMaximize)
					{
						pbox->OnMaximize(pbox);
					}
				}
				break;

				case SC_MINIMIZE:
				{
					if (pbox->OnMinimize)
					{
						pbox->OnMinimize(pbox);
					}
				}
				break;

				case SC_RESTORE:
				{
					if (pbox->OnRestore)
					{
						pbox->OnRestore(pbox);
					}
				}
				break;

				default:
				{
					/*Log_Write(0, "unhandled syscommand %d\n", wParam);*/
					return DefWindowProc(hWnd,uMsg,wParam,lParam);
				}
				break;
			}

			return 0;
		}
		break;

		case WM_SETCURSOR:
		{
			if (pbox->cursorimg)
			{
				SetCursor(pbox->cursorimg);
			}
			else
			{
				SetCursor(LoadCursor(NULL, IDC_ARROW));
			}

			return 0;
		}
		break;

		case WM_CAPTURECHANGED:
		{
			if (capturebox && Box_GetRoot(capturebox) == pbox)
			{
				Log_Write(0, "CaptureChanged %d\n", capturebox);
				if (capturebox->OnLoseMouseCapture)
				{
					capturebox->OnLoseMouseCapture(capturebox);
				}
				captured = 0;
				capturebox = NULL;
			}
			return 0;
		}
		break;

		default:
			break;
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}


struct Box_Timedfunc_s
{
	struct Box_Timedfunc_s *next;
	void (*func)(struct Box_s *, void *);
	struct Box_s *pbox;
	void *userdata;
	int period;
	int lastcalled;
	int destroy;
};


struct Box_Timedfunc_s *timedfuncs = NULL;

/***************************************************************************
 * Box_Init()
 *
 * Initializes the box system, by initializing common controls and
 * registering the universal box window class.
 *
 ***************************************************************************/
void Box_Init(int disablealphablending)
{
	WNDCLASS	wc;
	HANDLE hmsimg32;

	OleInitialize(NULL);
	InitCommonControls();
	
	memset(&wc, 0, sizeof(wc));
	wc.style         = /*CS_OWNDC |*/0;
	wc.lpfnWndProc	 = (WNDPROC) BoxWndProc;
	wc.hInstance	 = GetModuleHandle(NULL);
	wc.hIcon         = LoadIcon(GetModuleHandle(NULL), (LPCTSTR)IDI_ICON1);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "CustomBox";

	boxwndatom = RegisterClass(&wc);

	{
		OSVERSIONINFO osvi;

		memset(&osvi, 0, sizeof(osvi));
		osvi.dwOSVersionInfoSize = sizeof(osvi);

		GetVersionEx(&osvi);

		/* Only use the dropshadow on WinXP and above. */
		if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
                        if (osvi.dwMajorVersion > 5 || (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1))
			{
				wc.style = 0x00020000 /*CS_DROPSHADOW*/;
			}
		}
	}

	wc.lpszClassName = "CustomBoxMenu";

	boxmenuatom = RegisterClass(&wc);

	tahoma13_f = 
		CreateFont(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
				   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				   DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Tahoma");

	tahoma11_f = 
		CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
				   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				   DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Tahoma");

	tahoma11i_f = 
		CreateFont(-11, 0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE,
				   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				   DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Tahoma");

	tahoma11b_f =
		CreateFont(-11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
				   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				   DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Tahoma");

	tahoma11u_f =
		CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE,
				   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				   DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Tahoma");

	tahoma11ub_f =
		CreateFont(-11, 0, 0, 0, FW_BOLD, FALSE, TRUE, FALSE,
				   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				   DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Tahoma");

	tahoma10_f = 
		CreateFont(-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
				   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				   DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Tahoma");

	tahoma10i_f = 
		CreateFont(-10, 0, 0, 0, FW_BOLD, TRUE, FALSE, FALSE,
				   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				   DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Tahoma");

	tahoma10b_f =
		CreateFont(-10, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
				   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				   DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Tahoma");

	tahoma10u_f =
		CreateFont(-10, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE,
				   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				   DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Tahoma");

	tahoma10ub_f =
		CreateFont(-10, 0, 0, 0, FW_BOLD, FALSE, TRUE, FALSE,
				   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				   DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Tahoma");

	tahoma24b_f =
		CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				VARIABLE_PITCH | FF_SWISS, "Tahoma");

	W32_AlphaBlend = NULL;

	if (!disablealphablending && !Util_OldWinVer() && (hmsimg32 = LoadLibrary("msimg32.dll")))
	{
		HGDIOBJ temp;
		HBITMAP bitmapsrc, bitmapdst;
		HDC dcscreen, dcdst;
		char imagedata[16];
		struct BoxImage_s bisrc;
		COLORREF result;
		int error;

		W32_AlphaBlend = (void *)GetProcAddress(hmsimg32, "AlphaBlend");

		memset(&(imagedata[0]), 0, 16);

		imagedata[12] = 100;
		imagedata[13] = 0;
		imagedata[14] = 200;
		imagedata[15] = 192;
		bitmapsrc = BoxImage_CreateHBMPFromData(2, 2, imagedata);

		imagedata[12] = 222;
		imagedata[13] = 111;
		imagedata[14] = 0;
		imagedata[15] = 0;
		bitmapdst = BoxImage_CreateHBMPFromData(2, 2, imagedata);

		memset(&bisrc, 0, sizeof(bisrc));
		bisrc.x = 0;
		bisrc.y = 0;
		bisrc.w = 2;
		bisrc.h = 2;
		bisrc.complexalpha = 1;
		bisrc.hbmp = bitmapsrc;

		dcscreen = GetDC(NULL);
		dcdst = CreateCompatibleDC(dcscreen);
		temp = SelectObject(dcdst, bitmapdst);
		ReleaseDC(NULL, dcscreen);

		Box_Blend(dcdst, 1, 1, 1, 1, &bisrc, 1, 1, 1, 1, 0);

		result = GetPixel(dcdst, 1, 1);

		SelectObject(dcdst, temp);
		DeleteDC(dcdst);
		DeleteObject(bitmapsrc);
		DeleteObject(bitmapdst);

		error = abs(200 - GetRValue(result)) + abs(27 - GetGValue(result)) + abs(155 - GetBValue(result));

		if (error > 25)
		{
			Log_Write(0, "AlphaBlend test FAILED, disabling alpha blending...\n");
			W32_AlphaBlend = NULL;
		}
	}

	Box_InitIdleTracker();

	/*GPWrap_Init();*/
}

static int tickcount;

BOOL Box_Poll()
{
	MSG msg;

	struct Box_Timedfunc_s *current;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return FALSE;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}


	current = timedfuncs;

	tickcount = GetTickCount();
	while (current)
	{
		if (!current->destroy)
		{
			if (current->lastcalled == -1)
			{
				current->lastcalled = tickcount;
			}
			else if (tickcount - current->lastcalled > current->period)
			{
				current->lastcalled = tickcount;
				current->func(current->pbox, current->userdata);
			}
		}
		current = current->next;
	}

	{
		struct Box_Timedfunc_s **pcurrent;

		pcurrent = &timedfuncs;

		while (*pcurrent)
		{
			if ((*pcurrent)->destroy)
			{
				struct Box_Timedfunc_s *old = *pcurrent;
				*pcurrent = (*pcurrent)->next;
				free(old->userdata);
				free(old);
				continue;
			}
			pcurrent = &((*pcurrent)->next);
		}
	}

	Box_FlushDead();

	return TRUE;
}

/***************************************************************************
 * Box_Uninit()
 *
 * Uninitializes the box system, by unregistering the universal box class.
 *
 ***************************************************************************/
void Box_Uninit()
{
	Box_FlushDead();

	UnregisterClass("CustomBox", GetModuleHandle(NULL));

	Box_UninitIdleTracker();

	OleUninitialize();

	/*GPWrap_End();*/
}

void Box_AddTimedFunc(struct Box_s *pbox, void (*func)(struct Box_s *, void *), void *userdata, int period)
{
	struct Box_Timedfunc_s **current = &timedfuncs;

	while (*current)
	{
		if (   (*current)->func == func
			&& (*current)->pbox == pbox
			&& (*current)->period == period
			&& (*current)->destroy == 0)
		{
			return;
		}
		current = &((*current)->next);
	}

	*current = malloc(sizeof(**current));
	(*current)->func =       func;
	(*current)->pbox =       pbox;
	(*current)->period =     period;
	(*current)->userdata =   userdata;
	(*current)->lastcalled = -1;
	(*current)->next =       NULL;
	(*current)->destroy = 0;
}

void Box_RemoveTimedFunc(struct Box_s *pbox, void (*func)(struct Box_s *, void *), int period)
{
	struct Box_Timedfunc_s **current = &timedfuncs;

	while (*current)
	{
		if (   (*current)->func == func
			&& (*current)->pbox == pbox
			&& (*current)->period == period
			&& !(*current)->destroy)
		{
			(*current)->destroy = 1;
			/*
			struct Box_Timedfunc_s *old = *current;
			*current = (*current)->next;
			free(old->userdata);
			free(old);
			continue;
			*/
		}

		current = &((*current)->next);
	}
}

void Box_RemoveTimedFuncsForBox(struct Box_s *pbox)
{
	struct Box_Timedfunc_s **current = &timedfuncs;

	while (*current)
	{
		if ((*current)->pbox == pbox)
		{
			(*current)->destroy = 1;
		}

		current = &((*current)->next);
	}
}


/***************************************************************************
 * Box_CreateWindow()
 *
 * Creates and returns a new window with "pbox" as its root, "parent" as its
 * parent window, "dwStyle" as its window style and "dwExStyle" as its
 * EX window style.
 *
 ***************************************************************************/
HWND Box_CreateWindow(struct Box_s *pbox, char *titlebar, HWND parent, DWORD dwStyle, DWORD dwExStyle, int closeapp)
{
	RECT rc;
	HWND hwnd;

	rc.left   = pbox->x;
	rc.top    = pbox->y;
	rc.right  = pbox->x + pbox->w - 1;
	rc.bottom = pbox->y + pbox->h - 1;

	/* Adjust the window rectangle for things like title bars and such. */
	AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);

	hwnd = CreateWindowEx(dwExStyle, "CustomBox", titlebar, dwStyle | WS_CLIPCHILDREN,
						  rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1,
						  parent, NULL, GetModuleHandle(NULL), NULL);
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)pbox);
	
	pbox->OnMove	   = Box_OnMove;
	pbox->OnSizeWidth  = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;

	if (!pbox->OnMinimize)
	{
		pbox->OnMinimize = Box_OnMinimize;
	}

	if (!pbox->OnRestore)
	{
		pbox->OnRestore = Box_OnRestore;
	}
	
	/* Only app windows close the app when closed */
	if (closeapp)
	{
		pbox->OnClose = Box_OnClose_CloseApp;
	}
	else if (!(pbox->OnClose))
	{
		pbox->OnClose = Box_OnClose;
	}

	Box_SetHwnd(pbox, hwnd);

	/*BringWindowToTop(hwnd);*/
/*
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
*/

	return hwnd;
}


void Box_SetText(struct Box_s *pbox, const char *text)
{
	if (pbox->text)
	{
		free(pbox->text);
	}
	pbox->text = strdup(text);
}

void Box_ForceDraw(struct Box_s *pbox)
{
	HDC hdc, hdcsrc;
	HBITMAP hbmp, oldbmp;

	if (dragging)
	{
		return;
	}

	pbox = Box_GetRoot(pbox);

	hdc = GetDC(pbox->hwnd);
	hbmp = Box_Draw_Hbmp(pbox, FALSE);
		
	hdcsrc = CreateCompatibleDC(hdc);
	oldbmp = SelectObject(hdcsrc, hbmp);

	BitBlt(hdc, 0, 0, pbox->w, pbox->h, hdcsrc, 0, 0, SRCCOPY);

	SelectObject(hdcsrc, oldbmp);

	DeleteObject(hbmp);
	DeleteDC(hdcsrc);

	ReleaseDC(pbox->hwnd, hdc);
}

void Box_CreateWndCustom(struct Box_s *pbox, char *titlebar, HWND parent)
{
	HRGN hrgn, hrgnbottom;

	hrgn = CreateRoundRectRgn(0, 0, pbox->w + 1, pbox->h + 1, 15, 15);
	hrgnbottom = CreateRectRgn(0, pbox->h / 2, pbox->w, pbox->h);
	CombineRgn(hrgn, hrgn, hrgnbottom, RGN_OR);
	Box_CreateWindow(pbox, titlebar, NULL, WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
					WS_EX_APPWINDOW, 0 /*parent == NULL*/);
	SetWindowRgn(pbox->hwnd, hrgn, TRUE);
	DeleteObject(hrgn);
	DeleteObject(hrgnbottom);
	pbox->brcol = RGB(27, 31, 41);
	pbox->flags |= BOX_BORDER2;
/*
	if (!Box_IsMouseCaptured())
	{
		ShowWindow(pbox->hwnd, SW_SHOWNORMAL);
	}
	else
*/
	if (Box_CheckLastActive() || lockedbox)
	{
		pbox->active = FALSE;

		if (pbox->OnInactive)
		{
			pbox->OnInactive(pbox);
		}

		if (pbox->focus && pbox->focus->OnLoseFocus)
		{
			pbox->focus->OnLoseFocus(pbox->focus);
		}

		BringWindowToTop(lastactivebox->hwnd);
		ShowWindow(pbox->hwnd, SW_SHOWNOACTIVATE);
	}
	else
	{
		ShowWindow(pbox->hwnd, SW_SHOWNORMAL);
	}
}

void Box_MoveWndCustom(struct Box_s *pbox, int x, int y, int w, int h)
{
	HRGN hrgn, hrgnbottom;

	if (pbox->minw != 0 && w < pbox->minw)
	{
		w = pbox->minw;
	}

	if (pbox->minh != 0 && h < pbox->minh)
	{
		h = pbox->minh;
	}
	
	hrgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, 15, 15);
	hrgnbottom = CreateRectRgn(0, h / 2, w, h);
	CombineRgn(hrgn, hrgn, hrgnbottom, RGN_OR);
	SetWindowRgn(pbox->hwnd, hrgn, TRUE);
	DeleteObject(hrgn);
	DeleteObject(hrgnbottom);
	SetWindowPos(pbox->hwnd, NULL, x, y, w, h, SWP_NOACTIVATE | SWP_NOZORDER);
}

void Box_CreateWndCustom2(struct Box_s *pbox, char *titlebar, struct Box_s *parent)
{
	HRGN hrgn, hrgnbottom;

	hrgn = CreateRoundRectRgn(0, 0, pbox->w + 1, pbox->h + 1, 15, 15);
	hrgnbottom = CreateRectRgn(pbox->w / 2, 0, pbox->w, pbox->h);
	CombineRgn(hrgn, hrgn, hrgnbottom, RGN_OR);
	Box_CreateWindow(pbox, titlebar, parent->hwnd, WS_POPUP,
					0, parent == NULL);
	SetWindowRgn(pbox->hwnd, hrgn, TRUE);
	DeleteObject(hrgn);
	DeleteObject(hrgnbottom);
	pbox->brcol = RGB(27, 31, 41);
	pbox->flags |= BOX_BORDER3;

/*
	if (!Box_IsMouseCaptured())
	{
		ShowWindow(pbox->hwnd, SW_SHOWNORMAL);
	}
	else
*/
	{
		ShowWindow(pbox->hwnd, SW_SHOWNOACTIVATE);
	}

	pbox->parentactive = parent;
}

void Box_MoveWndCustom2(struct Box_s *pbox, int x, int y, int w, int h)
{
	HRGN hrgn, hrgnbottom;

	if (pbox->minw != 0 && w < pbox->minw)
	{
		w = pbox->minw;
	}

	if (pbox->minh != 0 && h < pbox->minh)
	{
		h = pbox->minh;
	}

	hrgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, 15, 15);
	hrgnbottom = CreateRectRgn(pbox->w / 2, 0, w, h);
	CombineRgn(hrgn, hrgn, hrgnbottom, RGN_OR);
	SetWindowRgn(pbox->hwnd, hrgn, TRUE);
	DeleteObject(hrgn);
	DeleteObject(hrgnbottom);
	SetWindowPos(pbox->hwnd, NULL, x, y, w, h, SWP_NOZORDER);
}

void Box_CreateWndMenu(struct Box_s *pbox, HWND parent)
{
	RECT rc;
	HWND hwnd;

	rc.left   = pbox->x;
	rc.top    = pbox->y;
	rc.right  = pbox->x + pbox->w - 1;
	rc.bottom = pbox->y + pbox->h - 1;

	/* Adjust the window rectangle for things like title bars and such. */
	AdjustWindowRectEx(&rc, WS_POPUP, FALSE, 0);

	hwnd = CreateWindowEx(0, "CustomBoxMenu", NULL, WS_POPUP | WS_CLIPCHILDREN,
	                      rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1,
	                      parent, NULL, GetModuleHandle(NULL), NULL);

	SetWindowLong(hwnd, GWL_USERDATA, (LONG)pbox);
	
	pbox->OnMove	   = Box_OnMove;
	pbox->OnSizeWidth  = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	
	Box_SetHwnd(pbox, hwnd);

	/*
	hwnd = CreateWindow("CustomBox", NULL, WS_OVERLAPPED, pbox->x + 300, pbox->y, pbox->w, pbox->h, NULL, NULL, NULL, NULL);
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)pbox);
	
	AnimateWindow(hwnd, 2000, AW_BLEND);
	*/
	/*
	Log_Write(0, "animate %d %d\n", pbox->hwnd, AnimateWindow(pbox->hwnd, 200, AW_SLIDE | AW_HIDE));
	Log_Write(0, "error %d\n", GetLastError());
	*/

	ShowWindow(pbox->hwnd, SW_SHOW);
/*
	Log_Write(0, "animate %d %d\n", pbox->hwnd, AnimateWindow(pbox->hwnd, 200, AW_BLEND | AW_ACTIVATE));
	dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &buf,
        0, NULL );
	Log_Write(0, "error %s\n", buf);
*/	
}

void Box_CreateWndMenu2(struct Box_s *pbox, HWND parent)
{
	RECT rc;
	HWND hwnd;

	rc.left   = pbox->x;
	rc.top    = pbox->y;
	rc.right  = pbox->x + pbox->w - 1;
	rc.bottom = pbox->y + pbox->h - 1;

	/* Adjust the window rectangle for things like title bars and such. */
	AdjustWindowRectEx(&rc, WS_POPUP, FALSE, 0);

	hwnd = CreateWindowEx(0, "CustomBoxMenu", NULL, WS_POPUP | WS_CLIPCHILDREN | WS_DISABLED,
	                      rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1,
	                      parent, NULL, GetModuleHandle(NULL), NULL);

	SetWindowLong(hwnd, GWL_USERDATA, (LONG)pbox);
	
	pbox->OnMove	   = Box_OnMove;
	pbox->OnSizeWidth  = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	
	Box_SetHwnd(pbox, hwnd);

	/*
	hwnd = CreateWindow("CustomBox", NULL, WS_OVERLAPPED, pbox->x + 300, pbox->y, pbox->w, pbox->h, NULL, NULL, NULL, NULL);
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)pbox);
	
	AnimateWindow(hwnd, 2000, AW_BLEND);
	*/
	/*
	Log_Write(0, "animate %d %d\n", pbox->hwnd, AnimateWindow(pbox->hwnd, 200, AW_SLIDE | AW_HIDE));
	Log_Write(0, "error %d\n", GetLastError());
	*/

	ShowWindow(pbox->hwnd, SW_SHOW);
/*
	Log_Write(0, "animate %d %d\n", pbox->hwnd, AnimateWindow(pbox->hwnd, 200, AW_BLEND));
	dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &buf,
        0, NULL );
	Log_Write(0, "error %s\n", buf);
*/	
}


void Box_CreateWndTooltip(struct Box_s *pbox, HWND parent)
{
	RECT rc;
	HWND hwnd;

	rc.left   = pbox->x;
	rc.top    = pbox->y;
	rc.right  = pbox->x + pbox->w - 1;
	rc.bottom = pbox->y + pbox->h - 1;

	/* Adjust the window rectangle for things like title bars and such. */
	AdjustWindowRectEx(&rc, WS_POPUP, FALSE, 0);

	hwnd = CreateWindowEx(0, "CustomBoxMenu", NULL, WS_POPUP | WS_CLIPCHILDREN | WS_DISABLED,
	                      rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1,
	                      parent, NULL, GetModuleHandle(NULL), NULL);

	SetWindowLong(hwnd, GWL_USERDATA, (LONG)pbox);
	
	pbox->OnMove	   = Box_OnMove;
	pbox->OnSizeWidth  = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	
	Box_SetHwnd(pbox, hwnd);

	/*
	hwnd = CreateWindow("CustomBox", NULL, WS_OVERLAPPED, pbox->x + 300, pbox->y, pbox->w, pbox->h, NULL, NULL, NULL, NULL);
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)pbox);
	
	AnimateWindow(hwnd, 2000, AW_BLEND);
	*/
	/*
	Log_Write(0, "animate %d %d\n", pbox->hwnd, AnimateWindow(pbox->hwnd, 200, AW_SLIDE | AW_HIDE));
	Log_Write(0, "error %d\n", GetLastError());
	*/

	ShowWindow(pbox->hwnd, SW_SHOWNOACTIVATE);
/*
	Log_Write(0, "animate %d %d\n", pbox->hwnd, AnimateWindow(pbox->hwnd, 200, AW_BLEND));
	dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &buf,
        0, NULL );
	Log_Write(0, "error %s\n", buf);
*/	
}

int Box_IsVisible(struct Box_s *pbox)
{
	while(pbox)
	{
		if (!(pbox->flags & BOX_VISIBLE))
		{
			return 0;
		}
		pbox = pbox->parent;
	}

	return 1;
}

void Box_ToFront(struct Box_s *pbox)
{
	struct Box_s *parent = pbox->parent;
	if (parent)
	{
		Box_Unlink(pbox);
		Box_AddChild(parent, pbox);
		Box_Repaint(parent);
	}
	else
	{
		BringWindowToTop(pbox->hwnd);
	}
}

void Box_SetFocus(struct Box_s *pbox)
{
	struct Box_s *root = Box_GetRoot(pbox);

	if (root->focus == pbox)
	{
		return;
	}

	if (root->focus && root->focus->OnLoseFocus)
	{
		struct Box_s *oldfocus = root->focus;
		root->focus = NULL;
		oldfocus->OnLoseFocus(oldfocus);
		Box_Repaint(oldfocus);
	}

	while (pbox->redirectfocus)
	{
		pbox = pbox->redirectfocus;
	}

	root->focus = pbox;

	if (pbox && pbox->OnGetFocus)
	{
		pbox->OnGetFocus(pbox);
		Box_Repaint(pbox);
	}
}

void Box_SetNextFocus(struct Box_s *pbox, int reverse)
{
	struct Box_s *root = Box_GetRoot(pbox);

	if (root->focus != pbox)
	{
		return;
	}

	if (reverse && pbox->prevfocus)
	{
		Box_SetFocus(pbox->prevfocus);
	}
	else if (pbox->nextfocus)
	{
		Box_SetFocus(pbox->nextfocus);
	}
}

int Box_HasFocus(struct Box_s *pbox)
{
	struct Box_s *root = Box_GetRoot(pbox);

	return (root->focus == pbox && root->active);
}

int Box_CheckXYInBox(struct Box_s *pbox, int x, int y)
{
	while (pbox)
	{
		if (x < 0 || y < 0 || x >= pbox->w || y >= pbox->h || !(pbox->flags & BOX_VISIBLE))
		{
			return 0;
		}

		x += pbox->x;
		y += pbox->y;

		pbox = pbox->parent;

		if (pbox)
		{
			x -= pbox->xoffset;
			y -= pbox->yoffset;
		}
	}

	return 1;
}

unsigned int idletrackerinit = 0;
HANDLE huser32;

BOOL (WINAPI * W32_GetLastInputInfo)(PLASTINPUTINFO plii);

void Box_InitIdleTracker()
{
	if ((huser32 = LoadLibrary("user32.dll")))
	{
		W32_GetLastInputInfo = (void *)GetProcAddress(huser32, "GetLastInputInfo");

		if (!W32_GetLastInputInfo)
		{
			FreeLibrary(huser32);
			return;
		}

		idletrackerinit = 1;
	}
}

void Box_UninitIdleTracker()
{
	if (huser32)
	{
		FreeLibrary(huser32);
	}
	idletrackerinit = 0;
}

int Box_GetIdleTime()
{
	if (idletrackerinit)
	{
		LASTINPUTINFO lii;
		lii.cbSize = sizeof(lii);

		W32_GetLastInputInfo(&lii);

                return GetTickCount() - lii.dwTime;
	}
	else
	{
		return 0;
	}
}

struct Box_s *Box_GetChessparkBox(HWND hwnd)
{
	WINDOWINFO wi;
	memset(&wi, 0, sizeof(wi));

	wi.cbSize = sizeof(wi);

	GetWindowInfo(hwnd, &wi);

	if (wi.atomWindowType == boxwndatom || wi.atomWindowType == boxmenuatom)
	{
		return (struct Box_s *)GetWindowLong(hwnd, GWL_USERDATA);
	}

	return NULL;
}

unsigned int Box_GetLastMouseMoveTime()
{
	return lastmousemovetime;
}

void Box_RegisterWindowName(struct Box_s *pbox, char *name)
{
	if (pbox->parent || !pbox->hwnd)
	{
		/* wtf, bail */
		return;
	}

	pbox->windowname = strdup(name);
	pbox->nextwindow = windowboxes;
	windowboxes = pbox;
}

struct Box_s *Box_GetWindowByName(char *name)
{
	struct Box_s *pbox = windowboxes;

	while (pbox)
	{
		if (pbox->windowname && stricmp(name, pbox->windowname) == 0)
		{
			return pbox;
		}
		pbox = pbox->nextwindow;
	}
	return NULL;
}
