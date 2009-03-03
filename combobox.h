#ifndef __COMBOBOX_H__
#define __COMBOBOX_H__

void ComboBox_AddEntry(struct Box_s *combo, char *name);
void ComboBox_AddEntry2(struct Box_s *combo, char *name, char *showtext);
void ComboBox_RemoveAllEntries(struct Box_s *combo);
void ComboBox_RemoveEntry(struct Box_s *combo, char *name);
void ComboBox_SetSelection(struct Box_s *combo, char *name);
char *ComboBox_GetSelectionName(struct Box_s *combo);
struct Box_s *ComboBox_Create(int x, int y, int w, int h, enum Box_flags flags);
void ComboBox_SetOnSelection(struct Box_s *combo, void (*onSelection)(struct Box_s *, char *));

struct Box_s *ComboEditBox_Create(int x, int y, int w, int h, enum Box_flags flags);
void ComboEditBox_SetNextFocus(struct Box_s *combo, struct Box_s *nextfocus);
void ComboEditBox_SetOnEnter(struct Box_s *combo, void *pfunc);
void ComboEditBox_SetOnLoseFocus(struct Box_s *combo, void *pfunc);
void ComboBox_SetDisabled(struct Box_s *combo, int disabled);

#endif