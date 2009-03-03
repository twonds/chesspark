#include "box.h"

#include "autosize.h"

#include "titlebar.h"
#include "text.h"
#include "button2.h"
#include "constants.h"
#include "ctrl.h"
#include "edit2.h"
#include "combobox.h"
#include "checkbox.h"
#include "i18n.h"
#include "stdbutton.h"

struct problemreportdata_s
{
	struct Box_s *sizeablecontent;
	struct Box_s *typecombo;
	struct Box_s *probedit;
	struct Box_s *logcheck;
	struct Box_s *cheatbox;
	struct Box_s *cheatnameedit;
	struct Box_s *cheatgameedit;
};

void ProbReportTypeCombo_OnSelection(struct Box_s *combobox, char *name)
{
	struct Box_s *dialog = Box_GetRoot(combobox);
	struct problemreportdata_s *data = dialog->boxdata;

	if (name && stricmp(name, "Technical Issues") == 0)
	{
		CheckBox_SetChecked(data->logcheck, 1);
	}
	else
	{
		CheckBox_SetChecked(data->logcheck, 0);
	}

	if (name && stricmp(name, "Cheating") == 0)
	{
		data->cheatbox->flags |= BOX_VISIBLE;
	}
	else
	{
		data->cheatbox->flags &= ~BOX_VISIBLE;
	}

	AutoSize_Fit(data->sizeablecontent);
	AutoSize_Fill(data->sizeablecontent);

	Box_MoveWndCustom2(dialog, dialog->x, dialog->y, data->sizeablecontent->w + 20, data->sizeablecontent->h + 50);
	Box_Repaint(dialog);
}

void ProblemReport_OnSend(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct problemreportdata_s *data = dialog->boxdata;
	char *type, *prob;
	int log;

	type = ComboBox_GetSelectionName(data->typecombo);
	prob = Edit2Box_GetText(data->probedit);
	log = CheckBox_GetChecked(data->logcheck);

	if (!prob || strlen(prob) == 0)
	{
		Edit2Box_SetAltText(data->probedit, _("You must include a description!"));
		Box_SetFocus(data->typecombo);
		Box_Repaint(data->probedit);
		return;
	}

	if (type && stricmp(type, "cheating") == 0)
	{
		char *cheatname = Edit2Box_GetText(data->cheatnameedit);
		char *cheatgame = Edit2Box_GetText(data->cheatgameedit);
		char *oldprob = prob;
		char *text = "cheater username: %s\ngame: %s\n%s";

		if (!cheatname || strlen(cheatname) == 0)
		{
			Edit2Box_SetAltText(data->cheatnameedit, _("You must include the cheater's name!"));
			Box_SetFocus(data->typecombo);
			Box_Repaint(data->cheatnameedit);
			return;
		}

		prob = malloc(strlen(prob) + strlen(cheatname) + (cheatgame ? strlen(cheatgame) : 0) + strlen(text) + 1);

		sprintf(prob, text, cheatname, cheatgame, oldprob);
	}

	Ctrl_PostProblem(type, prob, log);

	Box_Destroy(Box_GetRoot(pbox));
}

void ProblemReport_OnClose(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}

void ProblemReport_Create(struct Box_s *parent)
{
	struct problemreportdata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *pbox, *vertsize;

	memset(data, 0, sizeof(*data));

	dialog = Box_Create(0, 0, 440, 270, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;

	dialog->boxdata = data;

	dialog->titlebar = TitleBarCloseOnly_Add(dialog, _("Submit Report"), ProblemReport_OnClose);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	vertsize = AutoSize_Create(10, 30, 0, 0, 0, 0, AUTOSIZE_VERT);
	Box_AddChild(dialog, vertsize);
	data->sizeablecontent = vertsize;
	{
		struct Box_s *horizsize;

		horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
		Box_AddChild(vertsize, horizsize);
		{
			pbox = Text_Create(5, 2, 0, 20, BOX_VISIBLE | BOX_TRANSPARENT, TX_STRETCHHORIZ);
			pbox->fgcol = RGB(222, 222, 222);
			Text_SetText(pbox, "Problem type:");
			Box_AddChild(horizsize, pbox);

			AutoSize_AddSpacer(horizsize, 5);

			pbox = ComboBox_Create(0, 0, 120, 20, BOX_VISIBLE | BOX_BORDER);
			pbox->bgcol = RGB(255, 255, 255);
			ComboBox_AddEntry2(pbox, "Technical Issues", _("Technical Issues"));
			ComboBox_AddEntry2(pbox, "Cheating", _("Cheating"));
			ComboBox_AddEntry2(pbox, "Harassment", _("Harassment"));
			ComboBox_AddEntry2(pbox, "Other", _("Other"));
			ComboBox_SetSelection(pbox, "Technical Issues");
			ComboBox_SetOnSelection(pbox, ProbReportTypeCombo_OnSelection);
			Box_AddChild(horizsize, pbox);
			data->typecombo = pbox;

			AutoSize_AddSpacer(horizsize, 5);

			pbox = CheckBox_Create(0, 3, BOX_VISIBLE);
			CheckBox_SetChecked(pbox, 1);
			Box_AddChild(horizsize, pbox);
			data->logcheck = pbox;

			AutoSize_AddSpacer(horizsize, 5);

			pbox = CheckBoxLinkedText_Create(0, 3, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->logcheck);
			pbox->fgcol = RGB(222, 222, 222);
			Box_SetText(pbox, "Include debug log");
			Box_AddChild(horizsize, pbox);
		}

		AutoSize_AddSpacer(vertsize, 5);

		horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
		Box_AddChild(vertsize, horizsize);
		{
			pbox = Edit2Box_Create(0, 0, 190, 20, BOX_VISIBLE | BOX_BORDER, E2_HORIZ);
			pbox->bgcol = RGB(255, 255, 255);
			Edit2Box_SetAltText(pbox, _("Enter cheater's username here."));
			Box_AddChild(horizsize, pbox);
			data->cheatnameedit = pbox;

			AutoSize_AddSpacer(horizsize, 5);

			pbox = Edit2Box_Create(0, 0, 190, 20, BOX_VISIBLE | BOX_BORDER, E2_HORIZ);
			pbox->bgcol = RGB(255, 255, 255);
			Edit2Box_SetAltText(pbox, _("Enter game number here."));
			Box_AddChild(horizsize, pbox);
			data->cheatgameedit = pbox;
		}
		data->cheatbox = horizsize;
		horizsize->flags &= ~BOX_VISIBLE;

		AutoSize_AddSpacer(vertsize, 5);

		pbox = Text_Create(0, 0, 0, 20, BOX_TRANSPARENT, TX_STRETCHVERT);
		pbox->fgcol = RGB(222, 222, 222);
		Text_SetText(pbox, "Problem:");
		Box_AddChild(vertsize, pbox);

		pbox = Edit2Box_Create(0, 0, 300, 200, BOX_VISIBLE | BOX_BORDER, 0);
		pbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		pbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
		pbox->bgcol = RGB(255, 255, 255);
		Edit2Box_SetAltText(pbox, _("Please type in a description of your problem here."));
		Box_AddChild(vertsize, pbox);
		data->probedit = pbox;

		AutoSize_AddSpacer(vertsize, 5);

		horizsize = AutoSize_Create(0, 0, 0, 0, 0, 0, AUTOSIZE_HORIZ);
		horizsize->OnSizeWidth = Box_OnSizeWidth_Stretch;
		Box_AddChild(vertsize, horizsize);
		{
			struct Box_s *horizsize2;
			horizsize2 = AutoSizeSpace_Create(0, 0, 0, 0, 0, 0, 10, AUTOSIZE_HORIZ);
			horizsize2->OnSizeWidth = Box_OnSizeWidth_StickRight;
			Box_AddChild(horizsize, horizsize2);
			{	
				pbox = StdButton_Create(0, 0, 90, _("Send"), 0);
				Button2_SetOnButtonHit(pbox, ProblemReport_OnSend);
				Box_AddChild(horizsize2, pbox);

				pbox = StdButton_Create(0, 0, 90, _("Cancel"), 0);
				Button2_SetOnButtonHit(pbox, ProblemReport_OnClose);
				Box_AddChild(horizsize2, pbox);
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

		windowrect.left = parent->x;
		windowrect.right = windowrect.left + parent->w - 1;
		windowrect.top = parent->y;
		windowrect.bottom = windowrect.top + parent->h - 1;

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

	Box_CreateWndCustom(dialog, "Submit Report", parent->hwnd);

	BringWindowToTop(dialog->hwnd);
}