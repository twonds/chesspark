#ifndef __LINK_H__
#define __LINK_H__

struct Box_s *LinkBox_Create(int x, int y, int w, int h, enum Box_flags flags);
void LinkBox_SetClickFunc(struct Box_s *link, void (*OnClick)(struct Box_s *, void *), void *userdata);

#endif