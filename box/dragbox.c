#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <wingdi.h>
#include <commctrl.h>

#include "box.h"

#include "log.h"

#include "dragbox.h"


/***************************************************************************
 * dragboxdata_s
 *
 * Internal struct for DragBox, not for external use.
 * 
 ***************************************************************************/
struct dragboxdata_s
{
	enum DragState state;	/* Current state of the drag */
	int xstart;				/* X coord where drag started */
	int ystart;				/* Y coord where drag started */
	int dragid;
	void *data;
	void (*ondropempty)(struct Box_s *psrc, int xmouse, int ymouse, int id, void *data);
	void (*onspecialdrag)(struct Box_s *pbox, int xmouse, int ymouse);
};


void DragBox_OnDestroy(struct Box_s *pbox)
{
	struct dragboxdata_s *data = pbox->boxdata;
	if (data->state == DRAG_DRAGGING)
	{
		Box_DragEnd(pbox);
	}
	data->state = DRAG_NONE;
	Box_ReleaseMouse(pbox);
}

/***************************************************************************
 * DragBox_OnLButtonDown()
 *
 * DragBox message handler.
 * Sets the drag to the LBUTTONDOWN state if the xmouse is within the box's
 * rectangle.
 * 
 ***************************************************************************/
void DragBox_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct dragboxdata_s *data = pbox->boxdata;

	if (data->state == DRAG_DRAGGING)
	{
		/* somehow we're already dragging, so stop the drag */
		Box_DragEnd(pbox);
		data->state = DRAG_NONE;
		Box_ReleaseMouse(pbox);
		Box_OnLButtonDown(pbox, xmouse, ymouse);
		return;
	}
	
	if (!Box_CaptureMouse(pbox))
	{
		return;
	}

	data->state = DRAG_LBUTTONDOWN;
	data->xstart = xmouse;
	data->ystart = ymouse;

	Box_OnLButtonDown(pbox, xmouse, ymouse);
}


/***************************************************************************
 * Box_OnMouseMove()
 *
 * DragBox message handler.
 *
 * If the current state is LBUTTONDOWN and the mouse has moved outside a
 * 10x10 square from the initial mouse down, create a hbitmap of the current
 * box, sets it to an image list, use BeginDrag() to start dragging the
 * image, and sets the state to DRAGGING.
 *
 * If the current state is DRAGGING, move the drag image to the cursor's
 * screen position.
 * 
 ***************************************************************************/
void DragBox_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct dragboxdata_s *data = pbox->boxdata;
	
	if (data->state == DRAG_LBUTTONDOWN)
	{
		/* Make sure the mouse button is down, in case we missed an lbuttonup. */
		if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		{
			if (abs(data->xstart - xmouse) + abs(data->ystart - ymouse) > 5)
			{
				if (data->onspecialdrag)
				{
					/* special drag override, don't start dragging */
					Box_ReleaseMouse(pbox);
					data->onspecialdrag(pbox, xmouse, ymouse);
					data->state = DRAG_NONE;
				}
				else
				{
					Log_Write(0, "DragBox start %d\n", pbox);
					Box_DragStart(pbox, xmouse, ymouse, data->dragid);
					data->state = DRAG_DRAGGING;
				}
			}
		}
		else
		{
			data->state = DRAG_NONE;
			Box_ReleaseMouse(pbox);
		}
	}

	if (data->state == DRAG_DRAGGING && !data->onspecialdrag)
	{
		/* Make sure the mouse button is down, in case we missed an lbuttonup. */
		if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		{
			int x, y;
	
			Box_GetScreenCoords(pbox, &x, &y);
			x += xmouse;
			y += ymouse;

			Box_DragMove(pbox, x, y);
		}
		else
		{
			data->state = DRAG_NONE;
			Box_DragEnd(pbox);
			Box_ReleaseMouse(pbox);
		}
	}

	Box_OnMouseMove(pbox, xmouse, ymouse);
}


/***************************************************************************
 * DragBox_OnLButtonUp()
 *
 * DragBox message handler.
 * 
 * if the current state is DRAGGING, hide the drag image, end the drag, and
 * delete the image list.
 * 
 ***************************************************************************/
void DragBox_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{	
	struct dragboxdata_s *data = pbox->boxdata;

	if (data->state == DRAG_DRAGGING && !data->onspecialdrag)
	{
		int x, y;
		Log_Write(0, "DragBox end 1 %d\n", pbox);
		Box_DragEnd(pbox);

		Box_GetScreenCoords(pbox, &x, &y);
		x += xmouse;
		y += ymouse;
		Box_HandleDragDrop(pbox, x, y, data->dragid, data->data, data->ondropempty);
	}
	data->state = DRAG_NONE;
	Box_ReleaseMouse(pbox);

	Box_OnLButtonUp(pbox, xmouse, ymouse);
}

void DragBox_OnLoseMouseCapture(struct Box_s *pbox)
{
	struct dragboxdata_s *data = pbox->boxdata;

	Log_Write(0, "DragBox_OnLoseMouseCapture(%d)\n", pbox);

	if (data->state == DRAG_DRAGGING && !data->onspecialdrag)
	{
		Log_Write(0, "DragBox end 2 %d\n", pbox);
		Box_DragEnd(pbox);
	}
	data->state = DRAG_NONE;
}


/***************************************************************************
 * DragBox_Create()
 *
 * Allocates and returns a new box and assigns DragBox message handlers.
 * "x", "y" indicate coordinates relative to parent.
 * "w", "h" indicate width and height.
 * "flags" are a bit field of characteristics of the new box.
 * 
 ***************************************************************************/
struct Box_s *DragBox_Create(int x, int y, int w, int h, enum Box_flags flags,
	int dragid, void *dragdata, void (*ondropempty)(struct Box_s *psrc,
	int xmouse, int ymouse, int id, void *data))
{
	struct Box_s *pbox = Box_Create(x, y, w, h, flags);
	struct dragboxdata_s *data = (struct dragboxdata_s *)malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));
	
	data->state       = DRAG_NONE;
	data->xstart      = 0;
	data->ystart      = 0;
	data->dragid      = dragid;
	data->data        = dragdata;
	data->ondropempty = ondropempty;
	
	pbox->OnLButtonDown = DragBox_OnLButtonDown;
	pbox->OnLButtonUp   = DragBox_OnLButtonUp;
	pbox->OnMouseMove   = DragBox_OnMouseMove;
	pbox->OnDestroy     = DragBox_OnDestroy;
	pbox->OnLoseMouseCapture = DragBox_OnLoseMouseCapture;
	pbox->boxdata = data;

	return pbox;
}


/***************************************************************************
 * Box_GetDragState()
 *
 * Returns the current drag state of this DragBox.
 * 
 ***************************************************************************/
int DragBox_GetDragState(struct Box_s *pbox)
{
	struct dragboxdata_s *data = pbox->boxdata;
	return data->state;
}

void DragBox_SetOnSpecialDrag(struct Box_s *dragbox, void *onspecialdrag)
{
	struct dragboxdata_s *data = dragbox->boxdata;

	data->onspecialdrag = onspecialdrag;
}