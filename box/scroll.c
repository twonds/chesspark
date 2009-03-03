#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <wingdi.h>

#include "button.h"

#include "box.h"

#include "scroll.h"


void VScrollBox_UpdateTarget(struct Box_s *pbox);


struct vscrollthumbdata_s
{
	BOOL clicked;
	int ystart;
	int yboxstart;
};

struct vscrolldata_s
{
	struct Box_s *up;
	struct Box_s *down;
	struct Box_s *thumb;
	struct Box_s *track;
	struct Box_s *target;
};

void VScrollBox_OnUp(struct Box_s *pbox, void *userdata);
void VScrollBox_OnDown(struct Box_s *pbox, void *userdata);
void VScrollBox_OnPageUp(struct Box_s *pbox, void *userdata);
void VScrollBox_OnPageDown(struct Box_s *pbox, void *userdata);

void VScrollThumb_OnDestroy(struct Box_s *pbox)
{
	struct vscrollthumbdata_s *data = pbox->boxdata;

	Box_ReleaseMouse(pbox);
	data->clicked = FALSE;
}

void VScrollBox_OnDestroy(struct Box_s *pbox)
{
	Box_RemoveTimedFunc(pbox, VScrollBox_OnUp, 100);
	Box_RemoveTimedFunc(pbox, VScrollBox_OnDown, 100);
	Box_RemoveTimedFunc(pbox, VScrollBox_OnPageUp, 300);
	Box_RemoveTimedFunc(pbox, VScrollBox_OnPageDown, 300);
}

void VScrollThumb_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct vscrollthumbdata_s *data = pbox->boxdata;
	int x, y;

	if (!Box_CaptureMouse(pbox))
	{
		return;
	}
	
	Box_GetScreenCoords(pbox, &x, &y);
	x += xmouse;
	y += ymouse;

	data->clicked = TRUE;
	data->ystart = y;
	data->yboxstart = pbox->y;
}

void VScrollThumb_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct vscrollthumbdata_s *data = pbox->boxdata;
	struct vscrolldata_s *pdata = pbox->parent->boxdata;

	if (data->clicked)
	{
		int x, y, miny, maxy;

		Box_GetScreenCoords(pbox, &x, &y);
		x += xmouse;
		y += ymouse;

		miny = pdata->track->y + 3;
		maxy = pdata->track->h - pdata->thumb->h - 3 + pdata->track->y;

		pbox->y = data->yboxstart + y - data->ystart;
		if (pbox->y < miny)
		{
			pbox->y = miny;
		}

		if (pbox->y > maxy)
		{
			pbox->y = maxy;
		}

		
		VScrollBox_UpdateTarget(pbox->parent);
	
		Box_Repaint(pbox->parent);
	}
}


void VScrollThumb_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct vscrollthumbdata_s *data = pbox->boxdata;

	if (data->clicked)
	{
		Box_ReleaseMouse(pbox);
		data->clicked = FALSE;
	}
}


struct Box_s *VScrollThumb_Create(int x, int y, int w, int h, enum box_flags flags)
{
	struct vscrollthumbdata_s *data = malloc(sizeof(*data));
	struct Box_s *thumb, *pbox;

	memset(data, 0, sizeof(*data));

	thumb = Box_Create(x, y, w, h, flags);
	thumb->OnLButtonDown = VScrollThumb_OnLButtonDown;
	thumb->OnMouseMove   = VScrollThumb_OnMouseMove;
	thumb->OnLButtonUp   = VScrollThumb_OnLButtonUp;

	pbox = Box_Create(0, 0, w, 2, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_AddChild(thumb, pbox);

	pbox = Box_Create(0, 2, w, h - 4, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(thumb, pbox);

	pbox = Box_Create(0, h - 2, w, 2, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_AddChild(thumb, pbox);

	pbox = Box_Create(0, (h - 3) / 2, w, 3, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeHeight = Box_OnSizeHeight_Center;
	Box_AddChild(thumb, pbox);

	thumb->boxdata = data;
	return thumb;
}

void VScrollBox_OnScrollWheel(struct Box_s *pbox, float delta)
{
	struct vscrolldata_s *data = pbox->boxdata;
	int max = data->target->realh - data->target->h;

	if (!Box_IsVisible(data->target))
	{
		return;
	}

	if (delta > 0)
	{
		data->target->yoffset -= (int)(delta * 10.0f);

		if (data->target->yoffset < 0)
		{
			data->target->yoffset = 0;
		}
	}
	else
	{
		if (max > 0 && max > data->target->yoffset)
		{
			data->target->yoffset -= (int)(delta * 10.0f);
			if ((data->target->yoffset > max))
			{
				data->target->yoffset = max;
			}
		}
	}

	Box_Repaint(pbox->parent);
	VScrollBox_UpdateThumb(pbox);
}


void VScrollBox_OnUp(struct Box_s *pbox, void *userdata)
{
	struct vscrolldata_s *pdata = pbox->boxdata;

	pdata->target->yoffset -= 10;

	if (pdata->target->yoffset < 0)
	{
		pdata->target->yoffset = 0;
	}

	Box_Repaint(pbox->parent);
	VScrollBox_UpdateThumb(pbox);
}


void VScrollBox_OnPageUp(struct Box_s *pbox, void *userdata)
{
	struct vscrolldata_s *pdata = pbox->boxdata;

	pdata->target->yoffset -= pdata->target->h;

	if (pdata->target->yoffset < 0)
	{
		pdata->target->yoffset = 0;
	}

	Box_Repaint(pbox->parent);
	VScrollBox_UpdateThumb(pbox);
}


void ButtonVScrollUp_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	Button_OnLButtonDown(pbox, xmouse, ymouse);
	VScrollBox_OnUp(pbox->parent, NULL);
	Box_AddTimedFunc(pbox->parent, VScrollBox_OnUp, NULL, 100);
}

void ButtonVScrollUp_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	Button_OnLButtonUp(pbox, xmouse, ymouse);
	Box_RemoveTimedFunc(pbox->parent, VScrollBox_OnUp, 100);
}


void VScrollBox_OnDown(struct Box_s *pbox, void *userdata)
{
	struct vscrolldata_s *pdata = pbox->boxdata;

	int max = pdata->target->realh - pdata->target->h;

	if (max > 0 && max > pdata->target->yoffset)
	{
		pdata->target->yoffset += 10;
		if ((pdata->target->yoffset > max))
		{
			pdata->target->yoffset = max;
		}
	}

	Box_Repaint(pbox->parent);
	VScrollBox_UpdateThumb(pbox);
}

void VScrollBox_OnPageDown(struct Box_s *pbox, void *userdata)
{
	struct vscrolldata_s *pdata = pbox->boxdata;

	int max = pdata->target->realh - pdata->target->h;

	if (max > 0 && max > pdata->target->yoffset)
	{
		pdata->target->yoffset += pdata->target->h;
		if ((pdata->target->yoffset > max))
		{
			pdata->target->yoffset = max;
		}
	}

	Box_Repaint(pbox->parent);
	VScrollBox_UpdateThumb(pbox);
}

void ButtonVScrollDown_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	Button_OnLButtonDown(pbox, xmouse, ymouse);
	VScrollBox_OnDown(pbox->parent, NULL);
	Box_AddTimedFunc(pbox->parent, VScrollBox_OnDown, NULL, 100);
}

void ButtonVScrollDown_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	Button_OnLButtonUp(pbox, xmouse, ymouse);
	Box_RemoveTimedFunc(pbox->parent, VScrollBox_OnDown, 100);
}

void VScrollTrack_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct vscrolldata_s *data = pbox->parent->boxdata;
	if (ymouse < data->thumb->y - data->track->y)
	{
		VScrollBox_OnPageUp(pbox->parent, NULL);
		Box_AddTimedFunc(pbox->parent, VScrollBox_OnPageUp, NULL, 300);
	}
	else if (ymouse > data->thumb->y + data->thumb->h - data->track->y)
	{
		VScrollBox_OnPageDown(pbox->parent, NULL);
		Box_AddTimedFunc(pbox->parent, VScrollBox_OnPageDown, NULL, 300);
	}
}

void VScrollTrack_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct vscrolldata_s *data = pbox->parent->boxdata;
	Box_RemoveTimedFunc(pbox->parent, VScrollBox_OnPageUp, 300);
	Box_RemoveTimedFunc(pbox->parent, VScrollBox_OnPageDown, 300);
}

struct Box_s *VScrollBox_Create(int x, int y, int w, int h, enum Box_flags flags, struct Box_s *target)
{
	struct vscrolldata_s *data = malloc(sizeof(*data));
	struct Box_s *pbox;
	struct Box_s *scroll = Box_Create(x, y, w, h, flags);
	int thumbh;
	
	pbox = Button_Create(0, 0, w, w, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnLButtonDown = ButtonVScrollUp_OnLButtonDown;
	pbox->OnLButtonUp =   ButtonVScrollUp_OnLButtonUp;
	Box_AddChild(scroll, pbox);
	data->up = pbox;

	pbox = Button_Create(0, h - w, w, w, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->OnLButtonDown = ButtonVScrollDown_OnLButtonDown;
	pbox->OnLButtonUp =   ButtonVScrollDown_OnLButtonUp;
	Box_AddChild(scroll, pbox);
	data->down = pbox;

	pbox = Box_Create(0, w, w, h - w * 2, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	pbox->OnLButtonDown = VScrollTrack_OnLButtonDown;
	pbox->OnLButtonUp = VScrollTrack_OnLButtonUp;
	Box_AddChild(scroll, pbox);
	data->track = pbox;

	pbox = Box_Create(0, 0, w, 2, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_AddChild(data->track, pbox);

	pbox = Box_Create(0, 2, w, h - 4 - w * 2, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(data->track, pbox);

	pbox = Box_Create(0, h - 2 - w * 2, w, 2, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_AddChild(data->track, pbox);

	thumbh = 24;

	if (target->realh)
	{
		thumbh = target->h * (data->track->h - 6) / target->realh;
	}

	if (thumbh < 24)
	{
		thumbh = 24;
	}

	if (thumbh > data->track->h - 6)
	{
		thumbh = data->track->h - 6;
	}

	pbox = VScrollThumb_Create(0, data->track->y + 3, w, thumbh, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnDestroy = VScrollThumb_OnDestroy;
	Box_AddChild(scroll, pbox);
	data->thumb = pbox;

	data->target = target;

	scroll->boxdata = data;
	scroll->OnScrollWheel = VScrollBox_OnScrollWheel;
	scroll->OnDestroy = VScrollBox_OnDestroy;
	return scroll;
}

void VScrollBox_UpdateTarget(struct Box_s *pbox)
{
	struct vscrolldata_s *data = pbox->boxdata;
	
	if (data->target)
	{
		int miny, maxy;
		int newthumbh = 24;

		if (data->target->realh)
		{
			newthumbh = data->target->h * (data->track->h - 6) / data->target->realh;
		}

		if (newthumbh < 24)
		{
			newthumbh = 24;
		}

		if (newthumbh > data->track->h - 6)
		{
			newthumbh = data->track->h - 6;
		}

		Box_OnSizeHeight_Stretch(data->thumb, newthumbh - data->thumb->h);

		miny = data->track->y + 3;
		maxy = data->track->h - data->thumb->h - 3 + data->track->y;

		if (data->target->realh > data->target->h && data->target->yoffset < data->target->realh - data->target->h)
		{
			if (maxy == miny)
			{
				data->target->yoffset = 0;
			}
			else
			{
				data->target->yoffset = (data->thumb->y - miny) * (data->target->realh - data->target->h) / (maxy - miny);
			}
			Box_Repaint(data->target);
		}
		else
		{
			if (data->target->yoffset != 0 && data->thumb->y != maxy)
			{
				data->target->yoffset -= maxy - data->thumb->y;
				if (data->target->yoffset < 0)
				{
					data->target->yoffset = 0;
				}
				data->thumb->y = maxy;
			}
		}
	}
}

void VScrollBox_UpdateThumb(struct Box_s *pbox)
{
	struct vscrolldata_s *data = pbox->boxdata;
	if (data->target)
	{
		int miny, maxy;
		int newthumbh = 24;

		if (data->target->realh)
		{
			newthumbh = data->target->h * (data->track->h - 6) / data->target->realh;
		}

		if (newthumbh < 24)
		{
			newthumbh = 24;
		}

		if (newthumbh > data->track->h - 6)
		{
			newthumbh = data->track->h - 6;
		}

		Box_OnSizeHeight_Stretch(data->thumb, newthumbh - data->thumb->h);

		miny = data->track->y + 3;
		maxy = data->track->h - data->thumb->h - 3 + data->track->y;

		if (data->target->realh > data->target->h)
		{
			data->thumb->y = data->target->yoffset * (maxy - miny) / (data->target->realh - data->target->h) + miny;
			if (data->thumb->y > maxy)
			{
				data->thumb->y = maxy;
			}
		}
		else
		{
			if (data->target->yoffset == 0)
			{
				data->thumb->y = miny;
			}
			else
			{
				data->thumb->y = maxy;
			}
		}
		Box_Repaint(pbox);
	}
}

void VScrollBox_SetUpButton(struct Box_s *pbox, struct BoxImage_s *normal, struct BoxImage_s *pressed)
{
	struct vscrolldata_s *data = pbox->boxdata;
	Button_SetNormalImg(data->up, normal);
	Button_SetPressedImg(data->up, pressed);
}

void VScrollBox_SetDownButton(struct Box_s *pbox, struct BoxImage_s *normal, struct BoxImage_s *pressed)
{
	struct vscrolldata_s *data = pbox->boxdata;
	Button_SetNormalImg(data->down, normal);
	Button_SetPressedImg(data->down, pressed);
}

void VScrollBox_SetThumbImages(struct Box_s *pbox, struct BoxImage_s *top, struct BoxImage_s *center, struct BoxImage_s *bottom, struct BoxImage_s *centertex)
{
	struct vscrolldata_s *data = pbox->boxdata;
	data->thumb->child->img = top;
	data->thumb->child->sibling->img = center;
	data->thumb->child->sibling->sibling->img = bottom;
	data->thumb->child->sibling->sibling->sibling->img = centertex;
}

void VScrollBox_SetTrackImages(struct Box_s *pbox, struct BoxImage_s *top, struct BoxImage_s *center, struct BoxImage_s *bottom)
{
	struct vscrolldata_s *data = pbox->boxdata;
	data->track->child->img = top;
	data->track->child->sibling->img = center;
	data->track->child->sibling->sibling->img = bottom;
}