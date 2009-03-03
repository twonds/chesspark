#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>

#include <stdio.h>
#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "constants.h"
#include "edit2.h"
#include "titledrag.h"
#include "text.h"

#include "button2.h"
#include "checkbox.h"
#include "combobox.h"
#include "ctrl.h"
#include "i18n.h"
#include "imagemgr.h"
#include "util.h"
#include "view.h"
#include "stdbutton.h"
#include "titlebar.h"

#include "login.h"


struct Logindata_s
{
	struct Box_s *logo;
	struct Box_s *noaccount;
	struct Box_s *namelabel;
	struct Box_s *passlabel;
	struct Box_s *nameentry;
	struct Box_s *passentry;
	struct Box_s *rememberlabel;
	struct Box_s *remembercheck;
	struct Box_s *loginbutton;
	struct Box_s *connectanimation;
	struct Box_s *connectmessage;
	struct Box_s *errorbox;
	struct Box_s *cancelbutton;
	struct Box_s *languagelabel;
	struct Box_s *languagecombo;
	BOOL remembermechecked;
};

void LoginName_OnLoseFocus(struct Box_s *pbox)
{
	struct Logindata_s *data = Box_GetRoot(pbox)->boxdata;
	char *name = ComboBox_GetSelectionName(data->nameentry);

	if (name && !strchr(name, '@'))
	{
		char newname[512];
		strcpy(newname, name);
		strcat(newname, "@chesspark.com");
		ComboBox_SetSelection(data->nameentry, newname);
	}
}

void Login_OnLogin(struct Box_s *pbox)
{
	struct Logindata_s *data = Box_GetRoot(pbox)->boxdata;
	char *username, *password;

	data->errorbox->flags         &= ~BOX_VISIBLE;
	Box_Repaint(Box_GetRoot(pbox));

	LoginName_OnLoseFocus(pbox);

	username = ComboBox_GetSelectionName(data->nameentry);
	password = Edit2Box_GetText(data->passentry);

	Ctrl_Login(data->remembermechecked, username, password);

	free(username);
	free(password);
}

void LoginName_OnEnter(struct Box_s *pbox, char *name)
{
	LoginName_OnLoseFocus(pbox);
	Login_OnLogin(pbox);
}

void LoginName_OnSelect(struct Box_s *pbox, char *name)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct Logindata_s *data = Box_GetRoot(pbox)->boxdata;

	Edit2Box_SetText(data->passentry, NULL);
}

void LoginPass_OnEnter(struct Box_s *pbox, char *name)
{
	Login_OnLogin(pbox);
}

void LoginCheck_OnHit(struct Box_s *pbox, int checked)
{
	struct Logindata_s *data = Box_GetRoot(pbox)->boxdata;

	data->remembermechecked = checked;
}

void Login_OnCancel(struct Box_s *pbox)
{
	struct Logindata_s *data = Box_GetRoot(pbox)->boxdata;

	Ctrl_CancelLogin();
}

void Login_OnClose(struct Box_s *pbox)
{
	PostQuitMessage(0);
}


void Login_OnWndCreate(struct Box_s *pbox)
{
	struct Logindata_s *data = pbox->boxdata;
}

void LoginLanguageCombo_OnSelection(struct Box_s *pbox, char *language)
{
	if (stricmp(language, "English") == 0)
	{
                View_ResetLanguage("en");
	}
	else if (stricmp(language, "Magyar") == 0)
	{
		View_ResetLanguage("hu");
	}
	else if (stricmp(language, "Bork! Bork! Bork!") == 0)
	{
		View_ResetLanguage("se-bo");
	}
}

extern HFONT tahoma10_f;

struct Box_s *Login_Create()
{
	struct Logindata_s *data = malloc(sizeof(*data));
	struct Box_s *login, *pbox;
	int x = (GetSystemMetrics(SM_CXFULLSCREEN) - 360) / 2;
	int y = (GetSystemMetrics(SM_CYFULLSCREEN) - 420) / 2;
	char buffer[2048];

	memset(data, 0, sizeof(*data));

	login = Box_Create(x, y, 360, 420, BOX_VISIBLE);
	login->bgcol = DefaultBG;
	login->fgcol = UserInfoFG2;

	pbox = Box_Create((login->w - 300) / 2, 40, 300, 180, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->bgcol = DefaultBG;
	pbox->fgcol = UserInfoFG2;
	pbox->img = ImageMgr_GetImage("bigLogo.gif");
	Box_AddChild(login, pbox);
	data->logo = pbox;

	{
		char txt[256];
		pbox = Box_Create(0, data->logo->y + data->logo->h - Margin - 3, login->w, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		pbox->fgcol = UserInfoFG2;
		pbox->font = tahoma10_f;
		i18n_stringsub(txt, 256, _("Version %1, Build %2"), CHESSPARK_VERSION, CHESSPARK_BUILD);
		Box_SetText(pbox, txt);
		Box_AddChild(login, pbox);
	}


	pbox = Box_Create(Margin, data->logo->y + data->logo->h - TextHeight * 2 - Margin * 2, 360 - Margin * 2, TextHeight * 2 + Margin * 2, 0);
	pbox->bgcol = RGB(96, 96, 96);
	Box_AddChild(login, pbox);
	data->errorbox = pbox;

	pbox = Box_Create(Margin, Margin, data->errorbox->w - Margin * 2, TextHeight, BOX_VISIBLE | BOX_CENTERTEXT | BOX_TRANSPARENT);
	pbox->fgcol = RGB(160, 160, 160);
	Box_AddChild(data->errorbox, pbox);

	pbox = Box_Create(Margin, Margin + TextHeight, data->errorbox->w - Margin * 2, TextHeight, BOX_VISIBLE | BOX_CENTERTEXT | BOX_TRANSPARENT);
	pbox->fgcol = RGB(160, 160, 160);
	Box_AddChild(data->errorbox, pbox);

	pbox = Text_Create(Margin, Margin * 2 + LogoHeight, LogoWidth, TextHeight * 2, BOX_VISIBLE, TX_CENTERED);
	pbox->bgcol = DefaultBG;
	pbox->fgcol = UserInfoFG1;
	Text_SetText(pbox, _("^lDon't have an account? Click here!^l"));
	Text_SetLinkColor(pbox, CR_LtOrange);
	Box_AddChild(login, pbox);
	Text_SetLinkCallback(pbox, 1, Util_OpenURL2, strdup(_("http://www.chesspark.com/join/")));
	data->noaccount = pbox;

	pbox = Box_Create(Margin * 5, Margin * 3 + LogoHeight + TextHeight * 2, 80, TextHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->bgcol = DefaultBG;
	pbox->fgcol = TabFG2;
	Box_SetText(pbox, _("Username"));
	Box_AddChild(login, pbox);
	data->namelabel = pbox;

	pbox = ComboEditBox_Create(Margin * 5 + 80 , Margin * 3 + LogoHeight + TextHeight * 2 - 2, 216, 20, BOX_VISIBLE);
	pbox->bgcol = RGB(255, 255, 255);
	pbox->fgcol = RGB(0, 0, 0);
	Box_AddChild(login, pbox);
	data->nameentry = pbox;
	ComboEditBox_SetOnEnter(data->nameentry, LoginName_OnEnter);
	ComboEditBox_SetOnLoseFocus(data->nameentry, LoginName_OnLoseFocus);
	ComboBox_SetOnSelection(data->nameentry, LoginName_OnSelect);

	pbox = Box_Create(Margin * 5, Margin * 5 + LogoHeight + TextHeight * 3, 80, TextHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->bgcol = DefaultBG;
	pbox->fgcol = TabFG2;
	Box_SetText(pbox, _("Password"));
	Box_AddChild(login, pbox);
	data->passlabel = pbox;

	pbox = Edit2Box_Create(Margin * 5 + 80 , Margin * 5 + LogoHeight + TextHeight * 3 - 2, 216, 20, BOX_VISIBLE, E2_HORIZ | E2_PASSWORD);
	pbox->bgcol = RGB(255, 255, 255);
	pbox->fgcol = RGB(0, 0, 0);
	Box_AddChild(login, pbox);
	data->passentry = pbox;
	Edit2Box_SetOnEnter(data->passentry, LoginPass_OnEnter);

	pbox = Box_Create(Margin * 5, Margin * 7 + LogoHeight + TextHeight * 4, 80, TextHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	pbox->bgcol = DefaultBG;
	pbox->fgcol = TabFG2;
	Box_SetText(pbox, _("Language"));
	Box_AddChild(login, pbox);
	data->languagelabel = pbox;

	pbox = ComboBox_Create(Margin * 5 + 80 , Margin * 7 + LogoHeight + TextHeight * 4 - 2, 216, 20, BOX_VISIBLE);
	ComboBox_AddEntry(pbox, "English");
	ComboBox_AddEntry(pbox, "Magyar");
	ComboBox_AddEntry(pbox, "Bork! Bork! Bork!");
	ComboBox_SetOnSelection(pbox, LoginLanguageCombo_OnSelection);
	{
		char *langcode = i18n_GetCurrentLangCode();

		if (langcode && stricmp(langcode, "hu") == 0)
		{
			ComboBox_SetSelection(pbox, "Magyar");
		}
		else if (langcode && stricmp(langcode, "se-bo") == 0)
		{
			ComboBox_SetSelection(pbox, "Bork! Bork! Bork!");
		}
		else
		{
			ComboBox_SetSelection(pbox, "English");
		}
	}
	Box_AddChild(login, pbox);
	data->languagecombo = pbox;

	pbox = CheckBox_Create(Margin * 5 + 80 , Margin * 9 + LogoHeight + TextHeight * 5, BOX_VISIBLE);
	Box_AddChild(login, pbox);
	CheckBox_SetOnHit(pbox, LoginCheck_OnHit);
	data->remembercheck = pbox;

	pbox = CheckBoxLinkedText_Create(Margin * 5 + 104 , Margin * 9 + LogoHeight + TextHeight * 5, 100, TextHeight, BOX_VISIBLE | BOX_TRANSPARENT, data->remembercheck);
	pbox->fgcol = TabFG2;
	Box_SetText(pbox, _("Remember me"));
	Box_AddChild(login, pbox);
	data->rememberlabel = pbox;

	pbox = StdButton_Create(Margin * 5 + 216 , Margin * 13 + LogoHeight + TextHeight * 6, 80, _("Log In"), 0);
	Box_AddChild(login, pbox);
	Button2_SetOnButtonHit(pbox, Login_OnLogin);
	data->loginbutton = pbox;

	pbox = Text_Create(20, Margin * 13 + LogoHeight + TextHeight * 6, 140, 20, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	Text_SetText(pbox, _("^lForgot your Password?^l"));
	Text_SetLinkColor(pbox, CR_LtOrange);
	Box_AddChild(login, pbox);
	Text_SetLinkCallback(pbox, 1, Util_OpenURL2, strdup(_("http://www.chesspark.com/forgot/")));

	data->nameentry->nextfocus = data->passentry;
	data->passentry->nextfocus = data->remembercheck;
	data->remembercheck->nextfocus = data->loginbutton;
	data->loginbutton->nextfocus = data->nameentry;

	data->nameentry->prevfocus = data->remembercheck;
	data->passentry->prevfocus = data->nameentry;
	data->remembercheck->prevfocus = data->passentry;
	data->loginbutton->prevfocus = data->remembercheck;

	login->titlebar = TitleBarCloseOnly_Add(login, NULL, Login_OnClose);
	login->OnActive = TitleBarRoot_OnActive;
	login->OnInactive = TitleBarRoot_OnInactive;

	pbox = Box_Create((360 - 50) / 2, data->noaccount->y + data->noaccount->h + Margin, 50, 10, BOX_TRANSPARENT);
	pbox->img = ImageMgr_GetSubAnim("loadinganim", "loading.png", 17, 50, 10);
	Box_AddChild(login, pbox);
	data->connectanimation = pbox;

	pbox = Box_Create(Margin, data->connectanimation->y + data->connectanimation->h + Margin, 360 - Margin * 2, 16, BOX_TRANSPARENT | BOX_CENTERTEXT);
	pbox->fgcol = RGB(192, 192, 192);
	Box_AddChild(login, pbox);
	data->connectmessage = pbox;

	pbox = StdButton_Create((360 - 80) / 2, data->connectmessage->y + data->connectmessage->h + Margin, 80, _("Cancel"), 0);
	pbox->flags &= ~BOX_VISIBLE;
	Button2_SetOnButtonHit(pbox, Login_OnCancel);
	Box_AddChild(login, pbox);
	data->cancelbutton = pbox;

/*
	pbox = Text_Create(0, login->h - 14, login->w, 14, BOX_VISIBLE | BOX_TRANSPARENT, TX_RIGHT);
	{
		char txt[512];

		sprintf(txt, "^3Chesspark %s build %s", CHESSPARK_VERSION, CHESSPARK_BUILD);
		Text_SetText(pbox, txt);
	}
	Box_AddChild(login, pbox);
*/

	Box_CreateWndCustom(login, _("Chesspark"), NULL);

	login->boxdata = data;

	CheckBox_SetChecked(data->remembercheck, 0);

	if (GetRegInt("autologin"))
	{
		CheckBox_SetChecked(data->remembercheck, 1);
		data->remembermechecked = 1;
		ComboBox_SetSelection(data->nameentry, GetRegString("autologin_username", buffer, 2048));
		Edit2Box_SetText(data->passentry, GetRegString("autologin_password", buffer, 2048));
	}

	if (GetRegString("lastlogin1", buffer, 2048))
	{
		ComboBox_AddEntry(data->nameentry, buffer);
	}

	if (GetRegString("lastlogin2", buffer, 2048))
	{
		ComboBox_AddEntry(data->nameentry, buffer);
	}

	if (GetRegString("lastlogin3", buffer, 2048))
	{
		ComboBox_AddEntry(data->nameentry, buffer);
	}

	if (GetRegString("lastlogin4", buffer, 2048))
	{
		ComboBox_AddEntry(data->nameentry, buffer);
	}

	if (GetRegString("lastlogin5", buffer, 2048))
	{
		ComboBox_AddEntry(data->nameentry, buffer);
	}

	Box_SetFocus(data->nameentry);

	BringWindowToTop(login->hwnd);

	return login;
}


void Login_SetUsername(struct Box_s *pbox, char *username)
{
	struct Logindata_s *data = pbox->boxdata;
	ComboBox_SetSelection(data->nameentry, username);
}


void Login_SetPassword(struct Box_s *pbox, char *password)
{
	struct Logindata_s *data = pbox->boxdata;
	Edit2Box_SetText(data->passentry, password);
}


void Login_SetLoginState(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct Logindata_s *data = pbox->boxdata;
	/*
	ShowWindow(data->nameentry_wnd, SW_SHOW);
	ShowWindow(data->passentry_wnd, SW_SHOW);
	*/
	data->nameentry->flags        |= BOX_VISIBLE;
	data->passentry->flags        |= BOX_VISIBLE;
	data->namelabel->flags        |= BOX_VISIBLE;
	data->passlabel->flags        |= BOX_VISIBLE;
	data->remembercheck->flags    |= BOX_VISIBLE;
	data->rememberlabel->flags    |= BOX_VISIBLE;
	data->loginbutton->flags      |= BOX_VISIBLE;
	data->connectanimation->flags &= ~BOX_VISIBLE;
	data->connectmessage->flags   &= ~BOX_VISIBLE;
	data->cancelbutton->flags     &= ~BOX_VISIBLE;
	Button2_SetOnButtonHit(data->loginbutton, Login_OnLogin);

	Box_MoveWndCustom(dialog, dialog->x + (dialog->w - 360) / 2, dialog->y + (dialog->h - 420) / 2, 360, 420);

	Box_Repaint(pbox);
}


void Login_SetConnectingState(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct Logindata_s *data = pbox->boxdata;
	/*
	ShowWindow(data->nameentry_wnd, SW_HIDE);
	ShowWindow(data->passentry_wnd, SW_HIDE);
	*/
	data->nameentry->flags        &= ~BOX_VISIBLE;
	data->passentry->flags        &= ~BOX_VISIBLE;
	data->namelabel->flags        &= ~BOX_VISIBLE;
	data->passlabel->flags        &= ~BOX_VISIBLE;
	data->remembercheck->flags    &= ~BOX_VISIBLE;
	data->rememberlabel->flags    &= ~BOX_VISIBLE;
	data->loginbutton->flags      &= ~BOX_VISIBLE;
	data->languagelabel->flags    &= ~BOX_VISIBLE;
	data->languagecombo->flags    &= ~BOX_VISIBLE;
	data->connectanimation->flags |= BOX_VISIBLE;
	data->connectmessage->flags   |= BOX_VISIBLE;
	data->cancelbutton->flags     |= BOX_VISIBLE;
	Button2_SetOnButtonHit(data->loginbutton, NULL);
	Button2_SetOnButtonHit(data->cancelbutton, Login_OnCancel);

	Box_SetText(data->connectmessage, _("Connecting to Chesspark..."));

	Box_MoveWndCustom(dialog, dialog->x + (dialog->w - 360) / 2, dialog->y + (dialog->h - 380) / 2, 360, 380);

	Box_Repaint(pbox);
}


void Login_SetErrorState(struct Box_s *pbox, char *error1, char *error2)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct Logindata_s *data = pbox->boxdata;
	/*
	ShowWindow(data->nameentry_wnd, SW_SHOW);
	ShowWindow(data->passentry_wnd, SW_SHOW);
	*/
	data->nameentry->flags        |= BOX_VISIBLE;
	data->passentry->flags        |= BOX_VISIBLE;
	data->namelabel->flags        |= BOX_VISIBLE;
	data->passlabel->flags        |= BOX_VISIBLE;
	data->remembercheck->flags    |= BOX_VISIBLE;
	data->rememberlabel->flags    |= BOX_VISIBLE;
	data->loginbutton->flags      |= BOX_VISIBLE;
	data->errorbox->flags         |= BOX_VISIBLE;
	data->languagelabel->flags    |= BOX_VISIBLE;
	data->languagecombo->flags    |= BOX_VISIBLE;
	data->connectanimation->flags &= ~BOX_VISIBLE;
	data->connectmessage->flags   &= ~BOX_VISIBLE;
	data->cancelbutton->flags     &= ~BOX_VISIBLE;
	Button2_SetOnButtonHit(data->loginbutton, Login_OnLogin);
	Button2_SetOnButtonHit(data->cancelbutton, Login_OnCancel);
	Box_SetText(data->errorbox->child, error1);
	Box_SetText(data->errorbox->child->sibling, error2);

	Box_MoveWndCustom(dialog, dialog->x + (dialog->w - 360) / 2, dialog->y + (dialog->h - 420) / 2, 360, 420);

	Box_Repaint(pbox);
}
