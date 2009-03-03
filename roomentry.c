#include <stdio.h>
#include <stdlib.h>

#include "box.h"

#include "dragbox.h"
#include "edit.h"
#include "text.h"

#include "constants.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "menu.h"
#include "util.h"

#include "roomentry.h"

struct roomentrydata_s
{
	struct Box_s *edit;
	char *jid;
	char *searchtext;
	int users;
};

BOOL RoomEntry_VisibleFunc(struct Box_s *pbox)
{
	struct roomentrydata_s *data = pbox->boxdata;
	char *searchtext = Edit2Box_GetText(data->edit);
	
	if (!searchtext || strlen(searchtext) == 0 || strstr(data->searchtext, searchtext))
	{
		return TRUE;
	}
	return FALSE;
}

void RoomEntry_CountUsers(struct Box_s *pbox, int *users)
{
	struct roomentrydata_s *data = pbox->boxdata;
	if (RoomEntry_VisibleFunc(pbox) && data->users > 0)
	{
		*users += data->users;
	}
}

void RoomEntry_CountRooms(struct Box_s *pbox, int *rooms)
{
	struct roomentrydata_s *data = pbox->boxdata;
	if (RoomEntry_VisibleFunc(pbox))
	{
		*rooms += 1;
	}
}

void RoomEntry_OnRButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct roomentrydata_s *data = pbox->boxdata;
	int x, y;

	if (xmouse > 0 && ymouse > 0 && xmouse < pbox->w && ymouse < pbox->h)
	{
		Box_GetScreenCoords(pbox, &x, &y);
		x += xmouse;
		y += ymouse;

		Menu_PopupRoomMenu(pbox, data->jid, x, y);
	}
}

void RoomEntry_OnLButtonDblClk(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct roomentrydata_s *data = pbox->boxdata;
	Ctrl_JoinGroupChatDefaultNick(data->jid);
	/*View_PopupChatDialog(data->jid, Ctrl_GetDefaultNick(), 1);*/
}

int RoomEntry_SortFunc(struct Box_s *lbox, struct Box_s *gbox)
{
	struct roomentrydata_s *ldata = lbox->boxdata;
	struct roomentrydata_s *gdata = gbox->boxdata;

	return stricmp(ldata->searchtext, gdata->searchtext) < 0;
}
#if 0
void RoomEntryDrag_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct roomentrydata_s *data = pbox->parent->boxdata;
	struct Box_s *root = Box_GetRoot(pbox);

	Box_OnLButtonUp(pbox, xmouse, ymouse);

	if (DragBox_GetDragState(pbox) == DRAG_DRAGGING)
	{
		if (root->OnDragDrop)
		{
			int x, y;

			Box_GetRootCoords(pbox, &x, &y);
			x += xmouse;
			y += ymouse;
	
			root->OnDragDrop(root, x, y, ROOMDROPDATA_ID, strdup(data->jid));
		}
	}

	DragBox_OnLButtonUp(pbox, xmouse, ymouse);
}
#endif

struct Box_s *RoomEntry_Create(int x, int y, int w, int h, enum Box_flags flags, struct Box_s *edit, char *jid, char *name, char *topic, int users)
{
	struct Box_s *entrybox = Box_Create(x, y, w, h, flags);
	struct Box_s *dragbox;
	struct Box_s *subbox;
	struct roomentrydata_s *data = malloc(sizeof(*data));
	
	char txt[80];
	char unescaped[1024];
	char *userstring;

	entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	entrybox->OnLButtonDblClk = RoomEntry_OnLButtonDblClk;
	entrybox->OnRButtonUp = RoomEntry_OnRButtonUp;

	if (!name)
	{
		if (strstr(jid, "@chat.chesspark.com"))
		{
			name = Jid_GetBeforeAt(jid);
		}
		else
		{
			name = jid;
		}
	}

	if (users == -1)
	{
		sprintf(txt, "^b%s^n", UnescapeJID(name, unescaped, 1024));
	}
	else
	{
		if (users == 1)
		{
			userstring = _("user");
		}
		else
		{
			userstring = _("users");
		}

		sprintf(txt, "^b%s^n^0 - %d %s", UnescapeJID(name, unescaped, 1024), users, userstring);
	}

	dragbox = DragBox_Create(0, 0, w, h, BOX_VISIBLE | BOX_TRANSPARENT, ROOMDROPDATA_ID, strdup(jid), NULL);
	Box_AddChild(entrybox, dragbox);

	if (topic && strlen(topic) > 0)
	{
		subbox = Text_Create(16, 5, w - 16, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		subbox->fgcol = TabFG1;
		Text_SetText(subbox, txt);
		Box_AddChild(dragbox, subbox);

		subbox = Box_Create(16, 23, w - 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		Box_SetText(subbox, topic);
		Box_AddChild(dragbox, subbox);
	}
	else
	{
		subbox = Text_Create(16, (40 - 12) / 2, w - 16, 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		subbox->fgcol = TabFG1;
		Text_SetText(subbox, txt);
		Box_AddChild(dragbox, subbox);
	}

	memset(data, 0, sizeof(*data));
	if (topic)
	{
		data->searchtext = malloc(strlen(unescaped) + strlen(topic) + 2);
	}
	else
	{
		data->searchtext = malloc(strlen(unescaped));
	}
	strcpy(data->searchtext, unescaped);
	if (topic)
	{
		strcat(data->searchtext, topic);
	}
	data->edit = edit;
	data->jid = strdup(jid);
	data->users = users;
	entrybox->boxdata = data;

	/*dragbox->OnLButtonUp = RoomEntryDrag_OnLButtonUp;*/

	return entrybox;
}