#include <stdlib.h>

#include "box.h"

#include "image.h"
#include "tooltip.h"

#include "button.h"

enum buttonstate
{
	BUTTON_NORMAL = 0,
	BUTTON_PRESSED,
	BUTTON_HOVER,
	BUTTON_DISABLED
};

/***************************************************************************
 * buttondata_s
 *
 * Internal struct for Button, not for external use.
 * 
 ***************************************************************************/
struct buttondata_s
{
	BOOL clicked;
	void *userdata;
	struct BoxImage_s *normalimg;
	struct BoxImage_s *hoverimg;
	struct BoxImage_s *pressedimg;
	struct BoxImage_s *disabledimg;
	BOOL changecol;
	COLORREF normalbg;
	COLORREF hoverbg;
	COLORREF pressedbg;
	COLORREF disabledbg;
	COLORREF normalfg;
	COLORREF hoverfg;
	COLORREF pressedfg;
	COLORREF disabledfg;
	enum buttonstate state;
	void (*OnButtonHit)(struct Box_s *);
	void (*OnButtonHit2)(struct Box_s *, void *);
	char *tooltip;
};

void Button_OnDestroy(struct Box_s *pbox)
{
	struct buttondata_s *data = pbox->boxdata;

	ToolTipParent_OnDestroy(pbox);

	free(data->tooltip);
}

void Button_SetState(struct Box_s *pbox, enum buttonstate state)
{
	struct buttondata_s *data = pbox->boxdata;

	if (state == data->state)
	{
		return;
	}

	data->state = state;

	switch(state)
	{
		case BUTTON_PRESSED:
		{
			if (data->pressedimg)
			{
				pbox->img = data->pressedimg;
			}
			if (data->changecol)
			{
				pbox->bgcol = data->pressedbg;
				pbox->fgcol = data->pressedfg;
			}
			Box_Repaint(pbox);
		}
		break;

		case BUTTON_HOVER:
		{
			if (data->hoverimg)
			{
				pbox->img = data->hoverimg;
			}
			if (data->changecol)
			{
				pbox->bgcol = data->hoverbg;
				pbox->fgcol = data->hoverfg;
			}
			Box_Repaint(pbox);
		}
		break;

		case BUTTON_DISABLED:
		{
			if (data->disabledimg)
			{
				pbox->img = data->disabledimg;
			}
			if (data->changecol)
			{
				pbox->bgcol = data->disabledbg;
				pbox->fgcol = data->disabledfg;
			}
			Box_Repaint(pbox);
		}
		break;

		case BUTTON_NORMAL:
		default:
		{
			if (data->normalimg)
			{
				pbox->img = data->normalimg;
			}
			if (data->changecol)
			{
				pbox->bgcol = data->normalbg;
				pbox->fgcol = data->normalfg;
			}
			Box_Repaint(pbox);
		}
		break;
	}
}

void Button_SetNormalState(struct Box_s *pbox)
{
	Button_SetState(pbox, BUTTON_NORMAL);
}

void Button_SetHoverState(struct Box_s *pbox)
{
	Button_SetState(pbox, BUTTON_HOVER);
}

/***************************************************************************
 * Button_OnLButtonDown()
 *
 * Button message handler.
 * Records that the left mouse button was clicked here.
 * 
 ***************************************************************************/
void Button_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct buttondata_s *data = pbox->boxdata;
	if (data->state == BUTTON_DISABLED)
	{
		return;
	}
	data->clicked = TRUE;
	Button_SetState(pbox, BUTTON_PRESSED);
}


void Button_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct buttondata_s *data = pbox->boxdata;

	if (data->state == BUTTON_DISABLED)
	{
		return;
	}

	if (data->tooltip && Box_CheckXYInBox(pbox, xmouse, ymouse) && Box_GetRoot(pbox)->active) 
	{
		if (!pbox->tooltipvisible)
		{
			Box_AddTimedFunc(pbox, ToolTip_Popup, strdup(data->tooltip), 1000);
			pbox->tooltipvisible = 1;
		}
	}
	else
	{
		ToolTip_PopDown(pbox);
		pbox->tooltipvisible = 0;
	}

	if (xmouse >= 0 && ymouse >= 0 && xmouse < pbox->w && ymouse < pbox->h)
	{
		if (data->clicked/* && data->state != BUTTON_PRESSED*/)
		{
			Button_SetState(pbox, BUTTON_PRESSED);
		}
		else if (!Box_IsMouseCaptured()) /*if (data->state != BUTTON_HOVER)*/
		{
			Button_SetState(pbox, BUTTON_HOVER);
		}
	}
	else /*if (data->state != BUTTON_NORMAL)*/
	{
		Button_SetState(pbox, BUTTON_NORMAL);
	}
}

void Button_Trigger(struct Box_s *pbox)
{
	struct buttondata_s *data = pbox->boxdata;

	if (data->OnButtonHit2)
	{
		data->OnButtonHit2(pbox, data->userdata);
	}
	else if (data->OnButtonHit)
	{
		data->OnButtonHit(pbox);
	}
}


/***************************************************************************
 * Button_OnLButtonUp()
 *
 * Button message handler.
 * If the button was previously clicked and the mouse is still over the
 * box, call the button hit callback.
 * 
 ***************************************************************************/
void Button_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct buttondata_s *data = pbox->boxdata;

	if (data->state == BUTTON_DISABLED)
	{
		return;
	}

	if (xmouse >= 0 && ymouse >= 0 && xmouse < pbox->w && ymouse < pbox->h
		&& data->clicked)
	{
		Button_Trigger(pbox);
	}
	data->clicked = FALSE;
	Button_SetState(pbox, BUTTON_NORMAL);
}


/***************************************************************************
 * Button_SetOnButtonHit()
 *
 * Button message handler.
 * Sets the button specific hit callback.
 * 
 ***************************************************************************/
void Button_SetOnButtonHit(struct Box_s *pbox, void (*pfunc)(struct Box_s *))
{
	struct buttondata_s *data = pbox->boxdata;
	data->OnButtonHit = pfunc;
}

void Button_SetOnButtonHit2(struct Box_s *pbox, void (*pfunc)(struct Box_s *, void *), void *userdata)
{
	struct buttondata_s *data = pbox->boxdata;
	data->userdata = userdata;
	data->OnButtonHit2 = pfunc;
}


void Button_SetNormalImg(struct Box_s *pbox, struct BoxImage_s *img)
{
	struct buttondata_s *data = pbox->boxdata;
	data->normalimg = img;
	pbox->img = img;
}

void Button_SetHoverImg(struct Box_s *pbox, struct BoxImage_s *img)
{
	struct buttondata_s *data = pbox->boxdata;
	data->hoverimg = img;
}

void Button_SetPressedImg(struct Box_s *pbox, struct BoxImage_s *img)
{
	struct buttondata_s *data = pbox->boxdata;
	data->pressedimg = img;
}


void Button_SetDisabledImg(struct Box_s *pbox, struct BoxImage_s *img)
{
	struct buttondata_s *data = pbox->boxdata;
	data->disabledimg = img;
}

void Button_SetNormalBG(struct Box_s *pbox, COLORREF bgcol)
{
	struct buttondata_s *data = pbox->boxdata;
	data->changecol = 1;
	data->normalbg = bgcol;
}


void Button_SetHoverBG(struct Box_s *pbox, COLORREF bgcol)
{
	struct buttondata_s *data = pbox->boxdata;
	data->changecol = 1;
	data->hoverbg = bgcol;
}


void Button_SetPressedBG(struct Box_s *pbox, COLORREF bgcol)
{
	struct buttondata_s *data = pbox->boxdata;
	data->changecol = 1;
	data->pressedbg = bgcol;
}


void Button_SetNormalFG(struct Box_s *pbox, COLORREF fgcol)
{
	struct buttondata_s *data = pbox->boxdata;
	data->changecol = 1;
	data->normalfg = fgcol;
}


void Button_SetHoverFG(struct Box_s *pbox, COLORREF fgcol)
{
	struct buttondata_s *data = pbox->boxdata;
	data->changecol = 1;
	data->hoverfg = fgcol;
}


void Button_SetPressedFG(struct Box_s *pbox, COLORREF fgcol)
{
	struct buttondata_s *data = pbox->boxdata;
	data->changecol = 1;
	data->pressedfg = fgcol;
}


void Button_SetDisabledFG(struct Box_s *pbox, COLORREF fgcol)
{
	struct buttondata_s *data = pbox->boxdata;
	data->changecol = 1;
	data->disabledfg = fgcol;
}


void Button_SetDisabledBG(struct Box_s *pbox, COLORREF bgcol)
{
	struct buttondata_s *data = pbox->boxdata;
	data->changecol = 1;
	data->disabledbg = bgcol;
}

/***************************************************************************
 * Button_Create()
 *
 * Allocates and returns a new box and assigns Button message handlers and 
 * data.
 * "x", "y" indicate coordinates relative to parent.
 * "w", "h" indicate width and height.
 * "flags" are a bit field of characteristics of the new box.
 * 
 ***************************************************************************/
struct Box_s *Button_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Box_s *button;
	struct buttondata_s *data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	button = Box_Create(x, y, w, h, flags);
	button->OnLButtonDown = Button_OnLButtonDown;
	button->OnMouseMove   = Button_OnMouseMove;
	button->OnLButtonUp   = Button_OnLButtonUp;
	button->OnDestroy     = Button_OnDestroy;
	button->boxdata = data;

	return button;
}

void Button_SetDisabledState(struct Box_s *button, int state)
{
	struct buttondata_s *data = button->boxdata;

	if (state)
	{
		Button_SetState(button, BUTTON_DISABLED);
	}
	else
	{
		Button_SetState(button, BUTTON_NORMAL);
	}
}

void Button_SetTooltipText(struct Box_s *button, char *txt)
{
	struct buttondata_s *data = button->boxdata;

	data->tooltip = strdup(txt);
}

struct buttonlinkedtextdata_s
{
	struct Box_s *button;
	int clicked;
};

void ButtonLinkedText_OnLButtonDown(struct Box_s *pbox, int x, int y)
{
	struct buttonlinkedtextdata_s *data = pbox->boxdata;

	data->clicked = 1;
}

void ButtonLinkedText_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct buttonlinkedtextdata_s *data = pbox->boxdata;

	if (xmouse >= 0 && ymouse >= 0 && xmouse < pbox->w && ymouse < pbox->h
		&& data->clicked)
	{
		Button_Trigger(data->button);
	}

	data->clicked = 0;
}

struct Box_s *ButtonLinkedText_Create(int x, int y, int w, int h, enum Box_flags flags, struct Box_s *button)
{
	struct Box_s *buttontext;
	struct buttonlinkedtextdata_s *data;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	data->button = button;

	buttontext = Box_Create(x, y, w, h, flags);
	buttontext->boxdata = data;
	buttontext->OnLButtonDown = ButtonLinkedText_OnLButtonDown;
	buttontext->OnLButtonUp   = ButtonLinkedText_OnLButtonUp;

	return buttontext;
}