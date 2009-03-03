#include <stdio.h>
#include <stdlib.h>

#include "box.h"

#include "constants.h"
#include "dragbox.h"
#include "text.h"

#include "i18n.h"
#include "imagemgr.h"
#include "info.h"
#include "menu.h"
#include "model.h"
#include "namedlist.h"
#include "tooltip.h"
#include "util.h"
#include "view.h"

#include "participantentry.h"

struct participantentrydata_s
{
	char *jid;
	char *chatjid;
	char *realjid;
	char *nick;
	char *role;
	char *affiliation;
	char *showname;
	char *showaffiliation;
	char *showtitles;
	int notactivated;
	char *membertype;
	int ismuc;
	enum SStatus status;
};

void ParticipantEntry_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct participantentrydata_s *data = pbox->boxdata;

	if (Box_CheckXYInBox(pbox, xmouse, ymouse)/* && Box_GetRoot(pbox)->active*/) 
	{
		if (!pbox->tooltipvisible)
		{
			char txt[1024];
		
			/*i18n_stringsub(txt, 1024, "Nickname: %1\ncontact: %2\nrole: %3\naffiliation: %4", data->nick, data->jid, data->role, data->affiliation);*/
			if (data->showaffiliation)
			{
				i18n_stringsub(txt, 1024, "Nickname: %1\ncontact: %2\nroles: %3", data->nick, data->jid, data->showaffiliation);
			}
			else
			{
				i18n_stringsub(txt, 1024, "Nickname: %1\ncontact: %2", data->nick, data->jid);
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

	Box_OnMouseMove(pbox, xmouse, ymouse);
}

void ParticipantEntry_PopupMenu(struct Box_s *pbox, struct Box_s *window, int x, int y)
{
	struct participantentrydata_s *data = pbox->boxdata;
	
	Menu_PopupChatParticipantMenu(window, data->jid, data->chatjid, data->nick, data->role, data->affiliation, data->ismuc, data->ismuc, x, y);
}

void ParticipantEntry_OnRButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct participantentrydata_s *data = pbox->boxdata;
	int x, y;

	if (xmouse > 0 && ymouse > 0 && xmouse < pbox->w && ymouse < pbox->h)
	{
		Box_GetScreenCoords(pbox, &x, &y);
		x += xmouse;
		y += ymouse;

		ParticipantEntry_PopupMenu(pbox, pbox, x, y);
	}

}

void ParticipantEntry_OnLButtonDblClk(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct participantentrydata_s *data = pbox->boxdata;

	if (data->jid && Box_IsVisible(pbox) && !strchr(data->jid, '/'))
	{
		Model_PopupChatDialog(data->jid, 0);
	}
}

#if 0
void ParticipantEntryDrag_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	if (DragBox_GetDragState(pbox) == DRAG_DRAGGING)
	{
		struct participantentrydata_s *data = pbox->parent->boxdata;

		int x, y;

		Box_GetScreenCoords(pbox, &x, &y);
		x += xmouse;
		y += ymouse;

		View_OnJIDDrop(data->jid, x, y);
	}
	
	DragBox_OnLButtonUp(pbox, xmouse, ymouse);
}
#endif

void ParticipantEntry_Update(struct Box_s *entrybox)
{
	struct Box_s *subbox, *subbox2;
	struct participantentrydata_s *pedata = entrybox->boxdata;
	char txt[256];

	while (entrybox->child)
	{
		Box_Destroy(entrybox->child);
	}

	subbox = DragBox_Create(16, 0, entrybox->w, 16, BOX_VISIBLE | BOX_TRANSPARENT, PARTICIPANTJIDDROPDATA_ID, strdup(pedata->jid), NULL);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox2 = Text_Create(0, -1, entrybox->w, 14, BOX_VISIBLE | BOX_TRANSPARENT | BOX_NOCLIP, TX_ELLIPSIS);
	subbox2->OnSizeWidth = Text_OnSizeWidth_Stretch;
	if (pedata->realjid)
	{
		/*subbox->OnLButtonUp = ParticipantEntryDrag_OnLButtonUp;*/
	}
	entrybox->fgcol = TabFG1;

	txt[0] = 0;

	if (pedata->showaffiliation && (strstr(pedata->showaffiliation, "staff")))
	{
		strcpy(txt, "^2");
	}
	else if (pedata->showaffiliation && (strstr(pedata->showaffiliation, "helper")))
	{
		strcpy(txt, "^7");
	}/*
	else if (pedata->affiliation && (stricmp(pedata->affiliation, "Owner") == 0 || stricmp(pedata->affiliation, "Admin") == 0))
	{
		strcpy(txt, "^5^b");
	}
	else if (pedata->role && stricmp(pedata->role, "moderator") == 0)
	{
		strcpy(txt, "^5^b");
	}*/
	else if (pedata->membertype && stricmp(pedata->membertype, "pro") == 0)
	{
		strcpy(txt, "^8");
	}
	else if (pedata->notactivated)
	{
		strcpy(txt, "^9");
	}
	else if (pedata->role && stricmp(pedata->role, "participant") == 0)
	{
		strcpy(txt, "^6");
	}
	else if (pedata->role && stricmp(pedata->role, "visitor") == 0)
	{
		strcpy(txt, "^4");
	}
	else
	{
		strcpy(txt, "^6");
	}

	if (pedata->showtitles)
	{
		strcat(txt, pedata->showtitles);
		strcat(txt, " ");
	}

	strcat(txt, Util_DoubleCarats(pedata->showname));
	strcat(txt, "^n ");

	if (pedata->showaffiliation)
	{
		strcat(txt, "^4");
		strcat(txt, pedata->showaffiliation);
		strcat(txt, " ");
	}

	if (Model_IsJIDIgnored(pedata->realjid))
	{
		strcat(txt, "^4");
		strcat(txt, _("ignored"));
	}

	Text_SetText(subbox2, txt);
	Box_AddChild(subbox, subbox2);
	Box_AddChild(entrybox, subbox);

	switch (pedata->status)
	{
		case SSTAT_AVAILABLE:
		{
			subbox = Box_Create(2, 0, 12, 12, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->img = ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16);
			Box_AddChild(entrybox, subbox);
		}
		break;
		case SSTAT_AWAY:
		{
			subbox = Box_Create(2, 0, 12, 12, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->img = ImageMgr_GetSubImage("presenceAway", "PresenceIcons.png", 48, 0, 16, 16);
			Box_AddChild(entrybox, subbox);
		}
		break;
		case SSTAT_IDLE:
		{
			subbox = Box_Create(2, 2, 12, 12, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
			subbox->img = ImageMgr_GetSubImage("presenceIdle", "PresenceIcons.png", 32, 0, 16, 16);
			Box_AddChild(entrybox, subbox);
		}
		break;

		default:
			break;
	}

	Box_Repaint(entrybox);
}

void ParticipantEntry_SetRolesAndTitles(struct Box_s *entrybox,
  struct namedlist_s *roleslist, struct namedlist_s *titleslist)
{
	struct participantentrydata_s *pedata = entrybox->boxdata;
	struct namedlist_s *entry = roleslist;
	char titlesstring[1024];

	while (entry)
	{
		if ((stricmp(entry->data, "staff") == 0) || (stricmp(entry->data, "helper") == 0)  || (stricmp(entry->data, "computer") == 0))
		{
			char *trans;

			if (stricmp(entry->data, "staff") == 0)
			{
				trans = _("staff");
			}
			else if (stricmp(entry->data, "helper") == 0)
			{
				trans = _("helper");
			}
			else if (stricmp(entry->data, "computer") == 0)
			{
				trans = _("computer");
			}
			else
			{
				trans = entry->data;
			}

			free(pedata->showaffiliation);
			if ((pedata->role && stricmp(pedata->role, "moderator") == 0) || (pedata->role && stricmp(pedata->role, "owner") == 0))
			{
				char fullrole[256];
				char *trans2;

				if (stricmp(pedata->role, "owner") == 0)
				{
					trans2 = _("moderator");
					/*trans2 = _("owner");*/
				}
				else if (stricmp(pedata->role, "admin") == 0)
				{
					trans2 = _("moderator");
					/*trans2 = _("admin");*/
				}
				else if (stricmp(pedata->role, "moderator") == 0)
				{
					trans2 = _("moderator");
				}
				else
				{
					trans2 = pedata->role;
				}


				sprintf(fullrole, "%s %s", trans, trans2);

				pedata->showaffiliation = strdup(fullrole);
			}
			else
			{
				pedata->showaffiliation = strdup(trans);
			}
		}
		entry = entry->next;
	}

	titlesstring[0] = '\0';

	entry = titleslist;
	while (entry)
	{
		strcat(titlesstring, entry->data);
		entry = entry->next;
		if (entry)
		{
			strcat(titlesstring, " ");
		}
	}

	free(pedata->showtitles);
	
	if (titleslist)
	{
		pedata->showtitles = strdup(titlesstring);
	}
	else
	{
		pedata->showtitles = NULL;
	}
}

void ParticipantEntry_SetProfile(char *jid, struct profile_s *profile, struct Box_s *entrybox)
{
	struct participantentrydata_s *pedata = entrybox->boxdata;

	ParticipantEntry_SetRolesAndTitles(entrybox, profile->roles, profile->titles);

	free(pedata->showname);
	if (profile->nickname)
	{
		pedata->showname = strdup(profile->nickname);
	}
	else
	{
		/*
		char *barejid = Jid_Strip(jid);
		pedata->showname = strdup(barejid);
		free(barejid);
		*/
		pedata->showname = Model_GetFriendNick(jid);
	}

	if (profile->membertype)
	{
		free(pedata->membertype);
		pedata->membertype = strdup(profile->membertype);
	}

	ParticipantEntry_Update(entrybox);
}

void ParticipantEntry_OnDestroy(struct Box_s *entrybox)
{
	struct participantentrydata_s *pedata = entrybox->boxdata;

	ToolTipParent_OnDestroy(entrybox);

	if (pedata->realjid)
	{
		Model_UnsubscribeProfile(pedata->realjid, ParticipantEntry_SetProfile, entrybox);
	}
}

struct Box_s *ParticipantEntry_Create(char *targetjid, char *name,
  enum SStatus status, char *statusmsg, char *role, char *affiliation,
  char *realjid, int isGroupChat, struct namedlist_s *roleslist,
  struct namedlist_s *titleslist, int notactivated, char *membertype)
{
	struct Box_s *entrybox;
	char txt[256];
	struct participantentrydata_s *pedata;

	entrybox = Box_Create(0, 0, 150 - 30, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->OnSizeWidth     = Box_OnSizeWidth_Stretch;
	entrybox->OnRButtonUp     = ParticipantEntry_OnRButtonUp;
	entrybox->OnLButtonDblClk = ParticipantEntry_OnLButtonDblClk;
	entrybox->OnMouseMove     = ParticipantEntry_OnMouseMove;
	entrybox->OnDestroy       = ParticipantEntry_OnDestroy;
	
	pedata = malloc(sizeof(*pedata));
	memset(pedata, 0, sizeof(*pedata));

	pedata->chatjid = strdup(targetjid);

	pedata->realjid         = strdup(realjid);
	pedata->nick            = strdup(name);
	pedata->role            = strdup(role);
	pedata->affiliation     = strdup(affiliation);
	pedata->status          = status;
	pedata->ismuc           = isGroupChat;
	pedata->notactivated    = notactivated;
	pedata->membertype      = strdup(membertype);

	if (realjid)
	{
		char *barejid = Jid_Strip(realjid);
		pedata->jid = strdup(barejid);
		free(barejid);
	}
	else if (isGroupChat)
	{
		strcpy(txt, targetjid);
		strcat(txt, "/");
		strcat(txt, name);
		pedata->jid = strdup(txt);
	}
	else
	{
		char *barejid = Jid_Strip(targetjid);
		pedata->jid = strdup(barejid);
		free(barejid);
	}

	if ((strstr(targetjid, "@chat.chesspark.com") || strstr(targetjid, "@games.chesspark.com")) && CHESSPARK_LOCALCHATSUSEJIDNICK)
	{
		if (realjid)
		{
			pedata->showname = Model_GetFriendNick(realjid);
		}
		else
		{
			pedata->showname = Model_GetFriendNick(name);
		}

		if (affiliation && stricmp(affiliation, "owner") == 0)
		{
			pedata->showaffiliation = strdup(_("moderator"));
			/*pedata->showaffiliation = strdup(_("owner"));*/
		}
		else if (affiliation && stricmp(affiliation, "admin") == 0)
		{
			pedata->showaffiliation = strdup(_("moderator"));
			/*pedata->showaffiliation = strdup(_("admin"));*/
		}
		else if (role && stricmp(role, "moderator") == 0)
		{
			pedata->showaffiliation = strdup(_("moderator"));
		}
		else if (role && stricmp(role, "visitor") == 0)
		{
			pedata->showaffiliation = strdup(_("observer"));
		}
	}
	else
	{
		pedata->showname = strdup(name);
		if (affiliation && strcmp(affiliation, "none") != 0)
		{
			pedata->showaffiliation = strdup(affiliation);
		}
	}

	entrybox->boxdata = pedata;

	if ((strstr(targetjid, "@chat.chesspark.com") || strstr(targetjid, "@games.chesspark.com")))
	{
		ParticipantEntry_SetRolesAndTitles(entrybox, roleslist, titleslist);
	}
	ParticipantEntry_Update(entrybox);

	/*if (realjid && (strstr(targetjid, "@chat.chesspark.com") || strstr(targetjid, "@games.chesspark.com")))*/
	{
		Model_SubscribeProfileNoRequest(realjid, ParticipantEntry_SetProfile, entrybox);
	}

	return entrybox;
}

enum SStatus ParticipantEntry_GetStatus(struct Box_s *entrybox)
{
	struct participantentrydata_s *data = entrybox->boxdata;

	return data->status;
}

void ParticipantEntry_SetStatus(struct Box_s *entrybox, enum SStatus status)
{
	struct participantentrydata_s *data = entrybox->boxdata;

	data->status = status;

	ParticipantEntry_Update(entrybox);
	Box_Repaint(entrybox);
}

char *ParticipantEntry_GetRole(struct Box_s *entrybox)
{
	struct participantentrydata_s *data = entrybox->boxdata;

	return data->role;
}

char *ParticipantEntry_GetShowName(struct Box_s *entrybox)
{
	struct participantentrydata_s *data = entrybox->boxdata;
	char *finalname;
	int len = (int)(strlen(data->showname)) + 1;

	if (data->showtitles)
	{
		len += (int)(strlen(data->showtitles)) + 1;
	}

	finalname = malloc(len);
	finalname[0] = '\0';

	if (data->showtitles)
	{
		strcat(finalname, data->showtitles);
		strcat(finalname, " ");
	}

	strcat(finalname, data->showname);

	return finalname;
}

char *ParticipantEntry_GetRealJID(struct Box_s *entrybox)
{
	struct participantentrydata_s *data = entrybox->boxdata;

	return data->realjid;
}

int ParticipantEntry_UserHasMucPower(struct Box_s *entrybox)
{
	struct participantentrydata_s *data = entrybox->boxdata;

	if (stricmp(data->role, "moderator") == 0)
	{
		return 1;
	}
	return 0;
}

int ParticipantEntry_SortFunc(struct Box_s *lbox, struct Box_s *gbox)
{
	struct participantentrydata_s *ldata = lbox->boxdata;
	struct participantentrydata_s *gdata = gbox->boxdata;
	int lpro, gpro;

	if (ldata->status != gdata->status)
	{
		return ldata->status < gdata->status;
	}

	if (ldata->showaffiliation && gdata->showaffiliation && stricmp(ldata->showaffiliation, gdata->showaffiliation) != 0)
	{
		if (strstr(gdata->showaffiliation, _("staff")) && !strstr(ldata->showaffiliation, _("staff")))
		{
			return 0;
		}

		if (strstr(ldata->showaffiliation, _("staff")) && !strstr(gdata->showaffiliation, _("staff")))
		{
			return 1;
		}

		if (strstr(gdata->showaffiliation, _("helper")) && !strstr(ldata->showaffiliation, _("helper")))
		{
			return 0;
		}

		if (strstr(ldata->showaffiliation, _("helper")) && !strstr(gdata->showaffiliation, _("helper")))
		{
			return 1;
		}

		if (strstr(gdata->showaffiliation, _("owner")) && strstr(ldata->showaffiliation, _("owner")))
		{
			return 0;
		}

		if (strstr(ldata->showaffiliation, _("owner")) && !strstr(gdata->showaffiliation, _("owner")))
		{
			return 1;
		}

		if (strstr(gdata->showaffiliation, _("admin")) && strstr(ldata->showaffiliation, _("admin")))
		{
			return 0;
		}

		if (strstr(ldata->showaffiliation, _("admin")) && !strstr(gdata->showaffiliation, _("admin")))
		{
			return 1;
		}

		if (strstr(gdata->showaffiliation, _("moderator")) && !strstr(ldata->showaffiliation, _("moderator")))
		{
			return 0;
		}

		if (strstr(ldata->showaffiliation, _("moderator")) && !strstr(gdata->showaffiliation, _("moderator")))
		{
			return 1;
		}
	}

	lpro = ldata->membertype && stricmp(ldata->membertype, "pro") == 0;
	gpro = gdata->membertype && stricmp(gdata->membertype, "pro") == 0;

	if (lpro && !gpro)
	{
		return 1;
	}

	if (!lpro && gpro)
	{
		return 0;
	}

	if (!ldata->notactivated && gdata->notactivated)
	{
		return 1;
	}

	if (ldata->notactivated && !gdata->notactivated)
	{
		return 0;
	}

	if (!ldata->showaffiliation && gdata->showaffiliation)
	{
		if (strcmp(gdata->showaffiliation, _("observer")) == 0)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	if (!gdata->showaffiliation && ldata->showaffiliation)
	{
		if (strcmp(ldata->showaffiliation, _("observer")) == 0)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}

	return stricmp(ldata->showname, gdata->showname) < 0;

}
