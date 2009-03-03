#include <stdlib.h>

#include "box.h"

#include "image.h"
#include "tooltip.h"

#include "button2.h"

enum button2state
{
	BUTTON2_NORMAL = 0,
	BUTTON2_PRESSED,
	BUTTON2_HOVER,
	BUTTON2_DISABLED
};

/***************************************************************************
 * button2data_s
 *
 * Internal struct for Button2, not for external use.
 * 
 ***************************************************************************/
struct button2data_s
{
	BOOL clicked;
	struct Box_s *normalbox;
	struct Box_s *hoverbox;
	struct Box_s *pressedbox;
	struct Box_s *disabledbox;
	enum button2state state;
	void (*OnButtonHit)(struct Box_s *pbox);
	void (*OnButtonHit2)(struct Box_s *, void *);
	void *userdata;
	char *tooltip;
};

void Button2_OnDestroy(struct Box_s *pbox)
{
	struct button2data_s *data = pbox->boxdata;

	ToolTipParent_OnDestroy(pbox);

	free(data->tooltip);
}

void Button2_SetState(struct Box_s *pbox, enum button2state state)
{
	struct button2data_s *data = pbox->boxdata;

	if (state == data->state)
	{
		return;
	}

	data->state = state;

	switch(state)
	{
		case BUTTON2_PRESSED:
		{
			if (data->normalbox)
			{
				data->normalbox->flags   &= ~BOX_VISIBLE;
				Box_Repaint(data->normalbox);
			}
			if (data->pressedbox)
			{
				data->pressedbox->flags  |=  BOX_VISIBLE;
				Box_Repaint(data->pressedbox);
			}
			if (data->hoverbox)
			{
				data->hoverbox->flags    &= ~BOX_VISIBLE;
				Box_Repaint(data->hoverbox);
			}
			if (data->disabledbox)
			{
				data->disabledbox->flags &= ~BOX_VISIBLE;
				Box_Repaint(data->disabledbox);
			}

			Box_Repaint(pbox);
		}
		break;

		case BUTTON2_HOVER:
		{
			if (data->normalbox)
			{
				data->normalbox->flags   &= ~BOX_VISIBLE;
				Box_Repaint(data->normalbox);
			}
			if (data->pressedbox)
			{
				data->pressedbox->flags  &= ~BOX_VISIBLE;
				Box_Repaint(data->pressedbox);
			}
			if (data->hoverbox)
			{
				data->hoverbox->flags    |=  BOX_VISIBLE;
				Box_Repaint(data->hoverbox);
			}
			if (data->disabledbox)
			{
				data->disabledbox->flags &= ~BOX_VISIBLE;
				Box_Repaint(data->disabledbox);
			}

			Box_Repaint(pbox);
		}
		break;

		case BUTTON2_NORMAL:
		default:
		{
			if (data->normalbox)
			{
				data->normalbox->flags   |=  BOX_VISIBLE;
				Box_Repaint(data->normalbox);
			}
			if (data->pressedbox)
			{
				data->pressedbox->flags  &= ~BOX_VISIBLE;
				Box_Repaint(data->pressedbox);
			}
			if (data->hoverbox)
			{
				data->hoverbox->flags    &= ~BOX_VISIBLE;
				Box_Repaint(data->hoverbox);
			}
			if (data->disabledbox)
			{
				data->disabledbox->flags &= ~BOX_VISIBLE;
				Box_Repaint(data->disabledbox);
			}

			Box_Repaint(pbox);
		}
		break;

		case BUTTON2_DISABLED:
		{
			if (data->normalbox)
			{
				data->normalbox->flags   &= ~BOX_VISIBLE;
				Box_Repaint(data->normalbox);
			}
			if (data->pressedbox)
			{
				data->pressedbox->flags  &= ~BOX_VISIBLE;
				Box_Repaint(data->pressedbox);
			}
			if (data->hoverbox)
			{
				data->hoverbox->flags    &= ~BOX_VISIBLE;
				Box_Repaint(data->hoverbox);
			}
			if (data->disabledbox)
			{
				data->disabledbox->flags |= BOX_VISIBLE;
				Box_Repaint(data->disabledbox);
			}

			Box_Repaint(pbox);
		}
		break;
	}
}

/***************************************************************************
 * Button2_OnLButtonDown()
 *
 * Button2 message handler.
 * Records that the left mouse button2 was clicked here.
 * 
 ***************************************************************************/
void Button2_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct button2data_s *data = pbox->boxdata;
	if (data->state == BUTTON2_DISABLED)
	{
		return;
	}
	data->clicked = TRUE;
	Button2_SetState(pbox, BUTTON2_PRESSED);
}


void Button2_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct button2data_s *data = pbox->boxdata;

	Box_OnMouseMove(pbox, xmouse, ymouse);

	if (data->state == BUTTON2_DISABLED)
	{
		return;
	}

	if (data->tooltip && Box_CheckXYInBox(pbox, xmouse, ymouse)/* && Box_GetRoot(pbox)->active*/) 
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

	if (Box_CheckXYInBox(pbox, xmouse, ymouse))
	{
		if (data->clicked/* && data->state != BUTTON2_PRESSED*/)
		{
			Button2_SetState(pbox, BUTTON2_PRESSED);
		}
		else if (!Box_IsMouseCaptured()) /*if (data->state != BUTTON2_HOVER)*/
		{
			Button2_SetState(pbox, BUTTON2_HOVER);
		}
	}
	else /*if (data->state != BUTTON2_NORMAL)*/
	{
		Button2_SetState(pbox, BUTTON2_NORMAL);
	}
}


/***************************************************************************
 * Button2_OnLButtonUp()
 *
 * Button2 message handler.
 * If the button2 was previously clicked and the mouse is still over the
 * box, call the button2 hit callback.
 * 
 ***************************************************************************/
void Button2_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct button2data_s *data = pbox->boxdata;

	if (data->state == BUTTON2_DISABLED)
	{
		return;
	}

	if (xmouse >= 0 && ymouse >= 0 && xmouse < pbox->w && ymouse < pbox->h
		&& data->clicked)
	{
		Button2_SetState(pbox, BUTTON2_NORMAL);
		if (data->OnButtonHit2)
		{
			data->OnButtonHit2(pbox, data->userdata);
		}
		else if (data->OnButtonHit)
		{
			data->OnButtonHit(pbox);
		}
	}
	data->clicked = FALSE;
}


/***************************************************************************
 * Button2_SetOnButtonHit()
 *
 * Button2 message handler.
 * Sets the button2 specific hit callback.
 * 
 ***************************************************************************/
void Button2_SetOnButtonHit(struct Box_s *pbox, void (*pfunc)(struct Box_s *))
{
	struct button2data_s *data = pbox->boxdata;
	data->OnButtonHit = pfunc;
}


void Button2_SetOnButtonHit2(struct Box_s *pbox, void (*pfunc)(struct Box_s *, void *), void *userdata)
{
	struct button2data_s *data = pbox->boxdata;
	data->userdata = userdata;
	data->OnButtonHit2 = pfunc;
}

void Button2_SetNormal(struct Box_s *button2, struct Box_s *pbox)
{
	struct button2data_s *data = button2->boxdata;
	data->normalbox = pbox;
}

void Button2_SetHover(struct Box_s *button2, struct Box_s *pbox)
{
	struct button2data_s *data = button2->boxdata;
	data->hoverbox = pbox;
}

void Button2_SetPressed(struct Box_s *button2, struct Box_s *pbox)
{
	struct button2data_s *data = button2->boxdata;
	data->pressedbox = pbox;
}

void Button2_SetDisabled(struct Box_s *button2, struct Box_s *pbox)
{
	struct button2data_s *data = button2->boxdata;
	data->disabledbox = pbox;
}


/***************************************************************************
 * Button2_Create()
 *
 * Allocates and returns a new box and assigns Button2 message handlers and 
 * data.
 * "x", "y" indicate coordinates relative to parent.
 * "w", "h" indicate width and height.
 * "flags" are a bit field of characteristics of the new box.
 * 
 ***************************************************************************/
struct Box_s *Button2_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Box_s *button2;
	struct button2data_s *data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	button2 = Box_Create(x, y, w, h, flags);
	button2->OnLButtonDown = Button2_OnLButtonDown;
	button2->OnMouseMove   = Button2_OnMouseMove;
	button2->OnLButtonUp   = Button2_OnLButtonUp;
	button2->boxdata = data;

	return button2;
}

void Button2_SetDisabledState(struct Box_s *button2, int state)
{
	struct button2data_s *data = button2->boxdata;

	if (state)
	{
		Button2_SetState(button2, BUTTON2_DISABLED);
	}
	else
	{
		Button2_SetState(button2, BUTTON2_NORMAL);
	}
}

void Button2_SetTooltipText(struct Box_s *button, char *txt)
{
	struct button2data_s *data = button->boxdata;

	data->tooltip = strdup(txt);
}