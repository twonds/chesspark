#ifndef __TABS_H__
#define __TABS_H__

struct Box_s *TabCtrl_Create(int x, int y, int w, int h, enum Box_flags flags, int draggable, int centered, int candrop);
void TabCtrl_AddTab(struct Box_s *pbox, char *name, struct Box_s *target, int tabwidth);
void TabCtrl_AddTab2(struct Box_s *pbox, char *name, char *showname,
	struct Box_s *target, int tabwidth, struct BoxImage_s *icon,
	void *onclose, int dragid, void *dragdata, void (*ondropempty)(
		struct Box_s *psrc, int xmouse, int ymouse, int id,
		void *data));
void TabCtrl_RemoveTab(struct Box_s *pbox, char *name);
void TabCtrl_HideAll(struct Box_s *pbox);

void TabCtrl_ActivateFirst(struct Box_s *pbox);
char *TabCtrl_GetFirstTab(struct Box_s *pbox);
char *TabCtrl_GetActiveTab(struct Box_s *pbox);
void TabCtrl_ActivateTabByName(struct Box_s *pbox, char *tabname);
void TabCtrl_ActivateNextTab(struct Box_s *pbox);
void TabCtrl_ActivatePrevTab(struct Box_s *pbox);

void TabCtrl_SetTabIcon(struct Box_s *pbox, char *name, struct BoxImage_s *img);

void TabCtrl_SetTabDropFunc(struct Box_s *pbox, void (*OnTabDrop)(struct Box_s *, char *, int , int));

void TabCtrl_SetTabActivateFunc(struct Box_s *pbox, void (*OnTabActivate)(struct Box_s *, char *));
struct Box_s *TabCtrl_GetContentBox(struct Box_s *pbox, char *name);
void TabCtrl_UnhideAll(struct Box_s *pbox);
void TabCtrl_HandleTabDrop(struct Box_s *tabctrl, char *name, int x, int y);

int TabCtrl_GetNumTabs(struct Box_s *tabctrl);
void TabCtrl_HideDropIndicator(struct Box_s *tabctrl);
int TabCtrl_GetTabsWidth(struct Box_s *tabctrl);

void TabCtrl_SetDragImg(struct Box_s *tabctrl);
void TabCtrl_UnsetDragImg(struct Box_s *tabctrl);
void TabCtrl_ShowDropIndicator(struct Box_s *tabctrl, int x, int y);
int TabCtrl_GetDropPos(struct Box_s *tabctrl, int x, int y);
void TabCtrl_MoveTab(struct Box_s *tabctrl, char *name, int pos);

void TabCtrl_ActivateTabAndScrollVisible(struct Box_s *tabctrl, char *tabname);
void TabDrag_OnSpecialDrag(struct Box_s *dragtab, int xmouse, int ymouse);
void TabCtrl_SetTabNotify(struct Box_s *pbox, char *name);

void TabCtrl_SetOnSpecialDrag(struct Box_s *tabctrl, void (*onspecialdrag)(struct Box_s *tab, int xmouse, int ymouse, char *name));

#endif