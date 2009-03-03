#ifndef __EDIT_H__
#define __EDIT_H__

struct Box_s *EditBox_Create(int x, int y, int w, int h, enum Box_flags flags);
void EditBox_SetOnEnter(struct Box_s *pbox, void *pfunc);
void EditBox_SetOnKey(struct Box_s *pbox, void *pfunc);
void EditBox_ClearText(struct Box_s *pbox);
char *EditBox_GetText(struct Box_s *pbox);
void EditBox_SetText(struct Box_s *pbox, char *text);
void EditBox_SetNextFocus(struct Box_s *pbox, struct Box_s *next);

struct Box_s *EditPassBox_Create(int x, int y, int w, int h, enum Box_flags flags);
void EditBox_SetDisabled(struct Box_s *pbox, int disabled);
#endif