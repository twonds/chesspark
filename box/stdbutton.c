#include "box.h"

#include "text.h"

#include "button2.h"
#include "imagemgr.h"

extern HFONT tahoma11b_f;

static int StdButton_MeasureText(char *text, int w)
{
	struct Box_s *measure = Text_Create(0, 0, 0, 20, BOX_VISIBLE, TX_STRETCHHORIZ);
	measure->font = tahoma11b_f;

	Text_SetText(measure, text);

	if (measure->w + 20 > w)
	{
		w = measure->w + 20;
	}

	Box_Destroy(measure);

	return w;
}

struct Box_s *StdButton_Create(int x, int y, int w, char *buttontext, int lightbg)
{
	struct Box_s *button;
	struct Box_s *normal;
	struct Box_s *hover;
	struct Box_s *pressed;
	struct Box_s *disabled;
	struct Box_s *subbox;

	w = StdButton_MeasureText(buttontext, w);

	button   = Button2_Create(x, y, w, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	normal   = Box_Create(0, 0, w, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	hover    = Box_Create(0, 0, w, 23, BOX_TRANSPARENT);
	pressed  = Box_Create(0, 0, w, 23, BOX_TRANSPARENT);
	disabled = Box_Create(0, 0, w, 23, BOX_TRANSPARENT);

	normal->OnSizeWidth   = Box_OnSizeWidth_Stretch;
	hover->OnSizeWidth    = Box_OnSizeWidth_Stretch;
	pressed->OnSizeWidth  = Box_OnSizeWidth_Stretch;
	disabled->OnSizeWidth = Box_OnSizeWidth_Stretch;


	subbox = Box_Create(0, 0, 8, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonLeftNormal.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonLeftNormal.png");
	}
	Box_AddChild(normal, subbox);

	subbox = Box_Create(8, 0, w - 16, 23, BOX_VISIBLE | BOX_TILEIMAGE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonCenterNormal.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonCenterNormal.png");
	}
	Box_AddChild(normal, subbox);

	subbox = Box_Create(w - 8, 0, 8, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonRightNormal.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonRightNormal.png");
	}
	Box_AddChild(normal, subbox);

	if (lightbg)
	{
		subbox = Box_Create(1, 1, w, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = RGB(91, 91, 91);
	}
	else
	{
		subbox = Box_Create(1, 0, w, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = RGB(69, 69, 69);
	}
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->font = tahoma11b_f;
	Box_SetText(subbox, buttontext);
	Box_AddChild(normal, subbox);

	Box_AddChild(button, normal);


	subbox = Box_Create(0, 0, 8, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonLeftNormal.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonLeftNormal.png");
	}
	Box_AddChild(hover, subbox);

	subbox = Box_Create(8, 0, w - 16, 23, BOX_VISIBLE | BOX_TILEIMAGE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonCenterNormal.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonCenterNormal.png");
	}
	Box_AddChild(hover, subbox);

	subbox = Box_Create(w - 8, 0, 8, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonRightNormal.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonRightNormal.png");
	}
	Box_AddChild(hover, subbox);


	if (lightbg)
	{
		subbox = Box_Create(1, 1, w, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = RGB(170, 141, 70);
	}
	else
	{
		subbox = Box_Create(1, 0, w, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = RGB(157, 96, 38);
	}
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->font = tahoma11b_f;
	Box_SetText(subbox, buttontext);
	Box_AddChild(hover, subbox);

	Box_AddChild(button, hover);


	subbox = Box_Create(0, 0, 8, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonLeftPressed.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonLeftPressed.png");
	}
	Box_AddChild(pressed, subbox);

	subbox = Box_Create(8, 0, w - 16, 23, BOX_VISIBLE | BOX_TILEIMAGE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonCenterPressed.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonCenterPressed.png");
	}
	Box_AddChild(pressed, subbox);

	subbox = Box_Create(w - 8, 0, 8, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonRightPressed.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonRightPressed.png");
	}
	Box_AddChild(pressed, subbox);

	if (lightbg)
	{
		subbox = Box_Create(1, 0, w, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = RGB(91, 91, 91);
	}
	else
	{
		subbox = Box_Create(1, 1, w, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = RGB(69, 69, 69);
	}
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->font = tahoma11b_f;
	Box_SetText(subbox, buttontext);
	Box_AddChild(pressed, subbox);

	Box_AddChild(button, pressed);


	subbox = Box_Create(0, 0, 8, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonLeftNormal.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonLeftNormal.png");
	}
	Box_AddChild(disabled, subbox);

	subbox = Box_Create(8, 0, w - 16, 23, BOX_VISIBLE | BOX_TILEIMAGE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonCenterNormal.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonCenterNormal.png");
	}
	Box_AddChild(disabled, subbox);

	subbox = Box_Create(w - 8, 0, 8, 23, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	if (lightbg)
	{
		subbox->img = ImageMgr_GetImage("lightbuttonRightNormal.png");
	}
	else
	{
		subbox->img = ImageMgr_GetImage("buttonRightNormal.png");
	}
	Box_AddChild(disabled, subbox);


	if (lightbg)
	{
		subbox = Box_Create(1, 1, w, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = RGB(205, 205, 205);
	}
	else
	{
		subbox = Box_Create(1, 0, w, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
		subbox->fgcol = RGB(128, 128, 128);
	}
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->font = tahoma11b_f;
	Box_SetText(subbox, buttontext);
	Box_AddChild(disabled, subbox);

	Box_AddChild(button, disabled);

	Button2_SetNormal  (button, normal);
	Button2_SetHover   (button, hover);
	Button2_SetPressed (button, pressed);
	Button2_SetDisabled(button, disabled);

	return button;
}

void StdButton_SetText(struct Box_s *button, char *text)
{
	struct Box_s *textbox;

	textbox = button->child->child->sibling->sibling->sibling;
	Box_SetText(textbox, text);

	textbox = button->child->sibling->child->sibling->sibling->sibling;
	Box_SetText(textbox, text);

	textbox = button->child->sibling->sibling->child->sibling->sibling->sibling;
	Box_SetText(textbox, text);
}

void StdButton_SetTextAndResize(struct Box_s *button, char *text, int minw)
{
	minw = StdButton_MeasureText(text, minw);
	
	Box_OnSizeWidth_Stretch(button, minw - button->w);

	StdButton_SetText(button, text);
}
