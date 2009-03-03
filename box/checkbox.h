#ifndef __CHECKBOX_H__
#define __CHECKBOX_H__

struct Box_s *CheckBox_Create(int x, int y, enum Box_flags flags);
struct Box_s *CheckBoxLinkedText_Create(int x, int y, int w, int h, enum Box_flags flags, struct Box_s *checkbox);
void CheckBox_SetOnHit(struct Box_s *checkbox, void (*pfunc)(struct Box_s *, int));
void CheckBox_SetOnHit2(struct Box_s *checkbox, void (*pfunc)(struct Box_s *, int, void *), void *userdata);
void CheckBox_SetChecked(struct Box_s *checkbox, int checked);
int CheckBox_GetChecked(struct Box_s *checkbox);
void CheckBox_SetEnabled(struct Box_s *checkbox, int enabled);

#endif