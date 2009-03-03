#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>

#include "box.h"

#include "constants.h"

extern HFONT tahoma11b_f, tahoma11ub_f, tahoma10b_f, tahoma10ub_f;

struct linkdata_s
{
	HCURSOR lastcur;
	void (*OnClick)(struct Box_s *, void *);
	void *userdata;
};

void LinkBox_OnDestroy(struct Box_s *link)
{
	struct linkdata_s *data = link->boxdata;

	Box_UnlockMouseCursorImg(link);
}

void LinkBox_OnMouseMove(struct Box_s *link, int xmouse, int ymouse)
{
	struct linkdata_s *data = link->boxdata;

	if (Box_CheckXYInBox(link, xmouse, ymouse))
	{
		if (Box_LockMouseCursorImg(link, LoadCursor(NULL, IDC_HAND)))
		{
			if (link->font == tahoma11b_f)
			{
				link->font = tahoma11ub_f;
			}
			else if (link->font == tahoma10b_f)
			{
				link->font = tahoma10ub_f;
			}
			Box_Repaint(link);
		}
	}
	else 
	{
		if (Box_UnlockMouseCursorImg(link))
		{
			if (link->font == tahoma11ub_f)
			{
				link->font = tahoma11b_f;
			}
			else if (link->font == tahoma10ub_f)
			{
				link->font = tahoma10b_f;
			}
			Box_Repaint(link);
		}
	}
}

void LinkBox_OnLButtonDown(struct Box_s *link, int xmouse, int ymouse)
{
	struct linkdata_s *data = link->boxdata;

	if (data->OnClick)
	{
		data->OnClick(link, data->userdata);
	}
}

struct Box_s *LinkBox_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Box_s *link = Box_Create(x, y, w, h, flags);
	struct linkdata_s *data = malloc(sizeof(*data));

	memset(data, 0, sizeof(*data));

	link->boxdata = data;
	link->fgcol = RGB(163, 97, 7); /*RGB(255, 128, 64);*/
	link->font = tahoma11b_f;
	link->OnMouseMove = LinkBox_OnMouseMove;
	link->OnDestroy = LinkBox_OnDestroy;
	link->OnLButtonDown = LinkBox_OnLButtonDown;

	return link;
}

void LinkBox_SetClickFunc(struct Box_s *link, void (*OnClick)(struct Box_s *, void *), void *userdata)
{
	struct linkdata_s *data = link->boxdata;

	data->OnClick = OnClick;
	data->userdata = userdata;
}