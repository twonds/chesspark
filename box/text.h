#ifndef __TEXT_H__
#define __TEXT_H__

enum Text_flags
{
	TX_WRAP	         = 0x0001,
	TX_ELLIPSIS      = 0x0002,
	TX_CENTERED      = 0x0004,
	TX_RIGHT         = 0x0008,
	TX_STRETCHPARENT = 0x0010,
	TX_STRETCHHORIZ	 = 0x0020,
	TX_STRETCHVERT   = 0x0040,
	TX_SELECTABLE	 = 0x0080,
	TX_COPYMENU      = 0x0100,
	TX_NOCOMMANDS	 = 0x0200,
	TX_PASSWORD      = 0x0400,
	TX_FOCUS         = 0x0800,
	TX_PARENTHOVER   = 0x1000,
	TX_CENTERVERT    = 0x2000,
};

struct Box_s *Text_CreateReal(int x, int y, int w, int h, enum Box_flags flags, enum Text_flags tflags);
#if 1
#define Text_Create(x, y, w, h, f, g) Text_CreateReal(x, y, w, h, f, g)
#else
#define Text_Create(x, y, w, h, f, g) Text_CreateWrap(x, y, w, h, f, g, __FILE__, __LINE__)
#endif

void Text_SetText(struct Box_s *pbox, char *text);

void Text_OnSizeWidth_Stretch(struct Box_s *pbox, int dwidth);
void Text_SetLinkCallback(struct Box_s *pbox, int linknum, void (*OnClick)(struct Box_s *, void *), void *userdata);
void Text_SetLinkCallback2(struct Box_s *pbox, int linknum, void (*OnLClick)(struct Box_s *, void *), void (*OnRClick)(struct Box_s *, void *, int x, int y), void *userdata);
void Text_SetRLinkCallback(struct Box_s *pbox, int linknum, void (*OnRClick)(struct Box_s *, void *, int x, int y), void *userdata);
void Text_SetCursorToXYPos(struct Box_s *pbox, int x, int y);
void Text_SetRedirectFocusOnKey(struct Box_s *pbox, struct Box_s *focus);

void Text_GetCursorXY(struct Box_s *pbox, int *x, int *y);
void Text_OnCopy(struct Box_s *pbox);
void Text_DeleteSelection(struct Box_s *pbox);
void Text_InsertWTextAtCursor(struct Box_s *pbox, WCHAR *text);
void Text_SelectAll(struct Box_s *pbox);
void Text_BackspaceKey(struct Box_s *pbox);
void Text_DeleteKey(struct Box_s *pbox);
void Text_EndKey(struct Box_s *pbox, int shift);
void Text_HomeKey(struct Box_s *pbox, int shift);
void Text_CursorDown(struct Box_s *pbox, int shift);
void Text_CursorUp(struct Box_s *pbox, int shift);
void Text_CursorRight(struct Box_s *pbox, int shift, int ctrl);
void Text_CursorLeft(struct Box_s *pbox, int shift, int ctrl);

void Text_SetShowCursor(struct Box_s *pbox, int show);
void Text_SetAltWText(struct Box_s *pbox, WCHAR *wtext);
void Text_SetCursorPos(struct Box_s *pbox, int cursorpos, int selectpos);
void Text_SetWText(struct Box_s *pbox, WCHAR *wtext);
char *Text_GetText(struct Box_s *pbox);

void Text_SetLinkedSelectPrev(struct Box_s *text, struct Box_s *prev);
void Text_SetLinkedSelectNext(struct Box_s *text, struct Box_s *next);

void Text_SetParentList(struct Box_s *text, struct Box_s *list);

void Text_SetLinkTooltip(struct Box_s *pbox, int linknum, char *text);
int Text_TextSelected(struct Box_s *text);

#endif