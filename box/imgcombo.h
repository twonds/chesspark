#ifndef __IMGCOMBO_H__
#define __IMGCOMBO_H__

void ImgCombo_AddEntry(struct Box_s *combo, char *name, struct Box_s *normal, struct Box_s *selected);
void ImgCombo_RemoveAllEntries(struct Box_s *combo);
void ImgCombo_RemoveEntry(struct Box_s *combo, char *name);
void ImgCombo_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse);
void ImgCombo_SetSelection(struct Box_s *combo, char *name);
char *ImgCombo_GetSelectionName(struct Box_s *combo);
struct Box_s *ImgCombo_Create(int x, int y, int w, int h, enum Box_flags flags);
void ImgCombo_SetOnSelection(struct Box_s *combo, void (*onSelection)(struct Box_s *, char *));

#endif