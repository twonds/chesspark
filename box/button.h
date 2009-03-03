#ifndef __BUTTON_H__
#define __BUTTON_H__
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

void Button_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse);
void Button_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse);
void Button_SetOnButtonHit(struct Box_s *pbox, void (*pfunc)(struct Box_s *));
void Button_SetOnButtonHit2(struct Box_s *pbox, void (*pfunc)(struct Box_s *, void *), void *userdata);

void Button_SetNormalImg(struct Box_s *pbox, struct BoxImage_s *img);
void Button_SetHoverImg(struct Box_s *pbox, struct BoxImage_s *img);
void Button_SetPressedImg(struct Box_s *pbox, struct BoxImage_s *img);
void Button_SetDisabledImg(struct Box_s *pbox, struct BoxImage_s *img);

void Button_SetNormalBG(struct Box_s *pbox, COLORREF bgcol);
void Button_SetHoverBG(struct Box_s *pbox, COLORREF bgcol);
void Button_SetPressedBG(struct Box_s *pbox, COLORREF bgcol);

void Button_SetNormalFG(struct Box_s *pbox, COLORREF fgcol);
void Button_SetHoverFG(struct Box_s *pbox, COLORREF fgcol);
void Button_SetPressedFG(struct Box_s *pbox, COLORREF fgcol);

struct Box_s *Button_Create(int x, int y, int w, int h, enum Box_flags flags);
void Button_SetTooltipText(struct Box_s *button, char *txt);

void Button_SetNormalState(struct Box_s *pbox);
void Button_SetHoverState(struct Box_s *pbox);
void Button_SetDisabledState(struct Box_s *button, int state);
void Button_Trigger(struct Box_s *pbox);

void Button_OnDestroy(struct Box_s *pbox);
void Button_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse);

void Button_SetTooltipText(struct Box_s *button, char *txt);
struct Box_s *ButtonLinkedText_Create(int x, int y, int w, int h, enum Box_flags flags, struct Box_s *button);

#endif