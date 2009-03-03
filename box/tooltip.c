#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "box.h"

#include "text.h"

void ToolTip_PopDown(struct Box_s *pbox);

void ToolTip_CheckPopDown(struct Box_s *pbox, void *userdata)
{
	int x, y;
	POINT p;

	if (!pbox->tooltipbox)
	{
		Box_RemoveTimedFunc(pbox, ToolTip_CheckPopDown, 1000);
		return;
	}

	if (!Box_IsVisible(pbox)/* || !Box_GetRoot(pbox)->active*/)
	{
		ToolTip_PopDown(pbox);
		return;
	}

	GetCursorPos(&p);

	/* If we're not in the box anymore, kill the tooltip */

	Box_GetScreenCoords(pbox, &x, &y);

	if (!(p.x >= x && p.y >= y && p.x <= x + pbox->w && p.y <= y + pbox->h))
	{
		ToolTip_PopDown(pbox);
		return;
	}

	/* otherwise, keep on loopin */
}

void ToolTip_Popup(struct Box_s *pbox, char *tiptext)
{
	int x, y;
	POINT p;

	if (pbox->tooltipbox)
	{
		DestroyWindow(pbox->tooltipbox->hwnd);
		Box_Destroy(pbox->tooltipbox);
		pbox->tooltipbox = NULL;
	}

	if (!Box_IsVisible(pbox) || Box_IsDragging()/* || !Box_GetRoot(pbox)->active*/)
	{
		Box_RemoveTimedFunc(pbox, ToolTip_Popup, 1000);
		return;
	}

	GetCursorPos(&p);

	/* If we're not in the box anymore, punt */

	Box_GetScreenCoords(pbox, &x, &y);

	if (!(p.x >= x && p.y >= y && p.x <= x + pbox->w && p.y <= y + pbox->h))
	{
		Box_RemoveTimedFunc(pbox, ToolTip_Popup, 1000);
		return;
	}

	x = p.x;
	y = p.y + 16;

	pbox->tooltipbox = Box_Create(x, y, 100, 100, BOX_VISIBLE | BOX_BORDER);
	pbox->tooltipbox->bgcol = RGB(241, 243, 231); /*MenuBG;*/
	pbox->tooltipbox->brcol = RGB(145, 146, 147); /*MenuBR*/;
	{
		struct Box_s *subbox = Text_Create(6, 6, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHVERT | TX_STRETCHHORIZ);

		Text_SetText(subbox, tiptext);

		Box_AddChild(pbox->tooltipbox, subbox);
		pbox->tooltipbox->w = subbox->w + 12;
		pbox->tooltipbox->h = subbox->h + 12;
	}

	{
		HMONITOR hm;
		MONITORINFO mi;

		hm = MonitorFromPoint(p, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		if (pbox->tooltipbox->x + pbox->tooltipbox->w > mi.rcWork.right)
		{
			pbox->tooltipbox->x = mi.rcWork.right - pbox->tooltipbox->w;
		}

		if (pbox->tooltipbox->x < mi.rcWork.left)
		{
			pbox->tooltipbox->x = mi.rcWork.left;
		}

		if (pbox->tooltipbox->y + pbox->tooltipbox->h > mi.rcWork.bottom)
		{
			pbox->tooltipbox->y = p.y - pbox->tooltipbox->h - 2;
		}
	}

	Box_CreateWndTooltip(pbox->tooltipbox, pbox->hwnd);
	Box_RemoveTimedFunc(pbox, ToolTip_Popup, 1000);
	Box_AddTimedFunc(pbox, ToolTip_CheckPopDown, NULL, 1000);
}

void ToolTip_PopDown(struct Box_s *pbox)
{
	Box_RemoveTimedFunc(pbox, ToolTip_Popup, 1000);
	Box_RemoveTimedFunc(pbox, ToolTip_CheckPopDown, 1000);

	if (pbox->tooltipbox)
	{
		DestroyWindow(pbox->tooltipbox->hwnd);
		Box_Destroy(pbox->tooltipbox);
		pbox->tooltipbox = NULL;
	}
}

void ToolTipParent_OnDestroy(struct Box_s *pbox)
{
	Box_RemoveTimedFunc(pbox, ToolTip_Popup, 1000);
	Box_RemoveTimedFunc(pbox, ToolTip_CheckPopDown, 1000);
}