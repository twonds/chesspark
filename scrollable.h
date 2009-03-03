#ifndef __SCROLLABLE_H__
#define __SCROLLABLE_H__

struct Box_s *Scrollable_Create(int x, int y, int w, int h, enum Box_flags flags);

void Scrollable_Refresh(struct Box_s *pbox);

void Scrollable_ScrollToTop(struct Box_s *pbox);
void Scrollable_ScrollToBottom(struct Box_s *pbox);
int Scrollable_GetScroll(struct Box_s *pbox);
int Scrollable_GetHScroll(struct Box_s *pbox);
void Scrollable_SetScroll(struct Box_s *pbox, int yoffset);
void Scrollable_SetHScroll(struct Box_s *pbox, int xoffset);
void Scrollable_ScrollVisible(struct Box_s *pbox);
void Scrollable_SetBox(struct Box_s *pbox, struct Box_s *entry);

#endif