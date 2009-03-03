#ifndef __BUTTON2_H__
#define __BUTTON2_H__
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

void Button2_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse);
void Button2_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse);
void Button2_SetOnButtonHit(struct Box_s *pbox, void (*pfunc)(struct Box_s *));
void Button2_SetOnButtonHit2(struct Box_s *pbox, void (*pfunc)(struct Box_s *, void *), void *userdata);

void Button2_SetNormal(struct Box_s *button2, struct Box_s *pbox);
void Button2_SetHover(struct Box_s *button2, struct Box_s *pbox);
void Button2_SetPressed(struct Box_s *button2, struct Box_s *pbox);
void Button2_SetDisabled(struct Box_s *button2, struct Box_s *pbox);

struct Box_s *Button2_Create(int x, int y, int w, int h, enum Box_flags flags);
void Button2_SetTooltipText(struct Box_s *button, char *txt);
void Button2_SetDisabledState(struct Box_s *button2, int state);
void Button2_SetOnButtonHit(struct Box_s *pbox, void (*pfunc)(struct Box_s *));

#endif