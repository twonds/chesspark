#include <stdlib.h>

#include "box.h"

#include "button.h"

#include "constants.h"
#include "imagemgr.h"

struct titledata_s
{
	struct Box_s *close;
	struct Box_s *minimize;
	struct Box_s *maximize;
	struct Box_s *textbox;
	struct Box_s *icon;
	BOOL clicked;
	int xstart;				/* X coord where drag started */
	int ystart;				/* Y coord where drag started */
	int xwinstart;
	int ywinstart;
	int shorten;
};

void TitleBar_SetInactive(struct Box_s *title);

void TitleBar_OnDestroy(struct Box_s *pbox)
{
	Box_ReleaseMouse(pbox);
}

void TitleBar_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct titledata_s *data = pbox->boxdata;
	struct Box_s *root = Box_GetRoot(pbox);
	int x, y;

	if (xmouse > pbox->w - data->shorten)
	{
		Box_OnLButtonDown(pbox, xmouse, ymouse);
		return;
	}

	if (!Box_CaptureMouse(pbox))
	{
		Box_OnLButtonDown(pbox, xmouse, ymouse);
		return;
	}

	Box_GetScreenCoords(pbox, &x, &y);
	x += xmouse;
	y += ymouse;
	
	data->clicked = TRUE;
	data->xstart = x;
	data->ystart = y;
	data->xwinstart = root->x;
	data->ywinstart = root->y;

	Box_OnLButtonDown(pbox, xmouse, ymouse);
}


void TitleBar_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct titledata_s *data = pbox->boxdata;
	struct Box_s *root = Box_GetRoot(pbox);
	int x, y;

	if (data->clicked)
	{
		/* Make sure the mouse button is down, in case we missed an lbuttonup. */
		if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		{
			Box_GetScreenCoords(pbox, &x, &y);
			x += xmouse;
			y += ymouse;
	
			MoveWindow(pbox->hwnd, data->xwinstart + x - data->xstart, data->ywinstart + y - data->ystart, root->w, root->h, TRUE);
		}
		else
		{
			data->clicked = 0;
			Box_ReleaseMouse(pbox);
		}
	}

	Box_OnMouseMove(pbox, xmouse, ymouse);
}


void TitleBar_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct titledata_s *data = pbox->boxdata;
	
	if (data->clicked)
	{
		data->clicked = FALSE;
		Box_ReleaseMouse(pbox);
	}

	Box_OnLButtonUp(pbox, xmouse, ymouse);
}

void TitleBar_OnLoseMouseCapture(struct Box_s *title)
{
	struct titledata_s *data = title->boxdata;

	if (data->clicked)
	{
		data->clicked = 0;
	}
}

struct Box_s *TitleBar_Add2(struct Box_s *dialog, char *titletext, void *onClose, void *onMinimize, void *onMaximize, int icon)
{
	struct Box_s *title, *pbox;
	struct titledata_s *data;

	int shorten = 0, offset = 0, buttonx;

	if (onClose)
	{
		shorten += 18;
	}
	if (onMinimize)
	{
		shorten += 18;
	}
	if (onMaximize)
	{
		shorten += 18;
	}
	if (icon)
	{
		shorten += (TitleBarHeight - 16) + 10;
		offset = (TitleBarHeight - 16) + 10;
	}

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	title = Box_Create(0, 0, dialog->w, TitleBarHeight, BOX_VISIBLE);
	title->OnLButtonDown = TitleBar_OnLButtonDown;
	title->OnMouseMove = TitleBar_OnMouseMove;
	title->OnLButtonUp = TitleBar_OnLButtonUp;
	title->OnLoseMouseCapture = TitleBar_OnLoseMouseCapture;
	title->OnDestroy = TitleBar_OnDestroy;
	title->bgcol = InactiveTitleBarBG;
	title->OnSizeWidth = Box_OnSizeWidth_Stretch;
	title->boxdata = data;
	
	pbox = Box_Create(10 + offset, 5, dialog->w - 10 - shorten, TitleBarHeight - 5, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->fgcol = InactiveTitleBarFG;
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_SetText(pbox, titletext);
	Box_AddChild(title, pbox);
	data->textbox = pbox;

	if (icon)
	{
		pbox = Box_Create(5, (TitleBarHeight - 16) / 2, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		Box_AddChild(title, pbox);
		data->icon = pbox;
	}

	buttonx = dialog->w - 18;

	if (onClose)
	{
		pbox = Button_Create(buttonx, 5, 14, 12, BOX_VISIBLE | BOX_TRANSPARENT);
		pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Button_SetOnButtonHit(pbox, onClose);
		Box_AddChild(title, pbox);
		data->close = pbox;

		buttonx -= 16;
	}

	if (onMaximize)
	{
		pbox = Button_Create(buttonx, 5, 14, 12, BOX_VISIBLE | BOX_TRANSPARENT);
		pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Button_SetOnButtonHit(pbox, onMaximize);
		Box_AddChild(title, pbox);
		data->maximize = pbox;

		buttonx -= 16;
	}

	if (onMinimize)
	{
		pbox = Button_Create(buttonx, 5, 14, 12, BOX_VISIBLE | BOX_TRANSPARENT);
		pbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Button_SetOnButtonHit(pbox, onMinimize);
		Box_AddChild(title, pbox);
		data->minimize = pbox;

		buttonx -= 16;
	}

	data->shorten = shorten;

	TitleBar_SetInactive(title);

	Box_AddChild(dialog, title);

	return title;
}

struct Box_s *TitleBarOnly_Add(struct Box_s *dialog, char *titletext)
{
	return TitleBar_Add2(dialog, titletext, NULL, NULL, NULL, 0);
}

struct Box_s *TitleBarCloseOnly_Add(struct Box_s *dialog, char *titletext, void *onClose)
{
	return TitleBar_Add2(dialog, titletext, onClose, NULL, NULL, 0);
}

struct Box_s *TitleBar_Add(struct Box_s *dialog, char *titletext, void *onClose, void *onMinimize, void *onMaximize)
{
	return TitleBar_Add2(dialog, titletext, onClose, onMinimize, onMaximize, 0);
}

struct Box_s *TitleBarIcon_Add(struct Box_s *dialog, char *titletext, void *onClose, void *onMinimize, void *onMaximize)
{
	return TitleBar_Add2(dialog, titletext, onClose, onMinimize, onMaximize, 1);
}

void TitleBar_SetIcon(struct Box_s *title, struct BoxImage_s *icon)
{
	struct titledata_s *data = title->boxdata;

	if (data->icon)
	{
		data->icon->img = icon;
	}

	Box_Repaint(title);
}


void TitleBar_SetText(struct Box_s *title, char *titletext)
{
	struct titledata_s *data = title->boxdata;

	Box_SetText(data->textbox, titletext);

	Box_Repaint(title);
}

void TitleBar_SetInactive(struct Box_s *title)
{
	struct titledata_s *data = title->boxdata;

	title->bgcol = InactiveTitleBarBG;
	data->textbox->fgcol = InactiveTitleBarFG;

	if (data->close)
	{
		Button_SetNormalImg (data->close, ImageMgr_GetSubImage("inactiveCloseButton1", "inactiveWindowClose.png", 0,  0, 12, 12));
		Button_SetHoverImg  (data->close, ImageMgr_GetSubImage("inactiveCloseButton2", "inactiveWindowClose.png", 12, 0, 12, 12));
		Button_SetPressedImg(data->close, ImageMgr_GetSubImage("inactiveCloseButton3", "inactiveWindowClose.png", 24, 0, 12, 12));
	}

	if (data->minimize)
	{
		Button_SetNormalImg (data->minimize, ImageMgr_GetSubImage("inactiveMinimizeButton1", "inactiveWindowMinimize.png", 0,  0, 12, 12));
		Button_SetHoverImg  (data->minimize, ImageMgr_GetSubImage("inactiveMinimizeButton2", "inactiveWindowMinimize.png", 12, 0, 12, 12));
		Button_SetPressedImg(data->minimize, ImageMgr_GetSubImage("inactiveMinimizeButton3", "inactiveWindowMinimize.png", 24, 0, 12, 12));
	}

	if (data->maximize)
	{
		Button_SetNormalImg (data->maximize, ImageMgr_GetSubImage("inactiveMaximizeButton1", "inactiveWindowMaximize.png", 0,  0, 12, 12));
		Button_SetHoverImg  (data->maximize, ImageMgr_GetSubImage("inactiveMaximizeButton2", "inactiveWindowMaximize.png", 13, 0, 12, 12));
		Button_SetPressedImg(data->maximize, ImageMgr_GetSubImage("inactiveMaximizeButton3", "inactiveWindowMaximize.png", 26, 0, 12, 12));
	}

	Box_Repaint(title);
}

void TitleBar_SetActive(struct Box_s *title)
{
	struct titledata_s *data = title->boxdata;

	title->bgcol = ActiveTitleBarBG;
	data->textbox->fgcol = ActiveTitleBarFG;

	if (data->close)
	{
		Button_SetNormalImg (data->close, ImageMgr_GetSubImage("CloseButton1", "windowClose.png", 0,  0, 12, 12));
		Button_SetHoverImg  (data->close, ImageMgr_GetSubImage("CloseButton2", "windowClose.png", 12, 0, 12, 12));
		Button_SetPressedImg(data->close, ImageMgr_GetSubImage("CloseButton3", "windowClose.png", 24, 0, 12, 12));
	}

	if (data->minimize)
	{
		Button_SetNormalImg (data->minimize, ImageMgr_GetSubImage("MinimizeButton1", "windowMinimize.png", 0,  0, 12, 12));
		Button_SetHoverImg  (data->minimize, ImageMgr_GetSubImage("MinimizeButton2", "windowMinimize.png", 12, 0, 12, 12));
		Button_SetPressedImg(data->minimize, ImageMgr_GetSubImage("MinimizeButton3", "windowMinimize.png", 24, 0, 12, 12));
	}

	if (data->maximize)
	{
		Button_SetNormalImg (data->maximize, ImageMgr_GetSubImage("MaximizeButton1", "windowMaximize.png", 0,  0, 12, 12));
		Button_SetHoverImg  (data->maximize, ImageMgr_GetSubImage("MaximizeButton2", "windowMaximize.png", 13, 0, 12, 12));
		Button_SetPressedImg(data->maximize, ImageMgr_GetSubImage("MaximizeButton3", "windowMaximize.png", 26, 0, 12, 12));
	}

	Box_Repaint(title);
}

void TitleBarRoot_OnActive(struct Box_s *pbox)
{
	if (pbox->titlebar)
	{
		TitleBar_SetActive(pbox->titlebar);
	}
}

void TitleBarRoot_OnInactive(struct Box_s *pbox)
{
	struct Box_s *title = pbox->titlebar;
	struct titledata_s *data = title->boxdata;

	if (pbox->titlebar)
	{
		TitleBar_SetInactive(pbox->titlebar);
	}
}

int TitleBar_IsDragging(struct Box_s *title)
{
	struct titledata_s *data = title->boxdata;

	return data->clicked;
}

int TitleBar_SetDragging(struct Box_s *title, int x, int y)
{
	struct titledata_s *data = title->boxdata;
	int sx, sy;

	if (!Box_CaptureMouse(title))
	{
		return 0;
	}

	Box_GetScreenCoords(title, &sx, &sy);
	sx += x;
	sy += y;

	data->clicked = 1;
	data->xstart = Box_GetRoot(title)->x;
	data->ystart = Box_GetRoot(title)->y;
	data->xwinstart = sx;
	data->ywinstart = sy;

	return 1;
}

void TitleBar_ForceUndrag(struct Box_s *pbox)
{
	struct titledata_s *data = pbox->boxdata;

	data->clicked = FALSE;
	Box_ReleaseMouse(pbox);
}