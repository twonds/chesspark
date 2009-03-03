#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <direct.h>

#include "box.h"

#include "button.h"
#include "dragbox.h"
#include "edit.h"
#include "tabs.h"
#include "text.h"

#include "boxtypes.h"

#include "autodialog.h"

#include "audio.h"
#include "conn.h"
#include "constants.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "imagemgr.h"
#include "info.h"
#include "list.h"
#include "log.h"
#include "menu.h"
#include "model.h"
#include "namedlist.h"
#include "options.h"
#include "participantentry.h"
#include "view.h"
#include "sizer.h"
#include "subchat.h"
#include "titledrag.h"
#include "util.h"
#include "titlebar.h"
#include "tooltip.h"

#include "chatbox.h"

int ChatBox_OnDragDrop(struct Box_s *chatdst, struct Box_s *chatsrc, int x, int y, int dragid, void *dragdata);
void ChatBox_SetShowTabs(struct Box_s *chatbox, int show);

struct BoxImage_s *GetIconForStatus(enum SStatus status)
{
	switch (status)
	{
		case SSTAT_AVAILABLE:
		{
			return ImageMgr_GetSubImage("presenceAvailable", "PresenceIcons.png", 16, 0, 16, 16);
		}
		break;

		case SSTAT_AWAY:
		{
			return ImageMgr_GetSubImage("presenceAway", "PresenceIcons.png", 48, 0, 16, 16);
		}
		break;

		case SSTAT_IDLE:
		{
			return ImageMgr_GetSubImage("presenceIdle", "PresenceIcons.png", 32, 0, 16, 16);
		}
		break;

		default:
			return NULL;
	}
	return NULL;
}

struct chatboxdata_s
{
	struct Box_s *edit;
	struct Box_s *editborder;
	struct Box_s *drawer;
	struct Box_s *grippy;
	struct Box_s *tabctrl;
	struct Box_s *prefsbutton;
	struct Box_s *tabcontentbg;
	struct namedlist_s *chats;
	int drawersavedw;
	int drawervisible;
	int draweranimating;
	int tabsvisible;
	int normx;
	int normy;
	int normw;
	int normh;
	int normy2;
	int windowgroup;
	struct Box_s *overotherchat;
};

struct chattabcontentdata_s
{
	char *logfile;
	int logstarted;
	char *nick;
	char *targetjid;
	int isGroupChat;
	char *topic;
	struct Box_s *box;
	struct Box_s *participantlist;
	int state;
	int showcomposing;
	unsigned int lastchattime;
	int startedpast;
	int isinroom;
	int initialconnect;
	int localnotactivated;
};

struct chatboxdrawerdata_s
{
	struct Box_s *chatbox;
};

void ChatBox_RemoveChat(struct Box_s *pbox, char *targetjid);

void ChatBox_MucJoinTimeout(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chatbox = Box_GetRoot(pbox);
	struct chatboxdata_s *data = chatbox->boxdata;
	struct namedlist_s *entry;
	struct chattabcontentdata_s *tabdata;

	Box_RemoveTimedFunc(pbox, ChatBox_MucJoinTimeout, 15000);

	entry = data->chats;

	while (entry)
	{
		tabdata = entry->data;

		if (tabdata->box == pbox)
		{
			ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("Error: Invalid Room"), NULL, 1);
		}

		entry = entry->next;
	}
}

struct namedlist_s **ChatBox_GetChatTabEntry(struct Box_s *chatbox, char *targetjid)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct namedlist_s **chattab = NamedList_GetByName(&(data->chats), targetjid);

	if (!chattab)
	{
		char *barejid = Jid_Strip(targetjid);
		chattab = NamedList_GetByName(&(data->chats), barejid);
		free(barejid);
		if (!chattab)
		{
			return NULL;
		}
	}

	return chattab;
}

struct chattabcontentdata_s *ChatBox_GetChatTabContent(struct Box_s *chatbox, char *targetjid)
{
	struct namedlist_s **chattab = ChatBox_GetChatTabEntry(chatbox, targetjid);

	if (!chattab)
	{
		return NULL;
	}

	return (*chattab)->data;
}

void ChatBox_OnMove(struct Box_s *pbox, int x, int y)
{
	struct chatboxdata_s *data = pbox->boxdata;
	Box_OnMove(pbox, x, y);
	if (data->drawervisible)
	{
		Box_MoveWndCustom2(data->drawer, x - data->drawer->w, y + 20, data->drawer->w, pbox->h - 40);
	}
}

void ChatTabContent_OnDestroy(void *voiddata)
{
	struct chattabcontentdata_s *data = voiddata;

	if (data->showcomposing)
	{
		Model_UnsetComposing(data->targetjid);
		data->showcomposing = 0;
	}

	if (data->isGroupChat && data->isinroom)
	{
		char chatname[256];

		strcpy(chatname, data->targetjid);
		strcat(chatname, "/");
		strcat(chatname, data->nick);

		Ctrl_LeaveChat(chatname);

		/*Conn_SendPresence(SSTAT_OFFLINE, NULL, chatname, 1, 0);*/
	}

	if (data->logfile && data->logstarted)
	{
		FILE *fp = fopen(data->logfile, "a");
		if (fp)
		{
			fprintf(fp, "-----Log file closed at %s, %s-----\n", Info_GetLocalDate(), Info_GetLocalTime());
			fclose(fp);
		}
	}

	{
		char *barejid = Jid_Strip(data->targetjid);
		struct Box_s *pbox = Box_GetRoot(data->box);
		struct chatboxdata_s *bdata = pbox->boxdata;
		if (!IsIconic(pbox->hwnd))
		{
			View_SetSavedWindowPos2(barejid, pbox->x, pbox->y, pbox->w, pbox->h, bdata->drawervisible ? bdata->drawer->w: -1, bdata->windowgroup);
		}
	}


	Box_Destroy(data->box);
	Box_Destroy(data->participantlist);
	free(data->nick);
	free(data->targetjid);
}

void ChatBox_OnClose(struct Box_s *pbox)
{	
	View_CloseChatDialog(Box_GetRoot(pbox));
}

void ChatBox_OnRestore(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_RESTORE);
}

void ChatBox_OnMinimize(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_MINIMIZE);
}

void ChatBox_OnMaximize(struct Box_s *pbox)
{
	int maxx, maxy, maxw, maxh;
	RECT rc;
	struct chatboxdata_s *data;
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

void ChatBox_OnDestroy(struct Box_s *pbox)
{
	struct chatboxdata_s *data = pbox->boxdata;

	ToolTipParent_OnDestroy(pbox);

	while(data->chats)
	{
/*
		struct chattabcontentdata_s *tabdata = data->chats->data;
		char *barejid = Jid_Strip(tabdata->targetjid);
		if (!IsIconic(pbox->hwnd))
		{
			View_SetSavedWindowPos2(barejid, pbox->x, pbox->y, pbox->w, pbox->h, data->drawervisible ? data->drawer->w: -1);
		}
*/
		NamedList_Remove(&(data->chats));
	}

	if (data->drawervisible)
	{
		Box_Destroy(data->drawer);
	}

}

void ChatBox_TimedUnsetComposing(struct Box_s *chatbox, struct chattabcontentdata_s **userdata)
{
	struct chattabcontentdata_s *tabdata = *userdata;
	Box_RemoveTimedFunc(chatbox, ChatBox_TimedUnsetComposing, 5000);

	Model_UnsetComposing(tabdata->targetjid);
	tabdata->showcomposing = 0;
}

void ChatBox_OnEnter(struct Box_s *pbox, char *text)
{
	struct Box_s *chatbox = Box_GetRoot(pbox);
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	char *name;

	if (!text || strlen(text) == 0)
	{
		return;
	}

	name = TabCtrl_GetActiveTab(data->tabctrl);

	if (!name)
	{
		return;
	}

	tabdata = ChatBox_GetChatTabContent(chatbox, name);

	if (!tabdata)
	{
		return;
	}

	List_ScrollToBottom(tabdata->box);

	if (tabdata->isGroupChat)
	{
		/* if we're in a nickname state, change nickname */
		if (tabdata->state == 1)
		{
			tabdata->state = 0;
			tabdata->nick = strdup(text);
			Ctrl_ChangeNick(tabdata->targetjid, text);

			Edit2Box_ClearText(pbox);
			Box_Repaint(pbox);
			return;
		}
		
		/* if we're not connected, don't send text */
		if (!tabdata->isinroom)
		{
			return;
		}

		/* check for commands */
		if (text && strlen(text) > 0 && text[0] == '/' && strncmp(text, "/me", 3) != 0)
		{
			char *command = strdup(text);
			char *allparams = NULL;
			char *nickparam = NULL;
			char *commentparam = NULL;
			char *space = command;

			while (*space != '\0' && *space != ' ')
			{
				space++;
			}

			if (*space == ' ')
			{
				*space++ = '\0';
			}

			while (*space != '\0' && *space == ' ')
			{
				space++;
			}

			if (*space != '\0')
			{
				allparams = strdup(space);
				nickparam = space;
			}

			while (*space != '\0' && *space != ' ')
			{
				space++;
			}

			if (*space == ' ')
			{
				*space++ = '\0';
			}
/*
			if (nickparam && !List_GetEntryBoxAllGroups(tabdata->participantlist, nickparam))
			{
				char *nextspace = strrchr(nickparam, ' ');

				while (nextspace)
				{
					space = nextspace;
					*space = '\0';
					nextspace = strrchr(nickparam, ' ');

					if (List_GetEntryBoxAllGroups(tabdata->participantlist, nickparam))
					{
						space++;
						nextspace = NULL;
					}
					else
					{
						*space = ' ';
					}
				}
			}
*/
			while (*space != '\0' && *space == ' ')
			{
				space++;
			}

			if (*space != '\0')
			{
				commentparam = space;
			}

			Log_Write(0, "command %s nickparam %s commentparam %s", command, nickparam, commentparam);
/*
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
*/
			if (strcmp(command, "/nick") == 0)
			{
				if (strstr(tabdata->targetjid, "@chat.chesspark.com"))
				{
					ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^4You can't change your nickname in a chesspark room."), NULL, 0);
					List_ScrollToBottom(tabdata->box);
				}
				else if (!allparams)
				{
					ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^4Usage: /nick [nickname]."), NULL, 0);
					List_ScrollToBottom(tabdata->box);
				}
				else
				{
					if (strcmp(allparams, tabdata->nick) == 0)
					{
						ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^4You already have that nickname."), NULL, 0);
						List_ScrollToBottom(tabdata->box);
					}
					else if (List_GetEntryBoxAllGroups(tabdata->participantlist, allparams))
					{
						ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^4That nickname already exists."), NULL, 0);
						List_ScrollToBottom(tabdata->box);
					}
					else 
					{
						free(tabdata->nick);
						tabdata->nick = strdup(allparams);
						Ctrl_ChangeNick(tabdata->targetjid, allparams);
					}
				}
			}
			else if (strcmp(command, "/kick") == 0)
			{
				if (!nickparam)
				{
					ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^4Usage: /kick [nickname] <reason>."), NULL, 0);
					List_ScrollToBottom(tabdata->box);
				}
				else
				{
					Ctrl_KickUser(tabdata->targetjid, nickparam, commentparam);
				}
			}
			else if (strcmp(command, "/ban") == 0)
			{
				if (!nickparam)
				{
					ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^4Usage: /ban [jid] <reason>."), NULL, 0);
					List_ScrollToBottom(tabdata->box);
				}
				else
				{
					Ctrl_BanUser(tabdata->targetjid, nickparam, commentparam);
				}
			}
			else if (strcmp(command, "/unban") == 0)
			{
				if (!nickparam)
				{
					ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^4Usage: /unban [jid]."), NULL, 0);
					List_ScrollToBottom(tabdata->box);
				}
				else
				{
					Ctrl_UnbanUser(tabdata->targetjid, nickparam, commentparam);
				}
			}
			else if (strcmp(command, "/invite") == 0)
			{
				if (!nickparam)
				{
					ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^4Usage: /invite [jid] <reason>."), NULL, 0);
					List_ScrollToBottom(tabdata->box);
				}
				else
				{
					Ctrl_InviteUser(tabdata->targetjid, nickparam, commentparam);
				}
			}
			else if (strcmp(command, "/topic") == 0)
			{
				if (!allparams)
				{
					ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^4Usage: /topic [topic]."), NULL, 0);
					List_ScrollToBottom(tabdata->box);
				}
				else
				{
					Ctrl_SetTopic(tabdata->targetjid, allparams);
				}
			}
			else
			{
				ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^4Command not found."), NULL, 0);
				List_ScrollToBottom(tabdata->box);
			}

			free(command);

			Edit2Box_ClearText(pbox);
			Box_Repaint(pbox);
			return;
		}

		Ctrl_SendGroupMessage(tabdata->targetjid, text);
	}
	else
	{
		ChatBox_AddText(Box_GetRoot(pbox), tabdata->targetjid, Model_GetLoginJid(), text, NULL, 0);
		List_ScrollToBottom(tabdata->box);
		Ctrl_SendMessage(tabdata->targetjid, text);

		if (tabdata->showcomposing)
		{
			Model_UnsetComposing(tabdata->targetjid);
			tabdata->showcomposing = 0;
		}

		Box_RemoveTimedFunc(chatbox, ChatBox_TimedUnsetComposing, 5000);
	}
	
	Edit2Box_ClearText(pbox);
	Box_Repaint(pbox);
	
}

void ChatBox_OnKeyShortcut(struct Box_s *pbox, unsigned int vk, unsigned int scan)
{
	struct Box_s *chatbox = Box_GetRoot(pbox);
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	char *name = TabCtrl_GetActiveTab(data->tabctrl);
	int ctrldown, altdown, shiftdown;
	char keystate[256];

	if (!name)
	{
		return;
	}

	tabdata = ChatBox_GetChatTabContent(chatbox, name);

	if (!tabdata)
	{
		return;
	}

	GetKeyboardState(keystate);
	ctrldown =  keystate[VK_CONTROL] & 0x8000;
	altdown =   keystate[VK_MENU]    & 0x8000;
	shiftdown = keystate[VK_SHIFT]   & 0x8000;

	if (vk == VK_PRIOR /* Page up */)
	{
		List_ScrollPageUp(tabdata->box);
		return;
	}
	else if (vk == VK_NEXT /* Page down */)
	{
		List_ScrollPageDown(tabdata->box);
		return;
	}
	else if (vk == 80 && ctrldown && !altdown && shiftdown /* CTRL + SHIFT + P */)
	{
		ChatBox_ToggleShowParticipants(pbox);
		return;
	}
	else if (vk == 9 && ctrldown) /* CTRL + TAB */ 
	{
		if (shiftdown)
		{
			TabCtrl_ActivatePrevTab(data->tabctrl);
		}
		else
		{
			TabCtrl_ActivateNextTab(data->tabctrl);
		}
		return;
	}

	if (!tabdata->isGroupChat)
	{
		struct chattabcontentdata_s **userdata = malloc(sizeof(*userdata));
		*userdata = tabdata;

		if (!tabdata->showcomposing)
		{
			Model_SetComposing(tabdata->targetjid);
			tabdata->showcomposing = 1;
		}

		Box_RemoveTimedFunc(chatbox, ChatBox_TimedUnsetComposing, 5000);

		Box_AddTimedFunc(chatbox, ChatBox_TimedUnsetComposing, userdata, 5000);
	}
}

void ChatBox_OnPrefsButton(struct Box_s *pbox)
{
	struct Box_s *chatbox = Box_GetRoot(pbox);
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	int x, y, mucpower = 0;

	char *name = TabCtrl_GetActiveTab(data->tabctrl);

	if (!name)
	{
		return;
	}

	tabdata = ChatBox_GetChatTabContent(chatbox, name);

	if (!tabdata)
	{
		return;
	}

	Box_GetScreenCoords(pbox, &x, &y);
	x += pbox->w / 2;
	y += pbox->h / 2;

	if (tabdata->isGroupChat)
	{
		mucpower = ChatBox_LocalUserHasMucPower(chatbox, tabdata->targetjid);
	}

	Menu_PopupChatboxPrefsMenu(pbox, tabdata->targetjid, data->drawervisible, tabdata->isGroupChat, mucpower, tabdata->topic, x, y);
}

void ChatBox_OnAddButton(struct Box_s *pbox)
{
	struct Box_s *drawer = Box_GetRoot(pbox);
	struct chatboxdrawerdata_s *ddata = drawer->boxdata;
	struct Box_s *chatbox = ddata->chatbox;
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	int x, y;

	char *name = TabCtrl_GetActiveTab(data->tabctrl);

	if (!name)
	{
		return;
	}

	tabdata = ChatBox_GetChatTabContent(chatbox, name);

	if (!tabdata)
	{
		return;
	}

	Box_GetScreenCoords(pbox, &x, &y);
	x += pbox->w / 2;
	y += pbox->h / 2;

	Menu_PopupChatboxAddMenu(pbox, tabdata->targetjid, x, y);
}
#if 0
void ChatTab_OnTabDrop(struct Box_s *pbox, char *name, int x, int y)
{
	struct Box_s *chatbox = Box_GetRoot(pbox);
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;

	tabdata = ChatBox_GetChatTabContent(chatbox, name);

	if (!tabdata)
	{
		return;
	}

	View_OnChatTabDrop(name, tabdata->isGroupChat, x, y);
}
#endif

void ChatBox_UpdateTitlebarText(struct Box_s *chatbox)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct namedlist_s *chat = data->chats;
	char *name = TabCtrl_GetActiveTab(data->tabctrl);

	while (chat)
	{
		struct chattabcontentdata_s *tabdata = chat->data;

		if (stricmp(name, chat->name) == 0)
		{
			char txt[4096];
			char unescaped[1024];
			char *resource = Jid_GetResource(tabdata->targetjid);
			char *beforeat = Jid_GetBeforeAt(tabdata->targetjid);
			char *barejid  = Jid_Strip(tabdata->targetjid);
			int islocal = (strstr(tabdata->targetjid, "@chat.chesspark.com") != NULL);

			tabdata->participantlist->flags |= BOX_VISIBLE;

			if (tabdata->isGroupChat)
			{
				char *showname;

				if (islocal)
				{
					showname = UnescapeJID(beforeat, unescaped, 1024);
				}
				else
				{
					showname = UnescapeJID(tabdata->targetjid, unescaped, 1024);
				}

				if (!tabdata->initialconnect)
				{
					sprintf(txt, "%s - %s", showname, _("joining"));
				}
				else if (tabdata->topic)
				{
					sprintf(txt, "%s - %s", showname, tabdata->topic);
				}
				else
				{
					sprintf(txt, "%s", showname);
				}
			}
			else
			{
				struct namedlist_s **ppchat = View_GetPtrToChat(tabdata->targetjid, 1);

				if (ppchat && *ppchat && resource)
				{
					if (islocal)
					{
						i18n_stringsub(txt, 4096, _("Chatting with %1 from %2"), resource, beforeat);
					}
					else
					{
						i18n_stringsub(txt, 4096, _("Chatting with %1 from %2"), resource, barejid);
					}
				}
				else
				{
					i18n_stringsub(txt, 4096, _("Chatting with %1"), Model_GetFriendNick(tabdata->targetjid));
				}
			}
			TitleBar_SetText(chatbox->titlebar, txt);
			SetWindowText(chatbox->hwnd, txt);
			if (data->drawer)
			{
				SetWindowText(data->drawer->hwnd, txt);
			}

			return;
		}

		chat = chat->next;
	}
}

void ChatBox_UpdateTitlebarIcon(struct Box_s *chatbox)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct namedlist_s *chat = data->chats;
	char *name = TabCtrl_GetActiveTab(data->tabctrl);

	while (chat)
	{
		struct chattabcontentdata_s *tabdata = chat->data;

		if (stricmp(name, chat->name) == 0)
		{
			if (tabdata->isGroupChat)
			{
				TitleBar_SetIcon(chatbox->titlebar, ImageMgr_GetImage("groupchaticon.png"));
			}
			else
			{
				TitleBar_SetIcon(chatbox->titlebar, GetIconForStatus(Model_GetBestStatus(tabdata->targetjid)));
			}
		}

		if (tabdata->isGroupChat)
		{
			TabCtrl_SetTabIcon(data->tabctrl, chat->name, ImageMgr_GetImage("groupchaticon.png"));
		}
		else
		{
			TabCtrl_SetTabIcon(data->tabctrl, chat->name, GetIconForStatus(Model_GetBestStatus(tabdata->targetjid)));
		}

		chat = chat->next;
	}

	/*TabCtrl_RedoTabs(data->tabctrl);*/
	Box_Repaint(data->tabctrl);
}

void ChatTabCtrl_OnActivate(struct Box_s *pbox, char *name)
{
	struct Box_s *chatbox = Box_GetRoot(pbox);
	struct chatboxdata_s *data = chatbox->boxdata;
	struct namedlist_s *chat = data->chats;

	ChatBox_UpdateTitlebarText(chatbox);
	ChatBox_UpdateTitlebarIcon(chatbox);

	while (chat)
	{
		struct chattabcontentdata_s *tabdata = chat->data;

		if (stricmp(name, chat->name) == 0)
		{
			/*TabCtrl_SetTabNotify(data->tabctrl, name);*/
		}
		else
		{
			tabdata->participantlist->flags &= ~BOX_VISIBLE;
		}

		chat = chat->next;
	}

	Box_Repaint(data->drawer);
	Box_Repaint(chatbox->titlebar);
}

void ChatBox_OnSizeHeight(struct Box_s *pbox, int dheight)
{
	struct chatboxdata_s *data = pbox->boxdata;

	Box_OnSizeHeight_Stretch(pbox, dheight);

	if (data->drawervisible)
	{
		Box_MoveWndCustom2(data->drawer, pbox->x - data->drawer->w, pbox->y + 20, data->drawer->w, pbox->h - 40);
	}
}

void ChatBoxEdit_EditSizeFunc(struct Box_s *edit, int edith)
{
	struct Box_s *dialog = Box_GetRoot(edit);
	struct chatboxdata_s *data = dialog->boxdata;
	struct namedlist_s *chat;
	RECT rc;
	int dh = edith - data->edit->h;
	int newdialogh, newdialogy;

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

	rc = mi.rcWork;

	/* store old y if we're one line tall */
	if (data->edit->h <= 20)
	{
		data->normy2 = dialog->y;
	}

	newdialogy = data->normy2;
	newdialogh = dh + dialog->h;

	if (newdialogh + newdialogy > rc.bottom)
	{
		newdialogy = rc.bottom - newdialogh;
		if (newdialogy < rc.top)
		{
			newdialogy = rc.top;
			newdialogh = rc.bottom - rc.top;
			dh = (newdialogh - data->edit->y - Margin - 3) - data->edit->h;
		}
	}

	data->edit->OnSizeHeight = NULL;
	data->prefsbutton->OnSizeHeight = NULL;
	data->editborder->OnSizeHeight = NULL;

	chat = data->chats;
	while (chat)
	{
		struct chattabcontentdata_s *tabdata = chat->data;

		tabdata->box->OnSizeHeight = NULL;

		chat = chat->next;
	}
	data->tabcontentbg->OnSizeHeight = NULL;

	/* So this isn't changed */
	data->normy2 = -data->normy2;
	Box_MoveWndCustom(dialog, dialog->x, newdialogy, dialog->w, newdialogh);
	data->normy2 = -data->normy2;
		
	Box_OnSizeHeight_Stretch(data->edit, dh);
	Box_OnSizeHeight_Stretch(data->editborder, dh);

	data->edit->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	data->prefsbutton->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	data->editborder->OnSizeHeight = Box_OnSizeHeight_StickBottom;

	chat = data->chats;
	while (chat)
	{
		struct chattabcontentdata_s *tabdata = chat->data;

		tabdata->box->OnSizeHeight = Box_OnSizeHeight_Stretch;

		chat = chat->next;
	}
	data->tabcontentbg->OnSizeHeight = Box_OnSizeHeight_Stretch;

}

void ChatBox_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *chatbox = Box_GetRoot(pbox);
	struct chatboxdata_s *data = chatbox->boxdata;

	Box_OnMouseMove(pbox, xmouse, ymouse);

	if (Box_CheckXYInBox(pbox->titlebar, xmouse, ymouse) && !TitleBar_IsDragging(chatbox->titlebar)/* && Box_GetRoot(pbox)->active*/)
	{
		if (!pbox->tooltipbox)
		{
			char *name = TabCtrl_GetActiveTab(data->tabctrl);

			if (name)
			{
				struct namedlist_s **ppentry = NamedList_GetByName(&(data->chats), name);

				if (ppentry)
				{
					struct chattabcontentdata_s *tabdata = (*ppentry)->data;
					char txt[4096];
					char showname[512];

				if (tabdata->isGroupChat && strstr(tabdata->targetjid, "@chat.chesspark.com"))
				{
					char unescaped[1024];
					char *res = UnescapeJID(Jid_GetBeforeAt(tabdata->targetjid), unescaped, 1024);
					strcpy(showname, res);
				}
				else
				{
					char unescaped[1024];
					strcpy(showname, UnescapeJID(tabdata->targetjid, unescaped, 1024));
				}

				if (tabdata->topic)
				{
					sprintf(txt, "%s - %s", showname, tabdata->topic);
				}
				else
				{
					sprintf(txt, "%s", showname);
				}

					Box_AddTimedFunc(pbox, ToolTip_Popup, strdup(txt), 1000);
				}
			}
		}
	}
	else
	{
		ToolTip_PopDown(pbox);
	}
}

void ChatGrippy_OnPress(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	ChatBox_ToggleShowParticipants(dialog);
}

void DrawerGrippy_OnPress(struct Box_s *pbox)
{
	struct Box_s *drawer = Box_GetRoot(pbox);
	struct chatboxdrawerdata_s *ddata = drawer->boxdata;

	ChatBox_ToggleShowParticipants(ddata->chatbox);
}

void ChatBoxWSizer1_OnSizeHeight(struct Box_s *wsizer, int dheight)
{
	wsizer->h = (Box_GetRoot(wsizer)->h - 36) / 2 - 5;
}

void ChatBoxWSizer2_OnSizeHeight(struct Box_s *wsizer, int dheight)
{
	wsizer->y = (Box_GetRoot(wsizer)->h - 36) / 2 + 36;
	wsizer->h = (Box_GetRoot(wsizer)->h - 36) / 2 - 5;
}

void DrawerWSizer1_OnSizeHeight(struct Box_s *wsizer, int dheight)
{
	wsizer->h = (Box_GetRoot(wsizer)->h - 36) / 2;
}

void DrawerWSizer2_OnSizeHeight(struct Box_s *wsizer, int dheight)
{
	wsizer->y = (Box_GetRoot(wsizer)->h - 36) / 2 + 36;
	wsizer->h = (Box_GetRoot(wsizer)->h - 36) / 2;
}

void Grippy_OnDestroy(struct Box_s *grippy)
{
	Button_OnDestroy(grippy);

	Box_UnlockMouseCursorImg(grippy);
}

void Grippy_OnMouseMove(struct Box_s *grippy, int xmouse, int ymouse)
{
	Button_OnMouseMove(grippy, xmouse, ymouse);

	Box_UnlockMouseCursorImg(grippy);

	if (Box_CheckXYInBox(grippy, xmouse, ymouse))
	{
		Box_LockMouseCursorImg(grippy, LoadCursor(NULL, IDC_HAND));
	}
}

void ChatBox_ForceUnDrag(struct Box_s *dialog)
{
	struct chatboxdata_s *data = dialog->boxdata;

	/*
	Log_Write(0, "ChatBox_ForceUnDrag(%d)\n", dialog);
	Log_Write(0, "Box_IsDragging() %d\n", Box_IsDragging);
	*/
	if (Box_IsDragging())
	{
		Box_DragEnd(data->tabctrl);
	}

	/*
	Log_Write(0, "data->overotherchat %d\n", data->overotherchat);
	*/
		
	if (data->overotherchat)
	{
		struct chatboxdata_s *cdata = data->overotherchat->boxdata;

		ShowWindow(dialog->hwnd, SW_SHOW);
		if (data->drawervisible)
		{
			ShowWindow(data->drawer->hwnd, SW_SHOW);
		}

		ChatBox_SetShowTabs(data->overotherchat, TabCtrl_GetNumTabs(cdata->tabctrl) > 1);
		TabCtrl_HideDropIndicator(cdata->tabctrl);
		Box_Repaint(data->overotherchat);
		data->overotherchat = NULL;
	}

	/*
	Log_Write(0, "TitleBar_IsDragging(dialog->titlebar) %d\n", TitleBar_IsDragging(dialog->titlebar));
	*/

	if (TitleBar_IsDragging(dialog->titlebar))
	{
		TitleBar_ForceUndrag(dialog->titlebar);
	}
}

void ChatTitleBar_OnMouseMove(struct Box_s *titlebar, int xmouse, int ymouse)
{
	struct Box_s *dialog = Box_GetRoot(titlebar);
	struct chatboxdata_s *data = dialog->boxdata;
	int buttondown = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

	/*
	Log_Write(0, "ChatTitleBar_OnMouseMove(%d (%d), %d, %d)\n", titlebar, dialog, xmouse, ymouse);
	Log_Write(0, "buttondown %d data->overotherchat %d\n", buttondown, data->overotherchat);
	*/

	if (!buttondown && data->overotherchat)
	{
		ChatBox_ForceUnDrag(dialog);
		TitleBar_OnMouseMove(titlebar, xmouse, ymouse);
		return;
	}

	/*
	Log_Write(0, "TitleBar_IsDragging(titlebar) == %d\n", TitleBar_IsDragging(titlebar));
	*/
	if (Model_GetOption(OPTION_ENABLECHATMERGING) && TitleBar_IsDragging(titlebar))
	{
		/* Make sure the mouse button is down, in case we missed an lbuttonup. */
		if (buttondown)
		{
			HWND hwnd2;
			POINT pt;
			struct Box_s *over = NULL;

			Box_GetScreenCoords(titlebar, &(pt.x), &(pt.y));
			pt.x += xmouse;
			pt.y += ymouse;

			hwnd2 = WindowFromPoint(pt);

			/*
			Log_Write(0, "hwnd2 %d dialog->hwnd %d\n", hwnd2, dialog->hwnd);
			*/
			if (hwnd2 == dialog->hwnd)
			{
				/* calculate a point just above the window */
				POINT pt2;
				pt2.x = pt.x;
				pt2.y = dialog->y - 1;
				hwnd2 = WindowFromPoint(pt2);
			}

			/*
			Log_Write(0, "hwnd2 %d dialog->hwnd %d\n", hwnd2, dialog->hwnd);
			*/
			if (hwnd2 && hwnd2 != dialog->hwnd)
			{
				struct Box_s *hoverdialog = Box_GetChessparkBox(hwnd2);
				/*
				Log_Write(0, "hoverdialog %d\n", hoverdialog);
				*/
				if (hoverdialog)
				{
					/*
					Log_Write(0, "hoverdialog->boxtypeid %d\n", hoverdialog->boxtypeid);
					*/
					if (hoverdialog->boxtypeid == BOXTYPE_CHATBOX)
					{
						over = hoverdialog;
					}
				}
			}

			if (GetAsyncKeyState(VK_MENU) & 0x8000)
			{
				over = NULL;
			}

			/*
			Log_Write(0, "data->overotherchat %d over %d\n", data->overotherchat, over);
			*/
			if (data->overotherchat && data->overotherchat != over)
			{
				struct chatboxdata_s *cdata = data->overotherchat->boxdata;
				Box_DragEnd(data->tabctrl);
				ChatBox_SetShowTabs(data->overotherchat, TabCtrl_GetNumTabs(cdata->tabctrl) > 1);
				TabCtrl_HideDropIndicator(cdata->tabctrl);
				Box_Repaint(data->overotherchat);
				data->overotherchat = NULL;
			}

			/*
			Log_Write(0, "data->overotherchat %d over %d\n", data->overotherchat, over);
			*/
			if (over && data->overotherchat != over)
			{
				int oldflags = data->tabctrl->flags;
				int hdiff = 23 - data->tabctrl->h;
				int width = TabCtrl_GetTabsWidth(data->tabctrl);
				int oldwidth = data->tabctrl->w;
				data->tabctrl->flags |= BOX_VISIBLE;
				TabCtrl_SetDragImg(data->tabctrl);
				Box_OnSizeHeight_Stretch(data->tabctrl, hdiff);
				data->tabctrl->w = width;
				Box_DragStart(data->tabctrl, width/2, data->tabctrl->h / 2, FRIENDTABDROPDATA_ID);
				data->tabctrl->w = oldwidth;
				TabCtrl_UnsetDragImg(data->tabctrl);
				data->tabctrl->flags = oldflags;
				Box_OnSizeHeight_Stretch(data->tabctrl, -hdiff);
				ChatBox_SetShowTabs(over, 1);
				Box_Repaint(over);
				data->overotherchat = over;
			}

			/*
			Log_Write(0, "data->overotherchat %d\n", data->overotherchat);
			*/
			if (!data->overotherchat)
			{
				ShowWindow(dialog->hwnd, SW_SHOW);
				if (data->drawervisible)
				{
					ShowWindow(data->drawer->hwnd, SW_SHOW);
				}
			}
			else
			{
				ShowWindow(dialog->hwnd, SW_HIDE);
				if (data->drawervisible)
				{
					ShowWindow(data->drawer->hwnd, SW_HIDE);
				}
			}

			/*
			Log_Write(0, "data->overotherchat %d\n", data->overotherchat);
			*/
			if (data->overotherchat)
			{
				struct chatboxdata_s *cdata = over->boxdata;
				int x, y;
				x = pt.x - over->x - cdata->tabctrl->x;
				y = pt.y - over->y - cdata->tabctrl->y;
				TabCtrl_ShowDropIndicator(cdata->tabctrl, x, y);
				Box_DragMove(data->tabctrl, pt.x, pt.y);
			}
		}
		else
		{
			ChatBox_ForceUnDrag(dialog);
		}
	}

	TitleBar_OnMouseMove(titlebar, xmouse, ymouse);
}

void ChatTitleBar_OnLButtonUp(struct Box_s *titlebar, int xmouse, int ymouse)
{
	struct Box_s *dialog = Box_GetRoot(titlebar);
	struct chatboxdata_s *data = dialog->boxdata;

	/*
	Log_Write(0, "ChatTitleBar_OnLButtonUp(%d (%d), %d, %d)\n", titlebar, dialog, xmouse, ymouse);
	Log_Write(0, "data->overotherchat %d\n", data->overotherchat);
	*/

	if (data->overotherchat)
	{
		int sx, sy;
		struct chatboxdata_s *dstdata = data->overotherchat->boxdata;
		int x, y;
		int droppos = -1;

		Box_GetScreenCoords(titlebar, &sx, &sy);
		sx += xmouse;
		sy += ymouse;
		x = sx - data->overotherchat->x - dstdata->tabctrl->x;
		y = sy - data->overotherchat->y - dstdata->tabctrl->y;
		if (Box_CheckXYInBox(dstdata->tabctrl, x, y))
		{
			droppos = TabCtrl_GetDropPos(dstdata->tabctrl, x, y);
		}

		Box_DragEnd(data->tabctrl);
		/* move all chats to the new dialog */
		while (data->chats)
		{
			char *name = strdup(data->chats->name);

			ChatBox_MoveChat(dialog, data->overotherchat, name);
			TabCtrl_MoveTab(dstdata->tabctrl, name, droppos);
			TabCtrl_ActivateTabAndScrollVisible(dstdata->tabctrl, name);
			droppos++;

			free(name);
		}

		TitleBar_OnLButtonUp(titlebar, xmouse, ymouse);

		View_CloseChatDialog(dialog);
		return;
	}

	TitleBar_OnLButtonUp(titlebar, xmouse, ymouse);
}

void ChatBoxTab_OnSpecialDrag(struct Box_s *tab, int xmouse, int ymouse, char *name)
{
	int sx, sy;
	struct Box_s *newchat;
	struct Box_s *srcchat = Box_GetRoot(tab);

	/*
	Log_Write(0, "ChatBoxTab_OnSpecialDrag(%d (%d), %d, %d, %s)\n", tab, srcchat, xmouse, ymouse, name);
	*/

	/* don't allow dragging if chat merging is off */
	if (!Model_GetOption(OPTION_ENABLECHATMERGING))
	{
		return;
	}

	/* create a chatbox */
	Box_GetScreenCoords(tab, &sx, &sy);
	sx += xmouse;
	sy += ymouse;

	newchat = View_CreateEmptyChatBox(sx - 200, sy - 5);
	TitleBar_SetDragging(newchat->titlebar, -200, -5);
	ChatBox_MoveChat(srcchat, newchat, name);
}

void ChatTitleBar_OnLoseMouseCapture(struct Box_s *title)
{
	struct Box_s *chatbox = Box_GetRoot(title);

	struct chatboxdata_s *data = chatbox->boxdata;

	/*
	Log_Write(0, "ChatTitleBar_OnLoseMouseCapture(%d (%d))\n", title, chatbox);
	Log_Write(0, "TitleBar_IsDragging(chatbox->titlebar) %d data->overotherchat %d\n", TitleBar_IsDragging(chatbox->titlebar), data->overotherchat);
	*/
	if (data->overotherchat)
	{
		struct chatboxdata_s *cdata = data->overotherchat->boxdata;
		ShowWindow(chatbox->hwnd, SW_SHOW);
		if (data->drawervisible)
		{
			ShowWindow(data->drawer->hwnd, SW_SHOW);
		}
		Box_DragEnd(data->tabctrl);
		ChatBox_SetShowTabs(data->overotherchat, TabCtrl_GetNumTabs(cdata->tabctrl) > 1);
		TabCtrl_HideDropIndicator(cdata->tabctrl);
		Box_Repaint(data->overotherchat);
		data->overotherchat = NULL;
	}

	TitleBar_OnLoseMouseCapture(title);
}

void ChatBox_OnActive(struct Box_s *chatbox)
{
	View_SetLastActiveChatBox(chatbox);
	TitleBarRoot_OnActive(chatbox);
}

struct Box_s *ChatBox_Create(int x, int y, int w, int h, enum Box_flags flags, struct Box_s *roster, int group)
{
	struct Box_s *dialog = Box_Create(x, y, w, h, flags);
	struct Box_s *drawer;
	struct Box_s *pbox;
	struct Box_s *grippy;
	struct chatboxdata_s *data = malloc(sizeof(*data));
	struct chatboxdrawerdata_s *ddata = malloc(sizeof(*ddata));
	
	memset (data, 0, sizeof(*data));
	memset (ddata, 0, sizeof(*ddata));
	
	dialog->bgcol = DefaultBG;
	dialog->OnSizeWidth = Box_OnSizeWidth_Stretch;
	dialog->OnSizeHeight = ChatBox_OnSizeHeight;
	dialog->minw = 256;
	dialog->minh = 256;
	dialog->OnMouseMove = ChatBox_OnMouseMove;
	dialog->OnDragDrop = ChatBox_OnDragDrop;
	dialog->boxtypeid = BOXTYPE_CHATBOX;

	dialog->titlebar = TitleBarIcon_Add(dialog, _("Chesspark Chat"), ChatBox_OnClose, ChatBox_OnMinimize, ChatBox_OnMaximize);
	dialog->titlebar->OnMouseMove = ChatTitleBar_OnMouseMove;
	dialog->titlebar->OnLButtonUp = ChatTitleBar_OnLButtonUp;
	dialog->titlebar->OnLoseMouseCapture = ChatTitleBar_OnLoseMouseCapture;
	dialog->OnActive = ChatBox_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	/* requires a custom sizer set because of the grippy
	SizerSet_Create(dialog);
	*/
	{
		struct Box_s *parent = dialog;
		struct Box_s *sizerset = Box_Create(0, 0, parent->w, parent->h, BOX_VISIBLE | BOX_TRANSPARENT);
		struct Box_s *wsizer1, *wsizer2;
		sizerset->OnSizeHeight = Box_OnSizeHeight_Stretch;
		sizerset->OnSizeWidth = Box_OnSizeWidth_Stretch;
		Box_AddChild(parent, sizerset);

		wsizer1 = WSizer_Create(0, 4, 5, (parent->h - 36) / 2 - 5);
		wsizer2 = WSizer_Create(0, (parent->h - 36) / 2 + 36, 5, (parent->h - 36) / 2 - 5);
		wsizer1->OnSizeHeight = ChatBoxWSizer1_OnSizeHeight;
		wsizer2->OnSizeHeight = ChatBoxWSizer2_OnSizeHeight;

		Box_AddChild(sizerset, NWSizer_Create(0, 0, 4, 4));
		Box_AddChild(sizerset, NSizer_Create(5, 0, parent->w - 11, 4));
		Box_AddChild(sizerset, NESizer_Create(parent->w - 5, 0, 4, 4));
		Box_AddChild(sizerset, wsizer1);
		Box_AddChild(sizerset, wsizer2);
		Box_AddChild(sizerset, ESizer_Create(parent->w - 5, 5, 4, parent->h - 10));
		Box_AddChild(sizerset, SWSizer_Create(0, parent->h - 5, 4, 4));
		Box_AddChild(sizerset, SSizer_Create(5, parent->h - 4, parent->w - 10, 4));
		Box_AddChild(sizerset, SESizer_Create(parent->w - 5, parent->h - 5, 4, 4));
	}

	/* Tab BG */
	pbox = Box_Create(Margin, TitleBarHeight + Margin/* + 19*/, dialog->w - Margin * 2, dialog->h - Margin - 12/* - 20*/ - TitleBarHeight - 5 - 19, BOX_VISIBLE | BOX_BORDER);
	pbox->bgcol = TabBG2;
	pbox->brcol = RGB(176,178,183);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(dialog, pbox);
	data->tabcontentbg = pbox;
	
	{
		struct Box_s *subbox;
		/*
		subbox = Box_Create(0, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcornersul", "contentcorners.png", 0, 0, 5, 5);
		Box_AddChild(pbox, subbox);
		*/

		subbox = Box_Create(pbox->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcornersur", "contentcorners.png", 5, 0, 5, 5);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(pbox, subbox);

		subbox = Box_Create(pbox->w - 5, pbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcornersdr", "contentcorners.png", 5, 5, 5, 5);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(pbox, subbox);

		subbox = Box_Create(0, pbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcornersdl", "contentcorners.png", 0, 5, 5, 5);
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(pbox, subbox);
	}

	pbox = TabCtrl_Create(Margin, TitleBarHeight + Margin, w - Margin * 2, 20, BOX_TRANSPARENT, 1, 0, FRIENDTABDROPDATA_ID);
	pbox->fgcol = CR_DkOrange;
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	data->tabctrl = pbox;
	TabCtrl_SetOnSpecialDrag(pbox, ChatBoxTab_OnSpecialDrag);
	Box_AddChild(dialog, pbox);

	/*
	TabCtrl_SetTabDropFunc(pbox, ChatTab_OnTabDrop);
	*/
	TabCtrl_SetTabActivateFunc(pbox, ChatTabCtrl_OnActivate);
/*
	pbox = List_Create(Margin, TitleBarHeight + Margin + 20, w - Margin * 2, h - Margin - 52 - TitleBarHeight, BOX_VISIBLE, FALSE);
	pbox->bgcol = TabBG2;
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	data->list = pbox;
	Box_AddChild(dialog, pbox);

	TabCtrl_AddTab(data->tabctrl, targetjid, data->list, 90);
*/
	pbox = Button_Create(Margin, h - Margin - ButtonHeight - 3, 30, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->bgcol = RGB(128, 128, 128);
	Button_SetNormalImg (pbox, ImageMgr_GetSubImage("preferenceMenuButton1", "preferenceMenu.png", 0,  0, 30, 23));
	Button_SetPressedImg(pbox, ImageMgr_GetSubImage("preferenceMenuButton2", "preferenceMenu.png", 30, 0, 30, 23));
	Button_SetHoverImg  (pbox, ImageMgr_GetSubImage("preferenceMenuButton3", "preferenceMenu.png", 60, 0, 30, 23));
	Button_SetOnButtonHit(pbox, ChatBox_OnPrefsButton);
	Button_SetTooltipText(pbox, _("Preferences Menu"));
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	data->prefsbutton = pbox;
	Box_AddChild(dialog, pbox);
	
	pbox = Box_Create(ButtonWidth + Margin * 2, h - Margin - 25 - 1, w - ButtonWidth - Margin * 3, 22, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	data->editborder = pbox;
	Box_AddChild(dialog, pbox);

	{
		struct Box_s *subbox;

		subbox = Box_Create(0, 0, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("editborderul", "SearchBorder.png", 0, 0, 6, 6);
		Box_AddChild(pbox, subbox);

		subbox = Box_Create(6, 0, pbox->w - 12, 6, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		subbox->img = ImageMgr_GetSubImage("editborderu", "SearchBorder.png", 6, 0, 230 - 12, 6);
		Box_AddChild(pbox, subbox);

		subbox = Box_Create(pbox->w - 6, 0, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox->img = ImageMgr_GetSubImage("editborderur", "SearchBorder.png", 230 - 6, 0, 6, 6);
		Box_AddChild(pbox, subbox);


		subbox = Box_Create(0, 6, 6, pbox->h - 12, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
		subbox->img = ImageMgr_GetSubImage("editborderl", "SearchBorder.png", 0, 6, 6, 25 - 12);
		Box_AddChild(pbox, subbox);
		
		subbox = Box_Create(pbox->w - 6, 6, 6, pbox->h - 12, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
		subbox->img = ImageMgr_GetSubImage("editborderr", "SearchBorder.png", 230 - 6, 6, 6, 25 - 12);
		Box_AddChild(pbox, subbox);


		subbox = Box_Create(0, pbox->h - 6, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		subbox->img = ImageMgr_GetSubImage("editborderdl", "SearchBorder.png", 0, 25 - 6, 6, 6);
		Box_AddChild(pbox, subbox);

		subbox = Box_Create(6, pbox->h - 6, pbox->w - 12, 6, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		subbox->img = ImageMgr_GetSubImage("editborderd", "SearchBorder.png", 6, 25 - 6, 230 - 12, 6);
		Box_AddChild(pbox, subbox);

		subbox = Box_Create(pbox->w - 6, pbox->h - 6, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		subbox->img = ImageMgr_GetSubImage("editborderdr", "SearchBorder.png", 230 - 6, 25 - 6, 6, 6);
		Box_AddChild(pbox, subbox);
	}

	pbox = Edit2Box_Create(ButtonWidth + Margin * 2 + 5, h - Margin - 21 - 3, w - ButtonWidth - Margin * 3 - 10, 18, BOX_VISIBLE, 0);
	pbox->bgcol = RGB(241, 241, 241);
	pbox->fgcol = RGB(0, 0, 0);
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Edit2Box_SetOnEnter(pbox, ChatBox_OnEnter);
	Edit2Box_SetEditSizeFunc(pbox, ChatBoxEdit_EditSizeFunc);
	data->edit = pbox;
	Box_AddChild(dialog, pbox);

	pbox = Box_Create(w - 10, h - 10, 10, 10, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->OnSizeWidth  = Box_OnSizeWidth_StickRight;
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	pbox->img = ImageMgr_GetImage("windowResizeHandle.png");
	Box_AddChild(dialog, pbox);

	dialog->OnClose = ChatBox_OnClose;
	dialog->OnMinimize = ChatBox_OnMinimize;
	dialog->OnMaximize = ChatBox_OnMaximize;
	dialog->OnRestore = ChatBox_OnRestore;

	dialog->OnDestroy = ChatBox_OnDestroy;
	dialog->OnClose = ChatBox_OnClose;
	dialog->boxdata = data;
	dialog->OnCommand = Menu_OnCommand;


	drawer = Box_Create(dialog->x - 80, dialog->y + 20, 150, dialog->h - 40, BOX_VISIBLE);
	drawer->bgcol = DrawerBG;
	drawer->OnCommand = Menu_OnCommand;
	data->drawersavedw = 150;

	grippy = Button_Create(0, (dialog->h - 36) / 2, 5, 36, BOX_VISIBLE | BOX_TRANSPARENT);
	grippy->OnSizeHeight = Box_OnSizeHeight_Center;
	Button_SetNormalImg (grippy, ImageMgr_GetSubImage("drawerCloseButton4", "drawerCloseButton.png", 15, 0, 5, 36));
	Button_SetHoverImg  (grippy, ImageMgr_GetSubImage("drawerCloseButton5", "drawerCloseButton.png", 20, 0, 5, 36));
	Button_SetPressedImg(grippy, ImageMgr_GetSubImage("drawerCloseButton6", "drawerCloseButton.png", 25, 0, 5, 36));
	Button_SetOnButtonHit(grippy, ChatGrippy_OnPress);
	grippy->OnDestroy = Grippy_OnDestroy;
	grippy->OnMouseMove = Grippy_OnMouseMove;
	Box_AddChild(dialog, grippy);
	data->grippy = grippy;

	pbox = Box_Create(5, 5, drawer->w - 5, drawer->h - 22 - 5 - 5 - 5, BOX_VISIBLE | BOX_BORDER);
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	pbox->bgcol = RGB(180, 184, 165);
	pbox->brcol = RGB(90, 97, 108);
	Box_AddChild(drawer, pbox);
	
	{
		struct Box_s *subbox;
		subbox = Box_Create(0, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcorners3ul", "contentcorners3.png", 0, 0, 5, 5);
		Box_AddChild(pbox, subbox);

		subbox = Box_Create(pbox->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcorners3ur", "contentcorners3.png", 5, 0, 5, 5);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(pbox, subbox);

		subbox = Box_Create(pbox->w - 5, pbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcorners3dr", "contentcorners3.png", 5, 5, 5, 5);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(pbox, subbox);

		subbox = Box_Create(0, pbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcorners3dl", "contentcorners3.png", 0, 5, 5, 5);
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(pbox, subbox);
	}



	/* requires a custom sizer set because of the grippy
	pbox = DrawerSizer_Create(0, 0, 4, drawer->h);
	Box_AddChild(drawer, pbox);
	*/
	{
		struct Box_s *wsizer1, *wsizer2;

		wsizer1 = WSizer_Create(0, 0, 5, (drawer->h - 36) / 2);
		wsizer2 = WSizer_Create(0, (drawer->h - 36) / 2 + 36, 5, (drawer->h - 36) / 2);
		wsizer1->OnSizeHeight = DrawerWSizer1_OnSizeHeight;
		wsizer2->OnSizeHeight = DrawerWSizer2_OnSizeHeight;

		Box_AddChild(drawer, wsizer1);
		Box_AddChild(drawer, wsizer2);
	}
/*
	pbox = List_Create(5, 5, drawer->w - 10, drawer->h - 22 - 5 - 5 - 5, BOX_VISIBLE, 0);
	pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(drawer, pbox);
*/
	pbox = Button_Create(5, drawer->h - 22 - 5, 21, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	Button_SetNormalImg (pbox, ImageMgr_GetSubImage("addButton1", "addButtonFullStrip.png", 0,  0, 21, 23));
	Button_SetPressedImg(pbox, ImageMgr_GetSubImage("addButton2", "addButtonFullStrip.png", 21, 0, 21, 23));
	Button_SetHoverImg  (pbox, ImageMgr_GetSubImage("addButton3", "addButtonFullStrip.png", 42, 0, 21, 23));
	Button_SetOnButtonHit(pbox, ChatBox_OnAddButton);
	Button_SetTooltipText(pbox, _("Add Menu"));
	pbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_AddChild(drawer, pbox);
	
	grippy = Button_Create(0, (dialog->h - 36) / 2, 5, 36, BOX_VISIBLE | BOX_TRANSPARENT);
	grippy->OnSizeHeight = Box_OnSizeHeight_Center;
	Button_SetNormalImg (grippy, ImageMgr_GetSubImage("drawerCloseButton1", "drawerCloseButton.png", 0, 0, 5, 36));
	Button_SetHoverImg  (grippy, ImageMgr_GetSubImage("drawerCloseButton2", "drawerCloseButton.png", 5, 0, 5, 36));
	Button_SetPressedImg(grippy, ImageMgr_GetSubImage("drawerCloseButton3", "drawerCloseButton.png", 10, 0, 5, 36));
	Button_SetOnButtonHit(grippy, DrawerGrippy_OnPress);
	grippy->OnDestroy = Grippy_OnDestroy;
	grippy->OnMouseMove = Grippy_OnMouseMove;
	Box_AddChild(drawer, grippy);

	data->drawer = drawer;
	data->windowgroup = group;

	ddata->chatbox = dialog;
	drawer->boxdata = ddata;

	Box_CreateWndCustom(dialog, _("Chesspark Chat"), roster->hwnd);
	
	dialog->OnSizeHeight  = ChatBox_OnSizeHeight;
	dialog->OnMove        = ChatBox_OnMove;
	dialog->OnKeyShortcut = ChatBox_OnKeyShortcut;

	Box_SetFocus(data->edit);

	return dialog;
}

void ChatBox_AddTimeStamp(struct Box_s *pbox, char *targetjid, char *timestamp)
{
	struct chatboxdata_s *data = pbox->boxdata;
	struct chattabcontentdata_s *tabdata;

	tabdata = ChatBox_GetChatTabContent(pbox, targetjid);

	if (!tabdata)
	{
		return;
	}

	tabdata->lastchattime = SubChat_AddTimeStamp2(tabdata->box, tabdata->isGroupChat ? NULL : Model_GetFriendNick(tabdata->targetjid), timestamp, data->edit, tabdata->logfile);
}

void ChatTab_OnClose(struct Box_s *tab, char *name)
{
	struct Box_s *dialog = Box_GetRoot(tab);
	struct chatboxdata_s *data = dialog->boxdata;

	ChatBox_RemoveChat(dialog, name);

	if (!TabCtrl_GetFirstTab(data->tabctrl))
	{
		View_CloseChatDialog(dialog);
	}
}

void ChatTab_OnEmptyDrop(struct Box_s *psrc, int x, int y, int dragid, void *dragdata)
{
	char *jid = dragdata;

	View_OnChatTabDropEmpty(Box_GetRoot(psrc), jid, x, y);
}

void ChatBox_AddChat(struct Box_s *pbox, char *targetjid, char *nick, int isGroupChat)
{
	struct chatboxdata_s *data = pbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	struct Box_s *chatbox, *participantlist;
	char *showname;
	char unescaped[1024];

	chatbox = List_Create(Margin + 5, TitleBarHeight + Margin + 5 + (data->tabsvisible ? 19 : 0), pbox->w - Margin * 2 - 10, pbox->h - Margin - 12 - (data->tabsvisible ? 20 : 0) - TitleBarHeight - 10 - 5 - 19, 0, FALSE);
	chatbox->bgcol = TabBG2;
	chatbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	chatbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	List_SetEntrySelectable(chatbox, FALSE);
	List_SetStickyBottom(chatbox, 1);
	List_SetEntryLimit(chatbox, 1000);
	Box_AddChild(pbox, chatbox);

	participantlist = List_Create(10, 10, data->drawer->w - 15, data->drawer->h - 22 - 5 - 10 - 10, 0, 0);
	participantlist->OnSizeHeight = Box_OnSizeHeight_Stretch;
	participantlist->OnSizeWidth = Box_OnSizeWidth_Stretch;
	List_SetStripeBG1(participantlist, RGB(180, 184, 165));
	List_SetStripeBG2(participantlist, RGB(180, 184, 165));
	List_SetGroupSelectable(participantlist, FALSE);
	List_SetGroupCollapsible(participantlist, FALSE);
	List_SetHideDisclosureTriangles(participantlist, TRUE);
	List_SetEntrySortFunc(participantlist, ParticipantEntry_SortFunc);
	Box_AddChild(data->drawer, participantlist);

	if (isGroupChat)
	{
		if (strstr(targetjid, "@chat.chesspark.com"))
		{
			showname = strdup(UnescapeJID(Jid_GetBeforeAt(targetjid), unescaped, 1024));
		}
		else
		{
			showname = strdup(UnescapeJID(targetjid, unescaped, 1024));
		}
	}
	else
	{
		showname = Model_GetFriendNick(targetjid);
	}


	TabCtrl_AddTab2(data->tabctrl, targetjid, showname, chatbox, 100,
		isGroupChat ? ImageMgr_GetImage("groupchaticon.png") : GetIconForStatus(Model_GetBestStatus(targetjid)), ChatTab_OnClose,
		FRIENDTABDROPDATA_ID, isGroupChat ? Jid_Strip(targetjid) : strdup(targetjid), ChatTab_OnEmptyDrop);

	tabdata = malloc(sizeof(*tabdata));
	memset(tabdata, 0, sizeof(*tabdata));

	tabdata->box = chatbox;
	tabdata->isGroupChat = isGroupChat;
	tabdata->nick = strdup(nick);
	if (isGroupChat)
	{
		tabdata->targetjid = Jid_Strip(targetjid);
	}
	else
	{
		tabdata->targetjid = strdup(targetjid);
	}
	tabdata->participantlist = participantlist;

	NamedList_Add(&(data->chats), targetjid, tabdata, ChatTabContent_OnDestroy);

	if (!isGroupChat)
	{
		struct Box_s *entrybox;
		ChatBox_AddTimeStamp(pbox, targetjid, NULL);

		if (!List_GetGroupBox(tabdata->participantlist, _("Participants")))
		{
			List_AddGroup(tabdata->participantlist, _("Participants"));
		}

		entrybox = ParticipantEntry_Create(Jid_Strip(Model_GetLoginJid()), Model_GetFriendNick(Model_GetLoginJid()), Model_GetBestStatus(Model_GetLoginJid()), NULL, NULL, NULL, Model_GetLoginJid(), tabdata->isGroupChat, NULL, NULL, 0, NULL);
		List_AddEntry(tabdata->participantlist, Jid_Strip(Model_GetLoginJid()), _("Participants"), entrybox);
		entrybox = ParticipantEntry_Create(targetjid, Model_GetFriendNick(targetjid), Model_GetBestStatus(targetjid), NULL, NULL, NULL, targetjid, tabdata->isGroupChat, NULL, NULL, 0, NULL);
		List_AddEntry(tabdata->participantlist, Jid_Strip(targetjid), _("Participants"), entrybox);

		List_RedoEntries(tabdata->participantlist);
		Box_Repaint(tabdata->participantlist);

		/*
		ChatBox_SetParticipantStatus(pbox, targetjid, nick, SSTAT_AVAILABLE, NULL, "participant", "privatechat", NULL, 0);
		ChatBox_SetParticipantStatus(pbox, targetjid, targetjid, SSTAT_AVAILABLE, NULL, "participant", "privatechat", targetjid, 0);
		*/

		/* Cache profile so titles show */
		Model_GetProfile(targetjid, 1);
	}

	{
		char filename[MAX_PATH];
		char savepath[MAX_PATH];
		char *barejid, *fbarejid;
		char *resource, *fresource;
		struct namedlist_s **ppchat;
		char path[MAX_PATH];
		FILE *fp;

		barejid = Jid_Strip(targetjid);
		fbarejid = Util_StripInvalidFilenameCharacters(barejid);
		resource = Jid_GetResource(targetjid);
		fresource = Util_StripInvalidFilenameCharacters(resource);

		ppchat = View_GetPtrToChat(barejid, 1);

		Util_GetPublicSavePath(savepath, MAX_PATH);
		
		strcat(savepath, "/Chats");
		mkdir(savepath);

		if (!isGroupChat && resource && ppchat && *ppchat)
		{
			/* groupchat to participant, put under groupchat directory */
			sprintf(filename, "%s/%s", savepath, fbarejid);
			mkdir(filename);

			sprintf(filename, "%s/%s/%s", savepath, fbarejid, fresource);
			mkdir(filename);

			strcpy(path, filename);

			sprintf(filename, "%s/%s/%s/%s", savepath, fbarejid, fresource, Info_GetLocalDate());
		}
		else if (isGroupChat)
		{
			sprintf(filename, "%s/%s", savepath, fbarejid);
			mkdir(filename);

			strcpy(path, filename);

			sprintf(filename, "%s/%s/%s.log", savepath, fbarejid, Info_GetLocalDate());
		}
		else
		{
			char *nick, *fnick;

			nick = Model_GetFriendNick(targetjid);
			fnick = Util_StripInvalidFilenameCharacters(nick);

			sprintf(filename, "%s/%s", savepath, fnick);
			mkdir(filename);

			strcpy(path, filename);

			sprintf(filename, "%s/%s/%s.log", savepath, fnick, Info_GetLocalDate());
		}

		if (!isGroupChat)
		{
			HANDLE hff;
			WIN32_FIND_DATA wfd;
			char filename2[MAX_PATH];
			char filename3[MAX_PATH];
			unsigned int hft = 0;
			unsigned int lft = 0;

			filename2[0] = '\0';

			/* find last log file */
			strcpy(filename3, path);
			strcat(filename3, "/*.log");
			hff = FindFirstFile(filename3, &wfd);

			if (hff != INVALID_HANDLE_VALUE)
			{
				FILE *fp;

				if (wfd.ftLastWriteTime.dwHighDateTime > hft || (wfd.ftLastWriteTime.dwHighDateTime == hft && wfd.ftLastWriteTime.dwLowDateTime > lft))
				{
					hft = wfd.ftLastWriteTime.dwHighDateTime;
					lft = wfd.ftLastWriteTime.dwLowDateTime;
                                        strcpy(filename2, wfd.cFileName);
				}

				while(FindNextFile(hff, &wfd))
				{
					if (wfd.ftLastWriteTime.dwHighDateTime > hft || (wfd.ftLastWriteTime.dwHighDateTime == hft && wfd.ftLastWriteTime.dwLowDateTime > lft))
					{
						hft = wfd.ftLastWriteTime.dwHighDateTime;
						lft = wfd.ftLastWriteTime.dwLowDateTime;
		                                strcpy(filename2, wfd.cFileName);
					}
				}

				FindClose(hff);

				/* append history */

				strcpy(filename3, path);
				strcat(filename3, "/");
				strcat(filename3, filename2);

				fp = fopen(filename3, "r");

				fseek(fp, 0, SEEK_END);
				if (ftell(fp) > 1025)
				{
					int c;
					fseek(fp, -1025, SEEK_END);
					while ((c = fgetc(fp)) != '\n' && c != EOF);
					if (c == -1)
					{
						fseek(fp, -1025, SEEK_END);
					}
				}
				else
				{
					fseek(fp, 0, SEEK_SET);
				}

				{
					char line[1024];

					while (fgets(line, 1024, fp))
					{
						int len;
						line[1023] = '\0';
						len = strlen(line);
						if (len > 1 && line[len-1] == '\n')
						{
							line[len-1] = '\0';
						}
						
						if (strncmp(line, "---", 3) != 0)
						{
							SubChat_AddHistory(tabdata->box, line);
						}
					}
				}

				fclose(fp);
			}
		}

		tabdata->logfile = strdup(filename);
	}

	ChatBox_SetShowTabs(pbox, TabCtrl_GetNumTabs(data->tabctrl) > 1);

	if (!isGroupChat)
	{
		Model_GetProfile(targetjid, 1);
	}
}

void ChatBox_RemoveChat(struct Box_s *pbox, char *targetjid)
{
	struct chatboxdata_s *data = pbox->boxdata;

	NamedList_RemoveByName(&(data->chats), targetjid);
	TabCtrl_RemoveTab(data->tabctrl, targetjid);

	ChatBox_SetShowTabs(pbox, TabCtrl_GetNumTabs(data->tabctrl) > 1);
}

void ChatBox_ActivateChat(struct Box_s *pbox, char *targetjid, char *nick, int isGroupChat, int selecttab)
{
	struct chatboxdata_s *data = pbox->boxdata;
	int notabs = (data->chats == NULL);
	if (!ChatBox_GetChatTabContent(pbox, targetjid))
	{
		ChatBox_AddChat(pbox, targetjid, nick, isGroupChat);
		if (isGroupChat)
		{
			struct namedlist_s **chattab = NamedList_GetByName(&(data->chats), targetjid);

			if (chattab)
			{
				struct chattabcontentdata_s *tabdata = (*chattab)->data;

				Box_AddTimedFunc(tabdata->box, ChatBox_MucJoinTimeout, NULL, 15000);
			}
		}
	}
	if (selecttab || notabs || !Box_CheckLastActive())
	{
		TabCtrl_ActivateTabByName(data->tabctrl, targetjid);
	}
}

void ChatBox_ClearChatHistory(struct Box_s *pbox, char *targetjid)
{
	struct chatboxdata_s *data = pbox->boxdata;
	struct chattabcontentdata_s *tabdata = ChatBox_GetChatTabContent(pbox, targetjid);

	List_RemoveAllEntries(tabdata->box);
	List_ScrollToTop(tabdata->box);
	List_RedoEntries(tabdata->box);
	Box_Repaint(tabdata->box);
}

struct Box_s *ChatBox_AddText(struct Box_s *pbox, char *targetjid, char *name, char *text, char *timestamp, int notify)
{
	struct chatboxdata_s *data = pbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	struct namedlist_s *links = NULL;
	unsigned int currentsec;
	int skipmatchlinks = 0;
	struct Box_s *textbox = NULL;

	tabdata = ChatBox_GetChatTabContent(pbox, targetjid);

	if (!tabdata)
	{
		return NULL;
	}

	/* ignore messages from people on your ignore list */
	if (tabdata->isGroupChat && Model_IsJIDIgnored(name))
	{
		return NULL;
	}
	{
		struct Box_s *entrybox = List_GetEntryBoxAllGroups(tabdata->participantlist, name);

		if (entrybox)
		{
			char *realjid;

			realjid = ParticipantEntry_GetRealJID(entrybox);
			if (Model_IsJIDIgnored(realjid))
			{
				return NULL;
			}
		}
	}

	if (tabdata->isGroupChat && stricmp(tabdata->targetjid, "centralpark@chat.chesspark.com") == 0)
	{
		skipmatchlinks = 1;
	}

	if (!tabdata->logstarted)
	{
		FILE *fp = fopen(tabdata->logfile, "a");
		if (fp)
		{
			fprintf(fp, "-----Log file opened at %s, %s-----\n", Info_GetLocalDate(), Info_GetLocalTime());
			fclose(fp);
			tabdata->logstarted = 1;
		}
	}

	if (timestamp && tabdata->startedpast == 0)
	{
		List_RemoveAllEntries(tabdata->box);
		List_ScrollVisible(tabdata->box);
		tabdata->startedpast = 1;
		ChatBox_AddTimeStamp(pbox, targetjid, timestamp);
		List_RedoEntries(tabdata->box);
		Box_Repaint(tabdata->box);
	}

	if (!timestamp && tabdata->startedpast == 1)
	{
		tabdata->startedpast = 2;
	}

	if (timestamp)
	{	
		currentsec = (unsigned int)(Info_ConvertTimestampToTimeT(timestamp));
	}
	else
	{
		int sec, min, hour, day, mon, year;

		Info_GetTimeOfDay(&sec, &min, &hour, &day, &mon, &year);

		currentsec = (unsigned int)(Info_ConvertTimeOfDayToTimeT(sec, min, hour, day, mon, year));

	}

	/* Don't show history again */
	if (timestamp && tabdata->startedpast == 2 && currentsec <= tabdata->lastchattime)
	{
		return NULL;
	}

	if (currentsec >= tabdata->lastchattime + 5 * 60)
	{
		ChatBox_AddTimeStamp(pbox, targetjid, timestamp);
	}

	tabdata->lastchattime = currentsec;

	if (tabdata->isGroupChat)
	{
		struct Box_s *entrybox = List_GetEntryBoxAllGroups(tabdata->participantlist, name);
		char *showname;

		if (entrybox)
		{
			showname = ParticipantEntry_GetShowName(entrybox);
		}
		else if ((strstr(targetjid, "@chat.chesspark.com") || strstr(targetjid, "@games.chesspark.com")) && CHESSPARK_LOCALCHATSUSEJIDNICK)
		{
			showname = Model_GetFriendNick(name);
		}
		else
		{
			showname = name;
		}

		textbox = SubChat_AddText2(tabdata->box, tabdata->participantlist, name, showname, text, name && stricmp(name, tabdata->nick) == 0, data->edit, tabdata->logfile, timestamp, skipmatchlinks);
	}
	else if (name)
	{
		struct namedlist_s **ppchat = View_GetPtrToChat(name, 1);
		char *showname = NULL;
	
		if (ppchat && *ppchat)
		{
			showname = Jid_GetResource(name);
		}
		else
		{
			struct Box_s *entrybox = List_GetEntryBoxAllGroups(tabdata->participantlist, Jid_Strip(name));

			if (entrybox)
			{
				showname = ParticipantEntry_GetShowName(entrybox);
			}
			else
			{
				showname = Model_GetFriendNick(name);
			}

			if (!Model_GetOption(OPTION_DISABLEIMSOUNDS) && !Model_GetOption(OPTION_INITIALIMSOUNDONLY))
			{
				if (stricmp(Jid_Strip(name), tabdata->targetjid) == 0)
				{
					Audio_PlayWav("sounds/recvmsg.wav");
				}
				else
				{
					Audio_PlayWav("sounds/sendmsg.wav");
				}
			}
		}

		textbox = SubChat_AddText2(tabdata->box, tabdata->participantlist, Jid_Strip(name), showname, text, name && stricmp(Jid_Strip(name), Model_GetLoginJid()) == 0, data->edit, tabdata->logfile, timestamp, skipmatchlinks);
	}
	else
	{
		textbox = SubChat_AddText2(tabdata->box, tabdata->participantlist, NULL, NULL, text, 0, data->edit, tabdata->logfile, timestamp, skipmatchlinks);
	}

	if (notify && !(tabdata->isGroupChat && tabdata->nick && name && stricmp(tabdata->nick, name) == 0))
	{
		if (!pbox->active && !Model_GetOption(OPTION_NOCHATNOTIFY))
		{
			FlashWindow(pbox->hwnd, TRUE);
		}

		if (!(strcmp(tabdata->targetjid, TabCtrl_GetActiveTab(data->tabctrl)) == 0 && List_IsAtBottom(tabdata->box)))
		{
			TabCtrl_SetTabNotify(data->tabctrl, tabdata->targetjid);
		}
	}

	return textbox;
}

void ChatBox_InitialIMSound()
{
	if (!Model_GetOption(OPTION_DISABLEIMSOUNDS) && Model_GetOption(OPTION_INITIALIMSOUNDONLY))
	{
		Audio_PlayWav("sounds/recvmsg.wav");
	}
}

void ChatBox_AddCustom(struct Box_s *chatbox, char *targetjid, struct Box_s *custombox, int notify)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;

	tabdata = ChatBox_GetChatTabContent(chatbox, targetjid);

	if (!tabdata)
	{
		return;
	}

	SubChat_AddCustom(tabdata->box, custombox);

	if (notify && !chatbox->active && !Model_GetOption(OPTION_NOCHATNOTIFY))
	{
		FlashWindow(chatbox->hwnd, TRUE);
	}
}

void ChatBox_SetChatTopic(struct Box_s *pbox, char *targetjid, char *name, char *topic)
{
	struct chatboxdata_s *data = pbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	struct namedlist_s *links = NULL;

	tabdata = ChatBox_GetChatTabContent(pbox, targetjid);

	if (!tabdata)
	{
		return;
	}

	tabdata->topic = strdup(topic);

	if (name)
	{
		char txt[256];

		i18n_stringsub(txt, 256, _("%1 has set the topic to %2"), Model_GetFriendNick(name), topic);
		ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
	}

	/* refresh current tab */
	{
		char *name = TabCtrl_GetActiveTab(data->tabctrl);

		if (name)
		{
                        ChatTabCtrl_OnActivate(data->tabctrl, name);
		}
	}
}

void ChatBox_SetParticipantStatus(struct Box_s *pbox, char *targetjid,
  char *name, enum SStatus status, char *statusmsg, char *role,
  char *affiliation, char *realjid, char *nickchange, struct namedlist_s *roleslist,
  struct namedlist_s *titleslist, char *actorjid, char *reason, int notactivated,
  char *membertype)
{
	struct Box_s *entrybox;
	struct chatboxdata_s *data = pbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	char txt[256];
	char *groupname;
	char *showname;
	char *resource;

	tabdata = ChatBox_GetChatTabContent(pbox, targetjid);

	if (!tabdata)
	{
		return;
	}

	resource = Jid_GetResource(name);

	if (resource && strcmp(resource, tabdata->nick) == 0)
	{
		tabdata->isinroom = (status != SSTAT_OFFLINE);
		tabdata->localnotactivated = notactivated;
	}

	/* Everyone's a participant in local chats*/
	if (strstr(targetjid, "@chat.chesspark.com"))
	{
		groupname = _("Participants");
	}
	else if (role && stricmp(role, "participant") == 0)
	{
		groupname = _("Participants");
	}
	else if (role && stricmp(role, "moderator") == 0)
	{
		groupname = _("Moderators");
	}
	else if (role && stricmp(role, "player") == 0)
	{
		groupname = _("Players");
	}
	else if (role)
	{
		groupname = role;
	}
	else
	{
		groupname = _("Participants");
	}

	if (!resource)
	{
		resource = Jid_Strip(name);
	}

	if (tabdata->isGroupChat)
	{
		if (realjid)
		{
			showname = Model_GetFriendNick(realjid);
		}
		else
		{
			showname = Model_GetFriendNick(resource);
		}
	}
	else
	{
		showname = Model_GetFriendNick(name);
	}

	if (tabdata->isGroupChat)
	{
                entrybox = List_GetEntryBoxAllGroups(tabdata->participantlist, Jid_GetResource(name));
	}
	else
	{
		Log_Write(0, "nameget %s\n", Jid_Strip(name));
		entrybox = List_GetEntryBoxAllGroups(tabdata->participantlist, Jid_Strip(name));
	}


	/* If not a group chat, get the best status for the jid */
	if (!tabdata->isGroupChat)
	{
		status = Model_GetBestStatus(targetjid);
	}

	if (tabdata->isGroupChat)
	{
		if (status == SSTAT_OFFLINE)
		{
			if (nickchange)
			{
				struct Box_s *entrybox2;

				/* suppress nick change notifications in chesspark rooms, as you can't see nicknames anyway */
				if (strstr(tabdata->targetjid, "@chat.chesspark.com") == NULL)
				{
					i18n_stringsub(txt, 256, _("%1 is now known as %2"), resource, nickchange);
					ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
				}

				/* add a temp (fake) entry so we don't show the entering message again */
				entrybox2 = ParticipantEntry_Create(targetjid, nickchange, SSTAT_AVAILABLE, statusmsg, role, affiliation, realjid, tabdata->isGroupChat, NULL, NULL, notactivated, membertype);

				if (!List_GetGroupBox(tabdata->participantlist, groupname))
				{
					List_AddGroup(tabdata->participantlist, groupname);
				}

				List_AddEntry(tabdata->participantlist, nickchange, groupname, entrybox2);
			}
			else if (stricmp(resource, tabdata->nick) == 0)
			{
				struct Box_s *textbox = NULL;
				if (affiliation && stricmp(affiliation, "outcast") == 0)
				{

					if (actorjid && reason)
					{
						i18n_stringsub(txt, 256, _("You have been banned by ^l%1^l: %2."), Model_GetFriendNick(actorjid), reason);
					}
					else if (actorjid)
					{
						i18n_stringsub(txt, 256, _("You have been banned by ^l%1^l."), Model_GetFriendNick(actorjid));
					}
					else if (reason)
					{
						i18n_stringsub(txt, 256, _("You have been banned: %1"), reason);
					}
					else
					{
						i18n_stringsub(txt, 256, _("You have been banned."));
					}
#if 0
					if (actorjid)
					{
						i18n_stringsub(txt, 256, _("You have been banned by ^l%1^l."), Model_GetFriendNick(actorjid));
					}
					else
					{
						i18n_stringsub(txt, 256, _("You have been banned."));
					}
#endif
					textbox = ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
				}
				else if (entrybox)
				{
					if (actorjid && reason)
					{
						i18n_stringsub(txt, 256, _("You have been kicked by ^l%1^l: %2."), Model_GetFriendNick(actorjid), reason);
					}
					else if (actorjid)
					{
						i18n_stringsub(txt, 256, _("You have been kicked by ^l%1^l."), Model_GetFriendNick(actorjid));
					}
					else if (reason)
					{
						i18n_stringsub(txt, 256, _("You have been kicked: %1"), reason);
					}
					else
					{
						i18n_stringsub(txt, 256, _("You have been kicked."));
					}
					
					textbox = ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
				}

				if (textbox && actorjid)
				{
					Text_SetLinkCallback(textbox, 1, ViewLink_OpenChat, strdup(actorjid));
				}
			}
			else
			{
				if (affiliation && stricmp(affiliation, "outcast") == 0)
				{
					if (actorjid && reason)
					{
						i18n_stringsub(txt, 256, _("%1 has been banned by %2: %3."), showname, Model_GetFriendNick(actorjid), reason);
					}
					else if (actorjid)
					{
						i18n_stringsub(txt, 256, _("%1 has been banned by %2."), showname, Model_GetFriendNick(actorjid));
					}
					else if (reason)
					{
						i18n_stringsub(txt, 256, _("%1 has been banned: %2"), showname, reason);
					}
					else
					{
						i18n_stringsub(txt, 256, _("%1 has been banned."), showname);
					}
#if 0
					if (actorjid)
					{
						i18n_stringsub(txt, 256, _("%1 has been banned by %2."), showname, Model_GetFriendNick(actorjid));
					}
					else
					{
						i18n_stringsub(txt, 256, _("%1 has been banned."), showname);
					}
#endif
					ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
				}
				else if (entrybox)
				{
					if (actorjid && reason)
					{
						i18n_stringsub(txt, 256, _("%1 has been kicked by %2: %3."), showname, Model_GetFriendNick(actorjid), reason);
						ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
					}
					else if (reason)
					{
						i18n_stringsub(txt, 256, _("%1 has been kicked: %2."), showname, reason);
						ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
					}
					else if (actorjid)
					{
						i18n_stringsub(txt, 256, _("%1 has been kicked by %2."), showname, Model_GetFriendNick(actorjid));
						ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
					}
					else if (Model_GetOption(OPTION_SHOWMUCPRESENCEINCHAT) || (!data->drawervisible && Model_GetOption(OPTION_SHOWMUCPRESENCEINCHATWHENCLOSED)))
					{
						i18n_stringsub(txt, 256, _("%1 has left the room."), showname);
						ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
					}
				}
				else if (Model_GetOption(OPTION_SHOWMUCPRESENCEINCHAT) || (!data->drawervisible && Model_GetOption(OPTION_SHOWMUCPRESENCEINCHATWHENCLOSED)))
				{
					i18n_stringsub(txt, 256, _("%1 has left the room."), showname);
					ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
				}
			}
		}
		else if (!entrybox)
		{
			if (stricmp(resource, tabdata->nick) == 0)
			{
				if (tabdata->initialconnect)
				{
					unsigned int oldtime = tabdata->lastchattime;
					ChatBox_AddText(pbox, tabdata->targetjid, NULL, _("^bReconnected."), NULL, 0);
					tabdata->lastchattime = oldtime;
				}
				tabdata->initialconnect = 1;
				ChatBox_UpdateTitlebarText(pbox);
				Box_RemoveTimedFunc(tabdata->box, ChatBox_MucJoinTimeout, 15000);
			}
			else if (!entrybox && (Model_GetOption(OPTION_SHOWMUCPRESENCEINCHAT) || (!data->drawervisible && Model_GetOption(OPTION_SHOWMUCPRESENCEINCHATWHENCLOSED))))
			{
				i18n_stringsub(txt, 256, _("%1 has entered the room."), showname);
				ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
			}
		}
	}
	else
	{
		enum SStatus laststatus = SSTAT_OFFLINE;

		if (entrybox)
		{
			laststatus = ParticipantEntry_GetStatus(entrybox);
		}

		if (laststatus != status)
		{
			TitleBar_SetIcon(pbox->titlebar, GetIconForStatus(status));
			switch(status)
			{
				case SSTAT_AVAILABLE:
					i18n_stringsub(txt, 256, _("%1 is now available."), showname);
					ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
					break;
				case SSTAT_AWAY:
					i18n_stringsub(txt, 256, _("%1 is now away."), showname);
					ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
					break;
				case SSTAT_OFFLINE:
					i18n_stringsub(txt, 256, _("%1 is now offline."), showname);
					ChatBox_AddText(pbox, tabdata->targetjid, NULL, txt, NULL, 0);
					break;
				default:
					break;
			}
		}
	}

	if (tabdata->isGroupChat)
	{
		List_RemoveEntryByNameAllGroups(tabdata->participantlist, Jid_GetResource(name));
	}
	else
	{
		List_RemoveEntryByNameAllGroups(tabdata->participantlist, Jid_Strip(name));
	}

	if (status == SSTAT_OFFLINE)
	{
		List_RedoEntries(tabdata->participantlist);
		Box_Repaint(tabdata->participantlist);
		return;
	}

	if (tabdata->isGroupChat)
	{
                entrybox = ParticipantEntry_Create(targetjid, Jid_GetResource(name), status, statusmsg, role, affiliation, realjid, tabdata->isGroupChat, roleslist, titleslist, notactivated, membertype);
	}
	else
	{
                entrybox = ParticipantEntry_Create(targetjid, Model_GetFriendNick(name), status, statusmsg, role, affiliation, targetjid, tabdata->isGroupChat, roleslist, titleslist, notactivated, membertype);
	}

	if (!List_GetGroupBox(tabdata->participantlist, groupname))
	{
		List_AddGroup(tabdata->participantlist, groupname);
	}

	if (tabdata->isGroupChat)
	{
		List_AddEntry(tabdata->participantlist, Jid_GetResource(name), groupname, entrybox);
	}
	else
	{
		List_AddEntry(tabdata->participantlist, Jid_Strip(name), groupname, entrybox);
	}

	List_RedoEntries(tabdata->participantlist);
	Box_Repaint(tabdata->participantlist);
}

int ChatBox_LocalUserHasMucPower(struct Box_s *pbox, char *mucjid)
{
	struct Box_s *entrybox;
	struct chatboxdata_s *data = pbox->boxdata;
	struct chattabcontentdata_s *tabdata;

	tabdata = ChatBox_GetChatTabContent(pbox, mucjid);

	if (!tabdata)
	{
		return 0;
	}

	entrybox = List_GetEntryBoxAllGroups(tabdata->participantlist, tabdata->nick);

	if (!entrybox)
	{
		Log_Write(0, "Couldn't find entry in participants: %s", tabdata->nick);
		return 0;
	}

	return ParticipantEntry_UserHasMucPower(entrybox);
}

void ChatBox_AnimateDrawerOpen(struct Box_s *pbox, void *userdata)
{
	struct chatboxdata_s *data = pbox->boxdata;

	data->drawer->w += 10;

	if (data->drawer->w >= data->drawersavedw)
	{
		data->draweranimating = 0;
		data->drawervisible = 1;
		data->drawer->w = data->drawersavedw;
		data->drawer->minw = 150;
		Box_RemoveTimedFunc(pbox, ChatBox_AnimateDrawerOpen, 20);
	}

	Box_MoveWndCustom2(data->drawer, pbox->x - data->drawer->w, pbox->y + 20, data->drawer->w, pbox->h - 40);
}

void ChatBox_AnimateDrawerClose(struct Box_s *pbox, void *userdata)
{
	struct chatboxdata_s *data = pbox->boxdata;

	data->drawer->w -= 10;
	Box_MoveWndCustom2(data->drawer, pbox->x - data->drawer->w, pbox->y + 20, data->drawer->w, pbox->h - 40);

	if (data->drawer->w <= 0)
	{
		DestroyWindow(data->drawer->hwnd);
		data->draweranimating = 0;
		data->drawervisible = 0; 
		Box_RemoveTimedFunc(pbox, ChatBox_AnimateDrawerClose, 20);	

		data->grippy->flags |= BOX_VISIBLE;
		Box_Repaint(data->grippy);
	}
}

void ChatBox_QuickDrawerOpen(struct Box_s *pbox, int w)
{
	struct chatboxdata_s *data = pbox->boxdata;

	if (!data->drawervisible)
	{
		data->drawer->x = pbox->x - w;
		data->drawer->y = pbox->y + 20;
		data->drawer->minw = 150;
		data->drawersavedw = w;
		Box_OnSizeWidth_Stretch(data->drawer, w - data->drawer->w);
		Box_OnSizeHeight_Stretch(data->drawer, pbox->h - 40 - data->drawer->h);
		{
			char txt[512];
			GetWindowText(pbox->hwnd, &(txt[0]), 512);
			Box_CreateWndCustom2(data->drawer, txt, pbox);
		}
		data->draweranimating = 0;
		data->drawervisible = 1;
		Box_RemoveTimedFunc(pbox, ChatBox_AnimateDrawerOpen, 20);
		Box_RemoveTimedFunc(pbox, ChatBox_AnimateDrawerClose, 20);
	}
	else
	{
		data->drawer->minw = 150;
		data->drawersavedw = w;
		Box_MoveWndCustom2(data->drawer, pbox->x - w, pbox->y + 20, w, pbox->h - 40);

		Box_RemoveTimedFunc(pbox, ChatBox_AnimateDrawerOpen, 20);
		Box_RemoveTimedFunc(pbox, ChatBox_AnimateDrawerClose, 20);
	}

	data->draweranimating = 0;
	data->drawervisible = 1;

	data->grippy->flags &= ~BOX_VISIBLE;
	Box_Repaint(data->grippy);
}

void ChatBox_QuickDrawerClose(struct Box_s *pbox)
{
	struct chatboxdata_s *data = pbox->boxdata;

	if (data->drawervisible)
	{
		data->drawer->w = 0;
		data->drawer->minw = 0;
		Box_MoveWndCustom2(data->drawer, pbox->x - data->drawer->w, pbox->y + 20, data->drawer->w, pbox->h - 40);

		DestroyWindow(data->drawer->hwnd);
		data->draweranimating = 0;
		data->drawervisible = 0; 
		Box_RemoveTimedFunc(pbox, ChatBox_AnimateDrawerClose, 20);
	}
	data->grippy->flags |= BOX_VISIBLE;
	Box_Repaint(data->grippy);
}


void ChatBox_ToggleShowParticipants(struct Box_s *pbox)
{
	struct chatboxdata_s *data = pbox->boxdata;

	if (data->draweranimating)
	{
		return;
	}

	if (!data->drawervisible)
	{
		data->drawer->x = pbox->x;
		data->drawer->y = pbox->y - 20;
		data->drawer->w = 0;
		data->drawer->minw = 0;
		Box_OnSizeHeight_Stretch(data->drawer, pbox->h - 40 - data->drawer->h);
		/*data->drawer->h = pbox->h - 40;*/
		data->draweranimating = 1;
		{
			char txt[512];
			GetWindowText(pbox->hwnd, &(txt[0]), 512);
			Box_CreateWndCustom2(data->drawer, txt, pbox);
		}
		Box_AddTimedFunc(pbox, ChatBox_AnimateDrawerOpen, NULL, 20);
		Box_RemoveTimedFunc(pbox, ChatBox_AnimateDrawerClose, 20);

		data->grippy->flags &= ~BOX_VISIBLE;
		Box_Repaint(data->grippy);
	}
	else
	{
		data->drawersavedw = data->drawer->w;
		data->drawer->minw = 0;
		data->draweranimating = 1;
		Box_AddTimedFunc(pbox, ChatBox_AnimateDrawerClose, NULL, 20);
		Box_RemoveTimedFunc(pbox, ChatBox_AnimateDrawerOpen, 20);
	}
}

int ChatBox_HasChat(struct Box_s *pbox, char *jid, int isGroupChat)
{
	struct chatboxdata_s *data = pbox->boxdata;
	struct namedlist_s *chat;
	char *barejid;

	chat = data->chats;
	while (chat)
	{
		if (stricmp(jid, chat->name) == 0)
		{
			struct chattabcontentdata_s *tabdata = chat->data;

			if (tabdata->isGroupChat == isGroupChat)
			{
				return TRUE;
			}
		}
		chat = chat->next;
	}

	barejid = Jid_Strip(jid);
	chat = data->chats;
	while (chat)
	{
		if (stricmp(barejid, chat->name) == 0)
		{
			struct chattabcontentdata_s *tabdata = chat->data;

			if (tabdata->isGroupChat == isGroupChat)
			{
				free(barejid);
				return TRUE;
			}
		}
		chat = chat->next;
	}

	free(barejid);
	return FALSE;
}

int ChatBox_HasOnlyOneChat(struct Box_s *chatbox)
{
	struct chatboxdata_s *data = chatbox->boxdata;

	if (!data->chats)
	{
		return 0;
	}

	if (data->chats->next)
	{
		return 0;
	}

	return 1;
}

void ChatBox_MoveChat(struct Box_s *pboxsrc, struct Box_s *pboxdst, char *jid)
{
	struct chatboxdata_s *srcdata = pboxsrc->boxdata;
	struct chatboxdata_s *dstdata = pboxdst->boxdata;
	struct chattabcontentdata_s *tabdata;
	struct namedlist_s **chattab = ChatBox_GetChatTabEntry(pboxsrc, jid);
	struct Box_s *drawer;
	char *showname;
	int bottom;
	int dstpwidth;

	/*
	Log_Write(0, "ChatBox_MoveChat(%d, %d, %s)\n", pboxsrc, pboxdst, jid);
	*/

	if (!chattab)
	{
		return;
	}

	tabdata = (*chattab)->data;
	
	drawer = Box_GetRoot(tabdata->participantlist);

	NamedList_Unlink(chattab);
	
	NamedList_Add(&(dstdata->chats), jid, tabdata, ChatTabContent_OnDestroy);

	Box_Unlink(tabdata->box);
	Box_AddChild(pboxdst, tabdata->box);
	bottom = List_IsAtBottom(tabdata->box);
	
	tabdata->box->OnSizeWidth(tabdata->box, pboxdst->w - Margin * 2 - 10 - tabdata->box->w);
	if (dstdata->tabsvisible)
	{
		tabdata->box->y = dstdata->tabctrl->y + 24;
		tabdata->box->OnSizeHeight(tabdata->box, pboxdst->h - Margin - 12 - 20 - TitleBarHeight - 10 - 5 - 19 - tabdata->box->h);
	}
	else
	{
		tabdata->box->y = dstdata->tabctrl->y + 5;
		tabdata->box->OnSizeHeight(tabdata->box, pboxdst->h - Margin - 12 - 20 - TitleBarHeight - 10 - 5 - tabdata->box->h);
	}

	if (bottom)
	{
		List_ScrollToBottom(tabdata->box);
	}

	Box_Unlink(tabdata->participantlist);
	tabdata->participantlist->OnSizeHeight(tabdata->participantlist, pboxdst->h - 22 - 5 - 10 - 10 - 40 - tabdata->participantlist->h);

	if (dstdata->drawervisible)
	{
		dstpwidth = dstdata->drawer->w - 15;
	}
	else
	{
		dstpwidth = dstdata->drawersavedw - 15;
	}

	tabdata->participantlist->OnSizeWidth(tabdata->participantlist, dstpwidth - tabdata->participantlist->w);
	Box_AddChild(dstdata->drawer, tabdata->participantlist);

	if (tabdata->isGroupChat)
	{
		char unescaped[1024];

		if (strstr(jid, "@chat.chesspark.com"))
		{
			showname = strdup(UnescapeJID(Jid_GetBeforeAt(jid), unescaped, 1024));
		}
		else
		{
			showname = strdup(UnescapeJID(jid, unescaped, 1024));
		}
	}
	else
	{
		showname = Model_GetFriendNick(jid);
	}

	TabCtrl_RemoveTab(srcdata->tabctrl, jid);
	TabCtrl_AddTab2(dstdata->tabctrl, jid, showname, tabdata->box, 100, 
		tabdata->isGroupChat ? ImageMgr_GetImage("groupchaticon.png") : GetIconForStatus(Model_GetBestStatus(jid)), ChatTab_OnClose,
		FRIENDTABDROPDATA_ID, tabdata->targetjid, ChatTab_OnEmptyDrop);
	TabCtrl_ActivateTabByName(dstdata->tabctrl, jid);

	ChatBox_SetShowTabs(pboxdst, TabCtrl_GetNumTabs(dstdata->tabctrl) > 1);

	Box_Repaint(pboxdst);
	Box_Repaint(drawer);

	if (!srcdata->chats)
	{
		View_CloseChatDialog(pboxsrc);
	}
	else
	{
		ChatBox_SetShowTabs(pboxsrc, TabCtrl_GetNumTabs(srcdata->tabctrl) > 1);
		Box_Repaint(pboxsrc);
	}
}

int ChatBox_OnDragDrop(struct Box_s *chatdst, struct Box_s *chatsrc, int x, int y, int dragid, void *dragdata)
{
	struct chatboxdata_s *dstdata = chatdst->boxdata;

	/*
	Log_Write(0, "ChatBox_OnDragDrop(%d, %d, %d, %d, %d, %d)\n", chatdst, chatsrc, x, y, dragid, dragdata);
	*/

	if (dragid != FRIENDTABDROPDATA_ID)
	{
		return 0;
	}

	if (chatsrc != chatdst)
	{
		ChatBox_MoveChat(Box_GetRoot(chatsrc), chatdst, dragdata);
	}

	x -= (dstdata->tabctrl->x);
	y -= (dstdata->tabctrl->y);

	if (Box_CheckXYInBox(dstdata->tabctrl, x, y))
	{
		TabCtrl_HandleTabDrop(dstdata->tabctrl, dragdata, x, y);
	}

	return 1;
}


void ChatBox_RoomNickConflict(struct Box_s *pbox, char *roomjid, char *nick)
{
	struct Box_s *chatbox = Box_GetRoot(pbox);
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;

	tabdata = ChatBox_GetChatTabContent(chatbox, roomjid);

	if (!tabdata)
	{
		return;
	}

	/* auto nick change */
	{
		char newnick[512];
		char *p;
		int num = 0;

		strcpy(newnick, tabdata->nick);

		p = newnick + strlen(newnick);

		
		while (p != newnick && strchr("0123456789", *(p-1)))
		{
			p--;
		}

		while (*p == '0' && *(p+1) != '\0')
		{
			p++;
		}

		sscanf(p, "%d", &num);
		num++;
		sprintf(p, "%d", num);

		tabdata->nick = strdup(newnick);
		Ctrl_ChangeNick(tabdata->targetjid, newnick);
	}

	/*
	ChatBox_AddText(pbox, roomjid, NULL, _("Your nickname is in use in this room.  Please type in another and press enter."), NULL, 0);
	tabdata->state = 1;
	*/
}

void ChatBox_SetDisconnect(struct Box_s *chatbox)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	struct namedlist_s *chat = data->chats;

	while (chat)
	{
		tabdata = chat->data;

		tabdata->isinroom = 0;

		chat = chat->next;
	}
}

void ChatBox_ShowDisconnect(struct Box_s *chatbox)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	struct namedlist_s *chat = data->chats;

	while (chat)
	{
		unsigned int oldtime;
		tabdata = chat->data;

		oldtime = tabdata->lastchattime;
		ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^b^4Disconnected."), NULL, 0);
		tabdata->lastchattime = oldtime; /* preserve last chat time */
		if (tabdata->isGroupChat)
		{
			List_RemoveAllEntries(tabdata->participantlist);
		}
		else
		{
			List_RemoveEntryByNameAllGroups(tabdata->participantlist, Jid_Strip(tabdata->targetjid));
		}
		Box_Repaint(tabdata->participantlist);

		chat = chat->next;
	}
}

void ChatBox_ReconnectChats(struct Box_s *chatbox, int chessparkchats)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata; 
	struct namedlist_s *chat = data->chats;

	while (chat)
	{
		tabdata = chat->data;

		if (tabdata->isGroupChat && tabdata->initialconnect)
		{
			char chatname[512];
			int cpc = strstr(tabdata->targetjid, "@chat.chesspark.com") != NULL;
			
			if ((chessparkchats && cpc) || (!chessparkchats && !cpc))
			{
				strcpy(chatname, tabdata->targetjid);
				strcat(chatname, "/");
				strcat(chatname, tabdata->nick);

				List_RemoveAllEntries(tabdata->participantlist);
				/* simulate a hop, in case we're not fully disconnected */
				/*Ctrl_LeaveChat(chatname);*/
				Ctrl_ChangeNick(tabdata->targetjid, tabdata->nick);
				List_RedoEntries(tabdata->participantlist);
    				Box_Repaint(tabdata->participantlist);
			}
		}
		else if (!chessparkchats)
		{
			ChatBox_AddText(chatbox, tabdata->targetjid, NULL, _("^b^4Reconnected."), NULL, 0);
		}

		chat = chat->next;
	}
}

void ChatBox_TimedUnsetMyComposing(struct Box_s *chatbox, struct chattabcontentdata_s **userdata)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata = *userdata;
	char *name = TabCtrl_GetActiveTab(data->tabctrl);

	Box_RemoveTimedFunc(chatbox, ChatBox_TimedUnsetMyComposing, 60 * 1000 * 1);

	if (!name || stricmp(name, tabdata->targetjid) != 0)
	{
		return;
	}

	ChatBox_UnsetComposing(chatbox, tabdata->targetjid, NULL);
}

void ChatBox_SetComposing(struct Box_s *chatbox, char *jid, char *msgid)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	char txt[256];

	tabdata = ChatBox_GetChatTabContent(chatbox, jid);

	if (!tabdata)
	{
		return;
	}

	if (tabdata->isGroupChat)
	{
		return;
	}

	{
		char *resource = Jid_GetResource(tabdata->targetjid);
		char *beforeat = Jid_GetBeforeAt(tabdata->targetjid);
		char *barejid  = Jid_Strip(tabdata->targetjid);
		int islocal = strstr(tabdata->targetjid, "@chat.chesspark.com") != NULL;
		struct namedlist_s **ppchat = View_GetPtrToChat(tabdata->targetjid, 1);

		if (ppchat && *ppchat && resource)
		{
			if (islocal)
			{
				i18n_stringsub(txt, 256, _("Chatting with %1 from %2 - typing"), resource, beforeat);
			}
			else
			{
				i18n_stringsub(txt, 256, _("Chatting with %1 from %2 - typing"), resource, barejid);
			}
		}
		else
		{
			i18n_stringsub(txt, 256, _("Chatting with %1 - typing"), Model_GetFriendNick(tabdata->targetjid));
		}
	}

	TitleBar_SetText(chatbox->titlebar, txt);
	Box_Repaint(chatbox->titlebar);

	{
		struct chattabcontentdata_s **userdata = malloc(sizeof(*userdata));
		*userdata = tabdata;
		Box_AddTimedFunc(chatbox, ChatBox_TimedUnsetMyComposing, userdata, 60 * 1000 * 1);
	}
}

void ChatBox_UnsetComposing(struct Box_s *chatbox, char *jid, char *msgid)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	char txt[256];

	tabdata = ChatBox_GetChatTabContent(chatbox, jid);

	if (!tabdata)
	{
		return;
	}

	if (tabdata->isGroupChat)
	{
		return;
	}
	
	{
		char *resource = Jid_GetResource(tabdata->targetjid);
		char *beforeat = Jid_GetBeforeAt(tabdata->targetjid);
		char *barejid  = Jid_Strip(tabdata->targetjid);
		int islocal = strstr(tabdata->targetjid, "@chat.chesspark.com") != NULL;
		struct namedlist_s **ppchat = View_GetPtrToChat(tabdata->targetjid, 1);
	
		if (ppchat && *ppchat && resource)
		{
			if (islocal)
			{
				i18n_stringsub(txt, 256, _("Chatting with %1 from %2"), resource, beforeat);
			}
			else
			{
				i18n_stringsub(txt, 256, _("Chatting with %1 from %2"), resource, barejid);
			}
		}
		else
		{
			i18n_stringsub(txt, 256, _("Chatting with %1"), Model_GetFriendNick(tabdata->targetjid));
		}
	}

	TitleBar_SetText(chatbox->titlebar, txt);
	Box_Repaint(chatbox->titlebar);
}

void ChatBox_SetStatusOnAllChats(struct Box_s *chatbox, enum SStatus status,
	char *statusmsg)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;
	struct namedlist_s *chat = data->chats;

	while (chat)
	{
		tabdata = chat->data;

		if (tabdata->isGroupChat && tabdata->isinroom)
		{
			char chatname[512];
			strcpy(chatname, tabdata->targetjid);
			strcat(chatname, "/");
			strcat(chatname, tabdata->nick);

			Conn_SendPresence(status, statusmsg, chatname, 1, NULL, 0, 0);
		}
		else
		{
			struct Box_s *entrybox = List_GetEntryBoxAllGroups(tabdata->participantlist, Jid_Strip(Model_GetLoginJid()));

			if (entrybox)
			{
				ParticipantEntry_SetStatus(entrybox, status);
			}
		}

		chat = chat->next;
	}
}

int ChatBox_GetChatboxGroup(struct Box_s *chatbox)
{
	struct chatboxdata_s *data = chatbox->boxdata;

	return data->windowgroup;
}

void ChatBox_SetShowTabs(struct Box_s *chatbox, int show)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct namedlist_s *entry = data->chats;

	if (data->tabsvisible && !show)
	{
		data->tabcontentbg->OnSizeHeight(data->tabcontentbg, 20);
		data->tabcontentbg->y -= 19;
		while (entry)
		{
			struct chattabcontentdata_s *tabdata = entry->data;
			int bottom = List_IsAtBottom(tabdata->box);
			tabdata->box->OnSizeHeight(tabdata->box, 20);
			tabdata->box->y -= 19;
			if (bottom)
			{
				List_ScrollToBottom(tabdata->box);
			}
			entry = entry->next;
		}
		data->tabctrl->flags &= ~BOX_VISIBLE;
		data->tabsvisible = 0;
	}
	else if (!data->tabsvisible && show)
	{
		data->tabcontentbg->OnSizeHeight(data->tabcontentbg, -20);
		data->tabcontentbg->y += 19;
		while (entry)
		{
			struct chattabcontentdata_s *tabdata = entry->data;
			int bottom = List_IsAtBottom(tabdata->box);
			tabdata->box->OnSizeHeight(tabdata->box, -20);
			tabdata->box->y += 19;
			if (bottom)
			{
				List_ScrollToBottom(tabdata->box);
			}
			entry = entry->next;
		}
		data->tabctrl->flags |= BOX_VISIBLE;
		data->tabsvisible = 1;
	}
	Box_Repaint(chatbox);
}

void ChatBox_CloseChat(struct Box_s *chatbox, char *roomjid)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;

	tabdata = ChatBox_GetChatTabContent(chatbox, roomjid);

	if (!tabdata)
	{
		return;
	}

	ChatBox_RemoveChat(chatbox, roomjid);
	if (!TabCtrl_GetFirstTab(data->tabctrl))
	{
		View_CloseChatDialog(chatbox);
	}
}

void ChatBox_MucError(struct Box_s *chatbox, char *roomjid, int icode, char *error)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;

	tabdata = ChatBox_GetChatTabContent(chatbox, roomjid);

	if (!tabdata)
	{
		return;
	}

	if (tabdata->initialconnect)
	{
		if (icode == 405 && tabdata->localnotactivated)
		{
			struct Box_s *textbox;

			textbox = ChatBox_AddText(chatbox, roomjid, NULL, _("Message blocked.  To chat in public rooms, please confirm your email address.  For more information, click ^lhere^l"), NULL, 1);
			Text_SetLinkCallback(textbox, 1, Util_OpenURL2, "http://www.chesspark.com/help/howtoconfirm/");
		}
		else
		{
			ChatBox_AddText(chatbox, roomjid, NULL, error, NULL, 1);
		}
	}
	else
	{
		char titletext[512];
		char *chatname;

		if (strstr(roomjid, "@chat.chesspark.com"))
		{
			chatname = Jid_GetBeforeAt(roomjid);
		}
		else
		{
			chatname = roomjid;
		}

		ChatBox_RemoveChat(chatbox, roomjid);
		if (!TabCtrl_GetFirstTab(data->tabctrl))
		{
			View_CloseChatDialog(chatbox);
		}

		i18n_stringsub(titletext, 512, _("Error in %1 chatroom"), chatname);

		AutoDialog_Create(NULL, 300, titletext, error, NULL, NULL, NULL, NULL, NULL);
	}
}

int ChatBox_IsInRoom(struct Box_s *chatbox, char *roomjid)
{
	struct chatboxdata_s *data = chatbox->boxdata;
	struct chattabcontentdata_s *tabdata;

	tabdata = ChatBox_GetChatTabContent(chatbox, roomjid);

	if (!tabdata)
	{
		return 0;
	}

	return tabdata->isinroom;
}