#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <stdlib.h>

#include "box.h"

#include "button.h"
#include "image.h"
#include "sizer.h"
#include "tabs.h"
#include "titledrag.h"

#include "boxtypes.h"
#include "ctrl.h"
#include "constants.h"
#include "drop.h"
#include "i18n.h"
#include "imagemgr.h"
#include "view.h"
#include "list.h"
#include "menu.h"
#include "util.h"
#include "titlebar.h"

#include "spawn.h"

void SpawnTab_OnDropEmpty(struct Box_s *tabctrl, int x, int y, int dropid, void *dropdata);
void Spawn_RemoveTab(struct Box_s *pbox, char *name);
void Spawn_ActivateFirstTab(struct Box_s *pbox);

struct spawndata_s
{
	struct Box_s *tabs, *add;
	int oldwidth, oldheight;
	int normh, normy;
	int numtabs;
};


void Spawn_OnClose(struct Box_s *pbox)
{
	View_CloseSpawnWindow(Box_GetRoot(pbox));
}

void Spawn_OnMinimize(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_MINIMIZE);
}

void Spawn_OnRestore(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_RESTORE);
}

void Spawn_OnMaximize(struct Box_s *pbox)
{
	int maxx, maxy, maxw, maxh;
	RECT rc;
	struct spawndata_s *data;
	HRGN hrgn, hrgnbottom;

	RECT windowrect;
	HMONITOR hm;
	MONITORINFO mi;

	pbox = Box_GetRoot(pbox);
	data = pbox->boxdata;

	windowrect.left = pbox->x;
	windowrect.right = windowrect.left + pbox->w - 1;
	windowrect.top = pbox->y;
	windowrect.bottom = windowrect.top + pbox->h - 1;

	hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hm, &mi);

	rc = mi.rcWork;

	maxx = rc.left;
	maxy = rc.top;
	maxw = rc.right - rc.left;
	maxh = rc.bottom - rc.top;

	if (pbox->h != maxh)
	{
		data->normh = pbox->h;
		data->normy = pbox->y;

		hrgn = CreateRoundRectRgn(0, 0, pbox->w, maxh, 15, 15);
		hrgnbottom = CreateRectRgn(0, maxh / 2, pbox->w, maxh);
		CombineRgn(hrgn, hrgn, hrgnbottom, RGN_OR);
		MoveWindow(pbox->hwnd, pbox->x, maxy, pbox->w, maxh, TRUE);
		SetWindowRgn(pbox->hwnd, hrgn, TRUE);

		Box_ForceDraw(pbox);

	}
	else if (data->normh != 0)
	{
		hrgn = CreateRoundRectRgn(0, 0, pbox->w, data->normh, 15, 15);
		hrgnbottom = CreateRectRgn(0, data->normh / 2, pbox->w, data->normh);
		CombineRgn(hrgn, hrgn, hrgnbottom, RGN_OR);
		MoveWindow(pbox->hwnd, pbox->x, data->normy, pbox->w, data->normh, TRUE);
		SetWindowRgn(pbox->hwnd, hrgn, TRUE);

		Box_ForceDraw(pbox);
	}
}

struct Box_s *Spawn_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct spawndata_s *data = malloc(sizeof(*data));
	struct Box_s *pbox, *spawn;

	memset(data, 0, sizeof(*data));

	spawn = Box_Create(x, y, w, h, BOX_VISIBLE);
	spawn->bgcol = DefaultBG;
	spawn->fgcol = RGB(0, 0, 0);
	spawn->minh = 256;
	spawn->minw = 256;

	SizerSet_Create(spawn);

	TitleBar_Add(spawn, _("Chesspark"), Spawn_OnClose, Spawn_OnMinimize, Spawn_OnMaximize);

	spawn->OnClose = Spawn_OnClose;
	spawn->OnMinimize = Spawn_OnMinimize;
	spawn->OnMaximize = Spawn_OnMaximize;
	spawn->OnRestore = Spawn_OnRestore;

	/* Roster add button */
	/*pbox = Box_Create(42, 224, 29, 22, BOX_VISIBLE);*/
	pbox = Button_Create(Margin, h - ButtonHeight - Margin, 30, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button_SetOnButtonHit(pbox, Menu_PopupAddMenu);
	Button_SetNormalImg (pbox, ImageMgr_GetSubImage("addMenuButton1", "addMenuStrip.png", 0,  0, 30, 23));
	Button_SetPressedImg(pbox, ImageMgr_GetSubImage("addMenuButton2", "addMenuStrip.png", 30, 0, 30, 23));
	Button_SetHoverImg  (pbox, ImageMgr_GetSubImage("addMenuButton3", "addMenuStrip.png", 60, 0, 30, 23));
	Box_AddChild(spawn, pbox);
	data->add = pbox;

	/* Grabber */
	pbox = Box_Create(w - GrabberWidth, h - GrabberHeight, GrabberWidth, GrabberHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->img = ImageMgr_GetImage("windowResizeHandle.png");
	pbox->OnSizeWidth  = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_AddChild(spawn, pbox);

	/* Tab Control */
	/*pbox = TabCtrl_Create(7, 45, 244, 40, BOX_VISIBLE | BOX_TRANSPARENT);*/
	pbox = TabCtrl_Create(Margin, TitleBarHeight + Margin, w - Margin * 2, TabHeight, BOX_VISIBLE | BOX_TRANSPARENT, TRUE, TRUE, ROSTERTABDROPDATA_ID);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->bgcol = TabBG2;
	pbox->fgcol = TabFG1;
	Box_AddChild(spawn, pbox);
	data->tabs = pbox;

	spawn->boxdata = data;
	spawn->boxtypeid = BOXTYPE_ROSTERSPAWN;

	data->oldwidth = 256;
	data->oldheight = 256;

	Box_CreateWndCustom(spawn, _("Chesspark"), NULL);

	return spawn;
}

void SpawnTab_OnDropEmpty(struct Box_s *tabctrl, int x, int y, int dropid, void *dropdata)
{
	char *tabname = dropdata;
	struct Box_s *spawn = Box_GetRoot(tabctrl);

	View_SpawnRosterTabWindow(tabname, x, y, 256, 256);
	Spawn_RemoveTab(spawn, tabname);
	Spawn_ActivateFirstTab(spawn);
}


void Spawn_AddTab(struct Box_s *pbox, char *name, struct Box_s *target)
{
	struct spawndata_s *data = pbox->boxdata;

	target->x = Margin;
	target->y = data->tabs->y + data->tabs->h;

	if (target->OnSizeWidth)
	{
		target->OnSizeWidth(target, (pbox->w - Margin * 2) - target->w);
	}

	if (target->OnSizeHeight)
	{
		target->OnSizeHeight(target, (pbox->h - target->y - Margin * 2 - ButtonHeight) - target->h);
	}

	TabCtrl_AddTab2(data->tabs, name, name, target, -58, NULL, NULL, ROSTERTABDROPDATA_ID, strdup(name), NULL);
	Box_AddChild(pbox, target);

	data->numtabs++;
}

void Spawn_RemoveTab(struct Box_s *pbox, char *name)
{
	struct spawndata_s *data = pbox->boxdata;

	TabCtrl_RemoveTab(data->tabs, name);

	data->numtabs--;
}

void Spawn_ActivateTab(struct Box_s *pbox, char *name)
{
	struct spawndata_s *data = pbox->boxdata;

	TabCtrl_ActivateTabByName(data->tabs, name);
}

void Spawn_ActivateFirstTab(struct Box_s *pbox)
{
	struct spawndata_s *data = pbox->boxdata;

	TabCtrl_ActivateFirst(data->tabs);
}

struct Box_s *Spawn_GetTabContentBox(struct Box_s *spawn, char *name)
{
	struct spawndata_s *data = spawn->boxdata;

	return TabCtrl_GetContentBox(data->tabs, name);
}