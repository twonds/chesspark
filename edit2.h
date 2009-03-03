#ifndef __EDIT2_H__
#define __EDIT2_H__

enum Edit2_flags
{
	E2_HORIZ    = 0x0001,
	E2_PASSWORD = 0x0002,
	E2_NOFOCUS  = 0x0004,
};

struct Box_s *Edit2Box_Create(int x, int y, int w, int h, enum Box_flags flags, enum Edit2_flags editflags);
void Edit2Box_SetOnEnter(struct Box_s *pbox, void *pfunc);
void Edit2Box_SetOnKey(struct Box_s *pbox, void *pfunc);
void Edit2Box_ClearText(struct Box_s *pbox);
char *Edit2Box_GetText(struct Box_s *pbox);
void Edit2Box_SetText(struct Box_s *pbox, char *text);
void Edit2Box_SetAltText(struct Box_s *pbox, char *text);
void Edit2Box_SetNextFocus(struct Box_s *pbox, struct Box_s *next);

struct Box_s *Edit2PassBox_Create(int x, int y, int w, int h, enum Box_flags flags);
void Edit2Box_SetDisabled(struct Box_s *pbox, int disabled);

void Edit2Box_SetEditSizeFunc(struct Box_s *pbox, void (*editsizefunc)(struct Box_s *pbox, int edith));
void Edit2Box_SetTextCol(struct Box_s *pbox, int col);
void Edit2Box_SetAltWText(struct Box_s *pbox, WCHAR *wtext);
void Edit2Box_OnKeyDown(struct Box_s *pbox, int vk, int scan);
void Edit2Box_OnGetFocus(struct Box_s *pbox);
void Edit2Box_OnLoseFocus(struct Box_s *pbox);
void Edit2Box_ScrollToStart(struct Box_s *pbox);

#endif