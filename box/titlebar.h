#ifndef __TITLEBAR_H__
#define __TITLEBAR_H__

struct Box_s *TitleBarOnly_Add(struct Box_s *dialog, char *titletext);
struct Box_s *TitleBarCloseOnly_Add(struct Box_s *dialog, char *titletext, void *onClose);
struct Box_s *TitleBar_Add(struct Box_s *dialog, char *titletext, void *onClose, void *onMinimize, void *onMaximize);
struct Box_s *TitleBar_Add2(struct Box_s *dialog, char *titletext, void *onClose, void *onMinimize, void *onMaximize, int icon);
struct Box_s *TitleBarIcon_Add(struct Box_s *dialog, char *titletext, void *onClose, void *onMinimize, void *onMaximize);

void TitleBar_SetText(struct Box_s *title, char *titletext);

void TitleBarRoot_OnActive(struct Box_s *pbox);
void TitleBarRoot_OnInactive(struct Box_s *pbox);

void TitleBar_SetIcon(struct Box_s *title, struct BoxImage_s *icon);
void TitleBar_SetActive(struct Box_s *title);

int TitleBar_IsDragging(struct Box_s *title);
void TitleBar_ForceUndrag(struct Box_s *pbox);

void TitleBar_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse);
void TitleBar_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse);

int TitleBar_SetDragging(struct Box_s *title, int x, int y);
void TitleBar_OnLoseMouseCapture(struct Box_s *title);

#endif