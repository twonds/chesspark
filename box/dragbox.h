#ifndef __DRAGBOX_H__
#define __DRAGBOX_H__

enum DragState
{
	DRAG_NONE = 0,
	DRAG_LBUTTONDOWN,
	DRAG_DRAGGING
};

struct DropDataHeader_s
{
	int id;
};

/* DragBox creation function */
struct Box_s *DragBox_Create(int x, int y, int w, int h, enum Box_flags flags,
	int dragid, void *dragdata, void (*ondropempty)(struct Box_s *psrc,
	int xmouse, int ymouse, int id, void *data));

/* DragBox message handlers */
void DragBox_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse);
void DragBox_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse);
void DragBox_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse);

/* DragBox accessor function */
int DragBox_GetDragState(struct Box_s *pbox);

void DragBox_SetOnSpecialDrag(struct Box_s *dragbox, void *onspecialdrag);

#endif