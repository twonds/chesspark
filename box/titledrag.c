#if 0
#include <stdlib.h>

#include "box.h"

#include "titledrag.h"

struct titledragdata_s
{
	BOOL clicked;
	int xstart;				/* X coord where drag started */
	int ystart;				/* Y coord where drag started */
	int xwinstart;
	int ywinstart;
};

void TitleDrag_OnDestroy(struct Box_s *pbox)
{
	Box_ReleaseMouse(pbox);
}

void TitleDrag_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct titledragdata_s *data = pbox->boxdata;
	struct Box_s *root = Box_GetRoot(pbox);
	int x, y;

	if (!Box_CaptureMouse(pbox))
	{
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
}


void TitleDrag_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct titledragdata_s *data = pbox->boxdata;
	struct Box_s *root = Box_GetRoot(pbox);
	int x, y;

	if (data->clicked)
	{
		Box_GetScreenCoords(pbox, &x, &y);
		x += xmouse;
		y += ymouse;
	
		MoveWindow(pbox->hwnd, data->xwinstart + x - data->xstart, data->ywinstart + y - data->ystart, root->w, root->h, TRUE);
	}
}


void TitleDrag_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct titledragdata_s *data = pbox->boxdata;
	
	if (data->clicked)
	{
		data->clicked = FALSE;
		Box_ReleaseMouse(pbox);
	}
}

struct Box_s *TitleDrag_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Box_s *pbox = Box_Create(x, y, w, h, flags);
	struct titledragdata_s *data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));
	
	pbox->OnLButtonDown = TitleDrag_OnLButtonDown;
	pbox->OnMouseMove = TitleDrag_OnMouseMove;
	pbox->OnLButtonUp = TitleDrag_OnLButtonUp;

	pbox->boxdata = data;

	return pbox;
}

#endif