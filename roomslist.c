#include <stdlib.h>
#include <stdio.h>

#include "box.h"

#include "button.h"
#include "button2.h"
#include "edit.h"

#include "constants.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "imagemgr.h"
#include "list.h"
#include "menu.h"
#include "roomentry.h"
#include "text.h"
#include "view.h"

#include "info.h"

struct roomslistdata_s
{
	int lastupdate;
	int updating;
	struct Box_s *edit;
	struct Box_s *clear;
	struct Box_s *list;
	struct Box_s *error;
	struct Box_s *refresh;
	struct Box_s *refreshtext;
};

void RoomsList_UpdateLastUpdatedTime(struct Box_s *roomslist_box, void *userdata);
void RoomslistEdit_RefreshList(struct Box_s *pbox);
void RoomsList_RefreshTimeout(struct Box_s *roomslist_box, void *userdata);

void RoomsList_OnDestroy(struct Box_s *roomslist_box)
{
	Box_RemoveTimedFunc(roomslist_box, RoomsList_UpdateLastUpdatedTime, 1000);
	Box_RemoveTimedFunc(roomslist_box, RoomsList_RefreshTimeout, 20000);
}


void RoomslistClear_ClearEdit(struct Box_s *pbox)
{
	struct Box_s *roomslist_box = pbox->parent->parent;
	struct roomslistdata_s *data = roomslist_box->boxdata;

	Edit2Box_ClearText(data->edit);
	RoomslistEdit_RefreshList(data->edit);
	pbox->flags &= ~BOX_VISIBLE;
}

void RoomsList_RefreshGroupCount(struct Box_s *pbox, char *groupname)
{
	struct roomslistdata_s *data = pbox->boxdata;
	struct Box_s *groupbox;
	char txt[256];
	int users, rooms;
	char *roomstring, *userstring;

	users = 0;
	rooms = 0;

	List_CallEntryFuncOnGroup(data->list, groupname, RoomEntry_CountUsers, &users);
	List_CallEntryFuncOnGroup(data->list, groupname, RoomEntry_CountRooms, &rooms);

	if (users == 1)
	{
		userstring = _("user");
	}
	else
	{
		userstring = _("users");
	}

	if (rooms == 1)
	{
		roomstring = _("room");
	}
	else
	{
		roomstring = _("rooms");
	}

	sprintf(txt, "%s - %d %s, %d %s", groupname, rooms, roomstring, users, userstring);

	groupbox = List_GetGroupBox(data->list, groupname);
	if (groupbox)
	{
		Box_SetText(groupbox, txt);
		Box_Repaint(groupbox);
	}
}


void RoomsList_RefreshCounts(struct Box_s *pbox)
{
	RoomsList_RefreshGroupCount(pbox, _("Favorites"));
/*	RoomsList_RefreshGroupCount(pbox, "Chesspark Help & Technical Support");*/
	RoomsList_RefreshGroupCount(pbox, _("Public Chat Rooms"));
}


void RoomslistEdit_RefreshList(struct Box_s *pbox)
{
	struct Box_s *roomslist_box = pbox->parent->parent;
	struct roomslistdata_s *data = roomslist_box->boxdata;
	char *edittext = Edit2Box_GetText(pbox);

	RoomsList_RefreshCounts(roomslist_box);

	List_RedoEntries(data->list);

	if (edittext && strlen(edittext) > 0)
	{
		Button_SetOnButtonHit(data->clear, RoomslistClear_ClearEdit);
		data->clear->flags |= BOX_VISIBLE;
	}
	else
	{
		Button_SetOnButtonHit(data->clear, NULL);
		data->clear->flags &= ~BOX_VISIBLE;
	}

	if (List_HasVisibleEntry(data->list))
	{
		data->list->flags |= BOX_VISIBLE;
		data->error->flags &= ~BOX_VISIBLE;
	}
	else
	{
		if (!data->updating)
		{
			data->list->flags &= ~BOX_VISIBLE;
			data->error->flags |= BOX_VISIBLE;
		}
		else
		{
			data->list->flags |= BOX_VISIBLE;
			data->error->flags &= ~BOX_VISIBLE;
		}
	}

	Box_Repaint(data->list);
}

void RoomsList_RefreshTimeout(struct Box_s *roomslist_box, void *userdata)
{
	struct roomslistdata_s *data = roomslist_box->boxdata;

	Box_RemoveTimedFunc(roomslist_box, RoomsList_RefreshTimeout, 20000);

	Box_SetText(data->refreshtext, _("Refresh timed out!"));

	Box_Repaint(data->refreshtext);
}

void RoomslistRefreshButton_OnHit(struct Box_s *pbox)
{
	Ctrl_GetRooms();
}

void RoomslistErrorLink_OnClearSearch(struct Box_s *pbox, struct Box_s *roomslist_box)
{
	struct roomslistdata_s *data = roomslist_box->boxdata;

	RoomslistClear_ClearEdit(data->clear);
}

void RoomslistErrorLink_OnStartNewChat(struct Box_s *pbox, void *userdata)
{
	View_PopupSimpleDialog("joinchat_box", NULL);
}


void RoomsList_OnGroupDragDrop(struct Box_s *pdst, struct Box_s *psrc, int id, char *roomname, char *dropname)
{
	if (id == ROOMDROPDATA_ID)
	{
		if (stricmp(dropname, "Favorites") == 0)
		{
			Ctrl_AddChatToFavorites(roomname);
		}
	}
}


struct Box_s *RoomsList_Create()
{
	struct Box_s *toppanel, *subbox, *subbox2;
	struct roomslistdata_s *data;
	struct Box_s *roomslist_box = Box_Create(0, 0, 100, 100, BOX_TRANSPARENT);
	roomslist_box->OnSizeWidth = Box_OnSizeWidth_Stretch;
	roomslist_box->OnSizeHeight = Box_OnSizeHeight_Stretch;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	toppanel = Box_Create(0, 0, 100, 58, BOX_VISIBLE);
	toppanel->OnSizeWidth = Box_OnSizeWidth_Stretch;
	toppanel->bgcol = TabBG2;
	toppanel->fgcol = TabFG2;
	Box_AddChild(roomslist_box, toppanel);

	subbox = Box_Create(5, 4, 6, 25, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetSubImage("SearchBorderL", "SearchBorder.png", 0, 0, 6, 25);
	Box_AddChild(toppanel, subbox);

	subbox = Box_Create(11, 4, toppanel->w - 5 * 2 - 6 * 2, 25, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
	subbox->img = ImageMgr_GetSubImage("SearchBorderC", "SearchBorder.png", 6, 0, 218, 25);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(toppanel, subbox);

	subbox = Box_Create(toppanel->w - 5 - 6, 4, 6, 25, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetSubImage("SearchBorderR", "SearchBorder.png", 224, 0, 6, 25);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	Box_AddChild(toppanel, subbox);
/*
	subbox = Box_Create(5, 4, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->bgcol = RGB(255, 255, 255);
	Box_AddChild(toppanel, subbox);
*/
	subbox = Box_Create(13, 12, 12, 12, BOX_VISIBLE);
	subbox->img = ImageMgr_GetImage("searchIcon.png");
	subbox->bgcol = RGB(241, 241, 241);
	Box_AddChild(toppanel, subbox);
		
	subbox = Edit2Box_Create(32, 8, 100 - 64, 20, BOX_VISIBLE, 1);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->bgcol = RGB(241, 241, 241);
	Edit2Box_SetOnKey(subbox, RoomslistEdit_RefreshList);
	Box_AddChild(toppanel, subbox);
	data->edit = subbox;

	subbox = Button_Create(95 - 13 - 2, (25 - 12) / 2 + 4, 13, 13, 0);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	subbox->bgcol = RGB(241, 241, 241);
	subbox->img = ImageMgr_GetSubImage("DeleteIcon1", "DeleteIcon.png", 0, 0, 13, 13);
	Button_SetNormalImg(subbox, ImageMgr_GetSubImage("DeleteIcon1", "DeleteIcon.png", 0, 0, 13, 13));
	Button_SetHoverImg(subbox, ImageMgr_GetSubImage("DeleteIcon2", "DeleteIcon.png", 13, 0, 13, 13));
	Button_SetPressedImg(subbox, ImageMgr_GetSubImage("DeleteIcon2", "DeleteIcon.png", 13, 0, 13, 13));
	Button_SetTooltipText(subbox, _("Delete"));
	Box_AddChild(toppanel, subbox);
	data->clear = subbox;

	subbox = Button2_Create(5, 32, 90, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(toppanel, subbox);
	Button2_SetOnButtonHit(subbox, RoomslistRefreshButton_OnHit);
	data->refresh = subbox;

	subbox = Box_Create(5, 32, 90, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(toppanel, subbox);

	subbox2 = Box_Create(30, 2, 12, 14, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("spinbutton1", "spinbutton.png", 0, 0, 12, 14);
	Box_AddChild(subbox, subbox2);

	Button2_SetNormal(data->refresh, subbox);

	subbox = Box_Create(5, 32, 90, 18, BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(toppanel, subbox);

	subbox2 = Box_Create(25, 0, 7, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonL2", "roomrefreshbutton.png", 0, 18, 7, 18);
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(32, 0, 100 - 30 * 2 - 7 * 2, 18, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonC2", "roomrefreshbutton.png", 7, 18, 156, 18);
	subbox2->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(95 - 30 - 7, 0, 7, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonR2", "roomrefreshbutton.png", 163, 18, 7, 18);
	subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(30, 2, 12, 14, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("spinbutton2", "spinbutton.png", 12, 0, 12, 14);
	Box_AddChild(subbox, subbox2);

	Button2_SetPressed(data->refresh, subbox);

	subbox = Box_Create(5, 32, 90, 18,  BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(toppanel, subbox);

	subbox2 = Box_Create(25, 0, 7, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonL3", "roomrefreshbutton.png", 0, 36, 7, 18);
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(32, 0, 100 - 30 * 2 - 7 * 2, 18, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonC3", "roomrefreshbutton.png", 7, 36, 156, 18);
	subbox2->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(95 - 30 - 7, 0, 7, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("roomrefreshbuttonR3", "roomrefreshbutton.png", 163, 36, 7, 18);
	subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
	Box_AddChild(subbox, subbox2);

	subbox2 = Box_Create(30, 2, 12, 14, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox2->img = ImageMgr_GetSubImage("spinbutton3", "spinbutton.png", 24, 0, 12, 14);
	Box_AddChild(subbox, subbox2);

	Button2_SetHover(data->refresh, subbox);

	subbox = Box_Create(5, 32, 90, 18, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->fgcol = TabFG1;
	Box_SetText(subbox, _("Rooms list not loaded"));
	Box_AddChild(toppanel, subbox);
	data->refreshtext = subbox;

	subbox = Box_Create(5, 53, 90, 2, BOX_VISIBLE | BOX_TILEIMAGE);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->img = ImageMgr_GetImage("Horizrule.png");
	Box_AddChild(toppanel, subbox);

	subbox = List_Create(0, 58, 100, 100-58, BOX_VISIBLE, FALSE);
	subbox->bgcol = TabBG2;
	subbox->fgcol = TabFG2;
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	List_AddGroup(subbox, _("Favorites"));
/*	List_AddGroup(subbox, "Chesspark Help & Technical Support");*/
	List_AddGroup(subbox, _("Public Chat Rooms"));
	List_SetEntryVisibleFunc(subbox, RoomEntry_VisibleFunc);
	List_SetEmptyRClickFunc(subbox, Menu_PopupRoomsBlankMenu);
	List_SetGroupSelectable(subbox, FALSE);
	List_SetEntrySortFunc(subbox, RoomEntry_SortFunc);
	List_SetOnGroupDragDrop(subbox, RoomsList_OnGroupDragDrop);
	List_RedoEntries(subbox);
	Box_AddChild(roomslist_box, subbox);
	data->list = subbox;

	subbox = Text_Create(0, 58, 100, 100-58, 0, TX_CENTERED | TX_WRAP);
	subbox->bgcol = TabBG2;
	subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(roomslist_box, subbox);
	Text_SetText(subbox, _("Sorry, no public chat rooms match what you've entered.\n\n^lClear your search^l\nor\n^lStart a new chat room"));
	Text_SetLinkCallback(subbox, 1, RoomslistErrorLink_OnClearSearch, roomslist_box);
	Text_SetLinkCallback(subbox, 2, RoomslistErrorLink_OnStartNewChat, NULL);
	data->error = subbox;

	roomslist_box->boxdata = data;
	roomslist_box->OnDestroy = RoomsList_OnDestroy;

	return roomslist_box;
}

void RoomsList_AddChatroom(struct Box_s *roomslist_box, char *group, char *jid, char *name, char *topic, int users)
{
	struct roomslistdata_s *rldata = roomslist_box->boxdata;
	struct Box_s *entrybox = RoomEntry_Create(0, 0, roomslist_box->w, 40, BOX_VISIBLE | BOX_TRANSPARENT, rldata->edit, jid, name, topic, users);

	if (stricmp(group, _("Favorites")) != 0)
	{
	       Box_RemoveTimedFunc(roomslist_box, RoomsList_RefreshTimeout, 20000);
	}

	List_RemoveEntryByName(rldata->list, jid, group);
	List_AddEntry(rldata->list, jid, group, entrybox);
	RoomsList_RefreshCounts(roomslist_box);
	List_RedoEntries(rldata->list);
	Box_Repaint(rldata->list);
}

void RoomsList_RemoveChatroom(struct Box_s *roomslist_box, char *group, char *jid)
{
	struct roomslistdata_s *rldata = roomslist_box->boxdata;

	List_RemoveEntryByName(rldata->list, jid, group);
	RoomsList_RefreshCounts(roomslist_box);
	List_RedoEntries(rldata->list);
	Box_Repaint(rldata->list);
}

void RoomsList_UpdateLastUpdatedTime(struct Box_s *roomslist_box, void *userdata)
{
	struct roomslistdata_s *data = roomslist_box->boxdata;
	char txt[120];

	data->lastupdate++;
	{
		char updatedtxt[80];
		char buffer[120];

		i18n_stringsub(updatedtxt, 256, _("Updated %1 ago"), Info_SecsToTextShort(buffer, 120, (float)(data->lastupdate)));
		sprintf(txt, "%s - %s", _("Refresh"), updatedtxt);
	}

	Box_SetText(data->refreshtext, txt);

	Box_Repaint(data->refreshtext);
}

void RoomsList_ClearChatRooms(struct Box_s *roomslist_box)
{
	struct roomslistdata_s *data = roomslist_box->boxdata;

	List_RemoveAllEntries(data->list);
	List_AddGroup(data->list, _("Favorites"));
/*	List_AddGroup(data->list, "Chesspark Help & Technical Support");*/
	List_AddGroup(data->list, _("Public Chat Rooms"));
	
	Box_RemoveTimedFunc(roomslist_box, RoomsList_UpdateLastUpdatedTime, 1000);
	Box_SetText(data->refreshtext, _("Refreshing..."));
	data->updating = 1;
	Box_RemoveTimedFunc(roomslist_box, RoomsList_RefreshTimeout, 20000);
	Box_AddTimedFunc(roomslist_box, RoomsList_RefreshTimeout, NULL, 20000);

	List_RedoEntries(data->list);
	Box_Repaint(data->list);

	RoomslistEdit_RefreshList(data->edit);
}

void RoomsList_SetChatroomLoadStatusFinished(struct Box_s *roomslist_box)
{
	struct roomslistdata_s *data = roomslist_box->boxdata;

	Box_RemoveTimedFunc(roomslist_box, RoomsList_RefreshTimeout, 20000);

	Box_SetText(data->refreshtext, _("Finished loading chatrooms"));

	data->lastupdate = 0;
	data->updating = 0;
	Box_AddTimedFunc(roomslist_box, RoomsList_UpdateLastUpdatedTime, 0, 1000);
	RoomslistEdit_RefreshList(data->edit);
}
