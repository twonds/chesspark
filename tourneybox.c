#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "box.h"

#include "button2.h"
#include "edit.h"
#include "titledrag.h"
#include "button.h"
#include "sizer.h"
#include "tabs.h"
#include "text.h"

#include "ctrl.h"
#include "combobox.h"
#include "constants.h"
#include "edit2.h"
#include "imagemgr.h"
#include "info.h"
#include "link.h"
#include "list.h"
#include "menu.h"
#include "model.h"
#include "namedlist.h"
#include "participantentry.h"
#include "stdbutton.h"
#include "subchat.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

struct roundstartboxdata_s
{
	struct Box_s *roundcombo;
	struct Box_s *secondsedit;
	struct Box_s *minutesedit;
	struct Box_s *hoursedit;
	struct Box_s *daysedit;
	struct Box_s *monthcombo;
	struct Box_s *yearedit;
	char *tourneyid;
};

static char *months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

void RoundStartBox_OnNow(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct roundstartboxdata_s *data = dialog->boxdata;

	Ctrl_SetTournamentRoundStart(data->tourneyid, ComboBox_GetSelectionName(data->roundcombo), Info_GetCurrentTimestamp());

	Box_Destroy(dialog);
}

void RoundStartBox_OnOK(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct roundstartboxdata_s *data = dialog->boxdata;
	int sec, min, hour, day, mon, year;
	char *timestamp;
	char *num;
	int i;

	sec = 0;
	num = Edit2Box_GetText(data->secondsedit);
	if (num)
	{
		sscanf(num, "%d", &sec);
	}

	min = 0;
	num = Edit2Box_GetText(data->minutesedit);
	if (num)
	{
		sscanf(num, "%d", &min);
	}

	hour = 0;
	num = Edit2Box_GetText(data->hoursedit);
	if (num)
	{
		sscanf(num, "%d", &hour);
	}

	day = 0;
	num = Edit2Box_GetText(data->daysedit);
	if (num)
	{
		sscanf(num, "%d", &day);
	}

	mon = 1;
	for (i = 0; i < 12; i++)
	{
		if (strcmp(months[i], ComboBox_GetSelectionName(data->monthcombo)) == 0)
		{
			mon = i + 1;
		}
	}

	year = 0;
	num = Edit2Box_GetText(data->yearedit);
	if (num)
	{
		sscanf(num, "%d", &year);
	}

	timestamp = Info_ConvertTimeOfDayToTimestamp(sec, min, hour, day, mon, year);

	Ctrl_SetTournamentRoundStart(data->tourneyid, ComboBox_GetSelectionName(data->roundcombo), timestamp);

	Box_Destroy(dialog);
}

void RoundStartBox_OnCancel(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);

	Box_Destroy(dialog);
}

struct Box_s *RoundStartBox_Create(struct Box_s *parent, char *tourneyid)
{
	struct Box_s *dialog;
	struct Box_s *pbox;
	struct roundstartboxdata_s *data = malloc(sizeof(*data));
	int i, x, y;
	int sec, min, hour, day, mon, year;
	char num[256];

	data->tourneyid = strdup(tourneyid);

	Info_GetTimeOfDay(&sec, &min, &hour, &day, &mon, &year);

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;

		windowrect.left = parent->x;
		windowrect.right = windowrect.left + parent->w - 1;
		windowrect.top = parent->y;
		windowrect.bottom = windowrect.top + parent->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - 432) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - 160) / 2;
	}

	dialog = Box_Create(x, y, 432, 160, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;
	
	dialog->titlebar = TitleBarOnly_Add(dialog, "Set Round Start Time");
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	pbox = Box_Create(32, 48, 40, 16, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, "Round");
	Box_AddChild(dialog, pbox);

	pbox = ComboBox_Create(72, 48, 40, 20, BOX_VISIBLE);
	for (i = 1; i < 5; i++)
	{
		char num[3];

		sprintf(num, "%d", i);
		ComboBox_AddEntry(pbox, num);
	}
	Box_AddChild(dialog, pbox);
	ComboBox_SetSelection(pbox, "1");
	data->roundcombo = pbox;

	pbox = Box_Create(112, 48, 5, 20, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, ":");
	Box_AddChild(dialog, pbox);

	pbox = ComboBox_Create(117, 48, 80, 20, BOX_VISIBLE);
	for (i = 0; i < 12; i++)
	{
		ComboBox_AddEntry(pbox, months[i]);
	}
	ComboBox_SetSelection(pbox, months[mon - 1]);
	Box_AddChild(dialog, pbox);
	data->monthcombo = pbox;

	pbox = Edit2Box_Create(202, 48, 30, 20, BOX_VISIBLE, 1);
	pbox->bgcol = RGB(255, 255, 255);
	sprintf(num, "%d", day);
	Edit2Box_SetText(pbox, num);
	Box_AddChild(dialog, pbox);
	data->daysedit = pbox;

	pbox = Edit2Box_Create(237, 48, 40, 20, BOX_VISIBLE, 1);
	pbox->bgcol = RGB(255, 255, 255);
	sprintf(num, "%d", year);
	Edit2Box_SetText(pbox, num);
	Box_AddChild(dialog, pbox);
	data->yearedit = pbox;

	pbox = Edit2Box_Create(117, 78, 30, 20, BOX_VISIBLE, 1);
	pbox->bgcol = RGB(255, 255, 255);
	sprintf(num, "%d", hour);
	Edit2Box_SetText(pbox, num);
	Box_AddChild(dialog, pbox);
	data->hoursedit = pbox;

	pbox = Box_Create(147, 78, 5, 20, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, ":");
	Box_AddChild(dialog, pbox);

	pbox = Edit2Box_Create(152, 78, 30, 20, BOX_VISIBLE, 1);
	pbox->bgcol = RGB(255, 255, 255);
	sprintf(num, "%d", min);
	Edit2Box_SetText(pbox, num);
	Box_AddChild(dialog, pbox);
	data->minutesedit = pbox;

	pbox = Box_Create(182, 78, 5, 20, BOX_TRANSPARENT | BOX_VISIBLE);
	pbox->fgcol = PresenceFG;
	Box_SetText(pbox, ":");
	Box_AddChild(dialog, pbox);

	pbox = Edit2Box_Create(187, 78, 30, 20, BOX_VISIBLE, 1);
	pbox->bgcol = RGB(255, 255, 255);
	sprintf(num, "%d", sec);
	Edit2Box_SetText(pbox, num);
	Box_AddChild(dialog, pbox);
	data->secondsedit = pbox;

	pbox = StdButton_Create(432 - 300, 160 - 32, 80, "NOW!", 0);
	Button2_SetOnButtonHit(pbox, RoundStartBox_OnNow);
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(432 - 200, 160 - 32, 80, "OK", 0);
	Button2_SetOnButtonHit(pbox, RoundStartBox_OnOK);
	Box_AddChild(dialog, pbox);

	pbox = StdButton_Create(432 - 100, 160 - 32, 80, "Cancel", 0);
	Button2_SetOnButtonHit(pbox, RoundStartBox_OnCancel);
	Box_AddChild(dialog, pbox);

	dialog->boxdata = data;

	Box_CreateWndCustom(dialog, "Set Round Start Time", parent->hwnd);
	
	return dialog;
}

int TournamentPlayerEntrySortFunc(struct Box_s *left, struct Box_s *right)
{
	struct tournamentplayerinfo_s *linfo = left->boxdata;
	struct tournamentplayerinfo_s *rinfo = right->boxdata;
	
	if (linfo->score != rinfo->score)
	{
		return (linfo->score > rinfo->score);
	}

	if (linfo->rating != rinfo->rating)
	{
		return (linfo->rating > rinfo->rating);
	}

	return stricmp(Model_GetFriendNick(linfo->jid), Model_GetFriendNick(rinfo->jid)) < 0;
}

struct tourneyboxdata_s
{
	struct Box_s *sizerset;
	struct Box_s *vsizerbar;
	struct Box_s *chatlist;
	struct Box_s *chatborder;
	struct Box_s *participantlist;
	struct Box_s *participantborder;
	struct Box_s *edit;
	struct Box_s *editborder;
	struct Box_s *tourneyinfotextbox1;
	struct Box_s *messagebox;
	struct Box_s *tabs;

	struct Box_s *standingslist;
	struct Box_s *pairingslist;

	int hidechat;

	char *tourneyid;
	char *tourneychatjid;
	char *winner;

	struct tournamentinfo_s *info;
	int roundinplay;
	int tournamentover;

	int normx;
	int normy;
	int normw;
	int normh;
};

void TourneyBoxLists_OnSizeWidth(struct Box_s *pbox, int dw);
void TourneyBox_UpdateNextRoundMsg(struct Box_s *dialog, void *userdata);

void TourneyBox_OnClose(struct Box_s *pbox)
{
	struct Box_s *tourneybox = Box_GetRoot(pbox);
	struct tourneyboxdata_s *data = tourneybox->boxdata;

	View_CloseTourneyDialog(data->tourneyid);
}

void TourneyBox_OnRestore(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_RESTORE);
}

void TourneyBox_OnMinimize(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_MINIMIZE);
}

void TourneyBox_OnMaximize(struct Box_s *pbox)
{
	int maxx, maxy, maxw, maxh;
	RECT rc;
	struct tourneyboxdata_s *data;

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
		data->normx = pbox->x;
		data->normy = pbox->y;
		data->normw = pbox->w;
		data->normh = pbox->h;
		
		Box_MoveWndCustom(pbox, maxx, maxy, maxw, maxh);
		
		Box_ForceDraw(pbox);
	}
	else if (data->normh != 0)
	{
		Box_MoveWndCustom(pbox, data->normx, data->normy, data->normw, data->normh);

		Box_ForceDraw(pbox);
	}
}

void TourneyBox_OnDestroy(struct Box_s *tourneybox)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;

	if (data->tourneychatjid)
	{
		Ctrl_LeaveChat(data->tourneychatjid);
	}

	Box_RemoveTimedFunc(tourneybox, TourneyBox_UpdateNextRoundMsg, 1000);
}


void TourneyChatBox_OnEnter(struct Box_s *pbox, char *text);

void TourneyBox_SetSizeChatLists(struct Box_s *dialog, int chatw)
{
	struct tourneyboxdata_s *data = dialog->boxdata;
	int totalwidth = dialog->w - data->chatborder->x - 5;

	Box_OnSizeWidth_Stretch(data->chatborder, chatw - data->chatborder->w);
	Box_OnSizeWidth_Stretch(data->participantborder, totalwidth - chatw - 5 - data->participantborder->w);
	data->participantborder->x = data->chatborder->x + chatw + 5;
	data->vsizerbar->x = data->participantborder->x - 5;
	data->editborder->x = data->chatborder->x;
	data->edit->x = data->chatborder->x + 5;
	Box_OnSizeWidth_Stretch(data->editborder, data->participantborder->w + 5 + chatw - data->editborder->w);
	Box_OnSizeWidth_Stretch(data->edit, data->editborder->w - 10 - data->edit->w);

	Box_Repaint(dialog);
}

void TourneyBoxLists_OnSizeWidth(struct Box_s *pbox, int dw)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct tourneyboxdata_s *data = dialog->boxdata;
	int totalwidth = dialog->w - data->chatborder->x - 5;
	int chatw = data->chatborder->w + dw;

	if (totalwidth <= 85 || chatw <= 40)
	{
		chatw = 40;
	}
	else if (chatw >= totalwidth - 45)
	{
		chatw = totalwidth - 45;
	}

	TourneyBox_SetSizeChatLists(dialog, chatw);
}

void TourneyBoxEdit_EditSizeFunc(struct Box_s *edit, int edith)
{
	struct Box_s *dialog = Box_GetRoot(edit);
	struct tourneyboxdata_s *data = dialog->boxdata;
	int edity;

	if (edith > dialog->h / 2)
	{
		edith = dialog->h / 2;
	}

	edity = dialog->h - 6 - edith;

	Box_OnSizeHeight_Stretch(data->editborder, edith - data->edit->h);
	Box_OnSizeHeight_Stretch(data->edit, edith - data->edit->h);
	data->edit->y = edity;
	data->editborder->y = edity - 2;

	/*Box_OnSizeHeight_Stretch(data->chatlist,          dialog->h - data->chatlist->y          - edith - 8 - 10 - data->chatlist->h);*/
	Box_OnSizeHeight_Stretch(data->vsizerbar,         dialog->h - data->vsizerbar->y         - edith - 12 - data->vsizerbar->h);
	/*Box_OnSizeHeight_Stretch(data->participantlist,   dialog->h - data->participantlist->y   - edith - 8 - 10 - data->participantlist->h);*/
	Box_OnSizeHeight_Stretch(data->chatborder,        dialog->h - data->chatborder->y        - edith - 12 - data->chatborder->h);
	Box_OnSizeHeight_Stretch(data->participantborder, dialog->h - data->participantborder->y - edith - 12 - data->participantborder->h);
}

void TourneyBox_OnTourneyMenu(struct Box_s *pbox)
{
	struct Box_s *tourneybox = Box_GetRoot(pbox);
	struct tourneyboxdata_s *data = tourneybox->boxdata;
	int ismanager = 0, isplayer = 0, tourneystarted = 0;
	char *bareloginjid = Jid_Strip(Model_GetLoginJid());

	int x, y;

	Box_GetScreenCoords(pbox, &x, &y);
	x += pbox->w / 2;
	y += pbox->h / 2;

	if (data->info)
	{
		if (NamedList_GetByName(&(data->info->players), bareloginjid))
		{
			isplayer = 1;
		}
		if (stricmp(data->info->manager, bareloginjid) == 0)
		{
			ismanager = 1;
		}
		if (data->info->currentround)
		{
			tourneystarted = 1;
		}

	}

	Menu_PopupTourneyMenu(pbox, data->tourneyid, x, y, isplayer, ismanager, tourneystarted);
}

void TourneyBox_OnChatArea(struct Box_s *pbox)
{
	struct Box_s *tourneybox = Box_GetRoot(pbox);
	struct tourneyboxdata_s *data = tourneybox->boxdata;

	if (data->hidechat)
	{
		tourneybox->minw = 800;
		data->edit->flags              |= BOX_VISIBLE;
		data->editborder->flags        |= BOX_VISIBLE;
		data->chatborder->flags        |= BOX_VISIBLE;
		data->participantborder->flags |= BOX_VISIBLE;
		data->vsizerbar->OnSizeWidth = TourneyBoxLists_OnSizeWidth;
		if (data->normh)
		{
			TourneyBox_SetSizeChatLists(tourneybox, data->chatborder->w);
		}
		else
		{
			int w = data->editborder->w;
			if (w < 400)
			{
				w = 400;
			}
			Box_MoveWndCustom(tourneybox, tourneybox->x, tourneybox->y, data->standingslist->w + w + 20, tourneybox->h);
		}
		data->hidechat = 0;
	}
	else
	{
		tourneybox->minw = 388;
		data->edit->flags              &= ~BOX_VISIBLE;
		data->editborder->flags        &= ~BOX_VISIBLE;
		data->chatborder->flags        &= ~BOX_VISIBLE;
		data->participantborder->flags &= ~BOX_VISIBLE;
		data->vsizerbar->OnSizeWidth = NULL;
		if (data->normh)
		{
			TourneyBox_SetSizeChatLists(tourneybox, data->chatborder->w);
		}
		else
		{
			Box_MoveWndCustom(tourneybox, tourneybox->x, tourneybox->y, data->standingslist->w + 20, tourneybox->h);
		}
		data->hidechat = 1;
	}

}

struct Box_s *TourneyBox_Create(int x, int y, char *tourneyid, char *tourneychatjid)
{
	struct Box_s *dialog, *subbox;
	struct tourneyboxdata_s *data;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));
	data->tourneyid = strdup(tourneyid);
	data->tourneychatjid = strdup(tourneychatjid);

	dialog = Box_Create(x, y, 800, 582, BOX_VISIBLE);
	dialog->minw = 800;
	dialog->minh = 582;

	dialog->bgcol = DefaultBG;
	dialog->OnSizeWidth = Box_OnSizeWidth_Stretch;	

	data->sizerset = SizerSet_Create(dialog);
	/*
	if (!roomjid)
	{
		SizerSet_SetDisabled(data->sizerset, 1);
	}
*/
	dialog->titlebar = TitleBar_Add(dialog, "Tournament", TourneyBox_OnClose, TourneyBox_OnMinimize, TourneyBox_OnMaximize);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;
	
	dialog->OnClose = TourneyBox_OnClose;
	dialog->OnMinimize = TourneyBox_OnMinimize;
	dialog->OnMaximize = TourneyBox_OnMaximize;
	dialog->OnRestore = TourneyBox_OnRestore;

	subbox = Text_Create(5, TitleBarHeight + 5, 380, 40, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);
	Box_AddChild(dialog, subbox);
	data->tourneyinfotextbox1 = subbox;

	/* Tab BG */
	subbox = Box_Create(Margin, TitleBarHeight + 50 + TabHeight - 1, 380, dialog->h - TitleBarHeight - 50 - TabHeight - 75 + 1, BOX_VISIBLE | BOX_BORDER);
	subbox->bgcol = TabBG2;
	subbox->brcol = RGB(176,178,183);
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(dialog, subbox);
	
	{
		struct Box_s *subbox2;
		subbox2 = Box_Create(subbox->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersur", "contentcorners.png", 5, 0, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdr", "contentcorners.png", 5, 5, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(0, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdl", "contentcorners.png", 0, 5, 5, 5);
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);
	}

	subbox = TabCtrl_Create(Margin, TitleBarHeight + 50, 380, TabHeight, BOX_VISIBLE | BOX_TRANSPARENT, 0, TRUE, 0);
	subbox->bgcol = TabBG2;
	subbox->fgcol = TabFG1;
	Box_AddChild(dialog, subbox);
	data->tabs = subbox;

	subbox = Text_Create(Margin, dialog->h - 70, 380, 40, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT, TX_CENTERED | TX_WRAP);
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	subbox->fgcol = RGB(255, 255, 255);
	Box_AddChild(dialog, subbox);
	data->messagebox = subbox;

	subbox = Button_Create(320, dialog->h - 40, 30, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	Button_SetNormalImg (subbox, ImageMgr_GetSubImage("preferenceMenuButton1", "preferenceMenu.png", 0,  0, 30, 23));
	Button_SetPressedImg(subbox, ImageMgr_GetSubImage("preferenceMenuButton2", "preferenceMenu.png", 30, 0, 30, 23));
	Button_SetHoverImg  (subbox, ImageMgr_GetSubImage("preferenceMenuButton3", "preferenceMenu.png", 60, 0, 30, 23));
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Button_SetOnButtonHit(subbox, TourneyBox_OnTourneyMenu);
	Box_AddChild(dialog, subbox);
	
	subbox = Button_Create(354, dialog->h - 40, 36, 26, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	subbox->img = ImageMgr_GetSubImage("ChatAreaButton1", "ChatAreaButton.png", 0, 0, 26, 26);
	Button_SetOnButtonHit(subbox, TourneyBox_OnChatArea);
	Box_AddChild(dialog, subbox);

	subbox = List_Create(Margin + 5, TitleBarHeight + 50 + TabHeight, 370, dialog->h - TitleBarHeight - 50 - TabHeight - 75 - 10, 0, 0);
	subbox->bgcol = TabBG2;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	List_SetEntrySelectable(subbox, FALSE);
	List_SetEntrySortFunc(subbox, TournamentPlayerEntrySortFunc);
	Box_AddChild(dialog, subbox);
	data->standingslist = subbox;

	TabCtrl_AddTab(data->tabs, "Standings", subbox, 110);

	subbox = List_Create(Margin + 5, TitleBarHeight + 50 + TabHeight, 370, dialog->h - TitleBarHeight - 50 - TabHeight - 75 - 10, 0, 0);
	subbox->bgcol = TabBG2;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	List_SetEntrySelectable(subbox, FALSE);
	Box_AddChild(dialog, subbox);
	data->pairingslist = subbox;

	TabCtrl_AddTab(data->tabs, "Current Pairings", subbox, 110);

	subbox = Box_Create(388 + 4, TitleBarHeight + 4, 800 - 388 - 120 - 14, dialog->h - 30 - TitleBarHeight - 4, BOX_VISIBLE | BOX_BORDER);
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	subbox->bgcol = TabBG2;
	subbox->brcol = RGB(176,178,183);
	Box_AddChild(dialog, subbox);
	data->chatborder = subbox;
	
	{
		struct Box_s *subbox2;
		subbox2 = Box_Create(0, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersul", "contentcorners.png", 0, 0, 5, 5);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersur", "contentcorners.png", 5, 0, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdr", "contentcorners.png", 5, 5, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(0, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdl", "contentcorners.png", 0, 5, 5, 5);
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);
	}

	subbox = List_Create(5, 5, data->chatborder->w - 10, data->chatborder->h - 10, BOX_VISIBLE, FALSE);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	subbox->bgcol = TabBG2;
	Box_AddChild(data->chatborder, subbox);
	List_SetEntrySelectable(subbox, FALSE);
	List_SetStickyBottom(subbox, 1);
	data->chatlist = subbox;

	subbox = Box_Create(800 - 120 - 5, TitleBarHeight + 4, 120, dialog->h - 30 - TitleBarHeight - 4, BOX_VISIBLE | BOX_BORDER);
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	subbox->bgcol = RGB(180, 184, 165);
	subbox->brcol = RGB(176, 178, 183);
	Box_AddChild(dialog, subbox);
	data->participantborder = subbox;
	
	{
		struct Box_s *subbox2;
		subbox2 = Box_Create(0, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersul", "contentcorners.png", 0, 0, 5, 5);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersur", "contentcorners.png", 5, 0, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdr", "contentcorners.png", 5, 5, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(0, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdl", "contentcorners.png", 0, 5, 5, 5);
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);
	}

	subbox = List_Create(5, 5, data->participantborder->w - 10, data->participantborder->h - 10, BOX_VISIBLE, FALSE);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	List_SetStripeBG1(subbox, RGB(180, 184, 165));
	List_SetStripeBG2(subbox, RGB(180, 184, 165));
	List_SetGroupSelectable(subbox, FALSE);
	List_SetHideDisclosureTriangles(subbox, TRUE);
	List_SetSortGroups(subbox, 0);
	List_SetGroupCollapsible(subbox, FALSE);
	List_AddGroup(subbox, "Players");
	List_AddGroup(subbox, "Observers");
	List_SetEntrySortFunc(subbox, ParticipantEntry_SortFunc);
	List_RedoEntries(subbox);
	Box_AddChild(data->participantborder, subbox);
	data->participantlist = subbox;

	subbox = VSizerBar_Create(data->participantborder->x - 5, data->participantborder->y, 5, data->participantborder->h, data->chatborder, data->participantborder);
	subbox->OnSizeWidth = TourneyBoxLists_OnSizeWidth;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(dialog, subbox);
	data->vsizerbar = subbox;
	
	subbox = Box_Create(388 + 4, dialog->h - 26, 800 - 388 - 8, 22, BOX_VISIBLE | BOX_TRANSPARENT);
	/*subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;*/
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	data->editborder = subbox;
	Box_AddChild(dialog, subbox);

	{
		struct Box_s *subbox2;

		subbox2 = Box_Create(0, 0, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("editborderul", "SearchBorder.png", 0, 0, 6, 6);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(6, 0, subbox->w - 12, 6, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeWidth = Box_OnSizeWidth_Stretch;
		subbox2->img = ImageMgr_GetSubImage("editborderu", "SearchBorder.png", 6, 0, 230 - 12, 6);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 6, 0, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->img = ImageMgr_GetSubImage("editborderur", "SearchBorder.png", 230 - 6, 0, 6, 6);
		Box_AddChild(subbox, subbox2);


		subbox2 = Box_Create(0, 6, 6, subbox->h - 12, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeHeight = Box_OnSizeHeight_Stretch;
		subbox2->img = ImageMgr_GetSubImage("editborderl", "SearchBorder.png", 0, 6, 6, 25 - 12);
		Box_AddChild(subbox, subbox2);
		
		subbox2 = Box_Create(subbox->w - 6, 6, 6, subbox->h - 12, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_Stretch;
		subbox2->img = ImageMgr_GetSubImage("editborderr", "SearchBorder.png", 230 - 6, 6, 6, 25 - 12);
		Box_AddChild(subbox, subbox2);


		subbox2 = Box_Create(0, subbox->h - 6, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		subbox2->img = ImageMgr_GetSubImage("editborderdl", "SearchBorder.png", 0, 25 - 6, 6, 6);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(6, subbox->h - 6, subbox->w - 12, 6, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeWidth = Box_OnSizeWidth_Stretch;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		subbox2->img = ImageMgr_GetSubImage("editborderd", "SearchBorder.png", 6, 25 - 6, 230 - 12, 6);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 6, subbox->h - 6, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		subbox2->img = ImageMgr_GetSubImage("editborderdr", "SearchBorder.png", 230 - 6, 25 - 6, 6, 6);
		Box_AddChild(subbox, subbox2);
	}

	subbox = Edit2Box_Create(388 + 4 + 5, dialog->h - 22 - 2, 800 - 388 - 8 - 10, 18, BOX_VISIBLE, 0);
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	subbox->bgcol = RGB(241, 241, 241);
	subbox->fgcol = RGB(0, 0, 0);
	Edit2Box_SetOnEnter(subbox, TourneyChatBox_OnEnter);
	Edit2Box_SetEditSizeFunc(subbox, TourneyBoxEdit_EditSizeFunc);
	Box_AddChild(dialog, subbox);
	data->edit = subbox;

#if 0
	subbox = List_Create(388 + 4, TitleBarHeight + 4, 660 - 388 - 80 - 14, dialog->h - 30 - TitleBarHeight, BOX_VISIBLE, FALSE);
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	subbox->bgcol = TabBG2;
	Box_AddChild(dialog, subbox);
	List_SetEntrySelectable(subbox, FALSE);
	List_SetStickyBottom(subbox, 1);
	data->chatlist = subbox;

	subbox = List_Create(660 - 80 - 5, TitleBarHeight + 4, 80, dialog->h - 30 - TitleBarHeight, BOX_VISIBLE, FALSE);
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	List_SetStripeBG1(subbox, RGB(180, 184, 165));
	List_SetStripeBG2(subbox, RGB(180, 184, 165));
	List_SetGroupSelectable(subbox, FALSE);
	List_SetHideDisclosureTriangles(subbox, TRUE);
	List_SetGroupCollapsible(subbox, FALSE);
	Box_AddChild(dialog, subbox);
	data->participantlist = subbox;

	subbox = VSizerBar_Create(data->participantlist->x - 5, data->participantlist->y, 5, data->participantlist->h, data->chatlist, data->participantlist);
	subbox->OnSizeWidth = TourneyBoxLists_OnSizeWidth;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(dialog, subbox);
	data->vsizerbar = subbox;
	
	subbox = Edit2Box_Create(388 + 4, dialog->h - 22, 660 - 388 - 8, 18, BOX_VISIBLE, 0);
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	subbox->bgcol = RGB(255, 255, 255);
	subbox->fgcol = RGB(0, 0, 0);
	Edit2Box_SetOnEnter(subbox, TourneyChatBox_OnEnter);
	Edit2Box_SetEditSizeFunc(subbox, TourneyBoxEdit_EditSizeFunc);
	Box_AddChild(dialog, subbox);
	data->edit = subbox;
#endif	
	dialog->boxdata = data;
	dialog->OnCommand = Menu_OnCommand;
	dialog->OnDestroy = TourneyBox_OnDestroy;

	Box_CreateWndCustom(dialog, "Tournament", NULL);

	TabCtrl_ActivateFirst(data->tabs);

	return dialog;
}

void TourneyChatBox_OnEnter(struct Box_s *pbox, char *text)
{
	struct Box_s *tourneybox = Box_GetRoot(pbox);
	struct tourneyboxdata_s *data = tourneybox->boxdata;

	char *tourneyjid;

	if (!text || strlen(text) == 0)
	{
		return;
	}

	tourneyjid = Jid_Strip(data->tourneychatjid);

	/* check for commands */
	if (text && strlen(text) > 0 && text[0] == '/' && strncmp(text, "/me", 3) != 0)
	{
		char *command = strdup(text);
		char *param1 = NULL;
		char *param2 = NULL;
		char *space = command;

		while (*space != '\0' && *space != ' ')
		{
			space++;
		}

		while (*space != '\0' && *space == ' ')
		{
			space++;
		}

		if (*space != '\0' && *(space -1) == ' ')
		{
			*(space - 1) = '\0';
			param1 = space;
		}

		while (*space != '\0' && *space != ' ')
		{
			space++;
		}

		while (*space != '\0' && *space == ' ')
		{
			space++;
		}

		if (*space != '\0' && *(space -1) == ' ')
		{
			*(space - 1) = 0;
			param2 = space;
		}

		/*if (strcmp(command, "/nick") == 0)
		{
			free(tabdata->nick);
			tabdata->nick = strdup(param1);
			Ctrl_ChangeNick(tabdata->targetjid, param1);
		}
		else*/ if (param1 && strcmp(command, "/kick") == 0)
		{
			Ctrl_KickUser(tourneyjid, param1, param2);
		}
		else if (param1 && strcmp(command, "/ban") == 0)
		{
			Ctrl_BanUser(tourneyjid, param1, param2);
		}
		else if (param1 && strcmp(command, "/invite") == 0)
		{
			Ctrl_InviteUser(tourneyjid, param1, param2);
		}

		free(command);

		Edit2Box_ClearText(pbox);
		Box_Repaint(pbox);
		free(tourneyjid);
		return;
	}

	Ctrl_SendGroupMessage(tourneyjid, text);
	
	Edit2Box_ClearText(pbox);
	Box_Repaint(pbox);

	free(tourneyjid);	
}

void TourneyBox_AddChatMessage(struct Box_s *tourneybox, char *name, char *text)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;
	char *nick = Jid_GetResource(data->tourneychatjid);

	SubChat_AddText(data->chatlist, data->participantlist, data->tourneychatjid, name, text, name && stricmp(nick, name) == 0, data->edit);

	free(nick);
}
/*
void TourneyBox_SetProfile(struct Box_s *tourneybox, char *jid, struct namedlist_s *profileinfo)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;
	char *jid1 = Jid_Strip(jid);
	char *jidw = NULL;
	char *jidb = NULL;
	struct namedlist_s **allratingslist = NULL;
	struct namedlist_s **allratings = NULL;
	struct namedlist_s **ratinglist = NULL;
	struct rating_s *rating = NULL;

	char txt[120];
	struct Box_s *ratingbox;

	if (data->whitejid)
	{
		jidw = Jid_Strip(data->whitejid);
	}

	if (data->blackjid)
	{
		jidb = Jid_Strip(data->blackjid);
	}

	if (jidw && stricmp(jid1, jidw) == 0)
	{
		ratingbox = data->whiterating;
	}
	else if (jidb && stricmp(jid1, jidb) == 0)
	{
		ratingbox = data->blackrating;
	}
	else
	{
		free(jid1);
		free(jidw);
		free(jidb);
		return;
	}

	allratingslist = NamedList_GetByName(&profileinfo, "Ratings");
	if (allratingslist)
	{
		allratings = (*allratingslist)->data;
		ratinglist = NamedList_GetByName(allratings, data->category);
		if (ratinglist)
		{
			rating = (*ratinglist)->data;
		}
	}

	if (rating)
	{
		sprintf(txt, "%d", rating->rating);
		Box_SetText(ratingbox, txt);
		Box_Repaint(ratingbox);
	}
	else
	{
		sprintf(txt, "Unrated");
		Box_SetText(ratingbox, txt);
		Box_Repaint(ratingbox);
	}
}
*/

void TourneyBox_SetParticipantStatus(struct Box_s *tourneybox, char *targetjid,
  char *name, enum SStatus status, char *statusmsg, char *role,
  char *affiliation, char *realjid, char *nickchange,
  struct namedlist_s *roleslist, struct namedlist_s *titleslist)
{
	struct Box_s *entrybox;
	struct tourneyboxdata_s *data = tourneybox->boxdata;

	char txt[256];

	if (status == SSTAT_OFFLINE)
	{
		if (nickchange)
		{
			sprintf(txt, "^b^4%s is now known as %s", name, nickchange);
			TourneyBox_AddChatMessage(tourneybox, NULL, txt);

			/* add a temp (fake) entry so we don't show the entering message again */
			entrybox = ParticipantEntry_Create(targetjid, nickchange, SSTAT_AVAILABLE, statusmsg, role, affiliation, realjid, 1, NULL, NULL, 0, NULL);

			if (!List_GetGroupBox(data->participantlist, role))
			{
				List_AddGroup(data->participantlist, role);
			}

			List_AddEntry(data->participantlist, nickchange, role, entrybox);
		}
		else
		{
			sprintf(txt, "^b^4%s has left the room.", name);
			TourneyBox_AddChatMessage(tourneybox, NULL, txt);
		}
	}
	else if (!List_GetEntryBoxAllGroups(data->participantlist, name))
	{
		sprintf(txt, "^b^4%s has entered the room.", name);
		TourneyBox_AddChatMessage(tourneybox, NULL, txt);
	}

	List_RemoveEntryByNameAllGroups(data->participantlist, name);

	if (status == SSTAT_OFFLINE)
        {
		List_RedoEntries(data->participantlist);
		Box_Repaint(data->participantlist);
		return;
	}

	entrybox = ParticipantEntry_Create(targetjid, name, status, statusmsg, role, affiliation, realjid, 1, roleslist, titleslist, 0, NULL);

	if (!List_GetGroupBox(data->participantlist, role))
	{
		List_AddGroup(data->participantlist, role);
	}

	List_AddEntry(data->participantlist, name, role, entrybox);

	List_RedoEntries(data->participantlist);
	Box_Repaint(data->participantlist);
}

void TourneyBox_UpdateNextRoundMsg(struct Box_s *dialog, void *userdata)
{
	struct tourneyboxdata_s *data = dialog->boxdata;
	char *timestamp = NULL;
	struct namedlist_s *currentry = data->info->roundstarttimes;

	while (currentry)
	{
		int entryround;
		sscanf(currentry->name, "%d", &entryround);

		if (entryround == data->info->currentround)
		{
			timestamp = currentry->data;
		}

		currentry = currentry->next;
	}

	if (timestamp)
	{
		char txt[256];
		char timediff[13];

		Info_SecsToText2((float)difftime(Info_ConvertTimestampToTimeT(timestamp) - timezone, time(NULL)), timediff, NULL, 0);

		if (data->info->currentround > 0)
		{
			sprintf(txt, "Round %d ended.  Next round begins in %s", data->info->currentround, timediff);
		}
		else
		{
            sprintf(txt, "First round begins in %s", timediff);
		}

		Text_SetText(data->messagebox, txt);
		Box_Repaint(data->messagebox);
	}
	else
	{
		char txt[256];

		if (data->info->currentround > 0)
		{
			sprintf(txt, "Round %d ended.", data->info->currentround);
			Text_SetText(data->messagebox, txt);
			Box_Repaint(data->messagebox);
		}
	}
}

void TourneyBox_RefreshInfo(struct Box_s *tourneybox)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;
	char txt[512];
	int numplayers = 0, gamesinplay = 0;
	struct namedlist_s *currentry, *pairingslist;

	if (!data->info)
	{
		return;
	}

	currentry = data->info->players;

	List_RemoveAllEntries(data->standingslist);

	if (!currentry)
	{
		struct Box_s *entrybox;

		entrybox = Text_Create(0, 0, data->standingslist->w, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		Text_SetText(entrybox, "No players in this tournament.");

		List_AddEntry(data->standingslist, NULL, NULL, entrybox);
	}

	while (currentry)
	{
		struct Box_s *entrybox, *subbox;
		struct tournamentplayerinfo_s *pinfo = currentry->data;
		char txt[256];

		entrybox = Box_Create(0, 0, data->standingslist->w, 32, BOX_VISIBLE | BOX_TRANSPARENT);
		entrybox->boxdata = Info_DupeTournamentPlayerInfo(pinfo);

		subbox = Text_Create(0, 0, data->standingslist->w, 32, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		sprintf(txt, "^l%s^l\nscore: %.1f (%d wins, %d losses, %d draws) rating: %d\n", Model_GetFriendNick(currentry->name), pinfo->score, pinfo->wins, pinfo->losses, pinfo->draws, pinfo->rating);
		Text_SetText(subbox, txt);
		Text_SetLinkCallback(subbox, 1, Ctrl_ShowProfile2, currentry->name);
		Box_AddChild(entrybox, subbox);

		List_AddEntry(data->standingslist, currentry->name, NULL, entrybox);

		numplayers++;
		currentry = currentry->next;
	}

	List_RemoveAllEntries(data->pairingslist);

	currentry = data->info->roundpairings;
	pairingslist = NULL;

	while (currentry)
	{
		int roundnum;

		if (currentry->name)
		{
			sscanf(currentry->name, "%d", &roundnum);

			if (roundnum == data->info->currentround)
			{
				pairingslist = currentry->data;
			}
		}

		currentry = currentry->next;
	}

	currentry = pairingslist;
	gamesinplay = 0;

	if (!currentry)
	{
		struct Box_s *entrybox;

		entrybox = Text_Create(0, 0, data->pairingslist->w, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		Text_SetText(entrybox, "No pairings.");

		List_AddEntry(data->pairingslist, NULL, NULL, entrybox);
	}

	while (currentry)
	{
		struct Box_s *entrybox, *subbox;
		struct tournamentpairing_s *pairinfo = currentry->data;
		char txt[256];

		entrybox = Box_Create(0, 0, data->standingslist->w, 64, BOX_VISIBLE | BOX_TRANSPARENT);

		subbox = Box_Create(16, 16, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->fgcol = CR_DkOrange;
		Box_SetText(subbox, "v.");
		Box_AddChild(entrybox, subbox);

		subbox = Box_Create(32, 8, 1, 48, BOX_VISIBLE);
		subbox->bgcol = CR_DkOrange;
		Box_AddChild(entrybox, subbox);

		subbox = Text_Create(48, 0, 160, 64, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		Box_AddChild(entrybox, subbox);

		if (pairinfo->white && pairinfo->black)
		{
			struct tournamentplayerinfo_s *whiteinfo = NULL, *blackinfo = NULL;
			struct namedlist_s **playerentry;

			playerentry = NamedList_GetByName(&(data->info->players), pairinfo->white);
			if (playerentry)
			{
				whiteinfo = (*playerentry)->data;
			}

			playerentry = NamedList_GetByName(&(data->info->players), pairinfo->black);
			if (playerentry)
			{
				blackinfo = (*playerentry)->data;
			}

			if (whiteinfo && blackinfo)
			{
				sprintf(txt, "^l%s^l\nscore: %.1f rating: %d\n"
				             "^l%s^l\nscore: %.1f rating: %d\n",
					Model_GetFriendNick(pairinfo->white), whiteinfo->score, whiteinfo->rating,
					Model_GetFriendNick(pairinfo->black), blackinfo->score, blackinfo->rating);
				Text_SetText(subbox, txt);
				Text_SetLinkCallback(subbox, 1, Ctrl_ShowProfile2, pairinfo->white);
				Text_SetLinkCallback(subbox, 2, Ctrl_ShowProfile2, pairinfo->black);
			}
			else
			{
				sprintf(txt, "^l%s^l\nscore: ? rating: ?\n"
				             "^l%s^l\nscore: ? rating: ?",
					Model_GetFriendNick(pairinfo->white), Model_GetFriendNick(pairinfo->black));
				Text_SetText(subbox, txt);
				Text_SetLinkCallback(subbox, 1, Ctrl_ShowProfile2, pairinfo->white);
				Text_SetLinkCallback(subbox, 2, Ctrl_ShowProfile2, pairinfo->black);
			}
		}
		else if (pairinfo->bye)
		{
			struct tournamentplayerinfo_s *byeinfo = NULL;
			struct namedlist_s **playerentry;

			playerentry = NamedList_GetByName(&(data->info->players), pairinfo->bye);

			if (playerentry)
			{
				byeinfo = (*playerentry)->data;
			}

			if (byeinfo)
			{
				sprintf(txt, "^l%s^l\nscore: %.1f rating: %d",
					Model_GetFriendNick(pairinfo->bye), byeinfo->score, byeinfo->rating);
				Text_SetText(subbox, txt);
				Text_SetLinkCallback(subbox, 1, Ctrl_ShowProfile2, pairinfo->bye);
			}
			else
			{
				sprintf(txt, "^l%s^l\nscore: ? rating: ?",
					Model_GetFriendNick(pairinfo->bye));
				Text_SetText(subbox, txt);
				Text_SetLinkCallback(subbox, 1, Ctrl_ShowProfile2, pairinfo->bye);
			}
		}

		subbox = Box_Create(208, 0, 1, 64, BOX_VISIBLE);
		subbox->bgcol = CR_Gray;
		Box_AddChild(entrybox, subbox);

		subbox = Text_Create(224, 8, data->standingslist->w - 224, 48, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP);

		if (pairinfo->bye)
		{
			Text_SetText(subbox, "Got a bye.");
		}
		else if (pairinfo->gamestate == 0)
		{
			Text_SetText(subbox, "Game not yet started.");
		}
		else if (pairinfo->gamestate == 1)
		{
			sprintf(txt, "Game in progress. ^lWatch game %s.^l", pairinfo->gameid);
			gamesinplay++;
			Text_SetText(subbox, txt);
			Text_SetLinkCallback(subbox, 1, Ctrl_WatchGame2, pairinfo->gameid);
		}
		else if (pairinfo->gamestate == 2)
		{
			if (stricmp(pairinfo->winner, "white") == 0)
			{
				sprintf(txt, "Game Over.  Winner is %s.", Model_GetFriendNick(pairinfo->white));
			}
			else if (stricmp(pairinfo->winner, "black") == 0)
			{
				sprintf(txt, "Game Over.  Winner is %s.", Model_GetFriendNick(pairinfo->black));
			}
			else
			{
				sprintf(txt, "Game Over.  Draw.");
			}
			Text_SetText(subbox, txt);
		}

		Box_AddChild(entrybox, subbox);

		List_AddEntry(data->pairingslist, NULL, NULL, entrybox);

		currentry = currentry->next;
	}

	if (data->info->currentround)
	{
		sprintf(txt, 
			"^3^b%s^n^4  %s, %s | %s pairing\n"
			"Round %d of %d, %d games in play, %d players",
			data->info->name, data->info->variant, data->info->timecontrol,
			data->info->pairingtype, data->info->currentround,
			data->info->totalrounds, gamesinplay, numplayers);
	}
	else
	{
		sprintf(txt, 
			"^3^b%s^n^4  %s, %s | %s pairing\n"
			"Waiting for players, %d games in play, %d players",
			data->info->name, data->info->variant, data->info->timecontrol,
			data->info->pairingtype, gamesinplay, numplayers);
	}

	Text_SetText(data->tourneyinfotextbox1, txt);

	sprintf(txt, "%s - Tournament Room", data->info->name);

	TitleBar_SetText(tourneybox->titlebar, txt);
	SetWindowText(tourneybox->hwnd, txt);

	Box_Repaint(data->tourneyinfotextbox1);

	Box_RemoveTimedFunc(tourneybox, TourneyBox_UpdateNextRoundMsg, 1000);

	if (data->tournamentover)
	{
		if (data->winner)
		{
			sprintf(txt, "Tournament over! %s has won!", Model_GetFriendNick(data->winner));
		}
		else
		{
			sprintf(txt, "Tournament over! No winner.");
		}
		Text_SetText(data->messagebox, txt);
		Box_Repaint(data->messagebox);
	}
	else if (!data->roundinplay)
	{
		Box_AddTimedFunc(tourneybox, TourneyBox_UpdateNextRoundMsg, NULL, 1000);
	}
	else
	{
		sprintf(txt, "Round %d has begun!", data->info->currentround);
		Text_SetText(data->messagebox, txt);
		Box_Repaint(data->messagebox);
	}

	List_RedoEntries(data->standingslist);
	List_RedoEntries(data->pairingslist);
	Box_Repaint(data->standingslist);
	Box_Repaint(data->pairingslist);

}


void TourneyBox_SetTournamentInfo(struct Box_s *tourneybox, struct tournamentinfo_s *info)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;

	data->info = Info_DupeTournamentInfo(info);

	TourneyBox_RefreshInfo(tourneybox);
}

void TourneyBox_AddPlayer(struct Box_s *tourneybox, char *playerjid)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;
	char *bareplayerjid;
	struct tournamentplayerinfo_s *pinfo;

	if (!data->info)
	{
		return;
	}

	bareplayerjid = Jid_Strip(playerjid);

	if (NamedList_GetByName(&(data->info->players), bareplayerjid))
	{
		free(bareplayerjid);
		return;
	}

	pinfo = malloc(sizeof(*pinfo));

	memset(pinfo, 0, sizeof(*pinfo));

	NamedList_Add(&(data->info->players), bareplayerjid, pinfo, Info_DestroyTournamentPlayerInfo);
	free(bareplayerjid);

	{
		char txt[256];

		sprintf(txt, "^b^4%s has joined the tournament.", Model_GetFriendNick(playerjid));
		TourneyBox_AddChatMessage(tourneybox, NULL, txt);
	}

	TourneyBox_RefreshInfo(tourneybox);
}

void TourneyBox_RemovePlayer(struct Box_s *tourneybox, char *playerjid)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;
	char *bareplayerjid;

	if (!data->info)
	{
		return;
	}

	bareplayerjid = Jid_Strip(playerjid);

	if (!NamedList_GetByName(&(data->info->players), bareplayerjid))
	{
		free(bareplayerjid);
		return;
	}

	NamedList_RemoveByName(&(data->info->players), bareplayerjid);
	free(bareplayerjid);

	{
		char txt[256];

		if (data->info && data->info->currentround != 0)
		{
			sprintf(txt, "^b^4%s has forfeited the tournament.", Model_GetFriendNick(playerjid));
		}
		else
		{
			sprintf(txt, "^b^4%s has left the tournament.", Model_GetFriendNick(playerjid));
		}

		TourneyBox_AddChatMessage(tourneybox, NULL, txt);
	}

	TourneyBox_RefreshInfo(tourneybox);
}

void TourneyBox_StartRound(struct Box_s *tourneybox, int round, struct namedlist_s *pairinglist)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;
	char roundtxt[64];
	struct namedlist_s *entry, *dupepairinglist = NULL;

	if (!data->info)
	{
		return;
	}

	data->roundinplay = 1;
	data->info->currentround = round;

	sprintf(roundtxt, "%d", round);

	NamedList_RemoveByName(&(data->info->roundpairings), roundtxt);

	entry = pairinglist;
	while (entry)
	{
		NamedList_Add(&(dupepairinglist), entry->name, Info_DupeTournamentPairing(entry->data), Info_DestroyTournamentPairing);
		entry = entry->next;
	}

	NamedList_Add(&(data->info->roundpairings), roundtxt, dupepairinglist, NamedList_Destroy2);

	{
		char txt[256];

		sprintf(txt, "^b^4Round %d has begun!", round);
		TourneyBox_AddChatMessage(tourneybox, NULL, txt);
	}

	TourneyBox_RefreshInfo(tourneybox);
}

void TourneyBox_EndRound(struct Box_s *tourneybox, int round)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;

	if (!data->info)
	{
		return;
	}

	data->roundinplay = 0;
	data->info->currentround = round;

	{
		char txt[256];

		sprintf(txt, "^b^4Round %d has ended.", round);
		TourneyBox_AddChatMessage(tourneybox, NULL, txt);
	}

	TourneyBox_RefreshInfo(tourneybox);
}

void TourneyBox_End(struct Box_s *tourneybox, char *winner)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;

	if (!data->info)
	{
		return;
	}

	data->tournamentover = 1;
	data->winner = strdup(winner);

	{
		char txt[256];

		sprintf(txt, "Tournament is over, winner is %s!", Model_GetFriendNick(winner));
		TourneyBox_AddChatMessage(tourneybox, NULL, txt);
	}

	TourneyBox_RefreshInfo(tourneybox);
}

void TourneyBox_GamePlaying(struct Box_s *tourneybox, char *gameid, int round, char *white, char *black)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;
	char roundtxt[64];
	struct namedlist_s **entry, *entry2;
	struct tournamentpairing_s *pairing = NULL;

	if (!data->info)
	{
		return;
	}

	sprintf(roundtxt, "%d", round);

	entry = NamedList_GetByName(&(data->info->roundpairings), roundtxt);

	if (!entry || !*entry)
	{
		return;
	}

	entry2 = (*entry)->data;

	while (entry2)
	{
		struct tournamentpairing_s *currpair;
		currpair = entry2->data;

		if ( (currpair->white && stricmp(white, currpair->white) == 0)
		  && (currpair->black && stricmp(black, currpair->black) == 0))
		{
			pairing = currpair;
		}

		entry2 = entry2->next;
	}

	if (!pairing)
	{
		return;
	}

	pairing->gameid = strdup(gameid);
	pairing->gamestate = 1;

	TourneyBox_RefreshInfo(tourneybox);
}

void TourneyBox_GameStoppedPlaying(struct Box_s *tourneybox, char *gameid, int round, char *white, char *black, char *winner)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;
	char roundtxt[64];
	struct namedlist_s **entry, *entry2;
	struct tournamentpairing_s *pairing = NULL;

	if (!data->info)
	{
		return;
	}

	sprintf(roundtxt, "%d", round);

	entry = NamedList_GetByName(&(data->info->roundpairings), roundtxt);

	if (!entry || !*entry)
	{
		return;
	}

	entry2 = (*entry)->data;

	while (entry2)
	{
		struct tournamentpairing_s *currpair;
		currpair = entry2->data;

		if ( (currpair->white && stricmp(white, currpair->white) == 0)
		  && (currpair->black && stricmp(black, currpair->black) == 0))
		{
			pairing = currpair;
		}

		entry2 = entry2->next;
	}

	if (!pairing)
	{
		return;
	}

	pairing->gameid = strdup(gameid);
	pairing->gamestate = 2;
	pairing->winner = strdup(winner);

	TourneyBox_RefreshInfo(tourneybox);
}

void TourneyBox_UpdatePlayer(struct Box_s *tourneybox, char *jid, struct tournamentplayerinfo_s *pinfo)
{
	struct tourneyboxdata_s *data = tourneybox->boxdata;

	if (!data->info)
	{
		return;
	}

	NamedList_RemoveByName(&(data->info->players), jid);
	NamedList_Add(&(data->info->players), jid, Info_DupeTournamentPlayerInfo(pinfo), Info_DestroyTournamentPlayerInfo);

	TourneyBox_RefreshInfo(tourneybox);
}