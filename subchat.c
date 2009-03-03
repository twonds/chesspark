#include <stdio.h>
#include <stdlib.h>

#include "box.h"

#include "text.h"

#include "constants.h"
#include "i18n.h"
#include "info.h"
#include "list.h"
#include "log.h"
#include "menu.h"
#include "model.h"
#include "namedlist.h"
#include "options.h"
#include "participantentry.h"
#include "util.h"
#include "view.h"

extern HFONT tahoma10_f, tahoma10i_f;

int SubChat_CheckMatchLink(char *link, char *linkend)
{
	char *start, *end;
	char check[256];
	int i;

	end = strchr(link, ':');
	if (!end || end >= linkend)
	{
		return 0;
	}

	start = end + 1;
	end = strchr(start, ':');
	if (!end || end >= linkend)
	{
		return 1;
	}

	for (i = 0; i < 2; i++)
	{
		start = end + 1;

		if (start >= linkend)
		{
			return 1;
		}

		end = strchr(start, ':');
		if (!end || end >= linkend)
		{
			end = strchr(start, '|');
			if (end)
			{
				i = 2;
			}
		}

		if (!end || end >= linkend)
		{
			end = linkend;
		}

		if (end <= start)
		{
			return 1;
		}

		if (end - start > 255)
		{
			return 0;
		}

		strncpy(check, start, end - start);
		check[end - start] = '\0';
		Log_Write(0, "Check %s\n", check);

		if ((stricmp(check, "long") != 0) && (stricmp(check, "speed") != 0)
			&& (stricmp(check, "blitz") != 0) && (stricmp(check, "bullet") != 0)
			&& (stricmp(check, "atomic") != 0) && (stricmp(check, "chess960") != 0)
#ifdef CHESSPARK_CRAZYHOUSE
			&& (stricmp(check, "crazyhouse") != 0)
#endif
#ifdef CHESSPARK_LOSERS
			&& (stricmp(check, "losers") != 0)
#endif
#ifdef CHESSPARK_CHECKERS
			&& (stricmp(check, "checkers") != 0)
#endif
			)
		{
			return 0;
		}
	}

	return 1;
}


void SubChat_ParseTextForLinks(char *src, char *dst, struct namedlist_s **links, int skipmatchlinks)
{
	char *resources[9] = {"http://", "ftp://", "https://", "game:", "room:", "contact:", "help:", "profile:", "match:"};
	char *urlendchars = "\" '<>\n\t";
	char *urlendchars2 = "?!,";
	char *p;

	p = src;

	while(*p)
	{

		int i;
		char *urlstart = NULL, *urlend;
		char *url;

		for (i = 0; i < 9 - skipmatchlinks; i++)
		{
			char *newstart, *newend;
			int reject;
			newend = p;

			do
			{
				newstart = strstr(newend, resources[i]);
				newend = NULL;
				reject = 0;

				if (newstart)
				{
					newend = strpbrk(newstart + strlen(resources[i]), urlendchars);

					if (i > 2)
					{
						char *newend2 = strpbrk(newstart + strlen(resources[i]), urlendchars2);

						if (newend2 && (!newend || newend > newend2) && (*(newend2+1) == ' ' || *(newend2+1) == '\0'))
						{
							newend = newend2;
						}
					}

					if (!newend)
					{
						newend = newstart + strlen(newstart);
					}

					/*if (i > 2)*/
					{
						char *pipe = strchr(newstart, '|');

						if (pipe && pipe < newend && *(pipe + 1) == '"')
						{
							newend = strchr(pipe + 2, '"');

							if (newend)
							{
								if (newend == pipe + 2)
								{
									reject = 1;
								}
                                                                newend++;
							}
							else
							{
								newend = newstart + strlen(newstart);
								reject = 1;
							}
						}

						if (pipe && pipe + 1 == newend)
						{
							reject = 1;
						}
					}

					while (newend >= newstart + strlen(resources[i]) && *(newend-1) == '.')
					{
						newend--;
					}

					if (newend == newstart + strlen(resources[i]))
					{
						reject = 1;
					}

					if (i == 8 && !SubChat_CheckMatchLink(newstart, newend))
					{
						reject = 1;
					}
				}
			}
			while (reject && newend);

			if (newstart && (!urlstart || newstart < urlstart))
			{
				urlstart = newstart;
				urlend = newend;
			}
		}

		if (urlstart)
		{
			if (urlstart > p)
			{
				strncat(dst, p, urlstart - p);
			}

			url = malloc(urlend - urlstart + 1);
			strncpy(url, urlstart, urlend - urlstart);
			url[urlend - urlstart] = '\0';

			NamedList_Add(links, "dummy", url, NULL);

			strcat(dst, "^l");
			strcat(dst, url);
			strcat(dst, "^l");

			p = urlend;
		}
		else
		{
			strcat(dst, p);
			p += strlen(p);
		}
	}
}

struct Box_s *SubChat_AddSmallText2(struct Box_s *subchat, char *txt, int emote, struct Box_s *redirectfocus, char *logfile)
{
	struct Box_s *entrybox, *subbox;
	char *finaltxt, *p;
	struct namedlist_s *links = NULL;
	int numlinks;
	int isatbottom = List_IsAtBottom(subchat);
	struct Box_s *textbox = NULL;

	/* quick link count, count colons */
	numlinks = 0;
	p = strchr(txt, ':');
	while (p)
	{
		numlinks++;
		p = strchr(p + 1, ':');
	}

	finaltxt = malloc(2 + strlen(txt) + 1 + numlinks * 4);
	finaltxt[0] = '\0';

	SubChat_ParseTextForLinks(txt, finaltxt, &links, 0);

	entrybox = Box_Create(0, 0, subchat->w, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	entrybox->cursorimg = LoadCursor(NULL, IDC_IBEAM);

	subbox = Text_Create(0, 3, subchat->w, 14, BOX_VISIBLE | BOX_TRANSPARENT, TX_CENTERED | TX_WRAP | TX_STRETCHPARENT | TX_SELECTABLE | TX_FOCUS);
	textbox = subbox;
	if (emote)
	{
		subbox->font = tahoma10i_f;
	}
	else
	{
		subbox->font = tahoma10_f;
	}
	subbox->bgcol = TabBG1;
	if (emote)
	{
		subbox->fgcol = RGB(96, 96, 96);
	}
	else
	{
		subbox->fgcol = RGB(128, 128, 128);
	}
	subbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	Box_AddChild(entrybox, subbox);
	Text_SetText(subbox, finaltxt);
	Text_SetRedirectFocusOnKey(subbox, redirectfocus);

	numlinks = 1;

	while(links)
	{
		if (strlen(links->data) > 5 && strnicmp(links->data, "game:", 5) == 0)
		{
			char *gameid = links->data;
			gameid += 5;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_WatchGame_OnClick, strdup(gameid));
		}
		else if (strlen(links->data) > 5 && strnicmp(links->data, "room:", 5) == 0)
		{
			char *jid = links->data;
			jid += 5;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_JoinChat_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 8 && strnicmp(links->data, "contact:", 8) == 0)
		{
			char *jid = links->data;
			jid += 8;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_AddContact_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 6 && strnicmp(links->data, "match:", 6) == 0)
		{
			char *jid = links->data;
			jid += 6;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_MatchLink_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 6 && strnicmp(links->data, "help:", 5) == 0)
		{
			char *jid = links->data;
			jid += 5;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_HelpLink_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 6 && strnicmp(links->data, "profile:", 8) == 0)
		{
			char *jid = links->data;
			jid += 8;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_ProfileLink_OnClick, strdup(jid));
		}
		else
		{
			Text_SetLinkCallback(textbox, numlinks, Util_OpenURL3, strdup(links->data));
		}
		Text_SetRLinkCallback(textbox, numlinks, Menu_PopupURLMenu, strdup(links->data));
		numlinks++;
		NamedList_Remove(&links);
	}

	List_AddEntry(subchat, "dummy", NULL, entrybox);

	List_RedoEntries(subchat);

	if (isatbottom /*&& !((GetKeyState(VK_LBUTTON) & 0x80) && Box_GetRoot(subchat)->active)*/)
	{
		List_ScrollToBottom(subchat);
	}
	/*
	else if (subchat->hwnd)
	{
		FlashWindow(subchat->hwnd, 1);
	}
	*/

	Box_Repaint(subchat);

	if (logfile)
	{
		/* strip out all control codes for the log */
		char *logtext = malloc(strlen(finaltxt)+1);
		char *in = finaltxt;
		char *out = logtext;
		FILE *fp;

		while (*in)
		{
			if (*in == '^')
			{
				in+=2;
			}
			else
			{
				*out++ = *in++;
			}
		}
		*out = '\0';

		fp = fopen(logfile, "a");
		if (fp)
		{
			if (emote)
			{
				fprintf(fp, "* %s\n", logtext);
			}
			else
			{
				fprintf(fp, "** %s\n", logtext);
			}

			fclose(fp);
		}
	}

	return textbox;
}


struct Box_s *SubChat_AddSmallText(struct Box_s *subchat, char *txt, int emote, struct Box_s *redirectfocus)
{
	return SubChat_AddSmallText2(subchat, txt, emote, redirectfocus, NULL);
}

int SubChat_AddTimeStamp2(struct Box_s *subchat, char *nick, char *timestamp, struct Box_s *redirectfocus, char *logfile)
{
	char finaltxt[256];
	unsigned int hour, min, currentsec;
	char ampm[3];
	
	if (timestamp && strlen(timestamp) == 17)
	{
		Info_ConvertTimestampToTimeOfDay(timestamp, NULL, &min, &hour, NULL, NULL, NULL);

		currentsec = (unsigned int)(Info_ConvertTimestampToTimeT(timestamp));
	}
	else
	{
		int sec, day, mon, year;
		Info_GetTimeOfDay(&sec, &min, &hour, &day, &mon, &year);

		currentsec =  (unsigned int)(Info_ConvertTimeOfDayToTimeT(sec, min, hour, day, mon, year));
	}

	Info_24HourTo12Hour(&hour, &(ampm[0]));

	{
		char timetxt[80];
		sprintf (timetxt, "%d:%02d %s", hour, min, ampm);

                if (nick)
		{
			i18n_stringsub(finaltxt, 256, "Chatting with %1 %2", nick, timetxt);
		}
		else
		{
			sprintf(finaltxt, timetxt);
		}
	}

	if (!Model_GetOption(OPTION_PERLINECHATTIMESTAMPS))
	{
                SubChat_AddSmallText2(subchat, finaltxt, 0, redirectfocus, logfile);
	}

	return currentsec; /*min + 60 * (hour - (hour == 12 ? 12 : 0) + (ampm[0] == 'a' ? 0 : 12));*/
}

int SubChat_AddTimeStamp(struct Box_s *subchat, char *nick, char *timestamp, struct Box_s *redirectfocus)
{
	return SubChat_AddTimeStamp2(subchat, nick, timestamp, redirectfocus, NULL);
}

struct participantmenuinfo_s
{
	char *name;
	struct Box_s *participantlist;
};


void SubChatNameLink_OnLClick(struct Box_s *pbox, struct participantmenuinfo_s *pmi)
{
	struct Box_s *entrybox = List_GetEntryBoxAllGroups(pmi->participantlist, pmi->name);
	char *realjid = NULL;

	if (entrybox)
	{
		realjid = ParticipantEntry_GetRealJID(entrybox);
	}

	if (!realjid)
	{
		realjid = pmi->name;

		if (strchr(realjid, '@') == NULL)
		{
			return;
		}
	}

	Model_ShowProfile(realjid);
}

void SubChatNameLink_OnRClick(struct Box_s *pbox, struct participantmenuinfo_s *pmi, int x, int y)
{
	struct Box_s *entrybox = List_GetEntryBoxAllGroups(pmi->participantlist, pmi->name);

	if (!entrybox)
	{
		if (strchr(pmi->name, '@') == NULL)
		{
			return;
		}

		/* pretend their nickname is their jid, as 9 times out of 10 it will be */
		Menu_PopupChatParticipantMenu(pbox, pmi->name, pmi->name, pmi->name, NULL, NULL, 0, 1, x, y);
		return;
	}

	ParticipantEntry_PopupMenu(entrybox, pbox, x, y);
}


char *SubChatText_ProfanityFilter(char *text)
{
	char *newtext = strdup(text);
	char *badwords = _("fag* *fuck* *shit* cock ass dick cunt whore asshole* kike* nigger* wank* clit* *testicle* twat* *nigga* lesb* *nazi* *wetback* *jizz* *jism* *dike* *bitch*");
	char *currentword;
	char *punctuation = "~!@#$%^&*()_+`-=[]\\{}|;':\",./<>? ";

	currentword = badwords;
	while (currentword)
	{
		char *p = newtext;

		while (*p)
		{
			char *letter = currentword;
			char *found = NULL;
			char *oldp = p;

			if (*letter == '*')
			{
				letter++;
			}

			while (*p)
			{
				if (*p == *letter)
				{
					found = p;
					break;
				}
				p++;
			}

			if (found && currentword[0] != '*' && found > newtext)
			{
				char *prev = found - 1;
				if (!strchr(punctuation, *prev))
				{
					found = NULL;
				}
			}

			while (found && *p && (*p == *letter || (*p >= 'A' && *p <= 'Z' && *p + 'a' - 'A' == *letter)) && *letter != '*' && *letter != ' ')
			{
				p++;
				letter++;
				if (*letter != '*' && *letter != ' ')
				{
					while (*p && strchr(punctuation, *p))
					{
						p++;
					}
				}
			}

			if (found && *letter != '*' && *letter != ' ')
			{
				found = NULL;
			}

			if (found && *letter == ' ' && !strchr(punctuation, *p))
			{
				found = NULL;
			}

			if (found)
			{
				while(*found && found < p)
				{
					*found++ = '#';
				}
			}

			if (oldp == p)
			{
				p++;
			}
		}

		currentword = strchr(currentword, ' ');
		if (currentword)
		{
			currentword++;
		}
	}

	return newtext;
}

void TextEntryBox_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *child = pbox->child;

	while(child)
	{
		if (child->OnLButtonDown && (child->flags & BOX_VISIBLE))
		{
			int x = xmouse - child->x + pbox->xoffset;
			int y = ymouse - child->y + pbox->yoffset;
			child->OnLButtonDown(child, x, y);
		}
		child = child->sibling;
	}
}

struct Box_s *SubChat_AddHistory(struct Box_s *subchat, char *text)
{
	struct Box_s *entrybox;
	struct Box_s *textbox = NULL, *prevtext;
	char *finaltxt, *p;
	struct namedlist_s *links = NULL;
	int numlinks;
	char *urlendchars = "\" '<>()\n\t)";
	int isatbottom = List_IsAtBottom(subchat);

	/* Profanity filter */
	if (!Model_GetOption(OPTION_DISABLEPROFANITYFILTER))
	{
		text = SubChatText_ProfanityFilter(text);
	}

	/* double up carats in text, so users can't use ^ commands */
	text = Util_DoubleCarats(text);

	/* quick link count, count colons */
	numlinks = 0;
	p = strchr(text, ':');
	while (p)
	{
		numlinks++;
		p = strchr(p + 1, ':');
	}

	{
		finaltxt = malloc(strlen(text) + numlinks * 4 + 4 + 15);
	}

	finaltxt[0] = '\0';

	SubChat_ParseTextForLinks(text, finaltxt, &links, 0);

	free(text);

	entrybox = Box_Create(0, 0, subchat->w, 48, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->OnLButtonDown = TextEntryBox_OnLButtonDown;
	entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	entrybox->cursorimg = LoadCursor(NULL, IDC_IBEAM);

	/*textbox = Text_Create(32, 8, subchat->w - 48, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT | TX_SELECTABLE | TX_COPYMENU | TX_FOCUS);*/
	textbox = Text_Create(8, 8, subchat->w - 16, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT | TX_SELECTABLE | TX_COPYMENU | TX_FOCUS | TX_PARENTHOVER);
	textbox->bgcol = TabBG1;
	textbox->fgcol = RGB(128, 128, 128);
	textbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	Box_AddChild(entrybox, textbox);
	Text_SetText(textbox, finaltxt);

	numlinks = 1;

	while(links)
	{
		if (strlen(links->data) > 5 && strnicmp(links->data, "game:", 5) == 0)
		{
			char *gameid = links->data;
			gameid += 5;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_WatchGame_OnClick, strdup(gameid));
		}
		else if (strlen(links->data) > 5 && strnicmp(links->data, "room:", 5) == 0)
		{
			char *jid = links->data;
			jid += 5;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_JoinChat_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 8 && strnicmp(links->data, "contact:", 8) == 0)
		{
			char *jid = links->data;
			jid += 8;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_AddContact_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 6 && strnicmp(links->data, "match:", 6) == 0)
		{
			char *jid = links->data;
			jid += 6;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_MatchLink_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 6 && strnicmp(links->data, "help:", 5) == 0)
		{
			char *jid = links->data;
			jid += 5;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_HelpLink_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 6 && strnicmp(links->data, "profile:", 8) == 0)
		{
			char *jid = links->data;
			jid += 8;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_ProfileLink_OnClick, strdup(jid));
		}
		else
		{
			Text_SetLinkCallback(textbox, numlinks, Util_OpenURL3, strdup(links->data));
		}
		Text_SetRLinkCallback(textbox, numlinks, Menu_PopupURLMenu, strdup(links->data));
		numlinks++;
		NamedList_Remove(&links);
	}

	free(finaltxt);

	prevtext = List_GetLastText(subchat);
	if (prevtext)
	{
		Text_SetLinkedSelectNext(prevtext, textbox);
		Text_SetLinkedSelectPrev(textbox, prevtext);
	}

	List_SetLastText(subchat, textbox);

	List_AddEntry(subchat, "dummy", NULL, entrybox);

	List_RedoEntries(subchat);

	if (isatbottom /*&& !((GetKeyState(VK_LBUTTON) & 0x80) && Box_GetRoot(subchat)->active)*/)
	{
		List_ScrollToBottom(subchat);
	}

	Box_Repaint(subchat);

	return textbox;
}

struct Box_s *SubChat_AddText2(struct Box_s *subchat, struct Box_s *participantlist,
	char *targetjid, char *showname, char *text, int local,
	struct Box_s *redirectfocus, char *logfile, char *timestamp, int skipmatchlinks)
{
	struct Box_s *entrybox;
	struct Box_s *textbox = NULL, *prevtext;
	char *finaltxt, *p;
	struct namedlist_s *links = NULL;
	int numlinks;
	char *urlendchars = "\" '<>()\n\t)";
	int isatbottom = List_IsAtBottom(subchat);

	/* Profanity filter */
	if (!Model_GetOption(OPTION_DISABLEPROFANITYFILTER))
	{
		text = SubChatText_ProfanityFilter(text);
	}

	if (!showname)
	{
		return SubChat_AddSmallText(subchat, text, 0, redirectfocus);
	}

	/*
	if (name)
	{
		if ((strstr(targetjid, "@chat.chesspark.com") || strstr(targetjid, "@games.chesspark.com")) && CHESSPARK_LOCALCHATSUSEJIDNICK)
		{
			struct Box_s *entrybox = List_GetEntryBoxAllGroups(participantlist, name);

			if (entrybox)
			{
				showname = ParticipantEntry_GetShowName(entrybox);
			}
			else
			{
				showname = Model_GetFriendNick(name);
			}
		}
		else
		{
			showname = name;
		}
	}
	*/

	/* double up carats in text, so users can't use ^ commands */
	text = Util_DoubleCarats(text);

	Log_Write(0, "Doubled carats, text is %s\n", text);

	/* quick link count, count colons */
	numlinks = 0;
	p = strchr(text, ':');
	while (p)
	{
		numlinks++;
		p = strchr(p + 1, ':');
	}

	
	if (showname)
	{
		finaltxt = malloc(4 + strlen(showname) + 6 + strlen(text) + numlinks * 4 + 1 + 15);
	}
	else
	{
		finaltxt = malloc(strlen(text) + numlinks * 4 + 4 + 15);
	}

	finaltxt[0] = '\0';

	if (strncmp(text, "/me ", 4) == 0)
	{
		strcat(finaltxt, showname);
		strcat(finaltxt, text + 3);
		textbox = SubChat_AddSmallText2(subchat, finaltxt, 1, redirectfocus, logfile);
		free(finaltxt);
		free(text);
		return textbox;
	}

	if (Model_GetOption(OPTION_PERLINECHATTIMESTAMPS))
	{
		strcat(finaltxt, "[");
		if (timestamp)
		{
			strcat(finaltxt, Info_TimestampToLocalTime2(timestamp));
		}
		else
		{
			strcat(finaltxt, Info_GetLocalTime2());
		}
		strcat(finaltxt, "] ");
	}

	if (showname)
	{
		if (local)
		{
			strcat(finaltxt, "^L^2");
		}
		else
		{
			strcat(finaltxt, "^L^1");
		}

		strcat(finaltxt, showname);
		strcat(finaltxt, "^L: ^n");

	}
	SubChat_ParseTextForLinks(text, finaltxt, &links, skipmatchlinks);

	free(text);

	entrybox = Box_Create(0, 0, subchat->w, 48, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->OnLButtonDown = TextEntryBox_OnLButtonDown;
	entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	entrybox->cursorimg = LoadCursor(NULL, IDC_IBEAM);

	/*textbox = Text_Create(32, 8, subchat->w - 48, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT | TX_SELECTABLE | TX_COPYMENU | TX_FOCUS);*/
	textbox = Text_Create(8, 8, subchat->w - 16, 32, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHPARENT | TX_SELECTABLE | TX_COPYMENU | TX_FOCUS | TX_PARENTHOVER);
	textbox->bgcol = TabBG1;
	textbox->fgcol = RGB(0, 0, 0);
	textbox->OnSizeWidth = Text_OnSizeWidth_Stretch;
	Box_AddChild(entrybox, textbox);
	Text_SetText(textbox, finaltxt);
	Text_SetRedirectFocusOnKey(textbox, redirectfocus);

	numlinks = 2;
	{
		struct participantmenuinfo_s *pmi = malloc(sizeof(*pmi));

		pmi->name = strdup(targetjid);
		pmi->participantlist = participantlist;

		Log_Write(0, "pmi->name is %s\n", pmi->name);

                Text_SetLinkCallback(textbox, 1, SubChatNameLink_OnLClick, pmi);
                Text_SetRLinkCallback(textbox, 1, SubChatNameLink_OnRClick, pmi);
	}

	while(links)
	{
		if (strlen(links->data) > 5 && strnicmp(links->data, "game:", 5) == 0)
		{
			char *gameid = links->data;
			gameid += 5;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_WatchGame_OnClick, strdup(gameid));
		}
		else if (strlen(links->data) > 5 && strnicmp(links->data, "room:", 5) == 0)
		{
			char *jid = links->data;
			jid += 5;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_JoinChat_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 8 && strnicmp(links->data, "contact:", 8) == 0)
		{
			char *jid = links->data;
			jid += 8;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_AddContact_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 6 && strnicmp(links->data, "match:", 6) == 0)
		{
			char *jid = links->data;
			jid += 6;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_MatchLink_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 6 && strnicmp(links->data, "help:", 5) == 0)
		{
			char *jid = links->data;
			jid += 5;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_HelpLink_OnClick, strdup(jid));
		}
		else if (strlen(links->data) > 6 && strnicmp(links->data, "profile:", 8) == 0)
		{
			char *jid = links->data;
			jid += 8;

			Text_SetLinkCallback(textbox, numlinks, ViewLink_ProfileLink_OnClick, strdup(jid));
		}
		else
		{
			Text_SetLinkCallback(textbox, numlinks, Util_OpenURL3, strdup(links->data));
		}
		Text_SetRLinkCallback(textbox, numlinks, Menu_PopupURLMenu, strdup(links->data));
		numlinks++;
		NamedList_Remove(&links);
	}

	if (logfile)
	{
		/* strip out all control codes for the log */
		char *logtext = malloc(strlen(finaltxt)+1);
		char *in = finaltxt;
		char *out = logtext;
		FILE *fp;

		while (*in)
		{
			if (*in == '^')
			{
				in+=2;
			}
			else
			{
				*out++ = *in++;
			}
		}
		*out = '\0';

		fp = fopen(logfile, "a");
		if (fp)
		{
			fprintf(fp, "%s\n", logtext);
			fclose(fp);
		}
	}

	free(finaltxt);

	prevtext = List_GetLastText(subchat);
	if (prevtext)
	{
		Text_SetLinkedSelectNext(prevtext, textbox);
		Text_SetLinkedSelectPrev(textbox, prevtext);
	}

	List_SetLastText(subchat, textbox);

	List_AddEntry(subchat, "dummy", NULL, entrybox);

	List_RedoEntries(subchat);

	if (isatbottom /*&& !((GetKeyState(VK_LBUTTON) & 0x80) && Box_GetRoot(subchat)->active)*/)
	{
		List_ScrollToBottom(subchat);
	}

	Box_Repaint(subchat);

	return textbox;
}

struct Box_s *SubChat_AddText(struct Box_s *subchat, struct Box_s *participantlist,
	char *targetjid, char *showname, char *text, int local,
	struct Box_s *redirectfocus)
{
	return SubChat_AddText2(subchat, participantlist, targetjid, showname, text, local, redirectfocus, NULL, NULL, 0);
}


void SubChat_AddCustom(struct Box_s *subchat, struct Box_s *custombox)
{
	struct Box_s *entrybox;
	int isatbottom = List_IsAtBottom(subchat);

	entrybox = Box_Create(0, 0, subchat->w, 48, BOX_VISIBLE | BOX_TRANSPARENT);
	entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	entrybox->cursorimg = LoadCursor(NULL, IDC_IBEAM);

	Box_AddChild(entrybox, custombox);

	List_AddEntry(subchat, "dummy", NULL, entrybox);

	List_RedoEntries(subchat);

	if (isatbottom /*&& !((GetKeyState(VK_LBUTTON) & 0x80) && Box_GetRoot(subchat)->active)*/)
	{
		List_ScrollToBottom(subchat);
	}

	Box_Repaint(subchat);
}
