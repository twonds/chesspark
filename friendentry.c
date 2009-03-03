#include <stdio.h>
#include <stdlib.h>

#include "box.h"

#include "dragbox.h"
#include "text.h"

#include "constants.h"
#include "ctrl.h"
#include "i18n.h"
#include "imagemgr.h"
#include "info.h"
#include "mem.h"
#include "menu.h"
#include "model.h"
#include "namedlist.h"
#include "tooltip.h"
#include "util.h"
#include "view.h"

#include "friendentry.h"

void FriendEntry_Update(struct Box_s *pbox);

/* bit of a hack here for universally hiding avatars */
static int hideavatar = 0;
static int showoffline = 0;

struct friendentrydata_s
{
	char *jid;
	char *nickname;
	char *groupname;
	char *avatarhash;
	enum SStatus status;
	char *statusmsg;
	char *rating;
	struct namedlist_s *roles;
	struct namedlist_s *titles;
	struct namedlist_s *playinggamelist;
	struct namedlist_s *watchinggamelist;
	struct adhoccommand_s *command;
};



void FriendEntry_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct friendentrydata_s *data = pbox->boxdata;

	Box_OnMouseMove(pbox, xmouse, ymouse);

	if (Box_CheckXYInBox(pbox, xmouse, ymouse)/* && Box_GetRoot(pbox)->active*/) 
	{
		if (!pbox->tooltipvisible)
		{
			char txt[1024];
		
			strcpy(txt, data->jid);

			if (data->nickname)
			{
				strcat(txt, _("\nNickname: "));
				strcat(txt, data->nickname);
			}

			strcat(txt, _("\nStatus: "));

			switch (data->status)
			{
				case SSTAT_AVAILABLE:
				{
					strcat(txt, _("Online"));
				}
				break;

				case SSTAT_AWAY:
				{
					strcat(txt, _("Away"));
				}
				break;

				case SSTAT_OFFLINE:
				{
					strcat(txt, _("Offline"));
				}
				break;

				case SSTAT_PENDING:
				{
					strcat(txt, _("Pending Approval"));
				}
				break;
			}

			if (data->statusmsg)
			{
				strcat(txt, " - ");
				strcat(txt, data->statusmsg);
			}

			Box_AddTimedFunc(pbox, ToolTip_Popup, strdup(txt), 1000);
			pbox->tooltipvisible = 1;
		}
	}
	else
	{
		ToolTip_PopDown(pbox);
		pbox->tooltipvisible = 0;
	}
}

#if 0
void FriendEntryDrag_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct friendentrydata_s *data = pbox->parent->boxdata;
	struct Box_s *root = Box_GetRoot(pbox);

	Box_OnLButtonUp(pbox, xmouse, ymouse);

	if (DragBox_GetDragState(pbox) == DRAG_DRAGGING)
	{
		if (root->OnDragDrop)
		{
			struct FriendDropData_s *frienddropdata = malloc(sizeof(*frienddropdata));
			int x, y;

			Box_GetRootCoords(pbox, &x, &y);
			x += xmouse;
			y += ymouse;

			frienddropdata->jid = data->jid;
			frienddropdata->groupname = data->groupname;
	
			root->OnDragDrop(root, x, y, FRIENDDROPDATA_ID, frienddropdata);
		}
	}

	DragBox_OnLButtonUp(pbox, xmouse, ymouse);
}
#endif

void FriendEntry_OnRButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct friendentrydata_s *data = pbox->boxdata;
	int x, y;

	if (xmouse > 0 && ymouse > 0 && xmouse < pbox->w && ymouse < pbox->h)
	{
		char *gameid = NULL;
		Box_GetScreenCoords(pbox, &x, &y);
		x += xmouse;
		y += ymouse;

		if (data->playinggamelist)
		{
			gameid = data->playinggamelist->name;
		}
		else if (data->watchinggamelist)
		{
			gameid = data->watchinggamelist->name;
		}

		Menu_PopupListMenu(pbox, data->jid, data->groupname, x, y,
			data->status != SSTAT_PENDING && data->status != SSTAT_DENIED,
			data->status != SSTAT_OFFLINE, data->rating != NULL,
			data->playinggamelist, data->watchinggamelist, data->command);
	}
}


void FriendEntry_OnLButtonDblClk(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct friendentrydata_s *data = pbox->boxdata;
	
	Model_PopupChatDialog(data->jid, 0);
}

void FriendEntry_OnDestroy(struct Box_s *entry)
{
	struct friendentrydata_s *data = entry->boxdata;

#if 0
	Mem_RemoveMemRec(entry);
#endif
	ToolTipParent_OnDestroy(entry);

	free(data->jid);
	free(data->nickname);
	free(data->groupname);
	free(data->statusmsg);
	NamedList_Destroy2(data->playinggamelist);
	NamedList_Destroy2(data->watchinggamelist);
}

struct Box_s *FriendEntry_CreateReal(int x, int y, int w, int h, char *jid, char *nickname, char *groupname, enum SStatus status, char *statusmsg)
{
	struct Box_s *entrybox = Box_Create(x, y, w, h, BOX_VISIBLE | BOX_TRANSPARENT);
	struct friendentrydata_s *data = malloc(sizeof(*data));

	memset(data, 0, sizeof(*data));
	data->jid = strdup(jid);

	if (nickname && strlen(nickname) > 0)
	{
		data->nickname = strdup(nickname);
	}
	/*
	else
	{
		data->nickname = strdup(jid);
	}
	*/

	data->groupname = strdup(groupname);
	data->status = status;

	if (statusmsg)
	{
		data->statusmsg = strdup(statusmsg);
	}
	
	entrybox->OnSizeWidth     = Box_OnSizeWidth_Stretch;
	entrybox->OnRButtonUp     = FriendEntry_OnRButtonUp;
	entrybox->OnLButtonDblClk = FriendEntry_OnLButtonDblClk;
	entrybox->OnMouseMove     = FriendEntry_OnMouseMove;
	entrybox->OnDestroy       = FriendEntry_OnDestroy;
	entrybox->boxdata = data;
	entrybox->debug = "FriendEntry";

	FriendEntry_Update(entrybox);

	return entrybox;
}

struct Box_s *FriendEntry_CreateWrap(int x, int y, int w, int h, char *jid, char *nickname, char *groupname, enum SStatus status, char *statusmsg, const char *file, unsigned int line)
{
	struct Box_s *alloc = FriendEntry_CreateReal(x, y, w, h, jid, nickname, groupname, status, statusmsg);
	Mem_AddMemRec(alloc, 0, "friendentry", file, line);

	return alloc;
}

void FriendEntry_Update(struct Box_s *pbox)
{
	struct friendentrydata_s *data = pbox->boxdata;
	struct Box_s *drag;
	struct Box_s *subbox;
	char statusmsg[512];
	int namey = (pbox->h - 32) / 2;
	COLORREF namecol = TabFG1;
	COLORREF statuscol = RGB(0, 0, 0);

	statusmsg[0] = '\0';

	{
		struct FriendDropData_s *frienddropdata = malloc(sizeof(*frienddropdata));

		frienddropdata->jid = data->jid;
		frienddropdata->groupname = data->groupname;

		drag = DragBox_Create(0, 0, pbox->w, pbox->h, BOX_VISIBLE | BOX_TRANSPARENT, FRIENDDROPDATA_ID, frienddropdata, NULL);
	}

	while (pbox->child)
	{
		Box_Destroy(pbox->child);
	}

	if (data->status == SSTAT_OFFLINE && !showoffline)
	{
		return;
	}

	if (data->status == SSTAT_PENDING || data->status == SSTAT_DENIED || data->status == SSTAT_OFFLINE)
	{
		namecol = TabFG2;
		statuscol = TabFG2;
	}

	if (data->status == SSTAT_PENDING)
	{
		strcpy(statusmsg, _("Pending Approval"));
	}
	else if (data->playinggamelist)
	{
		strcat(statusmsg, "^i");
		strcat(statusmsg, _("Playing"));
		strcat(statusmsg, "^n");
	}
	else if (data->watchinggamelist)
	{
		strcat(statusmsg, "^i");
		strcat(statusmsg, _("Watching"));
		strcat(statusmsg, "^n");
	}

	if (data->status == SSTAT_AWAY && !data->statusmsg)
	{
		if (statusmsg[0] != '\0')
		{
			strcat(statusmsg, _(" - Away"));
		}
		else
		{
			strcpy(statusmsg, _("Away"));
		}
	}
	else if (data->statusmsg)
	{
		if (statusmsg[0] != '\0')
		{
			char txt[512];

			sprintf(txt, " - %s", Util_DoubleCarats(data->statusmsg));
			strcat(statusmsg, txt);
		}
		else
		{
			strcpy(statusmsg, Util_DoubleCarats(data->statusmsg));
		}
	}

	if (statusmsg[0] == '\0')
	{
		namey = (pbox->h - 16) / 2;
	}

	switch(data->status)
	{
		case SSTAT_AVAILABLE:
		{
			subbox = Box_Create(4, (pbox->h - 16) / 2, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
			subbox->img = ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16);
			Box_AddChild(drag, subbox);
		}
		break;
		case SSTAT_AWAY:
		{
			subbox = Box_Create(4, (pbox->h - 16) / 2, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
			subbox->img = ImageMgr_GetSubImage("presenceAway", "PresenceIcons.png", 48, 0, 16, 16);
			Box_AddChild(drag, subbox);
		}
		break;
		case SSTAT_IDLE:
		{
			subbox = Box_Create(4, (pbox->h - 16) / 2, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
			subbox->img = ImageMgr_GetSubImage("presenceIdle", "PresenceIcons.png", 32, 0, 16, 16);
			Box_AddChild(drag, subbox);
		}
		break;

		default:
			break;
	}

	subbox = Text_Create(28, namey, pbox->w - 30 - 46 + (hideavatar ? 41 : 0), 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	subbox->bgcol = TabBG1;
	subbox->fgcol = namecol;

	{
		char txt[512];
		int notfirst = 0;
		struct namedlist_s *entry;

		strcpy(txt, "^b");

		entry = data->titles;

		while (entry)
		{
			if (notfirst)
			{
				strcat(txt, ", ");
			}
			else
			{
				notfirst = 1;
			}

			strcat(txt, entry->data);

			entry = entry->next;
		}

		if (notfirst)
		{
			strcat(txt, " ");
		}

		if (data->nickname)
		{
			strcat(txt, data->nickname);
		}
		else
		{
			strcat(txt, Model_GetFriendNick(data->jid));
		}
		strcat(txt, "^n ");

		if (data->status == SSTAT_PENDING || data->status == SSTAT_DENIED || data->status == SSTAT_OFFLINE)
		{
		}
		else
		{
			strcat(txt, "^0");
		}

		notfirst = 0;
		entry = data->roles;

		while (entry)
		{
			if (notfirst)
			{
				strcat(txt, ", ");
			}
			else
			{
				notfirst = 1;
			}

			strcat(txt, entry->data);

			entry = entry->next;
		}

		if (data->rating)
		{
			if (notfirst)
			{
				strcat(txt, ", ");
			}
			else
			{
				notfirst = 1;
			}

			/*strcat(txt, "^l");*/
			strcat(txt, data->rating);
			/*strcat(txt, "^l");*/
		}

		Text_SetText(subbox, txt);

		if (data->rating)
		{
			Text_SetLinkCallback(subbox, 1, ViewLink_ShowProfile, strdup(data->jid));
		}

	}
	Box_AddChild(drag, subbox);


	if (!hideavatar)
	{
		if (data->avatarhash)
		{
			char buffer[MAX_PATH];
			char *filename;

			filename = Ctrl_GetAvatarFilenameByHash(data->avatarhash, buffer, MAX_PATH);

			if (filename)
			{
				struct BoxImage_s *parentimage, *scaledimage, *finalimage;
				char smallname[256];
				char dimmedname[256];	

				parentimage = ImageMgr_GetRootImage(filename);

				if (parentimage)
				{
					sprintf(smallname, "%s-30x30", filename);

					scaledimage = ImageMgr_GetRootAspectScaledImage(smallname, filename, 30, 30);

					sprintf(dimmedname, "%s-30x30dim", filename);

					if (data->status == SSTAT_OFFLINE || data->status == SSTAT_PENDING)
					{
						finalimage = ImageMgr_GetRootDimmedImage(dimmedname, smallname, 50);
					}
					else
					{
						finalimage = scaledimage;
					}

					subbox = Box_Create(pbox->w - 41, (pbox->h - 30) / 2, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
					subbox->img = finalimage;
					subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
					Box_AddChild(drag, subbox);
				}
			}
		}
		else if (data->status == SSTAT_OFFLINE || data->status == SSTAT_PENDING)
		{
			subbox = Box_Create(pbox->w - 41, (pbox->h - 30) / 2, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->img = ImageMgr_GetAspectScaledTransImage("DefaultAvatar-dim30x30", "DefaultAvatar.png", 30, 30, 25);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Box_AddChild(drag, subbox);
		}
		else
		{
			subbox = Box_Create(pbox->w - 41, (pbox->h - 30) / 2, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->img = ImageMgr_GetScaledImage("DefaultAvatar30x30", "DefaultAvatar.png", 30, 30);
			subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Box_AddChild(drag, subbox);
		}
	}

	if (statusmsg[0] != '\0')
	{
		subbox = Text_Create(28, namey + 16, pbox->w - 30 - 46 + (hideavatar ? 41 : 0), 16, BOX_VISIBLE | BOX_TRANSPARENT, 0);
		subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		subbox->bgcol = TabBG1;
		subbox->fgcol = statuscol;
		Text_SetText(subbox, statusmsg);
		Box_AddChild(drag, subbox);
	}

	Box_AddChild(pbox, drag);
	drag->OnSizeWidth = Box_OnSizeWidth_Stretch;

	Box_Repaint(pbox);
}

void FriendEntry_Refresh(struct Box_s *pbox, void *userdata)
{
	if (!pbox->boxdata)
	{
		return;
	}
	FriendEntry_Update(pbox);
}

void FriendEntry_SetStatus(struct Box_s *pbox, char *nickname,
	enum SStatus status, char *statusmsg, char *avatarhash, char *rating,
	struct namedlist_s *roles, struct namedlist_s *titles,
	char *gameid, struct gamesearchinfo_s *info, int stopped, int watching,
	struct adhoccommand_s *command)
{
	struct friendentrydata_s *data = pbox->boxdata;

	data->status = status;

	free(data->statusmsg);
	data->statusmsg = strdup(statusmsg);

	if (avatarhash)
	{
		free(data->avatarhash);
		data->avatarhash = strdup(avatarhash);
	}

	free(data->rating);
	data->rating = NULL;

	if (rating)
	{
		data->rating = strdup(rating);
		data->roles  = NamedList_DupeStringList(roles);
		data->titles = NamedList_DupeStringList(titles);
	}

	free(data->nickname);
	if (nickname && strlen(nickname) > 0)
	{
		data->nickname = strdup(nickname);
	}
	else
	{
		data->nickname = NULL;
		/*data->nickname = strdup(data->jid);*/
	}

	if (gameid)
	{
		NamedList_RemoveByName(&(data->playinggamelist), gameid);
		NamedList_RemoveByName(&(data->watchinggamelist), gameid);

		if (!stopped)
		{
                        if (watching)
			{
				NamedList_Add(&(data->watchinggamelist), gameid, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
			}
			else
			{
				NamedList_Add(&(data->playinggamelist), gameid, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
			}
		}
	}

	data->command = Info_DupeAdHocCommand(command);

	FriendEntry_Update(pbox);

	Box_Repaint(pbox);
}

BOOL FriendEntry_SortFunc(struct Box_s *lesser, struct Box_s *greater)
{
	struct friendentrydata_s *ldata = lesser->boxdata;
	struct friendentrydata_s *gdata = greater->boxdata;
	char *lnick, *gnick;

	if (!ldata || !gdata)
	{
		return TRUE;
	}

	if (ldata->status < gdata->status)
	{
		return TRUE;
	}

	if (ldata->status > gdata->status)
	{
		return FALSE;
	}

	if (ldata->nickname)
	{
		lnick = ldata->nickname;
	}
	else
	{
		lnick = ldata->jid;
	}

	if (gdata->nickname)
	{
		gnick = gdata->nickname;
	}
	else
	{
		gnick = gdata->jid;
	}

	if (stricmp(lnick, gnick) < 0)
	{
		return TRUE;
	}
		
	return FALSE;
}

BOOL FriendEntry_VisibleFunc(struct Box_s *pbox)
{
	struct friendentrydata_s *data = pbox->boxdata;

	if (data && data->status == SSTAT_OFFLINE)
	{
		return FALSE;
	}
	return TRUE;
}

void FriendEntry_SetShowIcon(int show)
{
	hideavatar = !show;
}

void FriendEntry_SetShowOffline(int show)
{
	showoffline = show;
}
