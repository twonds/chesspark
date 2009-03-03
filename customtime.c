#include <stdio.h>
#include <stdlib.h>

#include "box.h"
#include "button.h"
#include "edit.h"
#include "text.h"
#include "titledrag.h"

#include "constants.h"

#include "button2.h"
#include "autodialog.h"
#include "checkbox.h"
#include "combobox.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "imagemgr.h"
#include "info.h"
#include "link.h"
#include "model.h"
#include "stdbutton.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

extern HFONT tahoma10_f;

struct customtimedata_s
{
	struct Box_s *variantcombo;
	struct Box_s *parent;
	struct Box_s *whitecontrols;
	struct Box_s *blackcontrols;
	struct Box_s *delayincedit[2];
	struct Box_s *movesedit[6];
	struct Box_s *minutesedit[6];
	struct Box_s *movescombo[6];
	struct Box_s *withinafter[6];
	struct Box_s *delayincrementtext[2];
	struct Box_s *whitecorrcontrols;
	struct Box_s *corrdaysedit;
	struct Box_s *corrmovesedit;
	struct Box_s *blackcontrolsborder;
	struct Box_s *useincrementcheck;
	struct Box_s *sametimecontrolscheck;
	struct Box_s *saveedit;
	struct Box_s *savebutton;
	struct Box_s *saveresult;
	int useincrement;
	int sametimecontrols;
	int correspondence;
	void (*onSetCustomTimeControl)(struct Box_s *parent,
		struct timecontrol_s *whitetimecontrol,
		struct timecontrol_s *blacktimecontrol,
		char *name);
	void (*refreshCallback)(struct Box_s *);
	void (*destroyCallback)(struct Box_s *);
};

void CustomTime_OnIncrementCheck(struct Box_s *pbox, int state);
void CustomTime_OnSameTimeControlsCheck(struct Box_s *pbox, int state);

void CustomTime_OnCancel(struct Box_s *pbox)
{
	struct customtimedata_s *data = pbox->parent->boxdata;

	if (data->onSetCustomTimeControl)
	{
		data->onSetCustomTimeControl(data->parent, NULL, NULL, NULL);
	}
	Box_Destroy(Box_GetRoot(pbox));
}

void CustomTime_ParseTimeControls(struct Box_s *dialog, struct timecontrol_s **whitetc, struct timecontrol_s **blacktc)
{
	struct customtimedata_s *data = dialog->boxdata;
	char *movescombosel0, *movescombosel1, *movescombosel2;
	char *txt;
	int numcontrols, i, *p, num;

	if (data->correspondence)
	{
		*whitetc = malloc(sizeof(**whitetc));
		memset(*whitetc, 0, sizeof(**whitetc));

		(*whitetc)->correspondence = 1;
		(*whitetc)->delayinc = 0;
		(*whitetc)->controlarray = malloc(3 * sizeof(int));

		p = (*whitetc)->controlarray;
		*p++ = 1;
		
		txt = Edit2Box_GetText(data->corrmovesedit);
		sscanf(txt, "%d", &num);
		*p++ = num;

		txt = Edit2Box_GetText(data->corrdaysedit);
		sscanf(txt, "%d", &num);
		*p++ = num * 60 * 60 * 24;
	}
	else
	{
		*whitetc = malloc(sizeof(**whitetc));
		memset(*whitetc, 0, sizeof(**whitetc));

		movescombosel0 = ComboBox_GetSelectionName(data->movescombo[0]);
		movescombosel1 = ComboBox_GetSelectionName(data->movescombo[1]);
		movescombosel2 = ComboBox_GetSelectionName(data->movescombo[2]);

		if ((txt = Edit2Box_GetText(data->delayincedit[0])) && strlen(txt) > 0)
		{
			sscanf(txt, "%d", &num);
			if (!data->useincrement)
			{
				num = -num;
			}
			(*whitetc)->delayinc = num;
		}
		else
		{
			(*whitetc)->delayinc = 0;
		}

		if (movescombosel0 && strcmp(movescombosel0, _("Game Ends")) == 0)
		{
			numcontrols = 1;
		}
		else if (movescombosel1 && strcmp(movescombosel1, _("Game Ends")) == 0)
		{
			numcontrols = 2;
		}
		else if (movescombosel2 && strcmp(movescombosel2, _("Game Ends")) == 0)
		{
			numcontrols = 3;
		}

		(*whitetc)->controlarray = malloc((1 + 2 * numcontrols) * sizeof(int));
		p = (*whitetc)->controlarray;
		*p++ = numcontrols;

		for (i = 0; i < numcontrols; i++)
		{
			txt = Edit2Box_GetText(data->movesedit[i]);
			if (i != numcontrols - 1 && txt && strlen(txt) > 0)
			{
				sscanf(txt, "%d", &num);
				*p++ = num;
			}
			else
			{
				*p++ = -1;
			}

			txt = Edit2Box_GetText(data->minutesedit[i]);
			if (txt && strlen(txt) > 0)
			{
				sscanf(txt, "%d", &num);
				*p++ = num * 60;
			}
			else
			{
				*p++ = -1;
			}
		}
	}

	if (data->sametimecontrols || data->correspondence)
	{
		*blacktc = NULL;
	}
	else
	{
		*blacktc = malloc(sizeof(**blacktc));
		memset(*blacktc, 0, sizeof(**blacktc));

		movescombosel0 = ComboBox_GetSelectionName(data->movescombo[3]);
		movescombosel1 = ComboBox_GetSelectionName(data->movescombo[4]);
		movescombosel2 = ComboBox_GetSelectionName(data->movescombo[5]);

		if ((txt = Edit2Box_GetText(data->delayincedit[1])) && strlen(txt) > 0)
		{
			sscanf(txt, "%d", &num);
			if (!data->useincrement)
			{
				num = -num;
			}
			(*blacktc)->delayinc = num;
		}
		else
		{
			(*blacktc)->delayinc = 0;
		}

		if (movescombosel0 && strcmp(movescombosel0, _("Game Ends")) == 0)
		{
			numcontrols = 1;
		}
		else if (movescombosel1 && strcmp(movescombosel1, _("Game Ends")) == 0)
		{
			numcontrols = 2;
		}
		else if (movescombosel2 && strcmp(movescombosel2, _("Game Ends")) == 0)
		{
			numcontrols = 3;
		}

		(*blacktc)->controlarray = malloc((1 + 2 * numcontrols) * sizeof(int));
		p = (*blacktc)->controlarray;
		*p++ = numcontrols;

		for (i = 0; i < numcontrols; i++)
		{
			txt = Edit2Box_GetText(data->movesedit[i + 3]);
			if (i != numcontrols - 1 && txt && strlen(txt) > 0)
			{
				sscanf(txt, "%d", &num);
				*p++ = num;
			}
			else
			{
				*p++ = -1;
			}

			txt = Edit2Box_GetText(data->minutesedit[i + 3]);
			if (txt && strlen(txt) > 0)
			{
				sscanf(txt, "%d", &num);
				*p++ = num * 60;
			}
			else
			{
				*p++ = -1;
			}
		}
	}
}


void CustomTimeConflict2_OnYes(struct Box_s *pbox, struct Box_s *dialog)
{
	struct Box_s *popup = Box_GetRoot(pbox);
	struct customtimedata_s *data = dialog->boxdata;
	struct timecontrol_s *whitetc, *blacktc;
	char *name = Edit2Box_GetText(data->saveedit);

	Box_Destroy(popup);

	CustomTime_ParseTimeControls(dialog, &whitetc, &blacktc);

	if (data->onSetCustomTimeControl)
	{
                data->onSetCustomTimeControl(data->parent, whitetc, blacktc, name);
	}

	Box_Destroy(dialog);
}

void CustomTime_OnOK(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct customtimedata_s *data = dialog->boxdata;
	struct timecontrol_s *whitetc, *blacktc;
	char *name;
	int i;
	int abort = 0;

	for (i = 0; i < 2; i++)
	{
		char *parse;
		int inc = 0;
		parse = Edit2Box_GetText(data->delayincedit[i]);

		if (parse)
		{
			sscanf(parse, "%d", &inc);
		}

		if (inc < 0)
		{
			Edit2Box_SetText(data->delayincedit[i], "0");
		}

		if (inc > 60)
		{
			Edit2Box_SetText(data->delayincedit[i], "60");
			Edit2Box_ScrollToStart(data->delayincedit[i]);
			abort |= 1;
		}
	}

	for (i = 0; i < (data->sametimecontrols ? 3 : 6); i++)
	{
		char *parse;
		int time = 0;
		parse = Edit2Box_GetText(data->minutesedit[i]);

		if (parse)
		{
			sscanf(parse, "%d", &time);
		}

		if (time < 1 && !Edit2Box_GetDisabled(data->minutesedit[i]))
		{
			Edit2Box_SetText(data->minutesedit[i], "1");
			abort |= 4;
		}

		if (time > 480)
		{
			Edit2Box_SetText(data->minutesedit[i], "480");
			Edit2Box_ScrollToStart(data->minutesedit[i]);
			abort |= 2;
		}
	}


	if (abort)
	{
		if (abort == 1)
		{
			AutoDialog_Create(dialog, 300, "Increment too large!", _("The increment you have chosen is too large.  It has automatically been reset to 60."), NULL, NULL, NULL, NULL, NULL);
		}
		else if (abort == 2)
		{
			AutoDialog_Create(dialog, 300, "Time control too large!", _("The time control you have chosen is too large.  It has automatically been reset to 480."), NULL, NULL, NULL, NULL, NULL);
		}
		else if (abort == 3)
		{
			AutoDialog_Create(dialog, 300, "Time control and increment too large!", _("The time control and increment you have chosen are too large.  They have automatically been reset to 480 and 60."), NULL, NULL, NULL, NULL, NULL);
		}
		else if (abort == 4)
		{
			AutoDialog_Create(dialog, 300, "Time control too small!", _("The time control you have chosen is too small.  It has automatically been reset to 1."), NULL, NULL, NULL, NULL, NULL);
		}
		else if (abort == 5)
		{
			AutoDialog_Create(dialog, 300, "Time control too small, increment too large!", _("The time control and increment you have chosen are out of range.  They have automatically been reset to 1 and 60."), NULL, NULL, NULL, NULL, NULL);
		}
		else
		{
			AutoDialog_Create(dialog, 300, "Time out of range!", _("The time control and/or increment you have chosen are out of range.  They have automatically been reset."), NULL, NULL, NULL, NULL, NULL);
		}

		return;
	}


	name = Edit2Box_GetText(data->saveedit);

	if (!name && !data->savebutton)
	{
		Box_SetText(data->saveresult, _("Need a name!"));
		Box_Repaint(data->saveresult);
		return;
	}

	if (!data->savebutton)
	{
		if (Model_CustomTimeControlExists(name))
		{
			AutoDialog_Create(dialog, 500, _("Confirm Replace"), 
			  _("You already have a custom time control with this name.  Do you want to replace it?"), 
			  _("No"), _("Yes"), NULL, CustomTimeConflict2_OnYes, dialog);
			return;
		}
	}

	CustomTime_ParseTimeControls(dialog, &whitetc, &blacktc);

	if (data->onSetCustomTimeControl)
	{
                data->onSetCustomTimeControl(data->parent, whitetc, blacktc, name);
	}

	Box_Destroy(dialog);
}


void CustomTimeVariantCombo_OnSelection(struct Box_s *combobox, char *name)
{
	struct Box_s *dialog = Box_GetRoot(combobox);
	struct customtimedata_s *data = dialog->boxdata;

	if (strcmp(name, _("Standard")) == 0)
	{
		data->correspondence = 0;
		data->whitecontrols->flags |= BOX_VISIBLE;
		data->whitecorrcontrols->flags &= ~BOX_VISIBLE;
		CustomTime_OnSameTimeControlsCheck(data->sametimecontrolscheck, data->sametimecontrols);
		CustomTime_OnIncrementCheck(data->useincrementcheck, data->useincrement);
	}
	else
	{
		data->correspondence = 1;
		data->whitecontrols->flags &= ~BOX_VISIBLE;
		data->blackcontrols->flags &= ~BOX_VISIBLE;
		data->blackcontrolsborder->flags |= BOX_VISIBLE;
		data->whitecorrcontrols->flags |= BOX_VISIBLE;
	}

}

void CustomTimeMovesCombo_OnSelection(struct Box_s *combobox, char *name)
{
	struct Box_s *dialog = Box_GetRoot(combobox);
	struct customtimedata_s *data = dialog->boxdata;

	if (strcmp(name, _("Game Ends")) == 0)
	{
		if (combobox == data->movescombo[0])
		{
			Edit2Box_SetDisabled   (data->movesedit[0], 1);
			ComboBox_SetSelection(data->movescombo[0], _("Game Ends"));
			Edit2Box_SetDisabled (data->minutesedit[0], 0);
			Box_SetText         (data->withinafter[0], _("after"));
			
			Edit2Box_SetDisabled   (data->movesedit[1], 1);
			ComboBox_SetSelection(data->movescombo[1], NULL);
			Edit2Box_SetDisabled (data->minutesedit[1], 1);
			Box_SetText         (data->withinafter[1], NULL);
			
			Edit2Box_SetDisabled   (data->movesedit[2], 1);
			ComboBox_SetSelection(data->movescombo[2], NULL);
			Edit2Box_SetDisabled (data->minutesedit[2], 1);
			Box_SetText         (data->withinafter[2], NULL);
		}
		else if (combobox == data->movescombo[1])
		{
			Edit2Box_SetDisabled   (data->movesedit[0], 0);
			ComboBox_SetSelection(data->movescombo[0], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[0], 0);
			Box_SetText         (data->withinafter[0], _("within"));
			
			Edit2Box_SetDisabled   (data->movesedit[1], 1);
			ComboBox_SetSelection(data->movescombo[1], _("Game Ends"));
			Edit2Box_SetDisabled (data->minutesedit[1], 0);
			Box_SetText         (data->withinafter[1], _("after"));

			Edit2Box_SetDisabled   (data->movesedit[2], 1);
			ComboBox_SetSelection(data->movescombo[2], NULL);
			Edit2Box_SetDisabled (data->minutesedit[2], 1);
			Box_SetText         (data->withinafter[2], NULL);
		}
		else if (combobox == data->movescombo[2])
		{
			Edit2Box_SetDisabled   (data->movesedit[0], 0);
			ComboBox_SetSelection(data->movescombo[0], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[0], 0);
			Box_SetText         (data->withinafter[0], _("within"));

			Edit2Box_SetDisabled   (data->movesedit[1], 0);
			ComboBox_SetSelection(data->movescombo[1], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[1], 0);
			Box_SetText         (data->withinafter[1], _("within"));

			Edit2Box_SetDisabled   (data->movesedit[2], 1);
			ComboBox_SetSelection(data->movescombo[2], _("Game Ends"));
			Edit2Box_SetDisabled (data->minutesedit[2], 0);
			Box_SetText         (data->withinafter[2], _("after"));
		}
		else if (combobox == data->movescombo[3])
		{
			Edit2Box_SetDisabled   (data->movesedit[3], 1);
			ComboBox_SetSelection(data->movescombo[3], _("Game Ends"));
			Edit2Box_SetDisabled (data->minutesedit[3], 0);
			Box_SetText         (data->withinafter[3], _("after"));
			
			Edit2Box_SetDisabled   (data->movesedit[4], 1);
			ComboBox_SetSelection(data->movescombo[4], NULL);
			Edit2Box_SetDisabled (data->minutesedit[4], 1);
			Box_SetText         (data->withinafter[4], NULL);

			Edit2Box_SetDisabled   (data->movesedit[5], 1);
			ComboBox_SetSelection(data->movescombo[5], NULL);
			Edit2Box_SetDisabled (data->minutesedit[5], 1);
			Box_SetText         (data->withinafter[5], NULL);
		}
		else if (combobox == data->movescombo[4])
		{
			Edit2Box_SetDisabled   (data->movesedit[3], 0);
			ComboBox_SetSelection(data->movescombo[3], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[3], 0);
			Box_SetText         (data->withinafter[3], _("within"));
			
			Edit2Box_SetDisabled   (data->movesedit[4], 1);
			ComboBox_SetSelection(data->movescombo[4], _("Game Ends"));
			Edit2Box_SetDisabled (data->minutesedit[4], 0);
			Box_SetText         (data->withinafter[4], _("after"));

			Edit2Box_SetDisabled   (data->movesedit[5], 1);
			ComboBox_SetSelection(data->movescombo[5], NULL);
			Edit2Box_SetDisabled (data->minutesedit[5], 1);
			Box_SetText         (data->withinafter[5], NULL);
		}
		else if (combobox == data->movescombo[5])
		{
			Edit2Box_SetDisabled   (data->movesedit[3], 0);
			ComboBox_SetSelection(data->movescombo[3], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[3], 0);
			Box_SetText         (data->withinafter[3], _("within"));

			Edit2Box_SetDisabled   (data->movesedit[4], 0);
			ComboBox_SetSelection(data->movescombo[4], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[4], 0);
			Box_SetText         (data->withinafter[4], _("within"));

			Edit2Box_SetDisabled   (data->movesedit[5], 1);
			ComboBox_SetSelection(data->movescombo[5], _("Game Ends"));
			Edit2Box_SetDisabled (data->minutesedit[5], 0);
			Box_SetText         (data->withinafter[5], _("after"));
		}
	}
	else if (strcmp(name, _("Moves")) == 0)
	{
		if (combobox == data->movescombo[0])
		{
			Edit2Box_SetDisabled   (data->movesedit[0], 0);
			ComboBox_SetSelection(data->movescombo[0], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[0], 0);
			Box_SetText         (data->withinafter[0], _("within"));
			
			Edit2Box_SetDisabled   (data->movesedit[1], 1);
			ComboBox_SetSelection(data->movescombo[1], _("Game Ends"));
			Edit2Box_SetDisabled (data->minutesedit[1], 0);
			Box_SetText         (data->withinafter[1], _("after"));

			Edit2Box_SetDisabled   (data->movesedit[2], 1);
			ComboBox_SetSelection(data->movescombo[2], NULL);
			Edit2Box_SetDisabled (data->minutesedit[2], 1);
			Box_SetText         (data->withinafter[2], NULL);
		}
		else if (combobox == data->movescombo[1])
		{
			Edit2Box_SetDisabled   (data->movesedit[0], 0);
			ComboBox_SetSelection(data->movescombo[0], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[0], 0);
			Box_SetText         (data->withinafter[0], _("within"));

			Edit2Box_SetDisabled   (data->movesedit[1], 0);
			ComboBox_SetSelection(data->movescombo[1], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[1], 0);
			Box_SetText         (data->withinafter[1], _("within"));

			Edit2Box_SetDisabled   (data->movesedit[2], 1);
			ComboBox_SetSelection(data->movescombo[2], _("Game Ends"));
			Edit2Box_SetDisabled (data->minutesedit[2], 0);
			Box_SetText         (data->withinafter[2], _("after"));
		}
		else if (combobox == data->movescombo[3])
		{
			Edit2Box_SetDisabled   (data->movesedit[3], 0);
			ComboBox_SetSelection(data->movescombo[3], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[3], 0);
			Box_SetText         (data->withinafter[3], _("within"));
			
			Edit2Box_SetDisabled   (data->movesedit[4], 1);
			ComboBox_SetSelection(data->movescombo[4], _("Game Ends"));
			Edit2Box_SetDisabled (data->minutesedit[4], 0);
			Box_SetText         (data->withinafter[4], _("after"));

			Edit2Box_SetDisabled   (data->movesedit[5], 1);
			ComboBox_SetSelection(data->movescombo[5], NULL);
			Edit2Box_SetDisabled (data->minutesedit[5], 1);
			Box_SetText         (data->withinafter[5], NULL);
		}
		else if (combobox == data->movescombo[4])
		{
			Edit2Box_SetDisabled   (data->movesedit[3], 0);
			ComboBox_SetSelection(data->movescombo[3], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[3], 0);
			Box_SetText         (data->withinafter[3], _("within"));

			Edit2Box_SetDisabled   (data->movesedit[4], 0);
			ComboBox_SetSelection(data->movescombo[4], _("Moves"));
			Edit2Box_SetDisabled (data->minutesedit[4], 0);
			Box_SetText         (data->withinafter[4], _("within"));

			Edit2Box_SetDisabled   (data->movesedit[5], 1);
			ComboBox_SetSelection(data->movescombo[5], _("Game Ends"));
			Edit2Box_SetDisabled (data->minutesedit[5], 0);
			Box_SetText         (data->withinafter[5], _("after"));
		}
	}
	Box_Repaint(dialog);
}

void CustomTime_OnSameTimeControlsCheck(struct Box_s *pbox, int state)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct customtimedata_s *data = dialog->boxdata;

	data->sametimecontrols = state;

	if (!data->sametimecontrols)
	{
		data->blackcontrols->flags |= BOX_VISIBLE;
		data->blackcontrolsborder->flags &= ~BOX_VISIBLE;
		Box_Repaint(dialog);
	}
	else
	{
		data->blackcontrols->flags &= ~BOX_VISIBLE;
		data->blackcontrolsborder->flags |= BOX_VISIBLE;
		Box_Repaint(dialog);
	}
}

void CustomTime_OnIncrementCheck(struct Box_s *pbox, int state)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct customtimedata_s *data = dialog->boxdata;

	data->useincrement = state;

	if (!data->useincrement)
	{
		Box_SetText(data->delayincrementtext[0], _("Delay"));
		Box_SetText(data->delayincrementtext[1], _("Delay"));
		Box_Repaint(dialog);
	}
	else
	{
		Box_SetText(data->delayincrementtext[0], _("Increment"));
		Box_SetText(data->delayincrementtext[1], _("Increment"));
		Box_Repaint(dialog);
	}
}

void CustomTime_OnClearFields(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct customtimedata_s *data = dialog->boxdata;

	Edit2Box_SetText     (data->movesedit[0], NULL);
	Edit2Box_SetText     (data->minutesedit[0], NULL);
	Edit2Box_SetDisabled (data->movesedit[0], 1);
	ComboBox_SetSelection(data->movescombo[0], _("Game Ends"));
	Edit2Box_SetDisabled (data->minutesedit[0], 0);
	Box_SetText          (data->withinafter[0], _("after"));
			
	Edit2Box_SetText     (data->movesedit[1], NULL);
	Edit2Box_SetText     (data->minutesedit[1], NULL);
	Edit2Box_SetDisabled (data->movesedit[1], 1);
	ComboBox_SetSelection(data->movescombo[1], NULL);
	Edit2Box_SetDisabled (data->minutesedit[1], 1);
	Box_SetText          (data->withinafter[1], NULL);
			
	Edit2Box_SetText     (data->movesedit[2], NULL);
	Edit2Box_SetText     (data->minutesedit[2], NULL);
	Edit2Box_SetDisabled (data->movesedit[2], 1);
	ComboBox_SetSelection(data->movescombo[2], NULL);
	Edit2Box_SetDisabled (data->minutesedit[2], 1);
	Box_SetText          (data->withinafter[2], NULL);

	Edit2Box_SetText     (data->movesedit[3], NULL);
	Edit2Box_SetText     (data->minutesedit[3], NULL);
	Edit2Box_SetDisabled (data->movesedit[3], 1);
	ComboBox_SetSelection(data->movescombo[3], _("Game Ends"));
	Edit2Box_SetDisabled (data->minutesedit[3], 0);
	Box_SetText          (data->withinafter[3], _("after"));
			
	Edit2Box_SetText     (data->movesedit[4], NULL);
	Edit2Box_SetText     (data->minutesedit[4], NULL);
	Edit2Box_SetDisabled (data->movesedit[4], 1);
	ComboBox_SetSelection(data->movescombo[4], NULL);
	Edit2Box_SetDisabled (data->minutesedit[4], 1);
	Box_SetText          (data->withinafter[4], NULL);

	Edit2Box_SetText     (data->movesedit[5], NULL);
	Edit2Box_SetText     (data->minutesedit[5], NULL);
	Edit2Box_SetDisabled (data->movesedit[5], 1);
	ComboBox_SetSelection(data->movescombo[5], NULL);
	Edit2Box_SetDisabled (data->minutesedit[5], 1);
	Box_SetText          (data->withinafter[5], NULL);

	Edit2Box_SetText(data->delayincedit[0], NULL);
	Edit2Box_SetText(data->delayincedit[1], NULL);

	Box_SetText(data->delayincrementtext[0], _("Delay"));
	Box_SetText(data->delayincrementtext[1], _("Delay"));
	data->useincrement = 0;
	CheckBox_SetChecked(data->useincrementcheck, 0);

	data->blackcontrols->flags &= ~BOX_VISIBLE;
	data->blackcontrolsborder->flags |= BOX_VISIBLE;
	data->sametimecontrols = 1;
	CheckBox_SetChecked(data->sametimecontrolscheck, 1);

	Edit2Box_SetText(data->saveedit, NULL);
	if (data->savebutton)
	{
		Button2_SetDisabledState(data->savebutton, 1);
	}

	Box_Repaint(dialog);
}

void CustomTimeConflict_OnYes(struct Box_s *pbox, struct Box_s *dialog)
{
	struct Box_s *popup = Box_GetRoot(pbox);
	struct customtimedata_s *data = dialog->boxdata;
	struct timecontrol_s *whitetc, *blacktc;
	char *name = Edit2Box_GetText(data->saveedit);

	Box_Destroy(popup);

	CustomTime_ParseTimeControls(dialog, &whitetc, &blacktc);

	Model_SaveCustomTimeControl(name, whitetc, blacktc);

	Box_SetText(data->saveresult, _("Saved."));
	Box_Repaint(data->saveresult);

	Edit2Box_SetText(data->saveedit, NULL);
	Box_Repaint(data->saveedit);

	if (data->savebutton)
	{
		Button2_SetDisabledState(data->savebutton, 1);
		Box_Repaint(data->savebutton);
	}

	if (data->refreshCallback)
	{
		data->refreshCallback(data->parent);
	}
}

void CustomTime_OnSaveControl(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct customtimedata_s *data = dialog->boxdata;
	struct timecontrol_s *whitetc, *blacktc;
	char *name = Edit2Box_GetText(data->saveedit);
	int i;
	int abort = 0;

	for (i = 0; i < 2; i++)
	{
		char *parse;
		int inc = 0;
		parse = Edit2Box_GetText(data->delayincedit[i]);

		if (parse)
		{
			sscanf(parse, "%d", &inc);
		}

		if (inc < 0)
		{
			Edit2Box_SetText(data->delayincedit[i], "0");
		}

		if (inc > 60)
		{
			Edit2Box_SetText(data->delayincedit[i], "60");
			Edit2Box_ScrollToStart(data->delayincedit[i]);
			abort |= 1;
		}
	}

	for (i = 0; i < (data->sametimecontrols ? 3 : 6); i++)
	{
		char *parse;
		int time = 0;
		parse = Edit2Box_GetText(data->minutesedit[i]);

		if (parse)
		{
			sscanf(parse, "%d", &time);
		}

		if (time < 1 && !Edit2Box_GetDisabled(data->minutesedit[i]))
		{
			Edit2Box_SetText(data->minutesedit[i], "1");
			abort |= 4;
		}

		if (time > 480)
		{
			Edit2Box_SetText(data->minutesedit[i], "480");
			Edit2Box_ScrollToStart(data->minutesedit[i]);
			abort |= 2;
		}
	}


	if (abort)
	{
		if (abort == 1)
		{
			AutoDialog_Create(dialog, 300, "Increment too large!", _("The increment you have chosen is too large.  It has automatically been reset to 60."), NULL, NULL, NULL, NULL, NULL);
		}
		else if (abort == 2)
		{
			AutoDialog_Create(dialog, 300, "Time control too large!", _("The time control you have chosen is too large.  It has automatically been reset to 480."), NULL, NULL, NULL, NULL, NULL);
		}
		else if (abort == 3)
		{
			AutoDialog_Create(dialog, 300, "Time control and increment too large!", _("The time control and increment you have chosen are too large.  They have automatically been reset to 480 and 60."), NULL, NULL, NULL, NULL, NULL);
		}
		else if (abort == 4)
		{
			AutoDialog_Create(dialog, 300, "Time control too small!", _("The time control you have chosen is too small.  It has automatically been reset to 1."), NULL, NULL, NULL, NULL, NULL);
		}
		else if (abort == 5)
		{
			AutoDialog_Create(dialog, 300, "Time control too small, increment too large!", _("The time control and increment you have chosen are out of range.  They have automatically been reset to 1 and 60."), NULL, NULL, NULL, NULL, NULL);
		}
		else
		{
			AutoDialog_Create(dialog, 300, "Time out of range!", _("The time control and/or increment you have chosen are out of range.  They have automatically been reset."), NULL, NULL, NULL, NULL, NULL);
		}

		return;
	}

	if (!name || strlen(name) == 0)
	{
		return;
	}

	if (Model_CustomTimeControlExists(name))
	{
		AutoDialog_Create(dialog, 500, _("Confirm Replace"), 
		  _("You already have a custom time control with this name.  Do you want to replace it?"), 
		  _("No"), _("Yes"), NULL, CustomTimeConflict_OnYes, dialog);
		return;
	}

	CustomTime_ParseTimeControls(dialog, &whitetc, &blacktc);

	Model_SaveCustomTimeControl(name, whitetc, blacktc);

	Box_SetText(data->saveresult, _("Saved."));
	Box_Repaint(data->saveresult);

	Edit2Box_SetText(data->saveedit, NULL);
	Box_Repaint(data->saveedit);

	if (data->savebutton)
	{
		Button2_SetDisabledState(data->savebutton, 1);
		Box_Repaint(data->savebutton);
	}

	if (data->refreshCallback)
	{
		data->refreshCallback(data->parent);
	}
}

void CustomTimeSaveEdit_OnKey(struct Box_s *pbox, char *text)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct customtimedata_s *data = dialog->boxdata;

	Box_SetText(data->saveresult, NULL);
	Box_Repaint(data->saveresult);

	if (data->savebutton)
	{
		if (!text || strlen(text) == 0)
		{
			Button2_SetDisabledState(data->savebutton, 1);
		}
		else
		{
			Button2_SetDisabledState(data->savebutton, 0);
		}
		Box_Repaint(data->savebutton);
	}
}

void CustomTime_OnDestroy(struct Box_s *dialog)
{
	struct customtimedata_s *data = dialog->boxdata;

	if (data->parent && data->destroyCallback)
	{
		data->destroyCallback(data->parent);
	}
}

struct Box_s *CustomTime_Create(struct Box_s *parent, 
  void (*onSetCustomTimeControl)(struct Box_s *,
    struct timecontrol_s *, struct timecontrol_s *, char *name),
  char *name,
  struct timecontrol_s *whitetimecontrol,
  struct timecontrol_s *blacktimecontrol,
  void (*refreshCallback)(struct Box_s *),
  void (*destroyCallback)(struct Box_s *),
  int hassave)
{
	struct customtimedata_s *data = malloc(sizeof(*data));
	struct Box_s *dialog, *subbox;
	int i, j;
	int x, y;

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;

		windowrect.left = Box_GetRoot(parent)->x;
		windowrect.right = windowrect.left + Box_GetRoot(parent)->w - 1;
		windowrect.top = Box_GetRoot(parent)->y;
		windowrect.bottom = windowrect.top + Box_GetRoot(parent)->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - 800) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - 346) / 2;
	}

	memset(data, 0, sizeof(*data));
	data->onSetCustomTimeControl = onSetCustomTimeControl;
	data->refreshCallback = refreshCallback;
	data->destroyCallback = destroyCallback;
	data->parent = parent;

	dialog = Box_Create(x, y, 800, 346 + 60, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;

	subbox = Box_Create(20, 45, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Game Variant"));
	Box_AddChild(dialog, subbox);

	subbox = ComboBox_Create(30, 65, 120, 20, BOX_VISIBLE);
	ComboBox_AddEntry(subbox, _("Standard"));
	/*ComboBox_AddEntry(subbox, _("Correspondence"));*/
	ComboBox_SetSelection(subbox, _("Standard"));
	ComboBox_SetOnSelection(subbox, CustomTimeVariantCombo_OnSelection);
	Box_AddChild(dialog, subbox);
	data->variantcombo = subbox;

	subbox = Box_Create(20, 40 + 60, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("White"));
	Box_AddChild(dialog, subbox);

	subbox = Box_Create(20 + dialog->w / 2, 40 + 60, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Black"));
	Box_AddChild(dialog, subbox);

	subbox = Box_Create(410, 60 + 60, 370, 120, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->bgcol = DefaultBG;
	subbox->brcol = UserInfoFG2;
	Box_AddChild(dialog, subbox);
	data->blackcontrolsborder = subbox;

	{
		struct Box_s *subbox2;
		subbox2 = Box_Create(0, 0, subbox->w, 2, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeWidth = Box_OnSizeWidth_Stretch;
		subbox2->img = ImageMgr_GetSubImage("dotborder-top", "dotborder.png", 0, 0, 38, 2);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(0, subbox->h - 1, subbox->w, 2, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeWidth = Box_OnSizeWidth_Stretch;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		subbox2->img = ImageMgr_GetSubImage("dotborder-top", "dotborder.png", 0, 0, 38, 2);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(0, 1, 2, subbox->h - 1 , BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeHeight = Box_OnSizeHeight_Stretch;
		subbox2->img = ImageMgr_GetSubImage("dotborder-left", "dotborder.png", 0, 0, 2, 38);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 1, 0, 2, subbox->h, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_Stretch;
		subbox2->img = ImageMgr_GetSubImage("dotborder-left", "dotborder.png", 0, 0, 2, 38);
		Box_AddChild(subbox, subbox2);
	}

	{
		struct Box_s *parent;
		parent = Box_Create(0, 60, dialog->w, dialog->h, BOX_TRANSPARENT);
		Box_AddChild(dialog, parent);
		data->whitecorrcontrols = parent;

		subbox = Box_Create(30, 60, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->fgcol = UserInfoFG2;
		Box_SetText(subbox, _("Duration"));
		Box_AddChild(parent, subbox);

		subbox = Edit2Box_Create(30, 80, 30, 20, BOX_VISIBLE, 1);
		subbox->bgcol = RGB(255, 255, 255);
		Edit2Box_SetText(subbox, "10");
		Box_AddChild(parent, subbox);
		data->corrmovesedit = subbox;

		subbox = Box_Create(60, 80, 80, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = UserInfoFG2;
		Box_SetText(subbox, _("moves within"));
		Box_AddChild(parent, subbox);

		subbox = Edit2Box_Create(140, 80, 30, 20, BOX_VISIBLE, 1);
		subbox->bgcol = RGB(255, 255, 255);
		Edit2Box_SetText(subbox, "30");
		Box_AddChild(parent, subbox);
		data->corrdaysedit = subbox;

		subbox = Box_Create(175, 80, 30, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = UserInfoFG2;
		Box_SetText(subbox, _("days."));
		Box_AddChild(parent, subbox);
	}

	for (i = 0; i < 2; i++)
	{
		struct Box_s *parent;
		if (i == 0)
		{
			parent = Box_Create(0, 60, dialog->w, dialog->h, BOX_TRANSPARENT | BOX_VISIBLE);
			Box_AddChild(dialog, parent);
			data->whitecontrols = parent;
		}
		else
		{
			parent = Box_Create(0, 60, dialog->w, dialog->h, BOX_TRANSPARENT);
			Box_AddChild(dialog, parent);
			data->blackcontrols = parent;
		}

		subbox = Box_Create(30 + i * dialog->w / 2, 60, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->fgcol = UserInfoFG2;
		Box_SetText(subbox, _("Duration"));
		Box_AddChild(parent, subbox);

		subbox = Box_Create(295 + i * dialog->w / 2, 60, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->fgcol = UserInfoFG2;
		Box_SetText(subbox, _("Delay"));
		Box_AddChild(parent, subbox);
		data->delayincrementtext[i] = subbox;

		for (j = 0; j < 3; j++)
		{
			char txt[80];

			subbox = Box_Create(20 + i * dialog->w / 2, 80 + j * 25, 10, 16, BOX_VISIBLE | BOX_TRANSPARENT);
			subbox->fgcol = UserInfoFG2;
			sprintf(txt, "%d", j + 1);
			Box_SetText(subbox, txt);
			Box_AddChild(parent, subbox);

			subbox = Edit2Box_Create(30 + i * dialog->w / 2, 80 + j * 25, 30, 20, BOX_VISIBLE, 1);
			subbox->bgcol = RGB(255, 255, 255);
			Box_AddChild(parent, subbox);
			Edit2Box_SetDisabled(subbox, 1);
			data->movesedit[i*3+j] = subbox;

			subbox = ComboBox_Create(65 + i * dialog->w / 2, 80 + j * 25, 90, 20, BOX_VISIBLE);
			if (j != 2)
			{
				ComboBox_AddEntry(subbox, _("Moves"));
			}
			ComboBox_AddEntry(subbox, _("Game Ends"));
			if (j == 0)
			{
				ComboBox_SetSelection(subbox, _("Game Ends"));
			}
			else
			{
				ComboBox_SetSelection(subbox, NULL);
			}
			ComboBox_SetOnSelection(subbox, CustomTimeMovesCombo_OnSelection);
			Box_AddChild(parent, subbox);
			data->movescombo[i*3+j] = subbox;
			
			subbox = Box_Create(155 + i * dialog->w / 2, 80 + j * 25, 40, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
			subbox->fgcol = UserInfoFG2;
			if (j == 0)
			{
				Box_SetText(subbox, _("after"));
			}
			else
			{
				Box_SetText(subbox, NULL);
			}
			Box_AddChild(parent, subbox);
			data->withinafter[i*3+j] = subbox;

			subbox = Edit2Box_Create(195 + i * dialog->w / 2, 80 + j * 25, 30, 20, BOX_VISIBLE, 1);
			subbox->bgcol = RGB(255, 255, 255);
			Box_AddChild(parent, subbox);
			data->minutesedit[i*3+j] = subbox;
			if (j != 0)
			{
				Edit2Box_SetDisabled(subbox, 1);
			}

			subbox = Box_Create(225 + i * dialog->w / 2, 80 + j * 25, 50, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
			subbox->fgcol = UserInfoFG2;
			Box_SetText(subbox, _("minutes."));
			Box_AddChild(parent, subbox);
		}

		subbox = Box_Create(275 + i * dialog->w / 2, 80, 20, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = UserInfoFG2;
		Box_SetText(subbox, _("("));
		Box_AddChild(parent, subbox);

		subbox = Edit2Box_Create(295 + i * dialog->w / 2, 80, 25, 20, BOX_VISIBLE, 1);
		subbox->bgcol = RGB(255, 255, 255);
		Box_AddChild(parent, subbox);
		data->delayincedit[i] = subbox;

		subbox = Box_Create(320 + i * dialog->w / 2, 80, 60, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = UserInfoFG2;
		Box_SetText(subbox, _("seconds )"));
		Box_AddChild(parent, subbox);

	}

	subbox = CheckBox_Create(20 + dialog->w / 2, 160 + 60, BOX_VISIBLE);
	Box_AddChild(dialog, subbox);
	CheckBox_SetOnHit(subbox, CustomTime_OnSameTimeControlsCheck);
	CheckBox_SetChecked(subbox, 1);
	data->sametimecontrolscheck = subbox;
	data->sametimecontrols = 1;

	subbox = CheckBoxLinkedText_Create(35 + dialog->w / 2, 160 + 60, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->sametimecontrolscheck);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Black uses same settings as white."));
	Box_AddChild(dialog, subbox);

	subbox = CheckBox_Create(20, 195 + 60, BOX_VISIBLE);
	Box_AddChild(dialog, subbox);
	CheckBox_SetOnHit(subbox, CustomTime_OnIncrementCheck);
	data->useincrementcheck = subbox;
	data->useincrement = 0;

	subbox = CheckBoxLinkedText_Create(35, 195 + 60, 200, 20, BOX_VISIBLE | BOX_TRANSPARENT, data->useincrementcheck);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Use increment instead of delay."));
	Box_AddChild(dialog, subbox);

	subbox = Text_Create(35, 215 + 60, 600, 30, BOX_VISIBLE | BOX_TRANSPARENT, 0);
	subbox->font = tahoma10_f;
	subbox->fgcol = UserInfoFG2;
	Text_SetLinkColor(subbox, CR_LtOrange);
	Text_SetText(subbox, _("Delay causes the clocks to wait briefly before ticking.  Increment adds time\nat the end of each move. ^lLearn More...^l"));
	Text_SetLinkCallback(subbox, 1, Util_OpenURL2, strdup(_("http://chesspark.com/help/delayinc/")));
	Box_AddChild(dialog, subbox);

	subbox = Box_Create(20, 250 + 60, 150, 15, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	Box_SetText(subbox, _("Save Time Control As"));
	Box_AddChild(dialog, subbox);

	subbox = Edit2Box_Create(25, 270 + 60, 130, 18, BOX_VISIBLE, E2_HORIZ);
	subbox->bgcol = RGB(255, 255, 255);
	Edit2Box_SetAltWText(subbox, _L("Enter a name"));
	Edit2Box_SetOnKey(subbox, CustomTimeSaveEdit_OnKey);
	Edit2Box_SetText(subbox, name);
	Box_AddChild(dialog, subbox);
	data->saveedit = subbox;

	if (hassave)
	{
		subbox = StdButton_Create(165, 270 + 60, 70, _("Save"), 0);
		Button2_SetOnButtonHit(subbox, CustomTime_OnSaveControl);
		Button2_SetDisabledState(subbox, 1);
		Box_AddChild(dialog, subbox);
		data->savebutton = subbox;
	}

	subbox = Box_Create(245, 270 + 60, 200, 18, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = UserInfoFG2;
	Box_AddChild(dialog, subbox);
	data->saveresult = subbox;

	subbox = LinkBox_Create(20, dialog->h - 32, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = CR_LtOrange;
	Box_SetText(subbox, _("Need Help?"));
	LinkBox_SetClickFunc(subbox, Util_OpenURL2, strdup(_("http://chesspark.com/help/timecontrols/")));
	Box_AddChild(dialog, subbox);

	subbox = StdButton_Create(dialog->w - 325, dialog->h - 32, 120, _("Clear all fields"), 0);
	Button2_SetOnButtonHit(subbox, CustomTime_OnClearFields);
	Box_AddChild(dialog, subbox);

	subbox = StdButton_Create(dialog->w - 175, dialog->h - 32, 80, _("OK"), 0);
	Button2_SetOnButtonHit(subbox, CustomTime_OnOK);
	Box_AddChild(dialog, subbox);

	subbox = StdButton_Create(dialog->w - 90, dialog->h - 32, 80, _("Cancel"), 0);
	Button2_SetOnButtonHit(subbox, CustomTime_OnCancel);
	Box_AddChild(dialog, subbox);

	dialog->titlebar = TitleBarOnly_Add(dialog, _("Custom Time Settings"));
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	dialog->boxdata = data;
	
	Box_CreateWndCustom(dialog, _("Custom Time Settings"), parent->hwnd);


	if (whitetimecontrol && whitetimecontrol->correspondence)
	{
		char txt[120];

		ComboBox_SetSelection(data->variantcombo, _("Correspondence"));
		data->correspondence = 1;
		data->whitecontrols->flags &= ~BOX_VISIBLE;
		data->blackcontrols->flags &= ~BOX_VISIBLE;
		data->blackcontrolsborder->flags |= BOX_VISIBLE;
		data->whitecorrcontrols->flags |= BOX_VISIBLE;

		sprintf(txt, "%d", whitetimecontrol->controlarray[1]);
		Edit2Box_SetText(data->corrmovesedit, txt);

		sprintf(txt, "%d", whitetimecontrol->controlarray[2] / (60 * 60 * 24));
		Edit2Box_SetText(data->corrdaysedit, txt);
	}
	else if (whitetimecontrol)
	{
		char txt[120];
		int *p = whitetimecontrol->controlarray, numcontrols = *p;
		int i;

		sprintf(txt, "%d", abs(whitetimecontrol->delayinc));
		Edit2Box_SetText(data->delayincedit[0], txt);

		if (numcontrols > 3)
			numcontrols = 3;

		p++;
		for (i = 0; i < numcontrols; i++)
		{
			int moves = *p++;
			int time = *p++;

			if (moves != -1)
			{
				sprintf(txt, "%d", moves);
				Edit2Box_SetText(data->movesedit[i], txt);
			}

			if (time != -1)
			{
				
				sprintf(txt, "%d", time / 60);
				Edit2Box_SetText(data->minutesedit[i], txt);
			}

			if (moves != -1)
			{
				ComboBox_SetSelection(data->movescombo[i], _("Moves"));
			}
			else if (time != -1)
			{
				ComboBox_SetSelection(data->movescombo[i], _("Game Ends"));
			}
		}
	}

	if (blacktimecontrol)
	{
		char txt[120];
		int *p = blacktimecontrol->controlarray, numcontrols = *p;
		int i;

		sprintf(txt, "%d", abs(blacktimecontrol->delayinc));
		Edit2Box_SetText(data->delayincedit[1], txt);

		if (numcontrols > 3)
			numcontrols = 3;

		p++;
		for (i = 0; i < numcontrols; i++)
		{
			int moves = *p++;
			int time = *p++;

			if (moves != -1)
			{
				sprintf(txt, "%d", moves);
				Edit2Box_SetText(data->movesedit[i + 3], txt);
			}

			if (time != -1)
			{
				
				sprintf(txt, "%d", time / 60);
				Edit2Box_SetText(data->minutesedit[i + 3], txt);
			}

			if (moves != -1)
			{
				ComboBox_SetSelection(data->movescombo[i + 3], _("Moves"));
			}
			else if (time != -1)
			{
				ComboBox_SetSelection(data->movescombo[i + 3], _("Game Ends"));
			}
		}
	}

	BringWindowToTop(dialog->hwnd);

	dialog->OnDestroy = CustomTime_OnDestroy;

	return dialog;
}