#include <stdlib.h>

#include "box.h"

#include "button.h"
#include "dragbox.h"
#include "scroll.h"

#include "constants.h"
#include "../friendentry.h"
#include "i18n.h"
#include "imagemgr.h"
#include "namedtree.h"
/*#include "view.h"*/

#include "list.h"
#include "log.h"
#include "text.h"


struct entrydata_s
{
	struct Box_s *box;
	struct Box_s *list;
};


struct groupentrydata_s
{
	struct namedtree_s *entries;
	struct Box_s *box;
	struct Box_s *custombox;
	struct Box_s *list;
	BOOL open;
	char *name;
};


struct listdata_s
{
	struct namedtree_s *entrygroups;
	struct Box_s *entries;
	struct Box_s *scroll;
	int allgroup;
	BOOL (*entrysortfunc)(struct Box_s *lesser, struct Box_s *greater);
	BOOL (*entryvisiblefunc)(struct Box_s *entry);
	void (*listrclickfunc)(struct Box_s *list, int xmouse, int ymouse);
	void (*listgrouprclickfunc)(struct Box_s *list, char *groupname, int xmouse, int ymouse);
	void (*ongroupdragdrop)(struct Box_s *, struct Box_s *, int, void *, char *);
	int entryselectable;
	int groupselectable;
	struct Box_s *selected;
	BOOL showgroups;
	int hidenogroupheader;
	int sortgroups;
	COLORREF stripebg1;
	COLORREF stripebg2;
	int stickybottom;
	int hidetriangles;
	int groupcollapsible;
	int hasvisibleentry;
	int entrylimit;
	int spreadhorizsize;
	struct Box_s *lasttext;
	struct Box_s *dropindicator;
};


struct groupdata_s
{
	struct namedtree_s *listentry;
	struct Box_s *listbox;
};

void ListGroupTreeEntry_ShowDropIndicator(struct namedtree_s *entry, int x, int y)
{
	struct groupentrydata_s *gedata;
	struct Box_s *groupbox;
	struct groupdata_s *groupdata;
	struct Box_s *list;
	struct listdata_s *listdata;

	if (!entry)
	{
		return;
	}

	gedata = entry->data;
	groupbox = gedata->box;
	groupdata = groupbox->boxdata;
	list = groupdata->listbox;
	listdata = list->boxdata;

	if (x >= gedata->box->x - listdata->entries->xoffset
	 && y >= gedata->box->y - listdata->entries->yoffset
	 && x < gedata->box->x + gedata->box->w - listdata->entries->xoffset
	 && y < gedata->box->y + gedata->box->h - listdata->entries->yoffset)
	{
		struct Box_s *drop;

		drop = Box_Create(gedata->box->x + 2 - listdata->entries->xoffset, gedata->box->y + gedata->box->h / 2 - 28 - listdata->entries->yoffset, 3, 28, BOX_VISIBLE | BOX_TRANSPARENT | BOX_NOCLIP);
		drop->img = ImageMgr_GetImage("insertionPoint.png");
		/*drop->bgcol = RGB(0, 255, 255);*/
		Box_AddChild(list, drop);
		listdata->dropindicator = drop;
		Box_Repaint(list->parent);
		return;
	}

	ListGroupTreeEntry_ShowDropIndicator(entry->lchild, x, y);
	ListGroupTreeEntry_ShowDropIndicator(entry->rchild, x, y);
}

void List_HideDropIndicator(struct Box_s *list)
{
	struct listdata_s *data = list->boxdata;

	if (data->dropindicator)
	{
		Box_Destroy(data->dropindicator);
		data->dropindicator = NULL;
		Box_Repaint(list->parent);
	}
}

void List_ShowDropIndicator(struct Box_s *list, int x, int y)
{
	struct listdata_s *data = list->boxdata;

	List_HideDropIndicator(list);

	if (/*Box_CheckXYInBox(list, x, y) &&*/ Box_IsVisible(list) && Box_IsDragging())
	{
		ListGroupTreeEntry_ShowDropIndicator(data->entrygroups, x, y);
	}
}

void List_Select(struct Box_s *list, struct Box_s *selectbox)
{
	struct listdata_s *data = list->boxdata;
	struct Box_s *lastselect;

	Log_Write(0, "list_select %d %d\n", list, selectbox);

	if (selectbox == data->selected)
	{
		return;
	}

	lastselect = data->selected;
	data->selected = selectbox;

	if (selectbox && selectbox->parent)
	{
		/* groupbox check */
		if (selectbox->parent == data->entries)
		{
			selectbox->bgcol = TabBG4;
			Box_Repaint(selectbox);
		}
		else
		{
			selectbox->parent->bgcol = TabBG4;
			Box_Repaint(selectbox->parent);
		}
	}

	if (lastselect && lastselect->parent)
	{
		/* groupbox check */
		if (lastselect->parent == data->entries)
		{
			switch((int)(lastselect->userdata))
			{
				case 1:
					lastselect->bgcol = data->stripebg1;
					break;
				case 2:
				default:
					lastselect->bgcol = data->stripebg2;
					break;
			}
			Box_Repaint(lastselect);
		}
		else
		{
			switch((int)(lastselect->parent->userdata))
			{
				case 1:
					lastselect->parent->bgcol = data->stripebg1;
					break;
				case 2:
				default:
					lastselect->parent->bgcol = data->stripebg2;
					break;
			}
			Box_Repaint(lastselect->parent);
		}
	}
}


int Entries_SortEntries(struct entrydata_s *ldata, struct entrydata_s *gdata)
{
	struct Box_s *list = ldata->list;
	struct listdata_s *data = list->boxdata;

	if (!data->entrysortfunc)
	{
		return 0;
	}

	return data->entrysortfunc(ldata->box, gdata->box);
}

void List_SetEntrySortFunc(struct Box_s *pbox, BOOL (*entrysortfunc)(struct Box_s *, struct Box_s *))
{
	struct listdata_s *data = pbox->boxdata;

	data->entrysortfunc = entrysortfunc;
}

void* List_GetEntrySortFunc(struct Box_s *pbox)
{
	struct listdata_s *data = pbox->boxdata;

	return data->entrysortfunc;
}


void List_SetEntryVisibleFunc(struct Box_s *pbox, BOOL (*entryvisiblefunc)(struct Box_s *))
{
	struct listdata_s *data = pbox->boxdata;

	data->entryvisiblefunc = entryvisiblefunc;
}

void ListEntryBox_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *list = pbox->parent->parent;
	struct listdata_s *data = list->boxdata;
	Box_OnLButtonDown(pbox, xmouse, ymouse);

	if (data->entryselectable)
	{
		List_Select(list, pbox->child);
	}
}

void ListEntryBox_OnRButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *list = pbox->parent->parent;
	struct listdata_s *data = list->boxdata;
	Box_OnRButtonDown(pbox, xmouse, ymouse);

	if (data->entryselectable)
	{
		List_Select(list, pbox->child);
	}
}

void ListGroupBox_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *list = pbox->parent->parent;
	struct listdata_s *data = list->boxdata;
	Box_OnLButtonDown(pbox, xmouse, ymouse);

	if (data->groupselectable)
	{
		List_Select(list, pbox);
	}
}

void ListGroupBox_OnRButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *list = pbox->parent->parent;
	struct listdata_s *data = list->boxdata;
	Box_OnRButtonDown(pbox, xmouse, ymouse);

	if (data->groupselectable)
	{
		List_Select(list, pbox);
	}
}

void EntryData_Unlink(struct entrydata_s *edata)
{
	Box_Unlink(edata->box);
}

void GroupEntryData_Unlink(struct groupentrydata_s *gdata)
{
	struct namedtree_s *entry = gdata->entries;
	Box_Unlink(gdata->box);

	NamedTree_InOrder(&(gdata->entries), EntryData_Unlink);
}

static int zebra = 0;

struct coords_s
{
	int x;
	int y;
};

void *EntryData_Relink(struct namedtree_s **entry, struct coords_s *coords)
{
	struct entrydata_s *edata = (*entry)->data;
	struct listdata_s *data = edata->list->boxdata;

	if ((data->entryvisiblefunc && data->entryvisiblefunc(edata->box)) || !data->entryvisiblefunc)
	{
		struct Box_s *entrybox = Box_Create(coords->x, coords->y, data->entries->w, edata->box->h, BOX_VISIBLE);
		int nextline = 0;
		entrybox->OnSizeWidth = Box_OnSizeWidth_Stretch;
		entrybox->OnLButtonDown = ListEntryBox_OnLButtonDown;
		entrybox->OnRButtonDown = ListEntryBox_OnRButtonDown;

		if (zebra)
		{
			entrybox->bgcol = data->stripebg1;
			entrybox->userdata = (void *)1;
		}
		else
		{
			entrybox->bgcol = data->stripebg2;
			entrybox->userdata = (void *)2;
		}

		if (data->selected == edata->box)
		{
			entrybox->bgcol = TabBG4;
		}
		/*
		if ((group->name || data->allgroup) && data->showgroups)
		{
			edata->box->x = 0; /* never indented *
		}
		else*/
		{
			edata->box->x = 0;
		}

		if (data->spreadhorizsize)
		{
			Log_Write(0, "testin this %d %d %d", data->spreadhorizsize, coords->x + data->spreadhorizsize * 2, data->entries->w);
		}

		if (data->spreadhorizsize && coords->x + data->spreadhorizsize * 2 <= data->entries->w)
		{
			coords->x += data->spreadhorizsize;
		}
		else
		{
			nextline = 1;
		}
	
		Box_AddChild(entrybox, edata->box);
		Box_AddChild(data->entries, entrybox);

		/* stretch the entry to fill the width of the list, or if there's a spreadhorizsize, that */
		if (data->entries->w - edata->box->w - edata->box->x)
		{
			if (data->spreadhorizsize && data->spreadhorizsize * 2 <= data->entries->w)
			{
				Box_OnSizeWidth_Stretch(edata->box, data->spreadhorizsize - edata->box->w);
			}
			else
			{
                                Box_OnSizeWidth_Stretch(edata->box, data->entries->w - edata->box->w - edata->box->x);
			}
		}

		if (data->spreadhorizsize)
		{
			Log_Write(0, "entrybox at %d, %d\n", entrybox->x, entrybox->y);
		}

		entrybox->h = edata->box->h;
		if (entrybox->y + entrybox->h > data->entries->realh)
		{
			data->entries->realh = entrybox->y + entrybox->h;
		}

		if (nextline)
		{
			coords->x = 0;
			coords->y = data->entries->realh;
		}
		zebra = !zebra;

		data->hasvisibleentry = 1;
	}

	return NULL;
}

void *Group_Relink(struct namedtree_s **groupentry, void *userdata)
{
	struct namedtree_s *group = *groupentry;
	struct groupentrydata_s *gdata = group->data;
	struct listdata_s *data = gdata->list->boxdata;
	struct Box_s *groupbox = gdata->box;
	struct namedtree_s *entry = gdata->entries;
	
	/* No group name indicates no group */
	/*if ((group->name || data->allgroup) && data->showgroups && !(!data->hidenogroupheader && !group->name))*/
	if ((group->name || data->allgroup) && data->showgroups)
	{
		groupbox->y = data->entries->realh;
		if (data->entries->w - groupbox->w)
		{
			Box_OnSizeWidth_Stretch(groupbox, data->entries->w - groupbox->w);
		}
		/*groupbox->w = data->entries->w;*/
		if (zebra)
		{
			groupbox->bgcol = data->stripebg1;
			groupbox->userdata = (void *)1;
		}
		else
		{
			groupbox->bgcol = data->stripebg2;
			groupbox->userdata = (void *)2;
		}

		if (data->selected == gdata->box)
		{
			groupbox->bgcol = TabBG4;
		}

		Box_AddChild(data->entries, groupbox);

		data->entries->realh += groupbox->h;
		zebra = !zebra;
	}

	if ((gdata->open && data->showgroups) || (!group->name && !data->showgroups))
	{
		struct coords_s cds;
		cds.x = 0;
		cds.y = data->entries->realh;
		NamedTree_InOrder2(&(gdata->entries), EntryData_Relink, &cds);
	}

	return NULL;
}

void List_ArrangeEntries(struct Box_s *pbox, int scrollbar)
{
	struct listdata_s *data = pbox->boxdata;
	zebra = 1;

	/* Unlink all old entry boxes and group boxes */
	/*Log_Write(0, "Unlink all old entry boxes and group boxes start\n");*/
	NamedTree_InOrder(&(data->entrygroups), GroupEntryData_Unlink);
	/*Log_Write(0, "Unlink all old entry boxes and group boxes finish\n");*/

	/* Delete the empty entry box containers */
	while (data->entries->child)
	{
		Box_Destroy(data->entries->child);
	}
	data->hasvisibleentry = 0;
	
	/* Resize the main entry box container to max */
	if (scrollbar)
	{
		data->entries->w = pbox->w - 16;
	}
	else
	{
		data->entries->w = pbox->w;
	}

	data->entries->realh = 0;

	/* Relink all entries */
	/*Log_Write(0, "Relink all entry boxes and group boxes start\n");*/
	NamedTree_InOrder2(&(data->entrygroups), Group_Relink, NULL);
	/*Log_Write(0, "Relink all entry boxes and group boxes finish\n");*/
}

void List_RedoEntries(struct Box_s *pbox)
{
	struct listdata_s *data = pbox->boxdata;
	struct Box_s *scroll;

	/* arrange entries depending on whether we had a scrollbar previously */
	List_ArrangeEntries(pbox, data->scroll != NULL);

	/* If we don't need a scrollbar, but we sized for one, destroy it and size for without. */
	if (data->entries->realh <= data->entries->h && data->entries->yoffset == 0)
	{
		if (data->scroll)
		{
			Box_Destroy(data->scroll);
			data->scroll = NULL;
			List_ArrangeEntries(pbox, 1);
		}
		return;
	}

	/* If we need a scrollbar, but we sized for without one, resize for with. */
	if (!data->scroll)
	{
		List_ArrangeEntries(pbox, 1);
	}
	
	/* Snapping to bottom, currently disabled */
	/*
	if (data->entries->yoffset > data->entries->realh - data->entries->h && data->entries->h <= data->entries->realh)
	{
		data->entries->yoffset = data->entries->realh - data->entries->h;
	}
	*/

	if (data->scroll)
	{
		Box_OnSizeWidth_StickRight(data->scroll, pbox->w - 15 - data->scroll->x);
		if (pbox->h - data->scroll->h)
		{
			Box_OnSizeHeight_Stretch(data->scroll, pbox->h - data->scroll->h);
		}
		VScrollBox_UpdateThumb(data->scroll);
		return;
	}

	scroll = VScrollBox_Create(pbox->w - 15, 0, 15, pbox->h, BOX_VISIBLE | BOX_TRANSPARENT, data->entries);
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


void ListEntries_OnSizeHeight_Stretch(struct Box_s *pbox, int dheight)
{
	struct listdata_s *data = pbox->parent->boxdata;
	int atbottom = (pbox->realh > pbox->h) && pbox->realh - pbox->h - pbox->yoffset <= 0;

	Box_OnSizeHeight_Stretch(pbox, dheight);
	List_RedoEntries(pbox->parent);

	if (data->stickybottom && atbottom)
	{
		pbox->yoffset = pbox->realh - pbox->h;
		Box_Repaint(pbox->parent);
	}
}


void ListEntries_OnSizeWidth_Stretch(struct Box_s *pbox, int dwidth)
{
	struct listdata_s *data = pbox->parent->boxdata;
	int atbottom = (pbox->realh > pbox->h) && pbox->realh - pbox->h - pbox->yoffset <= 0;
	
	Box_OnSizeWidth_Stretch(pbox, dwidth);
	List_RedoEntries(pbox->parent);

	if (data->stickybottom && atbottom)
	{
		pbox->yoffset = pbox->realh - pbox->h;
		Box_Repaint(pbox->parent);
	}
}

void ListEntries_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *list = pbox->parent;
	struct listdata_s *data = pbox->parent->boxdata;

	Box_OnLButtonDown(pbox, xmouse, ymouse);

	if (ymouse > pbox->realh)
	{
		List_Select(list, NULL);
	}
}

void ListEntries_OnRButtonDown(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *list = pbox->parent;
	struct listdata_s *data = pbox->parent->boxdata;

	Box_OnRButtonDown(pbox, xmouse, ymouse);

	if (ymouse > pbox->realh)
	{
		List_Select(list, NULL);
	}
}

void ListEntries_OnRButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct listdata_s *data = pbox->parent->boxdata;
	
	Box_OnRButtonUp(pbox, xmouse, ymouse);

	if (!data->listrclickfunc)
	{
		return;
	}
	if (xmouse >= 0 && ymouse >= 0 && xmouse <= pbox->w && ymouse <= pbox->h && ymouse > pbox->realh - pbox->yoffset)
	{
		int x, y;
		Box_GetScreenCoords(pbox, &x, &y);

		x += xmouse;
		y += ymouse;

		data->listrclickfunc(pbox, x, y);
	}
}

void List_SetEmptyRClickFunc(struct Box_s *pbox, void (*listrclickfunc)(struct Box_s *list, int xmouse, int ymouse))
{
	struct listdata_s *data = pbox->boxdata;

	data->listrclickfunc = listrclickfunc;
}


void ListEntries_TimedScrollUp(struct Box_s *pbox, void *userdata)
{
	struct listdata_s *data = pbox->parent->boxdata;
	int oldoffset = pbox->yoffset;

	pbox->yoffset -= 10;

	if (pbox->yoffset < 0)
	{
		pbox->yoffset = 0;
	}

	if (oldoffset != pbox->yoffset)
	{
		Box_Repaint(pbox);
	}

	if (data->scroll)
	{
		VScrollBox_UpdateThumb(data->scroll);
	}
}


void ListEntries_TimedScrollDown(struct Box_s *pbox, void *userdata)
{
	struct listdata_s *data = pbox->parent->boxdata;
	int oldoffset = pbox->yoffset;

	int max = pbox->realh - pbox->h;

	if (max > 0 && max > pbox->yoffset)
	{
		pbox->yoffset += 10;
		if ((pbox->yoffset > max))
		{
			pbox->yoffset = max;
		}
	}

	if (oldoffset != pbox->yoffset)
	{
		Box_Repaint(pbox);
	}

	if (data->scroll)
	{
		VScrollBox_UpdateThumb(data->scroll);
	}
}


void ListEntries_OnMouseMove(struct Box_s *pbox, int x, int y)
{
	int remove = 1;

	Box_OnMouseMove(pbox, x, y);
	if (x >= 0 && x <= pbox->w && Box_IsDragging() && Box_IsVisible(pbox))
	{
		if (y <= 10)
		{
			Box_AddTimedFunc(pbox, ListEntries_TimedScrollUp, NULL, 100);
			Box_RemoveTimedFunc(pbox, ListEntries_TimedScrollDown, 100);
			remove = 0;
		}
		else if (y >= pbox->h - 10)
		{
			Box_RemoveTimedFunc(pbox, ListEntries_TimedScrollUp, 100);
			Box_AddTimedFunc(pbox, ListEntries_TimedScrollDown, NULL, 100);
			remove = 0;
		}
	}
	
	if (remove)
	{
		Box_RemoveTimedFunc(pbox, ListEntries_TimedScrollUp, 100);
		Box_RemoveTimedFunc(pbox, ListEntries_TimedScrollDown, 100);
	}

	List_ShowDropIndicator(pbox->parent, x, y);
}

void List_OnDestroy(struct Box_s *list)
{
	struct listdata_s *data = list->boxdata;

	NamedTree_Destroy(&(data->entrygroups));
}


struct Box_s *List_Create(int x, int y, int w, int h, enum Box_flags flags, int allgroup)
{
	struct Box_s *pbox = Box_Create(x, y, w, h, flags | BOX_TRANSPARENT);
	struct Box_s *entries = Box_Create(0, 0, w, h, BOX_VISIBLE);
	struct listdata_s *data = malloc(sizeof(*data));
	
	memset(data, 0, sizeof(*data));
	data->allgroup = allgroup;
	data->showgroups = TRUE;
	data->sortgroups = 1;
	data->stripebg1 = TabBG2;
	data->stripebg2 = TabBG3;
	data->entryselectable = TRUE;
	data->groupselectable = TRUE;
	data->groupcollapsible = 1;

	entries->bgcol = TabBG2;
	entries->OnMouseMove = ListEntries_OnMouseMove;
	entries->OnSizeHeight = ListEntries_OnSizeHeight_Stretch;
	entries->OnSizeWidth = ListEntries_OnSizeWidth_Stretch;
	entries->OnLButtonDown = ListEntries_OnLButtonDown;
	entries->OnRButtonDown = ListEntries_OnRButtonDown;
	entries->OnRButtonUp = ListEntries_OnRButtonUp;
	data->entries = entries;
	Box_AddChild(pbox, entries);

	pbox->boxdata = data;
	pbox->OnDestroy = List_OnDestroy;

	List_AddGroup(pbox, NULL);

	return pbox;
}

int ListGroup_OnDragDrop(struct Box_s *pdst, struct Box_s *psrc, int xmouse, int ymouse, int id, void *data)
{
	struct groupdata_s *groupdata = pdst->boxdata;
	struct Box_s *list = groupdata->listbox;
	struct listdata_s *listdata = list->boxdata;

	Log_Write(0, "ListGroup_OnDragDrop\n");

	if (listdata->ongroupdragdrop)
	{
		listdata->ongroupdragdrop(pdst, psrc, id, data, groupdata->listentry->name);
		return 1;
	}

	return 0;
}

void List_SetOnGroupDragDrop(struct Box_s *list, void (*ongroupdragdrop)(struct Box_s *, struct Box_s *, int, void *, char *))
{
	struct listdata_s *listdata = list->boxdata;

	listdata->ongroupdragdrop = ongroupdragdrop;
}

void ListGroup_ToggleOpenState(struct Box_s *pbox)
{
	struct groupdata_s *groupdata = pbox->parent->boxdata;
	struct namedtree_s *listentry = groupdata->listentry;
	struct groupentrydata_s *gedata = listentry->data;
	struct Box_s *list = groupdata->listbox;

	gedata->open = !gedata->open;

	if (gedata->open)
	{
		pbox->parent->child->sibling->img = ImageMgr_GetSubImage("OpenTriangle", "disclosureTriangles.png", 32, 0, 16, 16);
	}
	else
	{
		pbox->parent->child->sibling->img = ImageMgr_GetSubImage("ClosedTriangle", "disclosureTriangles.png", 16, 0, 16, 16);
	}

	List_RedoEntries(list);
	Box_Repaint(list);
}

void List_OpenGroup(struct Box_s *pbox, char *groupname)
{
	struct listdata_s *data = pbox->boxdata;
	struct namedtree_s **grouplistentry = NamedTree_GetByName(&(data->entrygroups), groupname);
	struct groupentrydata_s *gedata;

	if (!grouplistentry)
	{
		return;
	}

	gedata = (*grouplistentry)->data;

	gedata->open = TRUE;
	gedata->box->child->sibling->img = ImageMgr_GetSubImage("OpenTriangle", "disclosureTriangles.png", 32, 0, 16, 16);

	List_RedoEntries(pbox);
	Box_Repaint(pbox);
}

void List_SetSelectionToEntry(struct Box_s *pbox, char *entryname, char *groupname)
{
	struct listdata_s *data = pbox->boxdata;
	struct namedtree_s **grouplistentry = NamedTree_GetByName(&(data->entrygroups), groupname);
	struct groupentrydata_s *gedata;
	struct namedtree_s **entrylistentry;
	struct entrydata_s *edata;

	if (!grouplistentry)
	{
		return;
	}

	gedata = (*grouplistentry)->data;
	entrylistentry = NamedTree_GetByName(&(gedata->entries), entryname);

	if (!entrylistentry)
	{
		return;
	}

	edata = (*entrylistentry)->data;

	data->selected = edata->box;
}

void ListGroup_OnRButtonUp(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct groupdata_s *groupdata = pbox->boxdata;
	struct listdata_s *data = pbox->parent->parent->boxdata;
	
	int x, y;

	if (data->listgrouprclickfunc && xmouse >= 0 && ymouse >= 0 && xmouse <= pbox->w && ymouse <= pbox->h)
	{
		Box_GetScreenCoords(pbox, &x, &y);
		x += xmouse;
		y += ymouse;

		/*Menu_PopupGroupMenu(pbox, groupdata->listentry->name, x, y);*/
		data->listgrouprclickfunc(pbox, groupdata->listentry->name, x, y);
	}
}

void List_SetGroupRMenuFunc(struct Box_s *pbox, void (*listgrouprclickfunc)(struct Box_s *list, char *groupname, int xmouse, int ymouse))
{
	struct listdata_s *data = pbox->boxdata;
	
	data->listgrouprclickfunc = listgrouprclickfunc;
}

void GroupEntry_Destroy(struct groupentrydata_s *gedata)
{
	struct Box_s *list = gedata->list;
	struct listdata_s *data = list->boxdata;

	if (data->selected == gedata->box)
	{
		List_Select(list, NULL);
	}

	Box_Destroy(gedata->box);
	NamedTree_Destroy(&(gedata->entries));
	free(gedata->name);
	free (gedata);
}

int GroupEntry_Sort(struct groupentrydata_s *ldata, struct groupentrydata_s *rdata)
{
	if (!ldata || !rdata)
	{
		return 0;
	}

	if (!ldata->name)
	{
		return 1;
	}

	if (!rdata->name)
	{
		return 0;
	}

	return (stricmp(ldata->name, rdata->name) < 0);
}

struct namedtree_s **List_AddGroupReal(struct Box_s *pbox, char *name, struct Box_s *custombox)
{
	struct listdata_s *data = pbox->boxdata;
	struct groupdata_s *groupdata = malloc(sizeof(*groupdata));
	struct groupentrydata_s *gedata;
	struct Box_s *groupbox = Box_Create(0, 0, data->entries->w, 21, BOX_VISIBLE);
	struct Box_s *subbox;

	subbox = Button_Create(0, 0, groupbox->w, groupbox->h, BOX_VISIBLE | BOX_TRANSPARENT);
	Box_AddChild(groupbox, subbox);
	if (data->groupcollapsible)
	{
		Button_SetOnButtonHit(subbox, ListGroup_ToggleOpenState);
	}

	if (data->allgroup && name == NULL)
	{
		if (data->hidetriangles)
		{
			subbox = Box_Create(0, 3, 16, 16, BOX_TRANSPARENT);
		}
		else
		{
			subbox = Box_Create(0, 3, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		}
		subbox->img = ImageMgr_GetSubImage("ClosedTriangle", "disclosureTriangles.png", 16, 0, 16, 16);
		Box_AddChild(groupbox, subbox);
		/*Button_SetOnButtonHit(subbox, ListGroup_ToggleOpenState);*/
	
		if (data->hidetriangles)
		{
			subbox = Box_Create(4, 5, 12, 12, BOX_VISIBLE | BOX_TRANSPARENT);
		}
		else
		{
			subbox = Box_Create(20, 5, 12, 12, BOX_VISIBLE | BOX_TRANSPARENT);
		}
		subbox->img = ImageMgr_GetImage("chessparkIcon.png");
		Box_AddChild(groupbox, subbox);
		/*Button_SetOnButtonHit(subbox, ListGroup_ToggleOpenState);*/

		if (data->hidetriangles)
		{
			subbox = Box_Create(20, 3, data->entries->w - 4, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		}
		else
		{
			subbox = Box_Create(36, 3, data->entries->w - 20, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		}
		subbox->fgcol = TitleBarFG;
		Box_SetText(subbox, _("All"));
		Box_AddChild(groupbox, subbox);
		/*Button_SetOnButtonHit(subbox, ListGroup_ToggleOpenState);*/
		custombox = subbox;
	}
	else
	{
		if (data->hidetriangles)
		{
			subbox = Box_Create(0, 3, 16, 16, BOX_TRANSPARENT);
		}
		else
		{
			subbox = Box_Create(0, 3, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		}
		subbox->img = ImageMgr_GetSubImage("OpenTriangle", "disclosureTriangles.png", 32, 0, 16, 16);
		Box_AddChild(groupbox, subbox);

		if (custombox)
		{
			if (custombox->OnSizeWidth && (data->entries->w - 20 - custombox->w))
			{
				custombox->OnSizeWidth(custombox, data->entries->w - 20 - custombox->w);
			}
			Box_AddChild(groupbox, custombox);
		}
		else
		{
			if (data->hidetriangles)
			{
				subbox = Box_Create(4, 3, data->entries->w - 4, 16, BOX_VISIBLE | BOX_TRANSPARENT);
			}
			else
			{
				subbox = Box_Create(20, 3, data->entries->w - 20, 16, BOX_VISIBLE | BOX_TRANSPARENT);
			}
			subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
			subbox->fgcol = TitleBarFG;
			Box_SetText(subbox, name);
			Box_AddChild(groupbox, subbox);
			custombox = subbox;
		}
	}

	if (!data->hidetriangles)
	{
		groupbox->OnLButtonDown = ListGroupBox_OnLButtonDown;
	}
	groupbox->OnRButtonDown = ListGroupBox_OnRButtonDown;
	groupbox->OnSizeWidth = Box_OnSizeWidth_Stretch;

	gedata = malloc(sizeof(*gedata));
	memset(gedata, 0, sizeof(*gedata));
	gedata->box = groupbox;
	gedata->open = !(data->allgroup && name == NULL);
	gedata->custombox = custombox;
	gedata->list = pbox;
	gedata->name = strdup(name);

	groupdata->listentry = *NamedTree_Add(&(data->entrygroups), name, gedata, GroupEntry_Destroy, data->sortgroups ? GroupEntry_Sort : NULL);
	groupdata->listbox = pbox;

	groupbox->OnDragDrop = ListGroup_OnDragDrop;
	groupbox->OnRButtonUp = ListGroup_OnRButtonUp;
	groupbox->boxdata = groupdata;

	return &(groupdata->listentry);
}


void List_AddGroup(struct Box_s *pbox, char *name)
{
	List_AddGroupReal(pbox, name, NULL);
}


void List_AddCustomGroup(struct Box_s *pbox, char *name, struct Box_s *custombox)
{
	List_AddGroupReal(pbox, name, custombox);
}

void ListEntryData_Destroy(struct entrydata_s *edata)
{
	struct Box_s *list = edata->list;
	struct listdata_s *data = list->boxdata;

	if (data->selected == edata->box)
	{
		List_Select(list, NULL);
	}

	Box_Destroy(edata->box);
	free(edata);
}

void List_AddEntry(struct Box_s *pbox, char *entryname, char *groupname, struct Box_s *entrybox)
{
	struct listdata_s *data = pbox->boxdata;
	struct namedtree_s **group = NamedTree_GetByName(&(data->entrygroups), groupname);
	struct entrydata_s *edata;
	struct groupentrydata_s *gdata;

	if (!group)
	{
		int x = 4;
		x /= 0;
		/*group = List_AddGroupReal(pbox, groupname);*/
	}

	gdata = (*group)->data;

	edata = malloc(sizeof(*edata));
	memset (edata, 0, sizeof(*edata));
	edata->box = entrybox;
	edata->list = pbox;

	/*Log_Write(0, "Adding entry %s start\n", entryname);*/
	NamedTree_Add(&(gdata->entries), entryname, edata, ListEntryData_Destroy, Entries_SortEntries);
	/*Log_Write(0, "Adding entry %s finish\n", entryname);*/

	while(data->entrylimit && NamedTree_CountNodes(&(gdata->entries)) > data->entrylimit)
	{
		NamedTree_RemoveFirst(&(gdata->entries));
	}
}

void List_RenameGroup(struct Box_s *pbox, char *oldname, char *newname)
{
	struct listdata_s *data = pbox->boxdata;
	struct namedtree_s **group = NamedTree_GetByName(&(data->entrygroups), oldname);

	if (group)
	{
		struct groupentrydata_s *gdata = (*group)->data;
		free((*group)->name);
		(*group)->name = strdup(newname);
		Box_SetText(gdata->custombox, newname);
	}
}


struct Box_s *List_GetEntryBox(struct Box_s *pbox, char *entryname, char *groupname)
{
	struct listdata_s *data = pbox->boxdata;
	struct namedtree_s **group = NamedTree_GetByName(&(data->entrygroups), groupname);
	struct groupentrydata_s *gdata;
	struct namedtree_s **entry;

	if (!group)
	{
		return NULL;
	}

	gdata = (*group)->data;

	entry = NamedTree_GetByName(&(gdata->entries), entryname);

	if (entry)
	{
		struct entrydata_s *edata = (*entry)->data;
		return edata->box;
	}

	return NULL;
}

struct Box_s *GroupEntryData_GetEntryBox(struct groupentrydata_s *gdata, char *entryname)
{
	struct namedtree_s **entry = NamedTree_GetByName(&(gdata->entries), entryname);

	if (entry)
	{
		struct entrydata_s *edata = (*entry)->data;
		return edata->box;
	}

	return NULL;
}

void *GroupEntry_GetEntryByName(struct namedtree_s **group, char *entryname)
{
	struct groupentrydata_s *gdata;

	if (!*group)
	{
		return NULL;
	}

	gdata = (*group)->data;

	return NamedTree_GetByName(&(gdata->entries), entryname);
}

struct Box_s *List_GetEntryBoxAllGroups(struct Box_s *pbox, char *entryname)
{
	struct listdata_s *data = pbox->boxdata;
	struct namedtree_s **entry;
	
	entry = NamedTree_InOrder2(&(data->entrygroups), GroupEntry_GetEntryByName, entryname);

	if (entry)
	{
		struct entrydata_s *edata = (*entry)->data;
		return edata->box;
	}

	return NULL;
}

struct Box_s *List_GetGroupBox(struct Box_s *pbox, char *groupname)
{
	struct listdata_s *data = pbox->boxdata;
	struct namedtree_s **group = NamedTree_GetByName(&(data->entrygroups), groupname);
	struct groupentrydata_s *gdata;

	if (!group)
	{
		return NULL;
	}

	gdata = (*group)->data;

	return gdata->custombox;
}

void List_RemoveGroupByName(struct Box_s *pbox, char *groupname)
{
	struct listdata_s *data = pbox->boxdata;

	NamedTree_RemoveByName(&(data->entrygroups), groupname);
}

void List_RemoveEntryByName(struct Box_s *pbox, char *entryname, char *groupname)
{
	struct listdata_s *data = pbox->boxdata;
	struct namedtree_s **group = NamedTree_GetByName(&(data->entrygroups), groupname);

	if (group)
	{
		struct groupentrydata_s *gdata = (*group)->data;
	
		NamedTree_RemoveByName(&(gdata->entries), entryname);
	}
}

void *GroupEntry_RemoveEntryByName(struct namedtree_s **group, char *entryname)
{
	struct groupentrydata_s *gdata;

	if (!*group)
	{
		return NULL;
	}

	gdata = (*group)->data;

	NamedTree_RemoveByName(&(gdata->entries), entryname);

	return NULL;
}

void List_RemoveEntryByNameAllGroups(struct Box_s *pbox, char *entryname)
{
	struct listdata_s *data = pbox->boxdata;
	struct namedtree_s *group = data->entrygroups;
	
	NamedTree_InOrder2(&(data->entrygroups), GroupEntry_RemoveEntryByName, entryname);
}


void List_RemoveAllEntries(struct Box_s *pbox)
{
	struct listdata_s *data = pbox->boxdata;

	NamedTree_Destroy(&(data->entrygroups));

	List_AddGroup(pbox, NULL);
}

void List_ScrollToTop(struct Box_s *pbox)
{
	struct listdata_s *data = pbox->boxdata;

	data->entries->yoffset = 0;

	if (data->scroll)
	{
		VScrollBox_UpdateThumb(data->scroll);
	}

	Box_Repaint(pbox);
}

int List_IsAtBottom(struct Box_s *pbox)
{
	struct listdata_s *data = pbox->boxdata;
	int scrollspace = data->entries->realh - data->entries->h;

	return (scrollspace <= 0) || (data->entries->yoffset >= scrollspace);
}

void List_ScrollToBottom(struct Box_s *pbox)
{
	struct listdata_s *data = pbox->boxdata;

	if (data->entries->realh > data->entries->h)
	{
		data->entries->yoffset = data->entries->realh - data->entries->h;
	}
	else
	{
		data->entries->yoffset = 0;
	}

	if (data->scroll)
	{
		VScrollBox_UpdateThumb(data->scroll);
	}

	Box_Repaint(pbox);
}

int List_GetScroll(struct Box_s *pbox)
{
	struct listdata_s *data = pbox->boxdata;

	return data->entries->yoffset;
}

void List_SetScroll(struct Box_s *pbox, int yoffset)
{
	struct listdata_s *data = pbox->boxdata;

	data->entries->yoffset = yoffset;

	if (data->scroll)
	{
		VScrollBox_UpdateThumb(data->scroll);
	}

	Box_Repaint(pbox);
}

void List_ScrollVisible(struct Box_s *pbox)
{
	struct listdata_s *data = pbox->boxdata;
	if (data->entries->yoffset > data->entries->realh /*- data->entries->h*/)
	{
		if (data->entries->realh < data->entries->h)
		{
			List_ScrollToTop(pbox);
		}
		else
		{
			List_ScrollToBottom(pbox);
		}
	}
}

void List_ScrollPageUp(struct Box_s *pbox)
{
	struct listdata_s *data = pbox->boxdata;

	data->entries->yoffset -= data->entries->h;

	if (data->entries->yoffset < 0)
	{
		data->entries->yoffset = 0;
	}

	if (data->scroll)
	{
		VScrollBox_UpdateThumb(data->scroll);
	}

	Box_Repaint(pbox);
}

void List_ScrollPageDown(struct Box_s *pbox)
{
	struct listdata_s *data = pbox->boxdata;

	data->entries->yoffset += data->entries->h;

	if (data->entries->yoffset > data->entries->realh - data->entries->h)
	{
		data->entries->yoffset = data->entries->realh - data->entries->h;

		if (data->entries->yoffset < 0)
		{
			data->entries->yoffset = 0;
		}
	}

	if (data->scroll)
	{
		VScrollBox_UpdateThumb(data->scroll);
	}

	Box_Repaint(pbox);
}

void List_SetShowGroups(struct Box_s *pbox, int show)
{
	struct listdata_s *data = pbox->boxdata;

	data->showgroups = show;
}

struct funcinfo_s
{
	void (*func)(struct Box_s *, void *);
	void *userdata;
};

void *Entry_CallEntryFunc(struct namedtree_s **entry, struct funcinfo_s *finfo)
{
	struct entrydata_s *edata = (*entry)->data;
	finfo->func(edata->box, finfo->userdata);
	return NULL;
}

void *Group_CallEntryFunc(struct namedtree_s **group, struct funcinfo_s *finfo)
{
	struct groupentrydata_s *gdata = (*group)->data;
	return NamedTree_InOrder2(&(gdata->entries), Entry_CallEntryFunc, finfo);
}

void List_CallEntryFunc(struct Box_s *pbox, void (*func)(struct Box_s *, void *), void *userdata)
{
	struct listdata_s *data = pbox->boxdata;
	struct funcinfo_s finfo;

	finfo.func = func;
	finfo.userdata = userdata;
	NamedTree_InOrder2(&(data->entrygroups), Group_CallEntryFunc, &finfo);
}

void List_CallEntryFuncOnGroup(struct Box_s *pbox, char *groupname, void (*func)(struct Box_s *, void *), void *userdata)
{
	struct listdata_s *data = pbox->boxdata;
	struct namedtree_s **group = NamedTree_GetByName(&(data->entrygroups), groupname);
	struct groupentrydata_s *gdata;
	struct funcinfo_s finfo;

	finfo.func = func;
	finfo.userdata = userdata;

	if (!group)
	{
		return;
	}

	gdata = (*group)->data;

	NamedTree_InOrder2(&(gdata->entries), Entry_CallEntryFunc, &finfo);
}

void List_SetStripeBG1(struct Box_s *pbox, int col)
{
	struct listdata_s *data = pbox->boxdata;

	data->stripebg1 = col;
	data->entries->bgcol = col;
}

void List_SetStripeBG2(struct Box_s *pbox, int col)
{
	struct listdata_s *data = pbox->boxdata;

	data->stripebg2 = col;
}

void List_SetStripeBG(struct Box_s *pbox, int col)
{
	struct listdata_s *data = pbox->boxdata;

	data->entries->bgcol = col;
}

void List_SetEntrySelectable(struct Box_s *pbox, int selectable)
{
	struct listdata_s *data = pbox->boxdata;

	data->entryselectable = selectable;
}

void List_SetGroupSelectable(struct Box_s *pbox, int selectable)
{
	struct listdata_s *data = pbox->boxdata;

	data->groupselectable = selectable;
}

void List_SetGroupCollapsible(struct Box_s *pbox, int collapsible)
{
	struct listdata_s *data = pbox->boxdata;

	data->groupcollapsible = collapsible;
}

void List_SetStickyBottom(struct Box_s *pbox, int stickybottom)
{
	struct listdata_s *data = pbox->boxdata;

	data->stickybottom = stickybottom;
}

void List_SetHideDisclosureTriangles(struct Box_s *pbox, int hidetriangles)
{
	struct listdata_s *data = pbox->boxdata;

	data->hidetriangles = hidetriangles;
}

void List_SetSortGroups(struct Box_s *pbox, int sort)
{
	struct listdata_s *data = pbox->boxdata;

	data->sortgroups = sort;
}

int List_HasVisibleEntry(struct Box_s *pbox)
{
	struct listdata_s *data = pbox->boxdata;

	return data->hasvisibleentry;
}

void List_ReinsertEntry(struct Box_s *pbox, char *entryname, char *groupname)
{
	struct listdata_s *data = pbox->boxdata;
	struct namedtree_s **group = NamedTree_GetByName(&(data->entrygroups), groupname);
	struct groupentrydata_s *gdata;
	struct namedtree_s **entry;

	if (!group)
	{
		int x = 4;
		x /= 0;
		/*group = List_AddGroupReal(pbox, groupname);*/
	}

	gdata = (*group)->data;

	if (entry = NamedTree_GetByName(&(gdata->entries), entryname))
	{
                NamedTree_ReinsertNode(&(gdata->entries), entry, Entries_SortEntries);
	}
}

void List_SetLastText(struct Box_s *list, struct Box_s *text)
{
	struct listdata_s *data = list->boxdata;

	data->lasttext = text;
	Text_SetParentList(text, list);
}

struct Box_s *List_GetLastText(struct Box_s *list)
{
	struct listdata_s *data = list->boxdata;

	return data->lasttext;
}

void List_RemoveLastText(struct Box_s *list, struct Box_s *remove, struct Box_s *replace)
{
	struct listdata_s *data = list->boxdata;

	if (data->lasttext == remove)
	{
		data->lasttext = replace;
	}
}

void List_SetEntryLimit(struct Box_s *list, int numentries)
{
	struct listdata_s *data = list->boxdata;

	data->entrylimit = numentries;
}

void GroupEntryData_Resort(struct groupentrydata_s *gdata)
{
	struct Box_s *list = gdata->list;
	struct listdata_s *data = list->boxdata;

	NamedTree_Resort(&(gdata->entries), Entries_SortEntries);
}

void List_Resort(struct Box_s *list)
{
	struct listdata_s *data = list->boxdata;

	if (data->sortgroups)
	{
		NamedTree_Resort(&(data->entrygroups), GroupEntry_Sort);
	}
	NamedTree_InOrder(&(data->entrygroups), GroupEntryData_Resort);
}

void List_ScrollEntryVisible(struct Box_s *list, char *entryname, char *groupname)
{
	struct listdata_s *data = list->boxdata;
	struct namedtree_s **group = NamedTree_GetByName(&(data->entrygroups), groupname);
	struct groupentrydata_s *gdata;
	struct namedtree_s **entry;

	if (!group)
	{
		return;
	}

	gdata = (*group)->data;

	entry = NamedTree_GetByName(&(gdata->entries), entryname);

	if (entry)
	{
		struct entrydata_s *edata = (*entry)->data;

		/* If entry is off the viewable area, scroll so it's at the bottom. */
		if (edata->box->parent->y < data->entries->yoffset || edata->box->parent->y + edata->box->parent->h > data->entries->yoffset + data->entries->h)
		{
			int newoff = edata->box->parent->y + edata->box->parent->h - data->entries->h;

			if (newoff < 0)
			{
				newoff = 0;
			}

			data->entries->yoffset = newoff;
			Box_Repaint(data->entries);
		}

		return;
	}

	return;
}

void List_SetHideNoGroupHeader(struct Box_s *list, int hidenogroupheader)
{
	struct listdata_s *data = list->boxdata;

	data->hidenogroupheader = hidenogroupheader;
}

void List_SetSpreadHorizSize(struct Box_s *list, int spreadhorizsize)
{
	struct listdata_s *data = list->boxdata;

	data->spreadhorizsize = spreadhorizsize;
}
