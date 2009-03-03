#include <stdlib.h>

#include "box.h"

#include "sizer.h"

void *Box_OnSizeHeight_StickTop = NULL;
void *Box_OnSizeWidth_StickLeft = NULL;

struct sizerdata_s
{
	BOOL isover;
	BOOL isdrag;
	HCURSOR overcur;
	HCURSOR lastcur;
	int disabled;
	int xstart;
	int ystart;
	struct Box_s *target1;
	int xstartbox;
	int ystartbox;
	int wstartbox;
	int hstartbox;
	struct Box_s *target2;
	int xstartbox2;
	int ystartbox2;
	int wstartbox2;
	int hstartbox2;
};

void Sizer_OnDestroy(struct Box_s *sizer)
{
	Box_ReleaseMouse(sizer);
	Box_UnlockMouseCursorImg(sizer);
}

void Sizer_OnLButtonDown(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;
	int x, y;

	if (data->disabled)
	{
		return;
	}

	if (!Box_CaptureMouse(sizer))
	{
		return;
	}

	Box_GetScreenCoords(sizer, &x, &y);
	x += xmouse;
	y += ymouse;

	data->isdrag = TRUE;
	data->xstart = x;
	data->ystart = y;
	data->xstartbox = Box_GetRoot(sizer)->x;
	data->ystartbox = Box_GetRoot(sizer)->y;
	data->wstartbox = Box_GetRoot(sizer)->w;
	data->hstartbox = Box_GetRoot(sizer)->h;
}

void Sizer_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (Box_CheckXYInBox(sizer, xmouse, ymouse))
	{
		Box_LockMouseCursorImg(sizer, data->overcur);
	}
	else
	{
		Box_UnlockMouseCursorImg(sizer);
	}
}

void Sizer_OnLButtonUp(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		data->isdrag = FALSE;
		Box_ReleaseMouse(sizer);
	}
}

void Sizer_SetDisabled(struct Box_s *sizer, int disabled)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (!data->disabled && disabled)
	{
		if (data->isover && !data->isdrag)
		{
			data->isover = FALSE;
			Box_UnlockMouseCursorImg(sizer);
		}
		else if (data->isdrag)
		{
			data->isdrag = FALSE;
			Box_UnlockMouseCursorImg(sizer);
		}
	}

	data->disabled = disabled;
}

void NWSizer_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		int screenx, screeny, deltax, deltay, x, y, w, h;

		Box_GetScreenCoords(sizer, &screenx, &screeny);
		screenx += xmouse;
		screeny += ymouse;

		deltax = screenx - data->xstart;
		deltay = screeny - data->ystart;

		x = data->xstartbox + deltax;
		y = data->ystartbox + deltay;
		w = data->wstartbox - deltax;
		h = data->hstartbox - deltay;

		if (w < Box_GetRoot(sizer)->minw)
		{
			w = Box_GetRoot(sizer)->minw;
			x = data->xstartbox + (data->wstartbox - w);
		}

		if (h < Box_GetRoot(sizer)->minh)
		{
			h = Box_GetRoot(sizer)->minh;
			y = data->ystartbox + (data->hstartbox - h);
		}

		if (x == Box_GetRoot(sizer)->x && y == Box_GetRoot(sizer)->y && w == Box_GetRoot(sizer)->w && h == Box_GetRoot(sizer)->h)
		{
			return;
		}

		Box_MoveWndCustom(Box_GetRoot(sizer), x, y, w, h);
		Box_ForceDraw(Box_GetRoot(sizer));
	}
	else
	{
		Sizer_OnMouseMove(sizer, xmouse, ymouse);
	}
}

void NSizer_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		int screenx, screeny, deltax, deltay, x, y, w, h;

		Box_GetScreenCoords(sizer, &screenx, &screeny);
		screenx += xmouse;
		screeny += ymouse;

		deltax = screenx - data->xstart;
		deltay = screeny - data->ystart;

		x = data->xstartbox;
		y = data->ystartbox + deltay;
		w = data->wstartbox;
		h = data->hstartbox - deltay;

		if (h < Box_GetRoot(sizer)->minh)
		{
			h = Box_GetRoot(sizer)->minh;
			y = data->ystartbox + (data->hstartbox - h);
		}

		if (x == Box_GetRoot(sizer)->x && y == Box_GetRoot(sizer)->y && w == Box_GetRoot(sizer)->w && h == Box_GetRoot(sizer)->h)
		{
			return;
		}

		Box_MoveWndCustom(Box_GetRoot(sizer), x, y, w, h);
		Box_ForceDraw(Box_GetRoot(sizer));
	}
	else
	{
		Sizer_OnMouseMove(sizer, xmouse, ymouse);
	}
}

void NESizer_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		int screenx, screeny, deltax, deltay, x, y, w, h;
		
		Box_GetScreenCoords(sizer, &screenx, &screeny);
		screenx += xmouse;
		screeny += ymouse;

		deltax = screenx - data->xstart;
		deltay = screeny - data->ystart;

		x = data->xstartbox;
		y = data->ystartbox + deltay;
		w = data->wstartbox + deltax;
		h = data->hstartbox - deltay;

		if (w < Box_GetRoot(sizer)->minw)
		{
			w = Box_GetRoot(sizer)->minw;
		}

		if (h < Box_GetRoot(sizer)->minh)
		{
			h = Box_GetRoot(sizer)->minh;
			y = data->ystartbox + (data->hstartbox - h);
		}

		if (x == Box_GetRoot(sizer)->x && y == Box_GetRoot(sizer)->y && w == Box_GetRoot(sizer)->w && h == Box_GetRoot(sizer)->h)
		{
			return;
		}

		Box_MoveWndCustom(Box_GetRoot(sizer), x, y, w, h);
		Box_ForceDraw(Box_GetRoot(sizer));
	}
	else
	{
		Sizer_OnMouseMove(sizer, xmouse, ymouse);
	}
}

void WSizer_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		int screenx, screeny, deltax, deltay, x, y, w, h;
		
		Box_GetScreenCoords(sizer, &screenx, &screeny);
		screenx += xmouse;
		screeny += ymouse;

		deltax = screenx - data->xstart;
		deltay = screeny - data->ystart;

		x = data->xstartbox + deltax;
		y = data->ystartbox;
		w = data->wstartbox - deltax;
		h = data->hstartbox;

		if (w < Box_GetRoot(sizer)->minw)
		{
			w = Box_GetRoot(sizer)->minw;
			x = data->xstartbox + (data->wstartbox - w);
		}

		if (h < Box_GetRoot(sizer)->minh)
		{
			h = Box_GetRoot(sizer)->minh;
		}

		if (x == Box_GetRoot(sizer)->x && y == Box_GetRoot(sizer)->y && w == Box_GetRoot(sizer)->w && h == Box_GetRoot(sizer)->h)
		{
			return;
		}

		Box_MoveWndCustom(Box_GetRoot(sizer), x, y, w, h);
		Box_ForceDraw(Box_GetRoot(sizer));
	}
	else
	{
		Sizer_OnMouseMove(sizer, xmouse, ymouse);
	}
}

void ESizer_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		int screenx, screeny, deltax, deltay, x, y, w, h;
		
		Box_GetScreenCoords(sizer, &screenx, &screeny);
		screenx += xmouse;
		screeny += ymouse;

		deltax = screenx - data->xstart;
		deltay = screeny - data->ystart;

		x = data->xstartbox;
		y = data->ystartbox;
		w = data->wstartbox + deltax;
		h = data->hstartbox;

		if (w < Box_GetRoot(sizer)->minw)
		{
			w = Box_GetRoot(sizer)->minw;
		}

		if (x == Box_GetRoot(sizer)->x && y == Box_GetRoot(sizer)->y && w == Box_GetRoot(sizer)->w && h == Box_GetRoot(sizer)->h)
		{
			return;
		}

		Box_MoveWndCustom(Box_GetRoot(sizer), x, y, w, h);
		Box_ForceDraw(Box_GetRoot(sizer));
	}
	else
	{
		Sizer_OnMouseMove(sizer, xmouse, ymouse);
	}
}

void SWSizer_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		int screenx, screeny, deltax, deltay, x, y, w, h;
		
		Box_GetScreenCoords(sizer, &screenx, &screeny);
		screenx += xmouse;
		screeny += ymouse;

		deltax = screenx - data->xstart;
		deltay = screeny - data->ystart;

		x = data->xstartbox + deltax;
		y = data->ystartbox;
		w = data->wstartbox - deltax;
		h = data->hstartbox + deltay;

		if (w < Box_GetRoot(sizer)->minw)
		{
			w = Box_GetRoot(sizer)->minw;
			x = data->xstartbox + (data->wstartbox - w);
		}

		if (h < Box_GetRoot(sizer)->minh)
		{
			h = Box_GetRoot(sizer)->minh;
		}

		if (x == Box_GetRoot(sizer)->x && y == Box_GetRoot(sizer)->y && w == Box_GetRoot(sizer)->w && h == Box_GetRoot(sizer)->h)
		{
			return;
		}

		Box_MoveWndCustom(Box_GetRoot(sizer), x, y, w, h);
		Box_ForceDraw(Box_GetRoot(sizer));
	}
	else
	{
		Sizer_OnMouseMove(sizer, xmouse, ymouse);
	}
}


void SSizer_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		int screenx, screeny, deltax, deltay, x, y, w, h;
		
		Box_GetScreenCoords(sizer, &screenx, &screeny);
		screenx += xmouse;
		screeny += ymouse;

		deltax = screenx - data->xstart;
		deltay = screeny - data->ystart;

		x = data->xstartbox;
		y = data->ystartbox;
		w = data->wstartbox;
		h = data->hstartbox + deltay;

		if (h < Box_GetRoot(sizer)->minh)
		{
			h = Box_GetRoot(sizer)->minh;
		}

		if (x == Box_GetRoot(sizer)->x && y == Box_GetRoot(sizer)->y && w == Box_GetRoot(sizer)->w && h == Box_GetRoot(sizer)->h)
		{
			return;
		}

		Box_MoveWndCustom(Box_GetRoot(sizer), x, y, w, h);
		Box_ForceDraw(Box_GetRoot(sizer));
	}
	else
	{
		Sizer_OnMouseMove(sizer, xmouse, ymouse);
	}
}


void SESizer_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		int screenx, screeny, deltax, deltay, x, y, w, h;
		
		Box_GetScreenCoords(sizer, &screenx, &screeny);
		screenx += xmouse;
		screeny += ymouse;

		deltax = screenx - data->xstart;
		deltay = screeny - data->ystart;

		x = data->xstartbox;
		y = data->ystartbox;
		w = data->wstartbox + deltax;
		h = data->hstartbox + deltay;

		if (w < Box_GetRoot(sizer)->minw)
		{
			w = Box_GetRoot(sizer)->minw;
		}

		if (h < Box_GetRoot(sizer)->minh)
		{
			h = Box_GetRoot(sizer)->minh;
		}

		if (x == Box_GetRoot(sizer)->x && y == Box_GetRoot(sizer)->y && w == Box_GetRoot(sizer)->w && h == Box_GetRoot(sizer)->h)
		{
			return;
		}

		Box_MoveWndCustom(Box_GetRoot(sizer), x, y, w, h);
		Box_ForceDraw(Box_GetRoot(sizer));
	}
	else
	{
		Sizer_OnMouseMove(sizer, xmouse, ymouse);
	}
}

struct Box_s *Sizer_Create(int x, int y, int w, int h)
{
	struct Box_s *sizer = Box_Create(x, y, w, h, BOX_VISIBLE | BOX_TRANSPARENT);
	struct sizerdata_s *data = malloc(sizeof(*data));

	memset(data, 0, sizeof(*data));
	sizer->boxdata = data;

	sizer->OnLButtonDown = Sizer_OnLButtonDown;
	sizer->OnLButtonUp = Sizer_OnLButtonUp;
	sizer->OnDestroy = Sizer_OnDestroy;

	return sizer;
}

struct Box_s *NWSizer_Create(int x, int y, int w, int h)
{
	struct Box_s *sizer = Sizer_Create(x, y, w, h);
	struct sizerdata_s *data = sizer->boxdata;

	data->overcur = LoadCursor(NULL, IDC_SIZENWSE);

	sizer->OnMouseMove = NWSizer_OnMouseMove;
	sizer->OnSizeHeight = Box_OnSizeHeight_StickTop;
	sizer->OnSizeWidth = Box_OnSizeWidth_StickLeft;

	return sizer;
}

struct Box_s *NSizer_Create(int x, int y, int w, int h)
{
	struct Box_s *sizer = Sizer_Create(x, y, w, h);
	struct sizerdata_s *data = sizer->boxdata;

	data->overcur = LoadCursor(NULL, IDC_SIZENS);

	sizer->OnMouseMove = NSizer_OnMouseMove;
	sizer->OnSizeHeight = Box_OnSizeHeight_StickTop;
	sizer->OnSizeWidth = Box_OnSizeWidth_Stretch;

	return sizer;
}

struct Box_s *NESizer_Create(int x, int y, int w, int h)
{
	struct Box_s *sizer = Sizer_Create(x, y, w, h);
	struct sizerdata_s *data = sizer->boxdata;

	data->overcur = LoadCursor(NULL, IDC_SIZENESW);
	
	sizer->OnMouseMove = NESizer_OnMouseMove;
	sizer->OnSizeHeight = Box_OnSizeHeight_StickTop;
	sizer->OnSizeWidth = Box_OnSizeWidth_StickRight;

	return sizer;
}

struct Box_s *WSizer_Create(int x, int y, int w, int h)
{
	struct Box_s *sizer = Sizer_Create(x, y, w, h);
	struct sizerdata_s *data = sizer->boxdata;

	data->overcur = LoadCursor(NULL, IDC_SIZEWE);
	
	sizer->OnMouseMove = WSizer_OnMouseMove;
	sizer->OnSizeHeight = Box_OnSizeHeight_Stretch;
	sizer->OnSizeWidth = Box_OnSizeWidth_StickLeft;

	return sizer;
}

struct Box_s *ESizer_Create(int x, int y, int w, int h)
{
	struct Box_s *sizer = Sizer_Create(x, y, w, h);
	struct sizerdata_s *data = sizer->boxdata;

	data->overcur = LoadCursor(NULL, IDC_SIZEWE);
	
	sizer->OnMouseMove = ESizer_OnMouseMove;
	sizer->OnSizeHeight = Box_OnSizeHeight_Stretch;
	sizer->OnSizeWidth = Box_OnSizeWidth_StickRight;

	return sizer;
}

struct Box_s *SWSizer_Create(int x, int y, int w, int h)
{
	struct Box_s *sizer = Sizer_Create(x, y, w, h);
	struct sizerdata_s *data = sizer->boxdata;

	data->overcur = LoadCursor(NULL, IDC_SIZENESW);
	
	sizer->OnMouseMove = SWSizer_OnMouseMove;
	sizer->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	sizer->OnSizeWidth = Box_OnSizeWidth_StickLeft;

	return sizer;
}

struct Box_s *SSizer_Create(int x, int y, int w, int h)
{
	struct Box_s *sizer = Sizer_Create(x, y, w, h);
	struct sizerdata_s *data = sizer->boxdata;

	data->overcur = LoadCursor(NULL, IDC_SIZENS);
	
	sizer->OnMouseMove = SSizer_OnMouseMove;
	sizer->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	sizer->OnSizeWidth = Box_OnSizeWidth_Stretch;

	return sizer;
}

struct Box_s *SESizer_Create(int x, int y, int w, int h)
{
	struct Box_s *sizer = Sizer_Create(x, y, w, h);
	struct sizerdata_s *data = sizer->boxdata;

	data->overcur = LoadCursor(NULL, IDC_SIZENWSE);
	
	sizer->OnMouseMove = SESizer_OnMouseMove;
	sizer->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	sizer->OnSizeWidth = Box_OnSizeWidth_StickRight;

	return sizer;
}

struct Box_s *SizerSet_Create(struct Box_s *parent)
{
	struct Box_s *sizerset = Box_Create(0, 0, parent->w, parent->h, BOX_VISIBLE | BOX_TRANSPARENT);
	sizerset->OnSizeHeight = Box_OnSizeHeight_Stretch;
	sizerset->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(parent, sizerset);

	Box_AddChild(sizerset, NWSizer_Create(0, 0, 4, 4));
	Box_AddChild(sizerset, NSizer_Create(5, 0, parent->w - 11, 4));
	Box_AddChild(sizerset, NESizer_Create(parent->w - 5, 0, 4, 4));
	Box_AddChild(sizerset, WSizer_Create(0, 4, 5, parent->h - 10));
	Box_AddChild(sizerset, ESizer_Create(parent->w - 5, 5, 4, parent->h - 10));
	Box_AddChild(sizerset, SWSizer_Create(0, parent->h - 5, 4, 4));
	Box_AddChild(sizerset, SSizer_Create(5, parent->h - 4, parent->w - 10, 4));
	Box_AddChild(sizerset, SESizer_Create(parent->w - 5, parent->h - 5, 4, 4));

	return sizerset;
}

void SizerSet_SetDisabled(struct Box_s *sizerset, int disabled)
{
	struct Box_s *sizer = sizerset->child;

	while (sizer)
	{
		Sizer_SetDisabled(sizer, disabled);
		sizer = sizer->sibling;
	}
}

void SizerBar_OnLButtonDown(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;
	int x, y;

	if (data->disabled)
	{
		return;
	}

	if (!Box_CaptureMouse(sizer))
	{
		return;
	}

	Box_GetScreenCoords(sizer, &x, &y);
	x += xmouse;
	y += ymouse;

	data->isdrag = TRUE;
	data->xstart = x;
	data->ystart = y;

	data->xstartbox  = data->target1->x;
	data->ystartbox  = data->target1->y;
	data->wstartbox  = data->target1->w;
	data->hstartbox  = data->target1->h;

	data->xstartbox2 = data->target2->x;
	data->ystartbox2 = data->target2->y;
	data->wstartbox2 = data->target2->w;
	data->hstartbox2 = data->target2->h;
}

void VSizerBar_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		int screenx, screeny, deltax, deltay;
		int x1, y1, w1, h1, x2, y2, w2, h2;
		
		Box_GetScreenCoords(sizer, &screenx, &screeny);
		screenx += xmouse;
		screeny += ymouse;

		deltax = screenx - data->xstart;
		deltay = screeny - data->ystart;

		x1 = data->xstartbox;
		y1 = data->ystartbox;
		w1 = data->wstartbox + deltax;
		h1 = data->hstartbox;

		x2 = data->xstartbox2 + deltax;
		y2 = data->ystartbox2;
		w2 = data->wstartbox2 - deltax;
		h2 = data->hstartbox2;

		if (w1 < 20 || w2 < 20)
		{
			return;
		}

		if (data->target1->w != w1)
		{
			int dw = w1 - data->target1->w;

			/* FIXME: hack */
			Box_OnSizeWidth_Stretch(data->target1, dw);
		}

		if (data->target2->w != w2)
		{
			int dw = w2 - data->target2->w;

			/* FIXME: hack */
			Box_OnSizeWidth_Stretch(data->target2, dw);
		}

		data->target2->x = x2;

		sizer->x = x2 - 5;

		Box_Repaint(Box_GetRoot(sizer));

	}
	else
	{
		Sizer_OnMouseMove(sizer, xmouse, ymouse);
	}
}

struct Box_s *VSizerBar_Create(int x, int y, int w, int h, struct Box_s *left, struct Box_s *right)
{
	struct Box_s *sizer = Sizer_Create(x, y, w, h);
	struct sizerdata_s *data = sizer->boxdata;

	data->overcur = LoadCursor(NULL, IDC_SIZEWE);
	
	sizer->OnLButtonDown = SizerBar_OnLButtonDown;
	sizer->OnMouseMove = VSizerBar_OnMouseMove;
	sizer->OnSizeHeight = Box_OnSizeHeight_Stretch;
	sizer->OnSizeWidth = Box_OnSizeWidth_StickRight;

	data->target1 = left;
	data->target2 = right;

	return sizer;
}

void HSizerBar_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		int screenx, screeny, deltax, deltay;
		int x1, y1, w1, h1, x2, y2, w2, h2;
		
		Box_GetScreenCoords(sizer, &screenx, &screeny);
		screenx += xmouse;
		screeny += ymouse;

		deltax = screenx - data->xstart;
		deltay = screeny - data->ystart;

		x1 = data->xstartbox;
		y1 = data->ystartbox;
		w1 = data->wstartbox;
		h1 = data->hstartbox + deltay;

		x2 = data->xstartbox2;
		y2 = data->ystartbox2 + deltay;
		w2 = data->wstartbox2;
		h2 = data->hstartbox2 - deltay;

		if (h1 < 90 || h2 < 90)
		{
			return;
		}

		if (data->target1->h != h1)
		{
			int dh = h1 - data->target1->h;

			/* FIXME: hack */
			Box_OnSizeHeight_Stretch(data->target1, dh);
		}

		if (data->target2->h != h2)
		{
			int dh = h2 - data->target2->h;

			/* FIXME: hack */
			Box_OnSizeHeight_Stretch(data->target2, dh);
		}

		data->target2->y = y2;

		sizer->y = y2 - 5;

		Box_Repaint(Box_GetRoot(sizer));

	}
	else
	{
		Sizer_OnMouseMove(sizer, xmouse, ymouse);
	}
}

struct Box_s *HSizerBar_Create(int x, int y, int w, int h, struct Box_s *top, struct Box_s *bottom)
{
	struct Box_s *sizer = Sizer_Create(x, y, w, h);
	struct sizerdata_s *data = sizer->boxdata;

	data->overcur = LoadCursor(NULL, IDC_SIZENS);
	
	sizer->OnLButtonDown = SizerBar_OnLButtonDown;
	sizer->OnMouseMove = HSizerBar_OnMouseMove;
	sizer->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	sizer->OnSizeWidth = Box_OnSizeWidth_Stretch;

	data->target1 = top;
	data->target2 = bottom;

	return sizer;
}

void DrawerSizer_OnMouseMove(struct Box_s *sizer, int xmouse, int ymouse)
{
	struct sizerdata_s *data = sizer->boxdata;

	if (data->disabled)
	{
		return;
	}

	if (data->isdrag)
	{
		int screenx, screeny, deltax, deltay, x, y, w, h;
		
		Box_GetScreenCoords(sizer, &screenx, &screeny);
		screenx += xmouse;
		screeny += ymouse;

		deltax = screenx - data->xstart;
		deltay = screeny - data->ystart;

		x = data->xstartbox + deltax;
		y = data->ystartbox;
		w = data->wstartbox - deltax;
		h = data->hstartbox;

		if (w < Box_GetRoot(sizer)->minw)
		{
			w = Box_GetRoot(sizer)->minw;
			x = data->xstartbox + (data->wstartbox - w);
		}

		if (h < Box_GetRoot(sizer)->minh)
		{
			h = Box_GetRoot(sizer)->minh;
		}

		if (x == Box_GetRoot(sizer)->x && y == Box_GetRoot(sizer)->y && w == Box_GetRoot(sizer)->w && h == Box_GetRoot(sizer)->h)
		{
			return;
		}

		Box_MoveWndCustom2(Box_GetRoot(sizer), x, y, w, h);
		Box_ForceDraw(Box_GetRoot(sizer));
	}
	else
	{
		Sizer_OnMouseMove(sizer, xmouse, ymouse);
	}
}

struct Box_s *DrawerSizer_Create(int x, int y, int w, int h)
{
	struct Box_s *sizer = Sizer_Create(x, y, w, h);
	struct sizerdata_s *data = sizer->boxdata;

	data->overcur = LoadCursor(NULL, IDC_SIZEWE);
	
	sizer->OnMouseMove = DrawerSizer_OnMouseMove;
	sizer->OnSizeHeight = Box_OnSizeHeight_Stretch;
	sizer->OnSizeWidth = Box_OnSizeWidth_StickLeft;

	return sizer;
}