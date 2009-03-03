#include <stdlib.h>

#include "box.h"

#include "image.h"

#include "imagemgr.h"

enum checkboxstate
{
	BUTTON_NORMAL = 0,
	BUTTON_PRESSED,
	BUTTON_HOVER
};

/***************************************************************************
 * checkboxdata_s
 *
 * Internal struct for CheckBox, not for external use.
 * 
 ***************************************************************************/
struct checkboxdata_s
{
	BOOL clicked;
	enum checkboxstate state;
	struct BoxImage_s *normalimg;
	struct BoxImage_s *pressedimg;
	struct BoxImage_s *hoverimg;
	int checked;
	int enabled;
	void (*OnCheckBoxHit)(struct Box_s *, int);
	void (*OnHit2)(struct Box_s *, int, void *);
	void *userdata;
};

void CheckBox_SetState(struct Box_s *pbox, enum checkboxstate state)
{
	struct checkboxdata_s *data = pbox->boxdata;

	data->state = state;

	switch(state)
	{
		case BUTTON_PRESSED:
		{
			pbox->img = data->pressedimg;
		}
		break;

		case BUTTON_HOVER:
		{
			pbox->img = data->hoverimg;
		}
		break;
		case BUTTON_NORMAL:
		default:
		{
			pbox->img = data->normalimg;
		}
		break;
	}
	Box_Repaint(pbox);
}

void CheckBox_UpdateImages(struct Box_s *checkbox)
{
	struct checkboxdata_s *data = checkbox->boxdata;

	if (data->enabled)
	{
                if (!data->checked)
		{
			data->normalimg  = ImageMgr_GetSubImage("checkbox-enabled-unchecked-normal",  "checkbox.png",  0, 0, 13, 13);
			data->hoverimg   = ImageMgr_GetSubImage("checkbox-enabled-unchecked-hover",   "checkbox.png", 13, 0, 13, 13);
			data->pressedimg = ImageMgr_GetSubImage("checkbox-enabled-unchecked-pressed", "checkbox.png", 26, 0, 13, 13);
		}
		else
		{
			data->normalimg  = ImageMgr_GetSubImage("checkbox-enabled-checked-normal",  "checkbox.png", 39, 0, 13, 13);
			data->hoverimg   = ImageMgr_GetSubImage("checkbox-enabled-checked-hover",   "checkbox.png", 52, 0, 13, 13);
			data->pressedimg = ImageMgr_GetSubImage("checkbox-enabled-checked-pressed", "checkbox.png", 65, 0, 13, 13);
		}
	}
	else
	{
                if (!data->checked)
		{
			data->normalimg  = ImageMgr_GetSubImage("checkbox-disabled-unchecked", "checkbox.png", 78, 0, 13, 13);
			data->hoverimg   = ImageMgr_GetSubImage("checkbox-disabled-unchecked", "checkbox.png", 78, 0, 13, 13);
			data->pressedimg = ImageMgr_GetSubImage("checkbox-disabled-unchecked", "checkbox.png", 78, 0, 13, 13);
		}
		else
		{
			data->normalimg  = ImageMgr_GetSubImage("checkbox-disabled-checked", "checkbox.png", 91, 0, 13, 13);
			data->hoverimg   = ImageMgr_GetSubImage("checkbox-disabled-checked", "checkbox.png", 91, 0, 13, 13);
			data->pressedimg = ImageMgr_GetSubImage("checkbox-disabled-checked", "checkbox.png", 91, 0, 13, 13);
		}
	}

	CheckBox_SetState(checkbox, data->state);
}

/***************************************************************************
 * CheckBox_OnLButtonDown()
 *
 * CheckBox message handler.
 * Records that the left mouse button was clicked here.
 * 
 ***************************************************************************/
void CheckBox_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct checkboxdata_s *data = pbox->boxdata;
	data->clicked = TRUE;
	CheckBox_SetState(pbox, BUTTON_PRESSED);
}


void CheckBox_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct checkboxdata_s *data = pbox->boxdata;
	if (Box_CheckXYInBox(pbox, xmouse, ymouse))
	{
		if (data->clicked/* && data->state != BUTTON_PRESSED*/)
		{
			CheckBox_SetState(pbox, BUTTON_PRESSED);
		}
		else if (!Box_IsMouseCaptured()) /*if (data->state != BUTTON_HOVER)*/
		{
			CheckBox_SetState(pbox, BUTTON_HOVER);
		}
	}
	else /*if (data->state != BUTTON_NORMAL)*/
	{
		CheckBox_SetState(pbox, BUTTON_NORMAL);
	}
}

void CheckBox_Trigger(struct Box_s *pbox)
{
	struct checkboxdata_s *data = pbox->boxdata;

	data->checked = !data->checked;
	if (data->OnCheckBoxHit)
	{
		data->OnCheckBoxHit(pbox, data->checked);
	}
	if (data->OnHit2)
	{
		data->OnHit2(pbox, data->checked, data->userdata);
	}
}


/***************************************************************************
 * CheckBox_OnLButtonUp()
 *
 * CheckBox message handler.
 * If the checkbox was previously clicked and the mouse is still over the
 * box, call the checkbox hit callback.
 * 
 ***************************************************************************/
void CheckBox_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct checkboxdata_s *data = pbox->boxdata;
	if (xmouse >= 0 && ymouse >= 0 && xmouse < pbox->w && ymouse < pbox->h
		&& data->clicked)
	{
		CheckBox_Trigger(pbox);
	}
	data->clicked = FALSE;

	CheckBox_SetState(pbox, BUTTON_NORMAL);
	CheckBox_UpdateImages(pbox);
}


/***************************************************************************
 * CheckBox_SetOnCheckBoxHit()
 *
 * CheckBox message handler.
 * Sets the checkbox specific hit callback.
 * 
 ***************************************************************************/
void CheckBox_SetOnHit(struct Box_s *checkbox, void (*pfunc)(struct Box_s *, int))
{
	struct checkboxdata_s *data = checkbox->boxdata;
	data->OnCheckBoxHit = pfunc;
}

void CheckBox_SetOnHit2(struct Box_s *checkbox, void (*pfunc)(struct Box_s *, int, void *), void *userdata)
{
	struct checkboxdata_s *data = checkbox->boxdata;
	data->OnHit2 = pfunc;
	data->userdata = userdata;
}

void CheckBox_SetChecked(struct Box_s *checkbox, int checked)
{
	struct checkboxdata_s *data = checkbox->boxdata;
	data->checked = checked;
	CheckBox_UpdateImages(checkbox);
}

int CheckBox_GetChecked(struct Box_s *checkbox)
{
	struct checkboxdata_s *data = checkbox->boxdata;
	return data->checked;
}

void CheckBox_SetEnabled(struct Box_s *checkbox, int enabled)
{
	struct checkboxdata_s *data = checkbox->boxdata;
	data->enabled = enabled;
	CheckBox_UpdateImages(checkbox);
}

/***************************************************************************
 * CheckBox_Create()
 *
 * Allocates and returns a new box and assigns CheckBox message handlers and 
 * data.
 * "x", "y" indicate coordinates relative to parent.
 * "w", "h" indicate width and height.
 * "flags" are a bit field of characteristics of the new box.
 * 
 ***************************************************************************/
struct Box_s *CheckBox_Create(int x, int y, enum Box_flags flags)
{
	struct Box_s *checkbox;
	struct checkboxdata_s *data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	data->enabled = 1;

	checkbox = Box_Create(x, y, 13, 13, flags);
	checkbox->OnLButtonDown = CheckBox_OnLButtonDown;
	checkbox->OnMouseMove     = CheckBox_OnMouseMove;
	checkbox->OnLButtonUp   = CheckBox_OnLButtonUp;
	checkbox->boxdata = data;

	CheckBox_UpdateImages(checkbox);

	return checkbox;
}

struct checkboxlinkedtextdata_s
{
	struct Box_s *checkbox;
	int clicked;
};

void CheckBoxLinkedText_OnLButtonDown(struct Box_s *pbox, int x, int y)
{
	struct checkboxlinkedtextdata_s *data = pbox->boxdata;

	data->clicked = 1;
}

void CheckBoxLinkedText_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct checkboxlinkedtextdata_s *data = pbox->boxdata;

	if (xmouse >= 0 && ymouse >= 0 && xmouse < pbox->w && ymouse < pbox->h
		&& data->clicked)
	{
		CheckBox_Trigger(data->checkbox);
	}

	data->clicked = 0;
	CheckBox_UpdateImages(data->checkbox);
}

struct Box_s *CheckBoxLinkedText_Create(int x, int y, int w, int h, enum Box_flags flags, struct Box_s *checkbox)
{
	struct Box_s *checkboxtext;
	struct checkboxlinkedtextdata_s *data;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	data->checkbox = checkbox;

	checkboxtext = Box_Create(x, y, w, h, flags);
	checkboxtext->boxdata = data;
	checkboxtext->OnLButtonDown = CheckBoxLinkedText_OnLButtonDown;
	checkboxtext->OnLButtonUp   = CheckBoxLinkedText_OnLButtonUp;

	return checkboxtext;
}
