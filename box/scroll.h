#ifndef __SCROLL_H__
#define __SCROLL_H__

struct Box_s *VScrollBox_Create(int x, int y, int w, int h, enum Box_flags flags, struct Box_s *target);
void VScrollBox_UpdateTarget(struct Box_s *pbox);
void VScrollBox_UpdateThumb(struct Box_s *pbox);

void VScrollBox_SetUpButton(struct Box_s *pbox, struct BoxImage_s *normal, struct BoxImage_s *pressed);
void VScrollBox_SetDownButton(struct Box_s *pbox, struct BoxImage_s *normal, struct BoxImage_s *pressed);
void VScrollBox_SetThumbImages(struct Box_s *pbox, struct BoxImage_s *top, struct BoxImage_s *center, struct BoxImage_s *bottom, struct BoxImage_s *centertex);
void VScrollBox_SetTrackImages(struct Box_s *pbox, struct BoxImage_s *top, struct BoxImage_s *center, struct BoxImage_s *bottom);

void VScrollBox_OnScrollWheel(struct Box_s *pbox, float delta);

#endif