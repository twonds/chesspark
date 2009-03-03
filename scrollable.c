#include "box.h"

#include "scroll.h"

#include "imagemgr.h"

#include "log.h"

struct scrollabledata_s
{
	struct Box_s *entry;
	struct Box_s *scroll;
	int stickybottom;
};

void Scrollable_Refresh(struct Box_s *pbox)
{
	struct scrollabledata_s *data = pbox->boxdata;
	struct Box_s *scroll;
	
	if (data->entry->child)
	{
		data->entry->realw = data->entry->child->w;
		data->entry->realh = data->entry->child->h;
	}

	/* Add a scrollbar */

	if (data->entry->realh <= data->entry->h && data->entry->yoffset == 0)
	{
		if (data->scroll)
		{
			Box_Destroy(data->scroll);
			data->scroll = NULL;
		}

		Box_OnSizeWidth_Stretch(data->entry, pbox->w - data->entry->w);

		return;
	}
	
	/* Snapping to bottom, currently disabled */
	/*
	if (data->entry->yoffset > data->entry->realh - data->entry->h && data->entry->h <= data->entry->realh)
	{
		data->entry->yoffset = data->entry->realh - data->entry->h;
	}
	*/

	Box_OnSizeWidth_Stretch(data->entry, pbox->w - 16 - data->entry->w);

	if (data->scroll)
	{
		Box_OnSizeWidth_StickRight(data->scroll, pbox->w - 15 - data->scroll->x);
		Box_OnSizeHeight_Stretch(data->scroll, pbox->h - data->scroll->h);
		VScrollBox_UpdateThumb(data->scroll);
		return;
	}

	scroll = VScrollBox_Create(pbox->w - 15, 0, 15, pbox->h, BOX_VISIBLE | BOX_TRANSPARENT, data->entry);
	Box_AddChild(pbox, scroll);
	VScrollBox_SetUpButton(scroll, ImageMgr_GetSubImage("scrollbarupnormal", "scrollbarEndUp.png", 15, 0, 15, 15),
		ImageMgr_GetSubImage("scrollbaruppressed", "scrollbarEndUp.png", 0, 0, 15, 15));
	VScrollBox_SetDownButton(scroll, ImageMgr_GetSubImage("scrollbardownnormal", "scrollbarEndDown.png", 15, 0, 15, 15),
		ImageMgr_GetSubImage("scrollbardownpressed", "scrollbarEndDown.png", 0, 0, 15, 15));
	VScrollBox_SetThumbImages(scroll,
		ImageMgr_GetImage("scrollbarThumbEndTop.png"),
		ImageMgr_GetImage("scrollbarThumbCenterTall.png"),
		ImageMgr_GetImage("scrollbarThumbEndBottom.png"),
		ImageMgr_GetImage("scrollbarThumbCenterTexture.png"));
	VScrollBox_SetTrackImages(scroll,
		ImageMgr_GetImage("scrollbarTrackEndTop.png"),
		ImageMgr_GetImage("scrollbarTrackCenterTall.png"),
		ImageMgr_GetImage("scrollbarTrackEndBottom.png"));
	data->scroll = scroll;
}

void ScrollableEntry_OnSizeHeight_Stretch(struct Box_s *pbox, int dheight)
{
	struct scrollabledata_s *data = pbox->parent->boxdata;
	int bottom = pbox->h - pbox->realh + pbox->yoffset;

	Box_OnSizeHeight_Stretch(pbox, dheight);
	Scrollable_Refresh(pbox->parent);

	if (data->stickybottom && bottom - pbox->h + pbox->realh >= 0)
	{
		pbox->yoffset = bottom - pbox->h + pbox->realh;
		Box_Repaint(pbox->parent);
	}
}


void ScrollableEntry_OnSizeWidth_Stretch(struct Box_s *pbox, int dwidth)
{
	struct scrollabledata_s *data = pbox->parent->boxdata;
	int bottom = pbox->h - pbox->realh + pbox->yoffset;
	
	Box_OnSizeWidth_Stretch(pbox, dwidth);
	Scrollable_Refresh(pbox->parent);

	if (data->stickybottom && bottom - pbox->h + pbox->realh >= 0)
	{
		pbox->yoffset = bottom - pbox->h + pbox->realh;
		Box_Repaint(pbox->parent);
	}
}

void ScrollableEntry_OnScrollWheel(struct Box_s *entry, float delta)
{
	struct Box_s *scrollable = entry->parent;
	struct scrollabledata_s *data = scrollable->boxdata;
	struct Box_s *scroll = data->scroll;

	if (scroll)
	{
		VScrollBox_OnScrollWheel(scroll, delta);
	}
}

struct Box_s *Scrollable_Create(int x, int y, int w, int h, enum Box_flags flags)
{
	struct Box_s *pbox = Box_Create(x, y, w, h, flags);
	struct scrollabledata_s *data = malloc(sizeof(*data));

	memset(data, 0, sizeof(*data));

	data->entry = Box_Create(0, 0, w, h, BOX_VISIBLE | BOX_TRANSPARENT);
	data->entry->OnScrollWheel = ScrollableEntry_OnScrollWheel;
	data->entry->OnSizeHeight = ScrollableEntry_OnSizeHeight_Stretch;
	data->entry->OnSizeWidth =  ScrollableEntry_OnSizeWidth_Stretch;
	Box_AddChild(pbox, data->entry);

	pbox->boxdata = data;

	return pbox;
}

void Scrollable_ScrollToTop(struct Box_s *pbox)
{
	struct scrollabledata_s *data = pbox->boxdata;

	data->entry->yoffset = 0;

	if (data->scroll)
	{
		VScrollBox_UpdateThumb(data->scroll);
	}

	Box_Repaint(pbox);
}


void Scrollable_ScrollToBottom(struct Box_s *pbox)
{
	struct scrollabledata_s *data = pbox->boxdata;
	if (data->entry->realh > data->entry->h)
	{
		data->entry->yoffset = data->entry->realh - data->entry->h;
	}

	if (data->scroll)
	{
		VScrollBox_UpdateThumb(data->scroll);
	}

	Box_Repaint(pbox);
}

int Scrollable_GetScroll(struct Box_s *pbox)
{
	struct scrollabledata_s *data = pbox->boxdata;

	return data->entry->yoffset;
}

int Scrollable_GetHScroll(struct Box_s *pbox)
{
	struct scrollabledata_s *data = pbox->boxdata;

	return data->entry->xoffset;
}

void Scrollable_SetScroll(struct Box_s *pbox, int yoffset)
{
	struct scrollabledata_s *data = pbox->boxdata;

	data->entry->yoffset = yoffset;

	if (data->scroll)
	{
		VScrollBox_UpdateThumb(data->scroll);
	}

	Box_Repaint(pbox);
}

void Scrollable_SetHScroll(struct Box_s *pbox, int xoffset)
{
	struct scrollabledata_s *data = pbox->boxdata;

	data->entry->xoffset = xoffset;

	Box_Repaint(pbox);
}

void Scrollable_ScrollVisible(struct Box_s *pbox)
{
	struct scrollabledata_s *data = pbox->boxdata;

	if (data->entry->yoffset > data->entry->realh - data->entry->h)
	{
		if (data->entry->realh < data->entry->h)
		{
			Scrollable_ScrollToTop(pbox);
		}
		else
		{
            Scrollable_ScrollToBottom(pbox);
		}
	}
}

void Scrollable_SetBox(struct Box_s *pbox, struct Box_s *entry)
{
	struct scrollabledata_s *data = pbox->boxdata;

	Box_AddChild(data->entry, entry);
}
