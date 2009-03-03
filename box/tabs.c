#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <wingdi.h>
#include <commctrl.h>
#include <crtdbg.h>

#include "box.h"
#include "button.h"
#include "dragbox.h"
#include "imagemgr.h"

#include "tabs.h"

/* FIXME: rearrange code so I don't have to declare these as extern */
extern void CustomMenu_AddEntry2(struct Box_s *menu, char *text, struct BoxImage_s *icon, void (*OnHitCallback)(struct Box_s *, void *), void *userdata);
extern void CustomMenu_PopMenu(struct Box_s *menu, struct Box_s *parent, int mousex, int mousey);

/* BIG TODO: Fix all RGB values so they're either skinnable or use constants 
 *           Also fix all magic numbers    */

/***************************************************************************
 * tabdata_s
 *
 * Internal struct for tabs, not for external use.
 * 
 ***************************************************************************/
struct tabdata_s
{
	struct tabdata_s *nexttab;  /* Linked list to next tab */
	char *name;                 /* name of tab, internal */
	char *showname;             /* name shown on tab */
	struct Box_s *box_tab;      /* Ptr to tab box, subbox of tab control */
	struct Box_s *box_content;  /* Ptr to box that is hidden or shown depending on tab state */
	struct Box_s *tabctrl;
	struct Box_s *closebox;
	struct BoxImage_s *icon;
	int width;
	void (*onclose)(struct Box_s *pbox, char *name);
	void *dragdata;
	int dragid;
	void (*ondropempty)(struct Box_s *psrc, int xmouse, int ymouse, int id, void *data);
	int notify;
};


/***************************************************************************
 * tabctrldata_s
 *
 * Internal struct for TabCtrl, not for external use.
 * 
 ***************************************************************************/
struct tabctrldata_s
{
	struct tabdata_s *firsttab; /* Linked list of tabs */
	int numtabs;				/* Current number of tabs */
	int draggabletabs;			/* are tabs draggable? */
	int centered;
	struct tabdata_s *activetab;
	struct Box_s *tabsbox;
	struct Box_s *dropindicator;
	struct Box_s *leftarrow;
	struct Box_s *rightarrow;
	struct Box_s *downarrow;
	void (*OnTabActivate)(struct Box_s *pbox, char *name);
	int candrop;
	void (*onspecialdrag)(struct Box_s *tab, int xmouse, int ymouse, char *name);
	int hideall;
};

void TabCtrl_RedoTabs(struct Box_s *pbox);

void TabBox_SetActiveImg(struct Box_s *pbox, int active, int notify)
{
	struct Box_s *left = pbox->child;
	struct Box_s *center = left->sibling;
	struct Box_s *right = center->sibling;
	struct Box_s *text = right->sibling;

	if (active)
	{
		left->img   = ImageMgr_GetImage("TabActiveL.png");
		center->img = ImageMgr_GetImage("TabActiveC.png");
		right->img  = ImageMgr_GetImage("TabActiveR.png");
		if (notify)
		{
			left->bgcol   = RGB(247, 148,  28);/*TabBG2;*/
			center->bgcol = RGB(247, 148,  28);/*TabBG2;*/
			right->bgcol  = RGB(247, 148,  28);/*TabBG2;*/
			text->fgcol   = RGB(255, 255, 255);
		}
		else
		{
			left->bgcol   = RGB(213, 217, 197);/*TabBG2;*/
			center->bgcol = RGB(213, 217, 197);/*TabBG2;*/
			right->bgcol  = RGB(213, 217, 197);/*TabBG2;*/
			text->fgcol   = RGB(163,  97,   7);
		}
	}
	else
	{
		left->img   = ImageMgr_GetImage("TabInactiveL.png");
		center->img = ImageMgr_GetImage("TabInactiveC.png");
		right->img  = ImageMgr_GetImage("TabInactiveR.png");
		if (notify)
		{
			left->bgcol   = RGB(163,  97,   7);/*TabBG1;*/
			center->bgcol = RGB(163,  97,   7);/*TabBG1;*/
			right->bgcol  = RGB(163,  97,   7);/*TabBG1;*/
			text->fgcol   = RGB(255, 255, 255);
		}
		else
		{
			left->bgcol   = RGB(181, 185, 165);/*TabBG1;*/
			center->bgcol = RGB(181, 185, 165);/*TabBG1;*/
			right->bgcol  = RGB(181, 185, 165);/*TabBG1;*/
			text->fgcol   = RGB(163,  97,   7);
		}
	}
}



/***************************************************************************
 * TabBox_OnLButtonDown_ActivateTab()
 *
 * TabBox message handler.
 *
 * Makes visible the box associated with this tab, and makes invisible
 * the boxes associated with the other tabs.
 * 
 ***************************************************************************/
void TabBox_OnLButtonDown_ActivateTab(struct Box_s *pbox, int xmouse, int ymouse)
{
	struct Box_s *tabctrl = pbox->parent->parent;
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;
	struct tabdata_s *tabdata;

	/* if we're on top of a close button, don't do anything, the box handler will handle it */
	tabdata = ctrldata->firsttab;
	while (tabdata)
	{
		if (tabdata->box_tab == pbox && tabdata->closebox)
		{
			int x = xmouse - tabdata->closebox->x;
			int y = ymouse - tabdata->closebox->y;

			if (Box_CheckXYInBox(tabdata->closebox, x, y))
			{
				return;
			}
		}
		tabdata = tabdata->nexttab;
	}

	tabdata = ctrldata->firsttab;
	while (tabdata)
	{
		if (tabdata->box_tab == pbox)
		{
			if (tabdata->box_content)
			{
				tabdata->box_content->flags |= BOX_VISIBLE;
			}
			tabdata->notify = 0;
			TabBox_SetActiveImg(tabdata->box_tab, 1, tabdata->notify);
			ctrldata->activetab = tabdata;
		}
		else
		{
			if (tabdata->box_content)
			{
				tabdata->box_content->flags &= ~BOX_VISIBLE;
			}
			TabBox_SetActiveImg(tabdata->box_tab, 0, tabdata->notify);
		}
		if (tabdata->box_content)
		{
			Box_Repaint(tabdata->box_content);
		}
		tabdata = tabdata->nexttab;
	}

	if (ctrldata->OnTabActivate)
	{
		ctrldata->OnTabActivate(pbox, ctrldata->activetab->name);
	}

	Box_Repaint(tabctrl);
}


/***************************************************************************
 * DragBox_OnLButtonDown_ActivateTab()
 *
 * TabBox/DragBox message handler, used as an override for draggable tabs.
 *
 * Calls both TabBox_OnLButtonDown_ActivateTab and DragBox_OnLButtonDown.
 * 
 ***************************************************************************/
void DragBox_OnLButtonDown_ActivateTab(struct Box_s *pbox, int xmouse, int ymouse)
{
	TabBox_OnLButtonDown_ActivateTab(pbox, xmouse, ymouse);
	DragBox_OnLButtonDown(pbox, xmouse, ymouse);
}


/***************************************************************************
 * DragBox_OnLButtonUp_HideHighlight()
 *
 * TabBox/DragBox message handler, used as an override for draggable tabs.
 * 
 ***************************************************************************/
void TabCtrl_OnLButtonUp(struct Box_s *tabctrl, int xmouse, int ymouse)
{
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;

	TabCtrl_HideDropIndicator(tabctrl);

	Box_OnLButtonUp(tabctrl, xmouse, ymouse);
}


extern HFONT tahoma11b_f;

void TabClose_OnClose(struct Box_s *pbox)
{
	struct Box_s *tabbox;
	struct Box_s *tabctrl;
	struct tabctrldata_s *data;
	struct tabdata_s *tab;

	tabbox = pbox->parent;
	if (!tabbox)
	{
		return;
	}

	tabctrl = tabbox->parent->parent;
	if (!tabctrl)
	{
		return;
	}

	data = tabctrl->boxdata;
	if (!data)
	{
		return;
	}

	tab = data->firsttab;
	if (!tab)
	{
		return;
	}

	while (tab)
	{
		if (tab->box_tab == tabbox)
		{
			if (tab->onclose)
			{
				tab->onclose(tabbox, tab->name);
			}
			/*TabCtrl_RemoveTab(tabctrl, tab->name);*/
			return;
		}
		tab = tab->nexttab;
	}
}

void TabBox_RedoTab(struct Box_s *pboxtab, struct tabdata_s *tab, COLORREF textcolor)
{
	struct Box_s *tabctrl = tab->tabctrl;
	struct tabctrldata_s *data = tabctrl->boxdata;
	struct Box_s *subbox;
	
	while (pboxtab->child)
	{
		Box_Destroy(pboxtab->child);
	}

	subbox = Box_Create(0, 0, 4, pboxtab->h, BOX_VISIBLE);
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(pboxtab, subbox);

	subbox = Box_Create(4, 0, pboxtab->w - 8, pboxtab->h, BOX_VISIBLE | BOX_TILEIMAGE);
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	Box_AddChild(pboxtab, subbox);

	subbox = Box_Create(pboxtab->w - 4, 0, 4, pboxtab->h, BOX_VISIBLE);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(pboxtab, subbox);

	subbox = Box_Create(5 + (tab->icon ? 20 : 0), (pboxtab->h - 14) / 2, pboxtab->w - 10 - (tab->icon ? 20 : 0) - (tab->onclose ? 20 : 0), 14, BOX_VISIBLE | BOX_TRANSPARENT | (data->centered ? BOX_CENTERTEXT : 0));
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->fgcol = textcolor;
	subbox->font = tahoma11b_f;
	Box_SetText(subbox, tab->showname);
	Box_AddChild(pboxtab, subbox);

	if (tab->icon)
	{
		subbox = Box_Create(5, (pboxtab->h - 16) / 2, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = tab->icon;
		Box_AddChild(pboxtab, subbox);
	}

	if (tab->onclose)
	{
		subbox = Button_Create(pboxtab->w - 11 - 5, (pboxtab->h - 11) / 2, 13, 13, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox->img = ImageMgr_GetImage("deleteIcon.png");
		Button_SetOnButtonHit(subbox, TabClose_OnClose);
		Box_AddChild(pboxtab, subbox);
		tab->closebox = subbox;
	}

	if (tab == data->activetab)
	{
		TabBox_SetActiveImg(pboxtab, 1, tab->notify);
	}
	else
	{
		TabBox_SetActiveImg(pboxtab, 0, tab->notify);
	}
}

void TabDrag_OnSpecialDrag(struct Box_s *dragtab, int xmouse, int ymouse)
{
	struct Box_s *tabctrl = dragtab->parent->parent;
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;
	struct tabdata_s *tab = ctrldata->firsttab;

	while (tab)
	{
		if (tab->box_tab == dragtab)
		{
			ctrldata->onspecialdrag(dragtab, xmouse, ymouse, tab->name);
			return;
		}
		tab = tab->nexttab;
	}
}

/***************************************************************************
 * TabCtrl_RedoTabs()
 *
 * Internal helper.
 *
 * Destroys any existing children, and creates new boxes for each of the
 * tabs as children.  These boxes are the tabs in the control, not the tab
 * content.
 * 
 ***************************************************************************/
void TabCtrl_RedoTabs(struct Box_s *pbox)
{
	struct tabctrldata_s *data = pbox->boxdata;
	struct Box_s *pboxtab;
	struct tabdata_s *tab = data->firsttab;
	int x;
	int i;
		
	/* Destroy all existing children */
	while (data->tabsbox->child)
	{
		Box_Destroy(data->tabsbox->child);
	}

	/* If no tabs, bail out */
	if (data->numtabs == 0)
	{
		return;
	}

	x = 0;
	
	/* Create the tab controls */
	for (i = 0; i < data->numtabs; i++)
	{
		if (data->draggabletabs)
		{
			pboxtab = DragBox_Create(x, 0, tab->width, pbox->h, BOX_VISIBLE | BOX_CENTERTEXT, tab->dragid, tab->dragdata, tab->ondropempty);
			x += tab->width + 2;
			if (data->onspecialdrag)
			{
				DragBox_SetOnSpecialDrag(pboxtab, TabDrag_OnSpecialDrag);
			}
		}
		else
		{
			pboxtab = Box_Create(x, 0, tab->width, pbox->h, BOX_VISIBLE | BOX_CENTERTEXT);
			x += tab->width + 2;
		}
		pboxtab->OnSizeHeight = Box_OnSizeHeight_Stretch;

		TabBox_RedoTab(pboxtab, tab, pbox->fgcol);
		
		if (tab->box_content)
		{
			if (tab == data->activetab && !data->hideall)
			{
				tab->box_content->flags |= BOX_VISIBLE;
			}
			else
			{
				tab->box_content->flags &= ~BOX_VISIBLE;
			}
		}

		if (data->draggabletabs)
		{
			pboxtab->OnLButtonDown = DragBox_OnLButtonDown_ActivateTab;
		}
		else
		{
			pboxtab->OnLButtonDown = TabBox_OnLButtonDown_ActivateTab;
		}
		Box_AddChild(data->tabsbox, pboxtab);
		tab->box_tab = pboxtab;
		tab = tab->nexttab;
	}

	/* if we're over, resize tabs until we're not */
	if (x > pbox->w)
	{
		struct tabdata_s *tab = data->firsttab;
		int newsize = (pbox->w - 20)/ data->numtabs;

		if (newsize < 80)
		{
			newsize = 80;
		}

		while (tab)
		{
			struct tabdata_s *bumptab = tab->nexttab;
			int diff = newsize - tab->box_tab->w;

			Box_OnSizeWidth_Stretch(tab->box_tab, diff);

			while (bumptab)
			{
				bumptab->box_tab->x += diff;
				bumptab = bumptab->nexttab;
			}

			x += diff;

			tab = tab->nexttab;
		}
	}

	/* If we're still over, show the extra tab */
	if (x > pbox->w)
	{
		/*data->tabsbox->x = 26;*/
		Box_OnSizeWidth_Stretch(data->tabsbox, pbox->w - 78 - data->tabsbox->w);
		data->leftarrow->flags |= BOX_VISIBLE;
		data->rightarrow->flags |= BOX_VISIBLE;
		data->downarrow->flags |= BOX_VISIBLE;
	}
	else
	{
		/*data->tabsbox->x = 0;*/
		data->tabsbox->xoffset = 0;
		Box_OnSizeWidth_Stretch(data->tabsbox, pbox->w - data->tabsbox->w);
		data->leftarrow->flags &= ~BOX_VISIBLE;
		data->rightarrow->flags &= ~BOX_VISIBLE;
		data->downarrow->flags &= ~BOX_VISIBLE;
	}

	/* show visible tabs, but never show a partial tab */
#if 0
	tab = data->firsttab;
	while (tab)
	{
		if (tab->box_tab->x + tab->box_tab->w > data->tabsbox->w)
		{
			tab->box_tab->flags &= ~BOX_VISIBLE;
		}
		else
		{
			tab->box_tab->flags |= BOX_VISIBLE;
		}
		tab = tab->nexttab;
	}
#endif

/*
	if (x > pbox->w || data->tabsbox->xoffset > 0)
	{
		data->tabsbox->x = 15;
		Box_OnSizeWidth_Stretch(data->tabsbox, pbox->w - 30 - data->tabsbox->w);
		data->leftarrow->flags |= BOX_VISIBLE;
		data->rightarrow->flags |= BOX_VISIBLE;
	}
	else
	{
		data->tabsbox->x = 0;
		Box_OnSizeWidth_Stretch(data->tabsbox, pbox->w - data->tabsbox->w);
		data->leftarrow->flags &= ~BOX_VISIBLE;
		data->rightarrow->flags &= ~BOX_VISIBLE;
	}
*/
	data->tabsbox->realw = x;
	/* If we have leftover space on the right, scroll to the end */
	if (data->tabsbox->realw > data->tabsbox->w && data->tabsbox->xoffset > data->tabsbox->realw - data->tabsbox->w)
	{
		data->tabsbox->xoffset = data->tabsbox->realw - data->tabsbox->w;
	}
}

/***************************************************************************
 * TabCtrl_AddTabs()
 *
 * Adds "target" as a tab content and "name" as a tab name to the tab
 * control "pbox"
 * 
 ***************************************************************************/
void TabCtrl_AddTab2(struct Box_s *pbox, char *name, char *showname,
	struct Box_s *target, int tabwidth, struct BoxImage_s *icon,
	void *onclose, int dragid, void *dragdata, void (*ondropempty)(
		struct Box_s *psrc, int xmouse, int ymouse, int id,
		void *data))
{
	struct tabctrldata_s *data = (struct tabctrldata_s *)pbox->boxdata;
	struct tabdata_s *lasttab = data->firsttab;
	struct tabdata_s *newtab = malloc(sizeof(*newtab));
	memset(newtab, 0, sizeof(*newtab));

	if (lasttab)
	{
		while (lasttab->nexttab)
		{
			lasttab = lasttab->nexttab;
		}

		lasttab->nexttab = newtab;
	}
	else
	{
		data->firsttab = newtab;
	}

	newtab->box_content = target;
	newtab->name = strdup(name);
	newtab->showname = strdup(showname);
	newtab->icon = icon;
	newtab->dragid = dragid;
	newtab->dragdata = dragdata;
	newtab->ondropempty = ondropempty;

	if (tabwidth < 0)
	{
		int w = 50, h;
		Box_MeasureText2(pbox, tahoma11b_f, showname, &w, &h);
		newtab->width = w + 10 + (onclose ? 20 : 0) + (icon ? 20 : 0);
	}
	else
	{
		newtab->width = tabwidth;
	}

	newtab->tabctrl = pbox;
	newtab->onclose = onclose;

	data->numtabs++;

	TabCtrl_RedoTabs(pbox);
	Box_Repaint(pbox);
}

void TabCtrl_AddTab(struct Box_s *pbox, char *name, struct Box_s *target, int tabwidth)
{
	TabCtrl_AddTab2(pbox, name, name, target, tabwidth, NULL, NULL, 0, NULL, NULL);
}

void TabCtrl_RemoveTab(struct Box_s *pbox, char *name)
{
	struct tabctrldata_s *data = (struct tabctrldata_s *)pbox->boxdata;
	struct tabdata_s **lasttab = &(data->firsttab);

	while(*lasttab)
	{
		struct tabdata_s *next = (*lasttab)->nexttab;
		if (strcmp(name, (*lasttab)->name) == 0)
		{
			struct tabdata_s *old = *lasttab;
			*lasttab = next;
			data->numtabs--;
			if (data->activetab == old)
			{
				TabCtrl_ActivateFirst(pbox);
			}
			free(old);
			continue;
		}
		lasttab = &((*lasttab)->nexttab);
	}

	TabCtrl_RedoTabs(pbox);
	Box_Repaint(pbox);
}
/*
void TabCtrl_OnLeft(struct  Box_s *pbox, void *userdata)
{
	struct tabctrldata_s *data = pbox->boxdata;
	struct Box_s *tabsbox = data->tabsbox;

	tabsbox->xoffset -= 10;

	if (tabsbox->xoffset < 0)
	{
		tabsbox->xoffset = 0;
	}

	Box_Repaint(pbox);
}

void TabCtrl_OnRight(struct Box_s *pbox, void *userdata)
{
	struct tabctrldata_s *data = pbox->boxdata;
	struct Box_s *tabsbox = data->tabsbox;
	int max = tabsbox->realw - tabsbox->w;

	if (max > 0 && max > tabsbox->xoffset)
	{
		tabsbox->xoffset += 10;
		if ((tabsbox->xoffset > max))
		{
			tabsbox->xoffset = max;
		}
	}

	Box_Repaint(pbox);
}
*/
void TabsBox_OnSizeWidth_Stretch(struct Box_s *pbox, int dwidth)
{
	Box_OnSizeWidth_Stretch(pbox, dwidth);
	TabCtrl_RedoTabs(pbox->parent);
	Box_Repaint(pbox->parent);
}
/*
void TabLScrollButton_OnLButtonDown(struct Box_s *pbox, int x, int y)
{
	Box_AddTimedFunc(pbox->parent, TabCtrl_OnLeft, NULL, 100);
}

void TabLScrollButton_OnLButtonUp(struct Box_s *pbox, int x, int y)
{
	Box_RemoveTimedFunc(pbox->parent, TabCtrl_OnLeft, 100);
	Box_RemoveTimedFunc(pbox->parent, TabCtrl_OnRight, 100);	
}

void TabRScrollButton_OnLButtonDown(struct Box_s *pbox, int x, int y)
{
	Box_AddTimedFunc(pbox->parent, TabCtrl_OnRight, NULL, 100);
}

void TabRScrollButton_OnLButtonUp(struct Box_s *pbox, int x, int y)
{
	Box_RemoveTimedFunc(pbox->parent, TabCtrl_OnRight, 100);	
	Box_RemoveTimedFunc(pbox->parent, TabCtrl_OnLeft, 100);
}
*/

void TabLeftButton_OnLButtonDown(struct Box_s *pbox, int x, int y)
{
	struct Box_s *tabctrl = pbox->parent;
	struct tabctrldata_s *data = tabctrl->boxdata;
	struct Box_s *tabsbox = data->tabsbox;
	struct tabdata_s *tab;
	int found = 0;

	pbox->bgcol = RGB(181, 185, 165);

	/* Find the first invisible tab */
	tab = data->firsttab;
	while (tab && !found)
	{
		if (tab->box_tab->x + tab->box_tab->w + 3 < data->tabsbox->xoffset)
		{
			tab = tab->nexttab;
		}
		else
		{
			found = 1;
		}
	}

	if (!tab)
	{
		return;
	}

	tabsbox->xoffset = tab->box_tab->x;

	Box_Repaint(tabctrl);
}

void TabRightButton_OnLButtonDown(struct Box_s *pbox, int x, int y)
{
	struct Box_s *tabctrl = pbox->parent;
	struct tabctrldata_s *data = tabctrl->boxdata;
	struct Box_s *tabsbox = data->tabsbox;
	struct tabdata_s *tab;
	int found = 0;

	pbox->bgcol = RGB(181, 185, 165);

	/* scroll to the tab after this tab */
	tab = data->firsttab;
	while (tab && !found)
	{
		if (tab->box_tab->x > data->tabsbox->xoffset)
		{
			found = 1;
		}
		else
		{
			tab = tab->nexttab;
		}
	}

	if (!tab)
	{
		return;
	}

	tabsbox->xoffset = tab->box_tab->x;

	if (tabsbox->xoffset > tabsbox->realw - tabsbox->w)
	{
		tabsbox->xoffset = tabsbox->realw - tabsbox->w;
	}

	Box_Repaint(tabctrl);
}

void TabMenuEntry_OnHit(struct Box_s *pbox, struct tabdata_s *tab)
{
	struct Box_s *menu = Box_GetRoot(pbox);
	struct Box_s *tabctrl = tab->tabctrl;

	menu->OnInactive = NULL;
	Box_Destroy(menu);

	TabCtrl_ActivateTabAndScrollVisible(tabctrl, tab->name);
}

void TabDownButton_OnLButtonDown(struct Box_s *pbox, int x, int y)
{
	struct Box_s *tabctrl = pbox->parent;
	struct tabctrldata_s *data = tabctrl->boxdata;
	struct tabdata_s *currenttab;
	struct Box_s *menu;
	int boxx, boxy;

	menu = Box_Create(100, 100, 1, 8, BOX_VISIBLE | BOX_BORDER);
	menu->bgcol = RGB(241, 243, 231);
	menu->brcol = RGB(153, 153, 153);

	currenttab = data->firsttab;

	while (currenttab)
	{
		/*if (currenttab->box_tab->x + currenttab->box_tab->w > tabctrl->w - 16)*/
		{
			CustomMenu_AddEntry2(menu, currenttab->showname, currenttab->icon, TabMenuEntry_OnHit, currenttab);
		}
		currenttab = currenttab->nexttab;
	}

	Box_GetScreenCoords(pbox, &boxx, &boxy);

	CustomMenu_PopMenu(menu, pbox, boxx + pbox->w / 2, boxy + pbox->h / 2);
}

void TabCtrl_HideDropIndicator(struct Box_s *tabctrl)
{
	struct tabctrldata_s *data = tabctrl->boxdata;

	if (data->dropindicator)
	{
		Box_Destroy(data->dropindicator);
		data->dropindicator = NULL;
		Box_Repaint(tabctrl->parent);
	}
}

void TabCtrl_ShowDropIndicator(struct Box_s *tabctrl, int x, int y)
{
	struct tabctrldata_s *data = tabctrl->boxdata;

	TabCtrl_HideDropIndicator(tabctrl);

	if (/*Box_CheckXYInBox(tabctrl, x, y) &&*/ Box_IsVisible(tabctrl) && Box_IsDragging() && Box_GetDragId() == data->candrop)
	{
		struct tabdata_s *tab;
		int lastx = -1;

		if (Box_CheckXYInBox(tabctrl, x, y))
		{
			x -= (data->tabsbox->x - data->tabsbox->xoffset);
			tab = data->firsttab;
			while (tab)
			{
				if (x < tab->box_tab->x + tab->box_tab->w / 2)
				{
					lastx = tab->box_tab->x;
					tab = NULL;
				}
				else
				{
					tab = tab->nexttab;
				}
			}
		}

		if (lastx == -1)
		{
			tab = data->firsttab;
			while (tab)
			{
				if (tab->box_tab->x + tab->box_tab->w <= data->tabsbox->w)
				{
					lastx = tab->box_tab->x + tab->box_tab->w;
				}

				tab = tab->nexttab;
			}
		}

		if (lastx != -1)
		{
			struct Box_s *drop;
			lastx += (data->tabsbox->x - data->tabsbox->xoffset);

			drop = Box_Create(lastx - 2, tabctrl->h - 28, 3, 28, BOX_VISIBLE | BOX_TRANSPARENT | BOX_NOCLIP);
			drop->img = ImageMgr_GetImage("insertionPoint.png");
			/*drop->bgcol = RGB(0, 255, 255);*/
			Box_AddChild(tabctrl, drop);
			data->dropindicator = drop;
			Box_Repaint(tabctrl->parent);
		}
	}
}

void TabCtrl_OnMouseMove(struct Box_s *tabctrl, int x, int y)
{
	struct tabctrldata_s *data = tabctrl->boxdata;

	Box_OnMouseMove(tabctrl, x, y);

	TabCtrl_ShowDropIndicator(tabctrl, x, y);
}

/***************************************************************************
 * TabCtrl_Create()
 *
 * Allocates and returns a new TabCtrl box and assigns TabCtrl message
 * handlers.
 * "x", "y" indicate coordinates relative to parent.
 * "w", "h" indicate width and height.
 * "flags" are a bit field of characteristics of the new box.
 * 
 ***************************************************************************/
struct Box_s *TabCtrl_Create(int x, int y, int w, int h, enum Box_flags flags, int draggable, int centered, int candrop)
{
	struct Box_s *tabctrl, *subbox;
	struct tabctrldata_s *data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));
	tabctrl = Box_Create(x, y, w, h, flags);

	subbox = Box_Create(0, 0, w, h, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->OnSizeWidth = TabsBox_OnSizeWidth_Stretch;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Box_AddChild(tabctrl, subbox);
	data->tabsbox = subbox;
/*
	subbox = Box_Create(0, 0, 15, 15, 0);
	subbox->img = ImageMgr_GetSubImage("scrollbarleftnormal", "scrollbarEndLeft.png", 15, 0, 15, 15);
	subbox->OnLButtonDown = TabLScrollButton_OnLButtonDown;
	subbox->OnLButtonUp = TabLScrollButton_OnLButtonUp;
	Box_AddChild(tabctrl, subbox);
	data->leftarrow = subbox;
*/
	subbox = Box_Create(w - 76, 0, 24, 20, 0);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	subbox->img = ImageMgr_GetSubImage("tableftarrow", "tabArrows.png", 0, 0, 24, 20);
	subbox->OnLButtonDown = TabLeftButton_OnLButtonDown;
	subbox->bgcol = RGB(181, 185, 165);
	Box_AddChild(tabctrl, subbox);
	data->leftarrow = subbox;

	subbox = Box_Create(w - 50, 0, 24, 20, 0);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	subbox->img = ImageMgr_GetSubImage("tabrightarrow", "tabArrows.png", 24, 0, 24, 20);
	subbox->OnLButtonDown = TabRightButton_OnLButtonDown;
	subbox->bgcol = RGB(181, 185, 165);
	Box_AddChild(tabctrl, subbox);
	data->rightarrow = subbox;

	subbox = Box_Create(w - 24, 0, 24, 20, 0);
	subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
	subbox->img = ImageMgr_GetSubImage("tabdownarrow", "tabArrows.png", 48, 0, 24, 20);
	subbox->OnLButtonDown = TabDownButton_OnLButtonDown;
	subbox->bgcol = RGB(181, 185, 165);
	Box_AddChild(tabctrl, subbox);
	data->downarrow = subbox;

	tabctrl->boxdata = data;
	tabctrl->OnMouseMove = TabCtrl_OnMouseMove;
	tabctrl->OnLButtonUp = TabCtrl_OnLButtonUp;
	data->draggabletabs = draggable;
	data->centered = centered;
	data->candrop = candrop;
	
	return tabctrl;
}


void TabCtrl_HideAll(struct Box_s *pbox)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	struct tabdata_s *tabdata = ctrldata->firsttab;

	/*ctrldata->activetab = NULL;*/

	while (tabdata)
	{
		tabdata->box_content->flags &= ~BOX_VISIBLE;
		/*tabdata->box_tab->img = ctrldata->inactivetabimg;*/
		Box_Repaint(tabdata->box_content);
		tabdata = tabdata->nexttab;
	}

	ctrldata->hideall = 1;

	Box_Repaint(pbox);
}

void TabCtrl_UnhideAll(struct Box_s *pbox)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	struct tabdata_s *tabdata = ctrldata->firsttab;

	ctrldata->hideall = 0;

	while (tabdata)
	{
		if (ctrldata->activetab == tabdata)
		{
			tabdata->box_content->flags |= BOX_VISIBLE;
			TabBox_SetActiveImg(tabdata->box_tab, 1, tabdata->notify);
			ctrldata->activetab = tabdata;
		}
		else
		{
			tabdata->box_content->flags &= ~BOX_VISIBLE;
			TabBox_SetActiveImg(tabdata->box_tab, 0, tabdata->notify);
		}
		Box_Repaint(tabdata->box_content);
		tabdata = tabdata->nexttab;
	}

	if (ctrldata->OnTabActivate && ctrldata->activetab)
	{
		ctrldata->OnTabActivate(pbox, ctrldata->activetab->name);
	}

	Box_Repaint(pbox);
}

void TabCtrl_ActivateFirst(struct Box_s *pbox)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	struct tabdata_s *tabdata = ctrldata->firsttab;

	if (!tabdata)
	{
		ctrldata->activetab = NULL;
		return;
	}

	if (tabdata->box_content)
	{
		tabdata->box_content->flags |= BOX_VISIBLE;
	}
	tabdata->notify = 0;
	TabBox_SetActiveImg(tabdata->box_tab, 1, tabdata->notify);
	/*tabdata->box_tab->img = ctrldata->activetabimg;*/
	if (tabdata->box_content)
	{
		Box_Repaint(tabdata->box_content);
	}
	ctrldata->activetab = tabdata;
	tabdata = tabdata->nexttab;
	
	while (tabdata)
	{
		if (tabdata->box_content)
		{
			tabdata->box_content->flags &= ~BOX_VISIBLE;
		}
		TabBox_SetActiveImg(tabdata->box_tab, 0, tabdata->notify);
		/*tabdata->box_tab->img = ctrldata->inactivetabimg;*/
		if (tabdata->box_content)
		{
			Box_Repaint(tabdata->box_content);
		}
		tabdata = tabdata->nexttab;
	}

	if (ctrldata->OnTabActivate)
	{
		ctrldata->OnTabActivate(pbox, ctrldata->activetab->name);
	}

	Box_Repaint(pbox);
}

void TabCtrl_ActivateTabByName(struct Box_s *pbox, char *tabname)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	struct tabdata_s *tabdata = ctrldata->firsttab;

	while (tabdata)
	{
		if (strcmp(tabname, tabdata->name) == 0)
		{
			tabdata->box_content->flags |= BOX_VISIBLE;
			tabdata->notify = 0;
			TabBox_SetActiveImg(tabdata->box_tab, 1, tabdata->notify);
			ctrldata->activetab = tabdata;
		}
		else
		{
			tabdata->box_content->flags &= ~BOX_VISIBLE;
			TabBox_SetActiveImg(tabdata->box_tab, 0, tabdata->notify);
		}
		Box_Repaint(tabdata->box_content);
		tabdata = tabdata->nexttab;
	}

	if (ctrldata->OnTabActivate && ctrldata->activetab)
	{
		ctrldata->OnTabActivate(pbox, ctrldata->activetab->name);
	}

	Box_Repaint(pbox);
}

void TabCtrl_ActivateTabAndScrollVisible(struct Box_s *tabctrl, char *tabname)
{
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;
	struct tabdata_s **tabdata = &(ctrldata->firsttab);

	while (*tabdata)
	{
		if (strcmp(tabname, (*tabdata)->name) == 0)
		{
			if (ctrldata->tabsbox->realw > ctrldata->tabsbox->w)
			{
				if ((*tabdata)->box_tab->x < ctrldata->tabsbox->xoffset)
				{
					ctrldata->tabsbox->xoffset = (*tabdata)->box_tab->x;
				}
				else if ((*tabdata)->box_tab->x + (*tabdata)->box_tab->w > ctrldata->tabsbox->xoffset + ctrldata->tabsbox->w)
				{
					ctrldata->tabsbox->xoffset = (*tabdata)->box_tab->x;
				}

				if (ctrldata->tabsbox->xoffset > ctrldata->tabsbox->realw - ctrldata->tabsbox->w)
				{
					ctrldata->tabsbox->xoffset = ctrldata->tabsbox->realw - ctrldata->tabsbox->w;
				}
			}

			TabCtrl_ActivateTabByName(tabctrl, tabname);

			return;
		}
		tabdata = &((*tabdata)->nexttab);
	}
}

void TabCtrl_ActivateNextTab(struct Box_s *pbox)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	struct tabdata_s *tabdata = ctrldata->activetab;

	if (!tabdata || !tabdata->nexttab)
	{
		if (ctrldata->firsttab)
		{
			TabCtrl_ActivateTabAndScrollVisible(pbox, ctrldata->firsttab->name);
		}
		else
		{
			TabCtrl_ActivateFirst(pbox);
		}
		return;
	}

	TabCtrl_ActivateTabAndScrollVisible(pbox, tabdata->nexttab->name);
}

void TabCtrl_ActivatePrevTab(struct Box_s *pbox)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	struct tabdata_s *tabdata = ctrldata->firsttab;

	if (!tabdata || !ctrldata->activetab)
	{
		if (ctrldata->firsttab)
		{
			TabCtrl_ActivateTabAndScrollVisible(pbox, ctrldata->firsttab->name);
		}
		else
		{
			TabCtrl_ActivateFirst(pbox);
		}
		return;
	}

	while (tabdata->nexttab && tabdata->nexttab != ctrldata->activetab)
	{
		tabdata = tabdata->nexttab;
	}

	TabCtrl_ActivateTabAndScrollVisible(pbox, tabdata->name);
}

char *TabCtrl_GetActiveTab(struct Box_s *pbox)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	
	if (ctrldata->activetab)
	{
		return ctrldata->activetab->name;
	}
	return NULL;
}

char *TabCtrl_GetFirstTab(struct Box_s *pbox)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	
	if (ctrldata->firsttab)
	{
		return ctrldata->firsttab->name;
	}
	return NULL;
}

void TabCtrl_SetTabActivateFunc(struct Box_s *pbox, void (*OnTabActivate)(struct Box_s *, char *))
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;

	ctrldata->OnTabActivate = OnTabActivate;
}
/*
void TabCtrl_SetTabTextColor(struct Box_s *pbox, char *name, COLORREF col)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	struct tabdata_s *tabdata = ctrldata->firsttab;

	while (tabdata)
	{
		if (strcmp(name, tabdata->name) == 0)
		{
			tabdata->box_tab->child->sibling->sibling->sibling->fgcol = col;
			Box_Repaint(tabdata->box_tab);
			return;
		}
		tabdata = tabdata->nexttab;
	}
}
*/
void TabCtrl_SetTabNotify(struct Box_s *pbox, char *name)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	struct tabdata_s *tabdata = ctrldata->firsttab;

	while (tabdata)
	{
		if (strcmp(name, tabdata->name) == 0)
		{
			tabdata->notify = 1;
			TabBox_SetActiveImg(tabdata->box_tab, ctrldata->activetab == tabdata, tabdata->notify);
			Box_Repaint(tabdata->box_tab);

			if (tabdata->box_tab->x < ctrldata->tabsbox->xoffset)
			{
				ctrldata->leftarrow->bgcol = RGB(247, 148,  28);
				Box_Repaint(ctrldata->leftarrow);
			}
			else if (tabdata->box_tab->x + tabdata->box_tab->w > ctrldata->tabsbox->xoffset + ctrldata->tabsbox->w)
			{
				ctrldata->rightarrow->bgcol = RGB(247, 148,  28);
				Box_Repaint(ctrldata->rightarrow);
			}

			return;
		}
		tabdata = tabdata->nexttab;
	}

}

void TabCtrl_SetTabIcon(struct Box_s *pbox, char *name, struct BoxImage_s *img)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	struct tabdata_s *tabdata = ctrldata->firsttab;

	while (tabdata)
	{
		if (strcmp(name, tabdata->name) == 0)
		{
			tabdata->icon = img;
			TabBox_RedoTab(tabdata->box_tab, tabdata, tabdata->box_tab->child->sibling->sibling->sibling->fgcol);
			return;
		}

		tabdata = tabdata->nexttab;
	}
}

struct tabdata_s **TabCtrl_GetTabData(struct Box_s *tabctrl, char *name)
{
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;
	struct tabdata_s **tab = &(ctrldata->firsttab);

	while (*tab)
	{
		if (stricmp(name, (*tab)->name) == 0)
		{
			return tab;
		}

		tab = &((*tab)->nexttab);
	}

	return NULL;
}

void TabCtrl_MoveTab(struct Box_s *tabctrl, char *name, int pos)
{
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;
	struct tabdata_s **tabsrc, **tabdst;

	tabsrc = TabCtrl_GetTabData(tabctrl, name);

	if (!tabsrc)
	{
		return;
	}

	tabdst = &(ctrldata->firsttab);

	while (*tabdst)
	{
		if(pos == 0)
		{
			struct tabdata_s *next;

			if (tabsrc == tabdst || (*tabsrc)->nexttab == *tabdst)
			{
				return;
			}

			next = (*tabsrc)->nexttab;
			(*tabsrc)->nexttab = *tabdst;
			*tabdst = *tabsrc;
			*tabsrc = next;

			TabCtrl_RedoTabs(tabctrl);
			TabCtrl_ActivateTabByName(tabctrl, name);
			Box_Repaint(tabctrl->parent);
			return;
		}

		tabdst = &((*tabdst)->nexttab);
		pos--;
	}

	/* We've fallen out, move the tab to the end */
	tabdst = &(ctrldata->firsttab);

	while (*tabdst)
	{
		tabdst = &((*tabdst)->nexttab);
	}

	{
		struct tabdata_s *next;

		if (tabsrc == tabdst || (*tabsrc)->nexttab == *tabdst)
		{
			return;
		}

		next = (*tabsrc)->nexttab;
		(*tabsrc)->nexttab = *tabdst;
		*tabdst = *tabsrc;
		*tabsrc = next;

		TabCtrl_RedoTabs(tabctrl);
		TabCtrl_ActivateTabByName(tabctrl, name);
		Box_Repaint(tabctrl->parent);
		return;
	}

	return;
}

int TabCtrl_GetDropPos(struct Box_s *tabctrl, int x, int y)
{
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;
	struct tabdata_s **tabdst;
	int droppos = 0;

	tabdst = &(ctrldata->firsttab);
	x -= (ctrldata->tabsbox->x - ctrldata->tabsbox->xoffset);

	if (x < (ctrldata->tabsbox->w - 20))
	{
		while (*tabdst)
		{
			if (x < (*tabdst)->box_tab->x + (*tabdst)->box_tab->w / 2)
			{
				return droppos;
			}

			tabdst = &((*tabdst)->nexttab);
			droppos++;
		}
	}

	return droppos;
}

void TabCtrl_HandleTabDrop(struct Box_s *tabctrl, char *name, int x, int y)
{
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;
	struct tabdata_s **tabsrc, **tabdst;

	tabsrc = TabCtrl_GetTabData(tabctrl, name);

	if (!tabsrc)
	{
		return;
	}

	tabdst = &(ctrldata->firsttab);
	x -= (ctrldata->tabsbox->x - ctrldata->tabsbox->xoffset);

	if (x < (ctrldata->tabsbox->w - 20))
	{
		while (*tabdst)
		{
			if (x < (*tabdst)->box_tab->x + (*tabdst)->box_tab->w / 2)
			{
				struct tabdata_s *next;

				if (tabsrc == tabdst || (*tabsrc)->nexttab == *tabdst)
				{
					return;
				}

				next = (*tabsrc)->nexttab;
				(*tabsrc)->nexttab = *tabdst;
				*tabdst = *tabsrc;
				*tabsrc = next;

				TabCtrl_RedoTabs(tabctrl);
				TabCtrl_ActivateTabByName(tabctrl, name);
				Box_Repaint(tabctrl->parent);
				return;
			}

			tabdst = &((*tabdst)->nexttab);
		}
	}

	/* We've fallen out, move the tab to the end */
	tabdst = &(ctrldata->firsttab);

	while (*tabdst)
	{
		tabdst = &((*tabdst)->nexttab);
	}

	{
		struct tabdata_s *next;

		if (tabsrc == tabdst || (*tabsrc)->nexttab == *tabdst)
		{
			return;
		}

		next = (*tabsrc)->nexttab;
		(*tabsrc)->nexttab = *tabdst;
		*tabdst = *tabsrc;
		*tabsrc = next;

		TabCtrl_RedoTabs(tabctrl);
		TabCtrl_ActivateTabByName(tabctrl, name);
		Box_Repaint(tabctrl->parent);
		return;
	}

	return;
}

struct Box_s *TabCtrl_GetContentBox(struct Box_s *pbox, char *name)
{
	struct tabctrldata_s *ctrldata = pbox->boxdata;
	struct tabdata_s *tabdata = ctrldata->firsttab;

	while (tabdata)
	{
		if (strcmp(name, tabdata->name) == 0)
		{
			return tabdata->box_content;
		}
		tabdata = tabdata->nexttab;
	}

	return NULL;
}

int TabCtrl_GetNumTabs(struct Box_s *tabctrl)
{
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;

	return ctrldata->numtabs;
}

int TabCtrl_GetTabsWidth(struct Box_s *tabctrl)
{
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;
	struct tabdata_s *tab = ctrldata->firsttab;
	int w = 0;

	if (ctrldata->tabsbox->realw > tabctrl->w)
	{
		return tabctrl->w;
	}

	return ctrldata->tabsbox->realw;
}

void TabCtrl_SetOnSpecialDrag(struct Box_s *tabctrl, void (*onspecialdrag)(struct Box_s *tab, int xmouse, int ymouse, char *name))
{
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;

	ctrldata->onspecialdrag = onspecialdrag;
}

void TabCtrl_SetDragImg(struct Box_s *tabctrl)
{
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;
	struct tabdata_s *tab = ctrldata->firsttab;

	while (tab)
	{
		struct Box_s *pbox = tab->box_tab;
		struct Box_s *left = pbox->child;
		struct Box_s *center = left->sibling;
		struct Box_s *right = center->sibling;
		struct Box_s *text = right->sibling;

		left->img   = ImageMgr_GetImage("TabDragL.png");
		center->img = ImageMgr_GetImage("TabDragC.png");
		right->img  = ImageMgr_GetImage("TabDragR.png");
		text->fgcol = RGB(255, 255, 255);
		left->flags |= BOX_TRANSPARENT;
		right->flags |= BOX_TRANSPARENT;

		tab = tab->nexttab;
	}
}

void TabCtrl_UnsetDragImg(struct Box_s *tabctrl)
{
	struct tabctrldata_s *ctrldata = tabctrl->boxdata;
	struct tabdata_s *tab = ctrldata->firsttab;

	while (tab)
	{
		struct Box_s *pbox = tab->box_tab;
		struct Box_s *left = pbox->child;
		struct Box_s *center = left->sibling;
		struct Box_s *right = center->sibling;
		struct Box_s *text = right->sibling;

		left->flags &= ~BOX_TRANSPARENT;
		right->flags &= ~BOX_TRANSPARENT;

		TabBox_SetActiveImg(tab->box_tab, tab == ctrldata->activetab, tab->notify);
		tab = tab->nexttab;
	}
}