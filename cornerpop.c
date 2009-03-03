#include <stdlib.h>

#include "box.h"

#include "text.h"

#include "button2.h"
#include "constants.h"
#include "i18n.h"
#include "stdbutton.h"
#include "titlebar.h"

#include "menu.h"

static unsigned int cornerpoplength = 5;
static int cornerpoplocation = 0;

struct cornerpopdata_s
{
	struct Box_s *textbox;
	int left;
	int right;
	int top;
	int bottom;
	int fullw;
	unsigned int animstart;
	int animstep;
};

void CornerPop_SetLength(int len)
{
	cornerpoplength = len;
}

void CornerPop_SetLocation(char *loc)
{
	if (loc && stricmp(loc, "topleft") == 0)
	{
		cornerpoplocation = 1;
	}
	else if (loc && stricmp(loc, "topright") == 0)
	{
		cornerpoplocation = 2;
	}
	else if (loc && stricmp(loc, "bottomleft") == 0)
	{
		cornerpoplocation = 3;
	}
	else
	{
		cornerpoplocation = 0;
	}
}

void CornerPop_OnKeyDown(struct Box_s *pbox, int vk, int scan)
{
	if (vk == 27 || vk == 13)
	{
		Box_Destroy(Box_GetRoot(pbox));
	}
}

void CornerPop_Animate(struct Box_s *dialog, void *userdata)
{
	struct cornerpopdata_s *data = dialog->boxdata;
	unsigned int currtime = GetTickCount();

	switch(data->animstep)
	{
		case 0:
		{
			int w = (currtime - data->animstart) * data->fullw / 1000;
			if (w > data->fullw)
			{
				w = data->fullw;
				data->animstart = currtime;
				data->animstep = 1;
			}

			switch(cornerpoplocation)
			{
				default:
				case 0: /* bottom right */
					dialog->xoffset = 0;
                                        Box_MoveWndCustom(dialog, data->right - w, data->bottom - dialog->h, w, dialog->h);
					break;
				case 1: /* top left */
					dialog->xoffset = data->fullw - w;
                                        Box_MoveWndCustom(dialog, data->left, data->top, w, dialog->h);
					break;
				case 2: /* top right */
					dialog->xoffset = 0;
                                        Box_MoveWndCustom(dialog, data->right - w, data->top, w, dialog->h);
					break;
				case 3: /* bottom left */
					dialog->xoffset = data->fullw - w;
                                        Box_MoveWndCustom(dialog, data->left, data->bottom - dialog->h, w, dialog->h);
					break;
			}
		}
		break;

		case 1:
		{
			if ((currtime - data->animstart) > cornerpoplength * 1000)
			{
				data->animstart = currtime;
				data->animstep = 2;
			}
		}
		break;

		case 2:
		{
			int w = (currtime - data->animstart) * data->fullw / 1000;
			if (w > data->fullw)
			{
				w = data->fullw;
				data->animstart = currtime;
				data->animstep = 3;
			}
			w = data->fullw - w;

			switch(cornerpoplocation)
			{
				default:
				case 0: /* bottom right */
					dialog->xoffset = 0;
                                        Box_MoveWndCustom(dialog, data->right - w, data->bottom - dialog->h, w, dialog->h);
					break;
				case 1: /* top left */
					dialog->xoffset = data->fullw - w;
                                        Box_MoveWndCustom(dialog, data->left, data->top, w, dialog->h);
					break;
				case 2: /* top right */
					dialog->xoffset = 0;
                                        Box_MoveWndCustom(dialog, data->right - w, data->top, w, dialog->h);
					break;
				case 3: /* bottom left */
					dialog->xoffset = data->fullw - w;
                                        Box_MoveWndCustom(dialog, data->left, data->bottom - dialog->h, w, dialog->h);
					break;
			}
		}
		break;

		default:
		{
			Box_Destroy(dialog);
		}

		break;
	}
}

struct Box_s *CornerPop_Create(struct Box_s *parent, int w, char *titletext,
  char *dialogtext)
{
	struct cornerpopdata_s *data;

	struct Box_s *dialog, *subbox;

	if (cornerpoplength <= 0)
	{
		/* apparently the user figured out how to turn off corner notifications. :) */
		return NULL;
	}

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	dialog = Box_Create(0, 0, w, 10, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;
	dialog->titlebar = TitleBarOnly_Add(dialog, titletext);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;
	dialog->OnMinimize = Box_OnMinimize;
	dialog->OnRestore = Box_OnRestore;
	dialog->OnClose = Box_Destroy;
	dialog->focus = dialog;
	dialog->OnKeyDown = CornerPop_OnKeyDown;

	subbox = Text_Create(20, 22 + 20, w - 40, 10, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT | TX_SELECTABLE | TX_COPYMENU);
	subbox->fgcol = UserInfoFG2;
	Text_SetLinkColor(subbox, CR_LtOrange);
	Text_SetText(subbox, dialogtext);
	Box_AddChild(dialog, subbox);
	data->textbox = subbox;
	dialog->w = subbox->w + 40;
	dialog->h = subbox->h + 22 + 20 + 20 + 20 + 5;

	{
		RECT rect;

		SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
		data->top = rect.top;
		data->bottom = rect.bottom;
		data->left = rect.left;
		data->right = rect.right;
		data->fullw = subbox->w + 40;

		dialog->x = 0;
		dialog->y = 0;
		dialog->w = 0;
	
		Box_CreateWndCustom(dialog, titletext, NULL);
	}

	dialog->boxdata = data;
	dialog->OnCommand = Menu_OnCommand;

	SetWindowPos(dialog->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	/* make it appear active even when it's not */
	TitleBar_SetActive(dialog->titlebar);

	data->animstart = GetTickCount();
	data->animstep = 0;

	Box_AddTimedFunc(dialog, CornerPop_Animate, NULL, 17);

	return dialog;
}

void CornerPop_SetLinkCallback(struct Box_s *dialog, int linknum, void (*OnClick)(struct Box_s *, void *), void *userdata)
{
	struct cornerpopdata_s *data = dialog->boxdata;

	Text_SetLinkCallback(data->textbox, linknum, OnClick, userdata);
}

void CornerPop_ResetText(struct Box_s *dialog, char *titletext, char *dialogtext)
{
	struct cornerpopdata_s *data = dialog->boxdata;
	int x, y, w, h;

	TitleBar_SetText(dialog->titlebar, titletext);
	Text_SetText(data->textbox, dialogtext);
	
	w = data->textbox->w + 40;
	h = data->textbox->h + 22 + 20 + 20 + 20 + 5;

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;

		windowrect.left = dialog->x;
		windowrect.right = windowrect.left + dialog->w - 1;
		windowrect.top = dialog->y;
		windowrect.bottom = windowrect.top + dialog->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - dialog->w) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - dialog->h) / 2;
	}

	Box_MoveWndCustom(dialog, x, y, w, h);
}
