#include <stdio.h>
#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "checkbox.h"
#include "combobox.h"
#include "imagemgr.h"
#include "list.h"
#include "titledrag.h"
#include "text.h"

#include "constants.h"

#include "audio.h"
#include "autosize.h"
#include "boxtypes.h"
#include "button2.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "info.h"
#include "log.h"
#include "model.h"
#include "namedlist.h"
#include "profile.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

#include "options.h"


struct optionsdata_s
{
	struct Box_s *sizeablecontent;
	struct Box_s *optionpage;
	struct Box_s *ignorelistbox;
	struct namedlist_s *ignorelist;
	struct namedlist_s *localoptions;
	int autoawaytime;
	int notificationtime;
};

void Options_OnSave(struct Box_s *pbox)
{
	struct optionsdata_s *data = Box_GetRoot(pbox)->boxdata;

	Model_SetIgnoreList(data->ignorelist);
	{
		int volume = Audio_GetVolume();
		NamedList_RemoveByName(&(data->localoptions), OPTION_VOLUME);

		if (volume != 50)
		{
			char txt[256];
			sprintf(txt, "%d", volume);
			NamedList_AddString(&(data->localoptions), OPTION_VOLUME, txt);
		}

		NamedList_RemoveByName(&(data->localoptions), OPTION_NOTIFICATIONTIME);

		if (data->notificationtime != 5)
		{
			char txt[256];
			sprintf(txt, "%d", data->notificationtime);
			NamedList_AddString(&(data->localoptions), OPTION_NOTIFICATIONTIME, txt);
		}
	}
	Model_SetAllOptions(data->localoptions);

	View_CloseOptionsDialog();
}


void Options_OnCancel(struct Box_s *pbox)
{
	struct optionsdata_s *data = Box_GetRoot(pbox)->boxdata;

	View_CloseOptionsDialog();
}

void Options_OnDestroy(struct Box_s *dialog)
{
	struct optionsdata_s *data = dialog->boxdata;

	NamedList_Destroy(&(data->ignorelist));
}

void OptionsIgnoreListEntry_Remove(struct Box_s *pbox)
{
	struct Box_s *entrybox = pbox->parent;
	struct privacylistentry_s *entrydata = entrybox->boxdata;
	struct Box_s *dialog = Box_GetRoot(entrybox);
	struct optionsdata_s *data = dialog->boxdata;

	NamedList_RemoveByName(&(data->ignorelist), entrydata->value);
	List_RemoveEntryByName(data->ignorelistbox, entrydata->value, NULL);

	List_RedoEntries(data->ignorelistbox);
	Box_Repaint(data->ignorelistbox);
}

void OptionsIgnoreList_AddEntry(struct Box_s *list, struct privacylistentry_s *srcentry)
{
	struct Box_s *entry, *subbox;
	struct privacylistentry_s *entrydata = Info_DupePrivacyListEntry(srcentry);

	entry = Box_Create(0, 0, list->w - 2 * 8, 3 * 8, BOX_VISIBLE | BOX_TRANSPARENT);
	entry->OnSizeWidth = Box_OnSizeWidth_Stretch;

	subbox = Box_Create(3 * 8 + 16, (entry->h - 16) / 2, entry->w - 3 * 8 - 16 - 13 - 16 - 5, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->fgcol = TabFG1;
	Box_SetText(subbox, srcentry->value);
	Box_AddChild(entry, subbox);

	subbox = Button_Create((entry->w) - 13 - 16, (entry->h - 13) / 2, 13, 13, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetImage("deleteIcon.png");
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	Button_SetOnButtonHit(subbox, OptionsIgnoreListEntry_Remove);
	Button_SetTooltipText(subbox, _("Delete"));
	Box_AddChild(entry, subbox);

	entry->boxdata = entrydata;

	List_AddEntry(list, entrydata->value, NULL, entry);

	List_RedoEntries(list);
	Box_Repaint(list);
}

void OptionsCheck_OnHit(struct Box_s *checkbox, int checked, char *option)
{
	struct Box_s *dialog = Box_GetRoot(checkbox);
	struct optionsdata_s *data = dialog->boxdata;
	struct namedlist_s **entry = NamedList_GetByName((&data->localoptions), option);
	char *optiondata = NULL;

	if (entry)
	{
		optiondata = strdup((*entry)->data);
	}

	NamedList_Remove(entry);

	if (checked)
	{
		/* exceptions for options with extra data */
		if (stricmp(option, OPTION_AUTOAWAY) == 0)
		{
			char txt2[256];
			sprintf(txt2, "%d", data->autoawaytime);
			free(optiondata);
			optiondata = strdup(txt2);
		}
		NamedList_AddString(&(data->localoptions), option, optiondata);
	}

	free(optiondata);
}

void OptionsCheck_OnHitReverse(struct Box_s *checkbox, int checked, char *option)
{
	OptionsCheck_OnHit(checkbox, !checked, option);
}

void OptionsAwayTimeEdit_OnKey(struct Box_s *editbox, char *text)
{
	struct Box_s *dialog = Box_GetRoot(editbox);
	struct optionsdata_s *data = dialog->boxdata;

	if (text)
	{
		struct namedlist_s **entry = NamedList_GetByName((&data->localoptions), OPTION_AUTOAWAY);
		sscanf(text, "%d", &(data->autoawaytime));
		if (entry)
		{
			char txt2[256];
			sprintf(txt2, "%d", data->autoawaytime);
			NamedList_Remove(entry);
			NamedList_AddString(&(data->localoptions), OPTION_AUTOAWAY, txt2);
		}
	}
}

void OptionsNotificationTimeEdit_OnKey(struct Box_s *editbox, char *text)
{
	struct Box_s *dialog = Box_GetRoot(editbox);
	struct optionsdata_s *data = dialog->boxdata;

	if (text)
	{
		struct namedlist_s **entry = NamedList_GetByName((&data->localoptions), OPTION_NOTIFICATIONTIME);
		sscanf(text, "%d", &(data->notificationtime));
		if (entry)
		{
			char txt2[256];
			sprintf(txt2, "%d", data->notificationtime);
			NamedList_Remove(entry);
			NamedList_AddString(&(data->localoptions), OPTION_NOTIFICATIONTIME, txt2);
		}
	}
}

void OptionsNotificationLocationCombo_OnSelection(struct Box_s *combobox, char *selection)
{
	struct Box_s *dialog = Box_GetRoot(combobox);
	struct optionsdata_s *data = dialog->boxdata;

	if (selection)
	{
		int iselection;

		sscanf(selection, "%d", &iselection);

		NamedList_RemoveByName(&(data->localoptions), OPTION_NOTIFICATIONLOCATION);

		if (iselection == 1)
		{
			NamedList_AddString(&(data->localoptions), OPTION_NOTIFICATIONLOCATION, "topleft");
		}
		else if (iselection == 2)
		{
			NamedList_AddString(&(data->localoptions), OPTION_NOTIFICATIONLOCATION, "topright");
		}
		else if (iselection == 3)
		{
			NamedList_AddString(&(data->localoptions), OPTION_NOTIFICATIONLOCATION, "bottomleft");
		}

	}
}

void OptionsPiecesThemeCombo_OnSelection(struct Box_s *combobox, char *selection)
{
	struct Box_s *dialog = Box_GetRoot(combobox);
	struct optionsdata_s *data = dialog->boxdata;

	if (selection)
	{
		NamedList_RemoveByName(&(data->localoptions), OPTION_PIECESTHEME);
		
		if (selection && stricmp(selection, "alpha") != 0)
		{
			NamedList_AddString(&(data->localoptions), OPTION_PIECESTHEME, selection);
		}
	}
}

struct volumedragdata_s
{
	int clicked;
	int xclick;
	int yclick;
	int xstart;
	int ystart;
	int dragging;
};

void VolumeDrag_OnDestroy(struct Box_s *vdrag)
{
	struct volumedragdata_s *data = vdrag->boxdata;

	Box_ReleaseMouse(vdrag);
}

void VolumeDrag_OnLButtonDown(struct Box_s *vdrag, int xmouse, int ymouse)
{
	struct volumedragdata_s *data = vdrag->boxdata;

	data->clicked = 1;
	data->dragging = 0;
	Box_GetScreenCoords(vdrag, &(data->xclick), &(data->yclick));
	data->xclick += xmouse;
	data->yclick += ymouse;
	data->xstart = vdrag->x;
	data->ystart = vdrag->y;
	Box_ReleaseMouse(vdrag);

	Box_OnLButtonDown(vdrag, xmouse, ymouse);
}

void VolumeDrag_OnLButtonUp(struct Box_s *vdrag, int xmouse, int ymouse)
{
	struct volumedragdata_s *data = vdrag->boxdata;

	data->dragging = 0;
	data->clicked = 0;
	Box_ReleaseMouse(vdrag);

	Box_OnLButtonUp(vdrag, xmouse, ymouse);
}

void VolumeDrag_OnMouseMove(struct Box_s *vdrag, int xmouse, int ymouse)
{
	struct volumedragdata_s *data = vdrag->boxdata;
	int xscreen, yscreen;

	Box_GetScreenCoords(vdrag, &xscreen, &yscreen);

	xscreen += xmouse;
	yscreen += ymouse;

	if (data->clicked && abs(data->xclick - xscreen) > 5)
	{
		data->clicked = 0;
		if (Box_CaptureMouse(vdrag))
		{
			data->dragging = 1;
		}
	}

	if (data->dragging)
	{
		vdrag->x = data->xstart + xscreen - data->xclick;

		if (vdrag->x < 0)
		{
			vdrag->x = 0;
		}

		if (vdrag->x > vdrag->parent->w - vdrag->w)
		{
			vdrag->x = vdrag->parent->w - vdrag->w;
		}

		Box_Repaint(vdrag->parent);

		Audio_SetVolume(100 * vdrag->x / (vdrag->parent->w - vdrag->w));
	}
}

struct Box_s *VolumeDrag_Create(int x, int y, enum Box_flags flags)
{
	struct Box_s *vdrag = Box_Create(x, y, 10, 20, flags);
	struct volumedragdata_s *data = malloc(sizeof(*data));

	memset(data, 0, sizeof(*data));
	vdrag->boxdata = data;

	vdrag->OnLButtonDown = VolumeDrag_OnLButtonDown;
	vdrag->OnLButtonUp   = VolumeDrag_OnLButtonUp;
	vdrag->OnMouseMove   = VolumeDrag_OnMouseMove;
	vdrag->OnDestroy     = VolumeDrag_OnDestroy;

	return vdrag;
}

#define OPTION_ADDCHECK(option, title) \
horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);              \
Box_AddChild(vertsize, horizsize);                                          \
                                                                            \
AutoSize_AddSpacer(horizsize, 5);                                           \
                                                                            \
pbox = CheckBox_Create(0, 0, BOX_VISIBLE | BOX_TRANSPARENT);                \
CheckBox_SetChecked(pbox,                                                   \
	NamedList_GetByName(&(data->localoptions), option) != NULL);        \
CheckBox_SetOnHit2(pbox, OptionsCheck_OnHit, option);                       \
Box_AddChild(horizsize, pbox);                                              \
                                                                            \
AutoSize_AddSpacer(horizsize, 5);                                           \
                                                                            \
pbox2 = CheckBoxLinkedText_Create(0, 0, 0, 20,                              \
	BOX_VISIBLE | BOX_TRANSPARENT, pbox);                               \
pbox2->fgcol = UserInfoFG2;                                                 \
Box_SetText(pbox2, title);                                                  \
Box_MeasureText(pbox2, pbox2->text, &w, &h);                                \
pbox2->w = w;                                                               \
Box_AddChild(horizsize, pbox2);

#define OPTION_ADDREVERSECHECK(option, title) \
horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);              \
Box_AddChild(vertsize, horizsize);                                          \
                                                                            \
AutoSize_AddSpacer(horizsize, 5);                                           \
                                                                            \
pbox = CheckBox_Create(0, 0, BOX_VISIBLE | BOX_TRANSPARENT);                \
CheckBox_SetChecked(pbox,                                                   \
	NamedList_GetByName(&(data->localoptions), option) == NULL);        \
CheckBox_SetOnHit2(pbox, OptionsCheck_OnHitReverse, option);                \
Box_AddChild(horizsize, pbox);                                              \
                                                                            \
AutoSize_AddSpacer(horizsize, 5);                                           \
                                                                            \
pbox2 = CheckBoxLinkedText_Create(0, 0, 0, 20,                              \
	BOX_VISIBLE | BOX_TRANSPARENT, pbox);                               \
pbox2->fgcol = UserInfoFG2;                                                 \
Box_SetText(pbox2, title);                                                  \
Box_MeasureText(pbox2, pbox2->text, &w, &h);                                \
pbox2->w = w;                                                               \
Box_AddChild(horizsize, pbox2);

void Options_SetPage(struct Box_s *dialog, char *page)
{
	struct optionsdata_s *data = dialog->boxdata;
	int w, h;

	while (data->optionpage->child)
	{
		Box_Destroy(data->optionpage->child);
	}

	if (stricmp(page, "General Options") == 0)
	{
		struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		Box_AddChild(data->optionpage, vertsize);
		{
			struct Box_s *pbox, *pbox2, *horizsize;

			pbox = Text_Create(0, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Text_SetText(pbox, "^bGeneral Options");
			Box_AddChild(vertsize, pbox);

			OPTION_ADDCHECK(OPTION_AUTOAPPROVE, "Auto-approve friend requests");

			/* need to add autoaway manually since it has a edit box in it */
			horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			Box_AddChild(vertsize, horizsize);
			{
				AutoSize_AddSpacer(horizsize, 5);

				pbox = CheckBox_Create(0, 0, BOX_VISIBLE | BOX_TRANSPARENT);
				CheckBox_SetChecked(pbox, NamedList_GetByName(&(data->localoptions), "autoaway") != NULL);
				CheckBox_SetOnHit2(pbox, OptionsCheck_OnHit, "autoaway");
				Box_AddChild(horizsize, pbox);

				AutoSize_AddSpacer(horizsize, 5);

				pbox2 = CheckBoxLinkedText_Create(0, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT, pbox);
				pbox2->fgcol = UserInfoFG2;
				Box_SetText(pbox2, "Automatic away after");
				Box_MeasureText(pbox2, pbox2->text, &w, &h);
				pbox2->w = w;
				Box_AddChild(horizsize, pbox2);

				AutoSize_AddSpacer(horizsize, 4);

				pbox2 = Edit2Box_Create(0, -2, 26, 18, BOX_VISIBLE, E2_HORIZ);
				Edit2Box_SetOnKey(pbox2, OptionsAwayTimeEdit_OnKey);
				{
					char text[256];
					sprintf(text, "%d", data->autoawaytime);
					Edit2Box_SetText(pbox2, text);
				}
				pbox2->bgcol = RGB(255, 255, 255);
				Box_AddChild(horizsize, pbox2);

				AutoSize_AddSpacer(horizsize, 4);

				pbox2 = CheckBoxLinkedText_Create(0, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT, pbox);
				pbox2->fgcol = UserInfoFG2;
				Box_SetText(pbox2, "minutes");
				Box_MeasureText(pbox2, pbox2->text, &w, &h);
				pbox2->w = w;
				Box_AddChild(horizsize, pbox2);
			}
                        
			OPTION_ADDCHECK(OPTION_HIDEWELCOMEDIALOG, "Don't show welcome dialog");
			/*OPTION_ADDCHECK(OPTION_NOGAMESEARCHONLOGIN, "Don't show game search dialog on login");*/
		}
		AutoSize_Fit(vertsize);
		AutoSize_Fill(vertsize);
		Box_Repaint(data->optionpage);
	}
	else if (stricmp(page, "Friends List") == 0)
	{
		struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		Box_AddChild(data->optionpage, vertsize);
		{
			struct Box_s *pbox, *pbox2, *horizsize;

			pbox = Text_Create(0, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Text_SetText(pbox, "^bFriends List");
			Box_AddChild(vertsize, pbox);

			OPTION_ADDCHECK(OPTION_SHOWOFFLINE, "Show Offline Friends");
			OPTION_ADDCHECK(OPTION_SHOWAVATARS, "Show Friend Avatars");
		}
		AutoSize_Fit(vertsize);
		AutoSize_Fill(vertsize);
		Box_Repaint(data->optionpage);
	}
	else if (stricmp(page, "Chat Options") == 0)
	{
		struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		Box_AddChild(data->optionpage, vertsize);
		{
			struct Box_s *pbox, *pbox2, *horizsize;

			pbox = Text_Create(0, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Text_SetText(pbox, "^bChat Options");
			Box_AddChild(vertsize, pbox);

			OPTION_ADDCHECK(OPTION_SHOWMUCPRESENCEINCHAT, "Always show join/part notices in chat rooms");
			OPTION_ADDCHECK(OPTION_SHOWMUCPRESENCEINCHATWHENCLOSED, "Show join/part notices when participants list is closed");
			OPTION_ADDCHECK(OPTION_HIDEPARTICIPANTS, "Hide participants list by default");
			OPTION_ADDCHECK(OPTION_MESSAGEONAWAY, "Automatically respond to messages when away");
			OPTION_ADDCHECK(OPTION_PERLINECHATTIMESTAMPS, "Enable per line time stamps");
			OPTION_ADDCHECK(OPTION_DISABLEPROFANITYFILTER, "Disable profanity filter");
		}
		AutoSize_Fit(vertsize);
		AutoSize_Fill(vertsize);
		Box_Repaint(data->optionpage);
	}
	else if (stricmp(page, "Notification Options") == 0)
	{
		struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		Box_AddChild(data->optionpage, vertsize);
		{
			struct Box_s *pbox, *pbox2, *horizsize;

			pbox = Text_Create(0, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Text_SetText(pbox, "^bNotification Options");
			Box_AddChild(vertsize, pbox);

			OPTION_ADDCHECK(OPTION_NOCHATNOTIFY, "Don't alert on new chat messages");
			OPTION_ADDCHECK(OPTION_NOGAMENOTIFY, "Don't alert on new game invite");
			OPTION_ADDREVERSECHECK(OPTION_DISABLEROSTERGAMENOTIFICATIONS, "Enable notifications when a friend is playing");

			/* need to add notification speed and location manually since they are edit/combo boxes */
			horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			Box_AddChild(vertsize, horizsize);
			{
				AutoSize_AddSpacer(horizsize, 10 + 13);

				pbox2 = Box_Create(0, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox2->fgcol = UserInfoFG2;
				Box_SetText(pbox2, "Notifications stay on screen for ");
				Box_MeasureText(pbox2, pbox2->text, &w, &h);
				pbox2->w = w;
				Box_AddChild(horizsize, pbox2);

				AutoSize_AddSpacer(horizsize, 4);

				pbox2 = Edit2Box_Create(0, -2, 26, 18, BOX_VISIBLE, E2_HORIZ);
				Edit2Box_SetOnKey(pbox2, OptionsNotificationTimeEdit_OnKey);
				{
					char text[256];
					sprintf(text, "%d", data->notificationtime);
					Edit2Box_SetText(pbox2, text);
				}
				pbox2->bgcol = RGB(255, 255, 255);
				Box_AddChild(horizsize, pbox2);

				AutoSize_AddSpacer(horizsize, 4);

				pbox2 = Box_Create(0, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox2->fgcol = UserInfoFG2;
				Box_SetText(pbox2, "seconds");
				Box_MeasureText(pbox2, pbox2->text, &w, &h);
				pbox2->w = w;
				Box_AddChild(horizsize, pbox2);
			}

			horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			Box_AddChild(vertsize, horizsize);
			{
				AutoSize_AddSpacer(horizsize, 10 + 13);

				pbox2 = Box_Create(0, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox2->fgcol = UserInfoFG2;
				Box_SetText(pbox2, "Notifications appear in the ");
				Box_MeasureText(pbox2, pbox2->text, &w, &h);
				pbox2->w = w;
				Box_AddChild(horizsize, pbox2);

				AutoSize_AddSpacer(horizsize, 4);

				pbox2 = ComboBox_Create(0, -2, 90, 18, BOX_VISIBLE);
				ComboBox_SetOnSelection(pbox2, OptionsNotificationLocationCombo_OnSelection);
				ComboBox_AddEntry2(pbox2, "1", _("Top-left"));
				ComboBox_AddEntry2(pbox2, "2", _("Top-right"));
				ComboBox_AddEntry2(pbox2, "3", _("Bottom-left"));
				ComboBox_AddEntry2(pbox2, "0", _("Bottom-right"));
				{
					struct namedlist_s **entry = NamedList_GetByName((&data->localoptions), OPTION_NOTIFICATIONLOCATION);
					char *location = NULL;

					if (entry)
					{
						location = (*entry)->data;
					}

					if (location && stricmp(location, "topleft") == 0)
					{
						ComboBox_SetSelection(pbox2, "1");
					}
					else if (location && stricmp(location, "topright") == 0)
					{
						ComboBox_SetSelection(pbox2, "2");
					}
					else if (location && stricmp(location, "bottomleft") == 0)
					{
						ComboBox_SetSelection(pbox2, "3");
					}
					else 
					{
						ComboBox_SetSelection(pbox2, "0");
					}
				}
				pbox2->bgcol = RGB(255, 255, 255);
				Box_AddChild(horizsize, pbox2);

				AutoSize_AddSpacer(horizsize, 4);

				pbox2 = Box_Create(0, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT);
				pbox2->fgcol = UserInfoFG2;
				Box_SetText(pbox2, "corner");
				Box_MeasureText(pbox2, pbox2->text, &w, &h);
				pbox2->w = w;
				Box_AddChild(horizsize, pbox2);
			}
		}
		AutoSize_Fit(vertsize);
		AutoSize_Fill(vertsize);
		Box_Repaint(data->optionpage);
	}
	else if (stricmp(page, "Sound Options") == 0)
	{
		struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		Box_AddChild(data->optionpage, vertsize);
		{
			struct Box_s *pbox, *pbox2, *horizsize;

			pbox = Text_Create(0, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Text_SetText(pbox, "^bSound Options");
			Box_AddChild(vertsize, pbox);

			OPTION_ADDREVERSECHECK(OPTION_DISABLESOUNDS, "Enable sound");
			OPTION_ADDREVERSECHECK(OPTION_DISABLEGAMESOUNDS, "Enable game sounds");
			OPTION_ADDREVERSECHECK(OPTION_DISABLEIMSOUNDS, "Enable IM sounds");
			OPTION_ADDCHECK(OPTION_INITIALIMSOUNDONLY, "Only make IM sound on initial message");

			pbox = Text_Create(5, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Text_SetText(pbox, "Volume:");
			Box_AddChild(vertsize, pbox);

			pbox = Box_Create(5, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
			pbox->bgcol = TabBG1;
			pbox->img = ImageMgr_GetImage("HVolumeTrackCenter.png");
			Box_AddChild(vertsize, pbox);

			pbox2 = VolumeDrag_Create(Audio_GetVolume() * 110 / 100, 0, BOX_VISIBLE | BOX_TRANSPARENT);
			pbox2->img = ImageMgr_GetImage("HVolumeTrackThumb.png");
			Box_AddChild(pbox, pbox2);

		}
		AutoSize_Fit(vertsize);
		AutoSize_Fill(vertsize);
		Box_Repaint(data->optionpage);
	}
	else if (stricmp(page, "Game Options") == 0)
	{
		struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		Box_AddChild(data->optionpage, vertsize);
		{
			struct Box_s *pbox, *pbox2, *horizsize;

			pbox = Text_Create(0, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Text_SetText(pbox, "^bGame Options");
			Box_AddChild(vertsize, pbox);

			OPTION_ADDCHECK(OPTION_ENABLEANTISLIP, "Enable anti-slip on click");
			OPTION_ADDCHECK(OPTION_NOGAMEAUTOFLAG, "Disable auto-gameover when opponent's clock runs out");
			OPTION_ADDCHECK(OPTION_NODECLINEWHENAWAY, "Disable automatic game decline when away");
			OPTION_ADDCHECK(OPTION_ENABLELAGHIDING, "Enable lag hiding");
			OPTION_ADDREVERSECHECK(OPTION_DISABLEANIMATION, "Enable piece animation");
			OPTION_ADDCHECK(OPTION_SHOWLEGALMOVES, "Show legal moves in unrated standard games");

			/* need to add theme manually since it's a combo box */
			if (!Model_IsLocalMemberFree(1))
			{
				horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
				Box_AddChild(vertsize, horizsize);
				{
					AutoSize_AddSpacer(horizsize, 10 + 13);

					pbox2 = Box_Create(0, 0, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT);
					pbox2->fgcol = UserInfoFG2;
					Box_SetText(pbox2, "Piece theme is");
					Box_MeasureText(pbox2, pbox2->text, &w, &h);
					pbox2->w = w;
					Box_AddChild(horizsize, pbox2);

					AutoSize_AddSpacer(horizsize, 4);

					pbox2 = ComboBox_Create(0, -2, 90, 18, BOX_VISIBLE);
					ComboBox_SetOnSelection(pbox2, OptionsPiecesThemeCombo_OnSelection);
					ComboBox_AddEntry2(pbox2, "alpha", _("Alpha"));
					ComboBox_AddEntry2(pbox2, "eyes", _("Eyes"));
					ComboBox_AddEntry2(pbox2, "fantasy", _("Fantasy"));
					ComboBox_AddEntry2(pbox2, "skulls", _("Skulls"));
					ComboBox_AddEntry2(pbox2, "spatial", _("Spatial"));
					{
						struct namedlist_s **entry = NamedList_GetByName((&data->localoptions), OPTION_PIECESTHEME);
						char *theme = NULL;

						if (entry)
						{
							theme = (*entry)->data;
						}

						if (theme)
						{
							if (stricmp(theme, "eyes") == 0)
							{
								theme = "eyes";
							}
							else if (stricmp(theme, "fantasy") == 0)
							{
								theme = "fantasy";
							}
							else if (stricmp(theme, "skulls") == 0)
							{
								theme = "skulls";
							}
							else if (stricmp(theme, "spatial") == 0)
							{
								theme = "spatial";
							}
							else
							{
								theme = "alpha";
							}
						}

						if (!theme)
						{
							theme = "alpha";
						}

						ComboBox_SetSelection(pbox2, theme);
					}
					pbox2->bgcol = RGB(255, 255, 255);
					Box_AddChild(horizsize, pbox2);
				}
			}
		}
		AutoSize_Fit(vertsize);
		AutoSize_Fill(vertsize);
		Box_Repaint(data->optionpage);
	}
	else if (stricmp(page, "Privacy") == 0)
	{
		struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		Box_AddChild(data->optionpage, vertsize);
		{
			struct Box_s *pbox;
			struct namedlist_s *ignorelist = data->ignorelist;
			struct namedlist_s *entry;

			pbox = Text_Create(0, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Text_SetText(pbox, "^bPrivacy");
			Box_AddChild(vertsize, pbox);

			pbox = Text_Create(0, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Text_SetText(pbox, "Ignore list:");
			Box_AddChild(vertsize, pbox);

			pbox = List_Create(0, 0, 380, 100, BOX_VISIBLE, 0);
			Box_AddChild(vertsize, pbox);
			data->ignorelistbox = pbox;

			entry = ignorelist;
			while (entry)
			{
				OptionsIgnoreList_AddEntry(pbox, entry->data);
				entry = entry->next;
			}
		}
		AutoSize_Fit(vertsize);
		AutoSize_Fill(vertsize);
		Box_Repaint(data->optionpage);
	}
	else if (stricmp(page, "Help Prompts") == 0)
	{
		struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		Box_AddChild(data->optionpage, vertsize);
		{
			struct Box_s *pbox, *pbox2, *horizsize;

			pbox = Text_Create(0, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Text_SetText(pbox, "^bHelp Prompts");
			Box_AddChild(vertsize, pbox);

			OPTION_ADDCHECK(OPTION_HIDEGAMEFINDERHELP, "Hide game finder help bubble");
			OPTION_ADDCHECK(OPTION_HIDEWARNINGONDISCONNECT, "Hide forfeit warning when leaving page or disconnecting");
			OPTION_ADDCHECK(OPTION_HIDEVARIANTWARNING, "Hide warning when accepting a game with a non-standard variant");
		}
		AutoSize_Fit(vertsize);
		AutoSize_Fill(vertsize);
		Box_Repaint(data->optionpage);
	}
	else if (stricmp(page, "Advanced") == 0)
	{
		struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		Box_AddChild(data->optionpage, vertsize);
		{
			struct Box_s *pbox, *pbox2, *horizsize;

			pbox = Text_Create(0, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = UserInfoFG2;
			Text_SetText(pbox, "^bAdvanced");
			Box_AddChild(vertsize, pbox);

			OPTION_ADDCHECK(OPTION_DEVFEATURES, "Enable development features");
			/*OPTION_ADDCHECK(OPTION_ENABLEPUSHADS, "Enable new push-style game ads");*/
			OPTION_ADDCHECK(OPTION_TABNEWCHATS, "Open new chats as tabs in the current window");
			OPTION_ADDCHECK(OPTION_ENABLECHATMERGING, "Enable auto-merging chat dialogs");
			/*OPTION_ADDCHECK(OPTION_LOWBANDWIDTH, "Enable low-bandwidth mode");*/
		}
		AutoSize_Fit(vertsize);
		AutoSize_Fill(vertsize);
		Box_Repaint(data->optionpage);
	}
}

void OptionsLink_OnClick(struct Box_s *pbox, char *page)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct Box_s *parent = pbox->parent->parent;
	struct Box_s *child = parent->child;
	struct optionsdata_s *data = dialog->boxdata;

	while (child)
	{
		if (pbox->parent == child)
		{
			child->bgcol = RGB(128, 128, 128);
		}
		else
		{
			child->bgcol = RGB(31, 37, 48);
		}

		child = child->sibling;
	}

	Box_Repaint(dialog);

	Options_SetPage(dialog, page);
}

struct Box_s *Options_Create(struct Box_s *roster)
{
	struct optionsdata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *pbox, *vertsize;

	memset(data, 0, sizeof(*data));

	/*
	data->localoptions = NamedList_DupeStringList(Model_GetOptions());
	*/
	{
		struct namedlist_s *newoptions = Model_GetOptions();
		data->localoptions = NULL;

		while (newoptions)
		{
			if (stricmp(newoptions->name, OPTION_SEARCHFILTERS) == 0)
			{
				struct namedlist_s *searchfilters = NULL;
				struct namedlist_s *entry = newoptions->data;
				while (entry)
				{
					NamedList_Add(&searchfilters, entry->name, Info_DupeGameSearchInfo(entry->data), Info_DestroyGameSearchInfo);
					entry = entry->next;
				}
				NamedList_Add(&(data->localoptions), newoptions->name, searchfilters, NamedList_Destroy2);
			}
			else if (stricmp(newoptions->name, OPTION_LOGINROOMS) == 0)
			{
				struct namedlist_s *loginrooms = NULL;
				struct namedlist_s *entry = newoptions->data;

				while (entry)
				{
					NamedList_AddString(&loginrooms, entry->name, entry->data);
					entry = entry->next;
				}

				NamedList_Add(&(data->localoptions), newoptions->name, loginrooms, NamedList_Destroy2);
			}
			else
			{
				NamedList_AddString(&(data->localoptions), newoptions->name, newoptions->data);
			}
			newoptions = newoptions->next;
		}
	}



	{
		struct namedlist_s **entry = NamedList_GetByName(&(data->localoptions), OPTION_AUTOAWAY);

		if (entry)
		{
			sscanf((*entry)->data, "%d", &(data->autoawaytime));
		}
		else
		{
			data->autoawaytime = 5;
		}

		entry = NamedList_GetByName(&(data->localoptions), OPTION_NOTIFICATIONTIME);

		if (entry)
		{
			sscanf((*entry)->data, "%d", &(data->notificationtime));
		}
		else
		{
			data->notificationtime = 5;
		}
	}
	
	{
		struct namedlist_s *entry = data->localoptions;

		Log_Write(0, "Options listing: \n");
		while (entry)
		{
			Log_Write(0, "option %s\n", entry->name);
			entry = entry->next;
		}

		entry = Model_GetOptions();

		Log_Write(0, "Options listing: \n");
		while (entry)
		{
			Log_Write(0, "option %s\n", entry->name);
			entry = entry->next;
		}

	}

	data->ignorelist = NamedList_DupeList(Model_GetIgnoreList(), Info_DupePrivacyListEntry, Info_DestroyPrivacyListEntry);

	dialog = Box_Create(0, 0, 440, 270, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;

	dialog->boxdata = data;

	dialog->titlebar = TitleBarOnly_Add(dialog, _("Chesspark Options"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	vertsize = AutoSize_Create(10, 43, 0, 0, 0, 0, AUTOSIZE_VERT);
	Box_AddChild(dialog, vertsize);
	data->sizeablecontent = vertsize;
	{
		struct Box_s *horizsize;
		struct Box_s *vertsize2;

		vertsize2 = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
		vertsize2->OnSizeWidth = Box_OnSizeWidth_Stretch;
		Box_AddChild(vertsize, vertsize2);
		{
#if 0
			struct Box_s *horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			Box_AddChild(vertsize2, horizsize);
			{
				/* need a new asset for this, and it doesn't work right now anyway, so it's out */
				pbox = Button_Create(0, 0, 21, 24, BOX_VISIBLE | BOX_TRANSPARENT);
				Button_SetNormalImg (pbox, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
				Button_SetPressedImg(pbox, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
				Button_SetHoverImg  (pbox, ImageMgr_GetSubImage("BackButton4", "BackButton.png", 66, 0, 22, 24));
				/*Button_SetOnButtonHit(pbox, GamesListBackButton_OnHit);*/
				Button_SetTooltipText(pbox, _("Back"));
				Box_AddChild(horizsize, pbox);
				pbox = ComboBox_Create(0, 2, 150, 20, BOX_VISIBLE | BOX_BORDER);
				/*
				ComboBox_AddEntry(pbox, _("General Options"));
				ComboBox_AddEntry(pbox, _("Friends List"));
				ComboBox_AddEntry(pbox, _("Chat Options"));
				ComboBox_AddEntry(pbox, _("Notification Options"));
				ComboBox_SetSelection(pbox, _("General Options"));
				*/
				ComboBox_AddEntry(pbox, _("Privacy"));
				ComboBox_SetSelection(pbox, _("Privacy"));
				/*ComboBox_SetOnSelection(subbox, GamesListPageCombo_OnSelection);*/
				Box_AddChild(horizsize, pbox);
				/*data->combobox = subbox;*/
			}

			AutoSize_AddSpacer(vertsize2, 2);

			pbox = Box_Create(0, 0, vertsize2->w, 1, BOX_VISIBLE);
			pbox->bgcol = RGB(77, 77, 77);
			pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
			Box_AddChild(vertsize2, pbox);

			pbox = Box_Create(0, 0, vertsize2->w, 1, BOX_VISIBLE);
			pbox->bgcol = RGB(102, 102, 102);
			pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
			Box_AddChild(vertsize2, pbox);

			AutoSize_AddSpacer(vertsize2, 13);
#endif

			struct Box_s *horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
			Box_AddChild(vertsize2, horizsize);
			{
				struct Box_s *vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
				vertsize->OnSizeHeight = Box_OnSizeHeight_Stretch;
				vertsize->flags = BOX_VISIBLE | BOX_BORDER;
				vertsize->bgcol = RGB(31, 37, 48);
				vertsize->brcol = UserInfoFG2;
				Box_AddChild(horizsize, vertsize);
				{
					struct Box_s *horizsize;
					
					AutoSize_AddSpacer(vertsize, 10);
					
					horizsize = AutoSize_Create(2, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					horizsize->flags &= ~BOX_TRANSPARENT;
					horizsize->bgcol = RGB(128, 128, 128);
					Box_AddChild(vertsize, horizsize);
					{
						AutoSize_AddSpacer(horizsize, 10);

						pbox = Text_Create(10, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
						Text_SetLinkColor(pbox, CR_LtOrange);
						Text_SetText(pbox, "^lGeneral Options^l");
						Text_SetLinkCallback(pbox, 1, OptionsLink_OnClick, "General Options");
						Box_AddChild(horizsize, pbox);
					}

					AutoSize_AddSpacer(vertsize, 10);
					
					horizsize = AutoSize_Create(2, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					horizsize->flags &= ~BOX_TRANSPARENT;
					horizsize->bgcol = RGB(31, 37, 48);
					Box_AddChild(vertsize, horizsize);
					{
						AutoSize_AddSpacer(horizsize, 10);

						pbox = Text_Create(10, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
						Text_SetLinkColor(pbox, CR_LtOrange);
						Text_SetText(pbox, "^lFriends List^l");
						Text_SetLinkCallback(pbox, 1, OptionsLink_OnClick, "Friends List");
						Box_AddChild(horizsize, pbox);
					}

					AutoSize_AddSpacer(vertsize, 10);
					
					horizsize = AutoSize_Create(2, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					horizsize->flags &= ~BOX_TRANSPARENT;
					horizsize->bgcol = RGB(31, 37, 48);
					Box_AddChild(vertsize, horizsize);
					{
						AutoSize_AddSpacer(horizsize, 10);

						pbox = Text_Create(10, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
						Text_SetLinkColor(pbox, CR_LtOrange);
						Text_SetText(pbox, "^lChat Options^l");
						Text_SetLinkCallback(pbox, 1, OptionsLink_OnClick, "Chat Options");
						Box_AddChild(horizsize, pbox);
					}

					AutoSize_AddSpacer(vertsize, 10);
					
					horizsize = AutoSize_Create(2, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					horizsize->flags &= ~BOX_TRANSPARENT;
					horizsize->bgcol = RGB(31, 37, 48);
					Box_AddChild(vertsize, horizsize);
					{
						AutoSize_AddSpacer(horizsize, 10);

						pbox = Text_Create(10, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
						Text_SetLinkColor(pbox, CR_LtOrange);
						Text_SetText(pbox, "^lNotification Options^l");
						Text_SetLinkCallback(pbox, 1, OptionsLink_OnClick, "Notification Options");
						Box_AddChild(horizsize, pbox);
					}

					AutoSize_AddSpacer(vertsize, 10);
					
					horizsize = AutoSize_Create(2, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					horizsize->flags &= ~BOX_TRANSPARENT;
					horizsize->bgcol = RGB(31, 37, 48);
					Box_AddChild(vertsize, horizsize);
					{
						AutoSize_AddSpacer(horizsize, 10);

						pbox = Text_Create(10, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
						Text_SetLinkColor(pbox, CR_LtOrange);
						Text_SetText(pbox, "^lSound Options^l");
						Text_SetLinkCallback(pbox, 1, OptionsLink_OnClick, "Sound Options");
						Box_AddChild(horizsize, pbox);
					}

					AutoSize_AddSpacer(vertsize, 10);
					
					horizsize = AutoSize_Create(2, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					horizsize->flags &= ~BOX_TRANSPARENT;
					horizsize->bgcol = RGB(31, 37, 48);
					Box_AddChild(vertsize, horizsize);
					{
						AutoSize_AddSpacer(horizsize, 10);

						pbox = Text_Create(10, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
						Text_SetLinkColor(pbox, CR_LtOrange);
						Text_SetText(pbox, "^lGame Options^l");
						Text_SetLinkCallback(pbox, 1, OptionsLink_OnClick, "Game Options");
						Box_AddChild(horizsize, pbox);
					}

					AutoSize_AddSpacer(vertsize, 10);
					
					horizsize = AutoSize_Create(2, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					horizsize->flags &= ~BOX_TRANSPARENT;
					horizsize->bgcol = RGB(31, 37, 48);
					Box_AddChild(vertsize, horizsize);
					{
						AutoSize_AddSpacer(horizsize, 10);

						pbox = Text_Create(10, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
						Text_SetLinkColor(pbox, CR_LtOrange);
						Text_SetText(pbox, "^lPrivacy^l");
						Text_SetLinkCallback(pbox, 1, OptionsLink_OnClick, "Privacy");
						Box_AddChild(horizsize, pbox);
					}

					AutoSize_AddSpacer(vertsize, 10);
					
					horizsize = AutoSize_Create(2, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					horizsize->flags &= ~BOX_TRANSPARENT;
					horizsize->bgcol = RGB(31, 37, 48);
					Box_AddChild(vertsize, horizsize);
					{
						AutoSize_AddSpacer(horizsize, 10);

						pbox = Text_Create(10, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
						Text_SetLinkColor(pbox, CR_LtOrange);
						Text_SetText(pbox, "^lHelp Prompts^l");
						Text_SetLinkCallback(pbox, 1, OptionsLink_OnClick, "Help Prompts");
						Box_AddChild(horizsize, pbox);
					}

					AutoSize_AddSpacer(vertsize, 10);
					
					horizsize = AutoSize_Create(2, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
					horizsize->flags &= ~BOX_TRANSPARENT;
					horizsize->bgcol = RGB(31, 37, 48);
					Box_AddChild(vertsize, horizsize);
					{
						AutoSize_AddSpacer(horizsize, 10);

						pbox = Text_Create(10, 0, 130, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
						Text_SetLinkColor(pbox, CR_LtOrange);
						Text_SetText(pbox, "^lAdvanced^l");
						Text_SetLinkCallback(pbox, 1, OptionsLink_OnClick, "Advanced");
						Box_AddChild(horizsize, pbox);
					}

					AutoSize_AddSpacer(vertsize, 10);
				}

				AutoSize_AddSpacer(horizsize, 10);

				vertsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_VERT);
				Box_AddChild(horizsize, vertsize);
				{
					pbox = Box_Create(0, 0, 380, 170, BOX_VISIBLE | BOX_TRANSPARENT);
					Box_AddChild(vertsize, pbox);
					data->optionpage = pbox;
				}
			}
		}

		AutoSize_AddSpacer(vertsize, 37);

		horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
		horizsize->OnSizeWidth = Box_OnSizeWidth_Stretch;
		Box_AddChild(vertsize, horizsize);
		{
			struct Box_s *horizsize2 = AutoSizeSpace_Create(0, 0, 0, 0, 0, 0, 10, AUTOSIZE_HORIZ);
			horizsize2 = AutoSizeSpace_Create(0, 0, 0, 0, 0, 0, 10, AUTOSIZE_HORIZ);
			horizsize2->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Box_AddChild(horizsize, horizsize2);
			{	
				pbox = StdButton_Create(0, 0, 90, _("Save"), 0);
				Button2_SetOnButtonHit(pbox, Options_OnSave);
				Box_AddChild(horizsize2, pbox);
				/*data->acceptbutton = pbox;*/

				pbox = StdButton_Create(0, 0, 90, _("Cancel"), 0);
				Button2_SetOnButtonHit(pbox, Options_OnCancel);
				Box_AddChild(horizsize2, pbox);
				/*data->cancelbutton = pbox;*/
			}
		}
	}

	AutoSize_Fit(data->sizeablecontent);
	AutoSize_Fill(data->sizeablecontent);

	Box_OnSizeWidth_Stretch(dialog, data->sizeablecontent->w + 20 - dialog->w);
	Box_OnSizeHeight_Stretch(dialog, data->sizeablecontent->h + 50 - dialog->h);

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;
		int remainw, remainh;

		windowrect.left = roster->x;
		windowrect.right = windowrect.left + roster->w - 1;
		windowrect.top = roster->y;
		windowrect.bottom = windowrect.top + roster->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		remainw = mi.rcWork.right - mi.rcWork.left - dialog->w;
		remainh = mi.rcWork.bottom - mi.rcWork.top - dialog->h;

		dialog->x = remainw / 2;
		dialog->y = remainh / 2;

		dialog->x += mi.rcWork.left;
		dialog->y += mi.rcWork.top;
	}

	Options_SetPage(dialog, "General Options");

	Box_CreateWndCustom(dialog, _("Chesspark Options"), roster->hwnd);

	dialog->OnDestroy = Options_OnDestroy;

	return dialog;
}
