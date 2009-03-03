#ifndef __CORNERPOP_H__
#define __CORNERPOP_H__

struct Box_s *CornerPop_Create(struct Box_s *parent, int w, char *titletext,
  char *dialogtext);

void CornerPop_SetLinkCallback(struct Box_s *dialog, int linknum, void (*OnClick)(struct Box_s *, void *), void *userdata);

void CornerPop_SetLength(int len);
void CornerPop_SetLocation(char *loc);

#endif