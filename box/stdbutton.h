#ifndef __STDBUTTON_H__
#define __STDBUTTON_H__

struct Box_s *StdButton_Create(int x, int y, int w, char *buttontext, int lightbg);
void StdButton_SetText(struct Box_s *button, char *text);
void StdButton_SetTextAndResize(struct Box_s *button, char *text, int minw);

#endif