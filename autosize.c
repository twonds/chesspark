#include <stdlib.h>

#include "box.h"

#include "boxtypes.h"

#include "autosize.h"

struct autosizedata_s
{
	enum autosizeflags flags;
	int minw;
	int minh;
	int maxw;
	int maxh;
	int spacing;
};

struct Box_s *AutoSize_Create(int x, int y, int minw, int minh, int maxw, int maxh, enum autosizeflags flags)
{
	struct Box_s *autosize = Box_Create(x, y, minw, minh, BOX_VISIBLE | BOX_TRANSPARENT);
	struct autosizedata_s *data = malloc(sizeof(*data));

	memset(data, 0, sizeof(*data));

	data->flags = flags;
	data->minw = minw;
	data->minh = minh;
	data->maxw = maxw;
	data->maxh = maxh;

	autosize->boxtypeid = BOXTYPE_AUTOSIZE;
	autosize->boxdata = data;

	return autosize;
}

struct Box_s *AutoSizeSpace_Create(int x, int y, int minw, int minh, int maxw, int maxh, int spacing, enum autosizeflags flags)
{
	struct Box_s *autosize = AutoSize_Create(x, y, minw, minh, maxw, maxh, flags);
	struct autosizedata_s *data = autosize->boxdata;

	data->spacing = spacing;

	return autosize;
}


void AutoSize_Fit(struct Box_s *autosize)
{
	struct Box_s *child;
	struct autosizedata_s *data = autosize->boxdata;
	int x=0, y=0;

	child = autosize->child;

	while (child)
	{
		AutoSize_Fit(child);
		child = child->sibling;
	}

	if (autosize->boxtypeid != BOXTYPE_AUTOSIZE)
	{
		return;
	}

	if (!(autosize->flags & BOX_VISIBLE))
	{
		autosize->w = 0;
		autosize->h = 0;
		return;
	}

	if (data->flags & AUTOSIZE_EVENSPACING)
	{
		int spacing = 0;

		if (data->flags & AUTOSIZE_VERT)
		{
			child = autosize->child;

			while (child)
			{
				if ((child->flags & BOX_VISIBLE) && child->h > spacing)
				{
					spacing = child->h;
				}
				child = child->sibling;
			}

			child = autosize->child;

			while (child)
			{
				if (child->flags & BOX_VISIBLE)
				{
					child->y = y;
					y += spacing;

					if (child->w > x)
					{
						x = child->w;
					}
				}

				child = child->sibling;
			}
		}
		else
		{
			child = autosize->child;

			while (child)
			{
				if ((child->flags & BOX_VISIBLE) && child->w > spacing)
				{
					spacing = child->w;
				}
				child = child->sibling;
			}

			child = autosize->child;

			while (child)
			{
				if (child->flags & BOX_VISIBLE)
				{
					child->x = x;
					x += spacing;

					if (child->h > y)
					{
						y = child->h;
					}
				}

				child = child->sibling;
			}
		}
	}
	else
	{
		if (data->flags & AUTOSIZE_VERT)
		{
			int previous = 0;

			child = autosize->child;

			while (child)
			{
				if (child->flags & BOX_VISIBLE)
				{
					if (previous)
					{
						y += data->spacing;
					}

					previous = 1;

					child->y = y;
					y += child->h;

					if (child->OnSizeWidth == Box_OnSizeWidth_StickRight)
					{
						if (child->w + child->x > x)
						{
							x = child->w + child->x;
						}
					}
					else
					{
						if (child->w + child->x * 2 > x)
						{
							x = child->w + child->x * 2;
						}
					}
				}

				child = child->sibling;
			}
		}
		else
		{
			int previous = 0;

			child = autosize->child;

			while (child)
			{
				if (child->flags & BOX_VISIBLE)
				{
					if (previous)
					{
						x += data->spacing;
					}

					previous = 1;

					child->x = x;
					x += child->w;

					if (child->h + child->y > y)
					{
						y = child->h + child->y;
					}
				}

				child = child->sibling;
			}
		}
	}

	if (data->minw && x < data->minw)
	{
		x = data->minw;
	}
	
	if (data->maxw && x > data->maxw)
	{
		x = data->maxw;
	}

	if (data->minh && y < data->minh)
	{
		y = data->minh;
	}

	if (data->maxh && y > data->maxh)
	{
		y = data->maxh;
	}
	
	autosize->w = x;
	autosize->h = y;
}

void AutoSize_Fill(struct Box_s *autosize)
{
	struct Box_s *child;
	struct autosizedata_s *data = autosize->boxdata;
	int x=0, y=0;

	if (autosize->boxtypeid != BOXTYPE_AUTOSIZE)
	{
		child = autosize->child;

		while (child)
		{
			AutoSize_Fill(child);
			child = child->sibling;
		}
	
		return;
	}

	child = autosize->child;

	if (data->flags & AUTOSIZE_VERT)
	{
		while (child)
		{
			if (child->OnSizeWidth == Box_OnSizeWidth_StickRight)
			{
				child->x = autosize->w - child->w;
			}
			else if (child->OnSizeWidth && autosize->w - child->w - child->x * 2)
			{
				child->OnSizeWidth(child, autosize->w - child->w - child->x * 2);
			}

			/* assumes that one child, and exactly one child, is vertically scalable */
			if (child->OnSizeHeight == Box_OnSizeHeight_StickBottom)
			{
				child->y = autosize->h - child->h;
			}
			else if (child->OnSizeHeight)
			{
				int remaining = autosize->h;
				struct Box_s *child2 = autosize->child;

				while (child2)
				{
					if (child2 != child && child2->flags & BOX_VISIBLE)
					{
						remaining -= child2->h;
					}

					child2 = child2->sibling;
				}

				child->OnSizeHeight(child, remaining - child->h);
			}

			child = child->sibling;
		}
	}
        else
	{
		while (child)
		{
			if (child->OnSizeHeight == Box_OnSizeHeight_StickBottom)
			{
				child->y = autosize->h - child->h;
			}
			else if (child->OnSizeHeight && autosize->h - child->h - child->y * 2)
			{
				child->OnSizeHeight(child, autosize->h - child->h - child->y * 2);
			}

			/* assumes that one child, and exactly one child, is horizontally scalable */
			if (child->OnSizeWidth == Box_OnSizeWidth_StickRight)
			{
				child->x = autosize->w - child->w;
			}
			else if (child->OnSizeWidth)
			{
				int remaining = autosize->w;
				struct Box_s *child2 = autosize->child;

				while (child2)
				{
					if (child2 != child && child2->flags & BOX_VISIBLE)
					{
						remaining -= child2->w;
					}

					child2 = child2->sibling;
				}

				child->OnSizeWidth(child, remaining - child->w);
			}

			child = child->sibling;
		}
	}

	child = autosize->child;

	while (child)
	{
		AutoSize_Fill(child);
		child = child->sibling;
	}
}

struct Box_s *AutoSize_AddSpacer(struct Box_s *autosize, int spacersize)
{
	struct autosizedata_s *data = autosize->boxdata;
	struct Box_s *spacer;

	if (data->flags & AUTOSIZE_VERT)
	{
		spacer = Box_Create(0, 0, 0, spacersize, BOX_VISIBLE | BOX_TRANSPARENT);
	}
	else
	{
		spacer = Box_Create(0, 0, spacersize, 0, BOX_VISIBLE | BOX_TRANSPARENT);
	}

	Box_AddChild(autosize, spacer);
	return spacer;
}